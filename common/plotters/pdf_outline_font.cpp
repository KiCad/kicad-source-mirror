/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <plotters/pdf_outline_font.h>
#include <trace_helpers.h>
#include <wx/log.h>
#include <cstdlib>

#include <algorithm>
#include <cctype>
#include <limits>
#include <cmath>

#include <fmt/format.h>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>
#include <wx/ffile.h>

#include <core/utf8.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <font/font.h>

namespace
{
std::string formatUnicodeHex( uint32_t aCodepoint )
{
    if( aCodepoint <= 0xFFFF )
        return fmt::format( "{:04X}", aCodepoint );

    if( aCodepoint <= 0x10FFFF )
    {
        uint32_t value = aCodepoint - 0x10000;
        uint16_t high = 0xD800 + ( value >> 10 );
        uint16_t low = 0xDC00 + ( value & 0x3FF );
        return fmt::format( "{:04X}{:04X}", high, low );
    }

    return std::string( "003F" );
}

std::u32string utf8ToU32( const std::string& aUtf8 )
{
    std::u32string result;
    UTF8           utf8( aUtf8.c_str() );

    for( auto it = utf8.ubegin(); it < utf8.uend(); ++it )
        result.push_back( static_cast<uint32_t>( *it ) );

    return result;
}

std::string generateSubsetPrefix( unsigned aSubsetIndex )
{
    static constexpr char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string           prefix( 6, 'A' );

    for( int ii = 5; ii >= 0; --ii )
    {
        prefix[ii] = letters[ aSubsetIndex % 26 ];
        aSubsetIndex /= 26;
    }

    return prefix;
}

double unitsToPdf( double aValue, double aUnitsPerEm )
{
    if( aUnitsPerEm == 0.0 )
        return 0.0;

    return aValue * 1000.0 / aUnitsPerEm;
}
}

bool PDF_OUTLINE_FONT_SUBSET::GLYPH_KEY::operator<( const GLYPH_KEY& aOther ) const
{
    if( m_glyphIndex != aOther.m_glyphIndex )
        return m_glyphIndex < aOther.m_glyphIndex;

    return m_unicode < aOther.m_unicode;
}

