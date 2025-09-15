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

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <wx/string.h>

#include <font/stroke_font.h>

class PDF_STROKE_FONT_SUBSET;

struct PDF_STROKE_FONT_RUN
{
    PDF_STROKE_FONT_SUBSET* m_subset;
    std::string             m_bytes;
    bool                    m_bold = false;
    bool                    m_italic = false;
};

class PDF_STROKE_FONT_SUBSET
{
public:
    struct GLYPH
    {
        wxUniChar   m_unicode;
        int         m_code;
        int         m_glyphIndex;
        std::string m_name;
        std::string m_stream;
        double      m_width;
        double      m_minX;
        double      m_minY;
        double      m_maxX;
        double      m_maxY;
        int         m_charProcHandle;
    };

public:
    PDF_STROKE_FONT_SUBSET( const KIFONT::STROKE_FONT* aFont, double aUnitsPerEm,
                            unsigned aSubsetIndex, bool aBold, bool aItalic );

    bool Contains( wxUniChar aCode ) const;

    int EnsureGlyph( wxUniChar aCode );

    int CodeForGlyph( wxUniChar aCode ) const;

    bool IsFull() const;

    int GlyphCount() const;

    int FirstChar() const;

    int LastChar() const;

    double UnitsPerEm() const { return m_unitsPerEm; }

    double FontBBoxMinX() const { return m_bboxMinX; }
    double FontBBoxMinY() const { return m_bboxMinY; }
    double FontBBoxMaxX() const { return m_bboxMaxX; }
    double FontBBoxMaxY() const { return m_bboxMaxY; }

    const std::string& ResourceName() const { return m_resourceName; }
    const std::string& CMapName() const { return m_cmapName; }
    bool IsBold() const { return m_isBold; }
    bool IsItalic() const { return m_isItalic; }

    std::vector<GLYPH>& Glyphs() { return m_glyphs; }
    const std::vector<GLYPH>& Glyphs() const { return m_glyphs; }

    const std::vector<double>& Widths() const { return m_widths; }

    std::string BuildDifferencesArray() const;
    std::string BuildWidthsArray() const;
    std::string BuildToUnicodeCMap() const;

    void SetCharProcsHandle( int aHandle ) { m_charProcsHandle = aHandle; }
    int CharProcsHandle() const { return m_charProcsHandle; }

    void SetFontHandle( int aHandle ) { m_fontHandle = aHandle; }
    int FontHandle() const { return m_fontHandle; }

    void SetToUnicodeHandle( int aHandle ) { m_toUnicodeHandle = aHandle; }
    int ToUnicodeHandle() const { return m_toUnicodeHandle; }

private:
    int glyphIndexForUnicode( wxUniChar aCode ) const;

    std::string makeGlyphName( int aCode ) const;

    const GLYPH* glyphForCode( int aCode ) const;

private:
    const KIFONT::STROKE_FONT*           m_font;
    double                               m_unitsPerEm;
    std::string                          m_resourceName;
    std::string                          m_cmapName;
    std::map<wxUniChar, int>             m_unicodeToCode;
    std::vector<GLYPH>                   m_glyphs;
    std::vector<double>                  m_widths;
    int                                  m_nextCode;
    int                                  m_lastCode;
    double                               m_bboxMinX;
    double                               m_bboxMinY;
    double                               m_bboxMaxX;
    double                               m_bboxMaxY;
    int                                  m_charProcsHandle;
    int                                  m_fontHandle;
    int                                  m_toUnicodeHandle;
    bool                                 m_isBold;
    bool                                 m_isItalic;
};

class PDF_STROKE_FONT_MANAGER
{
public:
    PDF_STROKE_FONT_MANAGER();

    void Reset();

    void EncodeString( const wxString& aText, std::vector<PDF_STROKE_FONT_RUN>* aRuns,
                       bool aBold = false, bool aItalic = false );

    // Collect all subsets including style-group (bold/italic) subsets. Returned pointers are
    // owned by the manager; vector is a temporary snapshot.
    std::vector<PDF_STROKE_FONT_SUBSET*> AllSubsets() const;

private:
    PDF_STROKE_FONT_SUBSET* ensureSubsetForGlyph( wxUniChar aCode, bool aBold, bool aItalic );

    // style key packing: bit0 = bold, bit1 = italic
    static unsigned styleKey( bool aBold, bool aItalic ) { return ( aBold ? 1u : 0u ) | ( aItalic ? 2u : 0u ); }

    struct STYLE_GROUP
    {
        std::vector<std::unique_ptr<PDF_STROKE_FONT_SUBSET>> subsets; // may overflow 256 glyph limit
    };

    STYLE_GROUP& groupFor( bool aBold, bool aItalic );

private:
    std::unique_ptr<KIFONT::STROKE_FONT>               m_font;
    double                                             m_unitsPerEm;
    unsigned                                           m_nextSubsetIndex; // global counter for unique resource names
    std::map<unsigned, STYLE_GROUP>                      m_styleGroups; // all style groups including default
};

