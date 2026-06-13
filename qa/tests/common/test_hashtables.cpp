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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <hashtables.h>

BOOST_AUTO_TEST_SUITE( HashCombine )


BOOST_AUTO_TEST_CASE( KnownVectors )
{
    // Pinned results of the Boost hash_combine mixing step. The formula must
    // stay stable because diff-engine summary hashes that are persisted depend
    // on it. Computed directly from h ^ ( v + 0x9e3779b9 + (h<<6) + (h>>2) ).
    BOOST_CHECK_EQUAL( KiHashCombine( 0u, 0u ), static_cast<std::size_t>( 0x9e3779b9u ) );

    // h == 0 short-circuits the shifts, so the result is just v + 0x9e3779b9.
    BOOST_CHECK_EQUAL( KiHashCombine( 0u, 1u ), static_cast<std::size_t>( 0x9e3779bau ) );

    // Spot-check a non-zero seed against the longhand formula.
    {
        const std::size_t seed = 0x12345678u;
        const std::size_t value = 0xdeadbeefu;
        const std::size_t expected =
                seed ^ ( value + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 ) );
        BOOST_CHECK_EQUAL( KiHashCombine( seed, value ), expected );
    }
}


BOOST_AUTO_TEST_CASE( Deterministic )
{
    BOOST_CHECK_EQUAL( KiHashCombine( 42u, 99u ), KiHashCombine( 42u, 99u ) );
}


BOOST_AUTO_TEST_CASE( OrderSensitive )
{
    // Folding (a then b) differs from (b then a) for distinct operands.
    const std::size_t a = 7u;
    const std::size_t b = 13u;

    BOOST_CHECK( KiHashCombine( KiHashCombine( 0u, a ), b )
                 != KiHashCombine( KiHashCombine( 0u, b ), a ) );
}


BOOST_AUTO_TEST_SUITE_END()
