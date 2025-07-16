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

#include <schematic.h>
#include <sch_sheet.h>
#include <sch_view.h>
#include <sch_screen.h>
#include <gal/gal_display_options.h>
#include <gal/graphics_abstraction_layer.h>
#include <math/vector2wx.h>
#include <design_block.h>
#include <sch_preview_panel.h>
#include <pgm_base.h>
#include <sch_painter.h>
#include <eda_draw_frame.h>
#include <project_sch.h>
#include <eeschema_settings.h>
#include <eeschema_helpers.h>
#include <settings/settings_manager.h>
#include <widgets/sch_design_block_preview_widget.h>
#include <wx/log.h>
#include <wx/stattext.h>
#include <wx/panel.h>


SCH_DESIGN_BLOCK_PREVIEW_WIDGET::SCH_DESIGN_BLOCK_PREVIEW_WIDGET( wxWindow* aParent,
                                                                 EDA_DRAW_PANEL_GAL::GAL_TYPE aCanvasType,
                                                                 bool aIncludeStatus ) :
        DESIGN_BLOCK_PREVIEW_WIDGET( aParent ),
        m_preview( nullptr ),
        m_status( nullptr ),
        m_statusPanel( nullptr ),
        m_statusSizer( nullptr ),
        m_previewItem( nullptr )
{
    COMMON_SETTINGS*   common_settings = Pgm().GetCommonSettings();
    EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );

    m_galDisplayOptions.ReadConfig( *common_settings, cfg->m_Window, this );
    m_galDisplayOptions.m_forceDisplayCursor = false;

    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = aCanvasType;

    // Allows only a CAIRO or OPENGL canvas:
    if( canvasType != EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL && canvasType != EDA_DRAW_PANEL_GAL::GAL_FALLBACK )
    {
        canvasType = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    }

    m_preview = new SCH_PREVIEW_PANEL( this, wxID_ANY, wxDefaultPosition, wxSize( -1, -1 ), m_galDisplayOptions,
                                       canvasType );
    m_preview->SetStealsFocus( false );
    m_preview->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );
    m_preview->GetGAL()->SetAxesEnabled( false );

    // Do not display the grid: the look is not good for a small canvas area.
    // But mainly, due to some strange bug I (JPC) was unable to fix, the grid creates
    // strange artifacts on Windows when Eeschema is run from KiCad manager (but not in
    // stand alone...).
    m_preview->GetGAL()->SetGridVisibility( true );

    // Early initialization of the canvas background color,
    // before any OnPaint event is fired for the canvas using a wrong bg color
    KIGFX::VIEW* view = m_preview->GetView();
    auto         settings = static_cast<SCH_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    if( COLOR_SETTINGS* cs = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME ) )
        settings->LoadColors( cs );

    const COLOR4D& backgroundColor = settings->GetBackgroundColor();
    const COLOR4D& foregroundColor = settings->GetCursorColor();

    m_preview->GetGAL()->SetClearColor( backgroundColor );

    settings->m_ShowPinsElectricalType = false;
    settings->m_ShowPinNumbers = false;
    settings->m_ShowHiddenPins = false;
    settings->m_ShowHiddenFields = false;
    settings->m_ShowPinAltIcons = false;

    m_outerSizer = new wxBoxSizer( wxVERTICAL );

    if( aIncludeStatus )
    {
        m_statusPanel = new wxPanel( this );
        m_statusPanel->SetBackgroundColour( backgroundColor.ToColour() );
        m_status = new wxStaticText( m_statusPanel, wxID_ANY, wxEmptyString );
        m_status->SetForegroundColour( settings->GetLayerColor( LAYER_REFERENCEPART ).ToColour() );
        m_statusSizer = new wxBoxSizer( wxVERTICAL );
        m_statusSizer->Add( 0, 0, 1 ); // add a spacer
        m_statusSizer->Add( m_status, 0, wxALIGN_CENTER );
        m_statusSizer->Add( 0, 0, 1 ); // add a spacer
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

    Connect( wxEVT_SIZE, wxSizeEventHandler( SCH_DESIGN_BLOCK_PREVIEW_WIDGET::onSize ), nullptr, this );
}


SCH_DESIGN_BLOCK_PREVIEW_WIDGET::~SCH_DESIGN_BLOCK_PREVIEW_WIDGET()
{
    if( m_previewItem )
        m_preview->GetView()->Remove( m_previewItem );

    delete m_previewItem;
}


void SCH_DESIGN_BLOCK_PREVIEW_WIDGET::SetStatusText( wxString const& aText )
{
    wxCHECK( m_statusPanel, /* void */ );

    m_status->SetLabel( aText );
    m_preview->Hide();
    m_statusPanel->Show();
    Layout();
}


void SCH_DESIGN_BLOCK_PREVIEW_WIDGET::onSize( wxSizeEvent& aEvent )
{
    if( m_previewItem )
    {
        fitOnDrawArea();
        m_preview->ForceRefresh();
    }

    aEvent.Skip();
}


void SCH_DESIGN_BLOCK_PREVIEW_WIDGET::fitOnDrawArea()
{
    if( !m_previewItem )
        return;

    // set the view scale to fit the item on screen
    KIGFX::VIEW* view = m_preview->GetView();

    // Calculate the drawing area size, in internal units, for a scaling factor = 1.0
    view->SetScale( 1.0 );
    VECTOR2D clientSize = view->ToWorld( ToVECTOR2D( m_preview->GetClientSize() ), false );
    // Calculate the draw scale to fit the drawing area
    double scale =
            std::min( fabs( clientSize.x / m_itemBBox.GetWidth() ), fabs( clientSize.y / m_itemBBox.GetHeight() ) );

    // Above calculation will yield an exact fit; add a bit of whitespace around block
    scale /= 1.2;

    // Now fix the best scale
    view->SetScale( scale );
    view->SetCenter( m_itemBBox.Centre() );
}


void SCH_DESIGN_BLOCK_PREVIEW_WIDGET::DisplayDesignBlock( DESIGN_BLOCK* aDesignBlock )
{
    KIGFX::VIEW* view = m_preview->GetView();

    if( m_previewItem )
    {
        view->Clear();
        delete m_previewItem;
        m_previewItem = nullptr;
    }

    if( aDesignBlock && wxFileExists( aDesignBlock->GetSchematicFile() ) )
    {
        m_previewItem = EESCHEMA_HELPERS::LoadSchematic( aDesignBlock->GetSchematicFile(), SCH_IO_MGR::SCH_KICAD,
                                                         false, true, nullptr, false );
        BOX2I bBox;

        if( m_previewItem )
        {
            for( EDA_ITEM* item : m_previewItem->CurrentSheet().LastScreen()->Items() )
            {
                view->Add( item );

                if( item->Type() == SCH_FIELD_T )
                {
                    if( !static_cast<const SCH_FIELD*>( item )->IsVisible() )
                        continue;
                }

                bBox.Merge( item->GetBoundingBox() );
            }
        }

        m_itemBBox = bBox;

        if( !m_preview->IsShownOnScreen() )
        {
            m_preview->Show();

            if( m_statusPanel )
                m_statusPanel->Hide();

            Layout(); // Ensure panel size is up to date.
        }

        // Calculate the draw scale to fit the drawing area
        fitOnDrawArea();
    }

    m_preview->ForceRefresh();
    m_preview->Show();
}
