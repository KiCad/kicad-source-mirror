/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see CHANGELOG.txt for contributors.
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

#include "tool/tool_dispatcher.h"

#include <bit>
#include <optional>

#include <wx/log.h>
#include <wx/stc/stc.h>
#include <wx/settings.h>

#include <core/ignore.h>
#include <macros.h>
#include <trace_helpers.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <tool/action_manager.h>
#include <tool/action_menu.h>
#include <view/view.h>
#include <view/wx_view_controls.h>
#include <eda_draw_frame.h>
#include <core/kicad_algo.h>

#include <kiplatform/app.h>
#include <kiplatform/ui.h>


/// Store information about a mouse button state.
struct TOOL_DISPATCHER::BUTTON_STATE
{
    BUTTON_STATE( TOOL_MOUSE_BUTTONS aButton, const wxEventType& aDownEvent,
                 const wxEventType& aUpEvent, const wxEventType& aDblClickEvent ) :
        dragging( false ),
        pressed( false ),
        button( aButton ),
        downEvent( aDownEvent ),
        upEvent( aUpEvent ),
        dblClickEvent( aDblClickEvent )
    {};

    /// Flag indicating that dragging is active for the given button.
    bool dragging;

    /// Flag indicating that the given button is pressed.
    bool pressed;

    /// Point where dragging has started (in world coordinates).
    VECTOR2D dragOrigin;

    /// Point where dragging has started (in screen coordinates).
    VECTOR2D dragOriginScreen;

    /// Point where click event has occurred.
    VECTOR2D downPosition;

    /// Determines the mouse button for which information are stored.
    TOOL_MOUSE_BUTTONS button;

    /// The type of wxEvent that determines mouse button press.
    wxEventType downEvent;

    /// The type of wxEvent that determines mouse button release.
    wxEventType upEvent;

    /// The type of wxEvent that determines mouse button double click.
    wxEventType dblClickEvent;

    /// Time stamp for the last mouse button press event.
    wxLongLong downTimestamp;

    /// Restores initial state.
    void Reset()
    {
        dragging = false;
        pressed = false;
    }

    /// Checks the current state of the button.
    bool GetState() const
    {
        wxMouseState mouseState = wxGetMouseState();

        switch( button )
        {
        case BUT_LEFT:
            return mouseState.LeftIsDown();

        case BUT_MIDDLE:
            return mouseState.MiddleIsDown();

        case BUT_RIGHT:
            return mouseState.RightIsDown();

        case BUT_AUX1:
            return mouseState.Aux1IsDown();

        case BUT_AUX2:
            return mouseState.Aux2IsDown();

        default:
            wxFAIL_MSG( wxT( "unknown button" ) );
            return false;
        }
    }
};


TOOL_DISPATCHER::TOOL_DISPATCHER( TOOL_MANAGER* aToolMgr ) :
    m_toolMgr( aToolMgr )
{
    m_sysDragMinX = wxSystemSettings::GetMetric( wxSYS_DRAG_X );
    m_sysDragMinY = wxSystemSettings::GetMetric( wxSYS_DRAG_Y );

    m_sysDragMinX = m_sysDragMinX != -1 ? m_sysDragMinX : DragDistanceThreshold;
    m_sysDragMinY = m_sysDragMinY != -1 ? m_sysDragMinY : DragDistanceThreshold;

    m_buttons.push_back( new BUTTON_STATE( BUT_LEFT, wxEVT_LEFT_DOWN,
                         wxEVT_LEFT_UP, wxEVT_LEFT_DCLICK ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_RIGHT, wxEVT_RIGHT_DOWN,
                         wxEVT_RIGHT_UP, wxEVT_RIGHT_DCLICK ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_MIDDLE, wxEVT_MIDDLE_DOWN,
                         wxEVT_MIDDLE_UP, wxEVT_MIDDLE_DCLICK ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_AUX1, wxEVT_AUX1_DOWN,
                         wxEVT_AUX1_UP, wxEVT_AUX1_DCLICK ) );
    m_buttons.push_back( new BUTTON_STATE( BUT_AUX2, wxEVT_AUX2_DOWN,
                         wxEVT_AUX2_UP, wxEVT_AUX2_DCLICK ) );

    ResetState();
}


TOOL_DISPATCHER::~TOOL_DISPATCHER()
{
    for( BUTTON_STATE* st : m_buttons )
        delete st;
}

