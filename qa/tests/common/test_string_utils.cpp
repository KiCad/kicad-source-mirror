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
#include <string_utils.h>

BOOST_AUTO_TEST_SUITE( StringUtilsTests )

// std::string overload: no illegal chars -> returns false and string unchanged
BOOST_AUTO_TEST_CASE( ReplaceIllegalFileNameChars_StdString_NoChange )
{
    std::string name = "valid_filename-123._ok";
    std::string original = name;

    bool changed = ReplaceIllegalFileNameChars( name );

    BOOST_TEST( changed == false );
    BOOST_TEST( name == original );
}

// std::string overload: with illegal chars, default behavior -> percent-hex encoding
BOOST_AUTO_TEST_CASE( ReplaceIllegalFileNameChars_StdString_PercentEncoding )
{
    // Contains several illegal chars: \ / ? * | " < >
    std::string name = R"(bad\name/with?illegal*chars|"<>.txt)";
    bool        changed = ReplaceIllegalFileNameChars( name ); // aReplaceChar defaults to 0 -> percent-hex

    BOOST_TEST( changed == true );

    // Illegal characters should have been hex-encoded
    BOOST_TEST( name.find( "%5c" ) != std::string::npos ); // backslash
    BOOST_TEST( name.find( "%2f" ) != std::string::npos ); // forward slash
    BOOST_TEST( name.find( "%3f" ) != std::string::npos ); // question mark
    BOOST_TEST( name.find( "%2a" ) != std::string::npos ); // asterisk
    BOOST_TEST( name.find( "%7c" ) != std::string::npos ); // vertical bar
    BOOST_TEST( name.find( "%22" ) != std::string::npos ); // double quote
    BOOST_TEST( name.find( "%3c" ) != std::string::npos ); // less-than
    BOOST_TEST( name.find( "%3e" ) != std::string::npos ); // greater-than

    // Non-illegal characters should remain unchanged
    BOOST_TEST( name.find( ".txt" ) != std::string::npos );
    BOOST_TEST( name.find( "bad" ) != std::string::npos );
}

// std::string overload: replacement character provided -> use that character
BOOST_AUTO_TEST_CASE( ReplaceIllegalFileNameChars_StdString_ReplacementChar )
{
    std::string name = R"(a*bad?name|file)";
    bool        changed = ReplaceIllegalFileNameChars( name, '_' ); // replace illegal with underscore

    BOOST_TEST( changed == true );
    BOOST_TEST( name == "a_bad_name_file" );
}

// wxString overload: no illegal chars -> returns false and string unchanged
BOOST_AUTO_TEST_CASE( ReplaceIllegalFileNameChars_WxString_NoChange )
{
    wxString name = "valid_filename-123._ok";
    wxString original = name;

    bool changed = ReplaceIllegalFileNameChars( name );

    BOOST_TEST( changed == false );
    BOOST_TEST( name == original );
}

// wxString overload: percent-hex encoding when aReplaceChar == 0
BOOST_AUTO_TEST_CASE( ReplaceIllegalFileNameChars_WxString_PercentEncoding )
{
    wxString name = "bad\\name/with?illegal*chars|\"<>.txt";
    bool     changed = ReplaceIllegalFileNameChars( name ); // default -> percent-hex

    BOOST_TEST( changed == true );

    // Illegal characters should have been hex-encoded
    BOOST_TEST( name.Contains( "%5c" ) ); // backslash
    BOOST_TEST( name.Contains( "%2f" ) ); // forward slash
    BOOST_TEST( name.Contains( "%3f" ) ); // question mark
    BOOST_TEST( name.Contains( "%2a" ) ); // asterisk
    BOOST_TEST( name.Contains( "%7c" ) ); // vertical bar
    BOOST_TEST( name.Contains( "%22" ) ); // double quote
    BOOST_TEST( name.Contains( "%3c" ) ); // less-than
    BOOST_TEST( name.Contains( "%3e" ) ); // greater-than

    // Non-illegal characters should remain unchanged
    BOOST_TEST( name.Contains( ".txt" ) );
    BOOST_TEST( name.Contains( "bad" ) );
}

// wxString overload: replacement character provided -> use that character
BOOST_AUTO_TEST_CASE( ReplaceIllegalFileNameChars_WxString_ReplacementChar )
{
    wxString name = "a*bad?name|file";
    bool     changed = ReplaceIllegalFileNameChars( name, '_' );

    BOOST_TEST( changed == true );
    BOOST_TEST( name == "a_bad_name_file" );
}

BOOST_AUTO_TEST_SUITE_END()