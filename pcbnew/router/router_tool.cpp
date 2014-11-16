/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <wx/numdlg.h>

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/bind.hpp>

#include "class_draw_panel_gal.h"
#include "class_board.h"

#include <wxPcbStruct.h>
#include <id.h>
#include <macros.h>
#include <pcbnew_id.h>
#include <view/view_controls.h>
#include <pcbcommon.h>
#include <pcb_painter.h>
#include <dialogs/dialog_pns_settings.h>
#include <dialogs/dialog_track_via_size.h>
#include <base_units.h>

#include <tool/context_menu.h>
#include <tools/common_actions.h>

#include <ratsnest_data.h>

#include "router_tool.h"
#include "pns_segment.h"
#include "pns_router.h"
#include "trace.h"

using namespace KIGFX;
using boost::optional;

static TOOL_ACTION ACT_NewTrack( "pcbnew.InteractiveRouter.NewTrack",
                                 AS_CONTEXT, 'X',
                                 "New Track", "Starts laying a new track.");
static TOOL_ACTION ACT_EndTrack( "pcbnew.InteractiveRouter.EndTrack",
                                 AS_CONTEXT, WXK_END,
                                 "End Track", "Stops laying the current track.");
static TOOL_ACTION ACT_AutoEndRoute( "pcbnew.InteractiveRouter.AutoEndRoute",
                                     AS_CONTEXT, 'F',
                                     "Auto-end Track", "Automagically finishes currently routed track." );
static TOOL_ACTION ACT_Drag( "pcbnew.InteractiveRouter.Drag",
                                     AS_CONTEXT, 'G',
                                     "Drag Track/Via", "Drags a track or a via." );
static TOOL_ACTION ACT_PlaceThroughVia( "pcbnew.InteractiveRouter.PlaceVia",
                                 AS_CONTEXT, 'V',
                                 "Place Through Via", "Adds a through-hole via at the end of currently routed track." );
static TOOL_ACTION ACT_PlaceBlindVia( "pcbnew.InteractiveRouter.PlaceBlindVia",
                                 AS_CONTEXT, 'Z',
                                 "Place Blind/Buried Via", "Adds a blind or buried via at the end of currently routed track." );
static TOOL_ACTION ACT_PlaceMicroVia( "pcbnew.InteractiveRouter.PlaceMicroVia",
                                 AS_CONTEXT, 'Q',
                                 "Place Microvia", "Adds a microvia at the end of currently routed track." );
static TOOL_ACTION ACT_CustomTrackWidth( "pcbnew.InteractiveRouter.CustomTrackWidth",
                                      AS_CONTEXT, 'W',
                                      "Custom Track Width", "Shows a dialog for changing the track width and via size.");
static TOOL_ACTION ACT_RouterOptions( "pcbnew.InteractiveRouter.RouterOptions",
                                      AS_CONTEXT, 'E',
                                      "Routing Options...", "Shows a dialog containing router options.");
static TOOL_ACTION ACT_SwitchPosture( "pcbnew.InteractiveRouter.SwitchPosture",
                                      AS_CONTEXT, '/',
                                      "Switch Track Posture", "Switches posture of the currenly routed track.");

ROUTER_TOOL::ROUTER_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.InteractiveRouter" )
{
    m_router = NULL;
}


class CONTEXT_TRACK_WIDTH_MENU: public CONTEXT_MENU
{
public:
    CONTEXT_TRACK_WIDTH_MENU()
    {
        setCustomEventHandler( boost::bind( &CONTEXT_TRACK_WIDTH_MENU::handleCustomEvent,
                                            this, _1 ) );
    }

