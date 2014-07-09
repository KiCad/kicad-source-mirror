/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <wxPcbStruct.h>
#include <wxBasePcbFrame.h>

#include <tool/tool_manager.h>
#include <tool/tool_dispatcher.h>
#include <tools/common_actions.h>
#include <view/view.h>
#include <view/wx_view_controls.h>

#include <class_draw_panel_gal.h>
#include <pcbnew_id.h>

#include <boost/optional.hpp>
#include <boost/foreach.hpp>

///> Stores information about a mouse button state
struct TOOL_DISPATCHER::BUTTON_STATE
{
    BUTTON_STATE( TOOL_MOUSE_BUTTONS aButton, const wxEventType& aDownEvent,
                 const wxEventType& aUpEvent, const wxEventType& aDblClickEvent ) :
        button( aButton ),
        downEvent( aDownEvent ),
        upEvent( aUpEvent ),
        dblClickEvent( aDblClickEvent )
    {};

    ///> Flag indicating that dragging is active for the given button.
    bool dragging;

    ///> Flag indicating that the given button is pressed.
    bool pressed;

    ///> Point where dragging has started (in world coordinates).
    VECTOR2D dragOrigin;

    ///> Point where click event has occurred.
    VECTOR2D downPosition;

    ///> Difference between drag origin point and current mouse position (expressed as distance in
    ///> pixels).
    double dragMaxDelta;

    ///> Determines the mouse button for which information are stored.
    TOOL_MOUSE_BUTTONS button;

    ///> The type of wxEvent that determines mouse button press.
    wxEventType downEvent;

    ///> The type of wxEvent that determines mouse button release.
    wxEventType upEvent;

    ///> The type of wxEvent that determines mouse button double click.
    wxEventType dblClickEvent;

    ///> Time stamp for the last mouse button press event.
    wxLongLong downTimestamp;

    ///> Restores initial state.
    void Reset()
    {
        dragging = false;
        pressed = false;
    }
};


TOOL_DISPATCHER::TOOL_DISPATCHER( TOOL_MANAGER* aToolMgr ) :
    m_toolMgr( aToolMgr )
{
    m_buttons.push_back( new BUTTON_STATE( BUT_LEFT, wxEVT_LEFT_DOWN,
                         wxEVT_LEFT_UP, wxEVT_LEFT_DCLICK ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_RIGHT, wxEVT_RIGHT_DOWN,
                         wxEVT_RIGHT_UP, wxEVT_RIGHT_DCLICK ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_MIDDLE, wxEVT_MIDDLE_DOWN,
                         wxEVT_MIDDLE_UP, wxEVT_MIDDLE_DCLICK ) );

    ResetState();
}


TOOL_DISPATCHER::~TOOL_DISPATCHER()
{
    BOOST_FOREACH( BUTTON_STATE* st, m_buttons )
        delete st;
}


void TOOL_DISPATCHER::ResetState()
{
    BOOST_FOREACH( BUTTON_STATE* st, m_buttons )
        st->Reset();
}


KIGFX::VIEW* TOOL_DISPATCHER::getView()
{
    return static_cast<PCB_BASE_FRAME*>( m_toolMgr->GetEditFrame() )->GetGalCanvas()->GetView();
}


bool TOOL_DISPATCHER::handleMouseButton( wxEvent& aEvent, int aIndex, bool aMotion )
{
    BUTTON_STATE* st = m_buttons[aIndex];
    wxEventType type = aEvent.GetEventType();
    boost::optional<TOOL_EVENT> evt;
    bool isClick = false;

    bool up = type == st->upEvent;
    bool down = type == st->downEvent;
    bool dblClick = type == st->dblClickEvent;

    int mods = decodeModifiers<wxMouseEvent>( static_cast<wxMouseEvent*>( &aEvent ) );
    int args = st->button | mods;

    if( down )      // Handle mouse button press
    {
        st->downTimestamp = wxGetLocalTimeMillis();
        st->dragOrigin = m_lastMousePos;
        st->downPosition = m_lastMousePos;
        st->dragMaxDelta = 0;
        st->pressed = true;
        evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_DOWN, args );
    }
    else if( up )   // Handle mouse button release
    {
        st->pressed = false;

        if( st->dragging )
        {
            wxLongLong t = wxGetLocalTimeMillis();

            // Determine if it was just a single click or beginning of dragging
            if( t - st->downTimestamp < DragTimeThreshold &&
                    st->dragMaxDelta < DragDistanceThreshold )
                isClick = true;
            else
                evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_UP, args );
        }
        else
            isClick = true;

        if( isClick )
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_CLICK, args );

        st->dragging = false;
    }
    else if( dblClick )
    {
        evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_DBLCLICK, args );
    }

    if( st->pressed && aMotion )
    {
        st->dragging = true;
        double dragPixelDistance =
            getView()->ToScreen( m_lastMousePos - st->dragOrigin, false ).EuclideanNorm();
        st->dragMaxDelta = std::max( st->dragMaxDelta, dragPixelDistance );

        wxLongLong t = wxGetLocalTimeMillis();

        if( t - st->downTimestamp > DragTimeThreshold || st->dragMaxDelta > DragDistanceThreshold )
        {
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_DRAG, args );
            evt->SetMouseDragOrigin( st->dragOrigin );
            evt->SetMouseDelta( m_lastMousePos - st->dragOrigin );
        }
    }

    if( evt )
    {
        evt->SetMousePosition( isClick ? st->downPosition : m_lastMousePos );
        m_toolMgr->ProcessEvent( *evt );

        return true;
    }

    return false;
}


