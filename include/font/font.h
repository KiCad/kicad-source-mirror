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

#include <utf8.h>
#include <font/glyph.h>
#include <font/text_attributes.h>


namespace MARKUP
{
struct NODE;
}

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
    OVERBAR = 1 << 4
};


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

    virtual bool IsStroke() const { return false; }
    virtual bool IsOutline() const { return false; }
    virtual bool IsBold() const { return false; }
    virtual bool IsItalic() const { return false; }

    static FONT* GetFont( const wxString& aFontName = "", bool aBold = false,
                          bool aItalic = false );
    static bool  IsStroke( const wxString& aFontName );

    const wxString&    Name() const;
    inline const char* NameAsToken() const { return Name().utf8_str().data(); }

    /**
     * Draw a string.
     *
     * @param aGal is the graphics context.
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aRotationAngle is the text rotation angle
     * @return bounding box
     */
    VECTOR2D Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                   const VECTOR2D& aOrigin, const TEXT_ATTRIBUTES& aAttrs ) const;

    VECTOR2D Draw( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                   const TEXT_ATTRIBUTES& aAttributes ) const
    {
        return Draw( aGal, aText, aPosition, VECTOR2D( 0, 0 ), aAttributes );
    }

    virtual void KiDrawText( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                             const TEXT_ATTRIBUTES& aAttributes ) const;

    /**
     * Compute the boundary limits of aText (the bounding box of all shapes).
     *
     * @return a VECTOR2D giving the width and height of text.
     */
    virtual VECTOR2D StringBoundaryLimits( const KIGFX::GAL* aGal, const UTF8& aText,
                                           const VECTOR2D& aGlyphSize,
                                           double aGlyphThickness ) const = 0;

    VECTOR2D StringBoundaryLimits( const UTF8& aText, const VECTOR2D& aGlyphSize,
                                   double aGlyphThickness ) const
    {
        return StringBoundaryLimits( nullptr, aText, aGlyphSize, aGlyphThickness );
    }

    /**
     * Compute the vertical position of an overbar.  This is the distance between the text
     * baseline and the overbar.
     */
    virtual double ComputeOverbarVerticalPosition( double aGlyphHeight ) const = 0;

    /**
     * Compute the distance (interline) between 2 lines of text (for multiline texts).  This is
     * the distance between baselines, not the space between line bounding boxes.
     */
    virtual double GetInterline( double aGlyphHeight, double aLineSpacing = 1.0 ) const = 0;

    /**
     * Compute the X and Y size of a given text. The text is expected to be a single line.
     */
    virtual VECTOR2D ComputeTextLineSize( const KIGFX::GAL* aGal, const UTF8& aText ) const = 0;

    /**
     * Convert text string to an array of GLYPHs.
     *
     * @param aBoundingBox pointer to a BOX2I that will set to the bounding box, or nullptr
     * @param aGlyphs storage for the returned GLYPHs
     * @param aText text to convert to polygon/polyline
     * @param aGlyphSize glyph size
     * @param aPosition position of text (cursor position before this text)
     * @param aAngle text angle
     * @param aTextStyle text style flags
     * @return text cursor position after this text
     */
    virtual VECTOR2I GetTextAsGlyphs( BOX2I* aBoundingBox,
                                      std::vector<std::unique_ptr<GLYPH>>& aGlyphs,
                                      const UTF8& aText, const VECTOR2D& aGlyphSize,
                                      const wxPoint& aPosition, const EDA_ANGLE& aAngle,
                                      TEXT_STYLE_FLAGS aTextStyle ) const = 0;

protected:
    /**
     * Returns number of lines for a given text.
     *
     * @param aText is the text to be checked.
     * @return unsigned - The number of lines in aText.
     */
    inline unsigned linesCount( const UTF8& aText ) const
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
     * @param aBoundingBox is a pointer to a BOX2I variable which will be set to the bounding box,
     *                     or nullptr
     * @param aText is the text to be drawn.
     * @param aPosition is text position.
     * @param aAngle is text angle.
     * @param aIsMirrored is true if text should be drawn mirrored, false otherwise.
     * @return new cursor position
     */
    VECTOR2D drawSingleLineText( KIGFX::GAL* aGal, BOX2I* aBoundingBox, const UTF8& aText,
                                 const VECTOR2D& aPosition, const VECTOR2D& aGlyphSize,
                                 const EDA_ANGLE& aAngle, bool aIsItalic, bool aIsMirrored ) const;

    /**
     * Computes the bounding box for a single line of text.
     * Multiline texts should be split before using the function.
     *
     * @param aBoundingBox is a pointer to a BOX2I variable which will be set to the bounding box,
     *                     or nullptr
     * @param aText is the text to be drawn.
     * @param aPosition is text position.
     * @param aGlyphSize is glyph size.
     * @param aAngle is text angle.
     * @return new cursor position
     */
    VECTOR2D boundingBoxSingleLine( BOX2I* aBoundingBox, const UTF8& aText,
                                    const VECTOR2D& aPosition, const VECTOR2D& aGlyphSize,
                                    const EDA_ANGLE& aAngle, bool aIsItalic ) const;

    void getLinePositions( const UTF8& aText, const VECTOR2D& aPosition, wxArrayString& aStringList,
                           std::vector<wxPoint>& aPositions, int& aLineCount,
                           std::vector<VECTOR2D>& aBoundingBoxes,
                           const TEXT_ATTRIBUTES& aAttributes ) const;

    virtual VECTOR2D getBoundingBox( const UTF8& aString, const VECTOR2D& aGlyphSize,
                                     TEXT_STYLE_FLAGS aTextStyle = 0 ) const = 0;

    VECTOR2D drawMarkup( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>& aGlyphs,
                         const std::unique_ptr<MARKUP::NODE>& aNode, const VECTOR2D& aPosition,
                         const VECTOR2D& aGlyphSize, const EDA_ANGLE& aAngle,
                         TEXT_STYLE_FLAGS aTextStyle, int aLevel = 0 ) const;

    ///< Factor that determines the pitch between 2 lines.
    static constexpr double INTERLINE_PITCH_RATIO = 1.62;   // The golden mean

private:
    static FONT* getDefaultFont();

    VECTOR2D doDrawString( KIGFX::GAL* aGal, const UTF8& aText, const VECTOR2D& aPosition,
                           bool aParse, const TEXT_ATTRIBUTES& aAttrs ) const;
    VECTOR2D getBoundingBox( const UTF8& aText, TEXT_STYLE_FLAGS aTextStyle,
                             const TEXT_ATTRIBUTES& aAttrs ) const;

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
    os << "[Font \"" << aFont.Name() << "\"" << ( aFont.IsStroke() ? " stroke" : "" )
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
