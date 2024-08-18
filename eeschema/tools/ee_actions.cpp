/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 CERN
 * Copyright (C) 2019-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <core/typeinfo.h>
#include <layer_ids.h>
#include <sch_bitmap.h>
#include <sch_line_wire_bus_tool.h>
#include <tools/ee_actions.h>
#include <tool/tool_action.h>

// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


// EE_INSPECTION_TOOL
//
TOOL_ACTION EE_ACTIONS::runERC( TOOL_ACTION_ARGS()
        .Name( "eeschema.InspectionTool.runERC" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Electrical Rules Checker" ) )
        .Tooltip( _( "Show the electrical rules checker window" ) )
        .Icon( BITMAPS::erc ) );

TOOL_ACTION EE_ACTIONS::checkSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.InspectionTool.checkSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Symbol Checker" ) )
        .Tooltip( _( "Show the symbol checker window" ) )
        .Icon( BITMAPS::erc ) );

TOOL_ACTION EE_ACTIONS::diffSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.InspectionTool.diffSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Compare Symbol with Library" ) )
        .Tooltip( _( "Show differences between schematic symbol and its library equivalent" ) )
        .Icon( BITMAPS::library ) );

TOOL_ACTION EE_ACTIONS::showBusSyntaxHelp( TOOL_ACTION_ARGS()
        .Name( "eeschema.InspectionTool.showBusSyntaxHelp" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Bus Syntax Help" ) )
        .Icon( BITMAPS::bus_definition_tool ) );

TOOL_ACTION EE_ACTIONS::showSimulator( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showSimulator" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Simulator" ) )
        .Tooltip( _( "Show simulation window for running SPICE or IBIS simulations." ) )
        .Icon( BITMAPS::simulator ) );

TOOL_ACTION EE_ACTIONS::showDatasheet( TOOL_ACTION_ARGS()
        .Name( "eeschema.InspectionTool.showDatasheet" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'D' )
        .LegacyHotkeyName( "Show Datasheet" )
        .FriendlyName( _( "Show Datasheet" ) )
        .Tooltip( _( "Open the datasheet in a browser" ) )
        .Icon( BITMAPS::datasheet ) );


// EE_POINT_EDITOR
//
TOOL_ACTION EE_ACTIONS::pointEditorAddCorner( TOOL_ACTION_ARGS()
        .Name( "eeschema.PointEditor.addCorner" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Create Corner" ) )
        .Icon( BITMAPS::add_corner ) );

TOOL_ACTION EE_ACTIONS::pointEditorRemoveCorner( TOOL_ACTION_ARGS()
        .Name( "eeschema.PointEditor.removeCorner" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Remove Corner" ) )
        .Icon( BITMAPS::delete_cursor ) );


// EE_SELECTION_TOOL
//
TOOL_ACTION EE_ACTIONS::selectionActivate( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection" )
        .Scope( AS_GLOBAL )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::selectNode( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection.SelectNode" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + '3' )
        .LegacyHotkeyName( "Select Node" )
        .Tooltip( _( "Select a connection item under the cursor" ) ) );

TOOL_ACTION EE_ACTIONS::selectConnection( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection.SelectConnection" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + '4' )
        .LegacyHotkeyName( "Select Connection" )
        .Tooltip( _( "Select a complete connection" ) )
        .Icon( BITMAPS::net_highlight_schematic ) );

TOOL_ACTION EE_ACTIONS::selectionMenu( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection.SelectionMenu" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION EE_ACTIONS::addItemToSel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection.AddItemToSel" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION EE_ACTIONS::addItemsToSel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection.AddItemsToSel" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION EE_ACTIONS::removeItemFromSel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection.RemoveItemFromSel" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION EE_ACTIONS::removeItemsFromSel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection.RemoveItemsFromSel" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION EE_ACTIONS::clearSelection( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection.ClearSelection" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION EE_ACTIONS::syncSelection( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveSelection.SyncSelection" )
        .Scope( AS_GLOBAL ) );

// SYMBOL_EDITOR_CONTROL
//
TOOL_ACTION EE_ACTIONS::saveLibraryAs( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.saveLibraryAs" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'S' )
        .LegacyHotkeyName( "Save As" )
        .FriendlyName( _( "Save Library As..." ) )
        .Tooltip( _( "Save the current library to a new file" ) ) );

TOOL_ACTION EE_ACTIONS::newSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.newSymbol" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'N' )
        .FriendlyName( _( "New Symbol..." ) )
        .Tooltip( _( "Create a new symbol in an existing library" ) )
        .Icon( BITMAPS::new_component ) );

TOOL_ACTION EE_ACTIONS::deriveFromExistingSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.deriveFromExistingSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Derive from Existing Symbol" ) )
        .Tooltip( _( "Create a new symbol, derived from an existing symbol" ) )
        .Icon( BITMAPS::new_component ) );

TOOL_ACTION EE_ACTIONS::editSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.editSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Symbol" ) )
        .Tooltip( _( "Show selected symbol on editor canvas" ) )
        .Icon( BITMAPS::edit ) );

TOOL_ACTION EE_ACTIONS::duplicateSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.duplicateSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Duplicate Symbol" ) )
        .Icon( BITMAPS::duplicate ) );

TOOL_ACTION EE_ACTIONS::renameSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.renameFootprint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rename Symbol..." ) )
        .Icon( BITMAPS::edit ) );

TOOL_ACTION EE_ACTIONS::saveSymbolCopyAs( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.saveSymbolCopyAs" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Save Copy As..." ) )
        .Tooltip( _( "Save a copy of the current symbol to a different library" ) )
        .Icon( BITMAPS::save_as ) );

TOOL_ACTION EE_ACTIONS::deleteSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.deleteSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Delete Symbol" ) )
        .Tooltip( _( "Remove the selected symbol from its library" ) )
        .Icon( BITMAPS::trash ) );

TOOL_ACTION EE_ACTIONS::cutSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.cutSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Cut" ) )
        .Icon( BITMAPS::cut ) );

TOOL_ACTION EE_ACTIONS::copySymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.copySymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Copy" ) )
        .Icon( BITMAPS::copy ) );

TOOL_ACTION EE_ACTIONS::pasteSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.pasteSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Paste Symbol" ) )
        .Icon( BITMAPS::paste ) );

TOOL_ACTION EE_ACTIONS::importSymbol( TOOL_ACTION_ARGS()
        .Name("eeschema.SymbolLibraryControl.importSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Import Symbol..." ) )
        .Tooltip( _( "Import a symbol to the current library" ) )
        .Icon( BITMAPS::import_part ) );

