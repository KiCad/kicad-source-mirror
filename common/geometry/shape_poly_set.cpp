/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * Point in polygon algorithm adapted from Clipper Library (C) Angus Johnson,
 * subject to Clipper library license.
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
#include <set>
#include <list>
#include <algorithm>

#include <boost/foreach.hpp>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

using namespace ClipperLib;

SHAPE_POLY_SET::SHAPE_POLY_SET() :
    SHAPE( SH_POLY_SET )
{

}


SHAPE_POLY_SET::~SHAPE_POLY_SET()
{
}


int SHAPE_POLY_SET::NewOutline()
{
    SHAPE_LINE_CHAIN empty_path;
    POLYGON poly;
    poly.push_back( empty_path );
    m_polys.push_back( poly );
    return m_polys.size() - 1;
}


int SHAPE_POLY_SET::NewHole( int aOutline )
{
    m_polys.back().push_back( SHAPE_LINE_CHAIN() );

    return m_polys.back().size() - 2;
}


int SHAPE_POLY_SET::Append( int x, int y, int aOutline, int aHole )
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

    m_polys[aOutline][idx].Append( x, y );

    return m_polys[aOutline][idx].PointCount();
}


int SHAPE_POLY_SET::VertexCount( int aOutline , int aHole  ) const
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

    return m_polys[aOutline][idx].PointCount();
}


const VECTOR2I& SHAPE_POLY_SET::CVertex( int index, int aOutline , int aHole ) const
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

    return m_polys[aOutline][idx].CPoint( index );
}


VECTOR2I& SHAPE_POLY_SET::Vertex( int index, int aOutline , int aHole )
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

    return m_polys[aOutline][idx].Point( index );
}


int SHAPE_POLY_SET::AddOutline( const SHAPE_LINE_CHAIN& aOutline )
{
    assert( aOutline.IsClosed() );

    POLYGON poly;

    poly.push_back( aOutline );

    m_polys.push_back( poly );

    return m_polys.size() - 1;
}


int SHAPE_POLY_SET::AddHole( const SHAPE_LINE_CHAIN& aHole, int aOutline )
{
    assert ( m_polys.size() );

    if( aOutline < 0 )
        aOutline += m_polys.size();

    POLYGON& poly = m_polys[aOutline];

    assert( poly.size() );

    poly.push_back( aHole );

    return poly.size() - 1;
}


const Path SHAPE_POLY_SET::convertToClipper( const SHAPE_LINE_CHAIN& aPath, bool aRequiredOrientation )
{
    Path c_path;

    for( int i = 0; i < aPath.PointCount(); i++ )
    {
        const VECTOR2I& vertex = aPath.CPoint( i );
        c_path.push_back( IntPoint( vertex.x, vertex.y ) );
    }

    if( Orientation( c_path ) != aRequiredOrientation )
        ReversePath( c_path );

    return c_path;
}


const SHAPE_LINE_CHAIN SHAPE_POLY_SET::convertFromClipper( const Path& aPath )
{
    SHAPE_LINE_CHAIN lc;

    for( unsigned int i = 0; i < aPath.size(); i++ )
        lc.Append( aPath[i].X, aPath[i].Y );

    return lc;
}


void SHAPE_POLY_SET::booleanOp( ClipType type, const SHAPE_POLY_SET& b )
{
    Clipper c;

    c.StrictlySimple( true );

    BOOST_FOREACH( const POLYGON& poly, m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptSubject, true );
    }

    BOOST_FOREACH( const POLYGON& poly, b.m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), ptClip, true );
    }

    PolyTree solution;

    c.Execute( type, solution, pftNonZero, pftNonZero );

    importTree( &solution );
}


void SHAPE_POLY_SET::BooleanAdd( const SHAPE_POLY_SET& b )
{
    booleanOp( ctUnion, b );
}


