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

#ifndef RHI_DEVICE_PROXY_H
#define RHI_DEVICE_PROXY_H

#include <rhi/rhi_handle.h>
#include <rhi/rhi_buffer.h>
#include <rhi/rhi_texture.h>
#include <rhi/rhi_types.h>

#include <cstddef>

namespace KIGFX
{
class RENDER_THREAD;

/**
 * UI-thread access to GPU resources without touching the device, which only the render thread may mutate.
 *
 * Handles are reserved immediately from the thread-safe pool and the real work is queued to the render
 * thread. Queue order guarantees a resource exists before the same frame's uploads or draws use it.
 * Nothing here exposes a device pointer, so UI code cannot race the render thread on the device maps.
 */
class RHI_DEVICE_PROXY
{
public:
    explicit RHI_DEVICE_PROXY( RENDER_THREAD& aRenderThread ) : m_renderThread( aRenderThread ) {}

    RHI_DEVICE_PROXY( const RHI_DEVICE_PROXY& ) = delete;
    RHI_DEVICE_PROXY& operator=( const RHI_DEVICE_PROXY& ) = delete;

    /**
     * @return the reserved handle, or a null handle if the pool is exhausted or there is no device.
     */
    RHI_BufferHandle CreateBuffer( const RHI_BUFFER_DESC& aDesc );

    RHI_TextureHandle CreateTexture( const RHI_TEXTURE_DESC& aDesc );

    /**
     * The data is copied, so the caller may reuse it immediately. The buffer must have been created
     * through this proxy in the same or an earlier frame.
     */
    void UpdateBuffer( RHI_BufferHandle aHandle, const void* aData, size_t aSize );

    void UploadTexture( RHI_TextureHandle aHandle, const void* aData, size_t aSize );

    /**
     * Queued behind the handle's create and uploads. The GPU resource is freed once no in-flight frame
     * can still use it.
     */
    void DestroyBuffer( RHI_BufferHandle aHandle );
    void DestroyTexture( RHI_TextureHandle aHandle );

    RHI_BACKEND GetBackend() const;

    /**
     * @return the largest texture dimension the device supports, or 0 without a device.
     */
    uint32_t MaxTextureSize() const;

    /**
     * @return true if the render thread is running with a device.
     */
    bool IsInitialized() const;

private:
    RENDER_THREAD& m_renderThread;
};

} // namespace KIGFX

#endif // RHI_DEVICE_PROXY_H
