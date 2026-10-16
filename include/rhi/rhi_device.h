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

#include <chrono>
#include <optional>
#include <utility>
#include <rhi/rhi_types.h>
#include <rhi/rhi_handle.h>
#include <rhi/rhi_deferred_release.h>
#include <rhi/rhi_buffer.h>
#include <rhi/rhi_compute.h>
#include <rhi/rhi_texture.h>
#include <rhi/rhi_shader.h>
#include <rhi/rhi_pipeline.h>
#include <rhi/rhi_render_pass.h>
#include <rhi/rhi_command.h>
#include <rhi/rhi_surface.h>

/** Windows headers define ERROR as a macro. */
#undef ERROR

/**
 * Lets the render thread tell a stale swapchain, which it can recreate, from a lost device.
 */
enum class RHI_FRAME_OP_RESULT
{
    OK,
    OUT_OF_DATE,
    SURFACE_LOST,
    DEVICE_LOST,
    ERROR
};


enum class RHI_SURFACE_EXTENT_POLL_RESULT
{
    IDLE,
    WAITING,
    RESIZED,
    FAILED
};

/**
 * Creates, uses and destroys GPU resources through one graphics API backend.
 * RhiCreatePlatformDevice() creates the backend a platform is built with.
 */
class RHI_DEVICE
{
public:
    virtual ~RHI_DEVICE() = default;

    /** Compute defaults keep backends without compute support usable for graphics. */
    virtual bool                        SupportsCompute() const { return false; }
    virtual RHI_COMPUTE_DEVICE_LIMITS   ComputeDeviceLimits() const { return {}; }
    virtual RHI_ComputePipelineHandle   CreateComputePipeline( const RHI_COMPUTE_PIPELINE_DESC& ) { return {}; }
    virtual void                        DestroyComputePipeline( RHI_ComputePipelineHandle ) {}
    virtual RHI_COMPUTE_PIPELINE_LIMITS ComputePipelineLimits( RHI_ComputePipelineHandle ) const { return {}; }

    /** Submit, wait for completion, and copy all readbacks before returning. */
    virtual RHI_COMPUTE_RESULT ExecuteCompute( const RHI_COMPUTE_JOB& ) { return RHI_COMPUTE_RESULT::INVALID; }

    /** Backend and GPU name for display, such as "Vulkan (GeForce RTX 3080)". */
    virtual const char* GetName() const = 0;

    virtual RHI_BACKEND GetBackend() const = 0;

    /**
     * Largest texture width or height in texels. The default suits every GPU Metal runs on, and a
     * backend that can query its GPU overrides it.
     */
    virtual uint32_t MaxTextureSize() const { return 16384; }

    virtual bool IsInitialized() const = 0;

    /**
     * Reserve and realize in one call, so render thread only. Returns a null handle on failure.
     */
    RHI_BufferHandle CreateBuffer( const RHI_BUFFER_DESC& desc )
    {
        RHI_BufferHandle handle = ReserveBufferHandle();
        return handle && RealizeBuffer( handle, desc ) ? handle : RHI_BufferHandle{};
    }

    /**
     * Reserve a handle without creating the GPU buffer, so the UI thread can hand out handles while
     * RealizeBuffer() runs later on the render thread. Thread-safe. Returns a null handle when the
     * pool is exhausted.
     */
    virtual RHI_BufferHandle ReserveBufferHandle() = 0;

    /**
     * Create the GPU buffer for a reserved handle. Render thread only. On failure the handle is
     * released and false is returned.
     */
    virtual bool RealizeBuffer( RHI_BufferHandle handle, const RHI_BUFFER_DESC& desc ) = 0;

    /**
     * The handle's generation is retired, so later use of the stale handle is detected. Render thread
     * only, as it reads the frame counter. The UI thread destroys through RHI_DEVICE_PROXY.
     */
    virtual void DestroyBuffer( RHI_BufferHandle handle ) = 0;

