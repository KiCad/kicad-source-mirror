/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <algorithm>
#include <assert.h>                          // for assert
#include <cmath>                             // for sqrt, cos, hypot, isinf
#include <cstdio>
#include <istream>                           // for operator<<, operator>>
#include <limits>                            // for numeric_limits
#include <map>
#include <memory>
#include <set>
#include <string> // for char_traits, operator!=
#include <unordered_set>
#include <utility> // for swap, move
#include <vector>
#include <array>

#include <clipper2/clipper.h>
#include <geometry/geometry_utils.h>
#include <geometry/polygon_triangulation.h>
#include <geometry/seg.h>                    // for SEG, OPT_VECTOR2I
#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <geometry/rtree.h>
#include <math/box2.h>                       // for BOX2I
#include <math/util.h>                       // for KiROUND, rescale
#include <math/vector2d.h>                   // for VECTOR2I, VECTOR2D, VECTOR2
#include <hash.h>
#include <mmh3_hash.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>

#include <wx/log.h>

// ADVANCED_CFG::GetCfg() cannot be used on msys2/mingw builds (link failure)
// So we use the ADVANCED_CFG default values
#if defined( __MINGW32__ )
    #define TRIANGULATESIMPLIFICATIONLEVEL 50
    #define ENABLECACHEFRIENDLYFRACTURE true
#else
    #define TRIANGULATESIMPLIFICATIONLEVEL ADVANCED_CFG::GetCfg().m_TriangulateSimplificationLevel
    #define ENABLECACHEFRIENDLYFRACTURE ADVANCED_CFG::GetCfg().m_EnableCacheFriendlyFracture
#endif

SHAPE_POLY_SET::SHAPE_POLY_SET() :
    SHAPE( SH_POLY_SET )
{
}


SHAPE_POLY_SET::SHAPE_POLY_SET( const BOX2D& aRect ) :
    SHAPE( SH_POLY_SET )
{
    NewOutline();
    Append( VECTOR2I( aRect.GetLeft(),  aRect.GetTop() ) );
    Append( VECTOR2I( aRect.GetRight(), aRect.GetTop() ) );
    Append( VECTOR2I( aRect.GetRight(), aRect.GetBottom() ) );
    Append( VECTOR2I( aRect.GetLeft(),  aRect.GetBottom() ) );
    Outline( 0 ).SetClosed( true );
}


SHAPE_POLY_SET::SHAPE_POLY_SET( const SHAPE_LINE_CHAIN& aOutline ) :
    SHAPE( SH_POLY_SET )
{
    AddOutline( aOutline );
}


SHAPE_POLY_SET::SHAPE_POLY_SET( const POLYGON& aPolygon ) :
    SHAPE( SH_POLY_SET )
{
    AddPolygon( aPolygon );
}


SHAPE_POLY_SET::SHAPE_POLY_SET( const SHAPE_POLY_SET& aOther ) :
    SHAPE( aOther ),
    m_polys( aOther.m_polys )
{
    if( aOther.IsTriangulationUpToDate() )
    {
        m_triangulatedPolys.reserve( aOther.TriangulatedPolyCount() );

        for( unsigned i = 0; i < aOther.TriangulatedPolyCount(); i++ )
        {
            const TRIANGULATED_POLYGON* poly = aOther.TriangulatedPolygon( i );
            m_triangulatedPolys.push_back( std::make_unique<TRIANGULATED_POLYGON>( *poly ) );
        }

        m_hash = aOther.GetHash();
        m_hashValid = true;
        m_triangulationValid = true;
    }
    else
    {
        m_hash.Clear();
        m_hashValid = false;
        m_triangulationValid = false;
    }
}


SHAPE_POLY_SET::SHAPE_POLY_SET( const SHAPE_POLY_SET& aOther, DROP_TRIANGULATION_FLAG ) :
    SHAPE( aOther ),
    m_polys( aOther.m_polys )
{
    m_hash.Clear();
    m_hashValid = false;
    m_triangulationValid = false;
}


SHAPE_POLY_SET::~SHAPE_POLY_SET()
{
}


SHAPE* SHAPE_POLY_SET::Clone() const
{
    return new SHAPE_POLY_SET( *this );
}


SHAPE_POLY_SET SHAPE_POLY_SET::CloneDropTriangulation() const
{
    return SHAPE_POLY_SET( *this, DROP_TRIANGULATION_FLAG::SINGLETON );
}


bool SHAPE_POLY_SET::GetRelativeIndices( int aGlobalIdx,
                                         SHAPE_POLY_SET::VERTEX_INDEX* aRelativeIndices ) const
{
    int polygonIdx = 0;
    unsigned int contourIdx = 0;
    int vertexIdx = 0;

    int currentGlobalIdx = 0;

    for( polygonIdx = 0; polygonIdx < OutlineCount(); polygonIdx++ )
    {
        const POLYGON& currentPolygon = CPolygon( polygonIdx );

        for( contourIdx = 0; contourIdx < currentPolygon.size(); contourIdx++ )
        {
            const SHAPE_LINE_CHAIN& currentContour = currentPolygon[contourIdx];
            int totalPoints = currentContour.PointCount();

            for( vertexIdx = 0; vertexIdx < totalPoints; vertexIdx++ )
            {
                // Check if the current vertex is the globally indexed as aGlobalIdx
                if( currentGlobalIdx == aGlobalIdx )
                {
                    aRelativeIndices->m_polygon = polygonIdx;
                    aRelativeIndices->m_contour = contourIdx;
                    aRelativeIndices->m_vertex  = vertexIdx;

                    return true;
                }

                // Advance
                currentGlobalIdx++;
            }
        }
    }

    return false;
}


bool SHAPE_POLY_SET::GetGlobalIndex( SHAPE_POLY_SET::VERTEX_INDEX aRelativeIndices,
                                     int& aGlobalIdx ) const
{
    int          selectedVertex = aRelativeIndices.m_vertex;
    unsigned int selectedContour = aRelativeIndices.m_contour;
    unsigned int selectedPolygon = aRelativeIndices.m_polygon;

    // Check whether the vertex indices make sense in this poly set
    if( selectedPolygon < m_polys.size() && selectedContour < m_polys[selectedPolygon].size()
        && selectedVertex < m_polys[selectedPolygon][selectedContour].PointCount() )
    {
        POLYGON currentPolygon;

        aGlobalIdx = 0;

        for( unsigned int polygonIdx = 0; polygonIdx < selectedPolygon; polygonIdx++ )
        {
            currentPolygon = Polygon( polygonIdx );

            for( unsigned int contourIdx = 0; contourIdx < currentPolygon.size(); contourIdx++ )
                aGlobalIdx += currentPolygon[contourIdx].PointCount();
        }

        currentPolygon = Polygon( selectedPolygon );

        for( unsigned int contourIdx = 0; contourIdx < selectedContour; contourIdx++ )
            aGlobalIdx += currentPolygon[contourIdx].PointCount();

        aGlobalIdx += selectedVertex;

        return true;
    }
    else
    {
        return false;
    }
}


int SHAPE_POLY_SET::NewOutline()
{
    SHAPE_LINE_CHAIN empty_path;
    POLYGON          poly;

    empty_path.SetClosed( true );
    poly.push_back( empty_path );
    m_polys.push_back( std::move( poly ) );
    return m_polys.size() - 1;
}


int SHAPE_POLY_SET::NewHole( int aOutline )
{
    SHAPE_LINE_CHAIN empty_path;

    empty_path.SetClosed( true );

    // Default outline is the last one
    if( aOutline < 0 )
        aOutline += m_polys.size();

    // Add hole to the selected outline
    m_polys[aOutline].push_back( empty_path );

    return m_polys.back().size() - 2;
}


int SHAPE_POLY_SET::Append( int x, int y, int aOutline, int aHole, bool aAllowDuplication )
{
    assert( m_polys.size() );

    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int) m_polys.size() );
    assert( idx < (int) m_polys[aOutline].size() );

    m_polys[aOutline][idx].Append( x, y, aAllowDuplication );

    return m_polys[aOutline][idx].PointCount();
}


int SHAPE_POLY_SET::Append( const SHAPE_ARC& aArc, int aOutline, int aHole,
                            std::optional<int> aMaxError )
{
    assert( m_polys.size() );

    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int) m_polys.size() );
    assert( idx < (int) m_polys[aOutline].size() );

    if( aMaxError.has_value() )
        m_polys[aOutline][idx].Append( aArc, aMaxError.value() );
    else
        m_polys[aOutline][idx].Append( aArc );

    return m_polys[aOutline][idx].PointCount();
}


void SHAPE_POLY_SET::InsertVertex( int aGlobalIndex, const VECTOR2I& aNewVertex )
{
    VERTEX_INDEX index;

    if( aGlobalIndex < 0 )
        aGlobalIndex = 0;

    if( aGlobalIndex >= TotalVertices() )
    {
        Append( aNewVertex );
    }
    else
    {
        // Assure the position to be inserted exists; throw an exception otherwise
        if( GetRelativeIndices( aGlobalIndex, &index ) )
            m_polys[index.m_polygon][index.m_contour].Insert( index.m_vertex, aNewVertex );
        else
            throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );
    }
}


int SHAPE_POLY_SET::VertexCount( int aOutline, int aHole  ) const
{
    if( m_polys.size() == 0 ) // Empty poly set
        return 0;

    if( aOutline < 0 ) // Use last outline
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    if( aOutline >= (int) m_polys.size() ) // not existing outline
        return 0;

    if( idx >= (int) m_polys[aOutline].size() ) // not existing hole
        return 0;

    return m_polys[aOutline][idx].PointCount();
}


int SHAPE_POLY_SET::FullPointCount() const
{
    int full_count = 0;

    if( m_polys.size() == 0 ) // Empty poly set
        return full_count;

    for( int ii = 0; ii < OutlineCount(); ii++ )
    {
        // the first polygon in m_polys[ii] is the main contour,
        // only others are holes:
        for( int idx = 0; idx <= HoleCount( ii ); idx++ )
        {
            full_count += m_polys[ii][idx].PointCount();
        }
    }

    return full_count;
}


SHAPE_POLY_SET SHAPE_POLY_SET::Subset( int aFirstPolygon, int aLastPolygon )
{
    assert( aFirstPolygon >= 0 && aLastPolygon <= OutlineCount() );

    SHAPE_POLY_SET newPolySet;

    for( int index = aFirstPolygon; index < aLastPolygon; index++ )
        newPolySet.m_polys.push_back( Polygon( index ) );

    return newPolySet;
}


const VECTOR2I& SHAPE_POLY_SET::CVertex( int aIndex, int aOutline, int aHole ) const
{
    if( aOutline < 0 )
        aOutline += m_polys.size();

    int idx;

    if( aHole < 0 )
        idx = 0;
    else
        idx = aHole + 1;

    assert( aOutline < (int) m_polys.size() );
    assert( idx < (int) m_polys[aOutline].size() );

    return m_polys[aOutline][idx].CPoint( aIndex );
}


const VECTOR2I& SHAPE_POLY_SET::CVertex( int aGlobalIndex ) const
{
    SHAPE_POLY_SET::VERTEX_INDEX index;

    // Assure the passed index references a legal position; abort otherwise
    if( !GetRelativeIndices( aGlobalIndex, &index ) )
        throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );

    return m_polys[index.m_polygon][index.m_contour].CPoint( index.m_vertex );
}


const VECTOR2I& SHAPE_POLY_SET::CVertex( SHAPE_POLY_SET::VERTEX_INDEX index ) const
{
    return CVertex( index.m_vertex, index.m_polygon, index.m_contour - 1 );
}


bool SHAPE_POLY_SET::GetNeighbourIndexes( int aGlobalIndex, int* aPrevious, int* aNext ) const
{
    SHAPE_POLY_SET::VERTEX_INDEX index;

    // If the edge does not exist, throw an exception, it is an illegal access memory error
    if( !GetRelativeIndices( aGlobalIndex, &index ) )
        return false;

    // Calculate the previous and next index of aGlobalIndex, corresponding to
    // the same contour;
    VERTEX_INDEX inext = index;
    int lastpoint = m_polys[index.m_polygon][index.m_contour].SegmentCount();

    if( index.m_vertex == 0 )
    {
        index.m_vertex = lastpoint - 1;
        inext.m_vertex = 1;
    }
    else if( index.m_vertex == lastpoint )
    {
        index.m_vertex--;
        inext.m_vertex = 0;
    }
    else
    {
        inext.m_vertex++;
        index.m_vertex--;

        if( inext.m_vertex == lastpoint )
            inext.m_vertex = 0;
    }

    if( aPrevious )
    {
        int previous;
        GetGlobalIndex( index, previous );
        *aPrevious = previous;
    }

    if( aNext )
    {
        int next;
        GetGlobalIndex( inext, next );
        *aNext = next;
    }

    return true;
}


