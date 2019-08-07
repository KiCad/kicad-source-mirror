/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include <deque>
#include <cassert>

#include "pns_line.h"
#include "pns_node.h"
#include "pns_debug_decorator.h"
#include "pns_walkaround.h"
#include "pns_shove.h"
#include "pns_solid.h"
#include "pns_optimizer.h"
#include "pns_via.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_topology.h"

#include "time_limit.h"


typedef VECTOR2I::extended_type ecoord;

namespace PNS {

void SHOVE::replaceItems( ITEM* aOld, std::unique_ptr< ITEM > aNew )
{
    OPT_BOX2I changed_area = ChangedArea( aOld, aNew.get() );

    if( changed_area )
        m_affectedArea = m_affectedArea ? m_affectedArea->Merge( *changed_area ) : *changed_area;

    m_currentNode->Replace( aOld, std::move( aNew ) );
}

void SHOVE::replaceLine( LINE& aOld, LINE& aNew )
{
    OPT_BOX2I changed_area = ChangedArea( aOld, aNew );

    if( changed_area )
        m_affectedArea = m_affectedArea ? m_affectedArea->Merge( *changed_area ) : *changed_area;

    m_currentNode->Replace( aOld, aNew );
}

int SHOVE::getClearance( const ITEM* aA, const ITEM* aB ) const
{
    if( m_forceClearance >= 0 )
        return m_forceClearance;

    return m_currentNode->GetClearance( aA, aB );
}


void SHOVE::sanityCheck( LINE* aOld, LINE* aNew )
{
    assert( aOld->CPoint( 0 ) == aNew->CPoint( 0 ) );
    assert( aOld->CPoint( -1 ) == aNew->CPoint( -1 ) );
}


SHOVE::SHOVE( NODE* aWorld, ROUTER* aRouter ) :
    ALGO_BASE( aRouter )
{
    m_forceClearance = -1;
    m_root = aWorld;
    m_currentNode = aWorld;
    SetDebugDecorator( aRouter->GetInterface()->GetDebugDecorator() );

    // Initialize other temporary variables:
    m_draggedVia = NULL;
    m_iter = 0;
    m_multiLineMode = false;
}


SHOVE::~SHOVE()
{
}


LINE SHOVE::assembleLine( const SEGMENT* aSeg, int* aIndex )
{
    return m_currentNode->AssembleLine( const_cast<SEGMENT*>( aSeg ), aIndex, true );
}

// A dumb function that checks if the shoved line is shoved the right way, e.g.
// visually "outwards" of the line/via applying pressure on it. Unfortunately there's no
// mathematical concept of orientation of an open curve, so we use some primitive heuristics:
// if the shoved line wraps around the start of the "pusher", it's likely shoved in wrong direction.
bool SHOVE::checkBumpDirection( const LINE& aCurrent, const LINE& aShoved ) const
{
    const SEG& ss = aCurrent.CSegment( 0 );

    int dist = getClearance( &aCurrent, &aShoved ) + PNS_HULL_MARGIN;

    dist += aCurrent.Width() / 2;
    dist += aShoved.Width() / 2;

    const VECTOR2I ps = ss.A - ( ss.B - ss.A ).Resize( dist );

    return !aShoved.CLine().PointOnEdge( ps );
}


SHOVE::SHOVE_STATUS SHOVE::walkaroundLoneVia( LINE& aCurrent, LINE& aObstacle, LINE& aShoved )
{
    int clearance = getClearance( &aCurrent, &aObstacle );
    const SHAPE_LINE_CHAIN hull = aCurrent.Via().Hull( clearance, aObstacle.Width() );
    SHAPE_LINE_CHAIN path_cw;
    SHAPE_LINE_CHAIN path_ccw;

    if( ! aObstacle.Walkaround( hull, path_cw, true ) )
        return SH_INCOMPLETE;

    if( ! aObstacle.Walkaround( hull, path_ccw, false ) )
        return SH_INCOMPLETE;

    const SHAPE_LINE_CHAIN& shortest = path_ccw.Length() < path_cw.Length() ? path_ccw : path_cw;

    if( shortest.PointCount() < 2 )
        return SH_INCOMPLETE;

    if( aObstacle.CPoint( -1 ) != shortest.CPoint( -1 ) )
        return SH_INCOMPLETE;

    if( aObstacle.CPoint( 0 ) != shortest.CPoint( 0 ) )
        return SH_INCOMPLETE;

    aShoved.SetShape( shortest );

    if( m_currentNode->CheckColliding( &aShoved, &aCurrent ) )
        return SH_INCOMPLETE;

    return SH_OK;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::processHullSet( LINE& aCurrent, LINE& aObstacle,
                                                   LINE& aShoved, const HULL_SET& aHulls )
{
    const SHAPE_LINE_CHAIN& obs = aObstacle.CLine();

    int attempt;

    for( attempt = 0; attempt < 4; attempt++ )
    {
        bool invertTraversal = ( attempt >= 2 );
        bool clockwise = attempt % 2;
        int vFirst = -1, vLast = -1;

        SHAPE_LINE_CHAIN path;
        LINE l( aObstacle );

        for( int i = 0; i < (int) aHulls.size(); i++ )
        {
            const SHAPE_LINE_CHAIN& hull = aHulls[invertTraversal ? aHulls.size() - 1 - i : i];

            if( ! l.Walkaround( hull, path, clockwise ) )
                return SH_INCOMPLETE;

            path.Simplify();
            l.SetShape( path );
        }

        for( int i = 0; i < std::min( path.PointCount(), obs.PointCount() ); i++ )
        {
            if( path.CPoint( i ) != obs.CPoint( i ) )
            {
                vFirst = i;
                break;
            }
        }

        int k = obs.PointCount() - 1;
        for( int i = path.PointCount() - 1; i >= 0 && k >= 0; i--, k-- )
        {
            if( path.CPoint( i ) != obs.CPoint( k ) )
            {
                vLast = i;
                break;
            }
        }

        if( ( vFirst < 0 || vLast < 0 ) && !path.CompareGeometry( aObstacle.CLine() ) )
        {
            wxLogTrace( "PNS", "attempt %d fail vfirst-last", attempt );
            continue;
        }

        if( path.CPoint( -1 ) != obs.CPoint( -1 ) || path.CPoint( 0 ) != obs.CPoint( 0 ) )
        {
            wxLogTrace( "PNS", "attempt %d fail vend-start\n", attempt );
            continue;
        }

        if( !checkBumpDirection( aCurrent, l ) )
        {
            wxLogTrace( "PNS", "attempt %d fail direction-check", attempt );
            aShoved.SetShape( l.CLine() );

            continue;
        }

        if( path.SelfIntersecting() )
        {
            wxLogTrace( "PNS", "attempt %d fail self-intersect", attempt );
            continue;
        }

        bool colliding = m_currentNode->CheckColliding( &l, &aCurrent, ITEM::ANY_T, m_forceClearance );

        if( ( aCurrent.Marker() & MK_HEAD ) && !colliding )
        {
            JOINT* jtStart = m_currentNode->FindJoint( aCurrent.CPoint( 0 ), &aCurrent );

            for( ITEM* item : jtStart->LinkList() )
            {
                if( m_currentNode->CheckColliding( item, &l ) )
                    colliding = true;
            }
        }

        if( colliding )
        {
            wxLogTrace( "PNS", "attempt %d fail coll-check", attempt );
            continue;
        }

        aShoved.SetShape( l.CLine() );

        return SH_OK;
    }

    return SH_INCOMPLETE;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::ProcessSingleLine( LINE& aCurrent, LINE& aObstacle, LINE& aShoved )
{
    aShoved.ClearSegmentLinks();

    bool obstacleIsHead = false;

    for( SEGMENT* s : aObstacle.LinkedSegments() )
    {
        if( s->Marker() & MK_HEAD )
        {
            obstacleIsHead = true;
            break;
        }
    }

    SHOVE_STATUS rv;

    bool viaOnEnd = aCurrent.EndsWithVia();

    if( viaOnEnd && ( !aCurrent.LayersOverlap( &aObstacle ) || aCurrent.SegmentCount() == 0 ) )
    {
        rv = walkaroundLoneVia( aCurrent, aObstacle, aShoved );
    }
    else
    {
        int w = aObstacle.Width();
        int n_segs = aCurrent.SegmentCount();

        int clearance = getClearance( &aCurrent, &aObstacle ) + 1;

        HULL_SET hulls;

        hulls.reserve( n_segs + 1 );

        for( int i = 0; i < n_segs; i++ )
        {
            SEGMENT seg( aCurrent, aCurrent.CSegment( i ) );
            SHAPE_LINE_CHAIN hull = seg.Hull( clearance, w );

            hulls.push_back( hull );
        }

        if( viaOnEnd )
            hulls.push_back( aCurrent.Via().Hull( clearance, w ) );

        rv = processHullSet( aCurrent, aObstacle, aShoved, hulls );
    }

    if( obstacleIsHead )
        aShoved.Mark( aShoved.Marker() | MK_HEAD );

    return rv;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::onCollidingSegment( LINE& aCurrent, SEGMENT* aObstacleSeg )
{
    int segIndex;
    LINE obstacleLine = assembleLine( aObstacleSeg, &segIndex );
    LINE shovedLine( obstacleLine );
    SEGMENT tmp( *aObstacleSeg );

    if( obstacleLine.HasLockedSegments() )
        return SH_TRY_WALK;

    SHOVE_STATUS rv = ProcessSingleLine( aCurrent, obstacleLine, shovedLine );

    const double extensionWalkThreshold = 1.0;

    double obsLen = obstacleLine.CLine().Length();
    double shovedLen = shovedLine.CLine().Length();
    double extensionFactor = 0.0;

    if( obsLen != 0.0f )
        extensionFactor = shovedLen / obsLen - 1.0;

    if( extensionFactor > extensionWalkThreshold )
        return SH_TRY_WALK;

    assert( obstacleLine.LayersOverlap( &shovedLine ) );

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-segment", m_iter );
    m_logger.Log( &tmp, 0, "obstacle-segment" );
    m_logger.Log( &aCurrent, 1, "current-line" );
    m_logger.Log( &obstacleLine, 2, "obstacle-line" );
    m_logger.Log( &shovedLine, 3, "shoved-line" );
#endif

    if( rv == SH_OK )
    {
        if( shovedLine.Marker() & MK_HEAD )
        {
            if( m_multiLineMode )
                return SH_INCOMPLETE;

            m_newHead = shovedLine;
        }

        int rank = aCurrent.Rank();
        shovedLine.SetRank( rank - 1 );

        sanityCheck( &obstacleLine, &shovedLine );
        replaceLine( obstacleLine, shovedLine );

        if( !pushLineStack( shovedLine ) )
            rv = SH_INCOMPLETE;
    }

    return rv;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::onCollidingLine( LINE& aCurrent, LINE& aObstacle )
{
    LINE shovedLine( aObstacle );

    SHOVE_STATUS rv = ProcessSingleLine( aCurrent, aObstacle, shovedLine );

    #ifdef DEBUG
        m_logger.NewGroup( "on-colliding-line", m_iter );
        m_logger.Log( &aObstacle, 0, "obstacle-line" );
        m_logger.Log( &aCurrent, 1, "current-line" );
        m_logger.Log( &shovedLine, 3, "shoved-line" );
    #endif

    if( rv == SH_OK )
    {
        if( shovedLine.Marker() & MK_HEAD )
        {
            if( m_multiLineMode )
                return SH_INCOMPLETE;

            m_newHead = shovedLine;
        }

        sanityCheck( &aObstacle, &shovedLine );
        replaceLine( aObstacle, shovedLine );

        int rank = aObstacle.Rank();
        shovedLine.SetRank( rank - 1 );


        if( !pushLineStack( shovedLine ) )
        {
            rv = SH_INCOMPLETE;
        }
    }

    return rv;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::onCollidingSolid( LINE& aCurrent, ITEM* aObstacle )
{
    WALKAROUND walkaround( m_currentNode, Router() );
    LINE walkaroundLine( aCurrent );

    if( aCurrent.EndsWithVia() )
    {
        VIA vh = aCurrent.Via();
        VIA* via = NULL;
        JOINT* jtStart = m_currentNode->FindJoint( vh.Pos(), &aCurrent );

        if( !jtStart )
            return SH_INCOMPLETE;

        for( ITEM* item : jtStart->LinkList() )
        {
            if( item->OfKind( ITEM::VIA_T ) )
            {
                via = (VIA*) item;
                break;
            }
        }

        if( via && m_currentNode->CheckColliding( via, aObstacle ) )
            return onCollidingVia( aObstacle, via );
    }

    TOPOLOGY topo( m_currentNode );

    std::set<ITEM*> cluster = topo.AssembleCluster( aObstacle, aCurrent.Layers().Start() );

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-solid-cluster", m_iter );
    for( ITEM* item : cluster )
    {
        m_logger.Log( item, 0, "cluster-entry" );
    }
#endif

    walkaround.SetSolidsOnly( false );
    walkaround.RestrictToSet( true, cluster );
    walkaround.SetIterationLimit( 16 ); // fixme: make configurable

    int currentRank = aCurrent.Rank();
    int nextRank;

    bool success = false;

    for( int attempt = 0; attempt < 2; attempt++ )
    {
        if( attempt == 1 || Settings().JumpOverObstacles() )
        {

            nextRank = currentRank - 1;
            walkaround.SetSingleDirection( true );
        }
        else
        {
            nextRank = currentRank + 10000;
            walkaround.SetSingleDirection( false );
        }


    	WALKAROUND::WALKAROUND_STATUS status = walkaround.Route( aCurrent, walkaroundLine, false );

        if( status != WALKAROUND::DONE )
            continue;

        walkaroundLine.ClearSegmentLinks();
        walkaroundLine.Unmark();
    	walkaroundLine.Line().Simplify();

    	if( walkaroundLine.HasLoops() )
            continue;

    	if( aCurrent.Marker() & MK_HEAD )
    	{
            walkaroundLine.Mark( MK_HEAD );

            if( m_multiLineMode )
                continue;

            m_newHead = walkaroundLine;
        }

    	sanityCheck( &aCurrent, &walkaroundLine );

        if( !m_lineStack.empty() )
        {
            LINE lastLine = m_lineStack.front();

            if( m_currentNode->CheckColliding( &lastLine, &walkaroundLine ) )
            {
                LINE dummy( lastLine );

                if( ProcessSingleLine( walkaroundLine, lastLine, dummy ) == SH_OK )
                {
                    success = true;
                    break;
                }
            } else {
                success = true;
                break;
            }
        }
    }

    if(!success)
        return SH_INCOMPLETE;

    replaceLine( aCurrent, walkaroundLine );
    walkaroundLine.SetRank( nextRank );

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-solid", m_iter );
    m_logger.Log( aObstacle, 0, "obstacle-solid" );
    m_logger.Log( &aCurrent, 1, "current-line" );
    m_logger.Log( &walkaroundLine, 3, "walk-line" );
#endif

    popLineStack();

    if( !pushLineStack( walkaroundLine ) )
        return SH_INCOMPLETE;

    return SH_OK;
}


/*
 * Pops NODE stackframes which no longer collide with aHeadSet.  Optionally sets aDraggedVia
 * to the dragged via of the last unpopped state.
 */
NODE* SHOVE::reduceSpringback( const ITEM_SET& aHeadSet, VIA_HANDLE& aDraggedVia )
{
    while( !m_nodeStack.empty() )
    {
        SPRINGBACK_TAG& spTag = m_nodeStack.back();

        auto obs = spTag.m_node->CheckColliding( aHeadSet );

        if( !obs )
        {
            aDraggedVia = spTag.m_draggedVia;
            aDraggedVia.valid = true;

            delete spTag.m_node;
            m_nodeStack.pop_back();
        }
        else
           break;
    }

    return m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;
}


/*
 * Push the current NODE on to the stack.  aDraggedVia is the dragged via *before* the push
 * (which will be restored in the event the stackframe is popped).
 */
bool SHOVE::pushSpringback( NODE* aNode, const OPT_BOX2I& aAffectedArea, VIA* aDraggedVia )
{
    SPRINGBACK_TAG st;
    OPT_BOX2I prev_area;

    if( !m_nodeStack.empty() )
        prev_area = m_nodeStack.back().m_affectedArea;

    if( aDraggedVia )
    {
        st.m_draggedVia = aDraggedVia->MakeHandle();
    }

    st.m_node = aNode;

    if( aAffectedArea )
    {
        if( prev_area )
            st.m_affectedArea = prev_area->Merge( *aAffectedArea );
        else
            st.m_affectedArea = aAffectedArea;
    } else
        st.m_affectedArea = prev_area;

    m_nodeStack.push_back( st );

    return true;
}


/*
 * Push or shove a via by at least aForce.  (The via might be pushed or shoved slightly further
 * to keep it from landing on an existing joint.)
 */
SHOVE::SHOVE_STATUS SHOVE::pushOrShoveVia( VIA* aVia, const VECTOR2I& aForce, int aCurrentRank )
{
    LINE_PAIR_VEC draggedLines;
    VECTOR2I p0( aVia->Pos() );
    JOINT* jt = m_currentNode->FindJoint( p0, aVia );
    VECTOR2I p0_pushed( p0 + aForce );

    // nothing to do...
    if ( aForce.x == 0 && aForce.y == 0 )
        return SH_OK;

    if( !jt )
    {
        wxLogTrace( "PNS", "weird, can't find the center-of-via joint\n" );
        return SH_INCOMPLETE;
    }

    if( aVia->IsLocked() )
        return SH_TRY_WALK;

    if( jt->IsLocked() )
        return SH_INCOMPLETE;

    // make sure pushed via does not overlap with any existing joint
    while( true )
    {
        JOINT* jt_next = m_currentNode->FindJoint( p0_pushed, aVia );

        if( !jt_next )
            break;

        p0_pushed += aForce.Resize( 2 );
    }

    std::unique_ptr<VIA> pushedVia = Clone( *aVia );
    pushedVia->SetPos( p0_pushed );
    pushedVia->Mark( aVia->Marker() );

    for( ITEM* item : jt->LinkList() )
    {
        if( SEGMENT* seg = dyn_cast<SEGMENT*>( item ) )
        {
            LINE_PAIR lp;
            int segIndex;

            lp.first = assembleLine( seg, &segIndex );

            if( lp.first.HasLockedSegments() )
                return SH_TRY_WALK;

            assert( segIndex == 0 || ( segIndex == ( lp.first.SegmentCount() - 1 ) ) );

            if( segIndex == 0 )
                lp.first.Reverse();

            lp.second = lp.first;
            lp.second.ClearSegmentLinks();
            lp.second.DragCorner( p0_pushed, lp.second.CLine().Find( p0 ) );
            lp.second.AppendVia( *pushedVia );
            draggedLines.push_back( lp );
        }
    }

#ifdef DEBUG
    m_logger.Log( aVia, 0, "obstacle-via" );
#endif

    pushedVia->SetRank( aCurrentRank - 1 );

#ifdef DEBUG
    m_logger.Log( pushedVia.get(), 1, "pushed-via" );
#endif

    if( aVia->Marker() & MK_HEAD )      // push
    {
        m_draggedVia = pushedVia.get();
    }
    else
    {                                   // shove
        if( jt->IsStitchingVia() )
            pushLineStack( LINE( *pushedVia ) );
    }

    replaceItems( aVia, std::move( pushedVia ) );

    for( LINE_PAIR lp : draggedLines )
    {
        if( lp.first.Marker() & MK_HEAD )
        {
            lp.second.Mark( MK_HEAD );

            if( m_multiLineMode )
                return SH_INCOMPLETE;

            m_newHead = lp.second;
        }

        unwindLineStack( &lp.first );

        if( lp.second.SegmentCount() )
        {
            replaceLine( lp.first, lp.second );
            lp.second.SetRank( aCurrentRank - 1 );

            if( !pushLineStack( lp.second, true ) )
                return SH_INCOMPLETE;
        }
        else
        {
            m_currentNode->Remove( lp.first );
        }

#ifdef DEBUG
        m_logger.Log( &lp.first, 2, "fan-pre" );
        m_logger.Log( &lp.second, 3, "fan-post" );
#endif
    }

    return SH_OK;
}


/*
 * Calculate the minimum translation vector required to resolve a collision with a via and
 * shove the via by that distance.
 */
SHOVE::SHOVE_STATUS SHOVE::onCollidingVia( ITEM* aCurrent, VIA* aObstacleVia )
{
    RULE_RESOLVER* rr = m_currentNode->GetRuleResolver();
    int clearance = getClearance( aCurrent, aObstacleVia ) ;
    LINE_PAIR_VEC draggedLines;
    bool lineCollision = false;
    bool viaCollision = false;
    bool holeCollision = false;
    LINE* currentLine = NULL;
    VECTOR2I mtvLine;       // Minimum translation vector to correct line collisions
    VECTOR2I mtvVia;        // MTV to correct via collisions
    VECTOR2I mtvHoles;      // MTV to correct hole collisions
    VECTOR2I mtvSolid;      // MTV to correct solid collisions
    VECTOR2I mtv;           // Union of relevant MTVs (will correct all collisions)
    int rank = -1;

    if( aCurrent->OfKind( ITEM::LINE_T ) )
    {
#ifdef DEBUG
         m_logger.NewGroup( "push-via-by-line", m_iter );
         m_logger.Log( aCurrent, 4, "current" );
#endif

        currentLine = (LINE*) aCurrent;
        lineCollision = CollideShapes( aObstacleVia->Shape(), currentLine->Shape(),
                                       clearance + currentLine->Width() / 2 + PNS_HULL_MARGIN,
                                       true, mtvLine );

        if( currentLine->EndsWithVia() )
        {
            int currentNet = currentLine->Net();
            int obstacleNet = aObstacleVia->Net();

            if( currentNet != obstacleNet && currentNet >= 0 && obstacleNet >= 0 )
            {
                viaCollision = CollideShapes( currentLine->Via().Shape(), aObstacleVia->Shape(),
                                              clearance + PNS_HULL_MARGIN, true, mtvVia );
            }

            // hole-to-hole is a mechanical constraint (broken drill bits), not an electrical
            // one, so it has to be checked irrespective of matching nets.

            // temporarily removed hole-to-hole collision check due to conflicts with the springback algorithm...
            // we need to figure out a better solution here - TW
            holeCollision = false; //rr->CollideHoles( &currentLine->Via(), aObstacleVia, true, &mtvHoles );
        }

        // These aren't /actually/ lengths as we don't bother to do the square-root part,
        // but we're just comparing them to each other so it's faster this way.
        ecoord lineMTVLength = lineCollision ? mtvLine.SquaredEuclideanNorm() : 0;
        ecoord viaMTVLength = viaCollision ? mtvVia.SquaredEuclideanNorm() : 0;
        ecoord holeMTVLength = holeCollision ? mtvHoles.SquaredEuclideanNorm() : 0;

        if( lineMTVLength >= viaMTVLength && lineMTVLength >= holeMTVLength )
            mtv = mtvLine;
        else if( viaMTVLength >= lineMTVLength && viaMTVLength >= holeMTVLength )
            mtv = mtvVia;
        else
            mtv = mtvHoles;

        rank = currentLine->Rank();
    }
    else if( aCurrent->OfKind( ITEM::SOLID_T ) )
    {
        CollideShapes( aObstacleVia->Shape(), aCurrent->Shape(),
                       clearance + PNS_HULL_MARGIN, true, mtvSolid );
        mtv = -mtvSolid;
        rank = aCurrent->Rank() + 10000;
    }

    return pushOrShoveVia( aObstacleVia, mtv, rank );
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::onReverseCollidingVia( LINE& aCurrent, VIA* aObstacleVia )
{
    int n = 0;
    LINE cur( aCurrent );
    cur.ClearSegmentLinks();

    JOINT* jt = m_currentNode->FindJoint( aObstacleVia->Pos(), aObstacleVia );
    LINE shoved( aCurrent );
    shoved.ClearSegmentLinks();

    cur.RemoveVia();
    unwindLineStack( &aCurrent );

    for( ITEM* item : jt->LinkList() )
    {
        if( item->OfKind( ITEM::SEGMENT_T ) && item->LayersOverlap( &aCurrent ) )
        {
            SEGMENT* seg = (SEGMENT*) item;
            LINE head = assembleLine( seg );

            head.AppendVia( *aObstacleVia );

            SHOVE_STATUS st = ProcessSingleLine( head, cur, shoved );

            if( st != SH_OK )
            {
#ifdef DEBUG
                m_logger.NewGroup( "on-reverse-via-fail-shove", m_iter );
                m_logger.Log( aObstacleVia, 0, "the-via" );
                m_logger.Log( &aCurrent, 1, "current-line" );
                m_logger.Log( &shoved, 3, "shoved-line" );
#endif

                return st;
            }

            cur.SetShape( shoved.CLine() );
            n++;
        }
    }

    if( !n )
    {
#ifdef DEBUG
        m_logger.NewGroup( "on-reverse-via-fail-lonevia", m_iter );
        m_logger.Log( aObstacleVia, 0, "the-via" );
        m_logger.Log( &aCurrent, 1, "current-line" );
#endif

        LINE head( aCurrent );
        head.Line().Clear();
        head.AppendVia( *aObstacleVia );
        head.ClearSegmentLinks();

        SHOVE_STATUS st = ProcessSingleLine( head, aCurrent, shoved );

        if( st != SH_OK )
            return st;

        cur.SetShape( shoved.CLine() );
    }

    if( aCurrent.EndsWithVia() )
        shoved.AppendVia( aCurrent.Via() );

#ifdef DEBUG
    m_logger.NewGroup( "on-reverse-via", m_iter );
    m_logger.Log( aObstacleVia, 0, "the-via" );
    m_logger.Log( &aCurrent, 1, "current-line" );
    m_logger.Log( &shoved, 3, "shoved-line" );
#endif
    int currentRank = aCurrent.Rank();
    replaceLine( aCurrent, shoved );

    if( !pushLineStack( shoved ) )
        return SH_INCOMPLETE;

    shoved.SetRank( currentRank );

    return SH_OK;
}


void SHOVE::unwindLineStack( SEGMENT* aSeg )
{
    for( std::vector<LINE>::iterator i = m_lineStack.begin(); i != m_lineStack.end() ; )
    {
        if( i->ContainsSegment( aSeg ) )
            i = m_lineStack.erase( i );
        else
            i++;
    }

    for( std::vector<LINE>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end() ; )
    {
        if( i->ContainsSegment( aSeg ) )
            i = m_optimizerQueue.erase( i );
        else
            i++;
    }
}


void SHOVE::unwindLineStack( ITEM* aItem )
{
    if( aItem->OfKind( ITEM::SEGMENT_T ) )
        unwindLineStack( static_cast<SEGMENT*>( aItem ));
    else if( aItem->OfKind( ITEM::LINE_T ) )
    {
        LINE* l = static_cast<LINE*>( aItem );

        for( SEGMENT* seg : l->LinkedSegments() )
            unwindLineStack( seg );
    }
}


bool SHOVE::pushLineStack( const LINE& aL, bool aKeepCurrentOnTop )
{
    if( !aL.IsLinkedChecked() && aL.SegmentCount() != 0 )
        return false;

    if( aKeepCurrentOnTop && m_lineStack.size() > 0)
    {
        m_lineStack.insert( m_lineStack.begin() + m_lineStack.size() - 1, aL );
    }
    else
    {
        m_lineStack.push_back( aL );
    }

    m_optimizerQueue.push_back( aL );

    return true;
}

void SHOVE::popLineStack( )
{
    LINE& l = m_lineStack.back();

    for( std::vector<LINE>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end(); )
    {
        bool found = false;

        for( SEGMENT *s : l.LinkedSegments() )
        {
            if( i->ContainsSegment( s ) )
            {
                i = m_optimizerQueue.erase( i );
                found = true;
                break;
            }
        }

        if( !found )
            i++;
    }

    m_lineStack.pop_back();
}


/*
 * Resolve the next collision.
 */
SHOVE::SHOVE_STATUS SHOVE::shoveIteration( int aIter )
{
    LINE currentLine = m_lineStack.back();
    NODE::OPT_OBSTACLE nearest;
    SHOVE_STATUS st = SH_NULL;

    for( ITEM::PnsKind search_order : { ITEM::SOLID_T, ITEM::VIA_T, ITEM::SEGMENT_T } )
    {
         nearest = m_currentNode->NearestObstacle( &currentLine, search_order );

         if( nearest )
            break;
    }

    if( !nearest )
    {
        m_lineStack.pop_back();
        return SH_OK;
    }

    ITEM* ni = nearest->m_item;

    unwindLineStack( ni );

    if( !ni->OfKind( ITEM::SOLID_T ) && ni->Rank() >= 0 && ni->Rank() > currentLine.Rank() )
    {
        // Collision with a higher-ranking object (ie: one that we've already shoved)
        //
        switch( ni->Kind() )
        {
        case ITEM::VIA_T:
        {
            wxLogTrace( "PNS", "iter %d: reverse-collide-via", aIter );

            if( currentLine.EndsWithVia()
                    && m_currentNode->CheckColliding( &currentLine.Via(), (VIA*) ni ) )
            {
                st = SH_INCOMPLETE;
            }
            else
            {
                st = onReverseCollidingVia( currentLine, (VIA*) ni );
            }

            break;
        }

        case ITEM::SEGMENT_T:
        {
            wxLogTrace( "PNS", "iter %d: reverse-collide-segment ", aIter );
            LINE revLine = assembleLine( (SEGMENT*) ni );

            popLineStack();
            st = onCollidingLine( revLine, currentLine );
            if( !pushLineStack( revLine ) )
                return SH_INCOMPLETE;

            break;
        }

        default:
            assert( false );
        }
    }
    else
    {
        // Collision with a lower-ranking object or a solid
        //
        switch( ni->Kind() )
        {
        case ITEM::SEGMENT_T:
            wxLogTrace( "PNS", "iter %d: collide-segment ", aIter );

            st = onCollidingSegment( currentLine, (SEGMENT*) ni );

            if( st == SH_TRY_WALK )
                st = onCollidingSolid( currentLine, ni );

            break;

        case ITEM::VIA_T:
            wxLogTrace( "PNS", "iter %d: shove-via ", aIter );
            st = onCollidingVia( &currentLine, (VIA*) ni );

            if( st == SH_TRY_WALK )
                st = onCollidingSolid( currentLine, ni );

            break;

        case ITEM::SOLID_T:
            wxLogTrace( "PNS", "iter %d: walk-solid ", aIter );
            st = onCollidingSolid( currentLine, (SOLID*) ni );
            break;

        default:
            break;
        }
    }

    return st;
}


/*
 * Resolve collisions.
 * Each iteration pushes the next colliding object out of the way.  Iterations are continued as
 * long as they propagate further collisions, or until the iteration timeout or max iteration
 * count is reached.
 */
SHOVE::SHOVE_STATUS SHOVE::shoveMainLoop()
{
    SHOVE_STATUS st = SH_OK;

    m_affectedArea = OPT_BOX2I();

    wxLogTrace( "PNS", "ShoveStart [root: %d jts, current: %d jts]", m_root->JointCount(),
           m_currentNode->JointCount() );

    int iterLimit = Settings().ShoveIterationLimit();
    TIME_LIMIT timeLimit = Settings().ShoveTimeLimit();

    m_iter = 0;

    timeLimit.Restart();

    if( m_lineStack.empty() && m_draggedVia )
    {
        // If we're shoving a free via then push a proxy LINE (with the via on the end) onto
        // the stack.
        pushLineStack( LINE( *m_draggedVia ));
    }

    while( !m_lineStack.empty() )
    {
        st = shoveIteration( m_iter );

        m_iter++;

        if( st == SH_INCOMPLETE || timeLimit.Expired() || m_iter >= iterLimit )
        {
            st = SH_INCOMPLETE;
            break;
        }
    }

    return st;
}


OPT_BOX2I SHOVE::totalAffectedArea() const
{
    OPT_BOX2I area;

    if( !m_nodeStack.empty() )
        area = m_nodeStack.back().m_affectedArea;

    if( area  && m_affectedArea)
        area->Merge( *m_affectedArea );
    else if( !area )
        area = m_affectedArea;

    return area;
}


SHOVE::SHOVE_STATUS SHOVE::ShoveLines( const LINE& aCurrentHead )
{
    SHOVE_STATUS st = SH_OK;

    m_multiLineMode = false;

    // empty head? nothing to shove...

    if( !aCurrentHead.SegmentCount() && !aCurrentHead.EndsWithVia() )
        return SH_INCOMPLETE;

    LINE head( aCurrentHead );
    head.ClearSegmentLinks();

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_newHead = OPT_LINE();
    m_logger.Clear();

    // Pop NODEs containing previous shoves which are no longer necessary
    //
    ITEM_SET headSet;
    headSet.Add( aCurrentHead );

    VIA_HANDLE dummyVia;

    NODE* parent = reduceSpringback( headSet, dummyVia );

    // Create a new NODE to store this version of the world
    //
    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();
    m_currentNode->Add( head );

    m_currentNode->LockJoint( head.CPoint(0), &head, true );

    if( !head.EndsWithVia() )
        m_currentNode->LockJoint( head.CPoint( -1 ), &head, true );

    head.Mark( MK_HEAD );
    head.SetRank( 100000 );

    m_logger.NewGroup( "initial", 0 );
    m_logger.Log( &head, 0, "head" );

    if( head.EndsWithVia() )
    {
        std::unique_ptr< VIA >headVia = Clone( head.Via() );
        headVia->Mark( MK_HEAD );
        headVia->SetRank( 100000 );
        m_logger.Log( headVia.get(), 0, "head-via" );
        m_currentNode->Add( std::move( headVia ) );
    }

    if( !pushLineStack( head ) )
    {
        delete m_currentNode;
        m_currentNode = parent;

        return SH_INCOMPLETE;
    }

    st = shoveMainLoop();

    if( st == SH_OK )
    {
        runOptimizer( m_currentNode );

        if( m_newHead )
            st = m_currentNode->CheckColliding( &( *m_newHead ) ) ? SH_INCOMPLETE : SH_HEAD_MODIFIED;
        else
            st = m_currentNode->CheckColliding( &head ) ? SH_INCOMPLETE : SH_OK;
    }

    m_currentNode->RemoveByMarker( MK_HEAD );

    wxLogTrace( "PNS", "Shove status : %s after %d iterations",
           ( ( st == SH_OK || st == SH_HEAD_MODIFIED ) ? "OK" : "FAILURE"), m_iter );

    if( st == SH_OK || st == SH_HEAD_MODIFIED )
    {
        pushSpringback( m_currentNode, m_affectedArea, nullptr );
    }
    else
    {
        delete m_currentNode;

        m_currentNode = parent;
        m_newHead = OPT_LINE();
    }

    if(m_newHead)
        m_newHead->Unmark();

    if( m_newHead && head.EndsWithVia() )
    {
        VIA v = head.Via();
        v.SetPos( m_newHead->CPoint( -1 ) );
        m_newHead->AppendVia(v);
    }

    return st;
}


SHOVE::SHOVE_STATUS SHOVE::ShoveMultiLines( const ITEM_SET& aHeadSet )
{
    SHOVE_STATUS st = SH_OK;

    m_multiLineMode = true;

    ITEM_SET headSet;

    for( const ITEM* item : aHeadSet.CItems() )
    {
        const LINE* headOrig = static_cast<const LINE*>( item );

        // empty head? nothing to shove...
        if( !headOrig->SegmentCount() )
            return SH_INCOMPLETE;

        headSet.Add( *headOrig );
    }

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_logger.Clear();

    VIA_HANDLE dummyVia;

    NODE* parent = reduceSpringback( headSet, dummyVia );

    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();
    int n = 0;

    for( const ITEM* item : aHeadSet.CItems() )
    {
        const LINE* headOrig = static_cast<const LINE*>( item );
        LINE head( *headOrig );
        head.ClearSegmentLinks();

        m_currentNode->Add( head );

        head.Mark( MK_HEAD );
        head.SetRank( 100000 );
        n++;

        if( !pushLineStack( head ) )
            return SH_INCOMPLETE;

        if( head.EndsWithVia() )
        {
            std::unique_ptr< VIA > headVia = Clone( head.Via() );
            headVia->Mark( MK_HEAD );
            headVia->SetRank( 100000 );
            m_logger.Log( headVia.get(), 0, "head-via" );
            m_currentNode->Add( std::move( headVia ) );
        }
    }

    m_logger.NewGroup( "initial", 0 );
    //m_logger.Log( head, 0, "head" );

    st = shoveMainLoop();

    if( st == SH_OK )
        runOptimizer( m_currentNode );

    m_currentNode->RemoveByMarker( MK_HEAD );

    wxLogTrace( "PNS", "Shove status : %s after %d iterations",
           ( st == SH_OK ? "OK" : "FAILURE"), m_iter );

    if( st == SH_OK )
    {
        pushSpringback( m_currentNode, m_affectedArea, nullptr );
    }
    else
    {
        delete m_currentNode;
        m_currentNode = parent;
    }

    return st;
}

static VIA* findViaByHandle ( NODE *aNode, const VIA_HANDLE& handle )
{
    JOINT* jt = aNode->FindJoint( handle.pos, handle.layers.Start(), handle.net );

    if( !jt )
        return nullptr;

    for( ITEM* item : jt->LinkList() )
    {
        if ( item->OfKind( ITEM::VIA_T )) 
        {
            if( item->Net() == handle.net && item->Layers().Overlaps(handle.layers) )
                return static_cast<VIA*>( item );
        }
    }

    return nullptr;
}

SHOVE::SHOVE_STATUS SHOVE::ShoveDraggingVia( const VIA_HANDLE aOldVia, const VECTOR2I& aWhere, VIA_HANDLE& aNewVia )
{
     SHOVE_STATUS st = SH_OK;

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_newHead = OPT_LINE();
    m_draggedVia = NULL;

    auto viaToDrag = findViaByHandle( m_currentNode, aOldVia );

    if( !viaToDrag )
    {
        return SH_INCOMPLETE;
    }

    // Pop NODEs containing previous shoves which are no longer necessary
    ITEM_SET headSet;
    
    VIA headVia ( *viaToDrag );
    headVia.SetPos( aWhere );
    headSet.Add( headVia );
    VIA_HANDLE prevViaHandle;
    NODE* parent = reduceSpringback( headSet, prevViaHandle );

    if( prevViaHandle.valid )
    {
        aNewVia = prevViaHandle;
        viaToDrag = findViaByHandle( parent, prevViaHandle );
    }

    // Create a new NODE to store this version of the world
    //
    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();

    viaToDrag->Mark( MK_HEAD );
    viaToDrag->SetRank( 100000 );

    // Push the via to its new location
    //
    st = pushOrShoveVia( viaToDrag, ( aWhere - viaToDrag->Pos()), 0 );

    // Shove any colliding objects out of the way
    //
    if( st == SH_OK )
        st = shoveMainLoop();

    if( st == SH_OK )
        runOptimizer( m_currentNode );

    if( st == SH_OK || st == SH_HEAD_MODIFIED )
    {
        wxLogTrace( "PNS","setNewV %p", m_draggedVia );

        if (!m_draggedVia)
            m_draggedVia = viaToDrag;

        aNewVia = m_draggedVia->MakeHandle();

        pushSpringback( m_currentNode, m_affectedArea, viaToDrag );
    }
    else
    {
        delete m_currentNode;
        m_currentNode = parent;
    }

    return st;
}


void SHOVE::runOptimizer( NODE* aNode )
{
    OPTIMIZER optimizer( aNode );
    int optFlags = 0;
    int n_passes = 0;

    PNS_OPTIMIZATION_EFFORT effort = Settings().OptimizerEffort();

    OPT_BOX2I area = totalAffectedArea();

    int maxWidth = 0;

    for( LINE& line : m_optimizerQueue )
        maxWidth = std::max( line.Width(), maxWidth );

    if( area )
        area->Inflate( 10 * maxWidth );

    switch( effort )
    {
    case OE_LOW:
        optFlags = OPTIMIZER::MERGE_OBTUSE;
        n_passes = 1;
        break;

    case OE_MEDIUM:
        optFlags = OPTIMIZER::MERGE_SEGMENTS;

        if( area )
            optimizer.SetRestrictArea( *area );

        n_passes = 2;
        break;

    case OE_FULL:
        optFlags = OPTIMIZER::MERGE_SEGMENTS;
        n_passes = 2;
        break;

    default:
        break;
    }

    if( Settings().SmartPads() )
        optFlags |= OPTIMIZER::SMART_PADS;

    optimizer.SetEffortLevel( optFlags );
    optimizer.SetCollisionMask( ITEM::ANY_T );

    for( int pass = 0; pass < n_passes; pass++ )
    {
        std::reverse( m_optimizerQueue.begin(), m_optimizerQueue.end() );

        for( LINE& line : m_optimizerQueue)
        {
            if( !( line.Marker() & MK_HEAD ) )
            {
                LINE optimized;

                if( optimizer.Optimize( &line, &optimized ) )
                {
                    aNode->Remove( line );
                    line.SetShape( optimized.CLine() );
                    aNode->Add( line );
                }
            }
        }
    }
}


NODE* SHOVE::CurrentNode()
{
    return m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;
}


const LINE SHOVE::NewHead() const
{
    assert( m_newHead );

    return *m_newHead;
}


void SHOVE::SetInitialLine( LINE& aInitial )
{
    m_root = m_root->Branch();
    m_root->Remove( aInitial );
}

}
