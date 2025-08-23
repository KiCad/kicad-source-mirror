/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <core/kicad_algo.h>
#include <optional>
#include <map>
#include <stack>
#include <trace_helpers.h>
#include <kiplatform/ui.h>
#include <app_monitor.h>

#include <wx/event.h>
#include <wx/clipbrd.h>
#include <wx/app.h>

#include <math/vector2wx.h>

#include <view/view.h>
#include <view/view_controls.h>
#include <eda_base_frame.h>
#include <tool/tool_base.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/action_menu.h>
#include <tool/coroutine.h>
#include <tool/action_manager.h>

#include <class_draw_panel_gal.h>

/// Struct describing the current execution state of a TOOL
struct TOOL_MANAGER::TOOL_STATE
{
    TOOL_STATE( TOOL_BASE* aTool ) :
        theTool( aTool )
    {
        clear();
    }

    TOOL_STATE( const TOOL_STATE& aState )
    {
        theTool            = aState.theTool;
        idle               = aState.idle;
        shutdown           = aState.shutdown;
        pendingWait        = aState.pendingWait;
        pendingContextMenu = aState.pendingContextMenu;
        contextMenu        = aState.contextMenu;
        contextMenuTrigger = aState.contextMenuTrigger;
        cofunc             = aState.cofunc;
        initialEvent       = aState.initialEvent;
        wakeupEvent        = aState.wakeupEvent;
        waitEvents         = aState.waitEvents;
        transitions        = aState.transitions;
        vcSettings         = aState.vcSettings;
        // do not copy stateStack
    }

    ~TOOL_STATE()
    {
        wxASSERT_MSG( stateStack.empty(), wxT( "StateStack not empty!" ) );
    }

    /// The tool itself
    TOOL_BASE* theTool;

    /// Is the tool active (pending execution) or disabled at the moment
    bool idle;

    /// Should the tool shutdown during next execution
    bool shutdown;

    /// Flag defining if the tool is waiting for any event (i.e. if it
    /// issued a Wait() call).
    bool pendingWait;

    /// Is there a context menu being displayed
    bool pendingContextMenu;

    /// Context menu currently used by the tool
    ACTION_MENU* contextMenu;

    /// Defines when the context menu is opened
    CONTEXT_MENU_TRIGGER contextMenuTrigger;

    /// Tool execution context
    COROUTINE<int, const TOOL_EVENT&>* cofunc;

    /// The first event that triggered activation of the tool.
    TOOL_EVENT initialEvent;

    /// The event that triggered the execution/wakeup of the tool after Wait() call
    TOOL_EVENT wakeupEvent;

    /// List of events the tool is currently waiting for
    TOOL_EVENT_LIST waitEvents;

    /// List of possible transitions (ie. association of events and state handlers that are executed
    /// upon the event reception
    std::vector<TRANSITION> transitions;

    /// VIEW_CONTROLS settings to preserve settings when the tools are switched
    KIGFX::VC_SETTINGS vcSettings;

    TOOL_STATE& operator=( const TOOL_STATE& aState )
    {
        theTool            = aState.theTool;
        idle               = aState.idle;
        shutdown           = aState.shutdown;
        pendingWait        = aState.pendingWait;
        pendingContextMenu = aState.pendingContextMenu;
        contextMenu        = aState.contextMenu;
        contextMenuTrigger = aState.contextMenuTrigger;
        cofunc             = aState.cofunc;
        initialEvent       = aState.initialEvent;
        wakeupEvent        = aState.wakeupEvent;
        waitEvents         = aState.waitEvents;
        transitions        = aState.transitions;
        vcSettings         = aState.vcSettings;

        // do not copy stateStack
        return *this;
    }

    bool operator==( const TOOL_MANAGER::TOOL_STATE& aRhs ) const
    {
        return aRhs.theTool == theTool;
    }

    bool operator!=( const TOOL_MANAGER::TOOL_STATE& aRhs ) const
    {
        return aRhs.theTool != theTool;
    }

    /**
     * Store the current state of the tool on stack. Stacks are stored internally and are not
     * shared between different TOOL_STATE objects.
     */
    void Push()
    {
        auto state = std::make_unique<TOOL_STATE>( *this );
        stateStack.push( std::move( state ) );
        clear();
    }

