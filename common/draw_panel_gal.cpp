/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
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
#include <wx/dcclient.h>

#include <chrono>

#include <eda_draw_frame.h>
#include <kiface_base.h>
#include <macros.h>
#include <scoped_set_reset.h>
#include <settings/app_settings.h>
#include <trace_helpers.h>

#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <view/wx_view_controls.h>
#include <gal/painter.h>
#include <base_screen.h>
#include <gal/cursors.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/opengl/opengl_gal.h>
#include <gal/opengl/gpu_oom_error.h>
#include <gal/cairo/cairo_gal.h>
#include <gal/gal_modern.h>
#include <rhi/rhi_device.h>
#include <rhi/rhi_platform_device.h>
#include <rhi/render_thread.h>
#include <rhi/rhi_device_proxy.h>
#include <rhi/swapchain_controller.h>
#include <rhi/frame_mailbox.h>
#include <rhi/frame_packet.h>
#include <rhi/rhi_surface.h>
#ifdef __WXMAC__
#include <rhi/rhi_metal_view.h>
#endif
#include <advanced_config.h>
#include <frame_type.h>
#include <math/vector2wx.h>


#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>

#include <widgets/wx_infobar.h>

#include <kiplatform/touchpad.h>
#include <kiplatform/ui.h>

#include <atomic>
#include <stdexcept>

#include <core/profile.h>

#include <wx/display.h>
#include <wx/utils.h>

#include <pgm_base.h>
#include <confirm.h>


/**
 * Flag to enable drawing panel debugging output.
 *
 * @ingroup trace_env_vars
 */
[[maybe_unused]] static const wxChar traceDrawPanel[] = wxT( "KICAD_DRAW_PANEL" );

// Counts inputs to any canvas so the render thread never holds back a frame that answers one
static std::atomic<uint64_t> s_inputSerial{ 0 };

std::atomic<uint32_t> EDA_DRAW_PANEL_GAL::s_nextWindowId{ 1 };


static KIGFX::RHI_LINE_PIPELINE_KIND DetermineLinePipelineKind( const EDA_DRAW_FRAME* aFrame )
{
    if( !aFrame )
        return KIGFX::RHI_LINE_PIPELINE_KIND::PCB_TRACKS;

    switch( aFrame->GetFrameType() )
    {
    case FRAME_SCH:
    case FRAME_SCH_SYMBOL_EDITOR:
    case FRAME_SCH_VIEWER:
    case FRAME_SCH_DIFF:
        return KIGFX::RHI_LINE_PIPELINE_KIND::SCH_WIRES;

    default:
        return KIGFX::RHI_LINE_PIPELINE_KIND::PCB_TRACKS;
    }
}


static RHI_NATIVE_WINDOW_HANDLE GetNativeWindowHandle( wxWindow* aWindow )
{
    static_assert( static_cast<int>( RHI_DISPLAY_TYPE::COCOA ) == KIPLATFORM::UI::DISPLAY_COCOA );

    RHI_NATIVE_WINDOW_HANDLE handle;
    int                      displayType = KIPLATFORM::UI::DISPLAY_UNKNOWN;

    KIPLATFORM::UI::GetNativeWindowHandles( aWindow, &handle.nativeHandle, &handle.nativeDisplay, &displayType );
    handle.displayType = static_cast<RHI_DISPLAY_TYPE>( displayType );
    return handle;
}


static void ApplyAntialiasingMode( KIGFX::FRAME_DRAW_DESC& aDesc,
                                   KIGFX::GAL_ANTIALIASING_MODE aMode )
{
    // Fast relies on the shaders' analytic edge coverage, High Quality supersamples the layer FBOs
    aDesc.supersampleFactor = aMode == KIGFX::GAL_ANTIALIASING_MODE::AA_HIGHQUALITY ? 2u : 1u;
}


const RHI_NATIVE_WINDOW_HANDLE& EDA_DRAW_PANEL_GAL::getCachedNativeWindowHandle()
{
    if( !m_nativeHandleValid )
    {
        m_cachedNativeHandle = GetNativeWindowHandle( this );

#ifdef __WXMAC__
        if( m_cachedNativeHandle.displayType == RHI_DISPLAY_TYPE::COCOA
            && m_cachedNativeHandle.nativeHandle )
        {
            RHI_METAL_HOST_VIEW host = RhiMetalAttachHostView( m_cachedNativeHandle.nativeHandle );
            m_cachedNativeHandle.nativeHandle  = host.view;
            m_cachedNativeHandle.nativeDisplay = host.layer;
        }
#endif

        // A paint before the window is realized gets an invalid handle, so retry next frame
        m_nativeHandleValid = m_cachedNativeHandle.IsValid();
    }

    return m_cachedNativeHandle;
}


void EDA_DRAW_PANEL_GAL::releaseCachedNativeWindowHandle()
{
#ifdef __WXMAC__
    if( m_cachedNativeHandle.displayType == RHI_DISPLAY_TYPE::COCOA )
        RhiMetalDetachHostView( m_cachedNativeHandle.nativeHandle );
#endif

    m_cachedNativeHandle = {};
    m_nativeHandleValid = false;
}


void EDA_DRAW_PANEL_GAL::presentFrame()
{
    if( !m_swapchainCtrl )
        return;

    const wxSize cs = backendScreenSize();
    float dpiScale = static_cast<float>( GetContentScaleFactor() );
    const RHI_NATIVE_WINDOW_HANDLE& nativeHandle = getCachedNativeWindowHandle();

    // An unrealized window's handle would fail surface creation on the render thread
    if( nativeHandle.IsValid() )
    {
        // GTK drawing suppression needs the realized widget at its final layout size
        if( m_needsExternalRenderingSetup )
        {
            wxLogTrace( traceRhi, wxS( "DrawPanel: deferred PrepareForExternalRendering "
                                        "windowId=%u" ), m_windowId );
            KIPLATFORM::UI::PrepareForExternalRendering( this );
            m_needsExternalRenderingSetup = false;
        }

        // Caps a window that presents without vsync at one frame per display refresh
        m_swapchainCtrl->SetRefreshRate( displayRefreshRate() );
        m_swapchainCtrl->OnPaint( nativeHandle,
                                  static_cast<uint32_t>( cs.x ),
                                  static_cast<uint32_t>( cs.y ),
                                  dpiScale );
    }
    else if( IsShown() && !m_refreshTimer.IsRunning() )
    {
        // A timer rather than an idle refresh, so a surface that never maps cannot spin
        m_refreshTimer.StartOnce( 100 );
    }
}


