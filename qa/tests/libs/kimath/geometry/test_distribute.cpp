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

#include <geometry/distribute.h>

#include "geom_test_utils.h"

struct DistributeFixture
{
};

/**
 * Declares the FilletFixture struct as the boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( Distribute, DistributeFixture )

struct DISTRIBUTE_GAPS_TEST_CASE
{
    std::vector<std::pair<int, int>> m_extents;
    std::vector<int>                 m_expectedDeltas;
};

static void DoDistributeGapsTestChecks( const DISTRIBUTE_GAPS_TEST_CASE& aTestCase )
{
    // Actally do the chamfer
    const std::vector<int> deltas = GetDeltasForDistributeByGaps( aTestCase.m_extents );

    BOOST_REQUIRE_EQUAL( deltas.size(), aTestCase.m_expectedDeltas.size() );

    // First and last items should not be moved
    BOOST_CHECK_EQUAL( deltas.front(), 0 );
    BOOST_CHECK_EQUAL( deltas.back(), 0 );

    for( size_t i = 0; i < deltas.size(); ++i )
    {
        BOOST_CHECK_EQUAL( deltas[i], aTestCase.m_expectedDeltas[i] );
    }
}

BOOST_AUTO_TEST_CASE( DistributeGapsNoChangeNeeded )
{
    const DISTRIBUTE_GAPS_TEST_CASE testcase{
        {
                // Already evenly spaced (100 gaps)
                { 0, 100 },
                { 200, 300 },
                { 400, 500 },
        },
        { 0, 0, 0 },
    };

    DoDistributeGapsTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( DistributeGapsSimpleShiftNeeded )
{
    const DISTRIBUTE_GAPS_TEST_CASE testcase{
        {
                // Need to move item 1 51 to the right
                { 0, 100 },
                { 149, 249 },
                { 400, 500 },
        },
        { 0, 51, 0 },
    };

    DoDistributeGapsTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( DistributeGapsRounding )
{
    const DISTRIBUTE_GAPS_TEST_CASE testcase{
        {
                // Have to fit 3 gaps into total sum of gaps of 100
                // so 33.333333 per gap
                // (note one rounds up, the other down)
                { -100, 0 },
                { 0, 100 }, // Move this to 33 .. 133
                { 0, 100 }, // Move this to 167 .. 267
                { 300, 400 },
        },
        { 0, 33, 167, 0 },
    };

    DoDistributeGapsTestChecks( testcase );
}


struct DISTRIBUTE_POINTS_TEST_CASE
{
    std::vector<int> m_points;
    std::vector<int> m_expectedDeltas;
};

static void DoDistributePointsTestChecks( const DISTRIBUTE_POINTS_TEST_CASE& aTestCase )
{
    // Actally do the chamfer
    const std::vector<int> deltas = GetDeltasForDistributeByPoints( aTestCase.m_points );

    BOOST_REQUIRE_EQUAL( deltas.size(), aTestCase.m_expectedDeltas.size() );

    // First and last items should not be moved
    BOOST_CHECK_EQUAL( deltas.front(), 0 );
    BOOST_CHECK_EQUAL( deltas.back(), 0 );

    for( size_t i = 0; i < deltas.size(); ++i )
    {
        BOOST_CHECK_EQUAL( deltas[i], aTestCase.m_expectedDeltas[i] );
    }
}

BOOST_AUTO_TEST_CASE( DistributePointsNoChangeNeeded )
{
    const DISTRIBUTE_POINTS_TEST_CASE testcase{
        // Already evenly spaced (100 gaps)
        { 0, 100, 200, 300, 400 },
        { 0, 0, 0, 0, 0 },
    };

    DoDistributePointsTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( DistributePointsSimpleShiftNeeded )
{
    const DISTRIBUTE_POINTS_TEST_CASE testcase{
        // Need to move item 1 51 to the right
        { 0, 49, 200 },
        { 0, 51, 0 },
    };

    DoDistributePointsTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( DistributePointsRounding )
{
    const DISTRIBUTE_POINTS_TEST_CASE testcase{
        // Have to fit 3 gaps into total sum of gaps of 100
        // so 33.333333 per gap
        // (note one rounds up, the other down)
        { 0, 0, 0, 100 },
        { 0, 33, 67, 0 },
    };

    DoDistributePointsTestChecks( testcase );
}

BOOST_AUTO_TEST_SUITE_END()