int TOOL_DISPATCHER::decodeModifiers( const wxKeyboardState* aState )
{
    int mods = 0;
    int wxmods = aState->GetModifiers();

    // Returns the state of key modifiers (Alt, Ctrl and so on). Be carefull:
    // the flag wxMOD_ALTGR is defined in wxWidgets as wxMOD_CONTROL|wxMOD_ALT
    // So AltGr key cannot used as modifier key because it is the same as Alt key + Ctrl key.
#if CAN_USE_ALTGR_KEY
    if( wxmods & wxMOD_ALTGR )
        mods |= MD_ALTGR;
    else
#endif
    {
        if( wxmods & wxMOD_CONTROL )
            mods |= MD_CTRL;

        if( wxmods & wxMOD_ALT )
            mods |= MD_ALT;
    }

    if( wxmods & wxMOD_SHIFT )
        mods |= MD_SHIFT;

#ifdef wxMOD_META
    if( wxmods & wxMOD_META )
        mods |= MD_META;
#endif

#ifdef wxMOD_WIN
    if( wxmods & wxMOD_WIN )
        mods |= MD_SUPER;
#endif

    return mods;
}


void TOOL_DISPATCHER::ResetState()
{
    for( BUTTON_STATE* st : m_buttons )
        st->Reset();
}


KIGFX::VIEW* TOOL_DISPATCHER::getView()
{
    return m_toolMgr->GetView();
}


bool TOOL_DISPATCHER::handleMouseButton( wxEvent& aEvent, int aIndex, bool aMotion )
{
    BUTTON_STATE* st = m_buttons[aIndex];
    wxEventType type = aEvent.GetEventType();
    std::optional<TOOL_EVENT> evt;
    bool isClick = false;

//    bool up = type == st->upEvent;
//    bool down = type == st->downEvent;
    bool up = false, down = false;
    bool dblClick = type == st->dblClickEvent;
    bool state = st->GetState();

    if( !dblClick )
    {
        // Sometimes the dispatcher does not receive mouse button up event, so it stays
        // in the dragging mode even if the mouse button is not held anymore
        if( st->pressed && !state )
            up = true;
        // Don't apply same logic to down events as it kills touchpad tapping
        else if( !st->pressed && type == st->downEvent )
            down = true;
    }

    int mods = decodeModifiers( static_cast<wxMouseEvent*>( &aEvent ) );
    int args = st->button | mods;

    if( down )      // Handle mouse button press
    {
        st->downTimestamp = wxGetLocalTimeMillis();

        if( !st->pressed )      // save the drag origin on the first click only
        {
            st->dragOrigin = m_lastMousePos;
            st->dragOriginScreen = m_lastMousePosScreen;
        }

        st->downPosition = m_lastMousePos;
        st->pressed = true;
        evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_DOWN, args );
    }
    else if( up )   // Handle mouse button release
    {
        st->pressed = false;

        if( st->dragging )
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_UP, args );
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
        if( !st->dragging )
        {
#ifdef __WXMAC__
            if( wxGetLocalTimeMillis() - st->downTimestamp > DragTimeThreshold )
                st->dragging = true;
#endif
            VECTOR2D offset = m_lastMousePosScreen - st->dragOriginScreen;

            if( abs( offset.x ) > m_sysDragMinX || abs( offset.y ) > m_sysDragMinY )
                st->dragging = true;

        }

        if( st->dragging )
        {
            evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_DRAG, args );
            evt->setMouseDragOrigin( st->dragOrigin );
            evt->setMouseDelta( m_lastMousePos - st->dragOrigin );
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


/**
 * Helper to know if a special key ( see key list ) should be captured.
 *
 * If the event can be skipped on Linux, the event must be passed to the GUI if they are not
 * used by KiCad, especially the wxEVENT_CHAR_HOOK, if it is not handled.  Some keys have a
 * predefined action in wxWidgets so, even if not used, the even will be not skipped the unused
 * keys listed in isKeySpecialCode() will be not skipped.
 */
bool isKeySpecialCode( int aKeyCode )
{
    // These keys have predefined actions (like move thumbtrack cursor),
    // and we do not want these actions executed
    const std::vector<enum wxKeyCode> special_keys =
    {
        WXK_PAGEUP, WXK_PAGEDOWN,
        WXK_NUMPAD_PAGEUP, WXK_NUMPAD_PAGEDOWN
    };

    return alg::contains( special_keys, aKeyCode );
}


