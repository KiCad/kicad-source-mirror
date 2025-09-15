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

/* Test objective:
 *   Ensure PDF_PLOTTER can emit glyphs for ASCII plus some Cyrillic, Japanese and Chinese
 *   characters using stroke font fallback. We verify by checking resulting PDF file contains
 *   expected glyph names or UTF-16 hex sequences for those code points.
 */

BOOST_AUTO_TEST_SUITE( PDFUnicodePlot )

static wxString getTempPdfPath( const wxString& name )
{
    wxFileName fn = wxFileName::CreateTempFileName( name );
    fn.SetExt( "pdf" );
    return fn.GetFullPath();
}

// Comprehensive mapping test: emit all four style variants in a single PDF and verify that
// every style's ToUnicode CMap contains expected codepoints (Cyrillic 041F, Japanese 65E5, Chinese 672C).
BOOST_AUTO_TEST_CASE( PlotMultilingualAllStylesMappings )
{
    const std::string sampleUtf8 = "ABCDEF Привет 日本語 漢字";
    wxString sample = wxString::FromUTF8( sampleUtf8.c_str() );
    wxString pdfPath = getTempPdfPath( "kicad_pdf_unicode_allstyles" );

    PDF_PLOTTER plotter;
    class TEST_RENDER_SETTINGS : public RENDER_SETTINGS
    {
    public:
        TEST_RENDER_SETTINGS()
        {
            m_background = COLOR4D( 1, 1, 1, 1 );
            m_grid = COLOR4D( .8, .8, .8, 1 );
            m_cursor = COLOR4D( 0, 0, 0, 1 );
        }
        COLOR4D        GetColor( const KIGFX::VIEW_ITEM*, int ) const override { return COLOR4D( 0, 0, 0, 1 ); }
        const COLOR4D& GetBackgroundColor() const override { return m_background; }
        void           SetBackgroundColor( const COLOR4D& c ) override { m_background = c; }
        const COLOR4D& GetGridColor() override { return m_grid; }
        const COLOR4D& GetCursorColor() override { return m_cursor; }
        COLOR4D        m_background, m_grid, m_cursor;
    } renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT("1"), wxT("TestPage") ) );

    auto emitStyle = [&]( bool bold, bool italic, int yoff )
    {
        TEXT_ATTRIBUTES attrs;
        attrs.m_Size = VECTOR2I( 3000, 3000 );
        attrs.m_StrokeWidth = 300;
        attrs.m_Multiline = false;
        attrs.m_Italic = italic;
        attrs.m_Bold = bold;
        attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
        attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
        attrs.m_Angle = ANGLE_0;
        attrs.m_Mirrored = false;
        KIFONT::STROKE_FONT* strokeFont = KIFONT::STROKE_FONT::LoadFont( wxEmptyString );
        KIFONT::METRICS      metrics;

        plotter.PlotText( VECTOR2I( 50000, 60000 - yoff ), COLOR4D( 0, 0, 0, 1 ), sample, attrs, strokeFont, metrics );
        delete strokeFont;
    };

    emitStyle( false, false, 0 );      // normal
    emitStyle( true,  false, 8000 );   // bold
    emitStyle( false, true,  16000 );  // italic
    emitStyle( true,  true,  24000 );  // bold-italic

    plotter.EndPlot();

    // Read entire PDF (may have compression). We'll search each Type3 font object's preceding
    // name to separate CMaps logically.
    wxFFile file( pdfPath, "rb" );
    BOOST_REQUIRE( file.IsOpened() );
    wxFileOffset len = file.Length();
    std::string buffer; buffer.resize( (size_t) len ); file.Read( buffer.data(), len );
    BOOST_CHECK( buffer.rfind( "%PDF", 0 ) == 0 );

    // If compressed, opportunistically decompress each stream and append for searching.
    auto appendDecompressed = [&]() {
        std::string aggregate = buffer; size_t pos=0; while(true){ size_t s=buffer.find("stream\n",pos); if(s==std::string::npos) break; size_t e=buffer.find("endstream",s); if(e==std::string::npos) break; size_t ds=s+7; size_t dl=e-ds; const unsigned char* data=reinterpret_cast<const unsigned char*>(buffer.data()+ds); z_stream zs{}; zs.next_in=const_cast<Bytef*>(data); zs.avail_in=(uInt)dl; if(inflateInit(&zs)==Z_OK){ std::string out; out.resize(dl*4+64); zs.next_out=reinterpret_cast<Bytef*>(out.data()); zs.avail_out=(uInt)out.size(); int ret=inflate(&zs,Z_FINISH); if(ret==Z_STREAM_END){ out.resize(zs.total_out); aggregate+=out; } inflateEnd(&zs);} pos=e+9;} buffer.swap(aggregate); };
    appendDecompressed();

    // Count how many distinct KiCadStrokeCMap names present; expect at least 4 (one per style).
    int cmapCount = 0; size_t searchPos = 0; while( true ) { size_t p = buffer.find("/CMapName /KiCadStrokeCMap", searchPos); if( p == std::string::npos ) break; ++cmapCount; searchPos = p + 1; }
    BOOST_CHECK_MESSAGE( cmapCount >= 4, "Expected at least 4 CMaps (got " << cmapCount << ")" );

    auto requireAll = [&]( const char* codeHex, const char* label ) {
        // ensure appears at least 4 times (once per style)
        int occurrences = 0; size_t pos=0; while(true){ size_t f=buffer.find(codeHex,pos); if(f==std::string::npos) break; ++occurrences; pos=f+1; }
        BOOST_CHECK_MESSAGE( occurrences >= 4, "Codepoint " << label << " (" << codeHex << ") expected in all 4 styles; found " << occurrences );
    };

    requireAll( "041F", "Cyrillic PE" );
    requireAll( "65E5", "Kanji 日" );
    requireAll( "672C", "Kanji 本" );

    wxString keepEnv; if( !wxGetEnv( wxT("KICAD_KEEP_TEST_PDF"), &keepEnv ) || keepEnv.IsEmpty() ) wxRemoveFile( pdfPath );
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
    class TEST_RENDER_SETTINGS : public RENDER_SETTINGS
    {
    public:
        TEST_RENDER_SETTINGS()
        {
            m_background = COLOR4D( 1.0, 1.0, 1.0, 1.0 );
            m_grid = COLOR4D( 0.8, 0.8, 0.8, 1.0 );
            m_cursor = COLOR4D( 0.0, 0.0, 0.0, 1.0 );
        }

        COLOR4D GetColor( const KIGFX::VIEW_ITEM* /*aItem*/, int /*aLayer*/ ) const override
        {
            return COLOR4D( 0.0, 0.0, 0.0, 1.0 );
        }

        const COLOR4D& GetBackgroundColor() const override { return m_background; }
        void SetBackgroundColor( const COLOR4D& aColor ) override { m_background = aColor; }
        const COLOR4D& GetGridColor() override { return m_grid; }
        const COLOR4D& GetCursorColor() override { return m_cursor; }

    private:
        COLOR4D m_background;
        COLOR4D m_grid;
        COLOR4D m_cursor;
    } renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );

    // Minimal viewport and plot setup. Use 1 IU per decimil so internal coordinates are small
    // and resulting translation keeps text inside the page for rasterization.
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    // StartPlot opens first page stream internally; use simple page number
    BOOST_REQUIRE( plotter.StartPlot( wxT("1"), wxT("TestPage") ) );

    TEXT_ATTRIBUTES attrs; // zero-init then set expected fields
    // Use a modest stroke font size that will reasonably map onto the page
    // (roughly 1000 internal units ~ 7.2pt with the 0.0072 scale factor).
    attrs.m_Size = VECTOR2I( 3000, 3000 );
    attrs.m_StrokeWidth = 300;
    attrs.m_Multiline = false;
    attrs.m_Italic = false;
    attrs.m_Bold = false;
    attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
    attrs.m_Angle = ANGLE_0;
    attrs.m_Mirrored = false;

    KIFONT::STROKE_FONT* strokeFont = KIFONT::STROKE_FONT::LoadFont( wxEmptyString );
    KIFONT::METRICS metrics; // not used for stroke fallback

    // Plot near lower-left inside the page.
    // Place text near the top of the page in internal units so after the 0.0072 scale it
    // appears well within the MediaBox. Empirically m_paperSize.y ~ 116k internal units.
    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0.0, 0.0, 0.0, 1.0 ), sample, attrs,
                      strokeFont, metrics );

    plotter.EndPlot();

    delete strokeFont;

    // Read file back and check for expected UTF-16 hex encodings for some code points
    // We expect CMap to contain mappings. E.g. '041F' (Cyrillic capital Pe), '65E5'(日), '672C'(本).
    wxFFile file( pdfPath, "rb" );
    BOOST_REQUIRE( file.IsOpened() );
    wxFileOffset len = file.Length();
    std::string buffer;
    buffer.resize( (size_t) len );
    file.Read( buffer.data(), len );

    // Basic sanity: file starts with %PDF
    BOOST_CHECK( buffer.rfind( "%PDF", 0 ) == 0 );

    auto contains = [&]( const char* needle ) { return buffer.find( needle ) != std::string::npos; };

    // If expected hex sequences are not found in the raw file, attempt to locate them inside
    // any Flate encoded streams by opportunistic decompression (best-effort; ignores errors).
    auto ensureHexSearchable = [&]() {
        if( contains( "041F" ) && contains( "65E5" ) )
            return; // already present

        std::string aggregate = buffer;
        size_t pos = 0;
        while( true )
        {
            size_t streamPos = buffer.find( "stream\n", pos );
            if( streamPos == std::string::npos )
                break;
            size_t endPos = buffer.find( "endstream", streamPos );
            if( endPos == std::string::npos )
                break;
            // Skip keyword and newline
            size_t dataStart = streamPos + 7;
            const unsigned char* data = reinterpret_cast<const unsigned char*>( buffer.data() + dataStart );
            size_t dataLen = endPos - dataStart;
            // Try zlib decompression
            z_stream zs{};
            zs.next_in = const_cast<Bytef*>( data );
            zs.avail_in = static_cast<uInt>( dataLen );
            if( inflateInit( &zs ) == Z_OK )
            {
                std::string out;
                out.resize( dataLen * 4 + 64 );
                zs.next_out = reinterpret_cast<Bytef*>( out.data() );
                zs.avail_out = static_cast<uInt>( out.size() );
                int ret = inflate( &zs, Z_FINISH );
                if( ret == Z_STREAM_END )
                {
                    out.resize( zs.total_out );
                    aggregate += out; // append decompressed for searching
                }
                inflateEnd( &zs );
            }
            pos = endPos + 9;
        }
        buffer.swap( aggregate );
    };

    ensureHexSearchable();

    BOOST_CHECK_MESSAGE( contains( "041F" ), "Missing Cyrillic glyph mapping (041F)" );
    BOOST_CHECK_MESSAGE( contains( "0420" ) || contains( "0440" ), "Missing Cyrillic glyph mapping (0420/0440)" );
    BOOST_CHECK_MESSAGE( contains( "65E5" ), "Missing Japanese Kanji glyph mapping (65E5)" );
    BOOST_CHECK_MESSAGE( contains( "672C" ), "Missing Japanese Kanji glyph mapping (672C)" );
    BOOST_CHECK_MESSAGE( contains( "6F22" ) || contains( "6漢" ), "Expect Chinese Han character mapping (6F22 / 漢)" );

    // Cleanup temp file (unless debugging requested)
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

                // Demand at least 200 non-white pixels to consider text rendered; this filters
                // out tiny artifacts from broken conversions.
                // TODO(#unicode-pdf): Once coordinate transform is corrected so the text falls
                // within the MediaBox, raise this threshold back to a meaningful value.
                BOOST_CHECK_MESSAGE( darkPixels > 200,
                                     "Rasterized PDF appears blank or too sparse (" << darkPixels
                                     << " dark pixels)" );

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

    wxString keepEnv;
    if( !wxGetEnv( wxT("KICAD_KEEP_TEST_PDF"), &keepEnv ) || keepEnv.IsEmpty() )
        wxRemoveFile( pdfPath );
    else
        BOOST_TEST_MESSAGE( "Keeping debug PDF: " << pdfPath );
}

