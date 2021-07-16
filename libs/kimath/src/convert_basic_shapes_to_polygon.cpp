/**
 * @file convert_basic_shapes_to_polygon.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <type_traits>                  // for swap

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_line_chain.h>  // for SHAPE_LINE_CHAIN
#include <geometry/shape_poly_set.h>    // for SHAPE_POLY_SET, SHAPE_POLY_SE...
#include <math/util.h>
#include <math/vector2d.h>              // for VECTOR2I
#include <trigo.h>


void TransformCircleToPolygon( SHAPE_LINE_CHAIN& aCornerBuffer, wxPoint aCenter, int aRadius,
                               int aError, ERROR_LOC aErrorLoc, int aMinSegCount )
{
    wxPoint corner_position;
    int     numSegs = GetArcToSegmentCount( aRadius, aError, 360.0 );
    numSegs = std::max( aMinSegCount, numSegs );

    // The shape will be built with a even number of segs. Reason: the horizontal
    // diameter begins and ends to points on the actual circle, or circle
    // expanded by aError if aErrorLoc == ERROR_OUTSIDE.
    // This is used by Arc to Polygon shape convert.
    if( numSegs & 1 )
        numSegs++;

    int     delta = 3600 / numSegs;           // rotate angle in 0.1 degree
    int     radius = aRadius;

    if( aErrorLoc == ERROR_OUTSIDE )
    {
        // The outer radius should be radius+aError
        // Recalculate the actual approx error, as it can be smaller than aError
        // because numSegs is clamped to a minimal value
        int actual_delta_radius = CircleToEndSegmentDeltaRadius( radius, numSegs );
        radius += GetCircleToPolyCorrection( actual_delta_radius );
    }

    for( int angle = 0; angle < 3600; angle += delta )
    {
        corner_position.x   = radius;
        corner_position.y   = 0;
        RotatePoint( &corner_position, angle );
        corner_position += aCenter;
        aCornerBuffer.Append( corner_position.x, corner_position.y );
    }

    aCornerBuffer.SetClosed( true );
}


void TransformCircleToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aCenter, int aRadius,
                               int aError, ERROR_LOC aErrorLoc, int aMinSegCount )
{
    wxPoint corner_position;
    int     numSegs = GetArcToSegmentCount( aRadius, aError, 360.0 );
    numSegs = std::max( aMinSegCount, numSegs);

    // The shape will be built with a even number of segs. Reason: the horizontal
    // diameter begins and ends to points on the actual circle, or circle
    // expanded by aError if aErrorLoc == ERROR_OUTSIDE.
    // This is used by Arc to Polygon shape convert.
    if( numSegs & 1 )
        numSegs++;

    int     delta = 3600 / numSegs;           // rotate angle in 0.1 degree
    int     radius = aRadius;

    if( aErrorLoc == ERROR_OUTSIDE )
    {
        // The outer radius should be radius+aError
        // Recalculate the actual approx error, as it can be smaller than aError
        // because numSegs is clamped to a minimal value
        int actual_delta_radius = CircleToEndSegmentDeltaRadius( radius, numSegs );
        radius += GetCircleToPolyCorrection( actual_delta_radius );
    }

    aCornerBuffer.NewOutline();

    for( int angle = 0; angle < 3600; angle += delta )
    {
        corner_position.x = radius;
        corner_position.y = 0;
        RotatePoint( &corner_position, angle );
        corner_position += aCenter;
        aCornerBuffer.Append( corner_position.x, corner_position.y );
    }

    // Finish circle
    corner_position.x = radius;
    corner_position.y = 0;
    corner_position += aCenter;
    aCornerBuffer.Append( corner_position.x, corner_position.y );
}


void TransformOvalToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aStart, wxPoint aEnd,
                             int aWidth, int aError, ERROR_LOC aErrorLoc, int aMinSegCount )
{
    // To build the polygonal shape outside the actual shape, we use a bigger
    // radius to build rounded ends.
    // However, the width of the segment is too big.
    // so, later, we will clamp the polygonal shape with the bounding box
    // of the segment.
    int radius  = aWidth / 2;
    int numSegs = GetArcToSegmentCount( radius, aError, 360.0 );
    numSegs = std::max( aMinSegCount, numSegs );

    int delta = 3600 / numSegs;   // rotate angle in 0.1 degree

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
    wxPoint        endp = aEnd - aStart;
    wxPoint        startp = aStart;
    wxPoint        corner;
    SHAPE_POLY_SET polyshape;

    polyshape.NewOutline();

    // normalize the position in order to have endp.x >= 0
    // it makes calculations more easy to understand
    if( endp.x < 0 )
    {
        endp    = aStart - aEnd;
        startp  = aEnd;
    }

    // delta_angle is in radian
    double delta_angle = atan2( (double)endp.y, (double)endp.x );
    int    seg_len     = KiROUND( EuclideanNorm( endp ) );

    // Compute the outlines of the segment, and creates a polygon
    // Note: the polygonal shape is built from the equivalent horizontal
    // segment starting at {0,0}, and ending at {seg_len,0}

    // add right rounded end:

    for( int angle = 0; angle < 1800; angle += delta )
    {
        corner = wxPoint( 0, radius );
        RotatePoint( &corner, angle );
        corner.x += seg_len;
        polyshape.Append( corner.x, corner.y );
    }

    // Finish arc:
    corner = wxPoint( seg_len, -radius );
    polyshape.Append( corner.x, corner.y );

    // add left rounded end:
    for( int angle = 0; angle < 1800; angle += delta )
    {
        corner = wxPoint( 0, -radius );
        RotatePoint( &corner, angle );
        polyshape.Append( corner.x, corner.y );
    }

    // Finish arc:
    corner = wxPoint( 0, radius );
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
    polyshape.BooleanIntersection( bbox, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    // Note the final polygon is a simple, convex polygon with no hole
    // due to the shape of initial polygons

    // Rotate and move the polygon to its right location
    polyshape.Rotate( delta_angle, VECTOR2I( 0, 0 ) );
    polyshape.Move( startp );

    aCornerBuffer.Append( polyshape);
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
            outline.Append( cur.m_position );
        else
        {
            VECTOR2I position = cur.m_position;
            int      radius = cur.m_radius;
            double   cosNum = (double) incoming.x * outgoing.x + (double) incoming.y * outgoing.y;
            double   cosDen = (double) incoming.EuclideanNorm() * outgoing.EuclideanNorm();
            double   angle = acos( cosNum / cosDen );
            double   tanAngle2 = tan( ( M_PI - angle ) / 2 );

            if( aInflate )
            {
                radius += aInflate;
                position += incoming.Resize( aInflate / tanAngle2 )
                          + incoming.Perpendicular().Resize( -aInflate );
            }

            // Ensure 16+ segments per 360° and ensure first & last segment are the same size
            int    numSegs = std::max( 16, GetArcToSegmentCount( radius, aError, 360.0 ) );
            int    angDelta = 3600 / numSegs;
            int    targetAngle = RAD2DECIDEG( angle );
            int    angPos = ( angDelta + ( targetAngle % angDelta ) ) / 2;

            double   centerProjection = radius / tanAngle2;
            VECTOR2I arcStart = position - incoming.Resize( centerProjection );
            VECTOR2I arcEnd = position + outgoing.Resize( centerProjection );
            VECTOR2I arcCenter = arcStart + incoming.Perpendicular().Resize( radius );

            if( aErrorLoc == ERROR_INSIDE )
            {
                outline.Append( arcStart );
                VECTOR2I zeroRef = arcStart - arcCenter;

                for( ; angPos < targetAngle; angPos += angDelta )
                {
                    VECTOR2I pt = zeroRef;
                    RotatePoint( pt, -angPos );
                    outline.Append( pt + arcCenter );
                }

                outline.Append( arcEnd );
            }
            else
            {
                // The outer radius should be radius+aError, recalculate because numSegs is clamped
                int      actualDeltaRadius = CircleToEndSegmentDeltaRadius( radius, numSegs );
                int      radiusExtend = GetCircleToPolyCorrection( actualDeltaRadius );
                VECTOR2I arcExStart = arcStart + incoming.Perpendicular().Resize( -radiusExtend );
                VECTOR2I arcExEnd = arcEnd + outgoing.Perpendicular().Resize( -radiusExtend );

                // A larger radius will create "ears", so we intersect the first and last segment
                // of the rounded corner with the non-rounded outline
                SEG      inSeg( position - incoming, position );
                SEG      outSeg( position, position + outgoing );
                VECTOR2I zeroRef = arcExStart - arcCenter;
                VECTOR2I pt = zeroRef;

                RotatePoint( pt, -angPos );
                pt += arcCenter;
                OPT<VECTOR2I> intersect = inSeg.Intersect( SEG( arcExStart, pt ) );
                outline.Append( intersect.is_initialized() ? intersect.get() : arcStart );
                outline.Append( pt );
                angPos += angDelta;

                for( ; angPos < targetAngle; angPos += angDelta )
                {
                    pt = zeroRef;
                    RotatePoint( pt, -angPos );
                    pt += arcCenter;
                    outline.Append( pt );
                }

                intersect = outSeg.Intersect( SEG( pt, arcExEnd ) );
                outline.Append( intersect.is_initialized() ? intersect.get() : arcEnd );
            }
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


void TransformTrapezoidToPolygon( SHAPE_POLY_SET& aCornerBuffer, const wxPoint& aPosition,
                                  const wxSize& aSize, double aRotation, int aDeltaX, int aDeltaY,
                                  int aInflate, int aError, ERROR_LOC aErrorLoc )
{
    SHAPE_POLY_SET outline;
    wxSize         size( aSize / 2 );

    std::vector<ROUNDED_CORNER> corners;
    corners.reserve( 4 );
    corners.push_back( ROUNDED_CORNER( -size.x + aDeltaY, -size.y - aDeltaX ) );
    corners.push_back( ROUNDED_CORNER( size.x - aDeltaY, -size.y + aDeltaX ) );
    corners.push_back( ROUNDED_CORNER( size.x + aDeltaY, size.y - aDeltaX ) );
    corners.push_back( ROUNDED_CORNER( -size.x - aDeltaY, size.y + aDeltaX ) );

    if( aDeltaY == size.x || aDeltaX == size.y )
        CornerListRemoveDuplicates( corners );

    CornerListToPolygon( outline, corners, aInflate, aError, aErrorLoc );

    if( aRotation != 0.0 )
        outline.Rotate( DECIDEG2RAD( -aRotation ), VECTOR2I( 0, 0 ) );

    outline.Move( VECTOR2I( aPosition ) );
    aCornerBuffer.Append( outline );
}


void TransformRoundChamferedRectToPolygon( SHAPE_POLY_SET& aCornerBuffer, const wxPoint& aPosition,
                                           const wxSize& aSize, double aRotation, int aCornerRadius,
                                           double aChamferRatio, int aChamferCorners, int aInflate,
                                           int aError, ERROR_LOC aErrorLoc )
{
    SHAPE_POLY_SET outline;
    wxSize         size( aSize / 2 );
    int            chamferCnt = std::bitset<8>( aChamferCorners ).count();

    // Ensure size is > 0, to avoid generating unusable shapes which can crash kicad.
    size.x = std::max( 1, size.x );
    size.y = std::max( 1, size.y );

    std::vector<ROUNDED_CORNER> corners;
    corners.reserve( 4 + chamferCnt );
    corners.push_back( ROUNDED_CORNER( -size.x, -size.y, aCornerRadius ) );
    corners.push_back( ROUNDED_CORNER( size.x, -size.y, aCornerRadius ) );
    corners.push_back( ROUNDED_CORNER( size.x, size.y, aCornerRadius ) );
    corners.push_back( ROUNDED_CORNER( -size.x, size.y, aCornerRadius ) );

    if( aChamferCorners )
    {
        int shorterSide = std::min( aSize.x, aSize.y );
        int chamfer = aChamferRatio * shorterSide;
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

    if( aRotation != 0.0 )
        outline.Rotate( DECIDEG2RAD( -aRotation ), VECTOR2I( 0, 0 ) );

    outline.Move( VECTOR2I( aPosition ) );
    aCornerBuffer.Append( outline );
}


int ConvertArcToPolyline( SHAPE_LINE_CHAIN& aPolyline, VECTOR2I aCenter, int aRadius,
                          double aStartAngleDeg, double aArcAngleDeg, double aAccuracy,
                          ERROR_LOC aErrorLoc )
{
    double endAngle = aStartAngleDeg + aArcAngleDeg;
    int n = 2;

    if( aRadius >= aAccuracy )
        n = GetArcToSegmentCount( aRadius, aAccuracy, aArcAngleDeg )+1;  // n >= 3

    if( aErrorLoc == ERROR_OUTSIDE )
    {
        int seg360 = std::abs( KiROUND( n * 360.0 / aArcAngleDeg ) );
        int actual_delta_radius = CircleToEndSegmentDeltaRadius( aRadius, seg360 );
        aRadius += actual_delta_radius;
    }

    for( int i = 0; i <= n ; i++ )
    {
        double rot = aStartAngleDeg;
        rot += ( aArcAngleDeg * i ) / n;

        double x = aCenter.x + aRadius * cos( rot * M_PI / 180.0 );
        double y = aCenter.y + aRadius * sin( rot * M_PI / 180.0 );

        aPolyline.Append( KiROUND( x ), KiROUND( y ) );
    }

    return n;
}


void TransformArcToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aStart, wxPoint aMid,
                            wxPoint aEnd, int aWidth, int aError, ERROR_LOC aErrorLoc )
{
    SHAPE_ARC        arc( aStart, aMid, aEnd, aWidth );
    // Currentlye have currently 2 algos:
    // the first approximates the thick arc from its outlines
    // the second approximates the thick arc from segments given by SHAPE_ARC
    // using SHAPE_ARC::ConvertToPolyline
    // The actual approximation errors are similar but not exactly the same.
    //
    // For now, both algorithms are kept, the second is the initial algo used in Kicad.

#if 1
    // This appproximation convert the 2 ends to polygons, arc outer to polyline
    // and arc inner to polyline and merge shapes.
    int              radial_offset = ( aWidth + 1 ) / 2;

    SHAPE_POLY_SET   polyshape;
    std::vector<VECTOR2I> outside_pts;

    /// We start by making rounded ends on the arc
    TransformCircleToPolygon( polyshape, aStart, radial_offset, aError, aErrorLoc );
    TransformCircleToPolygon( polyshape, aEnd, radial_offset, aError, aErrorLoc );

    // The circle polygon is built with a even number of segments, so the
    // horizontal diameter has 2 corners on the biggest diameter
    // Rotate these 2 corners to match the start and ens points of inner and outer
    // end points of the arc appoximation outlines, build below.
    // The final shape is much better.
    double arc_angle_start_deg = arc.GetStartAngle();
    double arc_angle = arc.GetCentralAngle();
    double arc_angle_end_deg = arc_angle_start_deg + arc_angle;

    if( arc_angle_start_deg != 0 && arc_angle_start_deg != 180.0 )
        polyshape.Outline(0).Rotate( arc_angle_start_deg * M_PI/180.0, aStart );

    if( arc_angle_end_deg != 0 && arc_angle_end_deg != 180.0 )
        polyshape.Outline(1).Rotate( arc_angle_end_deg * M_PI/180.0, aEnd );

    VECTOR2I center = arc.GetCenter();
    int      radius = arc.GetRadius();

    int      arc_outer_radius = radius + radial_offset;
    int      arc_inner_radius = radius - radial_offset;
    ERROR_LOC errorLocInner = ERROR_OUTSIDE;
    ERROR_LOC errorLocOuter = ERROR_INSIDE;

    if( aErrorLoc == ERROR_OUTSIDE )
    {
        errorLocInner = ERROR_INSIDE;
        errorLocOuter = ERROR_OUTSIDE;
    }

    polyshape.NewOutline();

    ConvertArcToPolyline( polyshape.Outline(2), center, arc_outer_radius,
                          arc_angle_start_deg, arc_angle, aError, errorLocOuter );

    if( arc_inner_radius > 0 )
        ConvertArcToPolyline( polyshape.Outline(2), center, arc_inner_radius,
                              arc_angle_end_deg, -arc_angle, aError, errorLocInner );
    else
        polyshape.Append( center );
#else
    // This appproximation use SHAPE_ARC to convert the 2 ends to polygons,
    // approximate arc to polyline, convert the polyline corners to outer and inner
    // corners of outer and inner utliners and merge shapes.
    double defaultErr;
    SHAPE_LINE_CHAIN arcSpine = arc.ConvertToPolyline( SHAPE_ARC::DefaultAccuracyForPCB(),
                                                       &defaultErr);
    int              radius = arc.GetRadius();
    int              radial_offset = ( aWidth + 1 ) / 2;
    SHAPE_POLY_SET   polyshape;
    std::vector<VECTOR2I> outside_pts;

    // delta is the effective error approximation to build a polyline from an arc
    int segCnt360 = arcSpine.GetSegmentCount()*360.0/arc.GetCentralAngle();;
    int delta = CircleToEndSegmentDeltaRadius( radius+radial_offset, std::abs(segCnt360) );

    /// We start by making rounded ends on the arc
    TransformCircleToPolygon( polyshape, aStart, radial_offset, aError, aErrorLoc );
    TransformCircleToPolygon( polyshape, aEnd, radial_offset, aError, aErrorLoc );

    // The circle polygon is built with a even number of segments, so the
    // horizontal diameter has 2 corners on the biggest diameter
    // Rotate these 2 corners to match the start and ens points of inner and outer
    // end points of the arc appoximation outlines, build below.
    // The final shape is much better.
    double arc_angle_end_deg = arc.GetStartAngle();

    if( arc_angle_end_deg != 0 && arc_angle_end_deg != 180.0 )
        polyshape.Outline(0).Rotate( arc_angle_end_deg * M_PI/180.0, arcSpine.GetPoint( 0 ) );

    arc_angle_end_deg = arc.GetEndAngle();

    if( arc_angle_end_deg != 0 && arc_angle_end_deg != 180.0 )
        polyshape.Outline(1).Rotate( arc_angle_end_deg * M_PI/180.0, arcSpine.GetPoint( -1 ) );

    if( aErrorLoc == ERROR_OUTSIDE )
        radial_offset += delta + defaultErr/2;
    else
        radial_offset -= defaultErr/2;

    if( radial_offset < 0 )
        radial_offset = 0;

    polyshape.NewOutline();

    VECTOR2I center = arc.GetCenter();
    int last_index = arcSpine.GetPointCount() -1;

    for( std::size_t ii = 0; ii <= last_index; ++ii )
    {
        VECTOR2I offset = arcSpine.GetPoint( ii ) - center;
        int curr_rd = radius;

        polyshape.Append( offset.Resize( curr_rd - radial_offset ) + center );
        outside_pts.emplace_back( offset.Resize( curr_rd + radial_offset ) + center );
    }

    for( auto it = outside_pts.rbegin(); it != outside_pts.rend(); ++it )
        polyshape.Append( *it );
#endif

    // Can be removed, but usefull to display the outline:
    polyshape.Simplify( SHAPE_POLY_SET::PM_FAST );

    aCornerBuffer.Append( polyshape );

}


void TransformRingToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aCentre, int aRadius,
                             int aWidth, int aError, ERROR_LOC aErrorLoc )
{
    int inner_radius = aRadius - ( aWidth / 2 );
    int outer_radius = inner_radius + aWidth;

    if( inner_radius <= 0 )
    {   //In this case, the ring is just a circle (no hole inside)
        TransformCircleToPolygon( aCornerBuffer, aCentre, aRadius + ( aWidth / 2 ), aError,
                                  aErrorLoc );
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

    buffer.Fracture( SHAPE_POLY_SET::PM_FAST );
    aCornerBuffer.Append( buffer );
}
