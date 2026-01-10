/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
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

#include <optional>
#include <memory>

#include "pns_arc.h"
#include "pns_debug_decorator.h"
#include "pns_line_placer.h"
#include "pns_node.h"
#include "pns_router.h"
#include "pns_shove.h"
#include "pns_solid.h"
#include "pns_topology.h"
#include "pns_walkaround.h"
#include "pns_mouse_trail_tracer.h"

#include <wx/log.h>

namespace PNS {

LINE_PLACER::LINE_PLACER( ROUTER* aRouter ) :
    PLACEMENT_ALGO( aRouter )
{
    m_initial_direction = DIRECTION_45::N;
    m_world = nullptr;
    m_shove = nullptr;
    m_currentNode = nullptr;
    m_idle = true;

    // Init temporary variables (do not leave uninitialized members)
    m_lastNode = nullptr;
    m_placingVia = false;
    m_currentNet = nullptr;
    m_currentLayer = 0;
    m_startItem = nullptr;
    m_endItem = nullptr;
    m_chainedPlacement = false;
    m_orthoMode = false;
    m_placementCorrect = false;
}


LINE_PLACER::~LINE_PLACER()
{
}


void LINE_PLACER::setWorld( NODE* aWorld )
{
    m_world = aWorld;
}


const VIA LINE_PLACER::makeVia( const VECTOR2I& aP )
{
    // fixme: should belong to KICAD_IFACE
    auto iface = Router()->GetInterface();

    int start = m_sizes.ViaType() == VIATYPE::THROUGH ? iface->GetPNSLayerFromBoardLayer( F_Cu )
                                                      : m_sizes.GetLayerTop();
    int end = m_sizes.ViaType() == VIATYPE::THROUGH ? iface->GetPNSLayerFromBoardLayer( B_Cu )
                                                    : m_sizes.GetLayerBottom();

    const PNS_LAYER_RANGE layers(
        start ,
        end
    );

    return VIA( aP, layers, m_sizes.ViaDiameter(), m_sizes.ViaDrill(), nullptr, m_sizes.ViaType() );
}


bool LINE_PLACER::ToggleVia( bool aEnabled )
{
    m_placingVia = aEnabled;

    if( !aEnabled )
        m_head.RemoveVia();

    return true;
}


void LINE_PLACER::setInitialDirection( const DIRECTION_45& aDirection )
{
    m_initial_direction = aDirection;

    if( m_tail.SegmentCount() == 0 )
            m_direction = aDirection;
}


bool LINE_PLACER::handleSelfIntersections()
{
    SHAPE_LINE_CHAIN::INTERSECTIONS ips;
    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    // if there is no tail, there is nothing to intersect with
    if( tail.PointCount() < 2 )
        return false;

    if( head.PointCount() < 2 )
        return false;

    // completely new head trace? chop off the tail
    if( tail.CPoint(0) == head.CPoint(0) )
    {
        m_direction = m_initial_direction;
        tail.Clear();
        return true;
    }

    tail.Intersect( head, ips );

    // no intesection points - nothing to reduce
    if( ips.empty() )
        return false;

    int n = INT_MAX;
    VECTOR2I ipoint;

    // if there is more than one intersection, find the one that is
    // closest to the beginning of the tail.
    for( const SHAPE_LINE_CHAIN::INTERSECTION& i : ips )
    {
        if( i.index_our < n )
        {
            n = i.index_our;
            ipoint = i.p;
        }
    }

    // ignore the point where head and tail meet
    if( ipoint == head.CPoint( 0 ) || ipoint == tail.CLastPoint() )
        return false;

    // Intersection point is on the first or the second segment: just start routing
    // from the beginning
    if( n < 2 )
    {
        m_direction = m_initial_direction;
        tail.Clear();
        head.Clear();

        return true;
    }
    else
    {
        // Clip till the last tail segment before intersection.
        // Set the direction to the one of this segment.
        const SEG last = tail.CSegment( n - 1 );
        m_direction = DIRECTION_45( last );
        tail.Remove( n, -1 );
        return true;
    }

    return false;
}


bool LINE_PLACER::handlePullback()
{
    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    if( head.PointCount() < 2 )
        return false;

    int n = tail.PointCount();

    if( n == 0 )
    {
        return false;
    }
    else if( n == 1 )
    {
        tail.Clear();
        return true;
    }

    DIRECTION_45 first_head, last_tail;

    wxASSERT( tail.PointCount() >= 2 );

    if( !head.IsPtOnArc( 0 ) )
        first_head = DIRECTION_45( head.CSegment( 0 ) );
    else
        first_head = DIRECTION_45( head.CArcs()[head.ArcIndex(0)] );

    int lastSegIdx = tail.PointCount() - 2;

    if( !tail.IsPtOnArc( lastSegIdx ) )
        last_tail = DIRECTION_45( tail.CSegment( lastSegIdx ) );
    else
        last_tail = DIRECTION_45( tail.CArcs()[tail.ArcIndex(lastSegIdx)] );

    DIRECTION_45::AngleType angle = first_head.Angle( last_tail );

    // case 1: we have a defined routing direction, and the currently computed
    // head goes in different one.
    bool pullback_1 = false;    // (m_direction != DIRECTION_45::UNDEFINED && m_direction != first_head);

    // case 2: regardless of the current routing direction, if the tail/head
    // extremities form an acute or right angle, reduce the tail by one segment
    // (and hope that further iterations) will result with a cleaner trace
    bool pullback_2 = ( angle == DIRECTION_45::ANG_RIGHT || angle == DIRECTION_45::ANG_ACUTE );

    if( pullback_1 || pullback_2 )
    {
        if( !tail.IsArcSegment( lastSegIdx ) )
        {
            const SEG& seg = tail.CSegment( lastSegIdx );
            m_direction    = DIRECTION_45( seg );
            PNS_DBG( Dbg(), AddPoint, m_p_start, WHITE, 10000, wxT( "new-pstart [pullback3]" ) );

        }
        else
        {
            const SHAPE_ARC& arc = tail.CArcs()[tail.ArcIndex( lastSegIdx )];
            m_direction          = DIRECTION_45( arc );
        }

        PNS_DBG( Dbg(), Message, wxString::Format( "Placer: pullback triggered [%d] [%s %s]",
                    n, last_tail.Format(), first_head.Format() ) );

        // erase the last point in the tail, hoping that the next iteration will
        // result with a head trace that starts with a segment following our
        // current direction.
        if( n < 2 )
            tail.Clear(); // don't leave a single-point tail
        else
            tail.RemoveShape( -1 );

        if( !tail.SegmentCount() )
            m_direction = m_initial_direction;

        return true;
    }

    return false;
}


bool LINE_PLACER::reduceTail( const VECTOR2I& aEnd )
{
    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    int n = tail.SegmentCount();

    if( head.SegmentCount() < 1 )
        return false;

    // Don't attempt this for too short tails
    if( n < 2 )
        return false;

    // Start from the segment farthest from the end of the tail
    // int start_index = std::max(n - 1 - ReductionDepth, 0);

    DIRECTION_45 new_direction;
    VECTOR2I new_start;
    int reduce_index = -1;

    for( int i = tail.SegmentCount() - 1; i >= 0; i-- )
    {
        const SEG s = tail.CSegment( i );
        DIRECTION_45 dir( s );

        // calculate a replacement route and check if it matches
        // the direction of the segment to be replaced
        SHAPE_LINE_CHAIN replacement = dir.BuildInitialTrace( s.A, aEnd );

        if( replacement.SegmentCount() < 1 )
            continue;

        LINE tmp( m_tail, replacement );

        if( m_currentNode->CheckColliding( &tmp, ITEM::ANY_T ) )
            break;

        if( DIRECTION_45( replacement.CSegment( 0 ) ) == dir )
        {
            new_start = s.A;
            new_direction = dir;
            reduce_index = i;
        }
    }

    if( reduce_index >= 0 )
    {
        PNS_DBG( Dbg(), Message, wxString::Format( "Placer: reducing tail: %d" , reduce_index ) );
        SHAPE_LINE_CHAIN reducedLine = new_direction.BuildInitialTrace( new_start, aEnd );

        m_direction = new_direction;
        tail.Remove( reduce_index + 1, -1 );
        head.Clear();
        return true;
    }

    if( !tail.SegmentCount() )
        m_direction = m_initial_direction;

    return false;
}


bool LINE_PLACER::mergeHead()
{
    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    const int ForbiddenAngles = DIRECTION_45::ANG_ACUTE
                                    | DIRECTION_45::ANG_HALF_FULL
                                    | DIRECTION_45::ANG_UNDEFINED;

    head.Simplify();
    tail.Simplify();

    int n_head  = head.ShapeCount();
    int n_tail  = tail.ShapeCount();

    if( n_head < 3 )
    {
        PNS_DBG( Dbg(), Message, wxT( "Merge failed: not enough head segs." ) );
        return false;
    }

    if( n_tail && head.CPoint( 0 ) != tail.CLastPoint() )
    {
        PNS_DBG( Dbg(), Message, wxT( "Merge failed: head and tail discontinuous." ) );
        return false;
    }

    if( m_head.CountCorners( ForbiddenAngles ) != 0 )
        return false;

    DIRECTION_45 dir_tail, dir_head;

    if( !head.IsPtOnArc( 0 ) )
        dir_head = DIRECTION_45( head.CSegment( 0 ) );
    else
        dir_head = DIRECTION_45( head.CArcs()[head.ArcIndex( 0 )] );

    if( n_tail )
    {
        wxASSERT( tail.PointCount() >= 2 );
        int lastSegIdx = tail.PointCount() - 2;

        if( !tail.IsPtOnArc( lastSegIdx ) )
            dir_tail = DIRECTION_45( tail.CSegment( -1 ) );
        else
            dir_tail = DIRECTION_45( tail.CArcs()[tail.ArcIndex( lastSegIdx )] );

        if( dir_head.Angle( dir_tail ) & ForbiddenAngles )
            return false;
    }

    tail.Append( head );

    tail.Simplify();

    int lastSegIdx = tail.PointCount() - 2;

    if( !tail.IsArcSegment( lastSegIdx ) )
        m_direction = DIRECTION_45( tail.CSegment( -1 ) );
    else
        m_direction = DIRECTION_45( tail.CArcs()[tail.ArcIndex( lastSegIdx )] );

    head.Remove( 0, -1 );

    PNS_DBG( Dbg(), Message, wxString::Format( "Placer: merge %d, new direction: %s" , n_head,
                m_direction.Format() ) );

    head.Simplify();
    tail.Simplify();

    return true;
}


bool LINE_PLACER::clipAndCheckCollisions( const VECTOR2I& aP, const SHAPE_LINE_CHAIN& aL,
                                          SHAPE_LINE_CHAIN& aOut, int &thresholdDist )
{
    SHAPE_LINE_CHAIN l( aL );
    int idx = l.Split( aP );

    if( idx < 0)
        return false;

    bool rv = true;

    SHAPE_LINE_CHAIN l2 = l.Slice( 0, idx );
    int dist = l2.Length();

    PNS_DBG( Dbg(), AddPoint, aP, BLUE, 500000, wxString::Format( "hug-target-check-%d", idx ) );
    PNS_DBG( Dbg(), AddShape, &l2, BLUE, 500000, wxT( "hug-target-line" ) );

    if( dist < thresholdDist )
        rv = false;

    LINE ctest( m_head, l2 );

    if( m_currentNode->CheckColliding( &ctest ).has_value() )
        rv = false;

    if( rv )
    {
        aOut = std::move( l2 );
        thresholdDist = dist;
    }

    return rv;
}


bool LINE_PLACER::cursorDistMinimum( const SHAPE_LINE_CHAIN& aL, const VECTOR2I& aCursor,
                                     double lengthThreshold, SHAPE_LINE_CHAIN &aOut )
{
    std::vector<int>      dists;
    std::vector<VECTOR2I> pts;

    if( aL.PointCount() == 0 )
        return false;

    VECTOR2I lastP = aL.CLastPoint();
    int accumulatedDist = 0;

    dists.reserve( 2 * aL.PointCount() );

    for( int i = 0; i < aL.SegmentCount(); i++ )
    {
        const SEG& s = aL.CSegment( i );

        dists.push_back( ( aCursor - s.A ).EuclideanNorm() );
        pts.push_back( s.A );
        auto pn = s.NearestPoint( aCursor );

        if( pn != s.A && pn != s.B )
        {
            dists.push_back( ( pn - aCursor ).EuclideanNorm() );
            pts.push_back( pn );
        }

        accumulatedDist += s.Length();

        if ( accumulatedDist > lengthThreshold )
        {
            lastP = s.B;
            break;
        }
    }

    dists.push_back( ( aCursor - lastP ).EuclideanNorm() );
    pts.push_back( lastP );

    int minDistLoc = std::numeric_limits<int>::max();
    int minPLoc = -1;
    int minDistGlob = std::numeric_limits<int>::max();
    int minPGlob = -1;

    for( int i = 0; i < dists.size(); i++ )
    {
        int d = dists[i];

        if( d < minDistGlob )
        {
            minDistGlob = d;
            minPGlob = i;
        }
    }

    if( dists.size() >= 3 )
    {
        for( int i = 0; i < dists.size() - 3; i++ )
        {
            if( dists[i + 2] > dists[i + 1] && dists[i] > dists[i + 1] )
            {
                int d = dists[i + 1];
                if( d < minDistLoc )
                {
                    minDistLoc = d;
                    minPLoc    = i + 1;
                }
            }
        }

        if( dists.back() < minDistLoc && minPLoc >= 0 )
        {
            minDistLoc = dists.back();
            minPLoc    = dists.size() - 1;
        }
    }
    else
    {
        // Too few points: just use the global
        minDistLoc = minDistGlob;
        minPLoc    = minPGlob;
    }

// fixme: I didn't make my mind yet if local or global minimum feels better. I'm leaving both
// in the code, enabling the global one by default
    minPLoc = -1;
    int preferred;

    if( minPLoc < 0 )
    {
        preferred = minPGlob;
    }
    else
    {
        preferred = minPLoc;
    }

    int thresholdDist = 0;

    if( clipAndCheckCollisions( pts[preferred], aL, aOut, thresholdDist ) )
        return true;

    thresholdDist = 0;

    SHAPE_LINE_CHAIN l( aL ), prefL;
    int minDist = std::numeric_limits<int>::max();

    bool ok = false;

    for( int i = 0; i < pts.size() ; i++)
    {
        //PNS_DBG( Dbg(), AddPoint, pts[i], BLUE, 500000, wxT( "hug-target-fallback" ) );

        ok |= clipAndCheckCollisions( pts[i], aL, aOut, thresholdDist );
    }

    return ok;
}


bool LINE_PLACER::rhWalkBase( const VECTOR2I& aP, LINE& aWalkLine, int aCollisionMask,
                              PNS::PNS_MODE aMode, bool& aViaOk )
{
    LINE walkFull( m_head );
    LINE l1( m_head );

    PNS_DBG( Dbg(), AddItem, &m_tail, GREEN, 100000, wxT( "walk-base-old-tail" ) );
    PNS_DBG( Dbg(), AddItem, &m_head, BLUE, 100000, wxT( "walk-base-old-head" ) );

    VECTOR2I walkP = aP;

    WALKAROUND walkaround( m_currentNode, Router() );

    walkaround.SetSolidsOnly( false );
    walkaround.SetDebugDecorator( Dbg() );
    walkaround.SetLogger( Logger() );
    walkaround.SetIterationLimit( Settings().WalkaroundIterationLimit() );
    walkaround.SetItemMask( aCollisionMask );
    walkaround.SetAllowedPolicies( { WALKAROUND::WP_CCW, WALKAROUND::WP_CW } );

    int round = 0;

    do
    {
        l1.Clear();

        PNS_DBG( Dbg(), BeginGroup, wxString::Format( "walk-round-%d", round ), 0 );
        round++;

        aViaOk = buildInitialLine( walkP, l1, aMode, round == 0 );
        PNS_DBG( Dbg(), AddItem, &l1, BLUE, 20000, wxT( "walk-base-l1" ) );

        if( l1.EndsWithVia() )
            PNS_DBG( Dbg(), AddPoint, l1.Via().Pos(), BLUE, 100000, wxT( "walk-base-l1-via" ) );

        LINE initTrack( m_tail );
        initTrack.Line().Append( l1.CLine() );
        initTrack.Line().Simplify();


        double initialLength = initTrack.CLine().Length();
        double hugThresholdLength = initialLength * Settings().WalkaroundHugLengthThreshold();
        double hugThresholdLengthComplete =
                2.0 * initialLength * Settings().WalkaroundHugLengthThreshold();

        WALKAROUND::RESULT wr = walkaround.Route( initTrack );
        std::optional<LINE> bestLine;

        OPTIMIZER optimizer( m_currentNode );

        optimizer.SetEffortLevel( OPTIMIZER::MERGE_SEGMENTS );
        optimizer.SetCollisionMask( aCollisionMask );

        using WALKAROUND::WP_CW;
        using WALKAROUND::WP_CCW;

        int len_cw = wr.status[WP_CW] != WALKAROUND::ST_STUCK ? wr.lines[WP_CW].CLine().Length()
                                                      : std::numeric_limits<int>::max();
        int len_ccw = wr.status[WP_CCW] != WALKAROUND::ST_STUCK ? wr.lines[WP_CCW].CLine().Length()
                                                        : std::numeric_limits<int>::max();


        if( wr.status[ WP_CW ] == WALKAROUND::ST_DONE )
        {
            PNS_DBG( Dbg(), AddItem, &wr.lines[WP_CW], BLUE, 20000, wxT( "wf-result-cw-preopt" ) );
            LINE tmpHead, tmpTail;


            OPTIMIZER::Optimize( &wr.lines[WP_CW], OPTIMIZER::MERGE_SEGMENTS, m_currentNode );

            if( splitHeadTail( wr.lines[WP_CW], m_tail, tmpHead, tmpTail ) )
            {
                optimizer.Optimize( &tmpHead );
                wr.lines[WP_CW].SetShape( tmpTail.CLine () );
                wr.lines[WP_CW].Line().Append( tmpHead.CLine( ) );
            }

            PNS_DBG( Dbg(), AddItem, &wr.lines[WP_CW], RED, 20000, wxT( "wf-result-cw-postopt" ) );
            len_cw = wr.lines[WP_CW].CLine().Length();
            bestLine = wr.lines[WP_CW];
        }

        if( wr.status[WP_CCW] == WALKAROUND::ST_DONE )
        {
            PNS_DBG( Dbg(), AddItem, &wr.lines[WP_CCW], BLUE, 20000, wxT( "wf-result-ccw-preopt" ) );

            LINE tmpHead, tmpTail;

            OPTIMIZER::Optimize( &wr.lines[WP_CCW], OPTIMIZER::MERGE_SEGMENTS, m_currentNode );

            if( splitHeadTail( wr.lines[WP_CCW], m_tail, tmpHead, tmpTail ) )
            {
                optimizer.Optimize( &tmpHead );
                wr.lines[WP_CCW].SetShape( tmpTail.CLine () );
                wr.lines[WP_CCW].Line().Append( tmpHead.CLine( ) );
            }

            PNS_DBG( Dbg(), AddItem, &wr.lines[WP_CCW], RED, 20000, wxT( "wf-result-ccw-postopt" ) );
            len_ccw = wr.lines[WP_CCW].CLine().Length();

            if( len_ccw < len_cw )
                bestLine = wr.lines[WP_CCW];
        }

        int bestLength = len_cw < len_ccw ? len_cw : len_ccw;

        if( bestLength < hugThresholdLengthComplete && bestLine.has_value() )
        {
            walkFull.SetShape( bestLine->CLine() );
            walkP = walkFull.CLine().CLastPoint();
            PNS_DBGN( Dbg(), EndGroup );
            continue;
        }

        bool validCw = false;
        bool validCcw = false;
        int  distCcw = std::numeric_limits<int>::max();
        int  distCw = std::numeric_limits<int>::max();

        SHAPE_LINE_CHAIN l_cw, l_ccw;


        if( wr.status[WP_CW] != WALKAROUND::ST_STUCK )
        {
            validCw = cursorDistMinimum( wr.lines[WP_CW].CLine(), aP, hugThresholdLength, l_cw );

            if( validCw )
                distCw = ( aP - l_cw.CLastPoint() ).EuclideanNorm();

            PNS_DBG( Dbg(), AddShape, &l_cw, MAGENTA, 200000, wxString::Format( "wh-result-cw %s",
                                                                                 validCw ? "non-colliding"
                                                                                         : "colliding" ) );
        }

        if( wr.status[WP_CCW] != WALKAROUND::ST_STUCK )
        {
            validCcw = cursorDistMinimum( wr.lines[WP_CCW].CLine(), aP, hugThresholdLength, l_ccw );

            if( validCcw )
                distCcw = ( aP - l_ccw.CLastPoint() ).EuclideanNorm();

            PNS_DBG( Dbg(), AddShape, &l_ccw, MAGENTA, 200000, wxString::Format( "wh-result-ccw %s",
                                                                                 validCcw ? "non-colliding"
                                                                                          : "colliding" ) );
        }


        if( distCw < distCcw && validCw )
        {
            walkFull.SetShape( l_cw );
            walkP = l_cw.CLastPoint();
        }
        else if( validCcw )
        {
            walkFull.SetShape( l_ccw );
            walkP = l_ccw.CLastPoint();
        }
        else
        {
            PNS_DBGN( Dbg(), EndGroup );
            return false;
        }

        PNS_DBGN( Dbg(), EndGroup );
    } while( round < 2 && m_placingVia );


    if( l1.EndsWithVia() )
    {
        VIA v ( l1.Via() );
        v.SetPos( walkFull.CLastPoint() );
        walkFull.AppendVia( v );
    }

    PNS_DBG( Dbg(), AddItem, &walkFull, GREEN, 200000, wxT( "walk-full" ) );

    if( walkFull.EndsWithVia() )
    {
        PNS_DBG( Dbg(), AddPoint, walkFull.Via().Pos(), GREEN, 200000,
                 wxString::Format( "walk-via ok %d", aViaOk ? 1 : 0 ) );
    }

    aWalkLine = walkFull;

    return !walkFull.EndsWithVia() || aViaOk;
}


bool LINE_PLACER::rhWalkOnly( const VECTOR2I& aP, LINE& aNewHead, LINE& aNewTail )
{
    LINE walkFull;

    int effort = 0;
    bool viaOk = false;

    if( ! rhWalkBase( aP, walkFull, ITEM::ANY_T, RM_Walkaround, viaOk ) )
        return false;

    switch( Settings().OptimizerEffort() )
    {
    case OE_LOW:
        effort = 0;
        break;

    case OE_MEDIUM:
    case OE_FULL:
        effort = OPTIMIZER::MERGE_SEGMENTS;
        break;
    }

    DIRECTION_45::CORNER_MODE cornerMode = Settings().GetCornerMode();

    // Smart Pads is incompatible with 90-degree mode for now
    if( Settings().SmartPads()
            && ( cornerMode == DIRECTION_45::MITERED_45 || cornerMode == DIRECTION_45::ROUNDED_45 )
            && !m_mouseTrailTracer.IsManuallyForced() )
    {
        effort |= OPTIMIZER::SMART_PADS;
    }

    if( m_currentNode->CheckColliding( &walkFull ) )
    {
        PNS_DBG( Dbg(), AddItem, &walkFull, GREEN, 100000, wxString::Format( "collision check fail" ) );
        return false;
    }

    // OK, this deserves a bit of explanation. We used to calculate the walk path for the head only,
    // but then the clearance epsilon was added, with the intent of improving collision resolution robustness
    // (now a hull or a walk/shove line cannot collide with the 'owner' of the hull under any circumstances).
    // This, however, introduced a subtle bug. For a row/column/any other 'regular' arrangement
    // of overlapping hulls (think of pads of a SOP/SOIC chip or a regular via grid), walking around may
    // produce a new 'head' that is not considered colliding (due to the clearance epsilon), but with
    // its start point inside one of the subsequent hulls to process.
    // We can't have head[0] inside any hull for the algorithm to work - therefore, we now consider the entire
    // 'tail+head' trace when walking around and in case of success, reconstruct the
    // 'head' and 'tail' by splitting the walk line at a point that is as close as possible to the original
    // head[0], but not inside any obstacle hull.
    //
    // EXECUTIVE SUMMARY: asinine heuristic to make the router get stuck much less often.

    if( ! splitHeadTail( walkFull, m_tail, aNewHead, aNewTail ) )
        return false;

    if( m_placingVia && viaOk )
    {
        PNS_DBG( Dbg(), AddPoint, aNewHead.CLastPoint(), RED, 1000000, wxString::Format( "VIA" ) );

        aNewHead.AppendVia( makeVia( aNewHead.CLastPoint() ) );
    }

    OPTIMIZER::Optimize( &aNewHead, effort, m_currentNode );

    PNS_DBG( Dbg(), AddItem, &aNewHead, GREEN, 100000, wxString::Format( "walk-new-head" ) );
    PNS_DBG( Dbg(), AddItem, &aNewTail, BLUE, 100000, wxT( "walk-new-tail" ) );

    return true;
}


bool LINE_PLACER::rhMarkObstacles( const VECTOR2I& aP, LINE& aNewHead, LINE& aNewTail )
{
    buildInitialLine( aP, m_head, RM_MarkObstacles );
    m_head.SetBlockingObstacle( nullptr );

    auto obs = m_currentNode->NearestObstacle( &m_head );

    // If the head is in colliding state, snap to the hull of the first obstacle.
    // This way, one can route tracks as tightly as possible without enabling
    // the shove/walk mode that certain users find too intrusive.
    if( obs )
    {
        int clearance = m_currentNode->GetClearance( obs->m_item, &m_head, false );
        const SHAPE_LINE_CHAIN& hull = m_currentNode->GetRuleResolver()->HullCache(
                obs->m_item, clearance, m_head.Width(), m_head.Layer() );
        VECTOR2I nearest;

        DIRECTION_45::CORNER_MODE cornerMode = Settings().GetCornerMode();

        if( cornerMode == DIRECTION_45::MITERED_90 || cornerMode == DIRECTION_45::ROUNDED_90 )
            nearest = hull.BBox().NearestPoint( aP );
        else
            nearest = hull.NearestPoint( aP );

        if( ( nearest - aP ).EuclideanNorm() < m_head.Width() / 2 )
            buildInitialLine( nearest, m_head, RM_MarkObstacles );
    }

    // Note: Something like the below could be used to implement a "stop at first obstacle" mode,
    // but we don't have one right now and there isn't a lot of demand for one.  If we do end up
    // doing that, put it in a new routing mode as "highlight collisions" mode should not have
    // collision handling other than highlighting.
#if 0
    if( !Settings().AllowDRCViolations() )
    {
        NODE::OPT_OBSTACLE obs = m_currentNode->NearestObstacle( &m_head );

        if( obs && obs->m_distFirst != INT_MAX )
        {
            buildInitialLine( obs->m_ipFirst, m_head );
            m_head.SetBlockingObstacle( obs->m_item );
        }
    }
#endif

    aNewHead = m_head;
    aNewTail = m_tail;

    return true;
}


bool LINE_PLACER::splitHeadTail( const LINE& aNewLine, const LINE& aOldTail, LINE& aNewHead,
                                 LINE& aNewTail )
{
    LINE newTail( aOldTail );
    LINE newHead( aOldTail );
    LINE l2( aNewLine );

    newTail.RemoveVia();
    newHead.Clear();

    int  i;
    bool found = false;
    int  n = l2.PointCount();

    if( n > 1 && aOldTail.PointCount() > 1 )
    {
        if( l2.CLine().PointOnEdge( aOldTail.CLastPoint() ) )
        {
            l2.Line().Split( aOldTail.CLastPoint() );
        }

        for( i = 0; i < aOldTail.PointCount(); i++ )
        {
            if( l2.CLine().Find( aOldTail.CPoint( i ) ) < 0 )
            {
                found = true;
                break;
            }
        }

        if( !found )
            i--;

        // If the old tail doesn't have any points of the new line, we can't split it.
        if( i >= l2.PointCount() )
            i = l2.PointCount() - 1;

        newHead.Clear();

        if( i == 0 )
            newTail.Clear();
        else
            newTail.SetShape( l2.CLine().Slice( 0, i ) );

        newHead.SetShape( l2.CLine().Slice( i, -1 ) );
    }
    else
    {
        newTail.Clear();
        newHead = std::move( l2 );
    }

    PNS_DBG( Dbg(), AddItem, &newHead, BLUE, 500000, wxT( "head-post-split" ) );

    aNewHead = std::move( newHead );
    aNewTail = std::move( newTail );

    return true;
}


bool LINE_PLACER::rhShoveOnly( const VECTOR2I& aP, LINE& aNewHead, LINE& aNewTail )
{
    LINE walkSolids;

    bool viaOk = false;

    if( ! rhWalkBase( aP, walkSolids, ITEM::SOLID_T, RM_Shove, viaOk ) )
        return false;

    m_currentNode = m_shove->CurrentNode();

    m_shove->SetLogger( Logger() );
    m_shove->SetDebugDecorator( Dbg() );

    if( m_endItem )
    {
        // Make sure the springback algorithm won't erase the NODE that owns m_endItem.
        m_shove->SetSpringbackDoNotTouchNode( static_cast<const NODE*>( m_endItem->Owner() ) );
    }

    LINE newHead( walkSolids );

    if( walkSolids.EndsWithVia() )
        PNS_DBG( Dbg(), AddPoint, newHead.Via().Pos(), RED, 1000000, wxString::Format( "SVIA [%d]", viaOk?1:0 ) );

    if( m_placingVia && viaOk )
    {
        newHead.AppendVia( makeVia( newHead.CLastPoint() ) );
        PNS_DBG( Dbg(), AddPoint, newHead.Via().Pos(), GREEN, 1000000, "shove-new-via" );

    }

    m_shove->ClearHeads();
    m_shove->AddHeads( newHead, SHOVE::SHP_SHOVE );
    bool shoveOk = m_shove->Run() == SHOVE::SH_OK;

    m_currentNode = m_shove->CurrentNode();

    int effort = 0;

    switch( Settings().OptimizerEffort() )
    {
    case OE_LOW:
        effort = 0;
        break;

    case OE_MEDIUM:
    case OE_FULL:
        effort = OPTIMIZER::MERGE_SEGMENTS;
        break;
    }

    DIRECTION_45::CORNER_MODE cornerMode = Settings().GetCornerMode();

    // Smart Pads is incompatible with 90-degree mode for now
    if( Settings().SmartPads()
            && ( cornerMode == DIRECTION_45::MITERED_45 || cornerMode == DIRECTION_45::ROUNDED_45 )
            && !m_mouseTrailTracer.IsManuallyForced() )
    {
        effort |= OPTIMIZER::SMART_PADS;
    }

    if( shoveOk )
    {
        if( m_shove->HeadsModified() )
            newHead = m_shove->GetModifiedHead( 0 );

        if( newHead.EndsWithVia() )
        {
            PNS_DBG( Dbg(), AddPoint, newHead.Via().Pos(), GREEN, 1000000, "shove-via-preopt" );
            PNS_DBG( Dbg(), AddPoint, newHead.Via().Pos(), GREEN, 1000000, "shove-via-postopt" );
        }

        if( ! splitHeadTail( newHead, m_tail, aNewHead, aNewTail ) )
            return false;

        if( newHead.EndsWithVia() )
            aNewHead.AppendVia( newHead.Via() );

        OPTIMIZER::Optimize( &aNewHead, effort, m_currentNode );
        PNS_DBG( Dbg(), AddItem, aNewHead.Clone(), GREEN, 1000000, "head-sh-postopt" );

        return true;
    }
    else
    {
        return rhWalkOnly( aP, aNewHead, aNewTail );
    }

    return false;
}


bool LINE_PLACER::routeHead( const VECTOR2I& aP, LINE& aNewHead, LINE& aNewTail )
{
    switch( Settings().Mode() )
    {
    case RM_MarkObstacles:
        return rhMarkObstacles( aP, aNewHead, aNewTail );
    case RM_Walkaround:
        return rhWalkOnly( aP, aNewHead, aNewTail );
    case RM_Shove:
        return rhShoveOnly( aP, aNewHead, aNewTail );
    default:
        break;
    }

    return false;
}


bool LINE_PLACER::optimizeTailHeadTransition()
{
    LINE linetmp = Trace();

    PNS_DBG( Dbg(), Message, "optimize HT" );

    // NOTE: FANOUT_CLEANUP can override posture setting at the moment
    if( !m_mouseTrailTracer.IsManuallyForced() &&
        OPTIMIZER::Optimize( &linetmp, OPTIMIZER::FANOUT_CLEANUP, m_currentNode ) )
    {
        if( linetmp.SegmentCount() < 1 )
            return false;

        m_head = linetmp;
        m_direction = DIRECTION_45( linetmp.CSegment( 0 ) );
        m_tail.Line().Clear();

        return true;
    }

    SHAPE_LINE_CHAIN& head = m_head.Line();
    SHAPE_LINE_CHAIN& tail = m_tail.Line();

    int tailLookbackSegments = 3;

    //if(m_currentMode() == RM_Walkaround)
    //    tailLookbackSegments = 10000;

    int threshold = std::min( tail.PointCount(), tailLookbackSegments + 1 );

    if( tail.ShapeCount() < 3 )
        return false;

    // assemble TailLookbackSegments tail segments with the current head
    SHAPE_LINE_CHAIN opt_line = tail.Slice( -threshold, -1 );

    int end = std::min(2, head.PointCount() - 1 );

    opt_line.Append( head.Slice( 0, end ) );

    LINE new_head( m_tail, opt_line );

    // and see if it could be made simpler by merging obtuse/collnear segments.
    // If so, replace the (threshold) last tail points and the head with
    // the optimized line

    PNS_DBG( Dbg(), AddItem, &new_head, LIGHTCYAN, 10000, wxT( "ht-newline" ) );

    if( OPTIMIZER::Optimize( &new_head, OPTIMIZER::MERGE_SEGMENTS, m_currentNode ) )
    {
        LINE tmp( m_tail, opt_line );

        head.Clear();
        tail.Replace( -threshold, -1, new_head.CLine() );
        tail.Simplify();

        m_direction = DIRECTION_45( new_head.CSegment( -1 ) );

        return true;
    }

    return false;
}

void LINE_PLACER::updatePStart( const LINE& tail )
{
    if( tail.CLine().PointCount() )
        m_p_start = tail.CLine().CLastPoint();
    else
        m_p_start = m_currentStart;
}

void LINE_PLACER::routeStep( const VECTOR2I& aP )
{
    bool fail = false;
    bool go_back = false;

    int i, n_iter = 1;


    PNS_DBG( Dbg(), Message, wxString::Format( "routeStep: direction: %s head: %d, tail: %d shapes" ,
                                               m_direction.Format(),
                                               m_head.ShapeCount(),
                                               m_tail.ShapeCount() ) );

    PNS_DBG( Dbg(), BeginGroup, wxT( "route-step" ), 0 );

    PNS_DBG( Dbg(), AddItem, &m_tail, WHITE, 10000, wxT( "tail-init" ) );
    PNS_DBG( Dbg(), AddItem, &m_head, GREEN, 10000, wxT( "head-init" ) );

    for( i = 0; i < n_iter; i++ )
    {
        LINE prevTail( m_tail );
        LINE prevHead( m_head );
        LINE newHead, newTail;

        if( !go_back && Settings().FollowMouse() )
            reduceTail( aP );

        PNS_DBG( Dbg(), AddItem, &m_tail, WHITE, 10000, wxT( "tail-after-reduce" ) );
        PNS_DBG( Dbg(), AddItem, &m_head, GREEN, 10000, wxT( "head-after-reduce" ) );

        go_back = false;

        updatePStart( m_tail );

        if( !routeHead( aP, newHead, newTail ) )
        {
            m_tail = std::move( prevTail );
            m_head = std::move( prevHead );

            // If we fail to walk out of the initial point (no tail), instead of returning an empty
            // line, return a zero-length line so that the user gets some feedback that routing is
            // happening.  This will get pruned later.
            if( m_tail.PointCount() == 0 )
            {
                m_tail.Line().Append( m_p_start );
                m_tail.Line().Append( m_p_start, true );
            }

            fail = true;
        }

        updatePStart( m_tail );

        PNS_DBG( Dbg(), AddItem, &newHead, LIGHTGREEN, 100000, wxString::Format( "new_head [fail: %d]", fail?1:0 ) );

        if( fail )
            break;

        PNS_DBG( Dbg(), Message, wxString::Format( "N VIA H %d T %d\n", m_head.EndsWithVia() ? 1 : 0, m_tail.EndsWithVia() ? 1 : 0 ) );

        m_head = std::move( newHead );
        m_tail = std::move( newTail );

        if( handleSelfIntersections() )
        {
            n_iter++;
            go_back = true;
        }

        PNS_DBG( Dbg(), Message, wxString::Format( "SI VIA H %d T %d\n", m_head.EndsWithVia() ? 1 : 0, m_tail.EndsWithVia() ? 1 : 0 ) );

        PNS_DBG( Dbg(), AddItem, &m_tail, WHITE, 10000, wxT( "tail-after-si" ) );
        PNS_DBG( Dbg(), AddItem, &m_head, GREEN, 10000, wxT( "head-after-si" ) );

        if( !go_back && handlePullback() )
        {
            n_iter++;
            m_head.Clear();
            go_back = true;
        }

        PNS_DBG( Dbg(), Message, wxString::Format( "PB VIA H %d T %d\n", m_head.EndsWithVia() ? 1 : 0, m_tail.EndsWithVia() ? 1 : 0 ) );

        PNS_DBG( Dbg(), AddItem, &m_tail, WHITE, 100000, wxT( "tail-after-pb" ) );
        PNS_DBG( Dbg(), AddItem, &m_head, GREEN, 100000, wxT( "head-after-pb" ) );
    }


    if( !fail && Settings().FollowMouse() )
    {
        PNS_DBG( Dbg(), AddItem, &m_tail, WHITE, 10000, wxT( "tail-pre-merge" ) );
        PNS_DBG( Dbg(), AddItem, &m_head, GREEN, 10000, wxT( "head-pre-merge" ) );

        if( !optimizeTailHeadTransition() )
        {
            PNS_DBG( Dbg(), Message, wxString::Format( "PreM VIA H %d T %d\n", m_head.EndsWithVia() ? 1 : 0, m_tail.EndsWithVia() ? 1 : 0 ) );

            mergeHead();

            PNS_DBG( Dbg(), Message, wxString::Format( "PostM VIA H %d T %d\n", m_head.EndsWithVia() ? 1 : 0, m_tail.EndsWithVia() ? 1 : 0 ) );
        }

        PNS_DBG( Dbg(), AddItem, &m_tail, WHITE, 100000, wxT( "tail-post-merge" ) );
        PNS_DBG( Dbg(), AddItem, &m_head, GREEN, 100000, wxT( "head-post-merge" ) );
    }

    m_last_p_end = aP;

    PNS_DBGN( Dbg(), EndGroup );
}


bool LINE_PLACER::route( const VECTOR2I& aP )
{
    routeStep( aP );

    if( !m_head.PointCount() )
        return false;

    return m_head.CLastPoint() == aP;
}


const LINE LINE_PLACER::Trace() const
{
    SHAPE_LINE_CHAIN l( m_tail.CLine() );
    l.Append( m_head.CLine() );

    // Only simplify if we have more than two points, because if we have a zero-length seg as the
    // only part of the trace, we don't want it to be removed at this stage (will be the case if
    // the routing start point violates DRC due to track width in shove/walk mode, for example).
    if( l.PointCount() > 2 )
        l.Simplify();

    LINE tmp( m_head );

    tmp.SetShape( l );

    PNS_DBG( Dbg(), AddItem, &m_tail, GREEN, 100000, wxT( "tmp-tail" ) );
    PNS_DBG( Dbg(), AddItem, &m_head, LIGHTGREEN, 100000, wxT( "tmp-head" ) );

    return tmp;
}


const ITEM_SET LINE_PLACER::Traces()
{
    m_currentTrace = Trace();
    return ITEM_SET( &m_currentTrace );
}


void LINE_PLACER::FlipPosture()
{
    // In order to fix issue 12369 get the current line placer first direction
    // and copy it to the mouse trail tracer, as the current placer may have
    // changed the route.
    if( m_mouseTrailTracer.IsManuallyForced() == false && m_currentTrace.SegmentCount() > 0 )
    {
        DIRECTION_45 firstDirection( m_currentTrace.CSegment( 0 ) );

        m_mouseTrailTracer.SetDefaultDirections( firstDirection, DIRECTION_45::UNDEFINED );
    }

    m_mouseTrailTracer.FlipPosture();
}


NODE* LINE_PLACER::CurrentNode( bool aLoopsRemoved ) const
{
    if( aLoopsRemoved && m_lastNode )
        return m_lastNode;

    return m_currentNode;
}


bool LINE_PLACER::SplitAdjacentSegments( NODE* aNode, ITEM* aSeg, const VECTOR2I& aP )
{
    if( !aSeg )
        return false;

    if( !aSeg->OfKind( ITEM::SEGMENT_T ) )
        return false;

    const JOINT* jt = aNode->FindJoint( aP, aSeg );

    if( jt && jt->LinkCount() >= 1 )
        return false;

    SEGMENT* s_old = static_cast<SEGMENT*>( aSeg );

    std::unique_ptr<SEGMENT> s_new[2] = { Clone( *s_old ), Clone( *s_old ) };

    s_new[0]->SetEnds( s_old->Seg().A, aP );
    s_new[1]->SetEnds( aP, s_old->Seg().B );

    aNode->Remove( s_old );
    aNode->Add( std::move( s_new[0] ), true );
    aNode->Add( std::move( s_new[1] ), true );

    return true;
}


bool LINE_PLACER::SplitAdjacentArcs( NODE* aNode, ITEM* aArc, const VECTOR2I& aP )
{
    if( !aArc )
        return false;

    if( !aArc->OfKind( ITEM::ARC_T ) )
        return false;

    const JOINT* jt = aNode->FindJoint( aP, aArc );

    if( jt && jt->LinkCount() >= 1 )
        return false;

    ARC*             a_old = static_cast<ARC*>( aArc );
    const SHAPE_ARC& o_arc = a_old->Arc();

    std::unique_ptr<ARC> a_new[2] = { Clone( *a_old ), Clone( *a_old ) };

    a_new[0]->Arc().ConstructFromStartEndCenter( o_arc.GetP0(), aP, o_arc.GetCenter(),
                                                 o_arc.IsClockwise(), o_arc.GetWidth() );

    a_new[1]->Arc().ConstructFromStartEndCenter( aP, o_arc.GetP1(), o_arc.GetCenter(),
                                                 o_arc.IsClockwise(), o_arc.GetWidth() );

    aNode->Remove( a_old );
    aNode->Add( std::move( a_new[0] ), true );
    aNode->Add( std::move( a_new[1] ), true );

    return true;
}


bool LINE_PLACER::SetLayer( int aLayer )
{
    if( m_idle )
    {
        m_currentLayer = aLayer;
        return true;
    }
    else if( m_chainedPlacement )
    {
        return false;
    }
    else if( !m_startItem
            || ( m_startItem->OfKind( ITEM::VIA_T ) && m_startItem->Layers().Overlaps( aLayer ) )
            || ( m_startItem->OfKind( ITEM::SOLID_T ) && m_startItem->Layers().Overlaps( aLayer ) ) )
    {
        m_currentLayer = aLayer;
        m_p_start = m_currentStart;
        m_direction = m_initial_direction;
        m_mouseTrailTracer.Clear();
        m_head.Line().Clear();
        m_tail.Line().Clear();
        m_head.RemoveVia();
        m_tail.RemoveVia();
        m_head.SetLayer( m_currentLayer );
        m_tail.SetLayer( m_currentLayer );
        Move( m_currentEnd, nullptr );
        return true;
    }

    return false;
}


bool LINE_PLACER::Start( const VECTOR2I& aP, ITEM* aStartItem )
{
    m_placementCorrect = false;
    m_currentStart = VECTOR2I( aP );
    m_fixStart =  VECTOR2I( aP );
    m_currentEnd = VECTOR2I( aP );
    m_currentNet = aStartItem ? aStartItem->Net() : Router()->GetInterface()->GetOrphanedNetHandle();
    m_startItem = aStartItem;
    m_placingVia = false;
    m_chainedPlacement = false;
    m_fixedTail.Clear();
    m_endItem = nullptr;

    setInitialDirection( Settings().InitialDirection() );

    initPlacement();

    DIRECTION_45 initialDir = m_initial_direction;
    DIRECTION_45 lastSegDir = DIRECTION_45::UNDEFINED;

    if( aStartItem && aStartItem->Kind() == ITEM::SEGMENT_T )
    {
        // If we land on a segment endpoint, assume the starting direction is continuing along
        // the same direction as the endpoint.  If we started in the middle, don't set a
        // direction so that the posture solver is not biased.
        SEG seg = static_cast<SEGMENT*>( aStartItem )->Seg();

        if( aP == seg.A )
            lastSegDir = DIRECTION_45( seg.Reversed() );
        else if( aP == seg.B )
            lastSegDir = DIRECTION_45( seg );
    }
    else if( aStartItem && aStartItem->Kind() == ITEM::SOLID_T &&
             static_cast<SOLID*>( aStartItem )->Parent()->Type() == PCB_PAD_T )
    {
        double angle = static_cast<SOLID*>( aStartItem )->GetOrientation().AsDegrees();
        angle        = ( angle + 22.5 ) / 45.0;
        initialDir   = DIRECTION_45( static_cast<DIRECTION_45::Directions>( int( angle ) ) );
    }

    PNS_DBG( Dbg(), Message, wxString::Format( "Posture: init %s, last seg %s",
                initialDir.Format(), lastSegDir.Format() ) );

    m_mouseTrailTracer.Clear();
    m_mouseTrailTracer.AddTrailPoint( aP );
    m_mouseTrailTracer.SetTolerance( m_head.Width() );
    m_mouseTrailTracer.SetDefaultDirections( m_initial_direction, DIRECTION_45::UNDEFINED );
    m_mouseTrailTracer.SetMouseDisabled( !Settings().GetAutoPosture() );

    NODE *n;

    if ( Settings().Mode() == PNS::RM_Shove )
        n = m_shove->CurrentNode();
    else
        n = m_currentNode;

    m_fixedTail.AddStage( m_fixStart, m_currentLayer, m_placingVia, m_direction, n );

    return true;
}


void LINE_PLACER::initPlacement()
{
    m_idle = false;

    m_head.Line().Clear();
    m_tail.Line().Clear();
    m_head.SetNet( m_currentNet );
    m_tail.SetNet( m_currentNet );
    m_head.SetLayer( m_currentLayer );
    m_tail.SetLayer( m_currentLayer );
    m_head.SetWidth( m_sizes.TrackWidth() );
    m_tail.SetWidth( m_sizes.TrackWidth() );
    m_head.RemoveVia();
    m_tail.RemoveVia();

    m_last_p_end.reset();
    m_p_start = m_currentStart;
    m_direction = m_initial_direction;

    NODE* world = Router()->GetWorld();

    world->KillChildren();
    NODE* rootNode = world->Branch();

    SplitAdjacentSegments( rootNode, m_startItem, m_currentStart );

    setWorld( rootNode );

    wxLogTrace( wxT( "PNS" ), wxT( "world %p, intitial-direction %s layer %d" ),
                m_world,
                m_direction.Format().c_str(),
                m_currentLayer );

    m_lastNode = nullptr;
    m_currentNode = m_world;

    m_shove = std::make_unique<SHOVE>( m_world->Branch(), Router() );
}


bool LINE_PLACER::Move( const VECTOR2I& aP, ITEM* aEndItem )
{
    LINE current;
    int  eiDepth = -1;

    if( aEndItem && aEndItem->Owner() )
        eiDepth = static_cast<const NODE*>( aEndItem->Owner() )->Depth();

    if( m_lastNode )
    {
        delete m_lastNode;
        m_lastNode = nullptr;
    }

    m_endItem = aEndItem;

    bool reachesEnd = route( aP );

    current = Trace();

    VECTOR2I splitPoint = current.PointCount() ? current.CLine().CLastPoint() : m_p_start;

    if( reachesEnd && aEndItem && current.SegmentCount() && aEndItem->OfKind( ITEM::SEGMENT_T ) )
    {
        const SEG lastSeg = current.CLine().CSegment( current.SegmentCount() - 1 );
        const SEG targetSeg = static_cast<SEGMENT*>( aEndItem )->Seg();

        if( lastSeg.Collinear( targetSeg ) && targetSeg.Overlaps( lastSeg ) )
        {
            splitPoint = targetSeg.NearestPoint( lastSeg.A );
            current.Line().SetPoint( current.PointCount() - 1, splitPoint );
            m_head.Line().SetPoint( m_head.PointCount() - 1, splitPoint );
        }
    }

    if( !current.PointCount() )
        m_currentEnd = m_p_start;
    else
        m_currentEnd = splitPoint;

    NODE* latestNode = m_currentNode;
    m_lastNode = latestNode->Branch();

    if( reachesEnd
            && eiDepth >= 0
            && aEndItem && latestNode->Depth() >= eiDepth
            && current.SegmentCount() )
    {
        if ( aEndItem->Net() == m_currentNet )
            SplitAdjacentSegments( m_lastNode, aEndItem, splitPoint );

        if( Settings().RemoveLoops() )
            removeLoops( m_lastNode, current );
    }

    updateLeadingRatLine();
    m_mouseTrailTracer.AddTrailPoint( aP );
    return true;
}


bool LINE_PLACER::FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish )
{
    bool fixAll  = Settings().GetFixAllSegments();
    bool realEnd = false;

    LINE pl = Trace();

    if( Settings().Mode() == RM_MarkObstacles )
    {
        // Mark Obstacles is sort of a half-manual, half-automated mode in which the
        // user has more responsibility and authority.

        if( aEndItem )
        {
            // The user has indicated a connection should be made.  If either the trace or
            // endItem is net-less, then allow the connection by adopting the net of the other.
            if( m_router->GetInterface()->GetNetCode( m_currentNet ) <= 0 )
            {
                m_currentNet = aEndItem->Net();
                pl.SetNet( m_currentNet );
            }
            else if( m_router->GetInterface()->GetNetCode( aEndItem->Net() ) <= 0 )
            {
                aEndItem->SetNet( m_currentNet );
            }
        }
    }

    // Collisions still prevent fixing unless "Allow DRC violations" is checked
    // Note that collisions can occur even in walk/shove modes if the beginning of the trace
    // collides (for example if the starting track width is too high).

    if( !Settings().AllowDRCViolations() )
    {
        NODE* checkNode = ( Settings().Mode() == RM_Shove ) ? m_shove->CurrentNode() : m_world;
        std::optional<OBSTACLE> obs = checkNode->CheckColliding( &pl );

        if( obs )
        {
            // TODO: Determine why the shove node sometimes reports collisions against shoved objects.
            // For now, to work around this issue, we consider only solids in shove mode.
            if( Settings().Mode() != RM_Shove || obs->m_item->OfKind( ITEM::SOLID_T ) )
                return false;
        }
    }

    const SHAPE_LINE_CHAIN& l = pl.CLine();

    if( !l.SegmentCount() )
    {
        if( m_lastNode )
        {
            // Do a final optimization to the stored state
            NODE::ITEM_VECTOR removed, added;
            m_lastNode->GetUpdatedItems( removed, added );

            if( !added.empty() && added.back()->Kind() == ITEM::SEGMENT_T )
                simplifyNewLine( m_lastNode, static_cast<SEGMENT*>( added.back() ) );
        }

        // Nothing to commit if we have an empty line
        if( !pl.EndsWithVia() )
            return false;

        ///< @todo Determine what to do if m_lastNode is a null pointer.  I'm guessing return
        ///<       false but someone with more knowledge of the code will need to determine that..
        if( m_lastNode )
        {
            auto newVia = Clone( pl.Via() );
            newVia->ResetUid();
            m_lastNode->Add( std::move( newVia ) );
            m_shove->AddLockedSpringbackNode( m_lastNode );
        }

        m_currentNode = nullptr;

        m_idle = true;
        m_placementCorrect = true;
        return true;
    }

    VECTOR2I p_pre_last = l.CLastPoint();
    const VECTOR2I p_last = l.CLastPoint();

    if( l.PointCount() > 2 )
        p_pre_last = l.CPoints()[ l.PointCount() - 2 ];

    if( aEndItem && m_currentNet && m_currentNet == aEndItem->Net() )
        realEnd = true;

    if( aForceFinish )
        realEnd = true;

    // TODO: Rollback doesn't work properly if fix-all isn't enabled and we are placing arcs,
    // so if we are, act as though we are in fix-all mode.
    if( !fixAll && l.ArcCount() )
        fixAll = true;

    // TODO: lastDirSeg will be calculated incorrectly if we end on an arc
    SEG lastDirSeg = ( !fixAll && l.SegmentCount() > 1 ) ? l.CSegment( -2 ) : l.CSegment( -1 );
    DIRECTION_45 d_last( lastDirSeg );

    int lastV;

    if( realEnd || m_placingVia || fixAll )
        lastV = l.SegmentCount();
    else
        lastV = std::max( 1, l.SegmentCount() - 1 );

    ARC          arc;
    SEGMENT      seg;
    LINKED_ITEM* lastItem = nullptr;
    int          lastArc  = -1;

    for( int i = 0; i < lastV; i++ )
    {
        ssize_t arcIndex = l.ArcIndex( i );

        if( arcIndex < 0 || ( lastArc >= 0 && i == lastV - 1 && !l.IsPtOnArc( lastV ) ) )
        {
            seg = SEGMENT( pl.CSegment( i ), m_currentNet );
            seg.SetWidth( pl.Width() );
            seg.SetLayer( m_currentLayer );

            std::unique_ptr<SEGMENT> sp = std::make_unique<SEGMENT>( seg );
            lastItem = sp.get();

            if( !m_lastNode->Add( std::move( sp ) ) )
                lastItem = nullptr;
        }
        else
        {
            if( arcIndex == lastArc )
                continue;

            arc = ARC( l.Arc( arcIndex ), m_currentNet );
            arc.SetWidth( pl.Width() );
            arc.SetLayer( m_currentLayer );

            std::unique_ptr<ARC> ap = std::make_unique<ARC>( arc );
            lastItem = ap.get();

            if( !m_lastNode->Add( std::move( ap ) ) )
                lastItem = nullptr;

            lastArc  = arcIndex;
        }
    }

    if( pl.EndsWithVia() )
    {
        auto newVia = Clone( pl.Via() );
        newVia->ResetUid();
        m_lastNode->Add( std::move( newVia ) );
    }


    if( realEnd && lastItem )
        simplifyNewLine( m_lastNode, lastItem );

    if( !realEnd )
    {
        setInitialDirection( d_last );
        m_currentStart = ( m_placingVia || fixAll ) ? p_last : p_pre_last;

        m_fixedTail.AddStage( m_fixStart, m_currentLayer, m_placingVia, m_direction, m_currentNode );

        m_fixStart = m_currentStart;
        m_startItem = nullptr;
        m_placingVia = false;
        m_chainedPlacement = !pl.EndsWithVia();

        m_p_start = m_currentStart;
        m_direction = m_initial_direction;

        m_head.Line().Clear();
        m_tail.Line().Clear();
        m_head.RemoveVia();
        m_tail.RemoveVia();
        m_currentNode = m_lastNode;
        m_lastNode = m_lastNode->Branch();

        m_shove->AddLockedSpringbackNode( m_currentNode );

        DIRECTION_45 lastSegDir = pl.EndsWithVia() ? DIRECTION_45::UNDEFINED : d_last;

        m_mouseTrailTracer.Clear();
        m_mouseTrailTracer.SetTolerance( m_head.Width() );
        m_mouseTrailTracer.AddTrailPoint( m_currentStart );
        m_mouseTrailTracer.SetDefaultDirections( lastSegDir, DIRECTION_45::UNDEFINED );

        m_placementCorrect = true;
    }
    else
    {
        m_shove->AddLockedSpringbackNode( m_lastNode );
        m_placementCorrect = true;
        m_idle = true;
    }

    return realEnd;
}


