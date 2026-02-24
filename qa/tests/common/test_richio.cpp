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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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


BOOST_AUTO_TEST_SUITE_END()
