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
                               int aError, ERROR_LOC aErrorLoc )
{
    wxPoint corner_position;
    int     numSegs = GetArcToSegmentCount( aRadius, aError, 360.0 );

    // The shape will be built with a even number of segs. Reason: the horizontal
    // diameter begins and ends to points on the actual circle, or circle
    // expanded by aError if aErrorLoc == ERROR_OUTSIDE.
    // This is used by Arc to Polygon shape convert.
    if( numSegs & 1 )
        numSegs++;

    int     delta = 3600 / numSegs;           // rotate angle in 0.1 degree
    int     radius = aRadius;

    if( aErrorLoc == ERROR_OUTSIDE )
        radius += GetCircleToPolyCorrection( aError );

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
                               int aError, ERROR_LOC aErrorLoc )
{
    wxPoint corner_position;
    int     numSegs = GetArcToSegmentCount( aRadius, aError, 360.0 );

    // The shape will be built with a even number of segs. Reason: the horizontal
    // diameter begins and ends to points on the actual circle, or circle
    // expanded by aError if aErrorLoc == ERROR_OUTSIDE.
    // This is used by Arc to Polygon shape convert.
    if( numSegs & 1 )
        numSegs++;

    int     delta = 3600 / numSegs;           // rotate angle in 0.1 degree
    int     radius = aRadius;

    if( aErrorLoc == ERROR_OUTSIDE )
        radius += GetCircleToPolyCorrection( aError );

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
                             int aWidth, int aError, ERROR_LOC aErrorLoc )
{
    // To build the polygonal shape outside the actual shape, we use a bigger
    // radius to build rounded ends.
    // However, the width of the segment is too big.
    // so, later, we will clamp the polygonal shape with the bounding box
    // of the segment.
    int radius  = aWidth / 2;
    int numSegs = GetArcToSegmentCount( radius, aError, 360.0 );
    int delta = 3600 / numSegs;   // rotate angle in 0.1 degree
    int correction = GetCircleToPolyCorrection( aError );

    if( aErrorLoc == ERROR_OUTSIDE )
        radius += correction;

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


// Return a polygon representing a round rect centered at {0,0}
void TransformRoundRectToPolygon( SHAPE_POLY_SET& aCornerBuffer, const wxSize& aSize,
                                  int aCornerRadius, int aError, ERROR_LOC aErrorLoc )
{
    SHAPE_POLY_SET outline;

    wxPoint centers[4];
    wxSize size( aSize / 2 );

    size.x -= aCornerRadius;
    size.y -= aCornerRadius;

    // Ensure size is > 0, to avoid generating unusable shapes which can crash kicad.
    size.x = std::max( 1, size.x );
    size.y = std::max( 1, size.y );

    centers[0] = wxPoint( -size.x,  size.y );
    centers[1] = wxPoint(  size.x,  size.y );
    centers[2] = wxPoint(  size.x, -size.y );
    centers[3] = wxPoint( -size.x, -size.y );

    int numSegs = GetArcToSegmentCount( aCornerRadius, aError, 360.0 );

    // Choppy corners on rounded-corner rectangles look awful so enforce a minimum of
    // 4 segments per corner.
    if( numSegs < 16 )
        numSegs = 16;

    int delta = 3600 / numSegs;           // rotate angle in 0.1 degree
    int radius = aCornerRadius;

    if( aErrorLoc == ERROR_OUTSIDE )
        radius += GetCircleToPolyCorrection( aError );

    auto genArc =
            [&]( const wxPoint& aCenter, int aStart, int aEnd )
            {
                for( int angle = aStart + delta; angle < aEnd; angle += delta )
                {
                    wxPoint pt( -radius, 0 );
                    RotatePoint( &pt, angle );
                    pt += aCenter;
                    outline.Append( pt.x, pt.y );
                }
            };

    outline.NewOutline();

    outline.Append( centers[0] + wxPoint( -radius, 0 ) );
    genArc( centers[0], 0, 900 );
    outline.Append( centers[0] + wxPoint( 0, radius ) );
    outline.Append( centers[1] + wxPoint( 0, radius ) );
    genArc( centers[1], 900, 1800 );
    outline.Append( centers[1] + wxPoint( radius, 0 ) );
    outline.Append( centers[2] + wxPoint( radius, 0 ) );
    genArc( centers[2], 1800, 2700 );
    outline.Append( centers[2] + wxPoint( 0, -radius ) );
    outline.Append( centers[3] + wxPoint( 0, -radius ) );
    genArc( centers[3], 2700, 3600 );
    outline.Append( centers[3] + wxPoint( -radius, 0 ) );

    outline.Outline( 0 ).SetClosed( true );

    // The created outlines are bigger than the actual outlines, due to the fact
    // the corner radius is bigger than the initial value when building a shape outside the
    // actual shape.
    // However the bounding box shape does not need to be bigger: only rounded corners must
    // be modified.
    // So clamp the too big shape by the actual bounding box
    if( aErrorLoc == ERROR_OUTSIDE )
    {
        SHAPE_POLY_SET bbox;
        bbox.NewOutline();
        wxSize bbox_size = aSize/2;

        bbox.Append( wxPoint( -bbox_size.x, -bbox_size.y ) );
        bbox.Append( wxPoint( bbox_size.x, -bbox_size.y ) );
        bbox.Append( wxPoint( bbox_size.x, bbox_size.y ) );
        bbox.Append( wxPoint( -bbox_size.x, bbox_size.y ) );
        bbox.Outline( 0 ).SetClosed( true );

        outline.BooleanIntersection( bbox, SHAPE_POLY_SET::PM_FAST );
        // The result is a convex polygon, no need to simplify or fracture.
    }

    // Add the outline:
    aCornerBuffer.Append( outline );
}


void TransformRoundChamferedRectToPolygon( SHAPE_POLY_SET& aCornerBuffer, const wxPoint& aPosition,
                                           const wxSize& aSize, double aRotation,
                                           int aCornerRadius, double aChamferRatio,
                                           int aChamferCorners, int aError, ERROR_LOC aErrorLoc )
{
    SHAPE_POLY_SET outline;
    TransformRoundRectToPolygon( outline, aSize, aCornerRadius, aError, aErrorLoc );

    if( aChamferCorners )
    {
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
                // We recreate a rectangular area covering the full rounded corner
                // (max size = aSize/2) to rebuild the corner before chamfering, to be sure
                // the rounded corner shape does not overlap the chamfered corner shape:
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
    }

    // Rotate and move the outline:
    if( aRotation != 0.0 )
        outline.Rotate( DECIDEG2RAD( -aRotation ), VECTOR2I( 0, 0 ) );

    outline.Move( VECTOR2I( aPosition ) );

    // Add the outline:
    aCornerBuffer.Append( outline );
}


void TransformArcToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aStart, wxPoint aMid,
                            wxPoint aEnd, int aWidth, int aError, ERROR_LOC aErrorLoc )
{
    SHAPE_ARC        arc( aStart, aMid, aEnd, aWidth );
    SHAPE_LINE_CHAIN arcSpine = arc.ConvertToPolyline( aError );
    int              radial_offset = ( aWidth + 1 ) / 2;
    SHAPE_POLY_SET   polyshape;
    std::vector<VECTOR2I> outside_pts;

    /// We start by making rounded ends on the arc
    TransformCircleToPolygon( polyshape,
            wxPoint( arcSpine.GetPoint( 0 ).x, arcSpine.GetPoint( 0 ).y ), radial_offset, aError,
            aErrorLoc );

    TransformCircleToPolygon( polyshape,
            wxPoint( arcSpine.GetPoint( -1 ).x, arcSpine.GetPoint( -1 ).y ), radial_offset, aError,
            aErrorLoc );

    // The circle polygon is built with a even number of segments, so the
    // horizontal diameter has 2 corners on the biggest diameter
    // Rotate these 2 corners to match the start and ens points of inner and outer
    // end points of the arc appoximation outlines, build below.
    // The final shape is much better.
    double arc_angle_end_deg = arc.GetStartAngle();

    if( arc_angle_end_deg != 0 & arc_angle_end_deg != 180.0 )
        polyshape.Outline(0).Rotate( arc_angle_end_deg * M_PI/180.0, arcSpine.GetPoint( 0 ) );

    arc_angle_end_deg = arc.GetEndAngle();

    if( arc_angle_end_deg != 0 & arc_angle_end_deg != 180.0 )
        polyshape.Outline(1).Rotate( arc_angle_end_deg * M_PI/180.0, arcSpine.GetPoint( -1 ) );

    if( aErrorLoc == ERROR_OUTSIDE )
        radial_offset += aError;
    else
        radial_offset -= aError/2;

    if( radial_offset < 0 )
        radial_offset = 0;

    polyshape.NewOutline();

    VECTOR2I center = arc.GetCenter();
    int      radius = ( arc.GetP0() - center ).EuclideanNorm();
    int last_index = arcSpine.GetPointCount() -1;

    for( std::size_t ii = 0; ii <= last_index; ++ii )
    {
        VECTOR2I offset = arcSpine.GetPoint( ii ) - center;
        int curr_rd = radius;

        // This correction gives a better position of intermediate points of the sides of arc.
        if( ii > 0 && ii < last_index )
            curr_rd += aError/2;

        polyshape.Append( offset.Resize( curr_rd - radial_offset ) + center );
        outside_pts.emplace_back( offset.Resize( curr_rd + radial_offset ) + center );
    }

    for( auto it = outside_pts.rbegin(); it != outside_pts.rend(); ++it )
        polyshape.Append( *it );

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
    TransformCircleToPolygon( buffer.Hole( 0, 0 ), aCentre, inner_radius, aError, aErrorLoc );

    buffer.Fracture( SHAPE_POLY_SET::PM_FAST );
    aCornerBuffer.Append( buffer );
}
