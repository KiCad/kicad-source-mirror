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

struct ChamferFixture
{
};

/**
 * Declares the FilletFixture struct as the boost test fixture.
 */
BOOST_FIXTURE_TEST_SUITE( Chamfer, ChamferFixture )

struct TWO_LINE_CHAMFER_TEST_CASE
{
    SEG                           m_seg_a;
    SEG                           m_seg_b;
    CHAMFER_PARAMS                m_params;
    std::optional<CHAMFER_RESULT> m_expected_result;
};

static void DoChamferTestChecks( const TWO_LINE_CHAMFER_TEST_CASE& aTestCase )
{
    // Actally do the chamfer
    const std::optional<CHAMFER_RESULT> chamfer_result = ComputeChamferPoints( aTestCase.m_seg_a,
                                                                               aTestCase.m_seg_b,
                                                                               aTestCase.m_params );

    BOOST_REQUIRE_EQUAL( chamfer_result.has_value(), aTestCase.m_expected_result.has_value() );

    if( chamfer_result.has_value() )
    {
        const CHAMFER_RESULT& expected_result = aTestCase.m_expected_result.value();
        const CHAMFER_RESULT& actual_result = chamfer_result.value();

        BOOST_CHECK_PREDICATE( GEOM_TEST::SegmentsHaveSameEndPoints,
                               ( actual_result.m_chamfer )( expected_result.m_chamfer ) );

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
    }
}

BOOST_AUTO_TEST_CASE( SimpleChamferAtOrigin )
{
    /*           10
     *  0,0 +----+-------------> 1000
     *      |   /
     *      | /
     *   10 +
     *      |
     *      v 1000
     * */

    const TWO_LINE_CHAMFER_TEST_CASE testcase
    {
        { VECTOR2I( 0, 0 ), VECTOR2I( 1000, 0 ) },
        { VECTOR2I( 0, 0 ), VECTOR2I( 0, 1000 ) },
        { 10, 10 },
        { {
                SEG( VECTOR2I( 10, 0 ), VECTOR2I( 0, 10 ) ),       // chamfer
                { SEG( VECTOR2I( 10, 0 ), VECTOR2I( 1000, 0 ) ) }, // rest of the line A
                { SEG( VECTOR2I( 0, 10 ), VECTOR2I( 0, 1000 ) ) }, // rest of the line B
        } },
    };

    DoChamferTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( SimpleChamferNotAtOrigin )
{
    // Same as above but the intersection is not at the origin
    const TWO_LINE_CHAMFER_TEST_CASE testcase
    {
        { VECTOR2I( 1000, 1000 ), VECTOR2I( 2000, 1000 ) },
        { VECTOR2I( 1000, 1000 ), VECTOR2I( 1000, 2000 ) },
        { 10, 10 },
        { {
                SEG( VECTOR2I( 1010, 1000 ), VECTOR2I( 1000, 1010 ) ),
                { SEG( VECTOR2I( 1010, 1000 ), VECTOR2I( 2000, 1000 ) ) },
                { SEG( VECTOR2I( 1000, 1010 ), VECTOR2I( 1000, 2000 ) ) },
        } },
    };

    DoChamferTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( AsymmetricChamfer )
{
    // Same as above but the intersection is not at the origin
    const TWO_LINE_CHAMFER_TEST_CASE testcase
    {
        { VECTOR2I( 0, 0 ), VECTOR2I( 1000, 0 ) },
        { VECTOR2I( 0, 0 ), VECTOR2I( 0, 1000 ) },
        { 10, 100 },
        { {
                SEG( VECTOR2I( 10, 0 ), VECTOR2I( 0, 100 ) ),       // chamfer
                { SEG( VECTOR2I( 10, 0 ), VECTOR2I( 1000, 0 ) ) },  // rest of the line A
                { SEG( VECTOR2I( 0, 100 ), VECTOR2I( 0, 1000 ) ) }, // rest of the line B
        } },
    };

    DoChamferTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( ChamferFullLength )
{
    // Chamfer consumes the entire length of a line
    const TWO_LINE_CHAMFER_TEST_CASE testcase
    {
        { VECTOR2I( 0, 0 ), VECTOR2I( 1000, 0 ) },
        { VECTOR2I( 0, 0 ), VECTOR2I( 0, 100 ) },
        { 100, 100 },
        { {
                SEG( VECTOR2I( 100, 0 ), VECTOR2I( 0, 100 ) ),      // chamfer
                { SEG( VECTOR2I( 100, 0 ), VECTOR2I( 1000, 0 ) ) }, // rest of the line A
                std::nullopt,                                       // line b no longer exists
        } },
    };

    DoChamferTestChecks( testcase );
}

BOOST_AUTO_TEST_CASE( ChamferOverFullLength )
{
    // Chamfer consumes the entire length of a line
    const TWO_LINE_CHAMFER_TEST_CASE testcase
    {
        { VECTOR2I( 0, 0 ), VECTOR2I( 1000, 0 ) },
        { VECTOR2I( 0, 0 ), VECTOR2I( 0, 100 ) },
        { 150, 150 }, // > 100
        std::nullopt,
    };

    DoChamferTestChecks( testcase );
}

BOOST_AUTO_TEST_SUITE_END()