std::optional<VECTOR2I> LINE_PLACER::UnfixRoute()
{
    FIXED_TAIL::STAGE       st;
    std::optional<VECTOR2I> ret;

    if ( !m_fixedTail.PopStage( st ) )
        return ret;

    if( m_head.Line().PointCount() )
        ret = m_head.Line().CPoint( 0 );

    m_head.Line().Clear();
    m_tail.Line().Clear();
    m_startItem = nullptr;
    m_p_start = st.pts[0].p;
    m_fixStart = m_p_start;
    m_direction = st.pts[0].direction;
    m_placingVia = st.pts[0].placingVias;
    m_currentNode = st.commit;
    m_currentLayer = st.pts[0].layer;
    m_currentStart = m_p_start;
    m_head.SetLayer( m_currentLayer );
    m_tail.SetLayer( m_currentLayer );
    m_head.RemoveVia();
    m_tail.RemoveVia();

    m_mouseTrailTracer.Clear();
    m_mouseTrailTracer.SetDefaultDirections( m_initial_direction, m_direction );
    m_mouseTrailTracer.AddTrailPoint( m_p_start );

    m_shove->RewindSpringbackTo( m_currentNode );
    m_shove->UnlockSpringbackNode( m_currentNode );

    if( Settings().Mode() == PNS::RM_Shove )
    {
        m_currentNode = m_shove->CurrentNode();
        m_currentNode->KillChildren();
    }

    m_lastNode = m_currentNode->Branch();

    return ret;
}