TOOL_ACTION EE_ACTIONS::updateSymbolFields( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.updateSymbolFields" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Update Symbol Fields..." ) )
        .Tooltip( _( "Update symbol to match changes made in parent symbol" ) )
        .Icon( BITMAPS::refresh ) );

TOOL_ACTION EE_ACTIONS::setUnitDisplayName( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.setUnitDisplayName" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Set Unit Display Name..." ) )
        .Tooltip( _( "Set the display name for a particular unit in a multi-unit symbol" ) ) );

TOOL_ACTION EE_ACTIONS::addSymbolToSchematic( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.addSymbolToSchematic" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Symbol to Schematic" ) )
        .Tooltip( _( "Add the current symbol to the schematic" ) )
        .Icon( BITMAPS::add_symbol_to_schematic ) );

TOOL_ACTION EE_ACTIONS::showElectricalTypes( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.showElectricalTypes" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Pin Electrical Types" ) )
        .Tooltip( _( "Annotate pins with their electrical types" ) )
        .Icon( BITMAPS::pin_show_etype ) );

TOOL_ACTION EE_ACTIONS::showPinNumbers( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.showPinNumbers" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Pin Numbers" ) )
        .Tooltip( _( "Annotate pins with their numbers" ) )
        .Icon( BITMAPS::pin ) );

TOOL_ACTION EE_ACTIONS::exportSymbolView( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.exportSymbolView" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export View as PNG..." ) )
        .Tooltip( _( "Create PNG file from the current view" ) )
        .Icon( BITMAPS::export_png ) );

TOOL_ACTION EE_ACTIONS::exportSymbolAsSVG( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.exportSymbolAsSVG" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Symbol as SVG..." ) )
        .Tooltip( _( "Create SVG file from the current symbol" ) )
        .Icon( BITMAPS::export_svg ) );

TOOL_ACTION EE_ACTIONS::toggleSyncedPinsMode( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.toggleSyncedPinsMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Synchronized Pins Mode" ) )
        .Tooltip( _( "Synchronized Pins Mode\n"
                     "When enabled propagates all changes (except pin numbers) to other units.\n"
                     "Enabled by default for multiunit parts with interchangeable units." ) )
        .Icon( BITMAPS::pin2pin ) );

TOOL_ACTION EE_ACTIONS::showHiddenPins( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.showHiddenPins" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Hidden Pins" ) )
        .Icon( BITMAPS::hidden_pin ) );

TOOL_ACTION EE_ACTIONS::showHiddenFields( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolLibraryControl.showHiddenFields" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Hidden Fields" ) )
        .Icon( BITMAPS::text_sketch ) );


// SYMBOL_EDITOR_DRAWING_TOOLS
//
TOOL_ACTION EE_ACTIONS::placeSymbolPin( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.placeSymbolPin" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'P' )
        .LegacyHotkeyName( "Create Pin" )
        .FriendlyName( _( "Draw Pins" ) )
        .Icon( BITMAPS::pin )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_PIN_T ) );

TOOL_ACTION EE_ACTIONS::placeSymbolText( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.placeSymbolText" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Text" ) )
        .Icon( BITMAPS::text )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_TEXT_T ) );

TOOL_ACTION EE_ACTIONS::drawSymbolTextBox( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.drawSymbolTextBox" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Text Boxes" ) )
        .Icon( BITMAPS::add_textbox )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::drawSymbolLines( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.drawSymbolLines" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Lines" ) )
        .Tooltip( _( "Draw connected graphic lines" ) )
        .Icon( BITMAPS::add_graphical_segments )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::POLY ) );

TOOL_ACTION EE_ACTIONS::drawSymbolPolygon( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.drawSymbolPolygon" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Polygons" ) )
        .Icon( BITMAPS::add_graphical_polygon )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::POLY ) );

TOOL_ACTION EE_ACTIONS::placeSymbolAnchor( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.placeSymbolAnchor" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Move Symbol Anchor" ) )
        .Icon( BITMAPS::anchor  )
        .Flags( AF_ACTIVATE ) );

// SYMBOL_EDITOR_PIN_TOOL
//
TOOL_ACTION EE_ACTIONS::pushPinLength( TOOL_ACTION_ARGS()
        .Name( "eeschema.PinEditing.pushPinLength" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Push Pin Length" ) )
        .Tooltip( _( "Copy pin length to other pins in symbol" ) )
        .Icon( BITMAPS::pin_size_to ) );

TOOL_ACTION EE_ACTIONS::pushPinNameSize( TOOL_ACTION_ARGS()
        .Name( "eeschema.PinEditing.pushPinNameSize" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Push Pin Name Size" ) )
        .Tooltip( _( "Copy pin name size to other pins in symbol" ) )
        .Icon( BITMAPS::pin_size_to ) );

TOOL_ACTION EE_ACTIONS::pushPinNumSize( TOOL_ACTION_ARGS()
        .Name( "eeschema.PinEditing.pushPinNumSize" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Push Pin Number Size" ) )
        .Tooltip( _( "Copy pin number size to other pins in symbol" ) )
        .Icon( BITMAPS::pin_size_to ) );


// SCH_DRAWING_TOOLS
//
TOOL_ACTION EE_ACTIONS::placeSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeSymbol" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'A' )
        .LegacyHotkeyName( "Add Symbol" )
        .FriendlyName( _( "Place Symbols" ) )
        .Icon( BITMAPS::add_component )
        .Flags( AF_ACTIVATE )
        .Parameter<SCH_SYMBOL*>( nullptr ) );

TOOL_ACTION EE_ACTIONS::placePower( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placePowerSymbol" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'P' )
        .LegacyHotkeyName( "Add Power" )
        .FriendlyName( _( "Place Power Symbols" ) )
        .Icon( BITMAPS::add_power )
        .Flags( AF_ACTIVATE )
        .Parameter<SCH_SYMBOL*>( nullptr ) );

TOOL_ACTION EE_ACTIONS::placeNoConnect( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeNoConnect" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'Q' )
        .LegacyHotkeyName( "Add No Connect Flag" )
        .FriendlyName( _( "Place No Connect Flags" ) )
        .Icon( BITMAPS::noconn )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_NO_CONNECT_T ) );

TOOL_ACTION EE_ACTIONS::placeJunction( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeJunction" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'J' )
        .LegacyHotkeyName( "Add Junction" )
        .FriendlyName( _( "Place Junctions" ) )
        .Icon( BITMAPS::add_junction )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_JUNCTION_T ) );

