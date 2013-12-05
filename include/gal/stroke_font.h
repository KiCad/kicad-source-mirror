/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 *
 * Stroke font class
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

#ifndef STROKE_FONT_H_
#define STROKE_FONT_H_

#include <string>
#include <deque>

#include <eda_text.h>

#include <math/box2.h>

namespace KIGFX
{
class GAL;

typedef std::deque< std::deque<VECTOR2D> > GLYPH;
typedef std::deque<GLYPH>                  GLYPH_LIST;

/**
 * @brief Class STROKE_FONT implements stroke font drawing.
 *
 * A stroke font is composed of lines.
 */
class STROKE_FONT
{
public:
    /// Constructor
    STROKE_FONT( GAL* aGal );

    /// Destructor
    ~STROKE_FONT();

    // TODO Load font from a text file

    /**
     * @brief Load the new stroke font.
     *
     * @param aNewStrokeFont is the pointer to the font data.
     * @param aNewStrokeFontSize is the size of the font data.
     * @return True, if the font was successfully loaded, else false.
     */
    bool LoadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize );

    /**
     * @brief Draw a string.
     *
     * @param aText is the text to be drawn.
     * @param aPosition is the text position in world coordinates.
     * @param aRotationAngle is the text rotation angle.
     */
    void Draw( wxString aText, const VECTOR2D& aPosition, double aRotationAngle );

    /**
     * @brief Set the scale factor of the font for the glyph size.
     *
     * @param aScaleFactor is the scale factor of the font.
     */
    inline void SetScaleFactor( const double aScaleFactor )
    {
        m_scaleFactor = aScaleFactor;
    }

    /**
     * @brief Set the glyph size.
     *
     * @param aGlyphSize is the glyph size.
     */
    inline void SetGlyphSize( const VECTOR2D aGlyphSize )
    {
        m_glyphSize = aGlyphSize;
    }

    /**
     * @brief Set a bold property of current font.
     *
     * @param aBold tells if the font should be bold or not.
     */
    inline void SetBold( const bool aBold )
    {
        m_bold = aBold;
    }

    /**
     * @brief Set an italic property of current font.
     *
     * @param aItalic tells if the font should be italic or not.
     */
    inline void SetItalic( const bool aItalic )
    {
        m_italic = aItalic;
    }

    /**
     * @brief Set a mirrored property of text.
     *
     * @param aMirrored tells if the text should be mirrored or not.
     */
    inline void SetMirrored( const bool aMirrored )
    {
        m_mirrored = aMirrored;
    }

    /**
     * @brief Set the horizontal justify for text drawing.
     *
     * @param aHorizontalJustify is the horizontal justify value.
     */
    inline void SetHorizontalJustify( const EDA_TEXT_HJUSTIFY_T aHorizontalJustify )
    {
        m_horizontalJustify = aHorizontalJustify;
    }

    /**
     * @brief Set the vertical justify for text drawing.
     *
     * @param aVerticalJustify is the vertical justify value.
     */
    inline void SetVerticalJustify( const EDA_TEXT_VJUSTIFY_T aVerticalJustify )
    {
        m_verticalJustify = aVerticalJustify;
    }

    /**
     * Function SetGAL
     * Changes Graphics Abstraction Layer used for drawing items for a new one.
     * @param aGal is the new GAL instance.
     */
    void SetGAL( GAL* aGal )
    {
        m_gal = aGal;
    }

private:
    GAL*                m_gal;                                    ///< Pointer to the GAL
    GLYPH_LIST          m_glyphs;                                 ///< Glyph list
    std::deque<BOX2D>   m_glyphBoundingBoxes;                     ///< Bounding boxes of the glyphs
    double              m_scaleFactor;                            ///< Scale factor for the glyph
    VECTOR2D            m_glyphSize;                              ///< Size of the glyphs
    EDA_TEXT_HJUSTIFY_T m_horizontalJustify;                      ///< Horizontal justification
    EDA_TEXT_VJUSTIFY_T m_verticalJustify;                        ///< Vertical justification
    bool                m_bold, m_italic, m_mirrored, m_overbar;  ///< Properties of text

    /**
     * @brief Compute the bounding box of a given glyph.
     *
     * @param aGlyph is the glyph.
     * @param aGlyphBoundingX is the x-component of the bounding box size.
     * @return is the complete bounding box size.
     */
    BOX2D computeBoundingBox( const GLYPH& aGlyph, const VECTOR2D& aGlyphBoundingX ) const;

    /**
     * @brief Compute the size of a given text.
     *
     * @param aText is the text string.
     * @return is the text size.
     */
    VECTOR2D computeTextSize( const wxString& aText ) const;

    static const double LINE_HEIGHT_RATIO;
};
} // namespace KIGFX

#endif /* STROKE_FONT_H_ */
