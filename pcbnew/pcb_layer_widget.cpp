/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jean-pierre.charras@gpisa-lab.inpg.fr
 * Copyright (C) 2010-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010-2017 KiCad Developers, see change_log.txt for contributors.
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
 * @file pcb_layer_widget.cpp
 * @brief  Pcbnew specialization of LAYER_WIDGET layers manager
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <painter.h>

#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcb_display_options.h>
#include <tool/tool_manager.h>
#include <layer_widget.h>
#include <class_text_mod.h>
#include <widgets/indicator_icon.h>
#include <macros.h>
#include <menus_helpers.h>
#include <gal/graphics_abstraction_layer.h>
#include <pcb_painter.h>

#include <class_board.h>
#include <pcb_layer_widget.h>

#include <pcbnew.h>
#include <collectors.h>
#include <pcbnew_id.h>

#include <gal/graphics_abstraction_layer.h>


/// This is a read only template that is copied and modified before adding to LAYER_WIDGET
const LAYER_WIDGET::ROW PCB_LAYER_WIDGET::s_render_rows[] = {

#define RR  LAYER_WIDGET::ROW   // Render Row abbreviation to reduce source width
#define NOCOLOR COLOR4D::UNSPECIFIED    // specify rows that do not have a color selector icon

         // text                     id                      color       tooltip
    RR( _( "Footprints Front" ),     LAYER_MOD_FR,         NOCOLOR,  _( "Show footprints that are on board's front") ),
    RR( _( "Footprints Back" ),      LAYER_MOD_BK,         NOCOLOR,  _( "Show footprints that are on board's back") ),
    RR( _( "Values" ),               LAYER_MOD_VALUES,     NOCOLOR,  _( "Show footprint values") ),
    RR( _( "References" ),           LAYER_MOD_REFERENCES, NOCOLOR,  _( "Show footprint references") ),
    RR( _( "Footprint Text Front" ), LAYER_MOD_TEXT_FR,    NOCOLOR,  _( "Show footprint text on board's front" ) ),
    RR( _( "Footprint Text Back" ),  LAYER_MOD_TEXT_BK,    NOCOLOR,  _( "Show footprint text on board's back" ) ),
    RR( _( "Hidden Text" ),          LAYER_MOD_TEXT_INVISIBLE, WHITE, _( "Show footprint text marked as invisible" ) ),
    RR( _( "Pads Front" ),           LAYER_PAD_FR,         WHITE,    _( "Show footprint pads on board's front" ) ),
    RR( _( "Pads Back" ),            LAYER_PAD_BK,         WHITE,    _( "Show footprint pads on board's back" ) ),
    RR( _( "Through Hole Pads" ),    LAYER_PADS_TH,        YELLOW,   _( "Show through hole pads in specific color") ),
    RR(),
    RR( _( "Tracks" ),          LAYER_TRACKS,         NOCOLOR,  _( "Show tracks" ) ),
    RR( _( "Through Via" ),     LAYER_VIA_THROUGH,    WHITE,    _( "Show through vias" ) ),
    RR( _( "Bl/Buried Via" ),   LAYER_VIA_BBLIND,     WHITE,    _( "Show blind or buried vias" )  ),
    RR( _( "Micro Via" ),       LAYER_VIA_MICROVIA,   WHITE,    _( "Show micro vias") ),
    RR( _( "Non Plated Holes" ),LAYER_NON_PLATEDHOLES,WHITE,    _( "Show non plated holes in specific color") ),
    RR(),
    RR( _( "Ratsnest" ),        LAYER_RATSNEST,       WHITE,    _( "Show unconnected nets as a ratsnest") ),
    RR( _( "No-Connects" ),     LAYER_NO_CONNECTS,    BLUE,     _( "Show a marker on pads which have no net connected" ) ),
    RR( _( "Anchors" ),         LAYER_ANCHOR,         WHITE,    _( "Show footprint and text origins as a cross" ) ),
    RR( _( "Worksheet" ),       LAYER_WORKSHEET,      DARKRED,  _( "Show worksheet") ),
    RR( _( "Cursor" ),          LAYER_CURSOR,         WHITE,    _( "PCB Cursor" ), true, false ),
    RR( _( "Aux items" ),       LAYER_AUX_ITEMS,      WHITE,    _( "Auxiliary items (rulers, assistants, axes, etc.)" ), true, false ),
    RR( _( "Grid" ),            LAYER_GRID,           WHITE,    _( "Show the (x,y) grid dots" ) ),
    RR( _( "Background" ),      LAYER_PCB_BACKGROUND, BLACK,    _( "PCB Background" ), true, false )
};

