/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tool/tool_manager.h>
#include <tool/common_tools.h>
#include <tool/zoom_tool.h>
#include <gerbview_id.h>

#include "gerbview_actions.h"
#include "selection_tool.h"
#include "gerbview_control.h"


void GERBVIEW_ACTIONS::RegisterAllTools( TOOL_MANAGER* aToolManager )
{
    aToolManager->RegisterTool( new COMMON_TOOLS );
    aToolManager->RegisterTool( new GERBVIEW_SELECTION_TOOL );
    aToolManager->RegisterTool( new GERBVIEW_CONTROL );
    aToolManager->RegisterTool( new ZOOM_TOOL );
}

OPT<TOOL_EVENT> GERBVIEW_ACTIONS::TranslateLegacyId( int aId )
{
    switch( aId )
    {
    case ID_ZOOM_IN:        // toolbar button "Zoom In"
        return ACTIONS::zoomInCenter.MakeEvent();

    case ID_ZOOM_OUT:       // toolbar button "Zoom In"
        return ACTIONS::zoomOutCenter.MakeEvent();

    case ID_ZOOM_PAGE:      // toolbar button "Fit on Screen"
        return ACTIONS::zoomFitScreen.MakeEvent();

    case ID_ZOOM_SELECTION:
        return ACTIONS::zoomTool.MakeEvent();

    case ID_TB_MEASUREMENT_TOOL:
        return GERBVIEW_ACTIONS::measureTool.MakeEvent();

    case ID_NO_TOOL_SELECTED:
        return GERBVIEW_ACTIONS::selectionTool.MakeEvent();

    case ID_HIGHLIGHT_REMOVE_ALL:
        return GERBVIEW_ACTIONS::highlightClear.MakeEvent();

    case ID_HIGHLIGHT_CMP_ITEMS:
        return GERBVIEW_ACTIONS::highlightComponent.MakeEvent();

    case ID_HIGHLIGHT_NET_ITEMS:
        return GERBVIEW_ACTIONS::highlightNet.MakeEvent();

    case ID_GERBVIEW_SHOW_HELP:
        return GERBVIEW_ACTIONS::showHelp.MakeEvent();

    case ID_HIGHLIGHT_APER_ATTRIBUTE_ITEMS:
        return GERBVIEW_ACTIONS::highlightAttribute.MakeEvent();
        break;
    }

    return OPT<TOOL_EVENT>();
}
