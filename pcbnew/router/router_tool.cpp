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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "class_draw_panel_gal.h"
#include "class_board_item.h"
#include "class_board.h"

#include <wxPcbStruct.h>
#include <id.h>
#include <pcbnew_id.h>
#include <view/view_controls.h>
#include <pcbcommon.h>
#include <pcb_painter.h>

#include <tool/context_menu.h>
#include <tools/common_actions.h>

#include <ratsnest_data.h>

#include "router_tool.h"
#include "pns_segment.h"
#include "pns_router.h"
#include "trace.h"

using namespace KIGFX;
using boost::optional;

//static TOOL_ACTION ACT_AutoEndRoute( "pcbnew.InteractiveRouter.AutoEndRoute", AS_CONTEXT, 'G' );
//static TOOL_ACTION ACT_PlaceVia( "pcbnew.InteractiveRouter.PlaceVia", AS_CONTEXT, 'V' );
//static TOOL_ACTION ACT_OpenRouteOptions( "pcbnew.InteractiveRouter.OpenRouterOptions", AS_CONTEXT, 'T' );
//static TOOL_ACTION ACT_SwitchPosture( "pcbnew.InteractiveRouter.SwitchPosture", AS_CONTEXT, '/' );
//static TOOL_ACTION ACT_EndTrack( "pcbnew.InteractiveRouter.EndTrack", AS_CONTEXT, WXK_END );

ROUTER_TOOL::ROUTER_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.InteractiveRouter" )
{
    m_router = NULL;
    m_menu = new CONTEXT_MENU;

    m_menu->SetTitle( wxT( "Interactive router" ) );    // fixme: not implemented yet. Sorry.
    m_menu->Add( wxT( "Cancel" ), 1 );
    m_menu->Add( wxT( "New track" ), 2 );
    m_menu->Add( wxT( "End track" ), 3 );
    m_menu->Add( wxT( "Auto-end track" ), 4 );
    m_menu->Add( wxT( "Place via" ), 5 );
    m_menu->Add( wxT( "Switch posture" ), 6 );

    m_menu->Add( wxT( "Routing options..." ), 7 );
}


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
    m_router->SetBoard( getModel<BOARD>( PCB_T ) );
    m_router->SyncWorld();
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
    BOARD* board = getModel<BOARD>( PCB_T );

    NETCLASS* netClass = NULL;
    NETINFO_ITEM* ni = board->FindNet( aNetCode );

    if( ni )
    {
        wxString netClassName = ni->GetClassName();
        netClass = board->GetDesignSettings().m_NetClasses.Find( netClassName );
    }

    if( !netClass )
        netClass = board->GetDesignSettings().m_NetClasses.GetDefault();

    aWidth = netClass->GetTrackWidth();
    aViaDiameter = netClass->GetViaDiameter();
    aViaDrill = netClass->GetViaDrill();
}


PNS_ITEM* ROUTER_TOOL::pickSingleItem( const VECTOR2I& aWhere, int aNet, int aLayer )
{
    int tl = getView()->GetTopLayer();

    if( aLayer > 0 )
        tl = aLayer;

    PNS_ITEM* picked_seg = NULL;
    PNS_ITEM* picked_via = NULL;
    PNS_ITEMSET candidates = m_router->QueryHoverItems( aWhere );

    BOOST_FOREACH( PNS_ITEM* item, candidates.Items() )
    {
        if( !IsCopperLayer( item->GetLayers().Start() ) )
            continue;

        if( item->GetParent() && !item->GetParent()->ViewIsVisible() &&
                                 !item->GetParent()->IsSelected() )
            continue;

        if( aNet < 0 || item->GetNet() == aNet )
        {
            if( item->OfKind( PNS_ITEM::VIA | PNS_ITEM::SOLID ) )
            {
                if( item->GetLayers().Overlaps( tl ) || !picked_via )
                    picked_via = item;
            }
            else
            {
                if( item->GetLayers().Overlaps( tl ) || !picked_seg )
                    picked_seg = item;
            }
        }
    }

    if( DisplayOpt.ContrastModeDisplay )
    {
        if( picked_seg && !picked_seg->GetLayers().Overlaps( tl ) )
            picked_seg = NULL;
    }

    PNS_ITEM* rv = picked_via ? picked_via : picked_seg;

    if( rv && aLayer >= 0 && !rv->GetLayers().Overlaps( aLayer ) )
        rv = NULL;

    if( rv )
        TRACE( 0, "%s, layer : %d, tl: %d", rv->GetKindStr().c_str() % rv->GetLayers().Start() %
                tl );

    return rv;
}


