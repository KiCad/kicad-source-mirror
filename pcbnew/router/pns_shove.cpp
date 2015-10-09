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

#define PNS_DEBUG

#include <deque>
#include <cassert>

#include <boost/foreach.hpp>

#include "trace.h"
#include "range.h"

#include "pns_line.h"
#include "pns_node.h"
#include "pns_walkaround.h"
#include "pns_shove.h"
#include "pns_solid.h"
#include "pns_optimizer.h"
#include "pns_via.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_shove.h"
#include "pns_utils.h"
#include "pns_topology.h"

#include "time_limit.h"

#include <profile.h>

void PNS_SHOVE::replaceItems( PNS_ITEM* aOld, PNS_ITEM* aNew )
{
    OPT_BOX2I changed_area = ChangedArea( aOld, aNew );

    if( changed_area )
    {
        m_affectedAreaSum = m_affectedAreaSum ? m_affectedAreaSum->Merge ( *changed_area ) : *changed_area;
    }

    m_currentNode->Replace( aOld, aNew );
}


int PNS_SHOVE::getClearance( const PNS_ITEM* aA, const PNS_ITEM* aB ) const
{
    if( m_forceClearance >= 0 )
        return m_forceClearance;

    return m_currentNode->GetClearance( aA, aB );
}


void PNS_SHOVE::sanityCheck( PNS_LINE* aOld, PNS_LINE* aNew )
{
    assert( aOld->CPoint( 0 ) == aNew->CPoint( 0 ) );
    assert( aOld->CPoint( -1 ) == aNew->CPoint( -1 ) );
}


PNS_SHOVE::PNS_SHOVE( PNS_NODE* aWorld, PNS_ROUTER* aRouter ) :
    PNS_ALGO_BASE ( aRouter )
{
    m_forceClearance = -1;
    m_root = aWorld;
    m_currentNode = aWorld;

    // Initialize other temporary variables:
    m_draggedVia = NULL;
    m_iter = 0;
    m_multiLineMode = false;
}


PNS_SHOVE::~PNS_SHOVE()
{
}


PNS_LINE PNS_SHOVE::assembleLine( const PNS_SEGMENT* aSeg, int* aIndex )
{
    return m_currentNode->AssembleLine( const_cast<PNS_SEGMENT*>( aSeg ), aIndex, true );
}

