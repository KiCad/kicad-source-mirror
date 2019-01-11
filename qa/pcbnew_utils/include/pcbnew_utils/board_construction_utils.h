/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file board_construction_utils.h
 * Construction utilities for PCB tests
 */

#ifndef QA_PCBNEW_BOARD_CONSTRUCTION_UTILS__H
#define QA_PCBNEW_BOARD_CONSTRUCTION_UTILS__H

#include <vector>

#include <layers_id_colors_and_visibility.h>
#include <math/vector2d.h>

class MODULE;
class SEG;


namespace KI_TEST
{

/**
 * Draw a segment in the given module.
 * @param aMod   The module to add the segment to
 * @param aSeg   The segment geometry
 * @param aWidth The width of the segment
 * @param aLayer The layer to draw on
 */
void DrawSegment( MODULE& aMod, const SEG& aSeg, int aWidth, PCB_LAYER_ID aLayer );

/**
 * Draw a polyline - a set of linked segments
 * @param aMod   The module to add the segment to
 * @param aPts   The polyline points
 * @param aWidth The width of the segments
 * @param aLayer The layer to draw on
 */
void DrawPolyline(
        MODULE& aMod, const std::vector<VECTOR2I>& aPts, int aWidth, PCB_LAYER_ID aLayer );

/**
 * Draw an arc on a module
 * @param aMod    The module to add the segment to
 * @param aCentre The arc centre
 * @param aStart  The arc start point
 * @param aAngle  The arc angle (degrees, NOT deci-degrees)
 * @param aWidth  The width of the arc segment
 * @param aLayer  The layer to draw on
 */
void DrawArc( MODULE& aMod, const VECTOR2I& aCentre, const VECTOR2I& aStart, double aAngle,
        int aWidth, PCB_LAYER_ID aLayer );

/**
 * Draw a rectangle on a module
 * @param aMod    The module to add the rectangle to
 * @param aPos    Rectangle centre point
 * @param aSize   Rectangle size (x, y)
 * @param aRadius Corner radius (0 for a normal rect)
 * @param aWidth  Line width
 * @param aLayer  Layer to draw on
 */
void DrawRect( MODULE& aMod, const VECTOR2I& aPos, const VECTOR2I& aSize, int aRadius, int aWidth,
        PCB_LAYER_ID aLayer );

} // namespace KI_TEST

#endif // QA_PCBNEW_BOARD_CONSTRUCTION_UTILS__H