static int s_allowed_in_FpEditor[] =
{
    LAYER_MOD_TEXT_INVISIBLE,
    LAYER_NON_PLATEDHOLES,
    LAYER_PADS_TH,
    LAYER_PAD_FR,
    LAYER_PAD_BK,
    LAYER_GRID,
    LAYER_MOD_VALUES,
    LAYER_MOD_REFERENCES,
    LAYER_PCB_BACKGROUND
};


PCB_LAYER_WIDGET::PCB_LAYER_WIDGET( PCB_BASE_FRAME* aParent, wxWindow* aFocusOwner,
                                    bool aFpEditorMode ) :
        LAYER_WIDGET( aParent, aFocusOwner ),
        myframe( aParent )
{
    m_alwaysShowActiveCopperLayer = false;
    m_fp_editor_mode = aFpEditorMode;

    // Update default tabs labels
    SetLayersManagerTabsText();

    //-----<Popup menu>-------------------------------------------------
    // handle the popup menu over the layer window.
    m_LayerScrolledWindow->Connect( wxEVT_RIGHT_DOWN,
        wxMouseEventHandler( PCB_LAYER_WIDGET::onRightDownLayers ), NULL, this );

    // since Popupmenu() calls this->ProcessEvent() we must call this->Connect()
    // and not m_LayerScrolledWindow->Connect()

    Connect( ID_SHOW_ALL_COPPER_LAYERS, ID_LAST_VALUE - 1,
        wxEVT_COMMAND_MENU_SELECTED,
        wxCommandEventHandler( PCB_LAYER_WIDGET::onPopupSelection ), NULL, this );
}


bool PCB_LAYER_WIDGET::AreArbitraryColorsAllowed()
{
    return myframe->IsGalCanvasActive();
}


COLOR4D PCB_LAYER_WIDGET::getBackgroundLayerColor()
{
    return myframe->Settings().Colors().GetLayerColor( LAYER_PCB_BACKGROUND );
}


bool PCB_LAYER_WIDGET::isAllowedInFpMode( int aId )
{
    for( unsigned ii = 0; ii < arrayDim( s_allowed_in_FpEditor ); ii++ )
        if( s_allowed_in_FpEditor[ii] == aId )
            return true;

    return false;
}


bool PCB_LAYER_WIDGET::isLayerAllowedInFpMode( PCB_LAYER_ID aLayer )
{
    static LSET allowed = LSET::AllTechMask();
    allowed.set( F_Cu ).set( B_Cu );
    return allowed.test( aLayer );
}