bool LINE_PLACER::HasPlacedAnything() const
{
     return m_placementCorrect || m_fixedTail.StageCount() > 1;
}


bool LINE_PLACER::CommitPlacement()
{
    if( Settings().Mode() == PNS::RM_Shove )
    {
        m_shove->RewindToLastLockedNode();
        m_lastNode = m_shove->CurrentNode();
        m_lastNode->KillChildren();
    }

    if( m_lastNode )
        Router()->CommitRouting( m_lastNode );

    m_lastNode = nullptr;
    m_currentNode = nullptr;
    return true;
}


void LINE_PLACER::removeLoops( NODE* aNode, LINE& aLatest )
{
    if( !aLatest.SegmentCount() )
        return;

    if( aLatest.CLine().CPoint( 0 ) == aLatest.CLine().CLastPoint() )
        return;

    std::set<LINKED_ITEM *> toErase;
    aLatest.ClearLinks();
    aNode->Add( aLatest, true );

    for( int s = 0; s < aLatest.LinkCount(); s++ )
    {
        LINKED_ITEM* seg = aLatest.GetLink(s);
        LINE ourLine = aNode->AssembleLine( seg );
        JOINT a, b;
        std::vector<LINE> lines;

        aNode->FindLineEnds( ourLine, a, b );

        if( a == b )
            aNode->FindLineEnds( aLatest, a, b );

        aNode->FindLinesBetweenJoints( a, b, lines );

        int removedCount = 0;
        int total = 0;

        for( LINE& line : lines )
        {
            total++;

            if( !( line.ContainsLink( seg ) ) && line.SegmentCount() )
            {
                // Don't remove locked tracks
                bool hasLockedSegment = false;
                for( LINKED_ITEM* ss : line.Links() )
                {
                    if( ss->IsLocked() )
                    {
                        hasLockedSegment = true;
                        break;
                    }
                }

                if( !hasLockedSegment )
                {
                    for( LINKED_ITEM* ss : line.Links() )
                        toErase.insert( ss );

                    removedCount++;
                }
            }
        }

        PNS_DBG( Dbg(), Message, wxString::Format( "total segs removed: %d/%d", removedCount, total ) );
    }

    for( LINKED_ITEM* s : toErase )
        aNode->Remove( s );

    aNode->Remove( aLatest );
}


