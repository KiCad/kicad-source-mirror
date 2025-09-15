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

#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <wx/string.h>

#include <font/outline_font.h>

class PDF_OUTLINE_FONT_SUBSET;

struct PDF_OUTLINE_FONT_GLYPH
{
    uint16_t cid;
    double   xAdvance;
    double   yAdvance;
    double   xOffset;
    double   yOffset;
};

struct PDF_OUTLINE_FONT_RUN
{
    PDF_OUTLINE_FONT_SUBSET*             m_subset = nullptr;
    std::string                          m_bytes;
    std::vector<PDF_OUTLINE_FONT_GLYPH>  m_glyphs;
};

class PDF_OUTLINE_FONT_SUBSET
{
public:
    PDF_OUTLINE_FONT_SUBSET( KIFONT::OUTLINE_FONT* aFont, unsigned aSubsetIndex );

    uint16_t EnsureGlyph( uint32_t aGlyphIndex, const std::u32string& aUnicode );

    bool HasGlyphs() const;

    const std::string& ResourceName() const { return m_resourceName; }
    const std::string& BaseFontName() const { return m_baseFontName; }

    const std::vector<double>& Widths() const { return m_widths; }
    const std::vector<uint16_t>& CIDToGID() const { return m_cidToGid; }
    const std::vector<std::u32string>& CIDToUnicode() const { return m_cidToUnicode; }

    double UnitsPerEm() const { return m_unitsPerEm; }
    double Ascent() const { return m_ascent; }
    double Descent() const { return m_descent; }
    double CapHeight() const { return m_capHeight; }
    double ItalicAngle() const { return m_italicAngle; }
    double StemV() const { return m_stemV; }
    double BBoxMinX() const { return m_bboxMinX; }
    double BBoxMinY() const { return m_bboxMinY; }
    double BBoxMaxX() const { return m_bboxMaxX; }
    double BBoxMaxY() const { return m_bboxMaxY; }
    int Flags() const { return m_flags; }

    const std::vector<uint8_t>& FontFileData();

    std::string BuildWidthsArray() const;
    std::string BuildToUnicodeCMap() const;
    std::string BuildCIDToGIDStream() const;

    void SetFontFileHandle( int aHandle ) { m_fontFileHandle = aHandle; }
    int FontFileHandle() const { return m_fontFileHandle; }

    void SetFontDescriptorHandle( int aHandle ) { m_fontDescriptorHandle = aHandle; }
    int FontDescriptorHandle() const { return m_fontDescriptorHandle; }

    void SetCIDFontHandle( int aHandle ) { m_cidFontHandle = aHandle; }
    int CIDFontHandle() const { return m_cidFontHandle; }

    void SetCIDMapHandle( int aHandle ) { m_cidMapHandle = aHandle; }
    int CIDMapHandle() const { return m_cidMapHandle; }

    void SetToUnicodeHandle( int aHandle ) { m_toUnicodeHandle = aHandle; }
    int ToUnicodeHandle() const { return m_toUnicodeHandle; }

    void SetFontHandle( int aHandle ) { m_fontHandle = aHandle; }
    int FontHandle() const { return m_fontHandle; }

    KIFONT::OUTLINE_FONT* Font() const { return m_font; }

    void ForceSyntheticStyle( bool aBold, bool aItalic, double aItalicAngleDeg )
    {
        if( aBold )
            m_flags |= 1; // force bold flag
        if( aItalic )
            m_flags |= 64; // force italic flag
        if( aItalic )
            m_italicAngle = aItalicAngleDeg; // negative for right-leaning
        if( aBold && m_stemV < 140.0 )
            m_stemV = 140.0; // boost stem weight heuristic
    }

private:
    struct GLYPH_KEY
    {
        uint32_t        m_glyphIndex;
        std::u32string  m_unicode;

        bool operator<( const GLYPH_KEY& aOther ) const;
    };

    void ensureNotdef();

    static std::string makeResourceName( unsigned aSubsetIndex );
    static std::string makeSubsetName( KIFONT::OUTLINE_FONT* aFont, unsigned aSubsetIndex );
    static std::string sanitizeFontName( const wxString& aName );

private:
    KIFONT::OUTLINE_FONT*             m_font;
    std::string                       m_resourceName;
    std::string                       m_baseFontName;
    std::vector<double>               m_widths;
    std::vector<uint16_t>             m_cidToGid;
    std::vector<std::u32string>       m_cidToUnicode;
    std::map<GLYPH_KEY, uint16_t>     m_glyphMap;
    double                            m_unitsPerEm;
    double                            m_ascent;
    double                            m_descent;
    double                            m_capHeight;
    double                            m_italicAngle;
    double                            m_stemV;
    double                            m_bboxMinX;
    double                            m_bboxMinY;
    double                            m_bboxMaxX;
    double                            m_bboxMaxY;
    int                               m_flags;
    std::vector<uint8_t>              m_fontData;
    bool                              m_fontDataLoaded;
    uint16_t                          m_nextCID;

    int m_fontFileHandle;
    int m_fontDescriptorHandle;
    int m_cidFontHandle;
    int m_cidMapHandle;
    int m_toUnicodeHandle;
    int m_fontHandle;
};

class PDF_OUTLINE_FONT_MANAGER
{
public:
    PDF_OUTLINE_FONT_MANAGER();

    void Reset();

    void EncodeString( const wxString& aText, KIFONT::OUTLINE_FONT* aFont,
                       bool aItalicRequested, bool aBoldRequested,
                       std::vector<PDF_OUTLINE_FONT_RUN>* aRuns );

    std::vector<PDF_OUTLINE_FONT_SUBSET*> AllSubsets() const;

private:
    PDF_OUTLINE_FONT_SUBSET* ensureSubset( KIFONT::OUTLINE_FONT* aFont, bool aItalic, bool aBold );

private:
    struct SUBSET_KEY
    {
        KIFONT::OUTLINE_FONT* font;
        bool                  italic;
        bool                  bold;
        bool operator<( const SUBSET_KEY& o ) const
        {
            if( font < o.font ) return true;
            if( font > o.font ) return false;
            if( italic < o.italic ) return true;
            if( italic > o.italic ) return false;
            return bold < o.bold;
        }
    };

    std::map<SUBSET_KEY, std::unique_ptr<PDF_OUTLINE_FONT_SUBSET>> m_subsets;
    unsigned                                                                  m_nextSubsetIndex;
};
