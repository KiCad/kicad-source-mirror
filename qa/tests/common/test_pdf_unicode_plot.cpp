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

#include <boost/test/unit_test.hpp>

#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/utils.h>
#include <wx/image.h>
#include <zlib.h>

#include <plotters/plotters_pslike.h>
#include <advanced_config.h>
#include <render_settings.h>
#include <trigo.h>
#include <math/util.h>
#include <font/font.h>
#include <font/stroke_font.h>
#include <qa_utils/wx_utils/unit_test_utils.h>
#include <qa_utils/pdf_test_utils.h>

/* Test objective:
 *   Ensure PDF_PLOTTER can emit glyphs for ASCII plus some Cyrillic, Japanese and Chinese
 *   characters using stroke font fallback. We verify by checking resulting PDF file contains
 *   expected glyph names or UTF-16 hex sequences for those code points.
 */

BOOST_AUTO_TEST_SUITE( PDFUnicodePlot )

static wxString getTempPdfPath( const wxString& name ) { return MakeTempPdfPath( name ); }

// Comprehensive mapping test: emit all four style variants in a single PDF and verify that
// every style's ToUnicode CMap contains expected codepoints (Cyrillic 041F, Japanese 65E5, Chinese 672C).
BOOST_AUTO_TEST_CASE( PlotMultilingualAllStylesMappings )
{
    const std::string sampleUtf8 = "ABCDEF Привет 日本語 漢字";
    wxString sample = wxString::FromUTF8( sampleUtf8.c_str() );
    wxString pdfPath = getTempPdfPath( "kicad_pdf_unicode_allstyles" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT("1"), wxT("TestPage") ) );

    auto emitStyle = [&]( bool bold, bool italic, int yoff )
    {
        TEXT_ATTRIBUTES attrs = BuildTextAttributes( 3000, 300, bold, italic );
        auto strokeFont = LoadStrokeFontUnique();
        KIFONT::METRICS      metrics;

        plotter.PlotText( VECTOR2I( 50000, 60000 - yoff ), COLOR4D( 0, 0, 0, 1 ), sample, attrs,
                          strokeFont.get(), metrics );
    };

    emitStyle( false, false, 0 );      // normal
    emitStyle( true,  false, 8000 );   // bold
    emitStyle( false, true,  16000 );  // italic
    emitStyle( true,  true,  24000 );  // bold-italic

    plotter.EndPlot();

    // Read entire PDF (may have compression). We'll search each Type3 font object's preceding
    // name to separate CMaps logically.
    std::string buffer; BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer ) );
    BOOST_CHECK( buffer.rfind( "%PDF", 0 ) == 0 );

    // Count how many distinct KiCadStrokeCMap names present; expect at least 4 (one per style).
    int cmapCount = CountOccurrences( buffer, "/CMapName /KiCadStrokeCMap" );

    BOOST_CHECK_MESSAGE( cmapCount >= 4, "Expected at least 4 CMaps (got " << cmapCount << ")" );

    auto requireAll = [&]( const char* codeHex, const char* label ) {
        int occurrences = CountOccurrences( buffer, codeHex );
        BOOST_CHECK_MESSAGE( occurrences >= 4, "Codepoint " << label << " (" << codeHex
                                                            << ") expected in all 4 styles; found " << occurrences );
    };

    requireAll( "041F", "Cyrillic PE" );
    requireAll( "65E5", "Kanji 日" );
    requireAll( "672C", "Kanji 本" );

    MaybeRemoveFile( pdfPath );
}

BOOST_AUTO_TEST_CASE( PlotMultilingualText )
{
    // UTF-8 sample with Latin, Cyrillic, Japanese, Chinese (shared Han) characters.
    const std::string sampleUtf8 = "ABCDEF Привет 日本語 漢字";
    wxString sample = wxString::FromUTF8( sampleUtf8.c_str() );

    wxString pdfPath = getTempPdfPath( "kicad_pdf_unicode" );

    PDF_PLOTTER plotter;

    // Force uncompressed PDF streams via environment so we can directly search for
    // unicode hex strings in the output (otherwise they are Flate compressed).
    // Do not force debug writer; allow normal compression so page content is valid.
    // The plotter expects non-null render settings for default pen width and font queries.
    // Provide a minimal concrete RENDER_SETTINGS implementation for the plotter.
    SIMPLE_RENDER_SETTINGS renderSettings; plotter.SetRenderSettings( &renderSettings );

    // Minimal viewport and plot setup. Use 1 IU per decimil so internal coordinates are small
    // and resulting translation keeps text inside the page for rasterization.
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    // StartPlot opens first page stream internally; use simple page number
    BOOST_REQUIRE( plotter.StartPlot( wxT("1"), wxT("TestPage") ) );

    TEXT_ATTRIBUTES attrs = BuildTextAttributes( 3000, 300, false, false );
    auto strokeFont = LoadStrokeFontUnique();
    KIFONT::METRICS metrics; // not used for stroke fallback

    // Plot near lower-left inside the page.
    // Place text near the top of the page in internal units so after the 0.0072 scale it
    // appears well within the MediaBox. Empirically m_paperSize.y ~ 116k internal units.
    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0.0, 0.0, 0.0, 1.0 ), sample, attrs,
                      strokeFont.get(), metrics );

    plotter.EndPlot();

    // Read file back and check for expected UTF-16 hex encodings for some code points
    // We expect CMap to contain mappings. E.g. '041F' (Cyrillic capital Pe), '65E5'(日), '672C'(本).
    std::string buffer2; BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer2 ) );
    auto contains = [&]( const char* needle ) { return PdfContains( buffer2, needle ); };

    BOOST_CHECK_MESSAGE( contains( "041F" ), "Missing Cyrillic glyph mapping (041F)" );
    BOOST_CHECK_MESSAGE( contains( "0420" ) || contains( "0440" ), "Missing Cyrillic glyph mapping (0420/0440)" );
    BOOST_CHECK_MESSAGE( contains( "65E5" ), "Missing Japanese Kanji glyph mapping (65E5)" );
    BOOST_CHECK_MESSAGE( contains( "672C" ), "Missing Japanese Kanji glyph mapping (672C)" );
    BOOST_CHECK_MESSAGE( contains( "6F22" ) || contains( "6漢" ), "Expect Chinese Han character mapping (6F22 / 漢)" );

    // Cleanup temp file (unless debugging requested)
    // Optional: rasterize PDF to image (requires poppler 'pdftoppm').
    // We treat absence of the tool as a skipped sub-check rather than a failure.
    {
        long darkPixels = 0;
        if( RasterizePdfCountDark( pdfPath, 72, 240, darkPixels ) )
        {
            BOOST_CHECK_MESSAGE( darkPixels > 200,
                                 "Rasterized PDF appears blank or too sparse (" << darkPixels
                                 << " dark pixels)" );
        }
        else
        {
            BOOST_TEST_MESSAGE( "pdftoppm not available or failed; skipping raster validation" );
        }
    }

    MaybeRemoveFile( pdfPath );
}

