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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <cstdint>
#include <limits>
#include <random>

#include <geometry/geometry_predicates.h>
#include <math/wide_int.h>

BOOST_AUTO_TEST_SUITE( WideInt )

namespace
{
int sign( const KI_INT128& aValue )
{
    return ( aValue > KI_INT128( 0 ) ) - ( aValue < KI_INT128( 0 ) );
}
} // namespace


BOOST_AUTO_TEST_CASE( CrossWideKnownValues )
{
    BOOST_CHECK_EQUAL( ToDouble( CrossWide( VECTOR2L( 3, 4 ), VECTOR2L( 5, 6 ) ) ), -2.0 );
    BOOST_CHECK_EQUAL( ToDouble( DotWide( VECTOR2L( 3, 4 ), VECTOR2L( 5, 6 ) ) ), 39.0 );

    // The (+-1.1e9) diagonals wrap an int64 cross product
    const VECTOR2L d1( 2200000000LL, 2200000000LL );
    const VECTOR2L d2( 2200000000LL, -2200000000LL );

    BOOST_CHECK_EQUAL( sign( CrossWide( d1, d2 ) ), -1 );
    BOOST_CHECK_EQUAL( sign( CrossWide( d2, d1 ) ), 1 );
    BOOST_CHECK_EQUAL( sign( CrossWide( d1, d1 ) ), 0 );
    BOOST_CHECK_EQUAL( ToDouble( CrossWide( d1, d2 ) ), -2.0 * 2200000000.0 * 2200000000.0 );

    // Products differing by 1, invisible to a double product
    const int64_t k = 3000000000LL;

    BOOST_CHECK_EQUAL( ToDouble( CrossWide( VECTOR2L( k + 1, k ), VECTOR2L( k + 2, k + 1 ) ) ), 1.0 );
    BOOST_CHECK_EQUAL( ToDouble( CrossWide( VECTOR2L( k + 2, k + 1 ), VECTOR2L( k + 1, k ) ) ), -1.0 );
}


BOOST_AUTO_TEST_CASE( Int64Extremes )
{
    constexpr int64_t lo = std::numeric_limits<int64_t>::min();
    constexpr int64_t hi = std::numeric_limits<int64_t>::max();

    // (-2^63)^2 - 0 = 2^126
    BOOST_CHECK_EQUAL( ToDouble( CrossWide( VECTOR2L( lo, 0 ), VECTOR2L( 0, lo ) ) ), std::ldexp( 1.0, 126 ) );
    BOOST_CHECK_EQUAL( ToDouble( CrossWide( VECTOR2L( lo, 0 ), VECTOR2L( 0, hi ) ) ), -std::ldexp( 1.0, 126 ) );
    BOOST_CHECK_EQUAL( ToDouble( DotWide( VECTOR2L( lo, 0 ), VECTOR2L( lo, 0 ) ) ), std::ldexp( 1.0, 126 ) );
}


BOOST_AUTO_TEST_CASE( OrientationSignBeyondDoublePrecision )
{
    // The exact determinant is 1 but a double cross product rounds it to 0
    VECTOR2I a( -1500000000, -1500000000 );
    VECTOR2I b( 1500000001, 1500000000 );
    VECTOR2I c( 1500000002, 1500000001 );

    BOOST_CHECK_EQUAL( KIGEOM::OrientationSign( a, b, c ), 1 );
    BOOST_CHECK_EQUAL( KIGEOM::OrientationSign( a, c, b ), -1 );
}


BOOST_AUTO_TEST_CASE( InCircleDelaunayLegalExact )
{
    const VECTOR2I a( 0, 0 ), b( 1000000, 0 ), c( 0, 1000000 );

    // Cocircular is legal, inside is not, outside is
    BOOST_CHECK( KIGEOM::InCircleDelaunayLegal( a, b, c, VECTOR2I( 1000000, 1000000 ) ) );
    BOOST_CHECK( !KIGEOM::InCircleDelaunayLegal( a, b, c, VECTOR2I( 999999, 999999 ) ) );
    BOOST_CHECK( KIGEOM::InCircleDelaunayLegal( a, b, c, VECTOR2I( 1000001, 1000001 ) ) );

    // Exact determinant is 999999999000000000, far below the tolerance of a filtered double test
    BOOST_CHECK( !KIGEOM::InCircleDelaunayLegal( VECTOR2I( 0, 0 ), VECTOR2I( 1000000000, 0 ),
                                                 VECTOR2I( 1000000000, 1 ), VECTOR2I( 1, 1 ) ) );
}


