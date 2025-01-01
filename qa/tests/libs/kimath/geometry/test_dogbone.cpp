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

#include <geometry/corner_operations.h>

#include "geom_test_utils.h"

/**
 * Declares the DogboneFixture struct as the boost test fixture.
 */
BOOST_AUTO_TEST_SUITE( Dogbone )

struct DOGBONE_TEST_CASE
{
    SEG                           m_seg_a;
    SEG                           m_seg_b;
    int                           m_radius;
    bool                          m_add_slots;
    std::optional<DOGBONE_RESULT> m_expected_result;
};

static void DoDogboneTestChecks( const DOGBONE_TEST_CASE& aTestCase )
{
    // Actally do the chamfer
    const std::optional<DOGBONE_RESULT> dogbone_result = ComputeDogbone(
            aTestCase.m_seg_a, aTestCase.m_seg_b, aTestCase.m_radius, aTestCase.m_add_slots );

    BOOST_REQUIRE_EQUAL( dogbone_result.has_value(), aTestCase.m_expected_result.has_value() );

    if( dogbone_result.has_value() )
    {
        const DOGBONE_RESULT& expected_result = aTestCase.m_expected_result.value();
        const DOGBONE_RESULT& actual_result = dogbone_result.value();

        const SEG expected_arc_chord =
                SEG( expected_result.m_arc.GetP0(), expected_result.m_arc.GetP1() );
        const SEG actual_arc_chord =
                SEG( actual_result.m_arc.GetP0(), actual_result.m_arc.GetP1() );

        BOOST_CHECK_PREDICATE( GEOM_TEST::SegmentsHaveSameEndPoints,
                               (actual_arc_chord) ( expected_arc_chord ) );
        BOOST_CHECK_EQUAL( actual_result.m_arc.GetArcMid(), expected_result.m_arc.GetArcMid() );

        const auto check_updated_seg =
                [&]( const std::optional<SEG>& updated_seg, const std::optional<SEG>& expected_seg )
        {
            BOOST_REQUIRE_EQUAL( updated_seg.has_value(), expected_seg.has_value() );

            if( updated_seg.has_value() )
            {
                BOOST_CHECK_PREDICATE( GEOM_TEST::SegmentsHaveSameEndPoints,
                                       ( *updated_seg )( *expected_seg ) );
            }
        };

        check_updated_seg( actual_result.m_updated_seg_a, expected_result.m_updated_seg_a );
        check_updated_seg( actual_result.m_updated_seg_b, expected_result.m_updated_seg_b );

        BOOST_CHECK_EQUAL( actual_result.m_small_arc_mouth, expected_result.m_small_arc_mouth );
    }
}

BOOST_AUTO_TEST_CASE( SimpleRightAngleAtOrigin )
{
    /*     /---_
     *    / +----+-------------> 1000
     *    | | \
     *     \|  (0,0)
     *      +
     *      |
     *      v
     */

    const DOGBONE_TEST_CASE testcase{
        { VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ) },
        { VECTOR2I( 0, 0 ), VECTOR2I( 0, 100000 ) },
        10000,
        false,
        // A right angle is an easy one to see, because the end and center points are
        // all on 45 degree lines from the center
        {
                {
                        SHAPE_ARC{
                                VECTOR2I( 14142, 0 ),
                                VECTOR2I( 0, 0 ),
                                VECTOR2I( 0, 14142 ),
                                0,
                        },
                        SEG( VECTOR2I( 14142, 0 ), VECTOR2I( 100000, 0 ) ),
                        SEG( VECTOR2I( 0, 14142 ), VECTOR2I( 0, 100000 ) ),
                        false,
                },

        },
    };

    DoDogboneTestChecks( testcase );
}

BOOST_AUTO_TEST_SUITE_END()