void ROUTER_TOOL::setMsgPanel( bool aEnabled, int aEntry,
        const wxString& aUpperMessage, const wxString& aLowerMessage )
{
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME> ();

    if( m_panelItems.size() <= (unsigned int) aEntry )
        m_panelItems.resize( aEntry + 1 );

    m_panelItems[aEntry] = MSG_PANEL_ITEM( aUpperMessage, aLowerMessage, BLACK );
    frame->SetMsgPanel( m_panelItems );
}


void ROUTER_TOOL::clearMsgPanel()
{
    PCB_EDIT_FRAME* frame = getEditFrame<PCB_EDIT_FRAME> ();

    frame->ClearMsgPanel();
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


void ROUTER_TOOL::updateStartItem( TOOL_EVENT& aEvent )
{
    VIEW_CONTROLS* ctls = getViewControls();
    int tl = getView()->GetTopLayer();
    PNS_ITEM* startItem = NULL;

    if( aEvent.IsMotion() || aEvent.IsClick() )
    {
        VECTOR2I p = aEvent.Position();
        startItem = pickSingleItem( p );

        if( startItem && startItem->GetNet() >= 0 )
        {
            bool dummy;
            VECTOR2I cursorPos = m_router->SnapToItem( startItem, p, dummy );
            ctls->ForceCursorPosition( true, cursorPos );

            m_startSnapPoint = cursorPos;

            if( startItem->GetLayers().IsMultilayer() )
                m_startLayer = tl;
            else
                m_startLayer = startItem->GetLayers().Start();

            m_startItem = startItem;
        }
        else
        {
            m_startItem = NULL;
            m_startSnapPoint = p;
            m_startLayer = tl;
            ctls->ForceCursorPosition( false );
        }
    }
}


void ROUTER_TOOL::updateEndItem( TOOL_EVENT& aEvent )
{
    VIEW_CONTROLS* ctls = getViewControls();
    VECTOR2I p = aEvent.Position();
    int layer;

    if( m_router->GetCurrentNet() < 0 || !m_startItem )
    {
        m_endItem = NULL;
        m_endSnapPoint = p;
        return;
    }

    bool dummy;

    if( m_router->IsPlacingVia() )
        layer = -1;
    else
        layer = m_router->GetCurrentLayer();

    PNS_ITEM* endItem = pickSingleItem( p, m_startItem->GetNet(), layer );

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
        m_endSnapPoint = ctls->GetCursorPosition();
        ctls->ForceCursorPosition( false );
    }

    // Draw ratsnest for the currently routed track
    RN_DATA* ratsnest = getModel<BOARD>( PCB_T )->GetRatsnest();
    ratsnest->ClearSimple();

    if( ( m_endItem == NULL || m_endItem == m_startItem ) && m_startItem->GetNet() > 0 )
    {
        // The ending node has to be first, so the line for the track is drawn first
        ratsnest->AddSimple( m_endSnapPoint, m_startItem->GetNet() );

        // Those nodes are added just to force ratsnest not to drawn
        // lines to already routed parts of the track
        const PICKED_ITEMS_LIST& changes = m_router->GetLastChanges();
        for( unsigned int i = 0; i < changes.GetCount(); ++i )
        {
            // Block the new tracks, do not handle tracks that were moved
            // (moved tracks are saved in the undo buffer with UR_DELETED status instead)
            if( changes.GetPickedItemStatus( i ) == UR_NEW )
                ratsnest->AddBlocked( static_cast<BOARD_CONNECTED_ITEM*>( changes.GetPickedItem( i ) ) );
        }

        // Also the origin of the new track should be skipped in the ratsnest shown for the routed track
        ratsnest->AddBlocked( static_cast<BOARD_ITEM*>( m_startItem->GetParent() ) );
    }

    if( m_endItem )
        TRACE( 0, "%s, layer : %d", m_endItem->GetKindStr().c_str() %
                m_endItem->GetLayers().Start() );
}


