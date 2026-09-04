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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_drc_creepage_circle_prune.cpp
 *
 * Regression test for the early rejection in CU_SHAPE_CIRCLE::Paths( const BE_SHAPE_CIRCLE& ).
 * The path that must clear the target is the external tangent sqrt(dist^2 - R2^2) - R1 outside
 * the hole, or the radial gap R2 - dist - R1 inside it. Both are shorter than the centre
 * distance. These cases put the centres beyond the target and the surfaces inside it, where a
 * prune on the centre distance drops the pair.
 */

#include <cmath>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <base_units.h>
#include <drc/drc_creepage_utils.h>


namespace
{

// Callers select a side by index, so a reachable pair always yields both tangents
constexpr size_t TANGENTS_PER_PAIR = 2;

} // namespace


BOOST_AUTO_TEST_CASE( CreepageCircleToHoleKeepsReachableTangent )
{
    const double R1 = pcbIUScale.mmToIU( 0.1 );
    const int    R2 = pcbIUScale.mmToIU( 10.0 );
    const int    dist = pcbIUScale.mmToIU( 10.2 );
    const double maxWeight = pcbIUScale.mmToIU( 9.0 );

    const double expectedWeight = sqrt( (double) dist * dist - (double) R2 * R2 ) - R1;

    // Outside the hole, centres beyond the target, tangent inside it
    BOOST_REQUIRE_GT( dist, R2 );
    BOOST_REQUIRE_GT( dist, maxWeight );
    BOOST_REQUIRE_LT( expectedWeight, maxWeight );

    CU_SHAPE_CIRCLE copper( VECTOR2I( 0, 0 ), R1 );
    BE_SHAPE_CIRCLE hole( VECTOR2I( dist, 0 ), R2 );

    std::vector<PATH_CONNECTION> paths = copper.Paths( hole, maxWeight, maxWeight * maxWeight );

    BOOST_REQUIRE_EQUAL( paths.size(), TANGENTS_PER_PAIR );

    for( const PATH_CONNECTION& pc : paths )
        BOOST_CHECK_CLOSE( pc.weight, expectedWeight, 0.1 );
}


BOOST_AUTO_TEST_CASE( CreepageCircleInsideHoleKeepsRadialGap )
{
    const double R1 = pcbIUScale.mmToIU( 0.1 );
    const int    R2 = pcbIUScale.mmToIU( 10.0 );
    const int    dist = pcbIUScale.mmToIU( 9.0 );
    const double maxWeight = pcbIUScale.mmToIU( 6.0 );

    const double expectedWeight = R2 - dist - R1;

    // Inside the hole, centres beyond the target, radial gap inside it
    BOOST_REQUIRE_LE( dist, R2 );
    BOOST_REQUIRE_GT( dist, maxWeight );
    BOOST_REQUIRE_LT( expectedWeight, maxWeight );

    CU_SHAPE_CIRCLE copper( VECTOR2I( 0, 0 ), R1 );
    BE_SHAPE_CIRCLE hole( VECTOR2I( dist, 0 ), R2 );

    std::vector<PATH_CONNECTION> paths = copper.Paths( hole, maxWeight, maxWeight * maxWeight );

    BOOST_REQUIRE_EQUAL( paths.size(), TANGENTS_PER_PAIR );

    for( const PATH_CONNECTION& pc : paths )
        BOOST_CHECK_CLOSE( pc.weight, expectedWeight, 0.1 );
}
