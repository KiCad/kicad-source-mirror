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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */
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
#include <gal/cairo/cairo_gal.h>
#include <math/vector2wx.h>


#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>

#include <widgets/wx_infobar.h>

#include <kiplatform/ui.h>

#include <core/profile.h>

#include <pgm_base.h>
#include <confirm.h>


/**
 * Flag to enable drawing panel debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar traceDrawPanel[] = wxT( "KICAD_DRAW_PANEL" );


EDA_DRAW_PANEL_GAL::EDA_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
        wxScrolledCanvas( aParentWindow, aWindowId, aPosition, aSize ),
        m_MouseCapturedLost( false ),
        m_parent( aParentWindow ),
        m_edaFrame( nullptr ),
        m_lastRepaintStart( 0 ),
        m_lastRepaintEnd( 0 ),
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

    SwitchBackend( aGalType );
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );

    EnableScrolling( false, false ); // otherwise Zoom Auto disables GAL canvas
    KIPLATFORM::UI::SetOverlayScrolling( this, false ); // Prevent excessive repaint on GTK
    KIPLATFORM::UI::ImmControl( this, false ); // Ensure our panel can't suck in IME events

    Connect( wxEVT_SIZE, wxSizeEventHandler( EDA_DRAW_PANEL_GAL::onSize ), nullptr, this );
    Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler( EDA_DRAW_PANEL_GAL::onEnter ), nullptr,
             this );
    Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( EDA_DRAW_PANEL_GAL::onLostFocus ), nullptr,
             this );

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
        Connect( eventType, wxEventHandler( EDA_DRAW_PANEL_GAL::OnEvent ), nullptr,
                 m_eventDispatcher );

    // Set up timer to detect when drawing starts
    m_refreshTimer.SetOwner( this );
    Connect( m_refreshTimer.GetId(), wxEVT_TIMER,
             wxTimerEventHandler( EDA_DRAW_PANEL_GAL::onRefreshTimer ), nullptr, this );

    Connect( wxEVT_SHOW, wxShowEventHandler( EDA_DRAW_PANEL_GAL::onShowEvent ), nullptr, this );
}


EDA_DRAW_PANEL_GAL::~EDA_DRAW_PANEL_GAL()
{
    // Ensure EDA_DRAW_PANEL_GAL::onShowEvent is not fired during Dtor process
    Disconnect( wxEVT_SHOW, wxShowEventHandler( EDA_DRAW_PANEL_GAL::onShowEvent ) );
    StopDrawing();

    wxASSERT( !m_drawing );

    delete m_viewControls;
    delete m_view;
    delete m_gal;
    m_gal = nullptr;    // Ensure OnShow is not called
}


void EDA_DRAW_PANEL_GAL::SetFocus()
{
    KIPLATFORM::UI::ImeNotifyCancelComposition( this );
    wxScrolledCanvas::SetFocus();
    m_lostFocus = false;
}


void EDA_DRAW_PANEL_GAL::onPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    DoRePaint();
}


bool EDA_DRAW_PANEL_GAL::DoRePaint()
{
    if( !m_refreshMutex.try_lock() )
        return false;

    std::lock_guard<std::mutex> lock( m_refreshMutex, std::adopt_lock );

    if( !m_drawingEnabled )
        return false;

    if( !m_gal->IsInitialized() || !m_gal->IsVisible() || m_gal->IsContextLocked() )
        return false;

    if( m_drawing )
        return false;

    m_lastRepaintStart = wxGetLocalTimeMillis();

    // Repaint the canvas, and fix scrollbar cursors
    // Usually called by a OnPaint event, but because it does not use a wxPaintDC,
    // it can be called outside a wxPaintEvent.

    // Update current zoom settings if the canvas is managed by a EDA frame
    // (i.e. not by a preview panel in a dialog)
    if( !IsDialogPreview() && GetParentEDAFrame() && GetParentEDAFrame()->GetScreen() )
        GetParentEDAFrame()->GetScreen()->m_ScrollCenter = GetView()->GetCenter();

    if( Pgm().GetCommonSettings()->m_Appearance.show_scrollbars )
        m_viewControls->UpdateScrollbars();

    SCOPED_SET_RESET<bool> drawing( m_drawing, true );

    ( *m_PaintEventCounter )++;

    wxASSERT( m_painter );

    KIGFX::RENDER_SETTINGS* settings =
            static_cast<KIGFX::RENDER_SETTINGS*>( m_painter->GetSettings() );

    PROF_TIMER cntUpd("view-upd-items");
    PROF_TIMER cntTotal("view-total");
    PROF_TIMER cntCtx("view-context-create");
    PROF_TIMER cntCtxDestroy("view-context-destroy");
    PROF_TIMER cntRedraw("view-redraw-rects");

    bool isDirty = false;

    cntTotal.Start();

    try
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

        cntUpd.Stop();

        // GAL_DRAWING_CONTEXT can throw in the dtor, so we need to scope
        // the full lifetime inside the try block
        {
            cntCtx.Start();
            KIGFX::GAL_DRAWING_CONTEXT ctx( m_gal );
            cntCtx.Stop();

            if( m_view->IsTargetDirty( KIGFX::TARGET_OVERLAY )
                && !m_gal->HasTarget( KIGFX::TARGET_OVERLAY ) )
            {
                m_view->MarkDirty();
            }

            m_gal->SetClearColor( settings->GetBackgroundColor() );
            m_gal->SetGridColor( settings->GetGridColor() );
            m_gal->SetCursorColor( settings->GetCursorColor() );

            // TODO: find why ClearScreen() must be called here in opengl mode
            // and only if m_view->IsDirty() in Cairo mode to avoid display artifacts
            // when moving the mouse cursor
            if( m_backend == GAL_TYPE_OPENGL )
                m_gal->ClearScreen();

            if( m_view->IsDirty() )
            {
                if( m_backend != GAL_TYPE_OPENGL  // Already called in opengl
                        && m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED ) )
                {
                    m_gal->ClearScreen();
                }

                m_view->ClearTargets();

                // Grid has to be redrawn only when the NONCACHED target is redrawn
                if( m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED ) )
                    m_gal->DrawGrid();

                cntRedraw.Start();
                m_view->Redraw();
                cntRedraw.Stop();
                isDirty = true;
            }

            m_gal->DrawCursor( m_viewControls->GetCursorPosition() );

            cntCtxDestroy.Start();
        }

        // ctx goes out of scope here so destructor would be called
        cntCtxDestroy.Stop();
    }
    catch( std::exception& err )
    {
        if( GAL_FALLBACK != m_backend )
        {
            SwitchBackend( GAL_FALLBACK );

            DisplayInfoMessage( m_parent,
                                _( "Could not use OpenGL, falling back to software rendering" ),
                                wxString( err.what() ) );

            StartDrawing();
        }
        else
        {
            // We're well and truly banjaxed if we get here without a fallback.
            DisplayErrorMessage( m_parent, _( "Graphics error" ), wxString( err.what() ) );

            StopDrawing();
        }
    }

    if( isDirty )
    {
        KI_TRACE( traceGalProfile, "View timing: %s %s %s %s %s\n",
            cntTotal.to_string(),
            cntUpd.to_string(),
            cntRedraw.to_string(),
            cntCtx.to_string(),
            cntCtxDestroy.to_string()
        );
    }

    m_lastRepaintEnd = wxGetLocalTimeMillis();

    return true;
}


void EDA_DRAW_PANEL_GAL::onSize( wxSizeEvent& aEvent )
{
    // If we get a second wx update call before the first finishes, don't crash
    if( m_gal->IsContextLocked() )
        return;

    KIGFX::GAL_CONTEXT_LOCKER locker( m_gal );
    wxSize                    clientSize = GetClientSize();
    WX_INFOBAR* infobar = GetParentEDAFrame() ? GetParentEDAFrame()->GetInfoBar() : nullptr;

    if( ToVECTOR2I( clientSize ) == m_gal->GetScreenPixelSize() )
        return;

    // Note: ( +1, +1 ) prevents an ugly black line on right and bottom on Mac
    clientSize.x = std::max( 10, clientSize.x + 1 );
    clientSize.y = std::max( 10, clientSize.y + 1 );

    VECTOR2D bottom( 0, 0 );

    if( m_view )
        bottom = m_view->ToWorld( m_gal->GetScreenPixelSize(), true );

    m_gal->ResizeScreen( clientSize.GetX(), clientSize.GetY() );

    if( m_view )
    {
        if( infobar && infobar->IsLocked() )
        {
            VECTOR2D halfScreen( std::ceil( 0.5 * clientSize.x ), std::ceil( 0.5 * clientSize.y ) );
            m_view->SetCenter( bottom - m_view->ToWorld( halfScreen, false ) );
        }

        m_view->MarkTargetDirty( KIGFX::TARGET_CACHED );
        m_view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    }
}


void EDA_DRAW_PANEL_GAL::RequestRefresh()
{
    m_needIdleRefresh = true;
}


void EDA_DRAW_PANEL_GAL::Refresh( bool aEraseBackground, const wxRect* aRect )
{
    if( !DoRePaint() )
        RequestRefresh();
}


void EDA_DRAW_PANEL_GAL::ForceRefresh()
{
    if( !m_drawingEnabled )
    {
        if( m_gal && m_gal->IsInitialized() )
        {
            Connect( wxEVT_PAINT, wxPaintEventHandler( EDA_DRAW_PANEL_GAL::onPaint ), nullptr,
                     this );

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

    DoRePaint();
}


void EDA_DRAW_PANEL_GAL::SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher )
{
    m_eventDispatcher = aEventDispatcher;
}


void EDA_DRAW_PANEL_GAL::StartDrawing()
{
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


bool EDA_DRAW_PANEL_GAL::SwitchBackend( GAL_TYPE aGalType )
{
    // Do not do anything if the currently used GAL is correct
    if( aGalType == m_backend && m_gal != nullptr )
        return true;

    VECTOR2D grid_size = m_gal ? m_gal->GetGridSize() : VECTOR2D();
    bool     grid_visibility = m_gal ? m_gal->GetGridVisibility() : true;
    bool     result = true; // assume everything will be fine

    // Prevent refreshing canvas during backend switch
    StopDrawing();

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
                    DisplayInfoMessage(
                            m_parent,
                            _( "Could not use OpenGL, falling back to software rendering" ),
                            errormsg );
                    new_gal = new KIGFX::CAIRO_GAL( m_options, this, this, this );
                }
                else
                {
                    // We're well and truly banjaxed if we get here without a fallback.
                    DisplayInfoMessage( m_parent, _( "Could not use OpenGL" ), errormsg );
                }
            }

            break;
        }

        case GAL_TYPE_CAIRO:
            new_gal = new KIGFX::CAIRO_GAL( m_options, this, this, this );
            break;

        default:
            wxASSERT( false );
            KI_FALLTHROUGH;
            // warn about unhandled GAL canvas type, but continue with the fallback option

        case GAL_TYPE_NONE:
            // KIGFX::GAL is a stub - it actually does cannot display anything,
            // but prevents code relying on GAL canvas existence from crashing
            new_gal = new KIGFX::GAL( m_options );
            break;
        }
    }
    catch( std::runtime_error& err )
    {
        // Create a dummy GAL
        new_gal = new KIGFX::GAL( m_options );
        aGalType = GAL_TYPE_NONE;
        DisplayErrorMessage( m_parent, _( "Error switching GAL backend" ), wxString( err.what() ) );
        result = false;
    }

    // trigger update of the gal options in case they differ from the defaults
    m_options.NotifyChanged();

    delete m_gal;
    m_gal = new_gal;

    wxSize clientSize = GetClientSize();
    clientSize.x = std::max( 10, clientSize.x );
    clientSize.y = std::max( 10, clientSize.y );
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
        // Note: OpenGL requires reverse draw order when draw priority is enabled
        m_view->ReverseDrawOrder( aGalType == GAL_TYPE_OPENGL );
    }

    m_backend = aGalType;

    return result;
}


void EDA_DRAW_PANEL_GAL::OnEvent( wxEvent& aEvent )
{
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

    // Give events time to process, based on last render duration
    wxLongLong endDelta = wxGetLocalTimeMillis() - m_lastRepaintEnd;
    long long  timeLimit = ( m_lastRepaintEnd - m_lastRepaintStart ).GetValue() / 5;

    timeLimit = std::clamp( timeLimit, 3LL, 150LL );

    if( endDelta > timeLimit )
        Refresh();
    else
        RequestRefresh();
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

    aEvent.Skip();
}


void EDA_DRAW_PANEL_GAL::onIdle( wxIdleEvent& aEvent )
{
    if( m_needIdleRefresh )
    {
        m_needIdleRefresh = false;
        Refresh();
    }

    aEvent.Skip();
}


void EDA_DRAW_PANEL_GAL::onRefreshTimer( wxTimerEvent& aEvent )
{
    ForceRefresh();
}


void EDA_DRAW_PANEL_GAL::onShowEvent( wxShowEvent& aEvent )
{
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
    vcSettings.m_autoPanAcceleration = cfg->m_Input.auto_pan_acceleration;
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
