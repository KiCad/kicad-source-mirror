/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#include <vector>
#include <cstdio>
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <set>
#include <list>
#include <algorithm>

#include <boost/foreach.hpp>

#include "geometry/shape_poly_set.h"

using namespace ClipperLib;

int SHAPE_POLY_SET::NewOutline()
{
    Path empty_path;
    Paths poly;
    poly.push_back( empty_path );
    m_polys.push_back( poly );
    return m_polys.size() - 1;
}


int SHAPE_POLY_SET::NewHole( int aOutline )
{
    assert( false );
    return -1;
}


int SHAPE_POLY_SET::AppendVertex( int x, int y, int aOutline, int aHole )
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int)m_polys.size() );
    assert( idx < (int)m_polys[aOutline].size() );

    m_polys[aOutline][idx].push_back( IntPoint( x, y ) );

    return m_polys[aOutline][idx].size();
}


int SHAPE_POLY_SET::VertexCount( int aOutline, int aHole ) const
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert ( aOutline < (int)m_polys.size() );
    assert ( idx < (int)m_polys[aOutline].size() );

    return m_polys[aOutline][idx].size();
}


const VECTOR2I SHAPE_POLY_SET::GetVertex( int index, int aOutline, int aHole ) const
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int)m_polys.size() );
    assert( idx < (int)m_polys[aOutline].size() );

    IntPoint p = m_polys[aOutline][idx][index];
    return VECTOR2I (p.X, p.Y);
}


int SHAPE_POLY_SET::AddOutline( const SHAPE_LINE_CHAIN& aOutline )
{
    assert( aOutline.IsClosed() );

    Path p = convert( aOutline );
    Paths poly;

    if( !Orientation( p ) )
        ReversePath( p ); // outlines are always CW

    poly.push_back( p );

    m_polys.push_back( poly );

    return m_polys.size() - 1;
}


int SHAPE_POLY_SET::AddHole( const SHAPE_LINE_CHAIN& aHole, int aOutline )
{
    assert ( m_polys.size() );

    if( aOutline < 0 )
        aOutline += m_polys.size();

    Paths& poly = m_polys[aOutline];

    assert( poly.size() );

    Path p = convert( aHole );

    if( Orientation( p ) )
        ReversePath( p ); // holes are always CCW

    poly.push_back( p );

    return poly.size() - 1;
}


const ClipperLib::Path SHAPE_POLY_SET::convert( const SHAPE_LINE_CHAIN& aPath )
{
    Path c_path;

    for( int i = 0; i < aPath.PointCount(); i++ )
    {
        const VECTOR2I& vertex = aPath.CPoint( i );
        c_path.push_back( ClipperLib::IntPoint( vertex.x, vertex.y ) );
    }

    return c_path;
}


void SHAPE_POLY_SET::booleanOp( ClipperLib::ClipType type, const SHAPE_POLY_SET& b )
{
    Clipper c;

    c.StrictlySimple( true );

    BOOST_FOREACH( Paths& subject, m_polys )
    {
         c.AddPaths( subject, ptSubject, true );
    }

    BOOST_FOREACH( const Paths& clip, b.m_polys )
    {
        c.AddPaths( clip, ptClip, true );
    }

    PolyTree solution;

    c.Execute( type, solution, pftNonZero, pftNonZero );

    importTree( &solution );
}


void SHAPE_POLY_SET::Add( const SHAPE_POLY_SET& b )
{
    booleanOp( ctUnion, b );
}


void SHAPE_POLY_SET::Subtract( const SHAPE_POLY_SET& b )
{
    booleanOp( ctDifference, b );
}


void SHAPE_POLY_SET::Erode( int aFactor )
{
    ClipperOffset c;

    BOOST_FOREACH( Paths& p, m_polys )
        c.AddPaths(p, jtRound, etClosedPolygon );

    PolyTree solution;

    c.Execute( solution, aFactor );

    m_polys.clear();

    for( PolyNode* n = solution.GetFirst(); n; n = n->GetNext() )
    {
        Paths ps;
        ps.push_back( n->Contour );
        m_polys.push_back( ps );
    }
}


