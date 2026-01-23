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
#include <kiid.h>
#include <wx/string.h>
#include <set>

BOOST_AUTO_TEST_SUITE( Kiid )


BOOST_AUTO_TEST_CASE( Seeding )
{
    KIID::SeedGenerator( 0l );

    KIID a0;
    KIID b0;
    KIID c0;
    KIID d0;

    // Make sure the hashes are unique
    std::set<size_t> hashes;
    hashes.insert( a0.Hash() );
    hashes.insert( b0.Hash() );
    hashes.insert( c0.Hash() );
    hashes.insert( d0.Hash() );
    BOOST_CHECK_EQUAL( hashes.size(), 4 );

    KIID::SeedGenerator( 0l );

    KIID a;
    BOOST_CHECK_EQUAL( a.Hash(), a0.Hash() );

    KIID b;
    BOOST_CHECK_EQUAL( b.Hash(), b0.Hash() );

    KIID c;
    BOOST_CHECK_EQUAL( c.Hash(), c0.Hash() );

    KIID d;
    BOOST_CHECK_EQUAL( d.Hash(), d0.Hash() );
}


BOOST_AUTO_TEST_CASE( KiidPathTest )
{
    KIID a, b, c, d;

    KIID_PATH path1;
    KIID_PATH path2;

    path1.push_back( a );
    path1.push_back( b );
    path1.push_back( c );
    path1.push_back( d );

    path2.push_back( b );
    path2.push_back( c );
    path2.push_back( d );

    BOOST_CHECK( path1.EndsWith( path2 ) == true );
    BOOST_CHECK( path2.EndsWith( path1 ) == false );
}


BOOST_AUTO_TEST_CASE( LegacyTimestamp )
{
    timestamp_t ts_a = 0xAABBCCDD;
    timestamp_t ts_b = 0x00000012;

    wxString str_a( wxS( "AABBCCDD" ) );
    wxString str_b( wxS( "00000012" ) );

    KIID a( ts_a );
    KIID b( ts_b );

    BOOST_CHECK( a.AsLegacyTimestamp() == ts_a );
    BOOST_CHECK( a.AsLegacyTimestampString() == str_a );

    BOOST_CHECK( b.AsLegacyTimestamp() == ts_b );
    BOOST_CHECK( b.AsLegacyTimestampString() == str_b );

    BOOST_CHECK( KIID( str_a ).AsLegacyTimestamp() == ts_a );
    BOOST_CHECK( KIID( str_b ).AsLegacyTimestamp() == ts_b );

    BOOST_CHECK( KIID( str_a ).AsLegacyTimestampString() == str_a );
    BOOST_CHECK( KIID( str_b ).AsLegacyTimestampString() == str_b );
}


BOOST_AUTO_TEST_CASE( CombineDeterministic )
{
    KIID a, b, c;

    // Combining the same two KIIDs should always produce the same result
    KIID combined1 = KIID::Combine( a, b );
    KIID combined2 = KIID::Combine( a, b );
    BOOST_CHECK_EQUAL( combined1.AsString(), combined2.AsString() );

    // Combined result should be different from inputs
    BOOST_CHECK( combined1 != a );
    BOOST_CHECK( combined1 != b );

    // Different inputs should produce different outputs
    KIID combined3 = KIID::Combine( a, c );
    BOOST_CHECK( combined1 != combined3 );

    // Order matters for XOR with different values
    KIID combined4 = KIID::Combine( b, a );
    BOOST_CHECK_EQUAL( combined1.AsString(), combined4.AsString() );
}


BOOST_AUTO_TEST_CASE( CombineWithKnownValues )
{
    // Test with known UUID strings to verify XOR behavior
    KIID a( "00000000-0000-0000-0000-000000000001" );
    KIID b( "00000000-0000-0000-0000-000000000002" );

    KIID combined = KIID::Combine( a, b );

    // XOR of 0x01 and 0x02 = 0x03
    BOOST_CHECK_EQUAL( combined.AsString(), "00000000-0000-0000-0000-000000000003" );

    // Combining with self should give all zeros
    KIID selfCombined = KIID::Combine( a, a );
    BOOST_CHECK_EQUAL( selfCombined.AsString(), "00000000-0000-0000-0000-000000000000" );
}


BOOST_AUTO_TEST_CASE( CombineUniqueness )
{
    // Generate several random KIIDs and verify all combinations are unique.
    // XOR is commutative, so Combine(a,b) == Combine(b,a). We only count
    // unique unordered pairs, which gives n*(n-1)/2 combinations.
    std::vector<KIID> ids;

    for( int i = 0; i < 10; ++i )
        ids.push_back( KIID() );

    std::set<wxString> combinations;

    for( size_t i = 0; i < ids.size(); ++i )
    {
        for( size_t j = i + 1; j < ids.size(); ++j )
        {
            KIID combined = KIID::Combine( ids[i], ids[j] );
            combinations.insert( combined.AsString() );
        }
    }

    // All 45 unique unordered pairs (10*9/2) should produce unique results
    BOOST_CHECK_EQUAL( combinations.size(), 45 );
}


BOOST_AUTO_TEST_SUITE_END()
