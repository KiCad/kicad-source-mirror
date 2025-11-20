/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
#include <iterator>

#include <ki_any.h>

#include <math/vector2d.h>
#include <optional>
#include <atomic>

#include <tool/tool_action.h>
#include <wx/debug.h>

class COMMIT;
class TOOL_ACTION;
class TOOL_MANAGER;
class TOOL_BASE;
class TOOLS_HOLDER;

/**
 * Internal (GUI-independent) event definitions.
 */
enum TOOL_EVENT_CATEGORY : unsigned int
{
    TC_NONE     = 0x00,
    TC_MOUSE    = 0x01,
    TC_KEYBOARD = 0x02,
    TC_COMMAND  = 0x04,
    TC_MESSAGE  = 0x08,
    TC_VIEW     = 0x10,
    TC_ANY      = 0xffffffff
};

enum TOOL_ACTIONS : unsigned int
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

    /// Tool cancel event. Issued automagically when the user hits escape or selects End Tool from
    /// the context menu.
    TA_CANCEL_TOOL          = 0x2000,

    /// Context menu update. Issued whenever context menu is open and the user hovers the mouse
    /// over one of choices. Used in dynamic highlighting in disambiguation menu.
    TA_CHOICE_MENU_UPDATE   = 0x4000,

    /// Context menu choice. Sent if the user picked something from the context menu or
    /// closed it without selecting anything.
    TA_CHOICE_MENU_CHOICE   = 0x8000,

    /// Context menu is closed, no matter whether anything has been chosen or not.
    TA_CHOICE_MENU_CLOSED   = 0x10000,

    TA_CHOICE_MENU = TA_CHOICE_MENU_UPDATE | TA_CHOICE_MENU_CHOICE | TA_CHOICE_MENU_CLOSED,

    /// This event is sent *before* undo/redo command is performed.
    TA_UNDO_REDO_PRE        = 0x20000,

    /// This event is sent *after* undo/redo command is performed.
    TA_UNDO_REDO_POST       = 0x40000,

    /// Tool action (allows one to control tools).
    TA_ACTION               = 0x80000,

    /// Tool activation event.
    TA_ACTIVATE             = 0x100000,

    /// Tool re-activation event for tools already on the stack.
    TA_REACTIVATE           = 0x200000,

    /// Model has changed (partial update).
    TA_MODEL_CHANGE         = 0x400000,

    /// Tool priming event (a special mouse click).
    TA_PRIME                = 0x800001,

    TA_ANY = 0xffffffff
};

enum TOOL_MOUSE_BUTTONS : unsigned int
{
    BUT_NONE         = 0x0,
    BUT_LEFT         = 0x1,
    BUT_RIGHT        = 0x2,
    BUT_MIDDLE       = 0x4,
    BUT_AUX1         = 0x8,
    BUT_AUX2         = 0x10,
    BUT_BUTTON_MASK  = BUT_LEFT | BUT_RIGHT | BUT_MIDDLE | BUT_AUX1 | BUT_AUX2,
    BUT_ANY          = 0xffffffff
};

enum TOOL_MODIFIERS : unsigned int
{
    MD_SHIFT        = 0x1000,
    MD_CTRL         = 0x2000,
    MD_ALT          = 0x4000,
    MD_SUPER        = 0x8000,
    MD_META         = 0x10000,
    MD_ALTGR        = 0x20000,
    MD_MODIFIER_MASK = MD_SHIFT | MD_CTRL | MD_ALT | MD_SUPER | MD_META | MD_ALTGR,
};

/// Defines when a context menu is opened.
enum CONTEXT_MENU_TRIGGER
{
    CMENU_BUTTON = 0,   ///< On the right button.
    CMENU_NOW,          ///< Right now (after TOOL_INTERACTIVE::SetContextMenu).
    CMENU_OFF           ///< Never.
};

enum SYNCRONOUS_TOOL_STATE
{
    STS_RUNNING,
    STS_FINISHED,
    STS_CANCELLED
};

/**
 * Generic, UI-independent tool event.
 */
