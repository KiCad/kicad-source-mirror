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

#include <rhi/rhi_projection.h>


bool RhiBackendNeedsYFlip( RHI_BACKEND backend )
{
    // Vulkan and Metal present to top-down framebuffers, so GAL's OpenGL Y-up clip space
    // must be flipped to land right-side up on screen
    return backend == RHI_BACKEND::VULKAN || backend == RHI_BACKEND::METAL;
}


void RhiAdjustProjectionForBackend( float* m, RHI_BACKEND backend )
{
    if( !m )
        return;

    if( backend == RHI_BACKEND::NONE )
        return;

    // Column-major layout, m[col*4 + row]
    if( RhiBackendNeedsYFlip( backend ) )
    {
        m[1]  = -m[1];
        m[5]  = -m[5];
        m[9]  = -m[9];
        m[13] = -m[13];
    }

    // Remap Z from [-1,1] to [0,1]
    for( int col = 0; col < 4; col++ )
    {
        int idx2 = col * 4 + 2;
        int idx3 = col * 4 + 3;
        m[idx2] = 0.5f * m[idx2] + 0.5f * m[idx3];
    }
}
