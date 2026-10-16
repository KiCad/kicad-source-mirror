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

#include <rhi/rhi_handle.h>
#include <cstddef>

enum class RHI_BUFFER_TYPE
{
    VERTEX,
    INDEX,
    UNIFORM,
    STORAGE
};

struct RHI_BUFFER_DESC
{
    RHI_BUFFER_TYPE type       = RHI_BUFFER_TYPE::VERTEX;
    size_t          sizeBytes  = 0;
    bool            mappable   = false;
};

/**
 * Return true when [aOffset, aOffset + aSize) lies within aTotal bytes. The sum is never formed
 * because a bad range could wrap size_t and admit an out-of-bounds copy.
 */
constexpr bool RhiRangeWithinBuffer( size_t aOffset, size_t aSize, size_t aTotal )
{
    return aOffset <= aTotal && aSize <= aTotal - aOffset;
}
