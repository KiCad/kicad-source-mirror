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

#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include <math/vector2d.h>

#include "pns_line.h"
#include "pns_node.h"
#include "pns_via.h"
#include "pns_utils.h"
#include "pns_router.h"

using namespace std;
using boost::optional;

PNS_LINE* PNS_LINE::Clone() const
{
    PNS_LINE* l = new PNS_LINE();

    l->m_line = m_line;
    l->m_width  = m_width;
    l->m_layers = m_layers;
    l->m_net = m_net;
    l->m_movable = m_movable;
    l->m_segmentRefs = NULL;
    l->m_hasVia = m_hasVia;
    l->m_via = m_via;

    return l;
}


PNS_LINE* PNS_LINE::CloneProperties() const
{
    PNS_LINE* l = new PNS_LINE();

    l->m_width  = m_width;
    l->m_layers = m_layers;
    l->m_net = m_net;
    l->m_movable = m_movable;

    return l;
}


PNS_SEGMENT* PNS_SEGMENT::Clone() const
{
    PNS_SEGMENT* s = new PNS_SEGMENT;

    s->m_width = m_width;
    s->m_net = m_net;
    s->m_shape  = m_shape;
    s->m_layers = m_layers;

    return s;    // assert(false);
}


#if 1
bool PNS_LINE::MergeObtuseSegments()
{
    int step = m_line.PointCount() - 3;
    int iter = 0;

    int segs_pre = m_line.SegmentCount();

    if( step < 0 )
        return false;

    SHAPE_LINE_CHAIN current_path( m_line );

    while( 1 )
    {
        iter++;
        int n_segs = current_path.SegmentCount();
        int max_step = n_segs - 2;

        if( step > max_step )
            step = max_step;

        if( step < 2 )
        {
            m_line = current_path;
            return current_path.SegmentCount() < segs_pre;
        }

        bool found_anything = false;
        int n = 0;

        while( n < n_segs - step )
        {
            const SEG s1    = current_path.CSegment( n );
            const SEG s2    = current_path.CSegment( n + step );
            SEG s1opt, s2opt;

            if( DIRECTION_45( s1 ).IsObtuse( DIRECTION_45( s2 ) ) )
            {
                VECTOR2I ip = *s1.IntersectLines( s2 );

                if( s1.Distance( ip ) <= 1 || s2.Distance( ip ) <= 1 )
                {
                    s1opt = SEG( s1.a, ip );
                    s2opt = SEG( ip, s2.b );
                }
                else
                {
                    s1opt = SEG( s1.a, ip );
                    s2opt = SEG( ip, s2.b );
                }


                if( DIRECTION_45( s1opt ).IsObtuse( DIRECTION_45( s2opt ) ) )
                {
                    SHAPE_LINE_CHAIN opt_path;
                    opt_path.Append( s1opt.a );
                    opt_path.Append( s1opt.b );
                    opt_path.Append( s2opt.b );

                    PNS_LINE opt_track( *this, opt_path );

                    if( !m_world->CheckColliding( &opt_track, PNS_ITEM::ANY ) )
                    {
                        current_path.Replace( s1.Index() + 1, s2.Index(), ip );
                        n_segs = current_path.SegmentCount();
                        found_anything = true;
                        break;
                    }
                }
            }

            n++;
        }

        if( !found_anything )
        {
            if( step <= 2 )
            {
                m_line = current_path;
                return m_line.SegmentCount() < segs_pre;
            }

            step--;
        }
    }

    return m_line.SegmentCount() < segs_pre;
}


