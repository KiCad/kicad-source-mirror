/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "pcb_actions.h"
#include <pcbnew_id.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>


OPT<TOOL_EVENT> PCB_ACTIONS::TranslateLegacyId( int aId )
{
    switch( aId )
    {
    case ID_DIFF_PAIR_BUTT:
        return PCB_ACTIONS::routerActivateDiffPair.MakeEvent();

    case ID_TUNE_SINGLE_TRACK_LEN_BUTT:
        return PCB_ACTIONS::routerActivateTuneSingleTrace.MakeEvent();

    case ID_TUNE_DIFF_PAIR_LEN_BUTT:
        return PCB_ACTIONS::routerActivateTuneDiffPair.MakeEvent();

    case ID_TUNE_DIFF_PAIR_SKEW_BUTT:
        return PCB_ACTIONS::routerActivateTuneDiffPairSkew.MakeEvent();

    case ID_MENU_INTERACTIVE_ROUTER_SETTINGS:
        return PCB_ACTIONS::routerActivateSettingsDialog.MakeEvent();

    case ID_MENU_DIFF_PAIR_DIMENSIONS:
        return PCB_ACTIONS::routerActivateDpDimensionsDialog.MakeEvent();

    case ID_GEN_IMPORT_GRAPHICS_FILE:
        return PCB_ACTIONS::placeImportedGraphics.MakeEvent();

    case ID_POPUP_GRID_NEXT:
        return ACTIONS::gridNext.MakeEvent();

    case ID_POPUP_GRID_PREV:
        return ACTIONS::gridPrev.MakeEvent();

    case ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST:
        return PCB_ACTIONS::findMove.MakeEvent();

    case ID_NO_TOOL_SELECTED:
        return PCB_ACTIONS::selectionTool.MakeEvent();

    case ID_APPEND_FILE:
        return PCB_ACTIONS::appendBoard.MakeEvent();

    case ID_PCB_MUWAVE_TOOL_GAP_CMD:
        return PCB_ACTIONS::microwaveCreateGap.MakeEvent();

    case ID_PCB_MUWAVE_TOOL_STUB_CMD:
        return PCB_ACTIONS::microwaveCreateStub.MakeEvent();

    case ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD:
        return PCB_ACTIONS::microwaveCreateStubArc.MakeEvent();

    case ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD:
        return PCB_ACTIONS::microwaveCreateFunctionShape.MakeEvent();

    case ID_PCB_MUWAVE_TOOL_SELF_CMD:
        return PCB_ACTIONS::microwaveCreateLine.MakeEvent();
    }

    return OPT<TOOL_EVENT>();
}
