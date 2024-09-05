/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file geometry/shape_utils.h
 *
 * @brief Utility functions for working with shapes.
 *
 * These are free functions to avoid bloating the shape classes with functions
 * that only need to be used in a few places and can just use the public
 * interfaces.
 */

#include <array>
#include <optional>

#include <math/vector2d.h>
#include <math/box2.h>

class HALF_LINE;
class LINE;
class SEG;

namespace KIGEOM
{

/**
 * Returns a SEG such that the start point is smaller or equal
 * in x and y compared to the end point.
 */
SEG NormalisedSeg( const SEG& aSeg );

/**
 * Decompose a BOX2 into four segments.
 *
 * Segments are returned in the order: Top, Right, Bottom, Left.
 */
std::array<SEG, 4> BoxToSegs( const BOX2I& aBox );

/**
 * Get the segment of a half-line that is inside a box, if any.
 */
std::optional<SEG> ClipHalfLineToBox( const HALF_LINE& aRay, const BOX2I& aBox );

/**
 * Get the segment of a line that is inside a box, if any.
 */
std::optional<SEG> ClipLineToBox( const LINE& aLine, const BOX2I& aBox );

} // namespace KIGEOM