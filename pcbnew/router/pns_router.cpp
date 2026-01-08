/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <memory>
#include <vector>

#include <gal/graphics_abstraction_layer.h>

#include <advanced_config.h>
#include <settings/settings_manager.h>

#include <pcb_painter.h>
#include <pad.h>
#include <zone.h>

#include <geometry/shape.h>

#include "pns_node.h"
#include "pns_line_placer.h"
#include "pns_line.h"
#include "pns_solid.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_shove.h"
#include "pns_dragger.h"
#include "pns_multi_dragger.h"
#include "pns_component_dragger.h"
#include "pns_topology.h"
#include "pns_diff_pair_placer.h"
#include "pns_meander_placer.h"
#include "pns_meander_skew_placer.h"
#include "pns_dp_meander_placer.h"
#include "router_preview_item.h"

namespace PNS {

// an ugly singleton for drawing debug items within the router context.
// To be fixed sometime in the future.
static ROUTER* theRouter;

ROUTER::ROUTER()
{
    theRouter = this;

    m_state = IDLE;
    m_mode = PNS_MODE_ROUTE_SINGLE;

    m_logger = nullptr;

    if( ADVANCED_CFG::GetCfg().m_EnableRouterDump )
        m_logger = new LOGGER;

    // Initialize all other variables:
    m_lastNode = nullptr;
    m_iterLimit = 0;
    m_settings = nullptr;
    m_iface = nullptr;
    m_visibleViewArea.SetMaximum();
}


ROUTER* ROUTER::GetInstance()
{
    return theRouter;
}


ROUTER::~ROUTER()
{
    ClearWorld();
    theRouter = nullptr;
    delete m_logger;
}


void ROUTER::SyncWorld()
{
    ClearWorld();

    m_world = std::make_unique<NODE>( );
    m_iface->SyncWorld( m_world.get() );
    m_world->FixupVirtualVias();
}


void ROUTER::ClearWorld()
{
    if( m_world )
    {
        m_world->SetRuleResolver( nullptr );
        m_world->KillChildren();
        m_world.reset();
    }

    m_placer.reset();
}


bool ROUTER::RoutingInProgress() const
{
    return m_state != IDLE;
}


const ITEM_SET ROUTER::QueryHoverItems( const VECTOR2I& aP, int aSlopRadius )
{
    NODE*         node = m_placer ? m_placer->CurrentNode() : m_world.get();
    PNS::ITEM_SET ret;

    wxCHECK( node, ret );

    if( aSlopRadius > 0 )
    {
        NODE::OBSTACLES          obs;
        SEGMENT                  test( SEG( aP, aP ), nullptr );
        COLLISION_SEARCH_OPTIONS opts;

        test.SetWidth( 1 );
        test.SetLayers( PNS_LAYER_RANGE::All() );

        opts.m_differentNetsOnly = false;
        opts.m_overrideClearance = aSlopRadius;

        node->QueryColliding( &test, obs, opts );

        for( const OBSTACLE& obstacle : obs )
            ret.Add( obstacle.m_item, false );

        return ret;
    }
    else
    {
        return node->HitTest( aP );
    }
}


bool ROUTER::StartDragging( const VECTOR2I& aP, ITEM* aItem, int aDragMode )
{
    m_leaderSegments.clear();
    return StartDragging( aP, ITEM_SET( aItem ), aDragMode );
}


bool ROUTER::StartDragging( const VECTOR2I& aP, ITEM_SET aStartItems, int aDragMode )
{
    m_leaderSegments.clear();

    if( aStartItems.Empty() )
        return false;

    GetRuleResolver()->ClearCaches();

    if( aStartItems.Count( ITEM::SOLID_T ) == aStartItems.Size() )
    {
        m_dragger = std::make_unique<COMPONENT_DRAGGER>( this );
        m_state = DRAG_COMPONENT;
    }
    // more than 1 track segment or arc to drag? launch the multisegment dragger
    else if( aStartItems.Count( ITEM::SEGMENT_T | ITEM::ARC_T ) > 1 )
    {
        m_dragger = std::make_unique<MULTI_DRAGGER>( this );
        m_state = DRAG_SEGMENT;
    }
    else
    {
        m_dragger = std::make_unique<DRAGGER>( this );
        m_state = DRAG_SEGMENT;
    }

    m_dragger->SetMode( static_cast<PNS::DRAG_MODE>( aDragMode ) );
    m_dragger->SetWorld( m_world.get() );
    m_dragger->SetLogger( m_logger );
    m_dragger->SetDebugDecorator( m_iface->GetDebugDecorator() );

    if( m_logger )
        m_logger->Clear();

    if( m_logger )
    {
        if( aStartItems.Size() == 1 )
            m_logger->Log( LOGGER::EVT_START_DRAG, aP, aStartItems[0] );
        else if( aStartItems.Size() > 1 )
            m_logger->LogM( LOGGER::EVT_START_MULTIDRAG, aP, aStartItems.Items() ); // fixme default args
    }

    if( m_dragger->Start( aP, aStartItems ) )
    {
        return true;
    }
    else
    {
        m_dragger.reset();
        m_state = IDLE;
        return false;
    }
}


bool ROUTER::isStartingPointRoutable( const VECTOR2I& aWhere, ITEM* aStartItem, int aLayer )
{
    if( Settings().AllowDRCViolations() )
        return true;

    if( m_mode == PNS_MODE_ROUTE_DIFF_PAIR )
    {
        if( m_sizes.DiffPairGap() < m_sizes.MinClearance() )
        {
            SetFailureReason( _( "Diff pair gap is less than board minimum clearance." ) );
            return false;
        }
    }

    ITEM_SET candidates = QueryHoverItems( aWhere );
    wxString failureReason;

    for( ITEM* item : candidates.Items() )
    {
        // Edge cuts are put on all layers, but they're not *really* on all layers
        if( item->BoardItem() && item->BoardItem()->GetLayer() == Edge_Cuts )
            continue;

        if( !item->Layers().Overlaps( aLayer ) )
            continue;

        if( item->IsRoutable() )
        {
            failureReason = wxEmptyString;
            break;
        }
        else
        {
            BOARD_ITEM* parent = item->BoardItem();

            switch( parent->Type() )
            {
            case PCB_PAD_T:
            {
                PAD* pad = static_cast<PAD*>( parent );

                if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                    failureReason = _( "Cannot start routing from a non-plated hole." );
            }
                break;

            case PCB_ZONE_T:
            {
                ZONE* zone = static_cast<ZONE*>( parent );

                if( !zone->HasKeepoutParametersSet() )
                    break;

                if( !zone->GetZoneName().IsEmpty() )
                {
                    failureReason = wxString::Format( _( "Rule area '%s' disallows tracks." ),
                                                      zone->GetZoneName() );
                }
                else
                {
                    failureReason = _( "Rule area disallows tracks." );
                }
            }
                break;

            case PCB_FIELD_T:
            case PCB_TEXT_T:
            case PCB_TEXTBOX_T:
                failureReason = _( "Cannot start routing from a text item." );
                break;

            default:
                break;
            }
        }
    }

    if( !failureReason.IsEmpty() )
    {
        SetFailureReason( failureReason );
        return false;
    }

    VECTOR2I startPoint = aWhere;

    if( m_mode == PNS_MODE_ROUTE_SINGLE )
    {
        SHAPE_LINE_CHAIN dummyStartSeg;
        LINE             dummyStartLine;

        dummyStartSeg.Append( startPoint );
        dummyStartSeg.Append( startPoint, true );

        dummyStartLine.SetShape( dummyStartSeg );
        dummyStartLine.SetLayer( aLayer );
        dummyStartLine.SetNet( aStartItem ? aStartItem->Net() : 0 );
        dummyStartLine.SetWidth( m_sizes.TrackWidth() );

        if( m_world->CheckColliding( &dummyStartLine, ITEM::ANY_T ) )
        {
            // If the only reason we collide is track width; it's better to allow the user to start
            // anyway and just highlight the resulting collisions, so they can change width later.
            dummyStartLine.SetWidth( m_sizes.BoardMinTrackWidth() );

            if( m_world->CheckColliding( &dummyStartLine, ITEM::ANY_T ) )
            {
                ITEM_SET dummyStartSet( &dummyStartLine );
                NODE::ITEM_VECTOR highlightedItems;

                markViolations( m_world.get(), dummyStartSet, highlightedItems );

                for( ITEM* item : highlightedItems )
                    m_iface->HideItem( item );

                SetFailureReason( _( "The routing start point violates DRC." ) );
                return false;
            }
        }
    }
    else if( m_mode == PNS_MODE_ROUTE_DIFF_PAIR )
    {
        if( !aStartItem )
        {
            SetFailureReason( _( "Cannot start a differential pair in the middle of nowhere." ) );
            return false;
        }

        DP_PRIMITIVE_PAIR dpPair;
        wxString          errorMsg;

        if( !DIFF_PAIR_PLACER::FindDpPrimitivePair( m_world.get(), startPoint, aStartItem, dpPair,
                                                    &errorMsg ) )
        {
            SetFailureReason( errorMsg );
            return false;
        }

        // Check if the gap at the start point is compatible with the configured diff pair settings.
        // This catches neckdown areas where the existing tracks have different gap/width than configured.
        int actualGap = ( dpPair.AnchorP() - dpPair.AnchorN() ).EuclideanNorm();
        int configuredGap = m_sizes.DiffPairGap() + m_sizes.DiffPairWidth();

        // Allow some tolerance (10%) for minor differences, but warn about significant mismatches
        int tolerance = configuredGap / 10;

        if( std::abs( actualGap - configuredGap ) > tolerance )
        {
            SetFailureReason(
                    _( "The differential pair gap at the start point does not match "
                       "the configured gap. This can occur in neckdown areas where tracks "
                       "have narrower width and spacing. Adjust the differential pair "
                       "settings or start from a location with the correct gap." ) );
            return false;
        }

        SHAPE_LINE_CHAIN dummyStartSegA;
        SHAPE_LINE_CHAIN dummyStartSegB;
        LINE             dummyStartLineA;
        LINE             dummyStartLineB;

        dummyStartSegA.Append( dpPair.AnchorN() );
        dummyStartSegA.Append( dpPair.AnchorN(), true );

        dummyStartSegB.Append( dpPair.AnchorP() );
        dummyStartSegB.Append( dpPair.AnchorP(), true );

        dummyStartLineA.SetShape( dummyStartSegA );
        dummyStartLineA.SetLayer( aLayer );
        dummyStartLineA.SetNet( dpPair.PrimN()->Net() );
        dummyStartLineA.SetWidth( m_sizes.DiffPairWidth() );

        dummyStartLineB.SetShape( dummyStartSegB );
        dummyStartLineB.SetLayer( aLayer );
        dummyStartLineB.SetNet( dpPair.PrimP()->Net() );
        dummyStartLineB.SetWidth( m_sizes.DiffPairWidth() );

        if( m_world->CheckColliding( &dummyStartLineA, ITEM::ANY_T )
                || m_world->CheckColliding( &dummyStartLineB, ITEM::ANY_T ) )
        {
            // If the only reason we collide is track width; it's better to allow the user to start
            // anyway and just highlight the resulting collisions, so they can change width later.
            dummyStartLineA.SetWidth( m_sizes.BoardMinTrackWidth() );
            dummyStartLineB.SetWidth( m_sizes.BoardMinTrackWidth() );

            if( m_world->CheckColliding( &dummyStartLineA, ITEM::ANY_T )
                || m_world->CheckColliding( &dummyStartLineB, ITEM::ANY_T ) )
            {
                ITEM_SET          dummyStartSet;
                NODE::ITEM_VECTOR highlightedItems;

                dummyStartSet.Add( dummyStartLineA );
                dummyStartSet.Add( dummyStartLineB );
                markViolations( m_world.get(), dummyStartSet, highlightedItems );

                for( ITEM* item : highlightedItems )
                    m_iface->HideItem( item );

                SetFailureReason( _( "The routing start point violates DRC." ) );
                return false;
            }
        }
    }

    return true;
}


bool ROUTER::StartRouting( const VECTOR2I& aP, ITEM* aStartItem, int aLayer )
{
    GetRuleResolver()->ClearCaches();

    if( !isStartingPointRoutable( aP, aStartItem, aLayer ) )
        return false;

    switch( m_mode )
    {
    case PNS_MODE_ROUTE_SINGLE:
        m_placer = std::make_unique<LINE_PLACER>( this );
        break;

    case PNS_MODE_ROUTE_DIFF_PAIR:
        m_placer = std::make_unique<DIFF_PAIR_PLACER>( this );
        break;

    case PNS_MODE_TUNE_SINGLE:
        m_placer = std::make_unique<MEANDER_PLACER>( this );
        break;

    case PNS_MODE_TUNE_DIFF_PAIR:
        m_placer = std::make_unique<DP_MEANDER_PLACER>( this );
        break;

    case PNS_MODE_TUNE_DIFF_PAIR_SKEW:
        m_placer = std::make_unique<MEANDER_SKEW_PLACER>( this );
        break;

    default:
        return false;
    }

    m_placer->UpdateSizes( m_sizes );
    m_placer->SetLayer( aLayer );
    m_placer->SetDebugDecorator( m_iface->GetDebugDecorator() );
    m_placer->SetLogger( m_logger );

    if( m_placer->Start( aP, aStartItem ) )
    {
        m_state = ROUTE_TRACK;

        if( m_logger )
        {
            m_logger->Clear();
            m_logger->Log( LOGGER::EVT_START_ROUTE, aP, aStartItem, &m_sizes, m_placer->CurrentLayer() );
        }

        return true;
    }
    else
    {
        m_state = IDLE;
        m_placer.reset();

        return false;
    }
}


bool ROUTER::Move( const VECTOR2I& aP, ITEM* endItem )
{
    if( m_logger )
        m_logger->Log( LOGGER::EVT_MOVE, aP, endItem );

    switch( m_state )
    {
    case ROUTE_TRACK:
        return movePlacing( aP, endItem );

    case DRAG_SEGMENT:
    case DRAG_COMPONENT:
        return moveDragging( aP, endItem );

    default:
        break;
    }

    GetRuleResolver()->ClearTemporaryCaches();

    return false;
}


bool ROUTER::GetNearestRatnestAnchor( VECTOR2I& aOtherEnd, PNS_LAYER_RANGE& aOtherEndLayers,
                                      ITEM*& aOtherEndItem )
{
    // Can't finish something with no connections
    if( GetCurrentNets().empty() )
        return false;

    PLACEMENT_ALGO* placer = Placer();

    if( placer == nullptr || placer->Traces().Size() == 0 )
        return false;

    LINE* trace = dynamic_cast<LINE*>( placer->Traces()[0] );

    if( trace == nullptr )
        return false;

    PNS::NODE*    lastNode = placer->CurrentNode( true );
    PNS::TOPOLOGY topo( lastNode );

    // If the user has drawn a line, get the anchor nearest to the line end
    if( trace->SegmentCount() > 0 )
    {
        return topo.NearestUnconnectedAnchorPoint( trace, aOtherEnd, aOtherEndLayers,
                                                   aOtherEndItem );
    }

    // Otherwise, find the closest anchor to our start point

    // Get joint from placer start item
    const JOINT* jt = lastNode->FindJoint( placer->CurrentStart(), placer->CurrentLayer(),
                                           placer->CurrentNets()[0] );

    if( !jt )
        return false;

    // Get unconnected item from joint
    int        anchor;
    PNS::ITEM* it = topo.NearestUnconnectedItem( jt, &anchor );

    if( !it )
        return false;

    aOtherEnd = it->Anchor( anchor );
    aOtherEndLayers = it->Layers();
    aOtherEndItem = it;

    return true;
}


bool ROUTER::Finish()
{
    if( m_state != ROUTE_TRACK )
        return false;

    PLACEMENT_ALGO* placer = Placer();

    if( placer == nullptr || placer->Traces().Size() == 0 )
        return false;

    LINE* current = dynamic_cast<LINE*>( placer->Traces()[0] );

    if( current == nullptr )
        return false;

    // Get our current line and position and nearest ratsnest to them if it exists
    VECTOR2I    otherEnd;
    PNS_LAYER_RANGE otherEndLayers;
    ITEM*       otherEndItem = nullptr;

    // Get the anchor nearest to the end of the trace the user is routing
    if( !GetNearestRatnestAnchor( otherEnd, otherEndLayers, otherEndItem ) )
        return false;

    // Keep moving until we don't change position or hit the limit
    int      triesLeft = 5;
    VECTOR2I moveResultPoint;

    do
    {
        moveResultPoint = placer->CurrentEnd();
        Move( otherEnd, otherEndItem );
        triesLeft--;
    } while( placer->CurrentEnd() != moveResultPoint && triesLeft );

    // If we've made it, fix the route and we're done
    if( moveResultPoint == otherEnd && otherEndLayers.Overlaps( GetCurrentLayer() ) )
    {
        bool forceFinish = false;
        bool allowViolations = false;

        return FixRoute( otherEnd, otherEndItem, forceFinish, allowViolations );
    }

    return false;
}


bool ROUTER::ContinueFromEnd( ITEM** aNewStartItem )
{
    PLACEMENT_ALGO* placer = Placer();

    if( placer == nullptr || placer->Traces().Size() == 0 )
        return false;

    LINE* current = dynamic_cast<LINE*>( placer->Traces()[0] );

    if( current == nullptr )
        return false;

    int         currentLayer = GetCurrentLayer();
    VECTOR2I    currentEnd = placer->CurrentEnd();
    VECTOR2I    otherEnd;
    PNS_LAYER_RANGE otherEndLayers;
    ITEM*       otherEndItem = nullptr;

    // Get the anchor nearest to the end of the trace the user is routing
    if( !GetNearestRatnestAnchor( otherEnd, otherEndLayers, otherEndItem ) )
        return false;

    CommitRouting();

    // Commit whatever we've fixed and restart routing from the other end
    int nextLayer = otherEndLayers.Overlaps( currentLayer ) ? currentLayer : otherEndLayers.Start();

    if( !StartRouting( otherEnd, otherEndItem, nextLayer ) )
        return false;

    // Attempt to route to our current position
    Move( currentEnd, nullptr );

    *aNewStartItem = otherEndItem;

    return true;
}


bool ROUTER::moveDragging( const VECTOR2I& aP, ITEM* aEndItem )
{
    m_iface->EraseView();

    bool ret = m_dragger->Drag( aP );
    ITEM_SET dragged = m_dragger->Traces();

    m_leaderSegments = m_dragger->GetLastCommittedLeaderSegments();

    updateView( m_dragger->CurrentNode(), dragged, true );
    return ret;
}


void ROUTER::markViolations( NODE* aNode, ITEM_SET& aCurrent, NODE::ITEM_VECTOR& aRemoved )
{
    auto updateItem =
            [&]( ITEM* currentItem, ITEM* itemToMark )
            {
                std::unique_ptr<ITEM> tmp( itemToMark->Clone() );

                int  clearance;
                bool removeOriginal = true;

                clearance = aNode->GetClearance( currentItem, itemToMark );

                if( itemToMark->Layers().IsMultilayer() && !currentItem->Layers().IsMultilayer() )
                    tmp->SetLayer( currentItem->Layer() );

                if( itemToMark->IsCompoundShapePrimitive() )
                {
                    // We're only highlighting one (or more) of several primitives so we don't
                    // want all the other parts of the object to disappear
                    removeOriginal = false;
                }

                m_iface->DisplayItem( tmp.get(), clearance );

                if( removeOriginal )
                    aRemoved.push_back( itemToMark );
            };

    for( ITEM* item : aCurrent.Items() )
    {
        NODE::OBSTACLES obstacles;

        aNode->QueryColliding( item, obstacles );

        if( item->OfKind( ITEM::LINE_T ) )
        {
            LINE* l = static_cast<LINE*>( item );

            if( l->EndsWithVia() )
            {
                VIA v( l->Via() );
                aNode->QueryColliding( &v, obstacles );
            }
        }

        ITEM_SET draggedItems;

        if( GetDragger() )
            draggedItems = GetDragger()->Traces();

        for( const OBSTACLE& obs : obstacles )
        {
            // Don't mark items being dragged; only board items they collide with
            if( draggedItems.Contains( obs.m_item ) )
                continue;

            obs.m_item->Mark( obs.m_item->Marker() | MK_VIOLATION );
            updateItem( item, obs.m_item );
        }

        if( item->Kind() == ITEM::LINE_T )
        {
            LINE* line = static_cast<LINE*>( item );

            // Show clearance on any blocking obstacles
            if( line->GetBlockingObstacle() )
                updateItem( item, line->GetBlockingObstacle() );
        }
    }
}


void ROUTER::updateView( NODE* aNode, ITEM_SET& aCurrent, bool aDragging )
{
    NODE::ITEM_VECTOR removed, added;
    NODE::OBSTACLES obstacles;

    if( !aNode )
        return;

    markViolations( aNode, aCurrent, removed );

    aNode->GetUpdatedItems( removed, added );

    std::vector<const PNS::ITEM*> cacheCheckItems( added.begin(), added.end() );
    GetRuleResolver()->ClearCacheForItems( cacheCheckItems );

    for( ITEM* item : added )
    {
        int clearance = GetRuleResolver()->Clearance( item, nullptr );
        m_iface->DisplayItem( item, clearance, aDragging );
    }

    for( ITEM* item : removed )
        m_iface->HideItem( item );
}


void ROUTER::UpdateSizes( const SIZES_SETTINGS& aSizes )
{
    m_sizes = aSizes;

    // Change track/via size settings
    if( m_state == ROUTE_TRACK )
        m_placer->UpdateSizes( m_sizes );
}


bool ROUTER::movePlacing( const VECTOR2I& aP, ITEM* aEndItem )
{
    m_iface->EraseView();

    bool ret = m_placer->Move( aP, aEndItem );
    ITEM_SET current = m_placer->Traces();

    for( const ITEM* item : current.CItems() )
    {
        if( !item->OfKind( ITEM::LINE_T ) )
            continue;

        const LINE* l = static_cast<const LINE*>( item );
        int clearance = GetRuleResolver()->Clearance( item, nullptr );

        m_iface->DisplayItem( l, clearance, false, PNS_HEAD_TRACE );

        if( l->EndsWithVia() )
        {
            const VIA& via = l->Via();
            clearance = GetRuleResolver()->Clearance( &via, nullptr );

            if( via.HasHole() )
            {
                int holeClearance = GetRuleResolver()->Clearance( via.Hole(), nullptr );
                int annularWidth = std::max( 0, via.Diameter( l->Layer() ) - via.Drill() ) / 2;
                int excessHoleClearance = holeClearance - annularWidth;

                if( excessHoleClearance > clearance )
                    clearance = excessHoleClearance;
            }

            m_iface->DisplayItem( &l->Via(), clearance, false, PNS_HEAD_TRACE );
        }
    }

    //ITEM_SET tmp( &current );

    updateView( m_placer->CurrentNode( true ), current );

    return ret;
}


void ROUTER::GetUpdatedItems( std::vector<PNS::ITEM*>& aRemoved, std::vector<PNS::ITEM*>& aAdded,
                              std::vector<PNS::ITEM*>& aHeads )
{
    NODE *node = nullptr;
    ITEM_SET current;

    if( m_state == ROUTE_TRACK )
    {
        node = m_placer->CurrentNode( true );
        current = m_placer->Traces();
    }
    else if ( m_state == DRAG_SEGMENT )
    {
        node = m_dragger->CurrentNode();
        current = m_dragger->Traces();
    }

    // There probably should be a debugging assertion and possibly a PNS_LOGGER call here but
    // I'm not sure how to be proceed WLS.
    if( !node )
        return;

    node->GetUpdatedItems( aRemoved, aAdded );

    for( const ITEM* item : current.CItems() )
        aHeads.push_back( item->Clone() );
}


void ROUTER::CommitRouting( NODE* aNode )
{
    if( m_state == ROUTE_TRACK && !m_placer->HasPlacedAnything() )
        return;

    NODE::ITEM_VECTOR removed;
    NODE::ITEM_VECTOR added;
    NODE::ITEM_VECTOR changed;

    aNode->GetUpdatedItems( removed, added );

    for( ITEM* item : removed )
    {
        bool is_changed = false;

        // Items in remove/add that share the same parent are just updated versions
        // We move them to the updated vector to preserve attributes such as UUID and pad data
        if( item->Parent() )
        {
            for( NODE::ITEM_VECTOR::iterator added_it = added.begin();
                    added_it != added.end(); ++added_it )
            {
                if( ( *added_it )->Parent() && ( *added_it )->Parent() == item->Parent() )
                {
                    changed.push_back( *added_it );
                    added.erase( added_it );
                    is_changed = true;
                    break;
                }
            }
        }

        if( !is_changed && !item->IsVirtual() )
            m_iface->RemoveItem( item );
    }

    for( ITEM* item : added )
    {
        if( !item->IsVirtual() )
            m_iface->AddItem( item );
    }

    for( ITEM* item : changed )
    {
        if( !item->IsVirtual() )
            m_iface->UpdateItem( item );
    }

    m_iface->Commit();
    m_world->Commit( aNode );
}


bool ROUTER::FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish, bool aForceCommit )
{
    bool rv = false;

    if( m_logger )
        m_logger->Log( LOGGER::EVT_FIX, aP, aEndItem );

    switch( m_state )
    {
    case ROUTE_TRACK:
        rv = m_placer->FixRoute( aP, aEndItem, aForceFinish );
        break;

    case DRAG_SEGMENT:
    case DRAG_COMPONENT:
        rv = m_dragger->FixRoute( aForceCommit );
        break;

    default:
        break;
    }

    return rv;
}

