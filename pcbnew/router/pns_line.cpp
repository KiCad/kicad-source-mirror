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

#include <math/box2.h>
#include <math/vector2d.h>

#include "pns_line.h"
#include "pns_node.h"
#include "pns_via.h"
#include "pns_utils.h"
#include "pns_router.h"
#include "pns_debug_decorator.h"

#include <geometry/shape_rect.h>

namespace PNS {

LINE::LINE( const LINE& aOther ) :
        LINK_HOLDER( aOther ),
        m_line( aOther.m_line ),
        m_width( aOther.m_width ),
        m_snapThreshhold( aOther.m_snapThreshhold )
{
    m_net = aOther.m_net;
    m_movable = aOther.m_movable;
    m_layers = aOther.m_layers;

    m_via = nullptr;

    if( aOther.m_via )
    {
        if( aOther.m_via->BelongsTo( &aOther ) )
        {
            m_via = aOther.m_via->Clone();
            m_via->SetOwner( this );
            m_via->SetNet( m_net );
        }
        else
        {
            m_via = aOther.m_via;
        }
    }

    m_marker = aOther.m_marker;
    m_rank = aOther.m_rank;
    m_blockingObstacle = aOther.m_blockingObstacle;

    copyLinks( &aOther );
}


LINE::~LINE()
{
    if( m_via && m_via->BelongsTo( this ) )
        delete m_via;
}


LINE& LINE::operator=( const LINE& aOther )
{
    m_parent = aOther.m_parent;
    m_sourceItem = aOther.m_sourceItem;

    m_line = aOther.m_line;
    m_width = aOther.m_width;
    m_net = aOther.m_net;
    m_movable = aOther.m_movable;
    m_layers = aOther.m_layers;

    m_via = nullptr;

    if( aOther.m_via )
    {
        if( aOther.m_via->BelongsTo( &aOther ) )
        {
            m_via = aOther.m_via->Clone();
            m_via->SetOwner( this );
            m_via->SetNet( m_net );
        }
        else
        {
            m_via = aOther.m_via;
        }
    }

    m_marker = aOther.m_marker;
    m_rank = aOther.m_rank;
    m_routable = aOther.m_routable;
    m_owner = aOther.m_owner;
    m_snapThreshhold = aOther.m_snapThreshhold;
    m_blockingObstacle = aOther.m_blockingObstacle;

    copyLinks( &aOther );

    return *this;
}


LINE& LINE::operator=( LINE&& aOther ) noexcept
{
   if (this != &aOther)
   {
       m_parent = aOther.m_parent;
       m_sourceItem = aOther.m_sourceItem;

       m_line = std::move( aOther.m_line );
       m_width = aOther.m_width;
       m_net = aOther.m_net;
       m_movable = aOther.m_movable;
       m_layers = aOther.m_layers;

       m_via = nullptr;

       if( aOther.m_via )
       {
           if( aOther.m_via->BelongsTo( &aOther ) )
           {
               m_via = aOther.m_via->Clone();
               m_via->SetOwner( this );
               m_via->SetNet( m_net );
           }
           else
           {
               m_via = aOther.m_via;
           }
       }

       m_marker = aOther.m_marker;
       m_rank = aOther.m_rank;
       m_routable = aOther.m_routable;
       m_owner = aOther.m_owner;
       m_snapThreshhold = aOther.m_snapThreshhold;
       m_blockingObstacle = aOther.m_blockingObstacle;

       m_links = std::move( aOther.m_links );
   }

    return *this;
}


LINE* LINE::Clone() const
{
    LINE* l = new LINE( *this );

    return l;
}


void LINE::Mark( int aMarker ) const
{
    m_marker = aMarker;

    for( const LINKED_ITEM* s : m_links )
        s->Mark( aMarker );

}


void LINE::Unmark( int aMarker ) const
{
    for( const LINKED_ITEM* s : m_links )
        s->Unmark( aMarker );

    m_marker = 0;
}


int LINE::Marker() const
{
    int marker = m_marker;

    for( LINKED_ITEM* s : m_links )
        marker |= s->Marker();

    return marker;
}


SEGMENT* SEGMENT::Clone() const
{
    SEGMENT* s = new SEGMENT( *this );

    s->m_seg = m_seg;
    s->m_net = m_net;
    s->m_layers = m_layers;
    s->m_marker = m_marker;
    s->m_rank = m_rank;

    return s;
}


int LINE::CountCorners( int aAngles ) const
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

static int areNeighbours( int x, int y, int max = 0 )
{
    if( x > 0 && x - 1 == y )
        return true;

    if( x < max - 1 && x + 1 == y )
        return true;

    return false;
}

#ifdef TOM_EXTRA_DEBUG
SHAPE_LINE_CHAIN g_pnew, g_hnew;
#endif

bool LINE::Walkaround( const SHAPE_LINE_CHAIN& aObstacle, SHAPE_LINE_CHAIN& aPath, bool aCw ) const
{
    const SHAPE_LINE_CHAIN& line( CLine() );

    if( line.SegmentCount() < 1 )
    {
        return false;
    }

    const VECTOR2I pFirst = line.CPoint(0);

    bool inFirst = aObstacle.PointInside( pFirst ) && !aObstacle.PointOnEdge( pFirst );

    // We can't really walk around if the beginning of the path lies inside the obstacle hull.
    // Double check if it's not on the hull itself as this triggers many unroutable corner cases.
    if( inFirst  )
    {
	    return false;
    }

    enum VERTEX_TYPE { INSIDE = 0, OUTSIDE, ON_EDGE };

    // Represents an entry in directed graph of hull/path vertices. Scanning this graph
    // starting from the path's first point results (if possible) with a correct walkaround path
    struct VERTEX
    {
        // vertex classification (inside/outside/exactly on the hull)
        VERTEX_TYPE type;
        // true = vertex coming from the hull primitive
        bool isHull;
        // position
        VECTOR2I pos;
        // list of neighboring vertices
        std::vector<VERTEX*> neighbours;
        // index of this vertex in path (pnew)
        int indexp = -1;
        // index of this vertex in the hull (hnew)
        int indexh = -1;
        // visited indicator (for BFS search)
        bool visited = false;
    };

    SHAPE_LINE_CHAIN::INTERSECTIONS ips;

    HullIntersection( aObstacle, line, ips );

    SHAPE_LINE_CHAIN pnew( CLine() ), hnew( aObstacle );

    std::vector<VERTEX> vts;

    auto findVertex =
            [&]( const VECTOR2I& pos ) -> VERTEX*
            {
                for( VERTEX& v : vts )
                {
                    if( v.pos == pos )
                        return &v;
                }

                return nullptr;
            };

    // corner case for loopy tracks: insert the end loop point back into the hull
    if( const std::optional<SHAPE_LINE_CHAIN::INTERSECTION> isect = pnew.SelfIntersecting() )
    {
        if( isect->p != pnew.CLastPoint() )
            pnew.Split( isect->p );
    }

    // insert all intersections found into the new hull/path SLCs
    for( SHAPE_LINE_CHAIN::INTERSECTION& ip : ips )
    {
        if( pnew.Find( ip.p, 1 ) < 0)
            pnew.Split(ip.p);

        if( hnew.Find( ip.p, 1 ) < 0 )
            hnew.Split(ip.p);
    }

    for( int i = 0; i < pnew.PointCount(); i++ )
    {
        const VECTOR2I& p = pnew.CPoint( i );
        bool            onEdge = hnew.PointOnEdge( p );

        if ( !onEdge )
            continue;

        int idx = hnew.Find( p );

        if(idx < 0 )
            hnew.Split( p );
    }

    #ifdef TOM_EXTRA_DEBUG
    for( auto& ip : ips )
    {
        printf("Chk: %d %d\n", pnew.Find( ip.p ), hnew.Find(ip.p) );
    }
    #endif

    // we assume the default orientation of the hulls is clockwise, so just reverse the vertex
    // order if the caller wants a counter-clockwise walkaround
    if ( !aCw )
        hnew = hnew.Reverse();

    vts.reserve( 2 * ( hnew.PointCount() + pnew.PointCount() ) );

    // create a graph of hull/path vertices and classify them (inside/on edge/outside the hull)
    for( int i = 0; i < pnew.PointCount(); i++ )
    {
        const VECTOR2I& p = pnew.CPoint(i);
        bool            onEdge = hnew.PointOnEdge( p );
        bool            inside = hnew.PointInside( p );

        #ifdef TOM_EXTRA_DEBUG
        printf("pnew %d inside %d onedge %d\n", i, !!inside, !!onEdge );
        #endif

        VERTEX v;

        v.indexp = i;
        v.isHull = false;
        v.pos = p;
        v.type = inside && !onEdge ? INSIDE : onEdge ? ON_EDGE : OUTSIDE;
        vts.push_back( v );
    }

    #ifdef TOM_EXTRA_DEBUG
    g_pnew = pnew;
    g_hnew = hnew;
    #endif

    // each path vertex neighbour list points for sure to the next vertex in the path
    for( int i = 0; i < pnew.PointCount() - 1; i++ )
    {
        vts[i].neighbours.push_back( &vts[ i+1 ] );
    }

    // each path vertex neighbour list points for sure to the next vertex in the path
    for( int i = 1; i < pnew.PointCount() ; i++ )
    {
        vts[i].neighbours.push_back( &vts[ i-1 ] );
    }

    // insert hull vertices into the graph
    for( int i = 0; i < hnew.PointCount(); i++ )
    {
        const VECTOR2I& hp = hnew.CPoint( i );
        VERTEX*         vn = findVertex( hp );

        // if vertex already present (it's very likely that in recursive shoving hull and path vertices will overlap)
        // just mark it as a path vertex that also belongs to the hull
        if( vn )
        {
            vn->isHull = true;
            vn->indexh = i;
        }
        else // new hull vertex
        {
            VERTEX v;
            v.pos = hp;
            v.type = ON_EDGE;
            v.indexh = i;
            v.isHull = true;
            vts.push_back( v );
        }
    }

    // go around the hull and fix up the neighbour link lists
    for( int i = 0; i < hnew.PointCount(); i++ )
    {
        VERTEX* vc = findVertex( hnew.CPoint( i ) );
        VERTEX* vnext = findVertex( hnew.CPoint( i+1 ) );

        if( vc && vnext )
            vc->neighbours.push_back( vnext );
    }

    // In the case that the initial path ends *inside* the current obstacle (i.e. the mouse cursor
    // is somewhere inside the hull for the current obstacle) we want to end the walkaround at the
    // point closest to the cursor
    bool inLast  = aObstacle.PointInside( CLastPoint() ) && !aObstacle.PointOnEdge( CLastPoint() );
    bool appendV = true;
    int  lastDst = INT_MAX;

    int i = 0;
#ifdef TOM_EXTRA_DEBUG
    for( VERTEX* &v: vts )
    {
        if( v.indexh < 0 && v.type == ON_EDGE )
            v.type = OUTSIDE; // hack

        printf("V %d pos %d %d ip %d ih %d type %d\n", i++, v.pos.x, v.pos.y, v.indexp, v.indexh, v.type );
    }
#endif
    // vts[0] = start point
    VERTEX*          v = &vts[0];
    VERTEX*          v_prev = nullptr;
    SHAPE_LINE_CHAIN out;

    int iterLimit = 1000;

    // keep scanning the graph until we reach the end point of the path
    while( v->indexp != ( pnew.PointCount() - 1 ) )
    {
        iterLimit--;

        // I'm not 100% sure this algorithm doesn't have bugs that may cause it to freeze,
        // so here's a temporary iteration limit
        if( iterLimit == 0 )
            return false;

        if( v->visited )
        {
            // loop found? stop walking
            break;
        }

#ifdef TOM_EXTRA_DEBUG
        printf("---\nvisit ip %d ih %d type %d outs %d neig %d\n", v->indexp, v->indexh, v->type, out.PointCount(), v->neighbours.size() );
#endif
        out.Append( v->pos );

        VERTEX* v_next = nullptr;

        if( v->type == OUTSIDE )
        {
            // current vertex is outside? first look for any vertex further down the path
            // that is not inside the hull
            out.Append( v->pos );
            VERTEX* v_next_fallback = nullptr;

            for( VERTEX* vn : v->neighbours )
            {
                if( areNeighbours( vn->indexp , v->indexp, pnew.PointCount() )
                        && vn->type != INSIDE )
                {
                    if( !vn->visited )
                    {
                        v_next = vn;
                        break;
                    }
                    else if( vn != v_prev )
                    {
                        v_next_fallback = vn;
                    }
                }
            }

            if( !v_next )
                v_next = v_next_fallback;

            // such a vertex must always be present, if not, bummer.
            if( !v_next )
            {
                #ifdef TOM_EXTRA_DEBUG
                printf("FAIL VN fallback %p\n", v_next_fallback );
                #endif
                return false;
            }
        }
        else if( v->type == ON_EDGE )
        {
            // look first for the first vertex outside the hull
            for( VERTEX* vn : v->neighbours )
            {
#ifdef TOM_EXTRA_DEBUG
                printf( "- OUT scan ip %d ih %d type %d\n", vn->indexp, vn->indexh, vn->type );
#endif

                if( vn->type == OUTSIDE && !vn->visited )
                {
                    v_next = vn;
                    break;
                }
            }

            // no outside vertices found? continue traversing the hull
            if( !v_next )
            {
                for( VERTEX* vn : v->neighbours )
                {
                    #ifdef TOM_EXTRA_DEBUG
                    printf("- scan ip %d ih %d type %d\n", vn->indexp, vn->indexh, vn->type );
                    #endif
                    if( vn->type == ON_EDGE && !vn->isHull &&
                        areNeighbours( vn->indexp, v->indexp, pnew.PointCount() ) &&
                        ( vn->indexh == ( ( v->indexh + 1 ) % hnew.PointCount() ) ) )
                    {
                        v_next = vn;
                        break;
                    }
                }
            }

            // still nothing found? try to find the next (index-wise) point on the hull. I guess
            // we should never reach this part of the code, but who really knows?
            if( !v_next )
            {
#ifdef TOM_EXTRA_DEBUG
                printf("still no v_next\n");
#endif
                for( VERTEX* vn : v->neighbours )
                {
                    if( vn->type == ON_EDGE )
                    {
                        if( vn->indexh == ( ( v->indexh + 1 ) % hnew.PointCount() ) )
                        {
                            v_next = vn;
                            break;
                        }
                    }
                }

                if( v_next )
                {
                    for( VERTEX &vt : vts )
                    {
                        if( vt.isHull )
                            vt.visited = false;
                    }
                }

#ifdef TOM_EXTRA_DEBUG
                printf("v_next %p\n", v_next);
#endif

                // Did we get the next hull point but the end of the line is inside?  Instead of walking
                // around the hull some more (which will just end up taking us back to the start), lets
                // just project the normal of the endpoint onto this next segment and call it quits.
                if( inLast && v_next )
                {
                    int d = ( v_next->pos - CLastPoint() ).SquaredEuclideanNorm();

                    if( d < lastDst )
                    {
                        lastDst = d;
                    }
                    else
                    {
                        VECTOR2I proj = SEG( v->pos, v_next->pos ).NearestPoint( CLastPoint() );
                        out.Append( proj );
                        appendV = false;
                        break;
                    }
                }
            }
        }

        v->visited = true;
        v_prev = v;
        v = v_next;

        if( !v )
            return false;
    }

    if( appendV )
        out.Append( v->pos );

    aPath = std::move( out );
    return true;
}


const SHAPE_LINE_CHAIN SEGMENT::Hull( int aClearance, int aWalkaroundThickness, int aLayer ) const
{
    /*DEBUG_DECORATOR* debugDecorator = ROUTER::GetInstance()->GetInterface()->GetDebugDecorator();

    PNS_DBG( debugDecorator, Message, wxString::Format( wxT( "seghull %d %d" ), aWalkaroundThickness, aClearance ) );
    PNS_DBG(debugDecorator, AddShape, &m_seg, RED, 0, wxT("theseg") );
        */

   return SegmentHull( m_seg, aClearance, aWalkaroundThickness );
}

const LINE LINE::ClipToNearestObstacle( NODE* aNode ) const
{
    const int IterationLimit = 5;
    int i;
    LINE l( *this );

    for( i = 0; i < IterationLimit; i++ )
    {
        NODE::OPT_OBSTACLE obs = aNode->NearestObstacle( &l );

        if( obs )
        {
            l.RemoveVia();
            VECTOR2I collisionPoint = obs->m_ipFirst;
            int segIdx = l.Line().NearestSegment( collisionPoint );

            if( l.Line().IsArcSegment( segIdx ) )
            {
                // Don't clip at arcs, start again
                l.Line().Clear();
            }
            else
            {
                SEG nearestSegment = l.Line().CSegment( segIdx );
                VECTOR2I nearestPt = nearestSegment.NearestPoint( collisionPoint );
                int      p = l.Line().Split( nearestPt );
                l.Line().Remove( p + 1, -1 );
            }
        }
        else
        {
            break;
        }
    }

    if( i == IterationLimit )
        l.Line().Clear();

    return l;
}



SHAPE_LINE_CHAIN dragCornerInternal( const SHAPE_LINE_CHAIN& aOrigin, const VECTOR2I& aP, DIRECTION_45 aPreferredEndingDirection = DIRECTION_45() )
{
    std::optional<SHAPE_LINE_CHAIN> picked;
    int i;
    int d = 2;

    wxASSERT( aOrigin.PointCount() > 0 );

    if( aOrigin.PointCount() == 1 )
    {
        return DIRECTION_45().BuildInitialTrace( aOrigin.CPoint( 0 ), aP );
    }
    else if( aOrigin.SegmentCount() == 1 )
    {
        DIRECTION_45 dir( aOrigin.CPoint( 0 ) - aOrigin.CPoint( 1 ) );

        return DIRECTION_45().BuildInitialTrace( aOrigin.CPoint( 0 ), aP, dir.IsDiagonal() );
    }


    //if( aOrigin.CSegment( -1 ).Length() > 100000 * 30 ) // fixme: constant/parameter?
        d = 1;

    for( i = aOrigin.SegmentCount() - d; i >= 0; i-- )
    {
        DIRECTION_45     d_start( aOrigin.CSegment( i ) );
        const VECTOR2I&  p_start = aOrigin.CPoint( i );
        SHAPE_LINE_CHAIN paths[2];
        DIRECTION_45     dirs[2];
        DIRECTION_45     d_prev = ( i > 0 ? DIRECTION_45( aOrigin.CSegment( i-1 ) )
                                          : DIRECTION_45() );
        int              dirCount = 0;

        for( int j = 0; j < 2; j++ )
        {
            paths[j] = d_start.BuildInitialTrace( p_start, aP, j );

            if( paths[j].SegmentCount() < 1 )
                continue;

            assert( dirCount < int( sizeof( dirs ) / sizeof( dirs[0] ) ) );

            dirs[dirCount] = DIRECTION_45( paths[j].CSegment( 0 ) );
            ++dirCount;
        }

        if( aPreferredEndingDirection != DIRECTION_45::UNDEFINED )
        {
            for( int j = 0; j < dirCount; j++ )
            {
                DIRECTION_45 endingDir( paths[j].CSegment(-1) );
                if( endingDir == aPreferredEndingDirection )
                {
                    picked = paths[j];
                    break;
                }
            }
        }

        if( !picked )
        {
            for( int j = 0; j < dirCount; j++ )
            {
                if( dirs[j] == d_start )
                {
                    picked = paths[j];
                    break;
                }
            }
        }

        if( picked )
            break;

        for( int j = 0; j < dirCount; j++ )
        {
            if( dirs[j].IsObtuse( d_prev ) )
            {
                picked = paths[j];
                break;
            }
        }

        if( picked )
            break;
    }

    if( picked )
    {
        SHAPE_LINE_CHAIN path = aOrigin.Slice( 0, i );
        path.Append( *picked );

        return path;
    }

    DIRECTION_45 dir( aOrigin.CLastPoint() - aOrigin.CPoints()[ aOrigin.PointCount() - 2 ] );

    return DIRECTION_45().BuildInitialTrace( aOrigin.CPoint( 0 ), aP, dir.IsDiagonal() );
}


void LINE::dragCorner45( const VECTOR2I& aP, int aIndex, DIRECTION_45 aPreferredEndingDirection )
{
    SHAPE_LINE_CHAIN path;

    int width = m_line.Width();
    VECTOR2I snapped = snapDraggedCorner( m_line, aP, aIndex );

    if( aIndex == 0 )
    {
        path = dragCornerInternal( m_line.Reverse(), snapped, aPreferredEndingDirection ).Reverse();
    }
    else if( aIndex == m_line.SegmentCount() )
    {
        path = dragCornerInternal( m_line, snapped, aPreferredEndingDirection );
    }
    else
    {
        // Are we next to an arc? Insert a new point so we slice correctly
        if( m_line.IsPtOnArc( static_cast<size_t>( aIndex ) + 1 ) )
            m_line.Insert( aIndex + 1, m_line.CPoint( aIndex + 1 ) );

        // fixme: awkward behaviour for "outwards" drags
        path = dragCornerInternal( m_line.Slice( 0, aIndex ), snapped, aPreferredEndingDirection );
        SHAPE_LINE_CHAIN path_rev =
                dragCornerInternal( m_line.Slice( aIndex, -1 ).Reverse(), snapped, aPreferredEndingDirection ).Reverse();
        path.Append( path_rev );
    }

    path.Simplify();
    path.SetWidth( width );
    m_line = std::move( path );
}


void LINE::dragCornerFree( const VECTOR2I& aP, int aIndex )
{
    ssize_t idx = static_cast<ssize_t>( aIndex );
    ssize_t numpts = static_cast<ssize_t>( m_line.PointCount() );

    // If we're asked to drag the end of an arc, insert a new vertex to drag instead
    if( m_line.IsPtOnArc( idx ) )
    {
        if( idx == 0 || ( idx > 0 && !m_line.IsPtOnArc( idx - 1 ) ) )
        {
            m_line.Insert( idx, m_line.GetPoint( idx ) );
        }
        else if( ( idx == numpts - 1 ) || ( idx < numpts - 1 && !m_line.IsArcSegment( idx ) ) )
        {
            idx++;
            m_line.Insert( idx, m_line.GetPoint( idx ) );
        }
        else
        {
            wxASSERT_MSG( false, wxT( "Attempt to dragCornerFree in the middle of an arc!" ) );
        }
    }

    m_line.SetPoint( idx, aP );
    m_line.Simplify();
}

void LINE::DragCorner( const VECTOR2I& aP, int aIndex, bool aFreeAngle, DIRECTION_45 aPreferredEndingDirection )
{
    wxCHECK_RET( aIndex >= 0, wxT( "Negative index passed to LINE::DragCorner" ) );

    if( aFreeAngle )
    {
        dragCornerFree( aP, aIndex );
    }
    else
    {
        dragCorner45( aP, aIndex, aPreferredEndingDirection );
    }
}

void LINE::DragSegment( const VECTOR2I& aP, int aIndex, bool aFreeAngle )
{
    if( aFreeAngle )
    {
        assert( false );
    }
    else
    {
        dragSegment45( aP, aIndex );
    }
}

VECTOR2I LINE::snapDraggedCorner(
        const SHAPE_LINE_CHAIN& aPath, const VECTOR2I& aP, int aIndex ) const
{
    int s_start = std::max( aIndex - 2, 0 );
    int s_end = std::min( aIndex + 2, aPath.SegmentCount() - 1 );

    int      i, j;
    int      best_dist = INT_MAX;
    VECTOR2I best_snap = aP;

    if( m_snapThreshhold <= 0 )
        return aP;

    for( i = s_start; i <= s_end; i++ )
    {
        const SEG& a = aPath.CSegment( i );

        for( j = s_start; j < i; j++ )
        {
            const SEG& b = aPath.CSegment( j );

            if( !( DIRECTION_45( a ).IsObtuse( DIRECTION_45( b ) ) ) )
                continue;

            OPT_VECTOR2I ip = a.IntersectLines( b );

            if( ip )
            {
                int dist = ( *ip - aP ).EuclideanNorm();

                if( dist < m_snapThreshhold && dist < best_dist )
                {
                    best_dist = dist;
                    best_snap = *ip;
                }
            }
        }
    }

    return best_snap;
}

VECTOR2I LINE::snapToNeighbourSegments(
        const SHAPE_LINE_CHAIN& aPath, const VECTOR2I& aP, int aIndex ) const
{
    VECTOR2I     snap_p[2];
    DIRECTION_45 dragDir( aPath.CSegment( aIndex ) );
    int          snap_d[2] = { -1, -1 };

    if( m_snapThreshhold == 0 )
        return aP;

    if( aIndex >= 2 )
    {
        SEG s = aPath.CSegment( aIndex - 2 );

        if( DIRECTION_45( s ) == dragDir )
            snap_d[0] = s.LineDistance( aP );

        snap_p[0] = s.A;
    }

    if( aIndex < aPath.SegmentCount() - 2 )
    {
        SEG s = aPath.CSegment( aIndex + 2 );

        if( DIRECTION_45( s ) == dragDir )
            snap_d[1] = s.LineDistance( aP );

        snap_p[1] = s.A;
    }

    VECTOR2I best = aP;
    int      minDist = INT_MAX;

    for( int i = 0; i < 2; i++ )
    {
        if( snap_d[i] >= 0 && snap_d[i] < minDist && snap_d[i] <= m_snapThreshhold )
        {
            minDist = snap_d[i];
            best = snap_p[i];
        }
    }

    return best;
}

void LINE::dragSegment45( const VECTOR2I& aP, int aIndex )
{
    SHAPE_LINE_CHAIN path( m_line );
    VECTOR2I         target( aP );

    wxASSERT( aIndex < m_line.PointCount() );

    SEG guideA[2], guideB[2];
    int index = aIndex;

    target = snapToNeighbourSegments( path, aP, aIndex );

    // We require a valid s_prev and s_next.  If we are at the start or end of the line, we insert
    // a new point at the start or end so there is a zero-length segment for prev or next (we will
    // resize it as part of the drag operation).  If we are next to an arc, we do this also, as we
    // cannot drag away one of the arc's points.

    if( index == 0 || path.IsPtOnArc( index ) )
    {
        path.Insert( index > 0 ? index + 1 : 0, path.CPoint( index ) );
        index++;
    }

    if( index == path.SegmentCount() - 1 )
    {
        path.Insert( path.PointCount() - 1, path.CLastPoint() );
    }
    else if( path.IsPtOnArc( index + 1 ) )
    {
        path.Insert( index + 1, path.CPoint( index + 1 ) );
    }

    SEG          dragged = path.CSegment( index );
    DIRECTION_45 drag_dir( dragged );

    SEG s_prev = path.CSegment( index - 1 );
    SEG s_next = path.CSegment( index + 1 );

    DIRECTION_45 dir_prev( s_prev );
    DIRECTION_45 dir_next( s_next );

    if( dir_prev == drag_dir )
    {
        dir_prev = dir_prev.Left();
        path.Insert( index, path.CPoint( index ) );
        index++;
    }
    else if( dir_prev == DIRECTION_45::UNDEFINED )
    {
        dir_prev = drag_dir.Left();
    }

    if( dir_next == drag_dir )
    {
        dir_next = dir_next.Right();
        path.Insert( index + 1, path.CPoint( index + 1 ) );
    }
    else if( dir_next == DIRECTION_45::UNDEFINED )
    {
        dir_next = drag_dir.Right();
    }

    s_prev = path.CSegment( index - 1 );
    s_next = path.CSegment( index + 1 );
    dragged = path.CSegment( index );

    if( aIndex == 0 )
    {
        guideA[0] = SEG( dragged.A, dragged.A + drag_dir.Right().ToVector() );
        guideA[1] = SEG( dragged.A, dragged.A + drag_dir.Left().ToVector() );
    }
    else
    {
        if( dir_prev.Angle( drag_dir )
                & ( DIRECTION_45::ANG_OBTUSE | DIRECTION_45::ANG_HALF_FULL ) )
        {
            guideA[0] = SEG( s_prev.A, s_prev.A + drag_dir.Left().ToVector() );
            guideA[1] = SEG( s_prev.A, s_prev.A + drag_dir.Right().ToVector() );
        }
        else
            guideA[0] = guideA[1] = SEG( dragged.A, dragged.A + dir_prev.ToVector() );
    }

    if( aIndex == m_line.SegmentCount() - 1 )
    {
        guideB[0] = SEG( dragged.B, dragged.B + drag_dir.Right().ToVector() );
        guideB[1] = SEG( dragged.B, dragged.B + drag_dir.Left().ToVector() );
    }
    else
    {
        if( dir_next.Angle( drag_dir )
                & ( DIRECTION_45::ANG_OBTUSE | DIRECTION_45::ANG_HALF_FULL ) )
        {
            guideB[0] = SEG( s_next.B, s_next.B + drag_dir.Left().ToVector() );
            guideB[1] = SEG( s_next.B, s_next.B + drag_dir.Right().ToVector() );
        }
        else
            guideB[0] = guideB[1] = SEG( dragged.B, dragged.B + dir_next.ToVector() );
    }

    SEG s_current( target, target + drag_dir.ToVector() );

    int              best_len = INT_MAX;
    SHAPE_LINE_CHAIN best;

    for( int i = 0; i < 2; i++ )
    {
        for( int j = 0; j < 2; j++ )
        {
            OPT_VECTOR2I ip1 = s_current.IntersectLines( guideA[i] );
            OPT_VECTOR2I ip2 = s_current.IntersectLines( guideB[j] );

            SHAPE_LINE_CHAIN np;

            if( !ip1 || !ip2 )
                continue;

            SEG s1( s_prev.A, *ip1 );
            SEG s2( *ip1, *ip2 );
            SEG s3( *ip2, s_next.B );

            OPT_VECTOR2I ip;

            if( ( ip = s1.Intersect( s_next ) ) )
            {
                np.Append( s1.A );
                np.Append( *ip );
                np.Append( s_next.B );
            }
            else if( ( ip = s3.Intersect( s_prev ) ) )
            {
                np.Append( s_prev.A );
                np.Append( *ip );
                np.Append( s3.B );
            }
            else if( ( ip = s1.Intersect( s3 ) ) )
            {
                np.Append( s_prev.A );
                np.Append( *ip );
                np.Append( s_next.B );
            }
            else
            {
                np.Append( s_prev.A );
                np.Append( *ip1 );
                np.Append( *ip2 );
                np.Append( s_next.B );
            }

            if( np.Length() < best_len )
            {
                best_len = np.Length();
                best = std::move( np );
            }
        }
    }

    if( m_line.PointCount() == 1 )
        m_line = best;
    else if( aIndex == 0 )
        m_line.Replace( 0, 1, best );
    else if( aIndex == m_line.SegmentCount() - 1 )
        m_line.Replace( -2, -1, best );
    else
        m_line.Replace( aIndex, aIndex + 1, best );

    m_line.Simplify();
}


bool LINE::CompareGeometry( const LINE& aOther )
{
    return m_line.CompareGeometry( aOther.m_line );
}


void LINE::Reverse()
{
    m_line = m_line.Reverse();

    std::reverse( m_links.begin(), m_links.end() );
}


void LINE::AppendVia( const VIA& aVia )
{
    if( m_line.PointCount() > 1 && aVia.Pos() == m_line.CPoint( 0 ) )
    {
        Reverse();
    }

    m_via = aVia.Clone();
    m_via->SetOwner( this );
    m_via->SetNet( m_net );
}


void LINE::LinkVia( VIA* aVia )
{
    if( m_line.PointCount() > 1 && aVia->Pos() == m_line.CPoint( 0 ) )
    {
        Reverse();
    }

    m_via = aVia;
    Link( aVia );
}


void LINE::SetRank( int aRank )
{
    m_rank = aRank;

    for( auto s : m_links )
        s->SetRank( aRank );

}


int LINE::Rank() const
{
    int min_rank = INT_MAX;

    if( IsLinked() )
    {
        for( const LINKED_ITEM* item : m_links )
            min_rank = std::min( min_rank, item->Rank() );
    }
    else
    {
        min_rank = m_rank;
    }

    int rank = ( min_rank == INT_MAX ) ? -1 : min_rank;

    return rank;
}


void LINE::ClipVertexRange( int aStart, int aEnd )
{
    /**
     * We need to figure out which joints to keep after the clip operation, because arcs will have
     * multiple vertices.  It is assumed that anything calling this method will have determined the
     * vertex range to clip based on joints, meaning we will never clip in the middle of an arc.
     * Clipping in the middle of an arc would break this and various other things...
     */
    int firstLink = 0;
    int lastLink  = std::max( 0, static_cast<int>( m_links.size() ) - 1 );
    int linkIdx   = 0;

    int numPoints = static_cast<int>( m_line.PointCount() );

    for( int i = 0; i >= 0 && i < m_line.PointCount(); i = m_line.NextShape( i ) )
    {
        if( i <= aStart )
            firstLink = linkIdx;

        if( i < 0 || i >= aEnd - 1 || linkIdx >= lastLink )
        {
            lastLink = linkIdx;
            break;
        }

        linkIdx++;
    }

    wxASSERT( lastLink >= firstLink );

    m_line = m_line.Slice( aStart, aEnd );

    if( IsLinked() )
    {
        wxASSERT( m_links.size() < INT_MAX );
        wxASSERT( static_cast<int>( m_links.size() ) >= ( lastLink - firstLink ) );

        // Note: The range includes aEnd, but we have n-1 segments.
        std::rotate(
            m_links.begin(),
            m_links.begin() + firstLink,
            m_links.begin() + lastLink
        );

        m_links.resize( lastLink - firstLink + 1 );
    }
}


bool LINE::HasLoops() const
{
    for( int i = 0; i < PointCount(); i++ )
    {
        for( int j = i + 2; j < PointCount(); j++ )
        {
            if( CPoint( i ) == CPoint( j ) )
                return true;
        }
    }

    return false;
}


static void extendBox( BOX2I& aBox, bool& aDefined, const VECTOR2I& aP )
{
    if( aDefined )
    {
        aBox.Merge( aP );
    }
    else
    {
        aBox = BOX2I( aP, VECTOR2I( 0, 0 ) );
        aDefined = true;
    }
}


OPT_BOX2I LINE::ChangedArea( const LINE* aOther ) const
{
    BOX2I area;
    bool areaDefined = false;

    int i_start = -1;
    int i_end_self = -1, i_end_other = -1;

    SHAPE_LINE_CHAIN self( m_line );
    self.Simplify();
    SHAPE_LINE_CHAIN other( aOther->m_line );
    other.Simplify();

    int np_self = self.PointCount();
    int np_other = other.PointCount();

    int n = std::min( np_self, np_other );

    for( int i = 0; i < n; i++ )
    {
        const VECTOR2I p1 = self.CPoint( i );
        const VECTOR2I p2 = other.CPoint( i );

        if( p1 != p2 )
        {
            if( i != n - 1 )
            {
                SEG s = self.CSegment( i );

                if( !s.Contains( p2 ) )
                {
                    i_start = i;
                    break;
                }
            }
            else
            {
                i_start = i;
                break;
            }
        }
    }

    for( int i = 0; i < n; i++ )
    {
        const VECTOR2I p1 = self.CPoint( np_self - 1 - i );
        const VECTOR2I p2 = other.CPoint( np_other - 1 - i );

        if( p1 != p2 )
        {
            i_end_self = np_self - 1 - i;
            i_end_other = np_other - 1 - i;
            break;
        }
    }

    if( i_start < 0 )
        i_start = n;

    if( i_end_self < 0 )
        i_end_self = np_self - 1;

    if( i_end_other < 0 )
        i_end_other = np_other - 1;

    for( int i = i_start; i <= i_end_self; i++ )
        extendBox( area, areaDefined, self.CPoint( i ) );

    for( int i = i_start; i <= i_end_other; i++ )
        extendBox( area, areaDefined, other.CPoint( i ) );

    if( areaDefined )
    {
        area.Inflate( std::max( Width(), aOther->Width() ) );
        return area;
    }

    return OPT_BOX2I();
}


bool LINE::HasLockedSegments() const
{
    for( const auto seg : m_links )
    {
        if( seg->Marker() & MK_LOCKED )
            return true;
    }
    return false;
}


void LINE::Clear()
{
    ClearLinks();
    RemoveVia();
    m_line.Clear();
}


void LINE::RemoveVia()
{
    if( m_via )
    {
        if( ContainsLink( m_via ) )
            Unlink( m_via );
        if( m_via->BelongsTo( this ) )
            delete m_via;
    }

    m_via = nullptr;
}


const std::string SEGMENT::Format( ) const
{
    std::stringstream ss;
    ss << ITEM::Format() << " ";
    ss << m_seg.Format( false );
    return ss.str();
}


int LINE::FindSegment( const SEGMENT* aSeg ) const
{
    for( int i = 0; i < m_line.SegmentCount(); i++)
    {
        const SEG&s = m_line.CSegment(i);
        if( s == aSeg->Seg() )
            return i;
    }

    return -1;
}

}


