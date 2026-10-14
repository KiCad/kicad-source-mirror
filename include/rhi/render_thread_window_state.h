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

#ifndef RHI_RENDER_THREAD_WINDOW_STATE_H
#define RHI_RENDER_THREAD_WINDOW_STATE_H

#include <rhi/rhi_control_commands.h>
#include <rhi/rhi_handle.h>
#include <rhi/rhi_surface.h>
#include <rhi/frame_mailbox.h>
#include <rhi/frame_packet.h>
#include <rhi/rhi_static_margin.h>
#include <rhi/rhi_present_mode.h>

#include <chrono>
#include <cstdint>
#include <optional>


namespace KIGFX
{

/**
 * Per-window state owned by the render thread. It is created and destroyed by the
 * SWAPCHAIN_CREATE/RESIZE/DESTROY control commands and touched by no other thread.
 */
struct WINDOW_STATE
{
    RHI_WINDOW_ID            windowId   = 0;
    RHI_NATIVE_WINDOW_HANDLE nativeHandle;

    RHI_SurfaceHandle   surface;
    RHI_SwapchainHandle swapchain;

    uint32_t width    = 0;
    uint32_t height   = 0;
    float    dpiScale = 1.0f;

    uint32_t backingWidth  = 0;
    uint32_t backingHeight = 0;

    /**
     * Supersampling factor. Layer and compositor targets are the backing size times this and the
     * present blit downsamples to the swapchain, which stays at the backing size.
     */
    uint32_t supersampleFactor = 1;

    uint32_t RenderWidth() const { return backingWidth * supersampleFactor; }
    uint32_t RenderHeight() const { return backingHeight * supersampleFactor; }

    RHI_FramebufferHandle staticFBO;
    RHI_FramebufferHandle dynamicFBO;

    /**
     * Frame pixels the static target holds beyond each view edge so a pan within them is only a
     * shift. 0 when the static target is the view's size.
     */
    uint32_t staticMargin = 0;

    /** The static target's temp target, made when a static layer first needs one. */
    RHI_FramebufferHandle staticTempFBO;

    /**
     * Static content drawn without the margin, as during a zoom, goes into this view sized target
     * so the zoom does not clear and store an empty margin.
     */
    RHI_FramebufferHandle staticViewFBO;
    bool                  staticInMargin = true;    /**< the content shown is in staticFBO */

    RHI_FramebufferHandle ShownStaticFBO() const
    {
        return staticInMargin || !staticViewFBO ? staticFBO : staticViewFBO;
    }

    /** Where the view starts in the shown static target, in texels. */
    int32_t ShownStaticX() const
    {
        return staticInMargin ? ( static_cast<int32_t>( staticMargin ) - staticShift.x )
                                        * static_cast<int32_t>( supersampleFactor )
                              : 0;
    }

    int32_t ShownStaticY() const
    {
        return staticInMargin ? ( static_cast<int32_t>( staticMargin ) - staticShift.y )
                                        * static_cast<int32_t>( supersampleFactor )
                              : 0;
    }

    /** How far the view has moved since the static target was drawn. */
    RHI_VIEW_SHIFT staticShift;

    /** How far the view has moved since overlayMarginFBO was drawn. */
    RHI_VIEW_SHIFT overlayShift;

    uint32_t StaticWidth() const { return RenderWidth() + 2 * staticMargin * supersampleFactor; }
    uint32_t StaticHeight() const { return RenderHeight() + 2 * staticMargin * supersampleFactor; }
    RHI_FramebufferHandle tempFBO;

    /**
     * Copies of what lies under a difference layer, for the view sized targets and the static one.
     * Dropped once no difference layer is left.
     */
    RHI_FramebufferHandle diffBelowFBO;
    RHI_FramebufferHandle staticDiffBelowFBO;

    /**
     * Overlay content, drawn from a transparent start and composited over the dynamic target
     * (see RHI_GAL_PIPELINES::GetOverlayVariant).
     */
    RHI_FramebufferHandle overlayGeometryFBO;
    bool                  overlayGeometryDrawn = false;

    /**
     * Overlay content that stayed the same through a pan, drawn with the static target's margin.
     * Changing content goes to the view sized overlayGeometryFBO so a drag never pays for the margin.
     */
    RHI_FramebufferHandle overlayMarginFBO;
    bool                  overlayInMargin = false;

    /** An empty dynamicFBO is neither drawn nor composited. */
    bool dynamicGeometryDrawn = true;