void SHAPE_POLY_SET::BooleanSubtract( const SHAPE_POLY_SET& b )
{
    booleanOp( ctDifference, b );
}


void SHAPE_POLY_SET::Inflate( int aFactor, int aCircleSegmentsCount )
{
    ClipperOffset c;

    BOOST_FOREACH( const POLYGON& poly, m_polys )
    {
        for( unsigned int i = 0; i < poly.size(); i++ )
            c.AddPath( convertToClipper( poly[i], i > 0 ? false : true ), jtRound, etClosedPolygon );
    }

    PolyTree solution;

    c.ArcTolerance = fabs( (double) aFactor ) / M_PI / aCircleSegmentsCount;

    c.Execute( solution, aFactor );

    importTree( &solution );
}


void SHAPE_POLY_SET::importTree( PolyTree* tree)
{
    m_polys.clear();

    for( PolyNode* n = tree->GetFirst(); n; n = n->GetNext() )
    {
        if( !n->IsHole() )
        {
            POLYGON paths;
            paths.push_back( convertFromClipper( n->Contour ) );

            for( unsigned int i = 0; i < n->Childs.size(); i++ )
                paths.push_back( convertFromClipper( n->Childs[i]->Contour ) );

            m_polys.push_back(paths);
        }
    }
}

// Polygon fracturing code. Work in progress.

struct FractureEdge
{
    FractureEdge( bool connected, SHAPE_LINE_CHAIN* owner, int index ) :
        m_connected( connected ),
        m_next( NULL )
    {
        m_p1 = owner->CPoint( index );
        m_p2 = owner->CPoint( index + 1 );
    }

    FractureEdge( int y = 0 ) :
        m_connected( false ),
        m_next( NULL )
    {
        m_p1.x = m_p2.y = y;
    }

    FractureEdge( bool connected, const VECTOR2I& p1, const VECTOR2I& p2 ) :
        m_connected( connected ),
        m_p1( p1 ),
        m_p2( p2 ),
        m_next( NULL )
    {
    }

    bool matches( int y ) const
    {
        int y_min = std::min( m_p1.y, m_p2.y );
        int y_max = std::max( m_p1.y, m_p2.y );

        return ( y >= y_min ) && ( y <= y_max );
    }

    bool m_connected;
    VECTOR2I m_p1, m_p2;
    FractureEdge* m_next;
};


typedef std::vector<FractureEdge*> FractureEdgeSet;

