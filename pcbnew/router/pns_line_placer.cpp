/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "trace.h"

#include "pns_node.h"
#include "pns_line_placer.h"
#include "pns_walkaround.h"
#include "pns_shove.h"
#include "pns_utils.h"

using namespace std;
using boost::optional;

PNS_LINE_PLACER::PNS_LINE_PLACER( PNS_NODE* aWorld )
{
    m_initial_direction = DIRECTION_45( DIRECTION_45::N );
    m_follow_mouse = false;
    m_smoothing_step = 100000;
    m_smooth_mouse = false;
    m_iteration = 0;
    m_world = aWorld;
    m_mode  = RM_Smart;
    m_follow_mouse = true;
    m_shove = NULL;
};


PNS_LINE_PLACER::~PNS_LINE_PLACER()
{
    if( m_shove )
        delete m_shove;
}


void PNS_LINE_PLACER::ApplySettings( const PNS_ROUTING_SETTINGS& aSettings )
{
    m_follow_mouse = aSettings.m_followMouse;
    m_mode = aSettings.m_routingMode;
    m_walkaroundIterationLimit = aSettings.m_walkaroundIterationLimit;
    m_smartPads = aSettings.m_smartPads;
}


void PNS_LINE_PLACER::StartPlacement( const VECTOR2I& aStart, int aNet,
        int aWidth, int aLayer )
{
    m_direction = m_initial_direction;
    TRACE( 1, "world %p, intitial-direction %s layer %d\n",
            m_world % m_direction.Format().c_str() % aLayer );
    m_head.SetNet( aNet );
    m_tail.SetNet( aNet );
    m_head.SetWidth( aWidth );
    m_tail.SetWidth( aWidth );
    m_head.GetLine().Clear();
    m_tail.GetLine().Clear();
    m_head.SetLayer( aLayer );
    m_tail.SetLayer( aLayer );
    m_iteration = 0;
    m_p_start = aStart;
    m_currentNode = m_world->Branch();
    m_head.SetWorld( m_currentNode );
    m_tail.SetWorld( m_currentNode );
    // if(m_shove)
    // delete m_shove;
    m_shove = new PNS_SHOVE( m_currentNode );
    m_placingVia = false;
}


void PNS_LINE_PLACER::SetInitialDirection( const DIRECTION_45& aDirection )
{
    m_initial_direction = aDirection;

    if( m_tail.GetCLine().SegmentCount() == 0 )
        m_direction = aDirection;
}


bool PNS_LINE_PLACER::handleSelfIntersections()
{
    SHAPE_LINE_CHAIN::INTERSECTIONS ips;
    SHAPE_LINE_CHAIN& head = m_head.GetLine();
    SHAPE_LINE_CHAIN& tail = m_tail.GetLine();

    // if there is no tail, there is nothing to intersect with
    if( tail.PointCount() < 2 )
        return false;

    tail.Intersect( head, ips );

    // no intesection points - nothing to reduce
    if( ips.empty() )
        return false;

    int n = INT_MAX;
    VECTOR2I ipoint;

    // if there is more than one intersection, find the one that is
    // closest to the beginning of the tail.
    BOOST_FOREACH( SHAPE_LINE_CHAIN::INTERSECTION i, ips )
    {
        if( i.our.Index() < n )
        {
            n = i.our.Index();
            ipoint = i.p;
        }
    }

    // ignore the point where head and tail meet
    if( ipoint == head.CPoint( 0 ) || ipoint == tail.CPoint( -1 ) )
        return false;

    // Intersection point is on the first or the second segment: just start routing
    // from the beginning
    if( n < 2 )
    {
        m_p_start = tail.Point( 0 );
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
        m_p_start = last.A;
        m_direction = DIRECTION_45( last );
        tail.Remove( n, -1 );
        return true;
    }

    return false;
}