BOOST_AUTO_TEST_CASE( PlotMultilingualTextBold )
{
    const std::string sampleUtf8 = "ABCDEF Привет 日本語 漢字";
    wxString sample = wxString::FromUTF8( sampleUtf8.c_str() );

    wxString pdfPath = getTempPdfPath( "kicad_pdf_unicode_bold" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;
    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT("1"), wxT("TestPage") ) );

    TEXT_ATTRIBUTES attrs = BuildTextAttributes( 3000, 300, true, false );
    auto strokeFont = LoadStrokeFontUnique();
    KIFONT::METRICS metrics;
    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0.0, 0.0, 0.0, 1.0 ), sample, attrs,
                      strokeFont.get(), metrics );
    plotter.EndPlot();
    std::string buffer3; BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer3 ) );
    auto contains = [&]( const char* needle ) { return PdfContains( buffer3, needle ); };
    BOOST_CHECK_MESSAGE( contains( "041F" ), "Missing Cyrillic glyph mapping (bold 041F)" );
    BOOST_CHECK_MESSAGE( contains( "65E5" ), "Missing Japanese glyph mapping (bold 65E5)" );

    MaybeRemoveFile( pdfPath );
}

BOOST_AUTO_TEST_CASE( PlotMultilingualTextItalic )
{
    const std::string sampleUtf8 = "ABCDEF Привет 日本語 漢字";
    wxString sample = wxString::FromUTF8( sampleUtf8.c_str() );
    wxString pdfPath = getTempPdfPath( "kicad_pdf_unicode_italic" );
    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "TestPage" ) ) );
    TEXT_ATTRIBUTES attrs = BuildTextAttributes( 3000, 300, false, true );
    auto strokeFont = LoadStrokeFontUnique();
    KIFONT::METRICS      metrics;
    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0, 0, 0, 1 ), sample, attrs, strokeFont.get(), metrics );
    plotter.EndPlot();

    std::string buffer4; BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer4 ) );
    auto contains = [&]( const char* n ) { return PdfContains( buffer4, n ); };
    BOOST_CHECK_MESSAGE( contains( "041F" ), "Missing Cyrillic glyph mapping (italic 041F)" );
    BOOST_CHECK_MESSAGE( contains( "65E5" ), "Missing Japanese glyph mapping (italic 65E5)" );
    MaybeRemoveFile( pdfPath );
}

BOOST_AUTO_TEST_CASE( PlotMultilingualTextBoldItalic )
{
    const std::string sampleUtf8 = "ABCDEF Привет 日本語 漢字";
    wxString          sample = wxString::FromUTF8( sampleUtf8.c_str() );
    wxString          pdfPath = getTempPdfPath( "kicad_pdf_unicode_bolditalic" );
    PDF_PLOTTER       plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );

    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "TestPage" ) ) );

    TEXT_ATTRIBUTES attrs = BuildTextAttributes( 3000, 300, true, true );
    auto strokeFont = LoadStrokeFontUnique();
    KIFONT::METRICS      metrics;

    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0, 0, 0, 1 ), sample, attrs, strokeFont.get(), metrics );
    plotter.EndPlot();

    std::string buffer5; BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer5 ) );
    auto contains = [&]( const char* n ) { return PdfContains( buffer5, n ); };
    BOOST_CHECK_MESSAGE( contains( "041F" ), "Missing Cyrillic glyph mapping (bold-italic 041F)" );
    BOOST_CHECK_MESSAGE( contains( "65E5" ), "Missing Japanese glyph mapping (bold-italic 65E5)" );
    MaybeRemoveFile( pdfPath );
}

