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

#include <memory>
#include <vector>

#include <wx/filename.h>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <font/font.h>
#include <font/glyph.h>
#include <font/outline_font.h>

BOOST_AUTO_TEST_SUITE( OutlineFont )


static wxString getTestFontPath( const wxString& aFileName )
{
    wxFileName fontFile( KI_TEST::GetTestDataRootDir() );
    fontFile.RemoveLastDir();
    fontFile.AppendDir( wxT( "resources" ) );
    fontFile.AppendDir( wxT( "fonts" ) );
    fontFile.SetFullName( aFileName );

    return fontFile.GetFullPath();
}


static KIFONT::OUTLINE_FONT* loadTestOutlineFont( const wxString& aFontName,
                                                  const wxString& aFileName )
{
    wxString fontPath = getTestFontPath( aFileName );
    BOOST_REQUIRE( wxFileExists( fontPath ) );

    std::vector<wxString> embeddedFonts{ fontPath };
    KIFONT::FONT*         font = KIFONT::FONT::GetFont( aFontName, false, false, &embeddedFonts );

    BOOST_REQUIRE( font );
    BOOST_REQUIRE( font->IsOutline() );

    return static_cast<KIFONT::OUTLINE_FONT*>( font );
}


// Legacy "symbol" fonts place their glyphs in the U+F000..U+F0FF private-use range and carry no
// Unicode charmap that maps Basic Latin, so forcing the Unicode charmap maps every character to
// .notdef and text renders as tofu boxes. Drive the full load-and-shape path and confirm such a
// font resolves the symbol charmap and produces distinct ASCII glyphs.
BOOST_AUTO_TEST_CASE( SymbolFontRendersAsciiGlyphs )
{
    wxString fontPath = getTestFontPath( wxT( "symbol_pua_test.ttf" ) );
    BOOST_REQUIRE( wxFileExists( fontPath ) );

    std::vector<wxString> embeddedFonts{ fontPath };
    KIFONT::FONT*         font = KIFONT::FONT::GetFont( wxT( "KiCadSymbolTest" ), false, false,
                                                        &embeddedFonts );

    BOOST_REQUIRE( font );
    BOOST_REQUIRE( font->IsOutline() );

    KIFONT::OUTLINE_FONT* outline = static_cast<KIFONT::OUTLINE_FONT*>( font );
    FT_Face               face = outline->GetFace();

    // A Unicode charmap would mean fontconfig substituted a different font, or the symbol charmap
    // was not selected; either way the glyph lookup that resolves the symbol layout would not run.
    BOOST_REQUIRE( face );
    BOOST_REQUIRE( face->charmap );
    BOOST_CHECK_EQUAL( face->charmap->encoding, FT_ENCODING_MS_SYMBOL );

    std::vector<std::unique_ptr<KIFONT::GLYPH>> glyphs;
    outline->GetTextAsGlyphs( nullptr, &glyphs, wxT( "AB" ), VECTOR2I( 10000, 10000 ),
                              VECTOR2I( 0, 0 ), ANGLE_0, false, VECTOR2I( 0, 0 ), 0 );

    BOOST_REQUIRE_EQUAL( glyphs.size(), 2 );

    // The test font draws 'A' wider than 'B'; tofu would map both to the same .notdef glyph.
    double widthA = glyphs[0]->BoundingBox().GetWidth();
    double widthB = glyphs[1]->BoundingBox().GetWidth();

    BOOST_CHECK_GT( widthA, 0.0 );
    BOOST_CHECK_GT( widthB, 0.0 );
    BOOST_CHECK_GT( widthA, widthB );
}


// A normal Unicode font must keep its Unicode charmap and still map Basic Latin directly.
BOOST_AUTO_TEST_CASE( UnicodeFontKeepsUnicodeCharmap )
{
    wxString fontPath = getTestFontPath( wxT( "NotoSans-Regular.ttf" ) );
    BOOST_REQUIRE( wxFileExists( fontPath ) );

    std::vector<wxString> embeddedFonts{ fontPath };
    KIFONT::FONT*         font = KIFONT::FONT::GetFont( wxT( "Noto Sans" ), false, false,
                                                        &embeddedFonts );

    BOOST_REQUIRE( font );
    BOOST_REQUIRE( font->IsOutline() );

    KIFONT::OUTLINE_FONT* outline = static_cast<KIFONT::OUTLINE_FONT*>( font );
    FT_Face               face = outline->GetFace();

    BOOST_REQUIRE( face );
    BOOST_REQUIRE( face->charmap );
    BOOST_CHECK_EQUAL( face->charmap->encoding, FT_ENCODING_UNICODE );
    BOOST_CHECK( FT_Get_Char_Index( face, 'A' ) != 0 );
}


// The line spacing multiplier is FT_Face::height / FT_Face::units_per_EM. Both fields are integers,
// so an integer division truncated the fractional font-height ratio (and produced 0 for fonts with
// height < units_per_EM), leaving multiline text with lines too close together or overlapping. Drive
// GetInterline and confirm the fractional ratio survives. NotoSans has height 1362 over 1000 units,
// so the true 1.362 ratio truncated to 1.
BOOST_AUTO_TEST_CASE( InterlinePreservesFractionalFontHeight )
{
    KIFONT::OUTLINE_FONT* outline = loadTestOutlineFont( wxT( "Noto Sans" ),
                                                         wxT( "NotoSans-Regular.ttf" ) );
    FT_Face               face = outline->GetFace();

    BOOST_REQUIRE( face );
    BOOST_REQUIRE_GT( face->units_per_EM, 0 );
    BOOST_REQUIRE_GT( face->height, face->units_per_EM );

    const double           glyphHeight = 10000.0;
    const KIFONT::METRICS& metrics = KIFONT::METRICS::Default();

    double ratio = static_cast<double>( face->height ) / static_cast<double>( face->units_per_EM );
    double expected = metrics.GetInterline( glyphHeight * ratio );

    BOOST_CHECK_CLOSE( outline->GetInterline( glyphHeight, metrics ), expected, 1e-6 );

    // The integer-division bug truncated the ratio to 1.0, so the correct spacing must exceed it.
    BOOST_CHECK_GT( outline->GetInterline( glyphHeight, metrics ),
                    metrics.GetInterline( glyphHeight ) );
}


BOOST_AUTO_TEST_SUITE_END()
