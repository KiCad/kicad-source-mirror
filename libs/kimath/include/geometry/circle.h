/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __CIRCLE_H
#define __CIRCLE_H

#include <math/vector2d.h> // for VECTOR2I
#include <vector>          // for std::vector

class SEG;

/**
 * Class Circle
 * Represents basic circle geometry with utility geometry functions.
 */
class CIRCLE
{
public:
    int      Radius; ///< Public to make access simpler
    VECTOR2I Center; ///< Public to make access simpler

    CIRCLE();

    CIRCLE( const VECTOR2I& aCenter, int aRadius );

    CIRCLE( const CIRCLE& aOther );

    /**
     * Constructs this circle such that it is tangent to the given lines and passes through the
     * given point. There are two possible solutions, controlled by aAlternateSolution.
     *
     * When aAlternateSolution is false, find the best solution that can be used to fillet both
     * lines (i.e. choose the most likely quadrant and find the solution with smallest arc angle
     * between the tangent points on the lines)
     *
     * @param aLineA is the first tangent line. Treated as an infinite line except for the purpose
     * of selecting the solution to return.
     * @param aLineB is the second tangent line. Treated as an infinite line except for the purpose
     * of selecting the solution to return.
     * @param aP is the point to pass through
     * @param aAlternateSolution If true, returns the other solution.
     * @return *this
     */
    CIRCLE& ConstructFromTanTanPt( const SEG& aLineA, const SEG& aLineB, const VECTOR2I& aP,
                                   bool aAlternateSolution = false );

    /**
     * Function NearestPoint()
     *
     * Computes the point on the circumference of the circle that is the closest to aP.
     *
     * In other words: finds the intersection point of this circle and a line that passes through
     * both this circle's center and aP.
     *
     * @param aP
     * @return nearest point to aP
     */
    VECTOR2I NearestPoint( const VECTOR2I& aP ) const;

    /**
     * Function Intersect()
     *
     * Computes the intersection points between this circle and aCircle.
     *
     * @param aCircle The other circle to intersect with this.
     * @return std::vector containing:
     *           - 0 elements if the circles do not intersect
     *           - 1 element if the circles are tangent
     *           - 2 elements if the circles intersect
     */
    std::vector<VECTOR2I> Intersect( const CIRCLE& aCircle ) const;

    /**
     * Function Intersect()
     *
     * Computes the intersection points between this circle and aLine.
     *
     * @param aLine The line to intersect with this circle (end points ignored)
     * @return std::vector containing:
     *           - 0 elements if there is no intersection
     *           - 1 element if the line is tangent to the circle
     *           - 2 elements if the line intersects the circle
     */
    std::vector<VECTOR2I> Intersect( const SEG& aLine ) const;
};

#endif // __CIRCLE_H