EDA_DRAW_PANEL_GAL::EDA_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
        wxScrolledCanvas( aParentWindow, aWindowId, aPosition, aSize ),
        m_MouseCapturedLost( false ),
        m_parent( aParentWindow ),
        m_edaFrame( nullptr ),
        m_drawing( false ),
        m_drawingEnabled( false ),
        m_needIdleRefresh( false ),
        m_gal( nullptr ),
        m_view( nullptr ),
        m_painter( nullptr ),
        m_viewControls( nullptr ),
        m_backend( GAL_TYPE_NONE ),
        m_options( aOptions ),
        m_eventDispatcher( nullptr ),
        m_lostFocus( false ),
        m_recoveryAttempted( false ),
        m_contextBindFailures( 0 ),
        m_rebuiltAfterReset( false ),
        m_pendingResize( false ),
        m_stealsFocus( true ),
        m_statusPopup( nullptr )
{
#ifdef _WIN32
    // need to fix broken cairo rendering on Windows with wx 3.3
    SetDoubleBuffered( false );
#endif
    m_PaintEventCounter = std::make_unique<PROF_COUNTER>( "Draw panel paint events" );

    if( Pgm().GetCommonSettings()->m_Appearance.show_scrollbars )
        ShowScrollbars( wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS );
    else
        ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );

    SetLayoutDirection( wxLayout_LeftToRight );

    m_edaFrame = dynamic_cast<EDA_DRAW_FRAME*>( m_parent );

    // If we're in a dialog, we have to go looking for our parent frame
    if( !m_edaFrame )
    {
        wxWindow* ancestor = aParentWindow->GetParent();

        while( ancestor && !dynamic_cast<EDA_DRAW_FRAME*>( ancestor ) )
            ancestor = ancestor->GetParent();

        if( ancestor )
            m_edaFrame = dynamic_cast<EDA_DRAW_FRAME*>( ancestor );
    }

    m_linePipelineKind = DetermineLinePipelineKind( m_edaFrame );

    // Callers ask for the modern canvas before the saved choice is known. Starting it only to
    // switch away costs a GPU device, so other choices start on the fallback and the frame applies them.
    if( aGalType == GAL_TYPE_MODERN )
    {
        COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

        if( !cfg || ResolveStoredCanvasType( cfg->m_Graphics.canvas_type ) != GAL_TYPE_MODERN )
            aGalType = GAL_FALLBACK;
    }

    SwitchBackend( aGalType );
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );

    EnableScrolling( false, false ); // otherwise Zoom Auto disables GAL canvas
    KIPLATFORM::UI::SetOverlayScrolling( this, false ); // Prevent excessive repaint on GTK
    KIPLATFORM::UI::ImmControl( this, false ); // Ensure our panel can't suck in IME events

    Connect( wxEVT_SIZE, wxSizeEventHandler( EDA_DRAW_PANEL_GAL::onSize ), nullptr, this );
    Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( EDA_DRAW_PANEL_GAL::onEnter ), nullptr, this );
    Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( EDA_DRAW_PANEL_GAL::onLostFocus ), nullptr, this );

    const wxEventType events[] = {
        // Binding both EVT_CHAR and EVT_CHAR_HOOK ensures that all key events,
        // especially special key like arrow keys, are handled by the GAL event dispatcher,
        // and not sent to GUI without filtering, because they have a default action (scroll)
        // that must not be called.
        wxEVT_LEFT_UP,
        wxEVT_LEFT_DOWN,
        wxEVT_LEFT_DCLICK,
        wxEVT_RIGHT_UP,
        wxEVT_RIGHT_DOWN,
        wxEVT_RIGHT_DCLICK,
        wxEVT_MIDDLE_UP,
        wxEVT_MIDDLE_DOWN,
        wxEVT_MIDDLE_DCLICK,
        wxEVT_AUX1_UP,
        wxEVT_AUX1_DOWN,
        wxEVT_AUX1_DCLICK,
        wxEVT_AUX2_UP,
        wxEVT_AUX2_DOWN,
        wxEVT_AUX2_DCLICK,
        wxEVT_MOTION,
        wxEVT_MOUSEWHEEL,
        wxEVT_CHAR,
        wxEVT_CHAR_HOOK,
        wxEVT_MAGNIFY,
        KIGFX::WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE
    };

    for( wxEventType eventType : events )
        Connect( eventType, wxEventHandler( EDA_DRAW_PANEL_GAL::OnEvent ), nullptr, m_eventDispatcher );

    // Set up timer to detect when drawing starts
    m_refreshTimer.SetOwner( this );
    Connect( m_refreshTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler( EDA_DRAW_PANEL_GAL::onRefreshTimer ),
             nullptr, this );

    Connect( wxEVT_SHOW, wxShowEventHandler( EDA_DRAW_PANEL_GAL::onShowEvent ), nullptr, this );
}


EDA_DRAW_PANEL_GAL::~EDA_DRAW_PANEL_GAL()
{
    wxLogTrace( traceRhi, wxS( "DrawPanel: destructor windowId=%u backend=%d" ),
                m_windowId, static_cast<int>( m_backend ) );

    m_touchpadGestureHandler.reset();

    // Ensure EDA_DRAW_PANEL_GAL::onShowEvent is not fired during Dtor process
    Disconnect( wxEVT_SHOW, wxShowEventHandler( EDA_DRAW_PANEL_GAL::onShowEvent ) );
    StopDrawing();

    wxASSERT( !m_drawing );

    // Deleting the view can retire GAL groups, which needs the device proxy
    delete m_viewControls;
    delete m_view;

    teardownModernRenderInfra();

    delete m_gal;
    m_gal = nullptr;
}


void EDA_DRAW_PANEL_GAL::SetFocus()
{
    KIPLATFORM::UI::ImeNotifyCancelComposition( this );
    wxScrolledCanvas::SetFocus();
    m_lostFocus = false;
}


void EDA_DRAW_PANEL_GAL::onPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    // Consuming the paint region stops GTK3 clearing the modern canvas between presents. The other
    // backends repost their child's paint event here with no native paint context, where a wxPaintDC asserts.
    std::optional<wxPaintDC> dc;

    if( m_backend == GAL_TYPE_MODERN )
        dc.emplace( this );

    DoRePaint( false );
}


EDA_DRAW_PANEL_GAL::GAL_TYPE EDA_DRAW_PANEL_GAL::CanvasAfterFailure( GAL_TYPE aFailedCanvasType )
{
    if( IsAccelerated( aFailedCanvasType ) && aFailedCanvasType != GAL_FALLBACK )
        return GAL_FALLBACK;

    return GAL_TYPE_NONE;
}


bool EDA_DRAW_PANEL_GAL::abandonRhiCanvas( const wxString& aDetail )
{
    if( !EDA_DRAW_FRAME::CanvasFailed( GAL_TYPE_MODERN ) )
    {
        EDA_DRAW_FRAME::SetCanvasFailed( GAL_TYPE_MODERN );

#ifdef __WXMAC__
        showMessageLater( _( "Could not use Metal, switching to OpenGL" ), aDetail, false );
#else
        showMessageLater( _( "Could not use Vulkan, falling back to software rendering" ), aDetail, false );
#endif
    }

    wxLogTrace( traceRhi, wxS( "DrawPanel: leaving the RHI canvas for backend %d" ), static_cast<int>( GAL_FALLBACK ) );

    return SwitchBackend( GAL_FALLBACK );
}


void EDA_DRAW_PANEL_GAL::teardownModernRenderInfra()
{
    if( m_swapchainCtrl )
    {
        m_swapchainCtrl->OnCloseSync();
        m_swapchainCtrl.reset();
    }

    if( auto* galModern = dynamic_cast<KIGFX::GAL_MODERN*>( m_gal ) )
        galModern->ReleaseBuffers();

    // The proxy references the render thread. ReleaseBuffers() left no proxy call outstanding.
    m_deviceProxy.reset();

    // Joining the render thread keeps queued uploads from reading the GAL's geometry after it goes
    m_renderThread.reset();
    m_frameMailbox.reset();
    m_hasLastFullDrawDesc = false;

    releaseCachedNativeWindowHandle();
}


bool EDA_DRAW_PANEL_GAL::recoverFromGalError( const std::exception& aError )
{
    // Reinitializing would hit the same VRAM ceiling, so out-of-memory goes straight to the fallback
    const bool gpuOutOfMemory = dynamic_cast<const KIGFX::GPU_OOM_ERROR*>( &aError ) != nullptr;

    // The reinit below rewrites m_backend
    const GAL_TYPE failedBackend = m_backend;

    try
    {
        // Reset detection only runs inside LockContext, and this frame's lock came before the reset
        if( m_gal && !m_gal->IsContextLocked() )
            KIGFX::GAL_CONTEXT_LOCKER probe( m_gal );

        // A reset seen mid-frame surfaces as a GL error; rebuild rather than spend the one-shot recovery
        if( handleContextLoss() )
            return true;

        // Sleep/wake and GPU resets can invalidate the entire GL/device context.
        // Try a full reinit of the current backend before falling back.
        if( !gpuOutOfMemory && !m_recoveryAttempted )
        {
            m_recoveryAttempted = true;
            m_backend = GAL_TYPE_NONE;

            if( SwitchBackend( failedBackend ) )
            {
                StartDrawing();
                return true;
            }
        }

        if( failedBackend == GAL_TYPE_MODERN )
        {
            m_recoveryAttempted = false;
            abandonRhiCanvas( wxString( aError.what() ) );
            StartDrawing();
            return true;
        }

        if( CanvasAfterFailure( failedBackend ) != GAL_TYPE_NONE )
        {
            m_recoveryAttempted = false;
            SwitchBackend( GAL_FALLBACK );

            if( m_rebuiltAfterReset )
            {
                // Some drivers refuse new framebuffers to a process for the rest of its life after a reset
                m_rebuiltAfterReset = false;
                EDA_DRAW_FRAME::SetCanvasFailed( GAL_TYPE_OPENGL );
                showMessageLater( _( "The graphics driver was reset and OpenGL could not be restored" ),
                                  _( "Restart KiCad to use accelerated graphics again." ), false );
            }
            else
            {
                showMessageLater( _( "Could not use OpenGL, falling back to software rendering" ),
                                  wxString( aError.what() ), false );
            }

            StartDrawing();
            return true;
        }

        showMessageLater( _( "Graphics error" ), wxString( aError.what() ), true );
    }
    catch( std::exception& recoveryErr )
    {
        showMessageLater( _( "Graphics error during recovery" ), wxString( recoveryErr.what() ), true );
    }
    catch( ... )
    {
        showMessageLater( _( "Graphics error during recovery" ), _( "Unknown exception during backend switch" ),
                          true );
    }

    return false;
}


