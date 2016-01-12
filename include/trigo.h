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

/**
 * @file trigo.h
 */

#include <math.h>
#include <wx/gdicmn.h> // For wxPoint

/**
 * Function IsPointOnSegment
 * @param aSegStart The first point of the segment S.
 * @param aSegEnd The second point of the segment S.
 * @param aTestPoint The point P to test.
 * @return true if the point P is on the segment S.
 * faster than TestSegmentHit() because P should be exactly on S
 * therefore works fine only for H, V and 45 deg segm.
 * suitable for busses and wires in eeschema, otherwise use TestSegmentHit()
 */
bool IsPointOnSegment( const wxPoint& aSegStart, const wxPoint& aSegEnd,
                       const wxPoint& aTestPoint );

/**
 * Function SegmentIntersectsSegment
 *
 * @param a_p1_l1 The first point of the first line.
 * @param a_p2_l1 The second point of the first line.
 * @param a_p1_l2 The first point of the second line.
 * @param a_p2_l2 The second point of the second line.
 * @return bool - true if the two segments defined by four points intersect.
 * (i.e. if the 2 segments have at least a common point)
 */
bool SegmentIntersectsSegment( const wxPoint &a_p1_l1, const wxPoint &a_p2_l1,
                               const wxPoint &a_p1_l2, const wxPoint &a_p2_l2 );

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
inline void RotatePoint( wxPoint* point, double angle )
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
 * the angle is 0 to -1800, or + - 900)
 * Lorenzo: In fact usually atan2 already has to do these optimizations
 * (due to the discontinuity in tan) but this function also returns
 * in decidegrees instead of radians, so it's handier
 */
double ArcTangente( int dy, int dx );

//! @brief Euclidean norm of a 2D vector
//! @param vector Two-dimensional vector
//! @return Euclidean norm of the vector
inline double EuclideanNorm( const wxPoint &vector )
{
    // this is working with doubles
    return hypot( vector.x, vector.y );
}

inline double EuclideanNorm( const wxSize &vector )
{
    // this is working with doubles, too
    return hypot( vector.x, vector.y );
}

//! @brief Compute the distance between a line and a reference point
//! Reference: http://mathworld.wolfram.com/Point-LineDistance2-Dimensional.html
//! @param linePointA Point on line
//! @param linePointB Point on line
//! @param referencePoint Reference point
inline double DistanceLinePoint( const wxPoint &linePointA,
                                 const wxPoint &linePointB,
                                 const wxPoint &referencePoint )
{
    // Some of the multiple double casts are redundant. However in the previous
    // definition the cast was (implicitly) done too late, just before
    // the division (EuclideanNorm gives a double so from int it would
    // be promoted); that means that the whole expression were
    // vulnerable to overflow during int multiplications
    return fabs( ( double(linePointB.x - linePointA.x) *
                   double(linePointA.y - referencePoint.y) -
                   double(linePointA.x - referencePoint.x ) *
                   double(linePointB.y - linePointA.y) )
            / EuclideanNorm( linePointB - linePointA ) );
}

//! @brief Test, if two points are near each other
//! @param pointA First point
//! @param pointB Second point
//! @param threshold The maximum distance
//! @return True or false
inline bool HitTestPoints( const wxPoint &pointA, const wxPoint &pointB,
                           double threshold )
{
    wxPoint vectorAB = pointB - pointA;

    // Compare the distances squared. The double is needed to avoid
    // overflow during int multiplication
    double sqdistance = (double)vectorAB.x * vectorAB.x +
                        (double)vectorAB.y * vectorAB.y;

    return sqdistance < threshold * threshold;
}

//! @brief Determine the cross product
//! @param vectorA Two-dimensional vector
//! @param vectorB Two-dimensional vector
inline double CrossProduct( const wxPoint &vectorA, const wxPoint &vectorB )
{
    // As before the cast is to avoid int overflow
    return (double)vectorA.x * vectorB.y - (double)vectorA.y * vectorB.x;
}

/**
 * Function TestSegmentHit
 * test for hit on line segment
 * i.e. a reference point is within a given distance from segment
 * @param aRefPoint = reference point to test
 * @param aStart is the first end-point of the line segment
 * @param aEnd is the second end-point of the line segment
 * @param aDist = maximum distance for hit
*/
bool TestSegmentHit( const wxPoint &aRefPoint, wxPoint aStart,
                     wxPoint aEnd, int aDist );

/**
 * Function GetLineLength
 * returns the length of a line segment defined by \a aPointA and \a aPointB.
 * See also EuclideanNorm and Distance for the single vector or four
 * scalar versions
 * @return Length of a line (as double)
 */
inline double GetLineLength( const wxPoint& aPointA, const wxPoint& aPointB )
{
    // Implicitly casted to double
    return hypot( aPointA.x - aPointB.x,
                  aPointA.y - aPointB.y );
}

// These are the usual degrees <-> radians conversion routines
inline double DEG2RAD( double deg ) { return deg * M_PI / 180.0; }
inline double RAD2DEG( double rad ) { return rad * 180.0 / M_PI; }

// These are the same *but* work with the internal 'decidegrees' unit
inline double DECIDEG2RAD( double deg ) { return deg * M_PI / 1800.0; }
inline double RAD2DECIDEG( double rad ) { return rad * 1800.0 / M_PI; }

/* These are templated over T (and not simply double) because eeschema
   is still using int for angles in some place */

/// Normalize angle to be in the -360.0 .. 360.0:
template <class T> inline void NORMALIZE_ANGLE_360( T &Angle )
{
    while( Angle < -3600 )
        Angle += 3600;
    while( Angle > 3600 )
        Angle -= 3600;
}

/// Normalize angle to be in the 0.0 .. 360.0 range:
template <class T> inline void NORMALIZE_ANGLE_POS( T &Angle )
{
    while( Angle < 0 )
        Angle += 3600;
    while( Angle >= 3600 )
        Angle -= 3600;
}

/// Add two angles (keeping the result normalized). T2 is here
// because most of the time it's an int (and templates don't promote in
// that way)
template <class T, class T2> inline T AddAngles( T a1, T2 a2 )
{
    a1 += a2;
    NORMALIZE_ANGLE_POS( a1 );
    return a1;
}

template <class T> inline void NEGATE_AND_NORMALIZE_ANGLE_POS( T &Angle )
{
    Angle = -Angle;
    while( Angle < 0 )
        Angle += 3600;
    while( Angle >= 3600 )
        Angle -= 3600;
}

/// Normalize angle to be in the -90.0 .. 90.0 range
template <class T> inline void NORMALIZE_ANGLE_90( T &Angle )
{
    while( Angle < -900 )
        Angle += 1800;
    while( Angle > 900 )
        Angle -= 1800;
}

/// Normalize angle to be in the -180.0 .. 180.0 range
template <class T> inline void NORMALIZE_ANGLE_180( T &Angle )
{
    while( Angle <= -1800 )
        Angle += 3600;
    while( Angle > 1800 )
        Angle -= 3600;
}

/**
 * Circle generation utility: computes r * sin(a)
 * Where a is in decidegrees, not in radians.
 */
inline double sindecideg( double r, double a )
{
    return r * sin( DECIDEG2RAD( a ) );
}

/**
 * Circle generation utility: computes r * cos(a)
 * Where a is in decidegrees, not in radians.
 */
inline double cosdecideg( double r, double a )
{
    return r * cos( DECIDEG2RAD( a ) );
}

#endif
