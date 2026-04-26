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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRID_GEOMETRY_H
#define GRID_GEOMETRY_H

#include <math/vector2d.h>


/**
 * Geometry of a regular grid (cartesian or polar) and the math operations for
 * snapping and coverage testing.
 */
struct GRID_GEOMETRY
{
    enum class KIND
    {
        CARTESIAN,
        POLAR
    };

    KIND     kind = KIND::CARTESIAN;
    VECTOR2D origin;
    VECTOR2D pitch;             ///< Cartesian: (dx, dy); polar: (dr, dPhi rad).
    double   orientation = 0.0; ///< Rotation about origin, radians CCW.
    VECTOR2D extent;            ///< Cartesian: (dx, dy) = size/2 from origin.
                                ///< Polar: (rMax, phiMax rad).
    unsigned priority = 0;      ///< Higher wins where grids overlap.  0 is reserved for
                                ///< the background grid; see TakesPrecedenceOver.

    /**
     * Snap a point to the nearest on-grid position.
     *
     * @return aPoint unchanged for degenerate geometry (non-positive pitch).
     */
    VECTOR2D Snap( const VECTOR2D& aPoint ) const;

    /**
     * @return true if aPoint is within aTolerance of the grid's coverage.
     */
    bool Contains( const VECTOR2D& aPoint, double aTolerance = 0.0 ) const;

    /**
     * @return the area of the grid's coverage region.  Used as a tiebreaker between
     *         overlapping grids of equal priority - smaller wins (more specific).
     */
    double Area() const;

    /**
     * @return true if this grid owns a point that both grids cover: higher priority
     *         first, then the smaller (more specific) area.
     *
     * The Compare for std::stable_sort - the winner sorts FIRST, ties keep document
     * order.  The background grid is the only one at priority 0, so it always sorts last.
     */
    bool TakesPrecedenceOver( const GRID_GEOMETRY& aOther ) const
    {
        if( priority != aOther.priority )
            return priority > aOther.priority;

        return Area() < aOther.Area();
    }
};


#endif
