/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <bitmaps.h>
#include <gerbview.h>
#include "gerbview_draw_panel_gal.h"
#include <gerbview_frame.h>
#include <gerber_file_image_list.h>
#include <core/arraydim.h>
#include <lset.h>
#include <view/view.h>
#include <gerbview_painter.h>
#include <gal/graphics_abstraction_layer.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tools/gerbview_actions.h>

#include "layer_widget.h"
#include "gbr_layer_box_selector.h"
#include "gerbview_layer_widget.h"
#include "dcode_selection_box.h"


GERBER_LAYER_WIDGET::GERBER_LAYER_WIDGET( GERBVIEW_FRAME* aParent, wxWindow* aFocusOwner ) :
    LAYER_WIDGET( aParent, aFocusOwner ),
    m_frame( aParent )
{
    m_alwaysShowActiveLayer = false;

    ReFillRender();

    // Update default tabs labels for GerbView
    SetLayersManagerTabsText( );

    // handle the popup menu over the layer window.
    m_LayerScrolledWindow->Connect( wxEVT_RIGHT_DOWN,
                                    wxMouseEventHandler( GERBER_LAYER_WIDGET::onRightDownLayers ),
                                    nullptr, this );

    // since Popupmenu() calls this->ProcessEvent() we must call this->Connect()
    // and not m_LayerScrolledWindow->Connect()
    Connect( ID_LAYER_MANAGER_START, ID_LAYER_MANAGER_END, wxEVT_COMMAND_MENU_SELECTED,
             wxCommandEventHandler( GERBER_LAYER_WIDGET::onPopupSelection ), nullptr, this );
}


GERBER_FILE_IMAGE_LIST* GERBER_LAYER_WIDGET::GetImagesList()
{
    return &GERBER_FILE_IMAGE_LIST::GetImagesList();
}


void GERBER_LAYER_WIDGET::SetLayersManagerTabsText()
{
    m_notebook->SetPageText( 0, _( "Layers" ) );
    m_notebook->SetPageText( 1, _( "Items" ) );
}


void GERBER_LAYER_WIDGET::CollectCurrentColorSettings(  COLOR_SETTINGS* aColorSettings )
{
    std::vector<int>render_layers{ LAYER_DCODES, LAYER_NEGATIVE_OBJECTS, LAYER_GERBVIEW_GRID,
                            LAYER_GERBVIEW_DRAWINGSHEET, LAYER_GERBVIEW_PAGE_LIMITS,
                            LAYER_GERBVIEW_BACKGROUND };

    for( int layer: render_layers )
    {
        int row = findRenderRow( layer );

        if( row < 0 )
            continue;

        COLOR4D color = GetRenderColor( row );

        if( color != COLOR4D::UNSPECIFIED )
            aColorSettings->SetColor( layer, color );
    }

    for( int layer = GERBVIEW_LAYER_ID_START; layer < GERBVIEW_LAYER_ID_START + GERBER_DRAWLAYERS_COUNT; layer++ )
    {
        int row = findLayerRow( layer - GERBVIEW_LAYER_ID_START );

        if( row < 0 )   // Not existing in layer list
            continue;

        COLOR4D color = GetLayerColor( row );

        if( color != COLOR4D::UNSPECIFIED )
            aColorSettings->SetColor( layer, color );
    }
}


void GERBER_LAYER_WIDGET::ReFillRender()
{
    ClearRenderRows();

    // Fixed "Rendering" tab rows within the LAYER_WIDGET, only the initial color
    // is changed before appending to the LAYER_WIDGET.  This is an automatic variable
    // not a static variable, change the color & state after copying from code to renderRows
    // on the stack.
    LAYER_WIDGET::ROW renderRows[7] = {

#define RR  LAYER_WIDGET::ROW   // Render Row abbreviation to reduce source width

        RR( _( "DCodes" ),           LAYER_DCODES,                WHITE,
            _( "Show DCodes identification" ) ),
        RR( _( "Negative Objects" ), LAYER_NEGATIVE_OBJECTS,      DARKGRAY,
            _( "Show negative objects in this color" ) ),
        RR(),
        RR( _( "Grid" ),             LAYER_GERBVIEW_GRID,         WHITE,
            _( "Show the (x,y) grid dots" ) ),
        RR( _( "Drawing Sheet" ),    LAYER_GERBVIEW_DRAWINGSHEET, DARKRED,
            _( "Show drawing sheet border and title block") ),
        RR( _( "Page Limits" ),      LAYER_GERBVIEW_PAGE_LIMITS,  WHITE,
            _( "Show drawing sheet page limits" ) ),
        RR( _( "Background" ),       LAYER_GERBVIEW_BACKGROUND,   BLACK,
            _( "PCB Background" ), true, false )
    };

    for( unsigned row = 0; row < arrayDim( renderRows ); ++row )
    {
        if( renderRows[row].color != COLOR4D::UNSPECIFIED )       // does this row show a color?
            renderRows[row].color = m_frame->GetVisibleElementColor( renderRows[row].id );

        if( renderRows[row].id )    // if not the separator
            renderRows[row].state = m_frame->IsElementVisible( renderRows[row].id );
    }

    AppendRenderRows( renderRows, arrayDim(renderRows) );
}


