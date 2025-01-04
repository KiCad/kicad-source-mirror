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

#ifndef PREVIEW_PREVIEW_UTILS__H_
#define PREVIEW_PREVIEW_UTILS__H_

#include <eda_units.h>
#include <gal/color4d.h>
#include <math/vector2d.h>

namespace KIGFX
{
class GAL;
class VIEW;

namespace PREVIEW
{

struct TEXT_DIMS
{
    VECTOR2I GlyphSize;
    int      StrokeWidth;
    int      ShadowWidth;
    double   LinePitch;
};

/**
 * Default alpha of "de-emphasised" features (like previously locked-in lines.
 */
double PreviewOverlayDeemphAlpha( bool aDeemph = true );


/**
 * Get a formatted string showing a dimension to a sane precision with an optional prefix and
 * unit suffix.
 */
wxString DimensionLabel( const wxString& prefix, double aVal, const EDA_IU_SCALE& aIuScale,
                         EDA_UNITS aUnits, bool aIncludeUnits = true );

/**
 * Set the GAL glyph height to a constant scaled value, so that it always looks the same on screen.
 *
 * @param aGal the GAL to draw on.
 * @param aRelativeSize similar to HTML font sizes; 0 will give a standard size while +1 etc.
 *                      will give larger and -1 etc. will give smaller.
 * @returns the text widths for the resulting glyph size.
 */
TEXT_DIMS GetConstantGlyphHeight( KIGFX::GAL* aGal, int aRelativeSize = 0 );

COLOR4D GetShadowColor( const COLOR4D& aColor );

/**
 * Draw strings next to the cursor.
 *
 * The GAL attribute context will be restored to its original state after this function is called.
 *
 * @param aGal the GAL to draw on.
 * @param aCursorPos the position of the cursor to draw next to.
 * @param aTextQuadrant a vector pointing to the quadrant to draw the text in.
 * @param aStrings list of strings to draw, top to bottom.
 */
void DrawTextNextToCursor( KIGFX::VIEW* aView, const VECTOR2D& aCursorPos,
                           const VECTOR2D& aTextQuadrant, const wxArrayString& aStrings,
                           bool aDrawingDropShadows );

} // namespace PREVIEW
} // namespace KIGFX

#endif  // PREVIEW_PREVIEW_UTILS__H_
