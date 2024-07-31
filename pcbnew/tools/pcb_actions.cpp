/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2023 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "tool/tool_action.h"
#include "tool/tool_event.h"
#include <pcbnew_id.h>
#include <bitmaps.h>
#include <layer_ids.h>
#include <microwave/microwave_tool.h>
#include <pcb_reference_image.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <router/pns_router.h>
#include <router/pns_routing_settings.h>

// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

// clang-format off

// CONVERT_TOOL
//
TOOL_ACTION PCB_ACTIONS::convertToPoly( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Convert.convertToPoly" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Create Polygon from Selection..." ) )
        .Tooltip( _( "Creates a graphic polygon from the selection" ) )
        .Icon( BITMAPS::add_graphical_polygon ) );

TOOL_ACTION PCB_ACTIONS::convertToZone( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Convert.convertToZone" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Create Zone from Selection..." ) )
        .Tooltip( _( "Creates a copper zone from the selection" ) )
        .Icon( BITMAPS::add_zone ) );

TOOL_ACTION PCB_ACTIONS::convertToKeepout( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Convert.convertToKeepout" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Create Rule Area from Selection..." ) )
        .Tooltip( _( "Creates a rule area from the selection" ) )
        .Icon( BITMAPS::add_keepout_area ) );

TOOL_ACTION PCB_ACTIONS::convertToLines( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Convert.convertToLines" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Create Lines from Selection..." ) )
        .Tooltip( _( "Creates graphic lines from the selection" ) )
        .Icon( BITMAPS::add_line ) );

TOOL_ACTION PCB_ACTIONS::convertToArc( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Convert.convertToArc" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Create Arc from Selection" ) )
        .Tooltip( _( "Creates an arc from the selected line segment" ) )
        .Icon( BITMAPS::add_arc ) );

TOOL_ACTION PCB_ACTIONS::convertToTracks( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Convert.convertToTracks" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Create Tracks from Selection" ) )
        .Tooltip( _( "Creates tracks from the selected graphic lines" ) )
        .Icon( BITMAPS::add_tracks ) );


// DRAWING_TOOL
//
TOOL_ACTION PCB_ACTIONS::drawLine( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.line" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'L' )
        .LegacyHotkeyName( "Draw Line" )
        .FriendlyName( _( "Draw Line" ) )
        .Tooltip( _( "Draw a line" ) )
        .Icon( BITMAPS::add_graphical_segments )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawPolygon( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.graphicPolygon" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'P' )
        .LegacyHotkeyName( "Draw Graphic Polygon" )
        .FriendlyName( _( "Draw Graphic Polygon" ) )
        .Tooltip( _( "Draw a graphic polygon" ) )
        .Icon( BITMAPS::add_graphical_polygon )
        .Flags( AF_ACTIVATE )
        .Parameter( ZONE_MODE::GRAPHIC_POLYGON ) );

TOOL_ACTION PCB_ACTIONS::drawRectangle( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.rectangle" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Rectangle" ) )
        .Tooltip( _( "Draw a rectangle" ) )
        .Icon( BITMAPS::add_rectangle )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawCircle( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.circle" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'C' )
        .LegacyHotkeyName( "Draw Circle" )
        .FriendlyName( _( "Draw Circle" ) )
        .Tooltip( _( "Draw a circle" ) )
        .Icon( BITMAPS::add_circle )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawArc( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.arc" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'A' )
        .LegacyHotkeyName( "Draw Arc" )
        .FriendlyName( _( "Draw Arc" ) )
        .Tooltip( _( "Draw an arc" ) )
        .Icon( BITMAPS::add_arc )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::placeCharacteristics( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.placeCharacteristics" )
        .Scope( AS_GLOBAL )
        .LegacyHotkeyName( "Add Board Characteristics" )
        .FriendlyName( _( "Add Board Characteristics" ) )
        .Tooltip( _( "Add a board characteristics table on a graphic layer" ) )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::placeStackup( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.placeStackup" )
        .Scope( AS_GLOBAL )
        .LegacyHotkeyName( "Add Stackup Table" )
        .FriendlyName( _( "Add Stackup Table" ) )
        .Tooltip( _( "Add a board stackup table on a graphic layer" ) )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::placeReferenceImage( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.placeReferenceImage" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Reference Image" ) )
        .Tooltip( _( "Add a bitmap image to be used as a reference (image will not be included in any output)" ) )
        .Icon( BITMAPS::image )
        .Flags( AF_ACTIVATE )
        .Parameter<PCB_REFERENCE_IMAGE*>( nullptr ) );

TOOL_ACTION PCB_ACTIONS::placeText( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.text" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'T' )
        .LegacyHotkeyName( "Add Text" )
        .FriendlyName( _( "Add Text" ) )
        .Tooltip( _( "Add a text item" ) )
        .Icon( BITMAPS::text )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawTextBox( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.textbox" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Text Box" ) )
        .Tooltip( _( "Add a line-wrapped text item" ) )
        .Icon( BITMAPS::add_textbox )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawTable( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.drawTable" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Table" ) )
        .Tooltip( _( "Draw table" ) )
        .Icon( BITMAPS::spreadsheet )   // JEY TODO
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::spacingIncrease( TOOL_ACTION_ARGS()
        .Name( "pcbnew.lengthTuner.SpacingIncrease" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '1' )
        .LegacyHotkeyName( "Increase meander spacing by one step." )
        .FriendlyName( _( "Increase Spacing" ) )
        .Tooltip( _( "Increase tuning pattern spacing by one step." ) )
        .Icon( BITMAPS::router_len_tuner_dist_incr ) );

TOOL_ACTION PCB_ACTIONS::spacingDecrease( TOOL_ACTION_ARGS()
        .Name( "pcbnew.lengthTuner.SpacingDecrease" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '2' )
        .LegacyHotkeyName( "Decrease meander spacing by one step." )
        .FriendlyName( _( "Decrease Spacing" ) )
        .Tooltip( _( "Decrease tuning pattern spacing by one step." ) )
        .Icon( BITMAPS::router_len_tuner_dist_decr ) );

TOOL_ACTION PCB_ACTIONS::amplIncrease( TOOL_ACTION_ARGS()
        .Name( "pcbnew.lengthTuner.AmplIncrease" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '3' )
        .LegacyHotkeyName( "Increase meander amplitude by one step." )
        .FriendlyName( _( "Increase Amplitude" ) )
        .Tooltip( _( "Increase tuning pattern amplitude by one step." ) )
        .Icon( BITMAPS::router_len_tuner_amplitude_incr ) );

TOOL_ACTION PCB_ACTIONS::amplDecrease( TOOL_ACTION_ARGS()
        .Name( "pcbnew.lengthTuner.AmplDecrease" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '4' )
        .LegacyHotkeyName( "Decrease meander amplitude by one step." )
        .FriendlyName( _( "Decrease Amplitude" ) )
        .Tooltip( _( "Decrease tuning pattern amplitude by one step." ) )
        .Icon( BITMAPS::router_len_tuner_amplitude_decr ) );


TOOL_ACTION PCB_ACTIONS::drawAlignedDimension( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.alignedDimension" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'H' )
        .LegacyHotkeyName( "Add Dimension" )
        .FriendlyName( _( "Add Aligned Dimension" ) )
        .Tooltip( _( "Add an aligned linear dimension" ) )
        .Icon( BITMAPS::add_aligned_dimension )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawCenterDimension( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.centerDimension" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Center Dimension" ) )
        .Tooltip( _( "Add a center dimension" ) )
        .Icon( BITMAPS::add_center_dimension )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawRadialDimension( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.radialDimension" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Radial Dimension" ) )
        .Tooltip( _( "Add a radial dimension" ) )
        .Icon( BITMAPS::add_radial_dimension )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawOrthogonalDimension( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.orthogonalDimension" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Orthogonal Dimension" ) )
        .Tooltip( _( "Add an orthogonal dimension" ) )
        .Icon( BITMAPS::add_orthogonal_dimension )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawLeader( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.leader" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Leader" ) )
        .Tooltip( _( "Add a leader dimension" ) )
        .Icon( BITMAPS::add_leader )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawZone( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.zone" )
        .Scope( AS_GLOBAL )
#ifdef __WXOSX_MAC__
        .DefaultHotkey( MD_ALT + 'Z' )
#else
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'Z' )
#endif
        .LegacyHotkeyName( "Add Filled Zone" )
        .FriendlyName( _( "Add Filled Zone" ) )
        .Tooltip( _( "Add a filled zone" ) )
        .Icon( BITMAPS::add_zone )
        .Flags( AF_ACTIVATE )
        .Parameter( ZONE_MODE::ADD ) );