    /**
     * The buffer need not be mappable. The backend may go through a staging copy.
     */
    virtual void UpdateBuffer( RHI_BufferHandle handle, const void* data,
                               size_t sizeBytes, size_t offset = 0 ) = 0;

    /**
     * Reserve and realize in one call, so render thread only. Returns a null handle on failure.
     */
    RHI_TextureHandle CreateTexture( const RHI_TEXTURE_DESC& desc )
    {
        RHI_TextureHandle handle = ReserveTextureHandle();
        return handle && RealizeTexture( handle, desc ) ? handle : RHI_TextureHandle{};
    }

    /**
     * Thread-safe handle reservation, realized later by RealizeTexture(). See ReserveBufferHandle().
     */
    virtual RHI_TextureHandle ReserveTextureHandle() = 0;

    /**
     * Create the GPU texture for a reserved handle. Render thread only. On failure the handle is
     * released and false is returned.
     */
    virtual bool RealizeTexture( RHI_TextureHandle handle, const RHI_TEXTURE_DESC& desc ) = 0;

    /** Render thread only. See DestroyBuffer(). */
    virtual void DestroyTexture( RHI_TextureHandle handle ) = 0;

    /**
     * Data is tightly packed rows in the texture's format, exactly width * height * bpp bytes.
     */
    virtual void UploadTexture( RHI_TextureHandle handle, const void* data, size_t sizeBytes ) = 0;

    /**
     * UpdateBuffer() and UploadTexture() calls between these share one submission instead of
     * syncing with the GPU each time. The default does nothing.
     */
    virtual void BeginUploadBatch() {}
    virtual void EndUploadBatch() {}

    /** Returns a null handle on failure. */
    virtual RHI_ShaderHandle CreateShader( const RHI_SHADER_DESC& desc ) = 0;

    virtual void DestroyShader( RHI_ShaderHandle handle ) = 0;

    /** Returns a null handle on failure. */
    virtual RHI_PipelineHandle CreatePipeline( const RHI_PIPELINE_DESC& desc ) = 0;

    virtual void DestroyPipeline( RHI_PipelineHandle handle ) = 0;

    /** Returns a null handle on failure. */
    virtual RHI_RenderPassHandle CreateRenderPass( const RHI_RENDER_PASS_DESC& desc ) = 0;

    virtual void DestroyRenderPass( RHI_RenderPassHandle handle ) = 0;

    /** Returns a null handle on failure. */
    virtual RHI_FramebufferHandle CreateFramebuffer( const RHI_FRAMEBUFFER_DESC& desc ) = 0;

    virtual void DestroyFramebuffer( RHI_FramebufferHandle handle ) = 0;

    /**
     * The texture lets a later pass sample the framebuffer. Returns a null handle if the framebuffer
     * or attachment index is invalid. The framebuffer owns the texture, so never destroy or release it.
     */
    virtual RHI_TextureHandle GetFramebufferColorTexture(
            RHI_FramebufferHandle handle, uint32_t attachmentIndex = 0 ) const = 0;

    /** Returns a null handle on failure. */
    virtual RHI_BindGroupLayoutHandle CreateBindGroupLayout(
            const RHI_BIND_GROUP_LAYOUT_DESC& desc ) = 0;

    virtual void DestroyBindGroupLayout( RHI_BindGroupLayoutHandle handle ) = 0;

    /** Returns a null handle on failure. */
    virtual RHI_BindGroupHandle CreateBindGroup( const RHI_BIND_GROUP_DESC& desc ) = 0;

    virtual void DestroyBindGroup( RHI_BindGroupHandle handle ) = 0;

    /** Only valid inside a render pass with a bound pipeline. */
    virtual void BindGroup( uint32_t groupIndex, RHI_BindGroupHandle group ) = 0;

    /**
     * Draws and dynamic state changes must come between this and EndRenderPass(). Returns false if a
     * render pass is already active or a handle is invalid.
     */
    virtual bool BeginRenderPass( const RHI_RENDER_PASS_BEGIN_DESC& desc ) = 0;

    /** Returns false if no render pass is active. */
    virtual bool EndRenderPass() = 0;

