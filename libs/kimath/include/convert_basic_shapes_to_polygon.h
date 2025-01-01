/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef CONVERT_BASIC_SHAPES_TO_POLYGON_H
#define CONVERT_BASIC_SHAPES_TO_POLYGON_H

#include <geometry/approximation.h>
#include <geometry/shape_poly_set.h>


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
 * Generate a polyline to approximate a arc
 *
 * @param aPolyline is a buffer to store the polyline.
 * @param aCenter is the center of the arc.
 * @param aRadius is the radius of the arc.
 * @param aStartAngleDeg is the starting point of the arc.
 * @param aArcAngleDeg is the angle of the arc.
 * @param aError is the internal units allowed for error approximation.
 * @param aErrorLoc determines if the approximation error be placed outside or inside the polygon.
 */
int ConvertArcToPolyline( SHAPE_LINE_CHAIN& aPolyline, VECTOR2I aCenter, int aRadius,
                          const EDA_ANGLE& aStartAngleDeg, const EDA_ANGLE& aArcAngleDeg,
                          double aAccuracy, ERROR_LOC aErrorLoc );


/**
 * Convert a circle to a polygon, using multiple straight lines.
 *
 * @param aBuffer is a buffer to store the polygon.
 * @param aCenter is the center of the circle.
 * @param aRadius is the radius of the circle.
 * @param aError is the internal units allowed for error approximation.
 * @param aErrorLoc determines if the approximation error be placed outside or inside the polygon.
 * @param aMinSegCount is the min count of segments to approximate.
 * Default = 0 to do not force a min count.
 */
void TransformCircleToPolygon( SHAPE_LINE_CHAIN& aBuffer, const VECTOR2I& aCenter, int aRadius,
                               int aError, ERROR_LOC aErrorLoc, int aMinSegCount = 0 );

/**
 * Convert a circle to a polygon, using multiple straight lines.
 *
 * @param aBuffer is a buffer to store the polygon.
 * @param aCenter is the center of the circle.
 * @param aRadius is the radius of the circle.
 * @param aError is the internal units allowed for error in approximation.
 * @param aErrorLoc determines if the approximation error be placed outside or inside the polygon.
 * @param aMinSegCount is the min count of segments to approximate.
 * Default = 0 to do not force a min count.
 */
void TransformCircleToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aCenter, int aRadius,
                               int aError, ERROR_LOC aErrorLoc, int aMinSegCount = 0 );


/**
 * Convert a oblong shape to a polygon, using multiple segments.
 *
 * It is similar to TransformRoundedEndsSegmentToPolygon, but the polygon is outside the actual
 * oblong shape (a segment with rounded ends).  It is suitable to create oblong clearance areas
 * because multiple segments create a smaller area than the circle.  The radius of the circle to
 * approximate must be bigger ( radius*aCorrectionFactor) to create segments outside the circle.
 *
 * @param aBuffer is a buffer to store the polygon.
 * @param aStart is the first point of the segment.
 * @param aEnd is the second point of the segment.
 * @param aWidth is the width of the segment.
 * @param aError is the internal units allowed for error in approximation.
 * @param aErrorLoc determines if the approximation error be placed outside or inside the polygon.
 * @param aMinSegCount is the min count of segments to approximate.
 * Default = 0 to do not force a min count.
 */
void TransformOvalToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aStart, const VECTOR2I& aEnd,
                             int aWidth, int aError, ERROR_LOC aErrorLoc, int aMinSegCount = 0 );

/**
 * Convert a rectangle or trapezoid to a polygon.
 *
 * This will generate at least 16 segments per circle (when using inflate).
 *
 * @param aBuffer is a buffer to store the polygon.
 * @param aPosition is the coordinate of the center of the rectangle.
 * @param aSize is the size of the rectangle.
 * @param aDeltaX is the delta for trapezoids in X direction
 * @param aDeltaY is the delta for trapezoids in Y direction
 * @param aInflate is the (positive) shape inflation or 0
 * @param aError is the IU allowed for error in approximation.
 * @param aErrorLoc determines if the approximation error be placed outside or inside the polygon.
 */
void TransformTrapezoidToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aPosition,
                                  const VECTOR2I& aSize, const EDA_ANGLE& aRotation, int aDeltaX,
                                  int aDeltaY, int aInflate, int aError, ERROR_LOC aErrorLoc );

/**
 * Convert a rectangle with rounded corners and/or chamfered corners to a polygon.
 *
 * Convert rounded corners arcs to multiple straight lines. This will generate at least
 * 16 segments per circle.
 *
 * @param aBuffer is a buffer to store the polygon.
 * @param aPosition is the coordinate of the center of the rectangle.
 * @param aSize is the size of the rectangle.
 * @param aCornerRadius is the radius of rounded corners (can be 0).
 * @param aRotation is the rotationof the rectangle.
 * @param aChamferRatio is the ratio between smaller rect side and chamfer value.
 * @param aChamferCorners is the identifier of the corners to chamfer:
 *  - 0 = no chamfer
 *  - 1 = TOP_LEFT
 *  - 2 = TOP_RIGHT
 *  - 4 = BOTTOM_LEFT
 *  - 8 = BOTTOM_RIGHT
 * One can have more than one chamfered corner by ORing the corner identifiers.
 * @param aInflate is the (positive) shape inflation or 0
 * @param aError is the IU allowed for error in approximation.
 * @param aErrorLoc determines if the approximation error be placed outside or inside the polygon.
 */
void TransformRoundChamferedRectToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aPosition,
                                           const VECTOR2I& aSize, const EDA_ANGLE& aRotation,
                                           int aCornerRadius, double aChamferRatio,
                                           int aChamferCorners, int aInflate, int aError,
                                           ERROR_LOC aErrorLoc );

/**
 * Convert arc to multiple straight segments.
 *
 * @param aBuffer is a buffer to store the polygon.
 * @param aCentre is the center of the arc or circle.
 * @param aStart is the start point of the arc or a point on the circle.
 * @param aArcAngle is the arc angle in 0.1 degrees. For a circle, aArcAngle = 3600.
 * @param aWidth is the width (thickness) of the line.
 * @param aError is the internal units allowed for error in approximation.
 * @param aErrorLoc determines if the approximation error be placed outside or inside the polygon.
 */
void TransformArcToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aStart, const VECTOR2I& aMid,
                            const VECTOR2I& aEnd, int aWidth, int aError, ERROR_LOC aErrorLoc );

/**
 * Convert arcs to multiple straight segments.
 *
 * @param aBuffer is a buffer to store the polygon.
 * @param aCentre is the center of the arc or circle.
 * @param aRadius is the radius of the circle.
 * @param aWidth is the width (thickness) of the ring.
 * @param aError is the internal units allowed for error in approximation.
 * @param aErrorLoc determines if the approximation error be placed outside or inside the polygon.
 */
void TransformRingToPolygon( SHAPE_POLY_SET& aBuffer, const VECTOR2I& aCentre, int aRadius,
                             int aWidth, int aError, ERROR_LOC aErrorLoc );

#endif     // CONVERT_BASIC_SHAPES_TO_POLYGON_H
