/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file polygon_2d.cpp
 */

#include "polygon_2d.h"
#include "../ray.h"
#include <wx/debug.h>


static bool polygon_IsPointInside( const SEGMENTS& aSegments, const SFVEC2F& aPoint )
{
    wxASSERT( aSegments.size() >= 3 );

    unsigned int i;
    unsigned int j        = aSegments.size() - 1;
    bool         oddNodes = false;

    for( i = 0; i < aSegments.size(); j = i++ )
    {
        const float polyJY = aSegments[j].m_Start.y;
        const float polyIY = aSegments[i].m_Start.y;

        if( ( ( polyIY <= aPoint.y ) && ( polyJY >= aPoint.y ) )
          || ( ( polyJY <= aPoint.y ) && ( polyIY >= aPoint.y ) ) )
        {
            const float polyJX = aSegments[j].m_Start.x;
            const float polyIX = aSegments[i].m_Start.x;

            if( ( polyIX <= aPoint.x ) || ( polyJX <= aPoint.x ) )
                oddNodes ^= ( ( polyIX + ( ( aPoint.y - polyIY ) * aSegments[i].m_inv_JY_minus_IY )
                              * aSegments[i].m_JX_minus_IX )
                            < aPoint.x );
        }
    }

    return oddNodes;
}


POLYGON_2D::POLYGON_2D( const SEGMENTS_WIDTH_NORMALS& aOpenSegmentList,
                        const OUTERS_AND_HOLES& aOuterAndHoles, const BOARD_ITEM& aBoardItem )
        : OBJECT_2D( OBJECT_2D_TYPE::POLYGON, aBoardItem )
{
    m_open_segments.resize( aOpenSegmentList.size() );

    // Copy vectors and structures
    for( unsigned int i = 0; i < aOpenSegmentList.size(); i++ )
        m_open_segments[i] = aOpenSegmentList[i];

    m_outers_and_holes = aOuterAndHoles;

    // Compute bounding box with the points of the polygon
    m_bbox.Reset();

    for( unsigned int i = 0; i < m_outers_and_holes.m_Outers.size(); i++ )
    {
        for( unsigned int j = 0; j < m_outers_and_holes.m_Outers[i].size(); j++ )
            m_bbox.Union( ( (SEGMENTS) m_outers_and_holes.m_Outers[i] )[j].m_Start );
    }

    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();

    // Some checks
    wxASSERT( m_open_segments.size() == aOpenSegmentList.size() );
    wxASSERT( m_open_segments.size() > 0 );

    wxASSERT( m_outers_and_holes.m_Outers.size() > 0 );
    wxASSERT( m_outers_and_holes.m_Outers.size() == aOuterAndHoles.m_Outers.size() );
    wxASSERT( m_outers_and_holes.m_Holes.size() == aOuterAndHoles.m_Holes.size() );

    wxASSERT( m_outers_and_holes.m_Outers[0].size() >= 3 );
    wxASSERT( m_outers_and_holes.m_Outers[0].size() == aOuterAndHoles.m_Outers[0].size() );

    wxASSERT( m_bbox.IsInitialized() );
}


bool POLYGON_2D::Intersects( const BBOX_2D& aBBox ) const
{
    return m_bbox.Intersects( aBBox );

    // !TODO: this is a quick not perfect implementation
    // in order to make it perfect the box must be checked against all the
    // polygons in the outers and not inside the holes
}


bool POLYGON_2D::Overlaps( const BBOX_2D& aBBox ) const
{
    // NOT IMPLEMENTED, why?
    return false;
}