void TOOL_DISPATCHER::DispatchWxEvent( wxEvent& aEvent )
{
    bool motion = false, buttonEvents = false;
    boost::optional<TOOL_EVENT> evt;

    int type = aEvent.GetEventType();

    // Mouse handling
    if( type == wxEVT_MOTION || type == wxEVT_MOUSEWHEEL ||
        type == wxEVT_LEFT_DOWN || type == wxEVT_LEFT_UP ||
        type == wxEVT_MIDDLE_DOWN || type == wxEVT_MIDDLE_UP ||
        type == wxEVT_RIGHT_DOWN || type == wxEVT_RIGHT_UP ||
        type == wxEVT_LEFT_DCLICK || type == wxEVT_MIDDLE_DCLICK || type == wxEVT_RIGHT_DCLICK ||
        // Event issued whem mouse retains position in screen coordinates,
        // but changes in world coordinates (e.g. autopanning)
        type == KIGFX::WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE )
    {
        wxMouseEvent* me = static_cast<wxMouseEvent*>( &aEvent );
        int mods = decodeModifiers<wxMouseEvent>( me );

        VECTOR2D screenPos = m_toolMgr->GetViewControls()->GetMousePosition();
        VECTOR2D pos = getView()->ToWorld( screenPos );

        if( pos != m_lastMousePos )
        {
            motion = true;
            m_lastMousePos = pos;
            static_cast<PCB_BASE_FRAME*>( m_toolMgr->GetEditFrame() )->UpdateStatusBar();
        }

        for( unsigned int i = 0; i < m_buttons.size(); i++ )
            buttonEvents |= handleMouseButton( aEvent, i, motion );

        if( !buttonEvents && motion )
        {
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_MOTION, mods );
            evt->SetMousePosition( pos );
        }

#ifdef __APPLE__
        // TODO That's a big ugly workaround, somehow DRAWPANEL_GAL loses focus
        // after second LMB click and currently I have no means to do better debugging
        if( type == wxEVT_LEFT_UP )
            static_cast<PCB_BASE_FRAME*>( m_toolMgr->GetEditFrame() )->GetGalCanvas()->SetFocus();
#endif /* __APPLE__ */
    }

    // Keyboard handling
    else if( type == wxEVT_CHAR )
    {
        wxKeyEvent* ke = static_cast<wxKeyEvent*>( &aEvent );
        int key = ke->GetKeyCode();
        int mods = decodeModifiers<wxKeyEvent>( ke );

        if( mods & MD_CTRL )
        {
#if !wxCHECK_VERSION( 2, 9, 0 )
            // I really look forward to the day when we will use only one version of wxWidgets..
            const int WXK_CONTROL_A = 1;
            const int WXK_CONTROL_Z = 26;
#endif

            // wxWidgets have a quirk related to Ctrl+letter hot keys handled by CHAR_EVT
            // http://docs.wxwidgets.org/trunk/classwx_key_event.html:
            // "char events for ASCII letters in this case carry codes corresponding to the ASCII
            // value of Ctrl-Latter, i.e. 1 for Ctrl-A, 2 for Ctrl-B and so on until 26 for Ctrl-Z."
            if( key >= WXK_CONTROL_A && key <= WXK_CONTROL_Z )
                key += 'A' - 1;
        }

        if( key == WXK_ESCAPE ) // ESC is the special key for cancelling tools
            evt = TOOL_EVENT( TC_COMMAND, TA_CANCEL_TOOL );
        else
            evt = TOOL_EVENT( TC_KEYBOARD, TA_KEY_PRESSED, key | mods );
    }

    if( evt )
        m_toolMgr->ProcessEvent( *evt );

    // pass the event to the GUI, it might still be interested in it
    aEvent.Skip();
}


void TOOL_DISPATCHER::DispatchWxCommand( wxCommandEvent& aEvent )
{
    boost::optional<TOOL_EVENT> evt;

    switch( aEvent.GetId() )
    {
    case ID_ZOOM_IN:        // toolbar button "Zoom In"
        evt = COMMON_ACTIONS::zoomInCenter.MakeEvent();
        break;

    case ID_ZOOM_OUT:       // toolbar button "Zoom In"
        evt = COMMON_ACTIONS::zoomOutCenter.MakeEvent();
        break;

    case ID_ZOOM_PAGE:      // toolbar button "Fit on Screen"
        evt = COMMON_ACTIONS::zoomFitScreen.MakeEvent();
        break;

    default:
        aEvent.Skip();
        break;
    }

    if( evt )
        m_toolMgr->ProcessEvent( *evt );
}
