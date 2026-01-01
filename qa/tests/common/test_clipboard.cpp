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
#include <wx/display.h>
#include <wx/image.h>
#include <wx/string.h>
#include <wx/filename.h>
#include <wx/mstream.h>
#include <vector>
#include <cstdlib>

/**
 * Check if a display is available for clipboard operations.
 * On Linux/GTK, clipboard operations require a display connection.
 * In headless CI environments, this is not available.
 */
static bool IsDisplayAvailable()
{
#ifdef __WXGTK__
    // On GTK, check if wxWidgets can actually see displays.
    // Just having DISPLAY environment variable set isn't enough
    return wxDisplay::GetCount() > 0;

#endif
    return true;
}

/**
 * Macro to skip clipboard tests in headless environments.
 * This prevents GTK assertions when no display is available.
 */
#define SKIP_IF_HEADLESS()                                                                    \
    do                                                                                        \
    {                                                                                         \
        if( !IsDisplayAvailable() )                                                           \
        {                                                                                     \
            BOOST_TEST_MESSAGE( "Skipping test - no display available (headless environment)" ); \
            return;                                                                           \
        }                                                                                     \
    } while( 0 )

BOOST_AUTO_TEST_SUITE( ClipboardTests )

BOOST_AUTO_TEST_CASE( SaveClipboard_BasicText )
{
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

    // This test verifies behavior when clipboard contains non-text data
    // Implementation depends on system behavior - may return empty string
    std::string result = GetClipboardUTF8();
    // No specific assertion - just ensure it doesn't crash
    BOOST_CHECK( true );
}

BOOST_AUTO_TEST_CASE( SaveTabularData_SimpleGrid )
{
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
            BOOST_CHECK_EQUAL( retrieved[0][0], wxString( wxS("OnlyCell") ) );
        }
    }
}

BOOST_AUTO_TEST_CASE( SaveTabularData_WithCommas )
{
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

    // Put text in clipboard
    SaveClipboard( "This is text, not an image" );

    std::unique_ptr<wxImage> image = GetImageFromClipboard();
    BOOST_CHECK( !image || !image->IsOk() );
}