bool PNS_LINE::MergeSegments()
{
    int step = m_line.PointCount() - 3;
    int iter = 0;

    int segs_pre = m_line.SegmentCount();

    if( step < 0 )
        return false;

    SHAPE_LINE_CHAIN current_path( m_line );

    while( 1 )
    {
        iter++;
        int n_segs = current_path.SegmentCount();
        int max_step = n_segs - 2;

        if( step > max_step )
            step = max_step;

        if( step < 2 )
        {
            m_line = current_path;
            return current_path.SegmentCount() < segs_pre;
        }

        bool found_anything = false;
        int n = 0;

        while( n < n_segs - step )
        {
            const SEG s1 = current_path.CSegment( n );
            const SEG s2 = current_path.CSegment( n + step );
            SEG s1opt, s2opt;

            if( n > 0 )
            {
                SHAPE_LINE_CHAIN path_straight = DIRECTION_45().BuildInitialTrace( s1.a,
                        s2.a,
                        false );
                SHAPE_LINE_CHAIN path_diagonal = DIRECTION_45().BuildInitialTrace( s1.a,
                        s2.a,
                        true );
            }

            if( DIRECTION_45( s1 ) == DIRECTION_45( s2 ) )
            {
                if( s1.Collinear( s2 ) )
                {
                    // printf("Colinear: np %d step %d n1 %d n2 %d\n", n_segs, step, n, n+step);

                    SHAPE_LINE_CHAIN opt_path;
                    opt_path.Append( s1.a );
                    opt_path.Append( s2.b );

                    PNS_LINE tmp( *this, opt_path );

                    if( !m_world->CheckColliding( &tmp, PNS_ITEM::ANY ) )
                    {
                        current_path.Remove( s1.Index() + 1, s2.Index() );
                        n_segs = current_path.SegmentCount();
                        found_anything = true;
                        break;
                    }
                }
            }
            else if( DIRECTION_45( s1 ).IsObtuse( DIRECTION_45( s2 ) ) )
            {
                VECTOR2I ip = *s1.IntersectLines( s2 );

                if( s1.Distance( ip ) <= 1 || s2.Distance( ip ) <= 1 )
                {
                    s1opt = SEG( s1.a, ip );
                    s2opt = SEG( ip, s2.b );
                }
                else
                {
                    s1opt = SEG( s1.a, ip );
                    s2opt = SEG( ip, s2.b );
                }


                if( DIRECTION_45( s1opt ).IsObtuse( DIRECTION_45( s2opt ) ) )
                {
                    SHAPE_LINE_CHAIN opt_path;
                    opt_path.Append( s1opt.a );
                    opt_path.Append( s1opt.b );
                    opt_path.Append( s2opt.b );

                    PNS_LINE opt_track( *this, opt_path );

                    if( !m_world->CheckColliding( &opt_track, PNS_ITEM::ANY ) )
                    {
                        current_path.Replace( s1.Index() + 1, s2.Index(), ip );
                        n_segs = current_path.SegmentCount();
                        found_anything = true;
                        break;
                    }
                }
            }

            n++;
        }

        if( !found_anything )
        {
            if( step <= 2 )
            {
                m_line = current_path;
                return m_line.SegmentCount() < segs_pre;
            }

            step--;
        }
    }

    return m_line.SegmentCount() < segs_pre;
}
#endif


int PNS_LINE::CountCorners( int aAngles )
{
    int count = 0;

    for( int i = 0; i < m_line.SegmentCount() - 1; i++ )
    {
        const SEG seg1 = m_line.CSegment( i );
        const SEG seg2 = m_line.CSegment( i + 1 );

        const DIRECTION_45 dir1( seg1 );
        const DIRECTION_45 dir2( seg2 );

        DIRECTION_45::AngleType a = dir1.Angle( dir2 );

        if( a & aAngles )
            count++;
    }

    return count;
}


// #define DUMP_TEST_CASES