void LINE_PLACER::simplifyNewLine( NODE* aNode, LINKED_ITEM* aLatest )
{
    wxASSERT( aLatest->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T ) );

    // Before we assemble the final line and run the optimizer, do a separate pass to clean up
    // colinear segments that exist on non-line-corner joints, as these will prevent proper assembly
    // of the line and won't get cleaned up by the optimizer.
    NODE::ITEM_VECTOR removed, added;
    aNode->GetUpdatedItems( removed, added );

    std::set<ITEM*> cleanup;

    auto processJoint =
            [&]( const JOINT* aJoint, ITEM* aItem )
            {
                if( !aJoint || aJoint->IsLineCorner() )
                    return;

                SEG refSeg = static_cast<SEGMENT*>( aItem )->Seg();

                NODE::ITEM_VECTOR toRemove;

                for( ITEM* neighbor : aJoint->CLinks().CItems() )
                {
                    if( neighbor == aItem
                        || !neighbor->OfKind( ITEM::SEGMENT_T | ITEM::ARC_T )
                        || !neighbor->LayersOverlap( aItem ) )
                    {
                        continue;
                    }

                    if( static_cast<const SEGMENT*>( neighbor )->Width()
                            != static_cast<const SEGMENT*>( aItem )->Width() )
                    {
                        continue;
                    }

                    const SEG& testSeg = static_cast<const SEGMENT*>( neighbor )->Seg();

                    if( refSeg.Contains( testSeg ) )
                    {
                        const JOINT* nA = aNode->FindJoint( neighbor->Anchor( 0 ), neighbor );
                        const JOINT* nB = aNode->FindJoint( neighbor->Anchor( 1 ), neighbor );

                        if( ( nA == aJoint && nB->LinkCount() == 1 ) ||
                            ( nB == aJoint && nA->LinkCount() == 1 ) )
                        {
                            cleanup.insert( neighbor );
                        }
                    }
                    else if( testSeg.Contains( refSeg ) )
                    {
                        const JOINT* aA = aNode->FindJoint( aItem->Anchor( 0 ), aItem );
                        const JOINT* aB = aNode->FindJoint( aItem->Anchor( 1 ), aItem );

                        if( ( aA == aJoint && aB->LinkCount() == 1 ) ||
                            ( aB == aJoint && aA->LinkCount() == 1 ) )
                        {
                            cleanup.insert( aItem );
                            return;
                        }
                    }
                }
            };

    for( ITEM* item : added )
    {
        if( !item->OfKind( ITEM::SEGMENT_T ) || cleanup.count( item ) )
            continue;

        const JOINT* jA = aNode->FindJoint( item->Anchor( 0 ), item );
        const JOINT* jB = aNode->FindJoint( item->Anchor( 1 ), item );

        processJoint( jA, item );
        processJoint( jB, item );
    }

    for( ITEM* seg : cleanup )
        aNode->Remove( seg );

    // And now we can proceed with assembling the final line and optimizing it.

    LINE l_orig = aNode->AssembleLine( aLatest, nullptr, false, false, false );
    LINE l( l_orig );

    bool optimized = OPTIMIZER::Optimize( &l, OPTIMIZER::MERGE_COLINEAR, aNode );

    SHAPE_LINE_CHAIN simplified( l.CLine() );

    simplified.Simplify();

    if( optimized || simplified.PointCount() != l.PointCount() )
    {
        aNode->Remove( l_orig );
        l.SetShape( simplified );
        aNode->Add( l );
        PNS_DBG( Dbg(), AddItem, &l, RED, 100000, wxT("simplified"));
    }
}


