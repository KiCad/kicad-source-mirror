/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
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

#include <wx/log.h>

#include <chrono>
#include <stack>

#include <advanced_config.h>

#include "pns_line.h"
#include "pns_segment.h"
#include "pns_arc.h"
#include "pns_node.h"
#include "pns_joint.h"
#include "pns_solid.h"
#include "pns_router.h"
#include "pns_utils.h"

#include "pns_diff_pair.h"
#include "pns_topology.h"

#include <board.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <pad.h>

namespace PNS {

bool TOPOLOGY::SimplifyLine( LINE* aLine )
{
    if( !aLine->IsLinked() || !aLine->SegmentCount() )
        return false;

    LINKED_ITEM* root = aLine->GetLink( 0 );
    LINE l = m_world->AssembleLine( root, nullptr, false, false, false );
    SHAPE_LINE_CHAIN simplified( l.CLine() );

    simplified.Simplify();

    if( simplified.PointCount() != l.PointCount() )
    {
        m_world->Remove( l );
        LINE lnew( l );
        lnew.SetShape( simplified );
        m_world->Add( lnew );
        return true;
    }

    return false;
}


const TOPOLOGY::JOINT_SET TOPOLOGY::ConnectedJoints( const JOINT* aStart )
{
    std::deque<const JOINT*> searchQueue;
    JOINT_SET                processed;

    searchQueue.push_back( aStart );
    processed.insert( aStart );

    while( !searchQueue.empty() )
    {
        const JOINT* current = searchQueue.front();
        searchQueue.pop_front();

        for( ITEM* item : current->LinkList() )
        {
            if( item->OfKind( ITEM::SEGMENT_T ) )
            {
                const JOINT* a = m_world->FindJoint( item->Anchor( 0 ), item );;
                const JOINT* b = m_world->FindJoint( item->Anchor( 1 ), item );;
                const JOINT* next = ( *a == *current ) ? b : a;

                if( processed.find( next ) == processed.end() )
                {
                    processed.insert( next );
                    searchQueue.push_back( next );
                }
            }
        }
    }

    return processed;
}


bool TOPOLOGY::NearestUnconnectedAnchorPoint( const LINE* aTrack, VECTOR2I& aPoint,
                                              PNS_LAYER_RANGE& aLayers, ITEM*& aItem )
{
    LINE track( *aTrack );
    VECTOR2I end;

    if( !track.PointCount() )
        return false;

    std::unique_ptr<NODE> tmpNode( m_world->Branch() );

    track.ClearLinks();
    tmpNode->Add( track );

    const JOINT* jt = tmpNode->FindJoint( track.CLastPoint(), &track );

    if( !jt || m_world->GetRuleResolver()->NetCode( jt->Net() ) <= 0 )
       return false;

    if( ( !track.EndsWithVia() && jt->LinkCount() >= 2 )
            || ( track.EndsWithVia() && jt->LinkCount() >= 3 ) ) // we got something connected
    {
        end = jt->Pos();
        aLayers = jt->Layers();
        aItem = jt->LinkList()[0];
    }
    else
    {
        int anchor;

        TOPOLOGY topo( tmpNode.get() );
        ITEM* it = topo.NearestUnconnectedItem( jt, &anchor );

        if( !it )
            return false;

        end = it->Anchor( anchor );
        aLayers = it->Layers();
        aItem = it;
    }

    aPoint = end;
    return true;
}


bool TOPOLOGY::LeadingRatLine( const LINE* aTrack, SHAPE_LINE_CHAIN& aRatLine )
{
    VECTOR2I end;
    // Ratline doesn't care about the layer
    PNS_LAYER_RANGE layers;
    ITEM*       unusedItem;

    if( !NearestUnconnectedAnchorPoint( aTrack, end, layers, unusedItem ) )
        return false;

    aRatLine.Clear();
    aRatLine.Append( aTrack->CLastPoint() );
    aRatLine.Append( end );
    return true;
}


ITEM* TOPOLOGY::NearestUnconnectedItem( const JOINT* aStart, int* aAnchor, int aKindMask )
{
    std::set<ITEM*> disconnected;

    m_world->AllItemsInNet( aStart->Net(), disconnected );

    for( const JOINT* jt : ConnectedJoints( aStart ) )
    {
        for( ITEM* link : jt->LinkList() )
        {
            if( disconnected.find( link ) != disconnected.end() )
                disconnected.erase( link );
        }
    }

    int best_dist = INT_MAX;
    ITEM* best = nullptr;

    for( ITEM* item : disconnected )
    {
        if( item->OfKind( aKindMask ) )
        {
            for( int i = 0; i < item->AnchorCount(); i++ )
            {
                VECTOR2I p = item->Anchor( i );
                int d = ( p - aStart->Pos() ).EuclideanNorm();

                if( d < best_dist )
                {
                    best_dist = d;
                    best = item;

                    if( aAnchor )
                        *aAnchor = i;
                }
            }
        }
    }

    return best;
}


TOPOLOGY::PATH_RESULT TOPOLOGY::followBranch( const JOINT* aStartJoint, LINKED_ITEM* aPrev,
                                              std::set<ITEM*>& aVisited,
                                              bool aFollowLockedSegments )
{
    using clock = std::chrono::steady_clock;

    PATH_RESULT best;
    best.m_end = aStartJoint;

    const int timeoutMs = ADVANCED_CFG::GetCfg().m_FollowBranchTimeout;
    auto startTime = clock::now();

    // State for iterative DFS: current joint, previous item, accumulated path items,
    // accumulated length, and the set of visited joints for this path
    struct STATE
    {
        const JOINT*            joint;
        LINKED_ITEM*            prev;
        ITEM_SET                pathItems;
        int                     pathLength;
        std::set<const JOINT*>  visitedJoints;
        ITEM*                   via;
    };

    std::stack<STATE> stateStack;

    // Initialize with starting state
    STATE initial;
    initial.joint = aStartJoint;
    initial.prev = aPrev;
    initial.pathLength = 0;
    initial.visitedJoints.insert( aStartJoint );
    initial.via = nullptr;

    stateStack.push( std::move( initial ) );

    while( !stateStack.empty() )
    {
        // Check timeout
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                clock::now() - startTime ).count();

        if( elapsed > timeoutMs )
        {
            wxLogTrace( wxT( "PNS_TUNE" ),
                        wxT( "followBranch: timeout after %lld ms, returning best path found" ),
                        elapsed );
            break;
        }

        STATE current = std::move( stateStack.top() );
        stateStack.pop();

        const JOINT* joint = current.joint;
        ITEM_SET links( joint->CLinks() );

        // Check for via at this joint
        ITEM* via = nullptr;

        for( ITEM* link : links )
        {
            if( link->OfKind( ITEM::VIA_T ) && !aVisited.contains( link ) )
            {
                via = link;
                break;
            }
        }

        // Find all unvisited branches from this joint
        bool foundBranch = false;

        for( ITEM* link : links )
        {
            if( !link->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
                continue;

            if( link == current.prev )
                continue;

            if( aVisited.contains( link ) )
                continue;

            LINE l = m_world->AssembleLine( static_cast<LINKED_ITEM*>( link ), nullptr,
                                            false, aFollowLockedSegments );

            if( l.CPoint( 0 ) != joint->Pos() )
                l.Reverse();

            const JOINT* nextJoint = m_world->FindJoint( l.CLastPoint(), &l );

            // Skip if we've already visited this joint in the current path
            if( current.visitedJoints.count( nextJoint ) )
                continue;

            foundBranch = true;

            // Build new state for this branch
            STATE nextState;
            nextState.joint = nextJoint;
            nextState.prev = l.Links().back();
            nextState.pathItems = current.pathItems;
            nextState.pathLength = current.pathLength + l.CLine().Length();
            nextState.visitedJoints = current.visitedJoints;
            nextState.visitedJoints.insert( nextJoint );
            nextState.via = via;

            // Add via and line to path
            if( via )
                nextState.pathItems.Add( via );

            nextState.pathItems.Add( l );

            stateStack.push( std::move( nextState ) );
        }

        // If no branches found, this is a terminal joint - check if it's the best path
        if( !foundBranch )
        {
            if( current.pathLength > best.m_length )
            {
                best.m_length = current.pathLength;
                best.m_end = joint;
                best.m_items = current.pathItems;
            }
        }
    }

    wxLogTrace( wxT( "PNS_TUNE" ),
                wxT( "followBranch: completed with best path length=%d, %d items" ),
                best.m_length, best.m_items.Size() );

    return best;
}


ITEM_SET TOPOLOGY::followTrivialPath( LINE* aLine2, const JOINT** aTerminalJointA,
                                      const JOINT** aTerminalJointB,
                                      bool aFollowLockedSegments )
{
    assert( aLine2->IsLinked() );

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "=== followTrivialPath START ===" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "followTrivialPath: initial line has %d segments, %zu links" ),
                aLine2->SegmentCount(), aLine2->Links().size() );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "followTrivialPath: line endpoints: (%d,%d) to (%d,%d)" ),
                aLine2->CPoint( 0 ).x, aLine2->CPoint( 0 ).y,
                aLine2->CLastPoint().x, aLine2->CLastPoint().y );

    ITEM_SET path;
    path.Add( *aLine2 );

    std::set<ITEM*> visited;

    for( LINKED_ITEM* link : aLine2->Links() )
        visited.insert( link );

    const JOINT* jtA = m_world->FindJoint( aLine2->CPoint( 0 ), aLine2 );
    const JOINT* jtB = m_world->FindJoint( aLine2->CLastPoint(), aLine2 );

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "followTrivialPath: LEFT branch starting from joint at (%d,%d)" ),
                jtA->Pos().x, jtA->Pos().y );
    PATH_RESULT left = followBranch( jtA, aLine2->Links().front(), visited, aFollowLockedSegments );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "followTrivialPath: LEFT branch result: length=%d, %d items" ),
                left.m_length, left.m_items.Size() );

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "followTrivialPath: RIGHT branch starting from joint at (%d,%d)" ),
                jtB->Pos().x, jtB->Pos().y );
    PATH_RESULT right = followBranch( jtB, aLine2->Links().back(), visited, aFollowLockedSegments );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "followTrivialPath: RIGHT branch result: length=%d, %d items" ),
                right.m_length, right.m_items.Size() );

    if( aTerminalJointA )
        *aTerminalJointA = left.m_end;

    if( aTerminalJointB )
        *aTerminalJointB = right.m_end;

    // Count segments as we build the final path
    int leftSegCount = 0;
    int rightSegCount = 0;
    int initialSegCount = 0;

    // Count initial segments
    for( int i = 0; i < aLine2->SegmentCount(); i++ )
        initialSegCount++;

    // Add left items
    for( ITEM* item : left.m_items )
    {
        path.Prepend( item );
        if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
        {
            LINE* l = dynamic_cast<LINE*>( item );
            if( l )
                leftSegCount += l->SegmentCount();
            else
                leftSegCount++;
        }
    }

    // Add right items
    for( ITEM* item : right.m_items )
    {
        path.Add( item );
        if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
        {
            LINE* l = dynamic_cast<LINE*>( item );
            if( l )
                rightSegCount += l->SegmentCount();
            else
                rightSegCount++;
        }
    }

    // Calculate total path length
    int totalLength = left.m_length + aLine2->CLine().Length() + right.m_length;

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "=== followTrivialPath SUMMARY ===" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "Starting segment count: %d" ), initialSegCount );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "Left branch: %d segments, length=%d" ), leftSegCount, left.m_length );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "Initial line: %d segments, length=%lld" ), initialSegCount, aLine2->CLine().Length() );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "Right branch: %d segments, length=%d" ), rightSegCount, right.m_length );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "Total segments in path: %d" ), leftSegCount + initialSegCount + rightSegCount );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "Total path length: %d" ), totalLength );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "Total items in result: %d" ), path.Size() );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "=== followTrivialPath END ===" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );

    return path;
}