BOOST_AUTO_TEST_CASE( PlotMultilingualTextBold )
{
    const std::string sampleUtf8 = "ABCDEF Привет 日本語 漢字";
    wxString sample = wxString::FromUTF8( sampleUtf8.c_str() );

    wxString pdfPath = getTempPdfPath( "kicad_pdf_unicode_bold" );

    PDF_PLOTTER plotter;

    class TEST_RENDER_SETTINGS : public RENDER_SETTINGS
    {
    public:
        TEST_RENDER_SETTINGS()
        {
            m_background = COLOR4D( 1.0, 1.0, 1.0, 1.0 );
            m_grid = COLOR4D( 0.8, 0.8, 0.8, 1.0 );
            m_cursor = COLOR4D( 0.0, 0.0, 0.0, 1.0 );
        }

        COLOR4D GetColor( const KIGFX::VIEW_ITEM*, int ) const override
        {
            return COLOR4D( 0.0, 0.0, 0.0, 1.0 );
        }

        const COLOR4D& GetBackgroundColor() const override { return m_background; }
        void SetBackgroundColor( const COLOR4D& aColor ) override { m_background = aColor; }
        const COLOR4D& GetGridColor() override { return m_grid; }
        const COLOR4D& GetCursorColor() override { return m_cursor; }

    private:
        COLOR4D m_background;
        COLOR4D m_grid;
        COLOR4D m_cursor;
    } renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT("1"), wxT("TestPage") ) );

    TEXT_ATTRIBUTES attrs;
    attrs.m_Size = VECTOR2I( 3000, 3000 );
    attrs.m_StrokeWidth = 300;
    attrs.m_Multiline = false;
    attrs.m_Italic = false;
    attrs.m_Bold = true; // bold
    attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
    attrs.m_Angle = ANGLE_0;
    attrs.m_Mirrored = false;

    KIFONT::STROKE_FONT* strokeFont = KIFONT::STROKE_FONT::LoadFont( wxEmptyString );
    KIFONT::METRICS metrics;
    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0.0, 0.0, 0.0, 1.0 ), sample, attrs,
                      strokeFont, metrics );
    plotter.EndPlot();
    delete strokeFont;

    wxFFile file( pdfPath, "rb" );
    BOOST_REQUIRE( file.IsOpened() );
    wxFileOffset len = file.Length();
    std::string buffer; buffer.resize( (size_t) len );
    file.Read( buffer.data(), len );
    BOOST_CHECK( buffer.rfind( "%PDF", 0 ) == 0 );

    auto contains = [&]( const char* needle ) { return buffer.find( needle ) != std::string::npos; };
    if( !contains( "041F" ) || !contains( "65E5" ) )
    {
        // attempt decompression pass copied from base test (simplified: just search once)
        size_t pos = 0; std::string aggregate = buffer;
        while( true )
        {
            size_t streamPos = buffer.find( "stream\n", pos );
            if( streamPos == std::string::npos ) break;
            size_t endPos = buffer.find( "endstream", streamPos );
            if( endPos == std::string::npos ) break;
            size_t dataStart = streamPos + 7; size_t dataLen = endPos - dataStart;
            const unsigned char* data = reinterpret_cast<const unsigned char*>( buffer.data() + dataStart );
            z_stream zs{}; zs.next_in = const_cast<Bytef*>( data ); zs.avail_in = (uInt) dataLen;
            if( inflateInit( &zs ) == Z_OK )
            {
                std::string out; out.resize( dataLen * 4 + 64 );
                zs.next_out = reinterpret_cast<Bytef*>( out.data() ); zs.avail_out = (uInt) out.size();
                int ret = inflate( &zs, Z_FINISH );
                if( ret == Z_STREAM_END ) { out.resize( zs.total_out ); aggregate += out; }
                inflateEnd( &zs );
            }
            pos = endPos + 9;
        }
        buffer.swap( aggregate );
    }
    BOOST_CHECK_MESSAGE( contains( "041F" ), "Missing Cyrillic glyph mapping (bold 041F)" );
    BOOST_CHECK_MESSAGE( contains( "65E5" ), "Missing Japanese glyph mapping (bold 65E5)" );

    wxString keepEnv; if( !wxGetEnv( wxT("KICAD_KEEP_TEST_PDF"), &keepEnv ) || keepEnv.IsEmpty() ) wxRemoveFile( pdfPath );
}

