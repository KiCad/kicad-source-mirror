/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef  PANELGAL_WXSTRUCT_H
#define  PANELGAL_WXSTRUCT_H

#include <wx/window.h>
#include <wx/timer.h>
#include <wx/grid.h> // needed for MSVC to see wxScrolledCanvas indirectly exported
#include <math/box2.h>
#include <math/vector2d.h>
#include <view/automatic_fit.h>
#include <widgets/msgpanel.h>
#include <rhi/frame_packet.h>
#include <rhi/rhi_surface.h>
#include <atomic>
#include <memory>
#include <optional>
#include <mutex>
#include <chrono>

#include <gal/cursors.h>

class BOARD;
class EDA_DRAW_FRAME;
class TOOL_DISPATCHER;
class PROF_COUNTER;
class wxImage;

namespace KIPLATFORM
{
namespace UI
{
class TOUCHPAD_GESTURE_HANDLER;
}
}

namespace KIGFX
{
class GAL;
class FRAME_MAILBOX;
class RENDER_THREAD;
class RHI_DEVICE_PROXY;
class SWAPCHAIN_CONTROLLER;
class VIEW;
class WX_VIEW_CONTROLS;
class VIEW_CONTROLS;
class PAINTER;
class GAL_DISPLAY_OPTIONS;
class VIEW_OVERLAY;
struct VC_SETTINGS;
}


class EDA_DRAW_PANEL_GAL : public wxScrolledCanvas
{
public:
    enum GAL_TYPE {
        GAL_TYPE_UNKNOWN = -1,  ///< not specified: a GAL engine must be set by the client
        GAL_TYPE_NONE = 0,      ///< GAL not used (the legacy wxDC engine is used)
        GAL_TYPE_OPENGL = 1,    /**< OpenGL implementation */
        GAL_TYPE_CAIRO = 2,     /**< Cairo implementation */
        GAL_TYPE_MODERN = 3,    /**< RHI implementation, Vulkan or Metal on macOS */
        GAL_TYPE_LAST           ///< Sentinel, do not use as a parameter
    };

#ifdef __WXMAC__
    // Cairo doesn't work on OSX so we really have no fallback available.
    static constexpr GAL_TYPE GAL_FALLBACK = GAL_TYPE_OPENGL;
#else
    static constexpr GAL_TYPE GAL_FALLBACK = GAL_TYPE_CAIRO;
#endif
    static constexpr bool GAL_FALLBACK_AVAILABLE = GAL_FALLBACK != GAL_TYPE_OPENGL;