bool EDA_DRAW_PANEL_GAL::handleContextLoss()
{
    KIGFX::GAL_CONTEXT_LOSS loss = m_gal ? m_gal->GetContextLoss() : KIGFX::GAL_CONTEXT_LOSS::NONE;

    if( loss == KIGFX::GAL_CONTEXT_LOSS::NONE )
        return false;

    // Deleting a GAL that someone still holds locked would leave them unlocking freed memory
    if( m_gal->IsContextLocked() )
        return true;

    // Hidden canvases rebuild when next shown, since a new GAL cannot initialize off screen
    if( !IsShownOnScreen() )
        return true;

    // Rebuilding while the driver is still resetting can fail or hang, so poll until it finishes
    if( !m_gal->IsResetSettled() )
    {
        m_refreshTimer.StartOnce( 20 );
        return true;
    }

    wxLogTrace( traceGalContext, wxS( "Rebuilding canvas %p after GPU reset, repeated %d" ), this,
                loss == KIGFX::GAL_CONTEXT_LOSS::REPEATED ? 1 : 0 );

    bool rebuilt = false;

    if( loss == KIGFX::GAL_CONTEXT_LOSS::RECOVERABLE || !GAL_FALLBACK_AVAILABLE )
    {
        GAL_TYPE backend = m_backend;

        // Forces SwitchBackend to build a new GAL of the same type
        m_backend = GAL_TYPE_NONE;
        rebuilt = SwitchBackend( backend );
        m_rebuiltAfterReset = rebuilt;
    }

    // SwitchBackend already reported a failed rebuild, so only a repeated loss needs the notice
    if( !rebuilt && GAL_FALLBACK_AVAILABLE )
    {
        static bool s_notified = false;

        wxLogTrace( traceGalContext, wxS( "Canvas %p falling back to software rendering" ), this );
        EDA_DRAW_FRAME::SetCanvasFailed( GAL_TYPE_OPENGL );
        SwitchBackend( GAL_FALLBACK );

        if( loss == KIGFX::GAL_CONTEXT_LOSS::REPEATED && !s_notified )
        {
            s_notified = true;
            showMessageLater( _( "The graphics driver keeps resetting, falling back to software rendering" ),
                              wxEmptyString, false );
        }
    }

    StartDrawing();
    return true;
}


void EDA_DRAW_PANEL_GAL::showMessageLater( const wxString& aTitle, const wxString& aDetail, bool aError )
{
    wxWindow* parent = m_parent;

    CallAfter(
            [parent, aTitle, aDetail, aError]()
            {
                if( aError )
                    DisplayErrorMessage( parent, aTitle, aDetail );
                else
                    DisplayInfoMessage( parent, aTitle, aDetail );
            } );
}


/// Frames to drop before giving up on the GL context and letting the backend recover
static constexpr int MAX_CONTEXT_BIND_RETRIES = 2;


void EDA_DRAW_PANEL_GAL::submitCachedScene()
{
    auto* galModern = static_cast<KIGFX::GAL_MODERN*>( m_gal );
    auto  packet = std::make_unique<KIGFX::FRAME_PACKET>();

    packet->drawDesc = m_lastFullStaticDesc;
    packet->drawDesc.reuseStaticFBO = true;

    // The cached desc holds the camera of the last full render, which would snap a pan or zoom back
    galModern->PopulateCameraUBOs( packet->drawDesc );
    galModern->PopulateCursorUBO( packet->drawDesc );

    // Unchanged epochs tell the render thread to keep both geometry FBOs
    packet->staticDrawDesc  = m_lastFullStaticDesc;
    packet->dynamicDrawDesc = m_lastFullDynamicDesc;
    packet->staticChanged   = true;
    packet->dynamicChanged  = true;
    packet->staticEpoch     = m_staticEpoch;
    packet->dynamicEpoch    = m_dynamicEpoch;

    packet->inputSerial = s_inputSerial.load( std::memory_order_relaxed );
    m_frameMailbox->Submit( std::move( packet ) );
}


