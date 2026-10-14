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

#ifndef RHI_CONTROL_COMMANDS_H
#define RHI_CONTROL_COMMANDS_H

#include <rhi/rhi_surface.h>

#include <cstdint>
#include <functional>
#include <variant>

namespace KIGFX
{
class FRAME_MAILBOX;
}


/**
 * Routes control commands to a swapchain. The main thread assigns it and the render thread treats
 * it as opaque.
 */
using RHI_WINDOW_ID = uint32_t;


struct RHI_CMD_FRAME_REQUEST
{
    RHI_WINDOW_ID windowId   = 0;
    uint64_t      generation = 0;
    float         refreshHz  = 0.0f;    /**< display refresh rate, 0 when unknown */
};


struct RHI_CMD_SWAPCHAIN_CREATE
{
    RHI_WINDOW_ID            windowId   = 0;
    RHI_NATIVE_WINDOW_HANDLE nativeHandle;
    uint32_t                 width      = 0;
    uint32_t                 height     = 0;
    float                    dpiScale   = 1.0f;

    /** Owned by the panel. */
    KIGFX::FRAME_MAILBOX*    frame2DMailbox = nullptr;
};


struct RHI_CMD_SWAPCHAIN_RESIZE
{
    RHI_WINDOW_ID windowId = 0;
    uint32_t      width    = 0;
    uint32_t      height   = 0;
    float         dpiScale = 1.0f;
};


struct RHI_CMD_SWAPCHAIN_DESTROY
{
    RHI_WINDOW_ID windowId = 0;
};


struct RHI_CMD_FENCE
{
    std::function<void()> onComplete;
};


/**
 * One entry in the render thread control queue.
 */
using RHI_CONTROL_COMMAND = std::variant<
    RHI_CMD_FRAME_REQUEST,
    RHI_CMD_SWAPCHAIN_CREATE,
    RHI_CMD_SWAPCHAIN_RESIZE,
    RHI_CMD_SWAPCHAIN_DESTROY,
    RHI_CMD_FENCE
>;



#endif // RHI_CONTROL_COMMANDS_H