// Verify that d1 bounding boxes account for X offset and stroke width so PDF viewers don't
// clip the rendered glyphs.  Regression test for gitlab.com/kicad/code/kicad/-/issues/23621.
BOOST_AUTO_TEST_CASE( GlyphBBoxIncludesOffsetAndStrokeWidth )
{
    const std::string sampleUtf8 = "MW";
    wxString sample = wxString::FromUTF8( sampleUtf8.c_str() );
    wxString pdfPath = getTempPdfPath( "kicad_pdf_bbox_check" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "BBoxTest" ) ) );

    TEXT_ATTRIBUTES attrs = BuildTextAttributes( 3000, 300, false, false );
    auto strokeFont = LoadStrokeFontUnique();
    KIFONT::METRICS metrics;

    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0, 0, 0, 1 ), sample, attrs,
                      strokeFont.get(), metrics );
    plotter.EndPlot();

    std::string buffer;
    BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer ) );

    // Parse d1 operators: format is "width 0 minX minY maxX maxY d1"
    // Verify that no glyph has a minX of exactly 0 (would mean X offset was not applied) and
    // that the bbox extends beyond the stroke center coordinates by at least half the stroke
    // width.
    const ADVANCED_CFG& cfg = ADVANCED_CFG::GetCfg();
    double unitsPerEm = 1000.0;
    double expectedXOffset = cfg.m_PDFStrokeFontXOffset * unitsPerEm;
    double widthFactor = 300.0 / 3000.0;
    double expectedHalfStroke = unitsPerEm * widthFactor / 2.0;

    // Find all d1 operators (skip .notdef which has all-zero bbox)
    std::string::size_type pos = 0;
    int checkedGlyphs = 0;

    while( ( pos = buffer.find( " d1 ", pos ) ) != std::string::npos )
    {
        // Walk backwards to find the start of the d1 line
        std::string::size_type lineStart = buffer.rfind( '\n', pos );

        if( lineStart == std::string::npos )
            lineStart = 0;
        else
            lineStart++;

        std::string d1Line = buffer.substr( lineStart, pos + 3 - lineStart );

        double width, wy, minX, minY, maxX, maxY;

        if( sscanf( d1Line.c_str(), "%lf %lf %lf %lf %lf %lf", &width, &wy, &minX, &minY,
                     &maxX, &maxY ) == 6 )
        {
            // Skip .notdef (all zeros)
            if( width == 0.0 && maxX == 0.0 )
            {
                pos += 4;
                continue;
            }

            // The bbox minX should be shifted by the X offset minus half stroke width.
            // With default offset 0.1 and stroke width 300 at size 3000, minX should be around
            // 0.1*1000 - 0.1*1000/2 = 100 - 50 = 50, not 0.
            BOOST_CHECK_MESSAGE( minX < -expectedHalfStroke + 1.0 || minX > 0.1,
                                 "Glyph bbox minX (" << minX
                                 << ") suggests X offset or stroke padding is missing" );

            // maxX must exceed the advance width to account for offset + stroke padding
            BOOST_CHECK_MESSAGE( maxX > width,
                                 "Glyph bbox maxX (" << maxX << ") must exceed advance width ("
                                 << width << ") to prevent clipping" );

            checkedGlyphs++;
        }

        pos += 4;
    }

    BOOST_CHECK_MESSAGE( checkedGlyphs >= 2,
                         "Expected at least 2 non-notdef glyphs, found " << checkedGlyphs );

    MaybeRemoveFile( pdfPath );
}

// Test Y offset bounding box fix: ensure characters are not clipped when Y offset is applied
BOOST_AUTO_TEST_CASE( PlotMultilingualTextWithYOffset )
{
    // Temporarily modify the Y offset configuration
    ADVANCED_CFG& cfg = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    double originalOffset = cfg.m_PDFStrokeFontYOffset;
    cfg.m_PDFStrokeFontYOffset = 0.2;  // 20% of EM unit offset upward

    const std::string sampleUtf8 = "Yg Test ñ";  // characters with ascenders and descenders
    wxString sample = wxString::FromUTF8( sampleUtf8.c_str() );
    wxString pdfPath = getTempPdfPath( "kicad_pdf_unicode_yoffset" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "TestPage" ) ) );

    TEXT_ATTRIBUTES attrs = BuildTextAttributes( 4000, 400, false, false );
    auto strokeFont = LoadStrokeFontUnique();
    KIFONT::METRICS      metrics;
    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0, 0, 0, 1 ), sample, attrs, strokeFont.get(), metrics );
    plotter.EndPlot();
    

    // Restore original Y offset
    cfg.m_PDFStrokeFontYOffset = originalOffset;

    // Basic PDF validation and decompression
    std::string buffer6; BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer6 ) );
    BOOST_CHECK( buffer6.rfind( "%PDF", 0 ) == 0 );

    // Check that bounding boxes exist and are reasonable (not clipped)
    // Look for d1 operators which specify character bounding boxes
    BOOST_CHECK_MESSAGE( buffer6.find( "d1" ) != std::string::npos,
                         "PDF should contain d1 operators for glyph bounding boxes" );

    MaybeRemoveFile( pdfPath );
}

