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

#include <boost/optional.hpp>

class TOOL_MANAGER;

/**
 * Internal (GUI-independent) event definitions.
 * Enums are mostly self-explanatory.
 */
enum TOOL_EventCategory
{
	TC_None     = 0x00,
	TC_Mouse    = 0x01,
	TC_Keyboard = 0x02,
	TC_Command  = 0x04,
	TC_Message  = 0x08,
	TC_View     = 0x10,
	TC_Any      = 0xffffffff
};

enum TOOL_Actions
{
    // UI input events
	TA_None         = 0x0000,
	TA_MouseClick   = 0x0001,
	TA_MouseUp      = 0x0002,
	TA_MouseDown    = 0x0004,
	TA_MouseDrag    = 0x0008,
	TA_MouseMotion  = 0x0010,
	TA_MouseWheel   = 0x0020,
	TA_Mouse        = 0x003f,
	TA_KeyUp        = 0x0040,
    TA_KeyDown      = 0x0080,
    TA_Keyboard     = TA_KeyUp | TA_KeyDown,

    // View related events
	TA_ViewRefresh  = 0x0100,
	TA_ViewZoom     = 0x0200,
	TA_ViewPan      = 0x0400,
	TA_ViewDirty    = 0x0800,
	TA_ChangeLayer  = 0x1000,

	// Tool cancel event. Issued automagically when the user hits escape or selects End Tool from the context menu.
	TA_CancelTool   = 0x2000,

	// Context menu update. Issued whenever context menu is open and the user hovers the mouse over one of choices.
	// Used in dynamic highligting in disambiguation menu 
	TA_ContextMenuUpdate = 0x4000,

	// Context menu choice. Sent if the user picked something from the context menu or closed it without selecting anything.
	TA_ContextMenuChoice = 0x8000,

	// Tool action
	TA_Action            = 0x10000,

	TA_Any = 0xffffffff
};

enum TOOL_MouseButtons
{
	MB_None         = 0x0,
	MB_Left         = 0x1,
	MB_Right        = 0x2,
	MB_Middle       = 0x4,
	MB_ButtonMask   = MB_Left | MB_Right | MB_Middle,
	MB_Any          = 0xffffffff
};

enum TOOL_Modifiers
{
    MD_ModShift     = 0x1000,
    MD_ModCtrl      = 0x2000,
    MD_ModAlt       = 0x4000,
    MD_ModifierMask = MD_ModShift | MD_ModCtrl | MD_ModAlt,
};

/// Scope of tool actions
enum TOOL_ActionScope
{
    AS_CONTEXT = 1,  ///> Action belongs to a particular tool (i.e. a part of a pop-up menu)
    AS_ACTIVE,       ///> All active tools
    AS_GLOBAL        ///> Global action (toolbar/main menu event, global shortcut)
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
    const std::string Format() const;

    TOOL_EVENT( TOOL_EventCategory aCategory = TC_None, TOOL_Actions aAction = TA_None,
                TOOL_ActionScope aScope = AS_GLOBAL ) :
        m_category( aCategory ),
        m_actions( aAction ),
        m_scope( aScope ),
        m_mouseButtons( 0 ),
        m_keyCode( 0 ),
        m_modifiers( 0 ) {}

    TOOL_EVENT( TOOL_EventCategory aCategory, TOOL_Actions aAction, int aExtraParam, TOOL_ActionScope aScope = AS_GLOBAL ) :
        m_category( aCategory ),
        m_actions( aAction ),
        m_scope( aScope )
        {
            if( aCategory == TC_Mouse )
            {
                m_mouseButtons = aExtraParam & MB_ButtonMask;
            }
            else if( aCategory == TC_Keyboard )
            {
                m_keyCode = aExtraParam & ~MD_ModifierMask;     // Filter out modifiers
            }
            else if ( aCategory == TC_Command )
            {
                m_commandId = aExtraParam;
            }

            if( aCategory & ( TC_Mouse | TC_Keyboard ) )
            {
                m_modifiers = aExtraParam & MD_ModifierMask;
            }
        }

    TOOL_EVENT( TOOL_EventCategory aCategory, TOOL_Actions aAction,
                const std::string& aExtraParam, TOOL_ActionScope aScope = AS_GLOBAL ) :
        m_category( aCategory ),
        m_actions( aAction ),
        m_scope( aScope ),
        m_mouseButtons( 0 )
        {
            if( aCategory == TC_Command )
                m_commandStr = aExtraParam;
        }

    TOOL_EventCategory Category() const
    {
        return m_category;
    }