TOOL_ACTION PCB_ACTIONS::drawVia( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.via" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'V' )
        .LegacyHotkeyName( "Add Vias" )
        .FriendlyName( _( "Add Vias" ) )
        .Tooltip( _( "Add free-standing vias" ) )
        .Icon( BITMAPS::add_via )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drawRuleArea( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.ruleArea" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'K' )
        .LegacyHotkeyName( "Add Keepout Area" )
        .FriendlyName( _( "Add Rule Area" ) )
        .Tooltip( _( "Add a rule area (keepout)" ) )
        .Icon( BITMAPS::add_keepout_area )
        .Flags( AF_ACTIVATE )
        .Parameter( ZONE_MODE::ADD ) );

TOOL_ACTION PCB_ACTIONS::drawZoneCutout( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.zoneCutout" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'C' )
        .LegacyHotkeyName( "Add a Zone Cutout" )
        .FriendlyName( _( "Add a Zone Cutout" ) )
        .Tooltip( _( "Add a cutout area of an existing zone" ) )
        .Icon( BITMAPS::add_zone_cutout )
        .Flags(  AF_ACTIVATE )
        .Parameter( ZONE_MODE::CUTOUT ) );

TOOL_ACTION PCB_ACTIONS::drawSimilarZone( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.similarZone" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + '.' )
        .LegacyHotkeyName( "Add a Similar Zone" )
        .FriendlyName( _( "Add a Similar Zone" ) )
        .Tooltip( _( "Add a zone with the same settings as an existing zone" ) )
        .Icon( BITMAPS::add_zone )
        .Flags( AF_ACTIVATE )
        .Parameter( ZONE_MODE::SIMILAR ) );

TOOL_ACTION PCB_ACTIONS::placeImportedGraphics( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.placeImportedGraphics" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'F' )
        .LegacyHotkeyName( "Place DXF" )
        .FriendlyName( _( "Import Graphics..." ) )
        .Tooltip( _( "Import 2D drawing file" ) )
        .Icon( BITMAPS::import_vector )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::setAnchor( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.setAnchor" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'N' )
        .LegacyHotkeyName( "Place the Footprint Anchor" )
        .FriendlyName( _( "Place the Footprint Anchor" ) )
        .Tooltip( _( "Set the coordinate origin point (anchor) of the footprint" ) )
        .Icon( BITMAPS::anchor )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::incWidth( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.incWidth" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_CTRL + '+' )
        .LegacyHotkeyName( "Increase Line Width" )
        .FriendlyName( _( "Increase Line Width" ) )
        .Tooltip( _( "Increase the line width" ) ) );

TOOL_ACTION PCB_ACTIONS::decWidth( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.decWidth" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_CTRL + '-' )
        .LegacyHotkeyName( "Decrease Line Width" )
        .FriendlyName( _( "Decrease Line Width" ) )
        .Tooltip( _( "Decrease the line width" ) ) );

TOOL_ACTION PCB_ACTIONS::arcPosture( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.arcPosture" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( '/' )
        .LegacyHotkeyName( "Switch Track Posture" )
        .FriendlyName( _( "Switch Arc Posture" ) )
        .Tooltip( _( "Switch the arc posture" ) ) );


TOOL_ACTION PCB_ACTIONS::magneticSnapActiveLayer( TOOL_ACTION_ARGS()
        .Name( "common.Control.magneticSnapActiveLayer" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Snap to Objects on the Active Layer Only" ) )
        .Tooltip( _( "Enables snapping to objects on the active layer only" ) ) );

TOOL_ACTION PCB_ACTIONS::magneticSnapAllLayers( TOOL_ACTION_ARGS()
        .Name( "common.Control.magneticSnapAllLayers" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Snap to Objects on All Layers" ) )
        .Tooltip( _( "Enables snapping to objects on all visible layers" ) ) );

TOOL_ACTION PCB_ACTIONS::magneticSnapToggle( TOOL_ACTION_ARGS()
        .Name( "common.Control.magneticSnapToggle" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'S' )
        .FriendlyName( _( "Toggle Snapping Between Active and All Layers" ) )
        .Tooltip( _( "Toggles between snapping on all visible layers and only the active area" ) ) );

TOOL_ACTION PCB_ACTIONS::deleteLastPoint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.deleteLastPoint" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( WXK_BACK )
        .FriendlyName( _( "Delete Last Point" ) )
        .Tooltip( _( "Delete the last point added to the current item" ) )
        .Icon( BITMAPS::undo ) );

TOOL_ACTION PCB_ACTIONS::closeOutline( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveDrawing.closeOutline" )
        .Scope( AS_CONTEXT )
        .FriendlyName( _( "Close Outline" ) )
        .Tooltip( _( "Close the in progress outline" ) )
        .Icon( BITMAPS::checked_ok ) );

// DRC
//
TOOL_ACTION PCB_ACTIONS::runDRC( TOOL_ACTION_ARGS()
        .Name( "pcbnew.DRCTool.runDRC" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Design Rules Checker" ) )
        .Tooltip( _( "Show the design rules checker window" ) )
        .Icon( BITMAPS::erc ) );


// EDIT_TOOL
//
TOOL_ACTION PCB_ACTIONS::editFpInFpEditor( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.EditFpInFpEditor" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'E' )
        .LegacyHotkeyName( "Edit with Footprint Editor" )
        .FriendlyName( _( "Open in Footprint Editor" ) )
        .Tooltip( _( "Opens the selected footprint in the Footprint Editor" ) )
        .Icon( BITMAPS::module_editor ) );

TOOL_ACTION PCB_ACTIONS::editLibFpInFpEditor( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.EditLibFpInFpEditor" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'E' )
        .FriendlyName( _( "Edit Library Footprint..." ) )
        .Tooltip( _( "Opens the selected footprint in the Footprint Editor" ) )
        .Icon( BITMAPS::module_editor ) );

TOOL_ACTION PCB_ACTIONS::getAndPlace( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.FindMove" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'T' )
        .LegacyHotkeyName( "Get and Move Footprint" )
        .FriendlyName( _( "Get and Move Footprint" ) )
        .Tooltip( _( "Selects a footprint by reference designator and places it under the cursor for moving" ) )
        .Icon( BITMAPS::move )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::move( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveMove.move" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'M' )
        .LegacyHotkeyName( "Move Item" )
        .FriendlyName( _( "Move" ) )
        .Tooltip( _( "Moves the selected item(s)" ) )
        .Icon( BITMAPS::move )
        .Flags( AF_ACTIVATE )
        .Parameter( ACTIONS::CURSOR_EVENT_TYPE::CURSOR_NONE ) );

TOOL_ACTION PCB_ACTIONS::moveIndividually( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveMove.moveIndividually" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'M' )
        .FriendlyName( _( "Move Individually" ) )
        .Tooltip( _( "Moves the selected items one-by-one" ) )
        .Icon( BITMAPS::move )
        .Flags( AF_ACTIVATE )
        .Parameter( ACTIONS::CURSOR_EVENT_TYPE::CURSOR_NONE ) );

TOOL_ACTION PCB_ACTIONS::moveWithReference( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveMove.moveWithReference" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Move with Reference" ) )
        .Tooltip( _( "Moves the selected item(s) with a specified starting point" ) )
        .Icon( BITMAPS::move )
        .Flags( AF_ACTIVATE )
        .Parameter( ACTIONS::CURSOR_EVENT_TYPE::CURSOR_NONE ) );

TOOL_ACTION PCB_ACTIONS::copyWithReference( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveMove.copyWithReference" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Copy with Reference" ) )
        .Tooltip( _( "Copy selected item(s) to clipboard with a specified starting point" ) )
        .Icon( BITMAPS::copy )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::duplicateIncrement(TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.duplicateIncrementPads" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'D' )
        .LegacyHotkeyName( "Duplicate Item and Increment" )
        .FriendlyName( _( "Duplicate and Increment" ) )
        .Tooltip( _( "Duplicates the selected item(s), incrementing pad numbers" ) )
        .Icon( BITMAPS::duplicate ) );

TOOL_ACTION PCB_ACTIONS::moveExact( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.moveExact" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'M' )
        .LegacyHotkeyName( "Move Item Exactly" )
        .FriendlyName( _( "Move Exactly..." ) )
        .Tooltip( _( "Moves the selected item(s) by an exact amount" ) )
        .Icon( BITMAPS::move_exactly ) );

TOOL_ACTION PCB_ACTIONS::pointEditorMoveCorner( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.moveCorner" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Move Corner To..." ) )
        .Tooltip( _( "Move the active corner to an exact location" ) )
        .Icon( BITMAPS::move_exactly ) );

TOOL_ACTION PCB_ACTIONS::pointEditorMoveMidpoint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.moveMidpoint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Move Midpoint To..." ) )
        .Tooltip( _( "Move the active midpoint to an exact location" ) )
        .Icon( BITMAPS::move_exactly ) );

TOOL_ACTION PCB_ACTIONS::createArray( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.createArray" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'T' )
        .LegacyHotkeyName( "Create Array" )
        .FriendlyName( _( "Create Array..." ) )
        .Icon( BITMAPS::array )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::rotateCw( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.rotateCw" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'R' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Rotate Item Clockwise (Modern Toolset only)" )
        .FriendlyName( _( "Rotate Clockwise" ) )
        .Tooltip( _( "Rotates selected item(s) clockwise" ) )
        .Icon( BITMAPS::rotate_cw )
        .Flags( AF_NONE )
        .Parameter( -1 ) );

TOOL_ACTION PCB_ACTIONS::rotateCcw( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.rotateCcw" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'R' )
        .LegacyHotkeyName( "Rotate Item" )
        .FriendlyName( _( "Rotate Counterclockwise" ) )
        .Tooltip( _( "Rotates selected item(s) counterclockwise" ) )
        .Icon( BITMAPS::rotate_ccw )
        .Flags( AF_NONE )
        .Parameter( 1 ) );

TOOL_ACTION PCB_ACTIONS::flip( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.flip" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'F' )
        .LegacyHotkeyName( "Flip Item" )
        .FriendlyName( _( "Change Side / Flip" ) )
        .Tooltip( _( "Flips selected item(s) to opposite side of board" ) )
        .Icon( BITMAPS::swap_layer ) );

TOOL_ACTION PCB_ACTIONS::mirrorH( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.mirrorHoriontally" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Mirror Horizontally" ) )
        .Tooltip( _( "Mirrors selected item across the Y axis" ) )
        .Icon( BITMAPS::mirror_h ) );

TOOL_ACTION PCB_ACTIONS::mirrorV( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.mirrorVertically" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Mirror Vertically" ) )
        .Tooltip( _( "Mirrors selected item across the X axis" ) )
        .Icon( BITMAPS::mirror_v ) );

TOOL_ACTION PCB_ACTIONS::swap( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.swap" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'S' )
        .FriendlyName( _( "Swap" ) )
        .Tooltip( _( "Swaps selected items' positions" ) )
        .Icon( BITMAPS::swap ) );

TOOL_ACTION PCB_ACTIONS::packAndMoveFootprints( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.packAndMoveFootprints" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'P' )
        .FriendlyName( _( "Pack and Move Footprints" ) )
        .Tooltip( _( "Sorts selected footprints by reference, packs based on size and initiates movement" ) )
        .Icon( BITMAPS::pack_footprints ) );

TOOL_ACTION PCB_ACTIONS::skip( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.skip" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( WXK_TAB )
        .FriendlyName( _( "Skip" ) )
        .Tooltip( _( "Skip item" ) )
        .Icon( BITMAPS::right ) );

TOOL_ACTION PCB_ACTIONS::changeTrackWidth( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.changeTrackWidth" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change Track Width" ) )
        .Tooltip( _( "Updates selected track & via sizes" ) ) );

TOOL_ACTION PCB_ACTIONS::filletTracks( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.filletTracks" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Fillet Tracks" ) )
        .Tooltip( _( "Adds arcs tangent to the selected straight track segments" ) ) );

TOOL_ACTION PCB_ACTIONS::filletLines( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.filletLines" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Fillet Lines..." ) )
        .Tooltip( _( "Adds arcs tangent to the selected lines" ) )
        .Icon( BITMAPS::fillet ) );

