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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <layer_ids.h>

class PAD;

namespace PAD_UTILS
{

/**
 * @brief Returns true if the pad's rounding ratio is valid (i.e. the pad
 * has a shape where that is meaningful)
 */
bool PadHasMeaningfulRoundingRadius( const PAD& aPad, PCB_LAYER_ID aLayer );


/**
 * @brief Get a sensible default for a rounded rectangle pad's rounding ratio
 *
 * According to IPC-7351C, this is 25%, or 0.25mm, whichever is smaller
 */
double GetDefaultIpcRoundingRatio( const PAD& aPad, PCB_LAYER_ID aLayer );

} // namespace PAD_UTILS
