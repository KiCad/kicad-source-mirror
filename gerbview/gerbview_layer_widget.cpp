/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2010 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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


/**
 * @file class_gerbview_layer_widget.cpp
 * @brief  GerbView layers manager.
 */

#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <macros.h>
#include <gbr_layer_box_selector.h>
#include <menus_helpers.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerber_file_image_list.h>
#include <layer_widget.h>
#include <gerbview_layer_widget.h>

#include <view/view.h>
#include <gerbview_painter.h>
#include <gal/graphics_abstraction_layer.h>


/*
 * Class GERBER_LAYER_WIDGET
 * is here to implement the abtract functions of LAYER_WIDGET so they
 * may be tied into the GERBVIEW_FRAME's data and so we can add a popup
 * menu which is specific to Pcbnew's needs.
 */


GERBER_LAYER_WIDGET::GERBER_LAYER_WIDGET( GERBVIEW_FRAME* aParent, wxWindow* aFocusOwner ) :
    LAYER_WIDGET( aParent, aFocusOwner ),
    myframe( aParent )
{
    m_alwaysShowActiveLayer = false;

    ReFillRender();

    // Update default tabs labels for GerbView
    SetLayersManagerTabsText( );

    //-----<Popup menu>-------------------------------------------------
    // handle the popup menu over the layer window.
    m_LayerScrolledWindow->Connect( wxEVT_RIGHT_DOWN,
        wxMouseEventHandler( GERBER_LAYER_WIDGET::onRightDownLayers ), NULL, this );

    // since Popupmenu() calls this->ProcessEvent() we must call this->Connect()
    // and not m_LayerScrolledWindow->Connect()
    Connect( ID_LAYER_MANAGER_START, ID_LAYER_MANAGER_END,
        wxEVT_COMMAND_MENU_SELECTED,
        wxCommandEventHandler( GERBER_LAYER_WIDGET::onPopupSelection ), NULL, this );
}

GERBER_FILE_IMAGE_LIST* GERBER_LAYER_WIDGET::GetImagesList()
{
    return &GERBER_FILE_IMAGE_LIST::GetImagesList();
}


bool GERBER_LAYER_WIDGET::AreArbitraryColorsAllowed()
{
    return myframe->IsGalCanvasActive();
}


void GERBER_LAYER_WIDGET::SetLayersManagerTabsText( )
{
    m_notebook->SetPageText(0, _("Layers") );
    m_notebook->SetPageText(1, _("Items") );
}

/**
 * Function ReFillRender
 * Rebuild Render for instance after the config is read
 */
void GERBER_LAYER_WIDGET::ReFillRender()
{
    ClearRenderRows();

    // Fixed "Rendering" tab rows within the LAYER_WIDGET, only the initial color
    // is changed before appending to the LAYER_WIDGET.  This is an automatic variable
    // not a static variable, change the color & state after copying from code to renderRows
    // on the stack.
    LAYER_WIDGET::ROW renderRows[6] = {

#define RR  LAYER_WIDGET::ROW   // Render Row abreviation to reduce source width

             // text                 id                      color     tooltip                 checked
        RR( _( "DCodes" ),           LAYER_DCODES,           WHITE,    _( "Show DCodes identification" ) ),
        RR( _( "Negative Objects" ), LAYER_NEGATIVE_OBJECTS, DARKGRAY, _( "Show negative objects in this color" ) ),
        RR(),
        RR( _( "Grid" ),             LAYER_GERBVIEW_GRID,    WHITE,    _( "Show the (x,y) grid dots" ) ),
        RR( _( "Worksheet" ),        LAYER_WORKSHEET,        DARKRED,  _( "Show worksheet") ),
        RR( _( "Background" ),       LAYER_PCB_BACKGROUND,   BLACK,    _( "PCB Background" ), true, false )
    };

    for( unsigned row=0;  row<arrayDim(renderRows);  ++row )
    {
        if( renderRows[row].color != COLOR4D::UNSPECIFIED )       // does this row show a color?
            renderRows[row].color = myframe->GetVisibleElementColor( renderRows[row].id );

        if( renderRows[row].id )    // if not the separator
            renderRows[row].state = myframe->IsElementVisible( renderRows[row].id );
    }

    AppendRenderRows( renderRows, arrayDim(renderRows) );
}


void GERBER_LAYER_WIDGET::AddRightClickMenuItems( wxMenu* aMenu )
{
    // Remember: menu text is capitalized (see our rules_for_capitalization_in_Kicad_UI.txt)
    AddMenuItem( aMenu, ID_SHOW_ALL_LAYERS, _( "Show All Layers" ),
                 KiBitmap( show_all_layers_xpm ) );

    AddMenuItem( aMenu, ID_SHOW_NO_LAYERS_BUT_ACTIVE,
                 _( "Hide All Layers But Active" ),
                 KiBitmap( select_w_layer_xpm ) );

    AddMenuItem( aMenu, ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE,
                 _( "Always Hide All Layers But Active" ),
                 KiBitmap( select_w_layer_xpm ) );

    AddMenuItem( aMenu, ID_SHOW_NO_LAYERS, _( "Hide All Layers" ),
                 KiBitmap( show_no_layers_xpm ) );

    aMenu->AppendSeparator();
    AddMenuItem( aMenu, ID_SORT_GBR_LAYERS, _( "Sort Layers if X2 Mode" ),
                 KiBitmap( reload_xpm ) );
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
    int  rowCount;
    int  menuId = event.GetId();
    bool visible = (menuId == ID_SHOW_ALL_LAYERS) ? true : false;
    long visibleLayers = 0;
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
        for( int row=0; row < rowCount; ++row )
        {
            wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
            int layer = getDecodedId( cb->GetId() );
            bool loc_visible = visible;

            if( force_active_layer_visible &&
                (layer == myframe->GetActiveLayer() ) )
            {
                loc_visible = true;
            }

            cb->SetValue( loc_visible );

            if( loc_visible )
                visibleLayers |= 1 << row;
            else
                visibleLayers &= ~( 1 << row );
        }

        myframe->SetVisibleLayers( visibleLayers );
        myframe->GetCanvas()->Refresh();
        break;

    case ID_SORT_GBR_LAYERS:
        myframe->SortLayersByX2Attributes();
        break;
    }
}