void ROUTER_TOOL::startRouting()
{
    bool saveUndoBuffer = true;
    VIEW_CONTROLS* ctls = getViewControls();

    int width = getDefaultWidth( m_startItem ? m_startItem->GetNet() : -1 );

    if( m_startItem && m_startItem->OfKind( PNS_ITEM::SEGMENT ) )
        width = static_cast<PNS_SEGMENT*>( m_startItem )->GetWidth();

    m_router->SetCurrentWidth( width );
    m_router->SwitchLayer( m_startLayer );

    getEditFrame<PCB_EDIT_FRAME>()->SetTopLayer( m_startLayer );

    if( m_startItem && m_startItem->GetNet() >= 0 )
        highlightNet( true, m_startItem->GetNet() );

    ctls->ForceCursorPosition( false );
    ctls->SetAutoPan( true );

    m_router->StartRouting( m_startSnapPoint, m_startItem );

    m_endItem = NULL;
    m_endSnapPoint = m_startSnapPoint;

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() )
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

            if( m_router->FixRoute( m_endSnapPoint, m_endItem ) )
                break;

            m_router->Move( m_endSnapPoint, m_endItem );
        }
        else if( evt->IsKeyPressed() )
        {
            switch( std::toupper( evt->KeyCode() ) )
            {
            case 'V':
            {
                int w, diameter, drill;
                getNetclassDimensions( m_router->GetCurrentNet(), w, diameter, drill );
                m_router->SetCurrentViaDiameter( diameter );
                m_router->SetCurrentViaDrill( drill );
                m_router->ToggleViaPlacement();
                getEditFrame<PCB_EDIT_FRAME>()->SetTopLayer( m_router->GetCurrentLayer() );
                m_router->Move( m_endSnapPoint, m_endItem );
                break;
            }

            case '/':
                m_router->FlipPosture();
                break;

            case '+':
            case '=':
                m_router->SwitchLayer( m_router->NextCopperLayer( true ) );
                updateEndItem( *evt );
                getEditFrame<PCB_EDIT_FRAME>()->SetTopLayer( m_router->GetCurrentLayer() );
                m_router->Move( m_endSnapPoint, m_endItem );

                break;

            case '-':
                m_router->SwitchLayer( m_router->NextCopperLayer( false ) );
                getEditFrame<PCB_EDIT_FRAME>()->SetTopLayer( m_router->GetCurrentLayer() );
                m_router->Move( m_endSnapPoint, m_endItem );
                break;
            }
        }
    }

    m_router->StopRouting();

    if( saveUndoBuffer )
    {
        // Save the recent changes in the undo buffer
        getEditFrame<PCB_EDIT_FRAME>()->SaveCopyInUndoList( m_router->GetLastChanges(),
                                                            UR_UNSPECIFIED );
        m_router->ClearLastChanges();
        getEditFrame<PCB_EDIT_FRAME>()->OnModify();
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

    // SetContextMenu ( m_menu );
    // setMsgPanel(true, 0, wxT("KiRouter"), wxT("Pick an item to start routing"));
    getEditFrame<PCB_EDIT_FRAME>()->SetToolID( ID_TRACK_BUTT, wxCURSOR_PENCIL, _( "Add tracks" ) );

    ctls->SetSnapping( true );
    ctls->ShowCursor( true );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( m_needsSync )
        {
            m_router->SyncWorld();
            m_needsSync = false;
        }

        if( evt->IsCancel() )
            break; // Finish
        else if( evt->Action() == TA_UNDO_REDO )
            m_needsSync = true;
        else if( evt->IsMotion() )
            updateStartItem( *evt );
        else if( evt->IsClick( BUT_LEFT ) )
        {
            updateStartItem( *evt );
            startRouting();
        }
    }

    // clearMsgPanel();

    // Restore the default settings
    ctls->SetAutoPan( false );
    ctls->ShowCursor( false );
    ctls->ForceCursorPosition( false );

    getEditFrame<PCB_EDIT_FRAME>()->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}
