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

/**
 * Matches KIPLATFORM::UI::NATIVE_DISPLAY_TYPE as reported by GetNativeWindowHandles().
 */
enum class RHI_DISPLAY_TYPE
{
    UNKNOWN = 0,
    X11     = 1,
    WAYLAND = 2,
    WINDOWS = 3,
    COCOA   = 4
};

/**
 * Platform-agnostic wrapper for a native window handle.
 * nativeHandle is the HWND on Windows, the Metal host NSView on macOS, the X11 Window cast to a
 * pointer, or the wl_surface. nativeDisplay is the CAMetalLayer, the X11 Display or the wl_display,
 * and is unused on Windows.
 */
struct RHI_NATIVE_WINDOW_HANDLE
{
    void*            nativeHandle  = nullptr;
    void*            nativeDisplay = nullptr;
    RHI_DISPLAY_TYPE displayType   = RHI_DISPLAY_TYPE::UNKNOWN;

    /**
     * A handle is usable only once the window is realized. X11 and Wayland also need the display
     * connection and Cocoa needs the attached Metal layer.
     */
    bool IsValid() const
    {
        if( nativeHandle == nullptr || displayType == RHI_DISPLAY_TYPE::UNKNOWN )
            return false;

        if( displayType == RHI_DISPLAY_TYPE::X11 || displayType == RHI_DISPLAY_TYPE::WAYLAND
            || displayType == RHI_DISPLAY_TYPE::COCOA )
            return nativeDisplay != nullptr;

        return true;
    }
};

/**
 * Descriptor for creating a surface from a native window.
 */
struct RHI_SURFACE_DESC
{
    RHI_NATIVE_WINDOW_HANDLE window;
};

/**
 * Descriptor for creating a swapchain.
 */
struct RHI_SWAPCHAIN_DESC
{
    RHI_SurfaceHandle  surface;
    uint32_t           width       = 0;
    uint32_t           height      = 0;
    float              dpiScale    = 1.0f;
};