void PCB_LAYER_WIDGET::AddRightClickMenuItems( wxMenu& menu )
{
    // menu text is capitalized:
    // http://library.gnome.org/devel/hig-book/2.20/design-text-labels.html.en#layout-capitalization
    AddMenuItem( &menu, ID_SHOW_ALL_COPPER_LAYERS,
                 _( "Show All Copper Layers" ),
                 KiBitmap( select_layer_pair_xpm ) );
    AddMenuItem( &menu, ID_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE,
                 _( "Hide All Copper Layers But Active" ),
                 KiBitmap( select_w_layer_xpm ) );
    AddMenuItem( &menu, ID_ALWAYS_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE,
                 _( "Always Hide All Copper Layers But Active" ),
                 KiBitmap( select_w_layer_xpm ) );
    AddMenuItem( &menu, ID_SHOW_NO_COPPER_LAYERS,
                 _( "Hide All Copper Layers" ),
                 KiBitmap( show_no_copper_layers_xpm ) );

    menu.AppendSeparator();

    AddMenuItem( &menu, ID_SHOW_ALL_NON_COPPER,
                 _( "Show All Non Copper Layers" ),
                 KiBitmap( select_w_layer_xpm ) );
    AddMenuItem( &menu, ID_HIDE_ALL_NON_COPPER,
                 _( "Hide All Non Copper Layers" ),
                 KiBitmap( show_no_copper_layers_xpm ) );

    menu.AppendSeparator();

    AddMenuItem( &menu, ID_SHOW_NO_LAYERS, _( "Hide All Layers" ),
                 KiBitmap( show_no_layers_xpm ) );
    AddMenuItem( &menu, ID_SHOW_ALL_LAYERS, _( "Show All Layers" ),
                 KiBitmap( show_all_layers_xpm ) );

    menu.AppendSeparator();

    AddMenuItem( &menu, ID_SHOW_ALL_FRONT, _( "Show All Front Layers" ),
                 KiBitmap( show_no_layers_xpm ) );

    AddMenuItem( &menu, ID_SHOW_ALL_BACK, _( "Show All Back Layers" ),
                 KiBitmap( show_all_layers_xpm ) );
}


void PCB_LAYER_WIDGET::onRightDownLayers( wxMouseEvent& event )
{
    wxMenu menu;

    AddRightClickMenuItems( menu );
    PopupMenu( &menu );

    passOnFocus();
}


void PCB_LAYER_WIDGET::onPopupSelection( wxCommandEvent& event )
{
    int     rowCount;
    int     menuId = event.GetId();
    bool    visible;
    bool    force_active_layer_visible;

    m_alwaysShowActiveCopperLayer = ( menuId == ID_ALWAYS_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE );
    force_active_layer_visible = ( menuId == ID_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE ||
            menuId == ID_ALWAYS_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE );

    switch( menuId )
    {
        case ID_SHOW_NO_LAYERS:
        case ID_SHOW_ALL_LAYERS:
            {
                visible = ( menuId == ID_SHOW_ALL_LAYERS );
                rowCount = GetLayerRowCount();

                for( int row=0;  row<rowCount;  ++row )
                {
                    bool isLast;
                    wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
                    PCB_LAYER_ID    layer = ToLAYER_ID( getDecodedId( cb->GetId() ) );
                    cb->SetValue( visible );

                    isLast = row == rowCount-1;

                    OnLayerVisible( layer, visible, isLast );

                    if( isLast )
                        break;
                }
                break;
            }

        case ID_SHOW_ALL_COPPER_LAYERS:
        case ID_ALWAYS_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE:
        case ID_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE:
        case ID_SHOW_NO_COPPER_LAYERS:
        case ID_HIDE_ALL_NON_COPPER:
        case ID_SHOW_ALL_NON_COPPER:
            {

                // Search the last copper layer row index:
                int lastCu = -1;
                rowCount = GetLayerRowCount();
                for( int row = rowCount-1; row>=0; --row )
                {
                    wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
                    PCB_LAYER_ID    layer = ToLAYER_ID( getDecodedId( cb->GetId() ) );

                    if( IsCopperLayer( layer ) )
                    {
                        lastCu = row;
                        break;
                    }
                }

                // Enable/disable the copper layers visibility:
                int startrow = 0;

                if(     ( menuId == ID_SHOW_ALL_NON_COPPER ) ||
                        ( menuId == ID_HIDE_ALL_NON_COPPER ) )
                {
                    startrow = lastCu + 1;
                }

                for( int row = startrow;  row<rowCount;  ++row )
                {
                    wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
                    PCB_LAYER_ID    layer = ToLAYER_ID( getDecodedId( cb->GetId() ) );

                    visible = ( ( menuId == ID_SHOW_ALL_COPPER_LAYERS ) || ( menuId == ID_SHOW_ALL_NON_COPPER ) );

                    if( force_active_layer_visible && (layer == myframe->GetActiveLayer() ) )
                        visible = true;

                    cb->SetValue( visible );

                    bool isLastLayer = (row == lastCu);

                    if(     ( menuId == ID_SHOW_ALL_NON_COPPER ) ||
                            ( menuId == ID_HIDE_ALL_NON_COPPER ) )
                    {
                        isLastLayer = false;
                    }
                    OnLayerVisible( layer, visible, isLastLayer );

                    if( isLastLayer )
                        break;
                }
                break;
            }

        case ID_SHOW_ALL_FRONT:
            {
                visible = false;
                rowCount = GetLayerRowCount();

                for( int row=0;  row<rowCount;  ++row )
                {
                    bool isLast;
                    wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
                    PCB_LAYER_ID    layer = ToLAYER_ID( getDecodedId( cb->GetId() ) );
                    isLast = ( row == rowCount-1 );

                    if(  layer == F_Paste || layer == F_SilkS ||
                         layer == F_Mask  || layer == F_Cu ||
                         layer == F_Fab || layer == F_CrtYd  || layer == Edge_Cuts )
                    {
                        visible = true;
                    }
                    else
                    {
                        visible = false;
                    }

                    cb->SetValue( visible );
                    OnLayerVisible( layer, visible, isLast );

                    if( isLast )
                        break;
                }
                break;
            }
        case ID_SHOW_ALL_BACK:
            {
                visible = false;
                rowCount = GetLayerRowCount();

                for( int row=0;  row<rowCount;  ++row )
                {
                    bool isLast;
                    wxCheckBox* cb = (wxCheckBox*) getLayerComp( row, COLUMN_COLOR_LYR_CB );
                    PCB_LAYER_ID    layer = ToLAYER_ID( getDecodedId( cb->GetId() ) );
                    isLast = ( row == rowCount-1 );

                    if( layer == B_Paste || layer == B_SilkS ||
                        layer == B_Mask  || layer == B_Cu ||
                        layer == B_Fab || layer == B_CrtYd || layer == Edge_Cuts )
                    {
                        visible = true;
                    }
                    else
                    {
                        visible = false;
                    }

                    cb->SetValue( visible );
                    OnLayerVisible( layer, visible, isLast );

                    if( isLast )
                        break;
                }
                break;
            }
    }
}