BOOST_AUTO_TEST_CASE( PlotOutlineFontEmbedding )
{
    wxString pdfPath = getTempPdfPath( "kicad_pdf_outline_font" );

    // Locate test font file (Noto Sans) in test resources
    wxFileName fontFile( KI_TEST::GetTestDataRootDir() );
    fontFile.RemoveLastDir();
    fontFile.AppendDir( wxT( "resources" ) );
    fontFile.AppendDir( wxT( "fonts" ) );
    fontFile.SetFullName( wxT( "NotoSans-Regular.ttf" ) );
    wxString fontPath = fontFile.GetFullPath();

    BOOST_REQUIRE( wxFileExists( fontPath ) );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT("1"), wxT("OutlineFont") ) );

    TEXT_ATTRIBUTES attrs = BuildTextAttributes( 4000, 0, false, false );

    std::vector<wxString> embeddedFonts;
    embeddedFonts.push_back( fontPath );

    KIFONT::FONT* outlineFont = KIFONT::FONT::GetFont( wxT( "Noto Sans" ), false, false, &embeddedFonts );
    KIFONT::METRICS metrics;

    wxString sample = wxString::FromUTF8( "Outline café" );

    plotter.PlotText( VECTOR2I( 42000, 52000 ), COLOR4D( 0, 0, 0, 1 ), sample, attrs,
                      outlineFont, metrics );

    plotter.EndPlot();

    wxFFile file( pdfPath, "rb" );
    BOOST_REQUIRE( file.IsOpened() );
    wxFileOffset len = file.Length();
    std::string buffer; buffer.resize( (size_t) len ); file.Read( buffer.data(), len );
    BOOST_CHECK( buffer.rfind( "%PDF", 0 ) == 0 );

    auto appendDecompressed = [&]() { std::string tmp; ReadPdfWithDecompressedStreams( pdfPath, tmp ); buffer.swap( tmp ); };
    appendDecompressed();

    BOOST_CHECK_MESSAGE( buffer.find( "/CIDFontType2" ) != std::string::npos,
                         "Expected CIDFontType2 descendant font" );
    BOOST_CHECK_MESSAGE( buffer.find( "/FontFile2" ) != std::string::npos,
                         "Embedded outline font should include FontFile2 stream" );
    BOOST_CHECK_MESSAGE( buffer.find( "AAAAAA+Noto-Sans" ) != std::string::npos,
                         "BaseFont should reference Noto Sans subset" );
    BOOST_CHECK_MESSAGE( buffer.find( "00E9" ) != std::string::npos,
                         "ToUnicode map should include Latin character with accent" );
    BOOST_CHECK_MESSAGE( buffer.find( "/KiCadOutline" ) != std::string::npos,
                         "Outline font resource entry missing" );

    // Optional: rasterize PDF to image (requires poppler 'pdftoppm').
    // We treat absence of the tool as a skipped sub-check rather than a failure.
    {
        wxString rasterBase = wxFileName::CreateTempFileName( wxT("kicad_pdf_raster") );
        wxString cmd = wxString::Format( wxT("pdftoppm -r 72 -singlefile -png \"%s\" \"%s\""),
                                         pdfPath, rasterBase );

        int ret = wxExecute( cmd, wxEXEC_SYNC );

        if( ret == 0 )
        {
            wxString pngPath = rasterBase + wxT(".png");

            if( wxFileExists( pngPath ) )
            {
                // Ensure PNG handler is available
                if( !wxImage::FindHandler( wxBITMAP_TYPE_PNG ) )
                    wxImage::AddHandler( new wxPNGHandler );

                wxImage img( pngPath );
                BOOST_REQUIRE_MESSAGE( img.IsOk(), "Failed to load rasterized PDF image" );

                long darkPixels = 0;
                int  w = img.GetWidth();
                int  h = img.GetHeight();

                for( int y = 0; y < h; ++y )
                {
                    for( int x = 0; x < w; ++x )
                    {
                        unsigned char r = img.GetRed( x, y );
                        unsigned char g = img.GetGreen( x, y );
                        unsigned char b = img.GetBlue( x, y );

                        // Count any non-near-white pixel as drawn content
                        if( r < 240 || g < 240 || b < 240 )
                            ++darkPixels;
                    }
                }

                // Demand at least 100 non-white pixels to consider outline font rendered correctly.
                // This threshold is lower than stroke font since outline fonts may render differently.
                BOOST_CHECK_MESSAGE( darkPixels > 100,
                                     "Rasterized PDF appears blank or too sparse (" << darkPixels
                                     << " dark pixels). Outline font may not be rendering correctly." );

                // Housekeeping
                wxRemoveFile( pngPath );
            }
            else
            {
                BOOST_TEST_MESSAGE( "pdftoppm succeeded but PNG output missing; skipping raster validation" );
            }
        }
        else
        {
            BOOST_TEST_MESSAGE( "pdftoppm not available or failed; skipping raster validation" );
        }
    }

    MaybeRemoveFile( pdfPath );
}

