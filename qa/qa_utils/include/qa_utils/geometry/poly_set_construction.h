/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * @file
 * Utilities for creating useful polygon shapes that are commonly
 * found in QA utilities and tests.
 */

#ifndef QA_UTILS_GEOMETRY_POLY_SET_CONSTRUCTION__H
#define QA_UTILS_GEOMETRY_POLY_SET_CONSTRUCTION__H

#include <geometry/shape_poly_set.h>

#include <qa_utils/geometry/line_chain_construction.h>

namespace KI_TEST
{

/**
 * Construct a #SHAPE_POLY_SET from a list of points for each of outlines
 * (no holes)
 */
SHAPE_POLY_SET BuildPolyset( const std::vector<SHAPE_LINE_CHAIN>& aOutlines );

/**
 * Build a #SHAPE_POLY_SET in the shape of a square outline, with a square
 * hole, both centred at the given point.
 * @param  aOuterSize the size (x and y) of the outer square
 * @param  aInnerSize the size (x and y) of the hole
 * @param  aCentre    the centre point of both squares (default: origin)
 * @return            a SHAPE_POLY_SET of the desired shape
 */
SHAPE_POLY_SET BuildHollowSquare(
        int aOuterSize, int aInnerSize, const VECTOR2I& aCentre = { 0, 0 } );

} // namespace KI_TEST

#endif // QA_UTILS_GEOMETRY_POLY_SET_CONSTRUCTION__H