// fixme: damn f*****g inefficient and incredibly crappily written
void PNS_LINE::NewWalkaround( const SHAPE_LINE_CHAIN& aObstacle,
        SHAPE_LINE_CHAIN& aPrePath,
        SHAPE_LINE_CHAIN& aWalkaroundPath,
        SHAPE_LINE_CHAIN& aPostPath,
        bool aCw ) const
{
    typedef SHAPE_LINE_CHAIN::Intersection Intersection;

    SHAPE_LINE_CHAIN l_orig( m_line );
    SHAPE_LINE_CHAIN l_hull;
    vector<bool> outside, on_edge, inside;
    SHAPE_LINE_CHAIN path;

    vector<Intersection> isects;

    // don't calculate walkaround for empty lines
    if( m_line.PointCount() < 2 )
        return;

#ifdef DUMP_TEST_CASES
    printf( "%s\n", m_line.Format().c_str() );
    printf( "%s\n", aObstacle.Format().c_str() );
#endif

    aObstacle.Intersect( m_line, isects );

    // printf("NewWalk intersectiosn :%d\n" ,isects.size());
    if( !aCw )
        l_hull = aObstacle.Reverse();
    else
        l_hull = aObstacle;

    BOOST_FOREACH( Intersection isect, isects ) {
        l_orig.Split( isect.p );
        l_hull.Split( isect.p );
    }


#ifdef DUMP_TEST_CASES
    printf( "%s\n", m_line.Format().c_str() );
    printf( "%s\n", aObstacle.Format().c_str() );
    printf( "%s\n", l_orig.Format().c_str() );
    printf( "%s\n", l_hull.Format().c_str() );
#endif


    // printf("Pts: line %d hull %d\n", l_orig.PointCount(), l_hull.PointCount());

    int first_post = -1;
    int last_pre = -1;

    for( int i = 0; i < l_orig.PointCount(); i++ )
    {
        int ei = l_hull.Find( l_orig.CPoint( i ) );
        bool edge   = ei >= 0;
        bool in     = l_hull.PointInside( l_orig.CPoint( i ) ) && !edge;
        bool out    = !( in || edge);

        outside.push_back( out );
        on_edge.push_back( edge );
        inside.push_back( in );
    }

    for( int i = l_orig.PointCount() - 1; i >= 1; i-- )
        if( inside[i] && outside[i - 1] )
        {
            SHAPE_LINE_CHAIN::Intersections ips;
            l_hull.Intersect( SEG( l_orig.CPoint( i ), l_orig.CPoint( i - 1 ) ), ips );
            l_orig.Remove( i, -1 );
            l_orig.Append( ips[0].p );
            break;
        }
        else if( inside[i] && on_edge[i - 1] )
        {
            l_orig.Remove( i, -1 );
            // n = i;
        }
        else if( !inside[i] )
            break;

    if( !outside.size() && on_edge.size() < 2 )
        return;

    for( int i = 0; i < l_orig.PointCount(); i++ )
    {
        const VECTOR2I p = l_orig.Point( i );

        if( outside[i] || ( on_edge[i] && i == ( l_orig.PointCount() - 1 ) ) )
        {
            if( last_pre < 0 )
                aPrePath.Append( p );

            path.Append( p );
        }
        else if( on_edge[i] )
        {
            int li = -1;

            if( last_pre < 0 )
            {
                aPrePath.Append( p );
                last_pre = path.PointCount();
            }

            if( i == l_orig.PointCount() - 1 || outside[i + 1] )
            {
                path.Append( p );
            }
            else
            {
                int vi2 = l_hull.Find( l_orig.CPoint( i ) );

                path.Append( l_hull.CPoint( vi2 ) );

                for( int j = (vi2 + 1) % l_hull.PointCount();
                     j != vi2;
                     j = (j + 1) % l_hull.PointCount() )
                {
                    path.Append( l_hull.CPoint( j ) );
                    li = l_orig.Find( l_hull.CPoint( j ) );

                    if( li >= 0 && ( li == (l_orig.PointCount() - 1 ) || 
                                outside[li + 1]) )
                        break;
                }

                if( li >= 0 )
                {
                    if( i >= li )
                        break;
                    else
                        i = li;
                }
            }

            first_post = path.PointCount() - 1;
        }
    }

    if( last_pre < 0 && first_post < 0 )
        return;

    aWalkaroundPath = path.Slice( last_pre, first_post );

    if( first_post >= 0 )
        aPostPath = path.Slice( first_post, -1 );
}


bool PNS_LINE::onEdge( const SHAPE_LINE_CHAIN& obstacle, VECTOR2I p, int& ei,
        bool& is_vertex ) const
{
    int vtx = obstacle.Find( p );

    if( vtx >= 0 )
    {
        ei = vtx;
        is_vertex = true;
        return true;
    }

    for( int s = 0; s < obstacle.SegmentCount(); s++ )
    {
        if( obstacle.CSegment( s ).Contains( p ) )
        {
            ei = s;
            is_vertex = false;
            return true;
        }
    }

    return false;
}