    /** Color only, without depth or stencil. */
    RHI_FramebufferHandle accumulationFBO;

    /**
     * Uniform buffers owned by one geometry target. Passes are submitted without waiting for the
     * GPU, so sharing a buffer between passes would stall on every rewrite.
     */
    struct PASS_UNIFORMS
    {
        RHI_BufferHandle    frameUBO;
        RHI_BindGroupHandle frameBG;
        RHI_BufferHandle    gridUBO;
        RHI_BindGroupHandle gridBG;
    };

    /** One window's uniforms for one frame. */
    struct FRAME_UNIFORMS
    {
        PASS_UNIFORMS       staticUniforms;
        PASS_UNIFORMS       dynamicUniforms;
        PASS_UNIFORMS       overlayUniforms;
        RHI_BufferHandle    cursorUBO;
        RHI_BindGroupHandle cursorBG;
    };

    /**
     * A frame's passes are still queued when the next frame is built, so frames rotate through
     * several uniform sets rather than waiting on the previous one.
     */
    static constexpr size_t UNIFORM_GENERATIONS = 3;

    FRAME_UNIFORMS uniformSets[UNIFORM_GENERATIONS];
    size_t         uniformGeneration = 0;

    FRAME_UNIFORMS& Uniforms() { return uniformSets[uniformGeneration]; }

    FRAME_DRAW_DESC drawDesc;

    /**
     * Static and dynamic pool descriptions, each drawn into its own target. The changed flags say
     * whether that target must be redrawn this frame.
     */
    FRAME_DRAW_DESC staticDrawDesc;
    FRAME_DRAW_DESC dynamicDrawDesc;
    bool            staticChanged  = true;
    bool            dynamicChanged = true;

    uint64_t staticEpoch  = 0;
    uint64_t dynamicEpoch = 0;

    /**
     * What a geometry target holds, so a frame asking for the same content at the same camera can
     * reuse it.
     */
    struct RENDERED_CONTENT
    {
        bool           valid = false;
        uint64_t       epoch = 0;
        bool           drawGrid = false;
        FRAME_UBO_DATA frameUBO;
        GRID_UBO_DATA  gridUBO;
        uint32_t       margin = 0;      /**< how much of the static target's margin was drawn */
    };

    RENDERED_CONTENT renderedStatic;
    RENDERED_CONTENT renderedDynamic;
    RENDERED_CONTENT renderedOverlay;   /**< the overlay target keeps the static target's margin */

    uint64_t staticPasses  = 0;     /**< times staticFBO was re-rendered */
    uint64_t geometryRenderPasses = 0;  /**< render passes begun while drawing geometry */
    uint64_t dynamicPasses = 0;     /**< times dynamicFBO was re-rendered */

    uint64_t requestedGeneration    = 0;
    uint64_t lastRenderedGeneration = 0;

    bool     needsStaticInvalidate = false;
    bool     active = false;
    bool     frameResultPending = false;

    uint64_t framesProcessed = 0;

    using Clock = std::chrono::steady_clock;
    Clock::time_point lastActivityTime;
    Clock::time_point lastFrameRequestTime;
    Clock::time_point lastRenderStartTime;
    Clock::time_point lastRenderCompleteTime;
    Clock::time_point lastPresentAttemptTime;
    Clock::time_point lastPresentSuccessTime;
    Clock::time_point lastPresentFailureTime;
    Clock::time_point nextPresentRetryAt;
    Clock::time_point lastSwapchainCreateAttemptTime;
    Clock::time_point lastLagLogTime;

    /**
     * A swapchain that presents immediately is drawn at most once per refresh unless the frame
     * answers input the last drawn frame had not seen.
     */
    float             refreshHz = 0.0f;     /**< 0 when unknown, which lifts the limit */
    Clock::time_point nextFrameAt;          /**< earliest time such a window may be drawn again */
    uint64_t          drawnInputSerial = 0; /**< FRAME_PACKET::inputSerial of the last packet loaded */

    uint64_t          presentSuccessCount = 0;
    uint64_t          presentFailureCount = 0;
    uint32_t          consecutivePresentFailures = 0;
    uint64_t          swapchainCreateAttempts = 0;
    uint64_t          lastPresentedGeneration = 0;

    /** Owned by the panel. */
    FRAME_MAILBOX* mailbox = nullptr;
};


} // namespace KIGFX


#endif // RHI_RENDER_THREAD_WINDOW_STATE_H
