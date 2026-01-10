/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013-2015 CERN
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

#include <pgm_base.h>
#include <core/profile.h>
#include <view/view.h>
#include <view/wx_view_controls.h>
#include <view/zoom_controller.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_dispatcher.h>
#include <trace_helpers.h>
#include <settings/common_settings.h>
#include <math/util.h> // for KiROUND
#include <geometry/geometry_utils.h>
#include <widgets/ui_common.h>
#include <class_draw_panel_gal.h>
#include <eda_draw_frame.h>
#include <kiway.h>
#include <kiplatform/ui.h>
#include <wx/log.h>

#ifdef __WXMSW__
#define USE_MOUSE_CAPTURE
#endif

using namespace KIGFX;

const wxEventType WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE = wxNewEventType();


static std::unique_ptr<ZOOM_CONTROLLER> GetZoomControllerForPlatform( bool aAcceleration )
{
#ifdef __WXMAC__
    // On Apple pointer devices, wheel events occur frequently and with
    // smaller rotation values.  For those devices, let's handle zoom
    // based on the rotation amount rather than the time difference.
    return std::make_unique<CONSTANT_ZOOM_CONTROLLER>( CONSTANT_ZOOM_CONTROLLER::MAC_SCALE );
#elif __WXGTK3__
    // GTK3 is similar, but the scale constant is smaller
    return std::make_unique<CONSTANT_ZOOM_CONTROLLER>( CONSTANT_ZOOM_CONTROLLER::GTK3_SCALE );
#else
    if( aAcceleration )
        return std::make_unique<ACCELERATING_ZOOM_CONTROLLER>();
    else
        return std::make_unique<CONSTANT_ZOOM_CONTROLLER>( CONSTANT_ZOOM_CONTROLLER::MSW_SCALE );
#endif
}


WX_VIEW_CONTROLS::WX_VIEW_CONTROLS( VIEW* aView, EDA_DRAW_PANEL_GAL* aParentPanel ) :
        VIEW_CONTROLS( aView ),
        m_state( IDLE ),
        m_parentPanel( aParentPanel ),
        m_dragStartPoint( 0, 0 ),
        m_panDirection( 0, 0 ),
        m_scrollScale( 1.0, 1.0 ),
        m_scrollPos( 0, 0 ),
        m_zoomStartPoint( 0, 0 ),
        m_cursorPos( 0, 0 ),
        m_updateCursor( true ),
        m_metaPanning( false ),
        m_metaPanStart( 0, 0 ),
        m_infinitePanWorks( false ),
        m_gestureLastZoomFactor( 1.0 ),
        m_gestureLastPos( 0, 0 )
{
    LoadSettings();

    m_MotionEventCounter = std::make_unique<PROF_COUNTER>( "Mouse motion events" );

    m_parentPanel->Connect( wxEVT_MOTION,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onMotion ), nullptr, this );
    m_parentPanel->Connect( wxEVT_MAGNIFY,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onMagnify ), nullptr, this );
    m_parentPanel->Connect( wxEVT_MOUSEWHEEL,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onWheel ), nullptr, this );
    m_parentPanel->Connect( wxEVT_MIDDLE_UP,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), nullptr, this );
    m_parentPanel->Connect( wxEVT_MIDDLE_DOWN,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), nullptr, this );
    m_parentPanel->Connect( wxEVT_LEFT_UP,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), nullptr, this );
    m_parentPanel->Connect( wxEVT_LEFT_DOWN,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), nullptr, this );
    m_parentPanel->Connect( wxEVT_RIGHT_UP,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), nullptr, this );
    m_parentPanel->Connect( wxEVT_RIGHT_DOWN,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), nullptr, this );
#if defined __WXMSW__
    m_parentPanel->Connect( wxEVT_ENTER_WINDOW,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onEnter ), nullptr, this );
#endif
    m_parentPanel->Connect( wxEVT_LEAVE_WINDOW,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onLeave ), nullptr, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_THUMBTRACK,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), nullptr, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_PAGEUP,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), nullptr, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_PAGEDOWN,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), nullptr, this );

    m_parentPanel->Connect( wxEVT_SCROLLWIN_BOTTOM,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), nullptr, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_TOP,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), nullptr, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_LINEUP,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), nullptr, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_LINEDOWN,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), nullptr, this );
#if defined USE_MOUSE_CAPTURE
    m_parentPanel->Connect( wxEVT_MOUSE_CAPTURE_LOST,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onCaptureLost ), nullptr, this );
#endif

#ifndef __WXOSX__
    if( m_parentPanel->EnableTouchEvents( wxTOUCH_ZOOM_GESTURE | wxTOUCH_PAN_GESTURES ) )
    {
        m_parentPanel->Connect( wxEVT_GESTURE_ZOOM,
                                wxZoomGestureEventHandler( WX_VIEW_CONTROLS::onZoomGesture ),
                                nullptr, this );

        m_parentPanel->Connect( wxEVT_GESTURE_PAN,
                                wxPanGestureEventHandler( WX_VIEW_CONTROLS::onPanGesture ), nullptr,
                                this );
    }
