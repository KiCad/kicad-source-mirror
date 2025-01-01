/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <algorithm>                    // for max, min
#include <bitset>                       // for bitset::count
#include <math.h>                       // for atan2

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_line_chain.h>  // for SHAPE_LINE_CHAIN
#include <geometry/shape_poly_set.h>    // for SHAPE_POLY_SET, SHAPE_POLY_SE...
#include <math/util.h>
#include <math/vector2d.h>              // for VECTOR2I
#include <trigo.h>


void TransformCircleToPolygon( SHAPE_LINE_CHAIN& aBuffer, const VECTOR2I& aCenter, int aRadius,
                               int aError, ERROR_LOC aErrorLoc, int aMinSegCount )
{
    VECTOR2I corner_position;
    int     numSegs = GetArcToSegmentCount( aRadius, aError, FULL_CIRCLE );
    numSegs = std::max( aMinSegCount, numSegs );

    // Round up to 8 to make segment approximations align properly at 45-degrees
    numSegs = ( numSegs + 7 ) / 8 * 8;

    EDA_ANGLE delta = ANGLE_360 / numSegs;
    int       radius = aRadius;

    if( aErrorLoc == ERROR_OUTSIDE )
    {
        // The outer radius should be radius+aError
        // Recalculate the actual approx error, as it can be smaller than aError
        // because numSegs is clamped to a minimal value
        int actual_delta_radius = CircleToEndSegmentDeltaRadius( radius, numSegs );
        radius += GetCircleToPolyCorrection( actual_delta_radius );
    }

    for( EDA_ANGLE angle = delta / 2; angle < ANGLE_360; angle += delta )
    {
        corner_position.x = radius;
        corner_position.y = 0;
        RotatePoint( corner_position, angle );
        corner_position += aCenter;
        aBuffer.Append( corner_position.x, corner_position.y );
    }

    aBuffer.SetClosed( true );
}


void TransformCircleToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aCenter, int aRadius,
                               int aError, ERROR_LOC aErrorLoc, int aMinSegCount )
{
    VECTOR2I corner_position;
    int      numSegs = GetArcToSegmentCount( aRadius, aError, FULL_CIRCLE );
    numSegs = std::max( aMinSegCount, numSegs );

    // Round up to 8 to make segment approximations align properly at 45-degrees
    numSegs = ( numSegs + 7 ) / 8 * 8;

    EDA_ANGLE delta = ANGLE_360 / numSegs;
    int       radius = aRadius;

    if( aErrorLoc == ERROR_OUTSIDE )
    {
        // The outer radius should be radius+aError
        // Recalculate the actual approx error, as it can be smaller than aError
        // because numSegs is clamped to a minimal value
        int actual_delta_radius = CircleToEndSegmentDeltaRadius( radius, numSegs );
        radius += GetCircleToPolyCorrection( actual_delta_radius );
    }

    aBuffer.NewOutline();

    for( EDA_ANGLE angle = delta / 2; angle < ANGLE_360; angle += delta )
    {
        corner_position.x = radius;
        corner_position.y = 0;
        RotatePoint( corner_position, angle );
        corner_position += aCenter;
        aBuffer.Append( corner_position.x, corner_position.y );
    }

    // Finish circle
    corner_position.x = radius;
    corner_position.y = 0;
    RotatePoint( corner_position, delta / 2 );
    corner_position += aCenter;
    aBuffer.Append( corner_position.x, corner_position.y );
}


void TransformOvalToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aStart, const VECTOR2I& aEnd,
                             int aWidth, int aError, ERROR_LOC aErrorLoc, int aMinSegCount )
{
    // To build the polygonal shape outside the actual shape, we use a bigger
    // radius to build rounded ends.
    // However, the width of the segment is too big.
    // so, later, we will clamp the polygonal shape with the bounding box
    // of the segment.
    int radius  = aWidth / 2;
    int numSegs = GetArcToSegmentCount( radius, aError, FULL_CIRCLE );
    numSegs = std::max( aMinSegCount, numSegs );

    // Round up to 8 to make segment approximations align properly at 45-degrees
    numSegs = ( numSegs + 7 ) / 8 * 8;

    EDA_ANGLE delta = ANGLE_360 / numSegs;

    if( aErrorLoc == ERROR_OUTSIDE )
    {
        // The outer radius should be radius+aError
        // Recalculate the actual approx error, as it can be smaller than aError
        // because numSegs is clamped to a minimal value
        int actual_delta_radius = CircleToEndSegmentDeltaRadius( radius, numSegs );
        int correction = GetCircleToPolyCorrection( actual_delta_radius );
        radius += correction;
    }

    // end point is the coordinate relative to aStart
    VECTOR2I       endp = aEnd - aStart;
    VECTOR2I       startp = aStart;
    VECTOR2I       corner;
    SHAPE_POLY_SET polyshape;

    polyshape.NewOutline();

    // normalize the position in order to have endp.x >= 0
    // it makes calculations more easy to understand
    if( endp.x < 0 )
    {
        endp    = aStart - aEnd;
        startp  = aEnd;
    }

    EDA_ANGLE delta_angle( endp );
    int       seg_len = endp.EuclideanNorm();

    // Compute the outlines of the segment, and creates a polygon
    // Note: the polygonal shape is built from the equivalent horizontal
    // segment starting at {0,0}, and ending at {seg_len,0}

    // add right rounded end:

    // Right arc start:
    corner = VECTOR2I( seg_len, radius );
    polyshape.Append( corner.x, corner.y );

    for( EDA_ANGLE angle = delta / 2; angle < ANGLE_180; angle += delta )
    {
        corner = VECTOR2I( 0, radius );
        RotatePoint( corner, angle );
        corner.x += seg_len;
        polyshape.Append( corner.x, corner.y );
    }

    // Finish right arc:
    corner = VECTOR2I( seg_len, -radius );
    polyshape.Append( corner.x, corner.y );

    // Left arc start:
    corner = VECTOR2I( 0, -radius );
    polyshape.Append( corner.x, corner.y );

    // add left rounded end:
    for( EDA_ANGLE angle = delta / 2; angle < ANGLE_180; angle += delta )
    {
        corner = VECTOR2I( 0, -radius );
        RotatePoint( corner, angle );
        polyshape.Append( corner.x, corner.y );
    }

    // Finish left arc:
    corner = VECTOR2I( 0, radius );
    polyshape.Append( corner.x, corner.y );

    // Now trim the edges of the polygonal shape which will be slightly outside the
    // track width.
    SHAPE_POLY_SET bbox;
    bbox.NewOutline();
    // Build the bbox (a horizontal rectangle).
    int halfwidth = aWidth / 2;     // Use the exact segment width for the bbox height
    corner.x = -radius - 2;         // use a bbox width slightly bigger to avoid
                                    // creating useless corner at segment ends
    corner.y = halfwidth;
    bbox.Append( corner.x, corner.y );
    corner.y = -halfwidth;
    bbox.Append( corner.x, corner.y );
    corner.x = radius + seg_len + 2;
    bbox.Append( corner.x, corner.y );
    corner.y = halfwidth;
    bbox.Append( corner.x, corner.y );

    // Now, clamp the shape
    polyshape.BooleanIntersection( bbox );
    // Note the final polygon is a simple, convex polygon with no hole
    // due to the shape of initial polygons

    // Rotate and move the polygon to its right location
    polyshape.Rotate( -delta_angle );
    polyshape.Move( startp );

    aBuffer.Append( polyshape);
}


struct ROUNDED_CORNER
{
    VECTOR2I m_position;
    int      m_radius;
    ROUNDED_CORNER( int x, int y ) : m_position( VECTOR2I( x, y ) ), m_radius( 0 ) {}
    ROUNDED_CORNER( int x, int y, int radius ) : m_position( VECTOR2I( x, y ) ), m_radius( radius ) {}
};


