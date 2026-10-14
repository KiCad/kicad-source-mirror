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
#include <rhi/rhi_render_pass.h>
#include <cstdint>

/** Top-left origin, in device pixels. */
struct RHI_VIEWPORT
{
    float x        = 0.0f;
    float y        = 0.0f;
    float width    = 0.0f;
    float height   = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

/** Same pixel space as RHI_VIEWPORT. */
struct RHI_SCISSOR_RECT
{
    int32_t  x      = 0;
    int32_t  y      = 0;
    uint32_t width  = 0;
    uint32_t height = 0;
};

struct RHI_RENDER_PASS_BEGIN_DESC
{
    RHI_RenderPassHandle  renderPass;
    RHI_FramebufferHandle framebuffer;

    /**
     * Replaces the render pass clear color of every CLEAR color attachment, so the background can
     * change per frame without a new render pass.
     */
    bool  hasClearColorOverride = false;
    float clearColorOverride[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

/** Largest push constant size SetPushConstants() accepts, in bytes. */
constexpr uint32_t RHI_MAX_PUSH_CONSTANT_BYTES = 256;
