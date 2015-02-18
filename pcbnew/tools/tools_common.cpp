#include <io_mgr.h>

#include <tool/tool_manager.h>

#include <tools/selection_tool.h>
#include <tools/edit_tool.h>
#include <tools/drawing_tool.h>
#include <tools/point_editor.h>
#include <tools/pcbnew_control.h>
#include <tools/pcb_editor_control.h>
#include <tools/placement_tool.h>
#include <tools/common_actions.h>

#include <router/router_tool.h>
#include <router/length_tuner_tool.h>

void registerAllTools ( TOOL_MANAGER *aToolManager )
{
    aToolManager->RegisterTool( new SELECTION_TOOL );
    aToolManager->RegisterTool( new ROUTER_TOOL );
    aToolManager->RegisterTool( new LENGTH_TUNER_TOOL );
    aToolManager->RegisterTool( new EDIT_TOOL );
    aToolManager->RegisterTool( new DRAWING_TOOL );
    aToolManager->RegisterTool( new POINT_EDITOR );
    aToolManager->RegisterTool( new PCBNEW_CONTROL );
    aToolManager->RegisterTool( new PCB_EDITOR_CONTROL );
    aToolManager->RegisterTool( new PLACEMENT_TOOL );
}