// Corner List requirements: no concave shape, corners in clockwise order, no duplicate corners
void CornerListToPolygon( SHAPE_POLY_SET& outline, std::vector<ROUNDED_CORNER>& aCorners,
                          int aInflate, int aError, ERROR_LOC aErrorLoc )
{
    assert( aInflate >= 0 );
    outline.NewOutline();
    VECTOR2I incoming = aCorners[0].m_position - aCorners.back().m_position;

    for( int n = 0, count = aCorners.size(); n < count; n++ )
    {
        ROUNDED_CORNER& cur = aCorners[n];
        ROUNDED_CORNER& next = aCorners[( n + 1 ) % count];
        VECTOR2I        outgoing = next.m_position - cur.m_position;

        if( !( aInflate || cur.m_radius ) )
        {
            outline.Append( cur.m_position );
        }
        else
        {
            VECTOR2I  cornerPosition = cur.m_position;
            int       radius = cur.m_radius;
            EDA_ANGLE endAngle;
            double    tanAngle2;

            if( ( incoming.x == 0 && outgoing.y == 0 ) || ( incoming.y == 0 && outgoing.x == 0 ) )
            {
                endAngle = ANGLE_90;
                tanAngle2 = 1.0;
            }
            else
            {
                double cosNum = (double) incoming.x * outgoing.x + (double) incoming.y * outgoing.y;
                double cosDen = (double) incoming.EuclideanNorm() * outgoing.EuclideanNorm();
                double angle = acos( cosNum / cosDen );
                tanAngle2 = tan( ( M_PI - angle ) / 2 );
                endAngle = EDA_ANGLE( angle, RADIANS_T );
            }

            if( aInflate && tanAngle2 )
            {
                radius += aInflate;
                cornerPosition += incoming.Resize( aInflate / tanAngle2 )
                                + incoming.Perpendicular().Resize( -aInflate );
            }

            // Ensure 16+ segments per 360deg and ensure first & last segment are the same size
            int       numSegs = std::max( 16, GetArcToSegmentCount( radius, aError, FULL_CIRCLE ) );
            EDA_ANGLE angDelta = ANGLE_360 / numSegs;
            EDA_ANGLE lastSeg = endAngle;

            if( lastSeg > ANGLE_0 )
            {
                while( lastSeg > angDelta )
                    lastSeg -= angDelta;
            }
            else
            {
                while( lastSeg < -angDelta )
                    lastSeg += angDelta;
            }

            EDA_ANGLE angPos = lastSeg.IsZero() ? angDelta : ( angDelta + lastSeg ) / 2;

            double   arcTransitionDistance = ( tanAngle2 > 0 ) ? ( radius / tanAngle2 ) : 0;
            VECTOR2I arcStart = cornerPosition - incoming.Resize( arcTransitionDistance );
            VECTOR2I arcCenter = arcStart + incoming.Perpendicular().Resize( radius );
            VECTOR2I arcEnd, arcStartOrigin;

            if( aErrorLoc == ERROR_INSIDE )
            {
                arcEnd = SEG( cornerPosition, arcCenter ).ReflectPoint( arcStart );
                arcStartOrigin = arcStart - arcCenter;
                outline.Append( arcStart );
            }
            else
            {
                // The outer radius should be radius+aError, recalculate because numSegs is clamped
                int actualDeltaRadius = CircleToEndSegmentDeltaRadius( radius, numSegs );
                int radiusExtend = GetCircleToPolyCorrection( actualDeltaRadius );
                arcStart += incoming.Perpendicular().Resize( -radiusExtend );
                arcStartOrigin = arcStart - arcCenter;

                // To avoid "ears", we only add segments crossing/within the non-rounded outline
                // Note: outlineIn is short and must be treated as defining an infinite line
                SEG      outlineIn( cornerPosition - incoming, cornerPosition );
                VECTOR2I prevPt = arcStart;
                arcEnd = cornerPosition; // default if no points within the outline are found

                while( angPos < endAngle )
                {
                    VECTOR2I pt = arcStartOrigin;
                    RotatePoint( pt, -angPos );
                    pt += arcCenter;
                    angPos += angDelta;

                    if( outlineIn.Side( pt ) > 0 )
                    {
                        OPT_VECTOR2I intersect = outlineIn.IntersectLines( SEG( prevPt, pt ) );

                        wxCHECK_RET( intersect, wxT( "No solutions exist!" ) );
                        outline.Append( *intersect );
                        outline.Append( pt );
                        arcEnd = SEG( cornerPosition, arcCenter ).ReflectPoint( *intersect );
                        break;
                    }

                    endAngle -= angDelta; // if skipping first, also skip last
                    prevPt = pt;
                }
            }

            for( ; angPos < endAngle; angPos += angDelta )
            {
                VECTOR2I pt = arcStartOrigin;
                RotatePoint( pt, -angPos );
                outline.Append( pt + arcCenter );
            }

            outline.Append( arcEnd );
        }

        incoming = outgoing;
    }
}