TOOL_ACTION PCB_ACTIONS::chamferLines( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.chamferLines" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Chamfer Lines..." ) )
        .Tooltip( _( "Cut away corners between selected lines" ) )
        .Icon( BITMAPS::chamfer ) );

TOOL_ACTION PCB_ACTIONS::simplifyPolygons( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.simplifyPolygons" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Simplify Polygons" ) )
        .Tooltip( _( "Simplify polygon outlines, removing superfluous points" ) ) );

TOOL_ACTION PCB_ACTIONS::healShapes( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.healShapes" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Heal Shapes" ) )
        .Tooltip( _( "Connect shapes, possibly extending or cutting them, or adding extra geometry" ) )
        .Icon( BITMAPS::heal_shapes ) );

TOOL_ACTION PCB_ACTIONS::extendLines( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.extendLines" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Extend Lines to Meet" ) )
        .Tooltip( _( "Extend lines to meet each other" ) ) );

TOOL_ACTION PCB_ACTIONS::mergePolygons( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.mergePolygons" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Merge Polygons" ) )
        .Tooltip( _( "Merge selected polygons into a single polygon" ) )
        .Icon( BITMAPS::merge_polygons ) );

TOOL_ACTION PCB_ACTIONS::subtractPolygons( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.subtractPolygons" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Subtract Polygons" ) )
        .Tooltip( _( "Subtract selected polygons from the last one selected" ) )
        .Icon( BITMAPS::subtract_polygons ) );

TOOL_ACTION PCB_ACTIONS::intersectPolygons( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.intersectPolygons" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Intersect Polygons" ) )
        .Tooltip( _( "Create the intersection of the selected polygons" ) )
        .Icon( BITMAPS::intersect_polygons ) );

TOOL_ACTION PCB_ACTIONS::deleteFull( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.deleteFull" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + static_cast<int>( WXK_DELETE ) )
        .LegacyHotkeyName( "Delete Full Track" )
        .FriendlyName( _( "Delete Full Track" ) )
        .Tooltip( _( "Deletes selected item(s) and copper connections" ) )
        .Icon( BITMAPS::delete_cursor )
        .Flags( AF_NONE )
        .Parameter( PCB_ACTIONS::REMOVE_FLAGS::ALT ) );

TOOL_ACTION PCB_ACTIONS::properties( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveEdit.properties" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'E' )
        .LegacyHotkeyName( "Edit Item" )
        .FriendlyName( _( "Properties..." ) )
        .Tooltip( _( "Displays item properties dialog" ) )
        .Icon( BITMAPS::edit ) );


// FOOTPRINT_EDITOR_CONTROL
//
TOOL_ACTION PCB_ACTIONS::newFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.newFootprint" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'N' )
        .LegacyHotkeyName( "New" )
        .FriendlyName( _( "New Footprint" ) )
        .Tooltip( _( "Create a new, empty footprint" ) )
        .Icon( BITMAPS::new_footprint ) );

TOOL_ACTION PCB_ACTIONS::createFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.createFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Create Footprint..." ) )
        .Tooltip( _( "Create a new footprint using the Footprint Wizard" ) )
        .Icon( BITMAPS::module_wizard ) );

TOOL_ACTION PCB_ACTIONS::editFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.editFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Footprint" ) )
        .Tooltip( _( "Show selected footprint on editor canvas" ) )
        .Icon( BITMAPS::edit ) );

TOOL_ACTION PCB_ACTIONS::duplicateFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.duplicateFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Duplicate Footprint" ) )
        .Tooltip( _( "Make a copy of the selected footprint" ) )
        .Icon( BITMAPS::duplicate ) );

TOOL_ACTION PCB_ACTIONS::renameFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.renameFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rename Footprint..." ) )
        .Tooltip( _( "Rename the selected footprint" ) )
        .Icon( BITMAPS::edit ) );

TOOL_ACTION PCB_ACTIONS::deleteFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.deleteFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Delete Footprint from Library" ) )
        .Tooltip( _( "Delete Footprint from Library" ) )
        .Icon( BITMAPS::trash ) );

TOOL_ACTION PCB_ACTIONS::cutFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.cutFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Cut Footprint" ) )
        .Icon( BITMAPS::cut ) );

TOOL_ACTION PCB_ACTIONS::copyFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.copyFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Copy Footprint" ) )
        .Icon( BITMAPS::copy ) );

TOOL_ACTION PCB_ACTIONS::pasteFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.pasteFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Paste Footprint" ) )
        .Icon( BITMAPS::paste ) );

TOOL_ACTION PCB_ACTIONS::importFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.importFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Import Footprint..." ) )
        .Tooltip( _( "Import footprint from file" ) )
        .Icon( BITMAPS::import_module ) );

TOOL_ACTION PCB_ACTIONS::exportFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.exportFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Current Footprint..." ) )
        .Tooltip( _( "Export edited footprint to file" ) )
        .Icon( BITMAPS::export_module ) );

TOOL_ACTION PCB_ACTIONS::footprintProperties( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.footprintProperties" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Footprint Properties..." ) )
        .Icon( BITMAPS::module_options ) );

TOOL_ACTION PCB_ACTIONS::checkFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.checkFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Footprint Checker" ) )
        .Tooltip( _( "Show the footprint checker window" ) )
        .Icon( BITMAPS::erc ) );

// GLOBAL_EDIT_TOOL
//
TOOL_ACTION PCB_ACTIONS::updateFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.updateFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Update Footprint..." ) )
        .Tooltip( _( "Update footprint to include any changes from the library" ) )
        .Icon( BITMAPS::refresh ) );

TOOL_ACTION PCB_ACTIONS::updateFootprints( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.updateFootprints" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Update Footprints from Library..." ) )
        .Tooltip( _( "Update footprints to include any changes from the library" ) )
        .Icon( BITMAPS::refresh ) );

TOOL_ACTION PCB_ACTIONS::removeUnusedPads( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.removeUnusedPads" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Remove Unused Pads..." ) )
        .Tooltip( _( "Remove or restore the unconnected inner layers on through hole pads and vias" ) )
        .Icon( BITMAPS::pads_remove ) );

TOOL_ACTION PCB_ACTIONS::changeFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.changeFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change Footprint..." ) )
        .Tooltip( _( "Assign a different footprint from the library" ) )
        .Icon( BITMAPS::exchange ) );

TOOL_ACTION PCB_ACTIONS::changeFootprints( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.changeFootprints" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change Footprints..." ) )
        .Tooltip( _( "Assign different footprints from the library" ) )
        .Icon( BITMAPS::exchange ) );

TOOL_ACTION PCB_ACTIONS::swapLayers( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.swapLayers" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Swap Layers..." ) )
        .Tooltip( _( "Move tracks or drawings from one layer to another" ) )
        .Icon( BITMAPS::swap_layer ) );

TOOL_ACTION PCB_ACTIONS::editTracksAndVias( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.editTracksAndVias" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Track & Via Properties..." ) )
        .Tooltip( _( "Edit track and via properties globally across board" ) )
        .Icon( BITMAPS::width_track_via ) );

TOOL_ACTION PCB_ACTIONS::editTextAndGraphics( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.editTextAndGraphics" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Text & Graphics Properties..." ) )
        .Tooltip( _( "Edit Text and graphics properties globally across board" ) )
        .Icon( BITMAPS::text ) );

TOOL_ACTION PCB_ACTIONS::editTeardrops( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.editTeardrops" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Teardrops..." ) )
        .Tooltip( _( "Add, remove or edit teardrops globally across board" ) )
        .Icon( BITMAPS::via ) );

TOOL_ACTION PCB_ACTIONS::globalDeletions( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.globalDeletions" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Global Deletions..." ) )
        .Tooltip( _( "Delete tracks, footprints and graphic items from board" ) )
        .Icon( BITMAPS::general_deletions ) );

TOOL_ACTION PCB_ACTIONS::cleanupTracksAndVias( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.cleanupTracksAndVias" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Cleanup Tracks & Vias..." ) )
        .Tooltip( _( "Cleanup redundant items, shorting items, etc." ) )
        .Icon( BITMAPS::cleanup_tracks_and_vias ) );

TOOL_ACTION PCB_ACTIONS::cleanupGraphics( TOOL_ACTION_ARGS()
        .Name( "pcbnew.GlobalEdit.cleanupGraphics" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Cleanup Graphics..." ) )
        .Tooltip( _( "Cleanup redundant items, etc." ) )
        .Icon( BITMAPS::cleanup_graphics ) );

// MICROWAVE_TOOL
//
TOOL_ACTION PCB_ACTIONS::microwaveCreateGap( TOOL_ACTION_ARGS()
        .Name( "pcbnew.MicrowaveTool.createGap" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Microwave Gap" ) )
        .Tooltip( _( "Create gap of specified length for microwave applications" ) )
        .Icon( BITMAPS::mw_add_gap )
        .Flags( AF_ACTIVATE )
        .Parameter( MICROWAVE_FOOTPRINT_SHAPE::GAP ) );

TOOL_ACTION PCB_ACTIONS::microwaveCreateStub( TOOL_ACTION_ARGS()
        .Name( "pcbnew.MicrowaveTool.createStub" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Microwave Stub" ) )
        .Tooltip( _( "Create stub of specified length for microwave applications" ) )
        .Icon( BITMAPS::mw_add_stub )
        .Flags( AF_ACTIVATE )
        .Parameter( MICROWAVE_FOOTPRINT_SHAPE::STUB ) );

TOOL_ACTION PCB_ACTIONS::microwaveCreateStubArc( TOOL_ACTION_ARGS()
        .Name( "pcbnew.MicrowaveTool.createStubArc" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Microwave Arc Stub" ) )
        .Tooltip( _( "Create stub (arc) of specified size for microwave applications" ) )
        .Icon( BITMAPS::mw_add_stub_arc )
        .Flags( AF_ACTIVATE )
        .Parameter( MICROWAVE_FOOTPRINT_SHAPE::STUB_ARC ) );

TOOL_ACTION PCB_ACTIONS::microwaveCreateFunctionShape( TOOL_ACTION_ARGS()
        .Name( "pcbnew.MicrowaveTool.createFunctionShape" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Microwave Polygonal Shape" ) )
        .Tooltip( _( "Create a microwave polygonal shape from a list of vertices" ) )
        .Icon( BITMAPS::mw_add_shape )
        .Flags( AF_ACTIVATE )
        .Parameter( MICROWAVE_FOOTPRINT_SHAPE::FUNCTION_SHAPE ) );

TOOL_ACTION PCB_ACTIONS::microwaveCreateLine( TOOL_ACTION_ARGS()
        .Name( "pcbnew.MicrowaveTool.createLine" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Microwave Line" ) )
        .Tooltip( _( "Create line of specified length for microwave applications" ) )
        .Icon( BITMAPS::mw_add_line )
        .Flags( AF_ACTIVATE ) );


// PAD_TOOL
//
TOOL_ACTION PCB_ACTIONS::copyPadSettings( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PadTool.CopyPadSettings" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Copy Pad Properties to Default" ) )
        .Tooltip( _( "Copy current pad's properties" ) )
        .Icon( BITMAPS::copy_pad_settings ) );

TOOL_ACTION PCB_ACTIONS::applyPadSettings( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PadTool.ApplyPadSettings" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Paste Default Pad Properties to Selected" ) )
        .Tooltip( _( "Replace the current pad's properties with those copied earlier" ) )
        .Icon( BITMAPS::apply_pad_settings ) );

TOOL_ACTION PCB_ACTIONS::pushPadSettings( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PadTool.PushPadSettings" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Push Pad Properties to Other Pads..." ) )
        .Tooltip( _( "Copy the current pad's properties to other pads" ) )
        .Icon( BITMAPS::push_pad_settings ) );

