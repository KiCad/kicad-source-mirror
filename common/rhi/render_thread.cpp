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

#include <rhi/render_thread.h>
#include <rhi/rhi_device.h>
#include <rhi/rhi_glyph_atlas.h>
#include <rhi/rhi_vertex.h>
#include <rhi/rhi_buffer.h>
#include <rhi/rhi_present_mode.h>
#include <rhi/rhi_supersample.h>
#include <rhi/layer_draw_plan.h>
#include <rhi/rhi_cull_runs.h>

#include <advanced_config.h>
#include <core/profile.h>
#include <kiplatform/app.h>
#include <trace_helpers.h>

#include <wx/image.h>
#include <wx/log.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <future>
#include <string>
#include <vector>

namespace
{
// Raising priority may need privileges, so a failure is only logged.
void ApplyRenderThreadPriority( const wxString& aPriority )
{
    const bool low  = aPriority.CmpNoCase( wxS( "low" ) ) == 0;
    const bool high = aPriority.CmpNoCase( wxS( "high" ) ) == 0;

    if( !low && !high )
        return; // "normal" or unrecognized leaves default scheduling

    if( !KIPLATFORM::APP::SetCurrentThreadPriority( high ) )
        wxLogTrace( traceRhi, wxS( "RenderThread: could not set thread priority '%s'" ), aPriority );
}
} // namespace


static long long WindowStateAgeMs( KIGFX::WINDOW_STATE::Clock::time_point aThen,
                                   KIGFX::WINDOW_STATE::Clock::time_point aNow )
{
    if( aThen.time_since_epoch().count() == 0 )
        return -1;

    return static_cast<long long>(
            std::chrono::duration_cast<std::chrono::milliseconds>( aNow - aThen ).count() );
}


static double WindowStateDurationMs( KIGFX::WINDOW_STATE::Clock::time_point aStart,
                                     KIGFX::WINDOW_STATE::Clock::time_point aEnd )
{
    if( aStart.time_since_epoch().count() == 0 || aEnd.time_since_epoch().count() == 0 )
        return 0.0;

    return std::chrono::duration<double, std::milli>( aEnd - aStart ).count();
}


// Depth is for the layer slices
static RHI_FRAMEBUFFER_DESC layerFramebufferDesc( const KIGFX::WINDOW_STATE& aWS )
{
    RHI_FRAMEBUFFER_DESC desc;
    desc.width  = aWS.RenderWidth();
    desc.height = aWS.RenderHeight();
    desc.colorFormats.push_back( RHI_TEXTURE_FORMAT::RGBA8 );
    desc.hasDepthStencil = true;
    return desc;
}


static void destroyDiffCopies( RHI_DEVICE& aDevice, KIGFX::WINDOW_STATE& aWS )
{
    aDevice.Release( aWS.diffBelowFBO );
    aDevice.Release( aWS.staticDiffBelowFBO );
}


static uint32_t staticMarginFor( const KIGFX::WINDOW_STATE& aWS, uint32_t aMaxTextureSize )
{
    if( aWS.supersampleFactor == 0 )
        return 0;

    const uint32_t renderSize = std::max( aWS.RenderWidth(), aWS.RenderHeight() );
    const uint32_t room = renderSize < aMaxTextureSize
                                  ? ( aMaxTextureSize - renderSize ) / ( 2 * aWS.supersampleFactor )
                                  : 0;

    // A quarter of the view lets a drag pan for several frames per redraw. The caps bound a
    // supersampled target's size and keep it within the device's texture limit.
    return std::min( { std::max( aWS.backingWidth, aWS.backingHeight ) / 4, 512u / aWS.supersampleFactor, room } );
}


// The largest factor up to aRequested, from 1 to 4, whose layer targets fit the device's texture size
static uint32_t fitSupersampleFactor( uint32_t aRequested, const KIGFX::WINDOW_STATE& aWS,
                                      uint32_t aMaxTextureSize )
{
    uint32_t factor = std::clamp<uint32_t>( aRequested, 1, 4 );

    while( factor > 1 && std::max( aWS.backingWidth, aWS.backingHeight ) * factor > aMaxTextureSize )
        --factor;

    return factor;
}


// How a target with a margin serves the current view, see WINDOW_STATE::staticMargin
struct MARGIN_PLAN
{
    bool                       reuse = false;  // show what the target holds, shifted by shift
    KIGFX::RHI_VIEW_SHIFT      shift;
    uint32_t                   drawMargin = 0; // how much of the margin a redraw covers
};


static MARGIN_PLAN planMarginTarget( const KIGFX::WINDOW_STATE::RENDERED_CONTENT& aDrawn, uint64_t aEpoch,
                                     const KIGFX::FRAME_DRAW_DESC& aNow, uint32_t aMargin, bool aDrawsGrid )
{
    MARGIN_PLAN plan;

    const bool sameGrid = aDrawn.drawGrid == aNow.drawGrid
                          && ( !aNow.drawGrid || KIGFX::RhiGridMovedOnly( aDrawn.gridUBO, aNow.gridUBO ) );
    const bool sameContent = aDrawn.valid && aDrawn.epoch == aEpoch && ( !aDrawsGrid || sameGrid );

    if( !sameContent )
        return plan;

    const std::optional<KIGFX::RHI_VIEW_SHIFT> shift = KIGFX::RhiViewShift( aDrawn.frameUBO, aNow.frameUBO );

    if( shift && std::abs( shift->x ) <= static_cast<int>( aDrawn.margin )
        && std::abs( shift->y ) <= static_cast<int>( aDrawn.margin ) )
    {
        plan.reuse = true;
        plan.shift = *shift;
    }
    else if( KIGFX::RhiViewMovedOnly( aDrawn.frameUBO, aNow.frameUBO ) )
    {
        // A zoom draws only the view, a pan its margin too
        plan.drawMargin = aMargin;
    }

    return plan;
}


// Sample only the part of a margin target the view sits in
static COMPOSITOR_LAYER marginLayer( const KIGFX::WINDOW_STATE& aWS, RHI_FramebufferHandle aFBO,
                                            const KIGFX::RHI_VIEW_SHIFT& aShift )
{
    COMPOSITOR_LAYER layer{ aFBO, 1.0f };

    if( aWS.staticMargin > 0 )
    {
        const float ss = static_cast<float>( aWS.supersampleFactor );
        const float w = static_cast<float>( aWS.StaticWidth() );
        const float h = static_cast<float>( aWS.StaticHeight() );

        layer.uvScale[0] = aWS.RenderWidth() / w;
        layer.uvScale[1] = aWS.RenderHeight() / h;
        layer.uvOffset[0] = ( static_cast<float>( aWS.staticMargin ) - aShift.x ) * ss / w;
        layer.uvOffset[1] = ( static_cast<float>( aWS.staticMargin ) - aShift.y ) * ss / h;
    }

    return layer;
}


static RHI_FRAMEBUFFER_DESC marginFramebufferDesc( const KIGFX::WINDOW_STATE& aWS )
{
    RHI_FRAMEBUFFER_DESC desc = layerFramebufferDesc( aWS );
    desc.width = aWS.StaticWidth();
    desc.height = aWS.StaticHeight();
    return desc;
}


static bool ShouldRetrySwapchainCreate( const KIGFX::WINDOW_STATE& aWS,
                                        KIGFX::WINDOW_STATE::Clock::time_point aNow,
                                        std::chrono::milliseconds aBackoff =
                                                std::chrono::milliseconds( 250 ) )
{
    if( aWS.lastSwapchainCreateAttemptTime.time_since_epoch().count() == 0 )
        return true;

    return std::chrono::duration_cast<std::chrono::milliseconds>(
                   aNow - aWS.lastSwapchainCreateAttemptTime ) >= aBackoff;
}


