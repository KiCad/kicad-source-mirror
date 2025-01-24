/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/geometry/line_chain_construction.h>

#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>

#include <algorithm>

#include "geom_test_utils.h"


BOOST_AUTO_TEST_SUITE( Fillet )

/*
 * @brief check that a single segment of a fillet complies with the geometric
 * constraint:
 *
 * 1: The end points are radius from the centre point
 * 2: The mid point error is acceptable
 * 3: The segment midpoints are perpendicular to the radius
 */
void TestFilletSegmentConstraints( const SEG& aSeg, VECTOR2I aRadCentre,
    int aRadius, int aError )
{
    const auto diffA = aRadCentre - aSeg.A;
    const auto diffB = aRadCentre - aSeg.B;
    const auto diffC = aRadCentre - aSeg.Center();

    // Check 1: radii (error of 1 for rounding)
    BOOST_CHECK_PREDICATE(
            KI_TEST::IsWithinAndBelow<int>, ( diffA.EuclideanNorm() )( aRadius )( 1 ) );
    BOOST_CHECK_PREDICATE(
            KI_TEST::IsWithinAndBelow<int>, ( diffB.EuclideanNorm() )( aRadius )( 1 ) );

    // Check 2: Mid-point error
    BOOST_CHECK_PREDICATE(
            KI_TEST::IsWithinAndBelow<int>, ( diffC.EuclideanNorm() )( aRadius )( aError + 1 ) );

    // Check 3: Mid-point -> radius centre perpendicular
    const EDA_ANGLE perpendularityMaxError = ANGLE_90 / 10;
    BOOST_CHECK_PREDICATE( GEOM_TEST::ArePerpendicular<int>,
            ( diffC )( aSeg.A - aSeg.B )( perpendularityMaxError ) );
}


/**
 * @brief: Create a square, fillet it, and check a corner for correctness
 */
void TestSquareFillet( int aSquareSize, int aRadius, int aError )
{
    using namespace GEOM_TEST;

    SHAPE_POLY_SET squarePolySet;

    squarePolySet.AddOutline( KI_TEST::BuildSquareChain( aSquareSize, VECTOR2I( 0, 0 ) ) );

    SHAPE_POLY_SET filleted = FilletPolySet( squarePolySet, aRadius, aError );

    // expect a single filleted polygon
    BOOST_CHECK_EQUAL( filleted.OutlineCount(), 1 );

    auto segIter = filleted.IterateSegments();

    const VECTOR2I radCentre { aSquareSize / 2 - aRadius,
        aSquareSize / 2 - aRadius };

    int checked = 0;

    for( ; segIter; segIter++ )
    {
        // Only check the first Quadrant
        if ( SegmentCompletelyInQuadrant( *segIter, QUADRANT::Q1 ) )
        {
            TestFilletSegmentConstraints( *segIter, radCentre, aRadius, aError );
            checked++;
        }
    }

    // we expect there to be at least one segment in the fillet
    BOOST_CHECK( checked > 0 );
}


/**
 * @brief: Create a square concave corner, fillet and check correctness
 */
void TestConcaveSquareFillet( int aSquareSize, int aRadius, int aError )
{
    using namespace GEOM_TEST;

    SHAPE_POLY_SET polySet;
    SHAPE_LINE_CHAIN polyLine;

    /*
     * L-shape:
     *    ----
     *    |   |
     * ----   |
     * |      |
     * --------
     */

    polyLine.Append( VECTOR2I{ 0, 0 } );
    polyLine.Append( VECTOR2I{ 0, aSquareSize / 2 } );
    polyLine.Append( VECTOR2I{ aSquareSize / 2 , aSquareSize / 2 } );
    polyLine.Append( VECTOR2I{ aSquareSize / 2 , aSquareSize } );
    polyLine.Append( VECTOR2I{ aSquareSize, aSquareSize } );
    polyLine.Append( VECTOR2I{ aSquareSize, 0 } );

    polyLine.SetClosed( true );

    polySet.AddOutline( polyLine );

    SHAPE_POLY_SET filleted = FilletPolySet(polySet, aRadius, aError);

    // expect a single filleted polygon
    BOOST_CHECK_EQUAL( filleted.OutlineCount(), 1 );

    auto segIter = filleted.IterateSegments();

    const VECTOR2I radCentre { aSquareSize / 2 - aRadius,
        aSquareSize / 2 + aRadius };

    int checked = 0;

    for( ; segIter; segIter++ )
    {
        // Only check segments around the concave corner
        if ( SegmentCompletelyWithinRadius( *segIter, radCentre, aRadius + 1) )
        {
            TestFilletSegmentConstraints( *segIter, radCentre, aRadius, aError );
            checked++;
        }
    }

    // we expect there to be at least one segment in the fillet
    BOOST_CHECK( checked > 0 );
}


struct SquareFilletTestCase
{
    int squareSize;
    int radius;
    int error;

    friend std::ostream& operator<<( std::ostream& os, const SquareFilletTestCase& testCase )
    {
        return os << "Square size: " << testCase.squareSize << ", Radius: " << testCase.radius
                  << ", Error: " << testCase.error;
    }
};

const std::vector<SquareFilletTestCase> squareFilletCases{
    { 1000, 120, 10 },
    { 1000, 10, 1 },

    /* Large error relative to fillet */
    { 1000, 10, 5 },

    /* Very small error relative to fillet(many segments in interpolation) */
    { 70000, 1000, 1 },
};

/**
 * Tests the SHAPE_POLY_SET::FilletPolygon method against certain geometric
 * constraints.
 */
BOOST_DATA_TEST_CASE( SquareFillet, boost::unit_test::data::make( squareFilletCases ), testCase )
{
    TestSquareFillet( testCase.squareSize, testCase.radius, testCase.error );
}

BOOST_DATA_TEST_CASE( SquareConcaveFillet, boost::unit_test::data::make( squareFilletCases ),
                      testCase )
{
    TestConcaveSquareFillet( testCase.squareSize, testCase.radius, testCase.error );
}


BOOST_AUTO_TEST_SUITE_END()
