/**
 * @file convert_basic_shapes_to_polygon.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <trigo.h>
#include <macros.h>
#include <common.h>
#include <convert_basic_shapes_to_polygon.h>


/**
 * Function TransformCircleToPolygon
 * convert a circle to a polygon, using multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCenter = the center of the circle
 * @param aRadius = the radius of the circle
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * Note: the polygon is inside the circle, so if you want to have the polygon
 * outside the circle, you should give aRadius calculated with a correction factor
 */
void TransformCircleToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                               wxPoint aCenter, int aRadius,
                               int aCircleToSegmentsCount )
{
    wxPoint corner_position;
    double delta    = 3600.0 / aCircleToSegmentsCount;    // rot angle in 0.1 degree
    double halfstep = delta/2;    // the starting value for rot angles

    aCornerBuffer.NewOutline();

    for( int ii = 0; ii < aCircleToSegmentsCount; ii++ )
    {
        corner_position.x   = aRadius;
        corner_position.y   = 0;
        double angle = (ii * delta) + halfstep;
        RotatePoint( &corner_position, angle );
        corner_position += aCenter;
        aCornerBuffer.Append( corner_position.x, corner_position.y );
    }
}

void TransformOvalClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                wxPoint aStart, wxPoint aEnd, int aWidth,
                                int aCircleToSegmentsCount, double aCorrectionFactor )
{
    // To build the polygonal shape outside the actual shape, we use a bigger
    // radius to build rounded ends.
    // However, the width of the segment is too big.
    // so, later, we will clamp the polygonal shape with the bounding box
    // of the segment.
    int     radius  = aWidth / 2;

    // Note if we want to compensate the radius reduction of a circle due to
    // the segment approx, aCorrectionFactor must be calculated like this:
    // For a circle the min radius is radius * cos( 2PI / s_CircleToSegmentsCount / 2)
    // aCorrectionFactor is 1 /cos( PI/s_CircleToSegmentsCount  )

    radius = radius * aCorrectionFactor;    // make segments outside the circles

    // end point is the coordinate relative to aStart
    wxPoint endp    = aEnd - aStart;
    wxPoint startp  = aStart;
    wxPoint corner;
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
    int seg_len        = KiROUND( EuclideanNorm( endp ) );

    double delta = 3600.0 / aCircleToSegmentsCount;    // rot angle in 0.1 degree

    // Compute the outlines of the segment, and creates a polygon
    // Note: the polygonal shape is built from the equivalent horizontal
    // segment starting ar 0,0, and ending at seg_len,0

    // add right rounded end:
    for( int ii = 0; ii < aCircleToSegmentsCount/2; ii++ )
    {
        corner = wxPoint( 0, radius );
        RotatePoint( &corner, delta*ii );
        corner.x += seg_len;
        polyshape.Append( corner.x, corner.y );
    }

    // Finish arc:
    corner = wxPoint( seg_len, -radius );
    polyshape.Append( corner.x, corner.y );

    // add left rounded end:
    for( int ii = 0; ii < aCircleToSegmentsCount/2; ii++ )
    {
        corner = wxPoint( 0, -radius );
        RotatePoint( &corner, delta*ii );
        polyshape.Append( corner.x, corner.y );
    }

    // Finish arc:
    corner = wxPoint( 0, radius );
    polyshape.Append( corner.x, corner.y );

    // Now, clamp the polygonal shape (too big) with the segment bounding box
    // the polygonal shape bbox equivalent to the segment has a too big height,
    // and the right width
    if( aCorrectionFactor > 1.0 )
    {
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
    }

    // Rotate and move the polygon to its right location
    polyshape.Rotate( delta_angle, VECTOR2I( 0, 0 ) );
    polyshape.Move( startp );

    aCornerBuffer.Append( polyshape);
}

/* Returns the centers of the rounded corners of a rect.
 */