    /**
     * Create a drawing panel that is contained inside \p aParentWindow.
     *
     * If \p aParentWindow is not an EDA frame, a search through all the parents
     * of the parent window will be done to find the frame.
     *
     * @param aParentWindow is the window immediately containing this panel.
     * @param aWindowId is the ID for this draw panel.
     * @param aPosition is the position of the panel.
     * @param aSize is the size of the panel.
     * @param aOptions are the display options for the panel.
     * @param aGalType is the type of graphics abstraction layer (GAL) for the panel.
     */
    EDA_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                        const wxPoint& aPosition, const wxSize& aSize,
                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions,
                        GAL_TYPE aGalType = GAL_TYPE_OPENGL );
    ~EDA_DRAW_PANEL_GAL();

    void SetFocus() override;

    bool StatusPopupHasFocus()
    {
        return m_statusPopup && m_statusPopup->HasFocus();
    }

    void SetStatusPopup( wxWindow* aPopup )
    {
        m_statusPopup = aPopup;
    }

    /**
     * Convert a stored canvas type setting into a backend that can actually be used.
     *
     * A settings file may have been written by hand or by another version of KiCad, so a value
     * outside the supported range is bad data rather than a programming error.  It resolves to
     * the OpenGL canvas, as does the retired wxDC canvas (#GAL_TYPE_NONE).  Only an explicitly
     * stored #GAL_TYPE_MODERN selects the RHI canvas.
     *
     * @param aStoredCanvasType is the raw COMMON_SETTINGS graphics.canvas_type value.
     * @return a GAL type that #SwitchBackend() implements.
     */
    static GAL_TYPE ResolveStoredCanvasType( int aStoredCanvasType );

    /**
     * Return the canvas that takes over from one that could not be used.
     *
     * A failed accelerated canvas goes straight to #GAL_FALLBACK.
     *
     * @return #GAL_TYPE_NONE when nothing is left to switch to.
     */
    static GAL_TYPE CanvasAfterFailure( GAL_TYPE aFailedCanvasType );

    /**
     * True for a canvas drawn on the GPU, which keeps cached layers between frames.
     */
    static bool IsAccelerated( GAL_TYPE aCanvasType )
    {
        return aCanvasType == GAL_TYPE_OPENGL || aCanvasType == GAL_TYPE_MODERN;
    }

    /**
     * Switch method of rendering graphics.
     *
     * @param aGalType is a type of rendering engine that you want to use.
     */
    virtual bool SwitchBackend( GAL_TYPE aGalType );

    /**
     * Apply the current native touchpad gesture preference to the active backend window.
     */
    void UpdateTouchpadGestureHandler();

    /**
     * Return the type of backend currently used by GAL canvas.
     */
    inline GAL_TYPE GetBackend() const { return m_backend; }

    /**
     * Return a pointer to the GAL instance used in the panel.
     *
     * @return The instance of GAL.
     */
    KIGFX::GAL* GetGAL() const { return m_gal; }

    /**
     * Return a pointer to the #KIGFX::VIEW instance used in the panel.
     *
     * @return The instance of #KIGFX::VIEW.
     */
    virtual KIGFX::VIEW* GetView() const { return m_view; }

    /**
     * Return a pointer to the #KIGFX::VIEW_CONTROLS instance used in the panel.
     *
     * @return The instance of #KIGFX::VIEW_CONTROLS.
     */
    KIGFX::VIEW_CONTROLS* GetViewControls() const
    {
        return (KIGFX::VIEW_CONTROLS*)( m_viewControls );
    }

    /**
     * Capture the current canvas contents into aDstImage.
     *
     * @return true on success, false if the backend can't be captured this way (the caller
     *         should fall back to a DC blit).
     */
    bool GetScreenshot( wxImage& aDstImage );

    /**
     * @copydoc wxWindow::Refresh()
     */
    virtual void Refresh( bool aEraseBackground = true, const wxRect* aRect = nullptr ) override;

    /**
     * Force a redraw.
     */
    void ForceRefresh();

    /**
     * Make sure a refresh gets done on the next idle event if it hasn't already.
     */
    void RequestRefresh();

    /**
     * Tell the backend which areas of this panel are covered by an overlaid infobar.
     *
     * Must be called whenever an infobar is shown, hidden or moved.
     */
    void UpdateOverlayExclusions();

    /**
     * Resize the GAL to the current client size of this panel.
     *
     * This must be used in preference to calling GAL::ResizeScreen() directly: it holds the
     * GL context lock while the compositor buffers are reallocated, clamps degenerate sizes
     * and invalidates the view.
     *
     * @param aForce reallocates even when the client size is unchanged, which is required
     *               after a display scale factor change because the buffers are sized in
     *               native pixels rather than client units.
     */
    void ResizeGal( bool aForce = false );

    /**
     * Return how many pixels larger than the client area a backend's GAL is made.
     */
    static int ScreenSizePad( GAL_TYPE aBackend );

    /**
     * Set a dispatcher that processes events and forwards them to tools.
     *
     * #EDA_DRAW_PANEL_GAL does not take over the ownership. Passing NULL disconnects all event
     * handlers from the #EDA_DRAW_PANEL_GAL and parent frame.
     *
     * @param aEventDispatcher is the object that will be used for dispatching events.
     */
    void SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher );

    /**
     * Begin drawing if it was stopped previously.
     */
    void StartDrawing();

    /**
     * Prevent the GAL canvas from further drawing until it is recreated or #StartDrawing()
     * is called.
     */
    void StopDrawing();

    /**
     * Take care of display settings for the given layer to be displayed in high contrast mode.
     */
    virtual void SetHighContrastLayer( int aLayer );

    /**
     * Move the selected layer to the top, so it is displayed above all others.
     */
    virtual void SetTopLayer( int aLayer );

    virtual void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
    {
        wxASSERT( false );
    }

    /**
     * Returns parent #EDA_DRAW_FRAME, if available or NULL otherwise.
     */
    EDA_DRAW_FRAME* GetParentEDAFrame() const { return m_edaFrame; }

    bool IsDialogPreview() const { return m_parent != (wxWindow*) m_edaFrame; }

    /**
     * Called when the window is shown for the first time. Updating the view's items tells whether
     * the canvas can draw, and it falls back to another canvas when it cannot.
     */
    virtual void OnShow();

    /**
     * Set whether focus is taken on certain events (mouseover, keys, etc).
     *
     * This should be true (and is by default) for any primary canvas, but can be false to make
     * well behaved preview panes and the like.
     */
    void SetStealsFocus( bool aStealsFocus ) { m_stealsFocus = aStealsFocus; }

    /**
     * Set the current cursor shape for this panel.
     */
    void SetCurrentCursor( KICURSOR aCursor );

    /**
     * Return the bounding box of the view that should be used if model is not valid.
     * For example, the drawing sheet bounding box for an empty PCB
     *
     * @return the default bounding box for the panel.
     */
    virtual BOX2I GetDefaultViewBBox() const { return BOX2I(); }

    /**
     * Scale and centre the view on @a aBox, and fit it again whenever the canvas changes size
     * until something else zooms or pans.
     *
     * @param aMarginScale is how much larger than the box the view is. By default a little
     *                     more on a short canvas, where the infobar covers a larger share.
     */
    void ZoomToFit( const BOX2I& aBox, std::optional<double> aMarginScale = std::nullopt );

    /**
     * Used to forward events to the canvas from popups, etc.
     */
    void OnEvent( wxEvent& aEvent );

    /**
     * Count an input so the frame that answers it is never held back by the display-rate limit.
     * Call before the input is acted on.
     */
    static void NoteInput();

    /**
     * Repaint the canvas, and fix scrollbar cursors
     *
     * Usually called by a OnPaint event.
     *
     * Does not use a wxPaintDC, so it can be called outside a wxPaintEvent.
     *
     * @return true if the repaint attempt was successful.
     */
    bool DoRePaint( bool aAllowSkip = true );

    /**
     * Create an overlay for rendering debug graphics.
     */
    std::shared_ptr<KIGFX::VIEW_OVERLAY> DebugOverlay();


    /**
     * Clear the contents of the debug overlay and removes it from the VIEW.
     */
    void ClearDebugOverlay();


    /**
     * Gets a populated View Controls settings object dervived from our program settings
     */
    static KIGFX::VC_SETTINGS GetVcSettings();

    /**
     * used on wxMSW: true after a wxEVT_MOUSE_CAPTURE_LOST was received
     * false after the mouse is recaptured.
     * Used to avoid calling twice a CaptureMouse(), not accepted by wxMSW
     */
    bool m_MouseCapturedLost;

    std::unique_ptr<PROF_COUNTER> m_PaintEventCounter;

    /**
     * Hook for subclasses to push per-frame grid sources onto the GAL.
     * Called from DoRePaint just before GAL::DrawGrid.
     */
    virtual void prepareGridSources() {}

    /**
     * Hook for subclasses to update view items whose inputs nothing else tracks.
     * Called from DoRePaint before the pending item updates are applied.
     */
    virtual void beforeRedraw() {}

