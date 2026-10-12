/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>

#include <vector>

#include <ki_exception.h>


BOOST_AUTO_TEST_SUITE( IOErrorTest )


BOOST_AUTO_TEST_CASE( WhatRetainsUtf8Message )
{
    const wxString message = wxString::FromUTF8( "Could not read \xe2\x80\x9c"
                                                 "board"
                                                 "\xe2\x80\x9d" );
    IO_ERROR       error( message, __FILE__, __FUNCTION__, __LINE__ );
    const char*    what = error.what();
    std::string    expected = message.utf8_string();

    std::vector<std::string> allocations( 1000, expected );

    BOOST_CHECK_EQUAL( allocations.front(), expected );
    BOOST_CHECK_EQUAL( std::string( what ), expected );
}


BOOST_AUTO_TEST_CASE( DerivedErrorsUpdateWhatMessage )
{
    PARSE_ERROR parseError( wxS( "Invalid token" ), __FILE__, __FUNCTION__, __LINE__, wxS( "example.kicad_pcb" ),
                            "invalid", 10, 4 );
    BOOST_CHECK_EQUAL( std::string( parseError.what() ), parseError.Problem().utf8_string() );

    FUTURE_FORMAT_ERROR futureError( parseError, wxS( "20261012" ) );
    BOOST_CHECK_EQUAL( std::string( futureError.what() ), futureError.Problem().utf8_string() );
}


BOOST_AUTO_TEST_SUITE_END()