    /** Only valid inside a render pass. */
    virtual void SetViewport( const RHI_VIEWPORT& viewport ) = 0;

    /** Only valid inside a render pass. */
    virtual void SetScissor( const RHI_SCISSOR_RECT& scissor ) = 0;

    /** Only valid inside a render pass. */
    virtual void BindPipeline( RHI_PipelineHandle pipeline ) = 0;

    /**
     * Size must not exceed RHI_MAX_PUSH_CONSTANT_BYTES. Only valid inside a render pass.
     */
    virtual void SetPushConstants( const void* data, size_t sizeBytes ) = 0;

    /** Only valid inside a render pass. */
    virtual void BindVertexBuffer( uint32_t slot, RHI_BufferHandle buffer ) = 0;

    /** Indices are 16 bits on every backend. Only valid inside a render pass. */
    virtual void BindIndexBuffer( RHI_BufferHandle buffer ) = 0;

    /** Draws are only valid inside a render pass with a bound pipeline. */
    virtual void Draw( uint32_t vertexCount, uint32_t firstVertex = 0 ) = 0;

    /** Non-indexed, so vertexOffset is ignored. */
    virtual void DrawInstanced( uint32_t vertexCount, uint32_t instanceCount,
                                uint32_t firstVertex = 0, int32_t vertexOffset = 0,
                                uint32_t firstInstance = 0 ) = 0;

    /** Reads 16-bit indices from the bound index buffer. */
    virtual void DrawIndexedInstanced( uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0,
                                       int32_t vertexOffset = 0, uint32_t firstInstance = 0 ) = 0;

    /** Returns a null handle if the native window handle is invalid. */
    virtual RHI_SurfaceHandle CreateSurface( const RHI_SURFACE_DESC& desc ) = 0;

    /** Destroy the surface's swapchain first. */
    virtual void DestroySurface( RHI_SurfaceHandle handle ) = 0;

    /** Returns a null handle on failure. */
    virtual RHI_SwapchainHandle CreateSwapchain( const RHI_SWAPCHAIN_DESC& desc ) = 0;

    /** Call when the window size or DPI scale changes. Returns false on failure. */
    virtual bool ResizeSwapchain( RHI_SwapchainHandle handle, uint32_t width,
                                  uint32_t height, float dpiScale ) = 0;

    virtual void DestroySwapchain( RHI_SwapchainHandle handle ) = 0;

    /** Destroy the resource, if there is one, and clear the handle. */
    void Release( RHI_BufferHandle& aHandle ) { DestroyBuffer( std::exchange( aHandle, {} ) ); }
    void Release( RHI_TextureHandle& aHandle ) { DestroyTexture( std::exchange( aHandle, {} ) ); }
    void Release( RHI_ShaderHandle& aHandle ) { DestroyShader( std::exchange( aHandle, {} ) ); }
    void Release( RHI_PipelineHandle& aHandle ) { DestroyPipeline( std::exchange( aHandle, {} ) ); }
    void Release( RHI_BindGroupLayoutHandle& aHandle ) { DestroyBindGroupLayout( std::exchange( aHandle, {} ) ); }
    void Release( RHI_RenderPassHandle& aHandle ) { DestroyRenderPass( std::exchange( aHandle, {} ) ); }
    void Release( RHI_FramebufferHandle& aHandle ) { DestroyFramebuffer( std::exchange( aHandle, {} ) ); }
    void Release( RHI_BindGroupHandle& aHandle ) { DestroyBindGroup( std::exchange( aHandle, {} ) ); }
    void Release( RHI_SurfaceHandle& aHandle ) { DestroySurface( std::exchange( aHandle, {} ) ); }
    void Release( RHI_SwapchainHandle& aHandle ) { DestroySwapchain( std::exchange( aHandle, {} ) ); }