void GetRoundRectCornerCenters( wxPoint aCenters[4], int aRadius,
                const wxPoint& aPosition, const wxSize& aSize, double aRotation )
{
    wxSize size( aSize/2 );

    size.x -= aRadius;
    size.y -= aRadius;

    // Ensure size is > 0, to avoid generating unusable shapes
    // which can crash kicad.
    if( size.x <= 1 )
        size.x = 1;
    if( size.y <= 1 )
        size.y = 1;

    aCenters[0].x = -size.x;
    aCenters[0].y = size.y;

    aCenters[1].x = size.x;
    aCenters[1].y = size.y;

    aCenters[2].x = size.x;
    aCenters[2].y = -size.y;

    aCenters[3].x = -size.x;
    aCenters[3].y = -size.y;

    // Rotate the polygon
    if( aRotation )
    {
        for( int ii = 0; ii < 4; ii++ )
            RotatePoint( &aCenters[ii], aRotation );
    }

    // move the polygon to the position
    for( int ii = 0; ii < 4; ii++ )
        aCenters[ii] += aPosition;
}

/**
 * Function TransformRoundRectToPolygon
 * convert a rectangle with rounded corners to a polygon
 * Convert arcs to multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aPosition = the coordinate of the center of the rectangle
 * @param aSize = the size of the rectangle
 * @param aCornerRadius = radius of rounded corners
 * @param aRotation = rotation in 0.1 degrees of the rectangle
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 */
void TransformRoundRectToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                  const wxPoint& aPosition, const wxSize& aSize,
                                  double aRotation, int aCornerRadius,
                                  int aCircleToSegmentsCount )
{
    wxPoint corners[4];
    GetRoundRectCornerCenters( corners, aCornerRadius, aPosition, aSize, aRotation );

    SHAPE_POLY_SET outline;
    outline.NewOutline();

    for( int ii = 0; ii < 4; ++ii )
        outline.Append( corners[ii].x, corners[ii].y );

    outline.Inflate( aCornerRadius, aCircleToSegmentsCount );

    // Add the outline:
    aCornerBuffer.Append( outline );
}


/**
 * Function TransformRoundedEndsSegmentToPolygon
 * convert a segment with rounded ends to a polygon
 * Convert arcs to multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aStart = the segment start point coordinate
 * @param aEnd = the segment end point coordinate
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = the segment width
 * Note: the polygon is inside the arc ends, so if you want to have the polygon
 * outside the circle, you should give aStart and aEnd calculated with a correction factor
 */
void TransformRoundedEndsSegmentToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                           wxPoint aStart, wxPoint aEnd,
                                           int aCircleToSegmentsCount,
                                           int aWidth )
{
    int     radius  = aWidth / 2;
    wxPoint endp    = aEnd - aStart; // end point coordinate for the same segment starting at (0,0)
    wxPoint startp  = aStart;
    wxPoint corner;
    VECTOR2I polypoint;

    aCornerBuffer.NewOutline();

    // normalize the position in order to have endp.x >= 0;
    if( endp.x < 0 )
    {
        endp    = aStart - aEnd;
        startp  = aEnd;
    }

    double delta_angle = ArcTangente( endp.y, endp.x ); // delta_angle is in 0.1 degrees
    int seg_len        = KiROUND( EuclideanNorm( endp ) );

    int delta = 3600 / aCircleToSegmentsCount;    // rot angle in 0.1 degree

    // Compute the outlines of the segment, and creates a polygon
    // add right rounded end:
    for( int ii = 0; ii < 1800; ii += delta )
    {
        corner = wxPoint( 0, radius );
        RotatePoint( &corner, ii );
        corner.x += seg_len;
        RotatePoint( &corner, -delta_angle );
        corner += startp;
        polypoint.x = corner.x;
        polypoint.y = corner.y;
        aCornerBuffer.Append( polypoint.x, polypoint.y );
    }

    // Finish arc:
    corner = wxPoint( seg_len, -radius );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    polypoint.x = corner.x;
    polypoint.y = corner.y;
    aCornerBuffer.Append( polypoint.x, polypoint.y );

    // add left rounded end:
    for( int ii = 0; ii < 1800; ii += delta )
    {
        corner = wxPoint( 0, -radius );
        RotatePoint( &corner, ii );
        RotatePoint( &corner, -delta_angle );
        corner += startp;
        polypoint.x = corner.x;
        polypoint.y = corner.y;
        aCornerBuffer.Append( polypoint.x, polypoint.y );
    }

    // Finish arc:
    corner = wxPoint( 0, radius );
    RotatePoint( &corner, -delta_angle );
    corner += startp;
    polypoint.x = corner.x;
    polypoint.y = corner.y;
    aCornerBuffer.Append( polypoint.x, polypoint.y );
}


