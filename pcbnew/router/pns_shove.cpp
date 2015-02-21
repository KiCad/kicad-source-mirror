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

#include "time_limit.h"

#include <profile.h>

static void sanityCheck( PNS_LINE* aOld, PNS_LINE* aNew )
{
    assert( aOld->CPoint( 0 ) == aNew->CPoint( 0 ) );
    assert( aOld->CPoint( -1 ) == aNew->CPoint( -1 ) );
}


PNS_SHOVE::PNS_SHOVE( PNS_NODE* aWorld, PNS_ROUTER* aRouter ) :
    PNS_ALGO_BASE ( aRouter )
{
    m_root = aWorld;
}


PNS_SHOVE::~PNS_SHOVE()
{
    // free all the stuff we've created during routing/dragging operation.
    BOOST_FOREACH( PNS_ITEM* item, m_gcItems )
        delete item;
}


// garbage-collected line assembling
PNS_LINE* PNS_SHOVE::assembleLine( const PNS_SEGMENT* aSeg, int* aIndex )
{
    PNS_LINE* l = m_currentNode->AssembleLine( const_cast<PNS_SEGMENT*>( aSeg ), aIndex );

    m_gcItems.push_back( l );

    return l;
}


// garbage-collected line cloning
PNS_LINE* PNS_SHOVE::cloneLine ( const PNS_LINE* aLine )
{
    PNS_LINE* l = aLine->Clone();

    m_gcItems.push_back( l );
    return l;
}