void LINE_PLACER::UpdateSizes( const SIZES_SETTINGS& aSizes )
{
    m_sizes = aSizes;

    if( !m_idle )
    {
        // If the track width continues from an existing track, we don't want to change the width.
        // Disallow changing width after the first segment has been fixed because we don't want to
        // go back and rip up tracks or allow DRC errors
        if( m_sizes.TrackWidthIsExplicit()
            || ( !HasPlacedAnything() && ( !m_startItem || m_startItem->Kind() != ITEM::SEGMENT_T ) ) )
        {
            m_head.SetWidth( m_sizes.TrackWidth() );
            m_tail.SetWidth( m_sizes.TrackWidth() );
            m_currentTrace.SetWidth( m_sizes.TrackWidth() );
        }

        if( m_head.EndsWithVia() )
        {
            m_head.SetViaDiameter( m_sizes.ViaDiameter() );
            m_head.SetViaDrill( m_sizes.ViaDrill() );
        }
    }
}


void LINE_PLACER::updateLeadingRatLine()
{
    LINE current = Trace();
    SHAPE_LINE_CHAIN ratLine;
    TOPOLOGY topo( m_lastNode );

    if( topo.LeadingRatLine( &current, ratLine ) )
        m_router->GetInterface()->DisplayRatline( ratLine, m_currentNet );
}