// A dumb function that checks if the shoved line is shoved the right way, e.g.
// visually "outwards" of the line/via applying pressure on it. Unfortunately there's no
// mathematical concept of orientation of an open curve, so we use some primitive heuristics:
// if the shoved line wraps around the start of the "pusher", it's likely shoved in wrong direction.
bool PNS_SHOVE::checkBumpDirection( const PNS_LINE& aCurrent, const PNS_LINE& aShoved ) const
{
    const SEG& ss = aCurrent.CSegment( 0 );

    int dist = getClearance( &aCurrent, &aShoved ) + PNS_HULL_MARGIN;

    dist += aCurrent.Width() / 2;
    dist += aShoved.Width() / 2;

    const VECTOR2I ps = ss.A - ( ss.B - ss.A ).Resize( dist );

    return !aShoved.CLine().PointOnEdge( ps );
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::walkaroundLoneVia( PNS_LINE& aCurrent, PNS_LINE& aObstacle,
                                                      PNS_LINE& aShoved )
{
    int clearance = getClearance( &aCurrent, &aObstacle );
    const SHAPE_LINE_CHAIN hull = aCurrent.Via().Hull( clearance, aObstacle.Width() );
    SHAPE_LINE_CHAIN path_cw, path_ccw;

    aObstacle.Walkaround( hull, path_cw, true );
    aObstacle.Walkaround( hull, path_ccw, false );

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


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::processHullSet( PNS_LINE& aCurrent, PNS_LINE& aObstacle,
                                                   PNS_LINE& aShoved, const HULL_SET& aHulls )
{
    const SHAPE_LINE_CHAIN& obs = aObstacle.CLine();

    int attempt;

    for( attempt = 0; attempt < 4; attempt++ )
    {
        bool invertTraversal = ( attempt >= 2 );
        bool clockwise = attempt % 2;
        int vFirst = -1, vLast = -1;

        SHAPE_LINE_CHAIN path;
        PNS_LINE l( aObstacle );

        for( int i = 0; i < (int) aHulls.size(); i++ )
        {
            const SHAPE_LINE_CHAIN& hull = aHulls[invertTraversal ? aHulls.size() - 1 - i : i];

            l.Walkaround( hull, path, clockwise );
            path.Simplify();
            l.SetShape( path );
        }

        for( int i = 0; i < std::min ( path.PointCount(), obs.PointCount() ); i++ )
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
            TRACE( 100, "attempt %d fail vfirst-last", attempt );
            continue;
        }

        if( path.CPoint( -1 ) != obs.CPoint( -1 ) || path.CPoint( 0 ) != obs.CPoint( 0 ) )
        {
            TRACE( 100, "attempt %d fail vend-start\n", attempt );
            continue;
        }

        if( !checkBumpDirection( aCurrent, l ) )
        {
            TRACE( 100, "attempt %d fail direction-check", attempt );
            aShoved.SetShape( l.CLine() );

            continue;
        }

        if( path.SelfIntersecting() )
        {
            TRACE( 100, "attempt %d fail self-intersect", attempt );
            continue;
        }

        bool colliding = m_currentNode->CheckColliding( &l, &aCurrent, PNS_ITEM::ANY, m_forceClearance );

        if( ( aCurrent.Marker() & MK_HEAD ) && !colliding )
        {
            PNS_JOINT* jtStart = m_currentNode->FindJoint( aCurrent.CPoint( 0 ), &aCurrent );

            BOOST_FOREACH( PNS_ITEM* item, jtStart->LinkList() )
            {
                if( m_currentNode->CheckColliding( item, &l ) )
                    colliding = true;
            }
        }

        if( colliding )
        {
            TRACE( 100, "attempt %d fail coll-check", attempt );
            continue;
        }

        aShoved.SetShape( l.CLine() );

        return SH_OK;
    }

    return SH_INCOMPLETE;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::ProcessSingleLine( PNS_LINE& aCurrent, PNS_LINE& aObstacle,
                                                      PNS_LINE& aShoved )
{
    aShoved.ClearSegmentLinks();

    bool obstacleIsHead = false;

    if( aObstacle.LinkedSegments() )
    {
        BOOST_FOREACH( PNS_SEGMENT* s, *aObstacle.LinkedSegments() )

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
            PNS_SEGMENT seg( aCurrent, aCurrent.CSegment( i ) );
            SHAPE_LINE_CHAIN hull = seg.Hull( clearance, w );

            hulls.push_back( hull );
        }

        if( viaOnEnd )
            hulls.push_back ( aCurrent.Via().Hull( clearance, w ) );

        rv = processHullSet ( aCurrent, aObstacle, aShoved, hulls );
    }

    if( obstacleIsHead )
        aShoved.Mark( aShoved.Marker() | MK_HEAD );

    return rv;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onCollidingSegment( PNS_LINE& aCurrent, PNS_SEGMENT* aObstacleSeg )
{
    int segIndex;
    PNS_LINE obstacleLine = assembleLine( aObstacleSeg, &segIndex );
    PNS_LINE shovedLine( obstacleLine );
    PNS_SEGMENT tmp( *aObstacleSeg );

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
        replaceItems( &obstacleLine, &shovedLine );

        if( !pushLine( shovedLine ) )
            rv = SH_INCOMPLETE;
    }

    return rv;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onCollidingLine( PNS_LINE& aCurrent, PNS_LINE& aObstacle )
{
    PNS_LINE shovedLine( aObstacle );

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
        replaceItems( &aObstacle, &shovedLine );

        int rank = aObstacle.Rank();
        shovedLine.SetRank( rank - 1 );


        if( !pushLine( shovedLine ) )
        {
            rv = SH_INCOMPLETE;
        }
    }

    return rv;
}

PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onCollidingSolid( PNS_LINE& aCurrent, PNS_ITEM* aObstacle )
{
    PNS_WALKAROUND walkaround( m_currentNode, Router() );
    PNS_LINE walkaroundLine( aCurrent );

    if( aCurrent.EndsWithVia() )
    {
        PNS_VIA vh = aCurrent.Via();
        PNS_VIA* via = NULL;
        PNS_JOINT* jtStart = m_currentNode->FindJoint( vh.Pos(), &aCurrent );

        if( !jtStart )
            return SH_INCOMPLETE;

        BOOST_FOREACH( PNS_ITEM* item, jtStart->LinkList() )
        {
            if( item->OfKind( PNS_ITEM::VIA ) )
            {
                via = (PNS_VIA*) item;
                break;
            }
        }

        if( via && m_currentNode->CheckColliding( via, aObstacle ) )
            return onCollidingVia( aObstacle, via );
    }

    PNS_TOPOLOGY topo( m_currentNode );

    std::set<PNS_ITEM*> cluster = topo.AssembleCluster( aObstacle, aCurrent.Layers().Start() );

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-solid-cluster", m_iter );
    BOOST_FOREACH( PNS_ITEM* item, cluster )
    {
        m_logger.Log( item, 0, "cluster-entry" );
    }
#endif

    walkaround.SetSolidsOnly( false );
    walkaround.RestrictToSet( true, cluster );
    walkaround.SetIterationLimit( 16 ); // fixme: make configurable

    int currentRank = aCurrent.Rank();
    int nextRank;

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


    	if( walkaround.Route( aCurrent, walkaroundLine, false ) != PNS_WALKAROUND::DONE )
            return SH_INCOMPLETE;

        walkaroundLine.ClearSegmentLinks();
        walkaroundLine.Unmark();
    	walkaroundLine.Line().Simplify();

    	if( walkaroundLine.HasLoops() )
            return SH_INCOMPLETE;

    	if( aCurrent.Marker() & MK_HEAD )
    	{
            walkaroundLine.Mark( MK_HEAD );

            if( m_multiLineMode )
                return SH_INCOMPLETE;

            m_newHead = walkaroundLine;
        }

    	sanityCheck( &aCurrent, &walkaroundLine );

        if( !m_lineStack.empty() )
        {
            PNS_LINE lastLine = m_lineStack.front();

            if( m_currentNode->CheckColliding( &lastLine, &walkaroundLine ) )
            {
                PNS_LINE dummy ( lastLine );

                if( ProcessSingleLine( walkaroundLine, lastLine, dummy ) == SH_OK )
                    break;
            } else
                break;
        }
    }

    replaceItems( &aCurrent, &walkaroundLine );
    walkaroundLine.SetRank( nextRank );

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-solid", m_iter );
    m_logger.Log( aObstacle, 0, "obstacle-solid" );
    m_logger.Log( &aCurrent, 1, "current-line" );
    m_logger.Log( &walkaroundLine, 3, "walk-line" );
#endif

    popLine();

    if( !pushLine( walkaroundLine ) )
        return SH_INCOMPLETE;

    return SH_OK;
}