    /**
     * Restore state of the tool from stack. Stacks are stored internally and are not
     * shared between different TOOL_STATE objects.
     *
     * @return True if state was restored, false if the stack was empty.
     */
    bool Pop()
    {
        delete cofunc;

        if( !stateStack.empty() )
        {
            *this = *stateStack.top().get();
            stateStack.pop();
            return true;
        }
        else
        {
            cofunc = nullptr;
            return false;
        }
    }

private:
    /// Stack preserving previous states of a TOOL.
    std::stack<std::unique_ptr<TOOL_STATE>> stateStack;

    /// Restores the initial state.
    void clear()
    {
        idle               = true;
        shutdown           = false;
        pendingWait        = false;
        pendingContextMenu = false;
        cofunc             = nullptr;
        contextMenu        = nullptr;
        contextMenuTrigger = CMENU_OFF;
        vcSettings.Reset();
        transitions.clear();
    }
};


TOOL_MANAGER::TOOL_MANAGER() :
        m_model( nullptr ),
        m_view( nullptr ),
        m_viewControls( nullptr ),
        m_frame( nullptr ),
        m_settings( nullptr ),
        m_warpMouseAfterContextMenu( true ),
        m_menuActive( false ),
        m_menuOwner( -1 ),
        m_activeState( nullptr ),
        m_shuttingDown( false )
{
    m_actionMgr = new ACTION_MANAGER( this );
}


TOOL_MANAGER::~TOOL_MANAGER()
{
    std::map<TOOL_BASE*, TOOL_STATE*>::iterator it, it_end;

    for( it = m_toolState.begin(), it_end = m_toolState.end(); it != it_end; ++it )
    {
        delete it->second->cofunc;  // delete cofunction
        delete it->second;          // delete TOOL_STATE
        delete it->first;           // delete the tool itself
    }

    delete m_actionMgr;
}


void TOOL_MANAGER::RegisterTool( TOOL_BASE* aTool )
{
    wxASSERT_MSG( m_toolNameIndex.find( aTool->GetName() ) == m_toolNameIndex.end(),
                  wxT( "Adding two tools with the same name may result in unexpected behavior.") );
    wxASSERT_MSG( m_toolIdIndex.find( aTool->GetId() ) == m_toolIdIndex.end(),
                  wxT( "Adding two tools with the same ID may result in unexpected behavior.") );
    wxASSERT_MSG( m_toolTypes.find( typeid( *aTool ).name() ) == m_toolTypes.end(),
                  wxT( "Adding two tools of the same type may result in unexpected behavior.") );

    wxLogTrace( kicadTraceToolStack,
                wxS( "TOOL_MANAGER::RegisterTool: Registering tool %s with ID %d" ),
                aTool->GetName(), aTool->GetId() );

    m_toolOrder.push_back( aTool );

    TOOL_STATE* st = new TOOL_STATE( aTool );

    m_toolState[aTool] = st;
    m_toolNameIndex[aTool->GetName()] = st;
    m_toolIdIndex[aTool->GetId()] = st;
    m_toolTypes[typeid( *aTool ).name()] = st->theTool;

    aTool->attachManager( this );
}


bool TOOL_MANAGER::InvokeTool( TOOL_ID aToolId )
{
    TOOL_BASE* tool = FindTool( aToolId );

    if( tool && tool->GetType() == INTERACTIVE )
        return invokeTool( tool );

    wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::InvokeTool - no tool with ID %d" ),
                                     aToolId );

    return false;       // there is no tool with the given id
}


bool TOOL_MANAGER::InvokeTool( const std::string& aToolName )
{
    TOOL_BASE* tool = FindTool( aToolName );

    if( tool && tool->GetType() == INTERACTIVE )
        return invokeTool( tool );

    wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::InvokeTool - no tool with name %s" ),
                aToolName );

    return false;       // there is no tool with the given name
}


bool TOOL_MANAGER::doRunAction( const std::string& aActionName, bool aNow, const ki::any& aParam,
                                COMMIT* aCommit )
{
    TOOL_ACTION* action = m_actionMgr->FindAction( aActionName );

    if( !action )
    {
        wxASSERT_MSG( false, wxString::Format( "Could not find action %s.", aActionName ) );
        return false;
    }

    doRunAction( *action, aNow, aParam, aCommit );

    return true;
}


VECTOR2D TOOL_MANAGER::GetMousePosition() const
{
    if( m_viewControls )
        return m_viewControls->GetMousePosition();
    else
        return ToVECTOR2D( KIPLATFORM::UI::GetMousePosition() );
}


VECTOR2D TOOL_MANAGER::GetCursorPosition() const
{
   if( m_viewControls )
       return m_viewControls->GetCursorPosition();
   else
       return ToVECTOR2D( KIPLATFORM::UI::GetMousePosition() );
}