bool EDA_DRAW_PANEL_GAL::DoRePaint( bool aAllowSkip )
{
    if( !m_refreshMutex.try_lock() )
    {
        wxLogTrace( traceRhi, wxS( "DrawPanel: DoRePaint SKIP (mutex locked)" ) );
        return false;
    }

    std::lock_guard<std::mutex> lock( m_refreshMutex, std::adopt_lock );

    if( !m_drawingEnabled )
        return false;

    // No surface may be created on an unmapped window. The other backends check visibility below.
    if( m_backend == GAL_TYPE_MODERN && !IsShown() )
    {
        wxLogTrace( traceRhi, wxS( "DrawPanel: DoRePaint SKIP (hidden) windowId=%u" ), m_windowId );
        return false;
    }

    if( handleContextLoss() )
        return false;

    if( m_backend == GAL_TYPE_MODERN && m_renderThread && m_renderThread->IsDeviceLost() )
    {
        // A device lost before it ever presented gives way to the fallback rather than retrying forever
        if( m_renderThread->HasPresented() )
        {
            m_backend = GAL_TYPE_NONE;

            if( SwitchBackend( GAL_TYPE_MODERN ) )
            {
                StartDrawing();
                return false;
            }
        }

        abandonRhiCanvas( _( "The graphics device was lost and could not be restored." ) );
        StartDrawing();
        return false;
    }

    if( !m_gal->IsInitialized() || !m_gal->IsVisible() || m_gal->IsContextLocked() )
    {
        wxLogTrace( traceRhi, wxS( "DrawPanel: DoRePaint SKIP (init=%d vis=%d locked=%d)" ),
                    m_gal->IsInitialized(), m_gal->IsVisible(), m_gal->IsContextLocked() );
        return false;
    }

    if( m_drawing )
    {
        wxLogTrace( traceRhi, wxS( "DrawPanel: DoRePaint SKIP (already drawing)" ) );
        return false;
    }

    // The context may have become current since the size change was deferred
    if( m_pendingResize )
        ResizeGal();

    m_lastRepaintStart = std::chrono::steady_clock::now();

    // Update current zoom settings if the canvas is managed by a EDA frame
    // (i.e. not by a preview panel in a dialog)
    if( !IsDialogPreview() && GetParentEDAFrame() && GetParentEDAFrame()->GetScreen() )
        GetParentEDAFrame()->GetScreen()->m_ScrollCenter = GetView()->GetCenter();

    if( Pgm().GetCommonSettings()->m_Appearance.show_scrollbars )
        m_viewControls->UpdateScrollbars();

#ifdef KICAD_GAL_PROFILE
    latencyProbeZoomToRender.Checkpoint("do-repaint-start");
#endif

    SCOPED_SET_RESET<bool> drawing( m_drawing, true );

    ( *m_PaintEventCounter )++;

    wxASSERT( m_painter );

    KIGFX::RENDER_SETTINGS* settings =
            static_cast<KIGFX::RENDER_SETTINGS*>( m_painter->GetSettings() );

    PROF_TIMER cntUpd("view-upd-items", false);
    PROF_TIMER cntTotal("view-total", false);
    PROF_TIMER cntCtx("view-context-create", false);
    PROF_TIMER cntCtxDestroy("view-context-destroy", false);
    PROF_TIMER cntRedraw("view-redraw-rects", false);

    bool isDirty = false;

    if( m_backend == GAL_TYPE_MODERN && m_frameMailbox )
    {
        static_cast<KIGFX::GAL_MODERN*>( m_gal )->ReleaseRetiredBuffers(
                m_frameMailbox->ConsumedBufferGeneration() );
    }

    cntTotal.Start();

    try
    {
        beforeRedraw();

        VECTOR2D cursorPos = m_viewControls->GetCursorPosition();
        bool viewDirty = m_view->IsDirty();
        bool cursorMoved = ( cursorPos != m_lastCursorPosition );
        bool hasPendingItemUpdates = m_view->HasPendingItemUpdates();

        // Skip all update work when nothing has changed since the previous frame.
        // Never skip when responding to a native paint event or explicit ForceRefresh
        // because the window content may have been invalidated by the OS.
        if( aAllowSkip && !viewDirty && !cursorMoved && !hasPendingItemUpdates )
        {
            m_lastRepaintEnd = std::chrono::steady_clock::now();
            return true;
        }

        if( hasPendingItemUpdates )
        {
            cntUpd.Start();

            try
            {
                m_view->UpdateItems();
            }
            catch( std::out_of_range& err )
            {
                // Don't do anything here but don't fail
                // This can happen when we don't catch `at()` calls
                wxLogTrace( traceDrawPanel, wxS( "Out of Range error: %s" ), err.what() );
            }
            catch( std::runtime_error& err )
            {
                // Handle GL errors (e.g. glMapBuffer failure) that surface during UpdateItems().
                // These can occur on macOS under memory pressure when embedding large 3D models.
                // Log and continue so the outer handler can decide whether to switch backends.
                wxLogTrace( traceDrawPanel, wxS( "Runtime error during UpdateItems: %s" ),
                            err.what() );
                throw;
            }

            cntUpd.Stop();
            viewDirty = m_view->IsDirty();
        }

        // After processing item updates, skip the GL cycle when neither the
        // view targets nor the cursor position have changed.
        if( aAllowSkip && !viewDirty && !cursorMoved )
        {
            m_lastRepaintEnd = std::chrono::steady_clock::now();
            return true;
        }

        m_lastCursorPosition = cursorPos;

        // A cursor move alone reuses the cached scene and skips the view traversal and upload
        const bool useOverlayFastPath = m_backend == GAL_TYPE_MODERN
                             && m_hasLastFullDrawDesc
                             && !viewDirty
                             && cursorMoved;

        // A zoom can carry groups across their level of detail
        if( m_backend == GAL_TYPE_MODERN )
            m_view->SyncRetainedScene();

        // Grid items are thinned and sized for the zoom they were made at, so they need a full redraw
        const bool useCameraFastPath = m_backend == GAL_TYPE_MODERN
                            && m_hasLastFullDrawDesc
                            && viewDirty
                            && m_view->IsCameraDirtyOnly()
                            && !m_gal->HasStaticRebuildPending()
                            && m_gal->GetGridSources().empty();

        if( useOverlayFastPath && m_frameMailbox && m_swapchainCtrl )
        {
            m_gal->SetCursorColor( settings->GetCursorColor() );
            m_gal->DrawCursor( cursorPos );
            submitCachedScene();
            presentFrame();

            m_lastRepaintEnd = std::chrono::steady_clock::now();

            m_recoveryAttempted = false;
            return true;
        }

        if( useCameraFastPath && m_frameMailbox && m_swapchainCtrl )
        {
            // GetDrawDesc() is unusable here because reused-scene frames overwrite the ring-buffered state
            auto* galModern = static_cast<KIGFX::GAL_MODERN*>( m_gal );

            m_gal->SetClearColor( settings->GetBackgroundColor() );
            m_gal->SetGridColor( settings->GetGridColor() );
            m_gal->SetCursorColor( settings->GetCursorColor() );
            m_gal->DrawCursor( cursorPos );

            if( galModern->SyncScene() )
            {
                KIGFX::FRAME_DRAW_DESC patched = galModern->GetStaticDrawDesc();
                patched.linePipelineKind  = m_lastFullStaticDesc.linePipelineKind;
                patched.supersampleFactor = m_lastFullStaticDesc.supersampleFactor;
                m_lastFullStaticDesc = patched;
                m_staticEpoch++;
            }

            auto packet = std::make_unique<KIGFX::FRAME_PACKET>();
            packet->drawDesc = m_lastFullStaticDesc;
            packet->drawDesc.reuseStaticFBO = false;
            packet->drawDesc.linePipelineKind = m_linePipelineKind;
            ApplyAntialiasingMode( packet->drawDesc, m_options.antialiasing_mode );

            galModern->PopulateCameraUBOs( packet->drawDesc );
            galModern->PopulateCursorUBO( packet->drawDesc );

            packet->staticDrawDesc  = m_lastFullStaticDesc;
            packet->dynamicDrawDesc = m_lastFullDynamicDesc;

            // The render thread decides reuse from each target desc's own flag and camera, not
            // drawDesc's, so stale ones would revert the canvas to the old zoom
            packet->staticDrawDesc.reuseStaticFBO   = false;
            packet->dynamicDrawDesc.reuseDynamicFBO = false;
            galModern->PopulateCameraUBOs( packet->staticDrawDesc );
            galModern->PopulateCameraUBOs( packet->dynamicDrawDesc );

            packet->staticChanged   = true;
            packet->dynamicChanged  = true;
            packet->staticEpoch     = m_staticEpoch;
            packet->dynamicEpoch    = m_dynamicEpoch;

            // A following cursor-only frame reuses these descs, which must not keep the old view
            m_lastFullStaticDesc.frameUBO    = packet->staticDrawDesc.frameUBO;
            m_lastFullStaticDesc.gridUBO     = packet->staticDrawDesc.gridUBO;
            m_lastFullStaticDesc.cursorUBO   = packet->staticDrawDesc.cursorUBO;
            m_lastFullDynamicDesc.frameUBO   = packet->dynamicDrawDesc.frameUBO;
            m_lastFullDynamicDesc.gridUBO    = packet->dynamicDrawDesc.gridUBO;
            m_lastFullDynamicDesc.cursorUBO  = packet->dynamicDrawDesc.cursorUBO;

            m_view->MarkClean();

            packet->inputSerial = s_inputSerial.load( std::memory_order_relaxed );
            m_frameMailbox->Submit( std::move( packet ) );

            presentFrame();

            m_lastRepaintEnd = std::chrono::steady_clock::now();
            m_recoveryAttempted = false;
            return true;
        }

        // GAL_DRAWING_CONTEXT can throw in the dtor, so we need to scope
        // the full lifetime inside the try block
        {
            cntCtx.Start();
            KIGFX::GAL_DRAWING_CONTEXT ctx( m_gal );
            cntCtx.Stop();

            if( !ctx.IsDrawing() )
            {
                // A canvas being torn down never reaches here; DoRePaint returns above once
                // the window stops being visible.  So repeated failures mean a live canvas
                // whose context is gone for good, and retrying forever would leave it blank
                if( ++m_contextBindFailures > MAX_CONTEXT_BIND_RETRIES )
                    throw std::runtime_error( "Could not make the OpenGL context current" );

                RequestRefresh();
                return false;
            }

            if( m_view->IsTargetDirty( KIGFX::TARGET_OVERLAY )
                && !m_gal->HasTarget( KIGFX::TARGET_OVERLAY ) )
            {
                m_view->MarkDirty();
            }

            m_gal->SetClearColor( settings->GetBackgroundColor() );
            m_gal->SetGridColor( settings->GetGridColor() );
            m_gal->SetCursorColor( settings->GetCursorColor() );

            // The OpenGL back buffer is undefined after SwapBuffers, even on a cursor-only repaint
            if( m_backend == GAL_TYPE_OPENGL )
                m_gal->ClearScreen();

            if( m_view->IsDirty() )
            {
                // Cairo composites all targets into one surface, so a dirty NONCACHED needs a full clear
                if( !IsAccelerated( m_backend ) && m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED ) )
                {
                    m_gal->ClearScreen();
                }

                // A rebuild the backend asked for itself also redraws the whole non-cached target
                if( m_gal->HasStaticRebuildPending() )
                {
                    m_view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
                    m_view->MarkTargetDirty( KIGFX::TARGET_OVERLAY );
                }

                // A retained scene keeps NONCACHED through a pan or zoom, but the grid is clipped to the old view
                const bool viewMoved = m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED );

                m_view->ClearTargets();

                // Grid has to be redrawn only when the NONCACHED target is redrawn
                if( viewMoved || m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED )
                        || m_gal->RefillsNonCachedTargets() )
                {
                    prepareGridSources();
                    m_gal->DrawGrid();
                }

                cntRedraw.Start();
                m_view->Redraw();
                cntRedraw.Stop();
                isDirty = true;
            }

            m_gal->DrawCursor( cursorPos );

	    #ifdef KICAD_GAL_PROFILE
	    latencyProbeZoomToRender.Checkpoint("do-repaint-pre-ctx-destroy");
	    #endif


            cntCtxDestroy.Start();
        }

        // ctx goes out of scope here so destructor would be called
        cntCtxDestroy.Stop();

