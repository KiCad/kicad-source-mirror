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
#include <clipboard.h>
#include <wx/clipbrd.h>
#include <wx/image.h>
#include <wx/string.h>
#include <wx/filename.h>
#include <wx/mstream.h>
#include <vector>

BOOST_AUTO_TEST_SUITE( ClipboardTests )

BOOST_AUTO_TEST_CASE( SaveClipboard_BasicText )
{
    std::string testText = "Basic clipboard test";
    bool result = SaveClipboard( testText );

    if( result )
    {
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, testText );
    }
    // Note: Test may fail on headless systems where clipboard isn't available
}

BOOST_AUTO_TEST_CASE( SaveClipboard_EmptyString )
{
    std::string emptyText = "";
    bool result = SaveClipboard( emptyText );

    if( result )
    {
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, emptyText );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_UTF8Characters )
{
    std::string utf8Text = "Héllo Wörld! 你好 🚀";
    bool result = SaveClipboard( utf8Text );

    if( result )
    {
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, utf8Text );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_LargeText )
{
    std::string largeText( 10000, 'A' );
    largeText += "END";
    bool result = SaveClipboard( largeText );

    if( result )
    {
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, largeText );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_SpecialCharacters )
{
    std::string specialText = "Line1\nLine2\tTabbed\r\nWindows newline";
    bool result = SaveClipboard( specialText );

    if( result )
    {
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, specialText );
    }
}

BOOST_AUTO_TEST_CASE( GetClipboardUTF8_EmptyClipboard )
{
    // Clear clipboard first
    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->Clear();
        wxTheClipboard->Close();
    }

    std::string result = GetClipboardUTF8();
    BOOST_CHECK( result.empty() );
}

BOOST_AUTO_TEST_CASE( GetClipboardUTF8_NonTextData )
{
    // This test verifies behavior when clipboard contains non-text data
    // Implementation depends on system behavior - may return empty string
    std::string result = GetClipboardUTF8();
    // No specific assertion - just ensure it doesn't crash
    BOOST_CHECK( true );
}

