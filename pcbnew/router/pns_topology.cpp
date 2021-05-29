/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "pns_node.h"
#include "pns_joint.h"
#include "pns_solid.h"
#include "pns_router.h"
#include "pns_utils.h"

#include "pns_diff_pair.h"
#include "pns_topology.h"

#include <board.h>

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


const TOPOLOGY::JOINT_SET TOPOLOGY::ConnectedJoints( JOINT* aStart )
{
    std::deque<JOINT*> searchQueue;
    JOINT_SET processed;

    searchQueue.push_back( aStart );
    processed.insert( aStart );

    while( !searchQueue.empty() )
    {
        JOINT* current = searchQueue.front();
        searchQueue.pop_front();

        for( ITEM* item : current->LinkList() )
        {
            if( item->OfKind( ITEM::SEGMENT_T ) )
            {
                SEGMENT* seg = static_cast<SEGMENT*>( item );
                JOINT* a = m_world->FindJoint( seg->Seg().A, seg );
                JOINT* b = m_world->FindJoint( seg->Seg().B, seg );
                JOINT* next = ( *a == *current ) ? b : a;

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


bool TOPOLOGY::LeadingRatLine( const LINE* aTrack, SHAPE_LINE_CHAIN& aRatLine )
{
    LINE track( *aTrack );
    VECTOR2I end;

    if( !track.PointCount() )
        return false;

    std::unique_ptr<NODE> tmpNode( m_world->Branch() );
    tmpNode->Add( track );

    JOINT* jt = tmpNode->FindJoint( track.CPoint( -1 ), &track );

    if( !jt || jt->Net() <= 0 )
       return false;

    if( ( !track.EndsWithVia() && jt->LinkCount() >= 2 )
            || ( track.EndsWithVia() && jt->LinkCount() >= 3 ) ) // we got something connected
    {
        end = jt->Pos();
    }
    else
    {
        int anchor;

        TOPOLOGY topo( tmpNode.get() );
        ITEM* it = topo.NearestUnconnectedItem( jt, &anchor );

        if( !it )
            return false;

        end = it->Anchor( anchor );
    }

    aRatLine.Clear();
    aRatLine.Append( track.CPoint( -1 ) );
    aRatLine.Append( end );
    return true;
}


ITEM* TOPOLOGY::NearestUnconnectedItem( JOINT* aStart, int* aAnchor, int aKindMask )
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
    ITEM* best = NULL;

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


bool TOPOLOGY::followTrivialPath( LINE* aLine, bool aLeft, ITEM_SET& aSet,
                                  std::set<ITEM*>& aVisited, JOINT** aTerminalJoint )
{
    assert( aLine->IsLinked() );

    VECTOR2I     anchor = aLeft ? aLine->CPoint( 0 ) : aLine->CPoint( -1 );
    LINKED_ITEM* last   = aLeft ? aLine->Links().front() : aLine->Links().back();
    JOINT*       jt     = m_world->FindJoint( anchor, aLine );

    assert( jt != NULL );

    aVisited.insert( last );

    if( jt->IsNonFanoutVia() || jt->IsTraceWidthChange() )
    {
        ITEM* via = NULL;
        SEGMENT* next_seg = NULL;

        for( ITEM* link : jt->Links().Items() )
        {
            if( link->OfKind( ITEM::VIA_T ) )
                via = link;
            else if( aVisited.find( link ) == aVisited.end() )
                next_seg = static_cast<SEGMENT*>( link );
        }

        if( !next_seg )
        {
            if( aTerminalJoint )
                *aTerminalJoint = jt;

            return false;
        }

        LINE l = m_world->AssembleLine( next_seg );

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
        }
        else
        {
            if( via )
                aSet.Add( via );

            aSet.Add( l );
        }

        return followTrivialPath( &l, aLeft, aSet, aVisited, aTerminalJoint );
    }

    if( aTerminalJoint )
        *aTerminalJoint = jt;

    return false;
}


const ITEM_SET TOPOLOGY::AssembleTrivialPath( ITEM* aStart,
                                              std::pair<JOINT*, JOINT*>* aTerminalJoints )
{
    ITEM_SET        path;
    std::set<ITEM*> visited;
    LINKED_ITEM*    seg = nullptr;

    if( aStart->Kind() == ITEM::VIA_T )
    {
        VIA*   via = static_cast<VIA*>( aStart );
        JOINT* jt  = m_world->FindJoint( via->Pos(), via );

        if( !jt->IsNonFanoutVia() )
            return ITEM_SET();

        for( const ITEM_SET::ENTRY& entry : jt->Links().Items() )
        {
            if( entry.item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
            {
                seg = static_cast<LINKED_ITEM*>( entry.item );
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

    LINE l = m_world->AssembleLine( seg );

    path.Add( l );

    JOINT* jointA = nullptr;
    JOINT* jointB = nullptr;

    followTrivialPath( &l, false, path, visited, &jointA );
    followTrivialPath( &l, true, path, visited, &jointB );

    if( aTerminalJoints )
    {
        wxASSERT( jointA && jointB );
        *aTerminalJoints = std::make_pair( jointA, jointB );
    }

    return path;
}


const ITEM_SET TOPOLOGY::AssembleTuningPath( ITEM* aStart, SOLID** aStartPad, SOLID** aEndPad )
{
    std::pair<JOINT*, JOINT*> joints;
    ITEM_SET initialPath = AssembleTrivialPath( aStart, &joints );

    PAD* padA = nullptr;
    PAD* padB = nullptr;

    auto getPadFromJoint =
            []( JOINT* aJoint, PAD** aTargetPad, SOLID** aTargetSolid )
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
                const std::shared_ptr<SHAPE_POLY_SET>& shape = aPad->GetEffectivePolygon();

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
                    else if( containsB &&
                             ( aForward ? vertex < aLine.PointCount() - 1 : vertex > 0 ) )
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
            [&]( JOINT* aJoint, PAD* aPad )
            {
                const std::shared_ptr<SHAPE_POLY_SET>& shape = aPad->GetEffectivePolygon();

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


const ITEM_SET TOPOLOGY::ConnectedItems( JOINT* aStart, int aKindMask )
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
    int refNet = aStart->Net();
    int coupledNet = m_world->GetRuleResolver()->DpCoupledNet( refNet );

    if( coupledNet < 0 )
        return false;

    std::set<ITEM*> coupledItems;

    m_world->AllItemsInNet( coupledNet, coupledItems );

    SEGMENT* coupledSeg = NULL, *refSeg;
    int minDist = std::numeric_limits<int>::max();

    if( ( refSeg = dyn_cast<SEGMENT*>( aStart ) ) != NULL )
    {
        for( ITEM* item : coupledItems )
        {
            if( SEGMENT* s = dyn_cast<SEGMENT*>( item ) )
            {
                if( s->Layers().Start() == refSeg->Layers().Start() && s->Width() == refSeg->Width() )
                {
                    int dist = s->Seg().Distance( refSeg->Seg() );
                    bool isParallel = refSeg->Seg().ApproxParallel( s->Seg() );
                    SEG p_clip, n_clip;

                    bool isCoupled = commonParallelProjection( refSeg->Seg(), s->Seg(), p_clip, n_clip );

                    if( isParallel && isCoupled && dist < minDist )
                    {
                        minDist = dist;
                        coupledSeg = s;
                    }
                }
            }
        }
    }
    else
    {
        return false;
    }

    if( !coupledSeg )
        return false;

    LINE lp = m_world->AssembleLine( refSeg );
    LINE ln = m_world->AssembleLine( coupledSeg );

    if( m_world->GetRuleResolver()->DpNetPolarity( refNet ) < 0 )
    {
        std::swap( lp, ln );
    }

    int gap = -1;

    if( refSeg->Seg().ApproxParallel( coupledSeg->Seg() ) )
    {
        // Segments are parallel -> compute pair gap
        const VECTOR2I refDir       = refSeg->Anchor( 1 ) - refSeg->Anchor( 0 );
        const VECTOR2I displacement = refSeg->Anchor( 1 ) - coupledSeg->Anchor( 1 );
        gap = (int) std::abs( refDir.Cross( displacement ) / refDir.EuclideanNorm() ) - lp.Width();
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

    pending.push_back( aStart );

    while( !pending.empty() )
    {
        NODE::OBSTACLES obstacles;
        ITEM* top = pending.front();

        pending.pop_front();

        visited.insert( top );

        m_world->QueryColliding( top, obstacles, ITEM::ANY_T, -1, false );

        for( OBSTACLE& obs : obstacles )
        {
            if( visited.find( obs.m_item ) == visited.end() && obs.m_item->Layers().Overlaps( aLayer ) && !( obs.m_item->Marker() & MK_HEAD ) )
            {
                visited.insert( obs.m_item );
                pending.push_back( obs.m_item );
            }
        }
    }

    return visited;
}

}
