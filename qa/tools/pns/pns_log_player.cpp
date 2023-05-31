/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers.
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

#include "pns_test_debug_decorator.h"
#include "pns_log_file.h"
#include "pns_log_player.h"

#include <pcbnew_utils/board_test_utils.h>

#define PNSLOGINFO PNS::DEBUG_DECORATOR::SRC_LOCATION_INFO( __FILE__, __FUNCTION__, __LINE__ )

using namespace PNS;

PNS_LOG_PLAYER::PNS_LOG_PLAYER()
{
    SetReporter( &NULL_REPORTER::GetInstance() );
}


PNS_LOG_PLAYER::~PNS_LOG_PLAYER()
{
    if( m_debugDecorator )
        delete m_debugDecorator;
}

void PNS_LOG_PLAYER::createRouter()
{
    m_viewTracker.reset( new PNS_LOG_VIEW_TRACKER );
    m_iface.reset( new PNS_LOG_PLAYER_KICAD_IFACE( m_viewTracker.get() ) );
    m_router.reset( new ROUTER );
    m_iface->SetBoard( m_board.get() );
    m_router->SetInterface( m_iface.get() );
    m_router->ClearWorld();
    m_router->SyncWorld();
    m_router->LoadSettings( new PNS::ROUTING_SETTINGS( nullptr, "" ) );
    m_router->Settings().SetMode( PNS::RM_Walkaround );
    m_router->Sizes().SetTrackWidth( 250000 );

    //m_router->Settings().SetOptimizeDraggedTrack( true );

    m_debugDecorator = new PNS_TEST_DEBUG_DECORATOR;
    m_debugDecorator->Clear();
    m_iface->SetDebugDecorator( m_debugDecorator );
}


const PNS_LOG_FILE::COMMIT_STATE PNS_LOG_PLAYER::GetRouterUpdatedItems()
{
    PNS_LOG_FILE::COMMIT_STATE state;
    std::vector<PNS::ITEM*> added, removed, heads;
    m_router->GetUpdatedItems( removed, added, heads );

    //printf("a %d r %d\n", added.size(), removed.size() );
    for( auto item : removed )
    {
        if( item->Parent() )
        {
            state.m_removedIds.insert( item->Parent()->m_Uuid );
        }
    }

    for( auto item : added )
    {
        state.m_addedItems.insert( item );
    }

    // fixme: update the state with the head trace (not supported in current testsuite)

    return state;
}

