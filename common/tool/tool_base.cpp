#include <tool/tool_event.h>
#include <tool/tool_manager.h>

KiGfx::VIEW *TOOL_BASE::getView()
{
	return m_toolMgr->GetView();
}

KiGfx::VIEW_CONTROLS *TOOL_BASE::getViewControls()
{	
	return m_toolMgr->GetViewControls();
}
	
wxWindow * TOOL_BASE::getEditFrameInt()
{
	return m_toolMgr->GetEditFrame();
}

EDA_ITEM * TOOL_BASE::getModelInt()
{
	return m_toolMgr->GetModel();
}