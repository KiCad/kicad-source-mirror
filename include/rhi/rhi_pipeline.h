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
#include <rhi/rhi_bind_group.h>
#include <rhi/rhi_texture.h>
#include <cstdint>
#include <vector>

enum class RHI_BLEND_FACTOR
{
    ZERO,
    ONE,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA
};

enum class RHI_BLEND_OP
{
    ADD
};

enum class RHI_COMPARE_OP
{
    LESS,
    LESS_EQUAL
};

enum class RHI_VERTEX_FORMAT
{
    FLOAT32X2,
    FLOAT32X4,
    FLOAT16X2,
    FLOAT16X4,
    UINT8X4_NORM,
    UINT16X2
};

enum class RHI_VERTEX_STEP_MODE
{
    VERTEX,
    INSTANCE
};

enum class RHI_PRIMITIVE_TOPOLOGY
{
    TRIANGLE_LIST
};

enum class RHI_DYNAMIC_STATE
{
    VIEWPORT,
    SCISSOR
};


struct RHI_VERTEX_ATTRIBUTE_DESC
{
    uint32_t         location   = 0;
    RHI_VERTEX_FORMAT format    = RHI_VERTEX_FORMAT::FLOAT32X4;
    uint32_t         offset     = 0;
};

struct RHI_VERTEX_BUFFER_LAYOUT_DESC
{
    uint32_t                                stride   = 0;
    RHI_VERTEX_STEP_MODE                    stepMode = RHI_VERTEX_STEP_MODE::VERTEX;
    std::vector<RHI_VERTEX_ATTRIBUTE_DESC>  attributes;
};

struct RHI_BLEND_STATE
{
    bool              enabled      = false;
    RHI_BLEND_FACTOR  srcColor     = RHI_BLEND_FACTOR::ONE;
    RHI_BLEND_FACTOR  dstColor     = RHI_BLEND_FACTOR::ZERO;
    RHI_BLEND_OP      colorOp      = RHI_BLEND_OP::ADD;
    RHI_BLEND_FACTOR  srcAlpha     = RHI_BLEND_FACTOR::ONE;
    RHI_BLEND_FACTOR  dstAlpha     = RHI_BLEND_FACTOR::ZERO;
    RHI_BLEND_OP      alphaOp      = RHI_BLEND_OP::ADD;
};

struct RHI_DEPTH_STENCIL_STATE
{
    bool            depthTestEnable  = false;
    bool            depthWriteEnable = false;
    RHI_COMPARE_OP  depthCompareOp   = RHI_COMPARE_OP::LESS;
};

/**
 * Everything here is fixed at creation except the states listed in dynamicStates.
 */
struct RHI_PIPELINE_DESC
{
    RHI_ShaderHandle vertexShader;
    RHI_ShaderHandle fragmentShader;

    std::vector<RHI_VERTEX_BUFFER_LAYOUT_DESC> vertexBuffers;

    RHI_PRIMITIVE_TOPOLOGY topology = RHI_PRIMITIVE_TOPOLOGY::TRIANGLE_LIST;

    /** One per color attachment. */
    std::vector<RHI_BLEND_STATE> blendStates;

    RHI_DEPTH_STENCIL_STATE depthStencil;

    /** Element n is bind group n, as passed to RHI_DEVICE::BindGroup(). */
    std::vector<RHI_BIND_GROUP_LAYOUT_DESC> bindGroupLayouts;

    RHI_TEXTURE_FORMAT colorFormat = RHI_TEXTURE_FORMAT::RGBA8;

    bool renderPassHasDepthStencil = false;

    std::vector<RHI_DYNAMIC_STATE> dynamicStates;
};


/**
 * Return true when the pipeline targets framebuffers with a depth/stencil attachment. Metal rejects a
 * bind whose pipeline and framebuffer disagree on it.
 */
inline bool RhiPipelineUsesDepthStencil( const RHI_PIPELINE_DESC& aDesc )
{
    return aDesc.renderPassHasDepthStencil || aDesc.depthStencil.depthTestEnable;
}