/**
 * Helper to know if a key should be managed by DispatchWxEvent() or if the event can be ignored
 * and skipped because the key is only a modifier that is not used alone in KiCad.
 */
static bool isKeyModifierOnly( int aKeyCode )
{
    static std::vector<enum wxKeyCode> special_keys =
    {
        WXK_CONTROL, WXK_RAW_CONTROL, WXK_SHIFT, WXK_ALT,
#ifdef WXK_WINDOWS_LEFT
        WXK_WINDOWS_LEFT, WXK_WINDOWS_RIGHT,
#endif
#ifdef WXK_MENU
        WXK_MENU,
#endif
#ifdef WXK_COMMAND
        WXK_COMMAND,
#endif
#ifdef WXK_META
        WXK_META,
#endif
    };

    return alg::contains( special_keys, aKeyCode );
}


static bool isMouseClick( wxEventType type )
{
    return type == wxEVT_LEFT_DOWN || type == wxEVT_LEFT_UP || type == wxEVT_LEFT_DCLICK
           || type == wxEVT_MIDDLE_DOWN || type == wxEVT_MIDDLE_UP || type == wxEVT_MIDDLE_DCLICK
           || type == wxEVT_RIGHT_DOWN || type == wxEVT_RIGHT_UP || type == wxEVT_RIGHT_DCLICK
           || type == wxEVT_AUX1_DOWN || type == wxEVT_AUX1_UP || type == wxEVT_AUX1_DCLICK
           || type == wxEVT_AUX2_DOWN || type == wxEVT_AUX2_UP || type == wxEVT_AUX2_DCLICK;
}


/**
 * Convert some special key codes to an equivalent.
 *
 *  - WXK_NUMPAD_UP to WXK_UP
 *  - WXK_NUMPAD_DOWN to WXK_DOWN
 *  - WXK_NUMPAD_LEFT to WXK_LEFT
 *  - WXK_NUMPAD_RIGHT to WXK_RIGHT
 *  - WXK_NUMPAD_PAGEUP to WXK_PAGEUP
 *  - WXK_NUMPAD_PAGEDOWN to WXK_PAGEDOWN
 *
 * @note wxEVT_CHAR_HOOK does this conversion when it is skipped by firing a wxEVT_CHAR
 *       with this converted code, but we do not skip these key events because they also
 *       have default action (scroll the panel).
 */
int translateSpecialCode( int aKeyCode )
{
    switch( aKeyCode )
    {
    case WXK_NUMPAD_UP: return WXK_UP;
    case WXK_NUMPAD_DOWN: return WXK_DOWN;
    case WXK_NUMPAD_LEFT: return WXK_LEFT;
    case WXK_NUMPAD_RIGHT: return WXK_RIGHT;
    case WXK_NUMPAD_PAGEUP: return WXK_PAGEUP;
    case WXK_NUMPAD_PAGEDOWN: return WXK_PAGEDOWN;
    default: break;
    };

    return aKeyCode;
}


