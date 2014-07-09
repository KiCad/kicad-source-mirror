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

#ifndef __TOOL_EVENT_H
#define __TOOL_EVENT_H

#include <cstdio>
#include <deque>

#include <math/vector2d.h>
#include <cassert>

#include <boost/optional.hpp>

class TOOL_ACTION;
class TOOL_MANAGER;

/**
 * Internal (GUI-independent) event definitions.
 * Enums are mostly self-explanatory.
 */
enum TOOL_EVENT_CATEGORY
{
    TC_NONE     = 0x00,
    TC_MOUSE    = 0x01,
    TC_KEYBOARD = 0x02,
    TC_COMMAND  = 0x04,
    TC_MESSAGE  = 0x08,
    TC_VIEW     = 0x10,
    TC_ANY      = 0xffffffff
};

enum TOOL_ACTIONS
{
    // UI input events
    TA_NONE                 = 0x0000,
    TA_MOUSE_CLICK          = 0x0001,
    TA_MOUSE_DBLCLICK       = 0x0002,
    TA_MOUSE_UP             = 0x0004,
    TA_MOUSE_DOWN           = 0x0008,
    TA_MOUSE_DRAG           = 0x0010,
    TA_MOUSE_MOTION         = 0x0020,
    TA_MOUSE_WHEEL          = 0x0040,
    TA_MOUSE                = 0x007f,

    TA_KEY_PRESSED          = 0x0080,
    TA_KEYBOARD             = TA_KEY_PRESSED,

    // View related events
    TA_VIEW_REFRESH         = 0x0100,
    TA_VIEW_ZOOM            = 0x0200,
    TA_VIEW_PAN             = 0x0400,
    TA_VIEW_DIRTY           = 0x0800,
    TA_VIEW                 = 0x0f00,

    TA_CHANGE_LAYER         = 0x1000,

    // Tool cancel event. Issued automagically when the user hits escape or selects End Tool from
    // the context menu.
    TA_CANCEL_TOOL          = 0x2000,

    // Context menu update. Issued whenever context menu is open and the user hovers the mouse
    // over one of choices. Used in dynamic highligting in disambiguation menu
    TA_CONTEXT_MENU_UPDATE  = 0x4000,

    // Context menu choice. Sent if the user picked something from the context menu or
    // closed it without selecting anything.
    TA_CONTEXT_MENU_CHOICE  = 0x8000,

    // This event is sent *before* undo/redo command is performed.
    TA_UNDO_REDO            = 0x10000,

    // Tool action (allows to control tools).
    TA_ACTION               = 0x20000,

    // Tool activation event.
    TA_ACTIVATE             = 0x40000,

    TA_ANY = 0xffffffff
};

enum TOOL_MOUSE_BUTTONS
{
    BUT_NONE         = 0x0,
    BUT_LEFT         = 0x1,
    BUT_RIGHT        = 0x2,
    BUT_MIDDLE       = 0x4,
    BUT_BUTTON_MASK  = BUT_LEFT | BUT_RIGHT | BUT_MIDDLE,
    BUT_ANY          = 0xffffffff
};

enum TOOL_MODIFIERS
{
    MD_SHIFT        = 0x1000,
    MD_CTRL         = 0x2000,
    MD_ALT          = 0x4000,
    MD_MODIFIER_MASK = MD_SHIFT | MD_CTRL | MD_ALT,
};

/// Scope of tool actions
enum TOOL_ACTION_SCOPE
{
    AS_CONTEXT = 1,  ///> Action belongs to a particular tool (i.e. a part of a pop-up menu)
    AS_ACTIVE,       ///> All active tools
    AS_GLOBAL        ///> Global action (toolbar/main menu event, global shortcut)
};

/// Flags for tool actions
enum TOOL_ACTION_FLAGS
{
    AF_NONE     = 0,
    AF_ACTIVATE = 1,    ///> Action activates a tool
    AF_NOTIFY   = 2     ///> Action is a notification (it is by default passed to all tools)
};

/// Defines when a context menu is opened.
enum CONTEXT_MENU_TRIGGER
{
    CMENU_BUTTON = 0,   // On the right button
    CMENU_NOW,          // Right now (after TOOL_INTERACTIVE::SetContextMenu)
    CMENU_OFF           // Never
};

/**
 * Class TOOL_EVENT
 *
 * Generic, UI-independent tool event.
 */
class TOOL_EVENT
{
public:
    /**
     * Function Format()
     * Returns information about event in form of a human-readable string.
     *
     * @return Event information.
     */
    const std::string Format() const;

    TOOL_EVENT( TOOL_EVENT_CATEGORY aCategory = TC_NONE, TOOL_ACTIONS aAction = TA_NONE,
            TOOL_ACTION_SCOPE aScope = AS_GLOBAL ) :
        m_category( aCategory ),
        m_actions( aAction ),
        m_scope( aScope ),
        m_mouseButtons( 0 ),
        m_keyCode( 0 ),
        m_modifiers( 0 ) {}