    void SetBoard( BOARD* aBoard )
    {
        BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

        wxString msg;
        m_board = aBoard;

        Append( ID_POPUP_PCB_SELECT_CUSTOM_WIDTH, _( "Custom size" ),
                wxEmptyString, wxITEM_CHECK );

        Append( ID_POPUP_PCB_SELECT_AUTO_WIDTH, _( "Use the starting track width" ),
                _( "Route using the width of the starting track." ), wxITEM_CHECK );

        Append( ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES, _( "Use netclass values" ),
                _( "Use track and via sizes from the net class" ), wxITEM_CHECK );

        for( unsigned i = 0; i < bds.m_TrackWidthList.size(); i++ )
        {
            msg = _( "Track ");
            msg << StringFromValue( g_UserUnit, bds.m_TrackWidthList[i], true );

            if( i == 0 )
                msg << _( " (from netclass)" );

            Append( ID_POPUP_PCB_SELECT_WIDTH1 + i, msg, wxEmptyString, wxITEM_CHECK );
        }

        AppendSeparator();

        for( unsigned i = 0; i < bds.m_ViasDimensionsList.size(); i++ )
        {
            msg = _("Via ");
            msg << StringFromValue( g_UserUnit, bds.m_ViasDimensionsList[i].m_Diameter, true );
            wxString drill = StringFromValue( g_UserUnit,
                                              bds.m_ViasDimensionsList[i].m_Drill,
                                              true );

            if( bds.m_ViasDimensionsList[i].m_Drill <= 0 )
            {
                msg << _ (", drill: default");
            }
            else
            {
                msg << _ (", drill: ") << drill;
            }

            if( i == 0 )
                msg << _( " (from netclass)" );

            Append( ID_POPUP_PCB_SELECT_VIASIZE1 + i, msg, wxEmptyString, wxITEM_CHECK );
        }
    }

protected:
    OPT_TOOL_EVENT handleCustomEvent( const wxEvent& aEvent )
    {
#if ID_POPUP_PCB_SELECT_VIASIZE1 < ID_POPUP_PCB_SELECT_WIDTH1
#error You have changed event ids order, it breaks code. Check the source code for more details.
// Recognising type of event (track width/via size) is based on comparison if the event id is
// within a specific range. If ranges of event ids changes, then the following is not valid anymore.
#endif
        BOARD_DESIGN_SETTINGS &bds = m_board->GetDesignSettings();

        int id = aEvent.GetId();

        // Initial settings, to be modified below
        bds.m_UseConnectedTrackWidth = false;
        bds.UseCustomTrackViaSize( false );

        if( id == ID_POPUP_PCB_SELECT_CUSTOM_WIDTH )
        {
            bds.UseCustomTrackViaSize( true );
        }

        else if( id == ID_POPUP_PCB_SELECT_AUTO_WIDTH )
        {
            bds.m_UseConnectedTrackWidth = true;
        }

        else if( id == ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES )
        {
            bds.SetViaSizeIndex( 0 );
            bds.SetTrackWidthIndex( 0 );
        }

        else if( id > ID_POPUP_PCB_SELECT_VIASIZE1 )     // via size has changed
        {
            assert( id < ID_POPUP_PCB_SELECT_WIDTH_END_RANGE );

            bds.SetViaSizeIndex( id - ID_POPUP_PCB_SELECT_VIASIZE1 );
        }

        else    // track width has changed
        {
            assert( id >= ID_POPUP_PCB_SELECT_WIDTH1 );
            assert( id < ID_POPUP_PCB_SELECT_VIASIZE );

            bds.SetTrackWidthIndex( id - ID_POPUP_PCB_SELECT_WIDTH1 );
        }

        return OPT_TOOL_EVENT( COMMON_ACTIONS::trackViaSizeChanged.MakeEvent() );
    }

    BOARD* m_board;
};


class ROUTER_TOOL_MENU: public CONTEXT_MENU
{
public:
    ROUTER_TOOL_MENU( BOARD* aBoard )
    {
        SetTitle( wxT( "Interactive Router" ) );
        Add( ACT_NewTrack );
        Add( ACT_EndTrack );
//        Add( ACT_AutoEndRoute );  // fixme: not implemented yet. Sorry.
        Add( ACT_Drag );
        Add( ACT_PlaceThroughVia );
        Add( ACT_PlaceBlindVia );
        Add( ACT_PlaceMicroVia );
        Add( ACT_SwitchPosture );

        AppendSeparator();

        CONTEXT_TRACK_WIDTH_MENU* trackMenu = new CONTEXT_TRACK_WIDTH_MENU;
        trackMenu->SetBoard( aBoard );
        AppendSubMenu( trackMenu, wxT( "Select Track Width" ) );

        Add( ACT_CustomTrackWidth );

        AppendSeparator();
        Add( ACT_RouterOptions );
    }
};


ROUTER_TOOL::~ROUTER_TOOL()
{
    delete m_router;
}


