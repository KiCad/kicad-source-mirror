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


#include <class_board.h>
#include <fctsys.h>
#include <gal/graphics_abstraction_layer.h>
#include <layer_widget.h>
#include <macros.h>
#include <menus_helpers.h>
#include <pcb_display_options.h>
#include <pcb_edit_frame.h>
#include <pcb_layer_widget.h>
#include <pcb_painter.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <pgm_base.h>
#include <settings/color_settings.h>
#include <tool/tool_manager.h>
#include <view/view.h>
#include <widgets/indicator_icon.h>


/// This is a read only template that is copied and modified before adding to LAYER_WIDGET
const LAYER_WIDGET::ROW PCB_LAYER_WIDGET::s_render_rows[] = {

#define RR  LAYER_WIDGET::ROW   // Render Row abbreviation to reduce source width
#define NOCOLOR COLOR4D::UNSPECIFIED    // specify rows that do not have a color selector icon

         // text                     id                      color       tooltip
    RR( _( "Footprints Front" ),     LAYER_MOD_FR,             NOCOLOR,  _( "Show footprints that are on board's front") ),
    RR( _( "Footprints Back" ),      LAYER_MOD_BK,             NOCOLOR,  _( "Show footprints that are on board's back") ),
    RR( _( "Values" ),               LAYER_MOD_VALUES,         NOCOLOR,  _( "Show footprint values") ),
    RR( _( "Reference Designators" ),LAYER_MOD_REFERENCES,     NOCOLOR,  _( "Show footprint reference designators") ),
    RR( _( "Footprint Text Front" ), LAYER_MOD_TEXT_FR,        NOCOLOR,  _( "Show footprint text on board's front" ) ),
    RR( _( "Footprint Text Back" ),  LAYER_MOD_TEXT_BK,        NOCOLOR,  _( "Show footprint text on board's back" ) ),
    RR( _( "Hidden Text" ),          LAYER_MOD_TEXT_INVISIBLE, WHITE,    _( "Show footprint text marked as invisible" ) ),
    RR( _( "Pads Front" ),           LAYER_PAD_FR,             WHITE,    _( "Show footprint pads on board's front" ) ),
    RR( _( "Pads Back" ),            LAYER_PAD_BK,             WHITE,    _( "Show footprint pads on board's back" ) ),
    RR( _( "Through Hole Pads" ),    LAYER_PADS_TH,            YELLOW,   _( "Show through hole pads in specific color") ),
    RR(),
    RR( _( "Tracks" ),               LAYER_TRACKS,             NOCOLOR,  _( "Show tracks" ) ),
    RR( _( "Through Via" ),          LAYER_VIA_THROUGH,        WHITE,    _( "Show through vias" ) ),
    RR( _( "Bl/Buried Via" ),        LAYER_VIA_BBLIND,         WHITE,    _( "Show blind or buried vias" )  ),
    RR( _( "Micro Via" ),            LAYER_VIA_MICROVIA,       WHITE,    _( "Show micro vias") ),
    RR( _( "Non Plated Holes" ),     LAYER_NON_PLATEDHOLES,    WHITE,    _( "Show non plated holes in specific color") ),
    RR(),
    RR( _( "Ratsnest" ),             LAYER_RATSNEST,           WHITE,    _( "Show unconnected nets as a ratsnest") ),
    RR( _( "No-Connects" ),          LAYER_NO_CONNECTS,        BLUE,     _( "Show a marker on pads which have no net connected" ) ),
    RR( _( "DRC Warnings" ),         LAYER_DRC_WARNING,        YELLOW,   _( "DRC violations with a Warning severity" ) ),
    RR( _( "DRC Errors" ),           LAYER_DRC_ERROR,          PURERED,  _( "DRC violations with an Error severity" ) ),
    RR( _( "Anchors" ),              LAYER_ANCHOR,             WHITE,    _( "Show footprint and text origins as a cross" ) ),
    RR( _( "Worksheet" ),            LAYER_WORKSHEET,          DARKRED,  _( "Show worksheet") ),
    RR( _( "Cursor" ),               LAYER_CURSOR,             WHITE,    _( "PCB Cursor" ), true, false ),
    RR( _( "Aux Items" ),            LAYER_AUX_ITEMS,          WHITE,    _( "Auxiliary items (rulers, assistants, axes, etc.)" ), true, false ),
    RR( _( "Grid" ),                 LAYER_GRID,               WHITE,    _( "Show the (x,y) grid dots" ) ),
    RR( _( "Background" ),           LAYER_PCB_BACKGROUND,     BLACK,    _( "PCB Background" ), true, false )
};

