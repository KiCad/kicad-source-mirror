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
 */

#pragma once

#include <cstdint>
#include <vector>

#include <math/vector2d.h>


namespace POISSON_DISK
{
    /**
     * Bridson's "Fast Poisson Disk Sampling in Arbitrary Dimensions" with toroidal
     * boundary conditions on the unit square [0, 1) × [0, 1).
     *
     * "Toroidal" means the minimum-distance check wraps around the unit-square edges,
     * so the resulting point set tiles seamlessly without near-touches at boundaries.
     *
     * Output is fully deterministic per (aMinDist, aSeed).
     *
     * @param aMinDist Minimum distance between samples.  Must be in (0, 0.5].
     * @param aSeed    RNG seed
     *
     * @return Sample positions in [0, 1) × [0, 1).
     */
    std::vector<VECTOR2D> ToroidalUnitTile( double aMinDist, uint32_t aSeed );
}