    TOOL_Actions Action() const
    {
        return m_actions;
    }

    const VECTOR2D Delta() const
    {
        return m_mouseDelta;
    }

    const VECTOR2D& Position() const
    {
        return m_mousePos;
    }

    const VECTOR2D& DragOrigin() const
    {
        return m_mouseDragOrigin;
    }

    int Buttons() const
    {
        return m_mouseButtons;
    }

    bool IsClick( int aButtonMask = MB_Any ) const
    {
        return ( m_actions == TA_MouseClick )
                && ( ( m_mouseButtons & aButtonMask ) == aButtonMask );
    }

    bool IsDrag( int aButtonMask = MB_Any ) const
    {
        return ( m_actions == TA_MouseDrag ) && ( ( m_mouseButtons & aButtonMask ) == aButtonMask );
    }

    bool IsMouseUp( int aButtonMask = MB_Any ) const
    {
        return ( m_actions == TA_MouseUp ) && ( ( m_mouseButtons & aButtonMask ) == aButtonMask );
    }

    bool IsMotion() const
    {
        return ( m_actions == TA_MouseMotion );
    }

    bool IsCancel() const
    {
        return m_actions == TA_CancelTool;
    }

    int Modifier( int aMask = MD_ModifierMask ) const
    {
        return ( m_modifiers & aMask );
    }

    int KeyCode() const
    {
        return m_keyCode;
    }

    bool IsKeyUp() const
    {
        return m_actions == TA_KeyUp;
    }

    bool IsKeyDown() const
    {
        return m_actions == TA_KeyDown;
    }

    void Ignore();

    void SetMouseDragOrigin( const VECTOR2D &aP )
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

    bool Matches( const TOOL_EVENT& aEvent ) const
    {
        if( !( m_category & aEvent.m_category ) )
            return false;

        if( !( m_actions & aEvent.m_actions ) )
            return false;

        if( m_category == TC_Command )
        {
            if( m_commandStr && aEvent.m_commandStr )
                return ( *m_commandStr == *aEvent.m_commandStr );
            if( m_commandId && aEvent.m_commandId )
                return ( *m_commandId == *aEvent.m_commandId );
        }

        return true;
    }

    boost::optional<int> GetCommandId()
    {
        return m_commandId;
    }

private:
    friend class TOOL_MANAGER;

    TOOL_EventCategory m_category;
    TOOL_Actions m_actions;
    TOOL_ActionScope m_scope;

    VECTOR2D m_mouseDelta;
    VECTOR2D m_mousePos;
    VECTOR2D m_mouseDragOrigin;

    int m_mouseButtons;
    int m_keyCode;
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
class TOOL_EVENT_LIST {
public:
    typedef TOOL_EVENT value_type;
    typedef std::deque<TOOL_EVENT>::iterator iterator;
    typedef std::deque<TOOL_EVENT>::const_iterator const_iterator;

    TOOL_EVENT_LIST() {};
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

    boost::optional<const TOOL_EVENT&> Matches( const TOOL_EVENT &b ) const
    {
        for( const_iterator i = m_events.begin(); i != m_events.end(); ++i )
            if ( i->Matches( b ) )
                return *i;
        return boost::optional<const TOOL_EVENT&>();
    }

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

    TOOL_EVENT_LIST& operator=( const TOOL_EVENT_LIST& b )
    {
        m_events.clear();

        for( std::deque<TOOL_EVENT>::const_iterator i = b.m_events.begin();
                i != b.m_events.end(); ++i )
        {
            m_events.push_back(*i);
        }

        return *this;
    }

    TOOL_EVENT_LIST& operator=( const TOOL_EVENT& b )
    {
        m_events.clear();
        m_events.push_back( b );
        return *this;
    }

    TOOL_EVENT_LIST& operator||( const TOOL_EVENT& b )
    {
        Add( b );
        return *this;
    }

    TOOL_EVENT_LIST& operator||( const TOOL_EVENT_LIST& b )
    {
        return *this;
    }

private:
    std::deque<TOOL_EVENT> m_events;
};

inline const TOOL_EVENT_LIST operator||( const TOOL_EVENT& a, const TOOL_EVENT& b )
{
	TOOL_EVENT_LIST l;

	l.Add( a );
	l.Add( b );

	return l;
}

inline const TOOL_EVENT_LIST operator||( const TOOL_EVENT& a, const TOOL_EVENT_LIST& b )
{
	TOOL_EVENT_LIST l( b );
	
	l.Add( a );
	return l;
}

#endif