bool PNS_SHOVE::reduceSpringback( const PNS_ITEMSET& aHeadSet )
{
    bool rv = false;

    while( !m_nodeStack.empty() )
    {
        SPRINGBACK_TAG spTag = m_nodeStack.back();

        if( !spTag.m_node->CheckColliding( aHeadSet ) )
        {
            rv = true;

            delete spTag.m_node;
            m_nodeStack.pop_back();
        }
        else
           break;
    }

    return rv;
}


bool PNS_SHOVE::pushSpringback( PNS_NODE* aNode, const PNS_ITEMSET& aHeadItems,
                                const PNS_COST_ESTIMATOR& aCost, const OPT_BOX2I& aAffectedArea )
{
    SPRINGBACK_TAG st;
    OPT_BOX2I prev_area;

    if( !m_nodeStack.empty() )
        prev_area = m_nodeStack.back().m_affectedArea;

    st.m_node = aNode;
    st.m_cost = aCost;
    st.m_headItems = aHeadItems;

    if( aAffectedArea )
    {
        if( prev_area )
            st.m_affectedArea = prev_area->Merge ( *aAffectedArea );
        else
            st.m_affectedArea = aAffectedArea;
    } else
        st.m_affectedArea = prev_area;

    m_nodeStack.push_back( st );

    return true;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::pushVia( PNS_VIA* aVia, const VECTOR2I& aForce, int aCurrentRank, bool aDryRun )
{
    LINE_PAIR_VEC draggedLines;
    VECTOR2I p0( aVia->Pos() );
    PNS_JOINT* jt = m_currentNode->FindJoint( p0, aVia );
    VECTOR2I p0_pushed( p0 + aForce );

    if( !jt )
    {
        TRACEn( 1, "weird, can't find the center-of-via joint\n" );
        return SH_INCOMPLETE;
    }

    if( jt->IsLocked() )
        return SH_INCOMPLETE;

    while( aForce.x != 0 || aForce.y != 0 )
    {
        PNS_JOINT* jt_next = m_currentNode->FindJoint( p0_pushed, aVia );

        if( !jt_next )
            break;

        p0_pushed += aForce.Resize( 2 ); // make sure pushed via does not overlap with any existing joint
    }

    PNS_VIA* pushedVia = aVia->Clone();
    pushedVia->SetPos( p0_pushed );
    pushedVia->Mark( aVia->Marker() );

    if( aVia->Marker() & MK_HEAD )
    {
        m_draggedVia = pushedVia;
        m_draggedViaHeadSet.Clear();
    }

    BOOST_FOREACH( PNS_ITEM* item, jt->LinkList() )
    {
        if( PNS_SEGMENT* seg = dyn_cast<PNS_SEGMENT*>( item ) )
        {
            LINE_PAIR lp;
            int segIndex;

            lp.first = assembleLine( seg, &segIndex );

            assert( segIndex == 0 || ( segIndex == ( lp.first.SegmentCount() - 1 ) ) );

            if( segIndex == 0 )
                lp.first.Reverse();

            lp.second = lp.first;
            lp.second.ClearSegmentLinks();
            lp.second.DragCorner( p0_pushed, lp.second.CLine().Find( p0 ) );
            lp.second.AppendVia( *pushedVia );
            draggedLines.push_back( lp );

            if( aVia->Marker() & MK_HEAD )
                m_draggedViaHeadSet.Add( lp.second );
        }
    }

    m_draggedViaHeadSet.Add( pushedVia );

    if( aDryRun )
        return SH_OK;

    replaceItems( aVia, pushedVia );

#ifdef DEBUG
    m_logger.Log( aVia, 0, "obstacle-via" );
#endif

    pushedVia->SetRank( aCurrentRank - 1 );

#ifdef DEBUG
    m_logger.Log( pushedVia, 1, "pushed-via" );
#endif

    BOOST_FOREACH( LINE_PAIR lp, draggedLines )
    {
        if( lp.first.Marker() & MK_HEAD )
        {
            lp.second.Mark( MK_HEAD );

            if( m_multiLineMode )
                return SH_INCOMPLETE;

            m_newHead = lp.second;
        }

        unwindStack( &lp.first );

        if( lp.second.SegmentCount() )
        {
            replaceItems( &lp.first, &lp.second );
            lp.second.SetRank( aCurrentRank - 1 );

            if( !pushLine( lp.second, true ) )
                return SH_INCOMPLETE;
        }
        else
        {
            m_currentNode->Remove( &lp.first );
        }

#ifdef DEBUG
        m_logger.Log( &lp.first, 2, "fan-pre" );
        m_logger.Log( &lp.second, 3, "fan-post" );
#endif
    }

    return SH_OK;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onCollidingVia( PNS_ITEM* aCurrent, PNS_VIA* aObstacleVia )
{
    int clearance = getClearance( aCurrent, aObstacleVia ) ;
    LINE_PAIR_VEC draggedLines;
    bool colLine = false, colVia = false;
    PNS_LINE* currentLine = NULL;
    VECTOR2I mtvLine, mtvVia, mtv, mtvSolid;
    int rank = -1;

    if( aCurrent->OfKind( PNS_ITEM::LINE ) )
    {
#ifdef DEBUG
         m_logger.NewGroup( "push-via-by-line", m_iter );
         m_logger.Log( aCurrent, 4, "current" );
#endif

        currentLine = (PNS_LINE*) aCurrent;
        colLine = CollideShapes( aObstacleVia->Shape(), currentLine->Shape(),
                                 clearance + currentLine->Width() / 2 + PNS_HULL_MARGIN,
                                 true, mtvLine );

        if( currentLine->EndsWithVia() )
             colVia = CollideShapes( currentLine->Via().Shape(), aObstacleVia->Shape(),
                                     clearance + PNS_HULL_MARGIN, true, mtvVia );

        if( !colLine && !colVia )
             return SH_OK;

        if( colLine && colVia )
            mtv = mtvVia.EuclideanNorm() > mtvLine.EuclideanNorm() ? mtvVia : mtvLine;
        else if( colLine )
            mtv = mtvLine;
        else
            mtv = mtvVia;

        rank = currentLine->Rank();
    }
    else if( aCurrent->OfKind( PNS_ITEM::SOLID ) )
    {
        CollideShapes( aObstacleVia->Shape(), aCurrent->Shape(),
                       clearance + PNS_HULL_MARGIN, true, mtvSolid );
        mtv = -mtvSolid;
        rank = aCurrent->Rank() + 10000;
    }

    return pushVia( aObstacleVia, mtv, rank );
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onReverseCollidingVia( PNS_LINE& aCurrent, PNS_VIA* aObstacleVia )
{
    int n = 0;
    PNS_LINE cur( aCurrent );
    cur.ClearSegmentLinks();

    PNS_JOINT* jt = m_currentNode->FindJoint( aObstacleVia->Pos(), aObstacleVia );
    PNS_LINE shoved( aCurrent );
    shoved.ClearSegmentLinks();

    cur.RemoveVia();
    unwindStack( &aCurrent );

    BOOST_FOREACH( PNS_ITEM* item, jt->LinkList() )
    {
        if( item->OfKind( PNS_ITEM::SEGMENT ) && item->LayersOverlap( &aCurrent ) )
        {
            PNS_SEGMENT* seg = (PNS_SEGMENT*) item;
            PNS_LINE head = assembleLine( seg );

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

        PNS_LINE head( aCurrent );
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
    replaceItems( &aCurrent, &shoved );

    if ( !pushLine( shoved ) )
        return SH_INCOMPLETE;

    shoved.SetRank( currentRank );

    return SH_OK;
}


void PNS_SHOVE::unwindStack( PNS_SEGMENT *aSeg )
{
    for( std::vector<PNS_LINE>::iterator i = m_lineStack.begin(); i != m_lineStack.end() ; )
    {
        if( i->ContainsSegment( aSeg ) )
            i = m_lineStack.erase( i );
        else
            i++;
    }

    for( std::vector<PNS_LINE>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end() ; )
    {
        if( i->ContainsSegment( aSeg ) )
            i = m_optimizerQueue.erase( i );
        else
            i++;
    }
}


void PNS_SHOVE::unwindStack( PNS_ITEM* aItem )
{
    if( aItem->OfKind( PNS_ITEM::SEGMENT ) )
        unwindStack( static_cast<PNS_SEGMENT*>( aItem ) );
    else if( aItem->OfKind( PNS_ITEM::LINE ) )
    {
        PNS_LINE* l = static_cast<PNS_LINE*>( aItem );

        if( !l->LinkedSegments() )
            return;

        BOOST_FOREACH( PNS_SEGMENT* seg, *l->LinkedSegments() )
            unwindStack( seg );
    }
}


bool PNS_SHOVE::pushLine( const PNS_LINE& aL, bool aKeepCurrentOnTop )
{
    if( aL.LinkCount() >= 0 && ( aL.LinkCount() != aL.SegmentCount() ) )
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

void PNS_SHOVE::popLine( )
{
    PNS_LINE& l = m_lineStack.back();

    for( std::vector<PNS_LINE>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end(); )
    {
        bool found = false;

        if( !l.LinkedSegments() )
            continue;

        BOOST_FOREACH( PNS_SEGMENT *s, *l.LinkedSegments() )
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


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::shoveIteration( int aIter )
{
    PNS_LINE currentLine = m_lineStack.back();
    PNS_NODE::OPT_OBSTACLE nearest;
    SHOVE_STATUS st = SH_NULL;

    PNS_ITEM::PnsKind search_order[] = { PNS_ITEM::SOLID, PNS_ITEM::VIA, PNS_ITEM::SEGMENT };

    for( int i = 0; i < 3; i++ )
    {
         nearest = m_currentNode->NearestObstacle( &currentLine, search_order[i] );

         if( nearest )
            break;
    }

    if( !nearest )
    {
        m_lineStack.pop_back();
        return SH_OK;
    }

    PNS_ITEM* ni = nearest->m_item;

    unwindStack( ni );

    if( !ni->OfKind( PNS_ITEM::SOLID ) && ni->Rank() >= 0 && ni->Rank() > currentLine.Rank() )
    {
        switch( ni->Kind() )
        {
        case PNS_ITEM::VIA:
        {
            PNS_VIA* revVia = (PNS_VIA*) ni;
            TRACE( 2, "iter %d: reverse-collide-via", aIter );

            if( currentLine.EndsWithVia() && m_currentNode->CheckColliding( &currentLine.Via(), revVia ) )
            {
                st = SH_INCOMPLETE;
            }
            else
            {
                st = onReverseCollidingVia ( currentLine, revVia );
            }

            break;
        }

        case PNS_ITEM::SEGMENT:
        {
            PNS_SEGMENT* seg = (PNS_SEGMENT*) ni;
            TRACE( 2, "iter %d: reverse-collide-segment ", aIter );
            PNS_LINE revLine = assembleLine( seg );

            popLine();
            st = onCollidingLine( revLine, currentLine );
            if ( !pushLine( revLine ) )
                return SH_INCOMPLETE;

            break;
        }

        default:
            assert( false );
        }
    }
    else
    { // "forward" collisions
        switch( ni->Kind() )
        {
        case PNS_ITEM::SEGMENT:
            TRACE( 2, "iter %d: collide-segment ", aIter );
            st = onCollidingSegment( currentLine, (PNS_SEGMENT*) ni );

            if( st == SH_TRY_WALK )
            {
                st = onCollidingSolid( currentLine, (PNS_SOLID*) ni );
            }
            break;

        case PNS_ITEM::VIA:
            TRACE( 2, "iter %d: shove-via ", aIter );
            st = onCollidingVia( &currentLine, (PNS_VIA*) ni );
            break;

        case PNS_ITEM::SOLID:
            TRACE( 2, "iter %d: walk-solid ", aIter );
            st = onCollidingSolid( currentLine, (PNS_SOLID*) ni );
            break;

        default:
            break;
        }
    }

    return st;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::shoveMainLoop()
{
    SHOVE_STATUS st = SH_OK;

    m_affectedAreaSum = OPT_BOX2I();

    TRACE( 1, "ShoveStart [root: %d jts, current: %d jts]", m_root->JointCount() %
           m_currentNode->JointCount() );

    int iterLimit = Settings().ShoveIterationLimit();
    TIME_LIMIT timeLimit = Settings().ShoveTimeLimit();

    m_iter = 0;

    timeLimit.Restart();

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


OPT_BOX2I PNS_SHOVE::totalAffectedArea() const
{
    OPT_BOX2I area;
    if( !m_nodeStack.empty() )
        area = m_nodeStack.back().m_affectedArea;

    if( area )
    {
        if ( m_affectedAreaSum )
            area->Merge ( *m_affectedAreaSum );
    } else
        area = m_affectedAreaSum;

    return area;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::ShoveLines( const PNS_LINE& aCurrentHead )
{
    SHOVE_STATUS st = SH_OK;

    m_multiLineMode = false;

    // empty head? nothing to shove...

    if( !aCurrentHead.SegmentCount() && !aCurrentHead.EndsWithVia() )
        return SH_INCOMPLETE;

    PNS_LINE head( aCurrentHead );
    head.ClearSegmentLinks();

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_newHead = OPT_LINE();
    m_logger.Clear();

    PNS_ITEMSET headSet;
    headSet.Add( aCurrentHead );

    reduceSpringback( headSet );

    PNS_NODE* parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;

    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();
    m_currentNode->Add( &head );

    m_currentNode->LockJoint( head.CPoint(0), &head, true );

    if( !head.EndsWithVia() )
        m_currentNode->LockJoint( head.CPoint( -1 ), &head, true );

    head.Mark( MK_HEAD );
    head.SetRank( 100000 );

    m_logger.NewGroup( "initial", 0 );
    m_logger.Log( &head, 0, "head" );

    PNS_VIA* headVia = NULL;

    if( head.EndsWithVia() )
    {
        headVia = head.Via().Clone();
        m_currentNode->Add( headVia );
        headVia->Mark( MK_HEAD );
        headVia->SetRank( 100000 );
        m_logger.Log( headVia, 0, "head-via" );
    }

    if( !pushLine( head ) )
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
            st = m_currentNode->CheckColliding( &(*m_newHead) ) ? SH_INCOMPLETE : SH_HEAD_MODIFIED;
        else
            st = m_currentNode->CheckColliding( &head ) ? SH_INCOMPLETE : SH_OK;
    }

    m_currentNode->RemoveByMarker( MK_HEAD );

    TRACE( 1, "Shove status : %s after %d iterations",
           ( ( st == SH_OK || st == SH_HEAD_MODIFIED ) ? "OK" : "FAILURE") % m_iter );

    if( st == SH_OK || st == SH_HEAD_MODIFIED )
    {
        pushSpringback( m_currentNode, headSet, PNS_COST_ESTIMATOR(), m_affectedAreaSum);
    }
    else
    {
        delete m_currentNode;

        m_currentNode = parent;
        m_newHead = OPT_LINE();
    }

    return st;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::ShoveMultiLines( const PNS_ITEMSET& aHeadSet )
{
    SHOVE_STATUS st = SH_OK;

    m_multiLineMode = true;

    PNS_ITEMSET headSet;

    BOOST_FOREACH( const PNS_ITEM* item, aHeadSet.CItems() )
    {
        const PNS_LINE* headOrig = static_cast<const PNS_LINE*>( item );

        // empty head? nothing to shove...
        if( !headOrig->SegmentCount() )
            return SH_INCOMPLETE;

        headSet.Add( *headOrig );
    }

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_logger.Clear();

    reduceSpringback( headSet );

    PNS_NODE* parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;

    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();
    int n = 0;

    BOOST_FOREACH( const PNS_ITEM* item, aHeadSet.CItems() )
    {
        const PNS_LINE* headOrig = static_cast<const PNS_LINE*>( item );
        PNS_LINE head( *headOrig );
        head.ClearSegmentLinks();

        m_currentNode->Add( &head );

        head.Mark( MK_HEAD );
        head.SetRank( 100000 );
        n++;

        if( !pushLine( head ) )
            return SH_INCOMPLETE;

        PNS_VIA* headVia = NULL;

        if( head.EndsWithVia() )
        {
            headVia = head.Via().Clone(); // fixme: leak
            m_currentNode->Add( headVia );
            headVia->Mark( MK_HEAD );
            headVia->SetRank( 100000 );
            m_logger.Log( headVia, 0, "head-via" );
        }
    }

    m_logger.NewGroup( "initial", 0 );
    //m_logger.Log( head, 0, "head" );

    st = shoveMainLoop();

    if( st == SH_OK )
        runOptimizer( m_currentNode );

    m_currentNode->RemoveByMarker( MK_HEAD );

    TRACE( 1, "Shove status : %s after %d iterations",
           ( st == SH_OK ? "OK" : "FAILURE") % m_iter );

    if( st == SH_OK )
    {
        pushSpringback( m_currentNode, PNS_ITEMSET(), PNS_COST_ESTIMATOR(), m_affectedAreaSum );
    }
    else
    {
        delete m_currentNode;
        m_currentNode = parent;
    }

    return st;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::ShoveDraggingVia( PNS_VIA* aVia, const VECTOR2I& aWhere,
                                                     PNS_VIA** aNewVia )
{
    SHOVE_STATUS st = SH_OK;

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_newHead = OPT_LINE();
    m_draggedVia = NULL;
    m_draggedViaHeadSet.Clear();

    PNS_NODE* parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;

    m_currentNode = parent;

    parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;

    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();

    aVia->Mark( MK_HEAD );

    st = pushVia( aVia, ( aWhere - aVia->Pos() ), 0 );
    st = shoveMainLoop();

    if( st == SH_OK )
        runOptimizer( m_currentNode );

    if( st == SH_OK || st == SH_HEAD_MODIFIED )
    {
        pushSpringback( m_currentNode, m_draggedViaHeadSet, PNS_COST_ESTIMATOR(), m_affectedAreaSum );
    }
    else
    {
        delete m_currentNode;
        m_currentNode = parent;
    }

    if( aNewVia )
    {
        *aNewVia = m_draggedVia;
    }
    return st;
}


void PNS_SHOVE::runOptimizer( PNS_NODE* aNode )
{
    PNS_OPTIMIZER optimizer( aNode );
    int optFlags = 0, n_passes = 0;

    PNS_OPTIMIZATION_EFFORT effort = Settings().OptimizerEffort();

    OPT_BOX2I area = totalAffectedArea();

    int maxWidth = 0;

    for( std::vector<PNS_LINE>::iterator i = m_optimizerQueue.begin();
             i != m_optimizerQueue.end(); ++i )
    {
        maxWidth = std::max( i->Width(), maxWidth );
    }

    if( area )
    {
        area->Inflate( 10 * maxWidth );
    }

    switch( effort )
    {
    case OE_LOW:
        optFlags = PNS_OPTIMIZER::MERGE_OBTUSE;
        n_passes = 1;
        break;

    case OE_MEDIUM:
        optFlags = PNS_OPTIMIZER::MERGE_SEGMENTS;

        if( area )
            optimizer.SetRestrictArea( *area );

        n_passes = 2;
        break;

    case OE_FULL:
        optFlags = PNS_OPTIMIZER::MERGE_SEGMENTS;
        n_passes = 2;
        break;

    default:
        break;
    }

    if( Settings().SmartPads() )
        optFlags |= PNS_OPTIMIZER::SMART_PADS;

    optimizer.SetEffortLevel( optFlags );
    optimizer.SetCollisionMask( PNS_ITEM::ANY );

    for( int pass = 0; pass < n_passes; pass++ )
    {
        std::reverse( m_optimizerQueue.begin(), m_optimizerQueue.end() );

        for( std::vector<PNS_LINE>::iterator i = m_optimizerQueue.begin();
             i != m_optimizerQueue.end(); ++i )
        {
            PNS_LINE& line = *i;

            if( !( line.Marker() & MK_HEAD ) )
            {
                PNS_LINE optimized;

                if( optimizer.Optimize( &line, &optimized ) )
                {
                    aNode->Remove( &line );
                    line.SetShape( optimized.CLine() );
                    aNode->Add( &line );
                }
            }
        }
    }
}


PNS_NODE* PNS_SHOVE::CurrentNode()
{
    return m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;
}


const PNS_LINE PNS_SHOVE::NewHead() const
{
    assert( m_newHead );

    return *m_newHead;
}


void PNS_SHOVE::SetInitialLine( PNS_LINE& aInitial )
{
    m_root = m_root->Branch();
    m_root->Remove( &aInitial );
}