TOOL_ACTION EE_ACTIONS::placeBusWireEntry( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeBusWireEntry" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'Z' )
        .LegacyHotkeyName( "Add Wire Entry" )
        .FriendlyName( _( "Place Wire to Bus Entries" ) )
        .Icon( BITMAPS::add_line2bus )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_BUS_WIRE_ENTRY_T ) );

TOOL_ACTION EE_ACTIONS::placeLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeLabel" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'L' )
        .LegacyHotkeyName( "Add Label" )
        .FriendlyName( _( "Place Net Labels" ) )
        .Icon( BITMAPS::add_label )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::placeClassLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeClassLabel" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Place Net Class Directives" ) )
        .Icon( BITMAPS::add_class_flag )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::placeHierLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeHierarchicalLabel" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'H' )
        .LegacyHotkeyName( "Add Hierarchical Label" )
        .FriendlyName( _( "Place Hierarchical Labels" ) )
        .Icon( BITMAPS::add_hierarchical_label )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::drawSheet( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawSheet" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'S' )
        .LegacyHotkeyName( "Add Sheet" )
        .FriendlyName( _( "Draw Hierarchical Sheets" ) )
        .Icon( BITMAPS::add_hierarchical_subsheet )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_SHEET_T ) );

TOOL_ACTION EE_ACTIONS::placeSheetPin( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeSheetPin" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Place Sheet Pins" ) )
        .Icon( BITMAPS::add_hierar_pin )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::syncSheetPins( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.syncSheetPins" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Sync Sheet Pins" ) )
        .Tooltip( _( "Synchronize sheet pins and hierarchical labels" ) )
        .Icon( BITMAPS::import_hierarchical_label )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::syncAllSheetsPins( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.syncAllSheetsPins" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Sync Sheet Pins" ) )
        .Tooltip( _( "Synchronize sheet pins and hierarchical labels" ) )
        .Icon( BITMAPS::import_hierarchical_label )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::placeGlobalLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeGlobalLabel" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'L' )
        .LegacyHotkeyName( "Add Global Label" )
        .FriendlyName( _( "Place Global Labels" ) )
        .Icon( BITMAPS::add_glabel )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::placeSchematicText( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeSchematicText" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'T' )
        .LegacyHotkeyName( "Add Graphic Text" )
        .FriendlyName( _( "Draw Text" ) )
        .Icon( BITMAPS::text )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::drawTextBox( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawTextBox" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Text Boxes" ) )
        .Icon( BITMAPS::add_textbox )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::RECTANGLE ) );

TOOL_ACTION EE_ACTIONS::drawTable( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawTable" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Tables" ) )
        .Icon( BITMAPS::spreadsheet )   // JEY TODO
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::drawRectangle( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawRectangle" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Rectangles" ) )
        .Icon( BITMAPS::add_rectangle )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::RECTANGLE ) );

TOOL_ACTION EE_ACTIONS::drawCircle( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawCircle" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Circles" ) )
        .Icon( BITMAPS::add_circle )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::CIRCLE ) );

TOOL_ACTION EE_ACTIONS::drawArc( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawArc" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Arcs" ) )
        .Icon( BITMAPS::add_arc )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::ARC ) );

TOOL_ACTION EE_ACTIONS::placeImage( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeImage" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Place Images" ) )
        .Icon( BITMAPS::image )
        .Flags( AF_ACTIVATE )
        .Parameter<SCH_BITMAP*>( nullptr ) );

TOOL_ACTION EE_ACTIONS::drawRuleArea( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawRuleArea" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Rule Areas" ) )
        .Icon( BITMAPS::add_keepout_area )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::RECTANGLE ) );

TOOL_ACTION EE_ACTIONS::deleteLastPoint( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.deleteLastPoint" )
        .Scope( AS_CONTEXT )
        .FriendlyName( _( "Delete Last Point" ) )
        .Tooltip( _( "Delete the last point added to the current item" ) )
        .Icon( BITMAPS::undo ) );

TOOL_ACTION EE_ACTIONS::closeOutline( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.closeOutline" )
        .Scope( AS_CONTEXT )
        .FriendlyName( _( "Close Outline" ) )
        .Tooltip( _( "Close the in-progress outline" ) )
        .Icon( BITMAPS::checked_ok ) );


// SCH_EDIT_TOOL
//
TOOL_ACTION EE_ACTIONS::repeatDrawItem( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.repeatDrawItem" )
        .Scope( AS_GLOBAL )
#ifdef __WXMAC__
        .DefaultHotkey( WXK_F1 )
#else
        .DefaultHotkey( WXK_INSERT )
#endif
        .LegacyHotkeyName( "Repeat Last Item" )
        .FriendlyName( _( "Repeat Last Item" ) )
        .Tooltip( _( "Duplicates the last drawn item" ) ) );

TOOL_ACTION EE_ACTIONS::rotateCW( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.rotateCW" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rotate Clockwise" ) )
        .Icon( BITMAPS::rotate_cw ) );

TOOL_ACTION EE_ACTIONS::rotateCCW( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.rotateCCW" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'R' )
        .LegacyHotkeyName( "Rotate Item" )
        .FriendlyName( _( "Rotate Counterclockwise" ) )
        .Icon( BITMAPS::rotate_ccw ) );

TOOL_ACTION EE_ACTIONS::mirrorV( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.mirrorV" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'Y' )
        .LegacyHotkeyName( "Mirror X" )  // Yes, these were backwards prior to 6.0....
        .FriendlyName( _( "Mirror Vertically" ) )
        .Tooltip( _( "Flips selected item(s) from top to bottom" ) )
        .Icon( BITMAPS::mirror_v ) );

TOOL_ACTION EE_ACTIONS::mirrorH( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.mirrorH" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'X' )
        .LegacyHotkeyName( "Mirror Y" )  // Yes, these were backwards prior to 6.0....
        .FriendlyName( _( "Mirror Horizontally" ) )
        .Tooltip( _( "Flips selected item(s) from left to right" ) )
        .Icon( BITMAPS::mirror_h ) );

TOOL_ACTION EE_ACTIONS::swap( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.swap" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'S' )
        .FriendlyName( _( "Swap" ) )
        .Tooltip( _( "Swap positions of selected items" ) )
        .Icon( BITMAPS::swap ) );

TOOL_ACTION EE_ACTIONS::properties( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.properties" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'E' )
        .LegacyHotkeyName( "Edit Item" )
        .FriendlyName( _( "Properties..." ) )
        .Icon( BITMAPS::edit ) );

