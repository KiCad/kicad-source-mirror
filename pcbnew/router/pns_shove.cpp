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

#include <deque>
#include <cassert>
#include <math/box2.h>

#include <wx/log.h>

#include "pns_arc.h"
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

// fixme - move all logger calls to debug decorator

typedef VECTOR2I::extended_type ecoord;

namespace PNS {

void SHOVE::replaceItems( ITEM* aOld, std::unique_ptr< ITEM > aNew )
{
    OPT_BOX2I changed_area = ChangedArea( aOld, aNew.get() );

    if( changed_area )
        m_affectedArea = m_affectedArea ? m_affectedArea->Merge( *changed_area ) : *changed_area;

    ROOT_LINE_ENTRY *re = nullptr;
    LINKED_ITEM::UNIQ_ID newId;

    if( aOld->OfKind( ITEM::VIA_T ) )
    {
        VIA* vold = static_cast<VIA*>( aOld );
        VIA* vnew = static_cast<VIA*>( aNew.get() );
        re = touchRootLine( vold );
        re->newVia = vnew;
        newId = static_cast<VIA*>( aNew.get() )->Uid();

        PNS_DBG( Dbg(), Message,
                 wxString::Format( "replace-via node=%p vold=%p [%d %d]-> vnew=%p [%d %d] nid %llu", m_currentNode, aOld,
                                   vold->Pos().x, vold->Pos().y, aNew.get(), vnew->Pos().x,
                                   vnew->Pos().y, newId ) );
    }

    m_currentNode->Replace( aOld, std::move( aNew ) );

    if( re )
        m_rootLineHistory[ newId ] = re;
}


SHOVE::ROOT_LINE_ENTRY* SHOVE::replaceLine( LINE& aOld, LINE& aNew, bool aIncludeInChangedArea, bool aAllowRedundantSegments, NODE* aNode )
{
    if( aIncludeInChangedArea )
    {
        OPT_BOX2I changed_area = ChangedArea( aOld, aNew );

        if( changed_area )
        {
            SHAPE_RECT r( *changed_area );
            PNS_DBG( Dbg(), AddShape, &r, BLUE, 0, wxT( "shove-changed-area" ) );

            m_affectedArea = m_affectedArea ? m_affectedArea->Merge( *changed_area )
                                            : *changed_area;
        }
    }

    int old_sc = aOld.SegmentCount();
    int old_lc = aOld.LinkCount();

    if( aOld.EndsWithVia() )
    {
        LINKED_ITEM* viaLink = nullptr;
        for( LINKED_ITEM* lnk : aOld.Links() )
        {
            if( lnk->OfKind( ITEM::VIA_T ) )
            {
                viaLink = lnk;
                break;
            }
        }
        if( viaLink )
            aOld.Unlink( viaLink );
    }

    bool  foundPredecessor = false;
    ROOT_LINE_ENTRY *rootEntry = nullptr;

    // Keep track of the 'root lines', i.e. the unmodified (pre-shove) versions
    // of the affected tracks in a map. The optimizer can then query the pre-shove shape
    // for each shoved line and perform additional constraint checks (i.e. prevent overzealous
    // optimizations)

    // Check if the shoved line already has an ancestor (e.g. line from a previous shove
    // iteration/cursor movement)
    for( LINKED_ITEM* link : aOld.Links() )
    {
        auto oldLineIter = m_rootLineHistory.find( link->Uid() );

        if( oldLineIter != m_rootLineHistory.end() )
        {
            rootEntry = oldLineIter->second;
            foundPredecessor = true;
            break;
        }
    }

    // If found, use it, otherwise, create new entry in the map (we have a genuine new 'root' line)
    if( !foundPredecessor )
    {
        if( ! rootEntry )
        {
            rootEntry = new ROOT_LINE_ENTRY( aOld.Clone() );
        }

        for( LINKED_ITEM* link : aOld.Links() )
        {
            m_rootLineHistory[link->Uid()] = rootEntry;
        }
    }

    // Now update the NODE (calling Replace invalidates the Links() in a LINE)
    if( aNode )
        aNode->Replace( aOld, aNew, aAllowRedundantSegments );
    else
        m_currentNode->Replace( aOld, aNew, aAllowRedundantSegments );


    // point the Links() of the new line to its oldest ancestor
    for( LINKED_ITEM* link : aNew.Links() )
    {
        m_rootLineHistory[ link->Uid() ] = rootEntry;
    }

    rootEntry->newLine = aNew;

    return rootEntry;
}


int SHOVE::getClearance( const ITEM* aA, const ITEM* aB ) const
{
    if( m_forceClearance >= 0 )
        return m_forceClearance;

    int clearance = m_currentNode->GetClearance( aA, aB, false );

    if( aA->HasHole() )
        clearance = std::max( clearance, m_currentNode->GetClearance( aA->Hole(), aB, false ) );

    if( aB->HasHole() )
        clearance = std::max( clearance, m_currentNode->GetClearance( aA, aB->Hole(), false ) );

    return clearance;
}


void SHOVE::sanityCheck( LINE* aOld, LINE* aNew )
{
    assert( aOld->CPoint( 0 ) == aNew->CPoint( 0 ) );
    assert( aOld->CLastPoint() == aNew->CLastPoint() );
}


SHOVE::SHOVE( NODE* aWorld, ROUTER* aRouter ) :
    ALGO_BASE( aRouter )
{
    m_optFlagDisableMask = 0;
    m_forceClearance = -1;
    m_root = aWorld;
    m_currentNode = aWorld;
    SetDebugDecorator( aRouter->GetInterface()->GetDebugDecorator() );

    // Initialize other temporary variables:
    m_draggedVia = nullptr;
    m_iter = 0;
    m_multiLineMode = false;
    m_headsModified = false;
    m_restrictSpringbackTagId = 0;
    m_springbackDoNotTouchNode = nullptr;
    m_defaultPolicy = SHP_SHOVE;
}


SHOVE::~SHOVE()
{
}


LINE SHOVE::assembleLine( const LINKED_ITEM* aSeg, int* aIndex, bool aPreCleanup )
{
    LINE cur = m_currentNode->AssembleLine( const_cast<LINKED_ITEM*>( aSeg ), aIndex, true );

    if( aPreCleanup )
    {
        LINE cleaned;

        if (preShoveCleanup( &cur, &cleaned ) )
            return cleaned;
    }

    return cur;
}


// A dumb function that checks if the shoved line is shoved the right way, e.g. visually
// "outwards" of the line/via applying pressure on it. Unfortunately there's no mathematical
// concept of orientation of an open curve, so we use some primitive heuristics: if the shoved
// line wraps around the start of the "pusher", it's likely shoved in wrong direction.

// Update: there's no concept of an orientation of an open curve, but nonetheless Tom's dumb
// as.... (censored)
// Two open curves put together make a closed polygon... Tom should learn high school geometry!
bool SHOVE::checkShoveDirection( const LINE& aCurLine, const LINE& aObstacleLine,
                                 const LINE& aShovedLine ) const
{
    SHAPE_LINE_CHAIN::POINT_INSIDE_TRACKER checker( aCurLine.CPoint( 0) );
    checker.AddPolyline( aObstacleLine.CLine() );
    checker.AddPolyline( aShovedLine.CLine().Reverse() );

    bool inside = checker.IsInside();

    return !inside;
}


/*
 * Push aObstacleLine away from aCurLine's via by the clearance distance, returning the result
 * in aResultLine.
 *
 * Must be called only when aCurLine itself is on another layer (or has no segments) so that it
 * can be ignored.
 */
bool SHOVE::shoveLineFromLoneVia( const LINE& aCurLine, const LINE& aObstacleLine,
                                                 LINE& aResultLine )
{
    // Build a hull for aCurLine's via and re-walk aObstacleLine around it.

    int        obstacleLineWidth = aObstacleLine.Width();
    const VIA& via = aCurLine.Via();
    int        clearance = getClearance( &via, &aObstacleLine );
    HOLE*      viaHole = via.Hole();
    int        holeClearance = getClearance( viaHole, &aObstacleLine );

    if( holeClearance + via.Drill() / 2 > clearance + via.Diameter( aObstacleLine.Layer() ) / 2 )
        clearance = holeClearance + via.Drill() / 2 - via.Diameter( aObstacleLine.Layer() ) / 2;

    SHAPE_LINE_CHAIN hull = aCurLine.Via().Hull( clearance, obstacleLineWidth, aCurLine.Layer() );
    SHAPE_LINE_CHAIN path_cw;
    SHAPE_LINE_CHAIN path_ccw;

    if( ! aObstacleLine.Walkaround( hull, path_cw, true ) )
        return false;

    if( ! aObstacleLine.Walkaround( hull, path_ccw, false ) )
        return false;

    const SHAPE_LINE_CHAIN& shortest = path_ccw.Length() < path_cw.Length() ? path_ccw : path_cw;

    if( shortest.PointCount() < 2 )
        return false;

    if( aObstacleLine.CLastPoint() != shortest.CLastPoint() )
        return false;

    if( aObstacleLine.CPoint( 0 ) != shortest.CPoint( 0 ) )
        return false;

    aResultLine.SetShape( shortest );

    if( aResultLine.Collide( &aCurLine, m_currentNode, aResultLine.Layer() ) )
        return false;

    return true;
}


/*
 * Re-walk aObstacleLine around the given set of hulls, returning the result in aResultLine.
 */
bool SHOVE::shoveLineToHullSet( const LINE& aCurLine, const LINE& aObstacleLine, LINE& aResultLine,
                                const HULL_SET& aHulls, bool aPermitAdjustingStart,
                                bool aPermitAdjustingEnd )
{
    const int c_ENDPOINT_ON_HULL_THRESHOLD = 1000;
    int attempt;
    bool permitAdjustingEndpoints = aPermitAdjustingStart || aPermitAdjustingEnd;

    PNS_DBG( Dbg(), BeginGroup, "shove-details", 1 );

    for( attempt = 0; attempt < 4; attempt++ )
    {
        bool invertTraversal = ( attempt >= 2 );
        bool clockwise = attempt % 2;
        int vFirst = -1, vLast = -1;
        SHAPE_LINE_CHAIN obs = aObstacleLine.CLine();
        LINE l( aObstacleLine );
        SHAPE_LINE_CHAIN path( l.CLine() );

        if( permitAdjustingEndpoints && l.SegmentCount() >= 1 )
        {
            auto minDistP = [&]( VECTOR2I pref, int& mdist, int& minhull ) -> VECTOR2I
            {
                int      min_dist = std::numeric_limits<int>::max();
                VECTOR2I nearestP;

                for( int i = 0; i < (int) aHulls.size(); i++ )
                {
                    const SHAPE_LINE_CHAIN& hull =
                            aHulls[invertTraversal ? aHulls.size() - 1 - i : i];
                    int  dist;
                    const VECTOR2I p = hull.NearestPoint( pref, true );

                    if( hull.PointInside( pref ) )
                        dist = 0;
                    else
                        dist = ( p - pref ).EuclideanNorm();

                    if( dist < c_ENDPOINT_ON_HULL_THRESHOLD && dist < min_dist )
                    {
                        bool reject = false;

                        if( !reject )
                        {
                            min_dist = dist;
                            nearestP = p;
                            minhull = invertTraversal ? aHulls.size() - 1 - i : i;
                        }
                    }
                }
                mdist = min_dist;
                return nearestP;
            };

            int      minDist0, minDist1, minhull0, minhull1 ;
            VECTOR2I p0 = minDistP( l.CPoint( 0 ), minDist0, minhull0 );
            VECTOR2I p1 = minDistP( l.CLastPoint(), minDist1, minhull1 );

            PNS_DBG( Dbg(), Message, wxString::Format( "mindists : %d %d hulls %d %d\n", minDist0, minDist1, minhull0, minhull1 ) );

            if( minDist1 < c_ENDPOINT_ON_HULL_THRESHOLD && aPermitAdjustingEnd )
            {
                l.Line().Append( p1 );
                obs = l.CLine();
                path = l.CLine();
            }

            if( minDist0 < c_ENDPOINT_ON_HULL_THRESHOLD && aPermitAdjustingStart )
            {
                l.Line().Insert( 0, p0 );
                obs = l.CLine();
                path = l.CLine();
            }
        }


        bool failWalk = false;

        for( int i = 0; i < (int) aHulls.size(); i++ )
        {
            const SHAPE_LINE_CHAIN& hull = aHulls[invertTraversal ? aHulls.size() - 1 - i : i];

            PNS_DBG( Dbg(), AddShape, &hull, YELLOW, 10000, wxString::Format( "hull[%d]", i ) );
            PNS_DBG( Dbg(), AddShape, &path, WHITE, l.Width(), wxString::Format( "path[%d]", i ) );
            PNS_DBG( Dbg(), AddShape, &obs, LIGHTGRAY, aObstacleLine.Width(),  wxString::Format( "obs[%d]", i ) );

            if( !l.Walkaround( hull, path, clockwise ) )
            {
                PNS_DBG( Dbg(), Message, wxString::Format( wxT( "Fail-Walk %s %s %d\n" ),
                                                           hull.Format().c_str(),
                                                           l.CLine().Format().c_str(),
                                                           clockwise? 1 : 0) );

                failWalk = true;
                break;
            }

            path.Simplify2();
            l.SetShape( path );
        }

        if( failWalk )
            continue;

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

        if( ( vFirst < 0 || vLast < 0 ) && !path.CompareGeometry( obs ) )
        {
            PNS_DBG( Dbg(), Message, wxString::Format( wxT( "attempt %d fail vfirst-last" ),
                                                       attempt ) );
            continue;
        }

        if( path.CLastPoint() != obs.CLastPoint() || path.CPoint( 0 ) != obs.CPoint( 0 ) )
        {
            PNS_DBG( Dbg(), Message, wxString::Format( wxT( "attempt %d fail vend-start\n" ),
                                                       attempt ) );
            continue;
        }

        if( !checkShoveDirection( aCurLine, aObstacleLine, l ) )
        {
            PNS_DBG( Dbg(), Message, wxString::Format( wxT( "attempt %d fail direction-check" ),
                                                       attempt ) );
            aResultLine.SetShape( l.CLine() );
            continue;
        }

        if( path.SelfIntersecting() )
        {
            PNS_DBG( Dbg(), Message, wxString::Format( wxT( "attempt %d fail self-intersect" ),
                                                       attempt ) );
            continue;
        }

        bool colliding = l.Collide( &aCurLine, m_currentNode, l.Layer() );

      #if 0
        if(( aCurLine.Marker() & MK_HEAD ) && !colliding )
        {
            const JOINT* jtStart = m_currentNode->FindJoint( aCurLine.CPoint( 0 ), &aCurLine );

            for( ITEM* item : jtStart->LinkList() )
            {
                if( item->Collide( &l, m_currentNode, l.Layer() ) )
                    colliding = true;
            }
        }
      #endif

        if( colliding )
        {
            PNS_DBG( Dbg(), Message, wxString::Format( wxT( "attempt %d fail coll-check" ),
                                                       attempt ) );
            continue;
        }

        aResultLine.SetShape( l.CLine() );

        PNS_DBGN( Dbg(), EndGroup );

        return true;
    }

    PNS_DBGN( Dbg(), EndGroup );

    return false;
}


/*
 * Push aObstacleLine line away from aCurLine by the clearance distance, and return the result in
 * aResultLine.
 */
bool SHOVE::ShoveObstacleLine( const LINE& aCurLine, const LINE& aObstacleLine,
                                              LINE& aResultLine )
{
    const int cHullFailureExpansionFactor = 1000;
    int extraHullExpansion = 0;

    bool voeStart = false, voeEnd = false;
    const JOINT* jtStart = nullptr;
    const JOINT* jtEnd = nullptr;

    if( aObstacleLine.PointCount() >= 2 )
    {
        jtStart = m_currentNode->FindJoint( aObstacleLine.CPoint( 0 ), &aObstacleLine );
        jtEnd = m_currentNode->FindJoint( aObstacleLine.CLastPoint(), &aObstacleLine );
    }

    if( jtStart )
        voeStart = jtStart->Via() != nullptr;
    if( jtEnd )
        voeEnd = jtEnd->Via() != nullptr;

    aResultLine.ClearLinks();
    bool viaOnEnd = aCurLine.EndsWithVia();

    LINE obstacleLine( aObstacleLine );
    std::optional<VIA> obsVia;

    if( obstacleLine.EndsWithVia() )
    {
        obsVia = aObstacleLine.Via();
        obstacleLine.RemoveVia();
    }

    PNS_DBG( Dbg(), Message, wxString::Format( wxT( "shove process-single: voe-cur %d voe-obs %d" ),
                                          aCurLine.EndsWithVia()?1:0, aObstacleLine.EndsWithVia()?1:0 ) );


    if( viaOnEnd && ( !aCurLine.LayersOverlap( &obstacleLine ) || aCurLine.SegmentCount() == 0 ) )
    {
        // Shove obstacleLine to the hull of aCurLine's via.
        return shoveLineFromLoneVia( aCurLine, obstacleLine, aResultLine );
    }
    else
    {
        // Build a set of hulls around the segments of aCurLine.  Hulls are at the clearance
        // distance + obstacleLine's linewidth so that when re-walking obstacleLine along the
        // hull it will be at the appropriate clearance.

        int      obstacleLineWidth = obstacleLine.Width();
        int      clearance = getClearance( &aCurLine, &obstacleLine );
        int      currentLineSegmentCount = aCurLine.SegmentCount();

        /*PNS_DBG( Dbg(), Message, wxString::Format( wxT( "shove process-single: cur net %d obs %d cl %d" ),
                                                   m_router->GetInterface()->GetNetCode( aCurLine.Net() ),
                                                   m_router->GetInterface()->GetNetCode( obstacleLine.Net() ),
                                                   clearance ) );*/

        for( int attempt = 0; attempt < 3; attempt++ )
        {
            HULL_SET hulls;

            hulls.reserve( currentLineSegmentCount + 1 );

            for( int i = 0; i < currentLineSegmentCount; i++ )
            {
                SEGMENT seg( aCurLine, aCurLine.CSegment( i ) );

                // Arcs need additional clearance to ensure the hulls are always bigger than the arc
                if( aCurLine.CLine().IsArcSegment( i ) )
                {
                    PNS_DBG( Dbg(), Message, wxString::Format( wxT( "shove add-extra-clearance %d" ),
                                                            SHAPE_ARC::DefaultAccuracyForPCB() ) );
                    clearance += KiROUND( SHAPE_ARC::DefaultAccuracyForPCB() );
                }

                SHAPE_LINE_CHAIN hull = seg.Hull( clearance + extraHullExpansion, obstacleLineWidth, obstacleLine.Layer() );

                hulls.push_back( hull );
            }

            if( viaOnEnd )
            {
                const VIA& via = aCurLine.Via();
                int        viaClearance = getClearance( &via, &obstacleLine );
                HOLE*      viaHole = via.Hole();
                int        holeClearance = getClearance( viaHole, &obstacleLine );
                int        layer = aObstacleLine.Layer();

                if( holeClearance + via.Drill() / 2 > viaClearance + via.Diameter( layer ) / 2 )
                {
                    viaClearance = holeClearance + via.Drill() / 2 - via.Diameter( layer ) / 2;
                }

                hulls.push_back( aCurLine.Via().Hull( viaClearance, obstacleLineWidth, layer ) );
            }

            bool permitMovingStart = (attempt >= 2) && !voeStart;
            bool permitMovingEnd = (attempt >= 2) && !voeEnd;

            if (shoveLineToHullSet( aCurLine, obstacleLine, aResultLine, hulls, permitMovingStart, permitMovingEnd ) )
            {
                if( obsVia )
                    aResultLine.AppendVia( *obsVia );

                return true;
            }

            extraHullExpansion += cHullFailureExpansionFactor;
        }
    }

    return false;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::onCollidingSegment( LINE& aCurrent, SEGMENT* aObstacleSeg )
{
    int segIndex;

    LINE obstacleLine = assembleLine( aObstacleSeg, &segIndex, true );
    LINE shovedLine( obstacleLine );
    SEGMENT tmp( *aObstacleSeg );

    if( obstacleLine.HasLockedSegments() )
    {
        PNS_DBG(Dbg(), Message, "try walk (locked segments)");
        return SH_TRY_WALK;
    }

    bool shoveOK = ShoveObstacleLine( aCurrent, obstacleLine, shovedLine );

    const double extensionWalkThreshold = 1.0;

    double obsLen = obstacleLine.CLine().Length();
    double shovedLen = shovedLine.CLine().Length();
    double extensionFactor = 0.0;

    if( obsLen != 0.0f )
        extensionFactor = shovedLen / obsLen - 1.0;

    //if( extensionFactor > extensionWalkThreshold )
      //  return SH_TRY_WALK;

    assert( obstacleLine.LayersOverlap( &shovedLine ) );

    if( Dbg() )
    {
        PNS_DBG( Dbg(), AddItem, aObstacleSeg, BLUE, 0, wxT( "colliding-segment" ) );
        PNS_DBG( Dbg(), AddItem, &aCurrent, RED, 10000, wxString::Format( "current-line [links %d l %d v %d]", aCurrent.LinkCount(), aCurrent.Layer(), aCurrent.EndsWithVia() ) );
        PNS_DBG( Dbg(), AddItem, &obstacleLine, GREEN, 10000, wxString::Format( "obstacle-line [links %d l %d v %d]", obstacleLine.LinkCount(), obstacleLine.Layer(), obstacleLine.EndsWithVia() ) );
        PNS_DBG( Dbg(), AddItem, &shovedLine, BLUE, 10000, wxT( "shoved-line" ) );
    }

    if( shoveOK )
    {
        int rank = aCurrent.Rank();

        shovedLine.SetRank( rank - 1 );
        shovedLine.Line().Simplify2();

        unwindLineStack( &obstacleLine );

        replaceLine( obstacleLine, shovedLine, true, false );

        if( !pushLineStack( shovedLine ) )
            return SH_INCOMPLETE;

        return SH_OK;
    }

    return SH_INCOMPLETE;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::onCollidingArc( LINE& aCurrent, ARC* aObstacleArc )
{
    int segIndex;
    LINE obstacleLine = assembleLine( aObstacleArc, &segIndex );
    LINE shovedLine( obstacleLine );
    ARC tmp( *aObstacleArc );

    if( obstacleLine.HasLockedSegments() )
        return SH_TRY_WALK;

    bool shoveOK = ShoveObstacleLine( aCurrent, obstacleLine, shovedLine );

    const double extensionWalkThreshold = 1.0;

    double obsLen = obstacleLine.CLine().Length();
    double shovedLen = shovedLine.CLine().Length();
    double extensionFactor = 0.0;

    if( obsLen != 0.0f )
        extensionFactor = shovedLen / obsLen - 1.0;

    if( extensionFactor > extensionWalkThreshold )
        return SH_TRY_WALK;

    assert( obstacleLine.LayersOverlap( &shovedLine ) );

    PNS_DBG( Dbg(), AddItem, &tmp, WHITE, 10000, wxT( "obstacle-arc" ) );
    PNS_DBG( Dbg(), AddItem, &aCurrent, RED, 10000, wxT( "current-line" ) );
    PNS_DBG( Dbg(), AddItem, &obstacleLine, GREEN, 10000, wxT( "obstacle-line" ) );
    PNS_DBG( Dbg(), AddItem, &shovedLine, BLUE, 10000, wxT( "shoved-line" ) );

    if( shoveOK )
    {
        int rank = aCurrent.Rank();
        shovedLine.SetRank( rank - 1 );

        replaceLine( obstacleLine, shovedLine, true, false );

        if( !pushLineStack( shovedLine ) )
            return SH_INCOMPLETE;
    }

    return SH_OK;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::onCollidingLine( LINE& aCurrent, LINE& aObstacle, int aNextRank )
{
    LINE shovedLine( aObstacle );

    bool shoveOK = ShoveObstacleLine( aCurrent, aObstacle, shovedLine );

    PNS_DBG( Dbg(), AddItem, &aObstacle, RED, 100000, wxT( "obstacle-line" ) );
    PNS_DBG( Dbg(), AddItem, &aCurrent, GREEN, 150000, wxT( "current-line" ) );
    PNS_DBG( Dbg(), AddItem, &shovedLine, BLUE, 200000, wxT( "shoved-line" ) );

    if( shoveOK )
    {
#if 0
        if( shovedLine.Marker() & MK_HEAD )
        {
            if( m_multiLineMode )
                return SH_INCOMPLETE;

            m_newHead = shovedLine;
        }
#endif
        replaceLine( aObstacle, shovedLine, true, false );

        shovedLine.SetRank( aNextRank );

        if( !pushLineStack( shovedLine ) )
            return SH_INCOMPLETE;

        return SH_OK;
    }

    return SH_INCOMPLETE;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::onCollidingSolid( LINE& aCurrent, ITEM* aObstacle, OBSTACLE& aObstacleInfo )
{
    LINE walkaroundLine( aCurrent );

    if( aCurrent.EndsWithVia() )
    {
        VIA vh = aCurrent.Via();
        VIA* via = nullptr;
        const JOINT* jtStart = m_currentNode->FindJoint( vh.Pos(), &aCurrent );

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

        // TODO(JE) viastacks -- can aObstacle be a via?
        if( via && via->Collide( aObstacle, m_currentNode, aObstacle->Layer() ) )
            return onCollidingVia( aObstacle, via, aObstacleInfo, aObstacle->Rank() - 1 );
    }

    TOPOLOGY topo( m_currentNode );
    TOPOLOGY::CLUSTER cluster = topo.AssembleCluster( aObstacle, aCurrent.Layers().Start(), 10.0 );

    PNS_DBG( Dbg(), BeginGroup, "walk-cluster", 1 );

    for( ITEM* item : cluster.m_items )
        PNS_DBG( Dbg(), AddItem, item, RED, 10000, wxT( "cl-item" ) );

    PNS_DBGN( Dbg(), EndGroup );

    WALKAROUND walkaround( m_currentNode, Router() );
    walkaround.SetDebugDecorator( Dbg() );
    walkaround.SetSolidsOnly( false );
    walkaround.RestrictToCluster( true, cluster );
    walkaround.SetAllowedPolicies( { WALKAROUND::WP_SHORTEST } );
    walkaround.SetIterationLimit( Settings().WalkaroundIterationLimit() ); // fixme: make configurable

    int currentRank = aCurrent.Rank();
    int nextRank;

    bool success = false;

    PNS_DBG( Dbg(), AddItem, &aCurrent, RED, 10000, wxT( "current-line" ) );

    for( int attempt = 0; attempt < 2; attempt++ )
    {
        if( attempt == 1 || Settings().JumpOverObstacles() )
            nextRank = currentRank - 1;
        else
            nextRank = currentRank + 10000;

    	WALKAROUND::RESULT status = walkaround.Route( aCurrent );

        if( status.status[ WALKAROUND::WP_SHORTEST ] != WALKAROUND::ST_DONE )//fixme policies!
            continue;

        walkaroundLine = status.lines[ WALKAROUND::WP_SHORTEST ];

        walkaroundLine.ClearLinks();
        walkaroundLine.Unmark();
    	walkaroundLine.Line().Simplify2();

    	if( walkaroundLine.HasLoops() )
            continue;

        PNS_DBG( Dbg(), AddItem, &walkaroundLine, BLUE, 10000, wxT( "walk-line" ) );

#if 0
    	if( aCurrent.Marker() & MK_HEAD )
    	{
            walkaroundLine.Mark( MK_HEAD );

            if( m_multiLineMode )
                continue;

            m_newHead = walkaroundLine;
        }
#endif


        if( !m_lineStack.empty() )
        {
            LINE lastLine = m_lineStack.front();

            if( lastLine.Collide( &walkaroundLine, m_currentNode, lastLine.Layer() ) )
            {
                LINE dummy( lastLine );

                if( ShoveObstacleLine( walkaroundLine, lastLine, dummy ) )
                {
                    success = true;
                    break;
                }
            }
            else
            {
                success = true;
                break;
            }
        }
    }

    if(!success)
        return SH_INCOMPLETE;

    replaceLine( aCurrent, walkaroundLine, true, false );
    walkaroundLine.SetRank( nextRank );


    popLineStack();

    if( !pushLineStack( walkaroundLine ) )
        return SH_INCOMPLETE;

    return SH_OK;
}


void SHOVE::pruneRootLines( NODE *aRemovedNode )
{
    PNS_DBG( Dbg(), Message, wxString::Format("prune called" ) );

    NODE::ITEM_VECTOR added, removed;
    aRemovedNode->GetUpdatedItems( removed, added );

    for( const ITEM* item : added )
    {
            if( item->OfKind( ITEM::LINKED_ITEM_MASK_T ) )
            {
                const LINKED_ITEM* litem = static_cast<const LINKED_ITEM*>( item );

                m_rootLineHistory.erase( litem->Uid() );
            }
    }
}


/*
 * Pops NODE stackframes which no longer collide with aHeadSet.  Optionally sets aDraggedVia
 * to the dragged via of the last unpopped state.
 */
NODE* SHOVE::reduceSpringback( const ITEM_SET& aHeadSet )
{
    while( m_nodeStack.size() > 1 )
    {
        SPRINGBACK_TAG& spTag = m_nodeStack.back();

        // Prevent the springback algo from erasing NODEs that might contain items used by the ROUTER_TOOL/LINE_PLACER.
        // I noticed this can happen for the endItem provided to LINE_PLACER::Move() and cause a nasty crash.
        if( spTag.m_node == m_springbackDoNotTouchNode )
            break;

        std::optional<OBSTACLE> obs = spTag.m_node->CheckColliding( aHeadSet );

        if( !obs && !spTag.m_locked )
        {
            int i;

            PNS_DBG( Dbg(), Message, wxString::Format( "pop-sp node=%p depth=%d", spTag.m_node, spTag.m_node->Depth() ) );

            pruneRootLines( spTag.m_node );


            delete spTag.m_node;
            m_nodeStack.pop_back();
        }
        else
        {
           break;
        }
    }

    if( m_nodeStack.empty() )
        return m_root;

    SPRINGBACK_TAG& spTag = m_nodeStack.back();

    for( int i = 0; i < spTag.m_draggedVias.size(); i++ )
    {
        if (spTag.m_draggedVias[i].valid)
        {
            m_headLines[i].prevVia = m_headLines[i].theVia = spTag.m_draggedVias[i];
            m_headLines[i].geometryModified = true;
            PNS_DBG( Dbg(), Message,
                     wxString::Format( "restore-springback-via depth=%d %d %d %d %d ",
                                       spTag.m_node->Depth(),
                                       (int) m_nodeStack.size(),
                                       m_headLines[i].theVia->pos.x,
                                       m_headLines[i].theVia->pos.y,
                                       m_headLines[i].theVia->layers.Start(),
                                       m_headLines[i].theVia->layers.End() ) );
        }
    }

    return m_nodeStack.back().m_node;
}


/*
 * Push the current NODE on to the stack.  aDraggedVia is the dragged via *before* the push
 * (which will be restored in the event the stackframe is popped).
 */
bool SHOVE::pushSpringback( NODE* aNode, const OPT_BOX2I& aAffectedArea )
{
    SPRINGBACK_TAG st;
    OPT_BOX2I prev_area;

    if( !m_nodeStack.empty() )
        prev_area = m_nodeStack.back().m_affectedArea;

    st.m_draggedVias.resize( m_headLines.size() );
    int n = 0;

    for( HEAD_LINE_ENTRY& head : m_headLines )
    {
        if ( head.theVia )
        {
            VIA_HANDLE vhandle = *head.theVia;

            PNS_DBG( Dbg(), Message,
                                wxString::Format( "push-sp via depth=%d %d %d %d %d ", aNode->Depth(), vhandle.pos.x,
                                                vhandle.pos.y,
                                                vhandle.layers.Start(),
                                                vhandle.layers.End() ) );

            st.m_draggedVias[n] = vhandle;
        }

        n++;
    }

    st.m_node = aNode;

    if( aAffectedArea )
    {
        if( prev_area )
            st.m_affectedArea = prev_area->Merge( *aAffectedArea );
        else
            st.m_affectedArea = aAffectedArea;
    }
    else
    {
        st.m_affectedArea = prev_area;
    }

    st.m_seq = ( m_nodeStack.empty() ? 1 : m_nodeStack.back().m_seq + 1 );
    st.m_locked = false;

    m_nodeStack.push_back( st );

    PNS_DBG( Dbg(), Message, wxString::Format( "push-sp depth=%d node=%p", st.m_node->Depth(), st.m_node ) );

    return true;
}


/*
 * Push or shove a via by at least aForce.  (The via might be pushed or shoved slightly further
 * to keep it from landing on an existing joint.)
 */
SHOVE::SHOVE_STATUS SHOVE::pushOrShoveVia( VIA* aVia, const VECTOR2I& aForce, int aNewRank, bool aDontUnwindStack )
{
    LINE_PAIR_VEC draggedLines;
    VECTOR2I p0( aVia->Pos() );
    const JOINT* jt = m_currentNode->FindJoint( p0, aVia );
    VECTOR2I p0_pushed( p0 + aForce );

    PNS_DBG( Dbg(), Message, wxString::Format( wxT( "via force [%d %d]\n" ), aForce.x, aForce.y ) );

    // nothing to do...
    if ( aForce.x == 0 && aForce.y == 0 )
        return SH_OK;

    if( !jt )
    {
        PNS_DBG( Dbg(), Message, wxT( "weird, can't find the center-of-via joint\n" ) );
        return SH_INCOMPLETE;
    }

    if( Settings().ShoveVias() == false || aVia->IsLocked() )
        return SH_TRY_WALK;

    if( jt->IsLocked() )
        return SH_INCOMPLETE;

    // make sure pushed via does not overlap with any existing joint
    while( true )
    {
        const JOINT* jt_next = m_currentNode->FindJoint( p0_pushed, aVia );

        if( !jt_next )
            break;

        p0_pushed += aForce.Resize( 2 );
    }

    std::unique_ptr<VIA> pushedVia = Clone( *aVia );
    pushedVia->SetPos( p0_pushed );
    pushedVia->Mark( aVia->Marker() );

    for( ITEM* item : jt->LinkList() )
    {
        if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
        {
            LINKED_ITEM* li = static_cast<LINKED_ITEM*>( item );
            LINE_PAIR lp;
            int segIndex;

            lp.first = assembleLine( li, &segIndex );

            if( lp.first.HasLockedSegments() )
                return SH_TRY_WALK;

            assert( segIndex == 0 || ( segIndex == ( lp.first.SegmentCount() - 1 ) ) );

            if( segIndex == 0 )
                lp.first.Reverse();

            lp.second = lp.first;
            lp.second.ClearLinks();
            lp.second.DragCorner( p0_pushed, lp.second.CLine().Find( p0 ) );
            lp.second.Line().Simplify2();
            draggedLines.push_back( std::move( lp ) );
        }
    }

    pushedVia->SetRank( aNewRank );
    PNS_DBG( Dbg(), Message, wxString::Format("via rank %d, fanout %d\n", pushedVia->Rank(), (int) draggedLines.size() ) );

    PNS_DBG( Dbg(), AddPoint, aVia->Pos(), LIGHTGREEN, 100000, "via-pre" );
    PNS_DBG( Dbg(), AddPoint, pushedVia->Pos(), LIGHTRED, 100000, "via-post" );

    VIA *v2 = pushedVia.get();

    if( !aDontUnwindStack )
        unwindLineStack( aVia );

    replaceItems( aVia, std::move( pushedVia ) );

    if( draggedLines.empty() ) // stitching via? make sure the router won't forget about it
    {
        LINE tmpLine;
        tmpLine.LinkVia( v2 );
        if( !pushLineStack( tmpLine ) )
            return SH_INCOMPLETE;
    }

    int n = 0;
    for( LINE_PAIR lp : draggedLines )
    {
        if( !aDontUnwindStack )
            unwindLineStack( &lp.first );

        PNS_DBG( Dbg(), Message, wxString::Format("fan %d/%d\n", n, (int) draggedLines.size() ) );
        n++;

        if( lp.second.SegmentCount() )
        {
            lp.second.ClearLinks();
            ROOT_LINE_ENTRY* rootEntry = replaceLine( lp.first, lp.second, true, true );

            lp.second.LinkVia( v2 );

            if( !aDontUnwindStack )
                unwindLineStack( &lp.second );

            lp.second.SetRank( aNewRank );

            if( rootEntry )
                rootEntry->newLine = lp.second; // fixme: it's inelegant


            PNS_DBG( Dbg(), Message, wxString::Format("PushViaF %p %d eov %d\n", &lp.second, lp.second.SegmentCount(), lp.second.EndsWithVia()?1:0 ) );

            if( !pushLineStack( lp.second ) ) //, true ) ) // WHY?
                return SH_INCOMPLETE;
        }
        else
        {
            m_currentNode->Remove( lp.first );
        }

        PNS_DBG( Dbg(), AddItem, &lp.first, LIGHTGREEN, 10000, wxT( "fan-pre" ) );
        PNS_DBG( Dbg(), AddItem, &lp.second, LIGHTRED, 10000, wxT( "fan-post" ) );
    }

    return SH_OK;
}


/*
 * Calculate the minimum translation vector required to resolve a collision with a via and
 * shove the via by that distance.
 */
SHOVE::SHOVE_STATUS SHOVE::onCollidingVia( ITEM* aCurrent, VIA* aObstacleVia, OBSTACLE& aObstacleInfo, int aNextRank )
{
    assert( aObstacleVia );

    int clearance = getClearance( aCurrent, aObstacleVia );
    VECTOR2I mtv;
    int rank = -1;

    bool lineCollision = false;
    bool viaCollision = false;
    bool solidCollision = false;
    VECTOR2I mtvLine, mtvVia, mtvSolid;

    PNS_DBG( Dbg(), BeginGroup, "push-via-by-line", 1 );

    if( aCurrent->OfKind( ITEM::LINE_T ) )
    {
        VIA vtmp ( *aObstacleVia );
        int layer = aCurrent->Layer();

        if( aObstacleInfo.m_maxFanoutWidth > 0
            && aObstacleInfo.m_maxFanoutWidth > aObstacleVia->Diameter( layer ) )
        {
            vtmp.SetDiameter( layer, aObstacleInfo.m_maxFanoutWidth );
        }

        LINE* currentLine = (LINE*) aCurrent;

        PNS_DBG( Dbg(), AddItem, currentLine, LIGHTRED, 10000, wxT( "current-line" ) );

        if( currentLine->EndsWithVia() )
        {
            PNS_DBG( Dbg(), AddItem, &currentLine->Via(), LIGHTRED, 10000, wxT( "current-line-via" ) );
        }

        PNS_DBG( Dbg(), AddItem, vtmp.Clone(), LIGHTRED, 100000, wxT( "orig-via" ) );

        lineCollision = vtmp.Shape( layer )->Collide( currentLine->Shape( -1 ),
                                                        clearance + currentLine->Width() / 2,
                                                        &mtvLine );

        // Check the via if present. Via takes priority.
        if( currentLine->EndsWithVia() )
        {
            const VIA& currentVia = currentLine->Via();
            int        viaClearance = getClearance( &currentVia, &vtmp );
            VECTOR2I   layerMtv;

            for( int viaLayer : currentVia.RelevantShapeLayers( &vtmp ) )
            {
                viaCollision |= currentVia.Shape( viaLayer )->Collide( vtmp.Shape( viaLayer ),
                                                                       viaClearance,
                                                                       &layerMtv );

                if( layerMtv.SquaredEuclideanNorm() > mtvVia.SquaredEuclideanNorm() )
                    mtvVia = layerMtv;
            }
        }
    }
    else if( aCurrent->OfKind( ITEM::SOLID_T ) )
    {
        PNS_DBG( Dbg(), Message, wxT("collidee-is-solid" ) );
        // TODO(JE) if this case is real, handle via stacks
        solidCollision = aCurrent->Shape( -1 )->Collide( aObstacleVia->Shape( -1 ), clearance,
                                                        &mtvSolid );
        //PNS_DBGN( Dbg(), EndGroup );

       // TODO: is this possible at all? We don't shove solids.
        //return SH_INCOMPLETE;
    }

    // fixme: we may have a sign issue in Collide(CIRCLE, LINE_CHAIN)
    if( viaCollision )
        mtv = -mtvVia;
    else if ( lineCollision )
        mtv = -mtvLine;
    else if ( solidCollision )
        mtv = -mtvSolid;
    else
        mtv = VECTOR2I(0, 0);

    SHOVE::SHOVE_STATUS st = pushOrShoveVia( aObstacleVia, -mtv, aNextRank );
    PNS_DBG( Dbg(), Message, wxString::Format("push-or-shove-via st %d", st ) );

    PNS_DBGN( Dbg(), EndGroup );

    return st;
}


/*
 * TODO describe....
 */
SHOVE::SHOVE_STATUS SHOVE::onReverseCollidingVia( LINE& aCurrent, VIA* aObstacleVia, OBSTACLE& aObstacleInfo )
{
    int n = 0;

    if( aCurrent.EndsWithVia() )
    {
        const VECTOR2I p0 = aCurrent.Via().Pos();
        const VECTOR2I p1 = aObstacleVia->Pos();

        int layer = aCurrent.Layer();
        int dist = (p0 - p1).EuclideanNorm() - aCurrent.Via().Diameter( layer ) / 2
                   - aObstacleVia->Diameter( layer ) / 2;

        int clearance = getClearance( &aCurrent.Via(), aObstacleVia );

        const SHAPE_LINE_CHAIN& hull = m_currentNode->GetRuleResolver()->HullCache(
                aObstacleVia, clearance, aCurrent.Width(), layer );

        bool epInsideHull = hull.PointInside( p0 );

        PNS_DBG( Dbg(), AddShape, &hull, LIGHTYELLOW,   100000, wxT( "obstacle-via-hull" ) );
        PNS_DBG( Dbg(), Message, wxString::Format("via2via coll check dist %d cl %d delta %d pi %d\n", dist, clearance, dist - clearance, epInsideHull ? 1 : 0) );

        bool viaCollision = false;

        for( int viaLayer : aCurrent.Via().RelevantShapeLayers( aObstacleVia ) )
        {
            viaCollision |=
                    aCurrent.Via().Shape( viaLayer )->Collide( aObstacleVia->Shape( viaLayer ),
                                                               clearance );
        }

        if( viaCollision )
        {
            return onCollidingVia( &aCurrent, aObstacleVia, aObstacleInfo, aCurrent.Rank() - 1 );
        }
    }

    LINE cur( aCurrent );
    cur.ClearLinks();

    const JOINT* jt = m_currentNode->FindJoint( aObstacleVia->Pos(), aObstacleVia );
    LINE shoved( aCurrent );
    shoved.ClearLinks();

    cur.RemoveVia();
    unwindLineStack( &aCurrent );

    for( ITEM* item : jt->LinkList() )
    {
        if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) && item->LayersOverlap( &aCurrent ) )
        {
            LINKED_ITEM* li = static_cast<LINKED_ITEM*>( item );
            LINE head = assembleLine( li );

            head.AppendVia( *aObstacleVia );

            bool shoveOK = ShoveObstacleLine( head, cur, shoved );

            if( !shoveOK )
            {
                PNS_DBG( Dbg(), BeginGroup, "on-reverse-via-fail-shove", m_iter );
                PNS_DBG( Dbg(), AddItem, aObstacleVia, LIGHTRED,   100000, wxT( "obstacle-via" ) );
                PNS_DBG( Dbg(), AddItem, &aCurrent,    LIGHTGREEN, 10000,  wxT( "current-line" ) );
                PNS_DBG( Dbg(), AddItem, &shoved,      LIGHTRED,   10000,  wxT( "shoved-line" ) );

                if( aCurrent.EndsWithVia() )
                {
                    PNS_DBG( Dbg(), AddItem, &aCurrent.Via(),    LIGHTGREEN, 100000,  wxT( "current-line-via" ) );
                }

                PNS_DBGN( Dbg(), EndGroup );

                return SH_INCOMPLETE;
            }

            cur.SetShape( shoved.CLine() );
            n++;
        }
    }

    if( !n )
    {
        PNS_DBG( Dbg(), BeginGroup, "on-reverse-via-fail-lonevia", m_iter );
        PNS_DBG( Dbg(), AddItem, aObstacleVia, LIGHTRED, 100000, wxT( "the-via" ) );
        PNS_DBG( Dbg(), AddItem, &aCurrent, LIGHTGREEN, 10000, wxT( "current-line" ) );
        PNS_DBGN( Dbg(), EndGroup );

        LINE head( aCurrent );
        head.Line().Clear();
        head.AppendVia( *aObstacleVia );
        head.ClearLinks();

        bool shoveOK = ShoveObstacleLine( head, aCurrent, shoved );

        if( !shoveOK )
            return SH_INCOMPLETE;

        cur.SetShape( shoved.CLine() );
    }

    if( aCurrent.EndsWithVia() )
        shoved.AppendVia( aCurrent.Via() );

    PNS_DBG( Dbg(), BeginGroup, "on-reverse-via", m_iter );
    PNS_DBG( Dbg(), AddItem, aObstacleVia, YELLOW, 0, wxT( "rr-the-via" ) );
    PNS_DBG( Dbg(), AddItem, &aCurrent, BLUE, 0, wxT( "rr-current-line" ) );
    PNS_DBG( Dbg(), AddItem, &shoved, GREEN, 0, wxT( "rr-shoved-line" ) );
    PNS_DBGN( Dbg(), EndGroup );

    int currentRank = aCurrent.Rank();
    unwindLineStack( &aCurrent );
    replaceLine( aCurrent, shoved, true, false );

    if( !pushLineStack( shoved ) )
        return SH_INCOMPLETE;

    shoved.SetRank( currentRank );

    return SH_OK;
}


void SHOVE::unwindLineStack( const LINKED_ITEM* aSeg )
{
    int d = 0;

    for( std::vector<LINE>::iterator i = m_lineStack.begin(); i != m_lineStack.end() ; )
    {
        if( i->ContainsLink( aSeg ) )
        {

// note to my future self: if we have a "tadpole" in the stack, keep track of the via even if the parent line has been deleted.
// otherwise - the via will be ignored in the case of collisions with tracks on another layer. Can happen pretty often in densely packed PCBs.
            if( i->EndsWithVia() && !aSeg->OfKind( ITEM::VIA_T ) )
            {
                VIA* via = nullptr;

                for( LINKED_ITEM* l : i->Links() )
                {
                    if( l->OfKind( ITEM::VIA_T ) )
                    {
                        via = static_cast<VIA*>( l );
                    }
                }

                if( via )
                {
                    i->ClearLinks();
                    i->Line().Clear();
                    i->LinkVia( via );
                }
                i++;
            }
            else
            {
                i = m_lineStack.erase( i );
            }
        }
        else
        {
            i++;
        }

        d++;
    }

    for( std::vector<LINE>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end() ; )
    {
        if( i->ContainsLink( aSeg ) && !aSeg->OfKind( ITEM::VIA_T ) )
            i = m_optimizerQueue.erase( i );
        else
            i++;
    }
}


void SHOVE::unwindLineStack( const ITEM* aItem )
{
    if( aItem->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) )
    {
        unwindLineStack( static_cast<const LINKED_ITEM*>( aItem ) );
    }
    else if( aItem->OfKind( ITEM::LINE_T ) )
    {
        const LINE* l = static_cast<const LINE*>( aItem );

        for( const LINKED_ITEM* seg : l->Links() )
            unwindLineStack( seg );
    }
}


bool SHOVE::pushLineStack( const LINE& aL, bool aKeepCurrentOnTop )
{
    if( !aL.IsLinked() && aL.SegmentCount() != 0 )
    {
        PNS_DBG( Dbg(), AddItem, &aL, BLUE, 10000, wxT( "push line stack failed" ) );

        return false;
    }

    if( aKeepCurrentOnTop && m_lineStack.size() > 0)
    {
        m_lineStack.insert( m_lineStack.begin() + m_lineStack.size() - 1, aL );
    }
    else
    {
        m_lineStack.push_back( aL );
    }


    pruneLineFromOptimizerQueue( aL );
    m_optimizerQueue.push_back( aL );

    return true;
}


bool SHOVE::pruneLineFromOptimizerQueue( const LINE& aLine )
{
    int pre = m_optimizerQueue.size();
    for( std::vector<LINE>::iterator i = m_optimizerQueue.begin(); i != m_optimizerQueue.end(); )
    {
        bool found = false;

        for( LINKED_ITEM* s : aLine.Links() )
        {
            PNS_DBG( Dbg(), Message,
                     wxString::Format( "cur lc %d lnk %p cnt %d", i->LinkCount(), s, aLine.LinkCount() ) );

            if( i->ContainsLink( s ) && !s->OfKind( ITEM::VIA_T ) )
            {

                i = m_optimizerQueue.erase( i );
                found = true;
                break;
            }
        }

        if( !found )
            i++;
    }

    return true;
}

void SHOVE::popLineStack( )
{
    LINE& l = m_lineStack.back();
    pruneLineFromOptimizerQueue( l );
    m_lineStack.pop_back();
}


bool SHOVE::fixupViaCollisions( const LINE* aCurrent, OBSTACLE& obs )
{
    int layer = aCurrent->Layer();

    // if the current obstacle is a via, consider also the lines connected to it
    // if their widths are larger or equal than the via diameter, the shove algorithm
    // will very likely fail in the subsequent iterations (as our base assumption is track
    // ends can never move on their own, only dragged by force-propagated vias

    // our colliding item is a via: just find the max width of the traces connected to it
    if( obs.m_item->OfKind( ITEM::VIA_T ) )
    {
        const VIA*   v = static_cast<const VIA*>( obs.m_item );
        int          maxw = 0;
        const JOINT* jv = m_currentNode->FindJoint( v->Pos(), v );

        ITEM_SET links( jv->CLinks() );

        for( ITEM* link : links )
        {
            if( link->OfKind( ITEM::SEGMENT_T ) ) // consider segments ...
            {
                const SEGMENT* seg = static_cast<const SEGMENT*>( link );
                maxw = std::max( seg->Width(), maxw );
            }
            else if( link->OfKind( ITEM::ARC_T ) ) // ... or arcs
            {
                const ARC* arc = static_cast<const ARC*>( link );
                maxw = std::max( arc->Width(), maxw );
            }
        }

        obs.m_maxFanoutWidth = 0;

        if( maxw > 0 && maxw >= v->Diameter( layer ) )
        {
            obs.m_maxFanoutWidth = maxw + 1;
            PNS_DBG( Dbg(), Message,
                     wxString::Format( "Fixup via: new-w %d via-w %d", maxw, v->Diameter( layer ) ) );

            return true;
        }
        return false;
    }


    // our colliding item is a segment. check if it has a via on either of the ends.
    if( !obs.m_item->OfKind( ITEM::SEGMENT_T ) )
        return false;

    const SEGMENT* s = static_cast<const SEGMENT*>( obs.m_item );
    int sl = s->Layer();

    const JOINT* ja = m_currentNode->FindJoint( s->Seg().A, s );
    const JOINT* jb = m_currentNode->FindJoint( s->Seg().B, s );

    VIA* vias[] = { ja->Via(), jb->Via() };

    for( int i = 0; i < 2; i++ )
    {
        VIA* v = vias[i];

        // via diameter is larger than the segment width - cool, the force propagation algo
        // will be able to deal with it, no need to intervene
        if( !v || v->Diameter( sl ) > s->Width() )
            continue;

        VIA vtest( *v );
        vtest.SetDiameter( sl, s->Width() );

        // enlarge the via to the width of the segment
        if( vtest.Collide( aCurrent, m_currentNode, aCurrent->Layer() ) )
        {
            // if colliding, drop the segment in the shove iteration loop and force-propagate the via instead
            obs.m_item = v;
            obs.m_maxFanoutWidth = s->Width() + 1;
            return true;
        }
    }
    return false;
}

bool SHOVE::patchTadpoleVia( ITEM* nearest, LINE& current )
{
    if (current.CLine().PointCount() < 1 )
        return false;

//    PNS_DBG(Dbg(), Message, wxString::Format( "cp %d %d", current.CLine().CLastPoint().x, current.CLine().CLastPoint().y ) );

    auto jtViaEnd = m_currentNode->FindJoint( current.CLine().CLastPoint(), &current );

//    PNS_DBG(Dbg(), Message, wxString::Format( "jt %p",  jtViaEnd ) );

    if ( !jtViaEnd )
        return false;

    auto viaEnd = jtViaEnd->Via();

    if (! viaEnd )
        return false;

    bool colliding = m_currentNode->CheckColliding( viaEnd ).has_value();

//    PNS_DBG(Dbg(), Message, wxString::Format( "patch-tadpole viaEnd %p colliding %d", viaEnd, colliding?1:0 ) );

    if( viaEnd && !current.EndsWithVia() && colliding )
    {
        current.LinkVia( viaEnd );
    }

    return false;
}

/*
 * Resolve the next collision.
 */
SHOVE::SHOVE_STATUS SHOVE::shoveIteration( int aIter )
{
    LINE currentLine = m_lineStack.back();
    NODE::OPT_OBSTACLE nearest;
    SHOVE_STATUS st = SH_NULL;

    ROUTER_IFACE* iface = Router()->GetInterface();

    if( Dbg() )
        Dbg()->SetIteration( aIter );

    PNS_DBG( Dbg(), AddItem, &currentLine, RED, currentLine.Width(),
             wxString::Format( wxT( "current sc=%d net=%s evia=%d" ),
             currentLine.SegmentCount(),
             iface->GetNetName( currentLine.Net() ),
             currentLine.EndsWithVia() ? 1 : 0 ) );

    for( ITEM::PnsKind search_order : { ITEM::SOLID_T, ITEM::VIA_T, ITEM::SEGMENT_T, ITEM::HOLE_T } )
    {
        COLLISION_SEARCH_OPTIONS opts;
        opts.m_kindMask = search_order;
        opts.m_filter = [ this ] ( const ITEM* item ) -> bool
        {
            bool rv = true;

            if( item->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T | ITEM::VIA_T | ITEM::SOLID_T | ITEM::HOLE_T ) )
            {
                const LINKED_ITEM* litem = static_cast<const LINKED_ITEM*>( item );
                ROOT_LINE_ENTRY* ent = this->findRootLine( litem );

                if( !ent && m_defaultPolicy & SHP_IGNORE )
                    rv = false;

                if( ent && ent->policy & SHP_IGNORE )
                    rv = false;
            }
            else
            {
                if( m_defaultPolicy & SHP_IGNORE )
                    rv = false;
            }

            return rv;
        };

        nearest = m_currentNode->NearestObstacle( &currentLine, opts );

         if( nearest )
         {
            PNS_DBG( Dbg(), AddShape, nearest->m_item->Shape( currentLine.Layer() ), YELLOW, 10000,
            wxString::Format( "nearest %p %s rank %d",
                                                        nearest->m_item,
                                                        nearest->m_item->KindStr(),
                                                        nearest->m_item->Rank() ) );
         }

         if( nearest )
            break;
    }

    if( !nearest )
    {
        m_lineStack.pop_back();
        PNS_DBG( Dbg(), Message, wxT( "no-nearest-item ") );
        return SH_OK;
    }

    bool viaFixup = fixupViaCollisions( &currentLine, *nearest );

    PNS_DBG( Dbg(), Message, wxString::Format( wxT( "iter %d: via-fixup %d" ), aIter, viaFixup?1:0 ) );


    ITEM* ni = nearest->m_item;

    UNITS_PROVIDER up( pcbIUScale, EDA_UNITS::MM );
    PNS_DBG( Dbg(), Message, wxString::Format( wxT( "NI: %s (%s) %p %d" ),
                                               ni->Format(),
                                               ni->Parent() ? ni->Parent()->GetItemDescription( &up, false )
                                                            : wxString( wxT( "null" ) ),
                                               ni,
                                               ni->OwningNode()->Depth() ) );

    unwindLineStack( ni );

    if( !ni->OfKind( ITEM::SOLID_T ) && ni->Rank() >= 0 && ni->Rank() > currentLine.Rank() )
    {
        // Collision with a higher-ranking object (ie: one that we've already shoved)
        //
        switch( ni->Kind() )
        {
        case ITEM::VIA_T:
        {
            PNS_DBG( Dbg(), BeginGroup, wxString::Format( wxT( "iter %d: reverse-collide-via" ), aIter ), 0 );

            patchTadpoleVia( ni, currentLine );

            // TODO(JE) viastacks -- via-via collisions here?
            if( currentLine.EndsWithVia()
                && nearest->m_item->Collide( &currentLine.Via(), m_currentNode,
                                             nearest->m_item->Layer() ) )
            {
                PNS_DBG( Dbg(), AddItem, nearest->m_item, YELLOW, 100000, wxT("v2v nearesti" ) );
                //PNS_DBG( Dbg(), AddItem, nearest->m_head,RED, 100000, wxString::Format("v2v nearesth force=%d,%d" ) );

                st = onCollidingVia( &currentLine, (VIA*) ni, *nearest, ni->Rank() + 1 );

                //ni->SetRank( currentLine.Rank() );
            }
            else
            {
                st = onReverseCollidingVia( currentLine, (VIA*) ni, *nearest );
            }

            PNS_DBGN( Dbg(), EndGroup );

            break;
        }

        case ITEM::SEGMENT_T:
        {
            PNS_DBG( Dbg(), BeginGroup, wxString::Format( wxT( "iter %d: reverse-collide-segment" ),
                                                          aIter ), 0 );

            PNS_DBG( Dbg(), AddItem, ni, YELLOW, 100000, wxT("head" ) );

            LINE revLine = assembleLine( static_cast<SEGMENT*>( ni ) );

            popLineStack();
            unwindLineStack( &revLine );
            patchTadpoleVia( ni, currentLine );

            if( currentLine.EndsWithVia()
                && currentLine.Via().Collide( (SEGMENT*) ni, m_currentNode, currentLine.Layer() ) )
            {
                VIA_HANDLE vh;
                vh.layers = currentLine.Via().Layers();
                vh.net =  currentLine.Via().Net();
                vh.pos =  currentLine.Via().Pos();
                vh.valid = true;
                auto rvia = m_currentNode->FindViaByHandle( vh );
                if ( !rvia )
                    st = SH_INCOMPLETE;
                else
                    st = onCollidingVia( &revLine, rvia, *nearest, revLine.Rank() + 1 );
            }
            else
                st = onCollidingLine( revLine, currentLine, revLine.Rank() + 1 );


            if( !pushLineStack( revLine ) )
            {
                return SH_INCOMPLETE;
            }


            PNS_DBGN( Dbg(), EndGroup );

            break;
        }

        case ITEM::ARC_T:
        {
            //TODO(snh): Handle Arc shove separate from track
            PNS_DBG( Dbg(), BeginGroup, wxString::Format( wxT( "iter %d: reverse-collide-arc " ), aIter ), 0 );
            LINE revLine = assembleLine( static_cast<ARC*>( ni ) );

            popLineStack();
            st = onCollidingLine( revLine, currentLine, revLine.Rank() - 1 );

            PNS_DBGN( Dbg(), EndGroup );

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
            PNS_DBG( Dbg(), BeginGroup, wxString::Format( wxT( "iter %d: collide-segment " ), aIter ), 0 );

            st = onCollidingSegment( currentLine, (SEGMENT*) ni );

            if( st == SH_TRY_WALK )
                st = onCollidingSolid( currentLine, ni, *nearest );

            PNS_DBGN( Dbg(), EndGroup );

            break;

            //TODO(snh): Customize Arc collide
        case ITEM::ARC_T:
            PNS_DBG( Dbg(), BeginGroup, wxString::Format( wxT( "iter %d: collide-arc " ), aIter ), 0 );

            st = onCollidingArc( currentLine, static_cast<ARC*>( ni ) );

            if( st == SH_TRY_WALK )
                st = onCollidingSolid( currentLine, ni, *nearest );

            PNS_DBGN( Dbg(), EndGroup );

            break;

        case ITEM::VIA_T:
            PNS_DBG( Dbg(), BeginGroup, wxString::Format( wxT( "iter %d: collide-via (fixup: %d)" ), aIter, 0 ), 0 );
            st = onCollidingVia( &currentLine, (VIA*) ni, *nearest, currentLine.Rank() - 1 );

            if( st == SH_TRY_WALK )
                st = onCollidingSolid( currentLine, ni, *nearest );

            PNS_DBGN( Dbg(), EndGroup );

            break;

        case ITEM::HOLE_T:
        case ITEM::SOLID_T:
            PNS_DBG( Dbg(), BeginGroup, wxString::Format( wxT( "iter %d: walk-solid " ), aIter ), 0);
            st = onCollidingSolid( currentLine, ni, *nearest );

            PNS_DBGN( Dbg(), EndGroup );

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

    PNS_DBG( Dbg(), Message, wxString::Format( "ShoveStart [root: %d jts, current: %d jts]",
                                               m_root->JointCount(),
                                               m_currentNode->JointCount() ) );

    int iterLimit = Settings().ShoveIterationLimit();
    TIME_LIMIT timeLimit = Settings().ShoveTimeLimit();

    m_iter = 0;

    timeLimit.Restart();

    if( m_lineStack.empty() && m_draggedVia )
    {
        // If we're shoving a free via then push a proxy LINE (with the via on the end) onto
        // the stack.
        pushLineStack( LINE( m_draggedVia ) );
    }

    while( !m_lineStack.empty() )
    {
        PNS_DBG( Dbg(), Message, wxString::Format( "iter %d: node %p stack %d ", m_iter,
                                                   m_currentNode, (int) m_lineStack.size() ) );

        st = shoveIteration( m_iter );

        m_iter++;

        if( st == SH_INCOMPLETE || timeLimit.Expired() || m_iter >= iterLimit )
        {
            PNS_DBG( Dbg(), Message, wxString::Format( "Fail [time limit expired: %d iter %d iter limit %d",
                                               timeLimit.Expired()?1:0, m_iter, iterLimit ) );
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


SHOVE::ROOT_LINE_ENTRY* SHOVE::findRootLine( const LINE& aLine )
{
        for( const LINKED_ITEM* link : aLine.Links() )
        {
                auto it = m_rootLineHistory.find( link->Uid() );

                if( it != m_rootLineHistory.end() )
                    return it->second;
        }

        return nullptr;
}

SHOVE::ROOT_LINE_ENTRY* SHOVE::findRootLine( const LINKED_ITEM *aItem )
{
        auto it = m_rootLineHistory.find( aItem->Uid() );

        if( it != m_rootLineHistory.end() )
            return it->second;

    return nullptr;
}


SHOVE::ROOT_LINE_ENTRY* SHOVE::touchRootLine( const LINE& aLine )
{
    for( const LINKED_ITEM* link : aLine.Links() )
    {
        auto it = m_rootLineHistory.find( link->Uid() );

        if( it != m_rootLineHistory.end() )
        {
            PNS_DBG( Dbg(), Message, wxString::Format( wxT( "touch [found] uid=%llu type=%s"), link->Uid(), link->KindStr() ) );

            return it->second;
        }
    }

    auto rootEntry = new ROOT_LINE_ENTRY( aLine.Clone() );


    for( const LINKED_ITEM* link : aLine.Links() )
    {
        PNS_DBG( Dbg(), Message, wxString::Format( wxT( "touch [create] uid=%llu type=%s"), link->Uid(), link->KindStr() ) );
        m_rootLineHistory[link->Uid()] = rootEntry;
    }


    return rootEntry;
}


SHOVE::ROOT_LINE_ENTRY* SHOVE::touchRootLine( const LINKED_ITEM* aItem )
{
    auto it = m_rootLineHistory.find( aItem->Uid() );

    if( it != m_rootLineHistory.end() )
    {
        PNS_DBG( Dbg(), Message, wxString::Format( wxT( "touch [create] uid=%llu"), aItem->Uid() ) );
        return it->second;
    }

    auto rootEntry = new ROOT_LINE_ENTRY( nullptr );

    PNS_DBG( Dbg(), Message, wxString::Format( wxT( "touch [create] uid=%llu"), aItem->Uid() ) );
    m_rootLineHistory[ aItem->Uid() ] = rootEntry;

    return rootEntry;
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
    {
        area->Inflate( maxWidth );
        area = area->Intersect( VisibleViewArea() );
    }

    switch( effort )
    {
    case OE_LOW:
        optFlags |= OPTIMIZER::MERGE_OBTUSE;
        n_passes = 1;
        break;

    case OE_MEDIUM:
        optFlags |= OPTIMIZER::MERGE_SEGMENTS;
        n_passes = 2;
        break;

    case OE_FULL:
        optFlags = OPTIMIZER::MERGE_SEGMENTS;
        n_passes = 2;
        break;

    default:
        break;
    }

    optFlags |= OPTIMIZER::LIMIT_CORNER_COUNT;

    if( area )
    {
        SHAPE_RECT r( *area );

        PNS_DBG( Dbg(), AddShape, &r, BLUE, 0, wxT( "opt-area" ) );

        optFlags |= OPTIMIZER::RESTRICT_AREA;
        optimizer.SetRestrictArea( *area, false );
    }

    DIRECTION_45::CORNER_MODE cornerMode = Settings().GetCornerMode();

    // Smart Pads is incompatible with 90-degree mode for now
    if( Settings().SmartPads()
            && ( cornerMode == DIRECTION_45::MITERED_45 || cornerMode == DIRECTION_45::ROUNDED_45 ) )
    {
        optFlags |= OPTIMIZER::SMART_PADS;
    }


    optimizer.SetEffortLevel( optFlags & ~m_optFlagDisableMask );
    optimizer.SetCollisionMask( ITEM::ANY_T );

    std::set<const ITEM*> itemsChk;

    auto iface = Router()->GetInterface();

    for( int pass = 0; pass < n_passes; pass++ )
    {
        std::reverse( m_optimizerQueue.begin(), m_optimizerQueue.end() );

        PNS_DBG( Dbg(), Message, wxString::Format( wxT( "optimize %d lines, pass %d"), (int)m_optimizerQueue.size(), (int)pass ) );

        for( int i = 0; i < m_optimizerQueue.size(); i++ )
        {
            LINE& lineToOpt = m_optimizerQueue[i];
            LINE* rootLine = nullptr;
            auto rootEntry = findRootLine( lineToOpt );

            if( rootEntry )
            {
                rootLine = rootEntry->rootLine;

                if( rootEntry->policy & SHP_DONT_OPTIMIZE )
                    continue;
                if( rootEntry->isHead )
                    continue;
            }

            LINE optimized;
            if( optimizer.Optimize( &lineToOpt, &optimized, rootLine ) )
            {
                assert( optimized.LinkCount() == 0 );

                //PNS_DBG( Dbg(), AddShape, &lineToOpt.CLine(), BLUE, 0, wxT( "shove-pre-opt" ) );
                //if( rootLine )
                  //  PNS_DBG( Dbg(), AddItem, rootLine, RED, 0, wxT( "shove-root-opt" ) );

                replaceLine( lineToOpt, optimized, false, aNode );
                m_optimizerQueue[i] = std::move( optimized ); // keep links in the lines in the queue up to date

                //PNS_DBG( Dbg(), AddShape, &optimized.CLine(), GREEN, 0, wxT( "shove-post-opt" ) );
            }
        }
    }
}


NODE* SHOVE::CurrentNode()
{
    return m_currentNode ? m_currentNode : m_root; //m_nodeStack.empty() ? m_root : m_nodeStack.back().m_node;
}


bool SHOVE::AddLockedSpringbackNode( NODE* aNode )
{
    SPRINGBACK_TAG sp;
    sp.m_node = aNode;
    sp.m_locked = true;

    m_nodeStack.push_back(sp);

    PNS_DBG( Dbg(), Message, wxString::Format( "addLockedSPNode node=%p stack=%d\n", sp.m_node, (int) m_nodeStack.size() ) );

    return true;
}


bool SHOVE::RewindSpringbackTo( NODE* aNode )
{
    bool found = false;

    auto iter = m_nodeStack.begin();

    while( iter != m_nodeStack.end() )
    {
        if ( iter->m_node == aNode )
        {
            found = true;
            break;
        }

        iter++;
    }

    if( !found )
        return false;

    auto start = iter;

    aNode->KillChildren();
    m_nodeStack.erase( start, m_nodeStack.end() );

    if( !m_nodeStack.empty() )
        m_currentNode = m_nodeStack.back().m_node;
    else
        m_currentNode = m_root;

    return true;
}


bool SHOVE::RewindToLastLockedNode()
{
    if( m_nodeStack.empty() )
        return false;

    while( !m_nodeStack.back().m_locked && m_nodeStack.size() > 1 )
        m_nodeStack.pop_back();

    m_currentNode = m_nodeStack.back().m_node;

    return m_nodeStack.back().m_locked;
}


void SHOVE::UnlockSpringbackNode( NODE* aNode )
{
    auto iter = m_nodeStack.begin();

    while( iter != m_nodeStack.end() )
    {
        if ( iter->m_node == aNode )
        {
            iter->m_locked = false;
            break;
        }

        iter++;
    }
}


void SHOVE::DisablePostShoveOptimizations( int aMask )
{
    m_optFlagDisableMask = aMask;
}


void SHOVE::SetSpringbackDoNotTouchNode( const NODE *aNode )
{
    m_springbackDoNotTouchNode = aNode;
}


void SHOVE::SetDefaultShovePolicy( int aPolicy )
{
    m_defaultPolicy = aPolicy;
}


void SHOVE::SetShovePolicy( const LINKED_ITEM* aItem, int aPolicy )
{
    auto rl = touchRootLine( aItem );
    rl->policy = aPolicy;
}

void SHOVE::SetShovePolicy( const LINE& aLine, int aPolicy )
{
    auto rl = touchRootLine( aLine );
    rl->policy = aPolicy;
}


void SHOVE::ClearHeads()
{
    m_headLines.clear();
}


void SHOVE::AddHeads( const LINE& aHead,  int aPolicy )
{
    m_headLines.push_back( SHOVE::HEAD_LINE_ENTRY( aHead, aPolicy ) );
}


void SHOVE::AddHeads( VIA_HANDLE aHead, VECTOR2I aNewPos, int aPolicy )
{
    SHOVE::HEAD_LINE_ENTRY ent( aHead, aPolicy );
    ent.viaNewPos = aNewPos;
    ent.prevVia = aHead;
    ent.theVia = aHead;
    m_headLines.push_back( std::move( ent ) );
}

void removeHead( NODE *aNode, LINE& head )
{
    for (auto lnk : head.Links() )
    {
        if( lnk->BelongsTo( aNode ) )
            aNode->Remove( lnk );
    }
}

void SHOVE::removeHeads()
{
    auto iface = Router()->GetInterface();

    NODE::ITEM_VECTOR removed, added;

    m_currentNode->GetUpdatedItems( removed, added );

    for( auto& item : added )
    {
        auto rootEntry = findRootLine( static_cast<LINKED_ITEM*>( item ) );
        if( rootEntry && rootEntry->isHead )
        {
            m_currentNode->Remove( item );
        }
    }
}


void SHOVE::reconstructHeads( bool aShoveFailed )
{
    int  i = 0;
    auto iface = Router()->GetInterface();

    PNS_DBG( Dbg(), Message, wxString::Format("reconstructing %zu heads", m_headLines.size() ) );

    for( auto& headEntry : m_headLines )
    {
        if( headEntry.origHead )
        {
            auto rootEntry = findRootLine( *headEntry.origHead );

            PNS_DBG( Dbg(), Message, wxString::Format("orig LinkC=%d RE=%p", headEntry.origHead->LinkCount(), rootEntry ) );

            assert( rootEntry );
            assert( rootEntry->rootLine );

            if( rootEntry->newLine )
            {
                headEntry.newHead = rootEntry->newLine;
                headEntry.geometryModified = !rootEntry->newLine->CLine().CompareGeometry( rootEntry->rootLine->CLine() );

                wxString msg = wxString::Format(
                        "head %d/%d [net %-20s]: root %p, lc-root %d, lc-new %d\n", i, (int) m_headLines.size(),
                        iface->GetNetName( rootEntry->rootLine->Net() ).c_str().AsChar(), rootEntry->rootLine, rootEntry->rootLine->LinkCount(), headEntry.newHead->LinkCount() );
                PNS_DBG( Dbg(), AddItem, rootEntry->rootLine, CYAN, 0, msg );
                PNS_DBG( Dbg(), Message, msg );

            }
            else
            {
                wxString msg = wxString::Format(
                        "head %d/%d [net %-20s]: unmodified, lc-orig %d\n", i, (int) m_headLines.size(),
                        iface->GetNetName( headEntry.origHead->Net() ).c_str().AsChar(),
                        headEntry.origHead->LinkCount() );
                PNS_DBG( Dbg(), Message, msg );
            }

            i++;
        } else {
            auto rootEntry = findRootLine( headEntry.draggedVia );

            if( rootEntry->newVia )
            {
                headEntry.geometryModified = true;
                headEntry.theVia = VIA_HANDLE( rootEntry->newVia->Pos(), rootEntry->newVia->Layers(), rootEntry->newVia->Net() );
                auto chk = m_currentNode->FindViaByHandle( *headEntry.theVia );
                wxString msg = wxString::Format( "[modif] via orig %p chk %p\n", headEntry.draggedVia, chk );

                PNS_DBG( Dbg(), Message, msg );
                assert( chk != nullptr );
            }
            else
            {
                headEntry.theVia = VIA_HANDLE( rootEntry->oldVia->Pos(), rootEntry->oldVia->Layers(), rootEntry->oldVia->Net() );
                auto chk = m_currentNode->FindViaByHandle( *headEntry.theVia );
                wxString msg = wxString::Format( "[unmodif] via orig %p chk %p\n", headEntry.draggedVia, chk );
                PNS_DBG( Dbg(), Message, msg );
                assert( chk != nullptr );

            }


        }

        m_headsModified |= headEntry.geometryModified;
    }
}



bool SHOVE::preShoveCleanup( LINE* aOld, LINE* aNew )
{
    //COLLISION_SEARCH_CONTEXT ctx;

    //ctx.options.m_differentNetsOnly = false;
    //ctx.options.m_kindMask = ITEM::SEGMENT_T; // fixme arcs

    SHAPE_LINE_CHAIN orig( aOld->CLine() );

    int vc_prev = orig.PointCount();
    orig.Simplify2();
    int vc_post = orig.PointCount();

    *aNew = *aOld;

    PNS_DBG( Dbg(), Message, wxString::Format( "**** PreshoveCleanup %d -> %d\n", vc_prev, vc_post ) );

    if( vc_prev != vc_post )
    {
        aNew->ClearLinks();
        aNew->SetShape( orig );
        replaceLine( *aOld, *aNew );
        return true;
    }

    return false;
}

// new algo
SHOVE::SHOVE_STATUS SHOVE::Run()
{
    SHOVE_STATUS st = SH_OK;

    m_multiLineMode = false;
    int currentHeadId = 0;
    int totalHeads = m_headLines.size();

    m_headsModified = false;
    m_lineStack.clear();
    m_optimizerQueue.clear();

    ITEM_SET headSet;

    PNS_DBG( Dbg(), Message, wxString::Format("shove run (heads: %d, currentNode=%p, depth=%d)", (int) m_headLines.size(), m_currentNode, m_currentNode->Depth() ) );

    for( auto& l : m_headLines )
    {
        if( l.theVia )
        {
            PNS_DBG( Dbg(), Message, wxString::Format("process head-via [%d %d] node=%p", l.theVia->pos.x, l.theVia->pos.y, m_currentNode ) );
            auto realVia = m_currentNode->FindViaByHandle( *l.theVia );
            assert( realVia != nullptr );
            headSet.Add( realVia->Clone() );
        }
        else
        {
            headSet.Add( *l.origHead->Clone() );
        }
    }

    // Pop NODEs containing previous shoves which are no longer necessary
    NODE*      parent = reduceSpringback( headSet );
    m_currentNode = parent->Branch();
    m_currentNode->ClearRanks();






    //nodeStats( Dbg(), m_currentNode, "right-after-branch" );

    auto iface = Router()->GetInterface();

    //    for ( auto& hq : m_headLines )
    //      if( hq.oldHead )
    //        m_currentNode->Remove( *hq.oldHead );


    // Push the via to its new location
    for( auto& headLineEntry : m_headLines )
    {
        //if( rootEntry->line ) // head already processed in previous steps
        //{
        //  PNS_DBG( Dbg(), Message, wxString::Format( "RL found" ) );

        //continue;
        //}
        m_currentNode->ClearRanks();

        if( headLineEntry.theVia )
        {
            VIA* viaToDrag = m_currentNode->FindViaByHandle( *headLineEntry.theVia );

            if( !viaToDrag )
            {
                st = SH_INCOMPLETE;
                break;
            }

            auto viaRoot = touchRootLine( viaToDrag );
            viaRoot->oldVia = viaToDrag;
            headLineEntry.draggedVia = viaToDrag;

            st = pushOrShoveVia( viaToDrag, ( headLineEntry.viaNewPos - viaToDrag->Pos() ), 0, true );

            if( st != SH_OK )
                break;
        }
        else
        {
            // Create a new NODE to store this version of the world
            assert( headLineEntry.origHead->LinkCount() == 0 );
            m_currentNode->Add( *headLineEntry.origHead, true );

            //nodeStats( Dbg(), m_currentNode, "add-head" );



            PNS_DBG( Dbg(), Message,
                     wxString::Format( "touchRoot ohlc %d roots %d re=%p\n",
                                       headLineEntry.origHead->LinkCount(),
                                       (int) m_rootLineHistory.size(),
                                       findRootLine( *headLineEntry.origHead ) ) );


            LINE head( *headLineEntry.origHead );

            // empty head? nothing to shove...
            if( !head.SegmentCount() && !head.EndsWithVia() )
            {
                st = SH_INCOMPLETE;
                break;
            }

            currentHeadId++;

            if( !( headLineEntry.policy & SHP_DONT_LOCK_ENDPOINTS ) )
            {
		    if( head.PointCount() > 0 )
		        m_currentNode->LockJoint( head.CPoint( 0 ), &head, true );

		    if( !head.EndsWithVia() )
		        m_currentNode->LockJoint( head.CLastPoint(), &head, true );
            }

            SetShovePolicy( head, headLineEntry.policy );

            //head.Mark( MK_HEAD );
            head.SetRank( 100000 ); //- 100 * currentHeadId );

            if( head.EndsWithVia() )
            {
                auto headVia = Clone( head.Via() );
                headVia->SetRank( 100000 ); // - 100 * currentHeadId );
                headLineEntry.origHead->LinkVia( headVia.get() );
                head.LinkVia( headVia.get() );
                m_currentNode->Add( std::move( headVia ) );
            }

            auto headRoot = touchRootLine( *headLineEntry.origHead );
            headRoot->isHead = true;
            headRoot->rootLine = new PNS::LINE( *headLineEntry.origHead );
            headRoot->policy = headLineEntry.policy;
            if( head.EndsWithVia() )
            {
                m_rootLineHistory[ headLineEntry.origHead->Via().Uid() ] = headRoot;
            }


            PNS_DBG( Dbg(), Message,
                     wxString::Format( "headLC %d, rlLC %d oolc %d eov %d\n", head.LinkCount(),
                                       headRoot->rootLine->LinkCount(),
                                       headLineEntry.origHead->LinkCount(),
                                       head.EndsWithVia()?1:0 ) );

            //auto rootEntry = findRootLine( &head );

            PNS_DBG( Dbg(), Message,
                     wxString::Format( "Shove heads %d/%d h-lc=%d net=%s Line=%d Policy=%s",
                                       currentHeadId, totalHeads, head.LinkCount(),
                                       iface->GetNetName( head.Net() ), headRoot->newLine ? 1 : 0,
                                       headRoot ? formatPolicy( headRoot->policy )
                                                : wxString( wxT( "default[ne]" ) ) ) );


            //        nodeStats( Dbg(), m_currentNode, "pre-push-stack" );

            if( !pushLineStack( head ) )
            {
                st = SH_INCOMPLETE;
                break;
            }
        }

        st = shoveMainLoop();

        //nodeStats( Dbg(), m_currentNode, "post-main-loop" );

        if( st != SH_OK )
            break;
    };

   PNS_DBG( Dbg(), Message,
                 wxString::Format( "Shove status : %s after %d iterations, heads: %d",
                                   ( ( st == SH_OK || st == SH_HEAD_MODIFIED ) ? "OK" : "FAILURE" ),
                                   m_iter, (int) m_headLines.size() ) );
    if( st == SH_OK )
    {
        //nodeStats( Dbg(), m_currentNode, "pre-opt" );

        runOptimizer( m_currentNode );

        reconstructHeads( false );
        removeHeads();

        // this must be called afrter reconstructHeads as it requires up-to-date via handles
        pushSpringback( m_currentNode, m_affectedArea );
    }
    else
    {
        //reconstructHeads( true );

        for( auto& headEntry : m_headLines )
        {
            if( headEntry.prevVia )
            {

                 PNS_DBG( Dbg(), Message,
                 wxString::Format( "Fail-restore via mod [%d, %d] orig [%d, %d]",
                    headEntry.theVia->pos.x,
                    headEntry.theVia->pos.y,
                    headEntry.prevVia->pos.x,
                    headEntry.prevVia->pos.y ) );

                headEntry.theVia = headEntry.prevVia;
                headEntry.geometryModified = true;
                m_headsModified = true;
            }
        }

        pruneRootLines( m_currentNode );

        delete m_currentNode;
        m_currentNode = parent;
    }

    return st;
}

enum SHOVE_POLICY
    {
        SHP_DEFAULT = 0,
        SHP_SHOVE = 0x1,
        SHP_WALK_FORWARD = 0x2,
        SHP_WALK_BACK = 0x4,
        SHP_IGNORE = 0x8,
        SHP_DONT_OPTIMIZE = 0x10
    };

const wxString SHOVE::formatPolicy( int aPolicy )
{
    if( aPolicy == SHP_DEFAULT )
        return wxT( "default" );

    wxString rv;

    if( aPolicy & SHP_SHOVE )
        rv.Append( "shove ");
    if( aPolicy & SHP_WALK_FORWARD )
        rv.Append( "walk-forward ");
    if( aPolicy & SHP_WALK_FORWARD )
        rv.Append( "walk-back ");
    if( aPolicy & SHP_IGNORE )
        rv.Append( "ignore ");
    if( aPolicy & SHP_IGNORE )
        rv.Append( "dont-optimize ");

    return rv;
}

bool SHOVE::HeadsModified( int aIndex ) const
{
    if( aIndex < 0 )
        return m_headsModified;
    else
        return m_headLines[ aIndex ].geometryModified;
}

const PNS::LINE SHOVE::GetModifiedHead( int aIndex ) const
{
    return *m_headLines[ aIndex ].newHead;
}

const VIA_HANDLE SHOVE::GetModifiedHeadVia( int aIndex ) const
{
    return *m_headLines[ aIndex ].theVia;
}



}

