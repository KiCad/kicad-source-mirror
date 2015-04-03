/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include "common_actions.h"
#include <tool/action_manager.h>
#include <pcbnew_id.h>
#include <wx/defs.h>

// These members are static in class COMMON_ACTIONS: Build them here:

// Selection tool actions
TOOL_ACTION COMMON_ACTIONS::selectionActivate( "pcbnew.InteractiveSelection",
        AS_GLOBAL, 0,
        "", "", NULL, AF_ACTIVATE ); // No description, it is not supposed to be shown anywhere

TOOL_ACTION COMMON_ACTIONS::selectionCursor( "pcbnew.InteractiveSelection.Cursor",
        AS_GLOBAL, 0,
        "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION COMMON_ACTIONS::selectItem( "pcbnew.InteractiveSelection.SelectItem",
        AS_GLOBAL, 0,
        "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION COMMON_ACTIONS::unselectItem( "pcbnew.InteractiveSelection.UnselectItem",
        AS_GLOBAL, 0,
        "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION COMMON_ACTIONS::selectionClear( "pcbnew.InteractiveSelection.Clear",
        AS_GLOBAL, 0,
        "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION COMMON_ACTIONS::selectConnection( "pcbnew.InteractiveSelection.SelectConnection",
        AS_GLOBAL, 0,
        _( "copper connection" ), _( "Selects whole copper connection." ) );

TOOL_ACTION COMMON_ACTIONS::selectNet( "pcbnew.InteractiveSelection.SelectNet",
        AS_GLOBAL, 0,
        _( "whole net" ), _( "Selects all tracks & vias belonging to the same net." ) );

TOOL_ACTION COMMON_ACTIONS::find( "pcbnew.InteractiveSelection.Find",
        AS_GLOBAL, 0,                                    // it is handled by wxWidgets hotkey system
        _( "Find an item" ), _( "Searches the document for an item" ), find_xpm );

TOOL_ACTION COMMON_ACTIONS::findDummy( "pcbnew.Find.Dummy", // only block the hotkey
        AS_GLOBAL, MD_CTRL + int( 'F' ) );

TOOL_ACTION COMMON_ACTIONS::findMove( "pcbnew.InteractiveSelection.FindMove",
        AS_GLOBAL, 'T' );

// Edit tool actions
TOOL_ACTION COMMON_ACTIONS::editFootprintInFpEditor( "pcbnew.InteractiveEdit.editFootprintInFpEditor",
        AS_CONTEXT, MD_CTRL + 'E',
        _( "Open in Footprint Editor" ),
        _( "Opens the selected footprint in the Footprint Editor" ),
        module_editor_xpm );

TOOL_ACTION COMMON_ACTIONS::copyPadToSettings( "pcbnew.InteractiveEdit.copyPadToSettings",
        AS_CONTEXT, 0,
        _( "Copy pad settings to Current Settings" ),
        _( "Copies the properties of selected pad to the current template pad settings." ) );

TOOL_ACTION COMMON_ACTIONS::copySettingsToPads( "pcbnew.InteractiveEdit.copySettingsToPads",
        AS_CONTEXT, 0,
        _( "Copy Current Settings to pads" ),
        _( "Copies the current template pad settings to the selected pad(s)." ) );

TOOL_ACTION COMMON_ACTIONS::globalEditPads ( "pcbnew.InteractiveEdit.globalPadEdit",
        AS_CONTEXT, 0,
        _( "Global Pad Edition" ),
        _( "Changes pad properties globally." ), global_options_pad_xpm );