bool TOOL_MANAGER::doRunAction( const TOOL_ACTION& aAction, bool aNow, const ki::any& aParam,
                                COMMIT* aCommit, bool aFromAPI )
{
    if( m_shuttingDown )
        return true;

    bool       retVal = false;
    TOOL_EVENT event = aAction.MakeEvent();

    if( event.Category() == TC_COMMAND )
        event.SetMousePosition( GetCursorPosition() );

    // Allow to override the action parameter
    if( aParam.has_value() )
        event.SetParameter( aParam );

    if( aNow )
    {
        TOOL_STATE* current = m_activeState;

        // An event with a commit must be run synchronously
        if( aCommit )
        {
            // We initialize the SYNCHRONOUS state to finished so that tools that don't have an
            // event loop won't hang if someone forgets to set the state.
            std::atomic<SYNCRONOUS_TOOL_STATE> synchronousControl = STS_FINISHED;

            event.SetSynchronous( &synchronousControl );
            event.SetCommit( aCommit );

            processEvent( event );

            while( synchronousControl == STS_RUNNING )
            {
                wxYield();          // Needed to honor mouse (and other) events during editing
                wxMilliSleep( 1 );  // Needed to avoid 100% use of one cpu core.
                                    // The sleeping time must be must be small to avoid
                                    // noticeable lag in mouse and editing events
                                    // (1 to 5 ms is a good value)
            }

            retVal = synchronousControl != STS_CANCELLED;
        }
        else
        {
            retVal = processEvent( event );
        }

        setActiveState( current );
        UpdateUI( event );
    }
    else
    {
        // It is really dangerous to pass a commit (whose lifetime we can't guarantee) to
        // deferred event processing.  There is a possibility that user actions will get run
        // in between, which might either affect the lifetime of the commit or push or pop
        // other commits.  However, we don't currently have a better solution for the API.
        if( aCommit )
        {
            wxASSERT_MSG( aFromAPI, wxT( "Deferred actions have no way of guaranteeing the "
                                         "lifetime of the COMMIT object" ) );
            event.SetCommit( aCommit );
        }

        PostEvent( event );
    }

    return retVal;
}


void TOOL_MANAGER::CancelTool()
{
    TOOL_EVENT evt( TC_COMMAND, TA_CANCEL_TOOL );

    processEvent( evt );
}


void TOOL_MANAGER::PrimeTool( const VECTOR2D& aPosition )
{
    int modifiers = 0;

    /*
     * Don't include any modifiers.  They're part of the hotkey, not part of the resulting
     * click.
     *
     * modifiers |= wxGetKeyState( WXK_SHIFT ) ? MD_SHIFT : 0;
     * modifiers |= wxGetKeyState( WXK_CONTROL ) ? MD_CTRL : 0;
     * modifiers |= wxGetKeyState( WXK_ALT ) ? MD_ALT : 0;
     */

    TOOL_EVENT evt( TC_MOUSE, TA_PRIME, BUT_LEFT | modifiers );
    evt.SetMousePosition( aPosition );

    PostEvent( evt );
}


void TOOL_MANAGER::PostEvent( const TOOL_EVENT& aEvent )
{
    // Horrific hack, but it's a crash bug.  Don't let inter-frame commands stack up
    // waiting to be processed.
    if( aEvent.IsSimulator() && m_eventQueue.size() > 0 && m_eventQueue.back().IsSimulator() )
        m_eventQueue.pop_back();

    m_eventQueue.push_back( aEvent );
}


int TOOL_MANAGER::GetHotKey( const TOOL_ACTION& aAction ) const
{
    return m_actionMgr->GetHotKey( aAction );
}


bool TOOL_MANAGER::invokeTool( TOOL_BASE* aTool )
{
    wxASSERT( aTool != nullptr );

    TOOL_EVENT evt( TC_COMMAND, TA_ACTIVATE, aTool->GetName() );
    evt.SetMousePosition( GetCursorPosition() );
    processEvent( evt );

    if( TOOL_STATE* active = GetCurrentToolState() )
        setActiveState( active );

    return true;
}