TOOL_ACTION PCB_ACTIONS::enumeratePads( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PadTool.enumeratePads" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Renumber Pads..." ) )
        .Tooltip( _( "Renumber pads by clicking on them in the desired order" ) )
        .Icon( BITMAPS::pad_enumerate )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::placePad( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PadTool.placePad" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Pad" ) )
        .Tooltip( _( "Add a pad" ) )
        .Icon( BITMAPS::pad )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::explodePad( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PadTool.explodePad" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'E' )
        .FriendlyName( _( "Edit Pad as Graphic Shapes" ) )
        .Tooltip( _( "Ungroups a custom-shaped pad for editing as individual graphic shapes" ) )
        .Icon( BITMAPS::custom_pad_to_primitives ) );

TOOL_ACTION PCB_ACTIONS::recombinePad( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PadTool.recombinePad" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'E' )
        .FriendlyName( _( "Finish Pad Edit" ) )
        .Tooltip( _( "Regroups all touching graphic shapes into the edited pad" ) )
        .Icon( BITMAPS::custom_pad_to_primitives ) );

TOOL_ACTION PCB_ACTIONS::defaultPadProperties( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PadTool.defaultPadProperties" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Default Pad Properties..." ) )
        .Tooltip( _( "Edit the pad properties used when creating new pads" ) )
        .Icon( BITMAPS::options_pad ) );


// SCRIPTING TOOL
//

TOOL_ACTION PCB_ACTIONS::pluginsShowFolder( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ScriptingTool.pluginsShowFolder" )
        .Scope( AS_GLOBAL )
#ifdef __WXMAC__
        .FriendlyName( _( "Reveal Plugin Folder in Finder" ) )
        .Tooltip( _( "Reveals the plugins folder in a Finder window" ) )
#else
        .FriendlyName( _( "Open Plugin Directory" ) )
        .Tooltip( _( "Opens the directory in the default system file manager" ) )
#endif
        .Icon( BITMAPS::directory_open ) );

// BOARD_EDITOR_CONTROL
//
TOOL_ACTION PCB_ACTIONS::boardSetup( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.boardSetup" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Board Setup..." ) )
        .Tooltip( _( "Edit board setup including layers, design rules and various defaults" ) )
        .Icon( BITMAPS::options_board ) );

TOOL_ACTION PCB_ACTIONS::importNetlist( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.importNetlist" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Import Netlist..." ) )
        .Tooltip( _( "Read netlist and update board connectivity" ) )
        .Icon( BITMAPS::netlist ) );

TOOL_ACTION PCB_ACTIONS::importSpecctraSession( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.importSpecctraSession" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Import Specctra Session..." ) )
        .Tooltip( _( "Import routed Specctra session (*.ses) file" ) )
        .Icon( BITMAPS::import ) );

TOOL_ACTION PCB_ACTIONS::exportSpecctraDSN( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.exportSpecctraDSN" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Specctra DSN..." ) )
        .Tooltip( _( "Export Specctra DSN routing info" ) )
        .Icon( BITMAPS::export_dsn ) );

TOOL_ACTION PCB_ACTIONS::generateGerbers( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.generateGerbers" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Gerbers (.gbr)..." ) )
        .Tooltip( _( "Generate Gerbers for fabrication" ) )
        .Icon( BITMAPS::post_gerber ) );

TOOL_ACTION PCB_ACTIONS::generateDrillFiles( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.generateDrillFiles" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Drill Files (.drl)..." ) )
        .Tooltip( _( "Generate Excellon drill file(s)" ) )
        .Icon( BITMAPS::post_drill ) );

TOOL_ACTION PCB_ACTIONS::generatePosFile( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.generatePosFile" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Component Placement (.pos, .gbr)..." ) )
        .Tooltip( _( "Generate component placement file(s) for pick and place" ) )
        .Icon( BITMAPS::post_compo ) );

TOOL_ACTION PCB_ACTIONS::generateReportFile( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.generateReportFile" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Footprint Report (.rpt)..." ) )
        .Tooltip( _( "Create report of all footprints from current board" ) )
        .Icon( BITMAPS::post_rpt ) );

TOOL_ACTION PCB_ACTIONS::generateIPC2581File( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.generateIPC2581File" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "IPC-2581 File (.xml)..." ) )
        .Tooltip( _( "Generate an IPC-2581 file" ) )
        .Icon( BITMAPS::post_xml ) );

TOOL_ACTION PCB_ACTIONS::generateD356File( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.generateD356File" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "IPC-D-356 Netlist File..." ) )
        .Tooltip( _( "Generate IPC-D-356 netlist file" ) )
        .Icon( BITMAPS::post_d356 ) );

TOOL_ACTION PCB_ACTIONS::generateBOM( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.generateBOM" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Bill of Materials..." ) )
        .Tooltip( _( "Create bill of materials from board" ) )
        .Icon( BITMAPS::post_bom ) );

// Track & via size control
TOOL_ACTION PCB_ACTIONS::trackWidthInc( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.trackWidthInc" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'W' )
        .LegacyHotkeyName( "Switch Track Width To Next" )
        .FriendlyName( _( "Switch Track Width to Next" ) )
        .Tooltip( _( "Change track width to next pre-defined size" ) ) );

TOOL_ACTION PCB_ACTIONS::trackWidthDec( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.trackWidthDec" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'W' )
        .LegacyHotkeyName( "Switch Track Width To Previous" )
        .FriendlyName( _( "Switch Track Width to Previous" ) )
        .Tooltip( _( "Change track width to previous pre-defined size" ) ) );

TOOL_ACTION PCB_ACTIONS::viaSizeInc( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.viaSizeInc" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '\'' )
        .LegacyHotkeyName( "Increase Via Size" )
        .FriendlyName( _( "Increase Via Size" ) )
        .Tooltip( _( "Change via size to next pre-defined size" ) ) );

TOOL_ACTION PCB_ACTIONS::viaSizeDec( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.viaSizeDec" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '\\' )
        .LegacyHotkeyName( "Decrease Via Size" )
        .FriendlyName( _( "Decrease Via Size" ) )
        .Tooltip( _( "Change via size to previous pre-defined size" ) ) );

TOOL_ACTION PCB_ACTIONS::trackViaSizeChanged( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.trackViaSizeChanged" )
        .Scope( AS_GLOBAL )
        .Flags( AF_NOTIFY ) );

TOOL_ACTION PCB_ACTIONS::assignNetClass( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.assignNetclass" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Assign Netclass..." ) )
        .Tooltip( _( "Assign a netclass to nets matching a pattern" ) )
        .Icon( BITMAPS::netlist ) );

TOOL_ACTION PCB_ACTIONS::zoneMerge( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.zoneMerge" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Merge Zones" ) )
        .Tooltip( _( "Merge zones" ) ) );

TOOL_ACTION PCB_ACTIONS::zoneDuplicate( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.zoneDuplicate" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Duplicate Zone onto Layer..." ) )
        .Tooltip( _( "Duplicate zone outline onto a different layer" ) )
        .Icon( BITMAPS::zone_duplicate ) );

TOOL_ACTION PCB_ACTIONS::placeFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.placeFootprint" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'A' )
        .LegacyHotkeyName( "Add Footprint" )
        .FriendlyName( _( "Add Footprint" ) )
        .Tooltip( _( "Add a footprint" ) )
        .Icon( BITMAPS::module )
        .Flags( AF_ACTIVATE )
        .Parameter<FOOTPRINT*>( nullptr ) );

TOOL_ACTION PCB_ACTIONS::drillOrigin( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.drillOrigin" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Drill/Place File Origin" ) )
        .Tooltip( _( "Place origin point for drill files and component placement files" ) )
        .Icon( BITMAPS::set_origin )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::drillResetOrigin( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.drillResetOrigin" )
        .Scope( AS_GLOBAL )
        .LegacyHotkeyName( "Reset Drill Origin" )
        .FriendlyName( _( "Reset Drill Origin" ) ) );

TOOL_ACTION PCB_ACTIONS::toggleLock( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.toggleLock" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'L' )
        .LegacyHotkeyName( "Lock/Unlock Footprint" )
        .FriendlyName( _( "Toggle Lock" ) )
        .Tooltip( _( "Lock or unlock selected items" ) )
        .Icon( BITMAPS::lock_unlock ) );

TOOL_ACTION PCB_ACTIONS::toggleHV45Mode( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.toggle45" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + ' ' )
        .FriendlyName( _( "Constrain to H, V, 45" ) )
        .Tooltip( _( "Limit actions to horizontal, vertical, or 45 degrees from the starting point" ) )
        .Icon( BITMAPS::hv45mode ) );

TOOL_ACTION PCB_ACTIONS::lock( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.lock" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Lock" ) )
        .Tooltip( _( "Prevent items from being moved and/or resized on the canvas" ) )
        .Icon( BITMAPS::locked ) );

TOOL_ACTION PCB_ACTIONS::unlock( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.unlock" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Unlock" ) )
        .Tooltip( _( "Allow items to be moved and/or resized on the canvas" ) )
        .Icon( BITMAPS::unlocked ) );

TOOL_ACTION PCB_ACTIONS::group( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.group" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Group Items" ) )
        .Tooltip( _( "Group the selected items so that they are treated as a single item" ) )
        .Icon( BITMAPS::group ) );