bool POLYGON_2D::Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const
{
    int   hitIndex = -1;
    float hitU     = 0.0f;
    float tMin     = 0.0f;

    for( unsigned int i = 0; i < m_open_segments.size(); i++ )
    {
        const SFVEC2F& s = m_open_segments[i].m_Precalc_slope;
        const SFVEC2F& q = m_open_segments[i].m_Start;

        float rxs = aSegRay.m_End_minus_start.x * s.y - aSegRay.m_End_minus_start.y * s.x;

        if( fabs( rxs ) > FLT_EPSILON )
        {
            const float inv_rxs = 1.0f / rxs;

            const SFVEC2F pq = q - aSegRay.m_Start;

            const float t = ( pq.x * s.y - pq.y * s.x ) * inv_rxs;

            if( ( t < 0.0f ) || ( t > 1.0f ) )
                continue;

            const float u =
                    ( pq.x * aSegRay.m_End_minus_start.y - pq.y * aSegRay.m_End_minus_start.x )
                    * inv_rxs;

            if( ( u < 0.0f ) || ( u > 1.0f ) )
                continue;

            if( ( hitIndex == -1 ) || ( t <= tMin ) )
            {
                tMin     = t;
                hitIndex = i;
                hitU     = u;
            }
        }
    }

    if( hitIndex >= 0 )
    {
        wxASSERT( ( tMin >= 0.0f ) && ( tMin <= 1.0f ) );

        if( aOutT )
            *aOutT = tMin;

        if( aNormalOut )
            *aNormalOut = glm::normalize( m_open_segments[hitIndex].m_Normals.m_Start * hitU +
                                          m_open_segments[hitIndex].m_Normals.m_End *
                                          ( 1.0f - hitU ) );

        return true;
    }

    return false;
}


INTERSECTION_RESULT POLYGON_2D::IsBBoxInside( const BBOX_2D& aBBox ) const
{

    return INTERSECTION_RESULT::MISSES;
}


bool POLYGON_2D::IsPointInside( const SFVEC2F& aPoint ) const
{
    // NOTE: we could add here a test for the bounding box, but because in the
    // 3d object it already checked for a 3d bbox.

    // First test if point is inside a hole.
    // If true it can early exit
    for( unsigned int i = 0; i < m_outers_and_holes.m_Holes.size(); i++ )
        if( !m_outers_and_holes.m_Holes[i].empty() )
            if( polygon_IsPointInside( m_outers_and_holes.m_Holes[i], aPoint ) )
                return false;

    // At this moment, the point is not inside a hole, so check if it is
    // inside the polygon
    for( unsigned int i = 0; i < m_outers_and_holes.m_Outers.size(); i++ )
        if( !m_outers_and_holes.m_Outers[i].empty() )
            if( polygon_IsPointInside( m_outers_and_holes.m_Outers[i], aPoint ) )
                return true;

    // Miss the polygon
    return false;
}


DUMMY_BLOCK_2D::DUMMY_BLOCK_2D( const SFVEC2F& aPbMin, const SFVEC2F& aPbMax,
                                const BOARD_ITEM& aBoardItem )
        : OBJECT_2D( OBJECT_2D_TYPE::DUMMYBLOCK, aBoardItem )
{
    m_bbox.Set( aPbMin, aPbMax );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();
}


DUMMY_BLOCK_2D::DUMMY_BLOCK_2D( const BBOX_2D& aBBox, const BOARD_ITEM& aBoardItem )
        : OBJECT_2D( OBJECT_2D_TYPE::DUMMYBLOCK, aBoardItem )
{
    m_bbox.Set( aBBox );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();
}


bool DUMMY_BLOCK_2D::Intersects( const BBOX_2D& aBBox ) const
{
    return m_bbox.Intersects( aBBox );
}


bool DUMMY_BLOCK_2D::Overlaps( const BBOX_2D& aBBox ) const
{
    // Not implemented
    return false;
}


bool DUMMY_BLOCK_2D::Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const
{
    // The dummy block will be never intersected because it have no edges,
    // only it have a plan surface of the size of the bounding box
    return false;
}


INTERSECTION_RESULT DUMMY_BLOCK_2D::IsBBoxInside( const BBOX_2D& aBBox ) const
{
    //!TODO:
    return INTERSECTION_RESULT::MISSES;
}


