/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

#include <geometry/circle.h>
#include <geometry/seg.h>
#include <geometry/shape.h>     // for MIN_PRECISION_IU
#include <math/util.h>          // for KiROUND
#include <math/vector2d.h>      // for VECTOR2I
#include <math.h>               // for sqrt
#include <trigo.h>              // for CalcArcMid


CIRCLE::CIRCLE()
{
    Center = { 0, 0 };
    Radius = 0;
}


CIRCLE::CIRCLE( const VECTOR2I& aCenter, int aRadius )
{
    Center = aCenter;
    Radius = aRadius;
}


CIRCLE::CIRCLE( const CIRCLE& aOther )
{
    Center = aOther.Center;
    Radius = aOther.Radius;
}


CIRCLE& CIRCLE::ConstructFromTanTanPt( const SEG& aLineA, const SEG& aLineB, const VECTOR2I& aP )
{
    //fixme: There might be more efficient / accurate solution than using geometrical constructs

    SEG anglebisector;
    VECTOR2I intersectPoint;

    auto furthestFromIntersect =
        [&]( VECTOR2I aPt1, VECTOR2I aPt2 ) -> VECTOR2I
        {
            if( ( aPt1 - intersectPoint ).EuclideanNorm()
                > ( aPt2 - intersectPoint ).EuclideanNorm() )
            {
                return aPt1;
            }
            else
            {
                return aPt2;
            }
        };

    auto closestToIntersect =
        [&]( VECTOR2I aPt1, VECTOR2I aPt2 ) -> VECTOR2I
        {
            if( ( aPt1 - intersectPoint ).EuclideanNorm()
                <= ( aPt2 - intersectPoint ).EuclideanNorm() )
            {
                return aPt1;
            }
            else
            {
                return aPt2;
            }
        };

    if( aLineA.ApproxParallel( aLineB ) )
    {
        // Special case, no intersection point between the two lines
        // The center will be in the line equidistant between the two given lines
        // The radius will be half the distance between the two lines
        // The possible centers can be found by intersection

        SEG      perpendicularAtoB( aLineA.A, aLineB.LineProject( aLineA.A ) );
        VECTOR2I midPt = perpendicularAtoB.Center();
        Radius = ( midPt - aLineA.A ).EuclideanNorm();

        anglebisector = aLineA.ParallelSeg( midPt );

        Center = aP; // use this circle as a construction to find the actual centers
        std::vector<VECTOR2I> possibleCenters = IntersectLine( anglebisector );

        wxCHECK_MSG( possibleCenters.size() > 0, *this, wxT( "No solutions exist!" ) );
        intersectPoint = aLineA.A; // just for the purpose of deciding which solution to return

        // For the special case of the two segments being parallel, we will return the solution
        // whose center is closest to aLineA.A
        Center = closestToIntersect( possibleCenters.front(), possibleCenters.back() );
    }
    else
    {
        // General case, using homothety.
        // All circles inscribed in the same angle are homothetic with center at the intersection
        // In this code, the prefix "h" denotes "the homothetic image"
        OPT_VECTOR2I intersectCalc = aLineA.IntersectLines( aLineB );
        wxCHECK_MSG( intersectCalc, *this, wxT( "Lines do not intersect but are not parallel?" ) );
        intersectPoint = *intersectCalc;

        if( aP == intersectPoint )
        {
            //Special case: The point is at the intersection of the two lines
            Center = aP;
            Radius = 0;
            return *this;
        }

        // Calculate bisector
        VECTOR2I lineApt = furthestFromIntersect( aLineA.A, aLineA.B );
        VECTOR2I lineBpt = furthestFromIntersect( aLineB.A, aLineB.B );
        VECTOR2I bisectorPt = CalcArcMid( lineApt, lineBpt, intersectPoint, true );

        anglebisector.A = intersectPoint;
        anglebisector.B = bisectorPt;

        // Create an arbitrary circle that is tangent to both lines
        CIRCLE hSolution;
        hSolution.Center = anglebisector.LineProject( aP );
        hSolution.Radius = aLineA.LineDistance( hSolution.Center );

        // Find the homothetic image of aP in the construction circle (hSolution)
        SEG                   throughaP( intersectPoint, aP );
        std::vector<VECTOR2I> hProjections = hSolution.IntersectLine( throughaP );
        wxCHECK_MSG( hProjections.size() > 0, *this, wxT( "No solutions exist!" ) );

        // We want to create a fillet, so the projection of homothetic projection of aP
        // should be the one closest to the intersection
        VECTOR2I hSelected = closestToIntersect( hProjections.front(), hProjections.back() );

        VECTOR2I hTanLineA = aLineA.LineProject( hSolution.Center );
        VECTOR2I hTanLineB = aLineB.LineProject( hSolution.Center );

        // To minimise errors, use the furthest away tangent point from aP
        if( ( hTanLineA - aP ).SquaredEuclideanNorm() > ( hTanLineB - aP ).SquaredEuclideanNorm() )
        {
            // Find the tangent at line A by homothetic inversion
            SEG          hT( hTanLineA, hSelected );
            OPT_VECTOR2I actTanA = hT.ParallelSeg( aP ).IntersectLines( aLineA );
            wxCHECK_MSG( actTanA, *this, wxT( "No solutions exist!" ) );

            // Find circle center by perpendicular intersection with the angle bisector
            SEG          perpendicularToTanA = aLineA.PerpendicularSeg( *actTanA );
            OPT_VECTOR2I actCenter = perpendicularToTanA.IntersectLines( anglebisector );
            wxCHECK_MSG( actCenter, *this, wxT( "No solutions exist!" ) );

            Center = *actCenter;
            Radius = aLineA.LineDistance( Center );
        }
        else
        {
            // Find the tangent at line B by inversion
            SEG          hT( hTanLineB, hSelected );
            OPT_VECTOR2I actTanB = hT.ParallelSeg( aP ).IntersectLines( aLineB );
            wxCHECK_MSG( actTanB, *this, wxT( "No solutions exist!" ) );

            // Find circle center by perpendicular intersection with the angle bisector
            SEG          perpendicularToTanB = aLineB.PerpendicularSeg( *actTanB );
            OPT_VECTOR2I actCenter = perpendicularToTanB.IntersectLines( anglebisector );
            wxCHECK_MSG( actCenter, *this, wxT( "No solutions exist!" ) );

            Center = *actCenter;
            Radius = aLineB.LineDistance( Center );
        }
    }

    return *this;
}


