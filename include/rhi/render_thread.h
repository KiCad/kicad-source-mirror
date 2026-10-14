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

#ifndef RHI_RENDER_THREAD_H
#define RHI_RENDER_THREAD_H

#include <rhi/rhi_control_commands.h>
#include <rhi/rhi_upload_commands.h>

#include <rhi/render_thread_window_state.h>
#include <rhi/rhi_gal_pipelines.h>
#include <rhi/rhi_glyph_atlas.h>
#include <rhi/rhi_compositor.h>

#include <core/sync_queue.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>


class RHI_DEVICE;
class wxImage;


namespace KIGFX
{

/**
 * Creates the RHI_DEVICE. It runs on the render thread so the GPU context is bound to that thread.
 */
using RHI_DEVICE_FACTORY = std::function<std::unique_ptr<RHI_DEVICE>()>;


/**
 * Thread that owns the RHI device. Other threads talk to it through a control queue for lifecycle
 * and frame commands and an upload queue for data transfers.
 */
class RENDER_THREAD
{
public:
    explicit RENDER_THREAD( RHI_DEVICE_FACTORY aDeviceFactory );

    ~RENDER_THREAD();

    RENDER_THREAD( const RENDER_THREAD& ) = delete;
    RENDER_THREAD& operator=( const RENDER_THREAD& ) = delete;

    /**
     * Block until the render thread has created the device and its pipelines.
     *
     * @return false if either could not be created.
     */
    bool Start();

    bool IsRunning() const;

    /**
     * Give the track shaders the font atlas glyph instances draw from. Only the first atlas is kept.
     * Callable from any thread.
     */
    void SetGlyphAtlas( std::shared_ptr<const RHI_GLYPH_ATLAS_DATA> aAtlas );

    /**
     * @return true if the command was enqueued, which it is until the thread starts shutting down.
     */
    bool SubmitControl( RHI_CONTROL_COMMAND aCmd );

    /**
     * Callable from any thread.
     */
    void SubmitUpload( RHI_UPLOAD_COMMAND aCmd );

    /**
     * Block until every command submitted before this call has been processed.
     *
     * @return true if the fence completed within the timeout.
     */
    bool WaitForFence( std::chrono::milliseconds aTimeout = std::chrono::milliseconds( 2000 ) );

    /**
     * Block until the render thread exits. Safe to call repeatedly or if the thread never started.
     */
    void Shutdown();

    /**
     * @return RHI_BACKEND::NONE before Start(). Safe from any thread.
     */
    RHI_BACKEND GetBackendType() const { return m_backendType.load( std::memory_order_acquire ); }

    /**
     * @return the device's RHI_DEVICE::MaxTextureSize(), or 0 before Start(). Safe from any thread.
     */
    uint32_t GetMaxTextureSize() const { return m_maxTextureSize.load( std::memory_order_acquire ); }

    /**
     * Reserve a handle without creating the GPU resource, so RHI_DEVICE_PROXY can hand it out on the
     * UI thread and create the resource on the render thread. Null when there is no device.
     */
    RHI_BufferHandle  ReserveBufferHandle();
    RHI_TextureHandle ReserveTextureHandle();

    /**
     * Read back a window's picture at backing resolution without the overlay and cursor. Blocks the
     * calling thread until the render thread has read it.
     */
    bool ReadbackWindowPixels( RHI_WINDOW_ID aWindowId, wxImage& aImage );

    /**
     * @return the render loop's phase, for diagnosing stalls. 0 idle, 1 control drain, 2 upload
     *         drain, 3 window processing, 4 geometry, 5 composite, 6 present.
     */
    int GetLoopPhase() const { return m_loopPhase.load( std::memory_order_relaxed ); }

    /**
     * @return true once the device was lost, after which nothing is drawn. See OnDeviceLost().
     */
    bool IsDeviceLost() const { return m_deviceLost.load( std::memory_order_acquire ); }