const ITEM_SET TOPOLOGY::AssembleTrivialPath( ITEM* aStart,
                                              std::pair<const JOINT*, const JOINT*>* aTerminalJoints,
                                              bool aFollowLockedSegments )
{
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "*** AssembleTrivialPath: START ***" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTrivialPath: aStart=%p, kind=%s" ),
                aStart, aStart->KindStr().c_str() );

    ITEM_SET        path;
    LINKED_ITEM*    seg = nullptr;

    if( aStart->Kind() == ITEM::VIA_T )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTrivialPath: starting from VIA" ) );
        VIA*         via = static_cast<VIA*>( aStart );
        const JOINT* jt  = m_world->FindJoint( via->Pos(), via );

        if( !jt->IsNonFanoutVia() )
        {
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTrivialPath: VIA is fanout, returning empty" ) );
            return ITEM_SET();
        }

        ITEM_SET links( jt->CLinks() );

        for( ITEM* item : links )
        {
            if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
            {
                seg = static_cast<LINKED_ITEM*>( item );
                wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTrivialPath: found segment/arc from VIA" ) );
                break;
            }
        }
    }
    else if( aStart->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
    {
        seg = static_cast<LINKED_ITEM*>( aStart );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTrivialPath: starting from SEGMENT/ARC" ) );
    }

    if( !seg )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTrivialPath: no segment found, returning empty" ) );
        return ITEM_SET();
    }

    // Assemble a line following through locked segments
    // TODO: consider if we want to allow tuning lines with different widths in the future
    LINE l = m_world->AssembleLine( seg, nullptr, false, aFollowLockedSegments );

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTrivialPath: assembled line with %d segments, length=%lld" ),
                l.SegmentCount(), l.CLine().Length() );

    const JOINT* jointA = nullptr;
    const JOINT* jointB = nullptr;

    path = followTrivialPath( &l, &jointA, &jointB, aFollowLockedSegments );

    if( aTerminalJoints )
    {
        wxASSERT( jointA && jointB );
        *aTerminalJoints = std::make_pair( jointA, jointB );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTrivialPath: terminal joints at (%d,%d) and (%d,%d)" ),
                    jointA->Pos().x, jointA->Pos().y, jointB->Pos().x, jointB->Pos().y );
    }

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTrivialPath: returning path with %d items" ), path.Size() );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "*** AssembleTrivialPath: END ***" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );

    return path;
}


