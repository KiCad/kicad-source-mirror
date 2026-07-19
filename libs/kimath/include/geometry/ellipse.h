/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_ELLIPSE_H
#define KICAD_ELLIPSE_H

#include <math/vector2d.h>
#include <geometry/eda_angle.h>
#include <core/mirror.h>

/// Plain ellipse / elliptical-arc data. Used by SHAPE_ELLIPSE, by EDA_SHAPE's
/// native ellipse storage, and by importers that need to round-trip ellipses.

template <typename NumericType>
class ELLIPSE
{
public:
    ELLIPSE() :
            MajorRadius( 0 ),
            MinorRadius( 0 )
    {
    }

    /**
     * Constructs an ellipse or elliptical arc.  The ellipse sweeps from aStartAngle to aEndAngle
     * in a counter-clockwise direction.
     *
     * @param aCenter is the center point of the ellipse.
     * @param aMajorRadius is the radius of the "x-axis" dimension of the ellipse.
     * @param aMinorRadius is the radius of the "y-axis" dimension of the ellipse.
     * @param aRotation is the angle of the ellipse "x-axis" relative to world x-axis.
     * @param aStartAngle is the starting angle of the elliptical arc.
     * @param aEndAngle is the ending angle of the elliptical arc.
     */
    ELLIPSE( const VECTOR2<NumericType>& aCenter, NumericType aMajorRadius,
             NumericType aMinorRadius, EDA_ANGLE aRotation, EDA_ANGLE aStartAngle = ANGLE_0,
             EDA_ANGLE aEndAngle = FULL_CIRCLE );

    /**
     * Constructs a DXF-style ellipse or elliptical arc, where the major axis is given by a point
     * rather than a radius, and therefore defines not only the major radius but also the rotation
     * of the ellipse.
     *
     * @param aCenter is the center point of the ellipse.
     * @param aMajor is the endpoint of the major axis, relative to the center.
     * @param aRatio is the ratio of the minor axis length to the major axis length.
     * @param aStartAngle is the starting angle of the elliptical arc.
     * @param aEndAngle is the ending angle of the elliptical arc.
     */
    ELLIPSE( const VECTOR2<NumericType>& aCenter, const VECTOR2<NumericType>& aMajor, double aRatio,
             EDA_ANGLE aStartAngle = ANGLE_0, EDA_ANGLE aEndAngle = FULL_CIRCLE );

    /**
     * Mirror the ellipse along a horizontal or vertical axis passing through aRef.
     */
    void Mirror( const VECTOR2<NumericType>& aRef, FLIP_DIRECTION aFlipDirection );

    /**
     * Get the subtended angle of the ellipse or elliptical arc at the center.
     */
    EDA_ANGLE GetSubtendedAngle() const;

    /**
     * Get the point on the ellipse at a given angle.  The angle is measured in the ellipse's
     * local coordinate system, where the x-axis is the major axis and the y-axis is the minor axis.
     */
    VECTOR2<NumericType> GetPointAtAngle( EDA_ANGLE angle ) const;

    /**
     * Get the point on the ellipse at the start angle of the arc.
     */
    VECTOR2<NumericType> GetArcStartPoint() const { return GetPointAtAngle( StartAngle ); }

    /**
     * Get the point on the ellipse at the end angle of the arc.
     */
    VECTOR2<NumericType> GetArcEndPoint() const { return GetPointAtAngle( EndAngle ); }

    /**
     * Get the parametric angle of a point on the ellipse.  The angle is measured in the ellipse's
     * local coordinate system, where the x-axis is the major axis and the y-axis is the minor axis.
     */
    EDA_ANGLE GetAngleAtPoint( const VECTOR2<NumericType>& aPt ) const;

    VECTOR2<NumericType> Center;
    NumericType MajorRadius;
    NumericType MinorRadius;
    EDA_ANGLE Rotation;
    EDA_ANGLE StartAngle;
    EDA_ANGLE EndAngle;

};

#endif //KICAD_ELLIPSE_H
