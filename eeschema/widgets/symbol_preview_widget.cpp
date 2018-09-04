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

SYMBOL_PREVIEW_WIDGET::SYMBOL_PREVIEW_WIDGET( wxWindow* aParent, KIWAY& aKiway ) :
    wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 ),
    m_kiway( aKiway ),
    m_preview( nullptr ),
    m_status( nullptr ),
    m_statusSizer( nullptr ),
    m_previewItem( nullptr )
{
    wxConfigBase* eeschemaConfig = GetNewConfig( wxString::FromUTF8( "eeschema" ) );
    m_galDisplayOptions.ReadConfig( eeschemaConfig, GAL_DISPLAY_OPTIONS_KEY );

    m_preview = new SCH_PREVIEW_PANEL( aParent, wxID_ANY, wxDefaultPosition, wxSize( -1, -1 ),
                                       m_galDisplayOptions, EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );
    m_preview->SetStealsFocus( false );

    SetBackgroundColour( *wxWHITE );
    SetForegroundColour( *wxBLACK );

    m_status = new wxStaticText( this, -1, wxEmptyString );
    m_statusSizer = new wxBoxSizer( wxVERTICAL );
    m_statusSizer->Add( 0, 0, 1 );
    m_statusSizer->Add( m_status, 0, wxALL | wxALIGN_CENTER, 0 );
    m_statusSizer->Add( 0, 0, 1 );

    auto outer_sizer = new wxBoxSizer( wxVERTICAL );
    outer_sizer->Add( m_preview, 1, wxALL | wxEXPAND, 0 );
    outer_sizer->Add( m_statusSizer, 1, wxALL | wxALIGN_CENTER, 0 );

    m_statusSizer->ShowItems( false );

    SetSizer( outer_sizer );
}


SYMBOL_PREVIEW_WIDGET::~SYMBOL_PREVIEW_WIDGET()
{
    if( m_previewItem )
        m_preview->GetView()->Remove( m_previewItem );
}


void SYMBOL_PREVIEW_WIDGET::SetStatusText( wxString const& aText )
{
    m_status->SetLabel( aText );
    m_statusSizer->ShowItems( true );
    m_preview->Hide();
    Layout();
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

        view->Add( alias );
        m_previewItem = alias;

        // Zoom to fit
        BOX2I     bBox = alias->GetPart()->GetUnitBoundingBox( aUnit, 0 );
        VECTOR2D  clientSize = m_preview->GetClientSize();
        double    scale = std::min( fabs( clientSize.x / bBox.GetWidth() ),
                                    fabs( clientSize.y / bBox.GetHeight() ) );

        // Above calculation will yield an exact fit; add a bit of whitespace around symbol
        scale /= 1.2;

        view->SetScale( scale );
        view->SetCenter( bBox.Centre() );
    }

    m_preview->ForceRefresh();

    m_preview->Show();
    m_statusSizer->ShowItems( false );
}


void SYMBOL_PREVIEW_WIDGET::DisplayPart( LIB_PART* aPart, int aUnit )
{
    KIGFX::VIEW* view = m_preview->GetView();

    if( m_previewItem )
    {
        view->Remove( m_previewItem );
        m_previewItem = nullptr;
    }

    if( aPart )
    {
        // If unit isn't specified for a multi-unit part, pick the first.  (Otherwise we'll
        // draw all of them.)
        if( aPart->IsMulti() && aUnit == 0 )
            aUnit = 1;

        view->Add( aPart );
        m_previewItem = aPart;

        // Zoom to fit
        BOX2I     bBox = aPart->GetUnitBoundingBox( aUnit, 0 );
        VECTOR2D  clientSize = m_preview->GetClientSize();
        double    scale = std::min( fabs( clientSize.x / bBox.GetWidth() ),
                                    fabs( clientSize.y / bBox.GetHeight() ) );

        // Above calculation will yield an exact fit; add a bit of whitespace around symbol
        scale /= 1.2;

        view->SetScale( scale );
        view->SetCenter( bBox.Centre() );
    }

    m_preview->ForceRefresh();

    m_preview->Show();
    m_statusSizer->ShowItems( false );
}