void PCB_LAYER_WIDGET::SetLayersManagerTabsText()
{
    m_notebook->SetPageText( 0, _( "Layers" ) );
    m_notebook->SetPageText( 1, _( "Items" ) );
}


void PCB_LAYER_WIDGET::ReFillRender()
{
    BOARD* board = myframe->GetBoard();
    auto settings = board->GetDesignSettings();

    ClearRenderRows();

    // Add "Items" tab rows to LAYER_WIDGET, after setting color and checkbox state.
    // Because s_render_rows is created static, we must explicitly call
    // wxGetTranslation for texts which are internationalized (tool tips
    // and item names)
    for( unsigned row=0;  row<arrayDim(s_render_rows);  ++row )
    {
        LAYER_WIDGET::ROW renderRow = s_render_rows[row];

        if( m_fp_editor_mode && !isAllowedInFpMode( renderRow.id ) )
            continue;

        if( renderRow.id == LAYER_VIA_MICROVIA && !settings.m_MicroViasAllowed )
            continue;

        if( renderRow.id == LAYER_VIA_BBLIND && !settings.m_BlindBuriedViaAllowed )
            continue;

        if( !renderRow.spacer )
        {
            renderRow.tooltip = wxGetTranslation( s_render_rows[row].tooltip );
            renderRow.rowName = wxGetTranslation( s_render_rows[row].rowName );

            if( renderRow.color != COLOR4D::UNSPECIFIED )       // does this row show a color?
            {
                // this window frame must have an established BOARD, i.e. after SetBoard()
                renderRow.color = myframe->Settings().Colors().GetItemColor( static_cast<GAL_LAYER_ID>( renderRow.id ) );
            }

            renderRow.state = board->IsElementVisible( static_cast<GAL_LAYER_ID>( renderRow.id ) );
        }

        AppendRenderRow( renderRow );
    }

    UpdateLayouts();
}


