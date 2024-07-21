/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pad.h>

namespace PNS {

bool TOPOLOGY::SimplifyLine( LINE* aLine )
{
    if( !aLine->IsLinked() || !aLine->SegmentCount() )
        return false;

    LINKED_ITEM* root = aLine->GetLink( 0 );
    LINE l = m_world->AssembleLine( root );
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
    tmpNode->Add( track );

    const JOINT* jt = tmpNode->FindJoint( track.CPoint( -1 ), &track );

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
    aRatLine.Append( aTrack->CPoint( -1 ) );
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


bool TOPOLOGY::followTrivialPath( LINE* aLine2, bool aLeft, ITEM_SET& aSet,
                                  const JOINT** aTerminalJoint, bool aFollowLockedSegments )
{
    assert( aLine2->IsLinked() );
    LINE*           curr_line = aLine2;
    std::set<ITEM*> visited;

    while( true )
    {
        VECTOR2I     anchor = aLeft ? curr_line->CPoint( 0 ) : curr_line->CPoint( -1 );
        LINKED_ITEM* last = aLeft ? curr_line->Links().front() : curr_line->Links().back();
        const JOINT* jt = m_world->FindJoint( anchor, curr_line );

        assert( jt != nullptr );

        if( !visited.insert( last ).second
            || ( !jt->IsNonFanoutVia() && !jt->IsTraceWidthChange() ) )
        {
            if( aTerminalJoint )
                *aTerminalJoint = jt;

            return false;
        }

        ITEM*    via = nullptr;
        SEGMENT* next_seg = nullptr;

        ITEM_SET links( jt->CLinks() );

        for( ITEM* link : links )
        {
            if( link->OfKind( ITEM::VIA_T ) )
                via = link;
            else if( visited.insert( link ).second )
                next_seg = static_cast<SEGMENT*>( link );
        }

        if( !next_seg )
        {
            if( aTerminalJoint )
                *aTerminalJoint = jt;

            return false;
        }

        LINE     l = m_world->AssembleLine( next_seg, nullptr, false, aFollowLockedSegments );
        VECTOR2I nextAnchor = ( aLeft ? l.CLine().CPoint( -1 ) : l.CLine().CPoint( 0 ) );

        if( nextAnchor != anchor )
        {
            l.Reverse();
        }

        if( aLeft )
        {
            if( via )
                aSet.Prepend( via );

            aSet.Prepend( l );
            curr_line = static_cast<PNS::LINE*>( aSet[0] );
        }
        else
        {
            if( via )
                aSet.Add( via );

            aSet.Add( l );
            curr_line = static_cast<PNS::LINE*>( aSet[aSet.Size() - 1] );
        }

        continue;
    }
}


const ITEM_SET TOPOLOGY::AssembleTrivialPath( ITEM* aStart,
                                              std::pair<const JOINT*, const JOINT*>* aTerminalJoints,
                                              bool aFollowLockedSegments )
{
    ITEM_SET        path;
    LINKED_ITEM*    seg = nullptr;

    if( aStart->Kind() == ITEM::VIA_T )
    {
        VIA*         via = static_cast<VIA*>( aStart );
        const JOINT* jt  = m_world->FindJoint( via->Pos(), via );

        if( !jt->IsNonFanoutVia() )
            return ITEM_SET();

        ITEM_SET links( jt->CLinks() );

        for( ITEM* item : links )
        {
            if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
            {
                seg = static_cast<LINKED_ITEM*>( item );
                break;
            }
        }
    }
    else if( aStart->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
    {
        seg = static_cast<LINKED_ITEM*>( aStart );
    }

    if( !seg )
        return ITEM_SET();

    // Assemble a line following through locked segments
    // TODO: consider if we want to allow tuning lines with different widths in the future
    LINE l = m_world->AssembleLine( seg, nullptr, false, aFollowLockedSegments );

    path.Add( l );

    const JOINT* jointA = nullptr;
    const JOINT* jointB = nullptr;

    followTrivialPath( &l, false, path, &jointB, aFollowLockedSegments );
    followTrivialPath( &l, true, path, &jointA, aFollowLockedSegments );

    if( aTerminalJoints )
    {
        wxASSERT( jointA && jointB );
        *aTerminalJoints = std::make_pair( jointA, jointB );
    }

    return path;
}


const ITEM_SET TOPOLOGY::AssembleTuningPath( ITEM* aStart, SOLID** aStartPad, SOLID** aEndPad )
{
    std::pair<const JOINT*, const JOINT*> joints;
    ITEM_SET initialPath = AssembleTrivialPath( aStart, &joints, true );

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
        getPadFromJoint( joints.first, &padA, aStartPad );

    if( joints.second )
        getPadFromJoint( joints.second, &padB, aEndPad );

    if( !padA && !padB )
        return initialPath;

    auto clipLineToPad =
            []( SHAPE_LINE_CHAIN& aLine, PAD* aPad, bool aForward = true )
            {
                const auto& shape = aPad->GetEffectivePolygon( ERROR_INSIDE );

                int start = aForward ? 0 : aLine.PointCount() - 1;
                int delta = aForward ? 1 : -1;

                // Skip the "first" (or last) vertex, we already know it's contained in the pad
                int clip = start;

                for( int vertex = start + delta;
                     aForward ? vertex < aLine.PointCount() : vertex >= 0;
                     vertex += delta )
                {
                    SEG seg( aLine.GetPoint( vertex ), aLine.GetPoint( vertex - delta ) );

                    bool containsA = shape->Contains( seg.A );
                    bool containsB = shape->Contains( seg.B );

                    if( containsA && containsB )
                    {
                        // Whole segment is inside: clip out this segment
                        clip = vertex;
                    }
                    else if( containsB )
                    {
                        // Only one point inside: Find the intersection
                        VECTOR2I loc;

                        if( shape->Collide( seg, 0, nullptr, &loc ) )
                        {
                            aLine.Replace( vertex - delta, vertex - delta, loc );
                        }
                    }
                }

                if( !aForward && clip < start )
                    aLine.Remove( clip + 1, start );
                else if( clip > start )
                    aLine.Remove( start, clip - 1 );

                // Now connect the dots
                aLine.Insert( aForward ? 0 : aLine.PointCount(), aPad->GetPosition() );
            };

    auto processPad =
            [&]( const JOINT* aJoint, PAD* aPad )
            {
                const auto& shape = aPad->GetEffectivePolygon( ERROR_INSIDE );

                for( int idx = 0; idx < initialPath.Size(); idx++ )
                {
                    if( initialPath[idx]->Kind() != ITEM::LINE_T )
                        continue;

                    LINE* line = static_cast<LINE*>( initialPath[idx] );

                    if( !aPad->FlashLayer( line->Layer() ) )
                        continue;

                    const std::vector<VECTOR2I>& points = line->CLine().CPoints();

                    if( points.front() != aJoint->Pos() && points.back() != aJoint->Pos() )
                        continue;

                    SHAPE_LINE_CHAIN& slc = line->Line();

                    if( shape->Contains( slc.CPoint( 0 ) ) )
                        clipLineToPad( slc, aPad, true );
                    else if( shape->Contains( slc.CPoint( -1 ) ) )
                        clipLineToPad( slc, aPad, false );
                }
            };

    if( padA )
        processPad( joints.first, padA );

    if( padB )
        processPad( joints.second, padB );

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
    VECTOR2I     targetPoint = aStart->Shape()->Centre();

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
                SEG::ecoord distTarget_sq = n_item->Shape()->SquaredDistance( targetPoint );
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

const std::set<ITEM*> TOPOLOGY::AssembleCluster( ITEM* aStart, int aLayer )
{
    std::set<ITEM*> visited;
    std::deque<ITEM*> pending;

    COLLISION_SEARCH_OPTIONS opts;

    opts.m_differentNetsOnly = false;
    opts.m_overrideClearance = 0;

    pending.push_back( aStart );

    while( !pending.empty() )
    {
        NODE::OBSTACLES obstacles;
        ITEM* top = pending.front();

        pending.pop_front();

        visited.insert( top );

        m_world->QueryColliding( top, obstacles, opts ); // only query touching objects

        for( const OBSTACLE& obs : obstacles )
        {
            bool trackOnTrack = ( obs.m_item->Net() != top->Net() ) &&  obs.m_item->OfKind( ITEM::SEGMENT_T ) && top->OfKind( ITEM::SEGMENT_T );

            if( trackOnTrack )
                continue;

            if( visited.find( obs.m_item ) == visited.end() &&
                obs.m_item->Layers().Overlaps( aLayer ) && !( obs.m_item->Marker() & MK_HEAD ) )
            {
                visited.insert( obs.m_item );
                pending.push_back( obs.m_item );
            }
        }
    }

    return visited;
}

}