#endif

    m_cursorWarped = false;

    m_panTimer.SetOwner( this );
    Connect( wxEVT_TIMER, wxTimerEventHandler( WX_VIEW_CONTROLS::onTimer ), nullptr, this );

    m_settings.m_lastKeyboardCursorPositionValid = false;
    m_settings.m_lastKeyboardCursorPosition = { 0.0, 0.0 };
    m_settings.m_lastKeyboardCursorCommand = 0;
}


WX_VIEW_CONTROLS::~WX_VIEW_CONTROLS()
{
#if defined USE_MOUSE_CAPTURE
    if( m_parentPanel->HasCapture() )
        m_parentPanel->ReleaseMouse();
#endif
}


void WX_VIEW_CONTROLS::LoadSettings()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    m_settings.m_warpCursor            = cfg->m_Input.center_on_zoom;
    m_settings.m_focusFollowSchPcb     = cfg->m_Input.focus_follow_sch_pcb;
    m_settings.m_autoPanSettingEnabled = cfg->m_Input.auto_pan;
    m_settings.m_autoPanAcceleration   = cfg->m_Input.auto_pan_acceleration;
    m_settings.m_horizontalPan         = cfg->m_Input.horizontal_pan;
    m_settings.m_zoomAcceleration      = cfg->m_Input.zoom_acceleration;
    m_settings.m_zoomSpeed             = cfg->m_Input.zoom_speed;
    m_settings.m_zoomSpeedAuto         = cfg->m_Input.zoom_speed_auto;
    m_settings.m_scrollModifierZoom    = cfg->m_Input.scroll_modifier_zoom;
    m_settings.m_scrollModifierPanH    = cfg->m_Input.scroll_modifier_pan_h;
    m_settings.m_scrollModifierPanV    = cfg->m_Input.scroll_modifier_pan_v;
    m_settings.m_dragLeft              = cfg->m_Input.drag_left;
    m_settings.m_dragMiddle            = cfg->m_Input.drag_middle;
    m_settings.m_dragRight             = cfg->m_Input.drag_right;
    m_settings.m_scrollReverseZoom     = cfg->m_Input.reverse_scroll_zoom;
    m_settings.m_scrollReversePanH     = cfg->m_Input.reverse_scroll_pan_h;
    m_settings.m_motionPanModifier     = cfg->m_Input.motion_pan_modifier;

    m_zoomController.reset();

    if( cfg->m_Input.zoom_speed_auto )
    {
        m_zoomController = GetZoomControllerForPlatform( cfg->m_Input.zoom_acceleration );
    }
    else
    {
        if( cfg->m_Input.zoom_acceleration )
        {
            m_zoomController =
                    std::make_unique<ACCELERATING_ZOOM_CONTROLLER>( cfg->m_Input.zoom_speed );
        }
        else
        {
            double scale = CONSTANT_ZOOM_CONTROLLER::MANUAL_SCALE_FACTOR * cfg->m_Input.zoom_speed;

            m_zoomController = std::make_unique<CONSTANT_ZOOM_CONTROLLER>( scale );
        }
    }
}


