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

BOOST_AUTO_TEST_CASE( ConvertPathToFileUri_UnixAbsolutePath )
{
    // /tmp exists on all Unix CI machines
    BOOST_TEST( ConvertPathToFileUri( wxS( "/tmp" ), nullptr ) == wxString( "file:///tmp" ) );
}

BOOST_AUTO_TEST_CASE( ConvertPathToFileUri_AlreadyHasScheme )
{
    BOOST_TEST( ConvertPathToFileUri( wxS( "https://example.com" ), nullptr ) == wxString( "https://example.com" ) );
    BOOST_TEST( ConvertPathToFileUri( wxS( "file:///path" ), nullptr ) == wxString( "file:///path" ) );
}

BOOST_AUTO_TEST_CASE( ConvertPathToFileUri_Empty )
{
    BOOST_TEST( ConvertPathToFileUri( wxEmptyString, nullptr ) == wxString( "" ) );
}

BOOST_AUTO_TEST_CASE( ConvertPathToFileUri_Tilde )
{
    BOOST_TEST( ConvertPathToFileUri( wxS( "~" ), nullptr ) == wxString( "~" ) );
}

BOOST_AUTO_TEST_CASE( ConvertPathToFileUri_NonexistentPath )
{
    BOOST_TEST( ConvertPathToFileUri( wxS( "/nonexistent/file.pdf" ), nullptr )
                == wxString( "/nonexistent/file.pdf" ) );
}

BOOST_AUTO_TEST_CASE( ConvertPathToFileUri_PlainText )
{
    // Non-path strings should pass through unchanged
    BOOST_TEST( ConvertPathToFileUri( wxS( "LM358" ), nullptr ) == wxString( "LM358" ) );
    BOOST_TEST( ConvertPathToFileUri( wxS( "100nF" ), nullptr ) == wxString( "100nF" ) );
}


// Helper: format a vector of pin numbers for diagnostics.
static wxString JoinPins( const std::vector<wxString>& aPins )
{
    wxString out;

    for( const wxString& p : aPins )
        out << '<' << p << '>';

    return out;
}


// A plain pin number (no brackets) is always a single pin, even when it contains the characters
// that are structural inside stacked notation.  This is the root cause of issue 23782's second
// bug, where a comma-containing pin number was mistaken for stacked notation.
BOOST_AUTO_TEST_CASE( StackedPinNotation_CommaInPlainNumberIsSinglePin )
{
    bool valid = false;

    std::vector<wxString> expanded = ExpandStackedPinNotation( wxS( "1,foo,bar,buz" ), &valid );
    BOOST_TEST( valid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 1u );
    BOOST_TEST( expanded[0] == wxString( "1,foo,bar,buz" ) );

    BOOST_TEST( CountStackedPinNotation( wxS( "1,foo,bar,buz" ), &valid ) == 1 );
    BOOST_TEST( valid );
}


// A pin number whose literal text contains a comma must survive a round-trip through stacked
// notation when escaped, instead of being split into several phantom pins (issue 23782, bug 1).
BOOST_AUTO_TEST_CASE( StackedPinNotation_EscapedCommaRoundTrips )
{
    const wxString raw = wxS( "1,foo" );
    const wxString escaped = EscapeStackedPinItem( raw );

    BOOST_TEST( escaped == wxString( "1\\,foo" ) );

    const wxString notation = wxS( "[" ) + escaped + wxS( ",2]" );

    bool                  valid = false;
    std::vector<wxString> expanded = ExpandStackedPinNotation( notation, &valid );

    BOOST_TEST( valid );
    BOOST_TEST_INFO( "expanded: " << JoinPins( expanded ) );
    BOOST_REQUIRE_EQUAL( expanded.size(), 2u );
    BOOST_TEST( expanded[0] == raw );
    BOOST_TEST( expanded[1] == wxString( "2" ) );

    BOOST_TEST( CountStackedPinNotation( notation, &valid ) == 2 );
    BOOST_TEST( valid );
}


// An escaped dash is a literal character, not a range separator.
BOOST_AUTO_TEST_CASE( StackedPinNotation_EscapedDashIsLiteral )
{
    const wxString notation = wxS( "[A\\-B,C]" );

    bool                  valid = false;
    std::vector<wxString> expanded = ExpandStackedPinNotation( notation, &valid );

    BOOST_TEST( valid );
    BOOST_TEST_INFO( "expanded: " << JoinPins( expanded ) );
    BOOST_REQUIRE_EQUAL( expanded.size(), 2u );
    BOOST_TEST( expanded[0] == wxString( "A-B" ) );
    BOOST_TEST( expanded[1] == wxString( "C" ) );

    // An unescaped dash still forms a range.
    expanded = ExpandStackedPinNotation( wxS( "[1-3]" ), &valid );
    BOOST_TEST( valid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 3u );
    BOOST_TEST( expanded[0] == wxString( "1" ) );
    BOOST_TEST( expanded[2] == wxString( "3" ) );
}


