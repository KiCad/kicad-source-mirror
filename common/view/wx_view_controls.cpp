/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013-2015 CERN
 * Copyright (C) 2012-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <profile.h>
#include <view/view.h>
#include <view/wx_view_controls.h>
#include <view/zoom_controller.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_dispatcher.h>
#include <trace_helpers.h>
#include <settings/common_settings.h>
#include <math/util.h>      // for KiROUND
#include <widgets/ui_common.h>
#include <class_draw_panel_gal.h>
#include <kiplatform/ui.h>
#include <wx/log.h>


#if defined __WXMSW__
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
        m_scrollScale( 1.0, 1.0 ),
        m_initialZoomScale( 0.0 ),
#ifdef __WXGTK3__
        m_lastTimestamp( 0 ),
#endif
        m_cursorPos( 0, 0 ),
        m_updateCursor( true )
{
    LoadSettings();

    m_MotionEventCounter = std::make_unique<PROF_COUNTER>( "Mouse motion events" );

    m_parentPanel->Connect( wxEVT_MOTION,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onMotion ), nullptr, this );
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
    m_parentPanel->Connect( wxEVT_MAGNIFY,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onMagnify ), nullptr, this );
#endif
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

    m_cursorWarped = false;

    m_panTimer.SetOwner( this );
    this->Connect( wxEVT_TIMER, wxTimerEventHandler( WX_VIEW_CONTROLS::onTimer ), nullptr, this );

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

    bool isAutoPanning = false;
    int x = aEvent.GetX();
    int y = aEvent.GetY();
    VECTOR2D mousePos( x, y );

    if( m_state != DRAG_PANNING && m_state != DRAG_ZOOMING )
        handleCursorCapture( x, y );

    if( m_settings.m_autoPanEnabled && m_settings.m_autoPanSettingEnabled )
        isAutoPanning = handleAutoPanning( aEvent );

    if( !isAutoPanning && aEvent.Dragging() )
    {
        if( m_state == DRAG_PANNING )
        {
            static bool justWarped = false;
            int warpX = 0;
            int warpY = 0;
            wxSize parentSize = m_parentPanel->GetClientSize();

            if( x < 0 )
            {
                warpX = parentSize.x;
            }
            else if(x >= parentSize.x )
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
                VECTOR2D delta = m_view->ToWorld( d, false );
                m_view->SetCenter( m_lookStartPoint + delta );
                aEvent.StopPropagation();
            }

            if( warpX || warpY )
            {
                if( !justWarped )
                {
                    m_parentPanel->WarpPointer( x + warpX, y + warpY );
                    m_dragStartPoint += VECTOR2D( warpX, warpY );
                    justWarped = true;
                }
                else
                    justWarped = false;
            }
            else
                justWarped = false;
        }
        else if( m_state == DRAG_ZOOMING )
        {
            static bool justWarped = false;
            int warpY = 0;
            wxSize parentSize = m_parentPanel->GetClientSize();

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
                double scale = exp( d.y * m_settings.m_zoomSpeed * 0.001 );

                wxLogTrace( traceZoomScroll, wxString::Format( "dy: %f  scale: %f", d.y, scale ) );

                m_view->SetScale( m_initialZoomScale * scale, m_view->ToWorld( m_zoomStartPoint ) );
                aEvent.StopPropagation();
            }

            if( warpY )
            {
                if( !justWarped )
                {
                    m_parentPanel->WarpPointer( x, y + warpY );
                    m_dragStartPoint += VECTOR2D( 0, warpY );
                    justWarped = true;
                }
                else
                    justWarped = false;
            }
            else
                justWarped = false;
        }
    }

    if( m_updateCursor )        // do not update the cursor position if it was explicitly set
        m_cursorPos = m_view->ToWorld( mousePos );
    else
        m_updateCursor = true;

    aEvent.Skip();
}


