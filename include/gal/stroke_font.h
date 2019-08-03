/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
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

#include <deque>
#include <algorithm>

#include <utf8.h>

#include <eda_text.h>

#include <math/box2.h>

namespace KIGFX
{
class GAL;

typedef std::deque< std::deque<VECTOR2D> > GLYPH;
typedef std::vector<GLYPH>                 GLYPH_LIST;

/**
 * @brief Class STROKE_FONT implements stroke font drawing.
 *
 * A stroke font is composed of lines.
 */
class STROKE_FONT
{
    friend class GAL;

public:
    /// Constructor
    STROKE_FONT( GAL* aGal );

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
     * @param aRotationAngle is the text rotation angle in radians.
     */
    void Draw( const UTF8& aText, const VECTOR2D& aPosition, double aRotationAngle );

    /**
     * Function SetGAL
     * Changes Graphics Abstraction Layer used for drawing items for a new one.
     * @param aGal is the new GAL instance.
     */
    void SetGAL( GAL* aGal )
    {
        m_gal = aGal;
    }

    /**
     * Compute the boundary limits of aText (the bounding box of all shapes).
     * The overbar and alignment are not taken in account, '~' characters are skipped.
     *
     * @return a VECTOR2D giving the width and height of text.
     */
    VECTOR2D ComputeStringBoundaryLimits( const UTF8& aText, const VECTOR2D& aGlyphSize,
                                          double aGlyphThickness ) const;

    /**
     * Compute the vertical position of an overbar, sometimes used in texts.
     * This is the distance between the text base line and the overbar.
     * @param aGlyphHeight is the height (vertical size) of the text.
     * @param aGlyphThickness is the thickness of the lines used to draw the text.
     * @return the relative position of the overbar axis.
     */
    double ComputeOverbarVerticalPosition( double aGlyphHeight, double aGlyphThickness ) const;

    /**
     * @brief Compute the distance (interline) between 2 lines of text (for multiline texts).
     *
     * @param aGlyphHeight is the height (vertical size) of the text.
     * @return the interline.
     */
    static double GetInterline( double aGlyphHeight );



private:
    GAL*                m_gal;                  ///< Pointer to the GAL
    GLYPH_LIST          m_glyphs;               ///< Glyph list
    std::vector<BOX2D>  m_glyphBoundingBoxes;   ///< Bounding boxes of the glyphs

    /**
     * @brief Compute the X and Y size of a given text. The text is expected to be
     * a only one line text.
     *
     * @param aText is the text string (one line).
     * @return the text size.
     */
    VECTOR2D computeTextLineSize( const UTF8& aText ) const;

    /**
     * Compute the vertical position of an overbar, sometimes used in texts.
     * This is the distance between the text base line and the overbar.
     * @return the relative position of the overbar axis.
     */
    double computeOverbarVerticalPosition() const;

    /**
     * @brief Compute the bounding box of a given glyph.
     *
     * @param aGlyph is the glyph.
     * @param aGlyphBoundingX is the x-component of the bounding box size.
     * @return is the complete bounding box size.
     */
    BOX2D computeBoundingBox( const GLYPH& aGlyph, const VECTOR2D& aGlyphBoundingX ) const;

    /**
     * @brief Draws a single line of text. Multiline texts should be split before using the
     * function.
     *
     * @param aText is the text to be drawn.
     */
    void drawSingleLineText( const UTF8& aText );

    /**
     * @brief Returns number of lines for a given text.
     *
     * @param aText is the text to be checked.
     * @return unsigned - The number of lines in aText.
     */
    inline unsigned linesCount( const UTF8& aText ) const
    {
        if( aText.empty() )
            return 0;   // std::count does not work well with empty strings
        else
            // aText.end() - 1 is to skip a newline character that is potentially at the end
            return std::count( aText.begin(), aText.end() - 1, '\n' ) + 1;
    }

    ///> Factor that determines relative vertical position of the overbar.
    static const double OVERBAR_POSITION_FACTOR;

    ///> Factor that determines relative line width for bold text.
    static const double BOLD_FACTOR;

    ///> Scale factor for a glyph
    static const double STROKE_FONT_SCALE;

    ///> Tilt factor for italic style (the is is the scaling factor
    ///> on dY relative coordinates to give a tilst shape
    static const double ITALIC_TILT;

    ///> Factor that determines the pitch between 2 lines.
    static const double INTERLINE_PITCH_RATIO;
};
} // namespace KIGFX

#endif // STROKE_FONT_H_