#ifdef KICAD_GAL_PROFILE
        latencyProbeZoomToRender.Checkpoint("do-repaint-ctx-done");
#endif

        // A clean view leaves GetDrawDesc() empty, which would blank the static FBO
        if( m_frameMailbox && m_swapchainCtrl )
        {
            auto* galModern = static_cast<KIGFX::GAL_MODERN*>( m_gal );

            if( !isDirty && m_hasLastFullDrawDesc && m_backend == GAL_TYPE_MODERN )
            {
                submitCachedScene();
            }
            else if( isDirty )
            {
                auto packet = std::make_unique<KIGFX::FRAME_PACKET>();

                packet->drawDesc = galModern->GetDrawDesc();
                packet->drawDesc.linePipelineKind = m_linePipelineKind;
                ApplyAntialiasingMode( packet->drawDesc, m_options.antialiasing_mode );

                // Lets the first frame show the cursor without waiting for a move
                galModern->PopulateCursorUBO( packet->drawDesc );

                packet->staticDrawDesc  = galModern->GetStaticDrawDesc();
                packet->dynamicDrawDesc = galModern->GetDynamicDrawDesc();
                packet->staticDrawDesc.linePipelineKind  = m_linePipelineKind;
                packet->dynamicDrawDesc.linePipelineKind = m_linePipelineKind;
                packet->staticDrawDesc.supersampleFactor  = packet->drawDesc.supersampleFactor;
                packet->dynamicDrawDesc.supersampleFactor = packet->drawDesc.supersampleFactor;

                packet->staticChanged  = galModern->TakeStaticChanged();
                packet->dynamicChanged = true;

                if( packet->staticChanged )
                    m_staticEpoch++;

                m_dynamicEpoch++;
                packet->staticEpoch  = m_staticEpoch;
                packet->dynamicEpoch = m_dynamicEpoch;

                m_lastFullStaticDesc  = packet->staticDrawDesc;
                m_lastFullDynamicDesc = packet->dynamicDrawDesc;
                m_hasLastFullDrawDesc = true;

                packet->inputSerial = s_inputSerial.load( std::memory_order_relaxed );
                m_frameMailbox->Submit( std::move( packet ) );
            }

            presentFrame();
        }

        m_recoveryAttempted = false;
        m_contextBindFailures = 0;
        m_rebuiltAfterReset = false;
    }
    catch( std::exception& err )
    {
        wxLogTrace( traceDrawPanel, wxS( "DoRePaint exception: %s" ), err.what() );

        if( recoverFromGalError( err ) )
            return true;

        StopDrawing();
    }
    catch( ... )
    {
        showMessageLater( _( "Graphics error" ), _( "Unknown exception" ), true );
        StopDrawing();
    }

    if( isDirty )
    {
#ifdef KICAD_GAL_PROFILE
        wxLogTrace( traceGalProfile, "View timing: %s %s %s %s %s",
            cntTotal.to_string(),
            cntUpd.to_string(),
            cntRedraw.to_string(),
            cntCtx.to_string(),
            cntCtxDestroy.to_string()
        );
#endif
    }

    m_lastRepaintEnd = std::chrono::steady_clock::now();

#ifdef KICAD_GAL_PROFILE
    wxLogTrace( traceGalProfile, "%s", latencyProbeZoomToRender.to_string() );
    latencyProbeRepaintToMotion.Reset();
    latencyProbeRepaintToMotion.Checkpoint("repaint-done");
#endif    

    return true;
}


void EDA_DRAW_PANEL_GAL::onSize( wxSizeEvent& aEvent )
{
    ResizeGal();

    // The first fit often runs before the frame has its final layout, and window managers,
    // infobars and panels resize the canvas later. Until someone zooms or pans, fit again.
    wxSize clientSize = GetClientSize();

    if( clientSize.x > 0 && clientSize.y > 0 && m_view
        && m_automaticFit.Holds( m_view->GetScale(), m_view->GetCenter() ) )
    {
        ZoomToFit( m_automaticFit.GetBox(), m_automaticFit.GetMarginScale() );
    }
}


void EDA_DRAW_PANEL_GAL::ZoomToFit( const BOX2I& aBox, std::optional<double> aMarginScale )
{
    KIGFX::VIEW* view = m_view;
    BOX2I        bBox = aBox;

    // The GAL's world units may have changed since the view was attached.
    m_gal->ComputeWorldScreenMatrix();

    // Avoid a temporary zoom that dirties retained targets before the fitted scale is known.
    VECTOR2D screenSize = view->ToWorld( ToVECTOR2I( GetClientSize() ), false ) * view->GetScale();

    // A box with no extent (an empty screen or a lone point) has nothing to fit
    if( bBox.GetWidth() == 0 && bBox.GetHeight() == 0 )
        bBox = GetDefaultViewBBox();

    VECTOR2D vsize = bBox.GetSize();
    double scale = 1.0 / std::max( fabs( vsize.x / screenSize.x ),
                                   fabs( vsize.y / screenSize.y ) );

    // if the scale isn't finite (most likely due to an empty canvas)
    // simply just make sure we are centered and quit out of trying to zoom to fit
    if( !std::isfinite( scale ) )
    {
        view->SetCenter( VECTOR2D( 0, 0 ) );
        m_automaticFit.Clear();
        Refresh();
        return;
    }

    // Reserve enough margin to limit the amount of the view that might be obscured behind the
    // infobar.
    double margin_scale_factor = aMarginScale.value_or( GetClientSize().y < 768 ? 1.10 : 1.04 );

    view->SetScale( scale / margin_scale_factor );
    view->SetCenter( bBox.Centre() );
    m_automaticFit.Record( aBox, aMarginScale, view->GetScale(), view->GetCenter() );
    Refresh();
}


int EDA_DRAW_PANEL_GAL::ScreenSizePad( GAL_TYPE aBackend )
{
    // A swapchain that does not match its surface rescales and blurs every frame
    if( aBackend == GAL_TYPE_MODERN )
        return 0;

#ifdef __WXMAC__
    // The aligned GL child covers the client area without the offset pixel
    if( aBackend == GAL_TYPE_OPENGL )
        return 0;
#endif

    // The ( +1, +1 ) prevents an ugly black line on right and bottom on Mac
    return 1;
}


wxSize EDA_DRAW_PANEL_GAL::backendScreenSize() const
{
    const int pad = ScreenSizePad( m_backend );
    wxSize    size = GetClientSize();

    // The backend window starts below what cannot be drawn over it
    size.y -= m_overlayInsetTop;

    return wxSize( std::max( 10, size.x + pad ), std::max( 10, size.y + pad ) );
}


void EDA_DRAW_PANEL_GAL::ResizeGal( bool aForce )
{
    // If we get a second wx update call before the first finishes, don't crash
    if( m_gal->IsContextLocked() )
        return;

    KIGFX::GAL_CONTEXT_LOCKER locker( m_gal );

    // Resizing reallocates the framebuffer, which only exists in this canvas' own context
    if( !m_gal->IsContextValid() )
    {
        // wx reports a given size once, so dropping it here would leave the canvas stuck at
        // whatever size the GAL was built with until something else resizes the window
        m_pendingResize = true;
        RequestRefresh();
        return;
    }

    m_pendingResize = false;

    const wxSize clientSize = backendScreenSize();

    // A client area that grew by exactly the pad would otherwise match the old GAL size
    if( !aForce && ToVECTOR2I( clientSize ) == m_gal->GetScreenPixelSize() )
        return;

    m_gal->ResizeScreen( clientSize.GetX(), clientSize.GetY() );

    wxWindow* backendWindow = dynamic_cast<wxWindow*>( m_gal );

    if( backendWindow && backendWindow->GetPosition().y != m_overlayInsetTop )
        backendWindow->Move( 0, m_overlayInsetTop );

    if( m_backend == GAL_TYPE_MODERN )
    {
        static_cast<KIGFX::GAL_MODERN*>( m_gal )->SetDpiScale(
                static_cast<float>( GetContentScaleFactor() ) );
    }

    if( m_swapchainCtrl )
    {
        float dpiScale = static_cast<float>( GetContentScaleFactor() );
        m_swapchainCtrl->OnResize( static_cast<uint32_t>( clientSize.x ),
                                   static_cast<uint32_t>( clientSize.y ),
                                   dpiScale );
    }

    if( m_view )
    {
        // ResizeScreen reallocates every compositor buffer, so nothing survives the resize
        m_view->MarkDirty();

        // New FBO sizes need a fresh upload, which the camera fast path would skip
        m_view->MarkGeometryDirty();
    }

    if( m_backend == GAL_TYPE_MODERN )
        Refresh();
}