class TOOL_EVENT
{
public:
    /**
     * Return information about event in form of a human-readable string.
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
        m_modifiers( 0 ),
        m_synchronousState( nullptr ),
        m_commit( nullptr ),
        m_firstResponder( nullptr )
    {
        init();
    }

    TOOL_EVENT( TOOL_EVENT_CATEGORY aCategory, TOOL_ACTIONS aAction, int aExtraParam,
                TOOL_ACTION_SCOPE aScope = AS_GLOBAL ) :
        m_category( aCategory ),
        m_actions( aAction ),
        m_scope( aScope ),
        m_mouseButtons( 0 ),
        m_keyCode( 0 ),
        m_modifiers( 0 ),
        m_synchronousState( nullptr ),
        m_commit( nullptr ),
        m_firstResponder( nullptr )
    {
        if( aCategory == TC_MOUSE )
        {
            setMouseButtons( aExtraParam & BUT_BUTTON_MASK );
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

        init();
    }

    TOOL_EVENT( TOOL_EVENT_CATEGORY aCategory, TOOL_ACTIONS aAction,
            const std::string& aExtraParam, TOOL_ACTION_SCOPE aScope = AS_GLOBAL ) :
        m_category( aCategory ),
        m_actions( aAction ),
        m_scope( aScope ),
        m_mouseButtons( 0 ),
        m_keyCode( 0 ),
        m_modifiers( 0 ),
        m_synchronousState( nullptr ),
        m_commit( nullptr ),
        m_firstResponder( nullptr )
    {
        if( aCategory == TC_COMMAND || aCategory == TC_MESSAGE )
            m_commandStr = aExtraParam;

        init();
    }

    /// Return the category (eg. mouse/keyboard/action) of an event.
    TOOL_EVENT_CATEGORY Category() const { return m_category; }

    /// Returns more specific information about the type of an event.
    TOOL_ACTIONS Action() const { return m_actions; }

    /// These give a tool a method of informing the TOOL_MANAGER that a particular event should
    /// be passed on to subsequent tools on the stack.  Defaults to true for TC_MESSAGES; false
    /// for everything else.
    bool PassEvent() const { return m_passEvent; }
    void SetPassEvent( bool aPass = true ) { m_passEvent = aPass; }

    /// Returns if it this event has a valid position (true for mouse events and context-menu
    /// or hotkey-based command events).
    bool HasPosition() const { return m_hasPosition; }
    void SetHasPosition( bool aHasPosition ) { m_hasPosition = aHasPosition; }

    /// Returns if the action associated with this event should be treated as immediate regardless
    /// of the current immediate action settings.
    bool ForceImmediate() const { return m_forceImmediate; }
    void SetForceImmediate( bool aForceImmediate = true ) { m_forceImmediate = aForceImmediate; }

    TOOL_BASE* FirstResponder() const { return m_firstResponder; }
    void SetFirstResponder( TOOL_BASE* aTool ) { m_firstResponder = aTool; }

    /// Control whether the tool is first being pushed to the stack or being reactivated after
    /// a pause.
    bool IsReactivate() const { return m_reactivate; }
    void SetReactivate( bool aReactivate = true ) { m_reactivate = aReactivate; }

    void SetSynchronous( std::atomic<SYNCRONOUS_TOOL_STATE>* aState )
    {
        m_synchronousState = aState;
    }
    std::atomic<SYNCRONOUS_TOOL_STATE>* SynchronousState() const { return m_synchronousState; }

    void SetCommit( COMMIT* aCommit ) { m_commit = aCommit; }
    COMMIT* Commit() const { return m_commit; }

    /// Return information about difference between current mouse cursor position and the place
    /// where dragging has started.
    const VECTOR2D Delta() const
    {
        return returnCheckedPosition( m_mouseDelta );
    }

    /// Return mouse cursor position in world coordinates.
    const VECTOR2D Position() const
    {
        return returnCheckedPosition( m_mousePos );
    }

    /// Return the point where dragging has started.
    const VECTOR2D DragOrigin() const
    {
        return returnCheckedPosition( m_mouseDragOrigin );
    }

    /// Return information about mouse buttons state.
    int Buttons() const
    {
        assert( m_category == TC_MOUSE );    // this should be used only with mouse events
        return m_mouseButtons;
    }

    bool IsClick( int aButtonMask = BUT_ANY ) const;

    bool IsDblClick( int aButtonMask = BUT_ANY ) const;

    bool IsDrag( int aButtonMask = BUT_ANY ) const
    {
        return m_actions == TA_MOUSE_DRAG && ( m_mouseButtons & aButtonMask ) == m_mouseButtons;
    }

    bool IsMouseDown( int aButtonMask = BUT_ANY ) const
    {
        return m_actions == TA_MOUSE_DOWN && ( m_mouseButtons & aButtonMask ) == m_mouseButtons;
    }

    bool IsMouseUp( int aButtonMask = BUT_ANY ) const
    {
        return m_actions == TA_MOUSE_UP && ( m_mouseButtons & aButtonMask ) == m_mouseButtons;
    }

    bool IsMotion() const
    {
        return m_actions == TA_MOUSE_MOTION;
    }

    bool IsMouseAction() const
    {
        return ( m_actions & TA_MOUSE );
    }

    bool IsCancel() const
    {
        return m_actions == TA_CANCEL_TOOL;
    }

    bool IsActivate() const
    {
        return m_actions == TA_ACTIVATE;
    }

    bool IsUndoRedo() const
    {
        return m_actions & ( TA_UNDO_REDO_PRE | TA_UNDO_REDO_POST );
    }

    bool IsChoiceMenu() const
    {
        return m_actions & TA_CHOICE_MENU;
    }

    bool IsPrime() const
    {
        return m_actions == TA_PRIME;
    }

    /// Return information about key modifiers state (Ctrl, Alt, etc.).
    int Modifier( int aMask = MD_MODIFIER_MASK ) const
    {
        return m_modifiers & aMask;
    }

    bool DisableGridSnapping() const
    {
        return Modifier( MD_CTRL );
    }

    int KeyCode() const
    {
        return m_keyCode;
    }

    bool IsKeyPressed() const
    {
        return m_actions == TA_KEY_PRESSED;
    }

    /**
     * Test whether two events match in terms of category & action or command.
     *
     * @param aEvent is the event to test against.
     * @return True if two events match, false otherwise.
     */
    bool Matches( const TOOL_EVENT& aEvent ) const
    {
        if( !( m_category & aEvent.m_category ) )
            return false;

        if( m_category == TC_COMMAND || m_category == TC_MESSAGE )
        {
            if( !m_commandStr.empty() && !aEvent.getCommandStr().empty() )
                return m_commandStr == aEvent.m_commandStr;

            if( (bool) m_commandId && (bool) aEvent.m_commandId )
                return *m_commandId == *aEvent.m_commandId;
        }

        // BUGFIX: TA_ANY should match EVERYTHING, even TA_NONE (for TC_MESSAGE)
        if( m_actions == TA_ANY && aEvent.m_actions == TA_NONE && aEvent.m_category == TC_MESSAGE )
            return true;

        // BUGFIX: This check must happen after the TC_COMMAND check because otherwise events of
        // the form { TC_COMMAND, TA_NONE } will be incorrectly skipped
        if( !( m_actions & aEvent.m_actions ) )
            return false;

        return true;
    }