TOOL_ACTION PCB_ACTIONS::ungroup( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.ungroup" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Ungroup Items" ) )
        .Tooltip( _( "Ungroup any selected groups" ) )
        .Icon( BITMAPS::group_ungroup ) );

TOOL_ACTION PCB_ACTIONS::removeFromGroup( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.removeFromGroup" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Remove Items" ) )
        .Tooltip( _( "Remove items from group" ) )
        .Icon( BITMAPS::group_remove ) );

TOOL_ACTION PCB_ACTIONS::groupEnter( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.groupEnter" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Enter Group" ) )
        .Tooltip( _( "Enter the group to edit items" ) )
        .Icon( BITMAPS::group_enter ) );

TOOL_ACTION PCB_ACTIONS::groupLeave( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.groupLeave" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Leave Group" ) )
        .Tooltip( _( "Leave the current group" ) )
        .Icon( BITMAPS::group_leave ) );

TOOL_ACTION PCB_ACTIONS::appendBoard( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.appendBoard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Append Board..." ) )
        .Tooltip( _( "Open another board and append its contents to this board" ) )
        .Icon( BITMAPS::add_board ) );

TOOL_ACTION PCB_ACTIONS::highlightNet( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.highlightNet" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '`' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Toggle Highlight of Selected Net (Modern Toolset only)" )
        .FriendlyName( _( "Highlight Net" ) )
        .Tooltip( _( "Highlight net under cursor" ) )
        .Icon( BITMAPS::net_highlight )
        .Parameter<int>( 0 ) );

TOOL_ACTION PCB_ACTIONS::toggleLastNetHighlight( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.toggleLastNetHighlight" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Toggle Last Net Highlight" ) )
        .Tooltip( _( "Toggle between last two highlighted nets" ) )
        .Parameter<int>( 0 ) );

TOOL_ACTION PCB_ACTIONS::clearHighlight( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.clearHighlight" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '~' )
        .FriendlyName( _( "Clear Net Highlighting" ) )
        .Tooltip( _( "Clear any existing net highlighting" ) ) );

TOOL_ACTION PCB_ACTIONS::toggleNetHighlight( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.toggleNetHighlight" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + '`' )
        .FriendlyName( _( "Toggle Net Highlight" ) )
        .Tooltip( _( "Toggle net highlighting" ) )
        .Icon( BITMAPS::net_highlight )
        .Parameter<int>( 0 ) );

TOOL_ACTION PCB_ACTIONS::highlightNetSelection( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.highlightNetSelection" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Highlight Net" ) )
        .Tooltip( _( "Highlight all copper items on the selected net(s)" ) )
        .Icon( BITMAPS::net_highlight )
        .Parameter<int>( 0 ) );

TOOL_ACTION PCB_ACTIONS::highlightItem( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.highlightItem" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::hideNetInRatsnest( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.hideNet" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Hide Net in Ratsnest" ) )
        .Tooltip( _( "Hide the selected net in the ratsnest of unconnected net lines/arcs" ) )
        .Icon( BITMAPS::hide_ratsnest )
        .Parameter<int>( 0 ) );    // Default to hiding selected net

TOOL_ACTION PCB_ACTIONS::showNetInRatsnest( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.showNet" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Net in Ratsnest" ) )
        .Tooltip( _( "Show the selected net in the ratsnest of unconnected net lines/arcs" ) )
        .Icon( BITMAPS::show_ratsnest )
        .Parameter<int>( 0 ) );    // Default to showing selected net

TOOL_ACTION PCB_ACTIONS::showEeschema( TOOL_ACTION_ARGS()
        .Name( "pcbnew.EditorControl.showEeschema" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Switch to Schematic Editor" ) )
        .Tooltip( _( "Open schematic in schematic editor" ) )
        .Icon( BITMAPS::icon_eeschema_24 ) );


// PCB_CONTROL
//

TOOL_ACTION PCB_ACTIONS::localRatsnestTool( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.localRatsnestTool" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Local Ratsnest" ) )
        .Tooltip( _( "Toggle ratsnest display of selected item(s)" ) )
        .Icon( BITMAPS::tool_ratsnest )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::hideLocalRatsnest( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.hideDynamicRatsnest" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::updateLocalRatsnest( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.updateLocalRatsnest" )
        .Scope( AS_GLOBAL )
        .Parameter( VECTOR2I() ) );

TOOL_ACTION PCB_ACTIONS::showPythonConsole( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.showPythonConsole" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Scripting Console" ) )
        .Tooltip( _( "Show the Python scripting console" ) )
        .Icon( BITMAPS::py_script ) );

TOOL_ACTION PCB_ACTIONS::showLayersManager( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.showLayersManager" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Appearance Manager" ) )
        .Tooltip( _( "Show/hide the appearance manager" ) )
        .Icon( BITMAPS::layers_manager ) );

TOOL_ACTION PCB_ACTIONS::showNetInspector( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.showNetInspector" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Net Inspector" ) )
        .Tooltip( _( "Show/hide the net inspector" ) )
        .Icon( BITMAPS::tools ) );

TOOL_ACTION PCB_ACTIONS::zonesManager( "pcbnew.Control.zonesManager",
        AS_GLOBAL, 0, "",
        _( "Zone Manager" ), _( "Show the zone manager dialog" ),
        BITMAPS::show_zone );

TOOL_ACTION PCB_ACTIONS::flipBoard( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.flipBoard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Flip Board View" ) )
        .Tooltip( _( "View board from the opposite side" ) )
        .Icon( BITMAPS::flip_board ) );

// Display modes
TOOL_ACTION PCB_ACTIONS::showRatsnest( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.showRatsnest" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Ratsnest" ) )
        .Tooltip( _( "Show board ratsnest" ) )
        .Icon( BITMAPS::general_ratsnest ) );

TOOL_ACTION PCB_ACTIONS::ratsnestLineMode( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.ratsnestLineMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Curved Ratsnest Lines" ) )
        .Tooltip( _( "Show ratsnest with curved lines" ) )
        .Icon( BITMAPS::curved_ratsnest ) );

TOOL_ACTION PCB_ACTIONS::ratsnestModeCycle( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.ratsnestModeCycle" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Ratsnest Mode (3-state)" ) )
        .Tooltip( _( "Cycle between showing ratsnests for all layers, just visible layers, and none" ) ) );

TOOL_ACTION PCB_ACTIONS::netColorModeCycle( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.netColorMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Net Color Mode (3-state)" ) )
        .Tooltip( _( "Cycle between using net and netclass colors for all nets, just ratsnests, and none" ) ) );

TOOL_ACTION PCB_ACTIONS::trackDisplayMode( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.trackDisplayMode" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'K' )
        .LegacyHotkeyName( "Track Display Mode" )
        .FriendlyName( _( "Sketch Tracks" ) )
        .Tooltip( _( "Show tracks in outline mode" ) )
        .Icon( BITMAPS::showtrack ) );

TOOL_ACTION PCB_ACTIONS::padDisplayMode( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.padDisplayMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Sketch Pads" ) )
        .Tooltip( _( "Show pads in outline mode" ) )
        .Icon( BITMAPS::pad_sketch ) );

TOOL_ACTION PCB_ACTIONS::viaDisplayMode( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.viaDisplayMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Sketch Vias" ) )
        .Tooltip( _( "Show vias in outline mode" ) )
        .Icon( BITMAPS::via_sketch ) );

TOOL_ACTION PCB_ACTIONS::graphicsOutlines( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.graphicOutlines" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Sketch Graphic Items" ) )
        .Tooltip( _( "Show graphic items in outline mode" ) )
        .Icon( BITMAPS::show_mod_edge ) );

TOOL_ACTION PCB_ACTIONS::textOutlines( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.textOutlines" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Sketch Text Items" ) )
        .Tooltip( _( "Show footprint texts in line mode" ) )
        .Icon( BITMAPS::text_sketch ) );

TOOL_ACTION PCB_ACTIONS::showPadNumbers( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.showPadNumbers" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Pad Numbers" ) )
        .Tooltip( _( "Show pad numbers" ) )
        .Icon( BITMAPS::pad_number ) );

TOOL_ACTION PCB_ACTIONS::zoneDisplayFilled( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.zoneDisplayEnable" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Zone Fills" ) )
        .Tooltip( _( "Show filled areas of zones" ) )
        .Icon( BITMAPS::show_zone ) );

TOOL_ACTION PCB_ACTIONS::zoneDisplayOutline( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.zoneDisplayDisable" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Zone Outlines" ) )
        .Tooltip( _( "Show only zone boundaries" ) )
        .Icon( BITMAPS::show_zone_disable ) );

TOOL_ACTION PCB_ACTIONS::zoneDisplayFractured( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.zoneDisplayOutlines" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Zone Fill Fracture Borders" ) )
        .Icon( BITMAPS::show_zone_outline_only ) );

TOOL_ACTION PCB_ACTIONS::zoneDisplayTriangulated( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.zoneDisplayTesselation" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Zone Fill Triangulation" ) )
        .Icon( BITMAPS::show_zone_triangulation ) );

TOOL_ACTION PCB_ACTIONS::zoneDisplayToggle( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.zoneDisplayToggle" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Toggle Zone Display" ) )
        .Tooltip( _( "Cycle between showing zone fills and just their outlines" ) )
        .Icon( BITMAPS::show_zone ) );


// Layer control

