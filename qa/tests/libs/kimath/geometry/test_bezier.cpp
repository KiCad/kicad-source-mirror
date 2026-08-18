/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <bezier_curves.h>


BOOST_AUTO_TEST_SUITE( Bezier )


static void checkVecClose( const VECTOR2D& aActual, const VECTOR2D& aExpected )
{
    constexpr double tol = 1e-9;

    BOOST_CHECK_SMALL( aActual.x - aExpected.x, tol );
    BOOST_CHECK_SMALL( aActual.y - aExpected.y, tol );
}


BOOST_AUTO_TEST_CASE( SplitPreservesEndpoints )
{
    const BEZIER<double> curve( { 0.0, 0.0 }, { 30.0, 90.0 }, { 70.0, -40.0 },
                                { 100.0, 0.0 } );
    BEZIER<double> left;
    BEZIER<double> right;

    curve.Split( 0.35, left, right );

    checkVecClose( left.Start, curve.Start );
    checkVecClose( left.End, curve.PointAt( 0.35 ) );
    checkVecClose( right.Start, left.End );
    checkVecClose( right.End, curve.End );
}


BOOST_AUTO_TEST_CASE( SubCurvePreservesOriginalParameterization )
{
    const BEZIER<double> curve( { 0.0, 0.0 }, { 30.0, 90.0 }, { 70.0, -40.0 },
                                { 100.0, 0.0 } );
    const double t0 = 0.2;
    const double t1 = 0.8;
    const BEZIER<double> subCurve = curve.SubCurve( t0, t1 );

    checkVecClose( subCurve.Start, curve.PointAt( t0 ) );
    checkVecClose( subCurve.End, curve.PointAt( t1 ) );
    checkVecClose( subCurve.PointAt( 0.5 ), curve.PointAt( t0 + ( t1 - t0 ) * 0.5 ) );
}


BOOST_AUTO_TEST_SUITE_END()
