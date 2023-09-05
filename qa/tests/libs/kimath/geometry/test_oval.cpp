/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <geometry/oval.h>

#include "geom_test_utils.h"

/**
 * @brief Check that two collections contain the same elements, ignoring order.
 *
 * I.e. expected contains everything in actual and vice versa.
 *
 * The collections lengths are also checked to weed out unexpected duplicates.
 *
 * @param expected a collection of expected elements
 * @param actual a collection of actual elements
 */
template <typename T>
void CHECK_COLLECTIONS_SAME_UNORDERED(const T& expected, const T& actual) {

    for( const auto& p : expected )
    {
        BOOST_CHECK_MESSAGE( std::find( actual.begin(), actual.end(), p ) != actual.end(),
                             "Expected item not found: " << p );
    }

    for( const auto& p : actual )
    {
        BOOST_CHECK_MESSAGE( std::find( expected.begin(), expected.end(), p ) != expected.end(),
                             "Unexpected item: " << p );
    }

    BOOST_CHECK_EQUAL( expected.size(), actual.size() );
}

struct OvalFixture
{
};

BOOST_FIXTURE_TEST_SUITE( Oval, OvalFixture )

struct OVAL_POINTS_TEST_CASE
{
    VECTOR2I m_size;
    EDA_ANGLE m_rotation;
    std::vector<VECTOR2I> m_expected_points;
};

void DoOvalPointTestChecks( const OVAL_POINTS_TEST_CASE& testcase )
{
    const auto sort_vectors_x_then_y = []( const VECTOR2I& a, const VECTOR2I& b ) {
        return LexicographicalCompare<VECTOR2I::coord_type>( a, b ) > 0;
    };

    std::vector<VECTOR2I> expected_points = testcase.m_expected_points;
    std::vector<VECTOR2I> actual_points =
            GetOvalKeyPoints( testcase.m_size, testcase.m_rotation, OVAL_ALL_KEY_POINTS );

    CHECK_COLLECTIONS_SAME_UNORDERED( expected_points, actual_points );
}

BOOST_AUTO_TEST_CASE( SimpleOvalVertical )
{
    const OVAL_POINTS_TEST_CASE testcase
    {
        { 1000, 3000 },
        { 0, DEGREES_T },
        {
            { 0, 0 },
            // Main points
            { 0, 1500 },
            { 0, -1500 },
            { 500, 0 },
            { -500, 0 },
            // Cap centres
            { 0, 1000 },
            { 0, -1000 },
            // Side segment ends
            { 500, 1000 },
            { 500, -1000 },
            { -500, 1000 },
            { -500, -1000 },
        },
    };

    DoOvalPointTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( SimpleOvalHorizontal )
{
    const OVAL_POINTS_TEST_CASE testcase
    {
        { 3000, 1000 },
        { 0, DEGREES_T },
        {
            { 0, 0 },
            // Main points
            { 0, 500 },
            { 0, -500 },
            { 1500, 0 },
            { -1500, 0 },
            // Cap centres
            { 1000, 0 },
            { -1000, 0 },
            // Side segment ends
            { 1000, 500 },
            { 1000, -500 },
            { -1000, 500 },
            { -1000, -500 },
        },
    };

    DoOvalPointTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( SimpleOval45Degrees )
{
    // In this case, it's useful to keep in mind the hypotenuse of
    // isoceles right-angled triangles is sqrt(2) times the length of the sides
    //  500 / sqrt(2) = 354
    // 1000 / sqrt(2) = 707
    // 1500 / sqrt(2) = 1061
    // 2000 / sqrt(2) = 1414

    const OVAL_POINTS_TEST_CASE testcase
    {
        { 4000, 1000 },
        { 45, DEGREES_T },
        {
            { 0, 0 },
            // Main points
            { 1414, -1414 },
            { -1414, 1414 },
            { 354, 354 },
            { -354, -354 },
            // Side segment ends
            { -1414, 707 },
            { 1414, -707 },
            { -707, 1414 },
            { 707, -1414 },
            // Cap centres
            { 1061, -1061 },
            { -1061, 1061 },
            // Extremum points (always one of NSEW of a cap centre because 45 degrees)
            { -1061 - 500, 1061 },
            { -1061, 1061 + 500 },
            { 1061 + 500, -1061 },
            { 1061, -1061 - 500 },
        },
    };

    DoOvalPointTestChecks( testcase );
}

BOOST_AUTO_TEST_SUITE_END()
