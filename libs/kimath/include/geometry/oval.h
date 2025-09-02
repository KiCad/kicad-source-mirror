/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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

#include <vector>

#include <geometry/point_types.h>
#include <geometry/shape_segment.h>

/**
 * @file Utility functions for ovals (oblongs/stadiums)
 *
 * An "oval" is represented by SHAPE_SEGMENT, but these functions
 * aren't required for most users of SHAPE_SEGMENT.
 */

class SHAPE_LINE_CHAIN;

namespace KIGEOM
{

SHAPE_LINE_CHAIN ConvertToChain( const SHAPE_SEGMENT& aOval );


enum OVAL_KEY_POINTS
{
    OVAL_CENTER = 1 << 0,
    OVAL_CAP_TIPS = 1 << 1,
    OVAL_CAP_CENTERS = 1 << 2,
    OVAL_SIDE_MIDPOINTS = 1 << 3,
    OVAL_SIDE_ENDS = 1 << 4,
    OVAL_CARDINAL_EXTREMES = 1 << 5,
    OVAL_ALL_KEY_POINTS = 0xFF
};

using OVAL_KEY_POINT_FLAGS = unsigned int;

/**
 * @brief Get a list of interesting points on an oval (rectangle
 * with semicircular end caps)
 *
 * This may includes:
 * - The middles of the sides
 * - The tips of the end caps
 * - The extreme cardinal points of the whole oval (if rotated non-cardinally)
 *
 * @param aOval  - The oval to get the points from
 * @param aFlags - The flags indicating which points to return
 *
 * @return std::vector<TYPED_POINT2I> - The list of points and their geomtrical types
 */
std::vector<TYPED_POINT2I> GetOvalKeyPoints( const SHAPE_SEGMENT& aOval, OVAL_KEY_POINT_FLAGS aFlags );

} // namespace KIGEOM
