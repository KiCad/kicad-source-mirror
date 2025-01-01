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

#include <pad.h>

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