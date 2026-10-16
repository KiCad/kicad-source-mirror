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

#include <rhi/gpu_compute.h>
#include <rhi/rhi_device.h>
#include <rhi/rhi_platform_device.h>


GPU_COMPUTE::GPU_COMPUTE( std::unique_ptr<RHI_DEVICE> aDevice ) :
        m_device( std::move( aDevice ) ),
        m_limits( m_device->ComputeDeviceLimits() )
{
}


GPU_COMPUTE::~GPU_COMPUTE() = default;


std::shared_ptr<GPU_COMPUTE> GPU_COMPUTE::Get()
{
    static std::mutex                 mutex;
    static std::weak_ptr<GPU_COMPUTE> instance;
    static bool                       unavailable = false;
    std::lock_guard<std::mutex>       lock( mutex );
    std::shared_ptr<GPU_COMPUTE>      service = instance.lock();

    if( service && !service->m_lost )
        return service;

    // Probing a missing GPU costs a device creation, so remember the failure
    if( unavailable )
        return nullptr;

    std::unique_ptr<RHI_DEVICE> device = RhiCreatePlatformDevice( false );

    if( !device || !device->SupportsCompute() )
    {
        unavailable = true;
        return nullptr;
    }

    service.reset( new GPU_COMPUTE( std::move( device ) ) );
    instance = service;
    return service;
}


RHI_COMPUTE_PIPELINE_LIMITS GPU_COMPUTE::PipelineLimits( RHI_ComputePipelineHandle aPipeline )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return m_device->ComputePipelineLimits( aPipeline );
}


RHI_BufferHandle GPU_COMPUTE::CreateStorageBuffer( size_t aBytes )
{
    if( !aBytes || aBytes > m_limits.maxStorageBufferRange )
        return {};

    std::lock_guard<std::mutex> lock( m_mutex );
    return m_device->CreateBuffer( { .type = RHI_BUFFER_TYPE::STORAGE, .sizeBytes = aBytes } );
}


RHI_ComputePipelineHandle GPU_COMPUTE::CreatePipeline( const RHI_COMPUTE_PIPELINE_DESC& aDesc )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return m_device->CreateComputePipeline( aDesc );
}


RHI_BindGroupLayoutHandle GPU_COMPUTE::CreateBindGroupLayout( const RHI_BIND_GROUP_LAYOUT_DESC& aDesc )
{
    if( !RhiValidateComputeBindGroupLayout( aDesc ) )
        return {};

    std::lock_guard<std::mutex> lock( m_mutex );
    return m_device->CreateBindGroupLayout( aDesc );
}


RHI_BindGroupHandle GPU_COMPUTE::CreateBindGroup( const RHI_BIND_GROUP_DESC& aDesc )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return m_device->CreateBindGroup( aDesc );
}


template <typename F>
void GPU_COMPUTE::destroy( F aDestroy )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    aDestroy();

    // Devices retire releases by frame count, and a compute device never ends frames on its own
    for( uint32_t frame = 0; frame < RHI_FRAMES_IN_FLIGHT; ++frame )
        m_device->EndFrame();
}


void GPU_COMPUTE::DestroyBuffer( RHI_BufferHandle aBuffer )
{
    destroy(
            [&]
            {
                m_device->DestroyBuffer( aBuffer );
            } );
}


void GPU_COMPUTE::DestroyPipeline( RHI_ComputePipelineHandle aPipeline )
{
    destroy(
            [&]
            {
                m_device->DestroyComputePipeline( aPipeline );
            } );
}


void GPU_COMPUTE::DestroyBindGroupLayout( RHI_BindGroupLayoutHandle aLayout )
{
    destroy(
            [&]
            {
                m_device->DestroyBindGroupLayout( aLayout );
            } );
}


void GPU_COMPUTE::DestroyBindGroup( RHI_BindGroupHandle aGroup )
{
    destroy(
            [&]
            {
                m_device->DestroyBindGroup( aGroup );
            } );
}


RHI_COMPUTE_RESULT GPU_COMPUTE::Submit( const RHI_COMPUTE_JOB& aJob )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    if( m_lost )
        return RHI_COMPUTE_RESULT::DEVICE_LOST;

    const RHI_COMPUTE_RESULT result = m_device->ExecuteCompute( aJob );
    m_lost = result == RHI_COMPUTE_RESULT::DEVICE_LOST;
    return result;
}