bool PNS_LINE::walkScan( const SHAPE_LINE_CHAIN& line,
        const SHAPE_LINE_CHAIN& obstacle,
        bool reverse,
        VECTOR2I& ip,
        int& index_o,
        int& index_l,
        bool& is_vertex ) const
{
    int sc = line.SegmentCount();

    for( int i = 0; i < line.SegmentCount(); i++ )
    {
        printf( "check-seg rev %d %d/%d %d\n", reverse, i, sc, sc - 1 - i );
        SEG tmp = line.CSegment( reverse ? sc - 1 - i : i );
        SEG s( tmp.a, tmp.b );

        if( reverse )
        {
            s.a = tmp.b;
            s.b = tmp.a;
        }

        if( onEdge( obstacle, s.a, index_o, is_vertex ) )
        {
            index_l = (reverse ?  sc - 1 - i : i);
            ip = s.a;
            printf( "vertex %d on-%s %d\n", index_l, 
                    is_vertex ? "vertex" : "edge", index_o );
            return true;
        }

        if( onEdge( obstacle, s.b, index_o, is_vertex ) )
        {
            index_l = (reverse ?  sc - 1 - i - 1 : i + 1);
            ip = s.b;
            printf( "vertex %d on-%s %d\n", index_l, 
                    is_vertex ? "vertex" : "edge", index_o );
            return true;
        }

        SHAPE_LINE_CHAIN::Intersections ips;
        int n_is = obstacle.Intersect( s, ips );

        if( n_is > 0 )
        {
            index_o = ips[0].our.Index();
            index_l = reverse ? sc - 1 - i : i;
            printf( "segment-%d intersects edge-%d\n", index_l, index_o );
            ip = ips[0].p;
            return true;
        }
    }

    return false;
}


bool PNS_LINE::Walkaround( SHAPE_LINE_CHAIN obstacle,
        SHAPE_LINE_CHAIN& pre,
        SHAPE_LINE_CHAIN& walk,
        SHAPE_LINE_CHAIN& post,
        bool cw ) const
{
    const SHAPE_LINE_CHAIN& line = GetCLine();
    VECTOR2I ip_start;
    int index_o_start, index_l_start;
    VECTOR2I ip_end;
    int index_o_end, index_l_end;

    bool is_vertex_start, is_vertex_end;

    if( line.SegmentCount() < 1 )
        return false;

    if( obstacle.PointInside( line.CPoint( 0 ) ) || 
            obstacle.PointInside( line.CPoint( -1 ) ) )
        return false;

// printf("forward:\n");
    bool found = walkScan( line,
            obstacle,
            false,
            ip_start,
            index_o_start,
            index_l_start,
            is_vertex_start );
    // printf("reverse:\n");
    found |= walkScan( line, obstacle, true, ip_end, index_o_end, index_l_end, is_vertex_end );

    if( !found || ip_start == ip_end )
    {
        pre = line;
        return true;
    }

    pre = line.Slice( 0, index_l_start );
    pre.Append( ip_start );
    walk.Clear();
    walk.Append( ip_start );

    if( cw )
    {
        int is = ( index_o_start + 1 ) % obstacle.PointCount();
        int ie = ( is_vertex_end ? index_o_end : index_o_end + 1 ) % obstacle.PointCount();

        while( 1 )
        {
            printf( "is %d\n", is );
            walk.Append( obstacle.CPoint( is ) );

            if( is == ie )
                break;

            is++;

            if( is == obstacle.PointCount() )
                is = 0;
        }
    }
    else
    {
        int is = index_o_start;
        int ie = ( is_vertex_end ? index_o_end : index_o_end ) % obstacle.PointCount();

        while( 1 )
        {
            printf( "is %d\n", is );
            walk.Append( obstacle.CPoint( is ) );

            if( is == ie )
                break;

            is--;

            if( is < 0 )
                is = obstacle.PointCount() - 1;
        }
    }

    walk.Append( ip_end );

    post.Clear();
    post.Append( ip_end );
    post.Append( line.Slice( is_vertex_end ? index_l_end : index_l_end + 1, -1 ) );

    // for(int i = (index_o_start + 1) % obstacle.PointCount();
    // i != (index_o_end + 1) % obstacle.PointCount(); i=(i+1) % obstacle.PointCount())
    // {
    // printf("append %d\n", i);
    // walk.Append(obstacle.CPoint(i));
    // }

    return true;
}


