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

#include <rhi/rhi_device_proxy.h>
#include <rhi/render_thread.h>
#include <rhi/rhi_upload_commands.h>

#include <memory>
#include <vector>

using namespace KIGFX;


RHI_BufferHandle RHI_DEVICE_PROXY::CreateBuffer( const RHI_BUFFER_DESC& aDesc )
{
    RHI_BufferHandle handle = m_renderThread.ReserveBufferHandle();

    if( !handle )
        return {};

    RHI_BUFFER_CREATE cmd;
    cmd.handle = handle;
    cmd.desc   = aDesc;
    m_renderThread.SubmitUpload( cmd );

    return handle;
}


RHI_TextureHandle RHI_DEVICE_PROXY::CreateTexture( const RHI_TEXTURE_DESC& aDesc )
{
    RHI_TextureHandle handle = m_renderThread.ReserveTextureHandle();

    if( !handle )
        return {};

    RHI_TEXTURE_CREATE cmd;
    cmd.handle = handle;
    cmd.desc   = aDesc;
    m_renderThread.SubmitUpload( cmd );

    return handle;
}


void RHI_DEVICE_PROXY::UpdateBuffer( RHI_BufferHandle aHandle, const void* aData, size_t aSize )
{
    if( !aHandle || !aData || aSize == 0 )
        return;

    RHI_GEOMETRY_UPLOAD upload;
    RhiUploadCopyStaging( upload, aData, aSize );
    upload.targetBuffer = aHandle;
    m_renderThread.SubmitUpload( std::move( upload ) );
}


void RHI_DEVICE_PROXY::UploadTexture( RHI_TextureHandle aHandle, const void* aData, size_t aSize )
{
    if( !aHandle || !aData || aSize == 0 )
        return;

    RHI_TEXTURE_UPLOAD upload;
    RhiUploadCopyStaging( upload, aData, aSize );
    upload.targetTexture = aHandle;
    m_renderThread.SubmitUpload( std::move( upload ) );
}


void RHI_DEVICE_PROXY::DestroyBuffer( RHI_BufferHandle aHandle )
{
    if( aHandle )
        m_renderThread.SubmitUpload( RHI_BUFFER_DESTROY{ aHandle } );
}


void RHI_DEVICE_PROXY::DestroyTexture( RHI_TextureHandle aHandle )
{
    if( aHandle )
        m_renderThread.SubmitUpload( RHI_TEXTURE_DESTROY{ aHandle } );
}


RHI_BACKEND RHI_DEVICE_PROXY::GetBackend() const
{
    return m_renderThread.GetBackendType();
}


uint32_t RHI_DEVICE_PROXY::MaxTextureSize() const
{
    return m_renderThread.GetMaxTextureSize();
}


bool RHI_DEVICE_PROXY::IsInitialized() const
{
    return m_renderThread.IsRunning();
}