void ROUTER_TOOL::Reset( RESET_REASON aReason )
{
    if( m_router )
        delete m_router;

    m_router = new PNS_ROUTER;

    TRACEn( 0, "Reset" );
    m_router->ClearWorld();
    m_router->SetBoard( getModel<BOARD>() );
    m_router->SyncWorld();
    m_router->LoadSettings( m_settings );
    m_needsSync = false;

    if( getView() )
        m_router->SetView( getView() );

    Go( &ROUTER_TOOL::Main, COMMON_ACTIONS::routerActivate.MakeEvent() );
}


int ROUTER_TOOL::getDefaultWidth( int aNetCode )
{
    int w, d1, d2;

    getNetclassDimensions( aNetCode, w, d1, d2 );

    return w;
}


void ROUTER_TOOL::getNetclassDimensions( int aNetCode, int& aWidth,
                                         int& aViaDiameter, int& aViaDrill )
{
    BOARD* board = getModel<BOARD>();
    BOARD_DESIGN_SETTINGS &bds = board->GetDesignSettings();

    NETCLASSPTR netClass;
    NETINFO_ITEM* ni = board->FindNet( aNetCode );

    if( ni )
    {
        wxString netClassName = ni->GetClassName();
        netClass = bds.m_NetClasses.Find( netClassName );
    }

    if( !netClass )
        netClass = bds.GetDefault();

    aWidth = netClass->GetTrackWidth();
    aViaDiameter = netClass->GetViaDiameter();
    aViaDrill = netClass->GetViaDrill();
}


PNS_ITEM* ROUTER_TOOL::pickSingleItem( const VECTOR2I& aWhere, int aNet, int aLayer )
{
    int tl = getView()->GetTopLayer();

    if( aLayer > 0 )
        tl = aLayer;

    PNS_ITEM* prioritized[4];

    for( int i = 0; i < 4; i++ )
        prioritized[i] = 0;

    PNS_ITEMSET candidates = m_router->QueryHoverItems( aWhere );

    BOOST_FOREACH( PNS_ITEM* item, candidates.Items() )
    {
        if( !IsCopperLayer( item->Layers().Start() ) )
            continue;

        // fixme: this causes flicker with live loop removal...
        //if( item->Parent() && !item->Parent()->ViewIsVisible() )
        //    continue;

        if( aNet < 0 || item->Net() == aNet )
        {
            if( item->OfKind( PNS_ITEM::VIA | PNS_ITEM::SOLID ) )
            {
                if( !prioritized[2] )
                    prioritized[2] = item;
                if( item->Layers().Overlaps( tl ) )
                    prioritized[0] = item;
            }
            else
            {
                if( !prioritized[3] )
                    prioritized[3] = item;
                if( item->Layers().Overlaps( tl ) )
                    prioritized[1] = item;
            }
        }
    }

    PNS_ITEM* rv = NULL;
    for( int i = 0; i < 4; i++ )
    {
        PNS_ITEM* item = prioritized[i];

        if( DisplayOpt.ContrastModeDisplay )
            if( item && !item->Layers().Overlaps( tl ) )
                item = NULL;

        if( item )
        {
            rv = item;
            break;
        }
    }

    if( rv && aLayer >= 0 && !rv->Layers().Overlaps( aLayer ) )
        rv = NULL;

    if( rv )
        TRACE( 0, "%s, layer : %d, tl: %d", rv->KindStr().c_str() % rv->Layers().Start() % tl );

    return rv;
}


void ROUTER_TOOL::highlightNet( bool aEnabled, int aNetcode )
{
    RENDER_SETTINGS* rs = getView()->GetPainter()->GetSettings();

    if( aNetcode >= 0 && aEnabled )
        rs->SetHighlight( true, aNetcode );
    else
        rs->SetHighlight( false );

    getView()->UpdateAllLayersColor();
}


void ROUTER_TOOL::handleCommonEvents( TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME> ();
    BOARD* board = getModel<BOARD> ();

#ifdef DEBUG
    if( aEvent.IsKeyPressed() )
    {
        switch( aEvent.KeyCode() )
        {
            case 'S':
                TRACEn( 2, "saving drag/route log...\n" );
                m_router->DumpLog();
                break;
        }
    }
    else
#endif
    if( aEvent.IsAction( &ACT_RouterOptions ) )
    {
        DIALOG_PNS_SETTINGS settingsDlg( frame, m_router->Settings() );

        if( settingsDlg.ShowModal() )
	    {
            // FIXME: do we need an explicit update?
        }
    }

    else if( aEvent.IsAction( &ACT_CustomTrackWidth ) )
    {
        BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
        DIALOG_TRACK_VIA_SIZE sizeDlg( frame, bds );

        if( sizeDlg.ShowModal() )
        {
            bds.UseCustomTrackViaSize( true );
            m_toolMgr->RunAction( COMMON_ACTIONS::trackViaSizeChanged );
        }
    }

    else if( aEvent.IsAction( &COMMON_ACTIONS::trackViaSizeChanged ) )
    {

        PNS_SIZES_SETTINGS sizes;
        sizes.ImportCurrent ( board->GetDesignSettings() );
        m_router->UpdateSizes ( sizes );
    }
}