namespace KIGFX
{


void RENDER_THREAD::recreateSwapchainAfterOutOfDate( WINDOW_STATE& aWS, bool aSurfaceLost )
{
    // Not resized in place because a failed resize leaves a non-null but empty swapchain that
    // Present() dereferences.
    m_device->Release( aWS.swapchain );

    // A swapchain must be destroyed before its surface
    if( aSurfaceLost )
        m_device->Release( aWS.surface );

    EnsureSwapchain( aWS, "present-out-of-date" );
}


bool RENDER_THREAD::EnsureSwapchain( WINDOW_STATE& aWS, const char* aReason )
{
    if( !m_device || aWS.swapchain
        || aWS.backingWidth == 0 || aWS.backingHeight == 0 )
    {
        return static_cast<bool>( aWS.swapchain );
    }

    auto now = WINDOW_STATE::Clock::now();

    if( !ShouldRetrySwapchainCreate( aWS, now ) )
        return false;

    // GTK/X11 can hand over a child window before it is mapped, and a VkSurfaceKHR made that early
    // never recovers, so a retry remakes the surface too.
    if( aWS.swapchainCreateAttempts > 0 || !aWS.surface )
    {
        if( aWS.surface )
        {
            wxLogTrace( traceRhi, wxS( "RenderThread: refreshing surface before "
                                        "swapchain retry windowId=%u reason=%s "
                                        "attempt=%llu native=%p" ),
                        aWS.windowId, aReason,
                        static_cast<unsigned long long>( aWS.swapchainCreateAttempts ) + 1,
                        aWS.nativeHandle.nativeHandle );
            m_device->DestroySurface( aWS.surface );
            aWS.surface = {};
        }

        if( aWS.nativeHandle.nativeHandle )
        {
            RHI_SURFACE_DESC surfDesc;
            surfDesc.window = aWS.nativeHandle;
            aWS.surface = m_device->CreateSurface( surfDesc );
        }

        wxLogTrace( traceRhi, wxS( "RenderThread: refreshed surface windowId=%u -> %s" ),
                    aWS.windowId, aWS.surface ? "OK" : "FAILED" );
    }

    if( !aWS.surface )
    {
        aWS.lastSwapchainCreateAttemptTime = now;
        aWS.swapchainCreateAttempts++;
        return false;
    }

    aWS.lastSwapchainCreateAttemptTime = now;
    aWS.swapchainCreateAttempts++;

    RHI_SWAPCHAIN_DESC scDesc;
    scDesc.surface  = aWS.surface;
    scDesc.width    = aWS.backingWidth;
    scDesc.height   = aWS.backingHeight;
    scDesc.dpiScale = aWS.dpiScale;
    aWS.swapchain = m_device->CreateSwapchain( scDesc );

    wxLogTrace( traceRhi, wxS( "RenderThread: EnsureSwapchain reason=%s windowId=%u "
                                "attempt=%llu %ux%u -> %s" ),
                aReason, aWS.windowId,
                static_cast<unsigned long long>( aWS.swapchainCreateAttempts ),
                aWS.backingWidth, aWS.backingHeight,
                aWS.swapchain ? "OK" : "FAILED" );

    if( aWS.swapchain )
    {
        // The window is not activated here so that a new window presents nothing until a frame
        // is requested.
        aWS.needsStaticInvalidate = true;
    }

    return static_cast<bool>( aWS.swapchain );
}


RENDER_THREAD::RENDER_THREAD( RHI_DEVICE_FACTORY aDeviceFactory ) :
        m_deviceFactory( std::move( aDeviceFactory ) )
{
}


RENDER_THREAD::~RENDER_THREAD()
{
    Shutdown();
}


bool RENDER_THREAD::Start()
{
    if( m_running.load( std::memory_order_acquire ) )
        return true;

    m_shutdownRequested.store( false, std::memory_order_release );

    std::promise<bool> started;
    std::future<bool>  initialized = started.get_future();

    m_thread = std::thread( &RENDER_THREAD::ThreadMain, this, std::move( started ) );

    if( !initialized.get() )
    {
        if( m_thread.joinable() )
            m_thread.join();

        return false;
    }

    return true;
}


bool RENDER_THREAD::IsRunning() const
{
    return m_running.load( std::memory_order_acquire );
}


void RENDER_THREAD::wakeRenderThread()
{
    // Taking the mutex keeps a notify from landing between the waiter's predicate check and its
    // wait, where it would be lost until the idle heartbeat.
    {
        std::lock_guard<std::mutex> lock( m_workMutex );
    }

    m_workCV.notify_one();
}


bool RENDER_THREAD::SubmitControl( RHI_CONTROL_COMMAND aCmd )
{
    {
        // Pushing under the lock keeps a command from landing after the shutdown drain
        std::lock_guard<std::mutex> lock( m_workMutex );

        if( !m_acceptingWork )
            return false;

        m_controlQueue.move_push( std::move( aCmd ) );
    }

    m_workCV.notify_one();
    return true;
}


void RENDER_THREAD::SubmitUpload( RHI_UPLOAD_COMMAND aCmd )
{
    {
        std::lock_guard<std::mutex> lock( m_workMutex );

        if( !m_acceptingWork )
            return;

        m_uploadQueue.move_push( std::move( aCmd ) );
        m_uploadsPending.store( true, std::memory_order_release );
    }

    m_workCV.notify_one();
}


RHI_BufferHandle RENDER_THREAD::ReserveBufferHandle()
{
    std::lock_guard<std::mutex> lock( m_deviceMutex );
    return m_device ? m_device->ReserveBufferHandle() : RHI_BufferHandle{};
}


RHI_TextureHandle RENDER_THREAD::ReserveTextureHandle()
{
    std::lock_guard<std::mutex> lock( m_deviceMutex );
    return m_device ? m_device->ReserveTextureHandle() : RHI_TextureHandle{};
}


bool RENDER_THREAD::WaitForFence( std::chrono::milliseconds aTimeout )
{
    return runFenced( {}, aTimeout );
}


bool RENDER_THREAD::runFenced( std::function<void()> aWork, std::chrono::milliseconds aTimeout )
{
    // Shared so the render thread can still fire the fence after this call has timed out
    auto              done = std::make_shared<std::promise<void>>();
    std::future<void> completed = done->get_future();

    RHI_CMD_FENCE fence;
    fence.onComplete = [done, work = std::move( aWork )]()
    {
        if( work )
            work();

        done->set_value();
    };

    if( !SubmitControl( std::move( fence ) ) )
        return false;

    return completed.wait_for( aTimeout ) == std::future_status::ready;
}


bool RENDER_THREAD::ReadbackWindowPixels( RHI_WINDOW_ID aWindowId, wxImage& aImage )
{
    struct READBACK
    {
        std::vector<uint8_t> pixels; // empty when the readback failed
        uint32_t             width = 0;
        uint32_t             height = 0;
        uint32_t             backingWidth = 0;
        uint32_t             backingHeight = 0;
    };

    // Shared so the render thread can still write it after this call has timed out
    auto out = std::make_shared<READBACK>();

    auto readback = [this, aWindowId, out]()
    {
        auto it = m_windows.find( aWindowId );

        if( it == m_windows.end() || !it->second.staticFBO || !m_device || IsDeviceLost() )
            return;

        const WINDOW_STATE& ws = it->second;

        // Targets are supersampled but screenshots are at backing resolution
        const uint32_t rw = ws.RenderWidth();
        const uint32_t rh = ws.RenderHeight();

        // Without the overlay and cursor, matching what the OpenGL canvas reads from its main buffer
        std::vector<COMPOSITOR_LAYER> layers;

        if( ws.staticInMargin )
            layers.push_back( marginLayer( ws, ws.staticFBO, ws.staticShift ) );
        else
            layers.push_back( { ws.ShownStaticFBO(), 1.0f } );

        if( ws.dynamicFBO && ws.dynamicGeometryDrawn )
            layers.push_back( { ws.dynamicFBO, 1.0f } );

        RHI_FRAMEBUFFER_DESC pictureDesc;
        pictureDesc.width = rw;
        pictureDesc.height = rh;
        pictureDesc.colorFormats.push_back( RHI_TEXTURE_FORMAT::RGBA8 );

        const RHI_FramebufferHandle picture = m_compositor && m_compositor->IsInitialized()
                                                      ? m_device->CreateFramebuffer( pictureDesc )
                                                      : RHI_FramebufferHandle();

        std::vector<uint8_t> renderPixels( static_cast<size_t>( rw ) * rh * 4 );

        const bool read = picture
                          && m_compositor->Composite( picture, rw, rh, layers, ws.drawDesc.backgroundColor )
                          && m_device->ReadbackFramebufferPixels( picture, 0, renderPixels.data(),
                                                                  renderPixels.size() );

        if( picture )
            m_device->DestroyFramebuffer( picture );

        if( !read )
            return;

        out->width = rw;
        out->height = rh;
        out->backingWidth = ws.backingWidth;
        out->backingHeight = ws.backingHeight;
        out->pixels = std::move( renderPixels );
    };

    if( !runFenced( std::move( readback ), std::chrono::seconds( 5 ) ) || out->pixels.empty() )
        return false;

    const size_t pixelCount = static_cast<size_t>( out->width ) * out->height;

    if( !aImage.Create( static_cast<int>( out->width ), static_cast<int>( out->height ), false ) )
        return false;

    unsigned char* rgb = aImage.GetData();

    // The readback is RGBA8 with a top-left origin and the screenshot is opaque RGB
    for( size_t i = 0; i < pixelCount; ++i )
        std::memcpy( rgb + i * 3, out->pixels.data() + i * 4, 3 );

    if( out->backingWidth != out->width || out->backingHeight != out->height )
    {
        aImage.Rescale( static_cast<int>( out->backingWidth ), static_cast<int>( out->backingHeight ),
                        wxIMAGE_QUALITY_BOX_AVERAGE );
    }

    return aImage.IsOk();
}


void RENDER_THREAD::Shutdown()
{
    wxLogTrace( traceRhi, wxS( "RenderThread: Shutdown requested (running=%s)" ),
                m_running.load( std::memory_order_acquire ) ? "yes" : "no" );

    if( !m_running.load( std::memory_order_acquire ) )
    {
        if( m_thread.joinable() )
            m_thread.join();

        return;
    }

    m_shutdownRequested.store( true, std::memory_order_release );
    wakeRenderThread();

    wxLogTrace( traceRhi, wxS( "RenderThread: waiting for thread join" ) );

    if( m_thread.joinable() )
        m_thread.join();

    wxLogTrace( traceRhi, wxS( "RenderThread: joined" ) );
}


void RENDER_THREAD::ThreadMain( std::promise<bool> aStarted )
{
    wxLogTrace( traceRhi, wxS( "RenderThread: starting" ) );

    ApplyRenderThreadPriority( ADVANCED_CFG::GetCfg().m_RenderThreadPriority );

    bool initialized = false;

    // An exception escaping this thread would terminate KiCad instead of letting the canvas fall back
    try
    {
        if( m_deviceFactory )
            m_device = m_deviceFactory();

        const bool deviceReady = m_device && m_device->IsInitialized();

        if( !deviceReady )
            wxLogTrace( traceRhi, wxS( "RenderThread: device init FAILED" ) );

        // A device without pipelines would draw nothing, so it fails the start and the canvas falls back
        initialized = deviceReady && InitRenderInfrastructure();

        if( deviceReady && !initialized )
            wxLogTrace( traceRhi, wxS( "RenderThread: InitRenderInfrastructure FAILED" ) );
    }
    catch( const std::exception& e )
    {
        wxLogTrace( traceRhi, wxS( "RenderThread: EXCEPTION during init: %s" ), e.what() );
    }
    catch( ... )
    {
        wxLogTrace( traceRhi, wxS( "RenderThread: UNKNOWN EXCEPTION during init" ) );
    }

    if( !initialized )
    {
        DestroyRenderInfrastructure();

        {
            std::lock_guard<std::mutex> lock( m_deviceMutex );
            m_device.reset();
        }

        aStarted.set_value( false );
        return;
    }

    m_backendType.store( m_device->GetBackend(), std::memory_order_release );
    m_maxTextureSize.store( m_device->MaxTextureSize(), std::memory_order_release );
    m_running.store( true, std::memory_order_release );

    {
        std::lock_guard<std::mutex> lock( m_workMutex );
        m_acceptingWork = true;
    }

    wxLogTrace( traceRhi, wxS( "RenderThread: device '%s' initialized OK" ), m_device->GetName() );

    aStarted.set_value( true );

    wxLogTrace( traceRhi, wxS( "RenderThread: entering main loop" ) );

    while( !m_shutdownRequested.load( std::memory_order_acquire ) )
    {
        try
        {
            m_loopPhase.store( 1, std::memory_order_relaxed );
            DrainControlQueue();

            if( m_shutdownRequested.load( std::memory_order_acquire ) )
                break;

            // Control commands still drain above so shutdown and window teardown work on a lost
            // device. Nothing else does until the canvas replaces this thread.
            if( !m_device || IsDeviceLost() )
            {
                m_uploadQueue.clear();
                std::this_thread::sleep_for( std::chrono::milliseconds( 8 ) );
                continue;
            }

            m_loopPhase.store( 2, std::memory_order_relaxed );
            DrainUploadQueue();

            m_loopPhase.store( 3, std::memory_order_relaxed );
            ProcessActiveWindows();

            m_loopPhase.store( 4, std::memory_order_relaxed );

            if( !IsDeviceLost() )
                m_device->EndFrame();

            m_loopPhase.store( 0, std::memory_order_relaxed );

            // Only deferred releases and a window still waiting for its swapchain need the short
            // tick. Otherwise a slow heartbeat lets an idle canvas sleep.
            bool needsTick = m_device && m_device->PendingDeferredReleases() > 0;

            for( const auto& [id, ws] : m_windows )
                needsTick = needsTick || !ws.swapchain;

            std::chrono::microseconds tick = needsTick ? std::chrono::milliseconds( 16 )
                                                       : std::chrono::milliseconds( 500 );

            if( m_deferredFrameDue )
            {
                const auto due = std::chrono::ceil<std::chrono::microseconds>( *m_deferredFrameDue
                                                                               - WINDOW_STATE::Clock::now() );

                tick = std::clamp( due, std::chrono::microseconds( 0 ), tick );
            }

            {
                // The predicate catches work queued while this thread was busy above, whose
                // notify arrived before the wait and was lost.
                std::unique_lock<std::mutex> lock( m_workMutex );
                m_workCV.wait_for( lock, tick,
                                   [this]()
                                   {
                                       return m_shutdownRequested.load(
                                                      std::memory_order_acquire )
                                              || !m_controlQueue.empty()
                                              || m_uploadsPending.load(
                                                         std::memory_order_acquire );
                                   } );
            }
        }
        catch( const std::exception& e )
        {
            wxLogTrace( traceRhi, wxS( "RenderThread: EXCEPTION in main loop phase=%d: %s" ),
                        m_loopPhase.load( std::memory_order_relaxed ), e.what() );
        }
        catch( ... )
        {
            wxLogTrace( traceRhi, wxS( "RenderThread: UNKNOWN EXCEPTION in main loop "
                                        "phase=%d" ),
                        m_loopPhase.load( std::memory_order_relaxed ) );
        }
    }

    wxLogTrace( traceRhi, wxS( "RenderThread: shutting down (windows=%zu)" ), m_windows.size() );

    {
        std::lock_guard<std::mutex> lock( m_workMutex );
        m_acceptingWork = false;
    }

    DrainControlQueue();
    DrainUploadQueue();

    DestroyAllWindows();
    DestroyRenderInfrastructure();

    {
        std::lock_guard<std::mutex> lock( m_deviceMutex );
        m_device.reset();
    }

    m_running.store( false, std::memory_order_release );

    wxLogTrace( traceRhi, wxS( "RenderThread: shutdown complete" ) );
}


void RENDER_THREAD::DrainControlQueue()
{
    RHI_CONTROL_COMMAND cmd;

    while( m_controlQueue.pop( cmd ) )
    {
        std::visit(
                [this]( auto&& arg )
                {
                    using T = std::decay_t<decltype( arg )>;

                    if constexpr( std::is_same_v<T, RHI_CMD_FRAME_REQUEST> )
                    {
                        if( !m_shutdownRequested.load( std::memory_order_acquire ) )
                            HandleFrameRequest( arg );
                    }
                    else if constexpr( std::is_same_v<T, RHI_CMD_SWAPCHAIN_CREATE> )
                    {
                        // A lost device makes nothing new, see OnDeviceLost()
                        if( !IsDeviceLost() )
                            HandleSwapchainCreate( arg );
                    }
                    else if constexpr( std::is_same_v<T, RHI_CMD_SWAPCHAIN_RESIZE> )
                    {
                        if( !IsDeviceLost() )
                            HandleSwapchainResize( arg );
                    }
                    else if constexpr( std::is_same_v<T, RHI_CMD_SWAPCHAIN_DESTROY> )
                    {
                        HandleSwapchainDestroy( arg );
                    }
                    else if constexpr( std::is_same_v<T, RHI_CMD_FENCE> )
                    {
                        HandleFence( arg );
                    }
                },
                cmd );
    }
}


void RENDER_THREAD::DrainUploadQueue()
{
    m_uploadsPending.store( false, std::memory_order_release );

    // Uploads to a lost device have nowhere to go, and the queue must not grow meanwhile
    if( !m_device || IsDeviceLost() )
    {
        m_uploadQueue.clear();
        return;
    }

    m_device->BeginUploadBatch();

    RHI_UPLOAD_COMMAND cmd;

    while( m_uploadQueue.pop( cmd ) )
    {
        std::visit(
                [&]( auto&& arg )
                {
                    using T = std::decay_t<decltype( arg )>;

                    if constexpr( std::is_same_v<T, RHI_GEOMETRY_UPLOAD> )
                    {
                        if( arg.targetBuffer && arg.HasStaging() )
                        {
                            m_device->UpdateBuffer( arg.targetBuffer, arg.stagingData,
                                                    arg.stagingSize, arg.targetOffset );
                        }
                    }
                    else if constexpr( std::is_same_v<T, RHI_TEXTURE_UPLOAD> )
                    {
                        if( arg.targetTexture && arg.HasStaging() )
                            m_device->UploadTexture( arg.targetTexture, arg.stagingData, arg.stagingSize );
                    }
                    else if constexpr( std::is_same_v<T, RHI_BUFFER_CREATE> )
                    {
                        // Realized before later uploads and draws use it. A failure frees the slot
                        // and leaves the handle invalid.
                        if( arg.handle )
                            m_device->RealizeBuffer( arg.handle, arg.desc );
                    }
                    else if constexpr( std::is_same_v<T, RHI_TEXTURE_CREATE> )
                    {
                        if( arg.handle )
                            m_device->RealizeTexture( arg.handle, arg.desc );
                    }
                    else if constexpr( std::is_same_v<T, RHI_BUFFER_DESTROY> )
                    {
                        m_device->DestroyBuffer( arg.handle );
                    }
                    else if constexpr( std::is_same_v<T, RHI_TEXTURE_DESTROY> )
                    {
                        m_device->DestroyTexture( arg.handle );
                    }
                },
                cmd );
    }

    m_device->EndUploadBatch();
}


bool RENDER_THREAD::InitRenderInfrastructure()
{
    if( !m_device )
        return false;

    m_pipelines = std::make_unique<RHI_GAL_PIPELINES>( *m_device );

    if( !m_pipelines->Initialize() )
    {
        m_pipelines.reset();
        return false;
    }

    // Two NDC triangles bound at slot 0 for all instanced geometry draws
    RHI_VERTEX unitQuad[6];

    auto setVert = []( RHI_VERTEX& v, float px, float py, float tu, float tv )
    {
        v.SetPosition( px, py );
        v.SetColor( 255, 255, 255, 255 );
        v.SetUV( tu, tv );
    };

    setVert( unitQuad[0], -1.0f, -1.0f, 0.0f, 0.0f );
    setVert( unitQuad[1],  1.0f, -1.0f, 1.0f, 0.0f );
    setVert( unitQuad[2],  1.0f,  1.0f, 1.0f, 1.0f );
    setVert( unitQuad[3], -1.0f, -1.0f, 0.0f, 0.0f );
    setVert( unitQuad[4],  1.0f,  1.0f, 1.0f, 1.0f );
    setVert( unitQuad[5], -1.0f,  1.0f, 0.0f, 1.0f );

    auto makeBuffer = [&]( RHI_BUFFER_TYPE aType, const void* aData, size_t aSize )
    {
        RHI_BUFFER_DESC desc;
        desc.type      = aType;
        desc.sizeBytes = aSize;

        RHI_BufferHandle buffer = m_device->CreateBuffer( desc );

        if( buffer )
            m_device->UpdateBuffer( buffer, aData, aSize );

        return buffer;
    };

    // The same two triangles drawn indexed, so each instance shades its 4 corners once
    const RHI_VERTEX corners[4] = { unitQuad[0], unitQuad[1], unitQuad[2], unitQuad[5] };
    const uint16_t   quadIndices[6] = { 0, 1, 2, 0, 2, 3 };

    m_unitQuadBuffer = makeBuffer( RHI_BUFFER_TYPE::VERTEX, unitQuad, sizeof( unitQuad ) );
    m_quadCornerBuffer = makeBuffer( RHI_BUFFER_TYPE::VERTEX, corners, sizeof( corners ) );
    m_quadIndexBuffer = makeBuffer( RHI_BUFFER_TYPE::INDEX, quadIndices, sizeof( quadIndices ) );

    RHI_RENDER_PASS_DESC rpDesc;
    RHI_COLOR_ATTACHMENT_DESC colorAttach;
    colorAttach.clearColor[3] = 0.0f;
    rpDesc.colorAttachments.push_back( colorAttach );
    rpDesc.depthStencilAttachment.enabled     = true;
    rpDesc.depthStencilAttachment.depthLoadOp = RHI_LOAD_OP::CLEAR;
    rpDesc.depthStencilAttachment.depthStoreOp = RHI_STORE_OP::STORE;
    rpDesc.depthStencilAttachment.stencilLoadOp = RHI_LOAD_OP::DONT_CARE;
    m_geometryRenderPass = m_device->CreateRenderPass( rpDesc );

    // Resumes geometry after inline compositing, keeping color and depth
    RHI_RENDER_PASS_DESC loadRpDesc = rpDesc;
    loadRpDesc.colorAttachments[0].loadOp = RHI_LOAD_OP::LOAD;
    loadRpDesc.depthStencilAttachment.depthLoadOp = RHI_LOAD_OP::LOAD;
    m_geometryLoadRenderPass = m_device->CreateRenderPass( loadRpDesc );

    RHI_RENDER_PASS_DESC depthLoadRpDesc = rpDesc;
    depthLoadRpDesc.depthStencilAttachment.depthLoadOp = RHI_LOAD_OP::LOAD;
    m_geometryDepthLoadRenderPass = m_device->CreateRenderPass( depthLoadRpDesc );

    RHI_RENDER_PASS_DESC discardRpDesc = rpDesc;
    discardRpDesc.depthStencilAttachment.depthStoreOp = RHI_STORE_OP::DONT_CARE;
    m_geometryDepthDiscardRenderPass = m_device->CreateRenderPass( discardRpDesc );

    discardRpDesc.depthStencilAttachment.depthLoadOp = RHI_LOAD_OP::LOAD;
    m_geometryDepthLoadDiscardRenderPass = m_device->CreateRenderPass( discardRpDesc );

    // The other passes and buffers have fallbacks, these do not
    if( !m_unitQuadBuffer || !m_geometryRenderPass || !m_geometryLoadRenderPass )
        return false;

    // Shared by all windows, each compositing into its own accumulation target
    m_compositor = std::make_unique<RHI_COMPOSITOR>( *m_device );

    if( !m_compositor->Initialize( m_pipelines->GetTextureSamplerLayout() ) )
        return false;

    m_compositor->SetCompositorPipeline( m_pipelines->GetCompositorPipeline() );

    return true;
}


void RENDER_THREAD::DestroyRenderInfrastructure()
{
    m_compositor.reset();

    if( m_device )
    {
        for( RHI_RenderPassHandle* pass : { &m_geometryRenderPass, &m_geometryLoadRenderPass,
                                            &m_geometryDepthDiscardRenderPass, &m_geometryDepthLoadDiscardRenderPass,
                                            &m_geometryDepthLoadRenderPass } )
        {
            m_device->Release( *pass );
        }

        for( RHI_BufferHandle* buffer : { &m_unitQuadBuffer, &m_quadCornerBuffer, &m_quadIndexBuffer } )
            m_device->Release( *buffer );
    }

    destroyGlyphResources();
    m_pipelines.reset();
}


void RENDER_THREAD::destroyGlyphResources()
{
    if( m_device )
    {
        m_device->Release( m_glyphBG );
        m_device->Release( m_glyphTexture );
        m_device->Release( m_glyphTable );
    }

    m_glyphBG = {};
    m_glyphTexture = {};
    m_glyphTable = {};
    m_glyphBGHasAtlas = false;
}


void RENDER_THREAD::SetGlyphAtlas( std::shared_ptr<const RHI_GLYPH_ATLAS_DATA> aAtlas )
{
    std::lock_guard<std::mutex> lock( m_glyphAtlasMutex );

    if( !m_glyphAtlas )
        m_glyphAtlas = std::move( aAtlas );
}


RHI_BindGroupHandle RENDER_THREAD::glyphBindGroup()
{
    std::shared_ptr<const RHI_GLYPH_ATLAS_DATA> atlas;

    {
        std::lock_guard<std::mutex> lock( m_glyphAtlasMutex );
        atlas = m_glyphAtlas;
    }

    // Every glyph pipeline needs the set, so it holds a blank atlas until a canvas supplies the font
    if( m_glyphBG && ( m_glyphBGHasAtlas || !atlas ) )
        return m_glyphBG;

    destroyGlyphResources();

    RHI_TEXTURE_DESC texDesc;
    texDesc.width = atlas ? atlas->width : 1;
    texDesc.height = atlas ? atlas->height : 1;
    texDesc.format = RHI_TEXTURE_FORMAT::RGBA8;
    texDesc.filter = RHI_FILTER::BILINEAR;
    m_glyphTexture = m_device->CreateTexture( texDesc );

    const std::vector<uint8_t> blankTexel( 4, 0 );
    const std::vector<float>   blankTable( ( RHI_GLYPH_TABLE_SIZE + 1 ) * 4, 0.0f );

    RHI_BUFFER_DESC bufDesc;
    bufDesc.type = RHI_BUFFER_TYPE::UNIFORM;
    bufDesc.sizeBytes = blankTable.size() * sizeof( float );
    bufDesc.mappable = true;
    m_glyphTable = m_device->CreateBuffer( bufDesc );

    if( !m_glyphTexture || !m_glyphTable )
    {
        destroyGlyphResources();
        return {};
    }

    const std::vector<uint8_t>& texels = atlas ? atlas->rgba : blankTexel;
    const std::vector<float>&   table = atlas ? atlas->table : blankTable;

    m_device->UploadTexture( m_glyphTexture, texels.data(), texels.size() );
    m_device->UpdateBuffer( m_glyphTable, table.data(), std::min( table.size(), blankTable.size() ) * sizeof( float ) );

    RHI_BIND_GROUP_DESC bgDesc;
    bgDesc.layout = m_pipelines->GetGlyphLayout();
    bgDesc.entries.push_back( { 0, {}, m_glyphTexture, 0, 0 } );
    bgDesc.entries.push_back(
            { RHI_GLYPH_TABLE_BINDING, m_glyphTable, {}, 0, static_cast<uint32_t>( bufDesc.sizeBytes ) } );
    m_glyphBG = m_device->CreateBindGroup( bgDesc );
    m_glyphBGHasAtlas = static_cast<bool>( atlas );

    return m_glyphBG;
}


void RENDER_THREAD::HandleFrameRequest( const RHI_CMD_FRAME_REQUEST& aCmd )
{
    auto it = m_windows.find( aCmd.windowId );

    if( it == m_windows.end() )
    {
        wxLogTrace( traceRhi, wxS( "RenderThread: FrameRequest windowId=%u UNKNOWN" ),
                    aCmd.windowId );
        return;
    }

    WINDOW_STATE& ws = it->second;
    ws.requestedGeneration = aCmd.generation;

    ws.refreshHz = aCmd.refreshHz;
    ws.active = true;
    ws.frameResultPending = true;
    ws.nextPresentRetryAt = {};
    ws.lastActivityTime = WINDOW_STATE::Clock::now();
    ws.lastFrameRequestTime = ws.lastActivityTime;
}


void RENDER_THREAD::ProcessActiveWindows( bool aIgnorePacing )
{
    m_activeWindowOrder.clear();
    m_deferredFrameDue.reset();

    if( IsDeviceLost() )
        return;

    for( auto& [id, ws] : m_windows )
    {
        const bool swapchainMissing = !ws.swapchain;

        if( swapchainMissing )
            EnsureSwapchain( ws, "idle" );

        if( swapchainMissing && ws.swapchain && ( ws.frameResultPending || ws.presentSuccessCount > 0 ) )
            ws.active = true;

        if( ws.swapchain )
        {
            switch( m_device->PollSurfaceExtent( ws.swapchain ) )
            {
            case RHI_SURFACE_EXTENT_POLL_RESULT::WAITING:
            {
                auto due = WINDOW_STATE::Clock::now() + std::chrono::milliseconds( 50 );
                deferFrameUntil( due );
                continue;
            }

            case RHI_SURFACE_EXTENT_POLL_RESULT::RESIZED:
                if( ws.frameResultPending || ws.presentSuccessCount > 0 )
                {
                    ws.active = true;
                    ws.nextPresentRetryAt = {};
                }
                break;

            case RHI_SURFACE_EXTENT_POLL_RESULT::FAILED:
                recreateSwapchainAfterOutOfDate( ws );
                ws.active = ws.frameResultPending || ws.presentSuccessCount > 0;

                if( ws.active )
                {
                    auto due = WINDOW_STATE::Clock::now() + std::chrono::milliseconds( 50 );
                    deferFrameUntil( due );
                }

                continue;

            case RHI_SURFACE_EXTENT_POLL_RESULT::IDLE:
                break;
            }
        }

        if( !ws.swapchain && ( ws.frameResultPending || ws.presentSuccessCount > 0 ) )
        {
            auto due = WINDOW_STATE::Clock::now() + std::chrono::milliseconds( 50 );
            deferFrameUntil( due );
            continue;
        }

        if( ws.nextPresentRetryAt.time_since_epoch().count() != 0 )
        {
            if( WINDOW_STATE::Clock::now() < ws.nextPresentRetryAt )
            {
                deferFrameUntil( ws.nextPresentRetryAt );
                continue;
            }

            ws.nextPresentRetryAt = {};
            ws.active = ws.frameResultPending;
        }

        if( ws.active )
            m_activeWindowOrder.push_back( &ws );
    }

    if( m_activeWindowOrder.empty() )
        return;

    std::sort( m_activeWindowOrder.begin(), m_activeWindowOrder.end(),
               []( const WINDOW_STATE* a, const WINDOW_STATE* b )
               {
                   return a->lastActivityTime > b->lastActivityTime;
               } );

    for( WINDOW_STATE* ws : m_activeWindowOrder )
    {
        // An earlier window may have just lost the device
        if( IsDeviceLost() )
            return;

        FRAME_MAILBOX* frameMailbox = ws->mailbox;

        // A present that does not wait for the display would draw unseen frames, so such a window is held to one
        // per refresh. New input is never held back, and a MAILBOX swapchain drops unseen frames itself.
        const bool newInput = frameMailbox && frameMailbox->PendingInputSerial() > ws->drawnInputSerial;
        const bool capped = ws->swapchain && ws->refreshHz > 0.0f
                            && !m_device->PresentWaitsForDisplay( ws->swapchain )
                            && !m_device->PresentReplacesQueued( ws->swapchain );
        const WINDOW_STATE::Clock::time_point frameStart = WINDOW_STATE::Clock::now();

        if( capped && !newInput && !aIgnorePacing && frameStart < ws->nextFrameAt )
        {
            deferFrameUntil( ws->nextFrameAt );
            continue;
        }

        if( capped )
        {
            ws->nextFrameAt = RhiNextFrameAt( frameStart, static_cast<double>( ws->refreshHz ),
                                              m_device->LastDisplayRefresh( ws->swapchain ) );
        }

        if( frameMailbox )
        {
            // Waiting for the previous present first lets this frame carry the newest input. A frame that already
            // has new input goes out with one more present in flight, which the swapchain can still hold.
            if( ws->swapchain )
            {
                const int allowed = RHI_PRESENTS_IN_FLIGHT + ( newInput ? 1 : 0 );

                m_device->WaitForPresentSlot( ws->swapchain, std::chrono::milliseconds( 50 ), allowed );
            }

            if( std::unique_ptr<FRAME_PACKET> packet = frameMailbox->Load() )
            {
                ws->drawDesc        = std::move( packet->drawDesc );
                ws->staticDrawDesc  = std::move( packet->staticDrawDesc );
                ws->dynamicDrawDesc = std::move( packet->dynamicDrawDesc );
                ws->staticChanged   = packet->staticChanged;
                ws->dynamicChanged  = packet->dynamicChanged;
                ws->staticEpoch     = packet->staticEpoch;
                ws->dynamicEpoch    = packet->dynamicEpoch;
                ws->drawnInputSerial = packet->inputSerial;

                // Supersampling resizes the layer targets but not the swapchain. The fit keeps a bad
                // factor from requesting a huge allocation.
                uint32_t factor = fitSupersampleFactor( ws->drawDesc.supersampleFactor, *ws,
                                                        m_device->MaxTextureSize() );

                if( factor != ws->supersampleFactor )
                {
                    ws->supersampleFactor = factor;
                    DestroyWindowFBOs( *ws );
                    CreateWindowFBOs( *ws );

                    // The new static target is empty
                    ws->needsStaticInvalidate = true;
                }

                // A producer that fills only drawDesc needs its frame state copied to the split descs. One
                // that numbers its content sets each target's camera itself, since the two can scale depth differently.
                const bool ownCameras = packet->staticEpoch != 0;

                auto propagateFrameState = [&]( FRAME_DRAW_DESC& aDst )
                {
                    if( !ownCameras )
                    {
                        aDst.frameUBO = ws->drawDesc.frameUBO;
                        aDst.gridUBO  = ws->drawDesc.gridUBO;
                        aDst.drawGrid = aDst.drawGrid || ws->drawDesc.drawGrid;
                    }

                    aDst.cursorUBO       = ws->drawDesc.cursorUBO;
                    aDst.drawCursor      = aDst.drawCursor || ws->drawDesc.drawCursor;
                    aDst.linePipelineKind = ws->drawDesc.linePipelineKind;

                    for( int c = 0; c < 4; ++c )
                        aDst.backgroundColor[c] = ws->drawDesc.backgroundColor[c];
                };

                propagateFrameState( ws->staticDrawDesc );
                propagateFrameState( ws->dynamicDrawDesc );
            }
        }

        WINDOW_STATE::Clock::time_point now = WINDOW_STATE::Clock::now();
        uint64_t pendingGenerations = ws->requestedGeneration >= ws->lastRenderedGeneration
                                              ? ws->requestedGeneration - ws->lastRenderedGeneration
                                              : 0;

        if( pendingGenerations >= 60
            && WindowStateAgeMs( ws->lastRenderCompleteTime, now ) >= 1000
            && ( ws->lastLagLogTime.time_since_epoch().count() == 0
                 || WindowStateAgeMs( ws->lastLagLogTime, now ) >= 1000 ) )
        {
            wxLogTrace( traceRhi, wxS( "RenderThread: window backlog windowId=%u pending=%llu "
                                        "reqGen=%llu rendGen=%llu reqAge=%lldms "
                                        "renderAge=%lldms phase=%d" ),
                        ws->windowId,
                        static_cast<unsigned long long>( pendingGenerations ),
                        static_cast<unsigned long long>( ws->requestedGeneration ),
                        static_cast<unsigned long long>( ws->lastRenderedGeneration ),
                        WindowStateAgeMs( ws->lastFrameRequestTime, now ),
                        WindowStateAgeMs( ws->lastRenderCompleteTime, now ),
                        m_loopPhase.load( std::memory_order_relaxed ) );
            ws->lastLagLogTime = now;
        }

        const uint64_t attemptedGeneration   = ws->requestedGeneration;
        const uint64_t presentFailuresBefore = ws->presentFailureCount;
        const bool     presented             = RenderWindow( *ws );

        if( presented )
        {
            ws->consecutivePresentFailures = 0;
            ws->nextPresentRetryAt = {};
        }
        else if( ws->frameResultPending && ws->swapchain
                 && ws->presentFailureCount > presentFailuresBefore )
        {
            ws->consecutivePresentFailures = std::min( ws->consecutivePresentFailures + 1, 5u );
            auto delay = std::chrono::milliseconds( 50u << ( ws->consecutivePresentFailures - 1 ) );
            ws->nextPresentRetryAt = WINDOW_STATE::Clock::now() + delay;
            deferFrameUntil( ws->nextPresentRetryAt );
            ws->active = true;
        }

        // A skipped or failed present must not acknowledge the request
        if( presented && ws->frameResultPending )
        {
            ws->lastRenderedGeneration = attemptedGeneration;
            ws->frameResultPending = false;
        }

        ws->active = !presented && ws->nextPresentRetryAt.time_since_epoch().count() != 0;
        ws->framesProcessed++;
    }
}


bool RENDER_THREAD::RenderWindow( WINDOW_STATE& aWS )
{
    if( !m_device || IsDeviceLost() )
        return false;

    aWS.lastRenderStartTime = WINDOW_STATE::Clock::now();
    const auto renderStart = aWS.lastRenderStartTime;

    // Read only by wxLogTrace, which release builds compile out
    [[maybe_unused]] double       geometryMs = 0.0;
    [[maybe_unused]] double       overlayMs = 0.0;
    [[maybe_unused]] double       compositeMs = 0.0;
    [[maybe_unused]] double       presentMs = 0.0;
    [[maybe_unused]] const double queueWaitMs = WindowStateDurationMs( aWS.lastFrameRequestTime, renderStart );

    if( aWS.backingWidth == 0 || aWS.backingHeight == 0 )
    {
        wxLogTrace( traceRhi, wxS( "RenderThread: RenderWindow windowId=%u SKIP "
                                    "(backing %ux%u)" ),
                    aWS.windowId, aWS.backingWidth, aWS.backingHeight );
        return false;
    }

    if( !aWS.swapchain )
        EnsureSwapchain( aWS, "render" );

    if( !m_pipelines || !m_pipelines->IsInitialized() )
        return false;

    aWS.uniformGeneration = ( aWS.uniformGeneration + 1 ) % WINDOW_STATE::UNIFORM_GENERATIONS;

    if( m_geometryRenderPass )
    {
        m_loopPhase.store( 4, std::memory_order_relaxed );
        PROF_TIMER geometryTimer;

        // The FBOs behind both targets were recreated or lost
        if( aWS.needsStaticInvalidate )
        {
            aWS.renderedStatic.valid = false;
            aWS.renderedDynamic.valid = false;
            aWS.renderedOverlay.valid = false;
        }

        auto holds = []( const WINDOW_STATE::RENDERED_CONTENT& aRendered, uint64_t aEpoch,
                         const FRAME_DRAW_DESC& aDesc )
        {
            return aRendered.valid && aRendered.epoch == aEpoch && aRendered.drawGrid == aDesc.drawGrid
                   && aRendered.frameUBO == aDesc.frameUBO && ( !aDesc.drawGrid || aRendered.gridUBO == aDesc.gridUBO );
        };

        auto record = []( WINDOW_STATE::RENDERED_CONTENT& aRendered, uint64_t aEpoch,
                          const FRAME_DRAW_DESC& aDesc )
        {
            aRendered.valid    = true;
            aRendered.epoch    = aEpoch;
            aRendered.drawGrid = aDesc.drawGrid;
            aRendered.frameUBO = aDesc.frameUBO;
            aRendered.gridUBO  = aDesc.gridUBO;
        };

        // A producer that numbers its content is judged on what the FBO holds, since the packet
        // that carried a change flag may have been displaced before it was read
        bool renderStatic = aWS.staticFBO
                            && ( aWS.staticEpoch != 0
                                         ? !holds( aWS.renderedStatic, aWS.staticEpoch, aWS.staticDrawDesc )
                                         : ( !aWS.staticDrawDesc.reuseStaticFBO
                                             && ( aWS.staticChanged || aWS.needsStaticInvalidate ) ) );

        // The target holds the current view exactly
        if( !renderStatic )
            aWS.staticShift = {};

        // A static target with a margin shows a pan within it shifted instead of drawing again
        uint32_t staticDrawMargin = 0;

        if( renderStatic && aWS.staticEpoch != 0 && aWS.staticMargin > 0 )
        {
            const MARGIN_PLAN plan =
                    planMarginTarget( aWS.renderedStatic, aWS.staticEpoch, aWS.staticDrawDesc, aWS.staticMargin, true );

            if( plan.reuse )
            {
                aWS.staticShift = plan.shift;
                renderStatic = false;
            }

            staticDrawMargin = plan.drawMargin;
        }

        bool renderDynamic = aWS.dynamicFBO
                             && ( aWS.dynamicEpoch != 0
                                          ? !holds( aWS.renderedDynamic, aWS.dynamicEpoch, aWS.dynamicDrawDesc )
                                          : ( !aWS.dynamicDrawDesc.reuseDynamicFBO && aWS.dynamicChanged ) );

        // Cleared only after the decision above so an invalidation queued by HandleSwapchainResize is not lost
        aWS.needsStaticInvalidate = false;

        if( renderStatic )
        {
            const bool viewOnly = aWS.staticEpoch != 0 && aWS.staticMargin > 0 && staticDrawMargin == 0;

            if( viewOnly && !aWS.staticViewFBO )
                aWS.staticViewFBO = m_device->CreateFramebuffer( layerFramebufferDesc( aWS ) );

            aWS.staticInMargin = !( viewOnly && aWS.staticViewFBO );

            RenderGeometryInto( aWS, aWS.staticDrawDesc, aWS.ShownStaticFBO(), true, GEOMETRY_RANGES::ALL,
                                staticDrawMargin );
            record( aWS.renderedStatic, aWS.staticEpoch, aWS.staticDrawDesc );
            aWS.renderedStatic.margin = staticDrawMargin;
            aWS.staticShift = {};
            aWS.staticPasses++;
        }

        if( renderDynamic )
        {
            const FRAME_DRAW_DESC& dynamic = aWS.dynamicDrawDesc;

            // A pan with nothing selected usually leaves the dynamic target empty. Skipping it saves
            // a full-size depth copy, a pass and a layer of the composite on every frame.
            aWS.dynamicGeometryDrawn =
                    dynamic.drawGrid || ( dynamic.layerRanges.empty() && dynamic.HasGeometry() )
                    || std::any_of( dynamic.layerRanges.begin(), dynamic.layerRanges.end(),
                                    []( const LAYER_RANGE& aRange )
                                    {
                                        return aRange.target != TARGET_OVERLAY && aRange.HasGeometry();
                                    } );

            if( aWS.dynamicGeometryDrawn )
                RenderGeometryInto( aWS, dynamic, aWS.dynamicFBO, false, GEOMETRY_RANGES::NOT_OVERLAY );

            // An overlay that cannot reach the view or its margin, as origin markers zoomed away from,
            // takes no pass and no composite
            const RHI_CULL_VIEW reach = RhiCullView(
                    aWS.staticMargin > 0 ? RhiWidenFrame( dynamic.frameUBO, aWS.staticMargin ) : dynamic.frameUBO );

            const bool hasOverlay = std::any_of( dynamic.layerRanges.begin(), dynamic.layerRanges.end(),
                                                 [&]( const LAYER_RANGE& aRange )
                                                 {
                                                     return aRange.target == TARGET_OVERLAY && aRange.HasGeometry()
                                                            && RhiRangeReaches( aRange, reach );
                                                 } );

            if( !hasOverlay )
                aWS.renderedOverlay.valid = false;

            if( hasOverlay && !aWS.overlayGeometryFBO )
                aWS.overlayGeometryFBO = m_device->CreateFramebuffer( layerFramebufferDesc( aWS ) );

            aWS.overlayGeometryDrawn = hasOverlay && aWS.overlayGeometryFBO;

            // The overlay keeps a margin too, so a pan with everything selected shows it shifted
            // instead of drawing the whole selection again
            MARGIN_PLAN overlayPlan;

            if( aWS.overlayGeometryDrawn && aWS.dynamicEpoch != 0 && aWS.staticMargin > 0 )
            {
                overlayPlan = planMarginTarget( aWS.renderedOverlay, aWS.dynamicEpoch, aWS.dynamicDrawDesc,
                                                aWS.staticMargin, false );
            }

            if( overlayPlan.drawMargin > 0 && !aWS.overlayMarginFBO )
                aWS.overlayMarginFBO = m_device->CreateFramebuffer( marginFramebufferDesc( aWS ) );

            if( aWS.overlayGeometryDrawn && overlayPlan.reuse )
            {
                aWS.overlayShift = overlayPlan.shift;
            }
            else if( aWS.overlayGeometryDrawn )
            {
                aWS.overlayInMargin = overlayPlan.drawMargin > 0 && aWS.overlayMarginFBO;

                RenderGeometryInto( aWS, aWS.dynamicDrawDesc,
                                    aWS.overlayInMargin ? aWS.overlayMarginFBO : aWS.overlayGeometryFBO, false,
                                    GEOMETRY_RANGES::OVERLAY, overlayPlan.drawMargin );
                record( aWS.renderedOverlay, aWS.dynamicEpoch, aWS.dynamicDrawDesc );
                aWS.renderedOverlay.margin = aWS.overlayInMargin ? overlayPlan.drawMargin : 0;
                aWS.overlayShift = {};
            }

            record( aWS.renderedDynamic, aWS.dynamicEpoch, aWS.dynamicDrawDesc );
            aWS.dynamicPasses++;
        }

        // A packet that fills only drawDesc is drawn into staticFBO. The change flags can be set with
        // empty split descs, so this gates on real geometry.
        const bool splitDescsEmpty = !aWS.staticDrawDesc.HasGeometry()
                                     && !aWS.dynamicDrawDesc.HasGeometry();

        if( splitDescsEmpty && aWS.staticFBO
            && !aWS.drawDesc.reuseStaticFBO
            && aWS.drawDesc.HasGeometry() )
        {
            RenderGeometry( aWS );
        }

        auto hasDifference = []( const FRAME_DRAW_DESC& aDesc )
        {
            return std::any_of( aDesc.layerRanges.begin(), aDesc.layerRanges.end(),
                                []( const LAYER_RANGE& aRange )
                                {
                                    return aRange.difference;
                                } );
        };

        // Diff layers come with gerbview's XOR mode, and the copies made for them go when it ends
        if( ( aWS.diffBelowFBO || aWS.staticDiffBelowFBO ) && !hasDifference( aWS.staticDrawDesc )
            && !hasDifference( aWS.dynamicDrawDesc ) )
        {
            destroyDiffCopies( *m_device, aWS );
        }

        geometryMs = geometryTimer.msecs();
    }

    // The cursor is drawn last in the composite, not into a target of its own and composited
    PROF_TIMER overlayTimer;
    const bool cursor = prepareCursor( aWS );
    overlayMs = overlayTimer.msecs();

    RHI_FramebufferHandle compositorOutput;

    if( m_compositor && m_compositor->IsInitialized()
        && aWS.accumulationFBO )
    {
        std::vector<COMPOSITOR_LAYER> layers;

        if( aWS.staticFBO && aWS.staticInMargin )
            layers.push_back( marginLayer( aWS, aWS.staticFBO, aWS.staticShift ) );
        else if( aWS.staticFBO )
            layers.push_back( { aWS.ShownStaticFBO(), 1.0f } );

        if( aWS.dynamicFBO && aWS.dynamicGeometryDrawn )
            layers.push_back( { aWS.dynamicFBO, 1.0f } );

        if( aWS.overlayGeometryDrawn && aWS.overlayInMargin )
            layers.push_back( marginLayer( aWS, aWS.overlayMarginFBO, aWS.overlayShift ) );
        else if( aWS.overlayGeometryDrawn )
            layers.push_back( { aWS.overlayGeometryFBO, 1.0f } );

        m_loopPhase.store( 5, std::memory_order_relaxed );
        PROF_TIMER compositeTimer;
        compositorOutput = m_compositor->Composite(
                aWS.accumulationFBO, aWS.RenderWidth(), aWS.RenderHeight(), layers, aWS.drawDesc.backgroundColor,
                [&]()
                {
                    if( cursor )
                        drawCursor( aWS );
                } );
        compositeMs = compositeTimer.msecs();
    }

    // A failed composite, as during the first resize from 1x1, presents the static target so the
    // first frame is not blank
    RHI_FramebufferHandle presentFBO = compositorOutput ? compositorOutput : aWS.ShownStaticFBO();

    // The static target holds a margin around the view, which the fallback leaves out
    RHI_SCISSOR_RECT        viewOfStatic;
    const RHI_SCISSOR_RECT* presentRect = nullptr;

    if( !compositorOutput && aWS.staticMargin > 0 && aWS.staticInMargin )
    {
        viewOfStatic.x = aWS.ShownStaticX();
        viewOfStatic.y = aWS.ShownStaticY();
        viewOfStatic.width = aWS.RenderWidth();
        viewOfStatic.height = aWS.RenderHeight();
        presentRect = &viewOfStatic;
    }

    bool presented = false;

    if( aWS.swapchain && presentFBO )
    {
        m_loopPhase.store( 6, std::memory_order_relaxed );
        aWS.lastPresentAttemptTime = WINDOW_STATE::Clock::now();
        PROF_TIMER presentTimer;
        RHI_FRAME_OP_RESULT presentResult = m_device->Present( aWS.swapchain, presentFBO, presentRect );
        presentMs = presentTimer.msecs();

        // KICAD_RHI_LOSE_DEVICE=<n> treats the process's n-th present as a lost device to exercise recovery
        static const int loseAt = []()
        {
            const char* value = std::getenv( "KICAD_RHI_LOSE_DEVICE" );
            return value ? std::atoi( value ) : 0;
        }();

        static std::atomic<int> presents{ 0 };

        if( presentResult == RHI_FRAME_OP_RESULT::OK && loseAt > 0 && ++presents == loseAt )
            presentResult = RHI_FRAME_OP_RESULT::DEVICE_LOST;

        if( presentResult == RHI_FRAME_OP_RESULT::DEVICE_LOST )
        {
            wxLogTrace( traceRhi, wxS( "RenderThread: Device lost during present windowId=%u" ), aWS.windowId );
            OnDeviceLost();
            return false;
        }
        else if( presentResult == RHI_FRAME_OP_RESULT::OUT_OF_DATE
                 || presentResult == RHI_FRAME_OP_RESULT::SURFACE_LOST )
        {
            aWS.lastPresentFailureTime = WINDOW_STATE::Clock::now();
            aWS.presentFailureCount++;
            recreateSwapchainAfterOutOfDate( aWS, presentResult == RHI_FRAME_OP_RESULT::SURFACE_LOST );
        }
        else if( presentResult != RHI_FRAME_OP_RESULT::OK )
        {
            aWS.lastPresentFailureTime = WINDOW_STATE::Clock::now();
            aWS.presentFailureCount++;
            wxLogTrace( traceRhi, wxS( "RenderThread: Present FAILED windowId=%u "
                                        "srcFBO=%s reqGen=%llu presentFail=%llu "
                                        "result=%d reqAge=%lldms renderAge=%lldms" ),
                        aWS.windowId, compositorOutput ? "valid" : "null",
                        static_cast<unsigned long long>( aWS.requestedGeneration ),
                        static_cast<unsigned long long>( aWS.presentFailureCount ),
                        static_cast<int>( presentResult ),
                        WindowStateAgeMs( aWS.lastFrameRequestTime,
                                          aWS.lastPresentFailureTime ),
                        WindowStateAgeMs( aWS.lastRenderCompleteTime,
                                          aWS.lastPresentFailureTime ) );
        }
        else
        {
            aWS.lastPresentSuccessTime = WINDOW_STATE::Clock::now();
            aWS.presentSuccessCount++;
            aWS.lastPresentedGeneration = aWS.requestedGeneration;
            presented = true;
            m_presented.store( true, std::memory_order_release );
        }
    }

    aWS.lastRenderCompleteTime = WINDOW_STATE::Clock::now();

    wxLogTrace( traceRhi, wxS( "Perf: RenderWindow windowId=%u gen=%llu total=%.3f "
                                "queue=%.3f geometry=%.3f overlay=%.3f composite=%.3f "
                                "present=%.3f reuseStatic=%d tracks=%u arcs=%u "
                                "pads=%u polys=%u bitmaps=%llu layers=%llu passes=%llu" ),
                aWS.windowId,
                static_cast<unsigned long long>( aWS.requestedGeneration ),
                WindowStateDurationMs( renderStart, aWS.lastRenderCompleteTime ),
                queueWaitMs, geometryMs, overlayMs, compositeMs, presentMs,
                aWS.drawDesc.reuseStaticFBO ? 1 : 0,
                aWS.drawDesc.count[RHI_SHAPE_TRACK] + aWS.drawDesc.negCount[RHI_SHAPE_TRACK],
                aWS.drawDesc.count[RHI_SHAPE_ARC] + aWS.drawDesc.negCount[RHI_SHAPE_ARC],
                aWS.drawDesc.count[RHI_SHAPE_PAD] + aWS.drawDesc.negCount[RHI_SHAPE_PAD],
                aWS.drawDesc.count[RHI_SHAPE_POLYGON],
                static_cast<unsigned long long>( aWS.drawDesc.bitmapDraws.size() ),
                static_cast<unsigned long long>( aWS.drawDesc.layerRanges.size() ),
                static_cast<unsigned long long>( aWS.geometryRenderPasses ) );

    return presented;
}


void RENDER_THREAD::RenderGeometry( WINDOW_STATE& aWS )
{
    // For a packet whose geometry is only in drawDesc
    aWS.staticInMargin = true;
    RenderGeometryInto( aWS, aWS.drawDesc, aWS.staticFBO, true );
}


void RENDER_THREAD::RenderGeometryInto( WINDOW_STATE& aWS, const FRAME_DRAW_DESC& dd, RHI_FramebufferHandle aTargetFBO,
                                        bool aLogSummary, GEOMETRY_RANGES aRanges, uint32_t aStaticDrawMargin )
{
    if( !aTargetFBO )
        return;

    const bool overlayPass = aRanges == GEOMETRY_RANGES::OVERLAY;

    auto drawsRange = [&]( const LAYER_RANGE& aRange )
    {
        return aRanges == GEOMETRY_RANGES::ALL || overlayPass == ( aRange.target == TARGET_OVERLAY );
    };

    // Set while a range whose elements are left premultiplied is drawn
    bool premultipliedRange = false;

    // Overlay content is blended the way the OpenGL canvas blends its overlay
    auto bindPipeline = [&]( RHI_PipelineHandle aPipeline )
    {
        if( overlayPass )
        {
            RHI_PipelineHandle premultiplied = m_pipelines->GetPremultipliedVariant( aPipeline );

            aPipeline = premultiplied != aPipeline ? premultiplied : m_pipelines->GetOverlayVariant( aPipeline );
        }
        else if( premultipliedRange )
            aPipeline = m_pipelines->GetPremultipliedVariant( aPipeline );

        m_device->BindPipeline( aPipeline );

        if( m_pipelines->UsesGlyphAtlas( aPipeline ) )
            m_device->BindGroup( 1, glyphBindGroup() );
    };

    if( aLogSummary && aWS.framesProcessed < 5 )
    {
        wxLogTrace( traceRhi, wxS( "RenderThread: RenderGeometry windowId=%u "
                                    "tracks=%u/%u arcs=%u/%u pads=%u/%u polys=%u bitmaps=%zu "
                                    "grid=%d cursor=%d layers=%zu" ),
                    aWS.windowId,
                    dd.count[RHI_SHAPE_TRACK], dd.negCount[RHI_SHAPE_TRACK],
                    dd.count[RHI_SHAPE_ARC], dd.negCount[RHI_SHAPE_ARC],
                    dd.count[RHI_SHAPE_PAD], dd.negCount[RHI_SHAPE_PAD],
                    dd.count[RHI_SHAPE_POLYGON],
                    dd.bitmapDraws.size(),
                    dd.drawGrid ? 1 : 0,
                    dd.drawCursor ? 1 : 0,
                    dd.layerRanges.size() );
    }

    // The static and overlay targets are larger than the view by a margin, see WINDOW_STATE::staticMargin.
    // Without the margin drawn, the view goes in its middle.
    const bool     staticTarget = ( aTargetFBO == aWS.staticFBO || aTargetFBO == aWS.overlayMarginFBO )
                                  && aWS.staticMargin > 0;
    const bool     drawsMargin = staticTarget && aStaticDrawMargin > 0;
    const uint32_t targetWidth = staticTarget ? aWS.StaticWidth() : aWS.RenderWidth();
    const uint32_t targetHeight = staticTarget ? aWS.StaticHeight() : aWS.RenderHeight();
    const uint32_t inset = staticTarget && !drawsMargin ? aWS.staticMargin * aWS.supersampleFactor : 0;

    RHI_VIEWPORT viewport;
    viewport.x      = static_cast<float>( inset );
    viewport.y      = static_cast<float>( inset );
    viewport.width  = static_cast<float>( drawsMargin ? targetWidth : aWS.RenderWidth() );
    viewport.height = static_cast<float>( drawsMargin ? targetHeight : aWS.RenderHeight() );

    RHI_SCISSOR_RECT scissor;
    scissor.x      = static_cast<int32_t>( inset );
    scissor.y      = static_cast<int32_t>( inset );
    scissor.width  = static_cast<uint32_t>( viewport.width );
    scissor.height = static_cast<uint32_t>( viewport.height );

    const FRAME_UBO_DATA frameData =
            RhiSupersampleFrame( drawsMargin ? RhiWidenFrame( dd.frameUBO, aStaticDrawMargin ) : dd.frameUBO,
                                 aWS.supersampleFactor );

    const RHI_CULL_VIEW cullView = RhiCullView( frameData );
    const GRID_UBO_DATA  gridData =
            RhiSupersampleGrid( drawsMargin ? RhiWidenGrid( dd.gridUBO, aStaticDrawMargin ) : dd.gridUBO,
                                aWS.supersampleFactor );

    // A temp target is composited texel for texel, so the static target needs one of its own size.
    // Made before any pass begins, as creating a target can submit work of its own.
    if( staticTarget && !aWS.staticTempFBO
        && std::any_of( dd.layerRanges.begin(), dd.layerRanges.end(),
                        []( const LAYER_RANGE& aRange )
                        {
                            return aRange.HasGeometry() && aRange.NeedsTempFBO();
                        } ) )
    {
        RHI_FRAMEBUFFER_DESC tempDesc = layerFramebufferDesc( aWS );
        tempDesc.width = targetWidth;
        tempDesc.height = targetHeight;
        aWS.staticTempFBO = m_device->CreateFramebuffer( tempDesc );
    }

    const RHI_FramebufferHandle tempFBO = staticTarget ? aWS.staticTempFBO : aWS.tempFBO;

    // A diff layer is differenced against a copy of what is under it, which needs a target of the
    // same size as well
    RHI_FramebufferHandle& belowFBO = staticTarget ? aWS.staticDiffBelowFBO : aWS.diffBelowFBO;

    if( !belowFBO
        && std::any_of( dd.layerRanges.begin(), dd.layerRanges.end(),
                        [&]( const LAYER_RANGE& aRange )
                        {
                            return aRange.difference && aRange.HasGeometry() && drawsRange( aRange );
                        } ) )
    {
        RHI_FRAMEBUFFER_DESC belowDesc = layerFramebufferDesc( aWS );
        belowDesc.width = targetWidth;
        belowDesc.height = targetHeight;
        belowFBO = m_device->CreateFramebuffer( belowDesc );
    }

    const bool drawsStatic = aTargetFBO == aWS.staticFBO || aTargetFBO == aWS.staticViewFBO;

    WINDOW_STATE::PASS_UNIFORMS& uniforms = overlayPass                    ? aWS.Uniforms().overlayUniforms
                                            : aTargetFBO == aWS.dynamicFBO ? aWS.Uniforms().dynamicUniforms
                                                                           : aWS.Uniforms().staticUniforms;

    const RHI_BindGroupHandle frameBG = writeUniform( uniforms.frameUBO, uniforms.frameBG,
                                                      m_pipelines->GetFrameUBOLayout(), &frameData,
                                                      sizeof( FRAME_UBO_DATA ) )
                                                ? uniforms.frameBG
                                                : RHI_BindGroupHandle{};

    // The font atlas is uploaded before a pass, as a copy cannot be recorded inside one
    glyphBindGroup();

    // Written before the first pass begins. An update between passes would have to wait for
    // the ones already submitted.
    if( dd.drawGrid )
    {
        writeUniform( uniforms.gridUBO, uniforms.gridBG, m_pipelines->GetFullscreenUBOLayout(), &gridData,
                      sizeof( GRID_UBO_DATA ) );
    }

    // Geometry shaders read their layer's dim from push constants, which hold nothing until set
    auto bindGeometry = [&]( RHI_PipelineHandle aPipeline, const RHI_LAYER_DIM& aDim )
    {
        bindPipeline( aPipeline );
        m_device->BindGroup( 0, frameBG );
        m_device->SetPushConstants( &aDim, sizeof( aDim ) );
    };

    auto drawInstanced = [&]( RHI_PipelineHandle aPipeline, RHI_BufferHandle aInstanceBuffer, uint32_t aInstanceCount,
                              uint32_t aFirstInstance = 0, const RHI_LAYER_DIM& aDim = {} )
    {
        if( !aPipeline || !aInstanceBuffer || aInstanceCount == 0 || !frameBG )
            return;

        bindGeometry( aPipeline, aDim );
        m_device->BindVertexBuffer( 1, aInstanceBuffer );

        if( m_quadCornerBuffer && m_quadIndexBuffer )
        {
            m_device->BindVertexBuffer( 0, m_quadCornerBuffer );
            m_device->BindIndexBuffer( m_quadIndexBuffer );
            m_device->DrawIndexedInstanced( 6, aInstanceCount, 0, 0, aFirstInstance );
        }
        else
        {
            m_device->BindVertexBuffer( 0, m_unitQuadBuffer );
            m_device->DrawInstanced( 6, aInstanceCount, 0, 0, aFirstInstance );
        }
    };

    auto drawBitmap = [&]( const BITMAP_DRAW_DESC& aBitmap )
    {
        if( !frameBG || !m_pipelines->GetBitmapPipeline()
            || !aBitmap.vertexBuffer || !aBitmap.texture )
        {
            return;
        }

        // The overlay pass needs the variant that also blends alpha
        bindPipeline( m_pipelines->GetBitmapPipeline() );
        m_device->BindGroup( 0, frameBG );

        RHI_BIND_GROUP_DESC texBGDesc;
        texBGDesc.layout = m_pipelines->GetTextureSamplerLayout();
        texBGDesc.entries.push_back( { 0, {}, aBitmap.texture, 0, 0 } );
        RHI_BindGroupHandle texBG = m_device->CreateBindGroup( texBGDesc );

        if( !texBG )
            return;

        m_device->BindGroup( 1, texBG );
        m_device->BindVertexBuffer( 0, aBitmap.vertexBuffer );
        m_device->Draw( aBitmap.vertexCount );
        m_device->DestroyBindGroup( texBG );
    };

    // Schematic wires are drawn as tracks by shaders of their own, where they have them
    const bool wires = dd.linePipelineKind == RHI_LINE_PIPELINE_KIND::SCH_WIRES;

    auto shapePipeline = [&]( RHI_SHAPE aShape, LAYER_DRAW_MODE aMode )
    {
        if( wires && aShape == RHI_SHAPE_TRACK && aMode != LAYER_DRAW_MODE::MARK )
        {
            return aMode == LAYER_DRAW_MODE::ONCE ? m_pipelines->GetWireOncePipeline()
                                                  : m_pipelines->GetWirePipeline();
        }

        return m_pipelines->GetShapePipeline( aShape, aMode );
    };

    auto drawGrid = [&]()
    {
        if( !dd.drawGrid || !m_pipelines->GetGridPipeline() || !uniforms.gridBG || !m_unitQuadBuffer )
            return;

        m_device->BindPipeline( m_pipelines->GetGridPipeline() );
        m_device->BindGroup( 0, uniforms.gridBG );
        m_device->BindVertexBuffer( 0, m_unitQuadBuffer );
        m_device->Draw( 6 );
    };

    auto drawLayerGeometry = [&]( const LAYER_RANGE& aRange )
    {
        premultipliedRange = aRange.overlayAlpha;

        auto pick = []( RHI_BufferHandle aOwn, RHI_BufferHandle aShared )
        {
            return aOwn ? aOwn : aShared;
        };

        // A layer that keeps relative depth is drawn into its own slice of the depth range.
        // With knockouts marked in depth the slice is halved, marks in front.
        const float mid = 0.5f * ( aRange.depthNear + aRange.depthFar );

        auto setSlice = [&]( bool aNearHalf, bool aOrdered )
        {
            RHI_VIEWPORT slice = viewport;
            slice.minDepth = ( aRange.depthKnockout && !aNearHalf ) ? mid : aRange.depthNear;
            slice.maxDepth = ( aRange.depthKnockout && aNearHalf ) ? mid : aRange.depthFar;

            // Runs are ordered by when they are drawn. Flattened to one depth, none of them can
            // lose the depth test to a rounding difference between element types.
            if( aOrdered )
                slice.minDepth = slice.maxDepth;

            m_device->SetViewport( slice );
        };

        for( const LAYER_DRAW_STEP& step : RhiPlanLayerDraws( aRange, dd.polarityRuns ) )
        {
            if( step.count == 0 )
                continue;

            // Polygons have no polarity flag, so their negative runs share one buffer with the rest
            const bool polygon = step.shape == RHI_SHAPE_POLYGON;
            const bool negatives = step.negatives && !polygon;

            const RHI_PipelineHandle pipeline = shapePipeline( step.shape, step.mode );
            const RHI_BufferHandle   own = negatives ? aRange.negBuffer[step.shape] : aRange.buffer[step.shape];
            const RHI_BufferHandle   buffer = pick( own, dd.buffer[step.shape] );

            const std::vector<RHI_RUN_BOUNDS>* runs =
                    aRange.cullRuns ? &aRange.cullRuns->Of( step.shape, negatives ) : nullptr;

            if( polygon )
            {
                if( !frameBG || !pipeline || !buffer )
                    continue;

                bindGeometry( pipeline, aRange.dim );
                m_device->BindVertexBuffer( 0, buffer );
            }

            setSlice( step.nearHalf, step.ordered );

            RhiForVisibleStretches( runs, RhiCullRunLength( step.shape ), step.start, step.count, cullView,
                                    [&]( uint32_t aFirst, uint32_t aCount )
                                    {
                                        if( polygon )
                                            m_device->Draw( aCount, aFirst );
                                        else
                                            drawInstanced( pipeline, buffer, aCount, aFirst, aRange.dim );
                                    } );
        }

        uint32_t bmpEnd = aRange.bitmapStart + aRange.bitmapCount;

        if( bmpEnd > dd.bitmapDraws.size() )
            bmpEnd = static_cast<uint32_t>( dd.bitmapDraws.size() );

        // A range holding only bitmaps has set no depth slice of its own yet
        if( aRange.bitmapStart < bmpEnd )
            setSlice( false, false );

        for( uint32_t bi = aRange.bitmapStart; bi < bmpEnd; ++bi )
            drawBitmap( dd.bitmapDraws[bi] );
    };

    auto beginPass = [&]( RHI_FramebufferHandle aFBO, RHI_RenderPassHandle aRenderPass ) -> bool
    {
        if( !aFBO || !aRenderPass )
            return false;

        RHI_RENDER_PASS_BEGIN_DESC rpBegin;
        rpBegin.renderPass  = aRenderPass;
        rpBegin.framebuffer = aFBO;

        if( !m_device->BeginRenderPass( rpBegin ) )
            return false;

        aWS.geometryRenderPasses++;

        m_device->SetViewport( viewport );
        m_device->SetScissor( scissor );
        return true;
    };

    // Covers the whole target because the temp target matches it texel for texel, whatever part of it
    // was drawn. Each texture goes to the binding of its place in the list.
    auto drawWhole = [&]( RHI_BindGroupLayoutHandle aLayout, std::initializer_list<RHI_TextureHandle> aTextures,
                          float aAlpha ) -> bool
    {
        RHI_BufferHandle quadVB = m_compositor->GetQuadVB();

        if( !quadVB )
            return false;

        RHI_BIND_GROUP_DESC bgDesc;
        bgDesc.layout = aLayout;

        for( RHI_TextureHandle tex : aTextures )
        {
            if( !tex )
                return false;

            bgDesc.entries.push_back( { static_cast<uint32_t>( bgDesc.entries.size() ), {}, tex, 0, 0 } );
        }

        RHI_BindGroupHandle bg = m_device->CreateBindGroup( bgDesc );

        if( !bg )
            return false;

        RHI_VIEWPORT whole;
        whole.width = static_cast<float>( targetWidth );
        whole.height = static_cast<float>( targetHeight );

        RHI_SCISSOR_RECT wholeScissor;
        wholeScissor.width = targetWidth;
        wholeScissor.height = targetHeight;

        COMPOSITOR_PUSH_CONSTANTS pc;
        pc.alpha = aAlpha;

        m_device->SetViewport( whole );
        m_device->SetScissor( wholeScissor );
        m_device->BindVertexBuffer( 0, quadVB );
        m_device->BindGroup( 0, bg );
        m_device->SetPushConstants( &pc, sizeof( pc ) );
        m_device->Draw( 6 );
        m_device->DestroyBindGroup( bg );
        m_device->SetViewport( viewport );
        m_device->SetScissor( scissor );
        return true;
    };

    auto compositeTemp = [&]( float aAlpha )
    {
        if( !m_compositor || !m_compositor->IsInitialized() || !m_pipelines->GetInlineCompositorPipeline() )
            return;

        bindPipeline( m_pipelines->GetInlineCompositorPipeline() );
        drawWhole( m_compositor->GetSingleTexLayout(), { m_device->GetFramebufferColorTexture( tempFBO, 0 ) }, aAlpha );
    };

    // A pass cannot sample the target it draws into, so a diff layer reads a copy of it
    auto copyBelow = [&]() -> bool
    {
        if( !belowFBO || !m_pipelines->GetDifferencePipeline()
            || !beginPass( belowFBO, m_geometryDepthDiscardRenderPass ? m_geometryDepthDiscardRenderPass
                                                                      : m_geometryRenderPass ) )
        {
            return false;
        }

        // Premultiplied over a target cleared to transparent, at alpha 1, is an exact copy
        m_device->BindPipeline( m_pipelines->GetInlineCompositorPipeline() );

        const bool copied = drawWhole( m_compositor->GetSingleTexLayout(),
                                       { m_device->GetFramebufferColorTexture( aTargetFBO, 0 ) }, 1.0f );

        m_device->EndRenderPass();
        return copied;
    };

    auto compositeDifference = [&]( float aAlpha )
    {
        m_device->BindPipeline( m_pipelines->GetDifferencePipeline() );
        drawWhole( m_pipelines->GetDifferenceLayout(),
                   { m_device->GetFramebufferColorTexture( tempFBO, 0 ),
                     m_device->GetFramebufferColorTexture( belowFBO, 0 ) },
                   aAlpha );
    };

    auto renderLegacy = [&]()
    {
        if( !beginPass( aTargetFBO, m_geometryRenderPass ) )
            return;

        for( LAYER_DRAW_MODE mode : { LAYER_DRAW_MODE::BLEND, LAYER_DRAW_MODE::NEGATIVE } )
        {
            const bool negatives = mode == LAYER_DRAW_MODE::NEGATIVE;

            for( int shape = 0; shape < RHI_INSTANCED_SHAPES; ++shape )
            {
                drawInstanced( shapePipeline( RHI_SHAPE( shape ), mode ), dd.buffer[shape],
                               negatives ? dd.negCount[shape] : dd.count[shape], negatives ? dd.count[shape] : 0 );
            }
        }

        RHI_PipelineHandle zonePipeline = shapePipeline( RHI_SHAPE_POLYGON, LAYER_DRAW_MODE::BLEND );

        if( frameBG && zonePipeline && dd.buffer[RHI_SHAPE_POLYGON] && dd.count[RHI_SHAPE_POLYGON] > 0 )
        {
            bindGeometry( zonePipeline, {} );
            m_device->BindVertexBuffer( 0, dd.buffer[RHI_SHAPE_POLYGON] );
            m_device->Draw( dd.count[RHI_SHAPE_POLYGON] );
        }

        for( const auto& bmp : dd.bitmapDraws )
            drawBitmap( bmp );

        drawGrid();
        m_device->EndRenderPass();
    };

    auto renderLayered = [&]()
    {
        // The view sits in the static target where its margin and the pan since it was drawn put it
        const uint32_t staticX = static_cast<uint32_t>( aWS.ShownStaticX() );
        const uint32_t staticY = static_cast<uint32_t>( aWS.ShownStaticY() );

        // Content kept apart from the static target is still ordered among its layers, by
        // testing against the depth the static target was drawn with
        const bool overStatic = dd.testAgainstStatic && !overlayPass && aWS.staticFBO && !drawsStatic
                                && m_geometryDepthLoadRenderPass
                                && m_device->CopyFramebufferDepth( aWS.ShownStaticFBO(), aTargetFBO, staticX,
                                                                   staticY );

        auto usesTemp = [&]( const LAYER_RANGE& aRange )
        {
            return aRange.HasGeometry() && drawsRange( aRange ) && aRange.NeedsTempFBO() && tempFBO
                   && m_geometryLoadRenderPass && m_pipelines->GetInlineCompositorPipeline() && m_compositor
                   && m_compositor->IsInitialized();
        };

        // The static target's depth is copied to the dynamic one, and a pass that a temp layer
        // splits is resumed from its depth. Otherwise the depth ends with the pass.
        const bool keepDepth = drawsStatic || std::any_of( dd.layerRanges.begin(), dd.layerRanges.end(), usesTemp );

        RHI_RenderPassHandle firstPass = overStatic ? m_geometryDepthLoadRenderPass : m_geometryRenderPass;

        if( !keepDepth && m_geometryDepthDiscardRenderPass && m_geometryDepthLoadDiscardRenderPass )
            firstPass = overStatic ? m_geometryDepthLoadDiscardRenderPass : m_geometryDepthDiscardRenderPass;

        bool inStaticPass = beginPass( aTargetFBO, firstPass );

        if( !inStaticPass )
            return;

        const bool drawsGrid = dd.drawGrid && !overlayPass;

        if( drawsGrid && dd.gridFirst )
            drawGrid();

        for( const LAYER_RANGE& lr : dd.layerRanges )
        {
            if( !lr.HasGeometry() || !drawsRange( lr ) )
                continue;

            const bool useTempFBO = usesTemp( lr );

            if( !useTempFBO )
            {
                if( !inStaticPass )
                {
                    inStaticPass = beginPass( aTargetFBO, m_geometryLoadRenderPass );

                    if( !inStaticPass )
                        return;
                }

                drawLayerGeometry( lr );
                continue;
            }

            if( inStaticPass )
            {
                m_device->EndRenderPass();
                inStaticPass = false;
            }

            RHI_RenderPassHandle tempPass =
                    m_geometryDepthDiscardRenderPass ? m_geometryDepthDiscardRenderPass : m_geometryRenderPass;

            // Composited over the whole target, a diff layer would land in front of the static
            // content that hides it. Drawn against the target's depth it leaves that uncovered.
            if( lr.difference && overStatic && m_geometryDepthLoadDiscardRenderPass
                && m_device->CopyFramebufferDepth( aTargetFBO, tempFBO ) )
            {
                tempPass = m_geometryDepthLoadDiscardRenderPass;
            }

            if( !beginPass( tempFBO, tempPass ) )
                continue;

            drawLayerGeometry( lr );
            m_device->EndRenderPass();

            const bool difference = lr.difference && copyBelow();

            if( !beginPass( aTargetFBO, m_geometryLoadRenderPass ) )
                return;

            if( difference )
                compositeDifference( lr.alpha );
            else
                compositeTemp( lr.alpha );

            m_device->EndRenderPass();
        }

        if( drawsGrid && !dd.gridFirst )
        {
            if( !inStaticPass )
            {
                inStaticPass = beginPass( aTargetFBO, m_geometryLoadRenderPass );

                if( !inStaticPass )
                    return;
            }

            drawGrid();
        }

        if( inStaticPass )
            m_device->EndRenderPass();
    };

    if( !dd.layerRanges.empty() )
        renderLayered();
    else if( !overlayPass )
        renderLegacy();
}


bool RENDER_THREAD::prepareCursor( WINDOW_STATE& aWS )
{
    const FRAME_DRAW_DESC& dd = aWS.drawDesc;

    if( !dd.drawCursor || !m_pipelines || !m_pipelines->GetCursorCompositePipeline() )
        return false;

    WINDOW_STATE::FRAME_UNIFORMS& uniforms = aWS.Uniforms();

    return writeUniform( uniforms.cursorUBO, uniforms.cursorBG, m_pipelines->GetFullscreenUBOLayout(),
                         &dd.cursorUBO, sizeof( CURSOR_UBO_DATA ) )
           && m_unitQuadBuffer;
}


bool RENDER_THREAD::writeUniform( RHI_BufferHandle& aUBO, RHI_BindGroupHandle& aBG,
                                  RHI_BindGroupLayoutHandle aLayout, const void* aData, size_t aSize )
{
    if( !aUBO )
    {
        // Host visible, so the per-frame update is a copy and not a staged GPU upload
        RHI_BUFFER_DESC desc;
        desc.type      = RHI_BUFFER_TYPE::UNIFORM;
        desc.sizeBytes = aSize;
        desc.mappable  = true;
        aUBO = m_device->CreateBuffer( desc );

        if( !aUBO )
            return false;
    }

    m_device->UpdateBuffer( aUBO, aData, aSize );

    if( !aBG )
    {
        RHI_BIND_GROUP_DESC bgDesc;
        bgDesc.layout = aLayout;
        bgDesc.entries.push_back( { RHI_UBO_BINDING, aUBO, {}, 0, static_cast<uint32_t>( aSize ) } );
        aBG = m_device->CreateBindGroup( bgDesc );
    }

    return static_cast<bool>( aBG );
}


void RENDER_THREAD::drawCursor( WINDOW_STATE& aWS )
{
    m_device->BindPipeline( m_pipelines->GetCursorCompositePipeline() );
    m_device->BindGroup( 0, aWS.Uniforms().cursorBG );
    m_device->BindVertexBuffer( 0, m_unitQuadBuffer );
    m_device->Draw( 6 );
}


void RENDER_THREAD::HandleSwapchainCreate( const RHI_CMD_SWAPCHAIN_CREATE& aCmd )
{
    wxLogTrace( traceRhi, wxS( "RenderThread: SwapchainCreate windowId=%u %ux%u dpi=%.1f "
                                "native=%p" ),
                aCmd.windowId, aCmd.width, aCmd.height, aCmd.dpiScale,
                aCmd.nativeHandle.nativeHandle );

    if( m_windows.count( aCmd.windowId ) )
    {
        wxLogTrace( traceRhi, wxS( "RenderThread: SwapchainCreate SKIPPED (window already "
                                    "exists)" ) );
        return;
    }

    WINDOW_STATE ws;
    ws.windowId     = aCmd.windowId;
    ws.nativeHandle = aCmd.nativeHandle;
    ws.width        = aCmd.width;
    ws.height       = aCmd.height;
    ws.dpiScale     = aCmd.dpiScale;

    ComputeBackingResolution( aCmd.width, aCmd.height, aCmd.dpiScale,
                              ws.backingWidth, ws.backingHeight );

    wxLogTrace( traceRhi, wxS( "RenderThread: backing resolution %ux%u" ),
                ws.backingWidth, ws.backingHeight );

    RHI_SURFACE_DESC surfDesc;
    surfDesc.window = aCmd.nativeHandle;
    ws.surface = m_device->CreateSurface( surfDesc );

    wxLogTrace( traceRhi, wxS( "RenderThread: surface=%s" ),
                ws.surface ? "OK" : "FAILED" );

    if( ws.surface )
        EnsureSwapchain( ws, "create" );

    ws.mailbox = aCmd.frame2DMailbox;

    m_windows.emplace( aCmd.windowId, std::move( ws ) );

    auto it = m_windows.find( aCmd.windowId );
    CreateWindowFBOs( it->second );

    wxLogTrace( traceRhi, wxS( "RenderThread: window %u created (FBOs: static=%s dynamic=%s)" ),
                aCmd.windowId,
                it->second.staticFBO ? "OK" : "null",
                it->second.dynamicFBO ? "OK" : "null" );
}


void RENDER_THREAD::HandleSwapchainResize( const RHI_CMD_SWAPCHAIN_RESIZE& aCmd )
{
    auto it = m_windows.find( aCmd.windowId );

    if( it == m_windows.end() )
    {
        wxLogTrace( traceRhi, wxS( "RenderThread: SwapchainResize windowId=%u NOT FOUND" ),
                    aCmd.windowId );
        return;
    }

    WINDOW_STATE& ws = it->second;
    ws.nextPresentRetryAt = {};
    ws.width    = aCmd.width;
    ws.height   = aCmd.height;
    ws.dpiScale = aCmd.dpiScale;

    uint32_t newBackingW = 0;
    uint32_t newBackingH = 0;
    ComputeBackingResolution( aCmd.width, aCmd.height, aCmd.dpiScale, newBackingW, newBackingH );

    bool backingChanged = ( newBackingW != ws.backingWidth || newBackingH != ws.backingHeight );

    wxLogTrace( traceRhi, wxS( "RenderThread: SwapchainResize windowId=%u %ux%u -> %ux%u "
                                "(backing %ux%u -> %ux%u, changed=%s)" ),
                aCmd.windowId, ws.backingWidth, ws.backingHeight,
                aCmd.width, aCmd.height, ws.backingWidth, ws.backingHeight,
                newBackingW, newBackingH, backingChanged ? "yes" : "no" );

    ws.backingWidth  = newBackingW;
    ws.backingHeight = newBackingH;

    if( ws.swapchain )
    {
        bool resizeOk = m_device->ResizeSwapchain( ws.swapchain, newBackingW, newBackingH,
                                                    aCmd.dpiScale );

        wxLogTrace( traceRhi, wxS( "RenderThread: ResizeSwapchain windowId=%u %ux%u -> %s" ),
                    aCmd.windowId, newBackingW, newBackingH,
                    resizeOk ? "OK" : "FAILED" );

        if( !resizeOk )
        {
            // A failed resize leaves the swapchain at an indeterminate size. A failed recreation
            // leaves it null for the frame loop to retry.
            recreateSwapchainAfterOutOfDate( ws );
        }
    }
    else if( ws.surface )
    {
        EnsureSwapchain( ws, "resize" );
    }

    // Targets that cannot be made stay null, so the window is not drawn at a stale size
    if( backingChanged )
    {
        DestroyWindowFBOs( ws );
        CreateWindowFBOs( ws );
    }

    ws.needsStaticInvalidate = true;
}


void RENDER_THREAD::HandleSwapchainDestroy( const RHI_CMD_SWAPCHAIN_DESTROY& aCmd )
{
    wxLogTrace( traceRhi, wxS( "RenderThread: SwapchainDestroy windowId=%u" ), aCmd.windowId );

    auto it = m_windows.find( aCmd.windowId );

    if( it == m_windows.end() )
    {
        wxLogTrace( traceRhi, wxS( "RenderThread: SwapchainDestroy NOT FOUND" ) );
        return;
    }

    destroyWindow( it->second );
    m_windows.erase( it );
}


void RENDER_THREAD::HandleFence( const RHI_CMD_FENCE& aCmd )
{
    // Uploads travel on their own queue, so the ones sent ahead of the fence are applied here.
    // Otherwise the fence can fire first and its sender reads buffers that are not yet written.
    DrainUploadQueue();

    // Frames requested ahead of the fence are drawn before it fires, so its sender sees their results.
    // Refresh pacing would hold them back, so it does not apply here.
    if( std::ranges::any_of( m_windows, []( const auto& aEntry ) { return aEntry.second.active; } ) )
        ProcessActiveWindows( true );

    if( aCmd.onComplete )
        aCmd.onComplete();
}


void RENDER_THREAD::DestroyAllWindows()
{
    // Without a device the resources died with it, so only the bookkeeping is cleared
    if( m_device )
    {
        for( auto& [id, ws] : m_windows )
            destroyWindow( ws );
    }

    m_windows.clear();
}


void RENDER_THREAD::destroyWindow( WINDOW_STATE& aWS )
{
    DestroyWindowFBOs( aWS );
    DestroyWindowUniforms( aWS );
    m_device->Release( aWS.swapchain );
    m_device->Release( aWS.surface );
}


void RENDER_THREAD::CreateWindowFBOs( WINDOW_STATE& aWS )
{
    if( aWS.backingWidth == 0 || aWS.backingHeight == 0 )
        return;

    // A resize can grow the window past what the current factor fits
    aWS.supersampleFactor = fitSupersampleFactor( aWS.supersampleFactor, aWS, m_device->MaxTextureSize() );

    const RHI_FRAMEBUFFER_DESC fbDesc = layerFramebufferDesc( aWS );

    aWS.staticMargin = staticMarginFor( aWS, m_device->MaxTextureSize() );
    aWS.staticShift = {};
    aWS.overlayShift = {};
    aWS.renderedStatic.valid = false;
    aWS.renderedOverlay.valid = false;

    const RHI_FRAMEBUFFER_DESC staticDesc = marginFramebufferDesc( aWS );

    aWS.staticFBO  = m_device->CreateFramebuffer( staticDesc );
    aWS.dynamicFBO = m_device->CreateFramebuffer( fbDesc );
    aWS.tempFBO    = m_device->CreateFramebuffer( fbDesc );

    RHI_FRAMEBUFFER_DESC compFBDesc;
    compFBDesc.width  = aWS.RenderWidth();
    compFBDesc.height = aWS.RenderHeight();
    compFBDesc.colorFormats.push_back( RHI_TEXTURE_FORMAT::RGBA8 );
    compFBDesc.hasDepthStencil = false;

    aWS.accumulationFBO = m_device->CreateFramebuffer( compFBDesc );
}


void RENDER_THREAD::DestroyWindowFBOs( WINDOW_STATE& aWS )
{
    // The compositor caches bind groups over these targets' textures, so stale ones would pile up
    if( m_compositor )
        m_compositor->InvalidateCachedBindGroups();

    for( RHI_FramebufferHandle* fbo : { &aWS.staticViewFBO, &aWS.staticFBO, &aWS.dynamicFBO, &aWS.tempFBO,
                                        &aWS.staticTempFBO, &aWS.overlayGeometryFBO, &aWS.overlayMarginFBO,
                                        &aWS.accumulationFBO } )
    {
        m_device->Release( *fbo );
    }

    destroyDiffCopies( *m_device, aWS );

    aWS.staticInMargin = true;
    aWS.overlayGeometryDrawn = false;
    aWS.overlayInMargin = false;
}


void RENDER_THREAD::DestroyWindowUniforms( WINDOW_STATE& aWS )
{
    for( WINDOW_STATE::FRAME_UNIFORMS& set : aWS.uniformSets )
    {
        for( WINDOW_STATE::PASS_UNIFORMS* uniforms :
             { &set.staticUniforms, &set.dynamicUniforms, &set.overlayUniforms } )
        {
            m_device->Release( uniforms->frameBG );
            m_device->Release( uniforms->gridBG );
            m_device->Release( uniforms->frameUBO );
            m_device->Release( uniforms->gridUBO );
        }

        m_device->Release( set.cursorBG );
        m_device->Release( set.cursorUBO );
    }
}


void RENDER_THREAD::ComputeBackingResolution( uint32_t aLogicalW, uint32_t aLogicalH,
                                               float aDpiScale,
                                               uint32_t& aBackingW, uint32_t& aBackingH )
{
    aBackingW = static_cast<uint32_t>( std::floor( static_cast<float>( aLogicalW ) * aDpiScale ) );
    aBackingH = static_cast<uint32_t>( std::floor( static_cast<float>( aLogicalH ) * aDpiScale ) );

    if( aBackingW == 0 )
        aBackingW = 1;

    if( aBackingH == 0 )
        aBackingH = 1;
}


void RENDER_THREAD::OnDeviceLost()
{
    wxLogTrace( traceRhi, wxS( "RenderThread: device lost, leaving recovery to the canvas" ) );

    m_uploadQueue.clear();
    m_deviceLost.store( true, std::memory_order_release );

    // The owner may otherwise not look until something else asks for a frame
    if( m_onDeviceLost )
        m_onDeviceLost();
}


} // namespace KIGFX
