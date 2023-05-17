/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021-2022 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Font abstract base class
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

#ifndef FONT_H_
#define FONT_H_

#include <iostream>
#include <map>
#include <algorithm>
#include <wx/string.h>
#include <font/glyph.h>
#include <font/text_attributes.h>

namespace KIGFX
{
class GAL;
}


enum TEXT_STYLE
{
    BOLD = 1,
    ITALIC = 1 << 1,
    SUBSCRIPT = 1 << 2,
    SUPERSCRIPT = 1 << 3,
    OVERBAR = 1 << 4,
    UNDERLINE = 1 << 5
};


/**
 * Tilt factor for italic style (this is the scaling factor on dY relative coordinates to give
 * a tilted shape).
 * This is applied directly to stroke fonts, and is used as an estimate for outline fonts (which
 * have the actual tilt built in to their polygonal glyph outlines).
 */
static constexpr double ITALIC_TILT = 1.0 / 8;


using TEXT_STYLE_FLAGS = unsigned int;


inline bool IsBold( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::BOLD;
}


inline bool IsItalic( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::ITALIC;
}


inline bool IsSuperscript( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::SUPERSCRIPT;
}


inline bool IsSubscript( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::SUBSCRIPT;
}


inline bool IsOverbar( TEXT_STYLE_FLAGS aFlags )
{
    return aFlags & TEXT_STYLE::OVERBAR;
}


std::string TextStyleAsString( TEXT_STYLE_FLAGS aFlags );


namespace KIFONT
{
/**
 * FONT is an abstract base class for both outline and stroke fonts
 */
class FONT
{
public:
    explicit FONT();

    virtual ~FONT()
    { }

    virtual bool IsStroke() const  { return false; }
    virtual bool IsOutline() const { return false; }
    virtual bool IsBold() const    { return false; }
    virtual bool IsItalic() const  { return false; }

    static FONT* GetFont( const wxString& aFontName = wxEmptyString, bool aBold = false,
                          bool aItalic = false );
    static bool IsStroke( const wxString& aFontName );

    const wxString& GetName() const { return m_fontName; };
    inline const char* NameAsToken() const { return GetName().utf8_str().data(); }

    /**
     * Draw a string.
     *
     * @param aGal is the graphics context.
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aCursor is the current text position (for multiple text blocks within a single text
     *                object, such as a run of superscript characters)
     * @param aAttrs are the styling attributes of the text, including its rotation
     */
    void Draw( KIGFX::GAL* aGal, const wxString& aText, const VECTOR2I& aPosition,
               const VECTOR2I& aCursor, const TEXT_ATTRIBUTES& aAttrs ) const;

    void Draw( KIGFX::GAL* aGal, const wxString& aText, const VECTOR2I& aPosition,
               const TEXT_ATTRIBUTES& aAttributes ) const
    {
        Draw( aGal, aText, aPosition, VECTOR2I( 0, 0 ), aAttributes );
    }

    /**
     * Compute the boundary limits of aText (the bounding box of all shapes).
     *
     * @return a VECTOR2I giving the width and height of text.
     */
    VECTOR2I StringBoundaryLimits( const wxString& aText, const VECTOR2I& aSize, int aThickness,
                                   bool aBold, bool aItalic ) const;

    /**
     * Insert \n characters into text to ensure that no lines are wider than \a aColumnWidth.
     */
    void LinebreakText( wxString& aText, int aColumnWidth, const VECTOR2I& aGlyphSize,
                        int aThickness, bool aBold, bool aItalic ) const;

    /**
     * Compute the vertical position of an overbar.  This is the distance between the text
     * baseline and the overbar.
     */
    virtual double ComputeOverbarVerticalPosition( double aGlyphHeight ) const = 0;

    /**
     * Compute the vertical position of an underline.  This is the distance between the text
     * baseline and the underline.
     */
    virtual double ComputeUnderlineVerticalPosition( double aGlyphHeight ) const = 0;

    /**
     * Compute the distance (interline) between 2 lines of text (for multiline texts).  This is
     * the distance between baselines, not the space between line bounding boxes.
     */
    virtual double GetInterline( double aGlyphHeight, double aLineSpacing = 1.0 ) const = 0;

