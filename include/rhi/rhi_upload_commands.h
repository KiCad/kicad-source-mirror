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

#ifndef RHI_UPLOAD_COMMANDS_H
#define RHI_UPLOAD_COMMANDS_H

#include <rhi/rhi_handle.h>
#include <rhi/rhi_buffer.h>
#include <rhi/rhi_texture.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>


/**
 * Staging data owned by the upload command, so the UI thread can reuse its source at once without
 * racing the render thread.
 *
 * The owner is type-erased so a producer can either copy bytes or move in a typed vector. The data
 * pointer stays valid as long as the owner lives.
 */
struct RHI_STAGING
{
    std::shared_ptr<const void> stagingOwner;
    const void*                 stagingData = nullptr;
    size_t                      stagingSize = 0;

    bool HasStaging() const { return stagingData && stagingSize > 0; }
};


struct RHI_GEOMETRY_UPLOAD : RHI_STAGING
{
    RHI_BufferHandle targetBuffer;
    size_t           targetOffset = 0;
};


struct RHI_TEXTURE_UPLOAD : RHI_STAGING
{
    RHI_TextureHandle targetTexture;
};


/**
 * Copy the source into the upload, for data the producer keeps using after queuing.
 */
inline void RhiUploadCopyStaging( RHI_STAGING& aUpload, const void* aSrc, size_t aBytes )
{
    auto held = std::make_shared<std::vector<uint8_t>>(
            static_cast<const uint8_t*>( aSrc ), static_cast<const uint8_t*>( aSrc ) + aBytes );
    aUpload.stagingData  = held->data();
    aUpload.stagingSize  = aBytes;
    aUpload.stagingOwner = std::move( held );
}


/**
 * Move the vector into the upload without a copy, for data the producer does not read after queuing.
 */
template <typename T>
inline void RhiUploadAdoptStaging( RHI_STAGING& aUpload, std::vector<T>&& aVector )
{
    auto held = std::make_shared<std::vector<T>>( std::move( aVector ) );
    aUpload.stagingData  = held->data();
    aUpload.stagingSize  = held->size() * sizeof( T );
    aUpload.stagingOwner = std::move( held );
}


/**
 * Create the GPU buffer for a handle the UI thread already reserved, so only the render thread
 * writes the device's resource maps. See RHI_DEVICE_PROXY.
 */
struct RHI_BUFFER_CREATE
{
    RHI_BufferHandle handle;
    RHI_BUFFER_DESC  desc;
};


/**
 * Texture counterpart of RHI_BUFFER_CREATE.
 */
struct RHI_TEXTURE_CREATE
{
    RHI_TextureHandle handle;
    RHI_TEXTURE_DESC  desc;
};


/**
 * Destroy a buffer the UI thread created, queued so it runs after the handle's create and uploads.
 * A create that failed has already freed the slot, so the stale handle is then ignored.
 */
struct RHI_BUFFER_DESTROY
{
    RHI_BufferHandle handle;
};


/**
 * Texture counterpart of RHI_BUFFER_DESTROY.
 */
struct RHI_TEXTURE_DESTROY
{
    RHI_TextureHandle handle;
};


/**
 * One entry in the render thread upload queue.
 */
using RHI_UPLOAD_COMMAND = std::variant<
    RHI_GEOMETRY_UPLOAD,
    RHI_TEXTURE_UPLOAD,
    RHI_BUFFER_CREATE,
    RHI_TEXTURE_CREATE,
    RHI_BUFFER_DESTROY,
    RHI_TEXTURE_DESTROY
>;



#endif // RHI_UPLOAD_COMMANDS_H