bool SHAPE_POLY_SET::IsPolygonSelfIntersecting( int aPolygonIndex ) const
{
    std::vector<SEG> segments;
    segments.reserve( FullPointCount() );

    for( CONST_SEGMENT_ITERATOR it = CIterateSegmentsWithHoles( aPolygonIndex ); it; it++ )
        segments.emplace_back( *it );

    std::sort( segments.begin(), segments.end(), []( const SEG& a, const SEG& b )
            {
                int min_a_x = std::min( a.A.x, a.B.x );
                int min_b_x = std::min( b.A.x, b.B.x );

                return min_a_x < min_b_x || ( min_a_x == min_b_x && std::min( a.A.y, a.B.y ) < std::min( b.A.y, b.B.y ) );
            } );

    for( auto it = segments.begin(); it != segments.end(); ++it )
    {
        SEG& firstSegment = *it;

        // Iterate through all remaining segments.
        auto innerIterator = it;
        int max_x = std::max( firstSegment.A.x, firstSegment.B.x );
        int max_y = std::max( firstSegment.A.y, firstSegment.B.y );

        // Start in the next segment, we don't want to check collision between a segment and itself
        for( innerIterator++; innerIterator != segments.end(); innerIterator++ )
        {
            SEG& secondSegment = *innerIterator;
            int min_x = std::min( secondSegment.A.x, secondSegment.B.x );
            int min_y = std::min( secondSegment.A.y, secondSegment.B.y );

            // We are ordered in minimum point order, so checking the static max (first segment) against
            // the ordered min will tell us if any of the following segments are withing the BBox
            if( max_x < min_x || ( max_x == min_x && max_y < min_y ) )
                break;

            int index_diff = std::abs( firstSegment.Index() - secondSegment.Index() );
            bool adjacent = ( index_diff == 1) || (index_diff == ((int)segments.size() - 1) );

            // Check whether the two segments built collide, only when they are not adjacent.
            if( !adjacent && firstSegment.Collide( secondSegment, 0 ) )
                return true;
        }
    }

    return false;
}


bool SHAPE_POLY_SET::IsSelfIntersecting() const
{
    for( unsigned int polygon = 0; polygon < m_polys.size(); polygon++ )
    {
        if( IsPolygonSelfIntersecting( polygon ) )
            return true;
    }

    return false;
}


int SHAPE_POLY_SET::AddOutline( const SHAPE_LINE_CHAIN& aOutline )
{
    POLYGON poly;

    poly.push_back( aOutline );

    // This is an assertion because if it's generated by KiCad code, it probably
    // indicates a bug elsewhere that should be fixed, but we also auto-fix it here
    // for SWIG plugins that might mess it up
    wxCHECK2_MSG( aOutline.IsClosed(), poly.back().SetClosed( true ),
                  "Warning: non-closed outline added to SHAPE_POLY_SET" );

    m_polys.push_back( std::move( poly ) );

    return (int) m_polys.size() - 1;
}


int SHAPE_POLY_SET::AddHole( const SHAPE_LINE_CHAIN& aHole, int aOutline )
{
    assert( m_polys.size() );

    if( aOutline < 0 )
        aOutline += (int) m_polys.size();

    assert( aOutline < (int)m_polys.size() );

    POLYGON& poly = m_polys[aOutline];

    assert( poly.size() );

    poly.push_back( aHole );

    return (int) poly.size() - 2;
}


int SHAPE_POLY_SET::AddPolygon( const POLYGON& apolygon )
{
    m_polys.push_back( apolygon );

    return m_polys.size() - 1;
}


double SHAPE_POLY_SET::Area()
{
    double area = 0.0;

    for( int i = 0; i < OutlineCount(); i++ )
    {
        area += Outline( i ).Area( true );

        for( int j = 0; j < HoleCount( i ); j++ )
            area -= Hole( i, j ).Area( true );
    }

    return area;
}


int SHAPE_POLY_SET::ArcCount() const
{
    int retval = 0;

    for( const POLYGON& poly : m_polys )
    {
        for( size_t i = 0; i < poly.size(); i++ )
            retval += poly[i].ArcCount();
    }

    return retval;
}


void SHAPE_POLY_SET::GetArcs( std::vector<SHAPE_ARC>& aArcBuffer ) const
{
    for( const POLYGON& poly : m_polys )
    {
        for( size_t i = 0; i < poly.size(); i++ )
        {
            for( SHAPE_ARC arc : poly[i].m_arcs )
                aArcBuffer.push_back( arc );
        }
    }
}


void SHAPE_POLY_SET::ClearArcs()
{
    for( POLYGON& poly : m_polys )
    {
        for( size_t i = 0; i < poly.size(); i++ )
            poly[i].ClearArcs();
    }
}


void SHAPE_POLY_SET::RebuildHolesFromContours()
{
    std::vector<SHAPE_LINE_CHAIN> contours;

    for( const POLYGON& poly : m_polys )
        contours.insert( contours.end(), poly.begin(), poly.end() );

    std::map<int, std::set<int>> parentToChildren;
    std::map<int, std::set<int>> childToParents;

    for( SHAPE_LINE_CHAIN& contour : contours )
        contour.GenerateBBoxCache();

    for( size_t i = 0; i < contours.size(); i++ )
    {
        const SHAPE_LINE_CHAIN& outline = contours[i];

        for( size_t j = 0; j < contours.size(); j++ )
        {
            if( i == j )
                continue;

            const SHAPE_LINE_CHAIN& candidate = contours[j];
            const VECTOR2I&         pt0 = candidate.CPoint( 0 );

            if( outline.PointInside( pt0, 0, true ) )
            {
                parentToChildren[i].emplace( j );
                childToParents[j].emplace( i );
            }
        }
    }

    std::set<int> topLevelParents;

    for( size_t i = 0; i < contours.size(); i++ )
    {
        if( childToParents[i].size() == 0 )
        {
            topLevelParents.emplace( i );
        }
    }

    SHAPE_POLY_SET result;

    std::function<void( int, int, std::vector<int> )> process;

    process =
            [&]( int myId, int parentOutlineId, const std::vector<int>& path )
            {
                std::set<int> relParents = childToParents[myId];

                for( int pathId : path )
                {
                    int erased = relParents.erase( pathId );
                    wxASSERT( erased > 0 );
                }

                wxASSERT( relParents.size() == 0 );

                int myOutline = -1;

                bool isOutline = path.size() % 2 == 0;

                if( isOutline )
                {
                    int outlineId = result.AddOutline( contours[myId] );
                    myOutline = outlineId;
                }
                else
                {
                    wxASSERT( parentOutlineId != -1 );
                    result.AddHole( contours[myId], parentOutlineId );
                }

                auto it = parentToChildren.find( myId );
                if( it != parentToChildren.end() )
                {
                    std::vector<int> thisPath = path;
                    thisPath.emplace_back( myId );

                    std::set<int> thisPathSet;
                    thisPathSet.insert( thisPath.begin(), thisPath.end() );

                    for( int childId : it->second )
                    {
                        const std::set<int>& childPathSet = childToParents[childId];

                        if( thisPathSet != childPathSet )
                            continue; // Only interested in immediate children

                        process( childId, myOutline, thisPath );
                    }
                }
            };

    for( int topParentId : topLevelParents )
    {
        std::vector<int> path;
        process( topParentId, -1, std::move( path ) );
    }

    *this = std::move( result );
}


void SHAPE_POLY_SET::booleanOp( Clipper2Lib::ClipType aType, const SHAPE_POLY_SET& aOtherShape )
{
    booleanOp( aType, *this, aOtherShape );
}


void SHAPE_POLY_SET::booleanOp( Clipper2Lib::ClipType aType, const SHAPE_POLY_SET& aShape,
                                const SHAPE_POLY_SET& aOtherShape )
{
    if( ( aShape.OutlineCount() > 1 || aOtherShape.OutlineCount() > 0 )
        && ( aShape.ArcCount() > 0 || aOtherShape.ArcCount() > 0 ) )
    {
        wxFAIL_MSG( wxT( "Boolean ops on curved polygons are not supported. You should call "
                         "ClearArcs() before carrying out the boolean operation." ) );
    }

    Clipper2Lib::Clipper64 c;

    std::vector<CLIPPER_Z_VALUE> zValues;
    std::vector<SHAPE_ARC> arcBuffer;
    std::map<VECTOR2I, CLIPPER_Z_VALUE> newIntersectPoints;

    Clipper2Lib::Paths64 paths;
    Clipper2Lib::Paths64 clips;

    for( const POLYGON& poly : aShape.m_polys )
    {
        for( size_t i = 0; i < poly.size(); i++ )
        {
            paths.push_back( poly[i].convertToClipper2( i == 0, zValues, arcBuffer ) );
        }
    }

    for( const POLYGON& poly : aOtherShape.m_polys )
    {
        for( size_t i = 0; i < poly.size(); i++ )
        {
            clips.push_back( poly[i].convertToClipper2( i == 0, zValues, arcBuffer ) );
        }
    }

    c.AddSubject( paths );
    c.AddClip( clips );

    Clipper2Lib::PolyTree64 solution;

    Clipper2Lib::ZCallback64 callback =
            [&]( const Clipper2Lib::Point64 & e1bot, const Clipper2Lib::Point64 & e1top,
                 const Clipper2Lib::Point64 & e2bot, const Clipper2Lib::Point64 & e2top,
                 Clipper2Lib::Point64 & pt )
            {
                auto arcIndex =
                    [&]( const ssize_t& aZvalue, const ssize_t& aCompareVal = -1 ) -> ssize_t
                    {
                        ssize_t retval;

                        retval = zValues.at( aZvalue ).m_SecondArcIdx;

                        if( retval == -1 || ( aCompareVal > 0 && retval != aCompareVal ) )
                            retval = zValues.at( aZvalue ).m_FirstArcIdx;

                        return retval;
                    };

                auto arcSegment =
                    [&]( const ssize_t& aBottomZ, const ssize_t aTopZ ) -> ssize_t
                    {
                        ssize_t retval = arcIndex( aBottomZ );

                        if( retval != -1 )
                        {
                            if( retval != arcIndex( aTopZ, retval ) )
                                retval = -1; // Not an arc segment as the two indices do not match
                        }

                        return retval;
                    };

                ssize_t e1ArcSegmentIndex = arcSegment( e1bot.z, e1top.z );
                ssize_t e2ArcSegmentIndex = arcSegment( e2bot.z, e2top.z );

                CLIPPER_Z_VALUE newZval;

                if( e1ArcSegmentIndex != -1 )
                {
                    newZval.m_FirstArcIdx = e1ArcSegmentIndex;
                    newZval.m_SecondArcIdx = e2ArcSegmentIndex;
                }
                else
                {
                    newZval.m_FirstArcIdx = e2ArcSegmentIndex;
                    newZval.m_SecondArcIdx = -1;
                }

                size_t z_value_ptr = zValues.size();
                zValues.push_back( newZval );

                // Only worry about arc segments for later processing
                if( newZval.m_FirstArcIdx != -1 )
                    newIntersectPoints.insert( { VECTOR2I( pt.x, pt.y ), newZval } );

                pt.z = z_value_ptr;
                //@todo amend X,Y values to true intersection between arcs or arc and segment
            };

    c.SetZCallback( std::move( callback ) ); // register callback

    c.Execute( aType, Clipper2Lib::FillRule::NonZero, solution );

    importTree( solution, zValues, arcBuffer );
    solution.Clear(); // Free used memory (not done in dtor)
}


void SHAPE_POLY_SET::BooleanAdd( const SHAPE_POLY_SET& b )
{
    booleanOp( Clipper2Lib::ClipType::Union, b );
}


void SHAPE_POLY_SET::BooleanSubtract( const SHAPE_POLY_SET& b )
{
    booleanOp( Clipper2Lib::ClipType::Difference, b );
}


void SHAPE_POLY_SET::BooleanIntersection( const SHAPE_POLY_SET& b )
{
    booleanOp( Clipper2Lib::ClipType::Intersection, b );
}