// Translate aLayer to the action that switches to it
TOOL_ACTION* PCB_ACTIONS::LayerIDToAction( PCB_LAYER_ID aLayer )
{
    switch( aLayer )
    {
    case F_Cu:      return &PCB_ACTIONS::layerTop;
    case In1_Cu:    return &PCB_ACTIONS::layerInner1;
    case In2_Cu:    return &PCB_ACTIONS::layerInner2;
    case In3_Cu:    return &PCB_ACTIONS::layerInner3;
    case In4_Cu:    return &PCB_ACTIONS::layerInner4;
    case In5_Cu:    return &PCB_ACTIONS::layerInner5;
    case In6_Cu:    return &PCB_ACTIONS::layerInner6;
    case In7_Cu:    return &PCB_ACTIONS::layerInner7;
    case In8_Cu:    return &PCB_ACTIONS::layerInner8;
    case In9_Cu:    return &PCB_ACTIONS::layerInner9;
    case In10_Cu:   return &PCB_ACTIONS::layerInner10;
    case In11_Cu:   return &PCB_ACTIONS::layerInner11;
    case In12_Cu:   return &PCB_ACTIONS::layerInner12;
    case In13_Cu:   return &PCB_ACTIONS::layerInner13;
    case In14_Cu:   return &PCB_ACTIONS::layerInner14;
    case In15_Cu:   return &PCB_ACTIONS::layerInner15;
    case In16_Cu:   return &PCB_ACTIONS::layerInner16;
    case In17_Cu:   return &PCB_ACTIONS::layerInner17;
    case In18_Cu:   return &PCB_ACTIONS::layerInner18;
    case In19_Cu:   return &PCB_ACTIONS::layerInner19;
    case In20_Cu:   return &PCB_ACTIONS::layerInner20;
    case In21_Cu:   return &PCB_ACTIONS::layerInner21;
    case In22_Cu:   return &PCB_ACTIONS::layerInner22;
    case In23_Cu:   return &PCB_ACTIONS::layerInner23;
    case In24_Cu:   return &PCB_ACTIONS::layerInner24;
    case In25_Cu:   return &PCB_ACTIONS::layerInner25;
    case In26_Cu:   return &PCB_ACTIONS::layerInner26;
    case In27_Cu:   return &PCB_ACTIONS::layerInner27;
    case In28_Cu:   return &PCB_ACTIONS::layerInner28;
    case In29_Cu:   return &PCB_ACTIONS::layerInner29;
    case In30_Cu:   return &PCB_ACTIONS::layerInner30;
    case B_Cu:      return &PCB_ACTIONS::layerBottom;
    default:        return nullptr;
    }
}

// Implemented as an accessor + static variable to ensure it is initialized when used
// in static action constructors
TOOL_ACTION_GROUP PCB_ACTIONS::layerDirectSwitchActions()
{
    static TOOL_ACTION_GROUP s_toolActionGroup( "pcbnew.Control.DirectLayerActions" );
    return s_toolActionGroup;
}

TOOL_ACTION PCB_ACTIONS::layerTop( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerTop" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .DefaultHotkey( WXK_PAGEUP )
        .LegacyHotkeyName( "Switch to Component (F.Cu) layer" )
        .FriendlyName( _( "Switch to Component (F.Cu) layer" ) )
        .Flags( AF_NOTIFY )
        .Parameter( F_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner1( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner1" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .LegacyHotkeyName( "Switch to Inner layer 1" )
        .FriendlyName( _( "Switch to Inner Layer 1" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In1_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner2( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner2" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .LegacyHotkeyName( "Switch to Inner layer 2" )
        .FriendlyName( _( "Switch to Inner Layer 2" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In2_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner3( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner3" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .LegacyHotkeyName( "Switch to Inner layer 3" )
        .FriendlyName( _( "Switch to Inner Layer 3" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In3_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner4( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner4" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .LegacyHotkeyName( "Switch to Inner layer 4" )
        .FriendlyName( _( "Switch to Inner Layer 4" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In4_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner5( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner5" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .LegacyHotkeyName( "Switch to Inner layer 5" )
        .FriendlyName( _( "Switch to Inner Layer 5" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In5_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner6( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner6" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .LegacyHotkeyName( "Switch to Inner layer 6" )
        .FriendlyName( _( "Switch to Inner Layer 6" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In6_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner7( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner7" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 7" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In7_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner8( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner8" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 8" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In8_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner9( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner9" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 9" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In9_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner10( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner10" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 10" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In10_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner11( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner11" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 11" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In11_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner12( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner12" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 12" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In12_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner13( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner13" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 13" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In13_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner14( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner14" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 14" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In14_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner15( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner15" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 15" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In15_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner16( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner16" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 16" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In16_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner17( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner17" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 17" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In17_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner18( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner18" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 18" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In18_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner19( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner19" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 19" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In19_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner20( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner20" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 20" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In20_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner21( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner21" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 21" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In21_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner22( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner22" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 22" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In22_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner23( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner23" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 23" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In23_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner24( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner24" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 24" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In24_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner25( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner25" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 25" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In25_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner26( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner26" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 26" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In26_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner27( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner27" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 27" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In27_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner28( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner28" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 28" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In28_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner29( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner29" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 29" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In29_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerInner30( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerInner30" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .FriendlyName( _( "Switch to Inner Layer 30" ) )
        .Flags( AF_NOTIFY )
        .Parameter( In30_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerBottom( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerBottom" )
        .Scope( AS_GLOBAL )
        .Group( PCB_ACTIONS::layerDirectSwitchActions() )
        .DefaultHotkey( WXK_PAGEDOWN )
        .LegacyHotkeyName( "Switch to Copper (B.Cu) layer" )
        .FriendlyName( _( "Switch to Copper (B.Cu) Layer" ) )
        .Flags( AF_NOTIFY )
        .Parameter( B_Cu ) );

TOOL_ACTION PCB_ACTIONS::layerNext( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerNext" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '+' )
        .LegacyHotkeyName( "Switch to Next Layer" )
        .FriendlyName( _( "Switch to Next Layer" ) )
        .Flags( AF_NOTIFY ) );

TOOL_ACTION PCB_ACTIONS::layerPrev( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerPrev" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '-' )
        .LegacyHotkeyName( "Switch to Previous Layer" )
        .FriendlyName( _( "Switch to Previous Layer" ) )
        .Flags( AF_NOTIFY ) );

TOOL_ACTION PCB_ACTIONS::layerToggle( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerToggle" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'V' )
        .LegacyHotkeyName( "Add Through Via" )
        .FriendlyName( _( "Toggle Layer" ) )
        .Tooltip( _( "Switch between layers in active layer pair" ) )
        .Flags( AF_NOTIFY ) );

TOOL_ACTION PCB_ACTIONS::layerAlphaInc( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerAlphaInc" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '}' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Increment Layer Transparency (Modern Toolset only)" )
        .FriendlyName( _( "Increase Layer Opacity" ) )
        .Tooltip( _( "Make the current layer less transparent" ) )
        .Icon( BITMAPS::contrast_mode ) );

TOOL_ACTION PCB_ACTIONS::layerAlphaDec( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerAlphaDec" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '{' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Decrement Layer Transparency (Modern Toolset only)" )
        .FriendlyName( _( "Decrease Layer Opacity" ) )
        .Tooltip( _( "Make the current layer more transparent" ) )
        .Icon( BITMAPS::contrast_mode ) );

TOOL_ACTION PCB_ACTIONS::layerPairPresetsCycle( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerPairPresetCycle" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'V' )
        .FriendlyName( _( "Cycle Layer Pair Presets" ) )
        .Tooltip( _( "Cycle between preset layer pairs" ) ) );

TOOL_ACTION PCB_ACTIONS::layerChanged( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.layerChanged" )
        .Scope( AS_GLOBAL )
        .Flags( AF_NOTIFY ) );

//Show board statistics tool
TOOL_ACTION PCB_ACTIONS::boardStatistics( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InspectionTool.ShowBoardStatistics" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Board Statistics" ) )
        .Tooltip( _( "Shows board statistics" ) ) );

TOOL_ACTION PCB_ACTIONS::inspectClearance( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InspectionTool.InspectClearance" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Clearance Resolution" ) )
        .Tooltip( _( "Show clearance resolution for the active layer between two selected objects" ) )
        .Icon( BITMAPS::mw_add_gap ) );

TOOL_ACTION PCB_ACTIONS::inspectConstraints( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InspectionTool.InspectConstraints" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Constraints Resolution" ) )
        .Tooltip( _( "Show constraints resolution for the selected object" ) )
        .Icon( BITMAPS::mw_add_gap ) );

TOOL_ACTION PCB_ACTIONS::diffFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InspectionTool.DiffFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Compare Footprint with Library" ) )
        .Tooltip( _( "Show differences between board footprint and its library equivalent" ) )
        .Icon( BITMAPS::library ) );

TOOL_ACTION PCB_ACTIONS::showFootprintAssociations( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InspectionTool.ShowFootprintAssociations" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Footprint Associations" ) )
        .Tooltip( _( "Show footprint library and schematic symbol associations" ) )
        .Icon( BITMAPS::edit_cmp_symb_links ) );

//Geographic re-annotation tool
TOOL_ACTION PCB_ACTIONS::boardReannotate( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ReannotateTool.ShowReannotateDialog" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Geographical Reannotate..." ) )
        .Tooltip( _( "Reannotate PCB in geographical order" ) )
        .Icon( BITMAPS::annotate ) );

TOOL_ACTION PCB_ACTIONS::repairBoard( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.repairBoard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Repair Board" ) )
        .Tooltip( _( "Run various diagnostics and attempt to repair board" ) )
        .Icon( BITMAPS::rescue )
        .Parameter( false ) );  // Don't repair quietly

TOOL_ACTION PCB_ACTIONS::repairFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ModuleEditor.repairFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Repair Footprint" ) )
        .Tooltip( _( "Run various diagnostics and attempt to repair footprint" ) ) );


// PLACEMENT_TOOL
//
TOOL_ACTION PCB_ACTIONS::alignTop( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.alignTop" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Align to Top" ) )
        .Tooltip( _( "Aligns selected items to the top edge of the item under the cursor" ) )
        .Icon( BITMAPS::align_items_top ) );

TOOL_ACTION PCB_ACTIONS::alignBottom( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.alignBottom" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Align to Bottom" ) )
        .Tooltip( _( "Aligns selected items to the bottom edge of the item under the cursor" ) )
        .Icon( BITMAPS::align_items_bottom ) );

TOOL_ACTION PCB_ACTIONS::alignLeft( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.alignLeft" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Align to Left" ) )
        .Tooltip( _( "Aligns selected items to the left edge of the item under the cursor" ) )
        .Icon( BITMAPS::align_items_left ) );

TOOL_ACTION PCB_ACTIONS::alignRight( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.alignRight" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Align to Right" ) )
        .Tooltip( _( "Aligns selected items to the right edge of the item under the cursor" ) )
        .Icon( BITMAPS::align_items_right ) );

TOOL_ACTION PCB_ACTIONS::alignCenterY( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.alignCenterY" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Align to Vertical Center" ) )
        .Tooltip( _( "Aligns selected items to the vertical center of the item under the cursor" ) )
        .Icon( BITMAPS::align_items_center ) );

TOOL_ACTION PCB_ACTIONS::alignCenterX( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.alignCenterX" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Align to Horizontal Center" ) )
        .Tooltip( _( "Aligns selected items to the horizontal center of the item under the cursor" ) )
        .Icon( BITMAPS::align_items_middle ) );

TOOL_ACTION PCB_ACTIONS::distributeHorizontallyCenters( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.distributeHorizontallyCenters" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Distribute Horizontally by Centers" ) )
        .Tooltip( _( "Distributes selected items between the left-most item and the right-most item"
                     "so that the item centers are equally distributed" ) )
        .Icon( BITMAPS::distribute_horizontal_centers ) );

TOOL_ACTION PCB_ACTIONS::distributeHorizontallyGaps( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.distributeHorizontallyGaps" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Distribute Horizontally with Even Gaps" ) )
        .Tooltip( _( "Distributes selected items between the left-most item and the right-most item "
                     "so that the gaps between items are equal" ) )
        .Icon( BITMAPS::distribute_horizontal_gaps ) );

TOOL_ACTION PCB_ACTIONS::distributeVerticallyGaps( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.distributeVerticallyGaps" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Distribute Vertically with Even Gaps" ) )
        .Tooltip( _( "Distributes selected items between the top-most item and the bottom-most item "
                     "so that the gaps between items are equal" ) )
        .Icon( BITMAPS::distribute_vertical_gaps ) );

TOOL_ACTION PCB_ACTIONS::distributeVerticallyCenters( TOOL_ACTION_ARGS()
        .Name( "pcbnew.AlignAndDistribute.distributeVerticallyCenters" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Distribute Vertically by Centers" ) )
        .Tooltip( _( "Distributes selected items between the top-most item and the bottom-most item "
                     "so that the item centers are equally distributed" ) )
        .Icon( BITMAPS::distribute_vertical_centers ) );

// PCB_POINT_EDITOR
//
TOOL_ACTION PCB_ACTIONS::pointEditorAddCorner( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PointEditor.addCorner" )
        .Scope( AS_GLOBAL )
#ifdef __WXMAC__
        .DefaultHotkey( WXK_F1 )
#else
        .DefaultHotkey( WXK_INSERT )
#endif
        .FriendlyName( _( "Create Corner" ) )
        .Tooltip( _( "Create a corner" ) )
        .Icon( BITMAPS::add_corner ) );

TOOL_ACTION PCB_ACTIONS::pointEditorRemoveCorner( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PointEditor.removeCorner" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Remove Corner" ) )
        .Tooltip( _( "Remove corner" ) )
        .Icon( BITMAPS::delete_cursor ) );

TOOL_ACTION PCB_ACTIONS::pointEditorArcKeepCenter( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PointEditor.arcKeepCenter" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Keep Arc Center, Adjust Radius" ) )
        .Tooltip( _( "Switch arc editing mode to keep center, adjust radius and endpoints" ) )
        .Parameter( ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ) );

TOOL_ACTION PCB_ACTIONS::pointEditorArcKeepEndpoint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PointEditor.arcKeepEndpoint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Keep Arc Endpoints or Direction of Starting Point" ) )
        .Tooltip( _( "Switch arc editing mode to keep endpoints, or to keep direction of the other point" ) )
        .Parameter( ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION ) );


// GROUP_TOOL
//
TOOL_ACTION PCB_ACTIONS::groupProperties( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Groups.groupProperties" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::pickNewGroupMember( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Groups.selectNewGroupMember" )
        .Scope( AS_GLOBAL ) );



// POSITION_RELATIVE_TOOL
//
TOOL_ACTION PCB_ACTIONS::positionRelative( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PositionRelative.positionRelative" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'P' )
        .LegacyHotkeyName( "Position Item Relative" )
        .FriendlyName( _( "Position Relative To..." ) )
        .Tooltip( _( "Positions the selected item(s) by an exact amount relative to another" ) )
        .Icon( BITMAPS::move_relative ) );

TOOL_ACTION PCB_ACTIONS::selectpositionRelativeItem( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PositionRelative.selectpositionRelativeItem" )
        .Scope( AS_GLOBAL ) );


// PCB_SELECTION_TOOL
//
TOOL_ACTION PCB_ACTIONS::selectionActivate( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection" )
        .Scope( AS_GLOBAL )
        // No description, not shown anywhere
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::selectionCursor( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.Cursor" )
        .Scope( AS_GLOBAL )
        .Parameter<CLIENT_SELECTION_FILTER>( nullptr ) );

TOOL_ACTION PCB_ACTIONS::selectItem( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SelectItem" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::selectItems( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SelectItems" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::unselectItem( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.UnselectItem" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::unselectItems( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.UnselectItems" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::reselectItem( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.ReselectItem" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::selectionClear( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.Clear" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::selectionMenu( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SelectionMenu" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::selectConnection( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SelectConnection" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'U' )
        .LegacyHotkeyName( "Select Single Track" )
        .FriendlyName( _( "Select/Expand Connection" ) )
        .Tooltip( _( "Selects a connection or expands an existing selection to junctions, pads, or entire connections" ) )
        .Icon( BITMAPS::add_tracks ) );

TOOL_ACTION PCB_ACTIONS::unrouteSelected( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.unrouteSelected" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Unroute Selected" ) )
        .Tooltip( _( "Unroutes selected items to the nearest pad." ) )
        .Icon( BITMAPS::general_deletions ) );

TOOL_ACTION PCB_ACTIONS::syncSelection( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SyncSelection" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::syncSelectionWithNets( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SyncSelectionWithNets" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION PCB_ACTIONS::selectNet( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SelectNet" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Select All Tracks in Net" ) )
        .Tooltip( _( "Selects all tracks & vias belonging to the same net." ) )
        .Parameter<int>( 0 ) );

TOOL_ACTION PCB_ACTIONS::deselectNet( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.DeselectNet" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Deselect All Tracks in Net" ) )
        .Tooltip( _( "Deselects all tracks & vias belonging to the same net." ) )
        .Parameter<int>( 0 ) );

TOOL_ACTION PCB_ACTIONS::selectUnconnected( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SelectUnconnected" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'O' )
        .FriendlyName( _( "Select All Unconnected Footprints" ) )
        .Tooltip( _( "Selects all unconnected footprints belonging to each selected net." ) ) );

TOOL_ACTION PCB_ACTIONS::grabUnconnected( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.GrabUnconnected" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'O' )
        .FriendlyName( _( "Grab Nearest Unconnected Footprints" ) )
        .Tooltip( _( "Selects and initiates moving the nearest unconnected footprint on each selected net." ) ) );

TOOL_ACTION PCB_ACTIONS::selectOnSheetFromEeschema( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SelectOnSheet" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Sheet" ) )
        .Tooltip( _( "Selects all footprints and tracks in the schematic sheet" ) )
        .Icon( BITMAPS::select_same_sheet ) );

TOOL_ACTION PCB_ACTIONS::selectSameSheet( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SelectSameSheet" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Items in Same Hierarchical Sheet" ) )
        .Tooltip( _( "Selects all footprints and tracks in the same schematic sheet" ) )
        .Icon( BITMAPS::select_same_sheet ) );

TOOL_ACTION PCB_ACTIONS::selectOnSchematic( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.SelectOnSchematic" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Select on Schematic" ) )
        .Tooltip( _( "Selects corresponding items in Schematic editor" ) )
        .Icon( BITMAPS::select_same_sheet ) );

