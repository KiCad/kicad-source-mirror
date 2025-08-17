/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <sch_view.h>
#include <gal/gal_display_options.h>
#include <gal/graphics_abstraction_layer.h>
#include <math/vector2wx.h>
#include <lib_symbol.h>
#include <libraries/symbol_library_adapter.h>
#include <sch_preview_panel.h>
#include <pgm_base.h>
#include <sch_painter.h>
#include <eda_draw_frame.h>
#include <project_sch.h>
#include <eeschema_settings.h>
#include <settings/settings_manager.h>
#include <wx/log.h>
#include <wx/stattext.h>


SYMBOL_PREVIEW_WIDGET::SYMBOL_PREVIEW_WIDGET( wxWindow* aParent, KIWAY* aKiway, bool aIncludeStatus,
                                              EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 ),
        m_kiway( aKiway ),
        m_preview( nullptr ),
        m_status( nullptr ),
        m_statusPanel( nullptr ),
        m_statusSizer( nullptr ),
        m_previewItem( nullptr )
{
    COMMON_SETTINGS*   common_settings = Pgm().GetCommonSettings();
    EESCHEMA_SETTINGS* app_settings = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    m_galDisplayOptions.ReadConfig( *common_settings, app_settings->m_Window, this );
    m_galDisplayOptions.m_forceDisplayCursor = false;

    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = aCanvasType;

    // Allows only a CAIRO or OPENGL canvas:
    if( canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL && canvasType != EDA_DRAW_PANEL_GAL::GAL_FALLBACK )
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;

    m_preview = new SCH_PREVIEW_PANEL( this, wxID_ANY, wxDefaultPosition, wxSize( -1, -1 ),
                                       m_galDisplayOptions, canvasType );
    m_preview->SetStealsFocus( false );
    m_preview->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );
    m_preview->GetGAL()->SetAxesEnabled( false );

    // Do not display the grid: the look is not good for a small canvas area.
    // But mainly, due to some strange bug I (JPC) was unable to fix, the grid creates
    // strange artifacts on Windows when Eeschema is run from KiCad manager (but not in
    // stand alone...).
    m_preview->GetGAL()->SetGridVisibility( false );

    // Early initialization of the canvas background color,
    // before any OnPaint event is fired for the canvas using a wrong bg color
    KIGFX::VIEW* view = m_preview->GetView();
    m_renderSettings = static_cast<SCH_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    if( COLOR_SETTINGS* cs = ::GetColorSettings( app_settings ? app_settings->m_ColorTheme : DEFAULT_THEME ) )
        m_renderSettings->LoadColors( cs );

    const COLOR4D& backgroundColor = m_renderSettings->GetBackgroundColor();
    const COLOR4D& foregroundColor = m_renderSettings->GetCursorColor();

    m_preview->GetGAL()->SetClearColor( backgroundColor );

    m_renderSettings->m_ShowPinsElectricalType = app_settings->m_LibViewPanel.show_pin_electrical_type;
    m_renderSettings->m_ShowPinNumbers = app_settings->m_LibViewPanel.show_pin_numbers;
    m_renderSettings->m_ShowHiddenPins = false;
    m_renderSettings->m_ShowHiddenFields = false;
    m_renderSettings->m_ShowPinAltIcons = false;
    m_renderSettings->m_ShowPinsElectricalType = false;

    m_outerSizer = new wxBoxSizer( wxVERTICAL );

    if( aIncludeStatus )
    {
        m_statusPanel = new wxPanel( this );
        m_statusPanel->SetBackgroundColour( backgroundColor.ToColour() );
        m_status = new wxStaticText( m_statusPanel, wxID_ANY, wxEmptyString );
        m_status->SetForegroundColour( m_renderSettings->GetLayerColor( LAYER_REFERENCEPART ).ToColour() );
        m_statusSizer = new wxBoxSizer( wxVERTICAL );
        m_statusSizer->Add( 0, 0, 1 );  // add a spacer
        m_statusSizer->Add( m_status, 0, wxALIGN_CENTER );
        m_statusSizer->Add( 0, 0, 1 );  // add a spacer
        m_statusPanel->SetSizer( m_statusSizer );

        // Give the status panel the same color scheme as the canvas so it isn't jarring when
        // switched to.
        m_statusPanel->SetBackgroundColour( backgroundColor.ToColour() );
        m_statusPanel->SetForegroundColour( foregroundColor.ToColour() );

        // Give the preview panel a small top border to align its top with the status panel,
        // and give the status panel a small bottom border to align its bottom with the preview
        // panel.
        m_outerSizer->Add( m_preview, 1, wxTOP | wxEXPAND, 5 );
        m_outerSizer->Add( m_statusPanel, 1, wxBOTTOM | wxEXPAND, 5 );

        // Hide the status panel to start
        m_statusPanel->Hide();
    }
    else
    {
        m_outerSizer->Add( m_preview, 1, wxEXPAND, 0 );
    }

    SetSizer( m_outerSizer );
    Layout();

    Connect( wxEVT_SIZE, wxSizeEventHandler( SYMBOL_PREVIEW_WIDGET::onSize ), nullptr, this );
}


SYMBOL_PREVIEW_WIDGET::~SYMBOL_PREVIEW_WIDGET()
{
    if( m_previewItem )
        m_preview->GetView()->Remove( m_previewItem );

    delete m_previewItem;
}


