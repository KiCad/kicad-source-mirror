/**
 * @file convert_basic_shapes_to_polygon.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <geometry/geometry_utils.h>


void TransformCircleToPolygon( SHAPE_LINE_CHAIN& aBuffer,
                               wxPoint aCenter, int aRadius,
                               int aError )
{
    wxPoint corner_position;
    int     numSegs = std::max( GetArcToSegmentCount( aRadius, aError, 360.0 ), 6 );
    int     delta = 3600 / numSegs;   // rotate angle in 0.1 degree
    double  correction = GetCircletoPolyCorrectionFactor( numSegs );
    int     radius = aRadius * correction;    // make segments outside the circles
    double  halfstep = delta/2;    // the starting value for rot angles

    for( int ii = 0; ii < numSegs; ii++ )
    {
        corner_position.x   = radius;
        corner_position.y   = 0;
        double angle = (ii * delta) + halfstep;
        RotatePoint( &corner_position, angle );
        corner_position += aCenter;
        aBuffer.Append( corner_position.x, corner_position.y );
    }

    aBuffer.SetClosed( true );
}


void TransformCircleToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aCenter, int aRadius,
                               int aError )
{
    wxPoint corner_position;
    int     numSegs = std::max( GetArcToSegmentCount( aRadius, aError, 360.0 ), 6 );
    int     delta = 3600 / numSegs;   // rotate angle in 0.1 degree
    double  correction = GetCircletoPolyCorrectionFactor( numSegs );
    int     radius = aRadius * correction;    // make segments outside the circles
    double  halfstep = delta/2;    // the starting value for rot angles

    aCornerBuffer.NewOutline();

    for( int ii = 0; ii < numSegs; ii++ )
    {
        corner_position.x   = radius;
        corner_position.y   = 0;
        double angle = (ii * delta) + halfstep;
        RotatePoint( &corner_position, angle );
        corner_position += aCenter;
        aCornerBuffer.Append( corner_position.x, corner_position.y );
    }
}


void TransformOvalToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aStart, wxPoint aEnd,
                             int aWidth, int aError )
{
    // To build the polygonal shape outside the actual shape, we use a bigger
    // radius to build rounded ends.
    // However, the width of the segment is too big.
    // so, later, we will clamp the polygonal shape with the bounding box
    // of the segment.
    int     radius  = aWidth / 2;
    int     numSegs = std::max( GetArcToSegmentCount( radius, aError, 360.0 ), 6 );
    int     delta = 3600 / numSegs;   // rotate angle in 0.1 degree
    double  correction = GetCircletoPolyCorrectionFactor( numSegs );

    radius = radius * correction;    // make segments outside the circles

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


    // Compute the outlines of the segment, and creates a polygon
    // Note: the polygonal shape is built from the equivalent horizontal
    // segment starting ar 0,0, and ending at seg_len,0

    // add right rounded end:
    for( int ii = 0; ii < numSegs / 2; ii++ )
    {
        corner = wxPoint( 0, radius );
        RotatePoint( &corner, delta * ii );
        corner.x += seg_len;
        polyshape.Append( corner.x, corner.y );
    }

    // Finish arc:
    corner = wxPoint( seg_len, -radius );
    polyshape.Append( corner.x, corner.y );

    // add left rounded end:
    for( int ii = 0; ii < numSegs / 2; ii++ )
    {
        corner = wxPoint( 0, -radius );
        RotatePoint( &corner, delta * ii );
        polyshape.Append( corner.x, corner.y );
    }

    // Finish arc:
    corner = wxPoint( 0, radius );
    polyshape.Append( corner.x, corner.y );

    // Now, clamp the polygonal shape (too big) with the segment bounding box
    // the polygonal shape bbox equivalent to the segment has a too big height,
    // and the right width
    if( correction > 1.0 )
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


void TransformRoundChamferedRectToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                  const wxPoint& aPosition, const wxSize& aSize,
                                  double aRotation, int aCornerRadius,
                                  double aChamferRatio, int aChamferCorners,
                                  int aApproxErrorMax, int aMinSegPerCircleCount )
{
    // Build the basic shape in orientation 0.0, position 0,0 for chamfered corners
    // or in actual position/orientation for round rect only
    wxPoint corners[4];
    GetRoundRectCornerCenters( corners, aCornerRadius,
                               aChamferCorners ? wxPoint( 0, 0 ) : aPosition,
                               aSize, aChamferCorners ? 0.0 : aRotation );

    SHAPE_POLY_SET outline;
    outline.NewOutline();

    for( int ii = 0; ii < 4; ++ii )
        outline.Append( corners[ii].x, corners[ii].y );

    int     numSegs = std::max( GetArcToSegmentCount( aCornerRadius, aApproxErrorMax, 360.0 ),
                                                      aMinSegPerCircleCount );
    outline.Inflate( aCornerRadius, numSegs );

    if( aChamferCorners == RECT_NO_CHAMFER )      // no chamfer
    {
        // Add the outline:
        aCornerBuffer.Append( outline );
        return;
    }

    // Now we have the round rect outline, in position 0,0 orientation 0.0.
    // Chamfer the corner(s).
    int chamfer_value = aChamferRatio * std::min( aSize.x, aSize.y );

    SHAPE_POLY_SET chamfered_corner;    // corner shape for the current corner to chamfer

    int corner_id[4] =
    {
        RECT_CHAMFER_TOP_LEFT, RECT_CHAMFER_TOP_RIGHT,
        RECT_CHAMFER_BOTTOM_LEFT, RECT_CHAMFER_BOTTOM_RIGHT
    };
    // Depending on the corner position, signX[] and signY[] give the sign of chamfer
    // coordinates relative to the corner position
    // The first corner is the top left corner, then top right, bottom left and bottom right
    int signX[4] = {1, -1, 1,-1 };
    int signY[4] = {1, 1, -1,-1 };

    for( int ii = 0; ii < 4; ii++ )
    {
        if( (corner_id[ii] & aChamferCorners) == 0 )
            continue;

        VECTOR2I corner_pos( -signX[ii]*aSize.x/2, -signY[ii]*aSize.y/2 );

        if( aCornerRadius )
        {
            // We recreate a rectangular area covering the full rounded corner (max size = aSize/2)
            // to rebuild the corner before chamfering, to be sure the rounded corner shape does not
            // overlap the chamfered corner shape:
            chamfered_corner.RemoveAllContours();
            chamfered_corner.NewOutline();
            chamfered_corner.Append( 0, 0 );
            chamfered_corner.Append( 0, signY[ii] * aSize.y / 2 );
            chamfered_corner.Append( signX[ii] * aSize.x / 2, signY[ii] * aSize.y / 2 );
            chamfered_corner.Append( signX[ii] * aSize.x / 2, 0 );
            chamfered_corner.Move( corner_pos );
            outline.BooleanAdd( chamfered_corner, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
        }

        // Now chamfer this corner
        chamfered_corner.RemoveAllContours();
        chamfered_corner.NewOutline();
        chamfered_corner.Append( 0, 0 );
        chamfered_corner.Append( 0, signY[ii] * chamfer_value );
        chamfered_corner.Append( signX[ii] * chamfer_value, 0 );
        chamfered_corner.Move( corner_pos );
        outline.BooleanSubtract( chamfered_corner, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    }

    // Rotate and move the outline:
    if( aRotation != 0.0 )
        outline.Rotate( DECIDEG2RAD( -aRotation ), VECTOR2I( 0, 0 ) );

    outline.Move( VECTOR2I( aPosition ) );

    // Add the outline:
    aCornerBuffer.Append( outline );
}


void TransformSegmentToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                wxPoint aStart, wxPoint aEnd,
                                int aError, int aWidth )
{
    int      radius  = aWidth / 2;
    wxPoint  endp    = aEnd - aStart; // end point coordinate for the same segment starting at (0,0)
    wxPoint  startp  = aStart;
    wxPoint  corner;
    VECTOR2I polypoint;
    int      numSegs = std::max( GetArcToSegmentCount( radius, aError, 360.0 ), 6 );
    double   correction = GetCircletoPolyCorrectionFactor( numSegs );
    int      delta = 3600 / numSegs;   // rotate angle in 0.1 degree

    radius = KiROUND( radius * correction );
    aCornerBuffer.NewOutline();

    // normalize the position in order to have endp.x >= 0;
    if( endp.x < 0 )
    {
        endp    = aStart - aEnd;
        startp  = aEnd;
    }

    double delta_angle = ArcTangente( endp.y, endp.x ); // delta_angle is in 0.1 degrees
    int seg_len        = KiROUND( EuclideanNorm( endp ) );

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


void TransformArcToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                            wxPoint aCentre, wxPoint aStart, double aArcAngle,
                            int aError, int aWidth )
{
    wxPoint arc_start, arc_end;
    int     dist = EuclideanNorm( aCentre - aStart );
    int     numSegs = std::max( GetArcToSegmentCount( dist, aError, 360.0 ), 6 );
    int     delta = 3600 / numSegs;   // rotate angle in 0.1 degree

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
        TransformSegmentToPolygon( aCornerBuffer, curr_start, curr_end, aError,
                                   aWidth );
        curr_start = curr_end;
    }

    if( curr_end != arc_end )
        TransformSegmentToPolygon( aCornerBuffer, curr_end, arc_end, aError, aWidth );
}


void TransformRingToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                             wxPoint aCentre, int aRadius,
                             int aError, int aWidth )
{
    // Compute the corners positions and creates the poly
    wxPoint curr_point;
    int     inner_radius    = aRadius - ( aWidth / 2 );
    int     outer_radius    = inner_radius + aWidth;

    if( inner_radius <= 0 )
    {   //In this case, the ring is just a circle (no hole inside)
        TransformCircleToPolygon( aCornerBuffer, aCentre, aRadius + ( aWidth / 2 ), aError );
        return;
    }

    SHAPE_POLY_SET buffer;

    TransformCircleToPolygon( buffer, aCentre, outer_radius, aError );

    // Build the hole:
    buffer.NewHole();
    TransformCircleToPolygon( buffer.Hole( 0, 0 ), aCentre, inner_radius, aError );

    buffer.Fracture( SHAPE_POLY_SET::PM_FAST );
    aCornerBuffer.Append( buffer );
}