TOOL_ACTION EE_ACTIONS::editReference( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.editReference" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'U' )
        .LegacyHotkeyName( "Edit Symbol Reference" )
        .FriendlyName( _( "Edit Reference Designator..." ) )
        .Icon( BITMAPS::edit_comp_ref ) );

TOOL_ACTION EE_ACTIONS::editValue( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.editValue" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'V' )
        .LegacyHotkeyName( "Edit Symbol Value" )
        .FriendlyName( _( "Edit Value..." ) )
        .Icon( BITMAPS::edit_comp_value ) );

TOOL_ACTION EE_ACTIONS::editFootprint( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.editFootprint" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'F' )
        .LegacyHotkeyName( "Edit Symbol Footprint" )
        .FriendlyName( _( "Edit Footprint..." ) )
        .Icon( BITMAPS::edit_comp_footprint ) );

TOOL_ACTION EE_ACTIONS::autoplaceFields( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.autoplaceFields" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'O' )
        .LegacyHotkeyName( "Autoplace Fields" )
        .FriendlyName( _( "Autoplace Fields" ) )
        .Tooltip( _( "Runs the automatic placement algorithm on the symbol's (or sheet's) fields" ) )
        .Icon( BITMAPS::autoplace_fields ) );

TOOL_ACTION EE_ACTIONS::changeSymbols( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.changeSymbols" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change Symbols..." ) )
        .Tooltip( _( "Assign different symbols from the library" ) )
        .Icon( BITMAPS::exchange ) );

TOOL_ACTION EE_ACTIONS::updateSymbols( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.updateSymbols" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Update Symbols from Library..." ) )
        .Tooltip( _( "Update symbols to include any changes from the library" ) )
        .Icon( BITMAPS::refresh ) );

TOOL_ACTION EE_ACTIONS::changeSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.changeSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change Symbol..." ) )
        .Tooltip( _( "Assign a different symbol from the library" ) )
        .Icon( BITMAPS::exchange ) );

TOOL_ACTION EE_ACTIONS::updateSymbol( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.updateSymbol" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Update Symbol..." ) )
        .Tooltip( _( "Update symbol to include any changes from the library" ) )
        .Icon( BITMAPS::refresh ) );

TOOL_ACTION EE_ACTIONS::assignNetclass( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.assignNetclass" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Assign Netclass..." ) )
        .Tooltip( _( "Assign a netclass to nets matching a pattern" ) )
        .Icon( BITMAPS::netlist ) );

TOOL_ACTION EE_ACTIONS::toggleDeMorgan( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toggleDeMorgan" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "De Morgan Conversion" ) )
        .Tooltip( _( "Switch between De Morgan representations" ) )
        .Icon( BITMAPS::morgan2 ) );

TOOL_ACTION EE_ACTIONS::showDeMorganStandard( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.showDeMorganStandard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "De Morgan Standard" ) )
        .Tooltip( _( "Switch to standard De Morgan representation" ) )
        .Icon( BITMAPS::morgan1 ) );

TOOL_ACTION EE_ACTIONS::showDeMorganAlternate( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.showDeMorganAlternate" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "De Morgan Alternate" ) )
        .Tooltip( _( "Switch to alternate De Morgan representation" ) )
        .Icon( BITMAPS::morgan2 ) );

TOOL_ACTION EE_ACTIONS::toLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toLabel" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change to Label" ) )
        .Tooltip( _( "Change existing item to a label" ) )
        .Icon( BITMAPS::add_line_label )
        .Flags( AF_NONE )
        .Parameter( SCH_LABEL_T ) );

TOOL_ACTION EE_ACTIONS::toCLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toCLabel" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change to Directive Label" ) )
        .Tooltip( _( "Change existing item to a directive label" ) )
        .Icon( BITMAPS::add_class_flag )
        .Flags( AF_NONE )
        .Parameter( SCH_DIRECTIVE_LABEL_T ) );

TOOL_ACTION EE_ACTIONS::toHLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toHLabel" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change to Hierarchical Label" ) )
        .Tooltip( _( "Change existing item to a hierarchical label" ) )
        .Icon( BITMAPS::add_hierarchical_label )
        .Flags( AF_NONE )
        .Parameter( SCH_HIER_LABEL_T ) );

TOOL_ACTION EE_ACTIONS::toGLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toGLabel" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change to Global Label" ) )
        .Tooltip( _( "Change existing item to a global label" ) )
        .Icon( BITMAPS::add_glabel )
        .Flags( AF_NONE )
        .Parameter( SCH_GLOBAL_LABEL_T ) );

TOOL_ACTION EE_ACTIONS::toText( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toText" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change to Text" ) )
        .Tooltip( _( "Change existing item to a text comment" ) )
        .Icon( BITMAPS::text )
        .Flags( AF_NONE )
        .Parameter( SCH_TEXT_T ) );

TOOL_ACTION EE_ACTIONS::toTextBox( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toTextBox" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Change to Text Box" ) )
        .Tooltip( _( "Change existing item to a text box" ) )
        .Icon( BITMAPS::add_textbox )
        .Flags( AF_NONE )
        .Parameter( SCH_TEXTBOX_T ) );

TOOL_ACTION EE_ACTIONS::cleanupSheetPins( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.cleanupSheetPins" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Cleanup Sheet Pins" ) )
        .Tooltip( _( "Delete unreferenced sheet pins" ) ) );

TOOL_ACTION EE_ACTIONS::editTextAndGraphics( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.editTextAndGraphics" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Text & Graphics Properties..." ) )
        .Tooltip( _( "Edit text and graphics properties globally across schematic" ) )
        .Icon( BITMAPS::text ) );

TOOL_ACTION EE_ACTIONS::symbolProperties( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.symbolProperties" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Symbol Properties..." ) )
        .Icon( BITMAPS::part_properties ) );

TOOL_ACTION EE_ACTIONS::pinTable( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.pinTable" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Pin Table..." ) )
        .Tooltip( _( "Displays pin table for bulk editing of pins" ) )
        .Icon( BITMAPS::pin_table ) );

TOOL_ACTION EE_ACTIONS::breakWire( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.breakWire" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Break" ) )
        .Tooltip( _( "Divide into connected segments" ) )
        .Icon( BITMAPS::break_line ) );

TOOL_ACTION EE_ACTIONS::slice( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.slice" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Slice" ) )
        .Tooltip( _( "Divide into unconnected segments" ) )
        .Icon( BITMAPS::slice_line ) );