bool TOOL_MANAGER::runTool( TOOL_BASE* aTool )
{
    wxASSERT( aTool != nullptr );

    wxString msg = wxString::Format( wxS( "TOOL_MANAGER::runTool - running tool %s" ), aTool->GetName() );
    APP_MONITOR::AddTransactionBreadcrumb( msg, "tool.run" );

    if( !isRegistered( aTool ) )
    {
        wxASSERT_MSG( false, wxT( "You cannot run unregistered tools" ) );
        return false;
    }

    TOOL_ID id = aTool->GetId();

    wxLogTrace( kicadTraceToolStack, msg,
                aTool->GetName() );

    if( aTool->GetType() == INTERACTIVE )
        static_cast<TOOL_INTERACTIVE*>( aTool )->resetTransitions();

    // If the tool is already active, bring it to the top of the active tools stack
    if( isActive( aTool ) && m_activeTools.size() > 1 )
    {
        auto it = std::find( m_activeTools.begin(), m_activeTools.end(), id );

        if( it != m_activeTools.end() )
        {
            if( it != m_activeTools.begin() )
            {
                m_activeTools.erase( it );
                m_activeTools.push_front( id );
            }

            return false;
        }
    }

    setActiveState( m_toolIdIndex[id] );
    aTool->Reset( TOOL_INTERACTIVE::RUN );

    // Add the tool on the front of the processing queue (it gets events first)
    m_activeTools.push_front( id );

    return true;
}


void TOOL_MANAGER::ShutdownAllTools()
{
    m_shuttingDown = true;

    // Create a temporary list of tools to iterate over since when the tools shutdown
    // they remove themselves from the list automatically (invalidating the iterator)
    ID_LIST tmpList = m_activeTools;

    // Make sure each tool knows that it is shutting down, so that loops get shut down
    // at the dispatcher
    for( auto id : tmpList )
    {
        if( m_toolIdIndex.count( id ) == 0 )
            continue;

        m_toolIdIndex[id]->shutdown = true;
    }

    for( auto id : tmpList )
    {
        ShutdownTool( id );
    }
}


void TOOL_MANAGER::ShutdownTool( TOOL_ID aToolId )
{
    TOOL_BASE* tool = FindTool( aToolId );

    if( tool && tool->GetType() == INTERACTIVE )
        ShutdownTool( tool );

    wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::ShutdownTool - no tool with ID %d" ),
                aToolId );
}


void TOOL_MANAGER::ShutdownTool( const std::string& aToolName )
{
    TOOL_BASE* tool = FindTool( aToolName );

    if( tool && tool->GetType() == INTERACTIVE )
        ShutdownTool( tool );

    wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::ShutdownTool - no tool with name %s" ),
                aToolName );
}


void TOOL_MANAGER::ShutdownTool( TOOL_BASE* aTool )
{
    wxASSERT( aTool != nullptr );

    TOOL_ID id = aTool->GetId();

    if( isActive( aTool ) )
    {
        TOOL_MANAGER::ID_LIST::iterator it = std::find( m_activeTools.begin(),
                                                        m_activeTools.end(), id );

        TOOL_STATE* st = m_toolIdIndex[*it];

        // the tool state handler is waiting for events (i.e. called Wait() method)
        if( st && st->pendingWait )
        {
            // Wake up the tool and tell it to shutdown
            st->shutdown = true;
            st->pendingWait = false;
            st->waitEvents.clear();

            if( st->cofunc )
            {
                wxLogTrace( kicadTraceToolStack,
                            wxS( "TOOL_MANAGER::ShutdownTool - Shutting down tool %s" ),
                            st->theTool->GetName() );

                setActiveState( st );
                bool end = !st->cofunc->Resume();

                if( end )
                    finishTool( st );
            }
        }
    }
}


TOOL_BASE* TOOL_MANAGER::FindTool( int aId ) const
{
    std::map<TOOL_ID, TOOL_STATE*>::const_iterator it = m_toolIdIndex.find( aId );

    if( it != m_toolIdIndex.end() )
        return it->second->theTool;

    return nullptr;
}


TOOL_BASE* TOOL_MANAGER::FindTool( const std::string& aName ) const
{
    std::map<std::string, TOOL_STATE*>::const_iterator it = m_toolNameIndex.find( aName );

    if( it != m_toolNameIndex.end() )
        return it->second->theTool;

    return nullptr;
}


void TOOL_MANAGER::DeactivateTool()
{
    // Deactivate the active tool, but do not run anything new
    TOOL_EVENT evt( TC_COMMAND, TA_CANCEL_TOOL );
    processEvent( evt );
}


void TOOL_MANAGER::ResetTools( TOOL_BASE::RESET_REASON aReason )
{
    if( aReason != TOOL_BASE::REDRAW )
        DeactivateTool();

    for( auto& state : m_toolState )
    {
        TOOL_BASE* tool = state.first;

        wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::ResetTools: Resetting tool '%s'" ),
                                             tool->GetName() );

        setActiveState( state.second );
        tool->Reset( aReason );

        if( tool->GetType() == INTERACTIVE )
            static_cast<TOOL_INTERACTIVE*>( tool )->resetTransitions();
    }
}