    /**
     * When @p aSrcFBO is valid its first color attachment is copied to the swapchain first.
     * OUT_OF_DATE means the swapchain must be recreated, and SURFACE_LOST means the surface must be too.
     *
     * @param aSourceRect the part of @p aSrcFBO to show, in texels in storage order as for
     *                    CopyFramebufferDepth(), or all of it when null.
     */
    virtual RHI_FRAME_OP_RESULT Present( RHI_SwapchainHandle handle, RHI_FramebufferHandle aSrcFBO = {},
                                          const RHI_SCISSOR_RECT* aSourceRect = nullptr ) = 0;

    /**
     * @p buffer must hold at least width * height * bpp bytes. Returns false if the handle or
     * attachment index is invalid or the backend cannot read back.
     */
    virtual bool ReadbackFramebufferPixels( RHI_FramebufferHandle handle,
                                            uint32_t attachmentIndex,
                                            void* buffer, size_t bufferSize ) = 0;

    /**
     * Fill all of @p aDestination with source depth from (@p aSourceX, @p aSourceY) on, ordered
     * after passes already issued. The offset counts texels in storage order, which follows
     * normalized device Y on every backend. Must not be called inside a render pass.
     *
     * @return false if either framebuffer lacks depth, the region does not fit in the source, the
     *         source has not been rendered yet or the device cannot copy depth.
     */
    virtual bool CopyFramebufferDepth( RHI_FramebufferHandle aSource, RHI_FramebufferHandle aDestination,
                                       uint32_t aSourceX = 0, uint32_t aSourceY = 0 )
    {
        return false;
    }

    /**
     * Wait until fewer than @p aAllowed presents are queued for the display or the timeout passes,
     * so the next frame renders from the newest input.
     *
     * @return true if it had to wait.
     */
    virtual bool WaitForPresentSlot( RHI_SwapchainHandle aSwapchain, std::chrono::milliseconds aTimeout,
                                     int aAllowed )
    {
        return false;
    }

    /** Retry a fixed surface extent that lagged behind the requested window size. */
    virtual RHI_SURFACE_EXTENT_POLL_RESULT PollSurfaceExtent( RHI_SwapchainHandle aSwapchain )
    {
        return RHI_SURFACE_EXTENT_POLL_RESULT::IDLE;
    }

    /**
     * False when presents show at once instead of at the next vertical blank, so frames drawn
     * faster than the display refreshes are never seen.
     */
    virtual bool PresentWaitsForDisplay( RHI_SwapchainHandle aSwapchain ) const { return true; }

    /**
     * True when a present replaces one still queued for the next vertical blank, as in Vulkan
     * MAILBOX mode, so frames drawn faster than the display only cost GPU time.
     */
    virtual bool PresentReplacesQueued( RHI_SwapchainHandle aSwapchain ) const { return false; }

    /**
     * The vertical blank at which the display last showed a frame, or nothing if the backend
     * cannot tell.
     */
    virtual std::optional<std::chrono::steady_clock::time_point>
    LastDisplayRefresh( RHI_SwapchainHandle aSwapchain ) const
    {
        return std::nullopt;
    }


    /**
     * Retire deferred releases that are due. The render thread must call this once per frame after
     * presenting.
     */
    virtual void EndFrame()
    {
        m_frameCounter++;
        ExecuteDeferredReleases();
    }

    /** Releases still waiting on later frames, which only EndFrame() retires. */
    size_t PendingDeferredReleases() const { return m_deferredReleaseQueue.PendingCount(); }

protected:
    virtual void ExecuteDeferredReleases() = 0;

    /**
     * Only the destroy that wins the pending-destroy claim enqueues, so racing destroys of one
     * handle cannot free it twice.
     */
    template <typename Tag>
    void deferDestroy( RHI_HANDLE_POOL<Tag>& aPool, RHI_DEFERRED_RESOURCE_TYPE aType,
                       RHI_HANDLE<Tag> aHandle )
    {
        if( aPool.MarkPendingDestroy( aHandle ) )
            m_deferredReleaseQueue.Enqueue( aType, aHandle.value, m_frameCounter );
    }

    uint64_t                   m_frameCounter = 0;
    RHI_DEFERRED_RELEASE_QUEUE m_deferredReleaseQueue;
};
