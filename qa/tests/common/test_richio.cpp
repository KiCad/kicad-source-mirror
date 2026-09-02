/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * @file
 * Test suite for RICHIO and related formatting utilities
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <richio.h>
#include <io/kicad/kicad_io_utils.h>

#define wxUSE_BASE64 1
#include <wx/base64.h>
#include <wx/mstream.h>


BOOST_AUTO_TEST_SUITE( RichIO )


/**
 * Verify that Prettify produces well-formed output for large (data ...) blocks such as
 * base64-encoded images, and that STRING_LINE_READER can read the result.
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23162
 */
BOOST_AUTO_TEST_CASE( PrettifyLargeImageData )
{
    STRING_FORMATTER fmt;

    fmt.Print( "(kicad_sch (version 20231120) (generator \"eeschema\")" );
    fmt.Print( "(image (at 0 0)" );

    const size_t imageSize = 2 * 1024 * 1024;
    std::vector<uint8_t> fakeImage( imageSize, 0x42 );

    wxMemoryOutputStream stream;
    stream.Write( fakeImage.data(), fakeImage.size() );

    KICAD_FORMAT::FormatStreamData( fmt, *stream.GetOutputStreamBuffer() );

    fmt.Print( ")" );  // close image
    fmt.Print( ")" );  // close kicad_sch

    std::string buf = fmt.GetString();

    KICAD_FORMAT::Prettify( buf );

    BOOST_CHECK_NO_THROW(
    {
        STRING_LINE_READER reader( buf, "test" );

        while( reader.ReadLine() )
        {
            // just consume
        }
    } );
}


/**
 * Verify that STRING_LINE_READER can handle prettified output containing a very long
 * quoted string (e.g. a property value that exceeds the old 1 MB limit).
 *
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23162
 */
BOOST_AUTO_TEST_CASE( PrettifyLongQuotedString )
{
    const size_t longLen = 1100000;
    std::string longValue( longLen, 'A' );

    STRING_FORMATTER fmt;

    fmt.Print( "(kicad_sch (version 20231120)" );
    fmt.Print( "(property \"Description\" %s (at 0 0 0))",
               fmt.Quotes( longValue ).c_str() );
    fmt.Print( ")" );

    std::string buf = fmt.GetString();

    KICAD_FORMAT::Prettify( buf );

    BOOST_CHECK_NO_THROW(
    {
        STRING_LINE_READER reader( buf, "test" );

        while( reader.ReadLine() )
        {
            // just consume
        }
    } );
}


/**
 * Indent writes two spaces per nesting level and nothing else.  Callers that want only the
 * indentation, such as DSN::WIRE::Format re-indenting a closing paren, depend on the exact width.
 */
BOOST_AUTO_TEST_CASE( IndentWritesTwoSpacesPerLevel )
{
    for( int nestLevel : { 0, 1, 2, 7 } )
    {
        STRING_FORMATTER fmt;

        const int written = fmt.Indent( nestLevel );

        BOOST_CHECK_EQUAL( fmt.GetString(), std::string( 2 * nestLevel, ' ' ) );
        BOOST_CHECK_EQUAL( written, 2 * nestLevel );
    }
}


/**
 * The nesting Print() overload prepends that same indentation to its formatted output.
 */
BOOST_AUTO_TEST_CASE( NestedPrintIndentsItsOutput )
{
    STRING_FORMATTER fmt;

    const int written = fmt.Print( 3, "(net %d)", 42 );

    BOOST_CHECK_EQUAL( fmt.GetString(), "      (net 42)" );
    BOOST_CHECK_EQUAL( written, 14 );
}


BOOST_AUTO_TEST_SUITE_END()