void LINE_PLACER::SetOrthoMode( bool aOrthoMode )
{
    m_orthoMode = aOrthoMode;
}


bool LINE_PLACER::buildInitialLine( const VECTOR2I& aP, LINE& aHead, PNS::PNS_MODE aMode, bool aForceNoVia )
{
    SHAPE_LINE_CHAIN l;
    DIRECTION_45 guessedDir = m_mouseTrailTracer.GetPosture( aP );

    PNS_DBG( Dbg(), Message, wxString::Format( wxT( "buildInitialLine: m_direction %s, guessedDir %s, tail points %d" ),
                                               m_direction.Format(), guessedDir.Format(), m_tail.PointCount() ) );

    DIRECTION_45::CORNER_MODE cornerMode = Settings().GetCornerMode();
    // Rounded corners don't make sense when routing orthogonally (single track at a time)
    if( m_orthoMode )
        cornerMode = DIRECTION_45::CORNER_MODE::MITERED_45;

    PNS_DBG( Dbg(), AddPoint, m_p_start, WHITE, 10000, wxT( "pstart [buildInitial]" ) );


    if( m_p_start == aP )
    {
        l.Clear();
    }
    else
    {
        if( Settings().GetFreeAngleMode() && Settings().Mode() == RM_MarkObstacles )
        {
            l = SHAPE_LINE_CHAIN( { m_p_start, aP } );
        }
        else
        {
            if( !m_tail.PointCount() )
                l = guessedDir.BuildInitialTrace( m_p_start, aP, false, cornerMode );
            else
                l = m_direction.BuildInitialTrace( m_p_start, aP, false, cornerMode );
        }

        if( l.SegmentCount() > 1 && m_orthoMode )
        {
            VECTOR2I newLast = l.CSegment( 0 ).LineProject( l.CLastPoint() );

            l.Remove( -1, -1 );
            l.SetPoint( 1, newLast );
        }
    }

    aHead.SetLayer( m_currentLayer );
    aHead.SetShape( l );

    PNS_DBG( Dbg(), AddItem, &aHead, CYAN, 10000, wxT( "initial-trace" ) );


    if( !m_placingVia || aForceNoVia )
        return true;

    VIA v( makeVia( aP ) );
    v.SetNet( aHead.Net() );

    if( aMode == RM_MarkObstacles )
    {
        aHead.AppendVia( v );
        return true;
    }

    const int collMask = ( aMode == RM_Walkaround ) ? ITEM::ANY_T : ITEM::SOLID_T;
    const int iterLimit = Settings().ViaForcePropIterationLimit();

    for( int attempt = 0; attempt < 2; attempt++)
    {
        VECTOR2I lead = aP - m_p_start;
        VECTOR2I force;

        if( attempt == 1 && m_last_p_end.has_value() )
            lead = aP - m_last_p_end.value();

        if( v.PushoutForce( m_currentNode, lead, force, collMask, iterLimit ) )
        {
            SHAPE_LINE_CHAIN line = guessedDir.BuildInitialTrace( m_p_start, aP + force, false, cornerMode );
            aHead = LINE( aHead, line );

            v.SetPos( v.Pos() + force );

            aHead.AppendVia( v );

            PNS_DBG( Dbg(), AddPoint, v.Pos(), GREEN, 1000000, "via-force-coll-2" );

            return true;
        }
    }

    return false; // via placement unsuccessful
}


