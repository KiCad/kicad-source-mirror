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

#include <rhi/rhi_compute.h>
#include <atomic>
#include <memory>
#include <mutex>

class RHI_DEVICE;

/**
 * Process-wide compute service on its own headless device, separate from the canvas device.
 * Calls are serialised and block until done, so run long jobs from a worker thread.
 */
class GPU_COMPUTE
{
public:
    /** Return nullptr if compute is unavailable; replace a lost instance on the next call. */
    static std::shared_ptr<GPU_COMPUTE> Get();
    ~GPU_COMPUTE();

    GPU_COMPUTE( const GPU_COMPUTE& ) = delete;
    GPU_COMPUTE& operator=( const GPU_COMPUTE& ) = delete;

    const RHI_COMPUTE_DEVICE_LIMITS& DeviceLimits() const { return m_limits; }
    RHI_COMPUTE_PIPELINE_LIMITS      PipelineLimits( RHI_ComputePipelineHandle aPipeline );
    RHI_BufferHandle                 CreateStorageBuffer( size_t aBytes );
    RHI_ComputePipelineHandle        CreatePipeline( const RHI_COMPUTE_PIPELINE_DESC& aDesc );
    RHI_BindGroupLayoutHandle        CreateBindGroupLayout( const RHI_BIND_GROUP_LAYOUT_DESC& aDesc );
    RHI_BindGroupHandle              CreateBindGroup( const RHI_BIND_GROUP_DESC& aDesc );
    void                             DestroyBuffer( RHI_BufferHandle aBuffer );
    void                             DestroyPipeline( RHI_ComputePipelineHandle aPipeline );
    void                             DestroyBindGroupLayout( RHI_BindGroupLayoutHandle aLayout );
    void                             DestroyBindGroup( RHI_BindGroupHandle aGroup );

    /** Run the whole job and copy its readbacks before returning. A job can be submitted again. */
    RHI_COMPUTE_RESULT Submit( const RHI_COMPUTE_JOB& aJob );

private:
    explicit GPU_COMPUTE( std::unique_ptr<RHI_DEVICE> aDevice );

    /** Destroy under the lock, then retire the release at once since no job can still be using it. */
    template <typename F>
    void destroy( F aDestroy );

    std::mutex                  m_mutex;
    std::unique_ptr<RHI_DEVICE> m_device;
    RHI_COMPUTE_DEVICE_LIMITS   m_limits;
    std::atomic<bool>           m_lost{ false };
};
