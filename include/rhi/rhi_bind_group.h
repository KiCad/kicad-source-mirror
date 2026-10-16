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
#include <rhi/rhi_handle.h>
#include <algorithm>
#include <cstdint>
#include <vector>

/** Most bind groups one pipeline can use. */
constexpr int RHI_MAX_BIND_GROUPS = 4;

enum class RHI_BINDING_TYPE
{
    UNIFORM_BUFFER,
    COMBINED_TEXTURE_SAMPLER,
    STORAGE_BUFFER,
    STORAGE_BUFFER_READONLY
};

/** Bit flags for the shader stages that see a binding. */
enum class RHI_SHADER_STAGE : uint32_t
{
    NONE     = 0,
    VERTEX   = 1u << 0,
    FRAGMENT = 1u << 1,
    COMPUTE  = 1u << 2,
    ALL      = ( 1u << 0 ) | ( 1u << 1 )  /**< Graphics stages only, compute is opted into explicitly */
};

inline RHI_SHADER_STAGE operator&( RHI_SHADER_STAGE a, RHI_SHADER_STAGE b )
{
    return static_cast<RHI_SHADER_STAGE>( static_cast<uint32_t>( a )
                                          & static_cast<uint32_t>( b ) );
}

inline bool operator!( RHI_SHADER_STAGE a )
{
    return static_cast<uint32_t>( a ) == 0;
}

/**
 * How often a group's resources change. A hint only, as the group index is the layout's position in
 * RHI_PIPELINE_DESC::bindGroupLayouts.
 */
enum class RHI_BIND_FREQUENCY
{
    PER_FRAME  = 0,
    PER_PASS   = 1,
    PER_DRAW   = 3
};

/**
 * MSL is generated with --msl-decoration-binding, so a Metal buffer index is its binding number in
 * every set and push constants take buffer 0. Uniform buffer bindings therefore start at 1 and must
 * not repeat across the sets of one pipeline. Textures and samplers have separate index tables.
 */
constexpr uint32_t RHI_UBO_BINDING = 1;

struct RHI_BINDING_ENTRY
{
    uint32_t          binding = 0;
    RHI_BINDING_TYPE  type    = RHI_BINDING_TYPE::UNIFORM_BUFFER;
    RHI_SHADER_STAGE  stages  = RHI_SHADER_STAGE::ALL;
};

struct RHI_BIND_GROUP_LAYOUT_DESC
{
    RHI_BIND_FREQUENCY                frequency = RHI_BIND_FREQUENCY::PER_FRAME;
    std::vector<RHI_BINDING_ENTRY>    entries;

    /**
     * @return false if a binding index repeats, an entry has no stages or the frequency is not a
     *         valid group index.
     */
    bool Validate() const;

    /** @return true if both layouts declare the same bindings with the same types and stages. */
    bool IsCompatible( const RHI_BIND_GROUP_LAYOUT_DESC& aOther ) const;

    /** @return nullptr when no entry has @p aBinding. */
    const RHI_BINDING_ENTRY* Find( uint32_t aBinding ) const
    {
        auto it = std::ranges::find( entries, aBinding, &RHI_BINDING_ENTRY::binding );

        return it == entries.end() ? nullptr : &*it;
    }
};


/**
 * Buffer bindings set bufferHandle; a COMBINED_TEXTURE_SAMPLER binding sets textureHandle.
 * The binding must exist in the group's layout with that type, which backends do not check.
 */
struct RHI_BIND_GROUP_RESOURCE
{
    uint32_t          binding       = 0;
    RHI_BufferHandle  bufferHandle;
    RHI_TextureHandle textureHandle;
    uint32_t          offset        = 0;
    uint32_t          size          = 0;
};

struct RHI_BIND_GROUP_DESC
{
    RHI_BindGroupLayoutHandle               layout;
    std::vector<RHI_BIND_GROUP_RESOURCE>    entries;
};