    TOOL_EVENT( TOOL_EVENT_CATEGORY aCategory, TOOL_ACTIONS aAction, int aExtraParam,
            TOOL_ACTION_SCOPE aScope = AS_GLOBAL ) :
        m_category( aCategory ),
        m_actions( aAction ),
        m_scope( aScope )
    {
        if( aCategory == TC_MOUSE )
        {
            m_mouseButtons = aExtraParam & BUT_BUTTON_MASK;
        }
        else if( aCategory == TC_KEYBOARD )
        {
            m_keyCode = aExtraParam & ~MD_MODIFIER_MASK;         // Filter out modifiers
        }
        else if( aCategory == TC_COMMAND )
        {
            m_commandId = aExtraParam;
        }

        if( aCategory & ( TC_MOUSE | TC_KEYBOARD ) )
        {
            m_modifiers = aExtraParam & MD_MODIFIER_MASK;
        }
    }

    TOOL_EVENT( TOOL_EVENT_CATEGORY aCategory, TOOL_ACTIONS aAction,
            const std::string& aExtraParam, TOOL_ACTION_SCOPE aScope = AS_GLOBAL ) :
        m_category( aCategory ),
        m_actions( aAction ),
        m_scope( aScope ),
        m_mouseButtons( 0 )
    {
        if( aCategory == TC_COMMAND || aCategory == TC_MESSAGE )
            m_commandStr = aExtraParam;
    }

    ///> Returns the category (eg. mouse/keyboard/action) of an event..
    TOOL_EVENT_CATEGORY Category() const
    {
        return m_category;
    }

    ///> Returns more specific information about the type of an event.
    TOOL_ACTIONS Action() const
    {
        return m_actions;
    }

    ///> Returns information about difference between current mouse cursor position and the place
    ///> where dragging has started.
    const VECTOR2D& Delta() const
    {
        assert( m_category == TC_MOUSE );    // this should be used only with mouse events
        return m_mouseDelta;
    }

    ///> Returns mouse cursor position in world coordinates.
    const VECTOR2D& Position() const
    {
        assert( m_category == TC_MOUSE );    // this should be used only with mouse events
        return m_mousePos;
    }

    ///> Returns the point where dragging has started.
    const VECTOR2D& DragOrigin() const
    {
        assert( m_category == TC_MOUSE );    // this should be used only with mouse events
        return m_mouseDragOrigin;
    }

    ///> Returns information about mouse buttons state.
    int Buttons() const
    {
        assert( m_category == TC_MOUSE );    // this should be used only with mouse events
        return m_mouseButtons;
    }

    bool IsClick( int aButtonMask = BUT_ANY ) const
    {
        return ( m_actions == TA_MOUSE_CLICK )
               && ( ( m_mouseButtons & aButtonMask ) == aButtonMask );
    }

    bool IsDblClick( int aButtonMask = BUT_ANY ) const
    {
        return ( m_actions == TA_MOUSE_DBLCLICK )
               && ( ( m_mouseButtons & aButtonMask ) == aButtonMask );
    }

    bool IsDrag( int aButtonMask = BUT_ANY ) const
    {
        return ( m_actions == TA_MOUSE_DRAG ) && ( ( m_mouseButtons & aButtonMask ) == aButtonMask );
    }

    bool IsMouseUp( int aButtonMask = BUT_ANY ) const
    {
        return ( m_actions == TA_MOUSE_UP ) && ( ( m_mouseButtons & aButtonMask ) == aButtonMask );
    }

    bool IsMotion() const
    {
        return m_actions == TA_MOUSE_MOTION;
    }

    bool IsCancel() const
    {
        return m_actions == TA_CANCEL_TOOL;
    }

    bool IsActivate() const
    {
        return m_actions == TA_ACTIVATE;
    }

    ///> Returns information about key modifiers state (Ctrl, Alt, etc.)
    int Modifier( int aMask = MD_MODIFIER_MASK ) const
    {
        return m_modifiers & aMask;
    }

    int KeyCode() const
    {
        return m_keyCode;
    }

    bool IsKeyPressed() const
    {
        return m_actions == TA_KEY_PRESSED;
    }

    void SetMouseDragOrigin( const VECTOR2D& aP )
    {
        m_mouseDragOrigin = aP;
     }

    void SetMousePosition( const VECTOR2D& aP )
    {
        m_mousePos = aP;
    }

    void SetMouseDelta( const VECTOR2D& aP )
    {
        m_mouseDelta = aP;
    }

    /**
     * Function Matches()
     * Tests whether two events match in terms of category & action or command.
     *
     * @param aEvent is the event to test against.
     * @return True if two events match, false otherwise.
     */
    bool Matches( const TOOL_EVENT& aEvent ) const
    {
        if( !( m_category & aEvent.m_category ) )
            return false;

        if( !( m_actions & aEvent.m_actions ) )
            return false;

        if( m_category == TC_COMMAND || m_category == TC_MESSAGE )
        {
            if( (bool) m_commandStr && (bool) aEvent.m_commandStr )
                return *m_commandStr == *aEvent.m_commandStr;

            if( (bool) m_commandId && (bool) aEvent.m_commandId )
                return *m_commandId == *aEvent.m_commandId;
        }

        return true;
    }