void ROUTER_TOOL::updateStartItem( TOOL_EVENT& aEvent )
{
    VIEW_CONTROLS* ctls = getViewControls();
    int tl = getView()->GetTopLayer();
    VECTOR2I cp = ctls->GetCursorPosition();
    PNS_ITEM* startItem = NULL;

    if( aEvent.IsMotion() || aEvent.IsClick() )
    {
        VECTOR2I p = aEvent.Position();
        startItem = pickSingleItem( p );
        bool snapEnabled = !aEvent.Modifier( MD_SHIFT );
        m_router->EnableSnapping ( snapEnabled );

        if( !snapEnabled && startItem && !startItem->Layers().Overlaps( tl ) )
            startItem = NULL;

        if( startItem && startItem->Net() >= 0 )
        {
            bool dummy;
            VECTOR2I psnap = m_router->SnapToItem( startItem, p, dummy );

            if( snapEnabled )
            {
                m_startSnapPoint = psnap;
                ctls->ForceCursorPosition( true, psnap );
            }
            else
            {
                m_startSnapPoint = cp;
                ctls->ForceCursorPosition( false );
            }

//            if( startItem->Layers().IsMultilayer() )
//                m_startLayer = tl;
//            else
//                m_startLayer = startItem->Layers().Start();

            m_startItem = startItem;
        }
        else
        {
            m_startItem = NULL;
            m_startSnapPoint = cp;
            ctls->ForceCursorPosition( false );
        }
    }
}


void ROUTER_TOOL::updateEndItem( TOOL_EVENT& aEvent )
{
    VIEW_CONTROLS* ctls = getViewControls();
    VECTOR2I p = getView()->ToWorld( ctls->GetMousePosition() );
    VECTOR2I cp = ctls->GetCursorPosition();
    int layer;
    bool snapEnabled = !aEvent.Modifier( MD_SHIFT );

    m_router->EnableSnapping( snapEnabled );

    if( !snapEnabled || m_router->GetCurrentNet() < 0 || !m_startItem )
    {
        m_endItem = NULL;
        m_endSnapPoint = cp;
        return;
    }

    bool dummy;

    if( m_router->IsPlacingVia() )
        layer = -1;
    else
        layer = m_router->GetCurrentLayer();

    PNS_ITEM* endItem = pickSingleItem( p, m_startItem->Net(), layer );

    if( endItem )
    {
        VECTOR2I cursorPos = m_router->SnapToItem( endItem, p, dummy );
        ctls->ForceCursorPosition( true, cursorPos );
        m_endItem = endItem;
        m_endSnapPoint = cursorPos;
    }
    else
    {
        m_endItem = NULL;
        m_endSnapPoint = cp;
        ctls->ForceCursorPosition( false );
    }

    if( m_endItem )
        TRACE( 0, "%s, layer : %d", m_endItem->KindStr().c_str() % m_endItem->Layers().Start() );
}

int ROUTER_TOOL::getStartLayer( const PNS_ITEM* aItem )
{
    int tl = getView()->GetTopLayer();

    if( m_startItem )
    {
        const PNS_LAYERSET& ls = m_startItem->Layers();

        if( ls.Overlaps( tl ) )
            return tl;
        else
            return ls.Start();
    }

    return tl;
}
void ROUTER_TOOL::switchLayerOnViaPlacement()
{
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();

    int al = frame->GetActiveLayer();
    int cl = m_router->GetCurrentLayer();

    if( cl != al )
    {
        m_router->SwitchLayer( al );
    }

    optional<int> newLayer = m_router->Sizes().PairedLayer( cl );

    if( newLayer )
    {
        m_router->SwitchLayer ( *newLayer );
        frame->SetActiveLayer ( ToLAYER_ID( *newLayer ) );
    }
}

