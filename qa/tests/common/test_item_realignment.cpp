/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <item_realignment.h>


class ItemRealignmentOrthoTestFixture
{
public:
    ItemRealignmentOrthoTestFixture()
    {
    }

    ORTHO_ITEM_REALIGNER m_aligner;
};


BOOST_FIXTURE_TEST_SUITE( ItemRealignmentTestSuite, ItemRealignmentOrthoTestFixture )


BOOST_AUTO_TEST_CASE( SimpleNull )
{
    // Make sure that two identical sets of points are aligned with no transform

    std::vector<VECTOR2I> ptsA{
        { 0, 0 },
        { 0, 1000 },
    };
    std::vector<VECTOR2I> ptsB = ptsA;

    std::optional<ITEM_REALIGNER_BASE::TRANSFORM> transform = m_aligner.GetTransform( ptsA, ptsB );

    BOOST_REQUIRE( transform.has_value() );
    BOOST_TEST( transform->m_Rotation == ANGLE_0 );
    BOOST_TEST( transform->m_Translation == VECTOR2I( 0, 0 ) );
}


BOOST_AUTO_TEST_CASE( SimpleRotation )
{
    std::vector<VECTOR2I> ptsA{
        { 0, 0 },
        { 1000, 0 },
    };
    std::vector<VECTOR2I> ptsB{
        { 0, 0 },
        { 0, 1000 },
    };

    std::optional<ITEM_REALIGNER_BASE::TRANSFORM> transform = m_aligner.GetTransform( ptsA, ptsB );

    BOOST_REQUIRE( transform.has_value() );
    BOOST_TEST( transform->m_Rotation == ANGLE_90 );
    BOOST_TEST( transform->m_Translation == VECTOR2I( 0, 0 ) );
}


BOOST_AUTO_TEST_CASE( AlmostRotation )
{
    std::vector<VECTOR2I> ptsA{
        { 0, 0 },
        { 1000, 0 },
    };
    // One point moved a bit, but we will interpret that as a point moved,
    // rather than a tiny rotation of the whole thing.
    std::vector<VECTOR2I> ptsB{
        { 0, 0 },
        { 1000, 10 },
    };

    std::optional<ITEM_REALIGNER_BASE::TRANSFORM> transform = m_aligner.GetTransform( ptsA, ptsB );

    BOOST_REQUIRE( transform.has_value() );
    BOOST_TEST( transform->m_Rotation == ANGLE_0 );
    BOOST_TEST( transform->m_Translation == VECTOR2I( 0, -5 ) );
}


BOOST_AUTO_TEST_CASE( MinorityOfPointsMoved )
{
    // In this case, the points don't rotate, but one of the points moves.
    // We want to leave the transform still locked to the matching 3 points, and let
    // the one outlier point be the one that moves.

    std::vector<VECTOR2I> ptsA = {
        { 0, 0 },
        { 1000, 0 },
        { 0, 1000 },
        { 1000, 1000 },
    };
    std::vector<VECTOR2I> ptsB = {
        { 0, 0 },
        { 1000, 10 },
        { 0, 1000 },
        { 1000, 1000 },
    };

    std::optional<ITEM_REALIGNER_BASE::TRANSFORM> transform = m_aligner.GetTransform( ptsA, ptsB );

    BOOST_REQUIRE( transform.has_value() );
    BOOST_TEST( transform->m_Rotation == ANGLE_0 );
    BOOST_TEST( transform->m_Translation == VECTOR2I( 0, 0 ) );
}


BOOST_AUTO_TEST_CASE( PointsSpacedApart )
{
    // In this case, all the points move apart - this is like a gullwing having the
    // the pad lengths changed. In this case, we want the transform to minimise
    // the error, rather than saying locked down one side.

    const int pitchXA = 1000;
    const int pitchY = 1000;
    const int pitchXB = 1100;

    // Note - the points aren't centred over the origin - this is
    // intentional so the transform doesn't just equal zero.
    std::vector<VECTOR2I> ptsA {
        { 0, -pitchY },
        { 0, 0 },
        { 0, pitchY },
        { pitchXA, -pitchY },
        { pitchXA, 0 },
        { pitchXA, pitchY },
    };
    std::vector<VECTOR2I> ptsB {
        { 0, -pitchY },
        { 0, 0 },
        { 0, pitchY },
        { pitchXB, -pitchY },
        { pitchXB, 0 },
        { pitchXB, pitchY },
    };

    std::optional<ITEM_REALIGNER_BASE::TRANSFORM> transform = m_aligner.GetTransform( ptsA, ptsB );

    const int expectedTranslationX = -std::abs( pitchXB - pitchXA ) / 2;
    const int expectedTranslationY = 0;

    BOOST_REQUIRE( transform.has_value() );
    BOOST_TEST( transform->m_Rotation == ANGLE_0 );
    BOOST_TEST( transform->m_Translation == VECTOR2I( expectedTranslationX, expectedTranslationY ) );
}


BOOST_AUTO_TEST_CASE( RotateAndMove )
{
    // In this case, the points are rotated and moved.

    const int pitch = 1000;

    // Rectangle 2000x1000 centred on 0,0
    std::vector<VECTOR2I> ptsA = {
        { -1000, -500 },
        { -1000, 500 },
        { 1000, -500 },
        { 1000, 500 },
    };
    // Same rectangle, but rotated 90 degrees ACW and centred on (1000,1000)
    std::vector<VECTOR2I> ptsB = {
        { 500, 2000 },
        { 1500, 2000 },
        { 500, 0 },
        { 1500, 0 },
    };

    std::optional<ITEM_REALIGNER_BASE::TRANSFORM> transform = m_aligner.GetTransform( ptsA, ptsB );

    // The expected translation is applied after the rotation about the origin,
    // so ptB[0] wll be rotated to (0, 500). Then to get to the expected position of ptsA[0] = (-1000, -500), we need to translate by (1000, -1000)
    const VECTOR2I expectedTranslation( 1000, -1000 );

    BOOST_REQUIRE( transform.has_value() );
    BOOST_TEST( transform->m_Rotation == ANGLE_270 ); // 90 CW
    BOOST_TEST( transform->m_Translation == expectedTranslation );
}


BOOST_AUTO_TEST_SUITE_END()