// Extract the device-space X translation of every stroke-font text block ("... cm BT") in a
// decompressed PDF content stream, in stream order.  The stroke plotter emits each rendered word
// as "q a b c d e f cm BT ...", where the 5th number (e) is the text-matrix X origin.
static std::vector<double> ExtractStrokeTextOriginsX( const std::string& aBuffer )
{
    std::vector<double> origins;
    size_t              pos = 0;

    while( ( pos = aBuffer.find( "cm BT", pos ) ) != std::string::npos )
    {
        std::string head = aBuffer.substr( 0, pos );
        std::vector<double> nums;

        for( size_t end = head.size(); end > 0 && nums.size() < 6; )
        {
            size_t start = head.find_last_not_of( " \n\r\t", end - 1 );

            if( start == std::string::npos )
                break;

            size_t tokStart = head.find_last_of( " \n\r\t", start );
            tokStart = ( tokStart == std::string::npos ) ? 0 : tokStart + 1;

            try
            {
                nums.push_back( std::stod( head.substr( tokStart, start - tokStart + 1 ) ) );
            }
            catch( ... )
            {
                break;
            }

            end = tokStart;
        }

        if( nums.size() == 6 )
            origins.push_back( nums[1] ); // reversed order: e is second-from-last collected

        pos += 5;
    }

    return origins;
}


// Issue #22606: PDF plotting of stroke-font text containing tab characters placed the following
// text at the wrong tab stop.  The plotter hardcoded the outline font's tab rule (2.4 * size),
// but the default schematic font is the stroke font, whose tab stops are column based and land
// much further right.  The result was tabs that came out too short in the PDF versus on screen.
//
// This drives the plotter with the reporter's own text ("v07:\terstversion") and checks that the
// text after the tab is placed exactly where the stroke font's own glyph model puts it.
BOOST_AUTO_TEST_CASE( PlotStrokeTextTabStopMatchesFont )
{
    wxString pdfPath = getTempPdfPath( "kicad_pdf_tabs" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "TabTest" ) ) );

    const int       sizeIu = 1270000; // 1.27 mm, the reporter's text size
    TEXT_ATTRIBUTES attrs = BuildTextAttributes( sizeIu, sizeIu / 10, false, false );
    auto            strokeFont = LoadStrokeFontUnique();
    KIFONT::METRICS metrics;

    // Two reference words establish the linear user->device X scale of the viewport.
    plotter.PlotText( VECTOR2I( 40000000, 60000000 ), COLOR4D( 0, 0, 0, 1 ), wxT( "M" ), attrs,
                      strokeFont.get(), metrics );
    plotter.PlotText( VECTOR2I( 80000000, 60000000 ), COLOR4D( 0, 0, 0, 1 ), wxT( "M" ), attrs,
                      strokeFont.get(), metrics );

    // The reporter's tabbed line: "v07:" then a tab then the rest of the line.
    plotter.PlotText( VECTOR2I( 40000000, 40000000 ), COLOR4D( 0, 0, 0, 1 ), wxT( "v07:\terstversion" ),
                      attrs, strokeFont.get(), metrics );

    plotter.EndPlot();

    std::string buffer;
    BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer ) );
    BOOST_CHECK( buffer.rfind( "%PDF", 0 ) == 0 );

    std::vector<double> origins = ExtractStrokeTextOriginsX( buffer );

    // Two reference "M" blocks plus the two segments ("v07:" and "erstversion") of the tabbed line.
    BOOST_REQUIRE_EQUAL( origins.size(), 4u );

    const double scale = ( origins[1] - origins[0] ) / ( 80000000.0 - 40000000.0 );
    BOOST_REQUIRE( std::abs( scale ) > 0.0 );

    // Distance in user units from the "v07:" origin to the post-tab "erstversion" origin.
    const double measuredOffsetIu = ( origins[3] - origins[2] ) / scale;

    const VECTOR2I size( sizeIu, sizeIu );

    // The stroke font's own glyph placement is the ground truth for the on-screen position.
    const double expectedOffsetIu =
            strokeFont->GetTextAsGlyphs( nullptr, nullptr, wxT( "v07:\t" ), size, VECTOR2I(),
                                         ANGLE_0, false, VECTOR2I(), 0 )
                    .x;

    // The old (buggy) hardcoded rule would have placed it here; kept only to document the gap.
    const double widthV07 =
            strokeFont->GetTextAsGlyphs( nullptr, nullptr, wxT( "v07:" ), size, VECTOR2I(), ANGLE_0,
                                         false, VECTOR2I(), 0 )
                    .x;
    const int    oldTabWidth = KiROUND( sizeIu * 4 * 0.6 );
    const double oldOffsetIu = widthV07 + ( oldTabWidth - KiROUND( widthV07 ) % oldTabWidth );

    const double tolerance = sizeIu * 0.02;

    BOOST_CHECK_MESSAGE( std::abs( measuredOffsetIu - expectedOffsetIu ) < tolerance,
                         "Post-tab text at " << measuredOffsetIu << " IU, font model expects "
                                             << expectedOffsetIu << " IU (old buggy rule: "
                                             << oldOffsetIu << " IU)" );

    MaybeRemoveFile( pdfPath );
}