BOOST_AUTO_TEST_CASE( WordsToDoubleBorrowAndRounding )
{
    using KIGEOM_WIDE::WordsToDouble;

    BOOST_CHECK_EQUAL( WordsToDouble( 0, 0 ), 0.0 );
    BOOST_CHECK_EQUAL( WordsToDouble( 0, UINT64_MAX ), 18446744073709551616.0 );
    BOOST_CHECK_EQUAL( WordsToDouble( 1, 0 ), std::ldexp( 1.0, 64 ) );
    BOOST_CHECK_EQUAL( WordsToDouble( -1, 0 ), -std::ldexp( 1.0, 64 ) );
    BOOST_CHECK_EQUAL( WordsToDouble( -1, UINT64_MAX ), -1.0 );

    // 2^64 + 2^11 + 1 is above the halfway point of a 53-bit mantissa, only the sticky bit rounds it up
    const double up = std::ldexp( 1.0, 64 ) + std::ldexp( 1.0, 12 );

    BOOST_CHECK_EQUAL( WordsToDouble( 1, ( uint64_t( 1 ) << 11 ) + 1 ), up );
    BOOST_CHECK_EQUAL( WordsToDouble( -2, ~( ( uint64_t( 1 ) << 11 ) + 1 ) + 1 ), -up );

    // Exactly halfway rounds to even
    BOOST_CHECK_EQUAL( WordsToDouble( 1, uint64_t( 1 ) << 11 ), std::ldexp( 1.0, 64 ) );
}


BOOST_AUTO_TEST_CASE( RandomAgainstLongDouble )
{
    std::mt19937_64 rng( 4242 );

    auto operand = [&]() -> int64_t
    {
        int64_t v = static_cast<int64_t>( rng() );

        return v >> ( rng() % 64 );
    };

    for( int i = 0; i < 100000; i++ )
    {
        VECTOR2L a( operand(), operand() );
        VECTOR2L b( operand(), operand() );

        // The x87 mantissa has 64 bits so each product is exact and the difference is close
        long double cross = static_cast<long double>( a.x ) * b.y - static_cast<long double>( a.y ) * b.x;
        long double dot = static_cast<long double>( a.x ) * b.x + static_cast<long double>( a.y ) * b.y;

        BOOST_REQUIRE_CLOSE_FRACTION( ToDouble( CrossWide( a, b ) ), static_cast<double>( cross ), 1e-9 );
        BOOST_REQUIRE_CLOSE_FRACTION( ToDouble( DotWide( a, b ) ), static_cast<double>( dot ), 1e-9 );
    }
}


#if defined( __SIZEOF_INT128__ ) && !defined( _MSC_VER )

BOOST_AUTO_TEST_CASE( WordsToDoubleAgainstInt128 )
{
    std::mt19937_64 rng( 987 );

    for( int i = 0; i < 300000; i++ )
    {
        unsigned __int128 mag = ( static_cast<unsigned __int128>( rng() ) << 64 ) | rng();
        mag >>= rng() % 128;

        // Sticky and tie patterns need many trailing zeros
        if( i % 3 == 0 )
            mag &= ~( ( static_cast<unsigned __int128>( 1 ) << ( rng() % 100 ) ) - 1 );

        __int128 v = static_cast<__int128>( mag );

        BOOST_REQUIRE_EQUAL( KIGEOM_WIDE::WordsToDouble( static_cast<int64_t>( v >> 64 ), static_cast<uint64_t>( v ) ),
                             static_cast<double>( v ) );
    }
}

#endif

BOOST_AUTO_TEST_SUITE_END()
