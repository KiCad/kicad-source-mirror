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

#include <core/base64.h>

#include <cstdint>
#include <string>
#include <vector>


static std::vector<uint8_t> bytes( const std::string& aText )
{
    return std::vector<uint8_t>( aText.begin(), aText.end() );
}


static std::string decoded( const std::string& aBase64 )
{
    std::vector<uint8_t> out;
    base64::decode( bytes( aBase64 ), out );
    return std::string( out.begin(), out.end() );
}


BOOST_AUTO_TEST_SUITE( Base64 )


BOOST_AUTO_TEST_CASE( DecodeKnownVectors )
{
    BOOST_CHECK_EQUAL( decoded( "" ), "" );
    BOOST_CHECK_EQUAL( decoded( "TWFu" ), "Man" );
    BOOST_CHECK_EQUAL( decoded( "TWE" ), "Ma" );
    BOOST_CHECK_EQUAL( decoded( "TQ" ), "M" );
    BOOST_CHECK_EQUAL( decoded( "TWE=" ), "Ma" );
    BOOST_CHECK_EQUAL( decoded( "TQ==" ), "M" );
    BOOST_CHECK_EQUAL( decoded( "aGVsbG8gd29ybGQ" ), "hello world" );
}


BOOST_AUTO_TEST_CASE( RoundTrip )
{
    std::vector<uint8_t> input;

    for( int len = 0; len < 64; len++ )
    {
        std::vector<uint8_t> encoded;
        std::vector<uint8_t> output;

        base64::encode( input, encoded );
        base64::decode( encoded, output );

        BOOST_CHECK_EQUAL_COLLECTIONS( output.begin(), output.end(), input.begin(), input.end() );

        input.push_back( static_cast<uint8_t>( len * 37 + 11 ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