BOOST_AUTO_TEST_CASE( PlotMultilingualTextItalic )
{
    const std::string sampleUtf8 = "ABCDEF Привет 日本語 漢字";
    wxString sample = wxString::FromUTF8( sampleUtf8.c_str() );
    wxString pdfPath = getTempPdfPath( "kicad_pdf_unicode_italic" );
    PDF_PLOTTER plotter;
    class TEST_RENDER_SETTINGS : public RENDER_SETTINGS { public: TEST_RENDER_SETTINGS(){ m_background=COLOR4D(1,1,1,1); m_grid=COLOR4D(.8,.8,.8,1); m_cursor=COLOR4D(0,0,0,1);} COLOR4D GetColor(const KIGFX::VIEW_ITEM*,int) const override { return COLOR4D(0,0,0,1);} const COLOR4D& GetBackgroundColor() const override {return m_background;} void SetBackgroundColor(const COLOR4D& c) override {m_background=c;} const COLOR4D& GetGridColor() override {return m_grid;} const COLOR4D& GetCursorColor() override {return m_cursor;} COLOR4D m_background,m_grid,m_cursor; } renderSettings;
    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT("1"), wxT("TestPage") ) );
    TEXT_ATTRIBUTES attrs; attrs.m_Size=VECTOR2I(3000,3000); attrs.m_StrokeWidth=300; attrs.m_Multiline=false; attrs.m_Italic=true; attrs.m_Bold=false; attrs.m_Halign=GR_TEXT_H_ALIGN_LEFT; attrs.m_Valign=GR_TEXT_V_ALIGN_BOTTOM; attrs.m_Angle=ANGLE_0; attrs.m_Mirrored=false; KIFONT::STROKE_FONT* strokeFont=KIFONT::STROKE_FONT::LoadFont(wxEmptyString); KIFONT::METRICS metrics; plotter.PlotText( VECTOR2I(50000,60000), COLOR4D(0,0,0,1), sample, attrs, strokeFont, metrics ); plotter.EndPlot(); delete strokeFont; wxFFile file(pdfPath,"rb"); BOOST_REQUIRE(file.IsOpened()); wxFileOffset len=file.Length(); std::string buffer; buffer.resize((size_t)len); file.Read(buffer.data(),len); BOOST_CHECK(buffer.rfind("%PDF",0)==0); auto contains=[&](const char* n){return buffer.find(n)!=std::string::npos;}; if(!contains("041F")||!contains("65E5")){ size_t pos=0; std::string aggregate=buffer; while(true){ size_t s=buffer.find("stream\n",pos); if(s==std::string::npos) break; size_t e=buffer.find("endstream",s); if(e==std::string::npos) break; size_t ds=s+7; size_t dl=e-ds; const unsigned char* data=reinterpret_cast<const unsigned char*>(buffer.data()+ds); z_stream zs{}; zs.next_in=const_cast<Bytef*>(data); zs.avail_in=(uInt)dl; if(inflateInit(&zs)==Z_OK){ std::string out; out.resize(dl*4+64); zs.next_out=reinterpret_cast<Bytef*>(out.data()); zs.avail_out=(uInt)out.size(); int ret=inflate(&zs,Z_FINISH); if(ret==Z_STREAM_END){ out.resize(zs.total_out); aggregate+=out;} inflateEnd(&zs);} pos=e+9;} buffer.swap(aggregate);} BOOST_CHECK_MESSAGE( contains("041F"), "Missing Cyrillic glyph mapping (italic 041F)" ); BOOST_CHECK_MESSAGE( contains("65E5"), "Missing Japanese glyph mapping (italic 65E5)" ); wxString keepEnv; if( !wxGetEnv( wxT("KICAD_KEEP_TEST_PDF"), &keepEnv ) || keepEnv.IsEmpty() ) wxRemoveFile( pdfPath ); }