bool ROUTER_TOOL::onViaCommand( VIATYPE_T aType )
{
    BOARD* board = getModel<BOARD> ();
    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();

    const int layerCount = bds.GetCopperLayerCount();
    int currentLayer = m_router->GetCurrentLayer();

    PNS_SIZES_SETTINGS sizes = m_router->Sizes();

    sizes.ClearLayerPairs();
    sizes.AddLayerPair( frame->GetScreen()->m_Route_Layer_TOP,
                        frame->GetScreen()->m_Route_Layer_BOTTOM );

    if( !m_router->IsPlacingVia() )
    {
        // Cannot place microvias or blind vias if not allowed (obvious)
        if( ( aType == VIA_BLIND_BURIED ) && ( !bds.m_BlindBuriedViaAllowed ) )
            return false;
        if( ( aType == VIA_MICROVIA ) && ( !bds.m_MicroViasAllowed ) )
            return false;

        //Can only place through vias on 2-layer boards
        if( ( aType != VIA_THROUGH ) && ( layerCount <= 2 ) )
            return false;

        //Can only place microvias if we're on an outer layer, or directly adjacent to one
        if( ( aType == VIA_MICROVIA ) && ( currentLayer > In1_Cu ) && ( currentLayer < layerCount-2 ) )
            return false;

        //Cannot place blind vias with front/back as the layer pair, this doesn't make sense
        if( ( aType == VIA_BLIND_BURIED ) && ( sizes.GetLayerTop() == F_Cu ) && ( sizes.GetLayerBottom() == B_Cu ) )
            return false;
    }


    sizes.SetViaType ( aType );
    m_router->ToggleViaPlacement( );
    m_router->UpdateSizes( sizes );

    m_router->Move( m_endSnapPoint, m_endItem );        // refresh

    return false;
}

void ROUTER_TOOL::performRouting()
{
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();
    bool saveUndoBuffer = true;
    VIEW_CONTROLS* ctls = getViewControls();
    BOARD* board = getModel<BOARD>();

	int routingLayer = getStartLayer ( m_startItem );
    frame->SetActiveLayer( ToLAYER_ID ( routingLayer ) );
    // fixme: switch on invisible layer

    if( m_startItem && m_startItem->Net() >= 0 )
    {
        highlightNet( true, m_startItem->Net() );
        // Update track width and via size shown in main toolbar comboboxes
        frame->SetCurrentNetClass( m_startItem->Parent()->GetNetClass()->GetName() );
    }
    else
        frame->SetCurrentNetClass( NETCLASS::Default );

    ctls->ForceCursorPosition( false );
    ctls->SetAutoPan( true );

    PNS_SIZES_SETTINGS sizes;
    sizes.Init ( board, m_startItem );
    sizes.AddLayerPair ( frame->GetScreen()->m_Route_Layer_TOP,
                         frame->GetScreen()->m_Route_Layer_BOTTOM );
    m_router->UpdateSizes( sizes );

    m_router->StartRouting( m_startSnapPoint, m_startItem, routingLayer );

    m_endItem = NULL;
    m_endSnapPoint = m_startSnapPoint;

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() || evt->IsActivate() )
            break;
        else if( evt->Action() == TA_UNDO_REDO )
        {
            saveUndoBuffer = false;
            break;
        }
        else if( evt->IsMotion() )
        {
            updateEndItem( *evt );
            m_router->Move( m_endSnapPoint, m_endItem );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            updateEndItem( *evt );
            bool needLayerSwitch = m_router->IsPlacingVia();

            if( m_router->FixRoute( m_endSnapPoint, m_endItem ) )
                break;

            if( needLayerSwitch )
            {
                switchLayerOnViaPlacement();
            }

            // Synchronize the indicated layer
            frame->SetActiveLayer( ToLAYER_ID( m_router->GetCurrentLayer() ) );

            m_router->Move( m_endSnapPoint, m_endItem );
        }
        else if( evt->IsAction( &ACT_PlaceThroughVia ) )
        {
            onViaCommand ( VIA_THROUGH );
        }
        else if( evt->IsAction( &ACT_PlaceBlindVia ) )
        {
            onViaCommand ( VIA_BLIND_BURIED );
        }
        else if( evt->IsAction( &ACT_PlaceMicroVia ) )
        {
            onViaCommand ( VIA_MICROVIA );
        }
        else if( evt->IsAction( &ACT_SwitchPosture ) )
        {
            m_router->FlipPosture();
            m_router->Move( m_endSnapPoint, m_endItem );        // refresh
        }
        else if( evt->IsAction( &COMMON_ACTIONS::layerChanged ) )
        {
            updateEndItem( *evt );
            m_router->SwitchLayer( frame->GetActiveLayer() );
            m_router->Move( m_endSnapPoint, m_endItem );        // refresh
        }
        else if( evt->IsAction( &ACT_EndTrack ) )
        {
            if( m_router->FixRoute( m_endSnapPoint, m_endItem ) )
                break;
        }

        handleCommonEvents( *evt );
    }

    m_router->StopRouting();

    if( saveUndoBuffer )
    {
        // Save the recent changes in the undo buffer
        frame->SaveCopyInUndoList( m_router->GetUndoBuffer(), UR_UNSPECIFIED );
        m_router->ClearUndoBuffer();
        frame->OnModify();
    }
    else
    {
        // It was interrupted by TA_UNDO_REDO event, so we have to sync the world now
        m_needsSync = true;
    }

    ctls->SetAutoPan( false );
    ctls->ForceCursorPosition( false );
    highlightNet( false );
}