void CornerListRemoveDuplicates( std::vector<ROUNDED_CORNER>& aCorners )
{
    VECTOR2I prev = aCorners[0].m_position;

    for( int pos = aCorners.size() - 1; pos >= 0; pos-- )
    {
        if( aCorners[pos].m_position == prev )
            aCorners.erase( aCorners.begin() + pos );
        else
            prev = aCorners[pos].m_position;
    }
}


void TransformTrapezoidToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aPosition,
                                  const VECTOR2I& aSize, const EDA_ANGLE& aRotation, int aDeltaX,
                                  int aDeltaY, int aInflate, int aError, ERROR_LOC aErrorLoc )
{
    SHAPE_POLY_SET              outline;
    VECTOR2I                    size( aSize / 2 );
    std::vector<ROUNDED_CORNER> corners;

    if( aInflate < 0 )
    {
        if( !aDeltaX && !aDeltaY ) // rectangle
        {
            size.x = std::max( 1, size.x + aInflate );
            size.y = std::max( 1, size.y + aInflate );
        }
        else if( aDeltaX ) // horizontal trapezoid
        {
            double slope = (double) aDeltaX / size.x;
            int    yShrink = KiROUND( ( std::hypot( size.x, aDeltaX ) * aInflate ) / size.x );
            size.y = std::max( 1, size.y + yShrink );
            size.x = std::max( 1, size.x + aInflate );
            aDeltaX = KiROUND( size.x * slope );

            if( aDeltaX > size.y ) // shrinking turned the trapezoid into a triangle
            {
                corners.reserve( 3 );
                corners.emplace_back( -size.x, -size.y - aDeltaX );
                corners.emplace_back( KiROUND( size.y / slope ), 0 );
                corners.emplace_back( -size.x, size.y + aDeltaX );
            }
        }
        else // vertical trapezoid
        {
            double slope = (double) aDeltaY / size.y;
            int    xShrink = KiROUND( ( std::hypot( size.y, aDeltaY ) * aInflate ) / size.y );
            size.x = std::max( 1, size.x + xShrink );
            size.y = std::max( 1, size.y + aInflate );
            aDeltaY = KiROUND( size.y * slope );

            if( aDeltaY > size.x )
            {
                corners.reserve( 3 );
                corners.emplace_back( 0, -KiROUND( size.x / slope ) );
                corners.emplace_back( size.x + aDeltaY, size.y );
                corners.emplace_back( -size.x - aDeltaY, size.y );
            }
        }

        aInflate = 0;
    }

    if( corners.empty() )
    {
        corners.reserve( 4 );
        corners.emplace_back( -size.x + aDeltaY, -size.y - aDeltaX );
        corners.emplace_back( size.x - aDeltaY, -size.y + aDeltaX );
        corners.emplace_back( size.x + aDeltaY, size.y - aDeltaX );
        corners.emplace_back( -size.x - aDeltaY, size.y + aDeltaX );

        if( std::abs( aDeltaY ) == std::abs( size.x ) || std::abs( aDeltaX ) == std::abs( size.y ) )
            CornerListRemoveDuplicates( corners );
    }

    CornerListToPolygon( outline, corners, aInflate, aError, aErrorLoc );

    if( !aRotation.IsZero() )
        outline.Rotate( aRotation );

    outline.Move( VECTOR2I( aPosition ) );
    aBuffer.Append( outline );
}


void TransformRoundChamferedRectToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aPosition,
                                           const VECTOR2I& aSize, const EDA_ANGLE& aRotation,
                                           int aCornerRadius, double aChamferRatio,
                                           int aChamferCorners, int aInflate, int aError,
                                           ERROR_LOC aErrorLoc )
{
    SHAPE_POLY_SET outline;
    VECTOR2I       size( aSize / 2 );
    int            chamferCnt = std::bitset<8>( aChamferCorners ).count();
    double         chamferDeduct = 0;

    if( aInflate < 0 )
    {
        size.x = std::max( 1, size.x + aInflate );
        size.y = std::max( 1, size.y + aInflate );
        chamferDeduct = aInflate * ( 2.0 - M_SQRT2 );
        aCornerRadius = std::max( 0, aCornerRadius + aInflate );
        aInflate = 0;
    }

    std::vector<ROUNDED_CORNER> corners;
    corners.reserve( 4 + chamferCnt );
    corners.emplace_back( -size.x, -size.y, aCornerRadius );
    corners.emplace_back( size.x, -size.y, aCornerRadius );
    corners.emplace_back( size.x, size.y, aCornerRadius );
    corners.emplace_back( -size.x, size.y, aCornerRadius );

    if( aChamferCorners )
    {
        int shorterSide = std::min( aSize.x, aSize.y );
        int chamfer = std::max( 0, KiROUND( aChamferRatio * shorterSide + chamferDeduct ) );
        int chamId[4] = { RECT_CHAMFER_TOP_LEFT, RECT_CHAMFER_TOP_RIGHT,
                          RECT_CHAMFER_BOTTOM_RIGHT, RECT_CHAMFER_BOTTOM_LEFT };
        int sign[8] = { 0, 1, -1, 0, 0, -1, 1, 0 };

        for( int cc = 0, pos = 0; cc < 4; cc++, pos++ )
        {
            if( !( aChamferCorners & chamId[cc] ) )
                continue;

            corners[pos].m_radius = 0;

            if( chamfer == 0 )
                continue;

            corners.insert( corners.begin() + pos + 1, corners[pos] );
            corners[pos].m_position.x += sign[( 2 * cc ) & 7] * chamfer;
            corners[pos].m_position.y += sign[( 2 * cc - 2 ) & 7] * chamfer;
            corners[pos + 1].m_position.x += sign[( 2 * cc + 1 ) & 7] * chamfer;
            corners[pos + 1].m_position.y += sign[( 2 * cc - 1 ) & 7] * chamfer;
            pos++;
        }

        if( chamferCnt > 1 && 2 * chamfer >= shorterSide )
            CornerListRemoveDuplicates( corners );
    }

    CornerListToPolygon( outline, corners, aInflate, aError, aErrorLoc );

    if( !aRotation.IsZero() )
        outline.Rotate( aRotation );

    outline.Move( aPosition );
    aBuffer.Append( outline );
}


int ConvertArcToPolyline( SHAPE_LINE_CHAIN& aPolyline, VECTOR2I aCenter, int aRadius,
                          const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aArcAngle,
                          double aAccuracy, ERROR_LOC aErrorLoc )
{
    int n = 2;

    if( aRadius >= aAccuracy )
        n = GetArcToSegmentCount( aRadius, aAccuracy, aArcAngle ) + 1;

    EDA_ANGLE delta = aArcAngle / n;

    if( aErrorLoc == ERROR_INSIDE )
    {
        // This is the easy case: with the error on the inside the endpoints of each segment
        // are error-free.

        EDA_ANGLE rot = aStartAngle;

        for( int i = 0; i <= n; i++, rot += delta )
        {
            double x = aCenter.x + aRadius * rot.Cos();
            double y = aCenter.y + aRadius * rot.Sin();

            aPolyline.Append( KiROUND( x ), KiROUND( y ) );
        }
    }
    else
    {
        // This is the hard case: with the error on the outside it's the segment midpoints
        // that are error-free.  So we need to add a half-segment at each end of the arc to get
        // them correct.

        int seg360 = std::abs( KiROUND( n * 360.0 / aArcAngle.AsDegrees() ) );
        int actual_delta_radius = CircleToEndSegmentDeltaRadius( aRadius, seg360 );
        int errorRadius = aRadius + actual_delta_radius;

        double x = aCenter.x + aRadius * aStartAngle.Cos();
        double y = aCenter.y + aRadius * aStartAngle.Sin();

        aPolyline.Append( KiROUND( x ), KiROUND( y ) );

        EDA_ANGLE rot = aStartAngle + delta / 2;

        for( int i = 0; i < n; i++, rot += delta )
        {
            x = aCenter.x + errorRadius * rot.Cos();
            y = aCenter.y + errorRadius * rot.Sin();

            aPolyline.Append( KiROUND( x ), KiROUND( y ) );
        }

        x = aCenter.x + aRadius * ( aStartAngle + aArcAngle ).Cos();
        y = aCenter.y + aRadius * ( aStartAngle + aArcAngle ).Sin();

        aPolyline.Append( KiROUND( x ), KiROUND( y ) );
    }

    return n;
}


void TransformArcToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aStart, const VECTOR2I& aMid,
                            const VECTOR2I& aEnd, int aWidth, int aError, ERROR_LOC aErrorLoc )
{
    SEG startToEnd( aStart, aEnd );
    int distanceToMid = startToEnd.Distance( aMid );

    if( distanceToMid <= 1 )
    {
        // Not an arc but essentially a straight line with a small error
        TransformOvalToPolygon( aBuffer, aStart, aEnd, aWidth + distanceToMid, aError, aErrorLoc );
        return;
    }

    // This appproximation builds a single polygon by starting with a 180 degree arc at one
    // end, then the outer edge of the arc, then a 180 degree arc at the other end, and finally
    // the inner edge of the arc.
    SHAPE_ARC arc( aStart, aMid, aEnd, aWidth );
    EDA_ANGLE arc_angle_start = arc.GetStartAngle();
    EDA_ANGLE arc_angle = arc.GetCentralAngle();
    EDA_ANGLE arc_angle_end = arc_angle_start + arc_angle;

    if( arc_angle < ANGLE_0 )
    {
        std::swap( arc_angle_start, arc_angle_end );
        arc = SHAPE_ARC( aEnd, aMid, aStart, aWidth );
        arc_angle = -arc_angle;
    }

    int       radial_offset = arc.GetWidth() / 2;
    int       arc_outer_radius = arc.GetRadius() + radial_offset;
    int       arc_inner_radius = arc.GetRadius() - radial_offset;
    ERROR_LOC errorLocInner = ( aErrorLoc == ERROR_INSIDE ) ? ERROR_OUTSIDE : ERROR_INSIDE;
    ERROR_LOC errorLocOuter = ( aErrorLoc == ERROR_INSIDE ) ? ERROR_INSIDE : ERROR_OUTSIDE;

    SHAPE_POLY_SET polyshape;
    polyshape.NewOutline();

    SHAPE_LINE_CHAIN& outline = polyshape.Outline( 0 );

    // Starting end
    ConvertArcToPolyline( outline, arc.GetP0(), radial_offset, arc_angle_start - ANGLE_180,
                          ANGLE_180, aError, aErrorLoc );

    // Outside edge
    ConvertArcToPolyline( outline, arc.GetCenter(), arc_outer_radius, arc_angle_start, arc_angle,
                          aError, errorLocOuter );

    // Other end
    ConvertArcToPolyline( outline, arc.GetP1(), radial_offset, arc_angle_end, ANGLE_180, aError,
                          aErrorLoc );

    // Inside edge
    if( arc_inner_radius > 0 )
    {
        ConvertArcToPolyline( outline, arc.GetCenter(), arc_inner_radius, arc_angle_end,
                              -arc_angle, aError, errorLocInner );
    }

    aBuffer.Append( polyshape );
}


void TransformRingToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aCentre, int aRadius,
                             int aWidth, int aError, ERROR_LOC aErrorLoc )
{
    int inner_radius = aRadius - ( aWidth / 2 );
    int outer_radius = inner_radius + aWidth;

    if( inner_radius <= 0 )
    {
        //In this case, the ring is just a circle (no hole inside)
        TransformCircleToPolygon( aBuffer, aCentre, aRadius + ( aWidth / 2 ), aError, aErrorLoc );
        return;
    }

    SHAPE_POLY_SET buffer;

    TransformCircleToPolygon( buffer, aCentre, outer_radius, aError, aErrorLoc );

    // Build the hole:
    buffer.NewHole();
    // The circle is the hole, so the approximation error location is the opposite of aErrorLoc
    ERROR_LOC inner_err_loc = aErrorLoc == ERROR_OUTSIDE ? ERROR_INSIDE : ERROR_OUTSIDE;
    TransformCircleToPolygon( buffer.Hole( 0, 0 ), aCentre, inner_radius,
                              aError, inner_err_loc );

    buffer.Fracture();
    aBuffer.Append( buffer );
}