    /**
     * Function IsAction()
     * Tests if the event contains an action issued upon activation of the given TOOL_ACTION.
     * @param aAction is the TOOL_ACTION to be checked against.
     * @return True if it matches the given TOOL_ACTION.
     */
    bool IsAction( const TOOL_ACTION* aAction ) const;

    boost::optional<int> GetCommandId() const
    {
        return m_commandId;
    }

    boost::optional<std::string> GetCommandStr() const
    {
        return m_commandStr;
    }

private:
    friend class TOOL_MANAGER;

    TOOL_EVENT_CATEGORY m_category;
    TOOL_ACTIONS m_actions;
    TOOL_ACTION_SCOPE m_scope;

    ///> Difference between mouse cursor position and
    ///> the point where dragging event has started
    VECTOR2D m_mouseDelta;

    ///> Current mouse cursor position
    VECTOR2D m_mousePos;

    ///> Point where dragging has started
    VECTOR2D m_mouseDragOrigin;

    ///> State of mouse buttons
    int m_mouseButtons;

    ///> Stores code of pressed/released key
    int m_keyCode;

    ///> State of key modifierts (Ctrl/Alt/etc.)
    int m_modifiers;

    boost::optional<int> m_commandId;
    boost::optional<std::string> m_commandStr;
};

typedef boost::optional<TOOL_EVENT> OPT_TOOL_EVENT;

/**
 * Class TOOL_EVENT_LIST
 *
 * A list of TOOL_EVENTs, with overloaded || operators allowing for
 * concatenating TOOL_EVENTs with little code.
 */
class TOOL_EVENT_LIST
{
public:
    typedef TOOL_EVENT value_type;
    typedef std::deque<TOOL_EVENT>::iterator iterator;
    typedef std::deque<TOOL_EVENT>::const_iterator const_iterator;

    ///> Default constructor. Creates an empty list.
    TOOL_EVENT_LIST()
    {}

    ///> Constructor for a list containing only one TOOL_EVENT.
    TOOL_EVENT_LIST( const TOOL_EVENT& aSingleEvent )
    {
        m_events.push_back( aSingleEvent );
    }

    /**
     * Function Format()
     * Returns information about event in form of a human-readable string.
     *
     * @return Event information.
     */
    const std::string Format() const;

    boost::optional<const TOOL_EVENT&> Matches( const TOOL_EVENT& aEvent ) const
    {
        for( const_iterator i = m_events.begin(); i != m_events.end(); ++i )
            if( i->Matches( aEvent ) )
                return *i;

        return boost::optional<const TOOL_EVENT&>();
    }

    /**
     * Function Add()
     * Adds a tool event to the list.
     * @param aEvent is the tool event to be addded.
     */
    void Add( const TOOL_EVENT& aEvent )
    {
        m_events.push_back( aEvent );
    }

    iterator begin()
    {
        return m_events.begin();
    }

    iterator end()
    {
        return m_events.end();
    }

    const_iterator cbegin() const
    {
        return m_events.begin();
    }

    const_iterator cend() const
    {
        return m_events.end();
    }

    int size() const
    {
        return m_events.size();
    }

    void clear()
    {
        m_events.clear();
    }

    TOOL_EVENT_LIST& operator=( const TOOL_EVENT_LIST& aEventList )
    {
        m_events.clear();

        for( std::deque<TOOL_EVENT>::const_iterator i = aEventList.m_events.begin();
             i != aEventList.m_events.end(); ++i )
        {
            m_events.push_back( *i );
        }

        return *this;
    }

    TOOL_EVENT_LIST& operator=( const TOOL_EVENT& aEvent )
    {
        m_events.clear();
        m_events.push_back( aEvent );
        return *this;
    }

    TOOL_EVENT_LIST& operator||( const TOOL_EVENT& aEvent )
    {
        Add( aEvent );
        return *this;
    }

    TOOL_EVENT_LIST& operator||( const TOOL_EVENT_LIST& aEvent )
    {
        return *this;
    }

private:
    std::deque<TOOL_EVENT> m_events;
};

inline const TOOL_EVENT_LIST operator||( const TOOL_EVENT& aEventA, const TOOL_EVENT& aEventB )
{
    TOOL_EVENT_LIST l;

    l.Add( aEventA );
    l.Add( aEventB );

    return l;
}

inline const TOOL_EVENT_LIST operator||( const TOOL_EVENT& aEvent,
                                         const TOOL_EVENT_LIST& aEventList )
{
    TOOL_EVENT_LIST l( aEventList );

    l.Add( aEvent );
    return l;
}

#endif
