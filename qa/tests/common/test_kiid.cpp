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


BOOST_AUTO_TEST_SUITE_END()
