/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013-2015 CERN
 * Copyright (C) 2012-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <view/view.h>
#include <view/wx_view_controls.h>
#include <view/zoom_controller.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_dispatcher.h>

using namespace KIGFX;

const wxEventType WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE = wxNewEventType();


static std::unique_ptr<ZOOM_CONTROLLER> GetZoomControllerForPlatform()
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
    return std::make_unique<ACCELERATING_ZOOM_CONTROLLER>();
#endif
}


WX_VIEW_CONTROLS::WX_VIEW_CONTROLS( VIEW* aView, wxScrolledCanvas* aParentPanel ) :
    VIEW_CONTROLS( aView ), m_state( IDLE ), m_parentPanel( aParentPanel ),
    m_scrollScale( 1.0, 1.0 ), m_lastTimestamp( 0 ), m_cursorPos( 0, 0 ), m_updateCursor( true )
{
    m_parentPanel->Connect( wxEVT_MOTION,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onMotion ), NULL, this );
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
    m_parentPanel->Connect( wxEVT_MAGNIFY,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onMagnify ), NULL, this );
#endif
    m_parentPanel->Connect( wxEVT_MOUSEWHEEL,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onWheel ), NULL, this );
    m_parentPanel->Connect( wxEVT_MIDDLE_UP,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), NULL, this );
    m_parentPanel->Connect( wxEVT_MIDDLE_DOWN,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), NULL, this );
    m_parentPanel->Connect( wxEVT_LEFT_UP,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), NULL, this );
    m_parentPanel->Connect( wxEVT_LEFT_DOWN,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), NULL, this );
    m_parentPanel->Connect( wxEVT_RIGHT_UP,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), NULL, this );
    m_parentPanel->Connect( wxEVT_RIGHT_DOWN,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onButton ), NULL, this );
#if defined _WIN32 || defined _WIN64
    m_parentPanel->Connect( wxEVT_ENTER_WINDOW,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onEnter ), NULL, this );
#endif
    m_parentPanel->Connect( wxEVT_LEAVE_WINDOW,
                            wxMouseEventHandler( WX_VIEW_CONTROLS::onLeave ), NULL, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_THUMBTRACK,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), NULL, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_PAGEUP,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), NULL, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_PAGEDOWN,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), NULL, this );

    m_parentPanel->Connect( wxEVT_SCROLLWIN_BOTTOM,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), NULL, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_TOP,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), NULL, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_LINEUP,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), NULL, this );
    m_parentPanel->Connect( wxEVT_SCROLLWIN_LINEDOWN,
                            wxScrollWinEventHandler( WX_VIEW_CONTROLS::onScroll ), NULL, this );

    m_zoomController = GetZoomControllerForPlatform();

    m_cursorWarped = false;

    m_panTimer.SetOwner( this );
    this->Connect( wxEVT_TIMER,
                   wxTimerEventHandler( WX_VIEW_CONTROLS::onTimer ), NULL, this );

    m_settings.m_lastKeyboardCursorPositionValid = false;
}


WX_VIEW_CONTROLS::~WX_VIEW_CONTROLS()
{
}