void SYMBOL_PREVIEW_WIDGET::SetStatusText( wxString const& aText )
{
    wxCHECK( m_statusPanel, /* void */ );

    m_status->SetLabel( aText );
    m_preview->Hide();
    m_statusPanel->Show();
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
    VECTOR2D clientSize = view->ToWorld( ToVECTOR2D( m_preview->GetClientSize() ), false );
    // Calculate the draw scale to fit the drawing area
    double    scale = std::min( fabs( clientSize.x / (double) m_itemBBox.GetWidth() ),
                                fabs( clientSize.y / (double) m_itemBBox.GetHeight() ) );

    // Above calculation will yield an exact fit; add a bit of whitespace around symbol
    scale /= 1.2;

    // Now fix the best scale
    view->SetScale( scale );
    view->SetCenter( m_itemBBox.Centre() );
}


void SYMBOL_PREVIEW_WIDGET::DisplaySymbol( const LIB_ID& aSymbolID, int aUnit, int aBodyStyle )
{
    KIGFX::VIEW* view = m_preview->GetView();
    std::unique_ptr< LIB_SYMBOL > symbol;

    try
    {
        LIB_SYMBOL* tmp = PROJECT_SCH::SymbolLibAdapter( &m_kiway->Prj() )->LoadSymbol( aSymbolID );

        if( tmp )
            symbol = tmp->Flatten();
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( _( "Error loading symbol %s from library '%s'." ) + wxS( "\n%s" ),
                    aSymbolID.GetLibItemName().wx_str(),
                    aSymbolID.GetLibNickname().wx_str(),
                    ioe.What() );
    }

    if( m_previewItem )
    {
        view->Remove( m_previewItem );
        delete m_previewItem;
        m_previewItem = nullptr;
    }

    if( symbol )
    {
        // This will flatten derived parts so that the correct final symbol can be shown.
        m_previewItem = symbol.release();

        // If unit isn't specified for a multi-unit part, pick the first.  (Otherwise we'll draw all of them.)
        m_renderSettings->m_ShowUnit = ( m_previewItem->IsMultiUnit() && aUnit == 0 ) ? 1 : aUnit;

        // For symbols having multiple body styles, use the first style.
        m_renderSettings->m_ShowBodyStyle = ( m_previewItem->IsMultiBodyStyle() && aBodyStyle == 0 ) ? 1 : aBodyStyle;

        m_previewItem->SetPreviewUnit( m_renderSettings->m_ShowUnit );
        m_previewItem->SetPreviewBodyStyle( m_renderSettings->m_ShowBodyStyle );

        if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        {
            if( cfg->m_AutoplaceFields.enable )
                m_previewItem->AutoplaceFields( nullptr, AUTOPLACE_AUTO );
        }

        view->Add( m_previewItem );

        // Get the symbol size, in internal units
        m_itemBBox = m_previewItem->GetUnitBoundingBox( m_renderSettings->m_ShowUnit,
                                                        m_renderSettings->m_ShowBodyStyle );

        if( !m_preview->IsShownOnScreen() )
        {
            m_preview->Show();

            if( m_statusPanel )
                m_statusPanel->Hide();

            Layout();   // Ensure panel size is up to date.
        }

        // Calculate the draw scale to fit the drawing area
        fitOnDrawArea();
    }

    m_preview->ForceRefresh();
}


void SYMBOL_PREVIEW_WIDGET::DisplayPart( LIB_SYMBOL* aSymbol, int aUnit, int aBodyStyle )
{
    KIGFX::VIEW* view = m_preview->GetView();

    if( m_previewItem )
    {
        view->Remove( m_previewItem );
        delete m_previewItem;
        m_previewItem = nullptr;
    }

    if( aSymbol )
    {
        m_previewItem = new LIB_SYMBOL( *aSymbol );

        // For symbols having a De Morgan body style, use the first style

        // If unit isn't specified for a multi-unit part, pick the first.  (Otherwise we'll draw all of them.)
        m_renderSettings->m_ShowUnit = ( m_previewItem->IsMultiUnit() && aUnit == 0 ) ? 1 : aUnit;

        m_renderSettings->m_ShowBodyStyle = ( m_previewItem->IsMultiBodyStyle() && aBodyStyle == 0 ) ? 1 : aBodyStyle;

        m_previewItem->SetPreviewUnit( m_renderSettings->m_ShowUnit );
        m_previewItem->SetPreviewBodyStyle( m_renderSettings->m_ShowBodyStyle );

        if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        {
            if( cfg->m_AutoplaceFields.enable )
                m_previewItem->AutoplaceFields( nullptr, AUTOPLACE_AUTO );
        }

        view->Add( m_previewItem );

        // Get the symbol size, in internal units
        m_itemBBox = m_previewItem->GetUnitBoundingBox( m_renderSettings->m_ShowUnit,
                                                        m_renderSettings->m_ShowBodyStyle,
                                                        true, false );

        // Calculate the draw scale to fit the drawing area
        fitOnDrawArea();
    }

    m_preview->ForceRefresh();
    m_preview->Show();

    if( m_statusPanel )
        m_statusPanel->Hide();

    Layout();
}
