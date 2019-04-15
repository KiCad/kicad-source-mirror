/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <common.h>
#include <eeschema_id.h>
#include <tool/tool_manager.h>
#include <tool/common_tools.h>
#include <tools/sch_editor_control.h>
#include <tools/sch_picker_tool.h>

#include <sch_actions.h>

OPT<TOOL_EVENT> SCH_ACTIONS::TranslateLegacyId( int aId )
{
    switch( aId )
    {
    case ID_ZOOM_REDRAW:
    case ID_POPUP_ZOOM_REDRAW:
    case ID_VIEWER_ZOOM_REDRAW:
        return ACTIONS::zoomRedraw.MakeEvent();

    case ID_POPUP_ZOOM_IN:
        return ACTIONS::zoomIn.MakeEvent();

    case ID_ZOOM_IN:        // toolbar button "Zoom In"
    case ID_VIEWER_ZOOM_IN:
        return ACTIONS::zoomInCenter.MakeEvent();

    case ID_POPUP_ZOOM_OUT:
        return ACTIONS::zoomOut.MakeEvent();

    case ID_ZOOM_OUT:       // toolbar button "Zoom Out"
    case ID_VIEWER_ZOOM_OUT:
        return ACTIONS::zoomOutCenter.MakeEvent();

    case ID_POPUP_ZOOM_PAGE:
    case ID_ZOOM_PAGE:      // toolbar button "Fit on Screen"
    case ID_VIEWER_ZOOM_PAGE:
        return ACTIONS::zoomFitScreen.MakeEvent();

    case ID_POPUP_ZOOM_CENTER:
        return ACTIONS::zoomCenter.MakeEvent();

    case ID_ZOOM_SELECTION:
        return ACTIONS::zoomTool.MakeEvent();

    case ID_POPUP_GRID_NEXT:
        return ACTIONS::gridNext.MakeEvent();

    case ID_POPUP_GRID_PREV:
        return ACTIONS::gridPrev.MakeEvent();

    case ID_HIGHLIGHT_BUTT:
        return SCH_ACTIONS::highlightNetCursor.MakeEvent();

    case ID_HIGHLIGHT_NET:
        return SCH_ACTIONS::highlightNet.MakeEvent();

    case ID_SCH_PLACE_COMPONENT:
        return SCH_ACTIONS::placeSymbol.MakeEvent();

    case ID_PLACE_POWER_BUTT:
        return SCH_ACTIONS::placePower.MakeEvent();
    }

    return OPT<TOOL_EVENT>();
}


void SCH_ACTIONS::RegisterAllTools( TOOL_MANAGER* aToolManager )
{
    aToolManager->RegisterTool( new COMMON_TOOLS );
    aToolManager->RegisterTool( new SCH_EDITOR_CONTROL );
    aToolManager->RegisterTool( new SCH_PICKER_TOOL );
}