const ITEM_SET TOPOLOGY::AssembleTuningPath( ROUTER_IFACE* aRouterIface, ITEM* aStart, SOLID** aStartPad,
                                             SOLID** aEndPad )
{
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "########## AssembleTuningPath: START ##########" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTuningPath: aStart=%p, kind=%s" ),
                aStart, aStart->KindStr().c_str() );

    std::pair<const JOINT*, const JOINT*> joints;
    ITEM_SET initialPath = AssembleTrivialPath( aStart, &joints, true );

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTuningPath: initial path has %d items" ), initialPath.Size() );

    PAD* padA = nullptr;
    PAD* padB = nullptr;

    auto getPadFromJoint =
            []( const JOINT* aJoint, PAD** aTargetPad, SOLID** aTargetSolid )
            {
                for( ITEM* item : aJoint->LinkList() )
                {
                    if( item->OfKind( ITEM::SOLID_T ) )
                    {
                        BOARD_ITEM* bi = static_cast<SOLID*>( item )->Parent();

                        if( bi->Type() == PCB_PAD_T )
                        {
                            *aTargetPad = static_cast<PAD*>( bi );

                            if( aTargetSolid )
                                *aTargetSolid = static_cast<SOLID*>( item );
                        }

                        break;
                    }
                }
            };

    if( joints.first )
    {
        getPadFromJoint( joints.first, &padA, aStartPad );
        if( padA )
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTuningPath: found start pad at joint (%d,%d)" ),
                        joints.first->Pos().x, joints.first->Pos().y );
    }

    if( joints.second )
    {
        getPadFromJoint( joints.second, &padB, aEndPad );
        if( padB )
            wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTuningPath: found end pad at joint (%d,%d)" ),
                        joints.second->Pos().x, joints.second->Pos().y );
    }

    if( !padA && !padB )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTuningPath: no pads found, returning initial path" ) );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "########## AssembleTuningPath: END ##########" ) );
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );
        return initialPath;
    }

    auto processPad = [&]( PAD* aPad, int aLayer )
    {
        wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTuningPath: processing pad, optimizing trace in pad" ) );
        for( int idx = 0; idx < initialPath.Size(); idx++ )
        {
            if( initialPath[idx]->Kind() != ITEM::LINE_T )
                continue;

            LINE*        line = static_cast<LINE*>( initialPath[idx] );
            SHAPE_LINE_CHAIN&  slc = line->Line();
            const PCB_LAYER_ID pcbLayer = aRouterIface->GetBoardLayerFromPNSLayer( line->Layer() );

            int lengthBefore = slc.Length();
            LENGTH_DELAY_CALCULATION::OptimiseTraceInPad( slc, aPad, pcbLayer );
            int lengthAfter = slc.Length();

            if( lengthBefore != lengthAfter )
                wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTuningPath: optimized line %d, length changed from %d to %d" ),
                            idx, lengthBefore, lengthAfter );
        }
    };

    if( padA )
        processPad( padA, joints.first->Layer() );

    if( padB )
        processPad( padB, joints.second->Layer() );

    // Calculate total path length
    int totalLength = 0;
    int lineCount = 0;
    for( int idx = 0; idx < initialPath.Size(); idx++ )
    {
        if( initialPath[idx]->Kind() == ITEM::LINE_T )
        {
            LINE* line = static_cast<LINE*>( initialPath[idx] );
            totalLength += line->CLine().Length();
            lineCount++;
        }
    }

    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "AssembleTuningPath: final path has %d items, %d lines, total length=%d" ),
                initialPath.Size(), lineCount, totalLength );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "########## AssembleTuningPath: END ##########" ) );
    wxLogTrace( wxT( "PNS_TUNE" ), wxT( "" ) );

    return initialPath;
}


