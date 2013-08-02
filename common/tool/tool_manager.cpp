#include <map>
#include <deque>


#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/map.hpp>

#include <wx/event.h>

#include <view/view.h>

#include <tool/tool_base.h>
#include <tool/tool_interactive.h>
#include <tool/tool_manager.h>
#include <tool/context_menu.h>
#include <tool/coroutine.h>

#include <wxPcbStruct.h>
#include <class_drawpanel_gal.h>

using boost::optional;
using namespace std;

struct TOOL_MANAGER::ToolState
{
	TOOL_BASE *theTool;
	
	bool idle;
	bool pendingWait;
	bool pendingContextMenu;

	CONTEXT_MENU *contextMenu;
	TOOL_ContextMenuTrigger contextMenuTrigger;

	COROUTINE<int, TOOL_EVENT&> *cofunc;
	
	TOOL_EVENT wakeupEvent;
	TOOL_EVENT_LIST waitEvents;

	std::vector<Transition> transitions;


};

TOOL_MANAGER::TOOL_MANAGER()
{
	
}

void TOOL_MANAGER::RegisterTool ( TOOL_BASE *aTool )
{
	ToolState *st = new ToolState;

	st->theTool = aTool;
	st->idle = true;
	st->pendingWait = false;
	st->pendingContextMenu = false;
	st->cofunc = NULL;
	st->contextMenuTrigger = CMENU_OFF;

	m_toolState[ aTool ] = st;
	m_toolNameIndex [ aTool->GetName() ] = st;
	m_toolIdIndex [ aTool->GetId() ] = st;
	
	aTool->m_toolMgr = this;
	
	if(aTool->GetType() == TOOL_Interactive)
		static_cast<TOOL_INTERACTIVE*>(aTool)->Reset();
}

void TOOL_MANAGER::ScheduleNextState( TOOL_BASE *aTool, TOOL_STATE_FUNC& aHandler, const TOOL_EVENT_LIST & aConditions )
{
	ToolState *st = m_toolState [aTool];
	st->transitions.push_back ( Transition (aConditions, aHandler ));
}

optional<TOOL_EVENT> TOOL_MANAGER::ScheduleWait( TOOL_BASE *aTool, const TOOL_EVENT_LIST & aConditions )
{
	ToolState *st = m_toolState [aTool];
	
	st->pendingWait = true;
	st->waitEvents = aConditions;
	st->cofunc->Yield();

	return st->wakeupEvent;
}

void TOOL_MANAGER::dispatchInternal ( TOOL_EVENT& aEvent )
{
	// iterate over all registered tools
	BOOST_FOREACH(ToolState *st, m_toolState | boost::adaptors::map_values)
	{
		// the tool state handler is waiting for events (i.e. called Wait() method)
		if(st->pendingWait)
		{

			if( st->waitEvents.Matches(aEvent) )
			{
				// got matching event? clear wait list and wake up the coroutine
				st->wakeupEvent = aEvent;
				st->pendingWait = false;
				st->waitEvents.clear();
				st->cofunc->Resume();
				if(!st->cofunc->Running())
					delete st->cofunc;

			}
		} else {
			// no state handler in progress - check if there are any transitions (defined by
			// Go() method that match the event.

			if(st->transitions.size()) {
				BOOST_FOREACH(Transition tr, st->transitions)
				{
					if(tr.first.Matches(aEvent))
					{
						st->transitions.clear();

						if(!st->cofunc)
							st->cofunc = new COROUTINE<int, TOOL_EVENT&>( tr.second );
						else 
							st->cofunc->SetEntry( tr.second );
							
						// got match? Run the handler.
						st->cofunc->Call(aEvent);
						
						if(!st->cofunc->Running())
							delete st->cofunc;
					}
				}
			}
		}
	}	
}

bool TOOL_MANAGER::ProcessEvent (TOOL_EVENT& aEvent)
{
	printf("process: %s\n", aEvent.Format().c_str());

	dispatchInternal(aEvent);
	

	BOOST_FOREACH(ToolState *st, m_toolState | boost::adaptors::map_values)
	{
		if(st->contextMenuTrigger == CMENU_NOW)
		{
			st->pendingWait = true;
			st->waitEvents = TOOL_EVENT ( TC_Any, TA_Any ); 
			st->contextMenuTrigger = CMENU_OFF;
			GetEditFrame()->PopupMenu( st->contextMenu->GetMenu() );

			TOOL_EVENT evt ( TC_Command, TA_ContextMenuChoice );
			dispatchInternal( evt );

			break;
		}
	}

	if(m_view->IsDirty())
	{
		PCB_EDIT_FRAME *f = static_cast<PCB_EDIT_FRAME*>(GetEditFrame());
		f->GetGalCanvas()->Refresh(); // fixme: ugly hack, provide a method in TOOL_DISPATCHER.
	}

	return false;
}

void TOOL_MANAGER::ScheduleContextMenu( TOOL_BASE *aTool, CONTEXT_MENU *aMenu, TOOL_ContextMenuTrigger aTrigger )
{
	ToolState *st = m_toolState [aTool];

	st->contextMenu = aMenu;
	st->contextMenuTrigger = aTrigger;
	
	if(aTrigger == CMENU_NOW)
		st->cofunc->Yield();	
}

TOOL_ID TOOL_MANAGER::MakeToolId( const std::string &aToolName )
{
	static int currentId;
	return currentId++;
}

void TOOL_MANAGER::SetEnvironment( EDA_ITEM *aModel, KiGfx::VIEW* aView, KiGfx::VIEW_CONTROLS *aViewControls, wxWindow *aFrame )
{
	m_model = aModel;
	m_view = aView;
	m_viewControls = aViewControls;
	m_editFrame = aFrame;
	// fixme: reset tools after changing environment
}
