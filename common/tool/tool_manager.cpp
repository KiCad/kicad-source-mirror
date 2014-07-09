/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include <map>
#include <deque>

#include <boost/foreach.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/map.hpp>

#include <wx/event.h>

#include <view/view.h>

#include <tool/tool_base.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/context_menu.h>
#include <tool/coroutine.h>
#include <tool/action_manager.h>

#include <wxPcbStruct.h>
#include <confirm.h>
#include <class_draw_panel_gal.h>

using boost::optional;

/// Struct describing the current execution state of a TOOL
struct TOOL_MANAGER::TOOL_STATE
{
    /// The tool itself
    TOOL_BASE* theTool;

    /// Is the tool active (pending execution) or disabled at the moment
    bool idle;

    /// Flag defining if the tool is waiting for any event (i.e. if it
    /// issued a Wait() call).
    bool pendingWait;

    /// Is there a context menu being displayed
    bool pendingContextMenu;

    /// Context menu currently used by the tool
    CONTEXT_MENU* contextMenu;

    /// Defines when the context menu is opened
    CONTEXT_MENU_TRIGGER contextMenuTrigger;

    /// Tool execution context
    COROUTINE<int, TOOL_EVENT&>* cofunc;

    /// The event that triggered the execution/wakeup of the tool after Wait() call
    TOOL_EVENT wakeupEvent;

    /// List of events the tool is currently waiting for
    TOOL_EVENT_LIST waitEvents;

    /// List of possible transitions (ie. association of events and state handlers that are executed
    /// upon the event reception
    std::vector<TRANSITION> transitions;

    bool operator==( const TOOL_MANAGER::TOOL_STATE& aRhs ) const
    {
        return aRhs.theTool == this->theTool;
    }

    bool operator!=( const TOOL_MANAGER::TOOL_STATE& aRhs ) const
    {
        return aRhs.theTool != this->theTool;
    }
};


TOOL_MANAGER::TOOL_MANAGER() :
    m_model( NULL ), m_view( NULL ), m_viewControls( NULL ), m_editFrame( NULL )
{
    m_actionMgr = new ACTION_MANAGER( this );

    // Register known actions
    std::list<TOOL_ACTION*>& actionList = GetActionList();
    BOOST_FOREACH( TOOL_ACTION* action, actionList )
        RegisterAction( action );
}


TOOL_MANAGER::~TOOL_MANAGER()
{
    boost::unordered_map<TOOL_BASE*, TOOL_STATE*>::iterator it, it_end;

    for( it = m_toolState.begin(), it_end = m_toolState.end(); it != it_end; ++it )
    {
        delete it->second->cofunc;  // delete cofunction
        delete it->second;          // delete TOOL_STATE
        delete it->first;           // delete the tool itself
    }

    m_toolState.clear();
    delete m_actionMgr;
}


void TOOL_MANAGER::RegisterTool( TOOL_BASE* aTool )
{
    wxASSERT_MSG( m_toolNameIndex.find( aTool->GetName() ) == m_toolNameIndex.end(),
            wxT( "Adding two tools with the same name may result in unexpected behaviour.") );
    wxASSERT_MSG( m_toolIdIndex.find( aTool->GetId() ) == m_toolIdIndex.end(),
            wxT( "Adding two tools with the same ID may result in unexpected behaviour.") );
    wxASSERT_MSG( m_toolTypes.find( typeid( *aTool ).name() ) == m_toolTypes.end(),
            wxT( "Adding two tools of the same type may result in unexpected behaviour.") );

    TOOL_STATE* st = new TOOL_STATE;

    st->theTool = aTool;
    st->pendingWait = false;
    st->pendingContextMenu = false;
    st->cofunc = NULL;
    st->contextMenuTrigger = CMENU_OFF;

    m_toolState[aTool] = st;
    m_toolNameIndex[aTool->GetName()] = st;
    m_toolIdIndex[aTool->GetId()] = st;
    m_toolTypes[typeid( *aTool ).name()] = st->theTool;

    aTool->m_toolMgr = this;

    if( !aTool->Init() )
    {
        std::string msg = StrPrintf( "Initialization of the %s tool failed",
                                     aTool->GetName().c_str() );

        DisplayError( NULL, wxString::FromUTF8( msg.c_str() ) );

        // Unregister the tool
        m_toolState.erase( aTool );
        m_toolNameIndex.erase( aTool->GetName() );
        m_toolIdIndex.erase( aTool->GetId() );
        m_toolTypes.erase( typeid( *aTool ).name() );

        delete st;
        delete aTool;
    }
}