    /**
     * Convert text string to an array of GLYPHs.
     *
     * @param aBBox pointer to a BOX2I that will set to the bounding box, or nullptr
     * @param aGlyphs storage for the returned GLYPHs
     * @param aText text to convert to polygon/polyline
     * @param aSize is the cap-height and em-width of the text
     * @param aPosition position of text (cursor position before this text)
     * @param aAngle text angle
     * @param aMirror is true if text should be drawn mirrored, false otherwise.
     * @param aOrigin is the point around which the text should be rotated, mirrored, etc.
     * @param aTextStyle text style flags
     * @return text cursor position after this text
     */
    virtual VECTOR2I GetTextAsGlyphs( BOX2I* aBBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                      const wxString& aText, const VECTOR2I& aSize,
                                      const VECTOR2I& aPosition, const EDA_ANGLE& aAngle,
                                      bool aMirror, const VECTOR2I& aOrigin,
                                      TEXT_STYLE_FLAGS aTextStyle ) const = 0;

protected:
    /**
     * Returns number of lines for a given text.
     *
     * @param aText is the text to be checked.
     * @return unsigned - The number of lines in aText.
     */
    inline unsigned linesCount( const wxString& aText ) const
    {
        if( aText.empty() )
            return 0; // std::count does not work well with empty strings
        else
            // aText.end() - 1 is to skip a newline character that is potentially at the end
            return std::count( aText.begin(), aText.end() - 1, '\n' ) + 1;
    }

    /**
     * Draws a single line of text. Multiline texts should be split before using the
     * function.
     *
     * @param aGal is a pointer to the graphics abstraction layer, or nullptr (nothing is drawn)
     * @param aBBox is an optional pointer to be filled with the bounding box.
     * @param aText is the text to be drawn.
     * @param aPosition is text position.
     * @param aSize is the cap-height and em-width of the text
     * @param aAngle is text angle.
     * @param aMirror is true if text should be drawn mirrored, false otherwise.
     * @param aOrigin is the point around which the text should be rotated, mirrored, etc.
     * @return new cursor position in non-rotated, non-mirrored coordinates
     */
    void drawSingleLineText( KIGFX::GAL* aGal, BOX2I* aBoundingBox, const wxString& aText,
                             const VECTOR2I& aPosition, const VECTOR2I& aSize,
                             const EDA_ANGLE& aAngle, bool aMirror, const VECTOR2I& aOrigin,
                             bool aItalic, bool aUnderline ) const;

    /**
     * Computes the bounding box for a single line of text.
     * Multiline texts should be split before using the function.
     *
     * @param aBBox is an optional pointer to be filled with the bounding box.
     * @param aText is the text to be drawn.
     * @param aPosition is text position.
     * @param aSize is the cap-height and em-width of the text.
     * @return new cursor position
     */
    VECTOR2I boundingBoxSingleLine( BOX2I* aBBox, const wxString& aText, const VECTOR2I& aPosition,
                                    const VECTOR2I& aSize, bool aItalic ) const;

    void getLinePositions( const wxString& aText, const VECTOR2I& aPosition,
                           wxArrayString& aTextLines, std::vector<VECTOR2I>& aPositions,
                           std::vector<VECTOR2I>& aExtents, const TEXT_ATTRIBUTES& aAttrs ) const;

    VECTOR2I drawMarkup( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                         const wxString& aText, const VECTOR2I& aPosition,
                         const VECTOR2I& aSize, const EDA_ANGLE& aAngle, bool aMirror,
                         const VECTOR2I& aOrigin, TEXT_STYLE_FLAGS aTextStyle ) const;

    void wordbreakMarkup( std::vector<std::pair<wxString, int>>* aWords, const wxString& aText,
                          const VECTOR2I& aSize, TEXT_STYLE_FLAGS aTextStyle ) const;

    ///< Factor that determines the pitch between 2 lines.
    static constexpr double INTERLINE_PITCH_RATIO = 1.61;   // The golden mean

private:
    static FONT* getDefaultFont();

protected:
    wxString     m_fontName;         ///< Font name
    wxString     m_fontFileName;     ///< Font file name

private:
    static FONT* s_defaultFont;

    static std::map< std::tuple<wxString, bool, bool>, FONT* > s_fontMap;
};

} //namespace KIFONT


inline std::ostream& operator<<(std::ostream& os, const KIFONT::FONT& aFont)
{
    os << "[Font \"" << aFont.GetName() << "\"" << ( aFont.IsStroke() ? " stroke" : "" )
       << ( aFont.IsOutline() ? " outline" : "" ) << ( aFont.IsBold() ? " bold" : "" )
       << ( aFont.IsItalic() ? " italic" : "" ) << "]";
    return os;
}


inline std::ostream& operator<<(std::ostream& os, const KIFONT::FONT* aFont)
{
    os << *aFont;
    return os;
}

#endif // FONT_H_
