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
 * Utilities for creating useful line chain idioms commonly founds in
 * QA utilities and tests
 */

#ifndef QA_UTILS_GEOMETRY_LINE_CHAIN_CONSTRUCTION__H
#define QA_UTILS_GEOMETRY_LINE_CHAIN_CONSTRUCTION__H

#include <geometry/shape_line_chain.h>

namespace KI_TEST
{

/**
 * Builds a rectangular #SHAPE_LINE_CHAIN of a certain size at a certain centre
 * @param  aSize   the rectangle size
 * @param  aCentre centre of the rectangle
 * @return         a closed line chain of the rectangle
 */
SHAPE_LINE_CHAIN BuildRectChain( const VECTOR2I& aSize, const VECTOR2I& aCentre = { 0, 0 } );

/**
 * Builds a square #SHAPE_LINE_CHAIN of a certain size at a certain centre
 * @param  aSize   the square size (x == y)
 * @param  aCentre centre of the square
 * @return         a closed line chain of the square
 */
SHAPE_LINE_CHAIN BuildSquareChain( int aSize, const VECTOR2I& aCentre = { 0, 0 } );

} // namespace KI_TEST

#endif // QA_UTILS_GEOMETRY_LINE_CHAIN_CONSTRUCTION__H