void PCB_LAYER_WIDGET::SyncLayerVisibilities()
{
    BOARD*  board = myframe->GetBoard();
    int     count = GetLayerRowCount();

    for( int row=0;  row<count;  ++row )
    {
        // this utilizes more implementation knowledge than ideal, eventually
        // add member ROW getRow() or similar to base LAYER_WIDGET.

        wxWindow* w = getLayerComp( row, COLUMN_ICON_ACTIVE );

        PCB_LAYER_ID layerId = ToLAYER_ID( getDecodedId( w->GetId() ) );

        // this does not fire a UI event
        setLayerCheckbox( layerId, board->IsLayerVisible( layerId ) );
    }
}


#define ALPHA_EPSILON 0.04

void PCB_LAYER_WIDGET::SyncLayerAlphaIndicators()
{
    int count = GetLayerRowCount();
    TOOL_MANAGER* mgr = myframe->GetToolManager();
    KIGFX::PCB_PAINTER* painter = static_cast<KIGFX::PCB_PAINTER*>( mgr->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();

    for( int row = 0; row < count; ++row )
    {
        // this utilizes more implementation knowledge than ideal, eventually
        // add member ROW getRow() or similar to base LAYER_WIDGET.

        wxWindow* w = getLayerComp( row, COLUMN_ICON_ACTIVE );
        PCB_LAYER_ID layerId = ToLAYER_ID( getDecodedId( w->GetId() ) );
        KIGFX::COLOR4D screenColor = settings->GetLayerColor( layerId );

        COLOR_SWATCH* swatch = static_cast<COLOR_SWATCH*>( getLayerComp( row, COLUMN_COLORBM ) );
        KIGFX::COLOR4D layerColor = swatch->GetSwatchColor();

        INDICATOR_ICON* indicator = static_cast<INDICATOR_ICON*>( getLayerComp( row, COLUMN_ALPHA_INDICATOR ) );

        if( std::abs( screenColor.a - layerColor.a ) > ALPHA_EPSILON )
        {
            if( screenColor.a < layerColor.a )
                indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::DOWN );
            else
                indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::UP );
        }
        else
            indicator->SetIndicatorState( ROW_ICON_PROVIDER::STATE::OFF );
    }
}


