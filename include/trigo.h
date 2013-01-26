/**
 * @file trigo.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 KiCad Developers, see change_log.txt for contributors.
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


#ifndef TRIGO_H
#define TRIGO_H


/*
 * Calculate the new point of coord coord pX, pY,
 * for a rotation center 0, 0, and angle in (1 / 10 degree)
 */
void RotatePoint( int *pX, int *pY, double angle );

/*
 * Calculate the new point of coord coord pX, pY,
 * for a rotation center cx, cy, and angle in (1 / 10 degree)
 */
void RotatePoint( int *pX, int *pY, int cx, int cy, double angle );

/*
 * Calculates the new coord point point
 * for a rotation angle in (1 / 10 degree)
 */
static inline void RotatePoint( wxPoint* point, double angle )
{
    RotatePoint( &point->x, &point->y, angle );
}

/*
 * Calculates the new coord point point
 * for a center rotation center and angle in (1 / 10 degree)
 */
void RotatePoint( wxPoint *point, const wxPoint & centre, double angle );

void RotatePoint( double *pX, double *pY, double angle );

void RotatePoint( double *pX, double *pY, double cx, double cy, double angle );

/* Return the arc tangent of 0.1 degrees coord vector dx, dy
 * between -1800 and 1800
 * Equivalent to atan2 (but faster for calculations if
 * the angle is 0 to -1800, or + - 900
 */
int ArcTangente( int dy, int dx );

//! @brief Compute the distance between a line and a reference point
//! Reference: http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html
//! @param linePointA Point on line
//! @param linePointB Point on line
//! @param referencePoint Reference point
double DistanceLinePoint( wxPoint linePointA, wxPoint linePointB, wxPoint referencePoint );

//! @brief Euclidean norm of a 2D vector
//! @param vector Two-dimensional vector
//! @return Euclidean norm of the vector
double EuclideanNorm( wxPoint vector );

//! @brief Test, if two points are near each other
//! @param pointA First point
//! @param pointB Second point
//! @param threshold The maximum distance
//! @return True or false
bool HitTestPoints( wxPoint pointA, wxPoint pointB, double threshold );

//! @brief Determine the cross product
//! @param vectorA Two-dimensional vector
//! @param vectorB Two-dimensional vector
double CrossProduct( wxPoint vectorA, wxPoint vectorB );


/**
 * Function TestSegmentHit
 * test for hit on line segment
 * i.e. a reference point is within a given distance from segment
 * @param aRefPoint = reference point to test
 * @param aStart is the first end-point of the line segment
 * @param aEnd is the second end-point of the line segment
 * @param aDist = maximum distance for hit
*/
bool TestSegmentHit( wxPoint aRefPoint, wxPoint aStart, wxPoint aEnd, int aDist );

/**
 * Function GetLineLength
 * returns the length of a line segment defined by \a aPointA and \a aPointB.
 * @return Length of a line.
 */
double GetLineLength( const wxPoint& aPointA, const wxPoint& aPointB );

#endif