void TOOL_MANAGER::InitTools()
{
    for( auto it = m_toolOrder.begin(); it != m_toolOrder.end(); /* iter inside */ )
    {
        TOOL_BASE* tool = *it;
        wxASSERT( m_toolState.count( tool ) );
        TOOL_STATE* state = m_toolState[tool];
        setActiveState( state );
        ++it;   // keep the iterator valid if the element is going to be erased

        if( !tool->Init() )
        {
            wxLogTrace( kicadTraceToolStack,
                        wxS( "TOOL_MANAGER initialization of tool '%s' failed" ),
                        tool->GetName() );

            // Unregister the tool
            setActiveState( nullptr );
            m_toolState.erase( tool );
            m_toolNameIndex.erase( tool->GetName() );
            m_toolIdIndex.erase( tool->GetId() );
            m_toolTypes.erase( typeid( *tool ).name() );

            delete state;
            delete tool;
        }
    }

    m_actionMgr->UpdateHotKeys( true );

    ResetTools( TOOL_BASE::RUN );
}


int TOOL_MANAGER::GetPriority( int aToolId ) const
{
    int priority = 0;

    for( TOOL_ID tool : m_activeTools )
    {
        if( tool == aToolId )
            return priority;

        ++priority;
    }

    return -1;
}


void TOOL_MANAGER::ScheduleNextState( TOOL_BASE* aTool, TOOL_STATE_FUNC& aHandler,
                                      const TOOL_EVENT_LIST& aConditions )
{
    TOOL_STATE* st = m_toolState[aTool];

    st->transitions.emplace_back( TRANSITION( aConditions, aHandler ) );
}


void TOOL_MANAGER::ClearTransitions( TOOL_BASE* aTool )
{
    m_toolState[aTool]->transitions.clear();
}


void TOOL_MANAGER::RunMainStack( TOOL_BASE* aTool, std::function<void()> aFunc )
{
    TOOL_STATE* st = m_toolState[aTool];
    setActiveState( st );
    wxCHECK( st->cofunc, /* void */ );
    st->cofunc->RunMainStack( std::move( aFunc ) );
}


TOOL_EVENT* TOOL_MANAGER::ScheduleWait( TOOL_BASE* aTool, const TOOL_EVENT_LIST& aConditions )
{
    TOOL_STATE* st = m_toolState[aTool];

    wxCHECK( !st->pendingWait, nullptr ); // everything collapses on two KiYield() in a row

    // indicate to the manager that we are going to sleep and we shall be
    // woken up when an event matching aConditions arrive
    st->pendingWait = true;
    st->waitEvents = aConditions;

    wxCHECK( st->cofunc, nullptr );

    // switch context back to event dispatcher loop
    st->cofunc->KiYield();

    // If the tool should shutdown, it gets a null event to break the loop
    if( st->shutdown )
        return nullptr;
    else
        return &st->wakeupEvent;
}


