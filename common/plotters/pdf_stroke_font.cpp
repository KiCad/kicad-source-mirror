/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <plotters/pdf_stroke_font.h>

#include <algorithm>
#include <cstdint>
#include <limits>

#include <fmt/format.h>

#include <font/glyph.h>
#include <advanced_config.h>

namespace
{
static constexpr int MAX_SIMPLE_FONT_CODES = 256;

// Build the stroked path for a glyph.
// KiCad's internal stroke font glyph coordinates use an inverted Y axis relative to the
// PDF coordinate system we are targeting here.  We therefore optionally flip Y so text
// renders upright.  A slightly thicker default stroke width (4% of EM) is used to improve
// legibility at typical plot zoom levels.
static std::string buildGlyphStream( const KIFONT::STROKE_GLYPH* aGlyph, double aUnitsPerEm,
                                     bool aInvertY, bool aBold )
{
    if( !aGlyph )
        return std::string();

    fmt::memory_buffer buffer;
    double factor = ADVANCED_CFG::GetCfg().m_PDFStrokeFontWidthFactor;

    if( aBold )
    {
        double boldMul = ADVANCED_CFG::GetCfg().m_PDFStrokeFontBoldMultiplier;
        if( boldMul < 1.0 ) boldMul = 1.0;
        factor *= boldMul;
    }

    if( factor <= 0.0 )
        factor = 0.04; // fallback safety

    double lw = aUnitsPerEm * factor;
    fmt::format_to( std::back_inserter( buffer ), "{:.3f} w 1 J 1 j ", lw );
    auto& cfg = ADVANCED_CFG::GetCfg();

    for( const std::vector<VECTOR2D>& stroke : *aGlyph )
    {
        bool firstPoint = true;

        for( const VECTOR2D& point : stroke )
        {
            double x = ( point.x + cfg.m_PDFStrokeFontXOffset ) * aUnitsPerEm;
            double y = point.y * aUnitsPerEm;

            if( aInvertY )
            {
                y = -y; // Mirror vertically about baseline (y=0)
                y += cfg.m_PDFStrokeFontYOffset * aUnitsPerEm;
            }

            if( firstPoint )
            {
                fmt::format_to( std::back_inserter( buffer ), "{:.3f} {:.3f} m ", x, y );
                firstPoint = false;
            }
            else
            {
                fmt::format_to( std::back_inserter( buffer ), "{:.3f} {:.3f} l ", x, y );
            }
        }

        if( !stroke.empty() )
            fmt::format_to( std::back_inserter( buffer ), "S " );
    }

    return std::string( buffer.data(), buffer.size() );
}

static std::string formatUnicodeHex( uint32_t aCodepoint )
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
} // anonymous namespace


PDF_STROKE_FONT_SUBSET::PDF_STROKE_FONT_SUBSET( const KIFONT::STROKE_FONT* aFont, double aUnitsPerEm,
                                                unsigned aSubsetIndex, bool aBold, bool aItalic ) :
        m_font( aFont ),
        m_unitsPerEm( aUnitsPerEm ),
        m_resourceName( fmt::format( "/KiCadStroke{}", aSubsetIndex ) ),
        m_cmapName( fmt::format( "KiCadStrokeCMap{}", aSubsetIndex ) ),
        m_widths( MAX_SIMPLE_FONT_CODES, 0.0 ),
        m_nextCode( 1 ),
        m_lastCode( 0 ),
        m_bboxMinX( std::numeric_limits<double>::max() ),
        m_bboxMinY( std::numeric_limits<double>::max() ),
        m_bboxMaxX( std::numeric_limits<double>::lowest() ),
        m_bboxMaxY( std::numeric_limits<double>::lowest() ),
        m_charProcsHandle( -1 ),
        m_fontHandle( -1 ),
        m_toUnicodeHandle( -1 ),
        m_isBold( aBold ),
        m_isItalic( aItalic )
{
    GLYPH notdef;
    notdef.m_unicode = 0;
    notdef.m_code = 0;
    notdef.m_glyphIndex = -1;
    notdef.m_name = ".notdef";
    notdef.m_stream.clear();
    notdef.m_width = 0.0;
    notdef.m_minX = 0.0;
    notdef.m_minY = 0.0;
    notdef.m_maxX = 0.0;
    notdef.m_maxY = 0.0;
    notdef.m_charProcHandle = -1;

    m_glyphs.push_back( notdef );
    m_bboxMinX = 0.0;
    m_bboxMinY = 0.0;
    m_bboxMaxX = 0.0;
    m_bboxMaxY = 0.0;
}