void EDA_DRAW_PANEL_GAL::UpdateOverlayExclusions()
{
    std::vector<wxRect> rects;
    int                 infobarsBottom = 0;

    for( wxWindow* child : GetChildren() )
    {
        if( dynamic_cast<WX_INFOBAR*>( child ) && child->IsShown() )
        {
            rects.push_back( child->GetRect() );
            infobarsBottom = std::max( infobarsBottom, child->GetRect().GetBottom() + 1 );
        }
    }

    // On Wayland the OpenGL canvas is a subsurface stacked above everything the toolkit draws
    // in the frame, so an infobar overlaid on it cannot show. The canvas makes room instead.
    int inset = 0;

#ifdef __WXGTK__
    if( m_backend == GAL_TYPE_OPENGL && wxGetDisplayInfo().type == wxDisplayType::wxDisplayWayland )
        inset = infobarsBottom;
#endif

    if( inset != m_overlayInsetTop )
    {
        m_overlayInsetTop = inset;
        ResizeGal( true );
        Refresh();
    }

    // Only the Cairo backend presents frames outside the paint cycle
    if( KIGFX::CAIRO_GAL* cairoGal = dynamic_cast<KIGFX::CAIRO_GAL*>( m_gal ) )
        cairoGal->SetOverlayExclusions( rects );
}


void EDA_DRAW_PANEL_GAL::RequestRefresh()
{
    m_needIdleRefresh = true;
}


float EDA_DRAW_PANEL_GAL::displayRefreshRate()
{
    const auto now = std::chrono::steady_clock::now();

    // The window can move to another display
    if( now - m_refreshRateAsked >= std::chrono::seconds( 1 ) )
    {
        const double reported = KIPLATFORM::UI::GetDisplayRefreshRate( this );

        // Headless, virtualized and some driver setups report 0 or nonsense
        m_refreshRate = reported >= 24.0 && reported <= 1000.0 ? static_cast<float>( reported ) : 0.0f;
        m_refreshRateAsked = now;
    }

    return m_refreshRate;
}


void EDA_DRAW_PANEL_GAL::Refresh( bool aEraseBackground, const wxRect* aRect )
{
    auto now = std::chrono::steady_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>( now - m_lastRepaintStart ).count();
    bool galInitialized = m_gal && m_gal->IsInitialized();

    // When vsync is available the driver throttles SwapBuffers, so we only need
    // a small guard to avoid queueing work faster than the GPU can consume it.
    // Without vsync, cap the render rate at the monitor refresh rate so the
    // GPU is not saturated producing frames that will never be shown.
    int minPeriodMs = 3;

    if( galInitialized && m_gal->GetSwapInterval() == 0 )
    {
        const float displayHz = displayRefreshRate();
        int         refreshHz = displayHz > 0.0f ? KiROUND( displayHz ) : 60;

        refreshHz += 5; // Repaint slightly faster to avoid adding latency

        minPeriodMs = 1000 / refreshHz;
    }

    if( delta >= minPeriodMs )
    {
        if( !DoRePaint() && IsShownOnScreen() )
            RequestRefresh();
    }
    else if( !m_refreshTimer.IsRunning() )
    {
        m_refreshDeferred = true;
        m_refreshTimer.StartOnce( static_cast<int>( minPeriodMs - delta ) );
    }
}


void EDA_DRAW_PANEL_GAL::ForceRefresh()
{
    wxLogTrace( traceRhi, wxS( "DrawPanel: ForceRefresh windowId=%u drawEnabled=%d "
                                "IsShown=%d IsShownOnScreen=%d galInit=%d" ),
                m_windowId, m_drawingEnabled, IsShown(), IsShownOnScreen(),
                m_gal ? m_gal->IsInitialized() : -1 );

    if( !m_drawingEnabled )
    {
        if( m_gal && m_gal->IsInitialized() )
        {
            Connect( wxEVT_PAINT, wxPaintEventHandler( EDA_DRAW_PANEL_GAL::onPaint ), nullptr, this );
            Connect( wxEVT_IDLE, wxIdleEventHandler( EDA_DRAW_PANEL_GAL::onIdle ), nullptr, this );

            m_drawingEnabled = true;
        }
        else
        {
            // Try again soon
            m_refreshTimer.StartOnce( 100 );
            return;
        }
    }

    DoRePaint( false );
}


bool EDA_DRAW_PANEL_GAL::GetScreenshot( wxImage& aDstImage )
{
    if( !m_gal )
        return false;

    if( m_backend == GAL_TYPE_OPENGL )
    {
        DoRePaint( false );

        // The repaint may have replaced a GAL lost to a GPU reset
        if( m_backend != GAL_TYPE_OPENGL || !m_gal )
            return false;

        return static_cast<KIGFX::OPENGL_GAL*>( m_gal )->GetScreenshot( aDstImage );
    }

    if( m_backend == GAL_TYPE_MODERN && m_renderThread )
    {
        DoRePaint( false );

        // The repaint may have torn down the render thread while recovering from an error
        if( m_backend != GAL_TYPE_MODERN || !m_renderThread )
            return false;

        return m_renderThread->ReadbackWindowPixels( m_windowId, aDstImage );
    }

    // The caller falls back to a wxClientDC capture
    return false;
}


void EDA_DRAW_PANEL_GAL::SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher )
{
    m_eventDispatcher = aEventDispatcher;
}


void EDA_DRAW_PANEL_GAL::StartDrawing()
{
    wxLogTrace( traceRhi, wxS( "DrawPanel: StartDrawing windowId=%u" ), m_windowId );

    // Start querying GAL if it is ready
    m_refreshTimer.StartOnce( 100 );
}


void EDA_DRAW_PANEL_GAL::StopDrawing()
{
    m_refreshTimer.Stop();
    m_drawingEnabled = false;

    Disconnect( wxEVT_PAINT, wxPaintEventHandler( EDA_DRAW_PANEL_GAL::onPaint ), nullptr, this );
    Disconnect( wxEVT_IDLE, wxIdleEventHandler( EDA_DRAW_PANEL_GAL::onIdle ), nullptr, this );
}


void EDA_DRAW_PANEL_GAL::SetHighContrastLayer( int aLayer )
{
    // Set display settings for high contrast mode
    KIGFX::RENDER_SETTINGS* rSettings = m_view->GetPainter()->GetSettings();

    SetTopLayer( aLayer );

    rSettings->ClearHighContrastLayers();
    rSettings->SetLayerIsHighContrast( aLayer );

    m_view->UpdateAllLayersColor();
}


void EDA_DRAW_PANEL_GAL::SetTopLayer( int aLayer )
{
    m_view->ClearTopLayers();
    m_view->SetTopLayer( aLayer );
    m_view->UpdateAllLayersOrder();
}


EDA_DRAW_PANEL_GAL::GAL_TYPE EDA_DRAW_PANEL_GAL::ResolveStoredCanvasType( int aStoredCanvasType )
{
    if( aStoredCanvasType == GAL_TYPE_CAIRO || aStoredCanvasType == GAL_TYPE_MODERN )
        return static_cast<GAL_TYPE>( aStoredCanvasType );

    // The retired wxDC canvas, and any value another KiCad version may have written, leave the
    // user with the accelerated canvas rather than the do-nothing stub GAL
    return GAL_TYPE_OPENGL;
}