TOOL_ACTION COMMON_ACTIONS::editActivate( "pcbnew.InteractiveEdit",
        AS_GLOBAL, 'M',
        _( "Move" ), _( "Moves the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::duplicate( "pcbnew.InteractiveEdit.duplicate",
        AS_GLOBAL, MD_CTRL + int( 'D' ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ), duplicate_module_xpm );

TOOL_ACTION COMMON_ACTIONS::duplicateIncrement( "pcbnew.InteractiveEdit.duplicateIncrementPads",
        AS_GLOBAL, MD_CTRL + MD_SHIFT + int( 'D' ),
        _( "Duplicate" ), _( "Duplicates the selected item(s), incrementing pad numbers" ) );

TOOL_ACTION COMMON_ACTIONS::moveExact( "pcbnew.InteractiveEdit.moveExact",
        AS_GLOBAL, MD_CTRL + int( 'M' ),
        _( "Move Exactly..." ), _( "Moves the selected item(s) by an exact amount" ),
        move_module_xpm );

TOOL_ACTION COMMON_ACTIONS::createArray( "pcbnew.InteractiveEdit.createArray",
        AS_GLOBAL, MD_CTRL + int( 'N' ),
        _( "Create array" ), _( "Create array" ), array_module_xpm, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::rotate( "pcbnew.InteractiveEdit.rotate",
        AS_GLOBAL, 'R',
        _( "Rotate" ), _( "Rotates selected item(s)" ), rotate_cw_xpm );

TOOL_ACTION COMMON_ACTIONS::flip( "pcbnew.InteractiveEdit.flip",
        AS_GLOBAL, 'F',
        _( "Flip" ), _( "Flips selected item(s)" ), swap_layer_xpm );

TOOL_ACTION COMMON_ACTIONS::remove( "pcbnew.InteractiveEdit.remove",
        AS_GLOBAL, WXK_DELETE,
        _( "Remove" ), _( "Deletes selected item(s)" ), delete_track_xpm );

TOOL_ACTION COMMON_ACTIONS::properties( "pcbnew.InteractiveEdit.properties",
        AS_GLOBAL, 'E',
        _( "Properties..." ), _( "Displays properties window" ), editor_xpm );


// Drawing tool actions
TOOL_ACTION COMMON_ACTIONS::drawLine( "pcbnew.InteractiveDrawing.line",
        AS_GLOBAL, 0,
        _( "Draw a line" ), _( "Draw a line" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::drawCircle( "pcbnew.InteractiveDrawing.circle",
        AS_GLOBAL, 0,
        _( "Draw a circle" ), _( "Draw a circle" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::drawArc( "pcbnew.InteractiveDrawing.arc",
        AS_GLOBAL, 0,
        _( "Draw an arc" ), _( "Draw an arc" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::placeText( "pcbnew.InteractiveDrawing.text",
        AS_GLOBAL, 0,
        _( "Add a text" ), _( "Add a text" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::drawDimension( "pcbnew.InteractiveDrawing.dimension",
        AS_GLOBAL, 0,
        _( "Add a dimension" ), _( "Add a dimension" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::drawZone( "pcbnew.InteractiveDrawing.zone",
        AS_GLOBAL, 0,
        _( "Add a filled zone" ), _( "Add a filled zone" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::drawKeepout( "pcbnew.InteractiveDrawing.keepout",
        AS_GLOBAL, 0,
        _( "Add a keepout area" ), _( "Add a keepout area" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::placeDXF( "pcbnew.InteractiveDrawing.placeDXF",
        AS_GLOBAL, 0,
        "", "", NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::setAnchor( "pcbnew.InteractiveDrawing.setAnchor",
        AS_GLOBAL, 0,
        _( "Place the footprint anchor" ), _( "Place the footprint anchor" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::incWidth( "pcbnew.InteractiveDrawing.incWidth",
        AS_CONTEXT, '+',
        _( "Increase the line width" ), _( "Increase the line width" ) );

TOOL_ACTION COMMON_ACTIONS::decWidth( "pcbnew.InteractiveDrawing.decWidth",
        AS_CONTEXT, '-',
        _( "Decrease the line width" ), _( "Decrease the line width" ) );

TOOL_ACTION COMMON_ACTIONS::arcPosture( "pcbnew.InteractiveDrawing.arcPosture",
        AS_CONTEXT, '/',
        _( "Switch the arc posture" ), _( "Switch the arc posture" ) );


// View Controls
TOOL_ACTION COMMON_ACTIONS::zoomIn( "pcbnew.Control.zoomIn",
        AS_GLOBAL, WXK_F1,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::zoomOut( "pcbnew.Control.zoomOut",
        AS_GLOBAL, WXK_F2,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::zoomInCenter( "pcbnew.Control.zoomInCenter",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::zoomOutCenter( "pcbnew.Control.zoomOutCenter",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::zoomCenter( "pcbnew.Control.zoomCenter",
        AS_GLOBAL, WXK_F4,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::zoomFitScreen( "pcbnew.Control.zoomFitScreen",
        AS_GLOBAL, WXK_HOME,
        "", "" );


// Display modes
TOOL_ACTION COMMON_ACTIONS::trackDisplayMode( "pcbnew.Control.trackDisplayMode",
        AS_GLOBAL, 'K',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::padDisplayMode( "pcbnew.Control.padDisplayMode",
        AS_GLOBAL, 'J',     // TODO temporarily, find a better hot key
        "", "" );

TOOL_ACTION COMMON_ACTIONS::viaDisplayMode( "pcbnew.Control.viaDisplayMode",
        AS_GLOBAL, 'L',     // TODO temporarily, find a better hot key
        "", "" );

TOOL_ACTION COMMON_ACTIONS::zoneDisplayEnable( "pcbnew.Control.zoneDisplayEnable",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::zoneDisplayDisable( "pcbnew.Control.zoneDisplayDisable",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::zoneDisplayOutlines( "pcbnew.Control.zoneDisplayOutlines",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::highContrastMode( "pcbnew.Control.highContrastMode",
        AS_GLOBAL, 'H',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::highContrastInc( "pcbnew.Control.highContrastInc",
        AS_GLOBAL, '>',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::highContrastDec( "pcbnew.Control.highContrastDec",
        AS_GLOBAL, '<',
        "", "" );


// Layer control
TOOL_ACTION COMMON_ACTIONS::layerTop( "pcbnew.Control.layerTop",
        AS_GLOBAL, WXK_PAGEUP,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerInner1( "pcbnew.Control.layerInner1",
        AS_GLOBAL, WXK_F5,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerInner2( "pcbnew.Control.layerInner2",
        AS_GLOBAL, WXK_F6,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerInner3( "pcbnew.Control.layerInner3",
        AS_GLOBAL, WXK_F7,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerInner4( "pcbnew.Control.layerInner4",
        AS_GLOBAL, WXK_F8,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerInner5( "pcbnew.Control.layerInner5",
        AS_GLOBAL, WXK_F9,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerInner6( "pcbnew.Control.layerInner6",
        AS_GLOBAL, WXK_F10,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerBottom( "pcbnew.Control.layerBottom",
        AS_GLOBAL, WXK_PAGEDOWN,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerNext( "pcbnew.Control.layerNext",
        AS_GLOBAL, '+',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerPrev( "pcbnew.Control.layerPrev",
        AS_GLOBAL, '-',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerAlphaInc( "pcbnew.Control.layerAlphaInc",
        AS_GLOBAL, '}',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerAlphaDec( "pcbnew.Control.layerAlphaDec",
        AS_GLOBAL, '{',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::layerChanged( "pcbnew.Control.layerChanged",
        AS_GLOBAL, 0,
        "", "", NULL, AF_NOTIFY );


// Grid control
TOOL_ACTION COMMON_ACTIONS::gridFast1( "pcbnew.Control.gridFast1",
        AS_GLOBAL, MD_ALT + int( '1' ),
        "", "" );

TOOL_ACTION COMMON_ACTIONS::gridFast2( "pcbnew.Control.gridFast2",
        AS_GLOBAL, MD_ALT + int( '2' ),
        "", "" );

TOOL_ACTION COMMON_ACTIONS::gridNext( "pcbnew.Control.gridNext",
        AS_GLOBAL, '`',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::gridPrev( "pcbnew.Control.gridPrev",
        AS_GLOBAL, MD_CTRL + int( '`' ),
        "", "" );

TOOL_ACTION COMMON_ACTIONS::gridSetOrigin( "pcbnew.Control.gridSetOrigin",
        AS_GLOBAL, 0,
        "", "" );


// Track & via size control
TOOL_ACTION COMMON_ACTIONS::trackWidthInc( "pcbnew.EditorControl.trackWidthInc",
        AS_GLOBAL, '[',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::trackWidthDec( "pcbnew.EditorControl.trackWidthDec",
        AS_GLOBAL, ']',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::viaSizeInc( "pcbnew.EditorControl.viaSizeInc",
        AS_GLOBAL, '\'',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::viaSizeDec( "pcbnew.EditorControl.viaSizeDec",
        AS_GLOBAL, '\\',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::trackViaSizeChanged( "pcbnew.EditorControl.trackViaSizeChanged",
        AS_GLOBAL, 0,
        "", "", NULL, AF_NOTIFY );


// Zone actions
TOOL_ACTION COMMON_ACTIONS::zoneFill( "pcbnew.EditorControl.zoneFill",
        AS_GLOBAL, 0,
        _( "Fill" ), _( "Fill zone(s)" ), fill_zone_xpm );

TOOL_ACTION COMMON_ACTIONS::zoneFillAll( "pcbnew.EditorControl.zoneFillAll",
        AS_GLOBAL, int( 'B' ),
        _( "Fill all" ), _( "Fill all zones" ) );

TOOL_ACTION COMMON_ACTIONS::zoneUnfill( "pcbnew.EditorControl.zoneUnfill",
        AS_GLOBAL, 0,
        _( "Unfill" ), _( "Unfill zone(s)" ), zone_unfill_xpm );

TOOL_ACTION COMMON_ACTIONS::zoneUnfillAll( "pcbnew.EditorControl.zoneUnfillAll",
        AS_GLOBAL, int( 'N' ),
        _( "Unfill all" ), _( "Unfill all zones" ) );


TOOL_ACTION COMMON_ACTIONS::placeTarget( "pcbnew.EditorControl.placeTarget",
        AS_GLOBAL, 0,
        _( "Add layer alignment target" ), _( "Add layer alignment target" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::placeModule( "pcbnew.EditorControl.placeModule",
        AS_GLOBAL, 'O',
        _( "Add modules" ), _( "Add modules" ), NULL, AF_ACTIVATE );


// Module editor tools
TOOL_ACTION COMMON_ACTIONS::placePad( "pcbnew.ModuleEditor.placePad",
        AS_GLOBAL, 0,
        _( "Add pads" ), _( "Add pads" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::enumeratePads( "pcbnew.ModuleEditor.enumeratePads",
        AS_GLOBAL, 0,
        _( "Enumerate pads" ), _( "Enumerate pads" ), pad_enumerate_xpm, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::copyItems( "pcbnew.ModuleEditor.copyItems",
        AS_GLOBAL, MD_CTRL + int( 'C' ),
        _( "Copy items" ), _( "Copy items" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::pasteItems( "pcbnew.ModuleEditor.pasteItems",
        AS_GLOBAL, MD_CTRL + int( 'V' ),
        _( "Paste items" ), _( "Paste items" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::moduleEdgeOutlines( "pcbnew.ModuleEditor.graphicOutlines",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::moduleTextOutlines( "pcbnew.ModuleEditor.textOutlines",
       AS_GLOBAL, 0,
       "", "" );


// Miscellaneous
TOOL_ACTION COMMON_ACTIONS::selectionTool( "pcbnew.Control.selectionTool",
        AS_GLOBAL, 0,
        "", "", NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::resetCoords( "pcbnew.Control.resetCoords",
        AS_GLOBAL, ' ',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::switchCursor( "pcbnew.Control.switchCursor",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION COMMON_ACTIONS::switchUnits( "pcbnew.Control.switchUnits",
        AS_GLOBAL, MD_CTRL + int( 'U' ),
        "", "" );

TOOL_ACTION COMMON_ACTIONS::showHelp( "pcbnew.Control.showHelp",
        AS_GLOBAL, '?',
        "", "" );

TOOL_ACTION COMMON_ACTIONS::toBeDone( "pcbnew.Control.toBeDone",
        AS_GLOBAL, 0,           // dialog saying it is not implemented yet
        "", "" );               // so users are aware of that


TOOL_ACTION COMMON_ACTIONS::routerActivateSingle( "pcbnew.InteractiveRouter.SingleTrack",
        AS_GLOBAL, 'X',
        _( "Run push & shove router (single tracks)" ),
        _( "Run push & shove router (single tracks)" ), ps_router_xpm, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::routerActivateDiffPair( "pcbnew.InteractiveRouter.DiffPair",
        AS_GLOBAL, '6',
        _( "Run push & shove router (differential pairs)" ),
        _( "Run push & shove router (differential pairs)" ), ps_diff_pair_xpm, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::routerActivateSettingsDialog( "pcbnew.InteractiveRouter.SettingsDialog",
        AS_GLOBAL, 0,
        _( "Open Interactive Router settings" ),
        _( "Open Interactive Router settings" ), NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::routerActivateDpDimensionsDialog( "pcbnew.InteractiveRouter.DpDimensionsDialog",
        AS_GLOBAL, 0,
        _( "Open Differential Pair Dimension settings" ),
        _( "Open Differential Pair Dimension settings" ), ps_diff_pair_gap_xpm, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::routerActivateTuneSingleTrace( "pcbnew.LengthTuner.TuneSingleTrack",
        AS_GLOBAL, '7',
        _( "Tune length of a single track" ), "", ps_tune_length_xpm, AF_ACTIVATE );


TOOL_ACTION COMMON_ACTIONS::routerActivateTuneDiffPair( "pcbnew.LengthTuner.TuneDiffPair",
        AS_GLOBAL, '8',
        _( "Tune length of a differential pair" ), "", NULL, AF_ACTIVATE );


TOOL_ACTION COMMON_ACTIONS::routerActivateTuneDiffPairSkew( "pcbnew.LengthTuner.TuneDiffPairSkew",
        AS_GLOBAL, '9',
        _( "Tune skew of a differential pair" ), "", NULL, AF_ACTIVATE );

TOOL_ACTION COMMON_ACTIONS::routerInlineDrag( "pcbnew.InteractiveRouter.InlineDrag",
        AS_GLOBAL, 0,
        "", "" );

// Point editor
TOOL_ACTION COMMON_ACTIONS::pointEditorUpdate( "pcbnew.PointEditor.update",
        AS_GLOBAL, 0,
        "", "" );    // No description, it is not supposed to be shown anywhere

TOOL_ACTION COMMON_ACTIONS::pointEditorBreakOutline( "pcbnew.PointEditor.breakOutline",
        AS_GLOBAL, 0,
        _( "Create corner" ), _( "Create corner" ), add_corner_xpm );

// Placement tool
TOOL_ACTION COMMON_ACTIONS::alignTop( "pcbnew.Place.alignTop",
        AS_GLOBAL, 0,
        _( "Align items to the top" ),
        _( "Aligns selected items to the top edge" ) );

TOOL_ACTION COMMON_ACTIONS::alignBottom( "pcbnew.Place.alignBottom",
        AS_GLOBAL, 0,
        _( "Align items to the bottom" ),
        _( "Aligns selected items to the bottom edge" ) );

TOOL_ACTION COMMON_ACTIONS::alignLeft( "pcbnew.Place.alignLeft",
        AS_GLOBAL, 0,
        _( "Align items to the left" ),
        _( "Aligns selected items to the left edge" ) );

TOOL_ACTION COMMON_ACTIONS::alignRight( "pcbnew.Place.alignRight",
        AS_GLOBAL, 0,
        _( "Align items to the right" ),
        _( "Aligns selected items to the right edge" ) );

TOOL_ACTION COMMON_ACTIONS::distributeHorizontally( "pcbnew.Place.distributeHorizontally",
        AS_GLOBAL, 0,
        _( "Distribute horizontally" ),
        _( "Distributes selected items along the horizontal axis" ) );

TOOL_ACTION COMMON_ACTIONS::distributeVertically( "pcbnew.Place.distributeVertically",
        AS_GLOBAL, 0,
        _( "Distribute vertically" ),
        _( "Distributes selected items along the vertical axis" ) );


boost::optional<TOOL_EVENT> COMMON_ACTIONS::TranslateLegacyId( int aId )
{
    switch( aId )
    {
    case ID_PCB_MODULE_BUTT:
        return COMMON_ACTIONS::placeModule.MakeEvent();

    case ID_TRACK_BUTT:
        return COMMON_ACTIONS::routerActivateSingle.MakeEvent();

    case ID_DIFF_PAIR_BUTT:
        return COMMON_ACTIONS::routerActivateDiffPair.MakeEvent();

    case ID_TUNE_SINGLE_TRACK_LEN_BUTT:
        return COMMON_ACTIONS::routerActivateTuneSingleTrace.MakeEvent();

    case ID_TUNE_DIFF_PAIR_LEN_BUTT:
        return COMMON_ACTIONS::routerActivateTuneDiffPair.MakeEvent();

    case ID_TUNE_DIFF_PAIR_SKEW_BUTT:
        return COMMON_ACTIONS::routerActivateTuneDiffPairSkew.MakeEvent();

    case ID_MENU_INTERACTIVE_ROUTER_SETTINGS:
        return COMMON_ACTIONS::routerActivateSettingsDialog.MakeEvent();

    case ID_MENU_DIFF_PAIR_DIMENSIONS:
        return COMMON_ACTIONS::routerActivateDpDimensionsDialog.MakeEvent();

    case ID_PCB_ZONES_BUTT:
        return COMMON_ACTIONS::drawZone.MakeEvent();

    case ID_PCB_KEEPOUT_AREA_BUTT:
        return COMMON_ACTIONS::drawKeepout.MakeEvent();

    case ID_PCB_ADD_LINE_BUTT:
    case ID_MODEDIT_LINE_TOOL:
        return COMMON_ACTIONS::drawLine.MakeEvent();

    case ID_PCB_CIRCLE_BUTT:
    case ID_MODEDIT_CIRCLE_TOOL:
        return COMMON_ACTIONS::drawCircle.MakeEvent();

    case ID_PCB_ARC_BUTT:
    case ID_MODEDIT_ARC_TOOL:
        return COMMON_ACTIONS::drawArc.MakeEvent();

    case ID_PCB_ADD_TEXT_BUTT:
    case ID_MODEDIT_TEXT_TOOL:
        return COMMON_ACTIONS::placeText.MakeEvent();

    case ID_PCB_DIMENSION_BUTT:
        return COMMON_ACTIONS::drawDimension.MakeEvent();

    case ID_PCB_MIRE_BUTT:
        return COMMON_ACTIONS::placeTarget.MakeEvent();

    case ID_MODEDIT_PAD_TOOL:
        return COMMON_ACTIONS::placePad.MakeEvent();

    case ID_GEN_IMPORT_DXF_FILE:
        return COMMON_ACTIONS::placeDXF.MakeEvent();

    case ID_MODEDIT_ANCHOR_TOOL:
        return COMMON_ACTIONS::setAnchor.MakeEvent();

    case ID_PCB_PLACE_GRID_COORD_BUTT:
    case ID_MODEDIT_PLACE_GRID_COORD:
        return COMMON_ACTIONS::gridSetOrigin.MakeEvent();

    case ID_ZOOM_IN:        // toolbar button "Zoom In"
        return COMMON_ACTIONS::zoomInCenter.MakeEvent();

    case ID_ZOOM_OUT:       // toolbar button "Zoom In"
        return COMMON_ACTIONS::zoomOutCenter.MakeEvent();

    case ID_ZOOM_PAGE:      // toolbar button "Fit on Screen"
        return COMMON_ACTIONS::zoomFitScreen.MakeEvent();

    case ID_TB_OPTIONS_SHOW_TRACKS_SKETCH:
        return COMMON_ACTIONS::trackDisplayMode.MakeEvent();

    case ID_TB_OPTIONS_SHOW_PADS_SKETCH:
        return COMMON_ACTIONS::padDisplayMode.MakeEvent();

    case ID_TB_OPTIONS_SHOW_VIAS_SKETCH:
        return COMMON_ACTIONS::viaDisplayMode.MakeEvent();

    case ID_TB_OPTIONS_SHOW_ZONES:
        return COMMON_ACTIONS::zoneDisplayEnable.MakeEvent();

    case ID_TB_OPTIONS_SHOW_ZONES_DISABLE:
        return COMMON_ACTIONS::zoneDisplayDisable.MakeEvent();

    case ID_TB_OPTIONS_SHOW_ZONES_OUTLINES_ONLY:
        return COMMON_ACTIONS::zoneDisplayOutlines.MakeEvent();

    case ID_TB_OPTIONS_SHOW_MODULE_EDGE_SKETCH:
        return COMMON_ACTIONS::moduleEdgeOutlines.MakeEvent();

    case ID_TB_OPTIONS_SHOW_MODULE_TEXT_SKETCH:
        return COMMON_ACTIONS::moduleTextOutlines.MakeEvent();

    case ID_TB_OPTIONS_SHOW_HIGH_CONTRAST_MODE:
        return COMMON_ACTIONS::highContrastMode.MakeEvent();

    case ID_TB_OPTIONS_SELECT_CURSOR:
        return COMMON_ACTIONS::switchCursor.MakeEvent();

    case ID_FIND_ITEMS:
        return COMMON_ACTIONS::find.MakeEvent();

    case ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST:
        return COMMON_ACTIONS::findMove.MakeEvent();

    case ID_NO_TOOL_SELECTED:
        return COMMON_ACTIONS::selectionTool.MakeEvent();

    case ID_PCB_DELETE_ITEM_BUTT:
    case ID_PCB_HIGHLIGHT_BUTT:
    case ID_PCB_SHOW_1_RATSNEST_BUTT:
    case ID_PCB_PLACE_OFFSET_COORD_BUTT:
    case ID_TB_OPTIONS_SHOW_MODULE_RATSNEST:
    case ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR_MICROWAVE:
    case ID_MENU_PCB_SHOW_HIDE_MUWAVE_TOOLBAR:
    case ID_MICROWAVE_V_TOOLBAR:
    case ID_MODEDIT_DELETE_TOOL:
        return COMMON_ACTIONS::toBeDone.MakeEvent();
    }

    return boost::optional<TOOL_EVENT>();
}
