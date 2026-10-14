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

#ifndef RHI_SWAPCHAIN_CONTROLLER_H
#define RHI_SWAPCHAIN_CONTROLLER_H

#include <rhi/rhi_control_commands.h>

#include <chrono>
#include <cstdint>


namespace KIGFX
{

class RENDER_THREAD;
class FRAME_MAILBOX;


struct RHI_PHASE_STALL_RESULT
{
    bool                      shouldLog          = false;
    int                       phase              = -1;
    uint64_t                  stalledGenerations = 0;
    std::chrono::milliseconds stallAge{ 0 };
};


struct RHI_PHASE_STALL_TRACKER
{
    using CLOCK = std::chrono::steady_clock;

    int               observedPhase           = -1;
    uint64_t          generationAtPhaseStart = 0;
    CLOCK::time_point phaseStartTime;
    CLOCK::time_point lastLogTime;
};


inline RHI_PHASE_STALL_RESULT EvaluateRenderThreadPhaseStall(
        RHI_PHASE_STALL_TRACKER& aTracker,
        int aPhase,
        uint64_t aGeneration,
        RHI_PHASE_STALL_TRACKER::CLOCK::time_point aNow,
        std::chrono::milliseconds aStallThreshold = std::chrono::milliseconds( 1000 ),
        uint64_t aGenerationThreshold = 60,
        std::chrono::milliseconds aRepeatThreshold = std::chrono::milliseconds( 1000 ) )
{
    RHI_PHASE_STALL_RESULT result;
    result.phase = aPhase;

    bool firstObservation = aTracker.phaseStartTime.time_since_epoch().count() == 0;
    bool phaseChanged = aTracker.observedPhase != aPhase;
    bool generationRewound = aGeneration < aTracker.generationAtPhaseStart;

    if( firstObservation || phaseChanged || generationRewound )
    {
        aTracker.observedPhase = aPhase;
        aTracker.generationAtPhaseStart = aGeneration;
        aTracker.phaseStartTime = aNow;
        aTracker.lastLogTime = {};
        return result;
    }

    result.stalledGenerations = aGeneration - aTracker.generationAtPhaseStart;
    result.stallAge = std::chrono::duration_cast<std::chrono::milliseconds>(
            aNow - aTracker.phaseStartTime );

    if( result.stalledGenerations < aGenerationThreshold
        || result.stallAge < aStallThreshold )
    {
        return result;
    }

    if( aTracker.lastLogTime.time_since_epoch().count() != 0
        && std::chrono::duration_cast<std::chrono::milliseconds>(
                   aNow - aTracker.lastLogTime ) < aRepeatThreshold )
    {
        return result;
    }

    result.shouldLog = true;
    aTracker.lastLogTime = aNow;
    return result;
}

/**
 * Turns one window's paint, resize and close events into render thread control commands.
 *
 * Not thread-safe. Call only from the UI thread.
 */
class SWAPCHAIN_CONTROLLER
{
public:
    SWAPCHAIN_CONTROLLER( RENDER_THREAD& aRenderThread, RHI_WINDOW_ID aWindowId );
    ~SWAPCHAIN_CONTROLLER();

    SWAPCHAIN_CONTROLLER( const SWAPCHAIN_CONTROLLER& ) = delete;
    SWAPCHAIN_CONTROLLER& operator=( const SWAPCHAIN_CONTROLLER& ) = delete;

    /**
     * The first call creates the swapchain. Every call requests a frame.
     *
     * @param aHandle is only used by the first call.
     * @return true if the commands were submitted.
     */
    bool OnPaint( const RHI_NATIVE_WINDOW_HANDLE& aHandle,
                  uint32_t aWidth, uint32_t aHeight, float aDpiScale = 1.0f );

    /**
     * @return true if the size is unchanged or the resize was submitted.
     */
    bool OnResize( uint32_t aWidth, uint32_t aHeight, float aDpiScale = 1.0f );

    /**
     * @return true if a swapchain existed and its destroy was submitted.
     */
    bool OnClose();

    /**
     * Close and wait for the render thread to release the window's GPU resources, so the native
     * window can be destroyed right after.
     *
     * @return true if the destroy was processed within the timeout.
     */
    bool OnCloseSync(
            std::chrono::milliseconds aTimeout = std::chrono::milliseconds( 2000 ) );

    void SetFrameMailbox( FRAME_MAILBOX* aMailbox ) { m_frameMailbox = aMailbox; }

    /** Display refresh rate sent with each frame request, 0 when unknown. */
    void SetRefreshRate( float aHz ) { m_refreshHz = aHz; }

private:
    RENDER_THREAD& m_renderThread;
    RHI_WINDOW_ID  m_windowId;
    bool           m_created       = false;
    uint64_t       m_generation    = 0;
    uint32_t       m_width         = 0;
    uint32_t       m_height        = 0;
    float          m_dpiScale      = 1.0f;
    float          m_refreshHz     = 0.0f;
    FRAME_MAILBOX* m_frameMailbox  = nullptr;
    uint64_t       m_lastLoggedGeneration = 0;
    RHI_PHASE_STALL_TRACKER m_phaseStallTracker;
};


} // namespace KIGFX


#endif // RHI_SWAPCHAIN_CONTROLLER_H
