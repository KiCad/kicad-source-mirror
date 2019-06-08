/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "symbol_preview_widget.h"
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <kiway.h>
#include <sch_view.h>
#include <gal/gal_display_options.h>
#include <class_libentry.h>
#include <symbol_lib_table.h>
#include <sch_preview_panel.h>
#include <pgm_base.h>
#include <sch_painter.h>
#include <eda_draw_frame.h>


SYMBOL_PREVIEW_WIDGET::SYMBOL_PREVIEW_WIDGET( wxWindow* aParent, KIWAY& aKiway,
                                              EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) :
    wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 ),
    m_kiway( aKiway ),
    m_preview( nullptr ), m_status( nullptr ), m_statusSizer( nullptr ), m_previewItem( nullptr )
{
    wxString eeschemaFrameKey( SCH_EDIT_FRAME_NAME );

    std::unique_ptr<wxConfigBase> eeschemaConfig = GetNewConfig( Pgm().App().GetAppName() );
    wxConfigBase&                 commonConfig = *Pgm().CommonSettings();

    m_galDisplayOptions.ReadConfig( commonConfig, *eeschemaConfig, eeschemaFrameKey, this );

    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = aCanvasType;

    // Allows only a CAIRO or OPENGL canvas:
    if( canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL &&
        canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO )
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;

    m_preview = new SCH_PREVIEW_PANEL( aParent, wxID_ANY, wxDefaultPosition, wxSize( -1, -1 ),
                                       m_galDisplayOptions, canvasType );
    m_preview->SetStealsFocus( false );

    // Do not display the grid: the look is not good for a small canvas area.
    // But mainly, due to some strange bug I (JPC) was unable to fix, the grid creates
    // strange artifacts on Windows when eeschema is run from Kicad manager (but not in stand alone...).
    m_preview->GetGAL()->SetGridVisibility( false );

    // Early initialization of the canvas background color,
    // before any OnPaint event is fired for the canvas using a wrong bg color
    KIGFX::VIEW* view = m_preview->GetView();
    auto settings = static_cast<KIGFX::SCH_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
    m_preview->GetGAL()->SetClearColor( settings->GetBackgroundColor() );

    SetBackgroundColour( *wxWHITE );
    SetForegroundColour( *wxBLACK );

    m_status = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_statusSizer = new wxBoxSizer( wxVERTICAL );
    m_statusSizer->Add( 0, 0, 1 );  // add a spacer
    m_statusSizer->Add( m_status, 0, wxALIGN_CENTER );
    m_statusSizer->Add( 0, 0, 1 );  // add a spacer

    auto outer_sizer = new wxBoxSizer( wxVERTICAL );
    outer_sizer->Add( m_preview, 1, wxTOP | wxEXPAND, 5 );
    outer_sizer->Add( m_statusSizer, 1, wxALIGN_CENTER );

    m_statusSizer->ShowItems( false );

    SetSizer( outer_sizer );

    Connect( wxEVT_SIZE, wxSizeEventHandler( SYMBOL_PREVIEW_WIDGET::onSize ), NULL, this );
}


SYMBOL_PREVIEW_WIDGET::~SYMBOL_PREVIEW_WIDGET()
{
    if( m_previewItem )
        m_preview->GetView()->Remove( m_previewItem );

    delete m_previewItem;
}


void SYMBOL_PREVIEW_WIDGET::SetStatusText( wxString const& aText )
{
    m_status->SetLabel( aText );
    m_statusSizer->ShowItems( true );
    m_preview->Hide();
    Layout();
}


void SYMBOL_PREVIEW_WIDGET::onSize( wxSizeEvent& aEvent )
{
    if( m_previewItem )
    {
        fitOnDrawArea();
        m_preview->ForceRefresh();
    }

    aEvent.Skip();
}


void SYMBOL_PREVIEW_WIDGET::fitOnDrawArea()
{
    if( !m_previewItem )
        return;

    // set the view scale to fit the item on screen
    KIGFX::VIEW* view = m_preview->GetView();

    // Calculate the drawing area size, in internal units, for a scaling factor = 1.0
    view->SetScale( 1.0 );
    VECTOR2D  clientSize = view->ToWorld( m_preview->GetClientSize(), false );
    // Calculate the draw scale to fit the drawing area
    double    scale = std::min( fabs( clientSize.x / m_itemBBox.GetWidth() ),
                                fabs( clientSize.y / m_itemBBox.GetHeight() ) );

    // Above calculation will yield an exact fit; add a bit of whitespace around symbol
    scale /= 1.2;

    // Now fix the best scale
    view->SetScale( scale );
    view->SetCenter( m_itemBBox.Centre() );
}


void SYMBOL_PREVIEW_WIDGET::DisplaySymbol( const LIB_ID& aSymbolID, int aUnit )
{
    KIGFX::VIEW* view = m_preview->GetView();
    auto settings = static_cast<KIGFX::SCH_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
    LIB_ALIAS* alias = nullptr;

    try
    {
        alias = m_kiway.Prj().SchSymbolLibTable()->LoadSymbol( aSymbolID );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( wxString::Format( _( "Error loading symbol %s from library %s.\n\n%s" ),
                                      aSymbolID.GetLibItemName().wx_str(),
                                      aSymbolID.GetLibNickname().wx_str(),
                                      ioe.What() ) );
    }

    if( m_previewItem )
    {
        view->Remove( m_previewItem );
        delete m_previewItem;
        m_previewItem = nullptr;
    }

    if( alias )
    {
        LIB_PART* part = alias->GetPart();

        // If unit isn't specified for a multi-unit part, pick the first.  (Otherwise we'll
        // draw all of them.)
        if( part->IsMulti() && aUnit == 0 )
            aUnit = 1;

        settings->m_ShowUnit = aUnit;

        // For symbols having a De Morgan body style, use the first style
        settings->m_ShowConvert = part->HasConversion() ? 1 : 0;

        m_previewItem = new LIB_ALIAS( *alias, part );
        view->Add( m_previewItem );

        // Get the symbole size, in internal units
        m_itemBBox = part->GetUnitBoundingBox( aUnit, 0 );

        if( !m_preview->IsShown() )
        {
            m_preview->Show();
            m_statusSizer->ShowItems( false );
            Layout();   // Ensure panel size is up to date.
        }

        // Calculate the draw scale to fit the drawing area
        fitOnDrawArea();
    }

    m_preview->ForceRefresh();
}


void SYMBOL_PREVIEW_WIDGET::DisplayPart( LIB_PART* aPart, int aUnit )
{
    KIGFX::VIEW* view = m_preview->GetView();

    if( m_previewItem )
    {
        view->Remove( m_previewItem );
        delete m_previewItem;
        m_previewItem = nullptr;
    }

    if( aPart )
    {
        // If unit isn't specified for a multi-unit part, pick the first.  (Otherwise we'll
        // draw all of them.)
        if( aPart->IsMulti() && aUnit == 0 )
            aUnit = 1;

        // For symbols having a De Morgan body style, use the first style
        auto settings = static_cast<KIGFX::SCH_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
        settings->m_ShowConvert = aPart->HasConversion() ? 1 : 0;
        m_previewItem = new LIB_PART( *aPart );
        view->Add( m_previewItem );

        // Get the symbole size, in internal units
        m_itemBBox = aPart->GetUnitBoundingBox( aUnit, 0 );

        // Calculate the draw scale to fit the drawing area
        fitOnDrawArea();
    }

    m_preview->ForceRefresh();

    m_preview->Show();
    m_statusSizer->ShowItems( false );
}