// SCH_EDITOR_CONTROL
//
TOOL_ACTION EE_ACTIONS::restartMove( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.restartMove" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION EE_ACTIONS::highlightNet( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.highlightNet" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '`' )
        .FriendlyName( _( "Highlight Net" ) )
        .Tooltip( _( "Highlight net under cursor" ) )
        .Icon( BITMAPS::net_highlight_schematic ) );

TOOL_ACTION EE_ACTIONS::clearHighlight( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.clearHighlight" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '~' )
        .FriendlyName( _( "Clear Net Highlighting" ) )
        .Tooltip( _( "Clear any existing net highlighting" ) ) );

TOOL_ACTION EE_ACTIONS::updateNetHighlighting( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.updateNetHighlighting" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION EE_ACTIONS::highlightNetTool( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.highlightNetTool" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Highlight Nets" ) )
        .Tooltip( _( "Highlight wires and pins of a net" ) )
        .Icon( BITMAPS::net_highlight_schematic )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::showNetNavigator( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showNetNavigator" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Net Navigator" ) ) );

TOOL_ACTION EE_ACTIONS::editWithLibEdit( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.editWithSymbolEditor" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'E' )
        .LegacyHotkeyName( "Edit with Symbol Editor" )
        .FriendlyName( _( "Edit with Symbol Editor" ) )
        .Tooltip( _( "Open the selected symbol in the Symbol Editor" ) )
        .Icon( BITMAPS::libedit ) );

TOOL_ACTION EE_ACTIONS::setExcludeFromBOM( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.setExcludeFromBOM" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Exclude from Bill of Materials" ) )
        .Tooltip( _( "Set the exclude from bill of materials attribute" ) ) );

TOOL_ACTION EE_ACTIONS::unsetExcludeFromBOM( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.unsetExcludeFromBOM" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Include in Bill of Materials" ) )
        .Tooltip( _( "Clear the exclude from bill of materials attribute" ) ) );

TOOL_ACTION EE_ACTIONS::toggleExcludeFromBOM( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.toggleExcludeFromBOM" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Toggle Exclude from Bill of Materials" ) )
        .Tooltip( _( "Toggle the exclude from bill of materials attribute" ) ) );

TOOL_ACTION EE_ACTIONS::setExcludeFromSimulation( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.setExcludeFromSimulation" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Exclude from Simulation" ) )
        .Tooltip( _( "Set the exclude from simulation attribute" ) ) );

TOOL_ACTION EE_ACTIONS::unsetExcludeFromSimulation( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.unsetExcludeFromSimulation" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Include in Simulation" ) )
        .Tooltip( _( "Clear the exclude from simulation attribute" ) ) );

TOOL_ACTION EE_ACTIONS::toggleExcludeFromSimulation( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.toggleExcludeFromSimulation" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Toggle Exclude from Simulation" ) )
        .Tooltip( _( "Toggle the exclude from simulation attribute" ) ) );

TOOL_ACTION EE_ACTIONS::setExcludeFromBoard( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.setExcludeFromBoard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Exclude from Board" ) )
        .Tooltip( _( "Set the exclude from board attribute" ) ) );

TOOL_ACTION EE_ACTIONS::unsetExcludeFromBoard( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.unsetExcludeFromBoard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Include on Board" ) )
        .Tooltip( _( "Clear the exclude from board attribute" ) ) );

TOOL_ACTION EE_ACTIONS::toggleExcludeFromBoard( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.toggleExcludeFromBoard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Toggle Exclude from Board" ) )
        .Tooltip( _( "Toggle the exclude from board attribute" ) ) );

TOOL_ACTION EE_ACTIONS::setDNP( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.setDNP" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Set Do Not Populate" ) )
        .Tooltip( _( "Set the do not populate attribute" ) ) );

TOOL_ACTION EE_ACTIONS::unsetDNP( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.unsetDNP" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Unset Do Not Populate" ) )
        .Tooltip( _( "Clear the do not populate attribute" ) ) );

TOOL_ACTION EE_ACTIONS::toggleDNP( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.toggleDNP" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_ALT + 'X' )
        .FriendlyName( _( "Toggle Do Not Populate" ) )
        .Tooltip( _( "Toggle the do not populate attribute" ) ) );

TOOL_ACTION EE_ACTIONS::editLibSymbolWithLibEdit( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.editLibSymbolWithSymbolEditor" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'E' )
        .FriendlyName( _( "Edit Library Symbol..." ) )
        .Tooltip( _( "Open the library symbol in the Symbol Editor" ) )
        .Icon( BITMAPS::libedit ) );

TOOL_ACTION EE_ACTIONS::editSymbolFields( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.editSymbolFields" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Symbol Fields..." ) )
        .Tooltip( _( "Bulk-edit fields of all symbols in schematic" ) )
        .Icon( BITMAPS::spreadsheet ) );

TOOL_ACTION EE_ACTIONS::editSymbolLibraryLinks( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.editSymbolLibraryLinks" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Symbol Library Links..." ) )
        .Tooltip( _( "Edit links between schematic and library symbols" ) )
        .Icon( BITMAPS::edit_cmp_symb_links ) );

TOOL_ACTION EE_ACTIONS::assignFootprints( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.assignFootprints" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Assign Footprints..." ) )
        .Tooltip( _( "Run footprint assignment tool" ) )
        .Icon( BITMAPS::icon_cvpcb_24 ) );

TOOL_ACTION EE_ACTIONS::importFPAssignments( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.importFPAssignments" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Import Footprint Assignments..." ) )
        .Tooltip( _( "Import symbol footprint assignments from .cmp file created by board editor" ) )
        .Icon( BITMAPS::import_footprint_names ) );

TOOL_ACTION EE_ACTIONS::annotate( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.annotate" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Annotate Schematic..." ) )
        .Tooltip( _( "Fill in schematic symbol reference designators" ) )
        .Icon( BITMAPS::annotate ) );

TOOL_ACTION EE_ACTIONS::schematicSetup( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.schematicSetup" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Schematic Setup..." ) )
        .Tooltip( _( "Edit schematic setup including annotation styles and electrical rules" ) )
        .Icon( BITMAPS::options_schematic ) );

TOOL_ACTION EE_ACTIONS::editPageNumber( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.editPageNumber" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Sheet Page Number..." ) )
        .Tooltip( _( "Edit the page number of the current or selected sheet" ) ) );

TOOL_ACTION EE_ACTIONS::rescueSymbols( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.rescueSymbols" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rescue Symbols..." ) )
        .Tooltip( _( "Find old symbols in project and rename/rescue them" ) )
        .Icon( BITMAPS::rescue ) );