std::vector<PNS::ITEM*> ROUTER::GetLastCommittedLeaderSegments()
{
    return m_leaderSegments;
};


std::optional<VECTOR2I> ROUTER::UndoLastSegment()
{
    if( !RoutingInProgress() )
        return std::nullopt;

    if( m_logger )
        m_logger->Log( LOGGER::EVT_UNFIX );

    return m_placer->UnfixRoute();
}


void ROUTER::CommitRouting()
{
    if( m_state == ROUTE_TRACK )
        m_placer->CommitPlacement();

    StopRouting();
}


void ROUTER::StopRouting()
{
    // Update the ratsnest with new changes

    if( m_placer )
    {
        std::vector<NET_HANDLE> nets;
        m_placer->GetModifiedNets( nets );

        // Update the ratsnest with new changes
        for( NET_HANDLE n : nets )
            m_iface->UpdateNet( n );
    }

    if( !RoutingInProgress() )
        return;

    m_placer.reset();
    m_dragger.reset();

    m_iface->EraseView();

    m_state = IDLE;
    m_world->KillChildren();
    m_world->ClearRanks();
}


void ROUTER::ClearViewDecorations()
{
    m_iface->EraseView();
}


void ROUTER::FlipPosture()
{
    if( m_state == ROUTE_TRACK )
    {
        m_placer->FlipPosture();
    }
}