/**
 * Function TransformArcToPolygon
 * Creates a polygon from an Arc
 * Convert arcs to multiple straight segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCentre = centre of the arc or circle
 * @param aStart = start point of the arc, or a point on the circle
 * @param aArcAngle = arc angle in 0.1 degrees. For a circle, aArcAngle = 3600
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = width (thickness) of the line
 */
void TransformArcToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                            wxPoint aCentre, wxPoint aStart, double aArcAngle,
                            int aCircleToSegmentsCount, int aWidth )
{
    wxPoint arc_start, arc_end;
    int     delta = 3600 / aCircleToSegmentsCount;   // rotate angle in 0.1 degree

    arc_end = arc_start = aStart;

    if( aArcAngle != 3600 )
    {
        RotatePoint( &arc_end, aCentre, -aArcAngle );
    }

    if( aArcAngle < 0 )
    {
        std::swap( arc_start, arc_end );
        aArcAngle = -aArcAngle;
    }

    // Compute the ends of segments and creates poly
    wxPoint curr_end    = arc_start;
    wxPoint curr_start  = arc_start;

    for( int ii = delta; ii < aArcAngle; ii += delta )
    {
        curr_end = arc_start;
        RotatePoint( &curr_end, aCentre, -ii );
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer, curr_start, curr_end,
                                              aCircleToSegmentsCount, aWidth );
        curr_start = curr_end;
    }

    if( curr_end != arc_end )
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                                              curr_end, arc_end,
                                              aCircleToSegmentsCount, aWidth );
}


/**
 * Function TransformRingToPolygon
 * Creates a polygon from a ring
 * Convert arcs to multiple straight segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCentre = centre of the arc or circle
 * @param aRadius = radius of the circle
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aWidth = width (thickness) of the ring
 */
void TransformRingToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                             wxPoint aCentre, int aRadius,
                             int aCircleToSegmentsCount, int aWidth )
{
    // Compute the corners positions and creates the poly
    wxPoint curr_point;
    int     inner_radius    = aRadius - ( aWidth / 2 );
    int     outer_radius    = inner_radius + aWidth;

    if( inner_radius <= 0 )
    {   //In this case, the ring is just a circle (no hole inside)
        TransformCircleToPolygon( aCornerBuffer, aCentre, aRadius + ( aWidth / 2 ),
                                  aCircleToSegmentsCount );
        return;
    }

    aCornerBuffer.NewOutline();

    // Draw the inner circle of the ring
    int     delta = 3600 / aCircleToSegmentsCount;   // rotate angle in 0.1 degree

    for( int ii = 0; ii < 3600; ii += delta )
    {
        curr_point.x    = inner_radius;
        curr_point.y    = 0;
        RotatePoint( &curr_point, ii );
        curr_point      += aCentre;
        aCornerBuffer.Append( curr_point.x, curr_point.y );
    }

    // Draw the last point of inner circle
    aCornerBuffer.Append( aCentre.x + inner_radius, aCentre.y );

    // Draw the outer circle of the ring
    // the first point creates also a segment from the inner to the outer polygon
    for( int ii = 0; ii < 3600; ii += delta )
    {
        curr_point.x    = outer_radius;
        curr_point.y    = 0;
        RotatePoint( &curr_point, -ii );
        curr_point      += aCentre;
        aCornerBuffer.Append( curr_point.x, curr_point.y );
    }

    // Draw the last point of outer circle
    aCornerBuffer.Append( aCentre.x + outer_radius, aCentre.y );

    // And connect the outer polygon to the inner polygon,.
    // because a segment from inner to the outer polygon was already created,
    // the final polygon is the inner and the outer outlines connected by
    // 2 overlapping segments
    aCornerBuffer.Append( aCentre.x + inner_radius, aCentre.y );
}
