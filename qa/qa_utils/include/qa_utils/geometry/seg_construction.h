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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Utilities for creating useful line chain idioms commonly founds in
 * QA utilities and tests
 */

#ifndef QA_UTILS_GEOMETRY_SEG_CONSTRUCTION__H
#define QA_UTILS_GEOMETRY_SEG_CONSTRUCTION__H

#include <geometry/seg.h>

namespace KI_TEST
{

/**
 * Build a horizontal segment from a point with a length
 * @param  aStart  the starting point
 * @param  aLength the segment length
 * @return         the resulting segment
 */
SEG BuildHSeg( const VECTOR2I& aStart, int aLength );

/**
 * Build a vertical segment from a point with a length
 * @param  aStart  the starting point
 * @param  aLength the segment length
 * @return         the resulting segment
 */
SEG BuildVSeg( const VECTOR2I& aStart, int aLength );

} // namespace KI_TEST

#endif // QA_UTILS_GEOMETRY_SEG_CONSTRUCTION__H