void GERBER_LAYER_WIDGET::AddRightClickMenuItems( wxMenu* aMenu )
{
    // Remember: menu text is capitalized (see our rules_for_capitalization_in_Kicad_UI.txt)
    KIUI::AddMenuItem( aMenu, ID_SHOW_ALL_LAYERS, _( "Show All Layers" ),
                       KiBitmap( BITMAPS::show_all_layers ) );

    KIUI::AddMenuItem( aMenu, ID_SHOW_NO_LAYERS_BUT_ACTIVE, _( "Hide All Layers But Active" ),
                       KiBitmap( BITMAPS::select_w_layer ) );

    KIUI::AddMenuItem( aMenu, ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE,
                       _( "Always Hide All Layers But Active" ),
                       KiBitmap( BITMAPS::select_w_layer ) );

    KIUI::AddMenuItem( aMenu, ID_SHOW_NO_LAYERS, _( "Hide All Layers" ),
                       KiBitmap( BITMAPS::show_no_layers ) );

    aMenu->AppendSeparator();

    KIUI::AddMenuItem( aMenu, ID_SORT_GBR_LAYERS_X2, _( "Sort Layers if X2 Mode" ),
                       KiBitmap( BITMAPS::reload ) );

    KIUI::AddMenuItem( aMenu, ID_SORT_GBR_LAYERS_FILE_EXT, _( "Sort Layers by File Extension" ),
                       KiBitmap( BITMAPS::reload ) );

    aMenu->AppendSeparator();

    KIUI::AddMenuItem( aMenu, ID_SET_GBR_LAYERS_DRAW_PRMS,
                       _( "Layers Display Parameters: Offset and Rotation" ),
                       KiBitmap( BITMAPS::tools ) );

    aMenu->AppendSeparator();

    KIUI::AddMenuItem( aMenu, ID_LAYER_MOVE_UP, _( "Move Current Layer Up" ),
                       KiBitmap( BITMAPS::up ) );

    KIUI::AddMenuItem( aMenu, ID_LAYER_MOVE_DOWN, _( "Move Current Layer Down" ),
                       KiBitmap( BITMAPS::down ) );

    KIUI::AddMenuItem( aMenu, ID_LAYER_DELETE, _( "Clear Current Layer..." ),
                       KiBitmap( BITMAPS::delete_gerber ) );
}


void GERBER_LAYER_WIDGET::onRightDownLayers( wxMouseEvent& event )
{
    wxMenu          menu;

    AddRightClickMenuItems( &menu );
    PopupMenu( &menu );

    passOnFocus();
}


void GERBER_LAYER_WIDGET::onPopupSelection( wxCommandEvent& event )
{
    int  layer;
    int  rowCount;
    int  menuId = event.GetId();
    bool visible = (menuId == ID_SHOW_ALL_LAYERS) ? true : false;
    LSET visibleLayers;
    bool force_active_layer_visible;

    switch( menuId )
    {
    case ID_SHOW_ALL_LAYERS:
    case ID_SHOW_NO_LAYERS:
    case ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE:
    case ID_SHOW_NO_LAYERS_BUT_ACTIVE:
        // Set the display layers options. Sorting layers has no effect to these options
        m_alwaysShowActiveLayer = ( menuId == ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE );
        force_active_layer_visible = ( menuId == ID_SHOW_NO_LAYERS_BUT_ACTIVE ||
                                       menuId == ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE );

        // Update icons and check boxes
        rowCount = GetLayerRowCount();

        for( int row = 0; row < rowCount; ++row )
        {
            wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
            layer = getDecodedId( cb->GetId() );
            bool loc_visible = visible;

            if( force_active_layer_visible && (layer == m_frame->GetActiveLayer() ) )
                loc_visible = true;

            cb->SetValue( loc_visible );
            visibleLayers[ row ] = loc_visible;
        }

        m_frame->SetVisibleLayers( visibleLayers );
        m_frame->GetCanvas()->Refresh();
        break;

    case ID_SORT_GBR_LAYERS_X2:
        m_frame->SortLayersByX2Attributes();
        break;

    case ID_SORT_GBR_LAYERS_FILE_EXT:
        m_frame->SortLayersByFileExtension();
        break;

    case ID_SET_GBR_LAYERS_DRAW_PRMS:
        m_frame->SetLayerDrawPrms();
        break;

    case ID_LAYER_MOVE_UP:
        m_frame->GetToolManager()->RunAction( GERBVIEW_ACTIONS::moveLayerUp );
        break;

    case ID_LAYER_MOVE_DOWN:
        m_frame->GetToolManager()->RunAction( GERBVIEW_ACTIONS::moveLayerDown );
        break;

    case ID_LAYER_DELETE:
        m_frame->Erase_Current_DrawLayer( false );

        break;
    }
}