void SHAPE_POLY_SET::BooleanXor( const SHAPE_POLY_SET& b )
{
    booleanOp( Clipper2Lib::ClipType::Xor, b );
}


void SHAPE_POLY_SET::BooleanAdd( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b )
{
    booleanOp( Clipper2Lib::ClipType::Union, a, b );
}


void SHAPE_POLY_SET::BooleanSubtract( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b )
{
    booleanOp( Clipper2Lib::ClipType::Difference, a, b );
}


void SHAPE_POLY_SET::BooleanIntersection( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b )
{
    booleanOp( Clipper2Lib::ClipType::Intersection, a, b );
}


void SHAPE_POLY_SET::BooleanXor( const SHAPE_POLY_SET& a, const SHAPE_POLY_SET& b )
{
    booleanOp( Clipper2Lib::ClipType::Xor, a, b );
}


void SHAPE_POLY_SET::InflateWithLinkedHoles( int aFactor, CORNER_STRATEGY aCornerStrategy,
                                             int aMaxError )
{
    Unfracture();
    Inflate( aFactor, aCornerStrategy, aMaxError );
    Fracture();
}


void SHAPE_POLY_SET::inflate2( int aAmount, int aCircleSegCount, CORNER_STRATEGY aCornerStrategy,
        bool aSimplify )
{
    using namespace Clipper2Lib;
    // A static table to avoid repetitive calculations of the coefficient
    // 1.0 - cos( M_PI / aCircleSegCount )
    // aCircleSegCount is most of time <= 64 and usually 8, 12, 16, 32
    #define SEG_CNT_MAX 64
    static double arc_tolerance_factor[SEG_CNT_MAX + 1];

    ClipperOffset c;

    // N.B. see the Clipper documentation for jtSquare/jtMiter/jtRound.  They are poorly named
    // and are not what you'd think they are.
    // http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/JoinType.htm
    JoinType joinType = JoinType::Round;    // The way corners are offsetted
    double   miterLimit = 2.0;      // Smaller value when using jtMiter for joinType

    switch( aCornerStrategy )
    {
    case CORNER_STRATEGY::ALLOW_ACUTE_CORNERS:
        joinType = JoinType::Miter;
        miterLimit = 10;        // Allows large spikes
        break;

    case CORNER_STRATEGY::CHAMFER_ACUTE_CORNERS: // Acute angles are chamfered
        joinType = JoinType::Miter;
        break;

    case CORNER_STRATEGY::ROUND_ACUTE_CORNERS: // Acute angles are rounded
        joinType = JoinType::Miter;
        break;

    case CORNER_STRATEGY::CHAMFER_ALL_CORNERS: // All angles are chamfered.
        joinType = JoinType::Square;
        break;

    case CORNER_STRATEGY::ROUND_ALL_CORNERS: // All angles are rounded.
        joinType = JoinType::Round;
        break;
    }

    std::vector<CLIPPER_Z_VALUE> zValues;
    std::vector<SHAPE_ARC>       arcBuffer;

    for( const POLYGON& poly : m_polys )
    {
        Paths64 paths;

        for( size_t i = 0; i < poly.size(); i++ )
            paths.push_back( poly[i].convertToClipper2( i == 0, zValues, arcBuffer ) );

        c.AddPaths( paths, joinType, EndType::Polygon );
    }

    // Calculate the arc tolerance (arc error) from the seg count by circle. The seg count is
    // nn = M_PI / acos(1.0 - c.ArcTolerance / abs(aAmount))
    // http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Classes/ClipperOffset/Properties/ArcTolerance.htm

    if( aCircleSegCount < 6 ) // avoid incorrect aCircleSegCount values
        aCircleSegCount = 6;

    double coeff;

    if( aCircleSegCount > SEG_CNT_MAX || arc_tolerance_factor[aCircleSegCount] == 0 )
    {
        coeff = 1.0 - cos( M_PI / aCircleSegCount );

        if( aCircleSegCount <= SEG_CNT_MAX )
            arc_tolerance_factor[aCircleSegCount] = coeff;
    }
    else
    {
        coeff = arc_tolerance_factor[aCircleSegCount];
    }

    c.ArcTolerance( std::abs( aAmount ) * coeff );
    c.MiterLimit( miterLimit );

    PolyTree64 tree;

    if( aSimplify )
    {
        Paths64 paths;
        c.Execute( aAmount, paths );

        Clipper2Lib::SimplifyPaths( paths, std::abs( aAmount ) * coeff, true );

        Clipper64 c2;
        c2.PreserveCollinear( false );
        c2.ReverseSolution( false );
        c2.AddSubject( paths );
        c2.Execute(ClipType::Union, FillRule::Positive, tree);
    }
    else
    {
        c.Execute( aAmount, tree );
    }

    importTree( tree, zValues, arcBuffer );
    tree.Clear();
}


void SHAPE_POLY_SET::inflateLine2( const SHAPE_LINE_CHAIN& aLine, int aAmount, int aCircleSegCount,
                                   CORNER_STRATEGY aCornerStrategy, bool aSimplify )
{
    using namespace Clipper2Lib;
    // A static table to avoid repetitive calculations of the coefficient
    // 1.0 - cos( M_PI / aCircleSegCount )
    // aCircleSegCount is most of time <= 64 and usually 8, 12, 16, 32
    #define SEG_CNT_MAX 64
    static double arc_tolerance_factor[SEG_CNT_MAX + 1];

    ClipperOffset c;

    // N.B. see the Clipper documentation for jtSquare/jtMiter/jtRound.  They are poorly named
    // and are not what you'd think they are.
    // http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Types/JoinType.htm
    JoinType joinType = JoinType::Round; // The way corners are offsetted
    double   miterLimit = 2.0;           // Smaller value when using jtMiter for joinType

    switch( aCornerStrategy )
    {
    case CORNER_STRATEGY::ALLOW_ACUTE_CORNERS:
        joinType = JoinType::Miter;
        miterLimit = 10; // Allows large spikes
        break;

    case CORNER_STRATEGY::CHAMFER_ACUTE_CORNERS: // Acute angles are chamfered
        joinType = JoinType::Miter;
        break;

    case CORNER_STRATEGY::ROUND_ACUTE_CORNERS: // Acute angles are rounded
        joinType = JoinType::Miter;
        break;

    case CORNER_STRATEGY::CHAMFER_ALL_CORNERS: // All angles are chamfered.
        joinType = JoinType::Square;
        break;

    case CORNER_STRATEGY::ROUND_ALL_CORNERS: // All angles are rounded.
        joinType = JoinType::Round;
        break;
    }

    std::vector<CLIPPER_Z_VALUE> zValues;
    std::vector<SHAPE_ARC>       arcBuffer;

    Path64 path = aLine.convertToClipper2( true, zValues, arcBuffer );
    c.AddPath( path, joinType, EndType::Butt );

    // Calculate the arc tolerance (arc error) from the seg count by circle. The seg count is
    // nn = M_PI / acos(1.0 - c.ArcTolerance / abs(aAmount))
    // http://www.angusj.com/delphi/clipper/documentation/Docs/Units/ClipperLib/Classes/ClipperOffset/Properties/ArcTolerance.htm

    if( aCircleSegCount < 6 ) // avoid incorrect aCircleSegCount values
        aCircleSegCount = 6;

    double coeff;

    if( aCircleSegCount > SEG_CNT_MAX || arc_tolerance_factor[aCircleSegCount] == 0 )
    {
        coeff = 1.0 - cos( M_PI / aCircleSegCount );

        if( aCircleSegCount <= SEG_CNT_MAX )
            arc_tolerance_factor[aCircleSegCount] = coeff;
    }
    else
    {
        coeff = arc_tolerance_factor[aCircleSegCount];
    }

    c.ArcTolerance( std::abs( aAmount ) * coeff );
    c.MiterLimit( miterLimit );

    PolyTree64 tree;

    if( aSimplify )
    {
        Paths64 paths2;
        c.Execute( aAmount, paths2 );

        Clipper2Lib::SimplifyPaths( paths2, std::abs( aAmount ) * coeff, false );

        Clipper64 c2;
        c2.PreserveCollinear( false );
        c2.ReverseSolution( false );
        c2.AddSubject( paths2 );
        c2.Execute( ClipType::Union, FillRule::Positive, tree );
    }
    else
    {
        c.Execute( aAmount, tree );
    }

    importTree( tree, zValues, arcBuffer );
    tree.Clear();
}


void SHAPE_POLY_SET::Inflate( int aAmount, CORNER_STRATEGY aCornerStrategy, int aMaxError,
                              bool aSimplify )
{
    int segCount = GetArcToSegmentCount( std::abs( aAmount ), aMaxError, FULL_CIRCLE );

    inflate2( aAmount, segCount, aCornerStrategy, aSimplify );
}


void SHAPE_POLY_SET::OffsetLineChain( const SHAPE_LINE_CHAIN& aLine, int aAmount,
                                  CORNER_STRATEGY aCornerStrategy, int aMaxError, bool aSimplify )
{
    int segCount = GetArcToSegmentCount( std::abs( aAmount ), aMaxError, FULL_CIRCLE );

    inflateLine2( aLine, aAmount, segCount, aCornerStrategy, aSimplify );
}


void SHAPE_POLY_SET::importPolyPath( const std::unique_ptr<Clipper2Lib::PolyPath64>& aPolyPath,
                                     const std::vector<CLIPPER_Z_VALUE>&             aZValueBuffer,
                                     const std::vector<SHAPE_ARC>&                   aArcBuffer )
{
    if( !aPolyPath->IsHole() )
    {
        POLYGON paths;
        paths.reserve( aPolyPath->Count() + 1 );
        paths.emplace_back( aPolyPath->Polygon(), aZValueBuffer, aArcBuffer );

        for( const std::unique_ptr<Clipper2Lib::PolyPath64>& child : *aPolyPath )
        {
            paths.emplace_back( child->Polygon(), aZValueBuffer, aArcBuffer );

            for( const std::unique_ptr<Clipper2Lib::PolyPath64>& grandchild : *child )
                importPolyPath( grandchild, aZValueBuffer, aArcBuffer );
        }

        m_polys.emplace_back( std::move( paths ) );
    }
}


void SHAPE_POLY_SET::importTree( Clipper2Lib::PolyTree64&            tree,
                                 const std::vector<CLIPPER_Z_VALUE>& aZValueBuffer,
                                 const std::vector<SHAPE_ARC>&       aArcBuffer )
{
    m_polys.clear();

    for( const std::unique_ptr<Clipper2Lib::PolyPath64>& n : tree )
        importPolyPath( n, aZValueBuffer, aArcBuffer );
}


void SHAPE_POLY_SET::importPaths( Clipper2Lib::Paths64&              aPath,
                                 const std::vector<CLIPPER_Z_VALUE>& aZValueBuffer,
                                 const std::vector<SHAPE_ARC>&       aArcBuffer )
{
    m_polys.clear();
    POLYGON path;

    for( const Clipper2Lib::Path64& n : aPath )
    {
        if( Clipper2Lib::Area( n ) > 0 )
        {
            if( !path.empty() )
                m_polys.emplace_back( path );

            path.clear();
        }
        else
        {
            wxCHECK2_MSG( !path.empty(), continue, wxT( "Cannot add a hole before an outline" ) );
        }

        path.emplace_back( n, aZValueBuffer, aArcBuffer );
    }

    if( !path.empty() )
        m_polys.emplace_back( std::move( path ) );
}


struct FractureEdge
{
    using Index = int;

    FractureEdge() = default;

    FractureEdge( const VECTOR2I& p1, const VECTOR2I& p2, Index next ) :
            m_p1( p1 ),
            m_p2( p2 ),
            m_next( next )
    {
    }

    bool matches( int y ) const
    {
        return ( y >= m_p1.y || y >= m_p2.y ) && ( y <= m_p1.y || y <= m_p2.y );
    }

    VECTOR2I m_p1;
    VECTOR2I m_p2;
    Index    m_next;
};


typedef std::vector<FractureEdge> FractureEdgeSet;


