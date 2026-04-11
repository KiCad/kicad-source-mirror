/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Macro definitions
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

#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

namespace KIGFX
{
/**
 * RENDER_TARGET: Possible rendering targets
 */
enum RENDER_TARGET
{
    TARGET_CACHED = 0,      ///< Main rendering target (cached)
    TARGET_NONCACHED,       ///< Auxiliary rendering target (noncached)
    TARGET_OVERLAY,         ///< Items that may change while the view stays the same (noncached)
    TARGET_TEMP,            ///< Temporary target for drawing in separate layer
    TARGETS_NUMBER          ///< Number of available rendering targets
};

// Used in view.h to initialize VIEW_MAX_LAYERS and graphic_abstraction_layer.cpp
#define MAX_LAYERS_FOR_VIEW 2048
} // namespace KIGFX

#endif /* DEFINITIONS_H_ */