bool PDF_STROKE_FONT_SUBSET::Contains( wxUniChar aCode ) const
{
    return m_unicodeToCode.find( aCode ) != m_unicodeToCode.end();
}


int PDF_STROKE_FONT_SUBSET::EnsureGlyph( wxUniChar aCode )
{
    auto it = m_unicodeToCode.find( aCode );

    if( it != m_unicodeToCode.end() )
        return it->second;

    if( IsFull() )
        return -1;

    int glyphIndex = glyphIndexForUnicode( aCode );

    int code = m_nextCode++;
    m_lastCode = std::max( m_lastCode, code );

    GLYPH data;
    data.m_unicode = aCode;
    data.m_code = code;
    data.m_glyphIndex = glyphIndex;
    data.m_name = makeGlyphName( code );
    data.m_charProcHandle = -1;

    const KIFONT::STROKE_GLYPH* glyph = nullptr;
    BOX2D bbox;

    if( m_font )
    {
        glyph = m_font->GetGlyph( glyphIndex );
        bbox = m_font->GetGlyphBoundingBox( glyphIndex );
    }

    VECTOR2D origin = bbox.GetOrigin();
    VECTOR2D size = bbox.GetSize();

    data.m_width = size.x * m_unitsPerEm;
    data.m_minX = origin.x * m_unitsPerEm;
    data.m_minY = origin.y * m_unitsPerEm;
    data.m_maxX = ( origin.x + size.x ) * m_unitsPerEm;
    data.m_maxY = ( origin.y + size.y ) * m_unitsPerEm;

    // Invert Y so glyphs render upright in PDF coordinate space.
    bool invertY = true;

    if( invertY )
    {
        // Mirror bounding box vertically about baseline.
        double newMinY = -data.m_maxY;
        double newMaxY = -data.m_minY;
        data.m_minY = newMinY;
        data.m_maxY = newMaxY;

        // Apply Y offset to bounding box to match the offset applied to stroke coordinates
        double yOffset = ADVANCED_CFG::GetCfg().m_PDFStrokeFontYOffset * m_unitsPerEm;
        data.m_minY += yOffset;
        data.m_maxY += yOffset;
    }

    // Build charproc stream: first specify width and bbox (d1 operator) then stroke path.
    double kerningFactor = ADVANCED_CFG::GetCfg().m_PDFStrokeFontKerningFactor;

    if( kerningFactor <= 0.0 )
        kerningFactor = 1.0;

    std::string strokes = buildGlyphStream( glyph, m_unitsPerEm, invertY, m_isBold );
    data.m_width = size.x * m_unitsPerEm * kerningFactor;
    data.m_stream = fmt::format( "{:.3f} 0 {:.3f} {:.3f} {:.3f} {:.3f} d1 {}",
                                 data.m_width,
                                 data.m_minX, data.m_minY, data.m_maxX, data.m_maxY,
                                 strokes );

    m_widths[code] = data.m_width;

    m_bboxMinX = std::min( m_bboxMinX, data.m_minX );
    m_bboxMinY = std::min( m_bboxMinY, data.m_minY );
    m_bboxMaxX = std::max( m_bboxMaxX, data.m_maxX );
    m_bboxMaxY = std::max( m_bboxMaxY, data.m_maxY );

    m_glyphs.push_back( std::move( data ) );
    m_unicodeToCode.emplace( aCode, code );

    return code;
}


int PDF_STROKE_FONT_SUBSET::CodeForGlyph( wxUniChar aCode ) const
{
    auto it = m_unicodeToCode.find( aCode );

    if( it != m_unicodeToCode.end() )
        return it->second;

    return -1;
}


bool PDF_STROKE_FONT_SUBSET::IsFull() const
{
    return m_nextCode >= MAX_SIMPLE_FONT_CODES;
}