bool CIRCLE::Contains( const VECTOR2I& aP ) const
{
    int64_t distance = ( VECTOR2L( aP ) - Center ).EuclideanNorm();

    return distance <= ( (int64_t) Radius + SHAPE::MIN_PRECISION_IU )
           && distance >= ( (int64_t) Radius - SHAPE::MIN_PRECISION_IU );
}


VECTOR2I CIRCLE::NearestPoint( const VECTOR2I& aP ) const
{
    VECTOR2I vec = aP - Center;

    // Handle special case where aP is equal to this circle's center
    if( vec.x == 0 && vec.y == 0 )
        vec.x = 1; // Arbitrary, to ensure the return value is always on the circumference

    return vec.Resize( Radius ) + Center;
}


VECTOR2D CIRCLE::NearestPoint( const VECTOR2D& aP ) const
{
    VECTOR2D vec = aP - Center;

    // Handle special case where aP is equal to this circle's center
    if( vec.x == 0 && vec.y == 0 )
        vec.x = 1; // Arbitrary, to ensure the return value is always on the circumference

    return vec.Resize( Radius ) + Center;
}


std::vector<VECTOR2I> CIRCLE::Intersect( const CIRCLE& aCircle ) const
{
    // From https://mathworld.wolfram.com/Circle-CircleIntersection.html
    //
    // Simplify the problem:
    // Let this circle be centered at (0,0), with radius r1
    // Let aCircle be centered at (d, 0), with radius r2
    // (i.e. d is the distance between the two circle centers)
    //
    // The equations of the two circles are
    // (1)   x^2 + y^2 = r1^2
    // (2)   (x - d)^2 + y^2 = r2^2
    //
    // Combining (1) into (2):
    //       (x - d)^2 + r1^2 - x^2 = r2^2
    // Expanding:
    //       x^2 - 2*d*x + d^2 + r1^2 - x^2 = r2^2
    // Rearranging for x:
    // (3)   x = (d^2 + r1^2 - r2^2) / (2 * d)
    //
    // Rearranging (1) gives:
    // (4)   y = sqrt(r1^2 - x^2)

    std::vector<VECTOR2I> retval;

    VECTOR2L vecCtoC = VECTOR2L( aCircle.Center ) - Center;
    int64_t  d = vecCtoC.EuclideanNorm();
    int64_t  r1 = Radius;
    int64_t  r2 = aCircle.Radius;

    if( d > ( r1 + r2 ) || ( d < ( std::abs( r1 - r2 ) ) ) )
        return retval; //circles do not intersect

    if( d == 0 )
        return retval; // circles are co-centered. Don't return intersection points

    // Equation (3)
    int64_t x = ( ( d * d ) + ( r1 * r1 ) - ( r2 * r2 ) ) / ( int64_t( 2 ) * d );
    int64_t r1sqMinusXsq = ( r1 * r1 ) - ( x * x );

    if( r1sqMinusXsq < 0 )
        return retval; //circles do not intersect

    // Equation (4)
    int64_t y = KiROUND( sqrt( r1sqMinusXsq ) );

    // Now correct back to original coordinates
    EDA_ANGLE rotAngle( vecCtoC );
    VECTOR2I  solution1( x, y );
    RotatePoint( solution1, -rotAngle );
    solution1 += Center;
    retval.push_back( solution1 );

    if( y != 0 )
    {
        VECTOR2I solution2( x, -y );
        RotatePoint( solution2, -rotAngle );
        solution2 += Center;
        retval.push_back( solution2 );
    }

    return retval;
}