// Regression test for GitLab issue 23740: stroke-font Type3 glyphs in the PDF output were
// plotted above and to the left of where they should have been.  The old code derived the
// vertical anchor from StringBoundaryLimits (which inflates by 3*thickness) and forgot to
// cancel the m_PDFStrokeFontYOffset baked into every Type3 glyph, so character placement
// varied with the caller's pen width and with the stroke-font Y offset.
//
// The test plots the same character with V_TOP, V_CENTER and V_BOTTOM alignments at the same
// anchor, parses the text-matrix ctm_f value out of the content stream, then verifies that
// ctm_f corresponds to the positions that FONT::getLinePositions would produce for GAL,
// translated to PDF device coordinates (with the stroke-font yOffset applied so glyph ink
// lands where screen rendering would).
BOOST_AUTO_TEST_CASE( StrokeFontVerticalAlignmentMatchesScreen )
{
    wxString pdfPath = getTempPdfPath( "kicad_pdf_valign" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;
    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "VAlignTest" ) ) );

    const int anchorY = 60000;
    const int sizeIU  = 3000;
    const int strokeW = 300;

    auto plotAt = [&]( GR_TEXT_V_ALIGN_T aVJustify, bool aItalic )
    {
        TEXT_ATTRIBUTES attrs = BuildTextAttributes( sizeIU, strokeW, false, aItalic );
        attrs.m_Valign = aVJustify;
        auto strokeFont = LoadStrokeFontUnique();
        KIFONT::METRICS metrics;
        plotter.PlotText( VECTOR2I( 50000, anchorY ), COLOR4D( 0, 0, 0, 1 ), wxT( "T" ), attrs,
                          strokeFont.get(), metrics );
    };

    plotAt( GR_TEXT_V_ALIGN_TOP, false );
    plotAt( GR_TEXT_V_ALIGN_CENTER, false );
    plotAt( GR_TEXT_V_ALIGN_BOTTOM, false );
    plotAt( GR_TEXT_V_ALIGN_TOP, true );
    plotAt( GR_TEXT_V_ALIGN_CENTER, true );
    plotter.EndPlot();

    std::string buffer;
    BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer ) );

    // Capture the full text matrix for every `q ... cm BT ... Tj ET Q` block.
    struct MATRIX_SAMPLE { double a, b, c, d, e, f; };
    std::vector<MATRIX_SAMPLE> matrices;
    std::string::size_type pos = 0;

    while( ( pos = buffer.find( "cm BT", pos ) ) != std::string::npos )
    {
        std::string::size_type lineStart = buffer.rfind( '\n', pos );
        lineStart = ( lineStart == std::string::npos ) ? 0 : lineStart + 1;

        std::string cmLine = buffer.substr( lineStart, pos - lineStart );
        MATRIX_SAMPLE m{};

        if( sscanf( cmLine.c_str(), "q %lf %lf %lf %lf %lf %lf", &m.a, &m.b, &m.c, &m.d, &m.e,
                    &m.f ) == 6 )
        {
            matrices.push_back( m );
        }

        pos += 5;
    }

    BOOST_REQUIRE_EQUAL( matrices.size(), 5u );

    const double fontSize  = (double) sizeIU;
    const double thickness = (double) strokeW;

    // GAL cursor offsets from FONT::getLinePositions (single line, height = 1.17 * size):
    //     V_TOP    = size                (+ size.y below anchor)
    //     V_CENTER = 0.415 * size        (size - height/2)
    //     V_BOTTOM = -0.17  * size       (size - height)
    // The PDF stroke path applies a constant yOffset on top of these, which drops out of the
    // deltas between alignments, so we check those deltas directly.  PDF device Y decreases
    // as IU Y increases, so ctm_f_TOP < ctm_f_CENTER < ctm_f_BOTTOM.
    const double expected_delta_top_center = ( 1.000 - 0.415 ) * fontSize;
    const double expected_delta_center_bot = ( 0.415 - ( -0.17 ) ) * fontSize;

    const double observed_delta_top_center = matrices[1].f - matrices[0].f;
    const double observed_delta_center_bot = matrices[2].f - matrices[1].f;

    BOOST_CHECK_CLOSE( observed_delta_top_center, expected_delta_top_center, 1.0 );
    BOOST_CHECK_CLOSE( observed_delta_center_bot, expected_delta_center_bot, 1.0 );

    BOOST_CHECK_MESSAGE( matrices[0].f < matrices[1].f && matrices[1].f < matrices[2].f,
                         "Expected ctm_f to decrease from V_BOTTOM to V_CENTER to V_TOP, got "
                             << matrices[0].f << ", " << matrices[1].f << ", " << matrices[2].f );

    // The uncorrected formula (pre-fix) would give 0.5 * (size + 3*thickness) for the V_TOP
    // to V_CENTER delta.  Ensure the observed delta does not match that, which would mean
    // the alignment fix is inactive.
    const double uncorrectedDelta = 0.5 * ( fontSize + 3.0 * thickness );
    BOOST_CHECK_MESSAGE( std::abs( observed_delta_top_center - uncorrectedDelta ) > 1.0,
                         "ctm_f delta for V_TOP vs V_CENTER matches the uncorrected formula; "
                         "the stroke-font alignment fix does not appear to be in effect." );

    // For italic text the Y axis of the text matrix is sheared (adj_c != 0 for horizontal
    // italic), so the baseline correction has to project onto (adj_c, adj_d) rather than
    // sin/cos of aOrient.  With the buggy sin/cos formula, italic_delta_e between V_TOP and
    // V_CENTER at angle 0 would be zero (same as the non-italic case); with the fix the
    // shear transfers part of the correction into the X direction.
    const double italic_delta_e = matrices[4].e - matrices[3].e;
    const double nonitalic_delta_e = matrices[1].e - matrices[0].e;

    BOOST_CHECK_MESSAGE( std::abs( matrices[3].c ) > 0.01,
                         "Italic text matrix does not show the expected shear (adj_c == "
                             << matrices[3].c << ")" );

    // Non-italic, horizontal text has adj_c == 0, so delta_e between V-alignments is 0.
    BOOST_CHECK_SMALL( nonitalic_delta_e, 1.0 );

    // Italic text must show a non-zero delta_e from the shear projection.  The direction
    // is determined by the sign of the shear: with tilt = -ITALIC_TILT and downward IU Y
    // correction, italic_delta_e has the opposite sign of adj_c (i.e., negative when the
    // italic shear leans right).
    BOOST_CHECK_MESSAGE( std::abs( italic_delta_e ) > 1.0,
                         "Italic baseline correction did not shear along the text-matrix Y "
                         "axis; italic_delta_e = "
                             << italic_delta_e
                             << " (should be non-zero when adj_c is non-zero)" );
    BOOST_CHECK_MESSAGE( italic_delta_e * matrices[3].c < 0,
                         "italic_delta_e should have the opposite sign of adj_c; got "
                             << italic_delta_e << " vs adj_c=" << matrices[3].c );

    MaybeRemoveFile( pdfPath );
}


// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24419
// PDF_PLOTTER::renderWord used to advance the per-word cursor by
// FONT::StringBoundaryLimits, which for stroke fonts inflates the returned
// bounding box by 3*thickness.  The Tj operator that actually renders the word
// advances by the sum of glyph widths (no inflation), so successive words were
// pushed too far apart in the rendered PDF.  The bug was visible on silkscreen
// text whose width exceeded the board edge in the exported PDF but fit on the
// PCB editor canvas.
//
// The fix is to derive the cursor advance from FONT::GetTextAsGlyphs, which
// matches exactly what Tj produces.  This test plots a multi-word string and
// verifies that the gap between word origins corresponds to the actual
// glyph-width sum rather than the inflated bbox.
BOOST_AUTO_TEST_CASE( StrokeFontWordSpacingMatchesGlyphAdvance )
{
    wxString pdfPath = getTempPdfPath( "kicad_pdf_wordspacing" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;
    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "WordSpacingTest" ) ) );

    const int       sizeIU  = 1900;          // mimics the 1.9mm silk text from issue #24419
    const int       strokeW = 380;           // bold thickness as in the issue file
    TEXT_ATTRIBUTES attrs   = BuildTextAttributes( sizeIU, strokeW, true, false );
    attrs.m_Halign          = GR_TEXT_H_ALIGN_LEFT;
    attrs.m_Valign          = GR_TEXT_V_ALIGN_BOTTOM;

    auto                  strokeFont = LoadStrokeFontUnique();
    const KIFONT::METRICS metrics;

    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0, 0, 0, 1 ),
                      wxT( "TEST text Silkscreen" ), attrs, strokeFont.get(), metrics );
    plotter.EndPlot();

    std::string buffer;
    BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer ) );

    // Each word becomes one `q ... cm BT ... Tj ET Q` block.  Capture the X translation (ctm_e)
    // of each block so we can verify the gap between words.
    std::vector<double> wordOriginX;
    std::string::size_type pos = 0;

    while( ( pos = buffer.find( "cm BT", pos ) ) != std::string::npos )
    {
        std::string::size_type lineStart = buffer.rfind( '\n', pos );
        lineStart = ( lineStart == std::string::npos ) ? 0 : lineStart + 1;

        std::string cmLine = buffer.substr( lineStart, pos - lineStart );
        double a, b, c, d, e, f;

        if( sscanf( cmLine.c_str(), "q %lf %lf %lf %lf %lf %lf", &a, &b, &c, &d, &e, &f ) == 6 )
            wordOriginX.push_back( e );

        pos += 5;
    }

    // Expect one block per word: "TEST", "text", "Silkscreen".
    BOOST_REQUIRE_EQUAL( wordOriginX.size(), 3u );

    // Compute the expected stroke-font cursor advance for "TEST " (word + trailing space) and
    // "text " in IU.  This is what the Tj operator will move the text cursor by and what the
    // renderWord cursor should match.  Compare ratios so we don't have to convert to device
    // units (userToDeviceSize is protected).
    auto strokeAdvanceIU = [&]( const wxString& aWord )
    {
        return strokeFont
                ->GetTextAsGlyphs( nullptr, nullptr, aWord, VECTOR2I( sizeIU, sizeIU ),
                                   VECTOR2I(), ANGLE_0, false, VECTOR2I(), TEXT_STYLE::BOLD )
                .x;
    };

    const double expectedTestAdvanceIU = (double) strokeAdvanceIU( wxT( "TEST " ) );
    const double expectedTextAdvanceIU = (double) strokeAdvanceIU( wxT( "text " ) );

    const double observedTestAdvanceDev = wordOriginX[1] - wordOriginX[0];
    const double observedTextAdvanceDev = wordOriginX[2] - wordOriginX[1];

    // device-per-IU scale factor derived from the first observed gap.
    const double scale = observedTestAdvanceDev / expectedTestAdvanceIU;

    // The TEST -> text advance and the text -> Silkscreen advance must both follow the
    // same IU-to-device scale: if the spacing matches glyph metrics, the second observed
    // gap equals scale * expectedTextAdvanceIU.
    const double expectedTextAdvanceDev = scale * expectedTextAdvanceIU;
    BOOST_CHECK_CLOSE( observedTextAdvanceDev, expectedTextAdvanceDev, 0.5 );

    // Sanity guard against regression to the inflated-bbox formula, which adds 3*thickness IU
    // of slop per word.  Use the second word so this check is independent of the scale derivation
    // above.
    const double inflatedTextAdvanceDev = scale * ( expectedTextAdvanceIU + 3.0 * strokeW );
    BOOST_CHECK_MESSAGE( std::abs( observedTextAdvanceDev - inflatedTextAdvanceDev )
                                 > 0.5 * ( inflatedTextAdvanceDev - expectedTextAdvanceDev ),
                         "Word spacing matches the buggy inflated-bbox formula ("
                                 << observedTextAdvanceDev << " ~= " << inflatedTextAdvanceDev
                                 << "); the renderWord cursor fix appears inactive." );

    MaybeRemoveFile( pdfPath );
}

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23843
// A border-less filled shape with a dashed/dotted stroke is plotted with a zero pen width.
// PDF_PLOTTER::SetDash then computed every dash element from that zero width, emitting an
// all-zero dash array "[0 0] 0 d". Such an array is illegal in PDF and makes strict viewers
// (Adobe Acrobat, Evince) abort rendering of the rest of the page, dropping the shape fill,
// the page background and any junction dots that follow. SetDash must fall back to a solid
// line "[] 0 d" when the requested pattern degenerates to all zeros.
BOOST_AUTO_TEST_CASE( SetDashZeroWidthNoIllegalDashArray )
{
    wxString pdfPath = getTempPdfPath( "kicad_pdf_zero_dash" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "DashTest" ) ) );

    // Reproduce the border-less dotted filled rectangle from the issue: a dotted dash style
    // with a zero pen width, followed by the filled (no-outline) rectangle itself. Exercise
    // every dashed style so a regression in any of them is caught.
    plotter.SetColor( COLOR4D( 1.0, 0.0, 0.0, 1.0 ) );

    for( LINE_STYLE style : { LINE_STYLE::DASH, LINE_STYLE::DOT, LINE_STYLE::DASHDOT,
                              LINE_STYLE::DASHDOTDOT } )
    {
        plotter.SetDash( 0, style );
    }

    plotter.SetDash( 0, LINE_STYLE::DOT );
    plotter.Rect( VECTOR2I( 50000, 50000 ), VECTOR2I( 70000, 60000 ), FILL_T::FILLED_SHAPE, 0 );

    plotter.EndPlot();

    std::string buffer;
    BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer ) );
    BOOST_CHECK( buffer.rfind( "%PDF", 0 ) == 0 );

    // No illegal all-zero dash array may be present anywhere in the content stream, for any
    // of the dashed styles.
    for( const char* illegalPattern : { "[0 0] 0 d", "[0 0 0 0] 0 d", "[0 0 0 0 0 0] 0 d" } )
    {
        BOOST_CHECK_MESSAGE( buffer.find( illegalPattern ) == std::string::npos,
                             "PDF contains an illegal all-zero dash array (" << illegalPattern
                             << "), which breaks strict viewers" );
    }

    // The zero-width dashed styles must have degenerated to a solid line.
    BOOST_CHECK_MESSAGE( buffer.find( "[] 0 d" ) != std::string::npos,
                         "Zero-width dashed style should fall back to a solid dash array" );

    // The fill operator for the rectangle must still be emitted.
    BOOST_CHECK_MESSAGE( buffer.find( " re f" ) != std::string::npos
                                 || buffer.find( "re\nf" ) != std::string::npos,
                         "Filled rectangle should still be plotted" );

    MaybeRemoveFile( pdfPath );
}