static int processEdge( FractureEdgeSet& edges, FractureEdge* edge )
{
    int x = edge->m_p1.x;
    int y = edge->m_p1.y;
    int min_dist = std::numeric_limits<int>::max();
    int x_nearest = 0;

    FractureEdge* e_nearest = NULL;

    for( FractureEdgeSet::iterator i = edges.begin(); i != edges.end(); ++i )
    {
        if( !(*i)->matches( y ) )
            continue;

        int x_intersect;

        if( (*i)->m_p1.y == (*i)->m_p2.y ) // horizontal edge
            x_intersect = std::max ( (*i)->m_p1.x, (*i)->m_p2.x );
        else
            x_intersect = (*i)->m_p1.x + rescale((*i)->m_p2.x - (*i)->m_p1.x,   y - (*i)->m_p1.y,   (*i)->m_p2.y - (*i)->m_p1.y );

        int dist = ( x - x_intersect );

        if( dist >= 0 && dist < min_dist && (*i)->m_connected )
        {
            min_dist = dist;
            x_nearest = x_intersect;
            e_nearest = (*i);
        }
    }

    if( e_nearest && e_nearest->m_connected )
    {
        int count = 0;

		FractureEdge* lead1 = new FractureEdge( true, VECTOR2I( x_nearest, y ), VECTOR2I( x, y ) );
        FractureEdge* lead2 = new FractureEdge( true, VECTOR2I( x, y ), VECTOR2I( x_nearest, y ) );
        FractureEdge* split_2 = new FractureEdge( true, VECTOR2I( x_nearest, y ), e_nearest->m_p2 );

        edges.push_back( split_2 );
        edges.push_back( lead1 );
        edges.push_back( lead2 );

        FractureEdge* link = e_nearest->m_next;

        e_nearest->m_p2 = VECTOR2I( x_nearest, y );
        e_nearest->m_next = lead1;
        lead1->m_next = edge;

        FractureEdge*last;
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

void SHAPE_POLY_SET::fractureSingle( POLYGON& paths )
{
    FractureEdgeSet edges;
    FractureEdgeSet border_edges;
    FractureEdge* root = NULL;

    bool first = true;

    if( paths.size() == 1 )
        return;

    int num_unconnected = 0;

    BOOST_FOREACH( SHAPE_LINE_CHAIN& path, paths )
    {
        int index = 0;

        FractureEdge *prev = NULL, *first_edge = NULL;

        int x_min = std::numeric_limits<int>::max();

        for( int i = 0; i < path.PointCount(); i++ )
        {
            const VECTOR2I& p = path.CPoint( i );

            if( p.x < x_min )
                x_min = p.x;
        }

        for( int i = 0; i < path.PointCount(); i++ )
        {
            FractureEdge* fe = new FractureEdge( first, &path, index++ );

            if( !root )
                root = fe;

            if( !first_edge )
                first_edge = fe;

            if( prev )
                prev->m_next = fe;

            if( i == path.PointCount() - 1 )
                fe->m_next = first_edge;

            prev = fe;
            edges.push_back( fe );

            if( !first )
            {
                if( fe->m_p1.x == x_min )
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
        int x_min = std::numeric_limits<int>::max();

        FractureEdge* smallestX = NULL;

        // find the left-most hole edge and merge with the outline
        for( FractureEdgeSet::iterator i = border_edges.begin(); i != border_edges.end(); ++i )
        {
            int xt = (*i)->m_p1.x;

            if( ( xt < x_min ) && ! (*i)->m_connected )
            {
                x_min = xt;
                smallestX = *i;
            }
        }

        num_unconnected -= processEdge( edges, smallestX );
    }

    paths.clear();
    SHAPE_LINE_CHAIN newPath;

    newPath.SetClosed( true );

    FractureEdge* e;

    for( e = root; e->m_next != root; e = e->m_next )
        newPath.Append( e->m_p1 );

    newPath.Append( e->m_p1 );

    for( FractureEdgeSet::iterator i = edges.begin(); i != edges.end(); ++i )
        delete *i;

    paths.push_back( newPath );
}


void SHAPE_POLY_SET::Fracture()
{
    Simplify(); // remove overlallping holes/degeneracy

    BOOST_FOREACH( POLYGON& paths, m_polys )
    {
        fractureSingle( paths );
    }
}


void SHAPE_POLY_SET::Simplify()
{
    SHAPE_POLY_SET empty;

    booleanOp( ctUnion, empty );
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
            ss << m_polys[i][j].PointCount() << "\n";
            for( int v = 0; v < m_polys[i][j].PointCount(); v++)
                ss << m_polys[i][j].CPoint( v ).x << " " << m_polys[i][j].CPoint( v ).y << "\n";
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
        POLYGON paths;

        aStream >> tmp;

        if( tmp != "poly" )
            return false;

        aStream >> tmp;
        int n_outlines = atoi( tmp.c_str() );

        if( n_outlines < 0 )
            return false;

        for( int j = 0; j < n_outlines; j++ )
        {
            SHAPE_LINE_CHAIN outline;

            outline.SetClosed( true );

            aStream >> tmp;
            int n_vertices = atoi( tmp.c_str() );
            for( int v = 0; v < n_vertices; v++ )
            {
                VECTOR2I p;

                aStream >> tmp; p.x = atoi( tmp.c_str() );
                aStream >> tmp; p.y = atoi( tmp.c_str() );
                outline.Append( p );
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

    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        if( i == 0 )
            bb = m_polys[i][0].BBox();
        else
            bb.Merge( m_polys[i][0].BBox() );
    }

    bb.Inflate( aClearance );
    return bb;
}


void SHAPE_POLY_SET::RemoveAllContours()
{
    m_polys.clear();
}


void SHAPE_POLY_SET::DeletePolygon( int aIdx )
{
    m_polys.erase( m_polys.begin() + aIdx );
}


void SHAPE_POLY_SET::Append( const SHAPE_POLY_SET& aSet )
{
    m_polys.insert( m_polys.end(), aSet.m_polys.begin(), aSet.m_polys.end() );
}


void SHAPE_POLY_SET::Append( const VECTOR2I& aP, int aOutline, int aHole )
{
    Append( aP.x, aP.y, aOutline, aHole );
}


bool SHAPE_POLY_SET::Contains( const VECTOR2I& aP, int aSubpolyIndex ) const
{
    // fixme: support holes!

    if( aSubpolyIndex >= 0 )
        return pointInPolygon( aP, m_polys[aSubpolyIndex][0] );

    BOOST_FOREACH ( const POLYGON& polys, m_polys )
    {
        if( pointInPolygon( aP, polys[0] ) )
            return true;
    }

    return false;
}


bool SHAPE_POLY_SET::pointInPolygon( const VECTOR2I& aP, const SHAPE_LINE_CHAIN& aPath ) const
{
    int result = 0;
    int cnt = aPath.PointCount();

    if ( !aPath.BBox().Contains( aP ) ) // test with bounding box first
        return false;

    if( cnt < 3 )
        return false;

    VECTOR2I ip = aPath.CPoint( 0 );

    for( int i = 1; i <= cnt; ++i )
    {
        VECTOR2I ipNext = ( i == cnt ? aPath.CPoint( 0 ) : aPath.CPoint( i ) );

        if( ipNext.y == aP.y )
        {
            if( ( ipNext.x == aP.x ) || ( ip.y == aP.y &&
                ( ( ipNext.x > aP.x ) == ( ip.x < aP.x ) ) ) )
                return true;
        }

        if( ( ip.y < aP.y ) != ( ipNext.y < aP.y ) )
        {
            if( ip.x >= aP.x )
            {
                if( ipNext.x > aP.x )
                    result = 1 - result;
                else
                {
                    int64_t d = (int64_t)( ip.x - aP.x ) * (int64_t)( ipNext.y - aP.y ) -
                                (int64_t)( ipNext.x - aP.x ) * (int64_t)( ip.y - aP.y );

                    if( !d )
                        return true;

                    if( ( d > 0 ) == ( ipNext.y > ip.y ) )
                        result = 1 - result;
                }
            }
            else
            {
                if( ipNext.x > aP.x )
                {
                    int64_t d = (int64_t)( ip.x - aP.x ) * (int64_t)( ipNext.y - aP.y ) -
                                (int64_t)( ipNext.x - aP.x ) * (int64_t)( ip.y - aP.y );

                if( !d )
                    return true;

                if( ( d > 0 ) == ( ipNext.y > ip.y ) )
                    result = 1 - result;
                }
            }
        }

        ip = ipNext;
    }

    return result ? true : false;
}


void SHAPE_POLY_SET::Move( const VECTOR2I& aVector )
{
    BOOST_FOREACH( POLYGON &poly, m_polys )
    {
        BOOST_FOREACH( SHAPE_LINE_CHAIN &path, poly )
        {
            path.Move( aVector );
        }
    }
}


int SHAPE_POLY_SET::TotalVertices() const
{
    int c = 0;

    BOOST_FOREACH( const POLYGON& poly, m_polys )
    {
        BOOST_FOREACH ( const SHAPE_LINE_CHAIN& path, poly )
        {
            c += path.PointCount();
        }
    }

    return c;
}
