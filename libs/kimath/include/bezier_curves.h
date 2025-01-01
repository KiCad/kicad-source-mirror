/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef BEZIER_CURVES_H
#define BEZIER_CURVES_H

#include <vector>
#include <math/vector2d.h>

template <typename T> class ELLIPSE;

/**
 * Bezier curves to polygon converter.
 *
 * Only quadratic and cubic Bezier curves are handled
 */
class BEZIER_POLY
{
public:
    BEZIER_POLY( const VECTOR2I& aStart, const VECTOR2I& aCtrl1,
                 const VECTOR2I& aCtrl2, const VECTOR2I& aEnd );

    BEZIER_POLY( const std::vector<VECTOR2I>& aControlPoints );

    BEZIER_POLY( const std::vector<VECTOR2D>& aControlPoints )
        : m_ctrlPts( aControlPoints )
    {
        m_minSegLen = 0.0;
    }

    /**
     * Convert a Bezier curve to a polygon.
     *
     * @param aOutput will be used as an output vector storing polygon points.
     * @param aMaxError maximum error in IU between the curve and the polygon.
     */
    void GetPoly( std::vector<VECTOR2I>& aOutput, int aMaxError = 10 );
    void GetPoly( std::vector<VECTOR2D>& aOutput, double aMaxError = 10.0 );

private:

    void getQuadPoly( std::vector<VECTOR2D>& aOutput, double aMaxError );
    void getCubicPoly( std::vector<VECTOR2D>& aOutput, double aMaxError );

    int findInflectionPoints( double& aT1, double& aT2 );
    int numberOfInflectionPoints();

    double thirdControlPointDeviation();

    void subdivide( double aT, BEZIER_POLY& aLeft, BEZIER_POLY& aRight );
    void recursiveSegmentation( std::vector<VECTOR2D>& aOutput, double aMaxError );

    void cubicParabolicApprox( std::vector<VECTOR2D>& aOutput, double aMaxError );

    bool isNaN() const;

    bool isFlat( double aMaxError ) const;

    VECTOR2D eval( double t );

    double m_minSegLen;

    ///< Control points
    std::vector<VECTOR2D> m_ctrlPts;
};


// TODO: Refactor BEZIER_POLY to use BEZIER

/**
 * Generic cubic Bezier representation
 */
template <typename NumericType>
class BEZIER
{
public:
    BEZIER() = default;

    constexpr BEZIER( const VECTOR2<NumericType>& aStart, const VECTOR2<NumericType>& aC1,
                      const VECTOR2<NumericType>& aC2, const VECTOR2<NumericType>& aEnd ) :
            Start( aStart ), C1( aC1 ), C2( aC2 ), End( aEnd )
    {
    }

    /**
     * Evaluate the Bezier curve at a given t value
     *
     * aT doesn't have to be in the range [0, 1], but if it's not, the
     * point will not be on the curve.
     *
     * @param aT the t value to evaluate the curve at (0 = start, 1 = end)
     * @return the point on the curve at t (0 <= t <= 1)
     */
    constexpr VECTOR2<NumericType> PointAt( double aT ) const
    {
        const double t2 = aT * aT;
        const double t3 = t2 * aT;
        const double t_m1 = 1.0 - aT;
        const double t_m1_2 = t_m1 * t_m1;
        const double t_m1_3 = t_m1_2 * t_m1;

        return ( t_m1_3 * Start ) + ( 3.0 * aT * t_m1_2 * C1 ) + ( 3.0 * t2 * t_m1 * C2 )
               + ( t3 * End );
    }

    VECTOR2<NumericType> Start;
    VECTOR2<NumericType> C1;
    VECTOR2<NumericType> C2;
    VECTOR2<NumericType> End;
};

/**
 * Transforms an ellipse or elliptical arc into a set of quadratic Bezier curves that approximate it
 */
template<typename T>
void TransformEllipseToBeziers( const ELLIPSE<T>& aEllipse, std::vector<BEZIER<T>>& aBeziers );

#endif  // BEZIER_CURVES_H
