#include <cstring>
#include <string>

#include <base_struct.h>

#include <tool/tool_event.h>
#include <tool/tool_manager.h>

#include <boost/foreach.hpp>

using namespace std;

struct FlagString {
	int flag;		
	std::string str;
};

static const std::string flag2string(int flag, const FlagString *exps)
{
	std::string rv;
	for(int i = 0; exps[i].str.length(); i++)
		if(exps[i].flag & flag)
			rv+=exps[i].str+" ";
	return rv;
}

const std::string TOOL_EVENT::Format() const
{
	std::string ev;

	const FlagString categories[] = {
		{TC_Mouse, "mouse"},
		{TC_Command, "command"},
		{TC_Message, "message"},
		{TC_View, "view"},
		{0, ""}
	};

	const FlagString actions[] = {
		{TA_MouseClick, "click"},
		{TA_MouseUp, "button-up"},
		{TA_MouseDown, "button-down"},
		{TA_MouseDrag, "drag"},
		{TA_MouseMotion, "motion"},
		{TA_MouseWheel, "wheel"},
		{TA_ViewRefresh, "view-refresh"},
		{TA_ViewZoom, "view-zoom"},
		{TA_ViewPan, "view-pan"},
		{TA_ViewDirty, "view-dirty"},
		{TA_ChangeLayer, "change-layer"},
		{TA_CancelTool, "cancel-tool"},
		{TA_ActivateTool, "activate-tool"},
		{TA_ContextMenuUpdate, "context-menu-update"},
		{TA_ContextMenuChoice, "context-menu-choice"},
		{0, ""}
	};

	const FlagString buttons[] = {
		{MB_None, "none"},
		{MB_Left, "left"},
		{MB_Right, "right"},
		{MB_Middle, "middle"},
		{MB_ModShift, "shift"},
		{MB_ModCtrl, "ctrl"},
		{MB_ModAlt, "alt"},
		{0, ""}
	};

	ev = "category: ";
	ev += flag2string(m_category, categories);
	ev +=" action: ";
	ev += flag2string(m_actions, actions);
	
	if(m_actions & TA_Mouse)
	{
		ev +=" btns: ";
		ev += flag2string(m_mouseButtons, buttons);
	};
	
	if(m_commandId)
	{
		char tmp[128];
		sprintf(tmp,"cmd-id: %d", *m_commandId);
		ev += tmp;
	}

	if(m_commandStr)
		ev += "cmd-str: " + (*m_commandStr);
		
	return ev;
}

const std::string TOOL_EVENT_LIST::Format() const
{
	string s;

	BOOST_FOREACH(TOOL_EVENT e, m_events)
		s+=e.Format()+" ";
	
	return s;
}