TOOL_ACTION EE_ACTIONS::remapSymbols( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.remapSymbols" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Remap Legacy Library Symbols..." ) )
        .Tooltip( _( "Remap library symbol references in legacy schematics to the symbol library table" ) )
        .Icon( BITMAPS::rescue ) );

TOOL_ACTION EE_ACTIONS::drawSheetOnClipboard( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.drawSheetOnClipboard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Drawing to Clipboard" ) )
        .Tooltip( _( "Export drawing of current sheet to clipboard" ) )
        .Icon( BITMAPS::copy ) );

TOOL_ACTION EE_ACTIONS::importGraphics( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.importGraphics" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'F' )
        .LegacyHotkeyName( "Place DXF" )
        .FriendlyName( _( "Import Graphics..." ) )
        .Tooltip( _( "Import 2D drawing file" ) )
        .Icon( BITMAPS::import_vector )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::showPcbNew( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showPcbNew" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Switch to PCB Editor" ) )
        .Tooltip( _( "Open PCB in board editor" ) )
        .Icon( BITMAPS::icon_pcbnew_24 ) );

TOOL_ACTION EE_ACTIONS::exportNetlist( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.exportNetlist" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Netlist..." ) )
        .Tooltip( _( "Export file containing netlist in one of several formats" ) )
        .Icon( BITMAPS::netlist ) );

TOOL_ACTION EE_ACTIONS::generateBOM( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.generateBOM" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Generate Bill of Materials..." ) )
        .Tooltip( _( "Generate a bill of materials for the current schematic" ) )
        .Icon( BITMAPS::post_bom ) );

TOOL_ACTION EE_ACTIONS::generateBOMLegacy( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.generateBOMLegacy" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Generate Legacy Bill of Materials..." ) )
        .Tooltip( _( "Generate a bill of materials for the current schematic (Legacy Generator)" ) )
        );

TOOL_ACTION EE_ACTIONS::generateBOMExternal( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.generateBOMExternal" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Generate Bill of Materials (External)..." ) )
        .Tooltip( _( "Generate a bill of materials for the current schematic using external generator" ) )
        );

TOOL_ACTION EE_ACTIONS::exportSymbolsToLibrary( TOOL_ACTION_ARGS()
       .Name( "eeschema.EditorControl.exportSymbolsToLibrary" )
       .Scope( AS_GLOBAL )
       .FriendlyName( _( "Export Symbols to Library..." ) )
       .Tooltip( _( "Add symbols used in schematic to an existing symbol library\n"
                    "(does not remove other symbols from this library)" ) )
       .Icon( BITMAPS::library_archive ) );

TOOL_ACTION EE_ACTIONS::exportSymbolsToNewLibrary( TOOL_ACTION_ARGS()
       .Name( "eeschema.EditorControl.exportSymbolsToNewLibrary" )
       .Scope( AS_GLOBAL )
       .FriendlyName( _( "Export Symbols to New Library..." ) )
       .Tooltip( _( "Create a new symbol library using the symbols used in the schematic\n"
                    "(if the library already exists it will be replaced)" ) )
       .Icon( BITMAPS::library_archive_as ) );

TOOL_ACTION EE_ACTIONS::selectOnPCB( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.selectOnPCB" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Select on PCB" ) )
        .Tooltip( _( "Select corresponding items in PCB editor" ) )
        .Icon( BITMAPS::select_same_sheet ) );

TOOL_ACTION EE_ACTIONS::toggleHiddenPins( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showHiddenPins" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Hidden Pins" ) )
        .Icon( BITMAPS::hidden_pin ) );

TOOL_ACTION EE_ACTIONS::toggleHiddenFields( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showHiddenFields" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Hidden Fields" ) ) );

TOOL_ACTION EE_ACTIONS::toggleDirectiveLabels( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showDirectiveLabels" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Directive Labels" ) ) );

TOOL_ACTION EE_ACTIONS::toggleERCWarnings( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showERCWarnings" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show ERC Warnings" ) )
        .Tooltip( _( "Show markers for electrical rules checker warnings" ) ) );

TOOL_ACTION EE_ACTIONS::toggleERCErrors( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showERCErrors" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show ERC Errors" ) )
        .Tooltip( _( "Show markers for electrical rules checker errors" ) ) );

TOOL_ACTION EE_ACTIONS::toggleERCExclusions( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showERCExclusions" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show ERC Exclusions" ) )
        .Tooltip( _( "Show markers for excluded electrical rules checker violations" ) ) );

TOOL_ACTION EE_ACTIONS::markSimExclusions( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.markSimExclusions" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Mark items excluded from simulation" ) )
        .Tooltip( _( "Draw 'X's over items which have been excluded from simulation" ) ) );

TOOL_ACTION EE_ACTIONS::toggleOPVoltages( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showOperatingPointVoltages" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show OP Voltages" ) )
        .Tooltip( _( "Show operating point voltage data from simulation" ) ) );

TOOL_ACTION EE_ACTIONS::toggleOPCurrents( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showOperatingPointCurrents" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show OP Currents" ) )
        .Tooltip( _( "Show operating point current data from simulation" ) ) );

TOOL_ACTION EE_ACTIONS::lineModeFree( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.lineModeFree" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Line Mode for Wires and Buses" ) )
        .Tooltip( _( "Draw and drag at any angle" ) )
        .Icon( BITMAPS::lines_any )
        .Flags( AF_NONE )
        .Parameter( LINE_MODE::LINE_MODE_FREE ) );

TOOL_ACTION EE_ACTIONS::lineMode90( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.lineModeOrthonal" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Line Mode for Wires and Buses" ) )
        .Tooltip( _( "Constrain drawing and dragging to horizontal or vertical motions" ) )
        .Icon( BITMAPS::lines90 )
        .Flags( AF_NONE )
        .Parameter( LINE_MODE::LINE_MODE_90) );

TOOL_ACTION EE_ACTIONS::lineMode45( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.lineMode45" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Line Mode for Wires and Buses" ) )
        .Tooltip( _( "Constrain drawing and dragging to horizontal, vertical, or 45-degree angle motions" ) )
        .Icon( BITMAPS::hv45mode )
        .Flags( AF_NONE )
        .Parameter( LINE_MODE::LINE_MODE_45 ) );

