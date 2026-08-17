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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include <boost/test/unit_test.hpp>

#include <cmath>

#include <geometry/poisson_disk.h>

BOOST_AUTO_TEST_SUITE( PoissonDisk )

// Distance squared on the unit torus (coordinates wrap at 1.0)
static double torDistSq( const VECTOR2D& a, const VECTOR2D& b )
{
    double dx = std::fabs( a.x - b.x );
    double dy = std::fabs( a.y - b.y );

    if( dx > 0.5 )
        dx = 1.0 - dx;

    if( dy > 0.5 )
        dy = 1.0 - dy;

    return dx * dx + dy * dy;
}


BOOST_AUTO_TEST_CASE( DegenerateInputs )
{
    BOOST_CHECK( POISSON_DISK::ToroidalUnitTile( 0.0, 1 ).empty() );
    BOOST_CHECK( POISSON_DISK::ToroidalUnitTile( -0.1, 1 ).empty() );
    BOOST_CHECK( POISSON_DISK::ToroidalUnitTile( 0.6, 1 ).empty() );
}


BOOST_AUTO_TEST_CASE( SamplesInUnitSquare )
{
    for( const VECTOR2D& pt : POISSON_DISK::ToroidalUnitTile( 0.125, 42 ) )
    {
        BOOST_CHECK( pt.x >= 0.0 && pt.x < 1.0 );
        BOOST_CHECK( pt.y >= 0.0 && pt.y < 1.0 );
    }
}


BOOST_AUTO_TEST_CASE( ToroidalMinDistanceRespected )
{
    const double minDist = 0.125;

    std::vector<VECTOR2D> samples = POISSON_DISK::ToroidalUnitTile( minDist, 42 );

    for( size_t i = 0; i < samples.size(); ++i )
    {
        for( size_t j = i + 1; j < samples.size(); ++j )
        {
            BOOST_CHECK_MESSAGE( torDistSq( samples[i], samples[j] ) >= minDist * minDist,
                                 "samples " << i << " and " << j << " closer than minDist" );
        }
    }
}


BOOST_AUTO_TEST_CASE( DeterministicForSeed )
{
    std::vector<VECTOR2D> a = POISSON_DISK::ToroidalUnitTile( 0.125, 42 );
    std::vector<VECTOR2D> b = POISSON_DISK::ToroidalUnitTile( 0.125, 42 );

    BOOST_REQUIRE_EQUAL( a.size(), b.size() );

    for( size_t i = 0; i < a.size(); ++i )
    {
        BOOST_CHECK_EQUAL( a[i].x, b[i].x );
        BOOST_CHECK_EQUAL( a[i].y, b[i].y );
    }

    // A different seed must give a different pattern
    std::vector<VECTOR2D> c = POISSON_DISK::ToroidalUnitTile( 0.125, 43 );

    bool differs = ( a.size() != c.size() );

    for( size_t i = 0; !differs && i < a.size(); ++i )
        differs = ( a[i] != c[i] );

    BOOST_CHECK( differs );
}


BOOST_AUTO_TEST_CASE( ReasonableDensity )
{
    const double minDist = 0.125;

    std::vector<VECTOR2D> samples = POISSON_DISK::ToroidalUnitTile( minDist, 42 );

    // Hexagonal packing bounds the count above at 2/(sqrt(3)*r^2) ~= 74 for r = 1/8.
    // Bridson sampling typically lands around 55-65; anything below 20 means the
    // generator stopped early.
    BOOST_CHECK_GE( samples.size(), 20 );
    BOOST_CHECK_LE( samples.size(), 74 );
}

BOOST_AUTO_TEST_SUITE_END()