bool PNS_LINE_PLACER::handlePullback()
{
    SHAPE_LINE_CHAIN& head = m_head.GetLine();
    SHAPE_LINE_CHAIN& tail = m_tail.GetLine();

    int n = tail.PointCount();

    if( n == 0 )
        return false;
    else if( n == 1 )
    {
        m_p_start = tail.CPoint( 0 );
        tail.Clear();
        return true;
    }

    DIRECTION_45 first_head( head.Segment( 0 ) );
    DIRECTION_45 last_tail( tail.Segment( -1 ) );
    DIRECTION_45::AngleType angle = first_head.Angle( last_tail );

    // case 1: we have a defined routing direction, and the currently computed
    // head goes in different one.
    bool pullback_1 = false;    // (m_direction != DIRECTION_45::UNDEFINED && m_direction != first_head);

    // case 2: regardless of the current routing direction, if the tail/head
    // extremities form an acute or right angle, reduce the tail by one segment
    // (and hope that further iterations) will result with a cleaner trace
    bool pullback_2 = (angle == DIRECTION_45::ANG_RIGHT || angle == DIRECTION_45::ANG_ACUTE);

    if( pullback_1 || pullback_2 )
    {
        const SEG last = tail.CSegment( -1 );
        m_direction = DIRECTION_45( last );
        m_p_start = last.A;

        TRACE( 0, "Placer: pullback triggered [%d] [%s %s]",
                n % last_tail.Format().c_str() % first_head.Format().c_str() );

        // erase the last point in the tail, hoping that the next iteration will
        // result with a head trace that starts with a segment following our
        // current direction.
        if( n < 2 )
            tail.Clear(); // don't leave a single-point tail
        else
            tail.Remove( -1, -1 );

        if( !tail.SegmentCount() )
            m_direction = m_initial_direction;

        return true;
    }

    return false;
}


bool PNS_LINE_PLACER::reduceTail( const VECTOR2I& aEnd )
{
    SHAPE_LINE_CHAIN& head = m_head.GetLine();
    SHAPE_LINE_CHAIN& tail = m_tail.GetLine();

    int n = tail.SegmentCount();

    // Don't attempt this for too short tails
    if( n < 2 )
        return false;

    // Start from the segment farthest from the end of the tail
    // int start_index = std::max(n - 1 - ReductionDepth, 0);

    DIRECTION_45 new_direction;
    VECTOR2I new_start;
    int reduce_index = -1;

    DIRECTION_45 head_dir( head.Segment( 0 ) );

    for( int i = tail.SegmentCount() - 1; i >= 0; i-- )
    {
        const SEG s = tail.CSegment( i );
        DIRECTION_45 dir( s );

        // calculate a replacement route and check if it matches
        // the direction of the segment to be replaced
        SHAPE_LINE_CHAIN replacement = dir.BuildInitialTrace( s.A, aEnd );

        PNS_LINE tmp( m_tail, replacement );

        if( m_currentNode->CheckColliding( &tmp, PNS_ITEM::ANY ) )
            break;

        if( DIRECTION_45( replacement.Segment( 0 ) ) == dir )
        {
            new_start = s.A;
            new_direction = dir;
            reduce_index = i;
        }
    }

    if( reduce_index >= 0 )
    {
        TRACE( 0, "Placer: reducing tail: %d", reduce_index );
        SHAPE_LINE_CHAIN reducedLine = new_direction.BuildInitialTrace( new_start, aEnd );

        m_p_start = new_start;
        m_direction = new_direction;
        tail.Remove( reduce_index + 1, -1 );
        head.Clear();
        return true;
    }

    if( !tail.SegmentCount() )
        m_direction = m_initial_direction;

    return false;
}


bool PNS_LINE_PLACER::checkObtusity( const SEG& a, const SEG& b ) const
{
    const DIRECTION_45 dir_a( a );
    const DIRECTION_45 dir_b( b );

    return dir_a.IsObtuse( dir_b ) || dir_a == dir_b;
}