bool DUMMY_BLOCK_2D::IsPointInside( const SFVEC2F& aPoint ) const
{
    // The dummy is filled in all his bounding box, so if it hit the bbox
    // it will hit this dummy
    if( m_bbox.Inside( aPoint ) )
        return true;

    return false;
}


typedef std::vector<SFVEC2F> KF_POINTS;

#define MAX_NR_DIVISIONS 96


static bool intersect( const SEGMENT_WITH_NORMALS& aSeg, const SFVEC2F& aStart,
                       const SFVEC2F& aEnd )
{
    const SFVEC2F r = aEnd - aStart;
    const SFVEC2F s = aSeg.m_Precalc_slope;
    const SFVEC2F q = aSeg.m_Start;

    const float rxs = r.x * s.y - r.y * s.x;

    if( fabs( rxs ) > glm::epsilon<float>() )
    {
        const float inv_rxs = 1.0f / rxs;

        const SFVEC2F pq = q - aStart;

        const float t = ( pq.x * s.y - pq.y * s.x ) * inv_rxs;

        if( ( t < 0.0f ) || ( t > 1.0f ) )
            return false;

        const float u = ( pq.x * r.y - pq.y * r.x ) * inv_rxs;

        if( ( u < 0.0f ) || ( u > 1.0f ) )
            return false;

        return true;
    }

    return false;
}


static void extractPathsFrom( const SEGMENTS_WIDTH_NORMALS& aSegList, const BBOX_2D& aBBox,
                              SEGMENTS_WIDTH_NORMALS& aOutSegThatIntersect )
{
    wxASSERT( aSegList.size() >= 3 );

    unsigned int i;
    unsigned int j = aSegList.size() - 1;

    const SFVEC2F p1( aBBox.Min().x, aBBox.Min().y );
    const SFVEC2F p2( aBBox.Max().x, aBBox.Min().y );
    const SFVEC2F p3( aBBox.Max().x, aBBox.Max().y );
    const SFVEC2F p4( aBBox.Min().x, aBBox.Max().y );

    aOutSegThatIntersect.clear();

    for( i = 0; i < aSegList.size(); j = i++ )
    {
        if( aBBox.Inside( aSegList[i].m_Start ) || aBBox.Inside( aSegList[j].m_Start ) )
        {
            // if the segment points are inside the bounding box then this
            // segment is touching the bbox.
            aOutSegThatIntersect.push_back( aSegList[i] );
        }
        else
        {
            // Check if a segment intersects the bounding box

            // Make a bounding box based on the segments start and end
            BBOX_2D segmentBBox( aSegList[i].m_Start, aSegList[j].m_Start );

            if( aBBox.Intersects( segmentBBox ) )
            {

                const SEGMENT_WITH_NORMALS& seg = aSegList[i];

                if( intersect( seg, p1, p2 ) || intersect( seg, p2, p3 ) || intersect( seg, p3, p4 )
                  || intersect( seg, p4, p1 ) )
                {
                    aOutSegThatIntersect.push_back( seg );
                }
            }
        }
    }
}


static void polygon_Convert( const SHAPE_LINE_CHAIN& aPath, SEGMENTS& aOutSegment,
                             float aBiuTo3dUnitsScale )
{
    aOutSegment.resize( aPath.PointCount() );

    for( int j = 0; j < aPath.PointCount(); j++ )
    {
        const VECTOR2I& a = aPath.CPoint( j );

        aOutSegment[j].m_Start = SFVEC2F( (float) a.x * aBiuTo3dUnitsScale,
                                          (float) -a.y * aBiuTo3dUnitsScale );
    }

    unsigned int i;
    unsigned int j = aOutSegment.size() - 1;

    for( i = 0; i < aOutSegment.size(); j = i++ )
    {
        // Calculate constants for each segment
        aOutSegment[i].m_inv_JY_minus_IY = 1.0f /
                ( aOutSegment[j].m_Start.y - aOutSegment[i].m_Start.y );

        aOutSegment[i].m_JX_minus_IX = ( aOutSegment[j].m_Start.x - aOutSegment[i].m_Start.x );
    }
}