void WX_VIEW_CONTROLS::onMotion( wxMouseEvent& aEvent )
{
    ( *m_MotionEventCounter )++;

    // Because Weston sends a motion event to previous location after warping the pointer
    wxPoint mouseRel = m_parentPanel->ScreenToClient( KIPLATFORM::UI::GetMousePosition() );

    bool     isAutoPanning = false;
    int      x = mouseRel.x;
    int      y = mouseRel.y;
    VECTOR2D mousePos( x, y );

    // Clear keyboard cursor position flag when actual mouse motion is detected
    // (i.e., not from cursor warping and position has changed)
    if( !m_cursorWarped && m_settings.m_lastKeyboardCursorPositionValid )
    {
        VECTOR2I screenPos( x, y );
        VECTOR2I keyboardScreenPos = m_view->ToScreen( m_settings.m_lastKeyboardCursorPosition );

        // If mouse has moved to a different position than the keyboard cursor position,
        // clear the keyboard position flag to allow mouse control
        if( screenPos != keyboardScreenPos )
        {
            m_settings.m_lastKeyboardCursorPositionValid = false;
            m_settings.m_lastKeyboardCursorPosition = { 0.0, 0.0 };
        }
    }

    // Automatic focus switching between SCH and PCB windows on canvas mouse motion
    if( m_settings.m_focusFollowSchPcb )
    {
        if( EDA_DRAW_FRAME* frame = m_parentPanel->GetParentEDAFrame() )
        {
            KIWAY_PLAYER* otherFrame = nullptr;

            if( frame->IsType( FRAME_PCB_EDITOR ) )
            {
                otherFrame = frame->Kiway().Player( FRAME_SCH, false );
            }
            else if( frame->IsType( FRAME_SCH ) )
            {
                otherFrame = frame->Kiway().Player( FRAME_PCB_EDITOR, false );
            }

            if( otherFrame && KIPLATFORM::UI::IsWindowActive( otherFrame )
                && !KIPLATFORM::UI::IsWindowActive( frame ) )
            {
                frame->Raise();
            }
        }
    }

    if( m_settings.m_motionPanModifier != WXK_NONE
        && wxGetKeyState( static_cast<wxKeyCode>( m_settings.m_motionPanModifier ) ) )
    {
        if( !m_metaPanning )
        {
            m_metaPanning = true;
            m_metaPanStart = mousePos;
            aEvent.StopPropagation();
        }
        else
        {
            VECTOR2D d = m_metaPanStart - mousePos;
            m_metaPanStart = mousePos;
            VECTOR2D delta = m_view->ToWorld( d, false );
            m_view->SetCenter( m_view->GetCenter() + delta );
            aEvent.StopPropagation();
        }

        if( m_updateCursor )
            m_cursorPos = GetClampedCoords( m_view->ToWorld( mousePos ) );
        else
            m_updateCursor = true;

        aEvent.Skip();
        return;
    }
    else
    {
        m_metaPanning = false;
    }

    if( m_state != DRAG_PANNING && m_state != DRAG_ZOOMING )
        handleCursorCapture( x, y );

    if( m_settings.m_autoPanEnabled && m_settings.m_autoPanSettingEnabled )
        isAutoPanning = handleAutoPanning( aEvent );

    if( !isAutoPanning && aEvent.Dragging() )
    {
        if( m_state == DRAG_PANNING )
        {
            static bool justWarped = false;
            int         warpX = 0;
            int         warpY = 0;
            wxSize      parentSize = m_parentPanel->GetClientSize();

            if( x < 0 )
            {
                warpX = parentSize.x;
            }
            else if( x >= parentSize.x )
            {
                warpX = -parentSize.x;
            }

            if( y < 0 )
            {
                warpY = parentSize.y;
            }
            else if( y >= parentSize.y )
            {
                warpY = -parentSize.y;
            }

            if( !justWarped )
            {
                VECTOR2D d = m_dragStartPoint - mousePos;
                m_dragStartPoint = mousePos;
                VECTOR2D delta = m_view->ToWorld( d, false );
                m_view->SetCenter( m_view->GetCenter() + delta );
                aEvent.StopPropagation();
            }

            if( warpX || warpY )
            {
                if( !justWarped )
                {
                    if( m_infinitePanWorks && KIPLATFORM::UI::WarpPointer( m_parentPanel, x + warpX, y + warpY ) )
                    {
                        m_dragStartPoint += VECTOR2D( warpX, warpY );
                        justWarped = true;
                    }
                }
                else
                {
                    justWarped = false;
                }
            }
            else
            {
                justWarped = false;
            }
        }
        else if( m_state == DRAG_ZOOMING )
        {
            static bool justWarped = false;
            int         warpY = 0;
            wxSize      parentSize = m_parentPanel->GetClientSize();

            if( y < 0 )
            {
                warpY = parentSize.y;
            }
            else if( y >= parentSize.y )
            {
                warpY = -parentSize.y;
            }

            if( !justWarped )
            {
                VECTOR2D d = m_dragStartPoint - mousePos;
                m_dragStartPoint = mousePos;

                double scale = exp( d.y * m_settings.m_zoomSpeed * 0.001 );

                wxLogTrace( traceZoomScroll, wxString::Format( "dy: %f  scale: %f", d.y, scale ) );

                m_view->SetScale( m_view->GetScale() * scale, m_view->ToWorld( m_zoomStartPoint ) );
                aEvent.StopPropagation();
            }

            if( warpY )
            {
                if( !justWarped )
                {
                    KIPLATFORM::UI::WarpPointer( m_parentPanel, x, y + warpY );
                    m_dragStartPoint += VECTOR2D( 0, warpY );
                    justWarped = true;
                }
                else
                    justWarped = false;
            }
            else
            {
                justWarped = false;
            }
        }
    }

    if( m_updateCursor )        // do not update the cursor position if it was explicitly set
        m_cursorPos = GetClampedCoords( m_view->ToWorld( mousePos ) );
    else
        m_updateCursor = true;

    aEvent.Skip();
}


