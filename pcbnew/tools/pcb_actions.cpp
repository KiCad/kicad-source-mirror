/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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


boost::optional<TOOL_EVENT> PCB_ACTIONS::TranslateLegacyId( int aId )
{
    switch( aId )
    {
    case ID_PCB_MODULE_BUTT:
        return PCB_ACTIONS::placeModule.MakeEvent();

    case ID_TRACK_BUTT:
        return PCB_ACTIONS::routerActivateSingle.MakeEvent();

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

    case ID_PCB_ZONES_BUTT:
        return PCB_ACTIONS::drawZone.MakeEvent();

    case ID_PCB_KEEPOUT_AREA_BUTT:
        return PCB_ACTIONS::drawKeepout.MakeEvent();

    case ID_PCB_ADD_LINE_BUTT:
    case ID_MODEDIT_LINE_TOOL:
        return PCB_ACTIONS::drawLine.MakeEvent();

    case ID_PCB_CIRCLE_BUTT:
    case ID_MODEDIT_CIRCLE_TOOL:
        return PCB_ACTIONS::drawCircle.MakeEvent();

    case ID_PCB_ARC_BUTT:
    case ID_MODEDIT_ARC_TOOL:
        return PCB_ACTIONS::drawArc.MakeEvent();

    case ID_PCB_ADD_TEXT_BUTT:
    case ID_MODEDIT_TEXT_TOOL:
        return PCB_ACTIONS::placeText.MakeEvent();

    case ID_PCB_DIMENSION_BUTT:
        return PCB_ACTIONS::drawDimension.MakeEvent();

    case ID_PCB_MIRE_BUTT:
        return PCB_ACTIONS::placeTarget.MakeEvent();

    case ID_MODEDIT_PAD_TOOL:
        return PCB_ACTIONS::placePad.MakeEvent();

    case ID_GEN_IMPORT_DXF_FILE:
        return PCB_ACTIONS::placeDXF.MakeEvent();

    case ID_MODEDIT_ANCHOR_TOOL:
        return PCB_ACTIONS::setAnchor.MakeEvent();

    case ID_PCB_PLACE_GRID_COORD_BUTT:
    case ID_MODEDIT_PLACE_GRID_COORD:
        return ACTIONS::gridSetOrigin.MakeEvent();

    case ID_ZOOM_IN:        // toolbar button "Zoom In"
        return ACTIONS::zoomInCenter.MakeEvent();

    case ID_ZOOM_OUT:       // toolbar button "Zoom In"
        return ACTIONS::zoomOutCenter.MakeEvent();

    case ID_ZOOM_PAGE:      // toolbar button "Fit on Screen"
        return ACTIONS::zoomFitScreen.MakeEvent();

    case ID_TB_OPTIONS_SHOW_TRACKS_SKETCH:
        return PCB_ACTIONS::trackDisplayMode.MakeEvent();

    case ID_TB_OPTIONS_SHOW_PADS_SKETCH:
        return PCB_ACTIONS::padDisplayMode.MakeEvent();

    case ID_TB_OPTIONS_SHOW_VIAS_SKETCH:
        return PCB_ACTIONS::viaDisplayMode.MakeEvent();

    case ID_TB_OPTIONS_SHOW_ZONES:
        return PCB_ACTIONS::zoneDisplayEnable.MakeEvent();

    case ID_TB_OPTIONS_SHOW_ZONES_DISABLE:
        return PCB_ACTIONS::zoneDisplayDisable.MakeEvent();

    case ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY:
        return PCB_ACTIONS::zoneDisplayOutlines.MakeEvent();

    case ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH:
        return PCB_ACTIONS::moduleEdgeOutlines.MakeEvent();

    case ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH:
        return PCB_ACTIONS::moduleTextOutlines.MakeEvent();

    case ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE:
        return PCB_ACTIONS::highContrastMode.MakeEvent();

    case ID_FIND_ITEMS:
        return PCB_ACTIONS::find.MakeEvent();

    case ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST:
        return PCB_ACTIONS::findMove.MakeEvent();

    case ID_NO_TOOL_SELECTED:
        return PCB_ACTIONS::selectionTool.MakeEvent();

    case ID_ZOOM_SELECTION:
        return PCB_ACTIONS::zoomTool.MakeEvent();

    case ID_PCB_DELETE_ITEM_BUTT:
    case ID_MODEDIT_DELETE_TOOL:
        return PCB_ACTIONS::deleteItemCursor.MakeEvent();

    case ID_PCB_PLACE_OFFSET_COORD_BUTT:
        return PCB_ACTIONS::drillOrigin.MakeEvent();

    case ID_PCB_HIGHLIGHT_BUTT:
        return PCB_ACTIONS::highlightNetCursor.MakeEvent();

    case ID_APPEND_FILE:
        return PCB_ACTIONS::appendBoard.MakeEvent();

    case ID_PCB_SHOW_1_RATSNEST_BUTT:
        return PCB_ACTIONS::toBeDone.MakeEvent();
    }

    return boost::optional<TOOL_EVENT>();
}