bool EDA_DRAW_PANEL_GAL::SwitchBackend( GAL_TYPE aGalType )
{
    // Do not do anything if the currently used GAL is correct
    if( aGalType == m_backend && m_gal != nullptr )
        return true;

    VECTOR2D grid_size = m_gal ? m_gal->GetGridSize() : VECTOR2D();
    bool     grid_visibility = m_gal ? m_gal->GetGridVisibility() : true;
    double   worldUnitLength = m_gal ? m_gal->GetWorldUnitLength() : 0.0;
    bool     result = true; // assume everything will be fine

    // Prevent refreshing canvas during backend switch
    StopDrawing();

    m_contextBindFailures = 0;
    m_pendingResize = false;

    KIGFX::GAL* new_gal = nullptr;

    try
    {
        switch( aGalType )
        {
        case GAL_TYPE_OPENGL:
        {
            wxString errormsg = KIGFX::OPENGL_GAL::CheckFeatures( m_options );

            if( errormsg.empty() )
            {
                new_gal = new KIGFX::OPENGL_GAL( GetVcSettings(), m_options, this, this, this );
            }
            else
            {
                if( GAL_FALLBACK != aGalType )
                {
                    aGalType = GAL_FALLBACK;
                    showMessageLater( _( "Could not use OpenGL, falling back to software rendering" ), errormsg,
                                      false );
                    new_gal = new KIGFX::CAIRO_GAL( m_options, this, this, this );
                }
                else
                {
                    // We're well and truly banjaxed if we get here without a fallback.
                    showMessageLater( _( "Could not use OpenGL" ), errormsg, false );
                    new_gal = new KIGFX::GAL( m_options );
                    aGalType = GAL_TYPE_NONE;
                    result = false;
                }
            }

            break;
        }

        case GAL_TYPE_CAIRO:
            new_gal = new KIGFX::CAIRO_GAL( m_options, this, this, this );
            break;

        case GAL_TYPE_MODERN:
        {
            teardownModernRenderInfra();

            // A device that failed once is not retried, so the user is told only once
            if( EDA_DRAW_FRAME::CanvasFailed( GAL_TYPE_MODERN ) )
                return abandonRhiCanvas( wxEmptyString );

            bool enableValidation = ADVANCED_CFG::GetCfg().m_EnableVulkanValidation;

            auto deviceFactory = [enableValidation]() -> std::unique_ptr<RHI_DEVICE>
            {
                return RhiCreatePlatformDevice( enableValidation );
            };

            m_renderThread = std::make_unique<KIGFX::RENDER_THREAD>(
                    std::move( deviceFactory ) );

            // A repaint sees the loss and rebuilds the canvas. The thread is joined before the panel is
            // destroyed, and wx drops anything still queued for it.
            m_renderThread->SetDeviceLostHandler(
                    [this]()
                    {
                        CallAfter(
                                [this]()
                                {
                                    Refresh();
                                } );
                    } );

            wxLogTrace( traceRhi, wxS( "DrawPanel: starting render thread" ) );

            if( m_renderThread->Start() )
            {
                wxLogTrace( traceRhi, wxS( "DrawPanel: render thread started" ) );

                // The render thread owns the device, so the UI thread reaches it only through the proxy
                m_deviceProxy = std::make_unique<KIGFX::RHI_DEVICE_PROXY>( *m_renderThread );

                new_gal = new KIGFX::GAL_MODERN( m_options, *m_deviceProxy );

                auto* galModern = static_cast<KIGFX::GAL_MODERN*>( new_gal );
                galModern->SetRenderThread( m_renderThread.get() );
                galModern->SetPersistentTargets( true );

                // Only the track shader draws the glyphs of the font atlas
                galModern->SetBitmapGlyphs( m_linePipelineKind == KIGFX::RHI_LINE_PIPELINE_KIND::PCB_TRACKS );

                m_windowId = s_nextWindowId++;
                m_frameMailbox = std::make_unique<KIGFX::FRAME_MAILBOX>();
                m_swapchainCtrl = std::make_unique<KIGFX::SWAPCHAIN_CONTROLLER>(
                        *m_renderThread, m_windowId );
                m_swapchainCtrl->SetFrameMailbox( m_frameMailbox.get() );

                wxLogTrace( traceRhi, wxS( "DrawPanel: windowId=%u swapchainCtrl created" ),
                            m_windowId );

                // The GDK background is set on the first paint, once the widget is laid out
                KIPLATFORM::UI::PrepareForExternalRendering( this );
                m_needsExternalRenderingSetup = true;

                // The native surface is being recreated
                m_nativeHandleValid = false;
            }
            else
            {
                wxLogTrace( traceRhi, wxS( "DrawPanel: render thread start FAILED" ) );

                m_renderThread.reset();

                return abandonRhiCanvas( wxEmptyString );
            }

            break;
        }

        default:
            wxASSERT( false );
            KI_FALLTHROUGH;

        case GAL_TYPE_NONE:
            // KIGFX::GAL is a stub that prevents code relying on GAL canvas
            // existence from crashing, but cannot display anything
            new_gal = new KIGFX::GAL( m_options );
            break;
        }
    }
    catch( std::runtime_error& err )
    {
        // The GAL is deleted while the device proxy it releases its buffers through is still alive
        if( aGalType == GAL_TYPE_MODERN )
        {
            delete new_gal;
            teardownModernRenderInfra();

            return abandonRhiCanvas( wxString( err.what() ) );
        }

        // A context the driver refuses to create would otherwise leave every frame on the blank stub
        if( aGalType == GAL_TYPE_OPENGL && GAL_FALLBACK_AVAILABLE )
        {
            try
            {
                new_gal = new KIGFX::CAIRO_GAL( m_options, this, this, this );
                aGalType = GAL_FALLBACK;
                EDA_DRAW_FRAME::SetCanvasFailed( GAL_TYPE_OPENGL );
                showMessageLater( _( "Could not use OpenGL, falling back to software rendering" ),
                                  wxString( err.what() ), false );
            }
            catch( std::runtime_error& )
            {
                new_gal = nullptr;
            }
        }

        if( !new_gal )
        {
            // Create a dummy GAL
            new_gal = new KIGFX::GAL( m_options );
            aGalType = GAL_TYPE_NONE;
            showMessageLater( _( "Error switching GAL backend" ), wxString( err.what() ), true );
            result = false;
        }
    }

    // trigger update of the gal options in case they differ from the defaults
    m_options.NotifyChanged();

    // The native touchpad hook is attached to the window that takes the backend's input
    m_touchpadGestureHandler.reset();

    // Queued uploads must not outlive the GAL_MODERN geometry they point at
    if( m_backend == GAL_TYPE_MODERN && aGalType != GAL_TYPE_MODERN )
        teardownModernRenderInfra();

    delete m_gal;
    m_gal = nullptr;

    m_gal = new_gal;

    // The view lays itself out in the editor's world units as it attaches
    if( worldUnitLength > 0.0 )
        m_gal->SetWorldUnitLength( worldUnitLength );

    if( aGalType == GAL_TYPE_MODERN )
    {
        KIGFX::GAL_MODERN* galModern = static_cast<KIGFX::GAL_MODERN*>( m_gal );

        galModern->SetDpiScale( static_cast<float>( GetContentScaleFactor() ) );
        galModern->SetCentringPad( ScreenSizePad( GAL_TYPE_OPENGL ) - ScreenSizePad( GAL_TYPE_MODERN ) );
    }

    // Matches ResizeGal() so the picture does not move at the next resize
    wxSize clientSize = GetClientSize();
    int    pad = ScreenSizePad( aGalType );

    clientSize.x = std::max( 10, clientSize.x + pad );
    clientSize.y = std::max( 10, clientSize.y + pad );
    m_gal->ResizeScreen( clientSize.GetX(), clientSize.GetY() );

    if( grid_size.x > 0 && grid_size.y > 0 )
        m_gal->SetGridSize( grid_size );

    m_gal->SetGridVisibility( grid_visibility );

    // Make sure the cursor is set on the new canvas
    SetCurrentCursor( KICURSOR::ARROW );

    if( m_painter )
        m_painter->SetGAL( m_gal );

    if( m_view )
    {
        m_view->SetGAL( m_gal );
        // OpenGL requires reverse draw order when draw priority is enabled.
        m_view->ReverseDrawOrder( aGalType == GAL_TYPE_OPENGL );

        // The new GAL starts with empty geometry caches
        m_view->UpdateAllItems( KIGFX::ALL );
    }

    m_backend = aGalType;

    // A shown backend window raises itself, which would bury what is overlaid on this canvas
    for( wxWindow* child : GetChildren() )
    {
        if( child != dynamic_cast<wxWindow*>( m_gal ) )
            child->Raise();
    }

    UpdateOverlayExclusions();

    UpdateTouchpadGestureHandler();

    if( m_view )
    {
        wxLogTrace( traceRhi, wxS( "DrawPanel: backend %d view centre (%.17g, %.17g) scale %.17g screen %dx%d" ),
                    static_cast<int>( m_backend ), m_view->GetCenter().x, m_view->GetCenter().y, m_view->GetScale(),
                    m_gal->GetScreenPixelSize().x, m_gal->GetScreenPixelSize().y );
    }

    return result;
}