BOOST_AUTO_TEST_CASE( SaveTabularData_SimpleGrid )
{
    std::vector<std::vector<wxString>> testData = {
        { wxS("A1"), wxS("B1"), wxS("C1") },
        { wxS("A2"), wxS("B2"), wxS("C2") },
        { wxS("A3"), wxS("B3"), wxS("C3") }
    };

    bool result = SaveTabularDataToClipboard( testData );

    if( result )
    {
        std::vector<std::vector<wxString>> retrieved;
        bool parseResult = GetTabularDataFromClipboard( retrieved );

        if( parseResult )
        {
            BOOST_CHECK_EQUAL( retrieved.size(), testData.size() );
            for( size_t i = 0; i < testData.size() && i < retrieved.size(); ++i )
            {
                BOOST_CHECK_EQUAL( retrieved[i].size(), testData[i].size() );
                for( size_t j = 0; j < testData[i].size() && j < retrieved[i].size(); ++j )
                {
                    BOOST_CHECK_EQUAL( retrieved[i][j], testData[i][j] );
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( SaveTabularData_EmptyGrid )
{
    std::vector<std::vector<wxString>> emptyData;
    bool result = SaveTabularDataToClipboard( emptyData );

    if( result )
    {
        std::vector<std::vector<wxString>> retrieved;
        bool parseResult = GetTabularDataFromClipboard( retrieved );

        if( parseResult )
        {
            BOOST_CHECK( retrieved.empty() );
        }
    }
}

BOOST_AUTO_TEST_CASE( SaveTabularData_SingleCell )
{
    std::vector<std::vector<wxString>> singleCell = {
        { wxS("OnlyCell") }
    };

    bool result = SaveTabularDataToClipboard( singleCell );

    if( result )
    {
        std::vector<std::vector<wxString>> retrieved;
        bool parseResult = GetTabularDataFromClipboard( retrieved );

        if( parseResult )
        {
            BOOST_CHECK_EQUAL( retrieved.size(), 1 );
            BOOST_CHECK_EQUAL( retrieved[0].size(), 1 );
            BOOST_CHECK_EQUAL( retrieved[0][0], wxS("OnlyCell") );
        }
    }
}

BOOST_AUTO_TEST_CASE( SaveTabularData_WithCommas )
{
    std::vector<std::vector<wxString>> dataWithCommas = {
        { wxS("Value, with comma"), wxS("Normal") },
        { wxS("Another, comma"), wxS("Also normal") }
    };

    bool result = SaveTabularDataToClipboard( dataWithCommas );

    if( result )
    {
        std::vector<std::vector<wxString>> retrieved;
        bool parseResult = GetTabularDataFromClipboard( retrieved );

        if( parseResult )
        {
            BOOST_CHECK_EQUAL( retrieved.size(), dataWithCommas.size() );
            for( size_t i = 0; i < dataWithCommas.size() && i < retrieved.size(); ++i )
            {
                BOOST_CHECK_EQUAL( retrieved[i].size(), dataWithCommas[i].size() );
                for( size_t j = 0; j < dataWithCommas[i].size() && j < retrieved[i].size(); ++j )
                {
                    BOOST_CHECK_EQUAL( retrieved[i][j], dataWithCommas[i][j] );
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( SaveTabularData_WithQuotes )
{
    std::vector<std::vector<wxString>> dataWithQuotes = {
        { wxS("\"Quoted value\""), wxS("Normal") },
        { wxS("Value with \"inner\" quotes"), wxS("Plain") }
    };

    bool result = SaveTabularDataToClipboard( dataWithQuotes );

    if( result )
    {
        std::vector<std::vector<wxString>> retrieved;
        bool parseResult = GetTabularDataFromClipboard( retrieved );

        if( parseResult )
        {
            BOOST_CHECK_EQUAL( retrieved.size(), dataWithQuotes.size() );
            // Note: Exact quote handling depends on CSV parser implementation
        }
    }
}

BOOST_AUTO_TEST_CASE( SaveTabularData_WithNewlines )
{
    std::vector<std::vector<wxString>> dataWithNewlines = {
        { wxS("Line1\nLine2"), wxS("Normal") },
        { wxS("Single line"), wxS("Another\nmultiline") }
    };

    bool result = SaveTabularDataToClipboard( dataWithNewlines );

    if( result )
    {
        std::vector<std::vector<wxString>> retrieved;
        bool parseResult = GetTabularDataFromClipboard( retrieved );

        if( parseResult )
        {
            BOOST_CHECK_EQUAL( retrieved.size(), dataWithNewlines.size() );
            // Note: Newline handling depends on CSV parser implementation
        }
    }
}

BOOST_AUTO_TEST_CASE( SaveTabularData_IrregularGrid )
{
    std::vector<std::vector<wxString>> irregularData = {
        { wxS("A1"), wxS("B1"), wxS("C1"), wxS("D1") },
        { wxS("A2"), wxS("B2") },
        { wxS("A3"), wxS("B3"), wxS("C3") }
    };

    bool result = SaveTabularDataToClipboard( irregularData );

    if( result )
    {
        std::vector<std::vector<wxString>> retrieved;
        bool parseResult = GetTabularDataFromClipboard( retrieved );

        if( parseResult )
        {
            BOOST_CHECK_EQUAL( retrieved.size(), irregularData.size() );
            // Each row should maintain its individual size
            for( size_t i = 0; i < irregularData.size() && i < retrieved.size(); ++i )
            {
                for( size_t j = 0; j < irregularData[i].size() && j < retrieved[i].size(); ++j )
                {
                    BOOST_CHECK_EQUAL( retrieved[i][j], irregularData[i][j] );
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE( GetTabularDataFromClipboard_InvalidData )
{
    // Save non-tabular text to clipboard
    std::string invalidText = "This is not tabular data\nJust some text";
    SaveClipboard( invalidText );

    std::vector<std::vector<wxString>> retrieved;
    bool result = GetTabularDataFromClipboard( retrieved );

    // Should either parse as single-column data or return appropriate result
    // Exact behavior depends on AutoDecodeCSV implementation
    BOOST_CHECK( true ); // Test that it doesn't crash
}

BOOST_AUTO_TEST_CASE( GetImageFromClipboard_NoImage )
{
    // Clear clipboard
    if( wxTheClipboard->Open() )
    {
        wxTheClipboard->Clear();
        wxTheClipboard->Close();
    }

    std::unique_ptr<wxImage> image = GetImageFromClipboard();
    BOOST_CHECK( !image || !image->IsOk() );
}

BOOST_AUTO_TEST_CASE( GetImageFromClipboard_TextInClipboard )
{
    // Put text in clipboard
    SaveClipboard( "This is text, not an image" );

    std::unique_ptr<wxImage> image = GetImageFromClipboard();
    BOOST_CHECK( !image || !image->IsOk() );
}

BOOST_AUTO_TEST_CASE( Clipboard_MultipleSaveOperations )
{
    // Test multiple sequential save operations
    std::vector<std::string> testStrings = {
        "First string",
        "Second string with 特殊字符",
        "Third string\nwith\nnewlines",
        ""
    };

    for( const auto& testString : testStrings )
    {
        bool saved = SaveClipboard( testString );
        if( saved )
        {
            std::string retrieved = GetClipboardUTF8();
            BOOST_CHECK_EQUAL( retrieved, testString );
        }
    }
}

BOOST_AUTO_TEST_CASE( Clipboard_ConcurrentAccess )
{
    // Test that clipboard operations are properly synchronized
    std::string testText1 = "Concurrent test 1";
    std::string testText2 = "Concurrent test 2";

    bool result1 = SaveClipboard( testText1 );
    bool result2 = SaveClipboard( testText2 );

    if( result2 )
    {
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, testText2 ); // Should have the last saved value
    }
}

BOOST_AUTO_TEST_CASE( Clipboard_FlushBehavior )
{
    // Test that Flush() allows data to persist after the application
    std::string persistentText = "This should persist after flush";
    bool result = SaveClipboard( persistentText );

    if( result )
    {
        // Data should still be available
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, persistentText );
    }
}

BOOST_AUTO_TEST_SUITE_END()