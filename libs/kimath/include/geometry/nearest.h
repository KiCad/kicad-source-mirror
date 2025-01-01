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

#include <variant>
#include <vector>

#include <math/vector2d.h>
#include <math/box2.h>

#include <geometry/circle.h>
#include <geometry/half_line.h>
#include <geometry/line.h>
#include <geometry/seg.h>
#include <geometry/shape_arc.h>


/**
 * A variant type that can hold any of the supported geometry types for
 * nearest point calculations.
 */
using NEARABLE_GEOM = std::variant<LINE, HALF_LINE, SEG, CIRCLE, SHAPE_ARC, BOX2I, VECTOR2I>;

/**
 * Get the nearest point on a geometry to a given point.
 */
VECTOR2I GetNearestPoint( const NEARABLE_GEOM& aGeom, const VECTOR2I& aPt );

/**
 * Get the nearest point on any of a list of geometries to a given point.
 *
 * @param aGeoms The geometries to check.
 * @param aPt The point to find the nearest point to.
 *
 * @return The nearest point on any of the geometries to the given point (or std::nullopt if
 *        no geometries were provided).
 */
OPT_VECTOR2I GetNearestPoint( const std::vector<NEARABLE_GEOM>& aGeoms, const VECTOR2I& aPt );