bool TOOL_MANAGER::dispatchInternal( TOOL_EVENT& aEvent )
{
    bool handled = false;

    wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::dispatchInternal - received event: %s" ),
                aEvent.Format() );

    auto it = m_activeTools.begin();

    // iterate over active tool stack
    while( it != m_activeTools.end() )
    {
        TOOL_STATE* st = m_toolIdIndex[*it];
        bool increment = true;

        // forward context menu events to the tool that created the menu
        if( aEvent.IsChoiceMenu() )
        {
            if( *it != m_menuOwner )
            {
                ++it;
                continue;
            }
        }

        // If we're pendingWait then we had better have a cofunc to process the wait.
        wxASSERT( !st || !st->pendingWait || st->cofunc );

        // the tool state handler is waiting for events (i.e. called Wait() method)
        if( st && st->cofunc && st->pendingWait && st->waitEvents.Matches( aEvent ) )
        {
            if( !aEvent.FirstResponder() )
                aEvent.SetFirstResponder( st->theTool );

            // got matching event? clear wait list and wake up the coroutine
            st->wakeupEvent = aEvent;
            st->pendingWait = false;
            st->waitEvents.clear();

            wxLogTrace( kicadTraceToolStack,
                        wxS( "TOOL_MANAGER::dispatchInternal - Waking tool %s for event: %s" ),
                        st->theTool->GetName(), aEvent.Format() );

            setActiveState( st );
            bool end = !st->cofunc->Resume();

            if( end )
            {
                it = finishTool( st );
                increment = false;
            }

            // If the tool did not request the event be passed to other tools, we're done
            if( !st->wakeupEvent.PassEvent() )
            {
                wxLogTrace( kicadTraceToolStack,
                            wxS( "TOOL_MANAGER::dispatchInternal - tool %s stopped passing "
                                 "event: %s" ),
                            st->theTool->GetName(), aEvent.Format() );

                return true;
            }
        }

        if( increment )
            ++it;
    }

    for( const auto& state : m_toolState )
    {
        TOOL_STATE* st = state.second;
        bool finished = false;

        // no state handler in progress - check if there are any transitions (defined by
        // Go() method that match the event.
        if( !st->transitions.empty() )
        {
            for( const TRANSITION& tr : st->transitions )
            {
                if( tr.first.Matches( aEvent ) )
                {
                    auto func_copy = tr.second;

                    if( !aEvent.FirstResponder() )
                        aEvent.SetFirstResponder( st->theTool );

                    // if there is already a context, then push it on the stack
                    // and transfer the previous view control settings to the new context
                    if( st->cofunc )
                    {
                        KIGFX::VC_SETTINGS viewControlSettings = st->vcSettings;
                        st->Push();
                        st->vcSettings = std::move( viewControlSettings );
                    }

                    st->cofunc = new COROUTINE<int, const TOOL_EVENT&>( std::move( func_copy ) );

                    wxLogTrace( kicadTraceToolStack,
                                wxS( "TOOL_MANAGER::dispatchInternal - Running tool %s for "
                                     "event: %s" ),
                                st->theTool->GetName(), aEvent.Format() );

                    // got match? Run the handler.
                    setActiveState( st );
                    st->idle = false;
                    st->initialEvent = aEvent;
                    st->cofunc->Call( st->initialEvent );
                    handled = true;

                    if( !st->cofunc->Running() )
                        finishTool( st ); // The coroutine has finished immediately?

                    // if it is a message, continue processing
                    finished = !( aEvent.Category() == TC_MESSAGE );

                    // there is no point in further checking, as transitions got cleared
                    break;
                }
            }
        }

        if( finished )
            break;      // only the first tool gets the event
    }

    wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::dispatchInternal - %s handle event: %s" ),
                ( handled ? wxS( "Did" ) : wxS( "Did not" ) ), aEvent.Format() );

    return handled;
}


bool TOOL_MANAGER::DispatchHotKey( const TOOL_EVENT& aEvent )
{
    if( aEvent.Action() == TA_KEY_PRESSED )
        return m_actionMgr->RunHotKey( aEvent.Modifier() | aEvent.KeyCode() );

    return false;
}


bool TOOL_MANAGER::dispatchActivation( const TOOL_EVENT& aEvent )
{
    wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::dispatchActivation - Received event: %s" ),
                aEvent.Format() );

    if( aEvent.IsActivate() )
    {
        auto tool = m_toolNameIndex.find( aEvent.getCommandStr() );

        if( tool != m_toolNameIndex.end() )
        {
            wxLogTrace( kicadTraceToolStack,
                        wxS( "TOOL_MANAGER::dispatchActivation - Running tool %s for event: %s" ),
                        tool->second->theTool->GetName(), aEvent.Format() );

            runTool( tool->second->theTool );
            return true;
        }
    }

    return false;
}


