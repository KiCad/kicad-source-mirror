/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "tool/tool_event.h"
#include <pcbnew_id.h>
#include <bitmaps.h>
#include <layers_id_colors_and_visibility.h>
#include <microwave/microwave_tool.h>
#include <tool/tool_manager.h>
#include <router/pns_router.h>
#include <router/pns_routing_settings.h>


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


// CONVERT_TOOL
//
TOOL_ACTION PCB_ACTIONS::convertToPoly( "pcbnew.Convert.convertToPoly",
        AS_GLOBAL, 0, "",
        _( "Convert to Polygon" ), _( "Creates a graphic polygon from the selection" ),
        BITMAPS::add_graphical_polygon );

TOOL_ACTION PCB_ACTIONS::convertToZone( "pcbnew.Convert.convertToZone",
        AS_GLOBAL, 0, "",
        _( "Convert to Zone" ), _( "Creates a copper zone from the selection" ),
        BITMAPS::add_zone );

TOOL_ACTION PCB_ACTIONS::convertToKeepout( "pcbnew.Convert.convertToKeepout",
        AS_GLOBAL, 0, "",
        _( "Convert to Rule Area" ), _( "Creates a rule area from the selection" ),
        BITMAPS::add_keepout_area );

TOOL_ACTION PCB_ACTIONS::convertToLines( "pcbnew.Convert.convertToLines",
        AS_GLOBAL, 0, "",
        _( "Convert to Lines" ), _( "Creates graphic lines from the selection" ),
        BITMAPS::add_line );

TOOL_ACTION PCB_ACTIONS::convertToArc( "pcbnew.Convert.convertToArc",
        AS_GLOBAL, 0, "",
        _( "Convert to Arc" ), _( "Converts selected line segment to an arc" ),
        BITMAPS::add_arc );

TOOL_ACTION PCB_ACTIONS::convertToTracks( "pcbnew.Convert.convertToTracks",
        AS_GLOBAL, 0, "",
        _( "Convert to Tracks" ), _( "Converts selected graphic lines to tracks" ),
        BITMAPS::add_tracks );