int ROUTER_TOOL::Main( TOOL_EVENT& aEvent )
{
    VIEW_CONTROLS* ctls = getViewControls();
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();
    BOARD* board = getModel<BOARD>();

    // Deselect all items
    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    frame->SetToolID( ID_TRACK_BUTT, wxCURSOR_PENCIL, _( "Interactive Router" ) );

    ctls->SetSnapping( true );
    ctls->ShowCursor( true );

    std::auto_ptr<ROUTER_TOOL_MENU> ctxMenu ( new ROUTER_TOOL_MENU( board ) );
    SetContextMenu ( ctxMenu.get() );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( m_needsSync )
        {
            m_router->SyncWorld();
            m_router->SetView( getView() );
            m_needsSync = false;
        }

        if( evt->IsCancel() || evt->IsActivate() )
            break; // Finish
        else if( evt->Action() == TA_UNDO_REDO )
            m_needsSync = true;
        else if( evt->IsMotion() )
            updateStartItem( *evt );
        else if( evt->IsClick( BUT_LEFT ) || evt->IsAction( &ACT_NewTrack ) )
        {
            updateStartItem( *evt );

            if( evt->Modifier( MD_CTRL ) )
                performDragging();
            else
                performRouting();
        }
        else if( evt->IsAction( &ACT_Drag ) )
            performDragging();

        handleCommonEvents( *evt );
    }

    // Restore the default settings
    ctls->SetAutoPan( false );
    ctls->ShowCursor( false );
    frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    // Store routing settings till the next invocation
    m_settings = m_router->Settings();

    return 0;
}


void ROUTER_TOOL::performDragging()
{
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME>();
    bool saveUndoBuffer = true;
    VIEW_CONTROLS* ctls = getViewControls();

    bool dragStarted = m_router->StartDragging( m_startSnapPoint, m_startItem );

    if( !dragStarted )
        return;

    if( m_startItem && m_startItem->Net() >= 0 )
        highlightNet( true, m_startItem->Net() );

    ctls->ForceCursorPosition( false );
    ctls->SetAutoPan( true );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() || evt->IsActivate() )
            break;
        else if( evt->Action() == TA_UNDO_REDO )
        {
            saveUndoBuffer = false;
            break;
        }
        else if( evt->IsMotion() )
        {
            updateEndItem( *evt );
            m_router->Move( m_endSnapPoint, m_endItem );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( m_router->FixRoute( m_endSnapPoint, m_endItem ) )
                break;
        }

        handleCommonEvents( *evt );
    }

    if( m_router->RoutingInProgress() )
        m_router->StopRouting();

    if( saveUndoBuffer )
    {
        // Save the recent changes in the undo buffer
        frame->SaveCopyInUndoList( m_router->GetUndoBuffer(), UR_UNSPECIFIED );
        m_router->ClearUndoBuffer();
        frame->OnModify();
    }
    else
    {
        // It was interrupted by TA_UNDO_REDO event, so we have to sync the world now
        m_needsSync = true;
    }

    ctls->SetAutoPan( false );
    ctls->ForceCursorPosition( false );
    highlightNet( false );
}