void ConvertPolygonToBlocks( const SHAPE_POLY_SET& aMainPath, CONTAINER_2D_BASE& aDstContainer,
                             float aBiuTo3dUnitsScale, float aDivFactor,
                             const BOARD_ITEM& aBoardItem, int aPolyIndex )
{
    // Get the path
    wxASSERT( aPolyIndex < aMainPath.OutlineCount() );

    const SHAPE_LINE_CHAIN& path = aMainPath.COutline( aPolyIndex );

    BOX2I pathBounds = path.BBox();

    // Convert the points to segments class
    BBOX_2D bbox;
    bbox.Reset();

    // Contains the main list of segments and each segment normal interpolated
    SEGMENTS_WIDTH_NORMALS segments_and_normals;

    // Contains a closed polygon used to calc if points are inside
    SEGMENTS segments;

    segments_and_normals.reserve( path.PointCount() );
    segments.reserve( path.PointCount() );

    SFVEC2F prevPoint;

    for( int i = 0; i < path.PointCount(); i++ )
    {
        const VECTOR2I& a = path.CPoint( i );

        const SFVEC2F point( (float) ( a.x ) * aBiuTo3dUnitsScale,
                             (float) ( -a.y ) * aBiuTo3dUnitsScale );

        // Only add points that are not coincident
        if( ( i == 0 ) || ( fabs( prevPoint.x - point.x ) > FLT_EPSILON )
          || ( fabs( prevPoint.y - point.y ) > FLT_EPSILON ) )
        {
            prevPoint = point;

            bbox.Union( point );

            SEGMENT_WITH_NORMALS sn;
            sn.m_Start = point;
            segments_and_normals.push_back( sn );

            POLYSEGMENT ps;
            ps.m_Start = point;
            segments.push_back( ps );
        }
    }

    bbox.ScaleNextUp();

    // Calc the slopes, normals and some statistics about this polygon
    unsigned int i;
    unsigned int j = segments_and_normals.size() - 1;

    // Temporary normal to the segment, it will later be used for interpolation
    std::vector<SFVEC2F> tmpSegmentNormals;
    tmpSegmentNormals.resize( segments_and_normals.size() );

    float medOfTheSquaresSegmentLength = 0.0f;

#ifdef PRINT_STATISTICS_3D_VIEWER
    float minLength = FLT_MAX;
#endif

    for( i = 0; i < segments_and_normals.size(); j = i++ )
    {
        const SFVEC2F slope = segments_and_normals[j].m_Start - segments_and_normals[i].m_Start;

        segments_and_normals[i].m_Precalc_slope = slope;

        // Calculate constants for each segment
        segments[i].m_inv_JY_minus_IY =
                1.0f / ( segments_and_normals[j].m_Start.y - segments_and_normals[i].m_Start.y );

        segments[i].m_JX_minus_IX =
                ( segments_and_normals[j].m_Start.x - segments_and_normals[i].m_Start.x );

        // The normal orientation expect a fixed polygon orientation (!TODO: which one?)
        //tmpSegmentNormals[i] = glm::normalize( SFVEC2F( -slope.y, +slope.x ) );
        tmpSegmentNormals[i] = glm::normalize( SFVEC2F( slope.y, -slope.x ) );

        const float length = slope.x * slope.x + slope.y * slope.y;

#ifdef PRINT_STATISTICS_3D_VIEWER
        if( length < minLength )
            minLength = length;
#endif

        medOfTheSquaresSegmentLength += length;
    }

#ifdef PRINT_STATISTICS_3D_VIEWER
    float minSegmentLength = sqrt( minLength );
#endif

    // This calc an approximation of medium lengths, that will be used to calc
    // the size of the division.
    medOfTheSquaresSegmentLength /= segments_and_normals.size();
    medOfTheSquaresSegmentLength = sqrt( medOfTheSquaresSegmentLength );

    // Compute the normal interpolation
    // If calculate the dot between the segments, if they are above/below some
    // threshold it will not interpolated it (ex: if you are in a edge corner
    // or in a smooth transaction)
    j = segments_and_normals.size() - 1;

    for( i = 0; i < segments_and_normals.size(); j = i++ )
    {
        const SFVEC2F normalBeforeSeg = tmpSegmentNormals[j];
        const SFVEC2F normalSeg       = tmpSegmentNormals[i];
        const SFVEC2F normalAfterSeg  = tmpSegmentNormals[( i + 1 ) % segments_and_normals.size()];

        const float dotBefore = glm::dot( normalBeforeSeg, normalSeg );
        const float dotAfter  = glm::dot( normalAfterSeg, normalSeg );

        if( dotBefore < 0.7f )
            segments_and_normals[i].m_Normals.m_Start = normalSeg;
        else
            segments_and_normals[i].m_Normals.m_Start =
                    glm::normalize( ( normalBeforeSeg * dotBefore ) + normalSeg );

        if( dotAfter < 0.7f )
            segments_and_normals[i].m_Normals.m_End = normalSeg;
        else
            segments_and_normals[i].m_Normals.m_End =
                    glm::normalize( ( normalAfterSeg * dotAfter ) + normalSeg );
    }

    SFVEC2UI grid_divisions;

    if( aDivFactor < 0.0f )
    {
        grid_divisions = SFVEC2UI( 1 );
    }
    else
    {
        if( aDivFactor <= FLT_EPSILON )
            aDivFactor = medOfTheSquaresSegmentLength;

        grid_divisions.x = (unsigned int) ( ( bbox.GetExtent().x / aDivFactor ) );
        grid_divisions.y = (unsigned int) ( ( bbox.GetExtent().y / aDivFactor ) );

        grid_divisions = glm::clamp(
                grid_divisions, SFVEC2UI( 1, 1 ), SFVEC2UI( MAX_NR_DIVISIONS, MAX_NR_DIVISIONS ) );
    }

    // Calculate the steps advance of the grid
    SFVEC2F blockAdvance;

    blockAdvance.x = bbox.GetExtent().x / (float) grid_divisions.x;
    blockAdvance.y = bbox.GetExtent().y / (float) grid_divisions.y;

    wxASSERT( blockAdvance.x > 0.0f );
    wxASSERT( blockAdvance.y > 0.0f );

    const int leftToRight_inc = ( pathBounds.GetRight() - pathBounds.GetLeft() ) / grid_divisions.x;

    const int topToBottom_inc = ( pathBounds.GetBottom() - pathBounds.GetTop() ) / grid_divisions.y;

    // Statistics
    unsigned int stats_n_empty_blocks       = 0;
    unsigned int stats_n_dummy_blocks       = 0;
    unsigned int stats_n_poly_blocks        = 0;
    unsigned int stats_sum_size_of_polygons = 0;

    // Step by each block of a grid trying to extract segments and create polygon blocks
    int   topToBottom = pathBounds.GetTop();
    float blockY      = bbox.Max().y;

    for( unsigned int iy = 0; iy < grid_divisions.y; iy++ )
    {
        int   leftToRight = pathBounds.GetLeft();
        float blockX      = bbox.Min().x;

        for( unsigned int ix = 0; ix < grid_divisions.x; ix++ )
        {
            BBOX_2D blockBox( SFVEC2F( blockX, blockY - blockAdvance.y ),
                              SFVEC2F( blockX + blockAdvance.x, blockY ) );

            // Make the box large to it will catch (intersect) the edges
            blockBox.ScaleNextUp();
            blockBox.ScaleNextUp();
            blockBox.ScaleNextUp();

            SEGMENTS_WIDTH_NORMALS extractedSegments;

            extractPathsFrom( segments_and_normals, blockBox, extractedSegments );

            if( extractedSegments.empty() )
            {

                SFVEC2F p1( blockBox.Min().x, blockBox.Min().y );
                SFVEC2F p2( blockBox.Max().x, blockBox.Min().y );
                SFVEC2F p3( blockBox.Max().x, blockBox.Max().y );
                SFVEC2F p4( blockBox.Min().x, blockBox.Max().y );

                if( polygon_IsPointInside( segments, p1 ) || polygon_IsPointInside( segments, p2 )
                  || polygon_IsPointInside( segments, p3 )
                  || polygon_IsPointInside( segments, p4 ) )
                {
                    // In this case, the segments are not intersecting the
                    // polygon, so it means that if any point is inside it,
                    // then all other are inside the polygon.
                    // This is a full bbox inside, so add a dummy box
                    aDstContainer.Add( new DUMMY_BLOCK_2D( blockBox, aBoardItem ) );
                    stats_n_dummy_blocks++;
                }
                else
                {
                    // Points are outside, so this block completely missed the polygon
                    // In this case, no objects need to be added
                    stats_n_empty_blocks++;
                }
            }
            else
            {
                // At this point, the borders of polygon were intersected by the
                // bounding box, so we must calculate a new polygon that will
                // close that small block.
                // This block will be used to calculate if points are inside
                // the (sub block) polygon.

                SHAPE_POLY_SET subBlockPoly;

                SHAPE_LINE_CHAIN sb = SHAPE_LINE_CHAIN( { VECTOR2I( leftToRight, topToBottom ),
                        VECTOR2I( leftToRight + leftToRight_inc, topToBottom ),
                        VECTOR2I( leftToRight + leftToRight_inc, topToBottom + topToBottom_inc ),
                        VECTOR2I( leftToRight, topToBottom + topToBottom_inc ) } );

                //sb.Append( leftToRight, topToBottom );
                sb.SetClosed( true );

                subBlockPoly.AddOutline( sb );

                // We need here a simple polygon with outlines and holes
                SHAPE_POLY_SET solution;
                solution.BooleanIntersection( aMainPath, subBlockPoly );

                OUTERS_AND_HOLES outersAndHoles;

                outersAndHoles.m_Holes.clear();
                outersAndHoles.m_Outers.clear();

                for( int idx = 0; idx < solution.OutlineCount(); idx++ )
                {
                    const SHAPE_LINE_CHAIN& outline = solution.Outline( idx );

                    SEGMENTS solutionSegment;

                    polygon_Convert( outline, solutionSegment, aBiuTo3dUnitsScale );
                    outersAndHoles.m_Outers.push_back( solutionSegment );

                    stats_sum_size_of_polygons += solutionSegment.size();

                    for( int holeIdx = 0; holeIdx < solution.HoleCount( idx ); holeIdx++ )
                    {
                        const SHAPE_LINE_CHAIN& hole = solution.Hole( idx, holeIdx );

                        polygon_Convert( hole, solutionSegment, aBiuTo3dUnitsScale );
                        outersAndHoles.m_Holes.push_back( solutionSegment );
                        stats_sum_size_of_polygons += solutionSegment.size();
                    }
                }

                if( !outersAndHoles.m_Outers.empty() )
                {
                    aDstContainer.Add( new POLYGON_2D( extractedSegments, outersAndHoles,
                                                       aBoardItem ) );
                    stats_n_poly_blocks++;
                }
            }

            blockX += blockAdvance.x;
            leftToRight += leftToRight_inc;
        }

        blockY -= blockAdvance.y;
        topToBottom += topToBottom_inc;
    }
}