bool  GERBER_LAYER_WIDGET::OnLayerSelected()
{
    if( !m_alwaysShowActiveLayer )
        return false;

    // postprocess after active layer selection ensure active layer visible
    wxCommandEvent event;
    event.SetId( ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE );
    onPopupSelection( event );
    return true;
}


void GERBER_LAYER_WIDGET::ReFill()
{
    ClearLayerRows();

    Freeze();

    for( int layer = 0; layer < GERBER_DRAWLAYERS_COUNT; ++layer )
    {
        // Don't show inactive layers
        if ( GetImagesList()->GetGbrImage(layer) == nullptr )
                continue;

        int      aRow = findLayerRow( layer );
        bool     visible = true;
        COLOR4D  color = m_frame->GetLayerColor( GERBER_DRAW_LAYER( layer ) );
        wxString msg = GetImagesList()->GetDisplayName( layer,
                                                        /* include layer number */ false,
                                                        /* Get the full name */ true );

        if( m_frame->GetCanvas() )
            visible = m_frame->GetCanvas()->GetView()->IsLayerVisible( GERBER_DRAW_LAYER( layer ) );
        else
            visible = m_frame->IsLayerVisible( layer );

        if( aRow >= 0 )
        {
            updateLayerRow( findLayerRow( layer ), msg );
            SetLayerVisible( layer, visible );
        }
        else
        {
            AppendLayerRow( LAYER_WIDGET::ROW( msg, layer, color, wxEmptyString, visible, true ) );
        }
    }

    UpdateLayouts();
    Thaw();
}


void GERBER_LAYER_WIDGET::OnLayerRightClick( wxMenu& aMenu )
{
    AddRightClickMenuItems( &aMenu );
}


void GERBER_LAYER_WIDGET::OnLayerColorChange( int aLayer, const COLOR4D& aColor )
{
    // NOTE: Active layer in GerbView is stored as 0-indexed, but layer color is
    //       stored according to the GERBER_DRAW_LAYER() offset.
    m_frame->SetLayerColor( GERBER_DRAW_LAYER( aLayer ), aColor );

    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();
    COLOR_SETTINGS* color_settings = m_frame->GetColorSettings();
    color_settings->SetColor( aLayer, aColor );

    view->GetPainter()->GetSettings()->LoadColors( color_settings );
    view->UpdateLayerColor( GERBER_DRAW_LAYER( aLayer ) );

    m_frame->GetCanvas()->Refresh();
}


bool GERBER_LAYER_WIDGET::OnLayerSelect( int aLayer )
{
    // the layer change from the GERBER_LAYER_WIDGET can be denied by returning
    // false from this function.
    int layer = m_frame->GetActiveLayer();

    m_frame->SetActiveLayer( aLayer, false );
    m_frame->syncLayerBox();

    if( layer != m_frame->GetActiveLayer() )
    {
        if( ! OnLayerSelected() )
        {
            auto settings = static_cast<KIGFX::GERBVIEW_PAINTER*>
                                ( m_frame->GetCanvas()->GetView()->GetPainter() )->GetSettings();
            int dcodeSelected = m_frame->m_DCodeSelector->GetSelectedDCodeId();
            settings->m_dcodeHighlightValue = dcodeSelected;
            m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::COLOR );
            m_frame->GetCanvas()->Refresh();
        }
    }

    return true;
}


void GERBER_LAYER_WIDGET::OnLayerVisible( int aLayer, bool isVisible, bool isFinal )
{
    LSET visibleLayers = m_frame->GetVisibleLayers();

    visibleLayers[ aLayer ] = isVisible;

    m_frame->SetVisibleLayers( visibleLayers );

    if( isFinal )
        m_frame->GetCanvas()->Refresh();
}


void GERBER_LAYER_WIDGET::OnRenderColorChange( int aId, const COLOR4D& aColor )
{
    m_frame->SetVisibleElementColor( aId, aColor );

    auto view = m_frame->GetCanvas()->GetView();

    COLOR_SETTINGS* color_settings = m_frame->GetColorSettings();
    color_settings->SetColor( aId, aColor );

    view->GetPainter()->GetSettings()->LoadColors( color_settings );
    view->UpdateLayerColor( aId );
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    view->UpdateAllItems( KIGFX::COLOR );
    m_frame->GetCanvas()->Refresh();
}


void GERBER_LAYER_WIDGET::OnRenderEnable( int aId, bool isEnabled )
{
    m_frame->SetElementVisibility( aId, isEnabled );

    if( m_frame->GetCanvas() )
    {
        if( aId == LAYER_GERBVIEW_GRID )
        {
            m_frame->GetCanvas()->GetGAL()->SetGridVisibility( m_frame->IsGridVisible() );
            m_frame->GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
        }
        else
        {
            m_frame->GetCanvas()->GetView()->SetLayerVisible( aId, isEnabled );
        }
    }

    m_frame->GetCanvas()->Refresh();
}