void WX_VIEW_CONTROLS::onWheel( wxMouseEvent& aEvent )
{
    const double wheelPanSpeed = 0.001;
    const int    axis = aEvent.GetWheelAxis();

    if( axis == wxMOUSE_WHEEL_HORIZONTAL && !m_settings.m_horizontalPan )
        return;

    // Pick the modifier, if any.  Shift beats control beats alt, we don't support more than one.
    int nMods = 0;
    int modifiers = 0;

    if( aEvent.ShiftDown() )
    {
        nMods += 1;
        modifiers = WXK_SHIFT;
    }

    if( aEvent.ControlDown() )
    {
        nMods += 1;
        modifiers = modifiers == 0 ? WXK_CONTROL : modifiers;
    }

    if( aEvent.AltDown() )
    {
        nMods += 1;
        modifiers = modifiers == 0 ? WXK_ALT : modifiers;
    }

    // Zero or one modifier is view control
    if( nMods <= 1 )
    {
        // Restrict zoom handling to the vertical axis, otherwise horizontal
        // scrolling events (e.g. touchpads and some mice) end up interpreted
        // as vertical scroll events and confuse the user.
        if( modifiers == m_settings.m_scrollModifierZoom && axis == wxMOUSE_WHEEL_VERTICAL )
        {
            const int    rotation = aEvent.GetWheelRotation() * ( m_settings.m_scrollReverseZoom ? -1 : 1 );
            const double zoomScale = m_zoomController->GetScaleForRotation( rotation );

            if( IsCursorWarpingEnabled() )
            {
                CenterOnCursor();
                m_view->SetScale( m_view->GetScale() * zoomScale );
            }
            else
            {
                const VECTOR2D anchor = m_view->ToWorld( VECTOR2D( aEvent.GetX(), aEvent.GetY() ) );
                m_view->SetScale( m_view->GetScale() * zoomScale, anchor );
            }

            // Refresh the zoom level and mouse position on message panel
            // (mouse position has not changed, only the zoom level has changed):
            refreshMouse( true );
        }
        else
        {
            // Scrolling
            VECTOR2D scrollVec = m_view->ToWorld( m_view->GetScreenPixelSize(), false )
                                 * ( (double) aEvent.GetWheelRotation() * wheelPanSpeed );
            double scrollX = 0.0;
            double scrollY = 0.0;
            bool   hReverse = false;

            if( axis != wxMOUSE_WHEEL_HORIZONTAL )
                hReverse = m_settings.m_scrollReversePanH;

            if( axis == wxMOUSE_WHEEL_HORIZONTAL || modifiers == m_settings.m_scrollModifierPanH )
            {
                if( hReverse )
                    scrollX = scrollVec.x;
                else
                    scrollX = ( axis == wxMOUSE_WHEEL_HORIZONTAL ) ? scrollVec.x : -scrollVec.x;
            }
            else
            {
                scrollY = -scrollVec.y;
            }

            VECTOR2D delta( scrollX, scrollY );

            m_view->SetCenter( m_view->GetCenter() + delta );
            refreshMouse( true );
        }

        // Do not skip this event, otherwise wxWidgets will fire
        // 3 wxEVT_SCROLLWIN_LINEUP or wxEVT_SCROLLWIN_LINEDOWN (normal wxWidgets behavior)
        // and we do not want that.
    }
    else
    {
        // When we have multiple mods, forward it for tool handling
        aEvent.Skip();
    }
}


void WX_VIEW_CONTROLS::onMagnify( wxMouseEvent& aEvent )
{
    // Scale based on the magnification from our underlying magnification event.
    VECTOR2D anchor = m_view->ToWorld( VECTOR2D( aEvent.GetX(), aEvent.GetY() ) );
    m_view->SetScale( m_view->GetScale() * ( aEvent.GetMagnification() + 1.0f ), anchor );

    aEvent.Skip();
}


void WX_VIEW_CONTROLS::setState( STATE aNewState )
{
    m_state = aNewState;
}


void WX_VIEW_CONTROLS::onButton( wxMouseEvent& aEvent )
{
    switch( m_state )
    {
    case IDLE:
    case AUTO_PANNING:
        if( ( aEvent.MiddleDown() && m_settings.m_dragMiddle == MOUSE_DRAG_ACTION::PAN )
            || ( aEvent.RightDown() && m_settings.m_dragRight == MOUSE_DRAG_ACTION::PAN ) )
        {
            m_dragStartPoint = VECTOR2D( aEvent.GetX(), aEvent.GetY() );
            setState( DRAG_PANNING );
            m_infinitePanWorks = KIPLATFORM::UI::InfiniteDragPrepareWindow( m_parentPanel );

#if defined USE_MOUSE_CAPTURE
            if( !m_parentPanel->HasCapture() )
                m_parentPanel->CaptureMouse();
#endif
        }
        else if( ( aEvent.MiddleDown() && m_settings.m_dragMiddle == MOUSE_DRAG_ACTION::ZOOM )
                 || ( aEvent.RightDown() && m_settings.m_dragRight == MOUSE_DRAG_ACTION::ZOOM ) )
        {
            m_dragStartPoint = VECTOR2D( aEvent.GetX(), aEvent.GetY() );
            m_zoomStartPoint = m_dragStartPoint;
            setState( DRAG_ZOOMING );

#if defined USE_MOUSE_CAPTURE
            if( !m_parentPanel->HasCapture() )
                m_parentPanel->CaptureMouse();
#endif
        }

        if( aEvent.LeftUp() )
            setState( IDLE ); // Stop autopanning when user release left mouse button

        break;

    case DRAG_ZOOMING:
    case DRAG_PANNING:
        if( aEvent.MiddleUp() || aEvent.LeftUp() || aEvent.RightUp() )
        {
            setState( IDLE );
            KIPLATFORM::UI::InfiniteDragReleaseWindow();

#if defined USE_MOUSE_CAPTURE
            if( !m_settings.m_cursorCaptured && m_parentPanel->HasCapture() )
                m_parentPanel->ReleaseMouse();
#endif
        }

        break;
    }

    aEvent.Skip();
}


