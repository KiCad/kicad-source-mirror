/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see change_log.txt for contributors.
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

#ifndef CONVERT_BASIC_SHAPES_TO_POLYGON_H
#define CONVERT_BASIC_SHAPES_TO_POLYGON_H

#include <geometry/shape_poly_set.h>
#include <geometry/geometry_utils.h>
#include <wx/gdicmn.h>      // for wxPoint


// The chamfer positions of chamfered rect shape.
// the position is relative to a pad with orientation = 0
// we can have 1 to 4 chamfered corners (0 corner = roundrect)
// The position list is the OR of corner to chamfer
enum RECT_CHAMFER_POSITIONS : int
{
    RECT_NO_CHAMFER           = 0,
    RECT_CHAMFER_TOP_LEFT     = 1,
    RECT_CHAMFER_TOP_RIGHT    = 2,
    RECT_CHAMFER_BOTTOM_LEFT  = 4,
    RECT_CHAMFER_BOTTOM_RIGHT = 8,
    RECT_CHAMFER_ALL = RECT_CHAMFER_BOTTOM_RIGHT
                     | RECT_CHAMFER_BOTTOM_LEFT
                     | RECT_CHAMFER_TOP_RIGHT
                     | RECT_CHAMFER_TOP_LEFT
};


/**
 * Function TransformCircleToPolygon
 * convert a circle to a polygon, using multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCenter = the center of the circle
 * @param aRadius = the radius of the circle
 * @param aError = the IU allowed for error in approximation
 * @param aErrorLoc = should the approximation error be placed outside or inside the polygon?
 */
void TransformCircleToPolygon( SHAPE_LINE_CHAIN& aCornerBuffer, wxPoint aCenter, int aRadius,
                               int aError, ERROR_LOC aErrorLoc );

/**
 * Function TransformCircleToPolygon
 * convert a circle to a polygon, using multiple straight lines
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCenter = the center of the circle
 * @param aRadius = the radius of the circle
 * @param aError = the IU allowed for error in approximation
 * @param aErrorLoc = should the approximation error be placed outside or inside the polygon?
 */
void TransformCircleToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aCenter, int aRadius,
                               int aError, ERROR_LOC aErrorLoc );


/**
 * convert a oblong shape to a polygon, using multiple segments
 * It is similar to TransformRoundedEndsSegmentToPolygon, but the polygon
 * is outside the actual oblong shape (a segment with rounded ends)
 * It is suitable to create oblong clearance areas.
 * because multiple segments create a smaller area than the circle, the
 * radius of the circle to approximate must be bigger ( radius*aCorrectionFactor)
 * to create segments outside the circle.
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aStart = the first point of the segment
 * @param aEnd = the second point of the segment
 * @param aWidth = the width of the segment
 * @param aError = the IU allowed for error in approximation
 * @param aErrorLoc = should the approximation error be placed outside or inside the polygon?
 */
void TransformOvalToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aStart, wxPoint aEnd,
                             int aWidth, int aError, ERROR_LOC aErrorLoc );


/**
 * convert a rectangle with rounded corners and/or chamfered corners to a polygon
 * Convert rounded corners arcs to multiple straight lines.  This will generate at least
 * 16 segments per circle
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aPosition = the coordinate of the center of the rectangle
 * @param aSize = the size of the rectangle
 * @param aCornerRadius = radius of rounded corners (can be 0)
 * @param aRotation = rotation in 0.1 degrees of the rectangle
 * @param aChamferRatio = ratio between smaller rect size and chamfer value
 * @param aChamferCorners = identifier of the corners to chamfer:
 *  0 = no chamfer
 *  1 = TOP_LEFT
 *  2 = TOP_RIGHT
 *  4 = BOTTOM_LEFT
 *  8 = BOTTOM_RIGHT
 * One can have more than one chamfered corner by ORing the corner identifers
 * @param aError = the IU allowed for error in approximation
 * @param aErrorLoc = should the approximation error be placed outside or inside the polygon?
 */
void TransformRoundChamferedRectToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                           const wxPoint& aPosition, const wxSize& aSize,
                                           double aRotation, int aCornerRadius,
                                           double aChamferRatio, int aChamferCorners,
                                           int aError, ERROR_LOC aErrorLoc );

/**
 * Function TransformArcToPolygon
 * Creates a polygon from an Arc
 * Convert arcs to multiple straight segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCentre = centre of the arc or circle
 * @param aStart = start point of the arc, or a point on the circle
 * @param aArcAngle = arc angle in 0.1 degrees. For a circle, aArcAngle = 3600
 * @param aWidth = width (thickness) of the line
 * @param aError = the IU allowed for error in approximation
 * @param aErrorLoc = should the approximation error be placed outside or inside the polygon?
 */
void TransformArcToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aStart, wxPoint aMid,
                            wxPoint aEnd, int aWidth, int aError, ERROR_LOC aErrorLoc );

/**
 * Function TransformRingToPolygon
 * Creates a polygon from a ring
 * Convert arcs to multiple straight segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aCentre = centre of the arc or circle
 * @param aRadius = radius of the circle
 * @param aWidth = width (thickness) of the ring
 * @param aError = the IU allowed for error in approximation
 * @param aErrorLoc = should the approximation error be placed outside or inside the polygon?
 */
void TransformRingToPolygon( SHAPE_POLY_SET& aCornerBuffer, wxPoint aCentre, int aRadius,
                             int aWidth, int aError, ERROR_LOC aErrorLoc );

#endif     // CONVERT_BASIC_SHAPES_TO_POLYGON_H