void TOOL_MANAGER::DispatchContextMenu( const TOOL_EVENT& aEvent )
{
    for( TOOL_ID toolId : m_activeTools )
    {
        TOOL_STATE* st = m_toolIdIndex[toolId];

        // the tool requested a context menu. The menu is activated on RMB click (CMENU_BUTTON mode)
        // or immediately (CMENU_NOW) mode. The latter is used for clarification lists.
        if( st->contextMenuTrigger == CMENU_OFF )
            continue;

        if( st->contextMenuTrigger == CMENU_BUTTON && !aEvent.IsClick( BUT_RIGHT ) )
            break;

        if( st->cofunc )
        {
            st->pendingWait = true;
            st->waitEvents = TOOL_EVENT( TC_ANY, TA_ANY );
        }

        // Store the menu pointer in case it is changed by the TOOL when handling menu events
        ACTION_MENU* m = st->contextMenu;

        if( st->contextMenuTrigger == CMENU_NOW )
            st->contextMenuTrigger = CMENU_OFF;

        // Store the cursor position, so the tools could execute actions
        // using the point where the user has invoked a context menu
        if( m_viewControls )
            m_menuCursor = m_viewControls->GetCursorPosition();

        // Save all tools cursor settings, as they will be overridden
        for( const std::pair<const TOOL_ID, TOOL_STATE*>& idState : m_toolIdIndex )
        {
            TOOL_STATE* s = idState.second;
            const auto& vc = s->vcSettings;

            if( vc.m_forceCursorPosition )
                m_cursorSettings[idState.first] = vc.m_forcedPosition;
            else
                m_cursorSettings[idState.first] = std::nullopt;
        }

        if( m_viewControls )
            m_viewControls->ForceCursorPosition( true, m_menuCursor );

        // Display a copy of menu
        std::unique_ptr<ACTION_MENU> menu( m->Clone() );

        m_menuOwner = toolId;
        m_menuActive = true;

        if( wxWindow* frame = dynamic_cast<wxWindow*>( m_frame ) )
            frame->PopupMenu( menu.get() );

        // Warp the cursor if a menu item was selected
        if( menu->GetSelected() >= 0 )
        {
            if( m_viewControls && m_warpMouseAfterContextMenu )
                m_viewControls->WarpMouseCursor( m_menuCursor, true, false );
        }
        // Otherwise notify the tool of a canceled menu
        else
        {
            TOOL_EVENT evt( TC_COMMAND, TA_CHOICE_MENU_CHOICE, -1 );
            evt.SetHasPosition( false );
            evt.SetParameter( m );
            dispatchInternal( evt );
        }

        // Restore setting in case it was vetoed
        m_warpMouseAfterContextMenu = true;

        // Notify the tools that menu has been closed
        TOOL_EVENT evt( TC_COMMAND, TA_CHOICE_MENU_CLOSED );
        evt.SetHasPosition( false );
        evt.SetParameter( m );
        dispatchInternal( evt );

        m_menuActive = false;
        m_menuOwner = -1;

        // Restore cursor settings
        for( const std::pair<const TOOL_ID,
             std::optional<VECTOR2D>>& cursorSetting : m_cursorSettings )
        {
            auto it = m_toolIdIndex.find( cursorSetting.first );
            wxASSERT( it != m_toolIdIndex.end() );

            if( it == m_toolIdIndex.end() )
                continue;

            KIGFX::VC_SETTINGS& vc = it->second->vcSettings;
            vc.m_forceCursorPosition = (bool) cursorSetting.second;
            vc.m_forcedPosition = cursorSetting.second ? *cursorSetting.second : VECTOR2D( 0, 0 );
        }

        m_cursorSettings.clear();
        break;
    }
}


void TOOL_MANAGER::WarpAfterContextMenu()
{
    if( m_viewControls && m_warpMouseAfterContextMenu )
        m_viewControls->WarpMouseCursor( m_menuCursor, true, false );

    // Don't warp again when the menu is closed
    m_warpMouseAfterContextMenu = false;
}


TOOL_MANAGER::ID_LIST::iterator TOOL_MANAGER::finishTool( TOOL_STATE* aState )
{
    auto it = std::find( m_activeTools.begin(), m_activeTools.end(), aState->theTool->GetId() );

    if( !aState->Pop() )
    {
        // Deactivate the tool if there are no other contexts saved on the stack
        if( it != m_activeTools.end() )
            it = m_activeTools.erase( it );

        aState->idle = true;
    }

    if( aState == m_activeState )
        setActiveState( nullptr );

    return it;
}


bool TOOL_MANAGER::ProcessEvent( const TOOL_EVENT& aEvent )
{
    // Once the tool manager is shutting down, don't start
    // activating more tools
    if( m_shuttingDown )
        return true;

    bool handled = processEvent( aEvent );

    TOOL_STATE* activeTool = GetCurrentToolState();

    if( activeTool )
        setActiveState( activeTool );

    if( m_view && m_view->IsDirty() )
    {
#if defined( __WXMAC__ )
        wxTheApp->ProcessPendingEvents(); // required for updating brightening behind a popup menu
#endif
    }

    UpdateUI( aEvent );

    return handled;
}


void TOOL_MANAGER::ScheduleContextMenu( TOOL_BASE* aTool, ACTION_MENU* aMenu,
                                        CONTEXT_MENU_TRIGGER aTrigger )
{
    TOOL_STATE* st = m_toolState[aTool];

    st->contextMenu = aMenu;
    st->contextMenuTrigger = aTrigger;
}