std::optional<TOOL_EVENT> TOOL_DISPATCHER::GetToolEvent( wxKeyEvent* aKeyEvent, bool* keyIsSpecial )
{
    std::optional<TOOL_EVENT> evt;
    int             key = aKeyEvent->GetKeyCode();
    int             unicode_key = aKeyEvent->GetUnicodeKey();

    // This wxEVT_CHAR_HOOK event can be ignored: not useful in KiCad
    if( isKeyModifierOnly( key ) )
    {
        aKeyEvent->Skip();
        return evt;
    }

    wxLogTrace( kicadTraceKeyEvent, wxS( "TOOL_DISPATCHER::GetToolEvent %s" ), dump( *aKeyEvent ) );

    // if the key event must be skipped, skip it here if the event is a wxEVT_CHAR_HOOK
    // and do nothing.
    *keyIsSpecial = isKeySpecialCode( key );

    if( aKeyEvent->GetEventType() == wxEVT_CHAR_HOOK )
        key = translateSpecialCode( key );

    int mods = decodeModifiers( aKeyEvent );

    if( mods & MD_CTRL )
    {
        // wxWidgets maps key codes related to Ctrl+letter handled by CHAR_EVT
        // (http://docs.wxwidgets.org/trunk/classwx_key_event.html):
        // char events for ASCII letters in this case carry codes corresponding to the ASCII
        // value of Ctrl-Latter, i.e. 1 for Ctrl-A, 2 for Ctrl-B and so on until 26 for Ctrl-Z.
        // They are remapped here to be more easy to handle in code
        // Note also on OSX wxWidgets has a different behavior and the mapping is made
        // only for ctrl+'A' to ctlr+'Z' (unicode code return 'A' to 'Z').
        // Others OS return WXK_CONTROL_A to WXK_CONTROL_Z, and Ctrl+'M' returns the same code as
        // the return key, so the remapping does not use the unicode key value.
#ifdef __APPLE__
        if( unicode_key >= 'A' && unicode_key <= 'Z' && key >= WXK_CONTROL_A && key <= WXK_CONTROL_Z )
#else
        ignore_unused( unicode_key );

        if( key >= WXK_CONTROL_A && key <= WXK_CONTROL_Z )
#endif
            key += 'A' - 1;
    }

#ifdef __APPLE__
    if( mods & MD_ALT )
    {
        // OSX maps a bunch of commonly used extended-ASCII characters onto the keyboard
        // using the ALT key.  Since we use ALT for some of our hotkeys, we need to map back
        // to the underlying keys.  The kVK_ANSI_* values come from Apple and are said to be
        // hardware independent.
        switch( aKeyEvent->GetRawKeyCode() )
        {
        case /* kVK_ANSI_1     */ 0x12: key = '1'; break;
        case /* kVK_ANSI_2     */ 0x13: key = '2'; break;
        case /* kVK_ANSI_3     */ 0x14: key = '3'; break;
        case /* kVK_ANSI_4     */ 0x15: key = '4'; break;
        case /* kVK_ANSI_6     */ 0x16: key = '6'; break;
        case /* kVK_ANSI_5     */ 0x17: key = '5'; break;
        case /* kVK_ANSI_Equal */ 0x18: key = '='; break;
        case /* kVK_ANSI_9     */ 0x19: key = '9'; break;
        case /* kVK_ANSI_7     */ 0x1A: key = '7'; break;
        case /* kVK_ANSI_Minus */ 0x1B: key = '-'; break;
        case /* kVK_ANSI_8     */ 0x1C: key = '8'; break;
        case /* kVK_ANSI_0     */ 0x1D: key = '0'; break;
        default: ;
        }
    }
#endif

    if( key == WXK_ESCAPE ) // ESC is the special key for canceling tools
        evt = TOOL_EVENT( TC_COMMAND, TA_CANCEL_TOOL, WXK_ESCAPE );
    else
        evt = TOOL_EVENT( TC_KEYBOARD, TA_KEY_PRESSED, key | mods );

    return evt;
}