bool PNS_LINE_PLACER::mergeHead()
{
    SHAPE_LINE_CHAIN& head = m_head.GetLine();
    SHAPE_LINE_CHAIN& tail = m_tail.GetLine();

    const int ForbiddenAngles = DIRECTION_45::ANG_ACUTE |
                                DIRECTION_45::ANG_HALF_FULL |
                                DIRECTION_45::ANG_UNDEFINED;

    head.Simplify();
    tail.Simplify();

    int n_head  = head.SegmentCount();
    int n_tail  = tail.SegmentCount();

    if( n_head < 3 )
    {
        TRACEn( 4, "Merge failed: not enough head segs." );
        return false;
    }

    if( n_tail && head.CPoint( 0 ) != tail.CPoint( -1 ) )
    {
        TRACEn( 4, "Merge failed: head and tail discontinuous." );
        return false;
    }

    if( m_head.CountCorners( ForbiddenAngles ) != 0 )
        return false;

    DIRECTION_45 dir_tail, dir_head;

    dir_head = DIRECTION_45( head.CSegment( 0 ) );

    if( n_tail )
    {
        dir_tail = DIRECTION_45( tail.CSegment( -1 ) );

        if( dir_head.Angle( dir_tail ) & ForbiddenAngles )
            return false;
    }

    if( !n_tail )
        tail.Append( head.CSegment( 0 ).A );

    for( int i = 0; i < n_head - 2; i++ )
    {
        tail.Append( head.CSegment( i ).B );
    }

    tail.Simplify();

    SEG last = tail.CSegment( -1 );

    m_p_start = last.B;
    m_direction = DIRECTION_45( last ).Right();

    head.Remove( 0, n_head - 2 );

    TRACE( 0, "Placer: merge %d, new direction: %s", n_head % m_direction.Format().c_str() );

    head.Simplify();
    tail.Simplify();

    return true;
}


bool PNS_LINE_PLACER::handleViaPlacement( PNS_LINE& aHead )
{
    if( !m_placingVia )
        return true;

    PNS_LAYERSET allLayers( 0, 15 );
    PNS_VIA v( aHead.GetCLine().CPoint( -1 ), allLayers, m_viaDiameter, aHead.GetNet() );
    v.SetDrill( m_viaDrill );

    VECTOR2I force;
    VECTOR2I lead = aHead.GetCLine().CPoint( -1 ) - aHead.GetCLine().CPoint( 0 );

    if( v.PushoutForce( m_shove->GetCurrentNode(), lead, force, true, 20 ) )
    {
        SHAPE_LINE_CHAIN line = m_direction.BuildInitialTrace(
                aHead.GetCLine().CPoint( 0 ),
                aHead.GetCLine().CPoint( -1 ) + force );
        aHead = PNS_LINE( aHead, line );

        v.SetPos( v.GetPos() + force );
        return true;
    }

    return false;
}


