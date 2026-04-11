/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file footprint_utils.h
 *
 * Collection of reusable/testable functions for footprint manipulation.
 */

#pragma once

#include <geometry/eda_angle.h>
#include <math/vector2d.h>

class FOOTPRINT;


/**
 * Compute position and angle shift between two footprints.
 *
 * This will only happen when the new footprint has a different origin or has been
 * rotated compared to the existing footprint. Most of the time, this returns
 * a null shift.
 *
 * The shift is computed based on pad positions and assumes that there are at least
 * two non-coincident pads with unique numbers that are the same in both footprints.
 */
bool ComputeFootprintShift( const FOOTPRINT& aExisting, const FOOTPRINT& aNew, VECTOR2I& aShift,
                            EDA_ANGLE& aAngleShift );