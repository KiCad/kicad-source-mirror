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
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include "base_set.h"

BOOST_AUTO_TEST_SUITE( BaseSetTests )

BOOST_AUTO_TEST_CASE( ConstructionAndSize )
{
    BASE_SET bs( 10 );
    BOOST_CHECK_EQUAL( bs.size(), 10 );
    BOOST_CHECK_EQUAL( bs.count(), 0 );

    bs.resize( 20 );
    BOOST_CHECK_EQUAL( bs.size(), 20 );
    BOOST_CHECK_EQUAL( bs.count(), 0 );
}

BOOST_AUTO_TEST_CASE( BitSettingAndResetting )
{
    BASE_SET bs( 10 );
    bs.set( 2 );
    BOOST_CHECK( bs.test( 2 ) );
    BOOST_CHECK_EQUAL( bs.count(), 1 );

    bs.reset( 2 );
    BOOST_CHECK( !bs.test( 2 ) );
    BOOST_CHECK_EQUAL( bs.count(), 0 );
}

BOOST_AUTO_TEST_CASE( SetOutOfRange )
{
    BASE_SET bs( 10 );
    BOOST_CHECK_EQUAL( bs.size(), 10 );
    BOOST_CHECK_EQUAL( bs.count(), 0 );

    bs.set( 10 );
    BOOST_CHECK_EQUAL( bs.size(), 11 );
    BOOST_CHECK_EQUAL( bs.count(), 1 );

    bs.reset( 10 );
    BOOST_CHECK_EQUAL( bs.size(), 11 );
    BOOST_CHECK_EQUAL( bs.count(), 0 );

    bs.reset( 20 );
    BOOST_CHECK_EQUAL( bs.size(), 21 );
    BOOST_CHECK_EQUAL( bs.count(), 0 );
}

BOOST_AUTO_TEST_CASE( IteratingSetBits )
{
    BASE_SET bs( 10 );
    bs.set( 2 );
    bs.set( 4 );

    auto it = bs.set_bits_begin();
    BOOST_CHECK_EQUAL( *it, 2 );
    ++it;
    BOOST_CHECK_EQUAL( *it, 4 );
    ++it;
    BOOST_CHECK( it == bs.set_bits_end() );

    // Custom reverse iterator test
    std::vector<size_t> reverse_set_bits;
    for( auto rit = bs.set_bits_rbegin(); rit != bs.set_bits_rend(); ++rit )
    {
        reverse_set_bits.push_back( *rit );
    }

    BOOST_CHECK_EQUAL( reverse_set_bits.size(), 2 );
    BOOST_CHECK_EQUAL( reverse_set_bits[0], 4 );
    BOOST_CHECK_EQUAL( reverse_set_bits[1], 2 );
}

// Test equality operator
BOOST_AUTO_TEST_CASE( BASE_SETEqualityOperator )
{
    BASE_SET set1( 10 );
    BASE_SET set2( 10 );
    BASE_SET set3( 15 );

    set1.set( 2 );
    set1.set( 4 );
    set2.set( 2 );
    set2.set( 4 );
    set3.set( 2 );

    BOOST_CHECK( set1 == set2 );
    BOOST_CHECK( !( set1 == set3 ) );
    BOOST_CHECK( !( set2 == set3 ) );
}

// Test less-than operator
BOOST_AUTO_TEST_CASE(BASE_SETComparisonOperator)
{
    BASE_SET set1( 10 );
    BASE_SET set2( 10 );
    BASE_SET set3( 15 );

    set1.set( 2 );
    set1.set( 5 );
    set2.set( 2 );
    set3.set( 2 );

    BOOST_CHECK( set3 < set1 ); // Although set3 is larger, set1 has a 1 at position 5, so set3 is less
    BOOST_CHECK( set2 < set3 ); // set2 and set3 both have the same values set but set3 has more positions available
    BOOST_CHECK( !( set1 < set3 ) );
    BOOST_CHECK( !( set1 < set2 ) ); // Although sizes are equal, elements in set2 are subsets of set1
}

// Test compare function
BOOST_AUTO_TEST_CASE(BASE_SETCompareFunction)
{
    BASE_SET set1( 10 );
    BASE_SET set2( 10 );
    BASE_SET set3( 10 );
    BASE_SET set4( 15 );

    set1.set( 2 );
    set1.set( 4 );

    set2.set( 2 );
    set2.set( 4 );

    set3.set( 2 );

    set4.set( 2 );

    BOOST_CHECK_EQUAL( set1.compare( set2 ), 0 ); // set1 and set2 are equal
    BOOST_CHECK_EQUAL( set1.compare( set3 ), 1 ); // set1 is greater than set3
    BOOST_CHECK_EQUAL( set3.compare( set1 ), -1 ); // set3 is less than set1
    BOOST_CHECK_EQUAL( set3.compare( set4 ), -1 ); // set3 is less than set4
    BOOST_CHECK_EQUAL( set4.compare( set3 ), 1 ); // set4 is greater than set3
}