void WX_VIEW_CONTROLS::onEnter( wxMouseEvent& aEvent )
{
    // Avoid stealing focus from text controls
    // This is particularly important for users using On-Screen-Keyboards
    // They may move the mouse over the canvas to reach the keyboard
    if( KIUI::IsInputControlFocused() )
    {
        return;
    }

#if defined( _WIN32 ) || defined( __WXGTK__ )
    // Win32 and some *nix WMs transmit mouse move and wheel events to all controls below the
    // mouse regardless of focus.  Forcing the focus here will cause the EDA FRAMES to immediately
    // become the top level active window.
    if( m_parentPanel->GetParent() != nullptr )
    {
        // this assumes the parent panel's parent is the eda window
        if( KIPLATFORM::UI::IsWindowActive( m_parentPanel->GetParent() ) )
        {
            m_parentPanel->SetFocus();
        }
    }
#else
    m_parentPanel->SetFocus();
#endif
}


void WX_VIEW_CONTROLS::onLeave( wxMouseEvent& aEvent )
{
#if !defined USE_MOUSE_CAPTURE
    onMotion( aEvent );
#endif
}


void WX_VIEW_CONTROLS::onCaptureLost( wxMouseEvent& aEvent )
{
    // This method must be present to suppress the capture-lost assertion

    // Set the flag to allow calling m_parentPanel->CaptureMouse()
    // Note: One cannot call m_parentPanel->CaptureMouse() twice, this is not accepted
    // by wxWidgets (MSW specific) so we need this guard
    m_parentPanel->m_MouseCapturedLost = true;
}


void WX_VIEW_CONTROLS::onTimer( wxTimerEvent& aEvent )
{
    switch( m_state )
    {
    case AUTO_PANNING:
    {
        if( !m_settings.m_autoPanEnabled )
        {
            setState( IDLE );
            return;
        }

#ifdef __WXMSW__
        // Hackfix: It's possible for the mouse to leave the canvas
        // without triggering any leave events on windows
        // Use a MSW only wx function
        if( !m_parentPanel->IsMouseInWindow() )
        {
            m_panTimer.Stop();
            setState( IDLE );
            return;
        }
#endif

        if( !m_parentPanel->HasFocus() && !m_parentPanel->StatusPopupHasFocus() )
        {
            setState( IDLE );
            return;
        }

        double borderSize = std::min( m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().x,
                                      m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().y );

        // When the mouse cursor is outside the area with no pan,
        // m_panDirection is the dist to this area limit ( in pixels )
        // It will be used also as pan value (the pan speed depends on this dist).
        VECTOR2D dir( m_panDirection );

        // When the mouse cursor is outside the area with no pan, the pan value
        // is accelerated depending on the dist between the area and the cursor
        float accel = 0.5f + ( m_settings.m_autoPanAcceleration / 5.0f );

        // For a small mouse cursor dist to area, just use the distance.
        // But for a dist > borderSize / 2, use an accelerated pan value

        if( dir.EuclideanNorm() >= borderSize ) // far from area limits
            dir = dir.Resize( borderSize * accel );
        else if( dir.EuclideanNorm() > borderSize / 2 ) // Near from area limits
            dir = dir.Resize( borderSize );

        dir = m_view->ToWorld( dir, false );
        m_view->SetCenter( m_view->GetCenter() + dir );

        refreshMouse( true );

        m_panTimer.Start();
    }
    break;

    case IDLE: // Just remove unnecessary warnings
    case DRAG_PANNING:
    case DRAG_ZOOMING: break;
    }
}


void WX_VIEW_CONTROLS::onZoomGesture( wxZoomGestureEvent& aEvent )
{
    if( aEvent.IsGestureStart() )
    {
        m_gestureLastZoomFactor = 1.0;
        m_gestureLastPos = VECTOR2D( aEvent.GetPosition().x, aEvent.GetPosition().y );
    }

    VECTOR2D evtPos( aEvent.GetPosition().x, aEvent.GetPosition().y );
    VECTOR2D deltaWorld = m_view->ToWorld( evtPos - m_gestureLastPos, false );

    m_view->SetCenter( m_view->GetCenter() - deltaWorld );

    m_view->SetScale( m_view->GetScale() * aEvent.GetZoomFactor() / m_gestureLastZoomFactor,
                      m_view->ToWorld( evtPos ) );

    m_gestureLastZoomFactor = aEvent.GetZoomFactor();
    m_gestureLastPos = evtPos;

    refreshMouse( true );
}


void WX_VIEW_CONTROLS::onPanGesture( wxPanGestureEvent& aEvent )
{
    VECTOR2I screenDelta( aEvent.GetDelta().x, aEvent.GetDelta().y );
    VECTOR2D deltaWorld = m_view->ToWorld( screenDelta, false );

    m_view->SetCenter( m_view->GetCenter() - deltaWorld );

    refreshMouse( true );
}


