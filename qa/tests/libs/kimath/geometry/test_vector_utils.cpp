/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/vector_utils.h>
#include <geometry/seg.h>


BOOST_AUTO_TEST_SUITE( VectorUtils )

BOOST_AUTO_TEST_CASE( PointIsInDirection )
{
    const VECTOR2I p0( 1000, 1000 );

    const VECTOR2I ne( 100, 100 );
    const VECTOR2I n( 0, 100 );
    const VECTOR2I s( 0, -100 );

    // To the east, so not directly inline with ne from p0
    const VECTOR2I p1_east = p0 + VECTOR2I( 1000, 0 );
    const VECTOR2I p1_west = p0 - VECTOR2I( 1000, 0 );

    BOOST_TEST( KIGEOM::PointIsInDirection( p1_east, ne, p0 ) );
    BOOST_TEST( !KIGEOM::PointIsInDirection( p1_west, ne, p0 ) );

    // Test the perpendicular corner case

    // Points on both sides are not in the direction
    BOOST_TEST( !KIGEOM::PointIsInDirection( p1_east, n, p0 ) );
    BOOST_TEST( !KIGEOM::PointIsInDirection( p1_west, n, p0 ) );

    // And they're also not in the opposite direction
    BOOST_TEST( !KIGEOM::PointIsInDirection( p1_east, s, p0 ) );
    BOOST_TEST( !KIGEOM::PointIsInDirection( p1_west, s, p0 ) );
}

BOOST_AUTO_TEST_CASE( ProjectsOntoSeg )
{
    const SEG seg( VECTOR2I( 0, 0 ), VECTOR2I( 100, 0 ) );

    BOOST_TEST( KIGEOM::PointProjectsOntoSegment( VECTOR2I( 50, 1000 ), seg ) );
    BOOST_TEST( !KIGEOM::PointProjectsOntoSegment( VECTOR2I( 150, 1000 ), seg ) );
}

BOOST_AUTO_TEST_CASE( LengthRatio )
{
    const SEG seg( VECTOR2I( 0, 0 ), VECTOR2I( 100, 0 ) );

    BOOST_TEST( KIGEOM::GetLengthRatioFromStart( VECTOR2I( 0, 0 ), seg ) == 0.0 );
    BOOST_TEST( KIGEOM::GetLengthRatioFromStart( VECTOR2I( 100, 0 ), seg ) == 1.0 );
    BOOST_TEST( KIGEOM::GetLengthRatioFromStart( VECTOR2I( 50, 0 ), seg ) == 0.5 );

    // Points not on the segment also work
    BOOST_CHECK_CLOSE( KIGEOM::GetLengthRatioFromStart( VECTOR2I( 0, 100 ), seg ), 1.0, 0.0001 );
    BOOST_CHECK_CLOSE( KIGEOM::GetLengthRatioFromStart( VECTOR2I( 0, 50 ), seg ), 0.5, 0.0001 );
}

BOOST_AUTO_TEST_CASE( NearestEndpoint )
{
    struct PtCase
    {
        VECTOR2I Point;
        bool     ExpectedStart;
    };

    const SEG seg( VECTOR2I( 0, 0 ), VECTOR2I( 100, 0 ) );

    const std::vector<PtCase> cases{
        { { -100, 0 }, true },
        { { 0, 0 }, true },
        { { 0, 50 }, true },
        // Make sure the tie breaks predictably
        // Equidistant points -> start
        { { 50, 0 }, true },
        { { 50, 50 }, true },
        { { 50, -50 }, true },
        // End points
        { { 200, 0 }, false },
        { { 100, 0 }, false },
        { { 100, 50 }, false },
    };

    for( const PtCase& pc : cases )
    {
        BOOST_TEST_INFO( "Point: " << pc.Point );
        BOOST_TEST( KIGEOM::GetNearestEndpoint( seg, pc.Point )
                    == ( pc.ExpectedStart ? seg.A : seg.B ) );
    }
}

BOOST_AUTO_TEST_SUITE_END()