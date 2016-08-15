/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include <cstdio>
#include <vector>

#include <view/view.h>
#include <view/view_item.h>
#include <view/view_group.h>
#include <gal/graphics_abstraction_layer.h>

#include <pcb_painter.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_circle.h>

#include "trace.h"
#include "pns_node.h"
#include "pns_line_placer.h"
#include "pns_line.h"
#include "pns_solid.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_shove.h"
#include "pns_dragger.h"
#include "pns_topology.h"
#include "pns_diff_pair_placer.h"
#include "pns_meander_placer.h"
#include "pns_meander_skew_placer.h"
#include "pns_dp_meander_placer.h"

#include <router/router_preview_item.h>

#include <class_board.h>
#include <class_board_connected_item.h>
#include <class_module.h>
#include <class_track.h>
#include <ratsnest_data.h>
#include <layers_id_colors_and_visibility.h>
#include <geometry/convex_hull.h>


// an ugly singleton for drawing debug items within the router context.
// To be fixed sometime in the future.
static PNS_ROUTER* theRouter;

PNS_ROUTER::PNS_ROUTER()
{
    theRouter = this;


    m_state = IDLE;
    m_world = NULL;
    m_placer = NULL;
    m_dragger = NULL;
    m_mode = PNS_MODE_ROUTE_SINGLE;

    // Initialize all other variables:
    m_lastNode = NULL;
    m_shove = NULL;
    m_iterLimit = 0;
    m_showInterSteps = false;
    m_snapshotIter = 0;
    m_view = NULL;
    m_snappingEnabled  = false;
    m_violation = false;
}


PNS_ROUTER* PNS_ROUTER::GetInstance()
{
    return theRouter;
}


PNS_ROUTER::~PNS_ROUTER()
{
    ClearWorld();
    theRouter = NULL;
}

void PNS_ROUTER::SyncWorld( )
{
    ClearWorld();

    m_world = new PNS_NODE;
    m_iface->SyncWorld( m_world );

}

void PNS_ROUTER::ClearWorld()
{
    if( m_world )
    {
        m_world->KillChildren();
        delete m_world;
    }

    if( m_placer )
        delete m_placer;


    m_world = NULL;
    m_placer = NULL;
}


bool PNS_ROUTER::RoutingInProgress() const
{
    return m_state != IDLE;
}


const PNS_ITEMSET PNS_ROUTER::QueryHoverItems( const VECTOR2I& aP )
{
    if( m_state == IDLE )
        return m_world->HitTest( aP );
    else
    {
        return m_placer->CurrentNode()->HitTest( aP );
    }
}

bool PNS_ROUTER::StartDragging( const VECTOR2I& aP, PNS_ITEM* aStartItem )
{
    if( !aStartItem || aStartItem->OfKind( PNS_ITEM::SOLID ) )
        return false;

    m_dragger = new PNS_DRAGGER( this );
    m_dragger->SetWorld( m_world );
    m_dragger->SetDebugDecorator ( m_iface->GetDebugDecorator () );

    if( m_dragger->Start ( aP, aStartItem ) )
        m_state = DRAG_SEGMENT;
    else
    {
        delete m_dragger;
        m_state = IDLE;
        return false;
    }

    return true;
}

bool PNS_ROUTER::StartRouting( const VECTOR2I& aP, PNS_ITEM* aStartItem, int aLayer )
{
    switch( m_mode )
    {
        case PNS_MODE_ROUTE_SINGLE:
            m_placer = new PNS_LINE_PLACER( this );
            break;
        case PNS_MODE_ROUTE_DIFF_PAIR:
            m_placer = new PNS_DIFF_PAIR_PLACER( this );
            break;
        case PNS_MODE_TUNE_SINGLE:
            m_placer = new PNS_MEANDER_PLACER( this );
            break;
        case PNS_MODE_TUNE_DIFF_PAIR:
            m_placer = new PNS_DP_MEANDER_PLACER( this );
            break;
        case PNS_MODE_TUNE_DIFF_PAIR_SKEW:
            m_placer = new PNS_MEANDER_SKEW_PLACER( this );
            break;

        default:
            return false;
    }

    m_placer->UpdateSizes ( m_sizes );
    m_placer->SetLayer( aLayer );
    m_placer->SetDebugDecorator ( m_iface->GetDebugDecorator () );

    bool rv = m_placer->Start( aP, aStartItem );

    if( !rv )
        return false;

    m_currentEnd = aP;
    m_state = ROUTE_TRACK;
    return rv;
}


void PNS_ROUTER::DisplayItems( const PNS_ITEMSET& aItems )
{
    for( const PNS_ITEM* item : aItems.CItems() )
        m_iface->DisplayItem( item );
}

void PNS_ROUTER::Move( const VECTOR2I& aP, PNS_ITEM* endItem )
{
    m_currentEnd = aP;

    switch( m_state )
    {
    case ROUTE_TRACK:
        movePlacing( aP, endItem );
        break;

    case DRAG_SEGMENT:
        moveDragging( aP, endItem );
        break;

    default:
        break;
    }
}


void PNS_ROUTER::moveDragging( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    m_iface->EraseView();

    m_dragger->Drag( aP );
    PNS_ITEMSET dragged = m_dragger->Traces();

    updateView( m_dragger->CurrentNode(), dragged );
}


