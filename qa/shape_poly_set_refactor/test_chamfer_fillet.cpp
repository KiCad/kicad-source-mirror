/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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
#include <boost/test/test_case_template.hpp>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_line_chain.h>
#include <polygon/PolyLine.h>
#include <algorithm>

#include <qa/data/fixtures_geometry.h>

/**
 * Declares the ChamferFilletFixture struct as the boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( ChamferFillet, ChamferFilletFixture )

/**
 * Function lexicographicOrder
 * defines a lexicographic order between two VECTOR2I objects. Used along with std::sort
 * when checking that two polygons have the same vertices.
 * @param  i is a VECTOR2I object.
 * @param  j is a VECTOR2I object.
 * @return   bool - true if (i.x, i.y) < (j.x, j.y) using the lexicographic order,
 *                i.e., i.x < j.x or i.x = j.x and i.y < j.y; false in any other case.
 */
bool lexicographicOrder( VECTOR2I i, VECTOR2I j )
{
    if( i.x != j.x )
        return( i.x < j.x );
    else
        return( i.y < j.y );
}

/**
 * Function TestLineChainEqualCPolyLine
 * tests the equality between a SHAPE_LINE_CHAIN polygon and a polygon inside a
 * CPolyLine object using Boost test suite.
 * @param lineChain is a SHAPE_LINE_CHAIN polygon object.
 * @param polyLine  is a CPolyLine polygon object.
 * @param contourIdx is the index of the contour inside polyLine that has to be tested
 *                   against lineChain.
 */
void TestLineChainEqualCPolyLine(SHAPE_LINE_CHAIN& lineChain, CPolyLine& polyLine,
                                 int contourIdx = 0)
{
    // Arrays to store the polygon points lexicographically ordered
    std::vector<VECTOR2I> chainPoints;
    std::vector<VECTOR2I> polyPoints;

    // Populate the array storing the new data with the lineChain corners
    for (int pointIdx = 0; pointIdx < lineChain.PointCount(); pointIdx++) {
        chainPoints.push_back(lineChain.Point(pointIdx));
    }

    int start = polyLine.GetContourStart(contourIdx);
    int end = polyLine.GetContourEnd(contourIdx);

    // Populate the array storing the legacy data with the polyLine corners
    for (int pointIdx = start; pointIdx <= end; pointIdx++) {
        polyPoints.push_back( VECTOR2I(polyLine.GetX(pointIdx), polyLine.GetY(pointIdx)) );
    }

    // Order the vectors in a lexicographic way
    std::sort(chainPoints.begin(), chainPoints.end(), lexicographicOrder);
    std::sort(polyPoints.begin(), polyPoints.end(), lexicographicOrder);

    // Compare every point coordinate to check the equality
    BOOST_CHECK_EQUAL_COLLECTIONS(chainPoints.begin(), chainPoints.end(),
                                  polyPoints.begin(), polyPoints.end());
}

/**
 * Tests the SHAPE_POLY_SET::ChamferPolygon, which has been refactored into SHAPE_POLY_SET from
 * CPolyLine::Chamfer. Assuming the code in CPolyLine is right, this test ensures the behaviour of
 * the new refactored code does not change anything.
 */
BOOST_AUTO_TEST_CASE( Chamfer )
{
    SHAPE_POLY_SET::POLYGON actual;
    CPolyLine expected;

    // Test different distances, up to the half of the minimum segment longitude
    for (int distance = 0; distance < 5; distance++) {
        // Chamfered polygon to be tested.
        actual = common.holeyPolySet.ChamferPolygon( distance, 0 );

        // Chamfered polygon assumed to be right.
        expected = *legacyPolyLine.Chamfer( distance );

        // Double check that there are no repeated corners in the legacy shape.
        expected.RemoveNullSegments();

        // Test equality
        for (size_t contourIdx = 0; contourIdx < actual.size(); contourIdx++)
        {
            TestLineChainEqualCPolyLine(actual[contourIdx], expected, contourIdx);
        }
    }
}

/**
 * Tests the SHAPE_POLY_SET::FilletPolygon, which has been refactored into
 * SHAPE_POLY_SET from CPolyLine::Fillet.
 * Assuming the code in CPolyLine is right, this test ensures the behaviour of the new
 * refactored code does not change anything.
 */
BOOST_AUTO_TEST_CASE( Fillet )
{
    SHAPE_POLY_SET::POLYGON actual;
    CPolyLine expected;

    // Test different radius, up to the half of the minimum segment longitude
    for (int radius = 1; radius < 5; radius++)
    {
        // Test different number of segments
        for (size_t segments = 1; segments < 100; segments++)
        {
            // Chamfered polygon to be tested.
            actual = common.holeyPolySet.FilletPolygon( radius, segments, 0 );

            // Chamfered polygon assumed to be right.
            expected = *legacyPolyLine.Fillet( radius, segments );

            // Double check that there are no repeated corners in the legacy shape.
            expected.RemoveNullSegments();

            // Test equality
            for (size_t contourIdx = 0; contourIdx < actual.size(); contourIdx++)
            {
                TestLineChainEqualCPolyLine(actual[contourIdx], expected, contourIdx);
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