PDF_OUTLINE_FONT_SUBSET::PDF_OUTLINE_FONT_SUBSET( KIFONT::OUTLINE_FONT* aFont, unsigned aSubsetIndex ) :
        m_font( aFont ),
        m_resourceName( makeResourceName( aSubsetIndex ) ),
        m_baseFontName( makeSubsetName( aFont, aSubsetIndex ) ),
        m_unitsPerEm( 1000.0 ),
        m_ascent( 0.0 ),
        m_descent( 0.0 ),
        m_capHeight( 0.0 ),
        m_italicAngle( 0.0 ),
        m_stemV( 80.0 ),
        m_bboxMinX( 0.0 ),
        m_bboxMinY( 0.0 ),
        m_bboxMaxX( 0.0 ),
        m_bboxMaxY( 0.0 ),
        m_flags( 32 ),
        m_fontDataLoaded( false ),
        m_nextCID( 1 ),
        m_fontFileHandle( -1 ),
        m_fontDescriptorHandle( -1 ),
        m_cidFontHandle( -1 ),
        m_cidMapHandle( -1 ),
        m_toUnicodeHandle( -1 ),
        m_fontHandle( -1 )
{
    FT_Face face = aFont ? aFont->GetFace() : nullptr;

    if( face )
    {
        if( face->units_per_EM > 0 )
            m_unitsPerEm = static_cast<double>( face->units_per_EM );

        m_ascent = unitsToPdf( static_cast<double>( face->ascender ), m_unitsPerEm );
        m_descent = unitsToPdf( static_cast<double>( face->descender ), m_unitsPerEm );
        m_capHeight = unitsToPdf( static_cast<double>( face->bbox.yMax ), m_unitsPerEm );
        m_bboxMinX = unitsToPdf( static_cast<double>( face->bbox.xMin ), m_unitsPerEm );
        m_bboxMinY = unitsToPdf( static_cast<double>( face->bbox.yMin ), m_unitsPerEm );
        m_bboxMaxX = unitsToPdf( static_cast<double>( face->bbox.xMax ), m_unitsPerEm );
        m_bboxMaxY = unitsToPdf( static_cast<double>( face->bbox.yMax ), m_unitsPerEm );

        if( face->style_flags & FT_STYLE_FLAG_ITALIC )
            m_italicAngle = -12.0;
        else if( aFont->IsItalic() )
            m_italicAngle = -12.0;

        if( aFont->IsBold() )
            m_stemV = 140.0;

        if( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
            m_flags |= 1;

        if( aFont->IsItalic() )
            m_flags |= 64;
    }

    m_widths.resize( 1, 0.0 );
    m_cidToGid.resize( 1, 0 );
    m_cidToUnicode.resize( 1 );
}

bool PDF_OUTLINE_FONT_SUBSET::HasGlyphs() const
{
    return m_nextCID > 1;
}

void PDF_OUTLINE_FONT_SUBSET::ensureNotdef()
{
    if( m_widths.empty() )
    {
        m_widths.resize( 1, 0.0 );
        m_cidToGid.resize( 1, 0 );
        m_cidToUnicode.resize( 1 );
    }
}

uint16_t PDF_OUTLINE_FONT_SUBSET::EnsureGlyph( uint32_t aGlyphIndex, const std::u32string& aUnicode )
{
    if( aGlyphIndex == 0 )
        return 0;

    GLYPH_KEY key{ aGlyphIndex, aUnicode };

    auto it = m_glyphMap.find( key );

    if( it != m_glyphMap.end() )
        return it->second;

    ensureNotdef();

    FT_Face face = m_font ? m_font->GetFace() : nullptr;

    if( !face )
        return 0;

    if( FT_Load_Glyph( face, aGlyphIndex, FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING ) )
        return 0;

    // For FT_LOAD_NO_SCALE the advance.x should be in font units (26.6) unless metrics differ.
    // We divide by 64.0 to get raw font units; convert to our internal PDF user units via unitsToPdf.
    double rawAdvance266 = static_cast<double>( face->glyph->advance.x );
    double rawAdvanceFontUnits = rawAdvance266 / 64.0;
    double advance = unitsToPdf( rawAdvanceFontUnits, m_unitsPerEm );

    uint16_t cid = m_nextCID++;

    if( m_widths.size() <= cid )
        m_widths.resize( cid + 1, 0.0 );

    if( m_cidToGid.size() <= cid )
        m_cidToGid.resize( cid + 1, 0 );

    if( m_cidToUnicode.size() <= cid )
        m_cidToUnicode.resize( cid + 1 );

    m_widths[cid] = advance;

    if( std::getenv( "KICAD_DEBUG_FONT_ADV" ) )
    {
        wxLogTrace( tracePdfPlotter,
                    "EnsureGlyph font='%s' gid=%u cid=%u rawAdvance26.6=%f rawAdvanceUnits=%f storedAdvancePdfUnits=%f unitsPerEm=%f", \
                    m_font ? m_font->GetName().ToUTF8().data() : "(null)", (unsigned) aGlyphIndex, (unsigned) cid, \
                    rawAdvance266, rawAdvanceFontUnits, advance, m_unitsPerEm );
    }
    m_cidToGid[cid] = static_cast<uint16_t>( aGlyphIndex );
    m_cidToUnicode[cid] = aUnicode;

    m_glyphMap.emplace( key, cid );

    return cid;
}

const std::vector<uint8_t>& PDF_OUTLINE_FONT_SUBSET::FontFileData()
{
    if( !m_fontDataLoaded )
    {
        m_fontDataLoaded = true;

        if( !m_font )
            return m_fontData;

        wxString fontFile = m_font->GetFileName();

        if( fontFile.IsEmpty() )
            return m_fontData;

        wxFFile file( fontFile, wxT( "rb" ) );

        if( !file.IsOpened() )
            return m_fontData;

        wxFileOffset length = file.Length();

        if( length > 0 )
        {
            m_fontData.resize( static_cast<size_t>( length ) );
            file.Read( m_fontData.data(), length );
        }
    }

    return m_fontData;
}

std::string PDF_OUTLINE_FONT_SUBSET::BuildWidthsArray() const
{
    // Return empty if there are no glyphs beyond .notdef
    if( m_nextCID <= 1 )
        return std::string( "[]" );

    // PDF expects widths in 1000/em units for CIDFontType2 /W array entries.
    // m_widths currently stores advance in PDF user units produced by unitsToPdf().
    // This is a bit of a fudge factor to reconstruct the output width
    double designScale = 0.0072 * 2.25;
    int logCount = 0;

    fmt::memory_buffer buffer;
    fmt::format_to( std::back_inserter( buffer ), "[ 1 [" );

    for( uint16_t cid = 1; cid < m_nextCID; ++cid )
    {
        double adv = m_widths[cid];
        long width1000 = 0;

        if( designScale != 0.0 )
            width1000 = lrint( adv / designScale );

        fmt::format_to( std::back_inserter( buffer ), " {}", width1000 );

        if( std::getenv( "KICAD_DEBUG_FONT_ADV" ) && logCount < 16 )
        {
            wxLogTrace( tracePdfPlotter, "BuildWidthsArray FIXED cid=%u advPdfUnits=%f width1000=%ld", (unsigned) cid, adv, width1000 );
            ++logCount;
        }
    }

    fmt::format_to( std::back_inserter( buffer ), " ] ]" );
    return std::string( buffer.data(), buffer.size() );
}

std::string PDF_OUTLINE_FONT_SUBSET::BuildToUnicodeCMap() const
{
    if( m_nextCID <= 1 )
        return std::string();

    fmt::memory_buffer buffer;

    std::string cmapName = m_baseFontName + "_ToUnicode";

    fmt::format_to( std::back_inserter( buffer ), "/CIDInit /ProcSet findresource begin\n" );
    fmt::format_to( std::back_inserter( buffer ), "12 dict begin\n" );
    fmt::format_to( std::back_inserter( buffer ), "begincmap\n" );
    fmt::format_to( std::back_inserter( buffer ), "/CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >> def\n" );
    fmt::format_to( std::back_inserter( buffer ), "/CMapName /{} def\n", cmapName );
    fmt::format_to( std::back_inserter( buffer ), "/CMapType 2 def\n" );
    fmt::format_to( std::back_inserter( buffer ), "1 begincodespacerange\n" );
    fmt::format_to( std::back_inserter( buffer ), "<0000> <FFFF>\n" );
    fmt::format_to( std::back_inserter( buffer ), "endcodespacerange\n" );

    size_t mappingCount = 0;

    for( uint16_t cid = 1; cid < m_nextCID; ++cid )
    {
        if( !m_cidToUnicode[cid].empty() )
            ++mappingCount;
    }

    fmt::format_to( std::back_inserter( buffer ), "{} beginbfchar\n", mappingCount );

    for( uint16_t cid = 1; cid < m_nextCID; ++cid )
    {
        const std::u32string& unicode = m_cidToUnicode[cid];

        if( unicode.empty() )
            continue;

        fmt::format_to( std::back_inserter( buffer ), "<{:04X}> <", cid );

        for( uint32_t codepoint : unicode )
            fmt::format_to( std::back_inserter( buffer ), "{}", formatUnicodeHex( codepoint ) );

        fmt::format_to( std::back_inserter( buffer ), ">\n" );
    }

    fmt::format_to( std::back_inserter( buffer ), "endbfchar\n" );
    fmt::format_to( std::back_inserter( buffer ), "endcmap\n" );
    fmt::format_to( std::back_inserter( buffer ), "CMapName currentdict /CMap defineresource pop\n" );
    fmt::format_to( std::back_inserter( buffer ), "end\n" );
    fmt::format_to( std::back_inserter( buffer ), "end\n" );

    return std::string( buffer.data(), buffer.size() );
}

std::string PDF_OUTLINE_FONT_SUBSET::BuildCIDToGIDStream() const
{
    std::string data;

    if( m_nextCID == 0 )
        return data;

    data.resize( static_cast<size_t>( m_nextCID ) * 2, 0 );

    for( uint16_t cid = 0; cid < m_nextCID; ++cid )
    {
        uint16_t gid = cid < m_cidToGid.size() ? m_cidToGid[cid] : 0;
        data[ cid * 2 ] = static_cast<char>( ( gid >> 8 ) & 0xFF );
        data[ cid * 2 + 1 ] = static_cast<char>( gid & 0xFF );
    }

    return data;
}

std::string PDF_OUTLINE_FONT_SUBSET::makeResourceName( unsigned aSubsetIndex )
{
    return fmt::format( "/KiCadOutline{}", aSubsetIndex );
}

std::string PDF_OUTLINE_FONT_SUBSET::sanitizeFontName( const wxString& aName )
{
    std::string utf8 = UTF8( aName ).substr();
    std::string sanitized;
    sanitized.reserve( utf8.size() );

    for( unsigned char ch : utf8 )
    {
        if( std::isalnum( ch ) )
            sanitized.push_back( static_cast<char>( ch ) );
        else
            sanitized.push_back( '-' );
    }

    if( sanitized.empty() )
        sanitized = "Font";

    return sanitized;
}

std::string PDF_OUTLINE_FONT_SUBSET::makeSubsetName( KIFONT::OUTLINE_FONT* aFont, unsigned aSubsetIndex )
{
    std::string prefix = generateSubsetPrefix( aSubsetIndex );
    std::string name = aFont ? sanitizeFontName( aFont->GetName() ) : std::string( "Font" );
    return fmt::format( "{}+{}", prefix, name );
}

PDF_OUTLINE_FONT_MANAGER::PDF_OUTLINE_FONT_MANAGER() :
        m_nextSubsetIndex( 0 )
{
}

void PDF_OUTLINE_FONT_MANAGER::Reset()
{
    m_subsets.clear();
    m_nextSubsetIndex = 0;
}

PDF_OUTLINE_FONT_SUBSET* PDF_OUTLINE_FONT_MANAGER::ensureSubset( KIFONT::OUTLINE_FONT* aFont, bool aItalic, bool aBold )
{
    if( !aFont )
        return nullptr;
    SUBSET_KEY key{ aFont, aItalic, aBold };
    auto it = m_subsets.find( key );
    if( it != m_subsets.end() )
        return it->second.get();
    auto subset = std::make_unique<PDF_OUTLINE_FONT_SUBSET>( aFont, m_nextSubsetIndex++ );
    PDF_OUTLINE_FONT_SUBSET* subsetPtr = subset.get();

    // Synthetic style application: if requested styles not actually present in font face flags.
    // Distinguish real face style from fake style flags so that a fake italic does not block
    // PDF shear application. We consider the face to have a real italic only if FT_STYLE_FLAG_ITALIC
    // is set. (m_fakeItal only indicates substitution missing an italic variant.)
    bool faceHasRealItalic = false;
    bool faceHasRealBold   = false;
    if( const FT_Face& face = aFont->GetFace() )
    {
        faceHasRealItalic = ( face->style_flags & FT_STYLE_FLAG_ITALIC ) != 0;
        faceHasRealBold   = ( face->style_flags & FT_STYLE_FLAG_BOLD ) != 0;
    }

    bool needSyntheticItalic = aItalic && !faceHasRealItalic; // ignore fake italic
    bool needSyntheticBold   = aBold && !faceHasRealBold;     // ignore fake bold

    if( std::getenv( "KICAD_DEBUG_SYN_STYLE" ) )
    {
        const FT_Face& face = aFont->GetFace();
        int            styleFlags = face ? face->style_flags : 0;
        const char*    fname = aFont->GetName().ToUTF8().data();
        const char*    styleName = ( face && face->style_name ) ? face->style_name : "(null)";
        bool           fakeItal = static_cast<KIFONT::OUTLINE_FONT*>( aFont )->IsFakeItalic();
        bool           fakeBold = static_cast<KIFONT::OUTLINE_FONT*>( aFont )->IsFakeBold();
        wxLogTrace( tracePdfPlotter,
                    "ensureSubset font='%s' styleName='%s' reqItalic=%d reqBold=%d FT_style_flags=%d "
                    "faceHasRealItalic=%d faceHasRealBold=%d fakeItal=%d fakeBold=%d syntheticItalic=%d "
                    "syntheticBold=%d subsetKey[i=%d b=%d] subsetIdx=%u",
                    fname ? fname : "(null)", styleName, (int) aItalic, (int) aBold, styleFlags,
                    (int) faceHasRealItalic, (int) faceHasRealBold, (int) fakeItal, (int) fakeBold,
                    (int) needSyntheticItalic, (int) needSyntheticBold, (int) aItalic, (int) aBold,
                    subsetPtr->Font()->GetFace() ? subsetPtr->Font()->GetFace()->face_index : 0 );
    }

    if( needSyntheticItalic || needSyntheticBold )
    {
        // Approx italic angle based on shear ITALIC_TILT (radians) => degrees
        double angleDeg = -std::atan( ITALIC_TILT ) * 180.0 / M_PI; // negative for conventional PDF italicAngle
        subsetPtr->ForceSyntheticStyle( needSyntheticBold, needSyntheticItalic, angleDeg );
        if( std::getenv( "KICAD_DEBUG_SYN_STYLE" ) )
        {
            wxLogTrace( tracePdfPlotter, "ForceSyntheticStyle applied: bold=%d italic=%d angle=%f", (int) needSyntheticBold, (int) needSyntheticItalic, angleDeg );
        }
    }

    m_subsets.emplace( key, std::move( subset ) );
    return subsetPtr;
}

void PDF_OUTLINE_FONT_MANAGER::EncodeString( const wxString& aText, KIFONT::OUTLINE_FONT* aFont,
                                             bool aItalicRequested, bool aBoldRequested,
                                             std::vector<PDF_OUTLINE_FONT_RUN>* aRuns )
{
    if( !aRuns || !aFont )
        return;

    auto permission = aFont->GetEmbeddingPermission();

    if( permission != KIFONT::OUTLINE_FONT::EMBEDDING_PERMISSION::INSTALLABLE
            && permission != KIFONT::OUTLINE_FONT::EMBEDDING_PERMISSION::EDITABLE )
    {
        return;
    }

    // If italic requested and font has a dedicated italic variant discoverable via style linkage,
    // the caller should already have selected that font (aFont). We still separate subsets by
    // italic flag so synthetic slant and regular do not share widths.
    PDF_OUTLINE_FONT_SUBSET* subset = ensureSubset( aFont, aItalicRequested, aBoldRequested );

    if( !subset )
        return;

    UTF8        utf8Text( aText );
    std::string textUtf8 = utf8Text.substr();

    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_set_cluster_level( buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_GRAPHEMES );
    hb_buffer_add_utf8( buffer, textUtf8.c_str(), static_cast<int>( textUtf8.size() ), 0,
                        static_cast<int>( textUtf8.size() ) );
    hb_buffer_guess_segment_properties( buffer );

    hb_font_t* hbFont = hb_ft_font_create_referenced( aFont->GetFace() );
    hb_ft_font_set_funcs( hbFont );
    hb_shape( hbFont, buffer, nullptr, 0 );

    unsigned int         glyphCount = 0;
    hb_glyph_info_t*     glyphInfo = hb_buffer_get_glyph_infos( buffer, &glyphCount );
    hb_glyph_position_t* glyphPos = hb_buffer_get_glyph_positions( buffer, &glyphCount );

    hb_font_destroy( hbFont );

    if( glyphCount == 0 )
    {
        hb_buffer_destroy( buffer );
        return;
    }

    PDF_OUTLINE_FONT_RUN run;
    run.m_subset = subset;

    bool hasVisibleGlyph = false;

    for( unsigned int ii = 0; ii < glyphCount; ++ii )
    {
        uint32_t glyphIndex = glyphInfo[ii].codepoint;

        size_t clusterStart = glyphInfo[ii].cluster;
        size_t clusterEnd = ( ii + 1 < glyphCount ) ? glyphInfo[ii + 1].cluster : textUtf8.size();

        if( clusterEnd < clusterStart )
            std::swap( clusterStart, clusterEnd );

        std::string clusterUtf8 = textUtf8.substr( clusterStart, clusterEnd - clusterStart );
        std::u32string unicode = utf8ToU32( clusterUtf8 );

        if( unicode.empty() )
            unicode.push_back( 0 );

        uint16_t cid = subset->EnsureGlyph( glyphIndex, unicode );

        if( cid != 0 )
            hasVisibleGlyph = true;

        run.m_bytes.push_back( static_cast<char>( ( cid >> 8 ) & 0xFF ) );
        run.m_bytes.push_back( static_cast<char>( cid & 0xFF ) );

        // Capture HarfBuzz positioning information and convert to PDF units
        PDF_OUTLINE_FONT_GLYPH glyph;
        glyph.cid = cid;
        // Convert from 26.6 fixed point to font units, then to PDF units
        double xAdvanceFontUnits = glyphPos[ii].x_advance / 64.0;
        double yAdvanceFontUnits = glyphPos[ii].y_advance / 64.0;
        double xOffsetFontUnits = glyphPos[ii].x_offset / 64.0;
        double yOffsetFontUnits = glyphPos[ii].y_offset / 64.0;

        glyph.xAdvance = unitsToPdf( xAdvanceFontUnits, subset->UnitsPerEm() );
        glyph.yAdvance = unitsToPdf( yAdvanceFontUnits, subset->UnitsPerEm() );
        glyph.xOffset = unitsToPdf( xOffsetFontUnits, subset->UnitsPerEm() );
        glyph.yOffset = unitsToPdf( yOffsetFontUnits, subset->UnitsPerEm() );
        run.m_glyphs.push_back( glyph );
    }

    hb_buffer_destroy( buffer );

    if( hasVisibleGlyph && !run.m_bytes.empty() )
        aRuns->push_back( std::move( run ) );
}

std::vector<PDF_OUTLINE_FONT_SUBSET*> PDF_OUTLINE_FONT_MANAGER::AllSubsets() const
{
    std::vector<PDF_OUTLINE_FONT_SUBSET*> result;
    result.reserve( m_subsets.size() );

    for( const auto& [ font, subset ] : m_subsets )
    {
        if( subset )
            result.push_back( subset.get() );
    }

    return result;
}