bool PNS_LINE_PLACER::routeHead( const VECTOR2I& aP, PNS_LINE& aNewHead,
        bool aCwWalkaround )
{
    // STAGE 1: route a simple two-segment trace between m_p_start and aP...
    SHAPE_LINE_CHAIN line = m_direction.BuildInitialTrace( m_p_start, aP );

    PNS_LINE initTrack( m_head, line );
    PNS_LINE walkFull, walkSolids;

    if( m_mode == RM_Ignore )
    {
        aNewHead = initTrack;
        return true;
    }

    handleViaPlacement( initTrack );

    m_currentNode = m_shove->GetCurrentNode();

    PNS_OPTIMIZER optimizer( m_currentNode );
    PNS_WALKAROUND walkaround( m_currentNode );

    walkaround.SetSolidsOnly( false );
    walkaround.SetIterationLimit( m_mode == RM_Walkaround ? 8 : 5 );
    // walkaround.SetApproachCursor(true, aP);

    PNS_WALKAROUND::WalkaroundStatus wf = walkaround.Route( initTrack, walkFull );

#if 0

    if( m_mode == RM_Walkaround )
    {
        // walkaround.
// PNSDisplayDebugLine (walkFull.GetCLine(), 4);

        if( wf == PNS_WALKAROUND::STUCK )
        {
            aNewHead = m_head;
            aNewHead.SetShape( walkFull.GetCLine() );
            aNewHead = aNewHead.ClipToNearestObstacle( m_currentNode );
            return false;
        }

        aNewHead = m_head;
        aNewHead.SetShape( walkFull.GetCLine() );

// printf("nh w %d l %d\n", aNewHead.GetWidth(), aNewHead.GetLayers().Start());
        return true;
    }

#endif

    PNS_COST_ESTIMATOR cost_walk, cost_orig;

    walkaround.SetApproachCursor( false, aP );
    walkaround.SetSolidsOnly( true );
    walkaround.SetIterationLimit( 10 );
    PNS_WALKAROUND::WalkaroundStatus stat_solids = walkaround.Route( initTrack, walkSolids );

    optimizer.SetEffortLevel( PNS_OPTIMIZER::MERGE_SEGMENTS );
    optimizer.SetCollisionMask( PNS_ITEM::SOLID );
    optimizer.Optimize( &walkSolids );
    #if 0
    optimizer.SetCollisionMask( -1 );
    optimizer.Optimize( &walkFull );
    #endif
    cost_orig.Add( initTrack );
    cost_walk.Add( walkFull );

    if( m_mode == RM_Smart || m_mode == RM_Shove )
    {
        PNS_LINE l2;

        bool walk_better = cost_orig.IsBetter( cost_walk, 1.5, 10.0 );
        walk_better = false;

#if 0
        printf( "RtTrk width %d %d %d", initTrack.GetWidth(),
                walkFull.GetWidth(), walkSolids.GetWidth() );
        printf( "init-coll %d\n", m_currentNode->CheckColliding( &initTrack ) ? 1 : 0 );
        printf( "total cost: walk cor %.0f len %.0f orig cor %.0f len %.0f walk-better %d\n",
                cost_walk.GetCornerCost(), cost_walk.GetLengthCost(),
                cost_orig.GetCornerCost(), cost_orig.GetLengthCost(),
                walk_better );
#endif

        if( m_mode == RM_Smart && wf == PNS_WALKAROUND::DONE && walk_better
            && walkFull.GetCLine().CPoint( -1 ) == initTrack.GetCLine().CPoint( -1 ) )
            l2 = walkFull;
        else if( stat_solids == PNS_WALKAROUND::DONE )
            l2 = walkSolids;
        else
            l2 = initTrack.ClipToNearestObstacle( m_shove->GetCurrentNode() );

        PNS_LINE l( m_tail );
        l.GetLine().Append( l2.GetCLine() );
        l.GetLine().Simplify();

        if( m_placingVia )
        {
            PNS_LAYERSET allLayers( 0, 15 );
            PNS_VIA v1( l.GetCLine().CPoint( -1 ), allLayers, m_viaDiameter );
            PNS_VIA v2( l2.GetCLine().CPoint( -1 ), allLayers, m_viaDiameter );
            v1.SetDrill( m_viaDrill );
            v2.SetDrill( m_viaDrill );

            l.AppendVia( v1 );
            l2.AppendVia( v2 );
        }

        PNS_SHOVE::ShoveStatus status = m_shove->ShoveLines( &l );
        m_currentNode = m_shove->GetCurrentNode();

        if( status == PNS_SHOVE::SH_OK )
        {
            optimizer.SetWorld( m_currentNode );
            optimizer.ClearCache();
            optimizer.SetEffortLevel( PNS_OPTIMIZER::MERGE_OBTUSE | PNS_OPTIMIZER::SMART_PADS );
            optimizer.SetCollisionMask( -1 );
            optimizer.Optimize( &l2 );

            aNewHead = l2;

            return true;
        }
        else
        {
            walkaround.SetWorld( m_currentNode );
            walkaround.SetSolidsOnly( false );
            walkaround.SetIterationLimit( 10 );
            walkaround.SetApproachCursor( true, aP );
            walkaround.Route( initTrack, l2 );
            aNewHead = l2.ClipToNearestObstacle( m_shove->GetCurrentNode() );
            // aNewHead = l2;

            return false;
        }
    }

    return false;
}