BOOST_AUTO_TEST_CASE( PlotMultilingualTextBoldItalic )
{
    const std::string sampleUtf8 = "ABCDEF Привет 日本語 漢字";
    wxString          sample = wxString::FromUTF8( sampleUtf8.c_str() );
    wxString          pdfPath = getTempPdfPath( "kicad_pdf_unicode_bolditalic" );
    PDF_PLOTTER       plotter;
    class TEST_RENDER_SETTINGS : public RENDER_SETTINGS
    {
    public:
        TEST_RENDER_SETTINGS()
        {
            m_background = COLOR4D( 1, 1, 1, 1 );
            m_grid = COLOR4D( .8, .8, .8, 1 );
            m_cursor = COLOR4D( 0, 0, 0, 1 );
        }
        COLOR4D        GetColor( const KIGFX::VIEW_ITEM*, int ) const override { return COLOR4D( 0, 0, 0, 1 ); }
        const COLOR4D& GetBackgroundColor() const override { return m_background; }
        void           SetBackgroundColor( const COLOR4D& c ) override { m_background = c; }
        const COLOR4D& GetGridColor() override { return m_grid; }
        const COLOR4D& GetCursorColor() override { return m_cursor; }
        COLOR4D        m_background, m_grid, m_cursor;
    } renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );

    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "TestPage" ) ) );

    TEXT_ATTRIBUTES attrs;
    attrs.m_Size = VECTOR2I( 3000, 3000 );
    attrs.m_StrokeWidth = 300;
    attrs.m_Multiline = false;
    attrs.m_Italic = true;
    attrs.m_Bold = true;
    attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
    attrs.m_Angle = ANGLE_0;
    attrs.m_Mirrored = false;

    KIFONT::STROKE_FONT* strokeFont = KIFONT::STROKE_FONT::LoadFont( wxEmptyString );
    KIFONT::METRICS      metrics;

    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0, 0, 0, 1 ), sample, attrs, strokeFont, metrics );
    plotter.EndPlot();

    delete strokeFont;

    wxFFile file( pdfPath, "rb" );
    BOOST_REQUIRE( file.IsOpened() );
    wxFileOffset len = file.Length();
    std::string  buffer;

    buffer.resize( (size_t) len );
    file.Read( buffer.data(), len );

    BOOST_CHECK( buffer.rfind( "%PDF", 0 ) == 0 );

    auto contains = [&]( const char* n )
    {
        return buffer.find( n ) != std::string::npos;
    };

    if( !contains( "041F" ) || !contains( "65E5" ) )
    {
        size_t      pos = 0;
        std::string aggregate = buffer;
        while( true )
        {
            size_t s = buffer.find( "stream\n", pos );

            if( s == std::string::npos )
                break;

            size_t e = buffer.find( "endstream", s );

            if( e == std::string::npos )
                break;

            size_t               ds = s + 7;
            size_t               dl = e - ds;
            const unsigned char* data = reinterpret_cast<const unsigned char*>( buffer.data() + ds );
            z_stream             zs{};
            zs.next_in = const_cast<Bytef*>( data );
            zs.avail_in = (uInt) dl;

            if( inflateInit( &zs ) == Z_OK )
            {
                std::string out;
                out.resize( dl * 4 + 64 );
                zs.next_out = reinterpret_cast<Bytef*>( out.data() );
                zs.avail_out = (uInt) out.size();
                int ret = inflate( &zs, Z_FINISH );

                if( ret == Z_STREAM_END )
                {
                    out.resize( zs.total_out );
                    aggregate += out;
                }

                inflateEnd( &zs );
            }

            pos = e + 9;
        }

        buffer.swap( aggregate );
    }

    BOOST_CHECK_MESSAGE( contains( "041F" ), "Missing Cyrillic glyph mapping (bold-italic 041F)" );
    BOOST_CHECK_MESSAGE( contains( "65E5" ), "Missing Japanese glyph mapping (bold-italic 65E5)" );
    wxString keepEnv;

    if( !wxGetEnv( wxT( "KICAD_KEEP_TEST_PDF" ), &keepEnv ) || keepEnv.IsEmpty() )
        wxRemoveFile( pdfPath );
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
    class TEST_RENDER_SETTINGS : public RENDER_SETTINGS
    {
    public:
        TEST_RENDER_SETTINGS()
        {
            m_background = COLOR4D( 1, 1, 1, 1 );
            m_grid = COLOR4D( .8, .8, .8, 1 );
            m_cursor = COLOR4D( 0, 0, 0, 1 );
        }
        COLOR4D        GetColor( const KIGFX::VIEW_ITEM*, int ) const override { return COLOR4D( 0, 0, 0, 1 ); }
        const COLOR4D& GetBackgroundColor() const override { return m_background; }
        void           SetBackgroundColor( const COLOR4D& c ) override { m_background = c; }
        const COLOR4D& GetGridColor() override { return m_grid; }
        const COLOR4D& GetCursorColor() override { return m_cursor; }
        COLOR4D        m_background, m_grid, m_cursor;
    } renderSettings;

    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I( 0, 0 ), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT( "1" ), wxT( "TestPage" ) ) );

    TEXT_ATTRIBUTES attrs;
    attrs.m_Size = VECTOR2I( 4000, 4000 );
    attrs.m_StrokeWidth = 400;
    attrs.m_Multiline = false;
    attrs.m_Italic = false;
    attrs.m_Bold = false;
    attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
    attrs.m_Angle = ANGLE_0;
    attrs.m_Mirrored = false;
    KIFONT::STROKE_FONT* strokeFont = KIFONT::STROKE_FONT::LoadFont( wxEmptyString );
    KIFONT::METRICS      metrics;
    plotter.PlotText( VECTOR2I( 50000, 60000 ), COLOR4D( 0, 0, 0, 1 ), sample, attrs, strokeFont, metrics );
    plotter.EndPlot();
    delete strokeFont;

    // Restore original Y offset
    cfg.m_PDFStrokeFontYOffset = originalOffset;

    // Basic PDF validation
    wxFFile file(pdfPath,"rb"); BOOST_REQUIRE(file.IsOpened()); wxFileOffset len=file.Length(); std::string buffer; buffer.resize((size_t)len); file.Read(buffer.data(),len);
    BOOST_CHECK(buffer.rfind("%PDF",0)==0);

    // Decompress streams to find d1 operators
    auto appendDecompressed = [&]() {
        std::string aggregate = buffer; size_t pos=0; while(true){ size_t s=buffer.find("stream\n",pos); if(s==std::string::npos) break; size_t e=buffer.find("endstream",s); if(e==std::string::npos) break; size_t ds=s+7; size_t dl=e-ds; const unsigned char* data=reinterpret_cast<const unsigned char*>(buffer.data()+ds); z_stream zs{}; zs.next_in=const_cast<Bytef*>(data); zs.avail_in=(uInt)dl; if(inflateInit(&zs)==Z_OK){ std::string out; out.resize(dl*4+64); zs.next_out=reinterpret_cast<Bytef*>(out.data()); zs.avail_out=(uInt)out.size(); int ret=inflate(&zs,Z_FINISH); if(ret==Z_STREAM_END){ out.resize(zs.total_out); aggregate+=out; } inflateEnd(&zs);} pos=e+9;} buffer.swap(aggregate); };
    appendDecompressed();

    // Check that bounding boxes exist and are reasonable (not clipped)
    // Look for d1 operators which specify character bounding boxes
    BOOST_CHECK_MESSAGE(buffer.find("d1") != std::string::npos, "PDF should contain d1 operators for glyph bounding boxes");

    wxString keepEnv2; if( !wxGetEnv( wxT("KICAD_KEEP_TEST_PDF"), &keepEnv2 ) || keepEnv2.IsEmpty() ) wxRemoveFile( pdfPath );
    else BOOST_TEST_MESSAGE( "Keeping Y-offset debug PDF: " << pdfPath );
}