TOOL_ACTION PCB_ACTIONS::filterSelection( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveSelection.FilterSelection" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Filter Selected Items..." ) )
        .Tooltip( _( "Remove items from the selection by type" ) )
        .Icon( BITMAPS::filter ) );


// ZONE_FILLER_TOOL
//
TOOL_ACTION PCB_ACTIONS::zoneFill( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ZoneFiller.zoneFill" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draft Fill Selected Zone(s)" ) )
        .Tooltip( _( "Update copper fill of selected zone(s) without regard to other interacting zones" ) )
        .Icon( BITMAPS::fill_zone )
        .Parameter<ZONE*>( nullptr ) );

TOOL_ACTION PCB_ACTIONS::zoneFillAll( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ZoneFiller.zoneFillAll" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'B' )
        .LegacyHotkeyName( "Fill or Refill All Zones" )
        .FriendlyName( _( "Fill All Zones" ) )
        .Tooltip( _( "Update copper fill of all zones" ) )
        .Icon( BITMAPS::fill_zone ) );

TOOL_ACTION PCB_ACTIONS::zoneFillDirty( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ZoneFiller.zoneFillDirty" )
        .Scope( AS_CONTEXT ) );

TOOL_ACTION PCB_ACTIONS::zoneUnfill( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ZoneFiller.zoneUnfill" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Unfill Selected Zone(s)" ) )
        .Tooltip( _( "Remove copper fill from selected zone(s)" ) )
        .Icon( BITMAPS::zone_unfill ) );

TOOL_ACTION PCB_ACTIONS::zoneUnfillAll( TOOL_ACTION_ARGS()
        .Name( "pcbnew.ZoneFiller.zoneUnfillAll" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'B' )
        .LegacyHotkeyName( "Remove Filled Areas in All Zones" )
        .FriendlyName( _( "Unfill All Zones" ) )
        .Tooltip( _( "Remove copper fill from all zones" ) )
        .Icon( BITMAPS::zone_unfill ) );


// AUTOPLACER_TOOL
//
TOOL_ACTION PCB_ACTIONS::autoplaceSelectedComponents( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Autoplacer.autoplaceSelected" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Place Selected Footprints" ) )
        .Tooltip( _( "Performs automatic placement of selected components" ) ) );

TOOL_ACTION PCB_ACTIONS::autoplaceOffboardComponents( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Autoplacer.autoplaceOffboard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Place Off-Board Footprints" ) )
        .Tooltip( _( "Performs automatic placement of components outside board area" ) ) );


// ROUTER_TOOL
//
TOOL_ACTION PCB_ACTIONS::routeSingleTrack( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SingleTrack" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'X' )
        .LegacyHotkeyName( "Add New Track" )
        .FriendlyName( _( "Route Single Track" ) )
        .Tooltip( _( "Route tracks" ) )
        .Icon( BITMAPS::add_tracks )
        .Flags( AF_ACTIVATE )
        .Parameter( PNS::PNS_MODE_ROUTE_SINGLE ) );

TOOL_ACTION PCB_ACTIONS::routeDiffPair( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.DiffPair" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '6' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Route Differential Pair (Modern Toolset only)" )
        .FriendlyName( _( "Route Differential Pair" ) )
        .Tooltip( _( "Route differential pairs" ) )
        .Icon( BITMAPS::ps_diff_pair )
        .Flags( AF_ACTIVATE )
        .Parameter( PNS::PNS_MODE_ROUTE_DIFF_PAIR ) );