void WX_VIEW_CONTROLS::onWheel( wxMouseEvent& aEvent )
{
#ifdef __WXGTK3__
    if( aEvent.GetTimestamp() == m_lastTimestamp )
    {
        aEvent.Skip( false );
        return;
    }

    m_lastTimestamp = aEvent.GetTimestamp();
#endif

    const double wheelPanSpeed = 0.001;
    const int    axis = aEvent.GetWheelAxis();

    if( axis == wxMOUSE_WHEEL_HORIZONTAL && !m_settings.m_horizontalPan )
        return;

    // Pick the modifier, if any.  Shift beats control beats alt, we don't support more than one.
    int modifiers =
            aEvent.ShiftDown() ? WXK_SHIFT :
            ( aEvent.ControlDown() ? WXK_CONTROL : ( aEvent.AltDown() ? WXK_ALT : 0 ) );

    // Restrict zoom handling to the vertical axis, otherwise horizontal
    // scrolling events (e.g. touchpads and some mice) end up interpreted
    // as vertical scroll events and confuse the user.
    if( axis == wxMOUSE_WHEEL_VERTICAL && modifiers == m_settings.m_scrollModifierZoom )
    {
        const int    rotation  = aEvent.GetWheelRotation();
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
        refreshMouse();
    }
    else
    {
        // Scrolling
        VECTOR2D scrollVec = m_view->ToWorld( m_view->GetScreenPixelSize(), false ) *
                             ( (double) aEvent.GetWheelRotation() * wheelPanSpeed );
        double scrollX = 0.0;
        double scrollY = 0.0;

        if( axis == wxMOUSE_WHEEL_HORIZONTAL || modifiers == m_settings.m_scrollModifierPanH )
            scrollX = scrollVec.x;
        else
            scrollY = -scrollVec.y;

        VECTOR2D delta( scrollX, scrollY );

        m_view->SetCenter( m_view->GetCenter() + delta );
        refreshMouse();
    }

    // Do not skip this event, otherwise wxWidgets will fire
    // 3 wxEVT_SCROLLWIN_LINEUP or wxEVT_SCROLLWIN_LINEDOWN (normal wxWidgets behavior)
    // and we do not want that.
    m_parentPanel->Refresh();
}


#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
void WX_VIEW_CONTROLS::onMagnify( wxMouseEvent& aEvent )
{
    // Scale based on the magnification from our underlying magnification event.
    VECTOR2D anchor = m_view->ToWorld( VECTOR2D( aEvent.GetX(), aEvent.GetY() ) );
    m_view->SetScale( m_view->GetScale() * ( aEvent.GetMagnification() + 1.0f ), anchor );

    aEvent.Skip();
}
#endif


void WX_VIEW_CONTROLS::onButton( wxMouseEvent& aEvent )
{
    switch( m_state )
    {
    case IDLE:
    case AUTO_PANNING:
        if( ( aEvent.MiddleDown() && m_settings.m_dragMiddle == MOUSE_DRAG_ACTION::PAN ) ||
            ( aEvent.RightDown() && m_settings.m_dragRight == MOUSE_DRAG_ACTION::PAN ) )
        {
            m_dragStartPoint = VECTOR2D( aEvent.GetX(), aEvent.GetY() );
            m_lookStartPoint = m_view->GetCenter();
            m_state = DRAG_PANNING;

#if defined USE_MOUSE_CAPTURE
            if( !m_parentPanel->HasCapture() )
                m_parentPanel->CaptureMouse();
#endif
        }
        else if( ( aEvent.MiddleDown() && m_settings.m_dragMiddle == MOUSE_DRAG_ACTION::ZOOM ) ||
                 ( aEvent.RightDown() && m_settings.m_dragRight == MOUSE_DRAG_ACTION::ZOOM ) )
        {
            m_dragStartPoint   = VECTOR2D( aEvent.GetX(), aEvent.GetY() );
            m_zoomStartPoint = m_dragStartPoint;
            m_initialZoomScale = m_view->GetScale();
            m_state = DRAG_ZOOMING;

#if defined USE_MOUSE_CAPTURE
            if( !m_parentPanel->HasCapture() )
                m_parentPanel->CaptureMouse();
#endif
        }

        if( aEvent.LeftUp() )
            m_state = IDLE;     // Stop autopanning when user release left mouse button

        break;

    case DRAG_ZOOMING:
    case DRAG_PANNING:
        if( aEvent.MiddleUp() || aEvent.LeftUp() || aEvent.RightUp() )
        {
            m_state = IDLE;

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

#if defined( _WIN32 )
    // Win32 transmits mouse move and wheel events to all controls below the mouse regardless
    // of focus.  Forcing the focus here will cause the EDA FRAMES to immediately become the
    // top level active window.
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
            m_state = IDLE;
            return;
        }

        if( !m_parentPanel->HasFocus() )
            break;

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

        if( dir.EuclideanNorm() >= borderSize )         // far from area limits
            dir = dir.Resize( borderSize * accel );
        else if( dir.EuclideanNorm() > borderSize / 2 ) // Near from area limits
            dir = dir.Resize( borderSize );

        dir = m_view->ToWorld( dir, false );
        m_view->SetCenter( m_view->GetCenter() + dir );

        refreshMouse();
    }
    break;

    case IDLE:    // Just remove unnecessary warnings
    case DRAG_PANNING:
    case DRAG_ZOOMING:
        break;
    }
}