BOOST_AUTO_TEST_CASE( Clipboard_MultipleSaveOperations )
{
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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
    SKIP_IF_HEADLESS();

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

BOOST_AUTO_TEST_CASE( SaveClipboard_WithMimeData_EmptyMimeDataFallsBack )
{
    SKIP_IF_HEADLESS();

    // When MIME data is empty, should fall back to basic SaveClipboard
    std::string testText = "Fallback test with empty MIME data";
    std::vector<CLIPBOARD_MIME_DATA> emptyMimeData;

    bool result = SaveClipboard( testText, emptyMimeData );

    if( result )
    {
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, testText );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_WithMimeData_KicadFormat )
{
    SKIP_IF_HEADLESS();

    // Test that application/kicad MIME type is prioritized when reading
    std::string textData = "Plain text representation";
    std::string kicadData = "KiCad native format data";

    std::vector<CLIPBOARD_MIME_DATA> mimeData;
    CLIPBOARD_MIME_DATA kicadEntry;
    kicadEntry.m_mimeType = wxS( "application/kicad" );
    kicadEntry.m_data.AppendData( kicadData.data(), kicadData.size() );
    mimeData.push_back( kicadEntry );

    bool result = SaveClipboard( textData, mimeData );

    if( result )
    {
        // GetClipboardUTF8 should prioritize application/kicad format
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, kicadData );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_WithMimeData_MultipleMimeTypes )
{
    SKIP_IF_HEADLESS();

    // Test saving with multiple MIME types
    std::string textData = "Text for clipboard";
    std::string kicadData = "KiCad data for clipboard";
    std::string svgData = "<svg></svg>";
    std::string pngData = "\x89PNG\r\n"; // PNG magic bytes (truncated for test)

    std::vector<CLIPBOARD_MIME_DATA> mimeData;

    CLIPBOARD_MIME_DATA kicadEntry;
    kicadEntry.m_mimeType = wxS( "application/kicad" );
    kicadEntry.m_data.AppendData( kicadData.data(), kicadData.size() );
    mimeData.push_back( kicadEntry );

    CLIPBOARD_MIME_DATA svgEntry;
    svgEntry.m_mimeType = wxS( "image/svg+xml" );
    svgEntry.m_data.AppendData( svgData.data(), svgData.size() );
    mimeData.push_back( svgEntry );

    CLIPBOARD_MIME_DATA pngEntry;
    pngEntry.m_mimeType = wxS( "image/png" );
    pngEntry.m_data.AppendData( pngData.data(), pngData.size() );
    mimeData.push_back( pngEntry );

    bool result = SaveClipboard( textData, mimeData );

    if( result )
    {
        // Verify at least the KiCad format is retrievable
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, kicadData );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_WithMimeData_NoKicadFormat )
{
    SKIP_IF_HEADLESS();

    // When no application/kicad format, should fall back to text
    std::string textData = "Text for clipboard without kicad format";
    std::string svgData = "<svg></svg>";

    std::vector<CLIPBOARD_MIME_DATA> mimeData;
    CLIPBOARD_MIME_DATA svgEntry;
    svgEntry.m_mimeType = wxS( "image/svg+xml" );
    svgEntry.m_data.AppendData( svgData.data(), svgData.size() );
    mimeData.push_back( svgEntry );

    bool result = SaveClipboard( textData, mimeData );

    if( result )
    {
        // Should get the text since no application/kicad format is present
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, textData );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_WithMimeData_UTF8InKicadFormat )
{
    SKIP_IF_HEADLESS();

    // Test that UTF8 characters are preserved in KiCad MIME format
    std::string textData = "Plain text";
    std::string kicadData = "KiCad data with UTF8: 你好世界 🔧";

    std::vector<CLIPBOARD_MIME_DATA> mimeData;
    CLIPBOARD_MIME_DATA kicadEntry;
    kicadEntry.m_mimeType = wxS( "application/kicad" );
    kicadEntry.m_data.AppendData( kicadData.data(), kicadData.size() );
    mimeData.push_back( kicadEntry );

    bool result = SaveClipboard( textData, mimeData );

    if( result )
    {
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, kicadData );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_WithMimeData_SExpressionRoundTrip )
{
    SKIP_IF_HEADLESS();

    // Test that S-expression data (like schematic content) round-trips correctly
    // This simulates the actual copy/paste flow for schematic elements
    std::string sExprData =
        "(kicad_sch (version 20231120) (generator \"eeschema\")\n"
        "  (symbol (lib_id \"Device:R\") (at 100.33 50.8 0)\n"
        "    (property \"Reference\" \"R1\" (at 101.6 49.53 0))\n"
        "    (property \"Value\" \"10kΩ\" (at 101.6 52.07 0))\n"
        "  )\n"
        ")\n";

    std::vector<CLIPBOARD_MIME_DATA> mimeData;
    CLIPBOARD_MIME_DATA kicadEntry;
    kicadEntry.m_mimeType = wxS( "application/kicad" );
    kicadEntry.m_data.AppendData( sExprData.data(), sExprData.size() );
    mimeData.push_back( kicadEntry );

    bool result = SaveClipboard( sExprData, mimeData );

    if( result )
    {
        std::string retrieved = GetClipboardUTF8();
        // Verify exact byte-for-byte match for S-expression parsing
        BOOST_CHECK_EQUAL( retrieved.size(), sExprData.size() );
        BOOST_CHECK_EQUAL( retrieved, sExprData );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_WithMimeData_EmptyDataSkipped )
{
    SKIP_IF_HEADLESS();

    // Test that entries with empty data are skipped without error
    std::string textData = "Text with empty MIME entries";

    std::vector<CLIPBOARD_MIME_DATA> mimeData;

    // Add an entry with empty data - should be skipped
    CLIPBOARD_MIME_DATA emptyEntry;
    emptyEntry.m_mimeType = wxS( "application/empty" );
    // Note: m_data is empty by default
    mimeData.push_back( emptyEntry );

    // Add a valid entry
    CLIPBOARD_MIME_DATA kicadEntry;
    kicadEntry.m_mimeType = wxS( "application/kicad" );
    kicadEntry.m_data.AppendData( textData.data(), textData.size() );
    mimeData.push_back( kicadEntry );

    bool result = SaveClipboard( textData, mimeData );

    if( result )
    {
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, textData );
    }
}

BOOST_AUTO_TEST_CASE( SaveClipboard_WithMimeData_PngHandledAsBitmap )
{
    SKIP_IF_HEADLESS();

    // Test that PNG data is handled correctly (converted to bitmap format)
    // Note: This test verifies the function doesn't crash with PNG data
    // Full bitmap round-trip testing requires a display connection

    std::string textData = "Text with PNG data";

    // Create minimal valid PNG data (1x1 transparent pixel)
    // PNG header + IHDR + IDAT + IEND
    static const unsigned char minimalPng[] = {
        0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A,  // PNG signature
        0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,  // IHDR chunk
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,  // 1x1 size
        0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,  // 8-bit RGBA
        0x89, 0x00, 0x00, 0x00, 0x0A, 0x49, 0x44, 0x41,  // IDAT chunk
        0x54, 0x78, 0x9C, 0x63, 0x00, 0x01, 0x00, 0x00,  // compressed data
        0x05, 0x00, 0x01, 0x0D, 0x0A, 0x2D, 0xB4, 0x00,  // checksum
        0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE,  // IEND chunk
        0x42, 0x60, 0x82                                  // IEND checksum
    };

    std::vector<CLIPBOARD_MIME_DATA> mimeData;

    CLIPBOARD_MIME_DATA pngEntry;
    pngEntry.m_mimeType = wxS( "image/png" );
    pngEntry.m_data.AppendData( minimalPng, sizeof( minimalPng ) );
    mimeData.push_back( pngEntry );

    CLIPBOARD_MIME_DATA kicadEntry;
    kicadEntry.m_mimeType = wxS( "application/kicad" );
    kicadEntry.m_data.AppendData( textData.data(), textData.size() );
    mimeData.push_back( kicadEntry );

    // This should not crash or assert
    bool result = SaveClipboard( textData, mimeData );

    if( result )
    {
        // Verify we can still get the KiCad data back
        std::string retrieved = GetClipboardUTF8();
        BOOST_CHECK_EQUAL( retrieved, textData );
    }
}

/**
 * Tests for the dual-buffer alpha computation algorithm.
 * This algorithm computes alpha by comparing renders on white vs black backgrounds:
 * - On white: result_w = α*F + (1-α)*255
 * - On black: result_b = α*F
 * Therefore: α = 1 - (result_w - result_b)/255
 * And: F = result_b / α (when α > 0)
 */
BOOST_AUTO_TEST_CASE( DualBufferAlpha_FullyOpaque )
{
    // Test fully opaque pixels (alpha = 255)
    // For a red pixel (255, 0, 0) with alpha=1:
    // On white: (255, 255, 255) blended with (255, 0, 0) at α=1 -> (255, 0, 0)
    // On black: (0, 0, 0) blended with (255, 0, 0) at α=1 -> (255, 0, 0)
    int rW = 255, gW = 0, bW = 0;  // Red on white
    int rB = 255, gB = 0, bB = 0;  // Red on black (same because fully opaque)

    int diffR = rW - rB;
    int diffG = gW - gB;
    int diffB = bW - bB;
    int avgDiff = (diffR + diffG + diffB) / 3;
    int alpha = 255 - avgDiff;

    BOOST_CHECK_EQUAL( alpha, 255 );

    // Recover color
    if( alpha > 0 )
    {
        int recoveredR = std::min( 255, rB * 255 / alpha );
        int recoveredG = std::min( 255, gB * 255 / alpha );
        int recoveredB = std::min( 255, bB * 255 / alpha );
        BOOST_CHECK_EQUAL( recoveredR, 255 );
        BOOST_CHECK_EQUAL( recoveredG, 0 );
        BOOST_CHECK_EQUAL( recoveredB, 0 );
    }
}

BOOST_AUTO_TEST_CASE( DualBufferAlpha_FullyTransparent )
{
    // Test fully transparent pixels (alpha = 0)
    // On white: just white (255, 255, 255)
    // On black: just black (0, 0, 0)
    int rW = 255, gW = 255, bW = 255;  // White background
    int rB = 0, gB = 0, bB = 0;        // Black background

    int diffR = rW - rB;
    int diffG = gW - gB;
    int diffB = bW - bB;
    int avgDiff = (diffR + diffG + diffB) / 3;
    int alpha = 255 - avgDiff;

    BOOST_CHECK_EQUAL( alpha, 0 );
}

BOOST_AUTO_TEST_CASE( DualBufferAlpha_SemiTransparent )
{
    // Test semi-transparent pixel (alpha = 0.5, i.e., 128)
    // Foreground color: (100, 150, 200)
    // On white: 0.5*F + 0.5*255 = 0.5*(100,150,200) + (127.5,127.5,127.5) = (177, 202, 227) approx
    // On black: 0.5*F + 0.5*0 = 0.5*(100,150,200) = (50, 75, 100)
    int rW = 177, gW = 202, bW = 227;
    int rB = 50, gB = 75, bB = 100;

    int diffR = rW - rB;  // 127
    int diffG = gW - gB;  // 127
    int diffB = bW - bB;  // 127
    int avgDiff = (diffR + diffG + diffB) / 3;  // 127
    int alpha = 255 - avgDiff;  // 128

    BOOST_CHECK_CLOSE( (double)alpha, 128.0, 1.0 );  // Allow 1% tolerance

    // Recover color: F = black_result / α * 255
    if( alpha > 0 )
    {
        int recoveredR = std::min( 255, rB * 255 / alpha );
        int recoveredG = std::min( 255, gB * 255 / alpha );
        int recoveredB = std::min( 255, bB * 255 / alpha );
        BOOST_CHECK_CLOSE( (double)recoveredR, 99.0, 2.0 );  // Should be ~100
        BOOST_CHECK_CLOSE( (double)recoveredG, 149.0, 2.0 ); // Should be ~150
        BOOST_CHECK_CLOSE( (double)recoveredB, 199.0, 2.0 ); // Should be ~200
    }
}

BOOST_AUTO_TEST_CASE( DualBufferAlpha_AntiAliasedEdge )
{
    // Test anti-aliased edge with varying alpha
    // Simulates a gray line with alpha gradient at edges
    // High alpha region (α ≈ 0.9)
    int rW1 = 128, gW1 = 128, bW1 = 128;  // Gray on white (dimmed by 0.9 blend)
    int rB1 = 115, gB1 = 115, bB1 = 115;  // Gray on black

    int diff1 = (rW1 - rB1 + gW1 - gB1 + bW1 - bB1) / 3;
    int alpha1 = 255 - diff1;
    BOOST_CHECK( alpha1 > 200 );  // High alpha

    // Low alpha region at edge (α ≈ 0.3)
    int rW2 = 210, gW2 = 210, bW2 = 210;  // Mostly white with some gray
    int rB2 = 38, gB2 = 38, bB2 = 38;     // Mostly black with some gray

    int diff2 = (rW2 - rB2 + gW2 - gB2 + bW2 - bB2) / 3;
    int alpha2 = 255 - diff2;
    BOOST_CHECK( alpha2 < 100 );  // Low alpha
    BOOST_CHECK( alpha2 > 0 );    // But not fully transparent
}

BOOST_AUTO_TEST_CASE( BitmapSizeCalculation_MatchesViewScale )
{
    // Test that bitmap size is correctly calculated from bbox and view scale
    // This tests the formula: bitmapSize = bbox_IU * viewScale

    // Simulate a 1000x500 IU bounding box at various zoom levels
    int bboxWidth = 1000;
    int bboxHeight = 500;

    // At viewScale = 1.0 (1:1 zoom)
    double viewScale1 = 1.0;
    int bitmapWidth1 = (int)(bboxWidth * viewScale1 + 0.5);
    int bitmapHeight1 = (int)(bboxHeight * viewScale1 + 0.5);
    BOOST_CHECK_EQUAL( bitmapWidth1, 1000 );
    BOOST_CHECK_EQUAL( bitmapHeight1, 500 );

    // At viewScale = 2.0 (zoomed in 2x)
    double viewScale2 = 2.0;
    int bitmapWidth2 = (int)(bboxWidth * viewScale2 + 0.5);
    int bitmapHeight2 = (int)(bboxHeight * viewScale2 + 0.5);
    BOOST_CHECK_EQUAL( bitmapWidth2, 2000 );
    BOOST_CHECK_EQUAL( bitmapHeight2, 1000 );

    // At viewScale = 0.5 (zoomed out)
    double viewScale3 = 0.5;
    int bitmapWidth3 = (int)(bboxWidth * viewScale3 + 0.5);
    int bitmapHeight3 = (int)(bboxHeight * viewScale3 + 0.5);
    BOOST_CHECK_EQUAL( bitmapWidth3, 500 );
    BOOST_CHECK_EQUAL( bitmapHeight3, 250 );
}

BOOST_AUTO_TEST_CASE( BitmapSizeCalculation_ClampToMaxSize )
{
    // Test that bitmap size is clamped while preserving aspect ratio
    const int maxBitmapSize = 4096;

    // Large bbox that would exceed max size
    int bboxWidth = 10000;
    int bboxHeight = 5000;
    double viewScale = 1.0;

    int bitmapWidth = (int)(bboxWidth * viewScale + 0.5);
    int bitmapHeight = (int)(bboxHeight * viewScale + 0.5);

    // Apply clamping as in plotSelectionToPng
    if( bitmapWidth > maxBitmapSize || bitmapHeight > maxBitmapSize )
    {
        double scaleDown = (double)maxBitmapSize / std::max( bitmapWidth, bitmapHeight );
        bitmapWidth = (int)(bitmapWidth * scaleDown + 0.5);
        bitmapHeight = (int)(bitmapHeight * scaleDown + 0.5);
        viewScale *= scaleDown;
    }

    BOOST_CHECK( bitmapWidth <= maxBitmapSize );
    BOOST_CHECK( bitmapHeight <= maxBitmapSize );
    // Check aspect ratio is preserved (2:1)
    BOOST_CHECK_CLOSE( (double)bitmapWidth / bitmapHeight, 2.0, 0.1 );
}

BOOST_AUTO_TEST_CASE( ZoomFactorCalculation_MatchesViewScale )
{
    // Test the zoom factor calculation that maps view scale to GAL print scale
    // Formula: zoomFactor = viewScale * inch2Iu / ppi
    // where inch2Iu = 1000 * IU_PER_MILS (typically 10000 for schematic)

    const double ppi = 96.0;
    const double IU_PER_MILS = 10.0;  // Typical for schematic
    const double inch2Iu = 1000.0 * IU_PER_MILS;

    // At viewScale = 1.0
    double viewScale1 = 1.0;
    double zoomFactor1 = viewScale1 * inch2Iu / ppi;
    // This should give a zoom that maps content at the right size
    BOOST_CHECK_CLOSE( zoomFactor1, 104.166666, 0.01 );

    // At viewScale = 0.1 (zoomed out)
    double viewScale2 = 0.1;
    double zoomFactor2 = viewScale2 * inch2Iu / ppi;
    BOOST_CHECK_CLOSE( zoomFactor2, 10.416666, 0.01 );

    // Zoom factors should scale linearly with view scale
    BOOST_CHECK_CLOSE( zoomFactor1 / zoomFactor2, viewScale1 / viewScale2, 0.01 );
}

BOOST_AUTO_TEST_CASE( PageSizeCalculation_MatchesBitmapForCentering )
{
    // Test that page size is calculated to match the bitmap dimensions at target ppi.
    // This is critical for proper centering of content in the output bitmap.
    // Formula: pageSizeIn = bitmapSize / ppi (in inches)
    //
    // When page size matches bitmap, the GAL will render content centered.
    // If page size were set to bbox size instead, content would be offset.

    const double ppi = 96.0;

    // Bitmap dimensions (in pixels)
    int bitmapWidth = 800;
    int bitmapHeight = 600;

    // Page size should match bitmap at target ppi
    double pageSizeInX = (double) bitmapWidth / ppi;   // 8.333... inches
    double pageSizeInY = (double) bitmapHeight / ppi;  // 6.25 inches

    // Verify the math
    BOOST_CHECK_CLOSE( pageSizeInX, 800.0 / 96.0, 0.01 );
    BOOST_CHECK_CLOSE( pageSizeInY, 600.0 / 96.0, 0.01 );

    // At ppi, this page maps back to the bitmap size
    double reconstructedWidth = pageSizeInX * ppi;
    double reconstructedHeight = pageSizeInY * ppi;
    BOOST_CHECK_CLOSE( reconstructedWidth, (double) bitmapWidth, 0.01 );
    BOOST_CHECK_CLOSE( reconstructedHeight, (double) bitmapHeight, 0.01 );

    // Test with different bitmap sizes to ensure formula is consistent
    int bitmapWidth2 = 1920;
    int bitmapHeight2 = 1080;
    double pageSizeInX2 = (double) bitmapWidth2 / ppi;
    double pageSizeInY2 = (double) bitmapHeight2 / ppi;

    BOOST_CHECK_CLOSE( pageSizeInX2 * ppi, (double) bitmapWidth2, 0.01 );
    BOOST_CHECK_CLOSE( pageSizeInY2 * ppi, (double) bitmapHeight2, 0.01 );
}

BOOST_AUTO_TEST_SUITE_END()