bool TOOL_MANAGER::InvokeTool( TOOL_ID aToolId )
{
    TOOL_BASE* tool = FindTool( aToolId );

    if( tool && tool->GetType() == INTERACTIVE )
        return invokeTool( tool );

    return false;       // there is no tool with the given id
}


bool TOOL_MANAGER::InvokeTool( const std::string& aToolName )
{
    TOOL_BASE* tool = FindTool( aToolName );

    if( tool && tool->GetType() == INTERACTIVE )
        return invokeTool( tool );

    return false;       // there is no tool with the given name
}


void TOOL_MANAGER::RegisterAction( TOOL_ACTION* aAction )
{
    m_actionMgr->RegisterAction( aAction );
}


void TOOL_MANAGER::UnregisterAction( TOOL_ACTION* aAction )
{
    m_actionMgr->UnregisterAction( aAction );
}


bool TOOL_MANAGER::RunAction( const std::string& aActionName )
{
    return m_actionMgr->RunAction( aActionName );
}


void TOOL_MANAGER::RunAction( const TOOL_ACTION& aAction )
{
    m_actionMgr->RunAction( &aAction );
}


bool TOOL_MANAGER::invokeTool( TOOL_BASE* aTool )
{
    wxASSERT( aTool != NULL );

    TOOL_EVENT evt( TC_COMMAND, TA_ACTION, aTool->GetName() );
    ProcessEvent( evt );

    return true;
}


bool TOOL_MANAGER::runTool( TOOL_ID aToolId )
{
    TOOL_BASE* tool = FindTool( aToolId );

    if( tool && tool->GetType() == INTERACTIVE )
        return runTool( tool );

    return false;       // there is no tool with the given id
}


bool TOOL_MANAGER::runTool( const std::string& aToolName )
{
    TOOL_BASE* tool = FindTool( aToolName );

    if( tool && tool->GetType() == INTERACTIVE )
        return runTool( tool );

    return false;       // there is no tool with the given name
}


bool TOOL_MANAGER::runTool( TOOL_BASE* aTool )
{
    wxASSERT( aTool != NULL );

    if( !isRegistered( aTool ) )
    {
        wxASSERT_MSG( false, wxT( "You cannot run unregistered tools" ) );
        return false;
    }

    // If the tool is already active, bring it to the top of the active tools stack
    if( isActive( aTool ) )
    {
        m_activeTools.erase( std::find( m_activeTools.begin(), m_activeTools.end(),
                                        aTool->GetId() ) );
        m_activeTools.push_front( aTool->GetId() );

        return false;
    }

    aTool->Reset( TOOL_INTERACTIVE::RUN );

    // Add the tool on the front of the processing queue (it gets events first)
    m_activeTools.push_front( aTool->GetId() );

    return true;
}


TOOL_BASE* TOOL_MANAGER::FindTool( int aId ) const
{
    boost::unordered_map<TOOL_ID, TOOL_STATE*>::const_iterator it = m_toolIdIndex.find( aId );

    if( it != m_toolIdIndex.end() )
        return it->second->theTool;

    return NULL;
}


TOOL_BASE* TOOL_MANAGER::FindTool( const std::string& aName ) const
{
    boost::unordered_map<std::string, TOOL_STATE*>::const_iterator it = m_toolNameIndex.find( aName );

    if( it != m_toolNameIndex.end() )
        return it->second->theTool;

    return NULL;
}


