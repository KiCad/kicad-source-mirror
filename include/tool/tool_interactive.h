/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#ifndef __TOOL_INTERACTIVE_H
#define __TOOL_INTERACTIVE_H

#include <string>
#include <tool/tool_menu.h>
#include <tool/tool_event.h>    // Needed for CONTEXT_MENU_TRIGGER
#include <tool/tool_base.h>

class ACTION_MENU;


struct REENTRANCY_GUARD
{
    REENTRANCY_GUARD( bool* aFlag ) :
            m_flag( aFlag )
    {
        *m_flag = true;
    }

    ~REENTRANCY_GUARD()
    {
        *m_flag = false;
    }

private:
    bool* m_flag;
};


class TOOL_INTERACTIVE : public TOOL_BASE
{
public:
    /**
     * Create a tool with given id & name. The name must be unique.
     */
    TOOL_INTERACTIVE( TOOL_ID aId, const std::string& aName );

    /**
     * Creates a tool with given name. The name must be unique.
     */
    TOOL_INTERACTIVE( const std::string& aName );
    virtual ~TOOL_INTERACTIVE();

    /**
     * Run the tool.
     *
     * After activation, the tool starts receiving events until it is finished.
     */
    void Activate();

    TOOL_MENU& GetToolMenu();

    /**
     * Assign a context menu and tells when it should be activated.
     *
     * @param aMenu is the menu to be assigned.
     * @param aTrigger determines conditions upon which the context menu is activated.
     */
    void SetContextMenu( ACTION_MENU* aMenu, CONTEXT_MENU_TRIGGER aTrigger = CMENU_BUTTON );

    /**
     * Call a function using the main stack.
     *
     * @param aFunc is the function to be calls.
     */
    void RunMainStack( std::function<void()> aFunc );

    /**
     * Define which state (aStateFunc) to go when a certain event arrives (aConditions).
     *
     * No conditions means any event.
     */
    template <class T>
    void Go( int (T::* aStateFunc)( const TOOL_EVENT& ),
             const TOOL_EVENT_LIST& aConditions = TOOL_EVENT( TC_ANY, TA_ANY ) );

    /**
     * Suspend execution of the tool until an event specified in aEventList arrives.
     *
     * No parameters means waiting for any event.
     */
    TOOL_EVENT* Wait( const TOOL_EVENT_LIST& aEventList = TOOL_EVENT( TC_ANY, TA_ANY ) );

    /**
     * The functions below are not yet implemented - their interface may change
     */
    /*template <class Parameters, class ReturnValue>
        bool InvokeTool( const std::string& aToolName, const Parameters& parameters,
                         ReturnValue& returnValue );

    template <class Parameters, class ReturnValue>
        bool InvokeWindow( const std::string& aWindowName, const Parameters& parameters,
                           ReturnValue& returnValue );

    template <class T>
        void Yield( const T& returnValue );*/

protected:
    std::unique_ptr<TOOL_MENU> m_menu;

private:
    /**
     * This method is meant to be overridden in order to specify handlers for events.
     *
     * It is called every time tool is reset or finished.
     */
    virtual void setTransitions() = 0;

    /**
     * Clear the current transition map and restores the default one created by setTransitions().
     */
    void resetTransitions();

    void goInternal( TOOL_STATE_FUNC& aState, const TOOL_EVENT_LIST& aConditions );

    friend class TOOL_MANAGER;
};

// hide TOOL_MANAGER implementation
template <class T>
void TOOL_INTERACTIVE::Go( int (T::* aStateFunc)( const TOOL_EVENT& ),
                           const TOOL_EVENT_LIST& aConditions )
{
    TOOL_STATE_FUNC sptr = std::bind( aStateFunc, static_cast<T*>( this ), std::placeholders::_1 );

    goInternal( sptr, aConditions );
}

#endif