static FractureEdge* processHole( FractureEdgeSet& edges, FractureEdge::Index provokingIndex,
                                  FractureEdge::Index edgeIndex, FractureEdge::Index bridgeIndex )
{
    FractureEdge& edge = edges[edgeIndex];
    int           x = edge.m_p1.x;
    int           y = edge.m_p1.y;
    int           min_dist = std::numeric_limits<int>::max();
    int           x_nearest = 0;

    FractureEdge* e_nearest = nullptr;

    // Since this function is run for all holes left to right, no need to
    // check for any edge beyond the provoking one because they will always be
    // further to the right, and unconnected to the outline anyway.
    for( FractureEdge::Index i = 0; i < provokingIndex; i++ )
    {
        FractureEdge& e = edges[i];
        // Don't consider this edge if it can't be bridged to, or faces left.
        if( !e.matches( y ) )
            continue;

        int x_intersect;

        if( e.m_p1.y == e.m_p2.y ) // horizontal edge
        {
            x_intersect = std::max( e.m_p1.x, e.m_p2.x );
        }
        else
        {
            x_intersect =
                    e.m_p1.x + rescale( e.m_p2.x - e.m_p1.x, y - e.m_p1.y, e.m_p2.y - e.m_p1.y );
        }

        int dist = ( x - x_intersect );

        if( dist >= 0 && dist < min_dist )
        {
            min_dist = dist;
            x_nearest = x_intersect;
            e_nearest = &e;
        }
    }

    if( e_nearest )
    {
        const FractureEdge::Index outline2hole_index = bridgeIndex;
        const FractureEdge::Index hole2outline_index = bridgeIndex + 1;
        const FractureEdge::Index split_index = bridgeIndex + 2;
        // Make an edge between the split outline edge and the hole...
        edges[outline2hole_index] = FractureEdge( VECTOR2I( x_nearest, y ), edge.m_p1, edgeIndex );
        // ...between the hole and the edge...
        edges[hole2outline_index] =
                FractureEdge( edge.m_p1, VECTOR2I( x_nearest, y ), split_index );
        // ...and between the split outline edge and the rest.
        edges[split_index] =
                FractureEdge( VECTOR2I( x_nearest, y ), e_nearest->m_p2, e_nearest->m_next );

        // Perform the actual outline edge split
        e_nearest->m_p2 = VECTOR2I( x_nearest, y );
        e_nearest->m_next = outline2hole_index;

        FractureEdge* last = &edge;
        for( ; last->m_next != edgeIndex; last = &edges[last->m_next] )
            ;
        last->m_next = hole2outline_index;
    }

    return e_nearest;
}


static void fractureSingleCacheFriendly( SHAPE_POLY_SET::POLYGON& paths )
{
    FractureEdgeSet edges;
    bool            outline = true;

    if( paths.size() == 1 )
        return;

    size_t total_point_count = 0;

    for( const SHAPE_LINE_CHAIN& path : paths )
    {
        total_point_count += path.PointCount();
    }

    if( total_point_count > (size_t) std::numeric_limits<FractureEdge::Index>::max() )
    {
        wxLogWarning( wxT( "Polygon has more points than int limit" ) );
        return;
    }

    // Reserve space in the edge set so pointers don't get invalidated during
    // the whole fracture process; one for each original edge, plus 3 per
    // path to join it to the outline.
    edges.reserve( total_point_count + paths.size() * 3 );

    // Sort the paths by their lowest X bound before processing them.
    // This ensures the processing order for processEdge() is correct.
    struct PathInfo
    {
        int                 path_or_provoking_index;
        FractureEdge::Index leftmost;
        int                 x;
        int                 y_or_bridge;
    };
    std::vector<PathInfo> sorted_paths;
    const int             paths_count = static_cast<int>( paths.size() );
    sorted_paths.reserve( paths_count );

    for( int path_index = 0; path_index < paths_count; path_index++ )
    {
        const SHAPE_LINE_CHAIN&      path = paths[path_index];
        const std::vector<VECTOR2I>& points = path.CPoints();
        const int                    point_count = static_cast<int>( points.size() );
        int                          x_min = std::numeric_limits<int>::max();
        int                          y_min = std::numeric_limits<int>::max();
        int                          leftmost = -1;

        for( int point_index = 0; point_index < point_count; point_index++ )
        {
            const VECTOR2I& point = points[point_index];
            if( point.x < x_min )
            {
                x_min = point.x;
                leftmost = point_index;
            }
            if( point.y < y_min )
                y_min = point.y;
        }

        sorted_paths.emplace_back( PathInfo{ path_index, leftmost, x_min, y_min } );
    }

    std::sort( sorted_paths.begin() + 1, sorted_paths.end(),
               []( const PathInfo& a, const PathInfo& b )
               {
                   if( a.x == b.x )
                       return a.y_or_bridge < b.y_or_bridge;
                   return a.x < b.x;
               } );

    FractureEdge::Index edge_index = 0;

    for( PathInfo& path_info : sorted_paths )
    {
        const SHAPE_LINE_CHAIN&      path = paths[path_info.path_or_provoking_index];
        const std::vector<VECTOR2I>& points = path.CPoints();
        const size_t                 point_count = points.size();

        // Index of the provoking (first) edge for this path
        const FractureEdge::Index provoking_edge = edge_index;

        for( size_t i = 0; i < point_count - 1; i++ )
        {
            edges.emplace_back( points[i], points[i + 1], edge_index + 1 );
            edge_index++;
        }

        // Create last edge looping back to the provoking one.
        edges.emplace_back( points[point_count - 1], points[0], provoking_edge );
        edge_index++;

        if( !outline )
        {
            // Repurpose the path sorting data structure to schedule the leftmost edge
            // for merging to the outline, which will in turn merge the rest of the path.
            path_info.path_or_provoking_index = provoking_edge;
            path_info.y_or_bridge = edge_index;

            // Reserve 3 additional edges to bridge with the outline.
            edge_index += 3;
            edges.resize( edge_index );
        }

        outline = false; // first path is always the outline
    }

    for( auto it = sorted_paths.begin() + 1; it != sorted_paths.end(); it++ )
    {
        auto edge = processHole( edges, it->path_or_provoking_index,
                                 it->path_or_provoking_index + it->leftmost, it->y_or_bridge );

        // If we can't handle the hole, the zone is broken (maybe)
        if( !edge )
        {
            wxLogWarning( wxT( "Broken polygon, dropping path" ) );

            return;
        }
    }

    paths.resize( 1 );
    SHAPE_LINE_CHAIN& newPath = paths[0];

    newPath.Clear();
    newPath.SetClosed( true );

    // Root edge is always at index 0
    FractureEdge* e = &edges[0];

    for( ; e->m_next != 0; e = &edges[e->m_next] )
        newPath.Append( e->m_p1 );

    newPath.Append( e->m_p1 );
}


struct FractureEdgeSlow
{
    FractureEdgeSlow( int y = 0 ) : m_connected( false ), m_next( nullptr ) { m_p1.x = m_p2.y = y; }

    FractureEdgeSlow( bool connected, const VECTOR2I& p1, const VECTOR2I& p2 ) :
            m_connected( connected ), m_p1( p1 ), m_p2( p2 ), m_next( nullptr )
    {
    }

    bool matches( int y ) const
    {
        return ( y >= m_p1.y || y >= m_p2.y ) && ( y <= m_p1.y || y <= m_p2.y );
    }

    bool              m_connected;
    VECTOR2I          m_p1;
    VECTOR2I          m_p2;
    FractureEdgeSlow* m_next;
};


typedef std::vector<FractureEdgeSlow*> FractureEdgeSetSlow;