void TOOL_MANAGER::ResetTools( TOOL_BASE::RESET_REASON aReason )
{
    BOOST_FOREACH( TOOL_BASE* tool, m_toolState | boost::adaptors::map_keys )
        tool->Reset( aReason );
}


int TOOL_MANAGER::GetPriority( int aToolId ) const
{
    int priority = 0;

    for( std::deque<int>::const_iterator it = m_activeTools.begin(),
            itEnd = m_activeTools.end(); it != itEnd; ++it )
    {
        if( *it == aToolId )
            return priority;

        ++priority;
    }

    return -1;
}


void TOOL_MANAGER::ScheduleNextState( TOOL_BASE* aTool, TOOL_STATE_FUNC& aHandler,
                                      const TOOL_EVENT_LIST& aConditions )
{
    TOOL_STATE* st = m_toolState[aTool];

    st->transitions.push_back( TRANSITION( aConditions, aHandler ) );
}


optional<TOOL_EVENT> TOOL_MANAGER::ScheduleWait( TOOL_BASE* aTool,
                                                 const TOOL_EVENT_LIST& aConditions )
{
    TOOL_STATE* st = m_toolState[aTool];

    // indicate to the manager that we are going to sleep and we shall be
    // woken up when an event matching aConditions arrive
    st->pendingWait = true;
    st->waitEvents = aConditions;

    // switch context back to event dispatcher loop
    st->cofunc->Yield();

    return st->wakeupEvent;
}


void TOOL_MANAGER::dispatchInternal( TOOL_EVENT& aEvent )
{
    // iterate over all registered tools
    BOOST_FOREACH( TOOL_ID toolId, m_activeTools )
    {
        TOOL_STATE* st = m_toolIdIndex[toolId];

        // the tool state handler is waiting for events (i.e. called Wait() method)
        if( st->pendingWait )
        {
            if( st->waitEvents.Matches( aEvent ) )
            {
                // By default, already processed events are not passed further
                m_passEvent = false;

                // got matching event? clear wait list and wake up the coroutine
                st->wakeupEvent = aEvent;
                st->pendingWait = false;
                st->waitEvents.clear();

                if( st->cofunc && !st->cofunc->Resume() )
                    finishTool( st ); // The couroutine has finished

                // If the tool did not request to propagate
                // the event to other tools, we should stop it now
                if( !m_passEvent )
                    break;
            }
        }
    }

    BOOST_FOREACH( TOOL_STATE* st, m_toolState | boost::adaptors::map_values )
    {
        // the tool scheduled next state(s) by calling Go()
        if( !st->pendingWait )
        {
            // no state handler in progress - check if there are any transitions (defined by
            // Go() method that match the event.
            if( st->transitions.size() )
            {
                BOOST_FOREACH( TRANSITION& tr, st->transitions )
                {
                    if( tr.first.Matches( aEvent ) )
                    {
                        // as the state changes, the transition table has to be set up again
                        st->transitions.clear();

                        // no tool context allocated yet? Create one.
                        if( !st->cofunc )
                            st->cofunc = new COROUTINE<int, TOOL_EVENT&>( tr.second );
                        else
                            st->cofunc->SetEntry( tr.second );

                        // got match? Run the handler.
                        st->cofunc->Call( aEvent );

                        if( !st->cofunc->Running() )
                            finishTool( st ); // The couroutine has finished immediately?

                        // there is no point in further checking, as transitions got cleared
                        break;
                    }
                }
            }
        }
    }
}


bool TOOL_MANAGER::dispatchStandardEvents( TOOL_EVENT& aEvent )
{
    if( aEvent.Action() == TA_KEY_PRESSED )
    {
        // Check if there is a hotkey associated
        if( m_actionMgr->RunHotKey( aEvent.Modifier() | aEvent.KeyCode() ) )
            return false;                       // hotkey event was handled so it does not go any further
    }
    else if( aEvent.Category() == TC_COMMAND )  // it may be a tool activation event
    {
        dispatchActivation( aEvent );
        // do not return false, as the event has to go on to the destined tool
    }

    return true;
}