    /**
     * @return true once a frame reached a window, which tells a working device from one that cannot draw.
     */
    bool HasPresented() const { return m_presented.load( std::memory_order_acquire ); }

    /**
     * Called on the render thread when the device is lost. Set before Start().
     */
    void SetDeviceLostHandler( std::function<void()> aHandler ) { m_onDeviceLost = std::move( aHandler ); }

    /**
     * Compute the physical pixel backing size from logical size and DPI scale, at least 1x1.
     */
    static void ComputeBackingResolution( uint32_t aLogicalW, uint32_t aLogicalH, float aDpiScale,
                                          uint32_t& aBackingW, uint32_t& aBackingH );

    /**
     * Stop drawing on a lost device. The GAL's buffers and textures died with it and only a new
     * canvas remakes them, so the owner learns of the loss through IsDeviceLost(). Render thread only.
     */
    void OnDeviceLost();

private:
    void ThreadMain( std::promise<bool> aStarted );
    void DrainControlQueue();
    void DrainUploadQueue();

    void HandleFrameRequest( const RHI_CMD_FRAME_REQUEST& aCmd );
    void HandleSwapchainCreate( const RHI_CMD_SWAPCHAIN_CREATE& aCmd );
    void HandleSwapchainResize( const RHI_CMD_SWAPCHAIN_RESIZE& aCmd );
    void HandleSwapchainDestroy( const RHI_CMD_SWAPCHAIN_DESTROY& aCmd );
    void HandleFence( const RHI_CMD_FENCE& aCmd );

    /**
     * @param aIgnorePacing draw every pending frame now, even one a window's refresh rate would defer.
     */
    void ProcessActiveWindows( bool aIgnorePacing = false );

    bool EnsureSwapchain( WINDOW_STATE& aWS, const char* aReason );

    /**
     * Retire and recreate an out of date swapchain, and its surface too when @p aSurfaceLost. A failed
     * recreation leaves the handle null so the retry backoff applies.
     */
    void recreateSwapchainAfterOutOfDate( WINDOW_STATE& aWS, bool aSurfaceLost = false );

    /**
     * @return true only if a frame was presented, so a skipped or failed frame does not report its
     *         generation as complete.
     */
    bool RenderWindow( WINDOW_STATE& aWS );
    void RenderGeometry( WINDOW_STATE& aWS );
    /** The ranges of a draw descriptor one geometry pass draws. */
    enum class GEOMETRY_RANGES : uint8_t
    {
        ALL,
        NOT_OVERLAY,
        OVERLAY
    };

    /**
     * @param aStaticDrawMargin how much of the static target's margin to draw when @p aFBO is the
     *                          static target. 0 draws only the view in its middle.
     */
    void RenderGeometryInto( WINDOW_STATE& aWS, const FRAME_DRAW_DESC& aDesc, RHI_FramebufferHandle aFBO,
                             bool aLogSummary, GEOMETRY_RANGES aRanges = GEOMETRY_RANGES::ALL,
                             uint32_t aStaticDrawMargin = 0 );
    /**
     * Fill the cursor's uniforms.
     *
     * @return false when there is no cursor to draw this frame.
     */
    bool prepareCursor( WINDOW_STATE& aWS );

    /**
     * Run work on the render thread after everything queued before it.
     *
     * @return false on timeout.
     */
    bool runFenced( std::function<void()> aWork, std::chrono::milliseconds aTimeout );

    /**
     * Write a uniform block, making its buffer and bind group on first use.
     *
     * @return false when either could not be made.
     */
    bool writeUniform( RHI_BufferHandle& aUBO, RHI_BindGroupHandle& aBG, RHI_BindGroupLayoutHandle aLayout,
                       const void* aData, size_t aSize );

    /** Draw the cursor into the open composite pass. */
    void drawCursor( WINDOW_STATE& aWS );

    void DestroyAllWindows();

    /** Free a window's targets, uniforms, swapchain and surface. */
    void destroyWindow( WINDOW_STATE& aWS );

