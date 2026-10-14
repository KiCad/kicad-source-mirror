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
#include <rhi/rhi_texture.h>
#include <cstdint>
#include <vector>

enum class RHI_LOAD_OP
{
    LOAD,
    CLEAR,
    DONT_CARE
};

enum class RHI_STORE_OP
{
    STORE,
    DONT_CARE
};

struct RHI_COLOR_ATTACHMENT_DESC
{
    RHI_TEXTURE_FORMAT format = RHI_TEXTURE_FORMAT::RGBA8;
    RHI_LOAD_OP        loadOp  = RHI_LOAD_OP::CLEAR;
    RHI_STORE_OP       storeOp = RHI_STORE_OP::STORE;
    float              clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
};

struct RHI_DEPTH_STENCIL_ATTACHMENT_DESC
{
    bool          enabled         = false;
    RHI_LOAD_OP   depthLoadOp     = RHI_LOAD_OP::CLEAR;
    RHI_STORE_OP  depthStoreOp    = RHI_STORE_OP::DONT_CARE;
    float         clearDepth      = 1.0f;
    RHI_LOAD_OP   stencilLoadOp   = RHI_LOAD_OP::CLEAR;
    RHI_STORE_OP  stencilStoreOp  = RHI_STORE_OP::DONT_CARE;
    uint32_t      clearStencil    = 0;
};

struct RHI_RENDER_PASS_DESC
{
    std::vector<RHI_COLOR_ATTACHMENT_DESC>  colorAttachments;
    RHI_DEPTH_STENCIL_ATTACHMENT_DESC       depthStencilAttachment;
};

struct RHI_FRAMEBUFFER_DESC
{
    uint32_t                                width  = 0;
    uint32_t                                height = 0;
    std::vector<RHI_TEXTURE_FORMAT>         colorFormats;
    bool                                    hasDepthStencil = false;
};