TOOL_ACTION EE_ACTIONS::lineModeNext( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.lineModeNext" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + static_cast<int>( WXK_SPACE ) )
        .FriendlyName( _( "Line Mode for Wires and Buses" ) )
        .Tooltip( _( "Switch to next line mode" ) ) );

TOOL_ACTION EE_ACTIONS::toggleAnnotateAuto( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.annotateAutomatically" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Annotate Automatically" ) )
        .Tooltip( _( "Toggle automatic annotation of new symbols" ) )
        .Icon( BITMAPS::annotate ) );

TOOL_ACTION EE_ACTIONS::repairSchematic( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.repairSchematic" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Repair Schematic" ) )
        .Tooltip( _( "Run various diagnostics and attempt to repair schematic" ) )
        .Icon( BITMAPS::rescue ) );

// Python Console
TOOL_ACTION EE_ACTIONS::showPythonConsole( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.showPythonConsole" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Scripting Console" ) )
        .Tooltip( _( "Show the Python scripting console" ) )
        .Icon( BITMAPS::py_script ) );

// SCH_NAVIGATE_TOOL
//
TOOL_ACTION EE_ACTIONS::changeSheet( TOOL_ACTION_ARGS()
        .Name( "eeschema.NavigateTool.changeSheet" )
        .Scope( AS_CONTEXT )
        .FriendlyName( _( "Change Sheet" ) )
        .Tooltip( _( "Change to provided sheet's contents in the schematic editor" ) )
        .Icon( BITMAPS::enter_sheet ) );

TOOL_ACTION EE_ACTIONS::enterSheet( TOOL_ACTION_ARGS()
        .Name( "eeschema.NavigateTool.enterSheet" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Enter Sheet" ) )
        .Tooltip( _( "Display the selected sheet's contents in the schematic editor" ) )
        .Icon( BITMAPS::enter_sheet ) );

TOOL_ACTION EE_ACTIONS::leaveSheet( TOOL_ACTION_ARGS()
        .Name( "eeschema.NavigateTool.leaveSheet" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + static_cast<int>( WXK_BACK ) )
        .LegacyHotkeyName( "Leave Sheet" )
        .FriendlyName( _( "Leave Sheet" ) )
        .Tooltip( _( "Display the parent sheet in the schematic editor" ) )
        .Icon( BITMAPS::leave_sheet ) );

TOOL_ACTION EE_ACTIONS::navigateUp( TOOL_ACTION_ARGS()
        .Name( "eeschema.NavigateTool.up" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + static_cast<int>( WXK_UP ) )
        .FriendlyName( _( "Navigate Up" ) )
        .Tooltip( _( "Navigate up one sheet in the hierarchy" ) )
        .Icon( BITMAPS::up ) );

TOOL_ACTION EE_ACTIONS::navigateBack( TOOL_ACTION_ARGS()
        .Name( "eeschema.NavigateTool.back" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + static_cast<int>( WXK_LEFT ) )
        .FriendlyName( _( "Navigate Back" ) )
        .Tooltip( _( "Move backward in sheet navigation history" ) )
        .Icon( BITMAPS::left ) );

TOOL_ACTION EE_ACTIONS::navigateForward( TOOL_ACTION_ARGS()
        .Name( "eeschema.NavigateTool.forward" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + static_cast<int>( WXK_RIGHT ) )
        .FriendlyName( _( "Navigate Forward" ) )
        .Tooltip( _( "Move forward in sheet navigation history" ) )
        .Icon( BITMAPS::right ) );

TOOL_ACTION EE_ACTIONS::navigatePrevious( TOOL_ACTION_ARGS()
        .Name( "eeschema.NavigateTool.previous" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_PAGEUP )
        .FriendlyName( _( "Previous Sheet" ) )
        .Tooltip( _( "Move to previous sheet by number" ) )
        .Icon( BITMAPS::left ) );

TOOL_ACTION EE_ACTIONS::navigateNext( TOOL_ACTION_ARGS()
        .Name( "eeschema.NavigateTool.next" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_PAGEDOWN )
        .FriendlyName( _( "Next Sheet" ) )
        .Tooltip( _( "Move to next sheet by number" ) )
        .Icon( BITMAPS::right ) );

TOOL_ACTION EE_ACTIONS::showHierarchy( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorTool.showHierarchy" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'H' )
        .FriendlyName( _( "Hierarchy Navigator" ) )
        .Tooltip( _( "Show or hide the schematic sheet hierarchy navigator" ) )
        .Icon( BITMAPS::hierarchy_nav ) );


// SCH_LINE_WIRE_BUS_TOOL
//
const DRAW_SEGMENT_EVENT_PARAMS drawWireActionParam = { LAYER_WIRE, false, nullptr };
TOOL_ACTION EE_ACTIONS::drawWire( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawingLineWireBus.drawWires" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'W' )
        .LegacyHotkeyName( "Begin Wire" )
        .FriendlyName( _( "Draw Wires" ) )
        .Icon( BITMAPS::add_line )
        .Flags( AF_ACTIVATE )
        .Parameter( &drawWireActionParam ) );

const DRAW_SEGMENT_EVENT_PARAMS drawBusActionParam = { LAYER_BUS, false, nullptr };
TOOL_ACTION EE_ACTIONS::drawBus( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawingLineWireBus.drawBuses" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'B' )
        .LegacyHotkeyName( "Begin Bus" )
        .FriendlyName( _( "Draw Buses" ) )
        .Icon( BITMAPS::add_bus )
        .Flags( AF_ACTIVATE )
        .Parameter( &drawBusActionParam ) );

TOOL_ACTION EE_ACTIONS::unfoldBus( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawingLineWireBus.unfoldBus" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'C' )
        .LegacyHotkeyName( "Unfold from Bus" )
        .FriendlyName( _( "Unfold from Bus" ) )
        .Tooltip( _( "Break a wire out of a bus" ) )
        .Icon( BITMAPS::INVALID_BITMAP )
        .Flags( AF_ACTIVATE )
        .Parameter<wxString*>( nullptr ) );

const DRAW_SEGMENT_EVENT_PARAMS drawLinesActionParam = { LAYER_NOTES, false, nullptr };
TOOL_ACTION EE_ACTIONS::drawLines( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawingLineWireBus.drawLines" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'I' )
        .LegacyHotkeyName( "Add Graphic PolyLine" )
        .FriendlyName( _( "Draw Lines" ) )
        .Icon( BITMAPS::add_graphical_segments )
        .Flags( AF_ACTIVATE )
        .Parameter( &drawLinesActionParam ) );

TOOL_ACTION EE_ACTIONS::undoLastSegment( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawingLineWireBus.undoLastSegment")
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_BACK )
        .FriendlyName( _( "Undo Last Segment" ) )
        .Tooltip( _( "Walks the current line back one segment." ) )
        .Icon( BITMAPS::undo ) );