void PCB_LAYER_WIDGET::ReFill()
{
    BOARD*  brd = myframe->GetBoard();
    LSET    enabled = brd->GetEnabledLayers();

    ClearLayerRows();

    wxString dsc;

    // show all coppers first, with front on top, back on bottom, then technical layers
    for( LSEQ cu_stack = enabled.CuStack(); cu_stack; ++cu_stack )
    {
        PCB_LAYER_ID layer = *cu_stack;

        switch( layer )
        {
        case F_Cu:
            dsc = _( "Front copper layer" );
            break;

        case B_Cu:
            dsc = _( "Back copper layer" );
            break;

        default:
            dsc = _( "Inner copper layer" );
            break;
        }

        AppendLayerRow( LAYER_WIDGET::ROW(
            brd->GetLayerName( layer ), layer, myframe->Settings().Colors().GetLayerColor( layer ),
            dsc, true ) );

        if( m_fp_editor_mode && LSET::ForbiddenFootprintLayers().test( layer ) )
        {
            getLayerComp( GetLayerRowCount()-1, COLUMN_COLOR_LYRNAME )->Enable( false );
            getLayerComp( GetLayerRowCount()-1, COLUMN_COLORBM )->SetToolTip( wxEmptyString );
        }
    }

    UpdateLayouts();


    // technical layers are shown in this order:
    // Because they are static, wxGetTranslation must be explicitly
    // called for tooltips.
    static const struct {
        PCB_LAYER_ID    layerId;
        wxString    tooltip;
    } non_cu_seq[] = {
        { F_Adhes,          _( "Adhesive on board's front" ) },
        { B_Adhes,          _( "Adhesive on board's back" ) },
        { F_Paste,          _( "Solder paste on board's front" )  },
        { B_Paste,          _( "Solder paste on board's back" ) },
        { F_SilkS,          _( "Silkscreen on board's front" ) },
        { B_SilkS,          _( "Silkscreen on board's back" )  },
        { F_Mask,           _( "Solder mask on board's front" ) },
        { B_Mask,           _( "Solder mask on board's back" )  },
        { Dwgs_User,        _( "Explanatory drawings" )      },
        { Cmts_User,        _( "Explanatory comments" )         },
        { Eco1_User,        _( "User defined meaning" )         },
        { Eco2_User,        _( "User defined meaning" )         },
        { Edge_Cuts,        _( "Board's perimeter definition" ) },
        { Margin,           _( "Board's edge setback outline" ) },
        { F_CrtYd,          _( "Footprint courtyards on board's front" ) },
        { B_CrtYd,          _( "Footprint courtyards on board's back" ) },
        { F_Fab,            _( "Footprint assembly on board's front" ) },
        { B_Fab,            _( "Footprint assembly on board's back" ) }
    };

    for( unsigned i=0;  i<arrayDim( non_cu_seq );  ++i )
    {
        PCB_LAYER_ID layer = non_cu_seq[i].layerId;

        if( !enabled[layer] )
            continue;

        AppendLayerRow( LAYER_WIDGET::ROW(
            brd->GetLayerName( layer ), layer,  myframe->Settings().Colors().GetLayerColor( layer ),
            wxGetTranslation( non_cu_seq[i].tooltip ), true ) );

        if( m_fp_editor_mode && LSET::ForbiddenFootprintLayers().test( layer ) )
        {
            getLayerComp( GetLayerRowCount()-1, COLUMN_COLOR_LYRNAME )->Enable( false );
            getLayerComp( GetLayerRowCount()-1, COLUMN_COLORBM )->SetToolTip( wxEmptyString );
        }
    }
}


//-----<LAYER_WIDGET callbacks>-------------------------------------------

void PCB_LAYER_WIDGET::OnLayerColorChange( int aLayer, COLOR4D aColor )
{
    // Avoid setting the alpha channel, when we are in legacy mode,
    // because in legacy mode the alpha channel is not used, but changing it
    // destroys the GAL color setup
    if( !myframe->IsGalCanvasActive() )
    {
        COLOR4D oldColor = myframe->Settings().Colors().GetLayerColor( aLayer );
        aColor.a = oldColor.a;
    }

    myframe->Settings().Colors().SetLayerColor( aLayer, aColor );

    if( myframe->IsGalCanvasActive() )
    {
        KIGFX::VIEW* view = myframe->GetGalCanvas()->GetView();
        view->GetPainter()->GetSettings()->ImportLegacyColors( &myframe->Settings().Colors() );
        view->UpdateLayerColor( aLayer );
        view->UpdateLayerColor( GetNetnameLayer( aLayer ) );
    }

    myframe->ReCreateHToolbar();

    myframe->GetCanvas()->Refresh();

    if( aLayer == LAYER_PCB_BACKGROUND )
        myframe->SetDrawBgColor( aColor );
}


bool PCB_LAYER_WIDGET::OnLayerSelect( int aLayer )
{
    // the layer change from the PCB_LAYER_WIDGET can be denied by returning
    // false from this function.
    PCB_LAYER_ID layer = ToLAYER_ID( aLayer );

    if( m_fp_editor_mode && LSET::ForbiddenFootprintLayers().test( layer ) )
        return false;

    myframe->SetActiveLayer( layer );
    PCB_DISPLAY_OPTIONS* displ_opts = (PCB_DISPLAY_OPTIONS*)myframe->GetDisplayOptions();

    if( m_alwaysShowActiveCopperLayer )
        OnLayerSelected();
    else if( displ_opts->m_ContrastModeDisplay )
        myframe->GetCanvas()->Refresh();

    return true;
}


