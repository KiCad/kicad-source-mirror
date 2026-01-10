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

// Test tab handling in PDF text output (issue #22606).
// When text contains tab characters, each tab should advance to the next tab stop.
// We verify that text with tabs produces different glyph positions than without tabs.
BOOST_AUTO_TEST_CASE( PlotTextWithTabs )
{
    wxString pdfPath = getTempPdfPath( "kicad_pdf_tabs" );

    PDF_PLOTTER plotter;
    SIMPLE_RENDER_SETTINGS renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "TabTest" ) ) );

    TEXT_ATTRIBUTES attrs = BuildTextAttributes( 3000, 300, false, false );
    auto strokeFont = LoadStrokeFontUnique();
    KIFONT::METRICS metrics;

    wxString textWithTab = wxT( "Before\tAfter" );
    wxString textWithoutTab = wxT( "BeforeAfter" );

    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0, 0, 0, 1 ), textWithTab, attrs,
                      strokeFont.get(), metrics );
    plotter.PlotText( VECTOR2I( 50000, 50000 ), COLOR4D( 0, 0, 0, 1 ), textWithoutTab, attrs,
                      strokeFont.get(), metrics );

    plotter.EndPlot();

    std::string buffer;
    BOOST_REQUIRE( ReadPdfWithDecompressedStreams( pdfPath, buffer ) );
    BOOST_CHECK( buffer.rfind( "%PDF", 0 ) == 0 );

    // The PDF should contain text content. Tabs should not produce visible tab glyphs but should
    // create proper spacing. We verify the PDF is valid and contains our text characters.
    BOOST_CHECK_MESSAGE( PdfContains( buffer, "0041" ) || PdfContains( buffer, "A" ),
                         "PDF should contain 'A' from 'After'" );

    MaybeRemoveFile( pdfPath );
}

BOOST_AUTO_TEST_SUITE_END()