static int processEdge( FractureEdgeSetSlow& edges, FractureEdgeSlow* edge )
{
    int x = edge->m_p1.x;
    int y = edge->m_p1.y;
    int min_dist = std::numeric_limits<int>::max();
    int x_nearest = 0;

    FractureEdgeSlow* e_nearest = nullptr;

    for( FractureEdgeSlow* e : edges )
    {
        if( !e->matches( y ) )
            continue;

        int x_intersect;

        if( e->m_p1.y == e->m_p2.y ) // horizontal edge
        {
            x_intersect = std::max( e->m_p1.x, e->m_p2.x );
        }
        else
        {
            x_intersect = e->m_p1.x
                          + rescale( e->m_p2.x - e->m_p1.x, y - e->m_p1.y, e->m_p2.y - e->m_p1.y );
        }

        int dist = ( x - x_intersect );

        if( dist >= 0 && dist < min_dist && e->m_connected )
        {
            min_dist = dist;
            x_nearest = x_intersect;
            e_nearest = e;
        }
    }

    if( e_nearest && e_nearest->m_connected )
    {
        int count = 0;

        FractureEdgeSlow* lead1 =
                new FractureEdgeSlow( true, VECTOR2I( x_nearest, y ), VECTOR2I( x, y ) );
        FractureEdgeSlow* lead2 =
                new FractureEdgeSlow( true, VECTOR2I( x, y ), VECTOR2I( x_nearest, y ) );
        FractureEdgeSlow* split_2 =
                new FractureEdgeSlow( true, VECTOR2I( x_nearest, y ), e_nearest->m_p2 );

        edges.push_back( split_2 );
        edges.push_back( lead1 );
        edges.push_back( lead2 );

        FractureEdgeSlow* link = e_nearest->m_next;

        e_nearest->m_p2 = VECTOR2I( x_nearest, y );
        e_nearest->m_next = lead1;
        lead1->m_next = edge;

        FractureEdgeSlow* last;

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


static void fractureSingleSlow( SHAPE_POLY_SET::POLYGON& paths )
{
    FractureEdgeSetSlow edges;
    FractureEdgeSetSlow border_edges;
    FractureEdgeSlow*   root = nullptr;

    bool first = true;

    if( paths.size() == 1 )
        return;

    int num_unconnected = 0;

    for( const SHAPE_LINE_CHAIN& path : paths )
    {
        const std::vector<VECTOR2I>& points = path.CPoints();
        int                          pointCount = points.size();

        FractureEdgeSlow *prev = nullptr, *first_edge = nullptr;

        int x_min = std::numeric_limits<int>::max();

        for( int i = 0; i < pointCount; i++ )
        {
            if( points[i].x < x_min )
                x_min = points[i].x;

            // Do not use path.CPoint() here; open-coding it using the local variables "points"
            // and "pointCount" gives a non-trivial performance boost to zone fill times.
            FractureEdgeSlow* fe = new FractureEdgeSlow( first, points[i],
                                                         points[i + 1 == pointCount ? 0 : i + 1] );

            if( !root )
                root = fe;

            if( !first_edge )
                first_edge = fe;

            if( prev )
                prev->m_next = fe;

            if( i == pointCount - 1 )
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
        int  x_min = std::numeric_limits<int>::max();
        auto it = border_edges.begin();

        FractureEdgeSlow* smallestX = nullptr;

        // find the left-most hole edge and merge with the outline
        for( ; it != border_edges.end(); ++it )
        {
            FractureEdgeSlow* border_edge = *it;
            int               xt = border_edge->m_p1.x;

            if( ( xt <= x_min ) && !border_edge->m_connected )
            {
                x_min = xt;
                smallestX = border_edge;
            }
        }

        int num_processed = processEdge( edges, smallestX );

        // If we can't handle the edge, the zone is broken (maybe)
        if( !num_processed )
        {
            wxLogWarning( wxT( "Broken polygon, dropping path" ) );

            for( FractureEdgeSlow* edge : edges )
                delete edge;

            return;
        }

        num_unconnected -= num_processed;
    }

    paths.clear();
    SHAPE_LINE_CHAIN newPath;

    newPath.SetClosed( true );

    FractureEdgeSlow* e;

    for( e = root; e->m_next != root; e = e->m_next )
        newPath.Append( e->m_p1 );

    newPath.Append( e->m_p1 );

    for( FractureEdgeSlow* edge : edges )
        delete edge;

    paths.push_back( std::move( newPath ) );
}


void SHAPE_POLY_SET::fractureSingle( POLYGON& paths )
{
    if( ENABLECACHEFRIENDLYFRACTURE )
        return fractureSingleCacheFriendly( paths );
    fractureSingleSlow( paths );
}


void SHAPE_POLY_SET::Fracture()
{
    Simplify();    // remove overlapping holes/degeneracy

    for( POLYGON& paths : m_polys )
        fractureSingle( paths );
}


void SHAPE_POLY_SET::unfractureSingle( SHAPE_POLY_SET::POLYGON& aPoly )
{
    assert( aPoly.size() == 1 );

    struct EDGE
    {
        int m_index = 0;
        SHAPE_LINE_CHAIN* m_poly = nullptr;
        bool m_duplicate = false;

        EDGE( SHAPE_LINE_CHAIN* aPolygon, int aIndex ) :
            m_index( aIndex ),
            m_poly( aPolygon )
        {}

        bool compareSegs( const SEG& s1, const SEG& s2 ) const
        {
            return (s1.A == s2.B && s1.B == s2.A);
        }

        bool operator==( const EDGE& aOther ) const
        {
            return compareSegs( m_poly->CSegment( m_index ),
                                aOther.m_poly->CSegment( aOther.m_index ) );
        }

        bool operator!=( const EDGE& aOther ) const
        {
            return !compareSegs( m_poly->CSegment( m_index ),
                                 aOther.m_poly->CSegment( aOther.m_index ) );
        }

        struct HASH
        {
            std::size_t operator()(  const EDGE& aEdge ) const
            {
                const SEG& a = aEdge.m_poly->CSegment( aEdge.m_index );
                std::size_t seed = 0xa82de1c0;
                hash_combine( seed, a.A.x, a.B.x, a.A.y, a.B.y );
                return seed;
            }
        };
    };

    struct EDGE_LIST_ENTRY
    {
        int              index;
        EDGE_LIST_ENTRY* next;
    };

    std::unordered_set<EDGE, EDGE::HASH> uniqueEdges;

    SHAPE_LINE_CHAIN lc = aPoly[0];
    lc.Simplify();

    auto edgeList = std::make_unique<EDGE_LIST_ENTRY[]>( lc.SegmentCount() );

    for( int i = 0; i < lc.SegmentCount(); i++ )
    {
        edgeList[i].index   = i;
        edgeList[i].next    = &edgeList[ (i != lc.SegmentCount() - 1) ? i + 1 : 0 ];
    }

    std::unordered_set<EDGE_LIST_ENTRY*> queue;

    for( int i = 0; i < lc.SegmentCount(); i++ )
    {
        EDGE e( &lc, i );
        uniqueEdges.insert( e );
    }

    for( int i = 0; i < lc.SegmentCount(); i++ )
    {
        EDGE    e( &lc, i );
        auto    it = uniqueEdges.find( e );

        if( it != uniqueEdges.end() && it->m_index != i )
        {
            int e1  = it->m_index;
            int e2  = i;

            if( e1 > e2 )
                std::swap( e1, e2 );

            int e1_prev = e1 - 1;

            if( e1_prev < 0 )
                e1_prev = lc.SegmentCount() - 1;

            int e2_prev = e2 - 1;

            if( e2_prev < 0 )
                e2_prev = lc.SegmentCount() - 1;

            int e1_next = e1 + 1;

            if( e1_next == lc.SegmentCount() )
                e1_next = 0;

            int e2_next = e2 + 1;

            if( e2_next == lc.SegmentCount() )
                e2_next = 0;

            edgeList[e1_prev].next  = &edgeList[ e2_next ];
            edgeList[e2_prev].next  = &edgeList[ e1_next ];
            edgeList[i].next = nullptr;
            edgeList[it->m_index].next = nullptr;
        }
    }

    for( int i = 0; i < lc.SegmentCount(); i++ )
    {
        if( edgeList[i].next )
            queue.insert( &edgeList[i] );
    }

    auto edgeBuf = std::make_unique<EDGE_LIST_ENTRY* []>( lc.SegmentCount() );

    int n = 0;
    int outline = -1;

    POLYGON result;
    double max_poly = 0.0;

    while( queue.size() )
    {
        EDGE_LIST_ENTRY* e_first = *queue.begin();
        EDGE_LIST_ENTRY* e = e_first;
        int              cnt = 0;

        do
        {
            edgeBuf[cnt++] = e;
            e = e->next;
        } while( e && e != e_first );

        SHAPE_LINE_CHAIN outl;

        for( int i = 0; i < cnt; i++ )
        {
            VECTOR2I p = lc.CPoint( edgeBuf[i]->index );
            outl.Append( p );
            queue.erase( edgeBuf[i] );
        }

        outl.SetClosed( true );

        double area = std::fabs( outl.Area() );

        if( area > max_poly )
        {
            outline = n;
            max_poly = area;
        }

        result.push_back( outl );
        n++;
    }

    if( outline > 0 )
        std::swap( result[0], result[outline] );

    aPoly = std::move( result );
}


bool SHAPE_POLY_SET::HasHoles() const
{
    // Iterate through all the polygons on the set
    for( const POLYGON& paths : m_polys )
    {
        // If any of them has more than one contour, it is a hole.
        if( paths.size() > 1 )
            return true;
    }

    // Return false if and only if every polygon has just one outline, without holes.
    return false;
}


void SHAPE_POLY_SET::Unfracture()
{
    for( POLYGON& path : m_polys )
        unfractureSingle( path );

    Simplify();    // remove overlapping holes/degeneracy
}


bool SHAPE_POLY_SET::isExteriorWaist( const SEG& aSegA, const SEG& aSegB ) const
{
    const VECTOR2I da = aSegA.B - aSegA.A;

    int axis = std::abs( da.x ) >= std::abs( da.y ) ? 0 : 1;

    std::array<VECTOR2I,4> pts = { aSegA.A, aSegA.B, aSegB.A, aSegB.B };

    std::sort( pts.begin(), pts.end(), [axis]( const VECTOR2I& p, const VECTOR2I& q )
    {
        if( axis == 0 )
            return p.x < q.x || ( p.x == q.x && p.y < q.y );
        else
            return p.y < q.y || ( p.y == q.y && p.x < q.x );
    } );

    VECTOR2I s = pts[1];
    VECTOR2I e = pts[2];

    // Check if there is polygon material on either side of the overlapping segments
    // Get the midpoint between s and e for testing
    VECTOR2I midpoint = ( s + e ) / 2;

    // Create perpendicular offset vector to check both sides
    VECTOR2I segDir = e - s;

    if( segDir.EuclideanNorm() > 25 )
    {
        VECTOR2I perp = segDir.Perpendicular().Resize( 10 );

        // Test points on both sides of the overlapping segment
        bool side1 = PointInside( midpoint + perp );
        bool side2 = PointInside( midpoint - perp );

        // Only return true if both sides are outside the polygon
        // This is the case for non-fractured segments
        if( !side1 && !side2 )
        {
            wxLogTrace( wxT( "collinear" ), wxT( "Found exterior waist between (%d,%d)-(%d,%d) and (%d,%d)-(%d,%d)" ),
                        aSegA.A.x, aSegA.A.y, aSegA.B.x, aSegA.B.y,
                        aSegB.A.x, aSegB.A.y, aSegB.B.x, aSegB.B.y );
            return true;
        }
    }

    return false;
}


void SHAPE_POLY_SET::splitCollinearOutlines()
{
    for( size_t polyIdx = 0; polyIdx < m_polys.size(); ++polyIdx )
    {
        bool changed = true;

        while( changed )
        {
            changed = false;

            SHAPE_LINE_CHAIN& outline = m_polys[polyIdx][0];
            intptr_t count = outline.PointCount();

            RTree<intptr_t, intptr_t, 2, intptr_t> rtree;

            for( intptr_t i = 0; i < count; ++i )
            {
                const VECTOR2I& a = outline.CPoint( i );
                const VECTOR2I& b = outline.CPoint( ( i + 1 ) % count );
                intptr_t min[2] = { std::min( a.x, b.x ), std::min( a.y, b.y ) };
                intptr_t max[2] = { std::max( a.x, b.x ), std::max( a.y, b.y ) };
                rtree.Insert( min, max, i );
            }

            bool found = false;
            int segA = -1;
            int segB = -1;

            for( intptr_t i = 0; i < count && !found; ++i )
            {
                const VECTOR2I& a = outline.CPoint( i );
                const VECTOR2I& b = outline.CPoint( ( i + 1 ) % count );
                SEG seg( a, b );
                intptr_t min[2] = { std::min( a.x, b.x ), std::min( a.y, b.y ) };
                intptr_t max[2] = { std::max( a.x, b.x ), std::max( a.y, b.y ) };

                auto visitor =
                        [&]( const int& j ) -> bool
                        {
                            if( j == i || j == ( ( i + 1 ) % count ) || j == ( ( i + count - 1 ) % count ) )
                                return true;

                            VECTOR2I oa = outline.CPoint( j );
                            VECTOR2I ob = outline.CPoint( ( j + 1 ) % count );
                            SEG other( oa, ob );

                            // Skip segments that share start/end points.  This is the case for
                            // fractured segments
                            if( oa == a && ob == b )
                                return true;

                            if( oa == b && ob == a )
                                return true;

                            if( seg.ApproxCollinear( other, 10 ) && isExteriorWaist( seg, other ) )
                            {
                                segA = i;
                                segB = j;
                                found = true;
                                return false;
                            }

                            return true;
                        };

                rtree.Search( min, max, visitor );
            }

            if( !found )
                break;

            int a0 = segA;
            int a1 = ( segA + 1 ) % outline.PointCount();
            int b0 = segB;
            int b1 = ( segB + 1 ) % outline.PointCount();

            SHAPE_LINE_CHAIN lc1;
            int idx = a1;
            lc1.Append( outline.CPoint( idx ) );

            while( idx != b0 )
            {
                idx = ( idx + 1 ) % outline.PointCount();
                lc1.Append( outline.CPoint( idx ) );
            }

            lc1.SetClosed( true );

            SHAPE_LINE_CHAIN lc2;
            idx = b1;
            lc2.Append( outline.CPoint( idx ) );

            while( idx != a0 )
            {
                idx = ( idx + 1 ) % outline.PointCount();
                lc2.Append( outline.CPoint( idx ) );
            }

            lc2.SetClosed( true );

            m_polys[polyIdx][0] = std::move( lc1 );

            POLYGON np;
            np.push_back( std::move( lc2 ) );
            m_polys.push_back( std::move( np ) );

            changed = true;
        }
    }
}


void SHAPE_POLY_SET::Simplify()
{
    splitCollinearOutlines();

    SHAPE_POLY_SET empty;

    booleanOp( Clipper2Lib::ClipType::Union, empty );
}


void SHAPE_POLY_SET::SimplifyOutlines( int aTolerance )
{
    for( POLYGON& paths : m_polys )
    {
        for( SHAPE_LINE_CHAIN& path : paths )
        {
            path.Simplify( aTolerance );
        }
    }
}


int SHAPE_POLY_SET::NormalizeAreaOutlines()
{
    // We are expecting only one main outline, but this main outline can have holes
    // if holes: combine holes and remove them from the main outline.
    // Note also we are usingin polygon
    // calculations, but it is not mandatory. It is used mainly
    // because there is usually only very few vertices in area outlines
    SHAPE_POLY_SET::POLYGON& outline = Polygon( 0 );
    SHAPE_POLY_SET holesBuffer;

    // Move holes stored in outline to holesBuffer:
    // The first SHAPE_LINE_CHAIN is the main outline, others are holes
    while( outline.size() > 1 )
    {
        holesBuffer.AddOutline( outline.back() );
        outline.pop_back();
    }

    Simplify();

    // If any hole, subtract it to main outline
    if( holesBuffer.OutlineCount() )
    {
        holesBuffer.Simplify();
        BooleanSubtract( holesBuffer );
    }

    // In degenerate cases, simplify might return no outlines
    if( OutlineCount() > 0 )
        RemoveNullSegments();

    return OutlineCount();
}


const std::string SHAPE_POLY_SET::Format( bool aCplusPlus ) const
{
    std::stringstream ss;

    ss << "SHAPE_LINE_CHAIN poly; \n";

    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        for( unsigned j = 0; j < m_polys[i].size(); j++ )
        {

            ss << "{ auto tmp = " << m_polys[i][j].Format() << ";\n";

            SHAPE_POLY_SET poly;

            if( j == 0 )
            {
                ss << " poly.AddOutline(tmp); } \n";
            }
            else
            {
                ss << " poly.AddHole(tmp); } \n";
            }

        }
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

            paths.push_back( std::move( outline ) );
        }

        m_polys.push_back( std::move( paths ) );
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


const BOX2I SHAPE_POLY_SET::BBoxFromCaches() const
{
    BOX2I bb;

    for( unsigned i = 0; i < m_polys.size(); i++ )
    {
        if( i == 0 )
            bb = *m_polys[i][0].GetCachedBBox();
        else
            bb.Merge( *m_polys[i][0].GetCachedBBox() );
    }

    return bb;
}


bool SHAPE_POLY_SET::PointOnEdge( const VECTOR2I& aP, int aAccuracy ) const
{
    // Iterate through all the polygons in the set
    for( const POLYGON& polygon : m_polys )
    {
        // Iterate through all the line chains in the polygon
        for( const SHAPE_LINE_CHAIN& lineChain : polygon )
        {
            if( lineChain.PointOnEdge( aP, aAccuracy ) )
                return true;
        }
    }

    return false;
}


bool SHAPE_POLY_SET::Collide( const SEG& aSeg, int aClearance, int* aActual,
                              VECTOR2I* aLocation ) const
{
    VECTOR2I nearest;
    ecoord dist_sq = SquaredDistanceToSeg( aSeg, aLocation ? &nearest : nullptr );

    if( dist_sq == 0 || dist_sq < SEG::Square( aClearance ) )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( dist_sq );

        return true;
    }

    return false;
}


bool SHAPE_POLY_SET::Collide( const VECTOR2I& aP, int aClearance, int* aActual,
                              VECTOR2I* aLocation ) const
{
    if( IsEmpty() || VertexCount() == 0 )
        return false;

    VECTOR2I nearest;
    ecoord dist_sq = SquaredDistance( aP, false, aLocation ? &nearest : nullptr );

    if( dist_sq == 0 || dist_sq < SEG::Square( aClearance ) )
    {
        if( aLocation )
            *aLocation = nearest;

        if( aActual )
            *aActual = sqrt( dist_sq );

        return true;
    }

    return false;
}


bool SHAPE_POLY_SET::Collide( const SHAPE* aShape, int aClearance, int* aActual,
                              VECTOR2I* aLocation ) const
{
    // A couple of simple cases are worth trying before we fall back on triangulation.

    if( aShape->Type() == SH_SEGMENT )
    {
        const SHAPE_SEGMENT* segment = static_cast<const SHAPE_SEGMENT*>( aShape );
        int                  extra = segment->GetWidth() / 2;

        if( Collide( segment->GetSeg(), aClearance + extra, aActual, aLocation ) )
        {
            if( aActual )
                *aActual = std::max( 0, *aActual - extra );

            return true;
        }

        return false;
    }

    if( aShape->Type() == SH_CIRCLE )
    {
        const SHAPE_CIRCLE* circle = static_cast<const SHAPE_CIRCLE*>( aShape );
        int                 extra = circle->GetRadius();

        if( Collide( circle->GetCenter(), aClearance + extra, aActual, aLocation ) )
        {
            if( aActual )
                *aActual = std::max( 0, *aActual - extra );

            return true;
        }

        return false;
    }

    const_cast<SHAPE_POLY_SET*>( this )->CacheTriangulation( false );

    int      actual = INT_MAX;
    VECTOR2I location;

    for( const std::unique_ptr<TRIANGULATED_POLYGON>& tpoly : m_triangulatedPolys )
    {
        for( const TRIANGULATED_POLYGON::TRI& tri : tpoly->Triangles() )
        {
            if( aActual || aLocation )
            {
                int      triActual;
                VECTOR2I triLocation;

                if( aShape->Collide( &tri, aClearance, &triActual, &triLocation ) )
                {
                    if( triActual < actual )
                    {
                        actual = triActual;
                        location = triLocation;
                    }
                }
            }
            else    // A much faster version of above
            {
                if( aShape->Collide( &tri, aClearance ) )
                    return true;
            }
        }
    }

    if( actual < INT_MAX )
    {
        if( aActual )
            *aActual = std::max( 0, actual );

        if( aLocation )
            *aLocation = location;

        return true;
    }

    return false;
}


void SHAPE_POLY_SET::RemoveAllContours()
{
    m_polys.clear();
    m_triangulatedPolys.clear();
    m_triangulationValid = false;
}


void SHAPE_POLY_SET::RemoveContour( int aContourIdx, int aPolygonIdx )
{
    // Default polygon is the last one
    if( aPolygonIdx < 0 )
        aPolygonIdx += m_polys.size();

    m_polys[aPolygonIdx].erase( m_polys[aPolygonIdx].begin() + aContourIdx );
}


void SHAPE_POLY_SET::RemoveOutline( int aOutlineIdx )
{
    m_polys.erase( m_polys.begin() + aOutlineIdx );
}


int SHAPE_POLY_SET::RemoveNullSegments()
{
    int removed = 0;

    ITERATOR iterator = IterateWithHoles();

    VECTOR2I    contourStart = *iterator;
    VECTOR2I    segmentStart, segmentEnd;

    VERTEX_INDEX indexStart;
    std::vector<VERTEX_INDEX> indices_to_remove;

    while( iterator )
    {
        // Obtain first point and its index
        segmentStart = *iterator;
        indexStart = iterator.GetIndex();

        // Obtain last point
        if( iterator.IsEndContour() )
        {
            segmentEnd = contourStart;

            // Advance
            iterator++;

            // If we have rolled into the next contour, remember its position
            // segmentStart and segmentEnd remain valid for comparison here
            if( iterator )
                contourStart = *iterator;
        }
        else
        {
            // Advance
            iterator++;

            // If we have reached the end of the SHAPE_POLY_SET, something is broken here
            wxCHECK_MSG( iterator, removed, wxT( "Invalid polygon.  Reached end without noticing.  Please report this error" ) );

            segmentEnd = *iterator;
        }

        // Remove segment start if both points are equal
        if( segmentStart == segmentEnd )
        {
            indices_to_remove.push_back( indexStart );
            removed++;
        }
    }

    // Proceed in reverse direction to remove the vertices because they are stored as absolute indices in a vector
    // Removing in reverse order preserves the remaining index values
    for( auto it = indices_to_remove.rbegin(); it != indices_to_remove.rend(); ++it )
        RemoveVertex( *it );

    return removed;
}


void SHAPE_POLY_SET::DeletePolygon( int aIdx )
{
    m_polys.erase( m_polys.begin() + aIdx );
}


void SHAPE_POLY_SET::DeletePolygonAndTriangulationData( int aIdx, bool aUpdateHash )
{
    m_polys.erase( m_polys.begin() + aIdx );

    if( m_triangulationValid )
    {
        for( int ii = m_triangulatedPolys.size() - 1; ii >= 0; --ii )
        {
            std::unique_ptr<TRIANGULATED_POLYGON>& triangleSet = m_triangulatedPolys[ii];

            if( triangleSet->GetSourceOutlineIndex() == aIdx )
                m_triangulatedPolys.erase( m_triangulatedPolys.begin() + ii );
            else if( triangleSet->GetSourceOutlineIndex() > aIdx )
                triangleSet->SetSourceOutlineIndex( triangleSet->GetSourceOutlineIndex() - 1 );
        }

        if( aUpdateHash )
        {
            m_hash = checksum();
            m_hashValid = true;
        }
    }
}


void SHAPE_POLY_SET::UpdateTriangulationDataHash()
{
    m_hash = checksum();
    m_hashValid = true;
}


void SHAPE_POLY_SET::Append( const SHAPE_POLY_SET& aSet )
{
    m_polys.insert( m_polys.end(), aSet.m_polys.begin(), aSet.m_polys.end() );
}


void SHAPE_POLY_SET::Append( const VECTOR2I& aP, int aOutline, int aHole )
{
    Append( aP.x, aP.y, aOutline, aHole );
}


bool SHAPE_POLY_SET::CollideVertex( const VECTOR2I& aPoint,
                                    SHAPE_POLY_SET::VERTEX_INDEX* aClosestVertex,
                                    int aClearance ) const
{
    // Shows whether there was a collision
    bool collision = false;

    // Difference vector between each vertex and aPoint.
    VECTOR2D    delta;
    ecoord      distance_squared;
    ecoord      clearance_squared = SEG::Square( aClearance );

    for( CONST_ITERATOR iterator = CIterateWithHoles(); iterator; iterator++ )
    {
        // Get the difference vector between current vertex and aPoint
        delta = *iterator - aPoint;

        // Compute distance
        distance_squared = delta.SquaredEuclideanNorm();

        // Check for collisions
        if( distance_squared <= clearance_squared )
        {
            if( !aClosestVertex )
                return true;

            collision = true;

            // Update clearance to look for closer vertices
            clearance_squared = distance_squared;

            // Store the indices that identify the vertex
            *aClosestVertex = iterator.GetIndex();
        }
    }

    return collision;
}


bool SHAPE_POLY_SET::CollideEdge( const VECTOR2I& aPoint,
                                  SHAPE_POLY_SET::VERTEX_INDEX* aClosestVertex,
                                  int aClearance ) const
{
    // Shows whether there was a collision
    bool   collision = false;
    ecoord clearance_squared = SEG::Square( aClearance );

    for( CONST_SEGMENT_ITERATOR iterator = CIterateSegmentsWithHoles(); iterator; iterator++ )
    {
        const SEG currentSegment = *iterator;
        ecoord    distance_squared = currentSegment.SquaredDistance( aPoint );

        // Check for collisions
        if( distance_squared <= clearance_squared )
        {
            if( !aClosestVertex )
                return true;

            collision = true;

            // Update clearance to look for closer edges
            clearance_squared = distance_squared;

            // Store the indices that identify the vertex
            *aClosestVertex = iterator.GetIndex();
        }
    }

    return collision;
}


void SHAPE_POLY_SET::BuildBBoxCaches() const
{
    for( int polygonIdx = 0; polygonIdx < OutlineCount(); polygonIdx++ )
    {
        COutline( polygonIdx ).GenerateBBoxCache();

        for( int holeIdx = 0; holeIdx < HoleCount( polygonIdx ); holeIdx++ )
            CHole( polygonIdx, holeIdx ).GenerateBBoxCache();
    }
}


bool SHAPE_POLY_SET::Contains( const VECTOR2I& aP, int aSubpolyIndex, int aAccuracy,
                               bool aUseBBoxCaches ) const
{
    if( m_polys.empty() )
        return false;

    // If there is a polygon specified, check the condition against that polygon
    if( aSubpolyIndex >= 0 )
        return containsSingle( aP, aSubpolyIndex, aAccuracy, aUseBBoxCaches );

    // In any other case, check it against all polygons in the set
    for( int polygonIdx = 0; polygonIdx < OutlineCount(); polygonIdx++ )
    {
        if( containsSingle( aP, polygonIdx, aAccuracy, aUseBBoxCaches ) )
            return true;
    }

    return false;
}


void SHAPE_POLY_SET::RemoveVertex( int aGlobalIndex )
{
    VERTEX_INDEX index;

    // Assure the to be removed vertex exists, abort otherwise
    if( GetRelativeIndices( aGlobalIndex, &index ) )
        RemoveVertex( index );
    else
        throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );
}


void SHAPE_POLY_SET::RemoveVertex( VERTEX_INDEX aIndex )
{
    m_polys[aIndex.m_polygon][aIndex.m_contour].Remove( aIndex.m_vertex );
}


void SHAPE_POLY_SET::SetVertex( int aGlobalIndex, const VECTOR2I& aPos )
{
    VERTEX_INDEX index;

    if( GetRelativeIndices( aGlobalIndex, &index ) )
        SetVertex( index, aPos );
    else
        throw( std::out_of_range( "aGlobalIndex-th vertex does not exist" ) );
}


void SHAPE_POLY_SET::SetVertex( const VERTEX_INDEX& aIndex, const VECTOR2I& aPos )
{
    m_polys[aIndex.m_polygon][aIndex.m_contour].SetPoint( aIndex.m_vertex, aPos );
}


bool SHAPE_POLY_SET::containsSingle( const VECTOR2I& aP, int aSubpolyIndex, int aAccuracy,
                                     bool aUseBBoxCaches ) const
{
    // Check that the point is inside the outline
    if( m_polys[aSubpolyIndex][0].PointInside( aP, aAccuracy ) )
    {
        // Check that the point is not in any of the holes
        for( int holeIdx = 0; holeIdx < HoleCount( aSubpolyIndex ); holeIdx++ )
        {
            const SHAPE_LINE_CHAIN& hole = CHole( aSubpolyIndex, holeIdx );

            // If the point is inside a hole it is outside of the polygon.  Do not use aAccuracy
            // here as it's meaning would be inverted.
            if( hole.PointInside( aP, 1, aUseBBoxCaches ) )
                return false;
        }

        return true;
    }

    return false;
}


void SHAPE_POLY_SET::Move( const VECTOR2I& aVector )
{
    for( POLYGON& poly : m_polys )
    {
        for( SHAPE_LINE_CHAIN& path : poly )
            path.Move( aVector );
    }

    for( std::unique_ptr<TRIANGULATED_POLYGON>& tri : m_triangulatedPolys )
        tri->Move( aVector );

    m_hash = checksum();
    m_hashValid = true;
}


void SHAPE_POLY_SET::Mirror( const VECTOR2I& aRef, FLIP_DIRECTION aFlipDirection )
{
    for( POLYGON& poly : m_polys )
    {
        for( SHAPE_LINE_CHAIN& path : poly )
            path.Mirror( aRef, aFlipDirection );
    }

    if( m_triangulationValid )
        CacheTriangulation();
}


void SHAPE_POLY_SET::Rotate( const EDA_ANGLE& aAngle, const VECTOR2I& aCenter )
{
    for( POLYGON& poly : m_polys )
    {
        for( SHAPE_LINE_CHAIN& path : poly )
            path.Rotate( aAngle, aCenter );
    }

    // Don't re-cache if the triangulation is already invalid
    if( m_triangulationValid )
        CacheTriangulation();
}


int SHAPE_POLY_SET::TotalVertices() const
{
    int c = 0;

    for( const POLYGON& poly : m_polys )
    {
        for( const SHAPE_LINE_CHAIN& path : poly )
            c += path.PointCount();
    }

    return c;
}


SHAPE_POLY_SET::POLYGON SHAPE_POLY_SET::ChamferPolygon( unsigned int aDistance, int aIndex )
{
    return chamferFilletPolygon( CHAMFERED, aDistance, aIndex, 0 );
}


SHAPE_POLY_SET::POLYGON SHAPE_POLY_SET::FilletPolygon( unsigned int aRadius, int aErrorMax,
                                                       int aIndex )
{
    return chamferFilletPolygon( FILLETED, aRadius, aIndex, aErrorMax );
}


SEG::ecoord SHAPE_POLY_SET::SquaredDistanceToPolygon( VECTOR2I aPoint, int aPolygonIndex,
                                                      VECTOR2I* aNearest ) const
{
    // We calculate the min dist between the segment and each outline segment.  However, if the
    // segment to test is inside the outline, and does not cross any edge, it can be seen outside
    // the polygon.  Therefore test if a segment end is inside (testing only one end is enough).
    // Use an accuracy of "1" to say that we don't care if it's exactly on the edge or not.
    if( containsSingle( aPoint, aPolygonIndex, 1 ) )
    {
        if( aNearest )
            *aNearest = aPoint;

        return 0;
    }

    CONST_SEGMENT_ITERATOR iterator = CIterateSegmentsWithHoles( aPolygonIndex );

    SEG::ecoord minDistance = (*iterator).SquaredDistance( aPoint );

    for( iterator++; iterator && minDistance > 0; iterator++ )
    {
        SEG::ecoord currentDistance = (*iterator).SquaredDistance( aPoint );

        if( currentDistance < minDistance )
        {
            if( aNearest )
                *aNearest = (*iterator).NearestPoint( aPoint );

            minDistance = currentDistance;
        }
    }

    return minDistance;
}


SEG::ecoord SHAPE_POLY_SET::SquaredDistanceToPolygon( const SEG& aSegment, int aPolygonIndex,
                                                      VECTOR2I* aNearest ) const
{
    // Check if the segment is fully-contained.  If so, its midpoint is a good-enough nearest point.
    if( containsSingle( aSegment.A, aPolygonIndex, 1 ) &&
        containsSingle( aSegment.B, aPolygonIndex, 1 ) )
    {
        if( aNearest )
            *aNearest = ( aSegment.A + aSegment.B ) / 2;

        return 0;
    }

    CONST_SEGMENT_ITERATOR iterator = CIterateSegmentsWithHoles( aPolygonIndex );
    SEG::ecoord            minDistance = (*iterator).SquaredDistance( aSegment );

    if( aNearest && minDistance == 0 )
        *aNearest = ( *iterator ).NearestPoint( aSegment );

    for( iterator++; iterator && minDistance > 0; iterator++ )
    {
        SEG::ecoord currentDistance = (*iterator).SquaredDistance( aSegment );

        if( currentDistance < minDistance )
        {
            if( aNearest )
                *aNearest = (*iterator).NearestPoint( aSegment );

            minDistance = currentDistance;
        }
    }

    // Return the maximum of minDistance and zero
    return minDistance < 0 ? 0 : minDistance;
}


SEG::ecoord SHAPE_POLY_SET::SquaredDistance( const VECTOR2I& aPoint, bool aOutlineOnly,
                                             VECTOR2I* aNearest ) const
{
    wxASSERT_MSG( !aOutlineOnly, wxT( "Warning: SHAPE_POLY_SET::SquaredDistance does not yet "
                                      "support aOutlineOnly==true" ) );

    SEG::ecoord currentDistance_sq;
    SEG::ecoord minDistance_sq = VECTOR2I::ECOORD_MAX;
    VECTOR2I    nearest;

    // Iterate through all the polygons and get the minimum distance.
    for( unsigned int polygonIdx = 0; polygonIdx < m_polys.size(); polygonIdx++ )
    {
        currentDistance_sq = SquaredDistanceToPolygon( aPoint, polygonIdx,
                                                       aNearest ? &nearest : nullptr );

        if( currentDistance_sq < minDistance_sq )
        {
            if( aNearest )
                *aNearest = nearest;

            minDistance_sq = currentDistance_sq;
        }
    }

    return minDistance_sq;
}


SEG::ecoord SHAPE_POLY_SET::SquaredDistanceToSeg( const SEG& aSegment, VECTOR2I* aNearest ) const
{
    SEG::ecoord currentDistance_sq;
    SEG::ecoord minDistance_sq = VECTOR2I::ECOORD_MAX;
    VECTOR2I    nearest;

    // Iterate through all the polygons and get the minimum distance.
    for( unsigned int polygonIdx = 0; polygonIdx < m_polys.size(); polygonIdx++ )
    {
        currentDistance_sq = SquaredDistanceToPolygon( aSegment, polygonIdx,
                                                       aNearest ? &nearest : nullptr );

        if( currentDistance_sq < minDistance_sq )
        {
            if( aNearest )
                *aNearest = nearest;

            minDistance_sq = currentDistance_sq;
        }
    }

    return minDistance_sq;
}


bool SHAPE_POLY_SET::IsVertexInHole( int aGlobalIdx )
{
    VERTEX_INDEX index;

    // Get the polygon and contour where the vertex is. If the vertex does not exist, return false
    if( !GetRelativeIndices( aGlobalIdx, &index ) )
        return false;

    // The contour is a hole if its index is greater than zero
    return index.m_contour > 0;
}


SHAPE_POLY_SET SHAPE_POLY_SET::Chamfer( int aDistance )
{
    SHAPE_POLY_SET chamfered;

    for( unsigned int idx = 0; idx < m_polys.size(); idx++ )
        chamfered.m_polys.push_back( ChamferPolygon( aDistance, idx ) );

    return chamfered;
}


SHAPE_POLY_SET SHAPE_POLY_SET::Fillet( int aRadius, int aErrorMax )
{
    SHAPE_POLY_SET filleted;

    for( size_t idx = 0; idx < m_polys.size(); idx++ )
        filleted.m_polys.push_back( FilletPolygon( aRadius, aErrorMax, idx ) );

    return filleted;
}


SHAPE_POLY_SET &SHAPE_POLY_SET::operator=( const SHAPE_POLY_SET& aOther )
{
    SHAPE::operator=( aOther );
    m_polys = aOther.m_polys;

    m_triangulatedPolys.clear();

    if( aOther.IsTriangulationUpToDate() )
    {
        m_triangulatedPolys.reserve( aOther.TriangulatedPolyCount() );

        for( unsigned i = 0; i < aOther.TriangulatedPolyCount(); i++ )
        {
            const TRIANGULATED_POLYGON* poly = aOther.TriangulatedPolygon( i );
            m_triangulatedPolys.push_back( std::make_unique<TRIANGULATED_POLYGON>( *poly ) );
        }

        m_hash = aOther.m_hash;
        m_hashValid = aOther.m_hashValid;
        m_triangulationValid = aOther.m_triangulationValid.load();
    }
    else
    {
        m_hash.Clear();
        m_hashValid = false;
        m_triangulationValid = false;
    }

    return *this;
}


HASH_128 SHAPE_POLY_SET::GetHash() const
{
    if( !m_hashValid )
        return checksum();

    return m_hash;
}


bool SHAPE_POLY_SET::IsTriangulationUpToDate() const
{
    if( !m_triangulationValid )
        return false;

    if( !m_hashValid )
        return false;

    HASH_128 hash = checksum();

    return hash == m_hash;
}


static SHAPE_POLY_SET partitionPolyIntoRegularCellGrid( const SHAPE_POLY_SET& aPoly, int aSize )
{
    BOX2I bb = aPoly.BBox();

    double w = bb.GetWidth();
    double h = bb.GetHeight();

    if( w == 0.0 || h == 0.0 )
        return aPoly;

    int n_cells_x, n_cells_y;

    if( w > h )
    {
        n_cells_x = w / aSize;
        n_cells_y = floor( h / w * n_cells_x ) + 1;
    }
    else
    {
        n_cells_y = h / aSize;
        n_cells_x = floor( w / h * n_cells_y ) + 1;
    }

    SHAPE_POLY_SET ps1( aPoly ), ps2( aPoly ), maskSetOdd, maskSetEven;

    for( int yy = 0; yy < n_cells_y; yy++ )
    {
        for( int xx = 0; xx < n_cells_x; xx++ )
        {
            VECTOR2I p;

            p.x = bb.GetX() + w * xx / n_cells_x;
            p.y = bb.GetY() + h * yy / n_cells_y;

            VECTOR2I p2;

            p2.x = bb.GetX() + w * ( xx + 1 ) / n_cells_x;
            p2.y = bb.GetY() + h * ( yy + 1 ) / n_cells_y;


            SHAPE_LINE_CHAIN mask;
            mask.Append( VECTOR2I( p.x, p.y ) );
            mask.Append( VECTOR2I( p2.x, p.y ) );
            mask.Append( VECTOR2I( p2.x, p2.y ) );
            mask.Append( VECTOR2I( p.x, p2.y ) );
            mask.SetClosed( true );

            if( ( xx ^ yy ) & 1 )
                maskSetOdd.AddOutline( mask );
            else
                maskSetEven.AddOutline( mask );
        }
    }

    ps1.BooleanIntersection( maskSetOdd );
    ps2.BooleanIntersection( maskSetEven );
    ps1.Fracture();
    ps2.Fracture();

    for( int i = 0; i < ps2.OutlineCount(); i++ )
        ps1.AddOutline( ps2.COutline( i ) );

    if( ps1.OutlineCount() )
        return ps1;
    else
        return aPoly;
}


void SHAPE_POLY_SET::cacheTriangulation( bool aPartition, bool aSimplify,
                                         std::vector<std::unique_ptr<TRIANGULATED_POLYGON>>* aHintData )
{
    std::unique_lock<std::mutex> lock( m_triangulationMutex );

    if( m_triangulationValid && m_hashValid )
    {
        if( m_hash == checksum() )
            return;
    }

    // Invalidate, in case anything goes wrong below
    m_triangulationValid = false;
    m_hashValid = false;

    auto triangulate =
            []( SHAPE_POLY_SET& polySet, int forOutline,
                std::vector<std::unique_ptr<TRIANGULATED_POLYGON>>& dest,
                std::vector<std::unique_ptr<TRIANGULATED_POLYGON>>* hintData )
            {
                bool triangulationValid = false;
                int pass = 0;
                int index = 0;

                if( hintData && hintData->size() != (unsigned) polySet.OutlineCount() )
                    hintData = nullptr;

                while( polySet.OutlineCount() > 0 )
                {
                    if( !dest.empty() && dest.back()->GetTriangleCount() == 0 )
                        dest.erase( dest.end() - 1 );

                    dest.push_back( std::make_unique<TRIANGULATED_POLYGON>( forOutline ) );
                    POLYGON_TRIANGULATION tess( *dest.back() );

                    // If the tessellation fails, we re-fracture the polygon, which will
                    // first simplify the system before fracturing and removing the holes
                    // This may result in multiple, disjoint polygons.
                    if( !tess.TesselatePolygon( polySet.Polygon( 0 ).front(),
                                                hintData ? hintData->at( index ).get() : nullptr ) )
                    {
                        ++pass;

                        if( pass == 1 )
                        {
                            polySet.SimplifyOutlines( TRIANGULATESIMPLIFICATIONLEVEL );
                        }
                        // In Clipper2, there is only one type of simplification
                        else
                        {
                            break;
                        }

                        triangulationValid = false;
                        hintData = nullptr;
                        continue;
                    }

                    polySet.DeletePolygon( 0 );
                    index++;
                    triangulationValid = true;
                }

                return triangulationValid;
            };

    m_triangulatedPolys.clear();

    if( aPartition )
    {
        for( int ii = 0; ii < OutlineCount(); ++ii )
        {
            // This partitions into regularly-sized grids (1cm in Pcbnew)
            SHAPE_POLY_SET flattened( Outline( ii ) );

            for( int jj = 0; jj < HoleCount( ii ); ++jj )
                flattened.AddHole( Hole( ii, jj ) );

            flattened.ClearArcs();

            if( flattened.HasHoles() || flattened.IsSelfIntersecting() )
                flattened.Fracture();
            else if( aSimplify )
                flattened.Simplify();

            SHAPE_POLY_SET partitions = partitionPolyIntoRegularCellGrid( flattened, 1e7 );

            // This pushes the triangulation for all polys in partitions
            // to be referenced to the ii-th polygon
            if( !triangulate( partitions, ii , m_triangulatedPolys, aHintData ) )
            {
                wxLogTrace( TRIANGULATE_TRACE, "Failed to triangulate partitioned polygon %d", ii );
            }
            else
            {
                m_hash = checksum();
                m_hashValid = true;
                // Set valid flag only after everything has been updated
                m_triangulationValid = true;
            }
        }
    }
    else
    {
        SHAPE_POLY_SET tmpSet( *this );

        tmpSet.ClearArcs();
        tmpSet.Fracture();

        if( !triangulate( tmpSet, -1, m_triangulatedPolys, aHintData ) )
        {
            wxLogTrace( TRIANGULATE_TRACE, "Failed to triangulate polygon" );
        }
        else
        {
            m_hash = checksum();
            m_hashValid = true;
            // Set valid flag only after everything has been updated
            m_triangulationValid = true;
        }
    }
}


HASH_128 SHAPE_POLY_SET::checksum() const
{
    MMH3_HASH hash( 0x68AF835D ); // Arbitrary seed

    hash.add( m_polys.size() );

    for( const POLYGON& outline : m_polys )
    {
        hash.add( outline.size() );

        for( const SHAPE_LINE_CHAIN& lc : outline )
        {
            hash.add( lc.PointCount() );

            for( int i = 0; i < lc.PointCount(); i++ )
            {
                VECTOR2I pt = lc.CPoint( i );

                hash.add( pt.x );
                hash.add( pt.y );
            }
        }
    }

    return hash.digest();
}


bool SHAPE_POLY_SET::HasTouchingHoles() const
{
    for( int i = 0; i < OutlineCount(); i++ )
    {
        if( hasTouchingHoles( CPolygon( i ) ) )
            return true;
    }

    return false;
}


bool SHAPE_POLY_SET::hasTouchingHoles( const POLYGON& aPoly ) const
{
    std::set<long long> ptHashes;

    for( const SHAPE_LINE_CHAIN& lc : aPoly )
    {
        for( const VECTOR2I& pt : lc.CPoints() )
        {
            const long long ptHash = (long long) pt.x << 32 | pt.y;

            if( ptHashes.count( ptHash ) > 0 )
                return true;

            ptHashes.insert( ptHash );
        }
    }

    return false;
}


bool SHAPE_POLY_SET::HasIndexableSubshapes() const
{
    return IsTriangulationUpToDate();
}


size_t SHAPE_POLY_SET::GetIndexableSubshapeCount() const
{
    size_t n = 0;

    for( const std::unique_ptr<TRIANGULATED_POLYGON>& t : m_triangulatedPolys )
        n += t->GetTriangleCount();

    return n;
}


void SHAPE_POLY_SET::GetIndexableSubshapes( std::vector<const SHAPE*>& aSubshapes ) const
{
    aSubshapes.reserve( GetIndexableSubshapeCount() );

    for( const std::unique_ptr<TRIANGULATED_POLYGON>& tpoly : m_triangulatedPolys )
    {
        for( const TRIANGULATED_POLYGON::TRI& tri : tpoly->Triangles() )
            aSubshapes.push_back( &tri );
    }
}


const BOX2I SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI::BBox( int aClearance ) const
{
    BOX2I bbox( parent->m_vertices[a] );
    bbox.Merge( parent->m_vertices[b] );
    bbox.Merge( parent->m_vertices[c] );

    if( aClearance != 0 )
        bbox.Inflate( aClearance );

    return bbox;
}


void SHAPE_POLY_SET::TRIANGULATED_POLYGON::AddTriangle( int a, int b, int c )
{
    m_triangles.emplace_back( a, b, c, this );
}


SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRIANGULATED_POLYGON( const TRIANGULATED_POLYGON& aOther )
{
    m_sourceOutline = aOther.m_sourceOutline;
    m_vertices = aOther.m_vertices;
    m_triangles = aOther.m_triangles;

    for( TRI& tri : m_triangles )
        tri.parent = this;
}


SHAPE_POLY_SET::TRIANGULATED_POLYGON& SHAPE_POLY_SET::TRIANGULATED_POLYGON::operator=( const TRIANGULATED_POLYGON& aOther )
{
    m_sourceOutline = aOther.m_sourceOutline;
    m_vertices = aOther.m_vertices;
    m_triangles = aOther.m_triangles;

    for( TRI& tri : m_triangles )
        tri.parent = this;

    return *this;
}


SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRIANGULATED_POLYGON( int aSourceOutline ) :
        m_sourceOutline( aSourceOutline )
{
}


SHAPE_POLY_SET::TRIANGULATED_POLYGON::~TRIANGULATED_POLYGON()
{
}


void SHAPE_POLY_SET::Scale( double aScaleFactorX, double aScaleFactorY, const VECTOR2I& aCenter )
{
    for( POLYGON& poly : m_polys )
    {
        for( SHAPE_LINE_CHAIN& path : poly )
        {
            for( int i = 0; i < path.PointCount(); i++ )
            {
                VECTOR2I pt = path.CPoint( i );
                VECTOR2D vec;
                vec.x = ( pt.x - aCenter.x ) * aScaleFactorX;
                vec.y = ( pt.y - aCenter.y ) * aScaleFactorY;
                pt.x = KiROUND<double, int>( aCenter.x + vec.x );
                pt.y = KiROUND<double, int>( aCenter.y + vec.y );
                path.SetPoint( i, pt );
            }
        }
    }

    if( m_triangulationValid )
        CacheTriangulation();
}


void
SHAPE_POLY_SET::BuildPolysetFromOrientedPaths( const std::vector<SHAPE_LINE_CHAIN>& aPaths,
                                               bool                                 aEvenOdd )
{
    Clipper2Lib::Clipper64 clipper;
    Clipper2Lib::PolyTree64 tree;
    Clipper2Lib::Paths64 paths;

    for( const SHAPE_LINE_CHAIN& path : aPaths )
    {
        Clipper2Lib::Path64 lc;
        lc.reserve( path.PointCount() );

        for( int i = 0; i < path.PointCount(); i++ )
            lc.emplace_back( path.CPoint( i ).x, path.CPoint( i ).y );

        paths.push_back( std::move( lc ) );
    }

    clipper.AddSubject( paths );
    clipper.Execute( Clipper2Lib::ClipType::Union, aEvenOdd ? Clipper2Lib::FillRule::EvenOdd
                                                            : Clipper2Lib::FillRule::NonZero, tree );

    std::vector<CLIPPER_Z_VALUE> zValues;
    std::vector<SHAPE_ARC> arcBuffer;

    importTree( tree, zValues, arcBuffer );
    tree.Clear(); // Free used memory (not done in dtor)
}


bool SHAPE_POLY_SET::PointInside( const VECTOR2I& aPt, int aAccuracy, bool aUseBBoxCache ) const
{
    for( int idx = 0; idx < OutlineCount(); idx++ )
    {
        if( COutline( idx ).PointInside( aPt, aAccuracy, aUseBBoxCache ) )
            return true;
    }

    return false;
}


const std::vector<SEG> SHAPE_POLY_SET::GenerateHatchLines( const std::vector<double>& aSlopes,
                                                           int aSpacing, int aLineLength ) const
{
    std::vector<SEG> hatchLines;

    // define range for hatch lines
    int min_x = CVertex( 0 ).x;
    int max_x = CVertex( 0 ).x;
    int min_y = CVertex( 0 ).y;
    int max_y = CVertex( 0 ).y;

    for( auto iterator = CIterateWithHoles(); iterator; iterator++ )
    {
        if( iterator->x < min_x )
            min_x = iterator->x;

        if( iterator->x > max_x )
            max_x = iterator->x;

        if( iterator->y < min_y )
            min_y = iterator->y;

        if( iterator->y > max_y )
            max_y = iterator->y;
    }

    auto sortEndsByDescendingX =
            []( const VECTOR2I& ref, const VECTOR2I& tst )
            {
                return tst.x < ref.x;
            };

    for( double slope : aSlopes )
    {
        int64_t max_a, min_a;

        if( slope > 0 )
        {
            max_a = KiROUND<double, int64_t>( max_y - slope * min_x );
            min_a = KiROUND<double, int64_t>( min_y - slope * max_x );
        }
        else
        {
            max_a = KiROUND<double, int64_t>( max_y - slope * max_x );
            min_a = KiROUND<double, int64_t>( min_y - slope * min_x );
        }

        min_a = ( min_a / aSpacing ) * aSpacing;

        // loop through hatch lines
        std::vector<VECTOR2I> pointbuffer;
        pointbuffer.reserve( 256 );

        for( int64_t a = min_a; a < max_a; a += aSpacing )
        {
            pointbuffer.clear();

            // Iterate through all vertices
            for( auto iterator = CIterateSegmentsWithHoles(); iterator; iterator++ )
            {
                const SEG seg = *iterator;
                VECTOR2I  pt;

                if( seg.IntersectsLine( slope, a, pt ) )
                {
                    // If the intersection point is outside the polygon, skip it
                    if( pt.x < min_x || pt.x > max_x || pt.y < min_y || pt.y > max_y )
                        continue;

                    // Add the intersection point to the buffer
                    pointbuffer.emplace_back( KiROUND( pt.x ), KiROUND( pt.y ) );
                }
            }

            // sort points in order of descending x (if more than 2) to
            // ensure the starting point and the ending point of the same segment
            // are stored one just after the other.
            if( pointbuffer.size() > 2 )
                sort( pointbuffer.begin(), pointbuffer.end(), sortEndsByDescendingX );

            // creates lines or short segments inside the complex polygon
            for( size_t ip = 0; ip + 1 < pointbuffer.size(); ip++ )
            {
                const VECTOR2I& p1 = pointbuffer[ip];
                const VECTOR2I& p2 = pointbuffer[ip + 1];

                // Avoid duplicated intersections or segments
                if( p1 == p2 )
                    continue;

                SEG candidate( p1, p2 );

                VECTOR2I mid( ( candidate.A.x + candidate.B.x ) / 2, ( candidate.A.y + candidate.B.y ) / 2 );

                // Check if segment is inside the polygon by checking its middle point
                if( containsSingle( mid, 0, 1, true ) )
                {
                    int dx = p2.x - p1.x;

                    // Push only one line for diagonal hatch or for small lines < twice
                    // the line length; else push 2 small lines
                    if( aLineLength == -1 || std::abs( dx ) < 2 * aLineLength )
                    {
                        hatchLines.emplace_back( candidate );
                    }
                    else
                    {
                        double dy = p2.y - p1.y;
                        slope = dy / dx;

                        if( dx > 0 )
                            dx = aLineLength;
                        else
                            dx = -aLineLength;

                        int x1 = KiROUND( p1.x + dx );
                        int x2 = KiROUND( p2.x - dx );
                        int y1 = KiROUND( p1.y + dx * slope );
                        int y2 = KiROUND( p2.y - dx * slope );

                        hatchLines.emplace_back( SEG( p1.x, p1.y, x1, y1 ) );

                        hatchLines.emplace_back( SEG( p2.x, p2.y, x2, y2 ) );
                    }
                }
            }
        }
    }

    return hatchLines;
}