TOOL_ACTION PCB_ACTIONS::routerSettingsDialog( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SettingsDialog" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + '<' )
        .LegacyHotkeyName( "Routing Options" )
        .FriendlyName( _( "Interactive Router Settings..." ) )
        .Tooltip( _( "Open Interactive Router settings" ) )
        .Icon( BITMAPS::tools ) );

TOOL_ACTION PCB_ACTIONS::routerDiffPairDialog( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.DiffPairDialog" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Differential Pair Dimensions..." ) )
        .Tooltip( _( "Open Differential Pair Dimension settings" ) )
        .Icon( BITMAPS::ps_diff_pair_gap ) );

TOOL_ACTION PCB_ACTIONS::routerHighlightMode( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.HighlightMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Router Highlight Mode" ) )
        .Tooltip( _( "Switch router to highlight mode" ) )
        .Flags( AF_NONE )
        .Parameter( PNS::RM_MarkObstacles ) );

TOOL_ACTION PCB_ACTIONS::routerShoveMode( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.ShoveMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Router Shove Mode" ) )
        .Tooltip( _( "Switch router to shove mode" ) )
        .Flags( AF_NONE )
        .Parameter( PNS::RM_Shove ) );

TOOL_ACTION PCB_ACTIONS::routerWalkaroundMode( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.WalkaroundMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Router Walkaround Mode" ) )
        .Tooltip( _( "Switch router to walkaround mode" ) )
        .Flags( AF_NONE )
        .Parameter( PNS::RM_Walkaround ) );

TOOL_ACTION PCB_ACTIONS::cycleRouterMode( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.CycleRouterMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Cycle Router Mode" ) )
        .Tooltip( _( "Cycle router to the next mode" ) ) );

TOOL_ACTION PCB_ACTIONS::selectLayerPair( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.SelectLayerPair" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Set Layer Pair..." ) )
        .Tooltip( _( "Change active layer pair for routing" ) )
        .Icon( BITMAPS::select_layer_pair )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PCB_ACTIONS::tuneSingleTrack( TOOL_ACTION_ARGS()
        .Name( "pcbnew.LengthTuner.TuneSingleTrack" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '7' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Tune Single Track (Modern Toolset only)" )
        .FriendlyName( _( "Tune Length of a Single Track" ) )
        .Tooltip( _( "Tune length of a single track" ) )
        .Icon( BITMAPS::ps_tune_length )
        .Flags( AF_ACTIVATE )
        .Parameter( PNS::PNS_MODE_TUNE_SINGLE ) );

TOOL_ACTION PCB_ACTIONS::tuneDiffPair( TOOL_ACTION_ARGS()
        .Name( "pcbnew.LengthTuner.TuneDiffPair" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '8' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Tune Differential Pair Length (Modern Toolset only)" )
        .FriendlyName( _( "Tune Length of a Differential Pair" ) )
        .Tooltip( _( "Tune length of a differential pair" ) )
        .Icon( BITMAPS::ps_diff_pair_tune_length )
        .Flags( AF_ACTIVATE )
        .Parameter( PNS::PNS_MODE_TUNE_DIFF_PAIR ) );

TOOL_ACTION PCB_ACTIONS::tuneSkew( TOOL_ACTION_ARGS()
        .Name( "pcbnew.LengthTuner.TuneDiffPairSkew" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '9' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Tune Differential Pair Skew (Modern Toolset only)" )
        .FriendlyName( _( "Tune Skew of a Differential Pair" ) )
        .Tooltip( _( "Tune skew of a differential pair" ) )
        .Icon( BITMAPS::ps_diff_pair_tune_phase )
        .Flags( AF_ACTIVATE )
        .Parameter( PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW ) );

TOOL_ACTION PCB_ACTIONS::routerInlineDrag( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.InlineDrag" )
        .Scope( AS_CONTEXT )
        .Parameter<int>( PNS::DM_ANY ) );

TOOL_ACTION PCB_ACTIONS::routerUndoLastSegment( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.UndoLastSegment" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( WXK_BACK )
        .FriendlyName( _( "Undo Last Segment" ) )
        .Tooltip( _( "Walks the current track back one segment." ) ) );

TOOL_ACTION PCB_ACTIONS::routerContinueFromEnd( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.ContinueFromEnd" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( MD_CTRL + 'E' )
        .FriendlyName( _( "Route From Other End" ) )
        .Tooltip( _( "Commits current segments and starts next segment from nearest ratsnest end." ) ) );

TOOL_ACTION PCB_ACTIONS::routerAttemptFinish( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.AttemptFinish" )
        .Scope( AS_CONTEXT )
        .DefaultHotkey( 'F' )
        .FriendlyName( _( "Attempt Finish" ) )
        .Tooltip( _( "Attempts to complete current route to nearest ratsnest end." ) )
        .Parameter<bool*>( nullptr ) );

TOOL_ACTION PCB_ACTIONS::routerRouteSelected( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.RouteSelected" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'X' )
        .FriendlyName( _( "Route Selected" ) )
        .Tooltip( _( "Sequentially route selected items from ratsnest anchor." ) )
        .Flags( AF_ACTIVATE )
        .Parameter( PNS::PNS_MODE_ROUTE_SINGLE ) );

TOOL_ACTION PCB_ACTIONS::routerRouteSelectedFromEnd( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.RouteSelectedFromEnd" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'E' )
        .FriendlyName( _( "Route Selected From Other End" ) )
        .Tooltip( _( "Sequentially route selected items from other end of ratsnest anchor." ) )
        .Flags( AF_ACTIVATE )
        .Parameter( PNS::PNS_MODE_ROUTE_SINGLE ) );

TOOL_ACTION PCB_ACTIONS::routerAutorouteSelected( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.Autoroute" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'F' )
        .FriendlyName( _( "Attempt Finish Selected (Autoroute)" ) )
        .Tooltip( _( "Sequentially attempt to automatically route all selected pads." ) )
        .Flags( AF_ACTIVATE )
        .Parameter( PNS::PNS_MODE_ROUTE_SINGLE ) );

TOOL_ACTION PCB_ACTIONS::breakTrack( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.BreakTrack" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Break Track" ) )
        .Tooltip( _( "Splits the track segment into two segments connected at the cursor position." ) )
        .Icon( BITMAPS::break_line ) );

TOOL_ACTION PCB_ACTIONS::drag45Degree( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.Drag45Degree" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'D' )
        .LegacyHotkeyName( "Drag Track Keep Slope" )
        .FriendlyName( _( "Drag 45 Degree Mode" ) )
        .Tooltip( _( "Drags the track segment while keeping connected tracks at 45 degrees." ) )
        .Icon( BITMAPS::drag_segment_withslope ) );

TOOL_ACTION PCB_ACTIONS::dragFreeAngle( TOOL_ACTION_ARGS()
        .Name( "pcbnew.InteractiveRouter.DragFreeAngle" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'G' )
        .LegacyHotkeyName( "Drag Item" )
        .FriendlyName( _( "Drag Free Angle" ) )
        .Tooltip( _( "Drags the nearest joint in the track without restricting the track angle." ) )
        .Icon( BITMAPS::drag_segment ) );


// GENERATOR_TOOL
//

TOOL_ACTION PCB_ACTIONS::regenerateAllTuning( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Generator.regenerateAllTuning" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Update All Tuning Patterns" ) )
        .Tooltip( _( "Attempt to re-tune existing tuning patterns within their bounds" ) )
        .Icon( BITMAPS::router_len_tuner )
        .Parameter( wxString( wxS( "tuning_pattern" ) ) ) );

TOOL_ACTION PCB_ACTIONS::regenerateAll( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Generator.regenerateAll" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rebuild All Generators" ) )
        .Tooltip( _( "Rebuilds geometry of all generators" ) )
        .Icon( BITMAPS::refresh )
        .Parameter( wxString( wxS( "*" ) ) ) );

TOOL_ACTION PCB_ACTIONS::regenerateSelected( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Generator.regenerateSelected" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rebuild Selected Generators" ) )
        .Tooltip( _( "Rebuilds geometry of selected generator(s)" ) )
        .Icon( BITMAPS::refresh ) );


TOOL_ACTION PCB_ACTIONS::genStartEdit( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Generator.genStartEdit" )
        .Scope( AS_CONTEXT ) );

TOOL_ACTION PCB_ACTIONS::genUpdateEdit( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Generator.genUpdateEdit" )
        .Scope( AS_CONTEXT ) );

TOOL_ACTION PCB_ACTIONS::genPushEdit( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Generator.genPushEdit" )
        .Scope( AS_CONTEXT ) );

TOOL_ACTION PCB_ACTIONS::genRevertEdit( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Generator.genRevertEdit" )
        .Scope( AS_CONTEXT ) );

TOOL_ACTION PCB_ACTIONS::genRemove( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Generator.genRemove" )
        .Scope( AS_CONTEXT ) );


TOOL_ACTION PCB_ACTIONS::generatorsShowManager( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Generator.showManager" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Generators Manager" ) )
        .Tooltip( _( "Show a manager dialog for Generator objects" ) )
        .Icon( BITMAPS::pin_table ) );


// LENGTH_TUNER_TOOL
//
TOOL_ACTION PCB_ACTIONS::lengthTunerSettings( TOOL_ACTION_ARGS()
        .Name( "pcbnew.LengthTuner.Settings" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'L' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Length Tuning Settings (Modern Toolset only)" )
        .MenuText( _( "Length Tuning Settings..." ) )
        .Tooltip( _( "Displays tuning pattern properties dialog" ) )
        .Icon( BITMAPS::router_len_tuner_setup ) );

TOOL_ACTION PCB_ACTIONS::ddAppendBoard( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.DdAppendBoard" )
        .Scope( AS_GLOBAL ) );


TOOL_ACTION PCB_ACTIONS::ddImportFootprint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.Control.ddImportFootprint" )
        .Scope( AS_GLOBAL ) );


const TOOL_EVENT PCB_EVENTS::SnappingModeChangedByKeyEvent( TC_MESSAGE, TA_ACTION,
                                                "common.Interactive.snappingModeChangedByKey" );

const TOOL_EVENT PCB_EVENTS::LayerPairPresetChangedByKeyEvent( TC_MESSAGE, TA_ACTION,
                                                "pcbnew.Control.layerPairPresetChangedByKey" );