bool TOOL_MANAGER::dispatchActivation( TOOL_EVENT& aEvent )
{
    // Look for the tool that has the same name as parameter in the processed command TOOL_EVENT
    BOOST_FOREACH( TOOL_STATE* st, m_toolState | boost::adaptors::map_values )
    {
        if( st->theTool->GetName() == aEvent.m_commandStr )
        {
            runTool( st->theTool );
            return true;
        }
    }

    return false;
}


void TOOL_MANAGER::finishTool( TOOL_STATE* aState )
{
    std::deque<TOOL_ID>::iterator it, itEnd;

    // Find the tool and deactivate it
    for( it = m_activeTools.begin(), itEnd = m_activeTools.end(); it != itEnd; ++it )
    {
        if( aState == m_toolIdIndex[*it] )
        {
            m_activeTools.erase( it );
            break;
        }
    }

    delete aState->cofunc;
    aState->cofunc = NULL;
}


bool TOOL_MANAGER::ProcessEvent( TOOL_EVENT& aEvent )
{
// wxLogDebug( "event: %s", aEvent.Format().c_str() );

    // Early dispatch of events destined for the TOOL_MANAGER
    if( !dispatchStandardEvents( aEvent ) )
        return false;

    dispatchInternal( aEvent );

    // popup menu handling
    BOOST_FOREACH( TOOL_ID toolId, m_activeTools )
    {
        TOOL_STATE* st = m_toolIdIndex[toolId];

        // the tool requested a context menu. The menu is activated on RMB click (CMENU_BUTTON mode)
        // or immediately (CMENU_NOW) mode. The latter is used for clarification lists.
        if( st->contextMenuTrigger != CMENU_OFF )
        {
            if( st->contextMenuTrigger == CMENU_BUTTON && !aEvent.IsClick( BUT_RIGHT ) )
                break;

            st->pendingWait = true;
            st->waitEvents = TOOL_EVENT( TC_ANY, TA_ANY );

            if( st->contextMenuTrigger == CMENU_NOW )
                st->contextMenuTrigger = CMENU_OFF;

            boost::scoped_ptr<CONTEXT_MENU> menu( new CONTEXT_MENU( *st->contextMenu ) );
            GetEditFrame()->PopupMenu( menu.get() );

            // If nothing was chosen from the context menu, we must notify the tool as well
            if( menu->GetSelected() < 0 )
            {
                TOOL_EVENT evt( TC_COMMAND, TA_CONTEXT_MENU_CHOICE, -1 );
                dispatchInternal( evt );
            }

            break;
        }
    }

    if( m_view->IsDirty() )
    {
        PCB_EDIT_FRAME* f = static_cast<PCB_EDIT_FRAME*>( GetEditFrame() );
        f->GetGalCanvas()->Refresh();    // fixme: ugly hack, provide a method in TOOL_DISPATCHER.
    }

    return false;
}


void TOOL_MANAGER::ScheduleContextMenu( TOOL_BASE* aTool, CONTEXT_MENU* aMenu,
                                        CONTEXT_MENU_TRIGGER aTrigger )
{
    TOOL_STATE* st = m_toolState[aTool];

    st->contextMenu = aMenu;
    st->contextMenuTrigger = aTrigger;
}


TOOL_ID TOOL_MANAGER::MakeToolId( const std::string& aToolName )
{
    static int currentId;

    return currentId++;
}


void TOOL_MANAGER::SetEnvironment( EDA_ITEM* aModel, KIGFX::VIEW* aView,
                                   KIGFX::VIEW_CONTROLS* aViewControls, wxWindow* aFrame )
{
    m_model = aModel;
    m_view = aView;
    m_viewControls = aViewControls;
    m_editFrame = aFrame;
}


bool TOOL_MANAGER::isActive( TOOL_BASE* aTool )
{
    if( !isRegistered( aTool ) )
        return false;

    // Just check if the tool is on the active tools stack
    return std::find( m_activeTools.begin(), m_activeTools.end(), aTool->GetId() ) != m_activeTools.end();
}