    bool InitRenderInfrastructure();
    void DestroyRenderInfrastructure();

    void CreateWindowFBOs( WINDOW_STATE& aWS );
    void DestroyWindowFBOs( WINDOW_STATE& aWS );
    void DestroyWindowUniforms( WINDOW_STATE& aWS );

    void wakeRenderThread();

    RHI_DEVICE_FACTORY m_deviceFactory;

    std::unique_ptr<RHI_DEVICE> m_device;

    /**
     * Guards the m_device pointer, not the device, against UI thread reads racing its creation or
     * destruction. The device's pools are thread-safe and its maps are render thread only.
     */
    mutable std::mutex          m_deviceMutex;

    std::atomic<RHI_BACKEND>    m_backendType{ RHI_BACKEND::NONE };
    std::atomic<uint32_t>       m_maxTextureSize{ 0 };

    SYNC_QUEUE<RHI_CONTROL_COMMAND> m_controlQueue;
    SYNC_QUEUE<RHI_UPLOAD_COMMAND>  m_uploadQueue;

    std::thread             m_thread;

    std::mutex              m_workMutex;
    std::condition_variable m_workCV;

    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_shutdownRequested{ false };

    /** Guarded by m_workMutex. Cleared before the shutdown drain so nothing queued later is lost. */
    bool              m_acceptingWork = false;

    /** Set by producers and cleared by the drain so the loop can sleep without missing uploads. */
    std::atomic<bool> m_uploadsPending{ false };
    std::atomic<bool> m_deviceLost{ false };
    std::atomic<bool> m_presented{ false };

    std::function<void()> m_onDeviceLost;

    std::atomic<int>  m_loopPhase{ 0 };     /**< see GetLoopPhase() */

    std::unordered_map<RHI_WINDOW_ID, WINDOW_STATE> m_windows;

    std::vector<WINDOW_STATE*> m_activeWindowOrder;

    /** Earliest time a window held to its display's refresh rate is due, so the loop wakes for it. */
    std::optional<WINDOW_STATE::Clock::time_point> m_deferredFrameDue;

    void deferFrameUntil( WINDOW_STATE::Clock::time_point aDue )
    {
        m_deferredFrameDue = m_deferredFrameDue ? std::min( *m_deferredFrameDue, aDue ) : aDue;
    }

    /** Shared by all windows and created once with the device. */
    std::unique_ptr<RHI_GAL_PIPELINES> m_pipelines;
    std::unique_ptr<RHI_COMPOSITOR>    m_compositor;
    RHI_BufferHandle                   m_unitQuadBuffer;
    RHI_BufferHandle                   m_quadCornerBuffer;   /**< the unit quad's 4 corners */
    RHI_BufferHandle                   m_quadIndexBuffer;    /**< its 2 triangles over those corners */

    /** The font atlas the track shaders draw glyphs from, its glyph table and their bind group. */
    RHI_TextureHandle   m_glyphTexture;
    RHI_BufferHandle    m_glyphTable;
    RHI_BindGroupHandle m_glyphBG;
    bool                m_glyphBGHasAtlas = false;

    mutable std::mutex                          m_glyphAtlasMutex;
    std::shared_ptr<const RHI_GLYPH_ATLAS_DATA> m_glyphAtlas;

    RHI_BindGroupHandle glyphBindGroup();
    void                destroyGlyphResources();

    RHI_RenderPassHandle               m_geometryRenderPass;
    RHI_RenderPassHandle               m_geometryLoadRenderPass;
    RHI_RenderPassHandle               m_geometryDepthLoadRenderPass;   /**< clears colour, keeps depth */

    /** As m_geometryRenderPass and m_geometryDepthLoadRenderPass, without storing depth nothing reads later. */
    RHI_RenderPassHandle               m_geometryDepthDiscardRenderPass;
    RHI_RenderPassHandle               m_geometryDepthLoadDiscardRenderPass;
};


} // namespace KIGFX

#endif // RHI_RENDER_THREAD_H