void WX_VIEW_CONTROLS::onScroll( wxScrollWinEvent& aEvent )
{
    const double linePanDelta = 0.05;
    const double pagePanDelta = 0.5;

    int type = aEvent.GetEventType();
    int dir = aEvent.GetOrientation();

    if( type == wxEVT_SCROLLWIN_THUMBTRACK )
    {
        auto        center = m_view->GetCenter();
        const auto& boundary = m_view->GetBoundary();

        // Flip scroll direction in flipped view
        const double xstart = ( m_view->IsMirroredX() ? boundary.GetRight() : boundary.GetLeft() );
        const double xdelta = ( m_view->IsMirroredX() ? -1 : 1 );

        if( dir == wxHORIZONTAL )
            center.x = xstart + xdelta * ( aEvent.GetPosition() / m_scrollScale.x );
        else
            center.y = boundary.GetTop() + aEvent.GetPosition() / m_scrollScale.y;

        m_view->SetCenter( center );
    }
    else if( type == wxEVT_SCROLLWIN_THUMBRELEASE || type == wxEVT_SCROLLWIN_TOP || type == wxEVT_SCROLLWIN_BOTTOM )
    {
        // Do nothing on thumb release, we don't care about it.
        // We don't have a concept of top or bottom in our viewport, so ignore those events.
    }
    else
    {
        double dist = 0;

        if( type == wxEVT_SCROLLWIN_PAGEUP )
        {
            dist = pagePanDelta;
        }
        else if( type == wxEVT_SCROLLWIN_PAGEDOWN )
        {
            dist = -pagePanDelta;
        }
        else if( type == wxEVT_SCROLLWIN_LINEUP )
        {
            dist = linePanDelta;
        }
        else if( type == wxEVT_SCROLLWIN_LINEDOWN )
        {
            dist = -linePanDelta;
        }
        else
        {
            wxCHECK_MSG( false, /* void */, wxT( "Unhandled event type" ) );
        }

        VECTOR2D scroll = m_view->ToWorld( m_view->GetScreenPixelSize(), false ) * dist;

        double scrollX = 0.0;
        double scrollY = 0.0;

        if( dir == wxHORIZONTAL )
            scrollX = -scroll.x;
        else
            scrollY = -scroll.y;

        VECTOR2D delta( scrollX, scrollY );

        m_view->SetCenter( m_view->GetCenter() + delta );
    }

    m_parentPanel->Refresh();
}


void WX_VIEW_CONTROLS::CaptureCursor( bool aEnabled )
{
#if defined USE_MOUSE_CAPTURE
    // Note: for some reason, m_parentPanel->HasCapture() can be false even if CaptureMouse()
    // was called (i.e. mouse was captured, so when need to test m_MouseCapturedLost to be
    // sure a wxEVT_MOUSE_CAPTURE_LOST event was fired before. Otherwise wxMSW complains
    // The IsModalDialogFocused is checked because it's possible to start a capture
    // due to event ordering while a modal dialog was just opened, the mouse capture steels focus
    // from the modal and causes odd behavior
    if( aEnabled && !m_parentPanel->HasCapture() && m_parentPanel->m_MouseCapturedLost
        && !KIUI::IsModalDialogFocused() )
    {
        m_parentPanel->CaptureMouse();

        // Clear the flag to allow calling m_parentPanel->CaptureMouse()
        // Calling it without calling ReleaseMouse() is not accepted by wxWidgets (MSW specific)
        m_parentPanel->m_MouseCapturedLost = false;
    }
    else if( !aEnabled && m_parentPanel->HasCapture() && m_state != DRAG_PANNING && m_state != DRAG_ZOOMING )
    {
        m_parentPanel->ReleaseMouse();

        // Mouse is released, calling CaptureMouse() is allowed now:
        m_parentPanel->m_MouseCapturedLost = true;
    }
#endif
    VIEW_CONTROLS::CaptureCursor( aEnabled );
}


void WX_VIEW_CONTROLS::CancelDrag()
{
    if( m_state == DRAG_PANNING || m_state == DRAG_ZOOMING )
    {
        setState( IDLE );

#if defined USE_MOUSE_CAPTURE
        if( !m_settings.m_cursorCaptured && m_parentPanel->HasCapture() )
            m_parentPanel->ReleaseMouse();
#endif
    }

    m_metaPanning = false;
}


VECTOR2D WX_VIEW_CONTROLS::GetMousePosition( bool aWorldCoordinates ) const
{
    wxPoint  msp = getMouseScreenPosition();
    VECTOR2D screenPos( msp.x, msp.y );

    return aWorldCoordinates ? GetClampedCoords( m_view->ToWorld( screenPos ) ) : screenPos;
}


VECTOR2D WX_VIEW_CONTROLS::GetRawCursorPosition( bool aEnableSnapping ) const
{
    GAL* gal = m_view->GetGAL();

    if( aEnableSnapping && gal->GetGridSnapping() )
    {
        return gal->GetGridPoint( m_cursorPos );
    }
    else
    {
        return m_cursorPos;
    }
}


VECTOR2D WX_VIEW_CONTROLS::GetCursorPosition( bool aEnableSnapping ) const
{
    if( m_settings.m_forceCursorPosition )
    {
        return m_settings.m_forcedPosition;
    }
    else
    {
        return GetClampedCoords( GetRawCursorPosition( aEnableSnapping ) );
    }
}


void WX_VIEW_CONTROLS::SetCursorPosition( const VECTOR2D& aPosition, bool aWarpView, bool aTriggeredByArrows,
                                          long aArrowCommand )
{
    m_updateCursor = false;

    VECTOR2D clampedPosition = GetClampedCoords( aPosition );

    if( aTriggeredByArrows )
    {
        m_settings.m_lastKeyboardCursorPositionValid = true;
        m_settings.m_lastKeyboardCursorPosition = clampedPosition;
        m_settings.m_lastKeyboardCursorCommand = aArrowCommand;
        m_cursorWarped = false;
    }
    else
    {
        m_settings.m_lastKeyboardCursorPositionValid = false;
        m_settings.m_lastKeyboardCursorPosition = { 0.0, 0.0 };
        m_settings.m_lastKeyboardCursorCommand = 0;
        m_cursorWarped = true;
    }

    WarpMouseCursor( clampedPosition, true, aWarpView );
    m_cursorPos = clampedPosition;
}