// Test boolean operator&=
BOOST_AUTO_TEST_CASE(BASE_SETAndAssignment)
{
    BASE_SET bs1( 10 );
    BASE_SET bs2( 10 );
    bs1.set( 1 );
    bs1.set( 3 );
    bs2.set( 2 );
    bs2.set( 3 );

    bs1 &= bs2;
    BOOST_CHECK_EQUAL( bs1.test( 1 ), false );
    BOOST_CHECK_EQUAL( bs1.test( 2 ), false );
    BOOST_CHECK_EQUAL( bs1.test( 3 ), true );
}

// Test boolean operator|=
BOOST_AUTO_TEST_CASE(BASE_SETOrAssignment)
{
    BASE_SET bs1( 10 );
    BASE_SET bs2( 10 );
    bs1.set( 1 );
    bs2.set( 2 );
    bs2.set( 3 );

    bs1 |= bs2;
    BOOST_CHECK_EQUAL( bs1.test( 1 ), true );
    BOOST_CHECK_EQUAL( bs1.test( 2 ), true );
    BOOST_CHECK_EQUAL( bs1.test( 3 ), true );
}

// Test boolean operator^=
BOOST_AUTO_TEST_CASE(BASE_SETXorAssignment)
{
    BASE_SET bs1( 10 );
    BASE_SET bs2( 10 );
    bs1.set( 1 );
    bs1.set( 3 );
    bs2.set( 2 );
    bs2.set( 3 );

    bs1 ^= bs2;
    BOOST_CHECK_EQUAL( bs1.test( 1 ), true );
    BOOST_CHECK_EQUAL( bs1.test( 2 ), true );
    BOOST_CHECK_EQUAL( bs1.test( 3 ), false );
}

// Test boolean operator~
BOOST_AUTO_TEST_CASE(BASE_SETNotOperator)
{
    BASE_SET bs1( 4 );
    bs1.set( 1 );
    bs1.set( 3 );
    BASE_SET bs2 = ~bs1;

    BOOST_CHECK_EQUAL( bs2.test( 0 ), true );
    BOOST_CHECK_EQUAL( bs2.test( 1 ), false );
    BOOST_CHECK_EQUAL( bs2.test( 2 ), true );
    BOOST_CHECK_EQUAL( bs2.test( 3 ), false );
}

// Test non-member operator&
BOOST_AUTO_TEST_CASE(BASE_SETAndOperator)
{
    BASE_SET bs1( 10 );
    BASE_SET bs2( 10 );
    bs1.set( 1 );
    bs1.set( 3 );
    bs2.set( 2 );
    bs2.set( 3 );

    BASE_SET result = bs1 & bs2;
    BOOST_CHECK_EQUAL( result.test( 1 ), false );
    BOOST_CHECK_EQUAL( result.test( 2 ), false );
    BOOST_CHECK_EQUAL( result.test( 3 ), true );
}

// Test non-member operator|
BOOST_AUTO_TEST_CASE(BASE_SETOrOperator)
{
    BASE_SET bs1( 10 );
    BASE_SET bs2( 10 );
    bs1.set( 1 );
    bs2.set( 2 );
    bs2.set( 3 );

    BASE_SET result = bs1 | bs2;
    BOOST_CHECK_EQUAL( result.test( 1 ), true );
    BOOST_CHECK_EQUAL( result.test( 2 ), true );
    BOOST_CHECK_EQUAL( result.test( 3 ), true );
}

// Test non-member operator^
BOOST_AUTO_TEST_CASE(BASE_SETXorOperator)
{
    BASE_SET bs1( 10 );
    BASE_SET bs2( 10 );
    bs1.set( 1 );
    bs1.set( 3 );
    bs2.set( 2 );
    bs2.set( 3 );

    BASE_SET result = bs1 ^ bs2;
    BOOST_CHECK_EQUAL( result.test( 1 ), true );
    BOOST_CHECK_EQUAL( result.test( 2 ), true );
    BOOST_CHECK_EQUAL( result.test( 3 ), false );
}

// Test std::hash specialization
BOOST_AUTO_TEST_CASE( BASE_SETHash )
{
    BASE_SET bs1( 10 );
    bs1.set( 1 );
    bs1.set( 3 );

    std::hash<BASE_SET> hashFn;
    size_t              hash = hashFn( bs1 );

    BASE_SET bs2( 10 );
    bs2.set( 1 );
    bs2.set( 3 );

    BOOST_CHECK_EQUAL( hash, hashFn( bs2 ) );

    bs2.set( 2 );
    BOOST_CHECK_NE( hash, hashFn( bs2 ) );
}

BOOST_AUTO_TEST_SUITE_END()
