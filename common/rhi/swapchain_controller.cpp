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

#include <rhi/swapchain_controller.h>
#include <rhi/render_thread.h>
#include <trace_helpers.h>

#include <wx/log.h>


namespace KIGFX
{


SWAPCHAIN_CONTROLLER::SWAPCHAIN_CONTROLLER( RENDER_THREAD& aRenderThread,
                                            RHI_WINDOW_ID aWindowId ) :
        m_renderThread( aRenderThread ),
        m_windowId( aWindowId )
{
}


SWAPCHAIN_CONTROLLER::~SWAPCHAIN_CONTROLLER()
{
    if( m_created )
        OnCloseSync( std::chrono::milliseconds( 1000 ) );
}


bool SWAPCHAIN_CONTROLLER::OnPaint( const RHI_NATIVE_WINDOW_HANDLE& aHandle,
                                    uint32_t aWidth, uint32_t aHeight, float aDpiScale )
{
    auto now = RHI_PHASE_STALL_TRACKER::CLOCK::now();

    if( !m_created )
    {
        wxLogTrace( traceRhi, wxS( "SwapchainCtrl: OnPaint first call windowId=%u %ux%u "
                                    "dpi=%.1f native=%p" ),
                    m_windowId, aWidth, aHeight, aDpiScale, aHandle.nativeHandle );

        RHI_CMD_SWAPCHAIN_CREATE create;
        create.windowId       = m_windowId;
        create.nativeHandle   = aHandle;
        create.width          = aWidth;
        create.height         = aHeight;
        create.dpiScale       = aDpiScale;
        create.frame2DMailbox = m_frameMailbox;

        if( !m_renderThread.SubmitControl( create ) )
        {
            wxLogTrace( traceRhi, wxS( "SwapchainCtrl: SubmitControl(CREATE) FAILED" ) );
            return false;
        }

        m_created  = true;
        m_width    = aWidth;
        m_height   = aHeight;
        m_dpiScale = aDpiScale;
    }

    RHI_CMD_FRAME_REQUEST req;
    req.windowId   = m_windowId;
    req.generation = m_generation++;
    req.refreshHz  = m_refreshHz;

    bool ok = m_renderThread.SubmitControl( req );

    if( !ok )
    {
        wxLogTrace( traceRhi, wxS( "SwapchainCtrl: SubmitControl(FRAME_REQUEST) FAILED "
                                    "windowId=%u gen=%llu" ),
                    m_windowId, static_cast<unsigned long long>( m_generation - 1 ) );
    }
    else if( ( m_generation - m_lastLoggedGeneration ) >= 60 )
    {
        wxLogTrace( traceRhi, wxS( "SwapchainCtrl: OnPaint windowId=%u gen=%llu" ),
                    m_windowId, static_cast<unsigned long long>( m_generation - 1 ) );
        m_lastLoggedGeneration = m_generation;
    }

    int rtPhase = m_renderThread.GetLoopPhase();
    auto stall = EvaluateRenderThreadPhaseStall( m_phaseStallTracker, rtPhase, m_generation, now );

    if( stall.shouldLog )
    {
        wxLogTrace( traceRhi, wxS( "SwapchainCtrl: suspected render-thread stall "
                                    "windowId=%u phase=%d gen=%llu queued=%llu age=%lldms" ),
                    m_windowId, stall.phase,
                    static_cast<unsigned long long>( m_generation ),
                    static_cast<unsigned long long>( stall.stalledGenerations ),
                    static_cast<long long>( stall.stallAge.count() ) );
    }

    return ok;
}


bool SWAPCHAIN_CONTROLLER::OnResize( uint32_t aWidth, uint32_t aHeight, float aDpiScale )
{
    if( !m_created )
    {
        wxLogTrace( traceRhi, wxS( "SwapchainCtrl: OnResize SKIPPED (not created) "
                                    "windowId=%u %ux%u" ),
                    m_windowId, aWidth, aHeight );
        return false;
    }

    if( aWidth == m_width && aHeight == m_height && aDpiScale == m_dpiScale )
        return true;

    wxLogTrace( traceRhi, wxS( "SwapchainCtrl: OnResize windowId=%u %ux%u -> %ux%u "
                                "dpi=%.1f" ),
                m_windowId, m_width, m_height, aWidth, aHeight, aDpiScale );

    m_width    = aWidth;
    m_height   = aHeight;
    m_dpiScale = aDpiScale;

    RHI_CMD_SWAPCHAIN_RESIZE resize;
    resize.windowId = m_windowId;
    resize.width    = aWidth;
    resize.height   = aHeight;
    resize.dpiScale = aDpiScale;

    return m_renderThread.SubmitControl( resize );
}


bool SWAPCHAIN_CONTROLLER::OnClose()
{
    wxLogTrace( traceRhi, wxS( "SwapchainCtrl: OnClose windowId=%u (created=%s)" ),
                m_windowId, m_created ? "yes" : "no" );

    if( !m_created )
        return false;

    RHI_CMD_SWAPCHAIN_DESTROY destroy;
    destroy.windowId = m_windowId;

    bool ok = m_renderThread.SubmitControl( destroy );
    m_created = false;

    wxLogTrace( traceRhi, wxS( "SwapchainCtrl: OnClose submit=%s" ), ok ? "OK" : "FAILED" );

    return ok;
}


bool SWAPCHAIN_CONTROLLER::OnCloseSync( std::chrono::milliseconds aTimeout )
{
    wxLogTrace( traceRhi, wxS( "SwapchainCtrl: OnCloseSync windowId=%u timeout=%lldms" ),
                m_windowId, static_cast<long long>( aTimeout.count() ) );

    if( !OnClose() )
        return false;

    bool ok = m_renderThread.WaitForFence( aTimeout );

    wxLogTrace( traceRhi, wxS( "SwapchainCtrl: OnCloseSync fence=%s" ),
                ok ? "signaled" : "TIMEOUT" );

    return ok;
}


} // namespace KIGFX