// DRAWING_TOOL
//
TOOL_ACTION PCB_ACTIONS::drawLine( "pcbnew.InteractiveDrawing.line",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'L', LEGACY_HK_NAME( "Draw Line" ),
        _( "Draw Line" ), _( "Draw a line" ),
        BITMAPS::add_graphical_segments, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawPolygon( "pcbnew.InteractiveDrawing.graphicPolygon",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'P', LEGACY_HK_NAME( "Draw Graphic Polygon" ),
        _( "Draw Graphic Polygon" ), _( "Draw a graphic polygon" ),
        BITMAPS::add_graphical_polygon, AF_ACTIVATE, (void*) ZONE_MODE::GRAPHIC_POLYGON );

TOOL_ACTION PCB_ACTIONS::drawRectangle( "pcbnew.InteractiveDrawing.rectangle",
        AS_GLOBAL, 0, "",
        _( "Draw Rectangle" ), _( "Draw a rectangle" ),
        BITMAPS::add_rectangle, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawCircle( "pcbnew.InteractiveDrawing.circle",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'C', LEGACY_HK_NAME( "Draw Circle" ),
        _( "Draw Circle" ), _( "Draw a circle" ),
        BITMAPS::add_circle, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawArc( "pcbnew.InteractiveDrawing.arc",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'A', LEGACY_HK_NAME( "Draw Arc" ),
        _( "Draw Arc" ), _( "Draw an arc" ),
        BITMAPS::add_arc, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::placeCharacteristics( "pcbnew.InteractiveDrawing.placeCharacteristics",
        AS_GLOBAL, 0,  LEGACY_HK_NAME( "Add Board Characteristics" ),
        _( "Add Board Characteristics" ),
        _( "Add a board characteristics table on a graphic layer" ),
        BITMAPS::config, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::placeStackup( "pcbnew.InteractiveDrawing.placeStackup", AS_GLOBAL, 0,
        LEGACY_HK_NAME( "Add Stackup Table" ),
        _( "Add Stackup Table" ),
        _( "Add a board stackup table on a graphic layer" ),
        BITMAPS::layers_manager, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::placeText( "pcbnew.InteractiveDrawing.text",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'T', LEGACY_HK_NAME( "Add Text" ),
        _( "Add Text" ), _( "Add a text item" ),
        BITMAPS::text, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawAlignedDimension( "pcbnew.InteractiveDrawing.alignedDimension",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'H', LEGACY_HK_NAME(  "Add Dimension" ),
        _( "Add Aligned Dimension" ), _( "Add an aligned linear dimension" ),
        BITMAPS::add_aligned_dimension, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawCenterDimension( "pcbnew.InteractiveDrawing.centerDimension",
        AS_GLOBAL, 0, "",
        _( "Add Center Dimension" ), _( "Add a center dimension" ),
        BITMAPS::add_center_dimension, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawOrthogonalDimension( "pcbnew.InteractiveDrawing.orthogonalDimension",
        AS_GLOBAL, 0, "",
        _( "Add Orthogonal Dimension" ), _( "Add an orthogonal dimension" ),
        BITMAPS::add_orthogonal_dimension, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawLeader( "pcbnew.InteractiveDrawing.leader",
        AS_GLOBAL, 0, "",
        _( "Add Leader" ), _( "Add a leader dimension" ),
        BITMAPS::add_leader, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawZone( "pcbnew.InteractiveDrawing.zone",
        AS_GLOBAL,
#ifdef __WXOSX_MAC__
        MD_ALT + 'Z',
#else
        MD_SHIFT + MD_CTRL + 'Z',
#endif
        LEGACY_HK_NAME( "Add Filled Zone" ),
        _( "Add Filled Zone" ), _( "Add a filled zone" ),
        BITMAPS::add_zone, AF_ACTIVATE, (void*) ZONE_MODE::ADD );

TOOL_ACTION PCB_ACTIONS::drawVia( "pcbnew.InteractiveDrawing.via",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'V', LEGACY_HK_NAME( "Add Vias" ),
        _( "Add Vias" ), _( "Add free-standing vias" ),
        BITMAPS::add_via, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawRuleArea( "pcbnew.InteractiveDrawing.ruleArea",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'K', LEGACY_HK_NAME( "Add Keepout Area" ),
        _( "Add Rule Area" ), _( "Add a rule area (keepout)" ),
        BITMAPS::add_keepout_area, AF_ACTIVATE, (void*) ZONE_MODE::ADD );

TOOL_ACTION PCB_ACTIONS::drawZoneCutout( "pcbnew.InteractiveDrawing.zoneCutout",
        AS_GLOBAL,
        MD_SHIFT + 'C', LEGACY_HK_NAME( "Add a Zone Cutout" ),
        _( "Add a Zone Cutout" ), _( "Add a cutout area of an existing zone" ),
        BITMAPS::add_zone_cutout, AF_ACTIVATE, (void*) ZONE_MODE::CUTOUT );

TOOL_ACTION PCB_ACTIONS::drawSimilarZone( "pcbnew.InteractiveDrawing.similarZone",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + '.', LEGACY_HK_NAME( "Add a Similar Zone" ),
        _( "Add a Similar Zone" ), _( "Add a zone with the same settings as an existing zone" ),
        BITMAPS::add_zone, AF_ACTIVATE, (void*) ZONE_MODE::SIMILAR );

TOOL_ACTION PCB_ACTIONS::placeImportedGraphics( "pcbnew.InteractiveDrawing.placeImportedGraphics",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'F', LEGACY_HK_NAME( "Place DXF" ),
        _( "Import Graphics..." ), _( "Import 2D drawing file" ),
        BITMAPS::import_vector, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::setAnchor( "pcbnew.InteractiveDrawing.setAnchor",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'N', LEGACY_HK_NAME( "Place the Footprint Anchor" ),
        _( "Place the Footprint Anchor" ), _( "Set the coordinate origin point (anchor) of the footprint" ),
        BITMAPS::anchor, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::incWidth( "pcbnew.InteractiveDrawing.incWidth",
        AS_CONTEXT,
        MD_CTRL + '+', LEGACY_HK_NAME( "Increase Line Width" ),
        _( "Increase Line Width" ), _( "Increase the line width" ) );

TOOL_ACTION PCB_ACTIONS::decWidth( "pcbnew.InteractiveDrawing.decWidth",
        AS_CONTEXT,
        MD_CTRL + '-', LEGACY_HK_NAME( "Decrease Line Width" ),
        _( "Decrease Line Width" ), _( "Decrease the line width" ) );

TOOL_ACTION PCB_ACTIONS::arcPosture( "pcbnew.InteractiveDrawing.arcPosture",
        AS_CONTEXT,
        '/', LEGACY_HK_NAME( "Switch Track Posture" ),
        _( "Switch Arc Posture" ), _( "Switch the arc posture" ) );

TOOL_ACTION PCB_ACTIONS::deleteLastPoint( "pcbnew.InteractiveDrawing.deleteLastPoint",
        AS_CONTEXT,
        WXK_BACK, "",
        _( "Delete Last Point" ), _( "Delete the last point added to the current item" ),
        BITMAPS::undo );

TOOL_ACTION PCB_ACTIONS::closeOutline( "pcbnew.InteractiveDrawing.closeOutline",
        AS_CONTEXT, 0, "",
        _( "Close Outline" ), _( "Close the in progress outline" ),
        BITMAPS::checked_ok );

TOOL_ACTION PCB_ACTIONS::toggleLine45degMode( "pcbnew.InteractiveDrawing.line45degMode",
        AS_GLOBAL, 0, "",
        _( "Limit Lines to 45 deg" ), _( "Limit graphic lines to H, V and 45 degrees" ),
        BITMAPS::INVALID_BITMAP, AF_NONE );

// DRC
//
TOOL_ACTION PCB_ACTIONS::runDRC( "pcbnew.DRCTool.runDRC",
        AS_GLOBAL, 0, "",
        _( "Design Rules Checker" ), _( "Show the design rules checker window" ),
        BITMAPS::erc );


// EDIT_TOOL
//
TOOL_ACTION PCB_ACTIONS::editFpInFpEditor( "pcbnew.EditorControl.EditFpInFpEditor",
        AS_GLOBAL,
        MD_CTRL + 'E', LEGACY_HK_NAME( "Edit with Footprint Editor" ),
        _( "Open in Footprint Editor" ),
        _( "Opens the selected footprint in the Footprint Editor" ),
        BITMAPS::module_editor );

TOOL_ACTION PCB_ACTIONS::getAndPlace( "pcbnew.InteractiveEdit.FindMove",
        AS_GLOBAL,
        'T', LEGACY_HK_NAME( "Get and Move Footprint" ),
        _( "Get and Move Footprint" ),
        _( "Selects a footprint by reference designator and places it under the cursor for moving"),
        BITMAPS::move, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::move( "pcbnew.InteractiveMove.move",
        AS_GLOBAL,
        'M', LEGACY_HK_NAME( "Move Item" ),
        _( "Move" ), _( "Moves the selected item(s)" ),
        BITMAPS::move, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::moveWithReference( "pcbnew.InteractiveMove.moveWithReference",
        AS_GLOBAL, 0, "",
        _( "Move with Reference" ),
        _( "Moves the selected item(s) with a specified starting point" ),
        BITMAPS::move, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::copyWithReference( "pcbnew.InteractiveMove.copyWithReference",
        AS_GLOBAL, 0, "",
        _( "Copy with Reference" ),
        _( "Copy selected item(s) to clipboard with a specified starting point" ),
        BITMAPS::copy, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::duplicateIncrement( "pcbnew.InteractiveEdit.duplicateIncrementPads",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'D', LEGACY_HK_NAME( "Duplicate Item and Increment" ),
        _( "Duplicate and Increment" ), _( "Duplicates the selected item(s), incrementing pad numbers" ),
        BITMAPS::duplicate );

TOOL_ACTION PCB_ACTIONS::moveExact( "pcbnew.InteractiveEdit.moveExact",
        AS_GLOBAL,
        MD_CTRL + 'M', LEGACY_HK_NAME( "Move Item Exactly" ),
        _( "Move Exactly..." ), _( "Moves the selected item(s) by an exact amount" ),
        BITMAPS::move_exactly );

TOOL_ACTION PCB_ACTIONS::createArray( "pcbnew.InteractiveEdit.createArray",
        AS_GLOBAL,
        MD_CTRL + 'T', LEGACY_HK_NAME( "Create Array" ),
        _( "Create Array..." ), _( "Create array" ),
        BITMAPS::array, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::rotateCw( "pcbnew.InteractiveEdit.rotateCw",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        MD_SHIFT + 'R', LEGACY_HK_NAME( "Rotate Item Clockwise (Modern Toolset only)" ),
        _( "Rotate Clockwise" ), _( "Rotates selected item(s) clockwise" ),
        BITMAPS::rotate_cw, AF_NONE, (void*) -1 );

TOOL_ACTION PCB_ACTIONS::rotateCcw( "pcbnew.InteractiveEdit.rotateCcw",
        AS_GLOBAL,
        'R', LEGACY_HK_NAME( "Rotate Item" ),
        _( "Rotate Counterclockwise" ), _( "Rotates selected item(s) counterclockwise" ),
        BITMAPS::rotate_ccw, AF_NONE, (void*) 1 );

TOOL_ACTION PCB_ACTIONS::flip( "pcbnew.InteractiveEdit.flip",
        AS_GLOBAL,
        'F', LEGACY_HK_NAME( "Flip Item" ),
        _( "Change Side / Flip" ), _( "Flips selected item(s) to opposite side of board" ),
        BITMAPS::swap_layer );

TOOL_ACTION PCB_ACTIONS::mirror( "pcbnew.InteractiveEdit.mirror",
        AS_GLOBAL, 0, "",
        _( "Mirror" ), _( "Mirrors selected item" ),
        BITMAPS::mirror_h );

TOOL_ACTION PCB_ACTIONS::changeTrackWidth( "pcbnew.InteractiveEdit.changeTrackWidth",
        AS_GLOBAL, 0, "",
        _( "Change Track Width" ), _( "Updates selected track & via sizes" ) );

TOOL_ACTION PCB_ACTIONS::filletTracks( "pcbnew.InteractiveEdit.filletTracks",
        AS_GLOBAL, 0, "",
        _( "Fillet Tracks" ), _( "Adds arcs tangent to the selected straight track segments" ) );

TOOL_ACTION PCB_ACTIONS::deleteFull( "pcbnew.InteractiveEdit.deleteFull",
        AS_GLOBAL,
        MD_SHIFT + WXK_DELETE, LEGACY_HK_NAME( "Delete Full Track" ),
        _( "Delete Full Track" ), _( "Deletes selected item(s) and copper connections" ),
        BITMAPS::delete_cursor, AF_NONE, (void*) REMOVE_FLAGS::ALT );

TOOL_ACTION PCB_ACTIONS::properties( "pcbnew.InteractiveEdit.properties",
        AS_GLOBAL,
        'E', LEGACY_HK_NAME( "Edit Item" ),
        _( "Properties..." ), _( "Displays item properties dialog" ),
        BITMAPS::edit );


// FOOTPRINT_EDITOR_CONTROL
//
TOOL_ACTION PCB_ACTIONS::showFootprintTree( "pcbnew.ModuleEditor.showFootprintTree",
        AS_GLOBAL, 0, "",
        _( "Show Footprint Tree" ), "",
        BITMAPS::search_tree );

TOOL_ACTION PCB_ACTIONS::hideFootprintTree( "pcbnew.ModuleEditor.hideFootprintTree",
        AS_GLOBAL, 0, "",
        _( "Hide Footprint Tree" ), "",
        BITMAPS::search_tree );

TOOL_ACTION PCB_ACTIONS::newFootprint( "pcbnew.ModuleEditor.newFootprint",
        AS_GLOBAL,
        MD_CTRL + 'N', LEGACY_HK_NAME( "New" ),
        _( "New Footprint..." ), _( "Create a new, empty footprint" ),
        BITMAPS::new_footprint );

TOOL_ACTION PCB_ACTIONS::createFootprint( "pcbnew.ModuleEditor.createFootprint",
        AS_GLOBAL, 0, "",
        _( "Create Footprint..." ), _( "Create a new footprint using the Footprint Wizard" ),
        BITMAPS::module_wizard );

TOOL_ACTION PCB_ACTIONS::editFootprint( "pcbnew.ModuleEditor.editFootprint",
        AS_GLOBAL, 0, "",
        _( "Edit Footprint" ), _( "Show selected footprint on editor canvas" ),
        BITMAPS::edit );

TOOL_ACTION PCB_ACTIONS::deleteFootprint( "pcbnew.ModuleEditor.deleteFootprint",
        AS_GLOBAL, 0, "",
        _( "Delete Footprint from Library" ), "",
        BITMAPS::trash );

TOOL_ACTION PCB_ACTIONS::cutFootprint( "pcbnew.ModuleEditor.cutFootprint",
        AS_GLOBAL, 0, "",
        _( "Cut Footprint" ), "",
        BITMAPS::cut );

TOOL_ACTION PCB_ACTIONS::copyFootprint( "pcbnew.ModuleEditor.copyFootprint",
        AS_GLOBAL, 0, "",
        _( "Copy Footprint" ), "",
        BITMAPS::copy );

TOOL_ACTION PCB_ACTIONS::pasteFootprint( "pcbnew.ModuleEditor.pasteFootprint",
        AS_GLOBAL, 0, "",
        _( "Paste Footprint" ), "",
        BITMAPS::paste );

TOOL_ACTION PCB_ACTIONS::importFootprint( "pcbnew.ModuleEditor.importFootprint",
        AS_GLOBAL, 0, "",
        _( "Import Footprint..." ), _( "Import footprint from file" ),
        BITMAPS::import_module );

TOOL_ACTION PCB_ACTIONS::exportFootprint( "pcbnew.ModuleEditor.exportFootprint",
        AS_GLOBAL, 0, "",
        _( "Export Footprint..." ), _( "Export footprint to file" ),
        BITMAPS::export_module );

TOOL_ACTION PCB_ACTIONS::footprintProperties( "pcbnew.ModuleEditor.footprintProperties",
        AS_GLOBAL, 0, "",
        _( "Footprint Properties..." ), _( "Edit footprint properties" ),
        BITMAPS::module_options );

TOOL_ACTION PCB_ACTIONS::checkFootprint( "pcbnew.ModuleEditor.checkFootprint",
        AS_GLOBAL, 0, "",
        _( "Footprint Checker" ), _( "Show the footprint checker window" ),
        BITMAPS::erc );

// GLOBAL_EDIT_TOOL
//
TOOL_ACTION PCB_ACTIONS::updateFootprint( "pcbnew.GlobalEdit.updateFootprint",
        AS_GLOBAL, 0, "",
        _( "Update Footprint..." ),
        _( "Update footprint to include any changes from the library" ),
        BITMAPS::refresh );

TOOL_ACTION PCB_ACTIONS::updateFootprints( "pcbnew.GlobalEdit.updateFootprints",
        AS_GLOBAL, 0, "",
        _( "Update Footprints from Library..." ),
        _( "Update footprints to include any changes from the library" ),
        BITMAPS::refresh );

TOOL_ACTION PCB_ACTIONS::removeUnusedPads( "pcbnew.GlobalEdit.removeUnusedPads",
        AS_GLOBAL, 0, "",
        _( "Remove Unused Pads..." ),
        _( "Remove or restore the unconnected inner layers on through hole pads and vias" ),
        BITMAPS::pads_remove );

TOOL_ACTION PCB_ACTIONS::changeFootprint( "pcbnew.GlobalEdit.changeFootprint",
        AS_GLOBAL, 0, "",
        _( "Change Footprint..." ), _( "Assign a different footprint from the library" ),
        BITMAPS::exchange );

TOOL_ACTION PCB_ACTIONS::changeFootprints( "pcbnew.GlobalEdit.changeFootprints",
        AS_GLOBAL, 0, "",
        _( "Change Footprints..." ), _( "Assign different footprints from the library" ),
        BITMAPS::exchange );

TOOL_ACTION PCB_ACTIONS::swapLayers( "pcbnew.GlobalEdit.swapLayers",
        AS_GLOBAL, 0, "",
        _( "Swap Layers..." ), _( "Move tracks or drawings from one layer to another" ),
        BITMAPS::swap_layer );

TOOL_ACTION PCB_ACTIONS::editTracksAndVias( "pcbnew.GlobalEdit.editTracksAndVias",
        AS_GLOBAL, 0, "",
        _( "Edit Track & Via Properties..." ),
        _( "Edit track and via properties globally across board" ),
        BITMAPS::width_track_via );

TOOL_ACTION PCB_ACTIONS::editTextAndGraphics( "pcbnew.GlobalEdit.editTextAndGraphics",
        AS_GLOBAL, 0, "",
        _( "Edit Text & Graphics Properties..." ),
        _( "Edit Text and graphics properties globally across board" ),
        BITMAPS::text );

TOOL_ACTION PCB_ACTIONS::globalDeletions( "pcbnew.GlobalEdit.globalDeletions",
        AS_GLOBAL, 0, "",
        _( "Global Deletions..." ),
        _( "Delete tracks, footprints and graphic items from board" ),
        BITMAPS::general_deletions );

TOOL_ACTION PCB_ACTIONS::cleanupTracksAndVias( "pcbnew.GlobalEdit.cleanupTracksAndVias",
        AS_GLOBAL, 0, "",
        _( "Cleanup Tracks & Vias..." ),
        _( "Cleanup redundant items, shorting items, etc." ),
        BITMAPS::delete_cursor );

TOOL_ACTION PCB_ACTIONS::cleanupGraphics( "pcbnew.GlobalEdit.cleanupGraphics",
        AS_GLOBAL, 0, "",
        _( "Cleanup Graphics..." ),
        _( "Cleanup redundant items, etc." ),
        BITMAPS::delete_cursor );

// MICROWAVE_TOOL
//
TOOL_ACTION PCB_ACTIONS::microwaveCreateGap( "pcbnew.MicrowaveTool.createGap",
        AS_GLOBAL, 0, "",
        _( "Add Microwave Gap" ), _( "Create gap of specified length for microwave applications" ),
        BITMAPS::mw_add_gap, AF_ACTIVATE, (void*) MICROWAVE_FOOTPRINT_SHAPE::GAP );

TOOL_ACTION PCB_ACTIONS::microwaveCreateStub( "pcbnew.MicrowaveTool.createStub",
        AS_GLOBAL, 0, "",
        _( "Add Microwave Stub" ), _( "Create stub of specified length for microwave applications" ),
        BITMAPS::mw_add_stub, AF_ACTIVATE, (void*) MICROWAVE_FOOTPRINT_SHAPE::STUB );

TOOL_ACTION PCB_ACTIONS::microwaveCreateStubArc( "pcbnew.MicrowaveTool.createStubArc",
        AS_GLOBAL, 0, "",
        _( "Add Microwave Arc Stub" ), _( "Create stub (arc) of specified size for microwave applications" ),
        BITMAPS::mw_add_stub_arc, AF_ACTIVATE, (void*) MICROWAVE_FOOTPRINT_SHAPE::STUB_ARC );

TOOL_ACTION PCB_ACTIONS::microwaveCreateFunctionShape( "pcbnew.MicrowaveTool.createFunctionShape",
        AS_GLOBAL, 0, "",
        _( "Add Microwave Polygonal Shape" ), _( "Create a microwave polygonal shape from a list of vertices" ),
        BITMAPS::mw_add_shape, AF_ACTIVATE, (void*) MICROWAVE_FOOTPRINT_SHAPE::FUNCTION_SHAPE );

TOOL_ACTION PCB_ACTIONS::microwaveCreateLine( "pcbnew.MicrowaveTool.createLine",
        AS_GLOBAL, 0, "",
        _( "Add Microwave Line" ), _( "Create line of specified length for microwave applications" ),
        BITMAPS::mw_add_line, AF_ACTIVATE );


// PAD_TOOL
//
TOOL_ACTION PCB_ACTIONS::copyPadSettings( "pcbnew.PadTool.CopyPadSettings",
        AS_GLOBAL, 0, "",
        _( "Copy Pad Properties to Default" ), _( "Copy current pad's properties" ),
        BITMAPS::copy_pad_settings );

TOOL_ACTION PCB_ACTIONS::applyPadSettings( "pcbnew.PadTool.ApplyPadSettings",
        AS_GLOBAL, 0, "",
        _( "Paste Default Pad Properties to Selected" ),
        _( "Replace the current pad's properties with those copied earlier" ),
        BITMAPS::apply_pad_settings );

TOOL_ACTION PCB_ACTIONS::pushPadSettings( "pcbnew.PadTool.PushPadSettings",
        AS_GLOBAL, 0, "",
        _( "Push Pad Properties to Other Pads..." ),
        _( "Copy the current pad's properties to other pads" ),
        BITMAPS::push_pad_settings );

TOOL_ACTION PCB_ACTIONS::enumeratePads( "pcbnew.PadTool.enumeratePads",
        AS_GLOBAL, 0, "",
        _( "Renumber Pads..." ),
        _( "Renumber pads by clicking on them in the desired order" ),
        BITMAPS::pad_enumerate, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::placePad( "pcbnew.PadTool.placePad",
        AS_GLOBAL, 0, "",
        _( "Add Pad" ), _( "Add a pad" ),
        BITMAPS::pad, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::explodePad( "pcbnew.PadTool.explodePad",
        AS_GLOBAL,
        MD_CTRL + 'E', "",
        _( "Edit Pad as Graphic Shapes" ),
        _( "Ungroups a custom-shaped pad for editing as individual graphic shapes" ),
        BITMAPS::custom_pad_to_primitives );

TOOL_ACTION PCB_ACTIONS::recombinePad( "pcbnew.PadTool.recombinePad",
        AS_GLOBAL,
        MD_CTRL + 'E', "",
        _( "Finish Pad Edit" ),
        _( "Regroups all touching graphic shapes into the edited pad" ),
        BITMAPS::custom_pad_to_primitives );

TOOL_ACTION PCB_ACTIONS::defaultPadProperties( "pcbnew.PadTool.defaultPadProperties",
        AS_GLOBAL, 0, "",
        _( "Default Pad Properties..." ), _( "Edit the pad properties used when creating new pads" ),
        BITMAPS::options_pad );


// BOARD_EDITOR_CONTROL
//
TOOL_ACTION PCB_ACTIONS::boardSetup( "pcbnew.EditorControl.boardSetup",
        AS_GLOBAL, 0, "",
        _( "Board Setup..." ),
        _( "Edit board setup including layers, design rules and various defaults" ),
        BITMAPS::options_board );

TOOL_ACTION PCB_ACTIONS::importNetlist( "pcbnew.EditorControl.importNetlist",
        AS_GLOBAL, 0, "",
        _( "Import Netlist..." ), _( "Read netlist and update board connectivity" ),
        BITMAPS::netlist );

TOOL_ACTION PCB_ACTIONS::importSpecctraSession( "pcbnew.EditorControl.importSpecctraSession",
        AS_GLOBAL, 0, "",
        _( "Import Specctra Session..." ), _( "Import routed Specctra session (*.ses) file" ),
        BITMAPS::import );

TOOL_ACTION PCB_ACTIONS::exportSpecctraDSN( "pcbnew.EditorControl.exportSpecctraDSN",
        AS_GLOBAL, 0, "",
        _( "Export Specctra DSN..." ), _( "Export Specctra DSN routing info" ),
        BITMAPS::export_dsn );

TOOL_ACTION PCB_ACTIONS::generateGerbers( "pcbnew.EditorControl.generateGerbers",
        AS_GLOBAL, 0, "",
        _( "Gerbers (.gbr)..." ), _( "Generate Gerbers for fabrication" ),
        BITMAPS::post_gerber );

TOOL_ACTION PCB_ACTIONS::generateDrillFiles( "pcbnew.EditorControl.generateDrillFiles",
        AS_GLOBAL, 0, "",
        _( "Drill Files (.drl)..." ), _( "Generate Excellon drill file(s)" ),
        BITMAPS::post_drill );

TOOL_ACTION PCB_ACTIONS::generatePosFile( "pcbnew.EditorControl.generatePosFile",
        AS_GLOBAL, 0, "",
        _( "Component Placement (.pos)..." ),
        _( "Generate component placement file(s) for pick and place" ),
        BITMAPS::post_compo );

TOOL_ACTION PCB_ACTIONS::generateReportFile( "pcbnew.EditorControl.generateReportFile",
        AS_GLOBAL, 0, "",
        _( "Footprint Report (.rpt)..." ),
        _( "Create report of all footprints from current board" ),
        BITMAPS::post_rpt );

TOOL_ACTION PCB_ACTIONS::generateD356File( "pcbnew.EditorControl.generateD356File",
        AS_GLOBAL, 0, "",
        _( "IPC-D-356 Netlist File..." ), _( "Generate IPC-D-356 netlist file" ),
        BITMAPS::post_d356 );

TOOL_ACTION PCB_ACTIONS::generateBOM( "pcbnew.EditorControl.generateBOM",
        AS_GLOBAL, 0, "",
        _( "BOM..." ), _( "Create bill of materials from board" ),
        BITMAPS::post_bom );

// Track & via size control
TOOL_ACTION PCB_ACTIONS::trackWidthInc( "pcbnew.EditorControl.trackWidthInc",
        AS_GLOBAL,
        'W', LEGACY_HK_NAME( "Switch Track Width To Next" ),
        _( "Switch Track Width to Next" ), _( "Change track width to next pre-defined size" ) );

TOOL_ACTION PCB_ACTIONS::trackWidthDec( "pcbnew.EditorControl.trackWidthDec",
        AS_GLOBAL,
        MD_SHIFT + 'W', LEGACY_HK_NAME( "Switch Track Width To Previous" ),
        _( "Switch Track Width to Previous" ), _( "Change track width to previous pre-defined size" ) );

TOOL_ACTION PCB_ACTIONS::viaSizeInc( "pcbnew.EditorControl.viaSizeInc",
        AS_GLOBAL,
        '\'', LEGACY_HK_NAME( "Increase Via Size" ),
        _( "Increase Via Size" ), _( "Change via size to next pre-defined size" ) );

TOOL_ACTION PCB_ACTIONS::viaSizeDec( "pcbnew.EditorControl.viaSizeDec",
        AS_GLOBAL,
        '\\', LEGACY_HK_NAME( "Decrease Via Size" ),
        _( "Decrease Via Size" ), _( "Change via size to previous pre-defined size" ) );

TOOL_ACTION PCB_ACTIONS::trackViaSizeChanged( "pcbnew.EditorControl.trackViaSizeChanged",
        AS_GLOBAL, 0, "",
        "", "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY );

TOOL_ACTION PCB_ACTIONS::zoneMerge( "pcbnew.EditorControl.zoneMerge",
        AS_GLOBAL, 0, "",
        _( "Merge Zones" ), _( "Merge zones" ) );

TOOL_ACTION PCB_ACTIONS::zoneDuplicate( "pcbnew.EditorControl.zoneDuplicate",
        AS_GLOBAL, 0, "",
        _( "Duplicate Zone onto Layer..." ), _( "Duplicate zone outline onto a different layer" ),
        BITMAPS::zone_duplicate );

TOOL_ACTION PCB_ACTIONS::placeTarget( "pcbnew.EditorControl.placeTarget",
        AS_GLOBAL, 0, "",
        _( "Add Layer Alignment Target" ), _( "Add a layer alignment target" ),
        BITMAPS::add_pcb_target, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::placeFootprint( "pcbnew.EditorControl.placeFootprint",
        AS_GLOBAL,
        'O', LEGACY_HK_NAME( "Add Footprint" ),
        _( "Add Footprint" ), _( "Add a footprint" ),
        BITMAPS::module, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drillOrigin( "pcbnew.EditorControl.drillOrigin",
        AS_GLOBAL, 0, "",
        _( "Drill/Place File Origin" ),
        _( "Place origin point for drill files and component placement files" ),
        BITMAPS::set_origin, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::toggleLock( "pcbnew.EditorControl.toggleLock",
        AS_GLOBAL,
        'L', LEGACY_HK_NAME( "Lock/Unlock Footprint" ),
        _( "Toggle Lock" ), _( "Lock or unlock selected items" ),
        BITMAPS::lock_unlock );

TOOL_ACTION PCB_ACTIONS::lock( "pcbnew.EditorControl.lock",
        AS_GLOBAL, 0, "",
        _( "Lock" ), _( "Prevent items from being moved and/or resized on the canvas" ),
        BITMAPS::locked );

TOOL_ACTION PCB_ACTIONS::unlock( "pcbnew.EditorControl.unlock",
        AS_GLOBAL, 0, "",
        _( "Unlock" ), _( "Allow items to be moved and/or resized on the canvas" ),
        BITMAPS::unlocked );

TOOL_ACTION PCB_ACTIONS::group( "pcbnew.EditorControl.group",
        AS_GLOBAL, 0, "",
        _( "Group" ), _( "Group the selected items so that they are treated as a single item" ),
        BITMAPS::group );

TOOL_ACTION PCB_ACTIONS::ungroup( "pcbnew.EditorControl.ungroup",
        AS_GLOBAL, 0, "",
        _( "Ungroup" ), _( "Ungroup any selected groups" ),
        BITMAPS::group_ungroup );

TOOL_ACTION PCB_ACTIONS::removeFromGroup( "pcbnew.EditorControl.removeFromGroup",
        AS_GLOBAL, 0, "",
        _( "Remove Items" ), _( "Remove items from group" ),
        BITMAPS::group_remove );

TOOL_ACTION PCB_ACTIONS::groupEnter( "pcbnew.EditorControl.groupEnter",
        AS_GLOBAL, 0, "",
        _( "Enter Group" ), _( "Enter the group to edit items" ),
        BITMAPS::group_enter );

TOOL_ACTION PCB_ACTIONS::groupLeave( "pcbnew.EditorControl.groupLeave",
        AS_GLOBAL, 0, "",
        _( "Leave Group" ), _( "Leave the current group" ),
        BITMAPS::group_leave );

TOOL_ACTION PCB_ACTIONS::appendBoard( "pcbnew.EditorControl.appendBoard",
        AS_GLOBAL, 0, "",
        _( "Append Board..." ), _( "Open another board and append its contents to this board" ),
        BITMAPS::add_board );

TOOL_ACTION PCB_ACTIONS::highlightNet( "pcbnew.EditorControl.highlightNet",
        AS_GLOBAL, 0, "", _( "Highlight Net" ), _( "Highlight the selected net" ),
        BITMAPS::net_highlight );

TOOL_ACTION PCB_ACTIONS::toggleLastNetHighlight( "pcbnew.EditorControl.toggleLastNetHighlight",
        AS_GLOBAL, 0, "",
        _( "Toggle Last Net Highlight" ), _( "Toggle between last two highlighted nets" ) );

TOOL_ACTION PCB_ACTIONS::clearHighlight( "pcbnew.EditorControl.clearHighlight",
        AS_GLOBAL, 0, "",
        _( "Clear Net Highlighting" ), _( "Clear any existing net highlighting" ) );

TOOL_ACTION PCB_ACTIONS::highlightNetTool( "pcbnew.EditorControl.highlightNetTool",
        AS_GLOBAL, 0, "",
        _( "Highlight Nets" ), _( "Highlight all copper items of a net" ),
        BITMAPS::net_highlight, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::highlightNetSelection( "pcbnew.EditorControl.highlightNetSelection",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        '`', LEGACY_HK_NAME( "Toggle Highlight of Selected Net (Modern Toolset only)" ),
        _( "Highlight Net" ), _( "Highlight all copper items of a net" ),
        BITMAPS::net_highlight );

TOOL_ACTION PCB_ACTIONS::highlightItem( "pcbnew.EditorControl.highlightItem",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::hideNet( "pcbnew.EditorControl.hideNet", AS_GLOBAL, 0, "",
        _( "Hide Net" ), _( "Hide the ratsnest for the selected net" ),
        BITMAPS::hide_ratsnest );

TOOL_ACTION PCB_ACTIONS::showNet( "pcbnew.EditorControl.showNet", AS_GLOBAL, 0, "",
        _( "Show Net" ), _( "Show the ratsnest for the selected net" ),
        BITMAPS::show_ratsnest );

TOOL_ACTION PCB_ACTIONS::showEeschema( "pcbnew.EditorControl.showEeschema",
        AS_GLOBAL, 0, "",
        _( "Switch to Schematic Editor" ), _( "Open schematic in Eeschema" ),
        BITMAPS::icon_eeschema_24 );


// PCB_CONTROL
//

TOOL_ACTION PCB_ACTIONS::localRatsnestTool( "pcbnew.Control.localRatsnestTool",
        AS_GLOBAL, 0, "",
        _( "Highlight Ratsnest" ), _( "Show ratsnest of selected item(s)" ),
        BITMAPS::tool_ratsnest, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::hideDynamicRatsnest( "pcbnew.Control.hideDynamicRatsnest",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::updateLocalRatsnest( "pcbnew.Control.updateLocalRatsnest",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::listNets( "pcbnew.Control.listNets",
        AS_GLOBAL, 0, "",
        _( "Net Inspector" ), _( "Show the net inspector" ),
        BITMAPS::list_nets );

TOOL_ACTION PCB_ACTIONS::showPythonConsole( "pcbnew.Control.showPythonConsole",
        AS_GLOBAL, 0, "",
        _( "Scripting Console" ), _( "Show the Python scripting console" ),
        BITMAPS::py_script );

TOOL_ACTION PCB_ACTIONS::showLayersManager( "pcbnew.Control.showLayersManager",
        AS_GLOBAL, 0, "",
        _( "Show Appearance Manager" ), _( "Show/hide the appearance manager" ),
        BITMAPS::layers_manager );

TOOL_ACTION PCB_ACTIONS::flipBoard( "pcbnew.Control.flipBoard",
        AS_GLOBAL, 0, "",
        _( "Flip Board View" ), _( "View board from the opposite side" ),
        BITMAPS::flip_board );

// Display modes
TOOL_ACTION PCB_ACTIONS::showRatsnest( "pcbnew.Control.showRatsnest",
        AS_GLOBAL, 0, "",
        _( "Show Ratsnest" ), _( "Show board ratsnest" ),
        BITMAPS::general_ratsnest );

TOOL_ACTION PCB_ACTIONS::ratsnestLineMode( "pcbnew.Control.ratsnestLineMode",
        AS_GLOBAL, 0, "",
        _( "Curved Ratsnest Lines" ), _( "Show ratsnest with curved lines" ),
        BITMAPS::curved_ratsnest );

TOOL_ACTION PCB_ACTIONS::trackDisplayMode( "pcbnew.Control.trackDisplayMode",
        AS_GLOBAL,
        'K', LEGACY_HK_NAME( "Track Display Mode" ),
        _( "Sketch Tracks" ), _( "Show tracks in outline mode" ),
        BITMAPS::showtrack );

TOOL_ACTION PCB_ACTIONS::padDisplayMode( "pcbnew.Control.padDisplayMode",
        AS_GLOBAL, 0, "",
        _( "Sketch Pads" ), _( "Show pads in outline mode" ),
        BITMAPS::pad_sketch );

TOOL_ACTION PCB_ACTIONS::viaDisplayMode( "pcbnew.Control.viaDisplayMode",
        AS_GLOBAL, 0, "",
        _( "Sketch Vias" ), _( "Show vias in outline mode" ),
        BITMAPS::via_sketch );

TOOL_ACTION PCB_ACTIONS::graphicsOutlines( "pcbnew.Control.graphicOutlines",
        AS_GLOBAL, 0, "",
        _( "Sketch Graphic Items" ), _( "Show graphic items in outline mode" ),
        BITMAPS::show_mod_edge );

TOOL_ACTION PCB_ACTIONS::textOutlines( "pcbnew.Control.textOutlines",
        AS_GLOBAL, 0, "",
        _( "Sketch Text Items" ), _( "Show footprint texts in line mode" ),
        BITMAPS::text_sketch );

TOOL_ACTION PCB_ACTIONS::showPadNumbers( "pcbnew.Control.showPadNumbers",
        AS_GLOBAL, 0, "",
        _( "Show pad numbers" ), _( "Show pad numbers" ),
        BITMAPS::pad_number );

TOOL_ACTION PCB_ACTIONS::zoomFootprintAutomatically( "pcbnew.Control.zoomFootprintAutomatically",
        AS_GLOBAL, 0, "",
        _( "Automatically zoom to fit" ), _( "Zoom to fit when changing footprint" ),
        BITMAPS::zoom_auto_fit_in_page );

TOOL_ACTION PCB_ACTIONS::zoneDisplayEnable( "pcbnew.Control.zoneDisplayEnable",
        AS_GLOBAL, 0, "",
        _( "Fill Zones" ), _( "Show filled areas of zones" ),
        BITMAPS::show_zone );

TOOL_ACTION PCB_ACTIONS::zoneDisplayDisable( "pcbnew.Control.zoneDisplayDisable",
        AS_GLOBAL, 0, "",
        _( "Wireframe Zones" ), _( "Show only zone boundaries" ),
        BITMAPS::show_zone_disable );

TOOL_ACTION PCB_ACTIONS::zoneDisplayOutlines( "pcbnew.Control.zoneDisplayOutlines",
        AS_GLOBAL, 0, "",
        _( "Sketch Zones" ), _( "Show solid areas of zones in outline mode" ),
        BITMAPS::show_zone_outline_only );

TOOL_ACTION PCB_ACTIONS::zoneDisplayToggle( "pcbnew.Control.zoneDisplayToggle",
        AS_GLOBAL,
        'A', "",
        _( "Toggle Zone Display" ),
        _( "Cycle between showing filled zones, wireframed zones and sketched zones" ),
        BITMAPS::show_zone );


// Layer control
TOOL_ACTION PCB_ACTIONS::layerTop( "pcbnew.Control.layerTop",
        AS_GLOBAL,
        WXK_PAGEUP, LEGACY_HK_NAME( "Switch to Component (F.Cu) layer" ),
        _( "Switch to Component (F.Cu) layer" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) F_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner1( "pcbnew.Control.layerInner1",
        AS_GLOBAL,
        0, LEGACY_HK_NAME( "Switch to Inner layer 1" ),
        _( "Switch to Inner layer 1" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In1_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner2( "pcbnew.Control.layerInner2",
        AS_GLOBAL,
        0, LEGACY_HK_NAME( "Switch to Inner layer 2" ),
        _( "Switch to Inner layer 2" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In2_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner3( "pcbnew.Control.layerInner3",
        AS_GLOBAL,
        0, LEGACY_HK_NAME( "Switch to Inner layer 3" ),
        _( "Switch to Inner layer 3" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In3_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner4( "pcbnew.Control.layerInner4",
        AS_GLOBAL,
        0, LEGACY_HK_NAME( "Switch to Inner layer 4" ),
        _( "Switch to Inner layer 4" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In4_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner5( "pcbnew.Control.layerInner5",
        AS_GLOBAL,
        0, LEGACY_HK_NAME( "Switch to Inner layer 5" ),
        _( "Switch to Inner layer 5" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In5_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner6( "pcbnew.Control.layerInner6",
        AS_GLOBAL,
        0, LEGACY_HK_NAME( "Switch to Inner layer 6" ),
        _( "Switch to Inner layer 6" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In6_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner7( "pcbnew.Control.layerInner7",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 7" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In7_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner8( "pcbnew.Control.layerInner8",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 8" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In8_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner9( "pcbnew.Control.layerInner9",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 9" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In9_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner10( "pcbnew.Control.layerInner10",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 10" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In10_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner11( "pcbnew.Control.layerInner11",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 11" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In11_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner12( "pcbnew.Control.layerInner12",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 12" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In12_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner13( "pcbnew.Control.layerInner13",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 13" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In13_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner14( "pcbnew.Control.layerInner14",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 14" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In14_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner15( "pcbnew.Control.layerInner15",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 15" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In15_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner16( "pcbnew.Control.layerInner16",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 16" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In16_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner17( "pcbnew.Control.layerInner17",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 17" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In17_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner18( "pcbnew.Control.layerInner18",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 18" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In18_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner19( "pcbnew.Control.layerInner19",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 19" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In19_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner20( "pcbnew.Control.layerInner20",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 20" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In20_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner21( "pcbnew.Control.layerInner21",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 21" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In21_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner22( "pcbnew.Control.layerInner22",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 22" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In22_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner23( "pcbnew.Control.layerInner23",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 23" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In23_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner24( "pcbnew.Control.layerInner24",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 24" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In24_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner25( "pcbnew.Control.layerInner25",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 25" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In25_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner26( "pcbnew.Control.layerInner26",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 26" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In26_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner27( "pcbnew.Control.layerInner27",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 27" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In27_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner28( "pcbnew.Control.layerInner28",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 28" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In28_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner29( "pcbnew.Control.layerInner29",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 29" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In29_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner30( "pcbnew.Control.layerInner30",
        AS_GLOBAL, 0, "",
        _( "Switch to Inner layer 30" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) In30_Cu );

TOOL_ACTION PCB_ACTIONS::layerBottom( "pcbnew.Control.layerBottom",
        AS_GLOBAL,
        WXK_PAGEDOWN, LEGACY_HK_NAME( "Switch to Copper (B.Cu) layer" ),
        _( "Switch to Copper (B.Cu) layer" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY, (void*) B_Cu );

TOOL_ACTION PCB_ACTIONS::layerNext( "pcbnew.Control.layerNext",
        AS_GLOBAL,
        '+', LEGACY_HK_NAME( "Switch to Next Layer" ),
        _( "Switch to Next Layer" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY );

TOOL_ACTION PCB_ACTIONS::layerPrev( "pcbnew.Control.layerPrev",
        AS_GLOBAL,
        '-', LEGACY_HK_NAME( "Switch to Previous Layer" ),
        _( "Switch to Previous Layer" ), "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY );

TOOL_ACTION PCB_ACTIONS::layerToggle( "pcbnew.Control.layerToggle",
        AS_GLOBAL,
        'V', LEGACY_HK_NAME( "Add Through Via" ),
        _( "Toggle Layer" ), _( "Switch between layers in active layer pair" ),
        BITMAPS::INVALID_BITMAP, AF_NOTIFY );

TOOL_ACTION PCB_ACTIONS::layerAlphaInc( "pcbnew.Control.layerAlphaInc",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        '}', LEGACY_HK_NAME( "Increment Layer Transparency (Modern Toolset only)" ),
        _( "Increase Layer Opacity" ), _( "Make the current layer more transparent" ),
        BITMAPS::contrast_mode );

TOOL_ACTION PCB_ACTIONS::layerAlphaDec( "pcbnew.Control.layerAlphaDec",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        '{', LEGACY_HK_NAME( "Decrement Layer Transparency (Modern Toolset only)" ),
        _( "Decrease Layer Opacity" ), _( "Make the current layer more transparent" ),
        BITMAPS::contrast_mode );

TOOL_ACTION PCB_ACTIONS::layerChanged( "pcbnew.Control.layerChanged",
        AS_GLOBAL, 0, "",
        "", "",
        BITMAPS::INVALID_BITMAP, AF_NOTIFY );

//Show board statistics tool
TOOL_ACTION PCB_ACTIONS::boardStatistics( "pcbnew.InspectionTool.ShowStatisticsDialog",
        AS_GLOBAL, 0, "",
        _( "Show Board Statistics" ), _( "Shows board statistics" ) );

TOOL_ACTION PCB_ACTIONS::inspectClearance( "pcbnew.InspectionTool.InspectClearance",
        AS_GLOBAL, 0, "",
        _( "Clearance Resolution..." ),
        _( "Show clearance resolution for the active layer between two selected objects" ),
        BITMAPS::mw_add_gap );

TOOL_ACTION PCB_ACTIONS::inspectConstraints( "pcbnew.InspectionTool.InspectConstraints",
        AS_GLOBAL, 0, "",
        _( "Constraints Resolution..." ),
        _( "Show constraints resolution for the selected object" ),
        BITMAPS::mw_add_gap );

//Geographic re-annotation tool
TOOL_ACTION PCB_ACTIONS::boardReannotate( "pcbnew.ReannotateTool.ShowReannotateDialog",
        AS_GLOBAL, 0, "",
        _( "Geographical Reannotate..." ), _( "Reannotate PCB in geographical order" ),
        BITMAPS::annotate );

TOOL_ACTION PCB_ACTIONS::repairBoard( "pcbnew.Control.repairBoard",
        AS_GLOBAL, 0, "",
        _( "Repair Board" ),
        _( "Run various diagnostics and attempt to repair board" ),
        BITMAPS::rescue );


// PLACEMENT_TOOL
//
TOOL_ACTION PCB_ACTIONS::alignTop( "pcbnew.AlignAndDistribute.alignTop",
        AS_GLOBAL, 0, "",
        _( "Align to Top" ),
        _( "Aligns selected items to the top edge" ), BITMAPS::align_items_top );

TOOL_ACTION PCB_ACTIONS::alignBottom( "pcbnew.AlignAndDistribute.alignBottom",
        AS_GLOBAL, 0, "",
        _( "Align to Bottom" ),
        _( "Aligns selected items to the bottom edge" ), BITMAPS::align_items_bottom );

TOOL_ACTION PCB_ACTIONS::alignLeft( "pcbnew.AlignAndDistribute.alignLeft",
        AS_GLOBAL, 0, "",
        _( "Align to Left" ),
        _( "Aligns selected items to the left edge" ), BITMAPS::align_items_left );

TOOL_ACTION PCB_ACTIONS::alignRight( "pcbnew.AlignAndDistribute.alignRight",
        AS_GLOBAL, 0, "",
        _( "Align to Right" ),
        _( "Aligns selected items to the right edge" ), BITMAPS::align_items_right );

TOOL_ACTION PCB_ACTIONS::alignCenterX( "pcbnew.AlignAndDistribute.alignCenterX",
        AS_GLOBAL, 0, "",
        _( "Align to Vertical Center" ),
        _( "Aligns selected items to the vertical center" ), BITMAPS::align_items_middle );

TOOL_ACTION PCB_ACTIONS::alignCenterY( "pcbnew.AlignAndDistribute.alignCenterY",
        AS_GLOBAL, 0, "",
        _( "Align to Horizontal Center" ),
        _( "Aligns selected items to the horizontal center" ), BITMAPS::align_items_center );

TOOL_ACTION PCB_ACTIONS::distributeHorizontally( "pcbnew.AlignAndDistribute.distributeHorizontally",
        AS_GLOBAL, 0, "",
        _( "Distribute Horizontally" ),
        _( "Distributes selected items along the horizontal axis" ), BITMAPS::distribute_horizontal );

TOOL_ACTION PCB_ACTIONS::distributeVertically( "pcbnew.AlignAndDistribute.distributeVertically",
        AS_GLOBAL, 0, "",
        _( "Distribute Vertically" ),
        _( "Distributes selected items along the vertical axis" ), BITMAPS::distribute_vertical );


// PCB_POINT_EDITOR
//
TOOL_ACTION PCB_ACTIONS::pointEditorAddCorner( "pcbnew.PointEditor.addCorner",
        AS_GLOBAL,
#ifdef __WXMAC__
        'I', "",
#else
        WXK_INSERT, "",
#endif
        _( "Create Corner" ), _( "Create a corner" ),
        BITMAPS::add_corner );

TOOL_ACTION PCB_ACTIONS::pointEditorRemoveCorner( "pcbnew.PointEditor.removeCorner",
        AS_GLOBAL, 0, "",
        _( "Remove Corner" ), _( "Remove corner" ),
        BITMAPS::delete_cursor );


// GROUP_TOOL
//
TOOL_ACTION PCB_ACTIONS::groupProperties( "pcbnew.Groups.groupProperties",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::pickNewGroupMember( "pcbnew.Groups.selectNewGroupMember",
        AS_GLOBAL );



// POSITION_RELATIVE_TOOL
//
TOOL_ACTION PCB_ACTIONS::positionRelative( "pcbnew.PositionRelative.positionRelative",
        AS_GLOBAL,
        MD_SHIFT + 'P', LEGACY_HK_NAME( "Position Item Relative" ),
        _( "Position Relative To..." ),
        _( "Positions the selected item(s) by an exact amount relative to another" ),
        BITMAPS::move_relative );

TOOL_ACTION PCB_ACTIONS::selectpositionRelativeItem( "pcbnew.PositionRelative.selectpositionRelativeItem",
        AS_GLOBAL );


// PCB_SELECTION_TOOL
//
TOOL_ACTION PCB_ACTIONS::selectionActivate( "pcbnew.InteractiveSelection",
        AS_GLOBAL, 0, "", "", "",   // No description, not shown anywhere
        BITMAPS::INVALID_BITMAP, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::selectionCursor( "pcbnew.InteractiveSelection.Cursor",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::selectItem( "pcbnew.InteractiveSelection.SelectItem",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::selectItems( "pcbnew.InteractiveSelection.SelectItems",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::unselectItem( "pcbnew.InteractiveSelection.UnselectItem",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::unselectItems( "pcbnew.InteractiveSelection.UnselectItems",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::selectionClear( "pcbnew.InteractiveSelection.Clear",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::selectionMenu( "pcbnew.InteractiveSelection.SelectionMenu",
        AS_GLOBAL );

TOOL_ACTION PCB_ACTIONS::selectConnection( "pcbnew.InteractiveSelection.SelectConnection",
        AS_GLOBAL,
        'U', LEGACY_HK_NAME( "Select Single Track" ),
        _( "Select/Expand Connection" ),
        _( "Selects a connection or expands an existing selection to junctions, pads, or entire connections" ),
        BITMAPS::add_tracks );

TOOL_ACTION PCB_ACTIONS::selectNet( "pcbnew.InteractiveSelection.SelectNet",
        AS_GLOBAL, 0, "",
        _( "Select All Tracks in Net" ),
        _( "Selects all tracks & vias belonging to the same net." ) );

TOOL_ACTION PCB_ACTIONS::deselectNet( "pcbnew.InteractiveSelection.DeselectNet",
        AS_GLOBAL, 0, "",
        _( "Deselect All Tracks in Net" ),
        _( "Deselects all tracks & vias belonging to the same net." ) );

TOOL_ACTION PCB_ACTIONS::selectOnSheetFromEeschema( "pcbnew.InteractiveSelection.SelectOnSheet",
        AS_GLOBAL, 0, "",
        _( "Sheet" ),
        _( "Selects all footprints and tracks in the schematic sheet" ),
        BITMAPS::select_same_sheet );

TOOL_ACTION PCB_ACTIONS::selectSameSheet( "pcbnew.InteractiveSelection.SelectSameSheet",
        AS_GLOBAL,  0, "",
        _( "Items in Same Hierarchical Sheet" ),
        _( "Selects all footprints and tracks in the same schematic sheet" ),
        BITMAPS::select_same_sheet );

TOOL_ACTION PCB_ACTIONS::filterSelection( "pcbnew.InteractiveSelection.FilterSelection",
        AS_GLOBAL, 0, "",
        _( "Filter Selected Items..." ), _( "Remove items from the selection by type" ),
        BITMAPS::filter );


// ZONE_FILLER_TOOL
//
TOOL_ACTION PCB_ACTIONS::zoneFill( "pcbnew.ZoneFiller.zoneFill",
        AS_GLOBAL, 0, "",
        _( "Fill" ), _( "Fill zone(s)" ),
        BITMAPS::fill_zone );

TOOL_ACTION PCB_ACTIONS::zoneFillAll( "pcbnew.ZoneFiller.zoneFillAll",
        AS_GLOBAL,
        'B', LEGACY_HK_NAME( "Fill or Refill All Zones" ),
        _( "Fill All" ), _( "Fill all zones" ),
        BITMAPS::fill_zone );

TOOL_ACTION PCB_ACTIONS::zoneUnfill( "pcbnew.ZoneFiller.zoneUnfill",
        AS_GLOBAL, 0, "",
        _( "Unfill" ), _( "Unfill zone(s)" ),
        BITMAPS::zone_unfill );

TOOL_ACTION PCB_ACTIONS::zoneUnfillAll( "pcbnew.ZoneFiller.zoneUnfillAll",
        AS_GLOBAL,
        MD_CTRL + 'B', LEGACY_HK_NAME( "Remove Filled Areas in All Zones" ),
        _( "Unfill All" ), _( "Unfill all zones" ),
        BITMAPS::zone_unfill );


// AUTOPLACER_TOOL
//
TOOL_ACTION PCB_ACTIONS::autoplaceSelectedComponents( "pcbnew.Autoplacer.autoplaceSelected",
        AS_GLOBAL, 0, "",
        _( "Place Selected Footprints" ),
        _( "Performs automatic placement of selected components" ) );

TOOL_ACTION PCB_ACTIONS::autoplaceOffboardComponents( "pcbnew.Autoplacer.autoplaceOffboard",
        AS_GLOBAL, 0, "",
        _( "Place Off-Board Footprints" ),
        _( "Performs automatic placement of components outside board area" ) );


// ROUTER_TOOL
//
TOOL_ACTION PCB_ACTIONS::routeSingleTrack( "pcbnew.InteractiveRouter.SingleTrack",
        AS_GLOBAL,
        'X', LEGACY_HK_NAME( "Add New Track" ),
        _( "Route Single Track" ), _( "Route tracks" ),
        BITMAPS::add_tracks, AF_ACTIVATE, (void*) PNS::PNS_MODE_ROUTE_SINGLE );

TOOL_ACTION PCB_ACTIONS::routeDiffPair( "pcbnew.InteractiveRouter.DiffPair",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        '6', LEGACY_HK_NAME( "Route Differential Pair (Modern Toolset only)" ),
        _( "Route Differential Pair" ), _( "Route differential pairs" ),
        BITMAPS::ps_diff_pair, AF_ACTIVATE, (void*) PNS::PNS_MODE_ROUTE_DIFF_PAIR );

TOOL_ACTION PCB_ACTIONS::routerSettingsDialog( "pcbnew.InteractiveRouter.SettingsDialog",
        AS_GLOBAL,
        MD_CTRL + MD_SHIFT + ',', LEGACY_HK_NAME( "Routing Options" ),
        _( "Interactive Router Settings..." ), _( "Open Interactive Router settings" ),
        BITMAPS::tools );

TOOL_ACTION PCB_ACTIONS::routerDiffPairDialog( "pcbnew.InteractiveRouter.DiffPairDialog",
        AS_GLOBAL, 0, "",
        _( "Differential Pair Dimensions..." ), _( "Open Differential Pair Dimension settings" ),
        BITMAPS::ps_diff_pair_gap );

TOOL_ACTION PCB_ACTIONS::routerHighlightMode( "pcbnew.InteractiveRouter.HighlightMode",
        AS_GLOBAL, 0, "",
        _( "Router Highlight Mode" ), _( "Switch router to highlight mode" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) PNS::RM_MarkObstacles );

TOOL_ACTION PCB_ACTIONS::routerShoveMode( "pcbnew.InteractiveRouter.ShoveMode",
        AS_GLOBAL, 0, "",
        _( "Router Shove Mode" ), _( "Switch router to shove mode" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) PNS::RM_Shove );

TOOL_ACTION PCB_ACTIONS::routerWalkaroundMode( "pcbnew.InteractiveRouter.WalkaroundMode",
        AS_GLOBAL, 0, "",
        _( "Router Walkaround Mode" ), _( "Switch router to walkaround mode" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) PNS::RM_Walkaround );

TOOL_ACTION PCB_ACTIONS::selectLayerPair( "pcbnew.InteractiveRouter.SelectLayerPair",
        AS_GLOBAL, 0, "",
        _( "Set Layer Pair..." ), _( "Change active layer pair for routing" ),
        BITMAPS::select_layer_pair, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::routerTuneSingleTrace( "pcbnew.LengthTuner.TuneSingleTrack",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        '7', LEGACY_HK_NAME( "Tune Single Track (Modern Toolset only)" ),
        _( "Tune length of a single track" ), _( "Tune length of a single track" ),
        BITMAPS::ps_tune_length, AF_ACTIVATE, (void*) PNS::PNS_MODE_TUNE_SINGLE );

TOOL_ACTION PCB_ACTIONS::routerTuneDiffPair( "pcbnew.LengthTuner.TuneDiffPair",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        '8', LEGACY_HK_NAME( "Tune Differential Pair Length (Modern Toolset only)" ),
        _( "Tune length of a differential pair" ), _( "Tune length of a differential pair" ),
        BITMAPS::ps_diff_pair_tune_length, AF_ACTIVATE, (void*) PNS::PNS_MODE_TUNE_DIFF_PAIR );

TOOL_ACTION PCB_ACTIONS::routerTuneDiffPairSkew( "pcbnew.LengthTuner.TuneDiffPairSkew",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        '9', LEGACY_HK_NAME( "Tune Differential Pair Skew (Modern Toolset only)" ),
        _( "Tune skew of a differential pair" ), _( "Tune skew of a differential pair" ),
        BITMAPS::ps_diff_pair_tune_phase, AF_ACTIVATE, (void*) PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW );

TOOL_ACTION PCB_ACTIONS::routerInlineDrag( "pcbnew.InteractiveRouter.InlineDrag",
        AS_CONTEXT );

TOOL_ACTION PCB_ACTIONS::inlineBreakTrack( "pcbnew.InteractiveRouter.InlineBreakTrack",
        AS_GLOBAL, 0, "",
        _( "Break Track" ),
        _( "Splits the track segment into two segments connected at the cursor position." ),
        BITMAPS::break_line );

TOOL_ACTION PCB_ACTIONS::breakTrack( "pcbnew.InteractiveRouter.BreakTrack",
        AS_GLOBAL, 0, "",
        _( "Break Track" ),
        _( "Splits the track segment into two segments connected at the cursor position." ),
        BITMAPS::break_line );

TOOL_ACTION PCB_ACTIONS::drag45Degree( "pcbnew.InteractiveRouter.Drag45Degree",
        AS_GLOBAL,
        'D', LEGACY_HK_NAME( "Drag Track Keep Slope" ),
        _( "Drag (45 degree mode)" ),
        _( "Drags the track segment while keeping connected tracks at 45 degrees." ),
        BITMAPS::drag_segment_withslope );

TOOL_ACTION PCB_ACTIONS::dragFreeAngle( "pcbnew.InteractiveRouter.DragFreeAngle",
        AS_GLOBAL,
        'G', LEGACY_HK_NAME( "Drag Item" ),
        _( "Drag (free angle)" ),
        _( "Drags the nearest joint in the track without restricting the track angle." ),
        BITMAPS::drag );


// LENGTH_TUNER_TOOL
//
TOOL_ACTION PCB_ACTIONS::lengthTunerSettingsDialog( "pcbnew.LengthTuner.Settings",
        AS_CONTEXT,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        MD_CTRL + 'L', LEGACY_HK_NAME( "Length Tuning Settings (Modern Toolset only)" ),
        _( "Length Tuning Settings..." ),
        _( "Sets the length tuning parameters for currently routed item." ),
        BITMAPS::router_len_tuner_setup );
