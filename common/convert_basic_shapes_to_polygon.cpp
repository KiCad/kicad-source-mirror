/**
 * @file convert_basic_shapes_to_polygon.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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
    int     delta       = 3600 / aCircleToSegmentsCount;    // rot angle in 0.1 degree
    int     halfstep    = 1800 / aCircleToSegmentsCount;    // the starting value for rot angles

    aCornerBuffer.NewOutline();

    for( int ii = 0; ii < aCircleToSegmentsCount; ii++ )
    {
        corner_position.x   = aRadius;
        corner_position.y   = 0;
        int     angle = (ii * delta) + halfstep;
        RotatePoint( &corner_position.x, &corner_position.y, angle );
        corner_position += aCenter;
        aCornerBuffer.Append( corner_position.x, corner_position.y );
    }
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
    aCornerBuffer.Append( aCentre.x + inner_radius, aCentre.y );
}