void LINE_PLACER::GetModifiedNets( std::vector<NET_HANDLE>& aNets ) const
{
    aNets.push_back( m_currentNet );
}


bool LINE_PLACER::AbortPlacement()
{
    m_world->KillChildren();
    m_lastNode = nullptr;
    return true;
}


FIXED_TAIL::FIXED_TAIL( int aLineCount )
{

}


FIXED_TAIL::~FIXED_TAIL()
{

}


void FIXED_TAIL::Clear()
{
    m_stages.clear();
}


void FIXED_TAIL::AddStage( const VECTOR2I& aStart, int aLayer, bool placingVias,
                           DIRECTION_45 direction, NODE* aNode )
{
    STAGE st;
    FIX_POINT pt;

    pt.p = aStart;
    pt.layer = aLayer;
    pt.direction = direction;
    pt.placingVias = placingVias;

    st.pts.push_back(pt);
    st.commit = aNode;

    m_stages.push_back( std::move( st ) );
}


bool FIXED_TAIL::PopStage( FIXED_TAIL::STAGE& aStage )
{
    if( !m_stages.size() )
        return false;

    aStage = m_stages.back();

    if( m_stages.size() > 1 )
        m_stages.pop_back();

    return true;
}


int FIXED_TAIL::StageCount() const
{
    return m_stages.size();
}

}