void WX_VIEW_CONTROLS::onScroll( wxScrollWinEvent& aEvent )
{
    const double linePanDelta = 0.05;
    const double pagePanDelta = 0.5;

    int type = aEvent.GetEventType();
    int dir = aEvent.GetOrientation();

    if( type == wxEVT_SCROLLWIN_THUMBTRACK )
    {
        auto center = m_view->GetCenter();
        const auto& boundary = m_view->GetBoundary();

        // Flip scroll direction in flipped view
        const double xstart = ( m_view->IsMirroredX() ?
                                boundary.GetRight() : boundary.GetLeft() );
        const double xdelta = ( m_view->IsMirroredX() ? -1 : 1 );

        if( dir == wxHORIZONTAL )
            center.x = xstart + xdelta * ( aEvent.GetPosition() / m_scrollScale.x );
        else
            center.y = boundary.GetTop() + aEvent.GetPosition() / m_scrollScale.y;

        m_view->SetCenter( center );
    }
    else
    {
        double dist = 0;

        if( type == wxEVT_SCROLLWIN_PAGEUP )
            dist = pagePanDelta;
        else if( type == wxEVT_SCROLLWIN_PAGEDOWN )
            dist = -pagePanDelta;
        else if( type == wxEVT_SCROLLWIN_LINEUP )
            dist = linePanDelta;
        else if( type == wxEVT_SCROLLWIN_LINEDOWN )
            dist = -linePanDelta;
        else
            wxCHECK_MSG( false, /* void */, wxT( "Unhandled event type" ) );

        VECTOR2D scroll = m_view->ToWorld( m_view->GetScreenPixelSize(), false ) * dist;

        double scrollX = 0.0;
        double scrollY = 0.0;

        if ( dir == wxHORIZONTAL )
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
    if( aEnabled && !m_parentPanel->HasCapture() && m_parentPanel->m_MouseCapturedLost )
    {
        m_parentPanel->CaptureMouse();

        // Clear the flag to allow calling m_parentPanel->CaptureMouse()
        // Calling it without calling ReleaseMouse() is not accepted by wxWidgets (MSW specific)
        m_parentPanel->m_MouseCapturedLost = false;
    }
    else if( !aEnabled && m_parentPanel->HasCapture()
             && m_state != DRAG_PANNING && m_state != DRAG_ZOOMING )
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
        m_state = IDLE;
#if defined USE_MOUSE_CAPTURE
        if( !m_settings.m_cursorCaptured && m_parentPanel->HasCapture() )
            m_parentPanel->ReleaseMouse();
#endif
    }
}


VECTOR2D WX_VIEW_CONTROLS::GetMousePosition( bool aWorldCoordinates ) const
{
    wxPoint msp = getMouseScreenPosition();
    VECTOR2D screenPos( msp.x, msp.y );

    return aWorldCoordinates ? m_view->ToWorld( screenPos ) : screenPos;
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
        return GetRawCursorPosition( aEnableSnapping );
    }
}


void WX_VIEW_CONTROLS::SetCursorPosition( const VECTOR2D& aPosition, bool aWarpView,
                                          bool aTriggeredByArrows, long aArrowCommand )
{
    m_updateCursor = false;

    if( aTriggeredByArrows )
    {
        m_settings.m_lastKeyboardCursorPositionValid = true;
        m_settings.m_lastKeyboardCursorPosition = aPosition;
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

    WarpCursor( aPosition, true, aWarpView );
    m_cursorPos = aPosition;
}


void WX_VIEW_CONTROLS::SetCrossHairCursorPosition( const VECTOR2D& aPosition,
                                                   bool aWarpView = true )
{
    m_updateCursor = false;

    const VECTOR2I& screenSize = m_view->GetGAL()->GetScreenPixelSize();
    BOX2I screen( VECTOR2I( 0, 0 ), screenSize );
    VECTOR2D screenPos = m_view->ToScreen( aPosition );

    if( aWarpView && !screen.Contains( screenPos ) )
        m_view->SetCenter( aPosition );

    m_cursorPos = aPosition;
}


void WX_VIEW_CONTROLS::WarpCursor( const VECTOR2D& aPosition, bool aWorldCoordinates,
                                   bool aWarpView )
{
    if( aWorldCoordinates )
    {
        const VECTOR2I& screenSize = m_view->GetGAL()->GetScreenPixelSize();
        BOX2I screen( VECTOR2I( 0, 0 ), screenSize );
        VECTOR2D screenPos = m_view->ToScreen( aPosition );

        if( !screen.Contains( screenPos ) )
        {
            if( aWarpView )
            {
                m_view->SetCenter( aPosition );
                m_parentPanel->WarpPointer( screenSize.x / 2, screenSize.y / 2 );
            }
        }
        else
        {
            m_parentPanel->WarpPointer( screenPos.x, screenPos.y );
        }
    }
    else
    {
        m_parentPanel->WarpPointer( aPosition.x, aPosition.y );
    }

    refreshMouse();
}


void WX_VIEW_CONTROLS::CenterOnCursor() const
{
    const VECTOR2I& screenSize = m_view->GetGAL()->GetScreenPixelSize();
    VECTOR2I screenCenter( screenSize / 2 );

    if( GetMousePosition( false ) != screenCenter )
    {
        m_view->SetCenter( GetCursorPosition() );
        m_parentPanel->WarpPointer( KiROUND( screenSize.x / 2 ), KiROUND( screenSize.y / 2 ) );
    }
}


bool WX_VIEW_CONTROLS::handleAutoPanning( const wxMouseEvent& aEvent )
{
    VECTOR2I p( aEvent.GetX(), aEvent.GetY() );
    VECTOR2I pKey( m_view->ToScreen(m_settings.m_lastKeyboardCursorPosition ) );

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
            m_state = IDLE;

            return false;
        }

        return true;
        break;

    case IDLE:
        if( borderHit )
        {
            m_state = AUTO_PANNING;
            m_panTimer.Start( (int) ( 250.0 / 60.0 ) );

            return true;
        }

        return false;
        break;

    case DRAG_PANNING:
    case DRAG_ZOOMING:
        return false;
    }

    wxCHECK_MSG( false, false, wxT( "This line should never be reached" ) );
}