std::vector<VECTOR2I> CIRCLE::Intersect( const SEG& aSeg ) const
{
    std::vector<VECTOR2I> retval;

    for( VECTOR2I& intersection : IntersectLine( aSeg ) )
    {
        if( aSeg.Contains( intersection ) )
            retval.push_back( intersection );
    }

    return retval;
}


std::vector<VECTOR2I> CIRCLE::IntersectLine( const SEG& aLine ) const
{
    std::vector<VECTOR2I> retval;

    //
    //           .   *   .
    //         *           *
    //  -----1-------m-------2----
    //      *                 *
    //     *         O         *
    //     *                   *
    //      *                 *
    //       *               *
    //         *           *
    //           '   *   '
    // Let O be the center of this circle, 1 and 2 the intersection points of the line
    // and M be the center of the chord connecting points 1 and 2
    //
    // M will be O projected perpendicularly to the line since a chord is always perpendicular
    // to the radius.
    //
    // The distance M1 = M2 can be computed by pythagoras since O1 = O2 = Radius
    //
    // M1= M2 = sqrt( Radius^2 - OM^2)
    //

    VECTOR2I m = aLine.LineProject( Center );    // O projected perpendicularly to the line
    int64_t  omDist = ( VECTOR2L( m ) - Center ).EuclideanNorm();

    if( omDist > ( (int64_t) Radius + SHAPE::MIN_PRECISION_IU ) )
    {
        return retval; // does not intersect
    }
    else if( omDist <= ( (int64_t) Radius + SHAPE::MIN_PRECISION_IU )
             && omDist >= ( (int64_t) Radius - SHAPE::MIN_PRECISION_IU ) )
    {
        retval.push_back( m );
        return retval; //tangent
    }

    int64_t radiusSquared = (int64_t) Radius * (int64_t) Radius;
    int64_t omDistSquared = omDist * omDist;

    int mTo1dist = sqrt( radiusSquared - omDistSquared );

    VECTOR2I mTo1vec = ( aLine.B - aLine.A ).Resize( mTo1dist );
    VECTOR2I mTo2vec = -mTo1vec;

    retval.push_back( mTo1vec + m );
    retval.push_back( mTo2vec + m );

    return retval;
}


bool CIRCLE::Contains( const VECTOR2I& aP )
{
    return ( aP - Center ).EuclideanNorm() < Radius;
}