void PNS_ROUTER::markViolations( PNS_NODE* aNode, PNS_ITEMSET& aCurrent,
                                 PNS_NODE::ITEM_VECTOR& aRemoved )
{
    for( PNS_ITEM* item : aCurrent.Items() )
    {
        PNS_NODE::OBSTACLES obstacles;

        aNode->QueryColliding( item, obstacles, PNS_ITEM::ANY );

        if( item->OfKind( PNS_ITEM::LINE ) )
        {
            PNS_LINE* l = static_cast<PNS_LINE*>( item );

            if( l->EndsWithVia() )
            {
                PNS_VIA v( l->Via() );
                aNode->QueryColliding( &v, obstacles, PNS_ITEM::ANY );
            }
        }

        for( PNS_OBSTACLE& obs : obstacles )
        {
            int clearance = aNode->GetClearance( item, obs.m_item );
            std::unique_ptr<PNS_ITEM> tmp( obs.m_item->Clone() );
            tmp->Mark( MK_VIOLATION );
            m_iface->DisplayItem( tmp.get(), -1, clearance );
            aRemoved.push_back( obs.m_item );
        }
    }
}


void PNS_ROUTER::updateView( PNS_NODE* aNode, PNS_ITEMSET& aCurrent )
{
    PNS_NODE::ITEM_VECTOR removed, added;
    PNS_NODE::OBSTACLES obstacles;

    if( !aNode )
        return;

    if( Settings().Mode() == RM_MarkObstacles )
        markViolations( aNode, aCurrent, removed );

    aNode->GetUpdatedItems( removed, added );

	for ( auto item : added )
        m_iface->DisplayItem( item );

    for ( auto item : removed )
        m_iface->HideItem ( item );
}


void PNS_ROUTER::UpdateSizes ( const PNS_SIZES_SETTINGS& aSizes )
{
    m_sizes = aSizes;

    // Change track/via size settings
    if( m_state == ROUTE_TRACK)
    {
        m_placer->UpdateSizes( m_sizes );
    }
}


void PNS_ROUTER::movePlacing( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    m_iface->EraseView();

    m_placer->Move( aP, aEndItem );
    PNS_ITEMSET current = m_placer->Traces();

    for( const PNS_ITEM* item : current.CItems() )
    {
        if( !item->OfKind( PNS_ITEM::LINE ) )
            continue;

        const PNS_LINE* l = static_cast<const PNS_LINE*>( item );
        m_iface->DisplayItem( l );

        if( l->EndsWithVia() )
            m_iface->DisplayItem( &l->Via() );
    }

    //PNS_ITEMSET tmp( &current );

    updateView( m_placer->CurrentNode( true ), current );
}


void PNS_ROUTER::CommitRouting( PNS_NODE* aNode )
{
    PNS_NODE::ITEM_VECTOR removed, added;

    aNode->GetUpdatedItems( removed, added );

    for ( auto item : removed )
        m_iface->RemoveItem ( item );

    for ( auto item : added )
        m_iface->AddItem( item );

    m_iface->Commit();
    m_world->Commit( aNode );
}


bool PNS_ROUTER::FixRoute( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    bool rv = false;

    switch( m_state )
    {
    case ROUTE_TRACK:
        rv = m_placer->FixRoute( aP, aEndItem );
        break;

    case DRAG_SEGMENT:
        rv = m_dragger->FixRoute();
        break;

    default:
        break;
    }

    if( rv )
       StopRouting();

    return rv;
}


void PNS_ROUTER::StopRouting()
{
    // Update the ratsnest with new changes

    if( m_placer )
    {
        std::vector<int> nets;
        m_placer->GetModifiedNets( nets );

        // Update the ratsnest with new changes
		for ( auto n : nets )
            m_iface->UpdateNet( n );
    }

    if( !RoutingInProgress() )
        return;

    if( m_placer )
        delete m_placer;

    if( m_dragger )
        delete m_dragger;

    m_placer = NULL;
    m_dragger = NULL;

    m_iface->EraseView();

    m_state = IDLE;
    m_world->KillChildren();
    m_world->ClearRanks();
}


void PNS_ROUTER::FlipPosture()
{
    if( m_state == ROUTE_TRACK )
    {
        m_placer->FlipPosture();
    }
}


void PNS_ROUTER::SwitchLayer( int aLayer )
{
    switch( m_state )
    {
    case ROUTE_TRACK:
        m_placer->SetLayer( aLayer );
        break;
    default:
        break;
    }
}


void PNS_ROUTER::ToggleViaPlacement()
{
    if( m_state == ROUTE_TRACK )
    {
        bool toggle = !m_placer->IsPlacingVia();
        m_placer->ToggleVia( toggle );
    }
}


const std::vector<int> PNS_ROUTER::GetCurrentNets() const
{
    if( m_placer )
        return m_placer->CurrentNets();

    return std::vector<int>();
}


int PNS_ROUTER::GetCurrentLayer() const
{
    if( m_placer )
        return m_placer->CurrentLayer();
    return -1;
}


void PNS_ROUTER::DumpLog()
{
    PNS_LOGGER* logger = NULL;

    switch( m_state )
    {
    case DRAG_SEGMENT:
        logger = m_dragger->Logger();
        break;

    case ROUTE_TRACK:
        logger = m_placer->Logger();
        break;

    default:
        break;
    }

    if( logger )
        logger->Save( "/tmp/shove.log" );
}


bool PNS_ROUTER::IsPlacingVia() const
{
    if( !m_placer )
        return false;

    return m_placer->IsPlacingVia();
}


void PNS_ROUTER::SetOrthoMode( bool aEnable )
{
    if( !m_placer )
        return;

    m_placer->SetOrthoMode( aEnable );
}


void PNS_ROUTER::SetMode( PNS_ROUTER_MODE aMode )
{
    m_mode = aMode;
}

void PNS_ROUTER::SetInterface( PNS_ROUTER_IFACE *aIface )
{
    m_iface = aIface;
    m_iface->SetRouter( this );
}