// Escaping characters that are not special leaves them untouched, and all special characters are
// escaped exactly once.
BOOST_AUTO_TEST_CASE( StackedPinNotation_EscapeStackedPinItem )
{
    BOOST_TEST( EscapeStackedPinItem( wxS( "A1" ) ) == wxString( "A1" ) );
    BOOST_TEST( EscapeStackedPinItem( wxS( "a,b" ) ) == wxString( "a\\,b" ) );
    BOOST_TEST( EscapeStackedPinItem( wxS( "a-b" ) ) == wxString( "a\\-b" ) );
    BOOST_TEST( EscapeStackedPinItem( wxS( "[x]" ) ) == wxString( "\\[x\\]" ) );
    BOOST_TEST( EscapeStackedPinItem( wxS( "a\\b" ) ) == wxString( "a\\\\b" ) );
}


// Existing, unescaped notation must continue to behave exactly as before (regression guard).
BOOST_AUTO_TEST_CASE( StackedPinNotation_LegacyNotationUnchanged )
{
    bool valid = false;

    std::vector<wxString> expanded = ExpandStackedPinNotation( wxS( "[6,7,9-11]" ), &valid );
    BOOST_TEST( valid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 5u );
    BOOST_TEST( expanded[0] == wxString( "6" ) );
    BOOST_TEST( expanded[1] == wxString( "7" ) );
    BOOST_TEST( expanded[2] == wxString( "9" ) );
    BOOST_TEST( expanded[3] == wxString( "10" ) );
    BOOST_TEST( expanded[4] == wxString( "11" ) );

    BOOST_TEST( CountStackedPinNotation( wxS( "[6,7,9-11]" ), &valid ) == 5 );
    BOOST_TEST( valid );
}


// The display splitter preserves range items and unescapes literal items.
BOOST_AUTO_TEST_CASE( StackedPinNotation_DisplaySplitPreservesRanges )
{
    std::vector<wxString> items = SplitStackedPinDisplayItems( wxS( "1-5,a\\,b,7" ) );

    BOOST_TEST_INFO( "items: " << JoinPins( items ) );
    BOOST_REQUIRE_EQUAL( items.size(), 3u );
    BOOST_TEST( items[0] == wxString( "1-5" ) );
    BOOST_TEST( items[1] == wxString( "a,b" ) );
    BOOST_TEST( items[2] == wxString( "7" ) );
}


// A backslash that does not precede a structural character is a literal character, so legacy
// notation that contained an un-escaped backslash keeps it.
BOOST_AUTO_TEST_CASE( StackedPinNotation_LiteralBackslashPreserved )
{
    bool valid = false;

    std::vector<wxString> expanded = ExpandStackedPinNotation( wxS( "[A\\B,C]" ), &valid );
    BOOST_TEST( valid );
    BOOST_TEST_INFO( "expanded: " << JoinPins( expanded ) );
    BOOST_REQUIRE_EQUAL( expanded.size(), 2u );
    BOOST_TEST( expanded[0] == wxString( "A\\B" ) );
    BOOST_TEST( expanded[1] == wxString( "C" ) );

    // A backslash before a structural character round-trips through escape/unescape.
    BOOST_TEST( EscapeStackedPinItem( wxS( "A\\B" ) ) == wxString( "A\\\\B" ) );
}


// An empty pin number is a single, valid pin regardless of whether validity is requested.
BOOST_AUTO_TEST_CASE( StackedPinNotation_EmptyIsSinglePin )
{
    bool valid = false;

    BOOST_TEST( CountStackedPinNotation( wxEmptyString, &valid ) == 1 );
    BOOST_TEST( valid );

    BOOST_TEST( CountStackedPinNotation( wxEmptyString, nullptr ) == 1 );

    std::vector<wxString> expanded = ExpandStackedPinNotation( wxEmptyString, &valid );
    BOOST_TEST( valid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 1u );
    BOOST_TEST( expanded[0] == wxEmptyString );
}

BOOST_AUTO_TEST_SUITE_END()