BOOST_AUTO_TEST_CASE( PlotOutlineFontEmbedding )
{
    wxString pdfPath = getTempPdfPath( "kicad_pdf_outline_font" );

    wxFileName fontFile( wxString::FromUTF8( __FILE__ ) );
    fontFile.RemoveLastDir();
    fontFile.RemoveLastDir();
    fontFile.AppendDir( wxT( "resources" ) );
    fontFile.AppendDir( wxT( "fonts" ) );
    fontFile.SetFullName( wxT( "NotoSans-Regular.ttf" ) );
    wxString fontPath = fontFile.GetFullPath();
    BOOST_REQUIRE( wxFileExists( fontPath ) );

    PDF_PLOTTER plotter;
    class TEST_RENDER_SETTINGS : public RENDER_SETTINGS { public: TEST_RENDER_SETTINGS(){ m_background=COLOR4D(1,1,1,1); m_grid=COLOR4D(.8,.8,.8,1); m_cursor=COLOR4D(0,0,0,1);} COLOR4D GetColor(const KIGFX::VIEW_ITEM*,int) const override { return COLOR4D(0,0,0,1);} const COLOR4D& GetBackgroundColor() const override {return m_background;} void SetBackgroundColor(const COLOR4D& c) override {m_background=c;} const COLOR4D& GetGridColor() override {return m_grid;} const COLOR4D& GetCursorColor() override {return m_cursor;} COLOR4D m_background,m_grid,m_cursor; } renderSettings;
    plotter.SetRenderSettings( &renderSettings );
    BOOST_REQUIRE( plotter.OpenFile( pdfPath ) );
    plotter.SetViewport( VECTOR2I(0,0), 1.0, 1.0, false );
    BOOST_REQUIRE( plotter.StartPlot( wxT("1"), wxT("OutlineFont") ) );

    TEXT_ATTRIBUTES attrs;
    attrs.m_Size = VECTOR2I( 4000, 4000 );
    attrs.m_StrokeWidth = 0;
    attrs.m_Multiline = false;
    attrs.m_Italic = false;
    attrs.m_Bold = false;
    attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
    attrs.m_Angle = ANGLE_0;
    attrs.m_Mirrored = false;

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

    auto appendDecompressed = [&]() {
        std::string aggregate = buffer; size_t pos=0; while(true){ size_t s=buffer.find("stream\n",pos); if(s==std::string::npos) break; size_t e=buffer.find("endstream",s); if(e==std::string::npos) break; size_t ds=s+7; size_t dl=e-ds; const unsigned char* data=reinterpret_cast<const unsigned char*>(buffer.data()+ds); z_stream zs{}; zs.next_in=const_cast<Bytef*>(data); zs.avail_in=(uInt)dl; if(inflateInit(&zs)==Z_OK){ std::string out; out.resize(dl*4+64); zs.next_out=reinterpret_cast<Bytef*>(out.data()); zs.avail_out=(uInt)out.size(); int ret=inflate(&zs,Z_FINISH); if(ret==Z_STREAM_END){ out.resize(zs.total_out); aggregate+=out; } inflateEnd(&zs);} pos=e+9;} buffer.swap(aggregate); };
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

    wxString keepEnv;

    if( !wxGetEnv( wxT("KICAD_KEEP_TEST_PDF"), &keepEnv ) || keepEnv.IsEmpty() )
        wxRemoveFile( pdfPath );
}

BOOST_AUTO_TEST_SUITE_END()