void WX_VIEW_CONTROLS::onMotion( wxMouseEvent& aEvent )
{
    bool isAutoPanning = false;
    VECTOR2D mousePos( aEvent.GetX(), aEvent.GetY() );

    if( m_settings.m_autoPanEnabled && m_settings.m_autoPanSettingEnabled )
        isAutoPanning = handleAutoPanning( aEvent );

    if( !isAutoPanning && aEvent.Dragging() )
    {
        if( m_state == DRAG_PANNING )
        {
            VECTOR2D d = m_dragStartPoint - mousePos;
            VECTOR2D delta = m_view->ToWorld( d, false );

            m_view->SetCenter( m_lookStartPoint + delta );
            aEvent.StopPropagation();
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
    const double wheelPanSpeed = 0.001;

#ifdef __WXGTK3__
    if( aEvent.GetTimestamp() == m_lastTimestamp )
    {
        aEvent.Skip( false );
        return;
    }

    m_lastTimestamp = aEvent.GetTimestamp();
#endif

    // mousewheelpan disabled:
    //      wheel + ctrl    -> horizontal scrolling;
    //      wheel + shift   -> vertical scrolling;
    //      wheel           -> zooming;
    // mousewheelpan enabled:
    //      wheel           -> pan;
    //      wheel + ctrl    -> zooming;
    //      wheel + shift   -> horizontal scrolling.

    if( ( !m_settings.m_enableMousewheelPan && ( aEvent.ControlDown() || aEvent.ShiftDown() ) ) ||
        ( m_settings.m_enableMousewheelPan && !aEvent.ControlDown() ) )
    {
        // Scrolling
        VECTOR2D scrollVec = m_view->ToWorld( m_view->GetScreenPixelSize(), false ) *
                             ( (double) aEvent.GetWheelRotation() * wheelPanSpeed );
        int axis = aEvent.GetWheelAxis();
        double scrollX = 0.0;
        double scrollY = 0.0;

        if( m_settings.m_enableMousewheelPan )
        {
            if ( axis == wxMOUSE_WHEEL_HORIZONTAL || aEvent.ShiftDown() )
                scrollX = scrollVec.x;
            else
                scrollY = -scrollVec.y;
        }
        else
        {
            if( aEvent.ControlDown() )
                scrollX = -scrollVec.x;
            else
                scrollY = -scrollVec.y;
        }

        VECTOR2D delta( scrollX, scrollY );

        m_view->SetCenter( m_view->GetCenter() + delta );
        refreshMouse();
    }
    else
    {
        int    rotation = aEvent.GetWheelRotation();
        double zoomScale = m_zoomController->GetScaleForRotation( rotation );

        if( IsCursorWarpingEnabled() )
        {
            CenterOnCursor();
            m_view->SetScale( m_view->GetScale() * zoomScale );
        }
        else
        {
            VECTOR2D anchor = m_view->ToWorld( VECTOR2D( aEvent.GetX(), aEvent.GetY() ) );
            m_view->SetScale( m_view->GetScale() * zoomScale, anchor );
        }
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
        if( aEvent.MiddleDown() ||
            ( aEvent.LeftDown() && m_settings.m_panWithLeftButton ) ||
            ( aEvent.RightDown() && m_settings.m_panWithRightButton ) )
        {
            m_dragStartPoint = VECTOR2D( aEvent.GetX(), aEvent.GetY() );
            m_lookStartPoint = m_view->GetCenter();
            m_state = DRAG_PANNING;
        }

        if( aEvent.LeftUp() )
            m_state = IDLE;     // Stop autopanning when user release left mouse button

        break;

    case DRAG_PANNING:
        if( aEvent.MiddleUp() || aEvent.LeftUp() || aEvent.RightUp() )
            m_state = IDLE;

        break;
    }

    aEvent.Skip();
}


void WX_VIEW_CONTROLS::onEnter( wxMouseEvent& aEvent )
{
    m_parentPanel->SetFocus();
}


void WX_VIEW_CONTROLS::onLeave( wxMouseEvent& aEvent )
{
    if( m_settings.m_cursorCaptured )
    {
        bool warp = false;
        int x = aEvent.GetX();
        int y = aEvent.GetY();
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

#if wxCHECK_VERSION( 3, 0, 0 )
        if( !m_parentPanel->HasFocus() )
            break;
#endif

        double borderSize = std::min( m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().x,
                                      m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().y );

        VECTOR2D dir( m_panDirection );

        if( dir.EuclideanNorm() > borderSize )
            dir = dir.Resize( borderSize );

        dir = m_view->ToWorld( dir, false );
        m_view->SetCenter( m_view->GetCenter() + dir * m_settings.m_autoPanSpeed );

        refreshMouse();
    }
    break;

    case IDLE:    // Just remove unnecessary warnings
    case DRAG_PANNING:
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
        {
            wxASSERT( "Unhandled event type" );
            return;
        }

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


void WX_VIEW_CONTROLS::SetGrabMouse( bool aEnabled )
{
    if( aEnabled && !m_settings.m_grabMouse )
        m_parentPanel->CaptureMouse();
    else if( !aEnabled && m_settings.m_grabMouse )
        m_parentPanel->ReleaseMouse();

    VIEW_CONTROLS::SetGrabMouse( aEnabled );
}


VECTOR2D WX_VIEW_CONTROLS::GetMousePosition( bool aWorldCoordinates ) const
{
    wxPoint msp = getMouseScreenPosition();
    VECTOR2D screenPos( msp.x, msp.y );

    return aWorldCoordinates ? m_view->ToWorld( screenPos ) : screenPos;
}


VECTOR2D WX_VIEW_CONTROLS::GetRawCursorPosition( bool aEnableSnapping ) const
{
    if( aEnableSnapping )
    {
        return m_view->GetGAL()->GetGridPoint( m_cursorPos );
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
        bool aTriggeredByArrows )
{
    m_updateCursor = false;

    if( aTriggeredByArrows )
    {
        m_settings.m_lastKeyboardCursorPositionValid = true;
        m_settings.m_lastKeyboardCursorPosition = aPosition;
        m_cursorWarped = false;
    }
    else
    {
        m_settings.m_lastKeyboardCursorPositionValid = false;
        m_cursorWarped = true;
    }

    WarpCursor( aPosition, true, aWarpView );
    m_cursorPos = aPosition;
}


void WX_VIEW_CONTROLS::SetCrossHairCursorPosition( const VECTOR2D& aPosition, bool aWarpView = true )
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

    if( m_cursorWarped || (m_settings.m_lastKeyboardCursorPositionValid && (p == pKey)) )
    {
        // last cursor move event came from keyboard cursor control. If auto-panning is enabled and
        // the next position is inside the autopan zone, check if it really came from a mouse event, otherwise
        // disable autopan temporarily. Also temporaly disable autopan if the cursor is in the autopan zone
        // because the application warped the cursor.

        m_cursorWarped = false;
        return true;
    }

    m_cursorWarped = false;

    // Compute areas where autopanning is active
    int borderStart = std::min( m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().x,
                                   m_settings.m_autoPanMargin * m_view->GetScreenPixelSize().y );
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
        return false;
    }

    wxASSERT_MSG( false, wxT( "This line should never be reached" ) );
    return false;    // Should not be reached, just avoid the compiler warnings..
}


void WX_VIEW_CONTROLS::refreshMouse()
{
    // Notify tools that the cursor position has changed in the world coordinates
    wxMouseEvent moveEvent( EVT_REFRESH_MOUSE );
    wxPoint msp = getMouseScreenPosition();
    moveEvent.SetX( msp.x );
    moveEvent.SetY( msp.y );

    // Set the modifiers state
#if wxCHECK_VERSION( 3, 0, 0 )
    moveEvent.SetControlDown( wxGetKeyState( WXK_CONTROL ) );
    moveEvent.SetShiftDown( wxGetKeyState( WXK_SHIFT ) );
    moveEvent.SetAltDown( wxGetKeyState( WXK_ALT ) );
#else
    // wx <3.0 do not have accessors, but the fields are exposed
    moveEvent.m_controlDown = wxGetKeyState( WXK_CONTROL );
    moveEvent.m_shiftDown = wxGetKeyState( WXK_SHIFT );
    moveEvent.m_altDown = wxGetKeyState( WXK_ALT );
#endif

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
#ifdef __WXGTK3__
    // Until we can handle the repaint events from scroll bar hide/show
    // todo: Implement area mapping for re-painting scrollbars
    return;
#endif

    const BOX2D viewport = m_view->GetViewport();
    const BOX2D& boundary = m_view->GetBoundary();

    m_scrollScale.x = 2e3 / viewport.GetWidth();    // TODO it does not have to be updated so often
    m_scrollScale.y = 2e3 / viewport.GetHeight();
    VECTOR2I newScroll( ( viewport.Centre().x - boundary.GetLeft() ) * m_scrollScale.x,
                ( viewport.Centre().y - boundary.GetTop() ) * m_scrollScale.y );

    // Flip scroll direction in flipped view
    if( m_view->IsMirroredX() )
        newScroll.x = ( boundary.GetRight() - viewport.Centre().x ) * m_scrollScale.x;

    // Adjust scrollbars only if it is needed. Otherwise there are cases when canvas is continuosly
    // refreshed (Windows)
    if( m_scrollPos != newScroll )
    {
        // Another example of wxWidgets being broken by design: scroll position is determined by the
        // left (or top, if vertical) edge of the slider. Fortunately, slider size seems to be constant
        // (at least for wxGTK and wxMSW), so we have to add its size to allow user to scroll the workspace
        // till the end.

        m_parentPanel->SetScrollbars( 1, 1,
#if defined(__LINUX__)
            m_scrollScale.x * boundary.GetWidth() + 1623, m_scrollScale.y * boundary.GetHeight() + 1623,
#elif defined(__WIN32__) || defined(__WIN64__)
            m_scrollScale.x * boundary.GetWidth() + 1377, m_scrollScale.y * boundary.GetHeight() + 741,
#else
            m_scrollScale.x * boundary.GetWidth(), m_scrollScale.y * boundary.GetHeight(),
#endif
            newScroll.x, newScroll.y, false );

        m_scrollPos = newScroll;
    }
}

void WX_VIEW_CONTROLS::ForceCursorPosition( bool aEnabled, const VECTOR2D& aPosition )
{
    m_settings.m_forceCursorPosition = aEnabled;
    m_settings.m_forcedPosition = aPosition;
}
