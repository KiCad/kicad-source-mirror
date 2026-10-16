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
#include <cstdint>

enum class RHI_SHADER_TYPE
{
    VERTEX,
    FRAGMENT,
    COMPUTE
};

/** The bytecode is SPIR-V on Vulkan and MSL source text on Metal. */
struct RHI_SHADER_DESC
{
    RHI_SHADER_TYPE type       = RHI_SHADER_TYPE::VERTEX;
    const uint8_t*  bytecode   = nullptr;
    size_t          sizeBytes  = 0;
};

/**
 * One shader stage as GenerateShaderHeader.cmake embeds it. The MSL text is present only in Metal
 * builds.
 */
struct RHI_SHADER_BLOB
{
    const uint8_t* spirv = nullptr;
    size_t         spirvSize = 0;
    const char*    msl = nullptr;
    size_t         mslSize = 0;
};