void WX_VIEW_CONTROLS::handleCursorCapture( int x, int y )
{
    if( m_settings.m_cursorCaptured )
    {
        bool warp = false;
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
            m_parentPanel->WarpPointer( x, y );
    }
}


void WX_VIEW_CONTROLS::refreshMouse()
{
    // Notify tools that the cursor position has changed in the world coordinates
    wxMouseEvent moveEvent( EVT_REFRESH_MOUSE );
    wxPoint msp = getMouseScreenPosition();
    moveEvent.SetX( msp.x );
    moveEvent.SetY( msp.y );

    // Set the modifiers state
    moveEvent.SetControlDown( wxGetKeyState( WXK_CONTROL ) );
    moveEvent.SetShiftDown( wxGetKeyState( WXK_SHIFT ) );
    moveEvent.SetAltDown( wxGetKeyState( WXK_ALT ) );

    m_cursorPos = m_view->ToWorld( VECTOR2D( msp.x, msp.y ) );
    wxPostEvent( m_parentPanel, moveEvent );
}


wxPoint WX_VIEW_CONTROLS::getMouseScreenPosition() const
{
    wxPoint msp = wxGetMousePosition();
    m_parentPanel->ScreenToClient( &msp.x, &msp.y );
    return msp;
}


void WX_VIEW_CONTROLS::UpdateScrollbars()
{
    const BOX2D viewport = m_view->GetViewport();
    const BOX2D& boundary = m_view->GetBoundary();

    m_scrollScale.x = 2e3 / viewport.GetWidth();    // TODO it does not have to be updated so often
    m_scrollScale.y = 2e3 / viewport.GetHeight();
    VECTOR2I newScroll( ( viewport.Centre().x - boundary.GetLeft() ) * m_scrollScale.x,
                        ( viewport.Centre().y - boundary.GetTop() ) * m_scrollScale.y );

    // We add the width of the scroll bar thumb to the range because the scroll range is given by
    // the full bar while the position is given by the left/top position of the thumb
    VECTOR2I newRange( m_scrollScale.x * boundary.GetWidth() +
                       m_parentPanel->GetScrollThumb( wxSB_HORIZONTAL ),
                       m_scrollScale.y * boundary.GetHeight() +
                       m_parentPanel->GetScrollThumb( wxSB_VERTICAL ) );

    // Flip scroll direction in flipped view
    if( m_view->IsMirroredX() )
        newScroll.x = ( boundary.GetRight() - viewport.Centre().x ) * m_scrollScale.x;

    // Adjust scrollbars only if it is needed. Otherwise there are cases when canvas is continuously
    // refreshed (Windows)
    if( m_scrollPos != newScroll || newRange.x != m_parentPanel->GetScrollRange( wxSB_HORIZONTAL )
            || newRange.y != m_parentPanel->GetScrollRange( wxSB_VERTICAL ) )
    {
        m_parentPanel->SetScrollbars( 1, 1, newRange.x, newRange.y, newScroll.x, newScroll.y,
                                      true );
        m_scrollPos = newScroll;

#if !defined( __APPLE__ ) && !defined( WIN32 )
        // Trigger a mouse refresh to get the canvas update in GTK (re-draws the scrollbars).
        // Note that this causes an infinite loop on OSX and Windows (in certain cases) as it
        // generates a paint event.
        refreshMouse();
#endif
    }
}

void WX_VIEW_CONTROLS::ForceCursorPosition( bool aEnabled, const VECTOR2D& aPosition )
{
    m_settings.m_forceCursorPosition = aEnabled;
    m_settings.m_forcedPosition = aPosition;
}
