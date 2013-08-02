#include <string>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tool/tool_interactive.h>
#include <tool/context_menu.h>

TOOL_INTERACTIVE::TOOL_INTERACTIVE( TOOL_ID aId, const std::string& aName ):
	TOOL_BASE(TOOL_Interactive, aId, aName)
	{};

TOOL_INTERACTIVE::TOOL_INTERACTIVE( const std::string& aName ):
	TOOL_BASE(TOOL_Interactive, TOOL_MANAGER::MakeToolId(aName), aName)
	{};


TOOL_INTERACTIVE::~TOOL_INTERACTIVE()
{

}

OPT_TOOL_EVENT TOOL_INTERACTIVE::Wait ( const TOOL_EVENT_LIST & aEventList )
{
	return m_toolMgr->ScheduleWait(this, aEventList);
}

void TOOL_INTERACTIVE::goInternal( TOOL_STATE_FUNC& aState, const TOOL_EVENT_LIST& aConditions )
{
	m_toolMgr->ScheduleNextState(this, aState, aConditions);
}

void TOOL_INTERACTIVE::Reset()
{

}

void TOOL_INTERACTIVE::SetContextMenu( CONTEXT_MENU *aMenu, TOOL_ContextMenuTrigger aTrigger )
{
	aMenu->setTool(this);
	m_toolMgr->ScheduleContextMenu(this, aMenu, aTrigger);
}