bool PCB_LAYER_WIDGET::OnLayerSelected()
{
    if( !m_alwaysShowActiveCopperLayer )
        return false;

    // postprocess after an active layer selection
    // ensure active layer visible
    wxCommandEvent event;
    event.SetId( ID_ALWAYS_SHOW_NO_COPPER_LAYERS_BUT_ACTIVE );
    onPopupSelection( event );

    return true;
}


void PCB_LAYER_WIDGET::OnLayerVisible( int aLayer, bool isVisible, bool isFinal )
{
    BOARD* brd = myframe->GetBoard();

    LSET visibleLayers = brd->GetVisibleLayers();

    if( visibleLayers.test( aLayer ) != isVisible )
    {
        visibleLayers.set( aLayer, isVisible );

        brd->SetVisibleLayers( visibleLayers );

        myframe->OnModify();

        EDA_DRAW_PANEL_GAL* galCanvas = myframe->GetGalCanvas();

        if( galCanvas )
            galCanvas->GetView()->SetLayerVisible( aLayer, isVisible );
    }

    if( isFinal )
        myframe->GetCanvas()->Refresh();
}


void PCB_LAYER_WIDGET::OnLayerRightClick( wxMenu& aMenu )
{
    AddRightClickMenuItems( aMenu );
}


void PCB_LAYER_WIDGET::OnRenderColorChange( int aId, COLOR4D aColor )
{
    wxASSERT( aId > GAL_LAYER_ID_START && aId < GAL_LAYER_ID_END );

    myframe->Settings().Colors().SetItemColor( static_cast<GAL_LAYER_ID>( aId ), aColor );

    EDA_DRAW_PANEL_GAL* galCanvas = myframe->GetGalCanvas();

    if( galCanvas && myframe->IsGalCanvasActive() )
    {
        if( aId == LAYER_GRID )
            galCanvas->GetGAL()->SetGridColor( aColor );

        KIGFX::VIEW* view = galCanvas->GetView();
        view->GetPainter()->GetSettings()->ImportLegacyColors( &myframe->Settings().Colors() );
        view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );   // useful to update rastnest
        view->UpdateLayerColor( aId );

        // plated-through-holes don't have their own color; they use the background color
        if( aId == LAYER_PCB_BACKGROUND )
            view->UpdateLayerColor( LAYER_PADS_PLATEDHOLES );

        galCanvas->ForceRefresh();
    }

    myframe->ReCreateHToolbar();

    myframe->GetCanvas()->Refresh();
}


void PCB_LAYER_WIDGET::OnRenderEnable( int aId, bool isEnabled )
{
    BOARD*  brd = myframe->GetBoard();
    wxASSERT( aId > GAL_LAYER_ID_START && aId < GAL_LAYER_ID_END );

    if( myframe->IsType( FRAME_PCB ) )
    {
        // The layer visibility status is saved in the board file so set the board
        // modified state so the user has the option to save the changes.
        if( brd->IsElementVisible( static_cast<GAL_LAYER_ID>( aId ) ) != isEnabled )
            myframe->OnModify();
    }

    brd->SetElementVisibility( static_cast<GAL_LAYER_ID>( aId ), isEnabled );

    EDA_DRAW_PANEL_GAL* galCanvas = myframe->GetGalCanvas();

    if( galCanvas && myframe->IsGalCanvasActive() )
    {
        if( aId == LAYER_GRID )
        {
            galCanvas->GetGAL()->SetGridVisibility( myframe->IsGridVisible() );
            galCanvas->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
        }
        else if( aId == LAYER_RATSNEST )
        {
            // don't touch the layers. ratsnest is enabled on per-item basis.
            galCanvas->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
            galCanvas->GetView()->SetLayerVisible( aId, true );
        }
        else
            galCanvas->GetView()->SetLayerVisible( aId, isEnabled );

        galCanvas->Refresh();
    }

    myframe->GetCanvas()->Refresh();
}

//-----</LAYER_WIDGET callbacks>------------------------------------------
