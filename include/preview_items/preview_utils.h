/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 Kicad Developers, see change_log.txt for contributors.
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

#include <common.h>
#include <gal/color4d.h>
#include <math/vector2d.h>

namespace KIGFX
{
class GAL;

namespace PREVIEW
{

/**
 * The default fill/stroke color of preview overlay items
 */
COLOR4D PreviewOverlayDefaultColor();

/**
 * The default alpha of overlay fills
 */
double PreviewOverlayFillAlpha();

/**
 * Default alpha of "de-emphasised" features (like previously locked-in
 * lines
 */
double PreviewOverlayDeemphAlpha( bool aDeemph = true );

/**
 * The colour of "special" angle overlay features
 */
COLOR4D PreviewOverlaySpecialAngleColor();

/**
 * Get a formatted string showing a dimension to a sane precision
 * with an optional prefix and unit suffix.
 */
wxString DimensionLabel( const wxString& prefix, double aVal,
                         EDA_UNITS_T aUnits );

/**
 * Set the GAL glyph height to a constant scaled value, so that it
 * always looks the same on screen
 *
 * @param aHeight the height of the glyph, in pixels
 */
void SetConstantGlyphHeight( KIGFX::GAL& aGal, double aHeight );

/**
 * Draw strings next to the cursor
 *
 * @param aGal the GAL to draw on
 * @param aCursorPos the position of the cursor to draw next to
 * @param aTextQuadrant a vector pointing to the quadrant to draw the
 * text in
 * @param aStrings list of strings to draw, top to bottom
 */
void DrawTextNextToCursor( KIGFX::GAL& aGal,
        const VECTOR2D& aCursorPos, const VECTOR2D& aTextQuadrant,
        const std::vector<wxString>& aStrings );

} // PREVIEW
} // KIGFX

#endif  // PREVIEW_PREVIEW_UTILS__H_
