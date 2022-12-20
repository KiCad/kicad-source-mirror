/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
     * @param aMinSegLen is the min dist between 2 successive points.
     * It can be used to reduce the number of points.
     * (the last point is always generated)
     * aMaxSegCount is the max number of segments created
     */
    void GetPoly( std::vector<VECTOR2I>& aOutput, int aMinSegLen = 0, int aMaxSegCount = 32 );
    void GetPoly( std::vector<VECTOR2D>& aOutput, double aMinSegLen = 0.0, int aMaxSegCount = 32 );

private:
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

    BEZIER( VECTOR2<NumericType> aStart, VECTOR2<NumericType> aC1, VECTOR2<NumericType> aC2,
            VECTOR2<NumericType> aEnd ) :
            Start( aStart ),
            C1( aC1 ),
            C2( aC2 ),
            End( aEnd )
    {}

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
