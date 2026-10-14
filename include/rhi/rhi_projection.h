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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <rhi/rhi_types.h>

/**
 * Convert a column-major OpenGL projection, with NDC Z in [-1, 1] and Y up, in place to the backend's
 * clip space. With no backend the matrix is left unchanged.
 */
void RhiAdjustProjectionForBackend( float* matrix4x4, RHI_BACKEND backend );

/**
 * @return true if the backend uses Y-down NDC.
 */
bool RhiBackendNeedsYFlip( RHI_BACKEND backend );