const ITEM_SET TOPOLOGY::ConnectedItems( const JOINT* aStart, int aKindMask )
{
    return ITEM_SET();
}


const ITEM_SET TOPOLOGY::ConnectedItems( ITEM* aStart, int aKindMask )
{
    return ITEM_SET();
}


bool commonParallelProjection( SEG p, SEG n, SEG &pClip, SEG& nClip );


bool TOPOLOGY::AssembleDiffPair( ITEM* aStart, DIFF_PAIR& aPair )
{
    NET_HANDLE   refNet = aStart->Net();
    NET_HANDLE   coupledNet = m_world->GetRuleResolver()->DpCoupledNet( refNet );
    LINKED_ITEM* startItem = dynamic_cast<LINKED_ITEM*>( aStart );

    if( !coupledNet || !startItem )
        return false;

    LINE lp = m_world->AssembleLine( startItem );

    std::vector<ITEM*> pItems;
    std::vector<ITEM*> nItems;

    for( ITEM* item : lp.Links() )
    {
        if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) && item->Layers() == startItem->Layers() )
            pItems.push_back( item );
    }

    std::set<ITEM*> coupledItems;
    m_world->AllItemsInNet( coupledNet, coupledItems );

    for( ITEM* item : coupledItems )
    {
        if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) && item->Layers() == startItem->Layers() )
            nItems.push_back( item );
    }

    LINKED_ITEM* refItem = nullptr;
    LINKED_ITEM* coupledItem = nullptr;
    SEG::ecoord  minDist_sq = std::numeric_limits<SEG::ecoord>::max();
    SEG::ecoord  minDistTarget_sq = std::numeric_limits<SEG::ecoord>::max();
    VECTOR2I     targetPoint = aStart->Shape( -1 )->Centre();

    auto findNItem = [&]( ITEM* p_item )
    {
        for( ITEM* n_item : nItems )
        {
            SEG::ecoord dist_sq = std::numeric_limits<SEG::ecoord>::max();

            if( n_item->Kind() != p_item->Kind() )
                continue;

            if( p_item->Kind() == ITEM::SEGMENT_T )
            {
                const SEGMENT* p_seg = static_cast<const SEGMENT*>( p_item );
                const SEGMENT* n_seg = static_cast<const SEGMENT*>( n_item );

                if( n_seg->Width() != p_seg->Width() )
                    continue;

                if( !p_seg->Seg().ApproxParallel( n_seg->Seg(), DP_PARALLELITY_THRESHOLD ) )
                    continue;

                SEG p_clip, n_clip;

                if( !commonParallelProjection( p_seg->Seg(), n_seg->Seg(), p_clip, n_clip ) )
                    continue;

                dist_sq = n_seg->Seg().SquaredDistance( p_seg->Seg() );
            }
            else if( p_item->Kind() == ITEM::ARC_T )
            {
                const ARC* p_arc = static_cast<const ARC*>( p_item );
                const ARC* n_arc = static_cast<const ARC*>( n_item );

                if( n_arc->Width() != p_arc->Width() )
                    continue;

                VECTOR2I    centerDiff = n_arc->CArc().GetCenter() - p_arc->CArc().GetCenter();
                SEG::ecoord centerDist_sq = centerDiff.SquaredEuclideanNorm();

                if( centerDist_sq > SEG::Square( DP_PARALLELITY_THRESHOLD ) )
                    continue;

                dist_sq = SEG::Square( p_arc->CArc().GetRadius() - n_arc->CArc().GetRadius() );
            }

            if( dist_sq <= minDist_sq )
            {
                SEG::ecoord distTarget_sq = n_item->Shape( -1 )->SquaredDistance( targetPoint );
                if( distTarget_sq < minDistTarget_sq )
                {
                    minDistTarget_sq = distTarget_sq;
                    minDist_sq = dist_sq;

                    refItem = static_cast<LINKED_ITEM*>( p_item );
                    coupledItem = static_cast<LINKED_ITEM*>( n_item );
                }
            }
        }
    };

    findNItem( startItem );

    if( !coupledItem )
    {
        LINKED_ITEM*    linked = static_cast<LINKED_ITEM*>( startItem );
        std::set<ITEM*> linksToTest;

        for( int i = 0; i < linked->AnchorCount(); i++ )
        {
            const JOINT* jt = m_world->FindJoint( linked->Anchor( i ), linked );

            if( !jt )
                continue;

            for( ITEM* link : jt->LinkList() )
            {
                if( link != linked )
                    linksToTest.emplace( link );
            }
        }

        for( ITEM* link : linksToTest )
            findNItem( link );
    }

    if( !coupledItem )
        return false;

    LINE ln = m_world->AssembleLine( coupledItem );

    if( m_world->GetRuleResolver()->DpNetPolarity( refNet ) < 0 )
        std::swap( lp, ln );

    int gap = -1;

    if( refItem && refItem->Kind() == ITEM::SEGMENT_T )
    {
        // Segments are parallel -> compute pair gap
        const VECTOR2I refDir       = refItem->Anchor( 1 ) - refItem->Anchor( 0 );
        const VECTOR2I displacement = refItem->Anchor( 1 ) - coupledItem->Anchor( 1 );
        gap = (int) std::abs( refDir.Cross( displacement ) / refDir.EuclideanNorm() ) - lp.Width();
    }
    else if( refItem && refItem->Kind() == ITEM::ARC_T )
    {
        const ARC* refArc = static_cast<ARC*>( refItem );
        const ARC* coupledArc = static_cast<ARC*>( coupledItem );
        gap = (int) std::abs( refArc->CArc().GetRadius() - coupledArc->CArc().GetRadius() ) - lp.Width();
    }

    aPair = DIFF_PAIR( lp, ln );
    aPair.SetWidth( lp.Width() );
    aPair.SetLayers( lp.Layers() );
    aPair.SetGap( gap );

    return true;
}