void PNS_LINE::NewWalkaround( const SHAPE_LINE_CHAIN& aObstacle,
        SHAPE_LINE_CHAIN& aPath,
        bool aCw ) const
{
    SHAPE_LINE_CHAIN walk, post;

    NewWalkaround( aObstacle, aPath, walk, post, aCw );
    aPath.Append( walk );
    aPath.Append( post );
    aPath.Simplify();
}


void PNS_LINE::Walkaround( const SHAPE_LINE_CHAIN& aObstacle,
        SHAPE_LINE_CHAIN& aPath,
        bool aCw ) const
{
    SHAPE_LINE_CHAIN walk, post;

    Walkaround( aObstacle, aPath, walk, post, aCw );
    aPath.Append( walk );
    aPath.Append( post );
    aPath.Simplify();
}


const SHAPE_LINE_CHAIN PNS_SEGMENT::Hull( int aClearance, int aWalkaroundThickness ) const
{
    int d = aClearance + 10;
    int x = (int)( 2.0 / ( 1.0 + M_SQRT2 ) * d ) + 2;

    const VECTOR2I a = m_shape.CPoint( 0 );
    const VECTOR2I b = m_shape.CPoint( 1 );

    VECTOR2I dir = b - a;

    VECTOR2I p0 = dir.Perpendicular().Resize( d );

    VECTOR2I ds = dir.Perpendicular().Resize( x / 2 );
    VECTOR2I pd = dir.Resize( x / 2 );
    VECTOR2I dp = dir.Resize( d );

    SHAPE_LINE_CHAIN s;

    s.SetClosed( true );

    s.Append( b + p0 + pd );
    s.Append( b + dp + ds );
    s.Append( b + dp - ds );
    s.Append( b - p0 + pd );
    s.Append( a - p0 - pd );
    s.Append( a - dp - ds );
    s.Append( a - dp + ds );
    s.Append( a + p0 - pd );

    // make sure the hull outline is always clockwise
    if( s.CSegment( 0 ).Side( a ) < 0 )
        return s.Reverse();
    else
        return s;
}


bool PNS_LINE::Is45Degree()
{
    for( int i = 0; i < m_line.SegmentCount(); i++ )
    {
        const SEG& s = m_line.CSegment( i );

        double angle = 180.0 / M_PI *
                       atan2( (double) s.b.y - (double) s.a.y, 
                              (double) s.b.x - (double) s.a.x );

        if( angle < 0 )
            angle += 360.0;

        double angle_a = fabs( fmod( angle, 45.0 ) );

        if( angle_a > 1.0 && angle_a < 44.0 )
            return false;
    }

    return true;
}


const PNS_LINE PNS_LINE::ClipToNearestObstacle( PNS_NODE* aNode ) const
{
    PNS_LINE l( *this );

    PNS_NODE::OptObstacle obs = aNode->NearestObstacle( &l );

    if( obs )
    {
        l.RemoveVia();
        int p = l.GetLine().Split( obs->ip_first );
        l.GetLine().Remove( p + 1, -1 );
    }

    return l;
}


void PNS_LINE::ShowLinks()
{
    if( !m_segmentRefs )
    {
        printf( "line %p: no links\n", this );
        return;
    }

    printf( "line %p: %d linked segs\n", this, (int)m_segmentRefs->size() );

    for( int i = 0; i < (int) m_segmentRefs->size(); i++ )
        printf( "seg %d: %p\n", i, (*m_segmentRefs)[i] );
}

