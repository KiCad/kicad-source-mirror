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

#include <gal/graphics_abstraction_layer.h>
#include <math/vector2d.h>

/**
 * @file item_drawing_utils.h
 *
 * Utility functions for drawing compound items (i.e. items that
 * need more than one GAL Draw call to render) the might be used be more
 * than one VIEW_ITEM Draw function.
 */

namespace KIGFX
{

/**
 * Draw a cross at a given position.
 *
 * @param aGal The graphics abstraction layer to draw with.
 * @param aPosition The position to draw the cross at.
 * @param aSize The size of the cross.
 */
void DrawCross( GAL& aGal, const VECTOR2I& aPosition, int aSize );

/**
 * Draw a dashed line.
 *
 * @param aGal The graphics abstraction layer to draw with.
 * @param aSeg The line to draw.
 * @param aDashSize The size of the dashes.
 */
void DrawDashedLine( GAL& aGal, const SEG& aSeg, double aDashSize );

} // namespace KIGFX