const TOPOLOGY::CLUSTER TOPOLOGY::AssembleCluster( ITEM* aStart, int aLayer, double aAreaExpansionLimit, NET_HANDLE aExcludedNet )
{
    CLUSTER cluster;
    std::deque<ITEM*> pending;

    COLLISION_SEARCH_OPTIONS opts;

    opts.m_differentNetsOnly = false;
    opts.m_overrideClearance = 0;

    pending.push_back( aStart );

    BOX2I clusterBBox = aStart->Shape( aLayer )->BBox();
    int64_t initialArea = clusterBBox.GetArea();

    while( !pending.empty() )
    {
        NODE::OBSTACLES obstacles;
        ITEM* top = pending.front();

        pending.pop_front();

        cluster.m_items.insert( top );

        m_world->QueryColliding( top, obstacles, opts ); // only query touching objects

        for( const OBSTACLE& obs : obstacles )
        {
            bool trackOnTrack = ( obs.m_item->Net() != top->Net() ) &&  obs.m_item->OfKind( ITEM::SEGMENT_T ) && top->OfKind( ITEM::SEGMENT_T );

            if( trackOnTrack )
                continue;

            if( aExcludedNet && obs.m_item->Net() == aExcludedNet )
                continue;

            if( obs.m_item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) && obs.m_item->Layers().Overlaps( aLayer ) )
            {
                auto line = m_world->AssembleLine( static_cast<LINKED_ITEM*>(obs.m_item) );
                clusterBBox.Merge( line.CLine().BBox() );
            }
            else
            {
                clusterBBox.Merge( obs.m_item->Shape( aLayer )->BBox() );
            }

            const int64_t currentArea = clusterBBox.GetArea();
            const double areaRatio = (double) currentArea / (double) ( initialArea + 1 );

            if( aAreaExpansionLimit > 0.0 && areaRatio > aAreaExpansionLimit )
                break;

            if( cluster.m_items.find( obs.m_item ) == cluster.m_items.end() &&
                obs.m_item->Layers().Overlaps( aLayer ) && !( obs.m_item->Marker() & MK_HEAD ) )
            {
                cluster.m_items.insert( obs.m_item );
                pending.push_back( obs.m_item );
            }
        }
    }

    return cluster;
}

}