    /**
     * Test if the event contains an action issued upon activation of the given #TOOL_ACTION.
     *
     * @param aAction is the TOOL_ACTION to be checked against.
     * @return True if it matches the given TOOL_ACTION.
     */
    bool IsAction( const TOOL_ACTION* aAction ) const;

    /**
     * Indicate the event should restart/end an ongoing interactive tool's event loop (eg esc
     * key, click cancel, start different tool).
     */
    bool IsCancelInteractive() const;

    /**
     * Indicate an selection-changed notification event.
     */
    bool IsSelectionEvent() const;

    /**
     * Indicate if the event is from one of the point editors.
     *
     * Usually used to allow the point editor to activate itself without de-activating the
     * current drawing tool.
     */
    bool IsPointEditor() const;

    /**
     * Indicate if the event is from one of the move tools.
     *
     * Usually used to allow move to be done without de-activating the current drawing tool.
     */
    bool IsMoveTool() const;

    /**
     * Indicate if the event is asking for an editor tool.
     *
     * Used to allow deleting an element without de-activating the current tool.
     */
    bool IsEditorTool() const;

    /**
     * Indicate if the event is from the simulator.
     */
    bool IsSimulator() const;

    bool HasParameter() const
    {
        return m_param.has_value();
    }

    /**
     * Return a parameter assigned to the event. Its meaning depends on the target tool.
     */
    template<typename T, std::enable_if_t<!std::is_pointer<T>::value>* = nullptr >
    T Parameter() const
    {
        T param;

        wxCHECK_MSG( m_param.has_value(), T(), "Attempted to get a parameter from an event with "
                                               "no parameter." );

        try
        {
            param = ki::any_cast<T>( m_param );
        }
        catch( const ki::bad_any_cast& )
        {
            wxCHECK_MSG( false, T(), wxString::Format( "Requested parameter type %s from event "
                                                       "with parameter type %s.",
                                                       typeid(T).name(),
                                                       m_param.type().name() ) );
        }

        return param;
    }

    /**
     * Return pointer parameter assigned to the event. Its meaning depends on the target tool.
     */
    template<typename T, std::enable_if_t<std::is_pointer<T>::value>* = nullptr>
    T Parameter() const
    {
        T param = nullptr;

        wxCHECK_MSG( m_param.has_value(), param, "Attempted to get a parameter from an event with "
                                                 "no parameter." );

        try
        {
            param = ki::any_cast<T>( m_param );
        }
        catch( const ki::bad_any_cast& )
        {
            wxCHECK_MSG( false, param, wxString::Format( "Requested parameter type %s from event "
                                                         "with parameter type %s.",
                                                         typeid(T).name(),
                                                         m_param.type().name() ) );
        }

        return param;
    }

    /**
     * Set a non-standard parameter assigned to the event. Its meaning depends on the
     * target tool.
     *
     * @param aParam is the new parameter.
     */
    template<typename T>
    void SetParameter(T aParam)
    {
        m_param = aParam;
    }