bool  GERBER_LAYER_WIDGET::OnLayerSelected()
{
    if( !m_alwaysShowActiveLayer )
        return false;

    // postprocess after active layer selection
    // ensure active layer visible
    wxCommandEvent event;
    event.SetId( ID_ALWAYS_SHOW_NO_LAYERS_BUT_ACTIVE );
    onPopupSelection( event );
    return true;
}


void GERBER_LAYER_WIDGET::ReFill()
{
    Freeze();

    ClearLayerRows();

    for( int layer = 0; layer < GERBER_DRAWLAYERS_COUNT; ++layer )
    {
        wxString msg = GetImagesList()->GetDisplayName( layer );

        bool visible = true;

        if( auto canvas = myframe->GetGalCanvas() )
        {
            visible = canvas->GetView()->IsLayerVisible( GERBER_DRAW_LAYER( layer ) );
        }
        else
        {
            visible = myframe->IsLayerVisible( layer );
        }

        AppendLayerRow( LAYER_WIDGET::ROW( msg, layer,
                        myframe->GetLayerColor( GERBER_DRAW_LAYER( layer ) ),
                        wxEmptyString, visible, true ) );
    }

    UpdateLayouts();
    Thaw();
}

//-----<LAYER_WIDGET callbacks>-------------------------------------------

void GERBER_LAYER_WIDGET::OnLayerRightClick( wxMenu& aMenu )
{
    AddRightClickMenuItems( &aMenu );
}


void GERBER_LAYER_WIDGET::OnLayerColorChange( int aLayer, COLOR4D aColor )
{
    // NOTE: Active layer in GerbView is stored as 0-indexed, but layer color is
    //       stored according to the GERBER_DRAW_LAYER() offset.

    myframe->SetLayerColor( GERBER_DRAW_LAYER( aLayer ), aColor );
    myframe->m_SelLayerBox->ResyncBitmapOnly();

    if( myframe->IsGalCanvasActive() )
    {
        KIGFX::VIEW* view = myframe->GetGalCanvas()->GetView();
        view->GetPainter()->GetSettings()->ImportLegacyColors( myframe->m_colorsSettings );
        view->UpdateLayerColor( GERBER_DRAW_LAYER( aLayer ) );
    }

    myframe->GetCanvas()->Refresh();
}


bool GERBER_LAYER_WIDGET::OnLayerSelect( int aLayer )
{
    // the layer change from the GERBER_LAYER_WIDGET can be denied by returning
    // false from this function.
    int layer = myframe->GetActiveLayer();

    myframe->SetActiveLayer( aLayer, false );
    myframe->syncLayerBox();

    if( layer != myframe->GetActiveLayer() )
    {
        if( ! OnLayerSelected() )
            myframe->GetCanvas()->Refresh();
    }

    return true;
}


void GERBER_LAYER_WIDGET::OnLayerVisible( int aLayer, bool isVisible, bool isFinal )
{
    long visibleLayers = myframe->GetVisibleLayers();

    if( isVisible )
        visibleLayers |= 1 << aLayer ;
    else
        visibleLayers &= ~( 1 << aLayer );

    myframe->SetVisibleLayers( visibleLayers );

    if( isFinal )
        myframe->GetCanvas()->Refresh();
}


void GERBER_LAYER_WIDGET::OnRenderColorChange( int aId, COLOR4D aColor )
{
    myframe->SetVisibleElementColor( aId, aColor );

    auto galCanvas = myframe->GetGalCanvas();

    if( galCanvas && myframe->IsGalCanvasActive() )
    {
        auto view = galCanvas->GetView();
        view->GetPainter()->GetSettings()->ImportLegacyColors( myframe->m_colorsSettings );
        view->UpdateLayerColor( aId );

        view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
        view->UpdateAllItems( KIGFX::COLOR );
    }

    if( galCanvas && myframe->IsGalCanvasActive() )
        galCanvas->Refresh();
    else
        myframe->GetCanvas()->Refresh();
}


void GERBER_LAYER_WIDGET::OnRenderEnable( int aId, bool isEnabled )
{
    myframe->SetElementVisibility( aId, isEnabled );

    auto galCanvas = myframe->GetGalCanvas();

    if( galCanvas )
    {
        if( aId == LAYER_GERBVIEW_GRID )
        {
            galCanvas->GetGAL()->SetGridVisibility( myframe->IsGridVisible() );
            galCanvas->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
        }
        else
            galCanvas->GetView()->SetLayerVisible( aId, isEnabled );
    }

    if( galCanvas && myframe->IsGalCanvasActive() )
        galCanvas->Refresh();
    else
        myframe->GetCanvas()->Refresh();
}

//-----</LAYER_WIDGET callbacks>------------------------------------------

/*
 * Virtual Function useAlternateBitmap
 * return true if bitmaps shown in Render layer list
 * must be alternate bitmap (when a gerber image is loaded), or false to use "normal" bitmap
 */
bool GERBER_LAYER_WIDGET::useAlternateBitmap(int aRow)
{
    return GetImagesList()->GetGbrImage( aRow ) != NULL;
}