void SHAPE_POLY_SET::importTree( ClipperLib::PolyTree* tree )
{
    m_polys.clear();

    for( PolyNode* n = tree->GetFirst(); n; n = n->GetNext() )
    {
        if( !n->IsHole() )
        {
            Paths paths;
            paths.push_back( n->Contour );

            for( unsigned i = 0; i < n->Childs.size(); i++ )
                paths.push_back( n->Childs[i]->Contour );

            m_polys.push_back( paths );
        }
    }
}


// Polygon fracturing code. Work in progress.
struct FractureEdge
{
    FractureEdge( bool connected, Path* owner, int index ) :
        m_connected( connected ),
        m_next( NULL )
    {
        m_p1 = (*owner)[index];
        m_p2 = (*owner)[(index + 1) % owner->size()];
    }

    FractureEdge( int64_t y = 0 ) :
        m_connected( false ),
        m_next( NULL )
    {
        m_p1.Y = m_p2.Y = y;
    }

    FractureEdge( bool connected, const IntPoint& p1, const IntPoint& p2 ) :
        m_connected( connected ),
        m_p1( p1 ),
        m_p2( p2 ),
        m_next( NULL )
    {
    }

    bool matches( int y ) const
    {
        int y_min = std::min( m_p1.Y, m_p2.Y );
        int y_max = std::max( m_p1.Y, m_p2.Y );

        return ( y >= y_min ) && ( y <= y_max );
    }

    bool m_connected;
    IntPoint m_p1, m_p2;
    FractureEdge* m_next;
};


typedef std::vector<FractureEdge*> FractureEdgeSet;

static int processEdge( FractureEdgeSet& edges, FractureEdge* edge )
{
    int64_t x = edge->m_p1.X;
    int64_t y = edge->m_p1.Y;
    int64_t min_dist = std::numeric_limits<int64_t>::max();
    int64_t x_nearest = 0;

    FractureEdge* e_nearest = NULL;

    for( FractureEdgeSet::iterator i = edges.begin(); i != edges.end(); ++i )
    {
        if( !(*i)->matches( y ) )
            continue;

        int64_t x_intersect;

        if( (*i)->m_p1.Y == (*i)->m_p2.Y ) // horizontal edge
            x_intersect = std::max( (*i)->m_p1.X, (*i)->m_p2.X );
        else
            x_intersect = (*i)->m_p1.X + rescale((*i)->m_p2.X - (*i)->m_p1.X,
                                                 y - (*i)->m_p1.Y,   (*i)->m_p2.Y - (*i)->m_p1.Y );

        int64_t dist = ( x - x_intersect );

        if( dist > 0 && dist < min_dist )
        {
            min_dist = dist;
            x_nearest = x_intersect;
            e_nearest = (*i);
        }
    }

    if( e_nearest && e_nearest->m_connected )
    {
        int count = 0;

        FractureEdge* lead1 = new FractureEdge( true, IntPoint( x_nearest, y), IntPoint( x, y ) );
        FractureEdge* lead2 = new FractureEdge( true, IntPoint( x, y), IntPoint( x_nearest, y ) );
        FractureEdge* split_2 = new FractureEdge( true, IntPoint( x_nearest, y ), e_nearest->m_p2 );

        edges.push_back( split_2 );
        edges.push_back( lead1 );
        edges.push_back( lead2 );

        FractureEdge* link = e_nearest->m_next;

        e_nearest->m_p2 = IntPoint( x_nearest, y );
        e_nearest->m_next = lead1;
        lead1->m_next = edge;
        FractureEdge* last;

        for( last = edge; last->m_next != edge; last = last->m_next )
        {
            last->m_connected = true;
            count++;
        }

        last->m_connected = true;
        last->m_next = lead2;
        lead2->m_next = split_2;
        split_2->m_next = link;

        return count + 1;
    }

    return 0;
}