    std::optional<int> GetCommandId() const
    {
        return m_commandId;
    }

    void SetMousePosition( const VECTOR2D& aP )
    {
        m_mousePos = aP;
    }

    void SetActionGroup( const TOOL_ACTION_GROUP& aGroup )
    {
        m_actionGroup = aGroup;
    }

    bool IsActionInGroup( const TOOL_ACTION_GROUP& aGroup ) const;

private:
    friend class TOOL_EVENT_LIST;
    friend class TOOL_DISPATCHER;
    friend class TOOL_MANAGER;
    friend class TOOLS_HOLDER;

    void init();

    const std::string& getCommandStr() const { return m_commandStr; }

    void setMouseDragOrigin( const VECTOR2D& aP )
    {
        m_mouseDragOrigin = aP;
     }

    void setMouseDelta( const VECTOR2D& aP )
    {
        m_mouseDelta = aP;
    }

    void setMouseButtons( int aButtons )
    {
        assert( ( aButtons & ~BUT_BUTTON_MASK ) == 0 );
        m_mouseButtons = aButtons;
    }

    void setModifiers( int aMods )
    {
        assert( ( aMods & ~MD_MODIFIER_MASK ) == 0 );
        m_modifiers = aMods;
    }

    /**
     * Ensure that the event is a type that has a position before returning a
     * position, otherwise return a null-constructed position.
     *
     * Used to defend the position accessors from runtime access when the event
     * does not have a valid position.
     *
     * @param aPos the position to return if the event is valid
     * @return the checked position
     */
    VECTOR2D returnCheckedPosition( const VECTOR2D& aPos ) const;

    TOOL_EVENT_CATEGORY m_category;
    TOOL_ACTIONS m_actions;
    TOOL_ACTION_SCOPE m_scope;
    bool m_passEvent;
    bool m_hasPosition;
    bool m_forceImmediate;


    /// Optional group that the parent action for the event belongs to.
    std::optional<TOOL_ACTION_GROUP> m_actionGroup;

    /// True when the tool is being re-activated from the stack.
    bool m_reactivate;

    /// Difference between mouse cursor position and the point where dragging event has started.
    VECTOR2D m_mouseDelta;

    /// Current mouse cursor position.
    VECTOR2D m_mousePos;

    /// Point where dragging has started.
    VECTOR2D m_mouseDragOrigin;

    /// State of mouse buttons.
    int m_mouseButtons;

    /// Stores code of pressed/released key.
    int m_keyCode;

    /// State of key modifiers (Ctrl/Alt/etc.).
    int m_modifiers;

    std::atomic<SYNCRONOUS_TOOL_STATE>* m_synchronousState;

    /// Commit the tool handling the event should add to
    COMMIT* m_commit;

    /// Generic parameter used for passing non-standard data.
    ki::any m_param;

    /// The first tool to receive the event.
    TOOL_BASE* m_firstResponder;

    std::optional<int> m_commandId;
    std::string        m_commandStr;
};

typedef std::optional<TOOL_EVENT> OPT_TOOL_EVENT;

/**
 * A list of TOOL_EVENTs, with overloaded || operators allowing for concatenating TOOL_EVENTs
 * with little code.
 */
class TOOL_EVENT_LIST
{
public:
    typedef TOOL_EVENT value_type;
    typedef std::deque<TOOL_EVENT>::iterator iterator;
    typedef std::deque<TOOL_EVENT>::const_iterator const_iterator;

    /// Default constructor. Creates an empty list.
    TOOL_EVENT_LIST()
    {}

    /// Constructor for a list containing only one TOOL_EVENT.
    TOOL_EVENT_LIST( const TOOL_EVENT& aSingleEvent )
    {
        m_events.push_back( aSingleEvent );
    }

    /// Copy an existing TOOL_EVENT_LIST
    TOOL_EVENT_LIST( const TOOL_EVENT_LIST& aEventList )
    {
        m_events.clear();

        for( const TOOL_EVENT& event : aEventList.m_events )
            m_events.push_back( event );
    }

    /**
     * Return information about event in form of a human-readable string.
     *
     * @return Event information.
     */
    const std::string Format() const;

    /**
     * Return a string containing the names of all the events in this list.
     *
     * @return Event names.
     */
    const std::string Names() const;

    OPT_TOOL_EVENT Matches( const TOOL_EVENT& aEvent ) const
    {
        for( const TOOL_EVENT& event : m_events )
        {
            if( event.Matches( aEvent ) )
                return event;
        }

        return OPT_TOOL_EVENT();
    }

    /**
     * Add a tool event to the list.
     *
     * @param aEvent is the tool event to be added.
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

        for( const TOOL_EVENT& event : aEventList.m_events )
            m_events.push_back( event );

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
        std::copy( aEvent.m_events.begin(), aEvent.m_events.end(), std::back_inserter( m_events ) );
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