void WX_VIEW_CONTROLS::SetCrossHairCursorPosition( const VECTOR2D& aPosition, bool aWarpView = true )
{
    m_updateCursor = false;

    VECTOR2D clampedPosition = GetClampedCoords( aPosition );

    const VECTOR2I& screenSize = m_view->GetGAL()->GetScreenPixelSize();
    BOX2I           screen( VECTOR2I( 0, 0 ), screenSize );
    VECTOR2D        screenPos = m_view->ToScreen( clampedPosition );

    if( aWarpView && !screen.Contains( screenPos ) )
        m_view->SetCenter( clampedPosition );

    m_cursorPos = clampedPosition;
}


void WX_VIEW_CONTROLS::WarpMouseCursor( const VECTOR2D& aPosition, bool aWorldCoordinates, bool aWarpView )
{
    if( aWorldCoordinates )
    {
        const VECTOR2I& screenSize = m_view->GetGAL()->GetScreenPixelSize();
        BOX2I           screen( VECTOR2I( 0, 0 ), screenSize );
        VECTOR2D        clampedPosition = GetClampedCoords( aPosition );
        VECTOR2D        screenPos = m_view->ToScreen( clampedPosition );

        if( !screen.Contains( screenPos ) )
        {
            if( aWarpView )
            {
                m_view->SetCenter( clampedPosition );
                KIPLATFORM::UI::WarpPointer( m_parentPanel, screenSize.x / 2, screenSize.y / 2 );
            }
        }
        else
        {
            KIPLATFORM::UI::WarpPointer( m_parentPanel, screenPos.x, screenPos.y );
        }
    }
    else
    {
        KIPLATFORM::UI::WarpPointer( m_parentPanel, aPosition.x, aPosition.y );
    }

    // If we are not refreshing because of mouse movement, don't set the modifiers because we
    // are refreshing for keyboard movement, which uses the same modifiers for other actions
    refreshMouse( m_updateCursor );
}


void WX_VIEW_CONTROLS::CenterOnCursor()
{
    const VECTOR2I& screenSize = m_view->GetGAL()->GetScreenPixelSize();
    VECTOR2D        screenCenter( screenSize / 2 );

    if( GetMousePosition( false ) != screenCenter )
    {
        VECTOR2D newCenter = GetCursorPosition();

        if( KIPLATFORM::UI::WarpPointer( m_parentPanel, screenCenter.x, screenCenter.y ) )
        {
            m_view->SetCenter( newCenter );
            m_dragStartPoint = screenCenter;
        }
    }
}


void WX_VIEW_CONTROLS::PinCursorInsideNonAutoscrollArea( bool aWarpMouseCursor )
{
    int border = std::min( m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().x,
                           m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().y );
    border += 2;

    VECTOR2D topLeft( border, border );
    VECTOR2D botRight( m_view->GetScreenPixelSize().x - border, m_view->GetScreenPixelSize().y - border );

    topLeft = m_view->ToWorld( topLeft );
    botRight = m_view->ToWorld( botRight );

    VECTOR2D pos = GetMousePosition( true );

    if( pos.x < topLeft.x )
        pos.x = topLeft.x;
    else if( pos.x > botRight.x )
        pos.x = botRight.x;

    if( pos.y < topLeft.y )
        pos.y = topLeft.y;
    else if( pos.y > botRight.y )
        pos.y = botRight.y;

    SetCursorPosition( pos, false, false, 0 );

    if( aWarpMouseCursor )
        WarpMouseCursor( pos, true );
}


bool WX_VIEW_CONTROLS::handleAutoPanning( const wxMouseEvent& aEvent )
{
    VECTOR2I p( aEvent.GetX(), aEvent.GetY() );
    VECTOR2I pKey( m_view->ToScreen( m_settings.m_lastKeyboardCursorPosition ) );

    if( m_cursorWarped || ( m_settings.m_lastKeyboardCursorPositionValid && p == pKey ) )
    {
        // last cursor move event came from keyboard cursor control. If auto-panning is enabled
        // and the next position is inside the autopan zone, check if it really came from a mouse
        // event, otherwise disable autopan temporarily. Also temporarily disable autopan if the
        // cursor is in the autopan zone because the application warped the cursor.

        m_cursorWarped = false;
        return true;
    }

    m_cursorWarped = false;

    // Compute areas where autopanning is active
    int borderStart = std::min( m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().x,
                                m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().y );
    borderStart = std::max( borderStart, 2 );
    int borderEndX = m_view->GetScreenPixelSize().x - borderStart;
    int borderEndY = m_view->GetScreenPixelSize().y - borderStart;

    if( p.x < borderStart )
        m_panDirection.x = -( borderStart - p.x );
    else if( p.x > borderEndX )
        m_panDirection.x = ( p.x - borderEndX );
    else
        m_panDirection.x = 0;

    if( p.y < borderStart )
        m_panDirection.y = -( borderStart - p.y );
    else if( p.y > borderEndY )
        m_panDirection.y = ( p.y - borderEndY );
    else
        m_panDirection.y = 0;

    bool borderHit = ( m_panDirection.x != 0 || m_panDirection.y != 0 );

    switch( m_state )
    {
    case AUTO_PANNING:
        if( !borderHit )
        {
            m_panTimer.Stop();
            setState( IDLE );

            return false;
        }

        return true;

    case IDLE:
        if( borderHit )
        {
            setState( AUTO_PANNING );
            m_panTimer.Start( (int) ( 250.0 / 60.0 ), true );

            return true;
        }

        return false;

    case DRAG_PANNING:
    case DRAG_ZOOMING: return false;
    }

    wxCHECK_MSG( false, false, wxT( "This line should never be reached" ) );

    return false;
}