void PNS_LOG_PLAYER::ReplayLog( PNS_LOG_FILE* aLog, int aStartEventIndex, int aFrom, int aTo )
{
    m_board = aLog->GetBoard();

    createRouter();

    m_router->LoadSettings( aLog->GetRoutingSettings() );

    int eventIdx = 0;
    int totalEvents = aLog->Events().size();

    for( auto evt : aLog->Events() )
    {
        if( eventIdx < aFrom || ( aTo >= 0 && eventIdx > aTo ) )
            continue;

        auto  item = aLog->ItemById( evt );
        ITEM* ritem = item ? m_router->GetWorld()->FindItemByParent( item ) : nullptr;

        eventIdx++;

        switch( evt.type )
        {
        case LOGGER::EVT_START_ROUTE:
        {
            m_debugDecorator->NewStage( "route-start", 0, PNSLOGINFO );
            m_viewTracker->SetStage( m_debugDecorator->GetStageCount() - 1 );

            auto msg = wxString::Format( "event [%d/%d]: route-start (%d, %d)", eventIdx, totalEvents, evt.p.x, evt.p.y );

            m_debugDecorator->Message( msg );
            m_reporter->Report( msg );

            m_router->StartRouting( evt.p, ritem, ritem ? ritem->Layers().Start() : F_Cu );
            break;
        }

        case LOGGER::EVT_START_DRAG:
        {
            m_debugDecorator->NewStage( "drag-start", 0, PNSLOGINFO );
            m_viewTracker->SetStage( m_debugDecorator->GetStageCount() - 1 );

            auto msg = wxString::Format( "event [%d/%d]: drag-start (%d, %d)", eventIdx, totalEvents, evt.p.x, evt.p.y );

            m_debugDecorator->Message( msg );
            m_reporter->Report( msg );

            bool rv = m_router->StartDragging( evt.p, ritem, 0 );
            break;
        }

        case LOGGER::EVT_FIX:
        {
            m_debugDecorator->NewStage( "fix", 0, PNSLOGINFO );
            m_viewTracker->SetStage( m_debugDecorator->GetStageCount() - 1 );
            m_debugDecorator->Message( wxString::Format( "fix (%d, %d)", evt.p.x, evt.p.y ) );
            bool rv = m_router->FixRoute( evt.p, ritem );
            printf( "  fix -> (%d, %d) ret %d\n", evt.p.x, evt.p.y, rv ? 1 : 0 );
            break;
        }

        case LOGGER::EVT_UNFIX:
        {
            m_debugDecorator->NewStage( "unfix", 0, PNSLOGINFO );
            m_viewTracker->SetStage( m_debugDecorator->GetStageCount() - 1 );
            m_debugDecorator->Message( wxString::Format( "unfix (%d, %d)", evt.p.x, evt.p.y ) );
            printf( "  unfix\n" );
            m_router->UndoLastSegment();
            break;
        }

        case LOGGER::EVT_MOVE:
        {
            m_debugDecorator->NewStage( "move", 0, PNSLOGINFO );
            m_viewTracker->SetStage( m_debugDecorator->GetStageCount() - 1 );

            auto msg = wxString::Format( "event [%d/%d]: move (%d, %d)", eventIdx, totalEvents, evt.p.x, evt.p.y );

            m_debugDecorator->Message( msg );
            m_reporter->Report( msg );

            bool ret = m_router->Move( evt.p, ritem );
            m_debugDecorator->SetCurrentStageStatus( ret );
            break;
        }

        case LOGGER::EVT_TOGGLE_VIA:
        {
            m_debugDecorator->NewStage( "toggle-via", 0, PNSLOGINFO );

             auto msg = wxString::Format( "event [%d/%d]: toggle-via", eventIdx, totalEvents );

            m_debugDecorator->Message( msg );
            m_reporter->Report( msg );

            m_viewTracker->SetStage( m_debugDecorator->GetStageCount() - 1 );
            m_router->ToggleViaPlacement();
            break;
        }

        default: break;
        }

        PNS::NODE* node = nullptr;

#if 0
        if( m_router->GetState() == PNS::ROUTER::ROUTE_TRACK )
        {
            m_debugDecorator->BeginGroup( "current route", 0 );

            auto traces = m_router->Placer()->Traces();

            for( const auto& t : traces.CItems() )
            {
                const LINE *l  = static_cast<LINE*>(t.item);
                const auto& sh = l->CLine();

                m_debugDecorator->AddItem( l, YELLOW, 0, wxT( "line seg" ) );
            }

            m_debugDecorator->EndGroup( PNSLOGINFO );

            node = m_router->Placer()->CurrentNode( true );
        }
        else if( m_router->GetState() == PNS::ROUTER::DRAG_SEGMENT )
        {
            node = m_router->GetDragger()->CurrentNode();
        }
        if( !node )
            return;

        NODE::ITEM_VECTOR removed, added;

        node->GetUpdatedItems( removed, added );

        if( ! added.empty() )
        {
            bool first = true;
            m_debugDecorator->BeginGroup( wxT( "node-added-items" ), 0 );

            for( auto t : added )
            {
                m_debugDecorator->AddItem( t, MAGENTA, 0, wxT( "seg" ) );
            }

            m_debugDecorator->EndGroup();
        }
#endif
    }


}


bool PNS_LOG_PLAYER::CompareResults( PNS_LOG_FILE* aLog )
{
    auto cstate = GetRouterUpdatedItems();

    printf("Comparing %zu added/%zu removed items\n", cstate.m_addedItems.size(), cstate.m_removedIds.size() );
    return cstate.Compare( aLog->GetExpectedResult() );
}


PNS_LOG_PLAYER_KICAD_IFACE::PNS_LOG_PLAYER_KICAD_IFACE( PNS_LOG_VIEW_TRACKER* aViewTracker ) :
        m_viewTracker( aViewTracker )
{
}

PNS_LOG_PLAYER_KICAD_IFACE::~PNS_LOG_PLAYER_KICAD_IFACE()
{
}

void PNS_LOG_PLAYER_KICAD_IFACE::HideItem( PNS::ITEM* aItem )
{
    //printf("DBG hide %p\n", aItem);
    m_viewTracker->HideItem( aItem );
}

void PNS_LOG_PLAYER_KICAD_IFACE::DisplayItem( const PNS::ITEM* aItem, int aClearance, bool aEdit, bool aIsHeadTrace )
{
    //printf("DBG disp %p\n", aItem);
    m_viewTracker->DisplayItem( aItem );
}


PNS_LOG_VIEW_TRACKER::PNS_LOG_VIEW_TRACKER()
{
}

PNS_LOG_VIEW_TRACKER::~PNS_LOG_VIEW_TRACKER()
{
}

void PNS_LOG_VIEW_TRACKER::SetStage( int aStage )
{
    m_currentStage = aStage;
    m_vitems[m_currentStage] = VIEW_ENTRIES();
}

void PNS_LOG_VIEW_TRACKER::HideItem( PNS::ITEM* aItem )
{
    ENTRY ent;
    ent.isHideOp = true;
    ent.item = aItem;
    m_vitems[m_currentStage].push_back( ent );
}

void PNS_LOG_VIEW_TRACKER::DisplayItem( const PNS::ITEM* aItem )
{
    ENTRY ent;
    ent.isHideOp = false;
    ent.item = aItem->Clone();
    m_vitems[m_currentStage].push_back( ent );
    //printf("DBG disp cur %d cnt %d\n", m_currentStage, m_vitems[m_currentStage].size() );
}