protected:
    virtual void onPaint( wxPaintEvent& WXUNUSED( aEvent ) );
    void onSize( wxSizeEvent& aEvent );
    void onEnter( wxMouseEvent& aEvent );
    void onLostFocus( wxFocusEvent& aEvent );
    void onIdle( wxIdleEvent& aEvent );
    void onRefreshTimer( wxTimerEvent& aEvent );
    void onShowEvent( wxShowEvent& aEvent );

    bool recoverFromGalError( const std::exception& aErr );

    /**
     * Refresh rate in Hz of the display showing the canvas, or 0 when unknown.
     */
    float displayRefreshRate();

    /**
     * Replace a GAL whose context was destroyed by a GPU reset.
     *
     * @return true if the GAL was replaced, so the current repaint must stop.
     */
    bool handleContextLoss();

    /**
     * Show a recovery message once the current event finishes.
     *
     * Recovery runs inside a repaint, where a modal dialog would stop every canvas from painting
     * until it was closed.
     */
    void showMessageLater( const wxString& aTitle, const wxString& aDetail, bool aError );

    /**
     * Leave the RHI canvas for #GAL_FALLBACK after its device could not be created or restored.
     * The user is told the first time it happens in a session.
     */
    bool abandonRhiCanvas( const wxString& aDetail );

    /**
     * Stop the RHI render thread and release its swapchain, mailbox and buffers. Must run before
     * the GAL_MODERN is destroyed, since in-flight uploads point at its geometry.
     */
    void teardownModernRenderInfra();

    /**
     * Return the native window handle for RHI surface creation, or an invalid handle until the
     * window is realized. It is fetched again only after swapchain recreation.
     */
    const RHI_NATIVE_WINDOW_HANDLE& getCachedNativeWindowHandle();
    void releaseCachedNativeWindowHandle();

    /**
     * Present the most recently submitted frame packet. An unmapped surface arms the refresh timer
     * instead.
     */
    void presentFrame();

    /**
     * The padded size of the backend window, which sits below the top overlay inset.
     */
    wxSize backendScreenSize() const;

    /**
     * Submit the last full scene again with the current camera and cursor, keeping all targets.
     */
    void submitCachedScene();

    wxWindow*                m_parent;           ///< Pointer to the parent window
    EDA_DRAW_FRAME*          m_edaFrame;         ///< Parent EDA_DRAW_FRAME (if available)

    std::chrono::steady_clock::time_point m_lastRepaintStart; ///< Timestamp of the last repaint start
    std::chrono::steady_clock::time_point m_lastRepaintEnd;   ///< Timestamp of the last repaint end
    wxTimer                               m_refreshTimer;     ///< Timer to prevent too-frequent refreshing

    std::mutex               m_refreshMutex;     ///< Blocks multiple calls to the draw

    /// True if GAL is currently redrawing the view
    bool                     m_drawing;

    /// Flag that determines if VIEW may use GAL for redrawing the screen.
    bool                     m_drawingEnabled;

    /// True when canvas needs to be refreshed from idle handler
    bool                     m_needIdleRefresh;

    /** The refresh timer was armed by Refresh() for a repaint it postponed */
    bool                     m_refreshDeferred = false;

    /// Last cursor position sent to GAL for drawing
    VECTOR2D                 m_lastCursorPosition;

    /// Interface for drawing objects on a 2D-surface
    KIGFX::GAL*              m_gal;

    /// Stores view settings (scale, center, etc.) and items to be drawn
    KIGFX::VIEW*             m_view;

    /// Contains information about how to draw items using GAL
    std::unique_ptr<KIGFX::PAINTER> m_painter;

    /// Control for VIEW (moving, zooming, etc.)
    KIGFX::WX_VIEW_CONTROLS* m_viewControls;

    /// Currently used GAL
    GAL_TYPE                    m_backend;
    KIGFX::GAL_DISPLAY_OPTIONS& m_options;

    /// Processes and forwards events to tools
    TOOL_DISPATCHER*         m_eventDispatcher;

    /// Flag to indicate that focus should be regained on the next mouse event. It is a workaround
    /// for cases when the panel loses keyboard focus, so it does not react to hotkeys anymore.
    bool                     m_lostFocus;

    /// What the last ZoomToFit() left, refitted on a resize until the view is zoomed or panned
    KIGFX::AUTOMATIC_FIT     m_automaticFit;

    /** Set after a GPU recovery attempt to prevent infinite retry loops */
    bool                     m_recoveryAttempted;

    /// Consecutive frames dropped because the GL context could not be made current
    int                      m_contextBindFailures;

    /// The GAL was rebuilt after a GPU reset and has not completed a frame yet
    bool                     m_rebuiltAfterReset;

    /// Set when a size change could not be applied because the GL context was unavailable
    bool                     m_pendingResize;

    /// Rows at the top of the panel left to the infobars where the backend window would hide them
    int                      m_overlayInsetTop = 0;

    /// Flag to indicate whether the panel should take focus at certain times (when moused over,
    /// and on various mouse/key events)
    bool                     m_stealsFocus;

    wxWindow*                m_statusPopup;

    /** Owns the RHI device and GPU context */
    std::unique_ptr<KIGFX::RENDER_THREAD> m_renderThread;

    /** The UI thread's access to the device. Must be destroyed before m_renderThread. */
    std::unique_ptr<KIGFX::RHI_DEVICE_PROXY> m_deviceProxy;

    std::unique_ptr<KIGFX::SWAPCHAIN_CONTROLLER> m_swapchainCtrl;

    /** Delivers frame packets to the render thread */
    std::unique_ptr<KIGFX::FRAME_MAILBOX> m_frameMailbox;

    /** Routes this window's work on the render thread */
    uint32_t m_windowId = 0;

    /** True until the first paint completes the deferred GTK drawing suppression */
    bool m_needsExternalRenderingSetup = false;

    /**
     * Fetching the handle forces a display-server flush, so it is cached between realizations and
     * fetched again only when m_nativeHandleValid is cleared on swapchain recreation.
     */
    RHI_NATIVE_WINDOW_HANDLE m_cachedNativeHandle;
    bool                     m_nativeHandleValid = false;

    /** What displayRefreshRate() last asked the window system */
    float                                 m_refreshRate = 0.0f;
    std::chrono::steady_clock::time_point m_refreshRateAsked;

    /** Draw descriptors of the last full redraw, reused by frames that need no new geometry */
    KIGFX::FRAME_DRAW_DESC m_lastFullStaticDesc;
    KIGFX::FRAME_DRAW_DESC m_lastFullDynamicDesc;
    bool m_hasLastFullDrawDesc = false;

    /** Content generations of the static and dynamic pools last submitted, see FRAME_PACKET */
    uint64_t m_staticEpoch = 1;
    uint64_t m_dynamicEpoch = 1;

    /** Fixed in the constructor from the parent frame's editor */
    KIGFX::RHI_LINE_PIPELINE_KIND m_linePipelineKind = KIGFX::RHI_LINE_PIPELINE_KIND::PCB_TRACKS;

    static std::atomic<uint32_t> s_nextWindowId;

    /// Optional overlay for drawing transient debug objects
    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_debugOverlay;

    std::unique_ptr<KIPLATFORM::UI::TOUCHPAD_GESTURE_HANDLER> m_touchpadGestureHandler;
};

#endif