void TOOL_DISPATCHER::DispatchWxEvent( wxEvent& aEvent )
{
    bool            motion = false;
    bool            buttonEvents = false;
    VECTOR2D        pos;
    std::optional<TOOL_EVENT> evt;
    bool            keyIsEscape  = false;  // True if the keypress was the escape key
    bool            keyIsSpecial = false;  // True if the key is a special key code
    wxWindow*       focus = wxWindow::FindFocus();

    // Required in win32 to ensure wxTimer events get scheduled in between other events
    // Or else we may stall them out entirely and never get them during actions like rapid
    // mouse moves.
    KIPLATFORM::APP::ForceTimerMessagesToBeCreatedIfNecessary();

    wxEventType type = aEvent.GetEventType();

    // Sometimes there is no window that has the focus (it happens when another PCB_BASE_FRAME
    // is opened and is iconized on Windows).
    // In this case, give the focus to the parent frame (GAL canvas itself does not accept the
    // focus when iconized for some obscure reason)
    if( focus == nullptr )
    {
        wxWindow* holderWindow = dynamic_cast<wxWindow*>( m_toolMgr->GetToolHolder() );

#if defined( _WIN32 ) || defined( __WXGTK__ )
        // Mouse events may trigger regardless of window status (windows feature)
        // However we need to avoid focus fighting (especially modals)
        if( holderWindow && KIPLATFORM::UI::IsWindowActive( holderWindow ) )
#else
        if( holderWindow )
#endif
        {
            holderWindow->SetFocus();
        }
    }

    if( isMouseClick( type ) )
    {
        if( m_toolMgr->GetToolHolder() && m_toolMgr->GetToolHolder()->GetToolCanvas() &&
            !m_toolMgr->GetToolHolder()->GetToolCanvas()->HasFocus() )
        {
            m_toolMgr->GetToolHolder()->GetToolCanvas()->SetFocus();
        }
    }

    // Mouse handling
    // Note: wxEVT_LEFT_DOWN event must always be skipped.
    if( type == wxEVT_MOTION || type == wxEVT_MOUSEWHEEL ||
        type == wxEVT_MAGNIFY ||
        isMouseClick( type ) ||
        // Event issued when mouse retains position in screen coordinates,
        // but changes in world coordinates (e.g. autopanning)
        type == KIGFX::WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE )
    {
        wxMouseEvent* me = static_cast<wxMouseEvent*>( &aEvent );
        int mods = decodeModifiers( me );

        if( m_toolMgr->GetViewControls() )
        {
            pos = m_toolMgr->GetViewControls()->GetMousePosition();
            m_lastMousePosScreen = m_toolMgr->GetViewControls()->GetMousePosition( false );

            if( pos != m_lastMousePos )
            {
                motion = true;
                m_lastMousePos = pos;
            }
        }

        for( unsigned int i = 0; i < m_buttons.size(); i++ )
            buttonEvents |= handleMouseButton( aEvent, i, motion );

        if( m_toolMgr->GetViewControls() )
        {
            if( !buttonEvents && motion )
            {
                evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_MOTION, mods );
                evt->SetMousePosition( pos );
            }
        }

        // We only handle wheel events that aren't for the view control.
        // Events with zero or one modifier are reserved for view control.
        // When using WX_VIEW_CONTROLS, these will already be handled, but
        // we still shouldn't consume such events if we get them (e.g. for
        // when WX_VIEW_CONTROLS is not in use, like in the 3D viewer)
        if( !evt && me->GetWheelRotation() != 0 )
        {
            const unsigned modBits =
                    static_cast<unsigned>( mods ) & MD_MODIFIER_MASK;
            const bool shouldHandle = std::popcount( modBits ) > 1;

            if( shouldHandle )
            {
                evt = TOOL_EVENT( TC_MOUSE, TA_MOUSE_WHEEL, mods );
                evt->SetParameter<int>( me->GetWheelRotation() );
            }
        }
    }
    else if( type == wxEVT_CHAR_HOOK || type == wxEVT_CHAR )
    {
        wxKeyEvent* ke = static_cast<wxKeyEvent*>( &aEvent );

        wxLogTrace( kicadTraceKeyEvent, wxS( "TOOL_DISPATCHER::DispatchWxEvent %s" ), dump( *ke ) );

        // Do not process wxEVT_CHAR_HOOK for a shift-modified key, as ACTION_MANAGER::RunHotKey
        // will run the un-shifted key and that's not what we want.  Wait to get the translated
        // key from wxEVT_CHAR.
        // See https://gitlab.com/kicad/code/kicad/-/issues/1809
        if( type == wxEVT_CHAR_HOOK && ke->GetModifiers() == wxMOD_SHIFT )
        {
            aEvent.Skip();
            return;
        }

        keyIsEscape = ( ke->GetKeyCode() == WXK_ESCAPE );

        if( KIUI::IsInputControlFocused( focus ) )
        {
            bool enabled = KIUI::IsInputControlEditable( focus );

            // Never process key events for tools when a text entry has focus
            if( enabled )
            {
                aEvent.Skip();
                return;
            }
            // Even if not enabled, allow a copy out
            else if( ke->GetModifiers() == wxMOD_CONTROL && ke->GetKeyCode() == 'C' )
            {
                aEvent.Skip();
                return;
            }
        }

        evt = GetToolEvent( ke, &keyIsSpecial );
    }
    else if( type == wxEVT_MENU_OPEN || type == wxEVT_MENU_CLOSE || type == wxEVT_MENU_HIGHLIGHT )
    {
        wxMenuEvent* tmp = dynamic_cast<wxMenuEvent*>( &aEvent );

        // Something is amiss if the event has these types and isn't a menu event, so bail out on
        // its processing
        if( !tmp )
        {
            aEvent.Skip();
            return;
        }

        wxMenuEvent& menuEvent = *tmp;

#if wxCHECK_VERSION( 3, 2, 0 )
        // Forward the event to the menu for processing
        if( ACTION_MENU* currentMenu = dynamic_cast<ACTION_MENU*>( menuEvent.GetMenu() ) )
            currentMenu->OnMenuEvent( menuEvent );
#else
        //
        // wxWidgets has several issues that we have to work around:
        //
        // 1) wxWidgets 3.0.x Windows has a bug where wxEVT_MENU_OPEN and wxEVT_MENU_HIGHLIGHT
        //    events are not captured by the ACTON_MENU menus.  So we forward them here.
        //    (FWIW, this one is fixed in wxWidgets 3.1.x.)
        //
        // 2) wxWidgets doesn't pass the menu pointer for wxEVT_MENU_HIGHLIGHT events.  So we
        //    store the menu pointer from the wxEVT_MENU_OPEN call.
        //
        // 3) wxWidgets has no way to tell whether a command is from a menu selection or a
        //    hotkey.  So we keep track of menu highlighting so we can differentiate.
        //

        static ACTION_MENU* currentMenu;

        if( type == wxEVT_MENU_OPEN )
        {
            currentMenu = dynamic_cast<ACTION_MENU*>( menuEvent.GetMenu() );

            if( currentMenu )
                currentMenu->OnMenuEvent( menuEvent );
        }
        else if( type == wxEVT_MENU_HIGHLIGHT )
        {
            if( currentMenu )
                currentMenu->OnMenuEvent( menuEvent );
        }
        else if( type == wxEVT_MENU_CLOSE )
        {
            if( currentMenu )
                currentMenu->OnMenuEvent( menuEvent );

            currentMenu = nullptr;
        }
#endif

        aEvent.Skip();
    }

    bool handled = false;

    if( evt )
    {
        wxLogTrace( kicadTraceToolStack, wxS( "TOOL_DISPATCHER::DispatchWxEvent %s" ),
                    evt->Format() );

        handled = m_toolMgr->ProcessEvent( *evt );

        wxLogTrace( kicadTraceToolStack,
                    wxS( "TOOL_DISPATCHER::DispatchWxEvent - Handled: %s  %s" ),
                    ( handled ? wxS( "true" ) : wxS( "false" ) ), evt->Format() );
    }

    // pass the event to the GUI, it might still be interested in it
    // Note wxEVT_CHAR_HOOK event is already skipped for special keys not used by KiCad
    // and wxEVT_LEFT_DOWN must be always Skipped.
    //
    // On OS X, key events are always meant to be caught.  An uncaught key event is assumed
    // to be a user input error by OS X (as they are pressing keys in a context where nothing
    // is there to catch the event).  This annoyingly makes OS X beep and/or flash the screen
    // in Pcbnew and the footprint editor any time a hotkey is used.  The correct procedure is
    // to NOT pass wxEVT_CHAR events to the GUI under OS X.
    //
    // On Windows, avoid to call wxEvent::Skip for special keys because some keys
    // (PAGE_UP, PAGE_DOWN) have predefined actions (like move thumbtrack cursor), and we do
    // not want these actions executed (most are handled by KiCad)

    if( !evt || type == wxEVT_LEFT_DOWN )
        aEvent.Skip();

    // Not handled wxEVT_CHAR must be Skipped (sent to GUI).
    // Otherwise accelerators and shortcuts in main menu or toolbars are not seen.
    // Escape key presses are never skipped by the handler since they correspond to tool cancel
    // events, and if they aren't skipped then they are propagated to other frames (which we
    // don't want).
    if( ( type == wxEVT_CHAR || type == wxEVT_CHAR_HOOK )
             && !keyIsSpecial
             && !handled
             && !keyIsEscape )
    {
        aEvent.Skip();
    }

    wxLogTrace( kicadTraceToolStack, "TOOL_DISPATCHER::DispatchWxEvent - Wx event skipped: %s",
                ( aEvent.GetSkipped() ? "true" : "false" ) );
}

//  LocalWords:  EDA CERN CHANGELOG txt Tomasz Wlostowski wxEvent WXK
//  LocalWords:  MERCHANTABILITY bool upEvent downEvent touchpad Ctrl
//  LocalWords:  wxEVENT isKeySpecialCode thumbtrack DispatchWxEvent
//  LocalWords:  NUMPAD PAGEUP PAGEDOWN wxEVT EVT OSX ctrl ctlr kVK
//  LocalWords:  unicode ESC keypress wxTimer iconized autopanning WX
//  LocalWords:  un Wx
