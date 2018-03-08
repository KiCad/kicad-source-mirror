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

/**
 * @file geometry_utils.cpp
 * @brief a few functions useful in geometry calculations.
 */

#include <math.h>
#include <common.h>
#include <geometry/geometry_utils.h>


int GetArcToSegmentCount( int aRadius, int aErrorMax, double aArcAngleDegree )
{
    // calculate the number of segments to approximate a circle by segments
    // given the max distance between the middle of a segment and the circle

    // error relative to the radius value:
    double rel_error = (double)aErrorMax / aRadius;
    // minimal arc increment in degrees:
    double step = 180 / M_PI * acos( 1.0 - rel_error ) * 2;
    // the minimal seg count for a arc
    int segCount = KiROUND( fabs( aArcAngleDegree ) / step );

    // Ensure at least one segment is used
    return std::max( segCount, 1 );
}


double GetCircletoPolyCorrectionFactor( int aSegCountforCircle )
{
    /* calculates the coeff to compensate radius reduction of circle
     * due to the segment approx.
     * For a circle the min radius is radius * cos( 2PI / aSegCountforCircle / 2)
     * this is the distance between the center and the middle of the segment.
     * therfore, to move the  middle of the segment to the circle (distance = radius)
     * the correctionFactor is 1 /cos( PI/aSegCountforCircle  )
     */
    double correctionFactor = 1.0 / cos( M_PI / aSegCountforCircle );

    return correctionFactor;
}