const KIGFX::VC_SETTINGS& TOOL_MANAGER::GetCurrentToolVC() const
{
    if( TOOL_STATE* active = GetCurrentToolState() )
        return active->vcSettings;

    return m_viewControls->GetSettings();
}


TOOL_ID TOOL_MANAGER::MakeToolId( const std::string& aToolName )
{
    static int currentId;

    return currentId++;
}


void TOOL_MANAGER::SetEnvironment( EDA_ITEM* aModel, KIGFX::VIEW* aView,
                                   KIGFX::VIEW_CONTROLS* aViewControls,
                                   APP_SETTINGS_BASE* aSettings, TOOLS_HOLDER* aFrame )
{
    m_model = aModel;
    m_view = aView;
    m_viewControls = aViewControls;
    m_frame = aFrame;
    m_settings = aSettings;
}


bool TOOL_MANAGER::isActive( TOOL_BASE* aTool ) const
{
    if( !isRegistered( aTool ) )
        return false;

    // Just check if the tool is on the active tools stack
    return alg::contains( m_activeTools, aTool->GetId() );
}


void TOOL_MANAGER::saveViewControls( TOOL_STATE* aState )
{
    aState->vcSettings = m_viewControls->GetSettings();

    if( m_menuActive )
    {
        // Context menu is active, so the cursor settings are overridden (see DispatchContextMenu())
        auto it = m_cursorSettings.find( aState->theTool->GetId() );

        if( it != m_cursorSettings.end() )
        {
            const KIGFX::VC_SETTINGS& curr = m_viewControls->GetSettings();

            // Tool has overridden the cursor position, so store the new settings
            if( !curr.m_forceCursorPosition || curr.m_forcedPosition != m_menuCursor )
            {
                if( !curr.m_forceCursorPosition )
                    it->second = std::nullopt;
                else
                    it->second = curr.m_forcedPosition;
            }
            else
            {
                std::optional<VECTOR2D> cursor = it->second;

                if( cursor )
                {
                    aState->vcSettings.m_forceCursorPosition = true;
                    aState->vcSettings.m_forcedPosition = *cursor;
                }
                else
                {
                    aState->vcSettings.m_forceCursorPosition = false;
                }
            }
        }
    }
}


void TOOL_MANAGER::applyViewControls( const TOOL_STATE* aState )
{
    m_viewControls->ApplySettings( aState->vcSettings );
}


bool TOOL_MANAGER::processEvent( const TOOL_EVENT& aEvent )
{
    wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::processEvent - %s" ), aEvent.Format() );

    // First try to dispatch the action associated with the event if it is a key press event
    bool handled = DispatchHotKey( aEvent );

    if( !handled )
    {
        TOOL_EVENT mod_event( aEvent );

        // Only immediate actions get the position.  Otherwise clear for tool activation
        if( GetToolHolder() && !GetToolHolder()->GetDoImmediateActions() )
        {
            // An tool-selection-event has no position
            if( !mod_event.getCommandStr().empty()
                    && mod_event.getCommandStr() != GetToolHolder()->CurrentToolName()
                    && !mod_event.ForceImmediate() )
            {
                mod_event.SetHasPosition( false );
            }
        }

        // If the event is not handled through a hotkey activation, pass it to the currently
        // running tool loops
        handled |= dispatchInternal( mod_event );
        handled |= dispatchActivation( mod_event );

        // Open the context menu if requested by a tool
        DispatchContextMenu( mod_event );

        // Dispatch any remaining events in the event queue
        while( !m_eventQueue.empty() )
        {
            TOOL_EVENT event = m_eventQueue.front();
            m_eventQueue.pop_front();
            processEvent( event );
        }
    }

    wxLogTrace( kicadTraceToolStack, wxS( "TOOL_MANAGER::processEvent - %s handle event: %s" ),
                                     ( handled ? "Did" : "Did not" ), aEvent.Format() );

    return handled;
}


void TOOL_MANAGER::setActiveState( TOOL_STATE* aState )
{
    if( m_activeState && m_viewControls )
        saveViewControls( m_activeState );

    m_activeState = aState;

    if( m_activeState && m_viewControls )
        applyViewControls( aState );
}


bool TOOL_MANAGER::IsToolActive( TOOL_ID aId ) const
{
    auto it = m_toolIdIndex.find( aId );

    if( it == m_toolIdIndex.end() )
        return false;

    return !it->second->idle;
}


void TOOL_MANAGER::UpdateUI( const TOOL_EVENT& aEvent )
{
    EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( GetToolHolder() );

    if( frame )
        frame->UpdateStatusBar();
}
