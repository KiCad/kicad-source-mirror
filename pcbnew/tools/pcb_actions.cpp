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


OPT<TOOL_EVENT> PCB_ACTIONS::TranslateLegacyId( int aId )
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

    case ID_PCB_DRAW_VIA_BUTT:
        return PCB_ACTIONS::drawVia.MakeEvent();

    case ID_PCB_KEEPOUT_AREA_BUTT:
        return PCB_ACTIONS::drawZoneKeepout.MakeEvent();

    case ID_PCB_ADD_LINE_BUTT:
    case ID_MODEDIT_LINE_TOOL:
        return PCB_ACTIONS::drawLine.MakeEvent();

    case ID_PCB_ADD_POLYGON_BUTT:
    case ID_MODEDIT_POLYGON_TOOL:
        return PCB_ACTIONS::drawGraphicPolygon.MakeEvent();

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

    case ID_PCB_TARGET_BUTT:
        return PCB_ACTIONS::placeTarget.MakeEvent();

    case ID_MODEDIT_PAD_TOOL:
        return PCB_ACTIONS::placePad.MakeEvent();

    case ID_GEN_IMPORT_GRAPHICS_FILE:
        return PCB_ACTIONS::placeImportedGraphics.MakeEvent();

    case ID_MODEDIT_ANCHOR_TOOL:
        return PCB_ACTIONS::setAnchor.MakeEvent();

    case ID_PCB_PLACE_GRID_COORD_BUTT:
    case ID_MODEDIT_PLACE_GRID_COORD:
        return ACTIONS::gridSetOrigin.MakeEvent();

    case ID_ZOOM_IN:        // toolbar button "Zoom In"
    case ID_VIEWER_ZOOM_IN:
        return ACTIONS::zoomInCenter.MakeEvent();

    case ID_ZOOM_OUT:       // toolbar button "Zoom In"
    case ID_VIEWER_ZOOM_OUT:
        return ACTIONS::zoomOutCenter.MakeEvent();

    case ID_ZOOM_PAGE:      // toolbar button "Fit on Screen"
    case ID_VIEWER_ZOOM_PAGE:
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

    case ID_TB_OPTIONS_SHOW_GRAPHIC_SKETCH:;
        return PCB_ACTIONS::graphicDisplayMode.MakeEvent();

    case ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH:
        return PCB_ACTIONS::moduleEdgeOutlines.MakeEvent();

    case ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH:;
        return PCB_ACTIONS::moduleTextOutlines.MakeEvent();

    case ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE:
        return PCB_ACTIONS::highContrastMode.MakeEvent();

    case ID_DEC_LAYER_ALPHA:
        return PCB_ACTIONS::layerAlphaDec.MakeEvent();

    case ID_INC_LAYER_ALPHA:
        return PCB_ACTIONS::layerAlphaInc.MakeEvent();

    case ID_FIND_ITEMS:
        return PCB_ACTIONS::find.MakeEvent();

    case ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST:
        return PCB_ACTIONS::findMove.MakeEvent();

    case ID_NO_TOOL_SELECTED:
        return PCB_ACTIONS::selectionTool.MakeEvent();

    case ID_ZOOM_SELECTION:
        return ACTIONS::zoomTool.MakeEvent();

    case ID_PCB_DELETE_ITEM_BUTT:
    case ID_MODEDIT_DELETE_TOOL:
        return PCB_ACTIONS::deleteItemCursor.MakeEvent();

    case ID_PCB_PLACE_OFFSET_COORD_BUTT:
        return PCB_ACTIONS::drillOrigin.MakeEvent();

    case ID_PCB_MEASUREMENT_TOOL:
    case ID_MODEDIT_MEASUREMENT_TOOL:
        return PCB_ACTIONS::measureTool.MakeEvent();

    case ID_PCB_HIGHLIGHT_BUTT:
        return PCB_ACTIONS::highlightNetCursor.MakeEvent();

    case ID_APPEND_FILE:
        return PCB_ACTIONS::appendBoard.MakeEvent();

    case ID_PCB_SHOW_1_RATSNEST_BUTT:
        return PCB_ACTIONS::showLocalRatsnest.MakeEvent();

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

    case ID_EDIT_CUT:
        return PCB_ACTIONS::cutToClipboard.MakeEvent();

    case ID_EDIT_COPY:
        return PCB_ACTIONS::copyToClipboard.MakeEvent();

    case ID_EDIT_PASTE:
        return PCB_ACTIONS::pasteFromClipboard.MakeEvent();

    case ID_POPUP_PCB_FILL_ALL_ZONES:
        return PCB_ACTIONS::zoneFillAll.MakeEvent();

    case ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_ALL_ZONES:
        return PCB_ACTIONS::zoneUnfillAll.MakeEvent();

    case ID_POPUP_PCB_AUTOPLACE_OFF_BOARD_MODULES:
        return PCB_ACTIONS::autoplaceOffboardComponents.MakeEvent();

    case ID_POPUP_PCB_AUTOPLACE_SELECTED_MODULES:
        return PCB_ACTIONS::autoplaceSelectedComponents.MakeEvent();

    case ID_PCBNEW_SHOW_HELP:
        return PCB_ACTIONS::showHelp.MakeEvent();

    }

    return OPT<TOOL_EVENT>();
}