void WX_VIEW_CONTROLS::handleCursorCapture( int x, int y )
{
    if( m_settings.m_cursorCaptured )
    {
        bool   warp = false;
        wxSize parentSize = m_parentPanel->GetClientSize();

        if( x < 0 )
        {
            x = 0;
            warp = true;
        }
        else if( x >= parentSize.x )
        {
            x = parentSize.x - 1;
            warp = true;
        }

        if( y < 0 )
        {
            y = 0;
            warp = true;
        }
        else if( y >= parentSize.y )
        {
            y = parentSize.y - 1;
            warp = true;
        }

        if( warp )
            KIPLATFORM::UI::WarpPointer( m_parentPanel, x, y );
    }
}


void WX_VIEW_CONTROLS::refreshMouse( bool aSetModifiers )
{
    // Notify tools that the cursor position has changed in the world coordinates
    wxMouseEvent moveEvent( EVT_REFRESH_MOUSE );
    wxPoint      msp = getMouseScreenPosition();
    moveEvent.SetX( msp.x );
    moveEvent.SetY( msp.y );

    if( aSetModifiers )
    {
        // Set the modifiers state
        moveEvent.SetControlDown( wxGetKeyState( WXK_CONTROL ) );
        moveEvent.SetShiftDown( wxGetKeyState( WXK_SHIFT ) );
        moveEvent.SetAltDown( wxGetKeyState( WXK_ALT ) );
    }

    m_cursorPos = GetClampedCoords( m_view->ToWorld( VECTOR2D( msp.x, msp.y ) ) );
    wxPostEvent( m_parentPanel, moveEvent );
}


wxPoint WX_VIEW_CONTROLS::getMouseScreenPosition() const
{
    wxPoint msp = KIPLATFORM::UI::GetMousePosition();
    m_parentPanel->ScreenToClient( &msp.x, &msp.y );
    return msp;
}


void WX_VIEW_CONTROLS::UpdateScrollbars()
{
    const BOX2D  viewport = m_view->GetViewport();
    const BOX2D& boundary = m_view->GetBoundary();

    m_scrollScale.x = 2e3 / viewport.GetWidth(); // TODO it does not have to be updated so often
    m_scrollScale.y = 2e3 / viewport.GetHeight();
    VECTOR2I newScroll( ( viewport.Centre().x - boundary.GetLeft() ) * m_scrollScale.x,
                        ( viewport.Centre().y - boundary.GetTop() ) * m_scrollScale.y );

    // We add the width of the scroll bar thumb to the range because the scroll range is given by
    // the full bar while the position is given by the left/top position of the thumb
    VECTOR2I newRange( m_scrollScale.x * boundary.GetWidth() + m_parentPanel->GetScrollThumb( wxSB_HORIZONTAL ),
                       m_scrollScale.y * boundary.GetHeight() + m_parentPanel->GetScrollThumb( wxSB_VERTICAL ) );

    // Flip scroll direction in flipped view
    if( m_view->IsMirroredX() )
        newScroll.x = ( boundary.GetRight() - viewport.Centre().x ) * m_scrollScale.x;

    // Adjust scrollbars only if it is needed. Otherwise there are cases when canvas is continuously
    // refreshed (Windows)
    if( m_scrollPos != newScroll || newRange.x != m_parentPanel->GetScrollRange( wxSB_HORIZONTAL )
        || newRange.y != m_parentPanel->GetScrollRange( wxSB_VERTICAL ) )
    {
        m_parentPanel->SetScrollbars( 1, 1, newRange.x, newRange.y, newScroll.x, newScroll.y, true );
        m_scrollPos = newScroll;

#if !defined( __APPLE__ ) && !defined( WIN32 )
        // Trigger a mouse refresh to get the canvas update in GTK (re-draws the scrollbars).
        // Note that this causes an infinite loop on OSX and Windows (in certain cases) as it
        // generates a paint event.
        refreshMouse( false );
#endif
    }
}


void WX_VIEW_CONTROLS::ForceCursorPosition( bool aEnabled, const VECTOR2D& aPosition )
{
    VECTOR2D clampedPosition = GetClampedCoords( aPosition );

    m_settings.m_forceCursorPosition = aEnabled;
    m_settings.m_forcedPosition = clampedPosition;
}