int PDF_STROKE_FONT_SUBSET::GlyphCount() const
{
    return static_cast<int>( m_glyphs.size() );
}


int PDF_STROKE_FONT_SUBSET::FirstChar() const
{
    return 0;
}


int PDF_STROKE_FONT_SUBSET::LastChar() const
{
    return std::max( 0, m_lastCode );
}


std::string PDF_STROKE_FONT_SUBSET::BuildDifferencesArray() const
{
    int first = FirstChar();
    int last = LastChar();

    fmt::memory_buffer buffer;
    fmt::format_to( std::back_inserter( buffer ), "[ {} ", first );

    for( int code = first; code <= last; ++code )
    {
        const GLYPH* glyph = glyphForCode( code );

        if( glyph )
        fmt::format_to( std::back_inserter( buffer ), "/{} ", glyph->m_name );
        else
        fmt::format_to( std::back_inserter( buffer ), "/.notdef " );
    }

    fmt::format_to( std::back_inserter( buffer ), "]" );
    return std::string( buffer.data(), buffer.size() );
}


std::string PDF_STROKE_FONT_SUBSET::BuildWidthsArray() const
{
    int first = FirstChar();
    int last = LastChar();

    fmt::memory_buffer buffer;
    fmt::format_to( std::back_inserter( buffer ), "[" );

    for( int code = first; code <= last; ++code )
        fmt::format_to( std::back_inserter( buffer ), " {:g}", m_widths[code] );

    fmt::format_to( std::back_inserter( buffer ), " ]" );
    return std::string( buffer.data(), buffer.size() );
}


std::string PDF_STROKE_FONT_SUBSET::BuildToUnicodeCMap() const
{
    size_t mappingCount = 0;

    for( const GLYPH& glyph : m_glyphs )
    {
        if( glyph.m_code == 0 )
            continue;

        ++mappingCount;
    }

    fmt::memory_buffer buffer;

    fmt::format_to( std::back_inserter( buffer ), "/CIDInit /ProcSet findresource begin\n" );
    fmt::format_to( std::back_inserter( buffer ), "12 dict begin\n" );
    fmt::format_to( std::back_inserter( buffer ), "begincmap\n" );
    fmt::format_to( std::back_inserter( buffer ), "/CIDSystemInfo << /Registry (KiCad) /Ordering (StrokeFont) /Supplement 0 >> def\n" );
    fmt::format_to( std::back_inserter( buffer ), "/CMapName /{} def\n", m_cmapName );
    fmt::format_to( std::back_inserter( buffer ), "/CMapType 2 def\n" );
    fmt::format_to( std::back_inserter( buffer ), "1 begincodespacerange\n" );
    fmt::format_to( std::back_inserter( buffer ), "<00> <FF>\n" );
    fmt::format_to( std::back_inserter( buffer ), "endcodespacerange\n" );

    fmt::format_to( std::back_inserter( buffer ), "{} beginbfchar\n", mappingCount );

    for( const GLYPH& glyph : m_glyphs )
    {
        if( glyph.m_code == 0 )
            continue;

        fmt::format_to( std::back_inserter( buffer ), "<{:02X}> <{}>\n", glyph.m_code,
                        formatUnicodeHex( static_cast<uint32_t>( glyph.m_unicode ) ) );
    }

    fmt::format_to( std::back_inserter( buffer ), "endbfchar\n" );
    fmt::format_to( std::back_inserter( buffer ), "endcmap\n" );
    fmt::format_to( std::back_inserter( buffer ), "CMapName currentdict /CMap defineresource pop\n" );
    fmt::format_to( std::back_inserter( buffer ), "end\n" );
    fmt::format_to( std::back_inserter( buffer ), "end\n" );

    return std::string( buffer.data(), buffer.size() );
}


int PDF_STROKE_FONT_SUBSET::glyphIndexForUnicode( wxUniChar aCode ) const
{
    int value = static_cast<int>( aCode );

    if( value < ' ' )
        return static_cast<int>( '?' ) - ' ';

    int index = value - ' ';
    int count = static_cast<int>( m_font ? m_font->GetGlyphCount() : 0 );

    if( index < 0 || index >= count )
        return static_cast<int>( '?' ) - ' ';

    return index;
}


