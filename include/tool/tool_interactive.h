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

#ifndef __TOOL_INTERACTIVE_H
#define __TOOL_INTERACTIVE_H

#include <string>

#include <tool/tool_event.h>
#include <tool/tool_base.h>

class CONTEXT_MENU;

class TOOL_INTERACTIVE : public TOOL_BASE
{
public:
    /**
     * Constructor
     *
     * Creates a tool with given id & name. The name must be unique. */
    TOOL_INTERACTIVE( TOOL_ID aId, const std::string& aName );

    /**
     * Constructor
     *
     * Creates a tool with given name. The name must be unique. */
    TOOL_INTERACTIVE( const std::string& aName );
    virtual ~TOOL_INTERACTIVE();

    /**
     * Function Activate()
     * Runs the tool. After activation, the tool starts receiving events until it is finished.
     */
    void Activate();

    /**
     * Function SetContextMenu()
     *
     * Assigns a context menu and tells when it should be activated.
     * @param aMenu is the menu to be assigned.
     * @param aTrigger determines conditions upon which the context menu is activated.
     */
    void SetContextMenu( CONTEXT_MENU* aMenu, CONTEXT_MENU_TRIGGER aTrigger = CMENU_BUTTON );

    /**
     * Function Go()
     *
     * Defines which state (aStateFunc) to go when a certain event arrives (aConditions).
     * No conditions means any event.
     */
    template <class T>
    void Go( int (T::* aStateFunc)( const TOOL_EVENT& ),
            const TOOL_EVENT_LIST& aConditions = TOOL_EVENT( TC_ANY, TA_ANY ) );

    /**
     * Function Wait()
     *
     * Suspends execution of the tool until an event specified in aEventList arrives.
     * No parameters means waiting for any event.
     */
    OPT_TOOL_EVENT Wait( const TOOL_EVENT_LIST& aEventList = TOOL_EVENT( TC_ANY, TA_ANY ) );

    /** functions below are not yet implemented - their interface may change */
    /*template <class Parameters, class ReturnValue>
        bool InvokeTool( const std::string& aToolName, const Parameters& parameters,
                         ReturnValue& returnValue );

    template <class Parameters, class ReturnValue>
        bool InvokeWindow( const std::string& aWindowName, const Parameters& parameters,
                           ReturnValue& returnValue );

    template <class T>
        void Yield( const T& returnValue );*/

protected:
    /* helper functions for constructing events for Wait() and Go() with less typing */
    const TOOL_EVENT evActivate( std::string aToolName = "" );
    const TOOL_EVENT evCommand( int aCommandId = -1 );
    const TOOL_EVENT evCommand( std::string aCommandStr = "" );
    const TOOL_EVENT evMotion();
    const TOOL_EVENT evClick( int aButton = BUT_ANY );
    const TOOL_EVENT evDrag( int aButton = BUT_ANY );
    const TOOL_EVENT evButtonUp( int aButton = BUT_ANY );
    const TOOL_EVENT evButtonDown(int aButton = BUT_ANY );

private:
    void goInternal( TOOL_STATE_FUNC& aState, const TOOL_EVENT_LIST& aConditions );
};

// hide TOOL_MANAGER implementation
template <class T>
void TOOL_INTERACTIVE::Go( int (T::* aStateFunc)( const TOOL_EVENT& ),
                           const TOOL_EVENT_LIST& aConditions )
{
    TOOL_STATE_FUNC sptr( static_cast<T*>( this ), aStateFunc );

    goInternal( sptr, aConditions );
}

#endif
