/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_manager.h>
#include <tool/common_tools.h>
#include <tool/zoom_tool.h>

#include "cvpcb_actions.h"
#include "cvpcb_control.h"
#include "cvpcb_selection_tool.h"
#include <cvpcb_id.h>


void CVPCB_ACTIONS::RegisterAllTools( TOOL_MANAGER* aToolManager )
{
    aToolManager->RegisterTool( new COMMON_TOOLS );
    aToolManager->RegisterTool( new ZOOM_TOOL );
    aToolManager->RegisterTool( new CVPCB_SELECTION_TOOL );
    aToolManager->RegisterTool( new CVPCB_CONTROL );
}


OPT<TOOL_EVENT> CVPCB_ACTIONS::TranslateLegacyId( int aId )
{
    switch( aId )
    {
    case ID_ZOOM_IN:        // toolbar button "Zoom In"
    case ID_VIEWER_ZOOM_IN:
        return ACTIONS::zoomInCenter.MakeEvent();

    case ID_ZOOM_OUT:       // toolbar button "Zoom In"
    case ID_VIEWER_ZOOM_OUT:
        return ACTIONS::zoomOutCenter.MakeEvent();

    case ID_ZOOM_PAGE:      // toolbar button "Fit on Screen"
    case ID_VIEWER_ZOOM_PAGE:
        return ACTIONS::zoomFitScreen.MakeEvent();

    case ID_ZOOM_SELECTION:
        return ACTIONS::zoomTool.MakeEvent();

    case ID_TB_MEASUREMENT_TOOL:
        return CVPCB_ACTIONS::measureTool.MakeEvent();

    case ID_NO_TOOL_SELECTED:
        return CVPCB_ACTIONS::no_selectionTool.MakeEvent();
    }

    return OPT<TOOL_EVENT>();
}