TOOL_ACTION EE_ACTIONS::switchSegmentPosture( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawingLineWireBus.switchPosture" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( '/' )
        .FriendlyName( _( "Switch Segment Posture" ) )
        .Tooltip( _( "Switches posture of the current segment." ) )
        .Icon( BITMAPS::change_entry_orient )
        .Flags( AF_NONE ) );

// SCH_MOVE_TOOL
//
TOOL_ACTION EE_ACTIONS::move( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveMove.move" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'M' )
        .LegacyHotkeyName( "Move Item" )
        .FriendlyName( _( "Move" ) )
        .Icon( BITMAPS::move )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::drag( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveMove.drag" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'G' )
        .LegacyHotkeyName( "Drag Item" )
        .FriendlyName( _( "Drag" ) )
        .Tooltip( _( "Move items while keeping their connections" ) )
        .Icon( BITMAPS::drag )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION EE_ACTIONS::alignToGrid( TOOL_ACTION_ARGS()
        .Name( "eeschema.AlignToGrid" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Align Items to Grid" ) )
        .Icon( BITMAPS::align_elements_to_grid )
        .Flags( AF_ACTIVATE ) );

// Schematic editor save copy curr sheet command
TOOL_ACTION EE_ACTIONS::saveCurrSheetCopyAs( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.saveCurrSheetCopyAs" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Save Current Sheet Copy As..." ) )
        .Tooltip( _( "Save a copy of the current sheet to another location or name" ) )
        .Icon( BITMAPS::save_as ) );

// Drag and drop
TOOL_ACTION EE_ACTIONS::ddAppendFile( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.ddAppendFile" )
        .Scope( AS_GLOBAL ) );

// SIMULATOR
TOOL_ACTION EE_ACTIONS::newAnalysisTab( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.newAnalysisTab" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'N' )
        .LegacyHotkeyName( "New" )
        .FriendlyName( _( "New Analysis Tab..." ) )
        .Tooltip( _( "Create a new tab containing a simulation analysis" ) )
        .Icon( BITMAPS::sim_add_plot ) );

TOOL_ACTION EE_ACTIONS::openWorkbook( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.openWorkbook" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'O' )
        .LegacyHotkeyName( "Open" )
        .FriendlyName( _( "Open Workbook..." ) )
        .Tooltip( _( "Open a saved set of analysis tabs and settings" ) )
        .Icon( BITMAPS::directory_open ) );

TOOL_ACTION EE_ACTIONS::saveWorkbook( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.saveWorkbook" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'S' )
        .LegacyHotkeyName( "Save" )
        .FriendlyName( _( "Save Workbook" ) )
        .Tooltip( _( "Save the current set of analysis tabs and settings" ) )
        .Icon( BITMAPS::save ) );

TOOL_ACTION EE_ACTIONS::saveWorkbookAs( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.saveWorkbookAs" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + MD_CTRL + 'S' )
        .LegacyHotkeyName( "Save As" )
        .FriendlyName( _( "Save Workbook As..." ) )
        .Tooltip( _( "Save the current set of analysis tabs and settings to another location" ) )
        .Icon( BITMAPS::sim_add_signal ) );

TOOL_ACTION EE_ACTIONS::exportPlotAsPNG( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulator.exportPNG" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Current Plot as PNG..." ) )
        .Icon( BITMAPS::export_png ) );

TOOL_ACTION EE_ACTIONS::exportPlotAsCSV( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulator.exportCSV" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Current Plot as CSV..." ) )
        .Icon( BITMAPS::export_file ) );

TOOL_ACTION EE_ACTIONS::exportPlotToClipboard( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulator.exportToClipboard" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Current Plot to Clipboard" ) )
        .Icon( BITMAPS::export_png ) );

TOOL_ACTION EE_ACTIONS::exportPlotToSchematic( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulator.exportPlotToSchematic" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Export Current Plot to Schematic" ) )
        .Icon( BITMAPS::export_png ) );

TOOL_ACTION EE_ACTIONS::toggleLegend( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulator.toggleLegend" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Legend" ) )
        .Icon( BITMAPS::text ) );

TOOL_ACTION EE_ACTIONS::toggleDottedSecondary( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulator.toggleDottedSecondary" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Dotted Current/Phase" ) )
        .Tooltip( _( "Draw secondary signal trace (current or phase) with a dotted line" ) ) );

TOOL_ACTION EE_ACTIONS::toggleDarkModePlots( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulator.toggleDarkModePlots" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Dark Mode Plots" ) )
        .Tooltip( _( "Draw plots with a black background" ) ) );

TOOL_ACTION EE_ACTIONS::simAnalysisProperties( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.simAnalysisProperties" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Analysis Tab..." ) )
        .Tooltip( _( "Edit the current analysis tab's SPICE command and plot setup" ) )
        .Icon( BITMAPS::sim_command ) );

TOOL_ACTION EE_ACTIONS::runSimulation( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.runSimulation" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'R' )
        .FriendlyName( _( "Run Simulation" ) )
        .Icon( BITMAPS::sim_run ) );

TOOL_ACTION EE_ACTIONS::stopSimulation( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.stopSimulation" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Stop Simulation" ) )
        .Icon( BITMAPS::sim_stop ) );

TOOL_ACTION EE_ACTIONS::simProbe( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.probe" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'P' )
        .FriendlyName( _( "Probe Schematic..." ) )
        .Tooltip( _( "Add a simulator probe" ) )
        .Icon( BITMAPS::sim_probe ) );

TOOL_ACTION EE_ACTIONS::simTune( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.tune" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'T' )
        .FriendlyName( _( "Add Tuned Value..." ) )
        .Tooltip( _( "Select a value to be tuned" ) )
        .Icon( BITMAPS::sim_tune ) );

TOOL_ACTION EE_ACTIONS::editUserDefinedSignals( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.editUserDefinedSignals" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "User-defined Signals..." ) )
        .Tooltip( _( "Add, edit or delete user-defined simulation signals" ) )
        .Icon( BITMAPS::sim_add_signal ) );

TOOL_ACTION EE_ACTIONS::showNetlist( TOOL_ACTION_ARGS()
        .Name( "eeschema.Simulation.showNetlist" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show SPICE Netlist" ) )
        .Icon( BITMAPS::netlist ) );