bool ROUTER::SwitchLayer( int aLayer )
{
    if( m_state == ROUTE_TRACK )
        return m_placer->SetLayer( aLayer );

    return false;
}


void ROUTER::ToggleViaPlacement()
{
    if( m_state == ROUTE_TRACK )
    {
        bool toggle = !m_placer->IsPlacingVia();
        m_placer->ToggleVia( toggle );

        if( m_logger )
            m_logger->Log( LOGGER::EVT_TOGGLE_VIA, VECTOR2I(), nullptr, &m_sizes );
    }
}


const std::vector<NET_HANDLE> ROUTER::GetCurrentNets() const
{
    if( m_placer )
        return m_placer->CurrentNets();
    else if( m_dragger )
        return m_dragger->CurrentNets();

    return std::vector<NET_HANDLE>();
}


int ROUTER::GetCurrentLayer() const
{
    if( m_placer )
        return m_placer->CurrentLayer();
    else if( m_dragger )
        return m_dragger->CurrentLayer();

    return -1;
}


LOGGER* ROUTER::Logger()
{
    return m_logger;
}


bool ROUTER::IsPlacingVia() const
{
    if( !m_placer )
        return false;

    return m_placer->IsPlacingVia();
}


void ROUTER::ToggleCornerMode()
{
    DIRECTION_45::CORNER_MODE mode = m_settings->GetCornerMode();

    switch( m_settings->GetCornerMode() )
    {
    case DIRECTION_45::CORNER_MODE::MITERED_45: mode = DIRECTION_45::CORNER_MODE::ROUNDED_45; break;
    case DIRECTION_45::CORNER_MODE::ROUNDED_45: mode = DIRECTION_45::CORNER_MODE::MITERED_90; break;
    case DIRECTION_45::CORNER_MODE::MITERED_90: mode = DIRECTION_45::CORNER_MODE::ROUNDED_90; break;
    case DIRECTION_45::CORNER_MODE::ROUNDED_90: mode = DIRECTION_45::CORNER_MODE::MITERED_45; break;
    }

    m_settings->SetCornerMode( mode );
}


void ROUTER::SetOrthoMode( bool aEnable )
{
    if( !m_placer )
        return;

    m_placer->SetOrthoMode( aEnable );
}


void ROUTER::SetMode( ROUTER_MODE aMode )
{
    m_mode = aMode;
}


void ROUTER::SetInterface( ROUTER_IFACE *aIface )
{
    m_iface = aIface;
}


void ROUTER::BreakSegmentOrArc( ITEM *aItem, const VECTOR2I& aP )
{
    NODE *node = m_world->Branch();

    LINE_PLACER placer( this );

    bool ret = false;

    if( aItem->OfKind( ITEM::SEGMENT_T ) )
        ret = placer.SplitAdjacentSegments( node, aItem, aP );
    else if( aItem->OfKind( ITEM::ARC_T ) )
        ret = placer.SplitAdjacentArcs( node, aItem, aP );

    if( ret )
    {
        CommitRouting( node );
    }
    else
    {
        delete node;
    }
}

}