// Companion to SetDashZeroWidthNoIllegalDashArray: the solid-line fallback must apply only to the
// degenerate all-zero case. A normal non-zero pen width must still emit a real dashed array, so a
// fix that collapsed every dashed style to solid would regress here.
BOOST_AUTO_TEST_CASE( SetDashNonZeroWidthKeepsDashArray )
{
    wxString pdfPath = getTempPdfPath( "kicad_pdf_nonzero_dash" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "DashTest" ) ) );

    plotter.SetColor( COLOR4D( 1.0, 0.0, 0.0, 1.0 ) );
    plotter.SetDash( 10000, LINE_STYLE::DASH );
    plotter.MoveTo( VECTOR2I( 50000, 50000 ) );
    plotter.FinishTo( VECTOR2I( 70000, 50000 ) );

    plotter.EndPlot();

    std::string buffer;
    BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer ) );

    // The dash array elements scale with the pen width, so a non-zero width yields a real,
    // non-zero pattern. Find the emitted array and confirm it is neither the solid fallback
    // "[]" nor the all-zero degenerate form.
    size_t dashPos = buffer.find( "] 0 d" );
    BOOST_REQUIRE_MESSAGE( dashPos != std::string::npos, "No dash array was emitted" );

    size_t openPos = buffer.rfind( '[', dashPos );
    BOOST_REQUIRE( openPos != std::string::npos && openPos < dashPos );

    std::string dashArray = buffer.substr( openPos, dashPos - openPos + 1 );

    BOOST_CHECK_MESSAGE( dashArray.find_first_of( "123456789" ) != std::string::npos,
                         "Non-zero dashed style should emit a real, non-zero dash array, got "
                                 << dashArray );
    BOOST_CHECK_MESSAGE( dashArray != "[]",
                         "A non-zero pen width must not fall back to the solid line" );
}

BOOST_AUTO_TEST_SUITE_END()