static int s_allowed_in_FpEditor[] =
{
    LAYER_MOD_TEXT_INVISIBLE,
    LAYER_NON_PLATEDHOLES,
    LAYER_PADS_TH,
    LAYER_PAD_FR,
    LAYER_PAD_BK,
    LAYER_MOD_VALUES,
    LAYER_MOD_REFERENCES,
    LAYER_CURSOR,
    LAYER_AUX_ITEMS,
    LAYER_GRID,
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


COLOR4D PCB_LAYER_WIDGET::getBackgroundLayerColor()
{
    return myframe->ColorSettings()->GetColor( LAYER_PCB_BACKGROUND );
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
    AddMenuItem( &menu, ID_SHOW_ALL_COPPER_LAYERS,
                 _( "Show All Copper Layers" ),
                 KiBitmap( select_layer_pair_xpm ) );
    AddMenuItem( &menu, ID_HIDE_ALL_COPPER_LAYERS,
                 _( "Hide All Copper Layers" ),
                 KiBitmap( show_no_copper_layers_xpm ) );

    menu.AppendSeparator();

    AddMenuItem( &menu, ID_HIDE_ALL_COPPER_LAYERS_BUT_ACTIVE,
                 _( "Hide All Copper Layers But Active" ),
                 KiBitmap( select_w_layer_xpm ) );
    AddMenuItem( &menu, ID_ALWAYS_HIDE_ALL_COPPER_LAYERS_BUT_ACTIVE,
                 _( "Always Hide All Copper Layers But Active" ),
                 KiBitmap( select_w_layer_xpm ) );

    menu.AppendSeparator();

    AddMenuItem( &menu, ID_SHOW_ALL_NON_COPPER,
                 _( "Show All Non Copper Layers" ),
                 KiBitmap( select_w_layer_xpm ) );
    AddMenuItem( &menu, ID_HIDE_ALL_NON_COPPER,
                 _( "Hide All Non Copper Layers" ),
                 KiBitmap( show_no_copper_layers_xpm ) );

    menu.AppendSeparator();

    AddMenuItem( &menu, ID_SHOW_ALL_LAYERS, _( "Show All Layers" ),
                 KiBitmap( show_all_layers_xpm ) );
    AddMenuItem( &menu, ID_SHOW_NO_LAYERS, _( "Hide All Layers" ),
                 KiBitmap( show_no_layers_xpm ) );

    menu.AppendSeparator();

    AddMenuItem( &menu, ID_SHOW_ONLY_FRONT, _( "Show Only Front Layers" ),
                 KiBitmap( show_all_front_layers_xpm ) );

    // Only show the internal layer option if internal layers are enabled
    if( myframe->GetBoard()->GetCopperLayerCount() > 2 )
    {
        AddMenuItem( &menu, ID_SHOW_ONLY_INNER, _( "Show Only Inner Layers" ),
                     KiBitmap( show_all_copper_layers_xpm ) );
    }

    AddMenuItem( &menu, ID_SHOW_ONLY_BACK, _( "Show Only Back Layers" ),
                 KiBitmap( show_all_back_layers_xpm ) );
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
    // Force the active layer to be visible
    bool forceActiveLayer = false;

    // Reset the always show property
    m_alwaysShowActiveCopperLayer = false;

    // Make a distinction between the layers we want to enable and those we
    // want to disable explictly. That way we can either or the current layerset
    // or operate on all layers.
    LSET layersToShow;
    LSET layersToHide;

    switch( event.GetId() )
    {
        case ID_SHOW_NO_LAYERS:
            layersToHide = LSET::AllLayersMask();
            break;

        case ID_SHOW_ALL_LAYERS:
            layersToShow = LSET::AllLayersMask();
            break;

        case ID_SHOW_ALL_COPPER_LAYERS:
            layersToShow = LSET::AllCuMask();
            break;

        case ID_ALWAYS_HIDE_ALL_COPPER_LAYERS_BUT_ACTIVE:
            m_alwaysShowActiveCopperLayer = true;
            KI_FALLTHROUGH;

        case ID_HIDE_ALL_COPPER_LAYERS_BUT_ACTIVE:
            forceActiveLayer = true;
            KI_FALLTHROUGH;

        case ID_HIDE_ALL_COPPER_LAYERS:
            layersToHide = LSET::AllCuMask();
            break;

        case ID_HIDE_ALL_NON_COPPER:
            layersToHide = LSET::AllNonCuMask();
            break;

        case ID_SHOW_ALL_NON_COPPER:
            layersToShow = LSET::AllNonCuMask();
            break;

        case ID_SHOW_ONLY_FRONT:
            // Include the edgecuts layer as well as the front layers and hide the other layers
            layersToShow = LSET::FrontMask().set( Edge_Cuts );
            layersToHide = ~layersToShow;
            break;

        case ID_SHOW_ONLY_INNER:
            // Include the edgecuts layer as well as the internal layers and hide the other layers
            layersToShow = LSET::InternalCuMask().set( Edge_Cuts );
            layersToHide = ~layersToShow;
            break;

        case ID_SHOW_ONLY_BACK:
            // Include the edgecuts layer as well as the back layers and hide the other layers
            layersToShow = LSET::BackMask().set( Edge_Cuts );
            layersToHide = ~layersToShow;
            break;
    }

    int  rowCount = GetLayerRowCount();

    for( int row = 0; row < rowCount;  ++row )
    {
        wxCheckBox*  cb    = static_cast<wxCheckBox*>( getLayerComp( row, COLUMN_COLOR_LYR_CB ) );
        PCB_LAYER_ID layer = ToLAYER_ID( getDecodedId( cb->GetId() ) );

        bool visible = cb->GetValue();

        if( layersToShow.Contains( layer ) )
            visible = true;

        if( layersToHide.Contains( layer ) )
            visible = false;

        // Force the active layer in the editor to be visible
        if( forceActiveLayer && ( layer == myframe->GetActiveLayer() ) )
            visible = true;

        cb->SetValue( visible );
        OnLayerVisible( layer, visible, false );
    }

    // Refresh the drawing canvas
    myframe->GetCanvas()->Refresh();
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

        // Don't remove microvia and bblind vias if they're not allowed: that's only a DRC
        // setting (which might be set to ignore) and the user can add them irrespective of
        // the setting.
        /*
        if( renderRow.id == LAYER_VIA_MICROVIA && !settings.m_MicroViasAllowed )
            continue;

        if( renderRow.id == LAYER_VIA_BBLIND && !settings.m_BlindBuriedViaAllowed )
            continue;
        */

        if( !renderRow.spacer )
        {
            renderRow.tooltip = wxGetTranslation( s_render_rows[row].tooltip );
            renderRow.rowName = wxGetTranslation( s_render_rows[row].rowName );

            if( renderRow.color != COLOR4D::UNSPECIFIED )       // does this row show a color?
            {
                // this window frame must have an established BOARD, i.e. after SetBoard()
                renderRow.color = myframe->ColorSettings()->GetColor(
                        static_cast<GAL_LAYER_ID>( renderRow.id ) );
                renderRow.defaultColor = myframe->ColorSettings()->GetDefaultColor(
                        static_cast<GAL_LAYER_ID>( renderRow.id ) );
            }

            if( renderRow.id == LAYER_RATSNEST )
                renderRow.state = myframe->GetDisplayOptions().m_ShowGlobalRatsnest;
            else if( renderRow.id == LAYER_GRID )
                renderRow.state = myframe->IsGridVisible();
            else
                renderRow.state = board->IsElementVisible(
                        static_cast<GAL_LAYER_ID>( renderRow.id ) );
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

        AppendLayerRow( LAYER_WIDGET::ROW( brd->GetLayerName( layer ), layer,
                myframe->ColorSettings()->GetColor( layer ), dsc, true, true,
                myframe->ColorSettings()->GetDefaultColor( layer ) ) );

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

        AppendLayerRow( LAYER_WIDGET::ROW( brd->GetLayerName( layer ), layer,
                myframe->ColorSettings()->GetColor( layer ),
                wxGetTranslation( non_cu_seq[i].tooltip ), true, true,
                myframe->ColorSettings()->GetDefaultColor( layer ) ) );

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
    COLOR_SETTINGS* cs = myframe->ColorSettings();
    cs->SetColor( aLayer, aColor );

    myframe->GetCanvas()->UpdateColors();

    KIGFX::VIEW* view = myframe->GetCanvas()->GetView();
    view->UpdateLayerColor( aLayer );
    view->UpdateLayerColor( GetNetnameLayer( aLayer ) );

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

    if( m_alwaysShowActiveCopperLayer )
        OnLayerSelected();
    else if( myframe->GetDisplayOptions().m_ContrastModeDisplay )
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
    event.SetId( ID_ALWAYS_HIDE_ALL_COPPER_LAYERS_BUT_ACTIVE );
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

        // Layer visibility is not stored in .kicad_mod files
        if( !m_fp_editor_mode )
            myframe->OnModify();

        if( myframe->GetCanvas() )
            myframe->GetCanvas()->GetView()->SetLayerVisible( aLayer, isVisible );
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

    myframe->ColorSettings()->SetColor( aId, aColor );
    myframe->GetCanvas()->UpdateColors();

    KIGFX::VIEW* view = myframe->GetCanvas()->GetView();
    view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );   // useful to update rastnest
    view->UpdateLayerColor( aId );

    // plated-through-holes don't have their own color; they use the background color
    if( aId == LAYER_PCB_BACKGROUND )
        view->UpdateLayerColor( LAYER_PADS_PLATEDHOLES );

    myframe->ReCreateHToolbar();
    myframe->GetCanvas()->ForceRefresh();
    myframe->GetCanvas()->Refresh();
}


void PCB_LAYER_WIDGET::OnRenderEnable( int aId, bool isEnabled )
{
    BOARD*  brd = myframe->GetBoard();
    wxASSERT( aId > GAL_LAYER_ID_START && aId < GAL_LAYER_ID_END );

    if( myframe->IsType( FRAME_PCB_EDITOR ) )
    {
        // The layer visibility status is saved in the board file so set the board
        // modified state so the user has the option to save the changes.
        if( brd->IsElementVisible( static_cast<GAL_LAYER_ID>( aId ) ) != isEnabled )
            myframe->OnModify();
    }

    // Grid is not set through the board visibility
    if( aId == LAYER_GRID )
        myframe->SetGridVisibility( isEnabled );
    else
        brd->SetElementVisibility( static_cast<GAL_LAYER_ID>( aId ), isEnabled );

    if( aId == LAYER_RATSNEST )
    {
        // don't touch the layers. ratsnest is enabled on per-item basis.
        myframe->GetCanvas()->GetView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
        myframe->GetCanvas()->GetView()->SetLayerVisible( aId, true );

        if( myframe->IsType( FRAME_PCB_EDITOR ) )
        {
            PCB_DISPLAY_OPTIONS opt = myframe->GetDisplayOptions();
            opt.m_ShowGlobalRatsnest = isEnabled;
            myframe->GetCanvas()->GetView()->UpdateDisplayOptions( opt );
        }
    }
    else if( aId != LAYER_GRID )
        myframe->GetCanvas()->GetView()->SetLayerVisible( aId, isEnabled );

    myframe->GetCanvas()->Refresh();
    myframe->GetCanvas()->Refresh();
}

//-----</LAYER_WIDGET callbacks>------------------------------------------