std::string PDF_STROKE_FONT_SUBSET::makeGlyphName( int aCode ) const
{
    return fmt::format( "g{:02X}", aCode );
}


const PDF_STROKE_FONT_SUBSET::GLYPH* PDF_STROKE_FONT_SUBSET::glyphForCode( int aCode ) const
{
    for( const GLYPH& glyph : m_glyphs )
    {
        if( glyph.m_code == aCode )
            return &glyph;
    }

    return nullptr;
}


PDF_STROKE_FONT_MANAGER::PDF_STROKE_FONT_MANAGER() :
        m_font( KIFONT::STROKE_FONT::LoadFont( wxEmptyString ) ),
        m_unitsPerEm( 1000.0 ),
        m_nextSubsetIndex( 0 )
{
    Reset();
}


void PDF_STROKE_FONT_MANAGER::Reset()
{
    m_styleGroups.clear();
    m_nextSubsetIndex = 0;

    if( !m_font )
        m_font.reset( KIFONT::STROKE_FONT::LoadFont( wxEmptyString ) );
}


PDF_STROKE_FONT_MANAGER::STYLE_GROUP& PDF_STROKE_FONT_MANAGER::groupFor( bool aBold, bool aItalic )
{
    unsigned key = styleKey( aBold, aItalic );
    return m_styleGroups[key];
}

void PDF_STROKE_FONT_MANAGER::EncodeString( const wxString& aText,
                                            std::vector<PDF_STROKE_FONT_RUN>* aRuns,
                                            bool aBold, bool aItalic )
{
    if( !aRuns )
        return;

    aRuns->clear();

    if( aText.empty() )
        return;

    PDF_STROKE_FONT_SUBSET* currentSubset = nullptr;
    std::string currentBytes;

    for( wxUniChar ch : aText )
    {
        PDF_STROKE_FONT_SUBSET* subset = ensureSubsetForGlyph( ch, aBold, aItalic );

        if( !subset )
            continue;

        int code = subset->EnsureGlyph( ch );

        if( code < 0 )
            continue;

        if( subset != currentSubset )
        {
            if( !currentBytes.empty() && currentSubset )
                aRuns->push_back( { currentSubset, currentBytes, aBold, aItalic } );

            currentSubset = subset;
            currentBytes.clear();
        }

        currentBytes.push_back( static_cast<char>( code ) );
    }

    if( !currentBytes.empty() && currentSubset )
        aRuns->push_back( { currentSubset, std::move( currentBytes ), aBold, aItalic } );
}

PDF_STROKE_FONT_SUBSET* PDF_STROKE_FONT_MANAGER::ensureSubsetForGlyph( wxUniChar aCode, bool aBold, bool aItalic )
{
    STYLE_GROUP& group = groupFor( aBold, aItalic );

    for( const std::unique_ptr<PDF_STROKE_FONT_SUBSET>& subset : group.subsets )
    {
        if( subset->Contains( aCode ) )
            return subset.get();
    }

    for( const std::unique_ptr<PDF_STROKE_FONT_SUBSET>& subset : group.subsets )
    {
        if( subset->IsFull() )
            continue;

        if( subset->EnsureGlyph( aCode ) >= 0 )
            return subset.get();
    }

    unsigned subsetIndex = m_nextSubsetIndex++;
    auto newSubset = std::make_unique<PDF_STROKE_FONT_SUBSET>( m_font.get(), m_unitsPerEm, subsetIndex, aBold, aItalic );
    PDF_STROKE_FONT_SUBSET* subsetPtr = newSubset.get();
    subsetPtr->EnsureGlyph( aCode );
    group.subsets.emplace_back( std::move( newSubset ) );
    return subsetPtr;
}

std::vector<PDF_STROKE_FONT_SUBSET*> PDF_STROKE_FONT_MANAGER::AllSubsets() const
{
    std::vector<PDF_STROKE_FONT_SUBSET*> out;

    for( const auto& [key, group] : m_styleGroups )
    {
        (void) key;
        for( const auto& up : group.subsets )
            out.push_back( up.get() );
    }
    return out;
}

