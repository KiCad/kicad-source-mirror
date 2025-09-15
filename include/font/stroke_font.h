/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef STROKE_FONT_H
#define STROKE_FONT_H

#include <gal/gal.h>
#include <map>
#include <deque>
#include <algorithm>
#include <core/utf8.h>
#include <math/box2.h>
#include <font/font.h>

namespace KIGFX
{
class GAL;
}

namespace KIFONT
{
/**
 * Implement a stroke font drawing.
 *
 * A stroke font is composed of lines.
 */
class GAL_API STROKE_FONT : public FONT
{
public:
    STROKE_FONT();

    bool IsStroke() const override { return true; }

    /**
     * Load a stroke font.
     *
     * @param aFontName is the name of the font. If empty, the standard KiCad stroke font is
     *                  loaded.
     */
    static STROKE_FONT* LoadFont( const wxString& aFontName );

    /**
     * Compute the distance (interline) between 2 lines of text (for multiline texts).
     *
     * This is the distance between baselines, not the space between line bounding boxes.
     */
    double GetInterline( double aGlyphHeight, const METRICS& aFontMetrics ) const override;

    VECTOR2I GetTextAsGlyphs( BOX2I* aBoundingBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                              const wxString& aText, const VECTOR2I& aSize,
                              const VECTOR2I& aPosition, const EDA_ANGLE& aAngle, bool aMirror,
                              const VECTOR2I& aOrigin, TEXT_STYLE_FLAGS aTextStyle ) const override;

    unsigned GetGlyphCount() const;

    const STROKE_GLYPH* GetGlyph( unsigned aIndex ) const;

    const BOX2D& GetGlyphBoundingBox( unsigned aIndex ) const;

private:
    /**
     * Load the standard KiCad stroke font.
     *
     * @param aNewStrokeFont is the pointer to the font data.
     * @param aNewStrokeFontSize is the size of the font data.
     */
    void loadNewStrokeFont( const char* const aNewStrokeFont[], int aNewStrokeFontSize );

private:
    const std::vector<std::shared_ptr<GLYPH>>* m_glyphs;
    const std::vector<BOX2D>*                  m_glyphBoundingBoxes;
    double                                     m_maxGlyphWidth;
};

} //namespace KIFONT

#endif // STROKE_FONT_H