// A dumb function that checks if the shoved line is shoved the right way, e.g.
// visually "outwards" of the line/via applying pressure on it. Unfortunately there's no
// mathematical concept of orientation of an open curve, so we use some primitive heuristics:
// if the shoved line wraps around the start of the "pusher", it's likely shoved in wrong direction.
bool PNS_SHOVE::checkBumpDirection( PNS_LINE* aCurrent, PNS_LINE* aShoved ) const
{
    const SEG ss = aCurrent->CSegment( 0 );

    int dist = m_currentNode->GetClearance( aCurrent, aShoved ) + PNS_HULL_MARGIN;

    dist += aCurrent->Width() / 2;
    dist += aShoved->Width() / 2;

    const VECTOR2I ps = ss.A - ( ss.B - ss.A ).Resize( dist );

    return !aShoved->CLine().PointOnEdge( ps );
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::walkaroundLoneVia( PNS_LINE* aCurrent, PNS_LINE* aObstacle,
                                                      PNS_LINE* aShoved )
{
    int clearance = m_currentNode->GetClearance( aCurrent, aObstacle );
    const SHAPE_LINE_CHAIN hull = aCurrent->Via().Hull( clearance, aObstacle->Width() );
    SHAPE_LINE_CHAIN path_cw, path_ccw;

    aObstacle->Walkaround( hull, path_cw, true );
    aObstacle->Walkaround( hull, path_ccw, false );

    const SHAPE_LINE_CHAIN& shortest = path_ccw.Length() < path_cw.Length() ? path_ccw : path_cw;

    if( shortest.PointCount() < 2 )
        return SH_INCOMPLETE;

    if( aObstacle->CPoint( -1 ) != shortest.CPoint( -1 ) )
        return SH_INCOMPLETE;

    if( aObstacle->CPoint( 0 ) != shortest.CPoint( 0 ) )
        return SH_INCOMPLETE;

    aShoved->SetShape( shortest );

    if( m_currentNode->CheckColliding( aShoved, aCurrent ) )
        return SH_INCOMPLETE;

    return SH_OK;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::processHullSet( PNS_LINE* aCurrent, PNS_LINE* aObstacle,
                                                   PNS_LINE* aShoved, const HULL_SET& aHulls )
{
    const SHAPE_LINE_CHAIN& obs = aObstacle->CLine();
    bool failingDirCheck = false;
    int attempt;

    for( attempt = 0; attempt < 4; attempt++ )
    {
        bool invertTraversal = ( attempt >= 2 );
        bool clockwise = attempt % 2;
        int vFirst = -1, vLast = -1;

        SHAPE_LINE_CHAIN path;
        PNS_LINE l( *aObstacle );

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

        if( ( vFirst < 0 || vLast < 0 ) && !path.CompareGeometry( aObstacle->CLine() ) )
        {
            TRACE( 100, "attempt %d fail vfirst-last", attempt );
            continue;
        }

        if( path.CPoint( -1 ) != obs.CPoint( -1 ) || path.CPoint( 0 ) != obs.CPoint( 0 ) )
        {
            TRACE( 100, "attempt %d fail vend-start\n", attempt );
            continue;
        }

        if( !checkBumpDirection( aCurrent, &l ) )
        {
            TRACE( 100, "attempt %d fail direction-check", attempt );
            failingDirCheck = true;
            aShoved->SetShape( l.CLine() );

            continue;
        }

        if( path.SelfIntersecting() )
        {
            TRACE( 100, "attempt %d fail self-intersect", attempt );
            continue;
        }

        bool colliding = m_currentNode->CheckColliding( &l, aCurrent );

        if( ( aCurrent->Marker() & MK_HEAD ) && !colliding )
        {
            PNS_JOINT* jtStart = m_currentNode->FindJoint( aCurrent->CPoint( 0 ), aCurrent );

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

        aShoved->SetShape( l.CLine() );

        return SH_OK;
    }

    return failingDirCheck ? SH_OK : SH_INCOMPLETE;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::processSingleLine( PNS_LINE* aCurrent, PNS_LINE* aObstacle,
                                                      PNS_LINE* aShoved )
{
    aShoved->ClearSegmentLinks();

    bool obstacleIsHead = false;

    if( aObstacle->LinkedSegments() )
    {
        BOOST_FOREACH( PNS_SEGMENT* s, *aObstacle->LinkedSegments() )

        if( s->Marker() & MK_HEAD )
        {
            obstacleIsHead = true;
            break;
        }
    }

    SHOVE_STATUS rv;

    bool viaOnEnd = aCurrent->EndsWithVia();

    if( viaOnEnd && ( !aCurrent->LayersOverlap( aObstacle ) || aCurrent->SegmentCount() == 0 ) )
    {
        rv = walkaroundLoneVia( aCurrent, aObstacle, aShoved );
    }
    else
    {
        int w = aObstacle->Width();
        int n_segs = aCurrent->SegmentCount();
        int clearance = m_currentNode->GetClearance( aCurrent, aObstacle );

        HULL_SET hulls;

        hulls.reserve( n_segs + 1 );

        for( int i = 0; i < n_segs; i++ )
        {
            PNS_SEGMENT seg( *aCurrent, aCurrent->CSegment( i ) );
            hulls.push_back( seg.Hull( clearance, w ) );
        }

        if( viaOnEnd )
            hulls.push_back ( aCurrent->Via().Hull( clearance, w ) );

        rv = processHullSet ( aCurrent, aObstacle, aShoved, hulls );
    }

    if( obstacleIsHead )
        aShoved->Mark( aShoved->Marker() | MK_HEAD );

    return rv;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onCollidingSegment( PNS_LINE* aCurrent, PNS_SEGMENT* aObstacleSeg )
{
    int segIndex;
    PNS_LINE* obstacleLine = assembleLine( aObstacleSeg, &segIndex );
    PNS_LINE* shovedLine = cloneLine( obstacleLine );

    SHOVE_STATUS rv = processSingleLine( aCurrent, obstacleLine, shovedLine );

    assert ( obstacleLine->LayersOverlap( shovedLine ) );

    if( rv == SH_OK )
    {
        if( shovedLine->Marker() & MK_HEAD )
            m_newHead = *shovedLine;

        sanityCheck( obstacleLine, shovedLine );
        m_currentNode->Replace( obstacleLine, shovedLine );
        sanityCheck( obstacleLine, shovedLine );

        int rank = aCurrent->Rank();
        shovedLine->SetRank( rank - 1 );

        pushLine( shovedLine );
    }

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-segment", m_iter );
    m_logger.Log( aObstacleSeg, 0, "obstacle-segment" );
    m_logger.Log( aCurrent, 1, "current-line" );
    m_logger.Log( obstacleLine, 2, "obstacle-line" );
    m_logger.Log( shovedLine, 3, "shoved-line" );
#endif

    return rv;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onCollidingLine( PNS_LINE* aCurrent, PNS_LINE* aObstacle )
{
    PNS_LINE* shovedLine = cloneLine( aObstacle );

    SHOVE_STATUS rv = processSingleLine( aCurrent, aObstacle, shovedLine );

    if( rv == SH_OK )
    {
        if( shovedLine->Marker() & MK_HEAD )
            m_newHead = *shovedLine;

        sanityCheck( aObstacle, shovedLine );
        m_currentNode->Replace( aObstacle, shovedLine );
        sanityCheck( aObstacle, shovedLine );

        int rank = aObstacle->Rank();
        shovedLine->SetRank ( rank );

        pushLine( shovedLine );

    #ifdef DEBUG
        m_logger.NewGroup( "on-colliding-line", m_iter );
        m_logger.Log( aObstacle, 0, "obstacle-line" );
        m_logger.Log( aCurrent, 1, "current-line" );
        m_logger.Log( shovedLine, 3, "shoved-line" );
    #endif
    }

    return rv;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onCollidingSolid( PNS_LINE* aCurrent, PNS_SOLID* aObstacleSolid )
{
    PNS_WALKAROUND walkaround( m_currentNode, Router() );
    PNS_LINE* walkaroundLine = cloneLine( aCurrent );

    if( aCurrent->EndsWithVia() )
    {
        PNS_VIA vh = aCurrent->Via();
        PNS_VIA* via = NULL;
        PNS_JOINT* jtStart = m_currentNode->FindJoint( vh.Pos(), aCurrent );

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

        if( via && m_currentNode->CheckColliding( via, aObstacleSolid ) )
            return onCollidingVia( aObstacleSolid, via );
    }

    walkaround.SetSolidsOnly( true );
    walkaround.SetIterationLimit ( 8 ); // fixme: make configurable

    int currentRank = aCurrent->Rank();
    int nextRank;

    if( !Settings().JumpOverObstacles() )
    {
        nextRank = currentRank + 10000;
        walkaround.SetSingleDirection( false );
    }
    else
    {
        nextRank = currentRank - 1;
        walkaround.SetSingleDirection( true );
    }

    if( walkaround.Route( *aCurrent, *walkaroundLine, false ) != PNS_WALKAROUND::DONE )
        return SH_INCOMPLETE;

    walkaroundLine->ClearSegmentLinks();
    walkaroundLine->Unmark();
    walkaroundLine->Line().Simplify();

    if( walkaroundLine->HasLoops() )
        return SH_INCOMPLETE;

    if( aCurrent->Marker() & MK_HEAD )
    {
        walkaroundLine->Mark( MK_HEAD );
        m_newHead = *walkaroundLine;
    }

    m_currentNode->Replace( aCurrent, walkaroundLine );
    walkaroundLine->SetRank( nextRank );

#ifdef DEBUG
    m_logger.NewGroup( "on-colliding-solid", m_iter );
    m_logger.Log( aObstacleSolid, 0, "obstacle-solid" );
    m_logger.Log( aCurrent, 1, "current-line" );
    m_logger.Log( walkaroundLine, 3, "walk-line" );
#endif

    popLine();
    pushLine( walkaroundLine );

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
                                const PNS_COST_ESTIMATOR& aCost )
{
    SPRINGBACK_TAG st;

    st.m_node = aNode;
    st.m_cost = aCost;
    st.m_headItems = aHeadItems;
    m_nodeStack.push_back( st );

    return true;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::pushVia( PNS_VIA* aVia, const VECTOR2I& aForce, int aCurrentRank )
{
    LINE_PAIR_VEC draggedLines;
    VECTOR2I p0 ( aVia->Pos() );
    PNS_JOINT* jt = m_currentNode->FindJoint( p0, 1, aVia->Net() );
    PNS_VIA* pushedVia = aVia -> Clone();

    pushedVia->SetPos( p0 + aForce );
    pushedVia->Mark( aVia->Marker() );

    if( aVia->Marker() & MK_HEAD )
    {
        m_draggedVia = pushedVia;
    }

    if( !jt )
    {
        TRACEn( 1, "weird, can't find the center-of-via joint\n" );
        return SH_INCOMPLETE;
    }

    BOOST_FOREACH( PNS_ITEM* item, jt->LinkList() )
    {
        if( item->OfKind( PNS_ITEM::SEGMENT ) )
        {
            PNS_SEGMENT* seg = (PNS_SEGMENT*) item;
            LINE_PAIR lp;
            int segIndex;

            lp.first = assembleLine( seg, &segIndex );

            assert( segIndex == 0 || ( segIndex == ( lp.first->SegmentCount() - 1 ) ) );

            if( segIndex == 0 )
                lp.first->Reverse();

            lp.second = cloneLine( lp.first );
            lp.second->ClearSegmentLinks();
            lp.second->DragCorner( p0 + aForce, lp.second->CLine().Find( p0 ) );
            lp.second->AppendVia ( *pushedVia );
            draggedLines.push_back( lp );
        }
    }

    m_currentNode->Remove( aVia );
    m_currentNode->Add ( pushedVia );

#ifdef DEBUG
    m_logger.Log( aVia, 0, "obstacle-via" );
#endif

    if( aVia->BelongsTo( m_currentNode ) )
        delete aVia;

    pushedVia->SetRank( aCurrentRank - 1 );

#ifdef DEBUG
    m_logger.Log( pushedVia, 1, "pushed-via" );
#endif

    BOOST_FOREACH( LINE_PAIR lp, draggedLines )
    {
        if( lp.first->Marker() & MK_HEAD )
        {
            lp.second->Mark( MK_HEAD );
            m_newHead = *lp.second;
        }

        unwindStack( lp.first );

        if( lp.second->SegmentCount() )
        {
            m_currentNode->Replace( lp.first, lp.second );
            lp.second->SetRank( aCurrentRank - 1 );
            pushLine( lp.second );
        }
        else
            m_currentNode->Remove( lp.first );

#ifdef DEBUG
        m_logger.Log( lp.first, 2, "fan-pre" );
        m_logger.Log( lp.second, 3, "fan-post" );
#endif
    }

    return SH_OK;
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onCollidingVia( PNS_ITEM* aCurrent, PNS_VIA* aObstacleVia )
{
    int clearance = m_currentNode->GetClearance( aCurrent, aObstacleVia ) ;
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


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::onReverseCollidingVia( PNS_LINE* aCurrent, PNS_VIA* aObstacleVia )
{
    std::vector<PNS_LINE*> steps;
    int n = 0;
    PNS_LINE* cur = cloneLine( aCurrent );
    cur->ClearSegmentLinks();

    PNS_JOINT* jt = m_currentNode->FindJoint( aObstacleVia->Pos(), aObstacleVia );
    PNS_LINE* shoved = cloneLine( aCurrent );
    shoved->ClearSegmentLinks();

    cur->RemoveVia();
    unwindStack( aCurrent );

    BOOST_FOREACH( PNS_ITEM* item, jt->LinkList() )
    {
        if( item->OfKind( PNS_ITEM::SEGMENT ) && item->LayersOverlap( aCurrent ) )
        {
            PNS_SEGMENT* seg = (PNS_SEGMENT*) item;
            PNS_LINE* head = assembleLine( seg );

            head->AppendVia( *aObstacleVia );

            SHOVE_STATUS st = processSingleLine( head, cur, shoved );

            if( st != SH_OK )
            {
#ifdef DEBUG
                m_logger.NewGroup( "on-reverse-via-fail-shove", m_iter );
                m_logger.Log( aObstacleVia, 0, "the-via" );
                m_logger.Log( aCurrent, 1, "current-line" );
                m_logger.Log( shoved, 3, "shoved-line" );
#endif

                return st;
            }

            cur->SetShape( shoved->CLine() );
            n++;
        }
    }

    if( !n )
    {
#ifdef DEBUG
        m_logger.NewGroup( "on-reverse-via-fail-lonevia", m_iter );
        m_logger.Log( aObstacleVia, 0, "the-via" );
        m_logger.Log( aCurrent, 1, "current-line" );
#endif

        PNS_LINE head( *aCurrent );
        head.Line().Clear();
        head.AppendVia( *aObstacleVia );
        head.ClearSegmentLinks();

        SHOVE_STATUS st = processSingleLine( &head, aCurrent, shoved );

        if( st != SH_OK )
            return st;

        cur->SetShape( shoved->CLine() );
    }

    if( aCurrent->EndsWithVia() )
        shoved->AppendVia( aCurrent->Via() );

#ifdef DEBUG
    m_logger.NewGroup( "on-reverse-via", m_iter );
    m_logger.Log( aObstacleVia, 0, "the-via" );
    m_logger.Log( aCurrent, 1, "current-line" );
    m_logger.Log( shoved, 3, "shoved-line" );
#endif
    int currentRank = aCurrent->Rank();
    m_currentNode->Replace( aCurrent, shoved );

    pushLine( shoved );
    shoved->SetRank( currentRank );

    return SH_OK;
}


void PNS_SHOVE::unwindStack( PNS_SEGMENT *aSeg )
{
    for( std::vector<PNS_LINE*>::iterator i = m_lineStack.begin(); i != m_lineStack.end(); )
    {
        if( (*i)->ContainsSegment( aSeg ) )
            i = m_lineStack.erase( i );
        else
            i++;
    }

    for( std::vector<PNS_LINE*>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end(); )
    {
        if( (*i)->ContainsSegment( aSeg ) )
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


void PNS_SHOVE::pushLine( PNS_LINE* aL )
{
    if( aL->LinkCount() >= 0 && ( aL->LinkCount() != aL->SegmentCount() ) )
        assert( false );

    m_lineStack.push_back( aL );
    m_optimizerQueue.push_back( aL );
}


void PNS_SHOVE::popLine( )
{
    PNS_LINE* l = m_lineStack.back();

    for( std::vector<PNS_LINE*>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end(); )
    {
        if( ( *i ) == l )
        {
            i = m_optimizerQueue.erase( i );
        }
        else
            i++;
    }

    m_lineStack.pop_back();
}


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::shoveIteration( int aIter )
{
    PNS_LINE* currentLine = m_lineStack.back();
    PNS_NODE::OPT_OBSTACLE nearest;
    SHOVE_STATUS st = SH_NULL;

    PNS_ITEM::PnsKind search_order[] = { PNS_ITEM::SOLID, PNS_ITEM::VIA, PNS_ITEM::SEGMENT };

    for( int i = 0; i < 3; i++ )
    {
         nearest = m_currentNode->NearestObstacle( currentLine, search_order[i] );

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

    if( !ni->OfKind( PNS_ITEM::SOLID ) && ni->Rank() >= 0 && ni->Rank() > currentLine->Rank() )
    {
        switch( ni->Kind() )
        {
            case PNS_ITEM::VIA:
            {
                PNS_VIA* revVia = (PNS_VIA*) ni;
                TRACE( 2, "iter %d: reverse-collide-via", aIter );

                if( currentLine->EndsWithVia() && m_currentNode->CheckColliding( &currentLine->Via(), revVia ) )
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
                PNS_LINE* revLine = assembleLine( seg );

                popLine();
                st = onCollidingLine( revLine, currentLine );
                pushLine( revLine );
                break;
            }

            default:
                assert( false );
        }
    }
    else
    { // "forward" collisoins
        switch( ni->Kind() )
        {
        case PNS_ITEM::SEGMENT:
            TRACE( 2, "iter %d: collide-segment ", aIter );
            st = onCollidingSegment( currentLine, (PNS_SEGMENT*) ni );
            break;

        case PNS_ITEM::VIA:
            TRACE( 2, "iter %d: shove-via ", aIter );
            st = onCollidingVia( currentLine, (PNS_VIA*) ni );
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


PNS_SHOVE::SHOVE_STATUS PNS_SHOVE::ShoveLines( const PNS_LINE& aCurrentHead )
{
    SHOVE_STATUS st = SH_OK;

    // empty head? nothing to shove...
    if( !aCurrentHead.SegmentCount() )
        return SH_INCOMPLETE;

    PNS_LINE* head = cloneLine( &aCurrentHead );
    head->ClearSegmentLinks();

    m_lineStack.clear();
    m_optimizerQueue.clear();
    m_newHead = OPT_LINE();
    m_logger.Clear();

    PNS_ITEMSET headSet( cloneLine( &aCurrentHead ) );

    reduceSpringback( headSet );

    PNS_NODE* parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;

    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();
    m_currentNode->Add( head );

    head->Mark( MK_HEAD );
    head->SetRank( 100000 );

    m_logger.NewGroup( "initial", 0 );
    m_logger.Log( head, 0, "head" );

    PNS_VIA* headVia = NULL;

    if( head->EndsWithVia() )
    {
        headVia = head->Via().Clone();
        m_currentNode->Add( headVia );
        headVia->Mark( MK_HEAD );
        headVia->SetRank( 100000 );
        m_logger.Log( headVia, 0, "head-via" );
    }

    pushLine( head );
    st = shoveMainLoop();
    runOptimizer( m_currentNode, head );

    if( m_newHead && st == SH_OK )
    {
        st = SH_HEAD_MODIFIED;
        //Router()->DisplayDebugLine( m_newHead->CLine(), 3, 20000 );
    }

    m_currentNode->RemoveByMarker( MK_HEAD );

    TRACE( 1, "Shove status : %s after %d iterations",
           ( ( st == SH_OK || st == SH_HEAD_MODIFIED ) ? "OK" : "FAILURE") % m_iter );

    if( st == SH_OK || st == SH_HEAD_MODIFIED )
    {
        pushSpringback( m_currentNode, headSet, PNS_COST_ESTIMATOR() );
    }
    else
    {
        delete m_currentNode;

        m_currentNode = parent;
        m_newHead = OPT_LINE();
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

    //reduceSpringback( aCurrentHead );

    PNS_NODE* parent = m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;
    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();

    aVia->Mark( MK_HEAD );

    st = pushVia( aVia, ( aWhere - aVia->Pos() ), 0 );
    st = shoveMainLoop();
    runOptimizer( m_currentNode, NULL );

    if( st == SH_OK || st == SH_HEAD_MODIFIED )
    {
        pushSpringback( m_currentNode, PNS_ITEMSET(), PNS_COST_ESTIMATOR() );
    }
    else
    {
        delete m_currentNode;
        m_currentNode = parent;
    }

    if( aNewVia )
        *aNewVia = m_draggedVia;

    return st;
}


void PNS_SHOVE::runOptimizer( PNS_NODE* aNode, PNS_LINE* aHead )
{
    PNS_OPTIMIZER optimizer( aNode );
    int optFlags = 0, n_passes = 0, extend = 0;

    PNS_OPTIMIZATION_EFFORT effort = Settings().OptimizerEffort();

    switch( effort )
    {
    case OE_LOW:
        optFlags = PNS_OPTIMIZER::MERGE_OBTUSE;
        n_passes = 1;
        extend = 0;
        break;

    case OE_MEDIUM:
        optFlags = PNS_OPTIMIZER::MERGE_OBTUSE;
        n_passes = 2;
        extend = 1;
        break;

    case OE_FULL:
        optFlags = PNS_OPTIMIZER::MERGE_SEGMENTS;
        n_passes = 2;
        break;

    default:
        break;
    }

    if( Settings().SmartPads() )
        optFlags |= PNS_OPTIMIZER::SMART_PADS ;

    optimizer.SetEffortLevel( optFlags );
    optimizer.SetCollisionMask( PNS_ITEM::ANY );

    for( int pass = 0; pass < n_passes; pass++ )
    {
        std::reverse( m_optimizerQueue.begin(), m_optimizerQueue.end() );

        for( std::vector<PNS_LINE*>::iterator i = m_optimizerQueue.begin();
             i != m_optimizerQueue.end(); ++i )
        {
            PNS_LINE* line = *i;

            if( !( line -> Marker() & MK_HEAD ) )
            {
                if( effort == OE_MEDIUM || effort == OE_LOW )
                {
                    RANGE<int> r = findShovedVertexRange( line );

                    if( r.Defined() )
                    {
                        int start_v = std::max( 0, r.MinV() - extend );
                        int end_v = std::min( line->PointCount() - 1 , r.MaxV()  + extend );
                        line->ClipVertexRange( start_v, end_v );
                    }
                }

                PNS_LINE optimized;

                if( optimizer.Optimize( line, &optimized ) )
                {
                    aNode->Remove( line );
                    line->SetShape( optimized.CLine() );
                    aNode->Add( line );
                }
            }
        }
    }
}


const RANGE<int> PNS_SHOVE::findShovedVertexRange( PNS_LINE* aL )
{
    RANGE<int> r;

    for( int i = 0; i < aL->SegmentCount(); i++ )
    {
        PNS_SEGMENT* s = (*aL->LinkedSegments())[i];
        PNS_JOINT* jt = m_root->FindJoint( s->Seg().A, s->Layer(), s->Net() );
        bool found = false;

        if( jt )
        {
            BOOST_FOREACH( PNS_ITEM* item, jt->LinkList() )
            {
                if( item->OfKind( PNS_ITEM::SEGMENT ) )
                {
                    PNS_SEGMENT* s_old = (PNS_SEGMENT*) item;

                    if( s_old->Net() == s->Net() &&
                        s_old->Layer() == s->Layer() &&
                        s_old->Seg().A == s->Seg().A &&
                        s_old->Seg().B == s->Seg().B )
                    {
                        found = true;
                        break;
                    }
                }
            }
        }

        if( !found )
        {
            r.Grow( i );
            r.Grow( i + 1 );
        }
    }

    return r;
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


void PNS_SHOVE::SetInitialLine( PNS_LINE* aInitial )
{
    m_root = m_root->Branch();
    m_root->Remove( aInitial );
}