bool PNS_LINE_PLACER::optimizeTailHeadTransition()
{
    SHAPE_LINE_CHAIN& head = m_head.GetLine();
    SHAPE_LINE_CHAIN& tail = m_tail.GetLine();

    const int TailLookbackSegments = 5;

    int threshold = min( tail.PointCount(), TailLookbackSegments + 1 );

    if( tail.SegmentCount() < 3 )
        return false;

    // assemble TailLookbackSegments tail segments with the current head
    SHAPE_LINE_CHAIN opt_line = tail.Slice( -threshold, -1 );

    opt_line.Append( head );
// opt_line.Simplify();

    PNS_LINE new_head( m_tail, opt_line );

    // and see if it could be made simpler by merging obtuse/collnear segments.
    // If so, replace the (threshold) last tail points and the head with
    // the optimized line

    // if(PNS_OPTIMIZER::Optimize(&new_head, PNS_OPTIMIZER::MERGE_SEGMENTS))

    if( new_head.MergeSegments() )
    {
        PNS_LINE tmp( m_tail, opt_line );

        TRACE( 0, "Placer: optimize tail-head [%d]", threshold );

        head.Clear();
        tail.Replace( -threshold, -1, new_head.GetCLine() );
        tail.Simplify();

        m_p_start = new_head.GetCLine().CPoint( -1 );
        m_direction = DIRECTION_45( new_head.GetCLine().CSegment( -1 ) );

        return true;
    }

    return false;
}


void PNS_LINE_PLACER::routeStep( const VECTOR2I& aP )
{
    bool fail = false;
    bool go_back = false;

    int i, n_iter = 1;

    PNS_LINE new_head;

    m_follow_mouse = true;

    TRACE( 2, "INIT-DIR: %s head: %d, tail: %d segs\n",
            m_initial_direction.Format().c_str() % m_head.GetCLine().SegmentCount() %
            m_tail.GetCLine().SegmentCount() );

    for( i = 0; i < n_iter; i++ )
    {
        if( !go_back && m_follow_mouse )
            reduceTail( aP );

        go_back = false;

        if( !routeHead( aP, new_head, true ) )
            fail = true;

        if( !new_head.Is45Degree() )
            fail = true;

        if( !m_follow_mouse )
            return;

        m_head = new_head;

        if( handleSelfIntersections() )
        {
            n_iter++;
            go_back = true;
        }

        if( !go_back && handlePullback() )
        {
            n_iter++;
            go_back = true;
        }
    }

    if( !fail )
    {
        if( optimizeTailHeadTransition() )
            return;

        mergeHead();
    }
}


bool PNS_LINE_PLACER::Route( const VECTOR2I& aP )
{
    if( m_smooth_mouse )
    {
        VECTOR2I p_cur = m_p_start;
        VECTOR2I step = (aP - m_p_start).Resize( m_smoothing_step );

        do
        {
            if( (p_cur - aP).EuclideanNorm() <= m_smoothing_step )
                p_cur = aP;
            else
                p_cur += step;

            routeStep( p_cur );
        } while( p_cur != aP );
    }
    else
        routeStep( aP );

    return CurrentEnd() == aP;
}


const PNS_LINE PNS_LINE_PLACER::GetTrace() const
{
    PNS_LINE tmp( m_head );

    tmp.SetShape( m_tail.GetCLine() );
    tmp.GetLine().Append( m_head.GetCLine() );
    tmp.GetLine().Simplify();
    return tmp;
}


void PNS_LINE_PLACER::FlipPosture()
{
    m_initial_direction = m_initial_direction.Right();
    m_direction = m_direction.Right();
}


void PNS_LINE_PLACER::GetUpdatedItems( PNS_NODE::ItemVector& aRemoved,
        PNS_NODE::ItemVector& aAdded )
{
    return m_shove->GetCurrentNode()->GetUpdatedItems( aRemoved, aAdded );
}


PNS_NODE* PNS_LINE_PLACER::GetCurrentNode() const
{
    return m_shove->GetCurrentNode();
}