void EDA_DRAW_PANEL_GAL::UpdateTouchpadGestureHandler()
{
    m_touchpadGestureHandler.reset();

    if( Pgm().GetCommonSettings()->m_Input.touchpad_mode != TOUCHPAD_MODE::NATIVE_GESTURES )
        return;

    // The modern canvas has no child window, so this panel receives the native pointer input
    wxWindow* inputWindow = m_backend == GAL_TYPE_MODERN ? static_cast<wxWindow*>( this )
                                                         : dynamic_cast<wxWindow*>( m_gal );

    if( inputWindow )
    {
        m_touchpadGestureHandler = KIPLATFORM::UI::CreateTouchpadGestureHandler(
                inputWindow,
                [this]( const KIPLATFORM::UI::TOUCHPAD_GESTURE& aGesture )
                {
                    if( !m_viewControls )
                        return;

                    m_viewControls->ApplyPanAndZoomGesture(
                            VECTOR2D( aGesture.panX, aGesture.panY ), aGesture.zoomFactor,
                            VECTOR2D( aGesture.zoomAnchor.x, aGesture.zoomAnchor.y ) );
                } );
    }
}


void EDA_DRAW_PANEL_GAL::OnShow()
{
    try
    {
        // Check if the current rendering backend can be properly initialized
        m_view->UpdateItems();
    }
    catch( const std::runtime_error& e )
    {
        // recoverFromGalError() tells the user the outcome either way
        if( !recoverFromGalError( e ) )
        {
            StopDrawing();
        }
        else if( m_edaFrame && m_edaFrame->GetCanvas() == this )
        {
            // The frame's tools and view settings must follow the new GAL
            m_edaFrame->ActivateGalCanvas();
        }
    }
}


void EDA_DRAW_PANEL_GAL::NoteInput()
{
    s_inputSerial.fetch_add( 1, std::memory_order_relaxed );
}


void EDA_DRAW_PANEL_GAL::OnEvent( wxEvent& aEvent )
{
    if( aEvent.GetEventType() != KIGFX::WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE )
        NoteInput();

    bool shouldSetFocus = m_lostFocus && m_stealsFocus
                          && !KIUI::IsInputControlFocused()                // Don't steal from input controls
                          && !KIUI::IsModalDialogFocused()                 // Don't steal from dialogs
                          && KIPLATFORM::UI::IsWindowActive( m_edaFrame ); // Don't steal from other windows

    if( shouldSetFocus )
        SetFocus();

    if( !m_eventDispatcher )
        aEvent.Skip();
    else
        m_eventDispatcher->DispatchWxEvent( aEvent );

    Refresh();
}


void EDA_DRAW_PANEL_GAL::onEnter( wxMouseEvent& aEvent )
{
    bool shouldSetFocus = m_stealsFocus
                          && !KIUI::IsInputControlFocused()                // Don't steal from input controls
                          && !KIUI::IsModalDialogFocused()                 // Don't steal from dialogs
                          && KIPLATFORM::UI::IsWindowActive( m_edaFrame ); // Don't steal from other windows

    // Getting focus is necessary in order to receive key events properly
    if( shouldSetFocus )
        SetFocus();

    aEvent.Skip();
}


void EDA_DRAW_PANEL_GAL::onLostFocus( wxFocusEvent& aEvent )
{
    m_lostFocus = true;

    m_viewControls->CancelDrag();

    // Reset the tool dispatcher's button state when focus is lost. This prevents
    // the dispatcher from thinking the button is still pressed when focus returns,
    // which can cause selection and drag operations to stop working.
    if( m_eventDispatcher )
        m_eventDispatcher->ResetState();

    aEvent.Skip();
}


void EDA_DRAW_PANEL_GAL::onIdle( wxIdleEvent& aEvent )
{
    if( m_needIdleRefresh )
    {
        wxLogTrace( traceRhi, wxS( "DrawPanel: onIdle refresh windowId=%u IsShown=%d "
                                    "IsShownOnScreen=%d" ),
                    m_windowId, IsShown(), IsShownOnScreen() );

        m_needIdleRefresh = false;
        Refresh();
    }

    aEvent.Skip();
}


void EDA_DRAW_PANEL_GAL::onRefreshTimer( wxTimerEvent& aEvent )
{
    const bool deferred = m_refreshDeferred;

    m_refreshDeferred = false;

    // A postponed Refresh() is not an expose and must not present the frame already on screen
    if( deferred && m_backend == GAL_TYPE_MODERN && m_drawingEnabled )
    {
        if( !DoRePaint() && IsShownOnScreen() )
            RequestRefresh();

        return;
    }

    ForceRefresh();
}


void EDA_DRAW_PANEL_GAL::onShowEvent( wxShowEvent& aEvent )
{
    wxLogTrace( traceRhi, wxS( "DrawPanel: onShowEvent windowId=%u show=%d IsShown=%d "
                                "IsShownOnScreen=%d galInit=%d" ),
                m_windowId, aEvent.IsShown(), IsShown(), IsShownOnScreen(),
                m_gal ? m_gal->IsInitialized() : -1 );

    if( m_gal && m_gal->IsInitialized() && m_gal->IsVisible() )
    {
        OnShow();
    }

}


void EDA_DRAW_PANEL_GAL::SetCurrentCursor( KICURSOR aCursor )
{
    if( !m_gal )
        return;

    DPI_SCALING_COMMON dpi( nullptr, m_parent );

    bool hidpi = false;

    // Cursor scaling factor cannot be set for a wxCursor on GTK and OSX (at least before wx 3.3),
    // resulting in 4x rendered size on 2x window scale.
    // MSW renders the bitmap as-is, without scaling, so this works here.
#ifdef __WXMSW__
    hidpi = dpi.GetContentScaleFactor() >= 2.0;
#endif

    m_gal->SetNativeCursorStyle( aCursor, hidpi );

    // The RHI canvas draws into the panel itself, and a new canvas window inherits the panel's cursor
#if wxCHECK_VERSION( 3, 3, 0 )
    SetCursorBundle( CURSOR_STORE::GetCursor( aCursor, hidpi ) );
#else
    SetCursor( CURSOR_STORE::GetCursor( aCursor, hidpi ) );
#endif
}


std::shared_ptr<KIGFX::VIEW_OVERLAY> EDA_DRAW_PANEL_GAL::DebugOverlay()
{
    if( !m_debugOverlay )
    {
        m_debugOverlay.reset( new KIGFX::VIEW_OVERLAY() );
        m_view->Add( m_debugOverlay.get() );
    }

    return m_debugOverlay;
}


void EDA_DRAW_PANEL_GAL::ClearDebugOverlay()
{
    if( m_debugOverlay )
    {
        m_view->Remove( m_debugOverlay.get() );
        m_debugOverlay = nullptr;
    }
}


KIGFX::VC_SETTINGS EDA_DRAW_PANEL_GAL::GetVcSettings()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    KIGFX::VC_SETTINGS vcSettings;
    vcSettings.m_warpCursor = cfg->m_Input.center_on_zoom;
    vcSettings.m_focusFollowSchPcb = cfg->m_Input.focus_follow_sch_pcb;
    vcSettings.m_autoPanSettingEnabled = cfg->m_Input.auto_pan;
    vcSettings.m_autoPanAcceleration = static_cast<float>( cfg->m_Input.auto_pan_acceleration );
    vcSettings.m_horizontalPan = cfg->m_Input.horizontal_pan;
    vcSettings.m_zoomAcceleration = cfg->m_Input.zoom_acceleration;
    vcSettings.m_zoomSpeed = cfg->m_Input.zoom_speed;
    vcSettings.m_zoomSpeedAuto = cfg->m_Input.zoom_speed_auto;
    vcSettings.m_scrollModifierZoom = cfg->m_Input.scroll_modifier_zoom;
    vcSettings.m_scrollModifierPanH = cfg->m_Input.scroll_modifier_pan_h;
    vcSettings.m_scrollModifierPanV = cfg->m_Input.scroll_modifier_pan_v;
    vcSettings.m_motionPanModifier = cfg->m_Input.motion_pan_modifier;
    vcSettings.m_dragLeft = cfg->m_Input.drag_left;
    vcSettings.m_dragMiddle = cfg->m_Input.drag_middle;
    vcSettings.m_dragRight = cfg->m_Input.drag_right;
    vcSettings.m_scrollReverseZoom = cfg->m_Input.reverse_scroll_zoom;
    vcSettings.m_scrollReversePanH = cfg->m_Input.reverse_scroll_pan_h;

    return vcSettings;
}
