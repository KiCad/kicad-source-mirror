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

class CIRCLE;
class SHAPE_LINE_CHAIN;
class SHAPE_POLY_SET;

namespace KIGEOM
{
/**
 * Adds a hole to a polygon if it is valid (i.e. it has 3 or more points
 * and a non-zero area.)
 *
 * This is important if you are reading in a polygon from a file and
 * aren't sure if it will end up being valid. Leaving invalid holes in
 * a SHAP_POLY_SET will violate the invariant that holes are non-null.
 *
 * @param aOutline is the outline to add the hole to.
 * @param aHole is the hole to add.
 * @return true if the hole was added, false if it was not.
 */
bool AddHoleIfValid( SHAPE_POLY_SET& aOutline, SHAPE_LINE_CHAIN&& aHole );

} // namespace KIGEOM