void SHAPE_POLY_SET::fractureSingle( ClipperLib::Paths& paths )
{
    FractureEdgeSet edges;
    FractureEdgeSet border_edges;
    FractureEdge* root = NULL;

    bool first = true;

    if( paths.size() == 1 )
        return;

    int num_unconnected = 0;

    BOOST_FOREACH( Path& path, paths )
    {
        int index = 0;

        FractureEdge *prev = NULL, *first_edge = NULL;

        int64_t x_min = std::numeric_limits<int64_t>::max();

        for( unsigned i = 0; i < path.size(); i++ )
        {
            if( path[i].X < x_min )
                x_min = path[i].X;
        }

        for( unsigned i = 0; i < path.size(); i++ )
        {
            FractureEdge* fe = new FractureEdge( first, &path, index++ );

            if( !root )
                root = fe;

            if( !first_edge )
                first_edge = fe;

            if( prev )
                prev->m_next = fe;

            if( i == path.size() - 1 )
                fe->m_next = first_edge;

            prev = fe;
            edges.push_back( fe );

            if( !first )
            {
                if( fe->m_p1.X == x_min )
                    border_edges.push_back( fe );
            }

            if( !fe->m_connected )
                num_unconnected++;
        }

        first = false; // first path is always the outline
    }

    // keep connecting holes to the main outline, until there's no holes left...
    while( num_unconnected > 0 )
    {
        int64_t x_min = std::numeric_limits<int64_t>::max();
        FractureEdge* smallestX;

        // find the left-most hole edge and merge with the outline
        for( FractureEdgeSet::iterator i = border_edges.begin(); i != border_edges.end(); ++i )
        {
            int64_t xt = (*i)->m_p1.X;
            if( ( xt < x_min ) && ! (*i)->m_connected )
            {
                x_min = xt;
                smallestX = *i;
            }
        }

        num_unconnected -= processEdge( edges, smallestX );
    }

    paths.clear();
    Path newPath;
    FractureEdge* e;

    for( e = root; e->m_next != root; e = e->m_next )
        newPath.push_back( e->m_p1 );

    newPath.push_back( e->m_p1 );

    for( FractureEdgeSet::iterator i = edges.begin(); i != edges.end(); ++i )
        delete *i;

    paths.push_back( newPath );
}


void SHAPE_POLY_SET::Fracture()
{
    BOOST_FOREACH( Paths& paths, m_polys )
    {
        fractureSingle( paths );
    }
}


void SHAPE_POLY_SET::Simplify()
{
    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        Paths out;
        SimplifyPolygons( m_polys[i], out, pftNonZero );
        m_polys[i] = out;
    }
}


const std::string SHAPE_POLY_SET::Format() const
{
    std::stringstream ss;

    ss << "polyset " << m_polys.size() << "\n";

    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        ss << "poly " << m_polys[i].size() << "\n";

        for( unsigned j = 0; j < m_polys[i].size(); j++)
        {
            ss << m_polys[i][j].size() << "\n";

            for( unsigned v = 0; v < m_polys[i][j].size(); v++)
                ss << m_polys[i][j][v].X << " " << m_polys[i][j][v].Y << "\n";
        }
        ss << "\n";
    }

    return ss.str();
}


bool SHAPE_POLY_SET::Parse( std::stringstream& aStream )
{
    std::string tmp;

    aStream >> tmp;

    if( tmp != "polyset" )
        return false;

    aStream >> tmp;

    int n_polys = atoi( tmp.c_str() );

    if( n_polys < 0 )
        return false;

    for( int i = 0; i < n_polys; i++ )
    {
        ClipperLib::Paths paths;
        aStream >> tmp;

        if( tmp != "poly" )
            return false;

        aStream >> tmp;
        int n_outlines = atoi( tmp.c_str() );

        if( n_outlines < 0 )
            return false;

        for( int j = 0; j < n_outlines; j++ )
        {
            ClipperLib::Path outline;

            aStream >> tmp;
            int n_vertices = atoi( tmp.c_str() );
            for( int v = 0; v < n_vertices; v++ )
            {
                ClipperLib::IntPoint p;

                aStream >> tmp; p.X = atoi( tmp.c_str() );
                aStream >> tmp; p.Y = atoi( tmp.c_str() );
                outline.push_back( p );
            }
            paths.push_back( outline );
        }
        m_polys.push_back( paths );
    }

    return true;
}


const BOX2I SHAPE_POLY_SET::BBox( int aClearance ) const
{
    BOX2I bb;
    bool first = true;

    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        for( unsigned j = 0; j < m_polys[i].size(); j++)
        {
            for( unsigned v = 0; v < m_polys[i][j].size(); v++)
            {
                VECTOR2I p( m_polys[i][j][v].X, m_polys[i][j][v].Y );

                if( first )
                    bb =  BOX2I( p, VECTOR2I( 0, 0 ) );
                else
                    bb.Merge( p );

                first = false;
            }
        }
    }

    bb.Inflate( aClearance );
    return bb;
}
