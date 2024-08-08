/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GEOMETRY_OVAL_H_
#define GEOMETRY_OVAL_H_

#include <vector>

#include <math/vector2d.h>
#include <geometry/eda_angle.h>
#include <geometry/point_types.h>

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
 * @param aOvalSize - The size of the oval (overall length and width)
 * @param aRotation - The rotation of the oval
 * @param aFlags - The flags indicating which points to return
 *
 * @return std::vector<TYPED_POINT2I> - The list of points and their geomtrical types
 */
std::vector<TYPED_POINT2I> GetOvalKeyPoints( const VECTOR2I& aOvalSize, const EDA_ANGLE& aRotation,
                                             OVAL_KEY_POINT_FLAGS aFlags );

#endif /* GEOMETRY_OVAL_H_ */