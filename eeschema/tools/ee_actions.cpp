/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
TOOL_ACTION EE_ACTIONS::runERC( "eeschema.InspectionTool.runERC",
        AS_GLOBAL, 0, "",
        _( "Electrical Rules Checker" ), _( "Perform electrical rules check" ),
        BITMAPS::erc );

TOOL_ACTION EE_ACTIONS::checkSymbol( "eeschema.InspectionTool.checkSymbol",
        AS_GLOBAL, 0, "",
        _( "Symbol Checker" ), _( "Show the symbol checker window" ),
        BITMAPS::erc );

TOOL_ACTION EE_ACTIONS::diffSymbol( "eeschema.InspectionTool.diffSymbol",
        AS_GLOBAL, 0, "",
        _( "Diff Symbol with Library" ),
        _( "Show differences between schematic symbol and its library equivalent" ),
        BITMAPS::library );

TOOL_ACTION EE_ACTIONS::showSimulator( "eeschema.EditorControl.showSimulator",
        AS_GLOBAL, 0, "",
        _( "Simulator" ), _( "Show simulation window for running SPICE or IBIS simulations." ),
        BITMAPS::simulator );

TOOL_ACTION EE_ACTIONS::showDatasheet( "eeschema.InspectionTool.showDatasheet",
        AS_GLOBAL,
        'D', LEGACY_HK_NAME( "Show Datasheet" ),
        _( "Show Datasheet" ), _( "Opens the datasheet in a browser" ),
        BITMAPS::datasheet );


// EE_POINT_EDITOR
//
TOOL_ACTION EE_ACTIONS::pointEditorAddCorner( "eeschema.PointEditor.addCorner",
        AS_GLOBAL, 0, "",
        _( "Create Corner" ), _( "Create a corner" ),
        BITMAPS::add_corner );

TOOL_ACTION EE_ACTIONS::pointEditorRemoveCorner( "eeschema.PointEditor.removeCorner",
        AS_GLOBAL, 0, "",
        _( "Remove Corner" ), _( "Remove corner" ),
        BITMAPS::delete_cursor );


// EE_SELECTION_TOOL
//
TOOL_ACTION EE_ACTIONS::selectionActivate( "eeschema.InteractiveSelection",
        AS_GLOBAL, 0, "", "", "",      // No description, not shown anywhere
        BITMAPS::INVALID_BITMAP, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::selectNode( "eeschema.InteractiveSelection.SelectNode",
        AS_GLOBAL,
        MD_ALT + '3', LEGACY_HK_NAME( "Select Node" ),
        _( "Select Node" ), _( "Select a connection item under the cursor" ) );

TOOL_ACTION EE_ACTIONS::selectConnection( "eeschema.InteractiveSelection.SelectConnection",
        AS_GLOBAL,
        MD_ALT + '4', LEGACY_HK_NAME( "Select Connection" ),
        _( "Select Connection" ), _( "Select a complete connection" ),
        BITMAPS::net_highlight_schematic);

TOOL_ACTION EE_ACTIONS::selectionMenu( "eeschema.InteractiveSelection.SelectionMenu",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::addItemToSel( "eeschema.InteractiveSelection.AddItemToSel",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::addItemsToSel( "eeschema.InteractiveSelection.AddItemsToSel",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::removeItemFromSel( "eeschema.InteractiveSelection.RemoveItemFromSel",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::removeItemsFromSel( "eeschema.InteractiveSelection.RemoveItemsFromSel",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::clearSelection( "eeschema.InteractiveSelection.ClearSelection",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::syncSelection( "eeschema.InteractiveSelection.SyncSelection",
        AS_GLOBAL );


// SYMBOL_EDITOR_CONTROL
//
TOOL_ACTION EE_ACTIONS::saveLibraryAs( "eeschema.SymbolLibraryControl.saveLibraryAs",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'S', LEGACY_HK_NAME( "Save As" ),
        _( "Save Library As..." ), _( "Save the current library to a new file." ) );

TOOL_ACTION EE_ACTIONS::newSymbol( "eeschema.SymbolLibraryControl.newSymbol",
        AS_GLOBAL,
        'N', "",
        _( "New Symbol..." ), _( "Create a new symbol" ),
        BITMAPS::new_component );

TOOL_ACTION EE_ACTIONS::deriveFromExistingSymbol( "eeschema.SymbolLibraryControl.deriveFromExistingSymbol",
        AS_GLOBAL, 0, "",
        _( "Derive from existing symbol" ), _( "Create a new symbol, derived from an existing symbol" ),
        BITMAPS::new_component );

TOOL_ACTION EE_ACTIONS::editSymbol( "eeschema.SymbolLibraryControl.editSymbol",
        AS_GLOBAL, 0, "",
        _( "Edit Symbol" ), _( "Show selected symbol on editor canvas" ),
        BITMAPS::edit );

TOOL_ACTION EE_ACTIONS::duplicateSymbol( "eeschema.SymbolLibraryControl.duplicateSymbol",
        AS_GLOBAL, 0, "",
        _( "Duplicate Symbol" ), _( "Make a copy of the selected symbol" ),
        BITMAPS::duplicate );

TOOL_ACTION EE_ACTIONS::renameSymbol( "eeschema.SymbolLibraryControl.renameFootprint",
        AS_GLOBAL, 0, "",
        _( "Rename Symbol..." ), _( "Rename the selected symbol" ),
        BITMAPS::edit );

TOOL_ACTION EE_ACTIONS::saveSymbolAs( "eeschema.SymbolLibraryControl.saveSymbolAs",
        AS_GLOBAL, 0, "",
        _( "Save As..." ),  _( "Save the current symbol to a different library." ),
        BITMAPS::save_as );

TOOL_ACTION EE_ACTIONS::deleteSymbol( "eeschema.SymbolLibraryControl.deleteSymbol",
        AS_GLOBAL, 0, "",
        _( "Delete Symbol" ), _( "Remove the selected symbol from its library" ),
        BITMAPS::trash );

TOOL_ACTION EE_ACTIONS::cutSymbol( "eeschema.SymbolLibraryControl.cutSymbol",
        AS_GLOBAL, 0, "",
        _( "Cut" ), "",
        BITMAPS::cut );

TOOL_ACTION EE_ACTIONS::copySymbol( "eeschema.SymbolLibraryControl.copySymbol",
        AS_GLOBAL, 0, "",
        _( "Copy" ), "",
        BITMAPS::copy );

TOOL_ACTION EE_ACTIONS::pasteSymbol( "eeschema.SymbolLibraryControl.pasteSymbol",
        AS_GLOBAL, 0, "",
        _( "Paste Symbol" ), "",
        BITMAPS::paste );

TOOL_ACTION EE_ACTIONS::importSymbol( "eeschema.SymbolLibraryControl.importSymbol",
        AS_GLOBAL, 0, "",
        _( "Import Symbol..." ), _( "Import a symbol to the current library" ),
        BITMAPS::import_part );

TOOL_ACTION EE_ACTIONS::exportSymbol( "eeschema.SymbolLibraryControl.exportSymbol",
        AS_GLOBAL, 0, "",
        _( "Export..." ), _( "Export a symbol to a new library file" ),
        BITMAPS::export_part );

TOOL_ACTION EE_ACTIONS::updateSymbolFields( "eeschema.SymbolLibraryControl.updateSymbolFields",
        AS_GLOBAL, 0, "",
        _( "Update Symbol Fields..." ), _( "Update symbol to match changes made in parent symbol" ),
        BITMAPS::refresh );

TOOL_ACTION EE_ACTIONS::setUnitDisplayName( "eeschema.SymbolLibraryControl.setUnitDisplayName",
                                            AS_GLOBAL, 0, "", _( "Set Unit Display Name..." ),
                                            _( "Set the display name for a unit" ) );

TOOL_ACTION EE_ACTIONS::addSymbolToSchematic( "eeschema.SymbolLibraryControl.addSymbolToSchematic",
        AS_GLOBAL, 0, "",
        _( "Add Symbol to Schematic" ), _( "Add Symbol to Schematic" ),
        BITMAPS::add_symbol_to_schematic );

TOOL_ACTION EE_ACTIONS::showElectricalTypes( "eeschema.SymbolLibraryControl.showElectricalTypes",
        AS_GLOBAL, 0, "",
        _( "Show Pin Electrical Types" ), _( "Annotate pins with their electrical types" ),
        BITMAPS::pin_show_etype );

TOOL_ACTION EE_ACTIONS::showPinNumbers( "eeschema.SymbolLibraryControl.showPinNumbers",
        AS_GLOBAL, 0, "",
        _( "Show Pin Numbers" ), _( "Annotate pins with their numbers" ),
        BITMAPS::pin );

TOOL_ACTION EE_ACTIONS::showSymbolTree( "eeschema.SymbolLibraryControl.showSymbolTree",
        AS_GLOBAL, 0, "",
        _( "Show Symbol Tree" ), "",
        BITMAPS::search_tree );

TOOL_ACTION EE_ACTIONS::hideSymbolTree( "eeschema.SymbolLibraryControl.hideSymbolTree",
        AS_GLOBAL, 0, "",
        _( "Hide Symbol Tree" ), "",
        BITMAPS::search_tree );

TOOL_ACTION EE_ACTIONS::exportSymbolView( "eeschema.SymbolLibraryControl.exportSymbolView",
        AS_GLOBAL, 0, "",
        _( "Export View as PNG..." ), _( "Create PNG file from the current view" ),
        BITMAPS::export_png );

TOOL_ACTION EE_ACTIONS::exportSymbolAsSVG( "eeschema.SymbolLibraryControl.exportSymbolAsSVG",
        AS_GLOBAL, 0, "",
        _( "Export Symbol as SVG..." ), _( "Create SVG file from the current symbol" ),
        BITMAPS::export_svg );

TOOL_ACTION EE_ACTIONS::toggleSyncedPinsMode( "eeschema.SymbolLibraryControl.toggleSyncedPinsMode",
        AS_GLOBAL, 0, "",
        _( "Synchronized Pins Mode" ),
        _( "Synchronized Pins Mode\n"
           "When enabled propagates all changes (except pin numbers) to other units.\n"
           "Enabled by default for multiunit parts with interchangeable units." ),
        BITMAPS::pin2pin );


// SYMBOL_EDITOR_DRAWING_TOOLS
//
TOOL_ACTION EE_ACTIONS::placeSymbolPin( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.placeSymbolPin" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'P' )
        .LegacyHotkeyName( "Create Pin" )
        .MenuText( _( "Add Pin" ) )
        .Tooltip( _( "Add a pin" ) )
        .Icon( BITMAPS::pin )
        .Flags( AF_ACTIVATE )
        .Parameter( LIB_PIN_T ) );

TOOL_ACTION EE_ACTIONS::placeSymbolText( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.placeSymbolText" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Add Text" ) )
        .Tooltip( _( "Add a text item" ) )
        .Icon( BITMAPS::text )
        .Flags( AF_ACTIVATE )
        .Parameter( LIB_TEXT_T ) );

TOOL_ACTION EE_ACTIONS::drawSymbolTextBox( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.drawSymbolTextBox" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Add Text Box" ) )
        .Tooltip( _( "Add a text box item" ) )
        .Icon( BITMAPS::add_textbox )
        .Flags( AF_ACTIVATE )
        .Parameter( LIB_TEXTBOX_T ) );

TOOL_ACTION EE_ACTIONS::drawSymbolLines( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.drawSymbolLines" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Add Lines" ) )
        .Tooltip( _( "Add connected graphic lines" ) )
        .Icon( BITMAPS::add_graphical_segments )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::POLY ) );

TOOL_ACTION EE_ACTIONS::drawSymbolPolygon( TOOL_ACTION_ARGS()
        .Name( "eeschema.SymbolDrawing.drawSymbolPolygon" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Add Polygon" ) )
        .Tooltip( _( "Draw polygons" ) )
        .Icon( BITMAPS::add_graphical_polygon )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::POLY ) );

TOOL_ACTION EE_ACTIONS::placeSymbolAnchor( "eeschema.SymbolDrawing.placeSymbolAnchor",
        AS_GLOBAL, 0, "",
        _( "Move Symbol Anchor" ), _( "Specify a new location for the symbol anchor" ),
        BITMAPS::anchor, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::finishDrawing( "eeschema.SymbolDrawing.finishDrawing",
        AS_GLOBAL, 0, "",
        _( "Finish Drawing" ), _( "Finish drawing shape" ),
        BITMAPS::checked_ok, AF_NONE );

// SYMBOL_EDITOR_PIN_TOOL
//
TOOL_ACTION EE_ACTIONS::pushPinLength( "eeschema.PinEditing.pushPinLength",
        AS_GLOBAL, 0, "",
        _( "Push Pin Length" ), _( "Copy pin length to other pins in symbol" ),
        BITMAPS::pin_size_to );

TOOL_ACTION EE_ACTIONS::pushPinNameSize( "eeschema.PinEditing.pushPinNameSize",
        AS_GLOBAL, 0, "",
        _( "Push Pin Name Size" ), _( "Copy pin name size to other pins in symbol" ),
        BITMAPS::pin_size_to );

TOOL_ACTION EE_ACTIONS::pushPinNumSize( "eeschema.PinEditing.pushPinNumSize",
        AS_GLOBAL, 0, "",
        _( "Push Pin Number Size" ), _( "Copy pin number size to other pins in symbol" ),
        BITMAPS::pin_size_to );


// SCH_DRAWING_TOOLS
//
TOOL_ACTION EE_ACTIONS::placeSymbol( "eeschema.InteractiveDrawing.placeSymbol",
        AS_GLOBAL,
        'A', LEGACY_HK_NAME( "Add Symbol" ),
        _( "Add Symbol" ), _( "Add symbols" ),
        BITMAPS::add_component, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placePower( "eeschema.InteractiveDrawing.placePowerSymbol",
        AS_GLOBAL,
        'P', LEGACY_HK_NAME( "Add Power" ),
        _( "Add Power" ), _( "Add power symbols" ),
        BITMAPS::add_power, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeNoConnect( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeNoConnect" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'Q' )
        .LegacyHotkeyName( "Add No Connect Flag" )
        .MenuText( _( "Add No Connect Flag" ) )
        .Tooltip( _( "Draw no-connection flags" ) )
        .Icon( BITMAPS::noconn )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_NO_CONNECT_T ) );

TOOL_ACTION EE_ACTIONS::placeJunction( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeJunction" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'J' )
        .LegacyHotkeyName( "Add Junction" )
        .MenuText( _( "Add Junction" ) )
        .Tooltip( _( "Draw junctions" ) )
        .Icon( BITMAPS::add_junction )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_JUNCTION_T ) );

TOOL_ACTION EE_ACTIONS::placeBusWireEntry( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.placeBusWireEntry" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'Z' )
        .LegacyHotkeyName( "Add Wire Entry" )
        .MenuText( _( "Add Wire to Bus Entry" ) )
        .Tooltip( _( "Add a wire entry to a bus" ) )
        .Icon( BITMAPS::add_line2bus )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_BUS_WIRE_ENTRY_T ) );

TOOL_ACTION EE_ACTIONS::placeLabel( "eeschema.InteractiveDrawing.placeLabel",
        AS_GLOBAL,
        'L', LEGACY_HK_NAME( "Add Label" ),
        _( "Add Label" ), _( "Draw net labels" ),
        BITMAPS::add_label, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeClassLabel( "eeschema.InteractiveDrawing.placeClassLabel",
        AS_GLOBAL, 0, "",
        _( "Add Net Class Directive" ), _( "Add net class directive labels" ),
        BITMAPS::add_class_flag, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeHierLabel( "eeschema.InteractiveDrawing.placeHierarchicalLabel",
        AS_GLOBAL,
        'H', LEGACY_HK_NAME( "Add Hierarchical Label" ),
        _( "Add Hierarchical Label" ), _( "Add hierarchical labels" ),
        BITMAPS::add_hierarchical_label, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drawSheet( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawSheet" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'S' )
        .LegacyHotkeyName( "Add Sheet" )
        .MenuText( _( "Add Sheet" ) )
        .Tooltip( _( "Draw hierarchical sheets" ) )
        .Icon( BITMAPS::add_hierarchical_subsheet )
        .Flags( AF_ACTIVATE )
        .Parameter( SCH_SHEET_T ) );

TOOL_ACTION EE_ACTIONS::importSheetPin( "eeschema.InteractiveDrawing.importSheetPin",
        AS_GLOBAL, 0, "",
        _( "Import Sheet Pin" ), _( "Import hierarchical sheet pins" ),
        BITMAPS::import_hierarchical_label, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeGlobalLabel( "eeschema.InteractiveDrawing.placeGlobalLabel",
        AS_GLOBAL,
        MD_CTRL + 'L', LEGACY_HK_NAME( "Add Global Label" ),
        _( "Add Global Label" ), _( "Add global labels" ),
        BITMAPS::add_glabel, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeSchematicText( "eeschema.InteractiveDrawing.placeSchematicText",
        AS_GLOBAL,
        'T', LEGACY_HK_NAME( "Add Graphic Text" ),
        _( "Add Text" ), _( "Draw text items" ),
        BITMAPS::text, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drawTextBox( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawTextBox" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Add Text Box" ) )
        .Tooltip( _( "Draw text box items" ) )
        .Icon( BITMAPS::add_textbox )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::RECT ) );

TOOL_ACTION EE_ACTIONS::drawRectangle( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawRectangle" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Add Rectangle" ) )
        .Tooltip( _( "Draw rectangles" ) )
        .Icon( BITMAPS::add_rectangle )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::RECT ) );

TOOL_ACTION EE_ACTIONS::drawCircle( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawCircle" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Add Circle" ) )
        .Tooltip( _( "Draw circles" ) )
        .Icon( BITMAPS::add_circle )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::CIRCLE ) );

TOOL_ACTION EE_ACTIONS::drawArc( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawing.drawArc" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Add Arc" ) )
        .Tooltip( _( "Draw arcs" ) )
        .Icon( BITMAPS::add_arc )
        .Flags( AF_ACTIVATE )
        .Parameter( SHAPE_T::ARC ) );

TOOL_ACTION EE_ACTIONS::placeImage( "eeschema.InteractiveDrawing.placeImage",
        AS_GLOBAL, 0, "",
        _( "Add Image" ), _( "Add bitmap images" ),
        BITMAPS::image, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::finishSheet( "eeschema.InteractiveDrawing.finishSheet",
        AS_GLOBAL, 0, "",
        _( "Finish Sheet" ), _( "Finish drawing sheet" ),
        BITMAPS::checked_ok, AF_NONE );


// SCH_EDIT_TOOL
//
TOOL_ACTION EE_ACTIONS::repeatDrawItem( "eeschema.InteractiveEdit.repeatDrawItem",
        AS_GLOBAL,
#ifdef __WXMAC__
        WXK_F1, LEGACY_HK_NAME( "Repeat Last Item" ),
#else
        WXK_INSERT, LEGACY_HK_NAME( "Repeat Last Item" ),
#endif
        _( "Repeat Last Item" ), _( "Duplicates the last drawn item" ) );

TOOL_ACTION EE_ACTIONS::rotateCW( "eeschema.InteractiveEdit.rotateCW",
        AS_GLOBAL, 0, "",
        _( "Rotate Clockwise" ), _( "Rotates selected item(s) clockwise" ),
        BITMAPS::rotate_cw );

TOOL_ACTION EE_ACTIONS::rotateCCW( "eeschema.InteractiveEdit.rotateCCW",
        AS_GLOBAL,
        'R', LEGACY_HK_NAME( "Rotate Item" ),
        _( "Rotate Counterclockwise" ), _( "Rotates selected item(s) counter-clockwise" ),
        BITMAPS::rotate_ccw );

TOOL_ACTION EE_ACTIONS::mirrorV( "eeschema.InteractiveEdit.mirrorV",
        AS_GLOBAL,
        'Y', LEGACY_HK_NAME( "Mirror X" ),  // Yes, these were backwards prior to 6.0....
        _( "Mirror Vertically" ), _( "Flips selected item(s) from top to bottom" ),
        BITMAPS::mirror_v );

TOOL_ACTION EE_ACTIONS::mirrorH( "eeschema.InteractiveEdit.mirrorH",
        AS_GLOBAL,
        'X', LEGACY_HK_NAME( "Mirror Y" ),  // Yes, these were backwards prior to 6.0....
        _( "Mirror Horizontally" ), _( "Flips selected item(s) from left to right" ),
        BITMAPS::mirror_h );

TOOL_ACTION EE_ACTIONS::swap( "eeschema.InteractiveEdit.swap",
        AS_GLOBAL,
        'S', "",
        _( "Swap" ), _( "Swaps selected items' positions" ),
        BITMAPS::swap );

TOOL_ACTION EE_ACTIONS::properties( "eeschema.InteractiveEdit.properties",
        AS_GLOBAL,
        'E', LEGACY_HK_NAME( "Edit Item" ),
        _( "Properties..." ), _( "Displays item properties dialog" ),
        BITMAPS::edit );

TOOL_ACTION EE_ACTIONS::editReference( "eeschema.InteractiveEdit.editReference",
        AS_GLOBAL,
        'U', LEGACY_HK_NAME( "Edit Symbol Reference" ),
        _( "Edit Reference Designator..." ), _( "Displays reference designator dialog" ),
        BITMAPS::edit_comp_ref );

TOOL_ACTION EE_ACTIONS::editValue( "eeschema.InteractiveEdit.editValue",
        AS_GLOBAL,
        'V', LEGACY_HK_NAME( "Edit Symbol Value" ),
        _( "Edit Value..." ), _( "Displays value field dialog" ),
        BITMAPS::edit_comp_value );

TOOL_ACTION EE_ACTIONS::editFootprint( "eeschema.InteractiveEdit.editFootprint",
        AS_GLOBAL,
        'F', LEGACY_HK_NAME( "Edit Symbol Footprint" ),
        _( "Edit Footprint..." ), _( "Displays footprint field dialog" ),
        BITMAPS::edit_comp_footprint );

TOOL_ACTION EE_ACTIONS::autoplaceFields( "eeschema.InteractiveEdit.autoplaceFields",
        AS_GLOBAL,
        'O', LEGACY_HK_NAME( "Autoplace Fields" ),
        _( "Autoplace Fields" ), _( "Runs the automatic placement algorithm on the symbol or sheet's fields" ),
        BITMAPS::autoplace_fields );

TOOL_ACTION EE_ACTIONS::changeSymbols( "eeschema.InteractiveEdit.changeSymbols",
        AS_GLOBAL, 0, "",
        _( "Change Symbols..." ),
        _( "Assign different symbols from the library" ),
        BITMAPS::exchange );

TOOL_ACTION EE_ACTIONS::updateSymbols( "eeschema.InteractiveEdit.updateSymbols",
        AS_GLOBAL, 0, "",
        _( "Update Symbols from Library..." ),
        _( "Update symbols to include any changes from the library" ),
        BITMAPS::refresh );

TOOL_ACTION EE_ACTIONS::changeSymbol( "eeschema.InteractiveEdit.changeSymbol",
        AS_GLOBAL, 0, "",
        _( "Change Symbol..." ),
        _( "Assign a different symbol from the library" ),
        BITMAPS::exchange );

TOOL_ACTION EE_ACTIONS::updateSymbol( "eeschema.InteractiveEdit.updateSymbol",
        AS_GLOBAL, 0, "",
        _( "Update Symbol..." ),
        _( "Update symbol to include any changes from the library" ),
        BITMAPS::refresh );

TOOL_ACTION EE_ACTIONS::assignNetclass( "eeschema.InteractiveEdit.assignNetclass",
        AS_GLOBAL, 0, "",
        _( "Assign Netclass..." ), _( "Assign a netclass to nets matching a pattern" ),
        BITMAPS::netlist);

TOOL_ACTION EE_ACTIONS::toggleDeMorgan( "eeschema.InteractiveEdit.toggleDeMorgan",
        AS_GLOBAL, 0, "",
        _( "De Morgan Conversion" ), _( "Switch between De Morgan representations" ),
        BITMAPS::morgan2 );

TOOL_ACTION EE_ACTIONS::showDeMorganStandard( "eeschema.InteractiveEdit.showDeMorganStandard",
        AS_GLOBAL, 0, "",
        _( "De Morgan Standard" ), _( "Switch to standard De Morgan representation" ),
        BITMAPS::morgan1 );

TOOL_ACTION EE_ACTIONS::showDeMorganAlternate( "eeschema.InteractiveEdit.showDeMorganAlternate",
        AS_GLOBAL, 0, "",
        _( "De Morgan Alternate" ), _( "Switch to alternate De Morgan representation" ),
        BITMAPS::morgan2 );

TOOL_ACTION EE_ACTIONS::toLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toLabel" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Change to Label" ) )
        .Tooltip( _( "Change existing item to a label" ) )
        .Icon( BITMAPS::add_line_label )
        .Flags( AF_NONE )
        .Parameter( SCH_LABEL_T ) );

TOOL_ACTION EE_ACTIONS::toCLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toCLabel" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Change to Directive Label" ) )
        .Tooltip( _( "Change existing item to a directive label" ) )
        .Icon( BITMAPS::add_class_flag )
        .Flags( AF_NONE )
        .Parameter( SCH_DIRECTIVE_LABEL_T ) );

TOOL_ACTION EE_ACTIONS::toHLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toHLabel" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Change to Hierarchical Label" ) )
        .Tooltip( _( "Change existing item to a hierarchical label" ) )
        .Icon( BITMAPS::add_hierarchical_label )
        .Flags( AF_NONE )
        .Parameter( SCH_HIER_LABEL_T ) );

TOOL_ACTION EE_ACTIONS::toGLabel( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toGLabel" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Change to Global Label" ) )
        .Tooltip( _( "Change existing item to a global label" ) )
        .Icon( BITMAPS::add_glabel )
        .Flags( AF_NONE )
        .Parameter( SCH_GLOBAL_LABEL_T ) );

TOOL_ACTION EE_ACTIONS::toText( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toText" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Change to Text" ) )
        .Tooltip( _( "Change existing item to a text comment" ) )
        .Icon( BITMAPS::text )
        .Flags( AF_NONE )
        .Parameter( SCH_TEXT_T ) );

TOOL_ACTION EE_ACTIONS::toTextBox( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveEdit.toTextBox" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Change to Text Box" ) )
        .Tooltip( _( "Change existing item to a text box" ) )
        .Icon( BITMAPS::add_textbox )
        .Flags( AF_NONE )
        .Parameter( SCH_TEXTBOX_T ) );

TOOL_ACTION EE_ACTIONS::cleanupSheetPins( "eeschema.InteractiveEdit.cleanupSheetPins",
        AS_GLOBAL, 0, "",
        _( "Cleanup Sheet Pins" ), _( "Delete unreferenced sheet pins" ) );

TOOL_ACTION EE_ACTIONS::editTextAndGraphics( "eeschema.InteractiveEdit.editTextAndGraphics",
        AS_GLOBAL, 0, "",
        _( "Edit Text & Graphics Properties..." ),
        _( "Edit text and graphics properties globally across schematic" ),
        BITMAPS::text );

TOOL_ACTION EE_ACTIONS::symbolProperties( "eeschema.InteractiveEdit.symbolProperties",
        AS_GLOBAL, 0, "",
        _( "Symbol Properties..." ), _( "Displays symbol properties dialog" ),
        BITMAPS::part_properties );

TOOL_ACTION EE_ACTIONS::pinTable( "eeschema.InteractiveEdit.pinTable",
        AS_GLOBAL, 0, "",
        _( "Pin Table..." ), _( "Displays pin table for bulk editing of pins" ),
        BITMAPS::pin_table );

TOOL_ACTION EE_ACTIONS::breakWire( "eeschema.InteractiveEdit.breakWire",
        AS_GLOBAL, 0, "",
        _( "Break" ), _( "Divide into connected segments" ),
        BITMAPS::break_line );

TOOL_ACTION EE_ACTIONS::slice( "eeschema.InteractiveEdit.slice",
        AS_GLOBAL, 0, "",
        _( "Slice" ), _( "Divide into unconnected segments" ),
        BITMAPS::slice_line );

// SCH_EDITOR_CONTROL
//
TOOL_ACTION EE_ACTIONS::restartMove( "eeschema.EditorControl.restartMove",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::highlightNet( "eeschema.EditorControl.highlightNet",
        AS_GLOBAL,
        '`', "",
        _( "Highlight Net" ), _( "Highlight net under cursor" ),
        BITMAPS::net_highlight_schematic );

TOOL_ACTION EE_ACTIONS::clearHighlight( "eeschema.EditorControl.clearHighlight",
        AS_GLOBAL, '~', "",
        _( "Clear Net Highlighting" ), _( "Clear any existing net highlighting" ) );

TOOL_ACTION EE_ACTIONS::updateNetHighlighting( "eeschema.EditorControl.updateNetHighlighting",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::highlightNetTool( "eeschema.EditorControl.highlightNetTool",
        AS_GLOBAL, 0, "",
        _( "Highlight Nets" ), _( "Highlight wires and pins of a net" ),
        BITMAPS::net_highlight_schematic, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::showNetNavigator( "eeschema.EditorControl.showNetNavigator",
        AS_GLOBAL, 0, "",
        _( "Show Net Navigator" ), _( "Toggle the net navigator panel visibility" ) );

TOOL_ACTION EE_ACTIONS::editWithLibEdit( "eeschema.EditorControl.editWithSymbolEditor",
        AS_GLOBAL,
        MD_CTRL + 'E', LEGACY_HK_NAME( "Edit with Symbol Editor" ),
        _( "Edit with Symbol Editor" ), _( "Open the selected symbol in the Symbol Editor" ),
        BITMAPS::libedit );

TOOL_ACTION EE_ACTIONS::setExcludeFromBOM( "eeschema.EditorControl.setExcludeFromBOM",
        AS_GLOBAL,
        0, "",
        _( "Exclude from bill of materials" ), _( "Set the exclude from bill of materials attribute" ) );

TOOL_ACTION EE_ACTIONS::unsetExcludeFromBOM( "eeschema.EditorControl.unsetExcludeFromBOM",
        AS_GLOBAL,
        0, "",
        _( "Include in bill of materials" ), _( "Clear the exclude from bill of materials attribute" ) );

TOOL_ACTION EE_ACTIONS::toggleExcludeFromBOM( "eeschema.EditorControl.toggleExcludeFromBOM",
        AS_GLOBAL,
        0, "",
        _( "Toggle Exclude from BOM" ), _( "Toggle the exclude from bill of materials attribute" ) );

TOOL_ACTION EE_ACTIONS::setExcludeFromSimulation( "eeschema.EditorControl.setExcludeFromSimulation",
        AS_GLOBAL,
        0, "",
        _( "Exclude from simulation" ), _( "Set the exclude from simulation attribute" ) );

TOOL_ACTION EE_ACTIONS::unsetExcludeFromSimulation( "eeschema.EditorControl.unsetExcludeFromSimulation",
        AS_GLOBAL,
        0, "",
        _( "Include in simulation" ), _( "Clear the exclude from simulation attribute" ) );

TOOL_ACTION EE_ACTIONS::toggleExcludeFromSimulation( "eeschema.EditorControl.toggleExcludeFromSimulation",
        AS_GLOBAL,
        0, "",
        _( "Toggle Exclude from simulation" ), _( "Toggle the exclude from simulation attribute" ) );

TOOL_ACTION EE_ACTIONS::setExcludeFromBoard( "eeschema.EditorControl.setExcludeFromBoard",
        AS_GLOBAL,
        0, "",
        _( "Exclude from board" ), _( "Set the exclude from board attribute" ) );

TOOL_ACTION EE_ACTIONS::unsetExcludeFromBoard( "eeschema.EditorControl.unsetExcludeFromBoard",
        AS_GLOBAL,
        0, "",
        _( "Include on board" ), _( "Clear the exclude from board attribute" ) );

TOOL_ACTION EE_ACTIONS::toggleExcludeFromBoard( "eeschema.EditorControl.toggleExcludeFromBoard",
        AS_GLOBAL,
        0, "",
        _( "Toggle Exclude from board" ), _( "Toggle the exclude from board attribute" ) );

TOOL_ACTION EE_ACTIONS::setDNP( "eeschema.EditorControl.setDNP",
        AS_GLOBAL,
        0, "",
        _( "Set do not populate" ), _( "Set the do not populate attribute" ) );

TOOL_ACTION EE_ACTIONS::unsetDNP( "eeschema.EditorControl.unsetDNP",
        AS_GLOBAL,
        0, "",
        _( "Unset do not populate" ), _( "Clear the do not populate attribute" ) );

TOOL_ACTION EE_ACTIONS::toggleDNP( "eeschema.EditorControl.toggleDNP",
        AS_GLOBAL,
        0, "",
        _( "Toggle do not populate" ), _( "Toggle the do not populate attribute" ) );

TOOL_ACTION EE_ACTIONS::editLibSymbolWithLibEdit( "eeschema.EditorControl.editLibSymbolWithSymbolEditor",
        AS_GLOBAL,
        MD_CTRL + MD_SHIFT + 'E', "",
        _( "Edit Library Symbol..." ), _( "Open the library symbol in the Symbol Editor" ),
        BITMAPS::libedit );

TOOL_ACTION EE_ACTIONS::editSymbolFields( "eeschema.EditorControl.editSymbolFields",
        AS_GLOBAL, 0, "",
        _( "Edit Symbol Fields..." ), _( "Bulk-edit fields of all symbols in schematic" ),
        BITMAPS::spreadsheet );

TOOL_ACTION EE_ACTIONS::editSymbolLibraryLinks( "eeschema.EditorControl.editSymbolLibraryLinks",
        AS_GLOBAL, 0, "",
        _( "Edit Symbol Library Links..." ), _( "Edit links between schematic and library symbols" ),
        BITMAPS::edit_cmp_symb_links );

TOOL_ACTION EE_ACTIONS::assignFootprints( "eeschema.EditorControl.assignFootprints",
        AS_GLOBAL, 0, "",
        _( "Assign Footprints..." ), _( "Run footprint assignment tool" ),
        BITMAPS::icon_cvpcb_24 );

TOOL_ACTION EE_ACTIONS::importFPAssignments( "eeschema.EditorControl.importFPAssignments",
        AS_GLOBAL, 0, "",
        _( "Import Footprint Assignments..." ),
        _( "Import symbol footprint assignments from .cmp file created by board editor" ),
        BITMAPS::import_footprint_names );

TOOL_ACTION EE_ACTIONS::annotate( "eeschema.EditorControl.annotate",
        AS_GLOBAL, 0, "",
        _( "Annotate Schematic..." ), _( "Fill in schematic symbol reference designators" ),
        BITMAPS::annotate );

TOOL_ACTION EE_ACTIONS::schematicSetup( "eeschema.EditorControl.schematicSetup",
        AS_GLOBAL, 0, "",
        _( "Schematic Setup..." ),
        _( "Edit schematic setup including annotation styles and electrical rules" ),
        BITMAPS::options_schematic );

TOOL_ACTION EE_ACTIONS::editPageNumber( "eeschema.EditorControl.editPageNumber",
        AS_GLOBAL, 0, "",
        _( "Edit Sheet Page Number..." ),
        _( "Edit the page number of the current or selected sheet" ) );

TOOL_ACTION EE_ACTIONS::rescueSymbols( "eeschema.EditorControl.rescueSymbols",
        AS_GLOBAL, 0, "",
        _( "Rescue Symbols..." ),
        _( "Find old symbols in project and rename/rescue them" ),
        BITMAPS::rescue );

TOOL_ACTION EE_ACTIONS::remapSymbols( "eeschema.EditorControl.remapSymbols",
        AS_GLOBAL, 0, "",
        _( "Remap Legacy Library Symbols..." ),
        _( "Remap library symbol references in legacy schematics to the symbol library table" ),
        BITMAPS::rescue );

TOOL_ACTION EE_ACTIONS::drawSheetOnClipboard( "eeschema.EditorControl.drawSheetOnClipboard",
        AS_GLOBAL, 0, "",
        _( "Export Drawing to Clipboard" ), _( "Export drawing of current sheet to clipboard" ),
        BITMAPS::copy );

TOOL_ACTION EE_ACTIONS::showPcbNew( "eeschema.EditorControl.showPcbNew",
        AS_GLOBAL, 0, "",
        _( "Switch to PCB Editor" ), _( "Open PCB in board editor" ),
        BITMAPS::icon_pcbnew_24 );

TOOL_ACTION EE_ACTIONS::exportNetlist( "eeschema.EditorControl.exportNetlist",
        AS_GLOBAL, 0, "",
        _( "Export Netlist..." ), _( "Export file containing netlist in one of several formats" ),
        BITMAPS::netlist );

TOOL_ACTION EE_ACTIONS::generateBOM( "eeschema.EditorControl.generateBOM",
        AS_GLOBAL, 0, "",
        _( "Generate BOM..." ), _( "Generate a bill of materials for the current schematic" ),
        BITMAPS::post_bom );

TOOL_ACTION EE_ACTIONS::exportSymbolsToLibrary( "eeschema.EditorControl.exportSymbolsToLibrary",
       AS_GLOBAL, 0, "",
       _( "Export Symbols to Library..." ),
       _( "Add symbols used in schematic to an existing symbol library\n"
          "(does not remove other symbols from this library)" ),
       BITMAPS::library_archive );

TOOL_ACTION EE_ACTIONS::exportSymbolsToNewLibrary( "eeschema.EditorControl.exportSymbolsToNewLibrary",
       AS_GLOBAL, 0, "",
       _( "Export Symbols to New Library..." ),
       _( "Create a new symbol library using the symbols used in the schematic\n"
          "(if the library already exists it will be replaced)" ),
       BITMAPS::library_archive_as );

TOOL_ACTION EE_ACTIONS::selectOnPCB( "eeschema.EditorControl.selectOnPCB",
        AS_GLOBAL, 0, "",
        _( "Select on PCB" ),
        _( "Select corresponding items in PCB editor" ),
        BITMAPS::select_same_sheet );

TOOL_ACTION EE_ACTIONS::toggleHiddenPins( "eeschema.EditorControl.showHiddenPins",
        AS_GLOBAL, 0, "",
        _( "Show Hidden Pins" ), _( "Toggle display of hidden pins" ),
        BITMAPS::hidden_pin );

TOOL_ACTION EE_ACTIONS::toggleHiddenFields( "eeschema.EditorControl.showHiddenFields",
        AS_GLOBAL, 0, "",
        _( "Show Hidden Fields" ), _( "Toggle display of hidden text fields" ) );

TOOL_ACTION EE_ACTIONS::toggleDirectiveLabels( "eeschema.EditorControl.showDirectiveLabels",
        AS_GLOBAL, 0, "",
        _( "Show Directive Labels" ), _( "Toggle display of directive labels" ) );

TOOL_ACTION EE_ACTIONS::toggleERCWarnings( "eeschema.EditorControl.showERCWarnings",
        AS_GLOBAL, 0, "",
        _( "Show ERC Warnings" ), _( "Show markers for electrical rules checker warnings" ) );

TOOL_ACTION EE_ACTIONS::toggleERCErrors( "eeschema.EditorControl.showERCErrors",
        AS_GLOBAL, 0, "",
        _( "Show ERC Errors" ), _( "Show markers for electrical rules checker errors" ) );

TOOL_ACTION EE_ACTIONS::toggleERCExclusions( "eeschema.EditorControl.showERCExclusions",
        AS_GLOBAL, 0, "",
        _( "Show ERC Exclusions" ),
        _( "Show markers for excluded electrical rules checker violations" ) );

TOOL_ACTION EE_ACTIONS::toggleOPVoltages( "eeschema.EditorControl.showOperatingPointVoltages",
        AS_GLOBAL, 0, "",
        _( "Show OP Voltages" ),
        _( "Show operating point voltage data from simulation" ) );

TOOL_ACTION EE_ACTIONS::toggleOPCurrents( "eeschema.EditorControl.showOperatingPointCurrents",
        AS_GLOBAL, 0, "",
        _( "Show OP Currents" ),
        _( "Show operating point current data from simulation" ) );

TOOL_ACTION EE_ACTIONS::lineModeFree( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.lineModeFree" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Line Mode for Wires and Buses" ) )
        .Tooltip( _( "Draw and drag at any angle" ) )
        .Icon( BITMAPS::lines_any )
        .Flags( AF_NONE )
        .Parameter( LINE_MODE::LINE_MODE_FREE ) );

TOOL_ACTION EE_ACTIONS::lineMode90( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.lineModeOrthonal" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Line Mode for Wires and Buses" ) )
        .Tooltip( _( "Constrain drawing and dragging to horizontal or vertical motions" ) )
        .Icon( BITMAPS::lines90 )
        .Flags( AF_NONE )
        .Parameter( LINE_MODE::LINE_MODE_90) );

TOOL_ACTION EE_ACTIONS::lineMode45( TOOL_ACTION_ARGS()
        .Name( "eeschema.EditorControl.lineMode45" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Line Mode for Wires and Buses" ) )
        .Tooltip( _( "Constrain drawing and dragging to horizontal, vertical, or 45-degree angle motions" ) )
        .Icon( BITMAPS::hv45mode )
        .Flags( AF_NONE )
        .Parameter( LINE_MODE::LINE_MODE_45 ) );

TOOL_ACTION EE_ACTIONS::lineModeNext( "eeschema.EditorControl.lineModeNext",
        AS_GLOBAL, MD_SHIFT + WXK_SPACE, "",
        _( "Line Mode for Wires and Buses" ), _( "Switch to next line mode" ),
        BITMAPS::unknown );

TOOL_ACTION EE_ACTIONS::toggleAnnotateAuto( "eeschema.EditorControl.annotateAutomatically",
        AS_GLOBAL, 0, "",
        _( "Annotate Automatically" ), _( "Toggle automatic annotation of new symbols" ),
        BITMAPS::annotate );

TOOL_ACTION EE_ACTIONS::repairSchematic( "eeschema.EditorControl.repairSchematic",
        AS_GLOBAL, 0, "",
        _( "Repair Schematic" ),
        _( "Run various diagnostics and attempt to repair schematic" ),
        BITMAPS::rescue );

// Python Console
TOOL_ACTION EE_ACTIONS::showPythonConsole( "eeschema.EditorControl.showPythonConsole",
        AS_GLOBAL, 0, "",
        _( "Scripting Console" ), _( "Show the Python scripting console" ),
        BITMAPS::py_script );

// SCH_NAVIGATE_TOOL
//
TOOL_ACTION EE_ACTIONS::changeSheet( "eeschema.NavigateTool.changeSheet",
        AS_CONTEXT, 0, "",
        _( "Change Sheet" ), _( "Change to provided sheet's contents in the schematic editor" ),
        BITMAPS::enter_sheet );

TOOL_ACTION EE_ACTIONS::enterSheet( "eeschema.NavigateTool.enterSheet",
        AS_GLOBAL, 0, "",
        _( "Enter Sheet" ), _( "Display the selected sheet's contents in the schematic editor" ),
        BITMAPS::enter_sheet );

TOOL_ACTION EE_ACTIONS::leaveSheet( "eeschema.NavigateTool.leaveSheet",
        AS_GLOBAL,
        MD_ALT + WXK_BACK, LEGACY_HK_NAME( "Leave Sheet" ),
        _( "Leave Sheet" ), _( "Display the parent sheet in the schematic editor" ),
        BITMAPS::leave_sheet );

TOOL_ACTION EE_ACTIONS::navigateUp( "eeschema.NavigateTool.up",
        AS_GLOBAL, MD_ALT + WXK_UP, "",
        _( "Navigate Up" ), _( "Navigate up one sheet in the hierarchy" ),
        BITMAPS::up );

TOOL_ACTION EE_ACTIONS::navigateBack( "eeschema.NavigateTool.back",
        AS_GLOBAL,
        MD_ALT + WXK_LEFT, "",
        _( "Navigate Back" ), _( "Move backward in sheet navigation history" ),
        BITMAPS::left );

TOOL_ACTION EE_ACTIONS::navigateForward( "eeschema.NavigateTool.forward",
        AS_GLOBAL,
        MD_ALT + WXK_RIGHT, "",
        _( "Navigate Forward" ), _( "Move forward in sheet navigation history" ),
        BITMAPS::right );

TOOL_ACTION EE_ACTIONS::navigatePrevious( "eeschema.NavigateTool.previous",
        AS_GLOBAL,
        WXK_PAGEUP, "",
        _( "Previous Sheet" ), _( "Move to previous sheet by number" ),
        BITMAPS::left );

TOOL_ACTION EE_ACTIONS::navigateNext( "eeschema.NavigateTool.next",
        AS_GLOBAL,
        WXK_PAGEDOWN, "",
        _( "Next Sheet" ), _( "Move to next sheet by number" ),
        BITMAPS::right );

TOOL_ACTION EE_ACTIONS::showHierarchy( "eeschema.EditorTool.showHierarchy",
        AS_GLOBAL,
        MD_CTRL + 'H', "",
        _( "Hierarchy Navigator" ), _( "Show or hide the schematic sheet hierarchy navigator" ),
        BITMAPS::hierarchy_nav );


// SCH_LINE_WIRE_BUS_TOOL
//
const DRAW_SEGMENT_EVENT_PARAMS drawWireActionParam = { LAYER_WIRE, false, nullptr };
TOOL_ACTION EE_ACTIONS::drawWire( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawingLineWireBus.drawWires" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'W' )
        .LegacyHotkeyName( "Begin Wire" )
        .MenuText( _( "Add Wire" ) )
        .Tooltip( _( "Add a wire" ) )
        .Icon( BITMAPS::add_line )
        .Flags( AF_ACTIVATE )
        .Parameter( &drawWireActionParam ) );

const DRAW_SEGMENT_EVENT_PARAMS drawBusActionParam = { LAYER_BUS, false, nullptr };
TOOL_ACTION EE_ACTIONS::drawBus( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawingLineWireBus.drawBuses" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'B' )
        .LegacyHotkeyName( "Begin Bus" )
        .MenuText( _( "Add Bus" ) )
        .Tooltip( _( "Add a bus" ) )
        .Icon( BITMAPS::add_bus )
        .Flags( AF_ACTIVATE )
        .Parameter( &drawBusActionParam ) );

TOOL_ACTION EE_ACTIONS::unfoldBus( "eeschema.InteractiveDrawingLineWireBus.unfoldBus",
        AS_GLOBAL,
        'C', LEGACY_HK_NAME( "Unfold from Bus" ),
        _( "Unfold from Bus" ), _( "Break a wire out of a bus" ),
                                   BITMAPS::INVALID_BITMAP, AF_ACTIVATE );

const DRAW_SEGMENT_EVENT_PARAMS drawLinesActionParam = { LAYER_NOTES, false, nullptr };
TOOL_ACTION EE_ACTIONS::drawLines( TOOL_ACTION_ARGS()
        .Name( "eeschema.InteractiveDrawingLineWireBus.drawLines" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'I' )
        .LegacyHotkeyName( "Add Graphic PolyLine" )
        .MenuText( _( "Add Lines" ) )
        .Tooltip( _( "Draw graphic lines" ) )
        .Icon( BITMAPS::add_graphical_segments )
        .Flags( AF_ACTIVATE )
        .Parameter( &drawLinesActionParam ) );

TOOL_ACTION EE_ACTIONS::undoLastSegment( "eeschema.InteractiveDrawingLineWireBus.undoLastSegment",
        AS_GLOBAL,
        WXK_BACK, "",
        _( "Undo Last Segment" ),  _( "Walks the current line back one segment." ),
        BITMAPS::undo );

TOOL_ACTION EE_ACTIONS::switchSegmentPosture( "eeschema.InteractiveDrawingLineWireBus.switchPosture",
        AS_GLOBAL,
        '/', "",
        _( "Switch Segment Posture" ), _( "Switches posture of the current segment." ),
        BITMAPS::change_entry_orient, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishLineWireOrBus( "eeschema.InteractiveDrawingLineWireBus.finish",
        AS_GLOBAL,
        'K', LEGACY_HK_NAME( "End Line Wire Bus" ),
        _( "Finish Wire or Bus" ), _( "Complete drawing at current segment" ),
        BITMAPS::checked_ok, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishWire( "eeschema.InteractiveDrawingLineWireBus.finishWire",
        AS_GLOBAL, 0,  "",
        _( "Finish Wire" ), _( "Complete wire with current segment" ),
        BITMAPS::checked_ok, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishBus( "eeschema.InteractiveDrawingLineWireBus.finishBus",
        AS_GLOBAL, 0,  "",
        _( "Finish Bus" ), _( "Complete bus with current segment" ),
        BITMAPS::checked_ok, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishLine( "eeschema.InteractiveDrawingLineWireBus.finishLine",
        AS_GLOBAL, 0,  "",
        _( "Finish Lines" ), _( "Complete connected lines with current segment" ),
        BITMAPS::checked_ok, AF_NONE );


// SCH_MOVE_TOOL
//
TOOL_ACTION EE_ACTIONS::move( "eeschema.InteractiveMove.move",
        AS_GLOBAL,
        'M', LEGACY_HK_NAME( "Move Item" ),
        _( "Move" ), _( "Moves the selected item(s)" ), BITMAPS::move, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drag( "eeschema.InteractiveMove.drag",
        AS_GLOBAL,
        'G', LEGACY_HK_NAME( "Drag Item" ),
        _( "Drag" ), _( "Drags the selected item(s)" ), BITMAPS::move, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::moveActivate( "eeschema.InteractiveMove",
        AS_GLOBAL, 0, "",
        _( "Move Activate" ), "", BITMAPS::move, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::symbolMoveActivate( "eeschema.SymbolMoveTool",
        AS_GLOBAL, 0, "",
        _( "Symbol Move Activate" ), "", BITMAPS::move, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::alignToGrid( "eeschema.AlignToGrid",
        AS_GLOBAL, 0, "",
        _( "Align Elements to Grid" ), "", BITMAPS::move, AF_ACTIVATE );

// Schematic editor save copy curr sheet command
TOOL_ACTION EE_ACTIONS::saveCurrSheetCopyAs( "eeschema.EditorControl.saveCurrSheetCopyAs",
        AS_GLOBAL,
        0, "",
        _( "Save Current Sheet Copy As..." ), _( "Save a copy of the current sheet to another location or name" ),
        BITMAPS::save_as );

// Drag and drop
TOOL_ACTION EE_ACTIONS::ddAppendFile( "eeschema.EditorControl.ddAppendFile",
        AS_GLOBAL );

// SIMULATOR
TOOL_ACTION EE_ACTIONS::newPlot( "eeschema.Simulation.newPlot",
        AS_GLOBAL,
        MD_CTRL + 'N', LEGACY_HK_NAME( "New" ),
        _( "New Plot" ), "",
        BITMAPS::new_generic );

TOOL_ACTION EE_ACTIONS::openWorkbook( "eeschema.Simulation.openWorkbook",
        AS_GLOBAL,
        MD_CTRL + 'O', LEGACY_HK_NAME( "Open" ),
        _( "Open Workbook..." ), "",
        BITMAPS::directory_open );

TOOL_ACTION EE_ACTIONS::saveWorkbook( "eeschema.Simulation.saveWorkbook",
        AS_GLOBAL,
        MD_CTRL + 'S', LEGACY_HK_NAME( "Save" ),
        _( "Save Workbook" ), "",
        BITMAPS::save );

TOOL_ACTION EE_ACTIONS::saveWorkbookAs( "eeschema.Simulation.saveWorkbookAs",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'S', LEGACY_HK_NAME( "Save As" ),
        _( "Save Workbook As..." ), "",
        BITMAPS::sim_add_signal );

TOOL_ACTION EE_ACTIONS::exportPlotAsPNG( "eeschema.Simulator.exportPNG",
        AS_GLOBAL, 0, "",
        _( "Export Current Plot as PNG..." ), "",
        BITMAPS::export_png );

TOOL_ACTION EE_ACTIONS::exportPlotAsCSV( "eeschema.Simulator.exportCSV",
        AS_GLOBAL, 0, "",
        _( "Export Current Plot as CSV..." ), "",
        BITMAPS::export_file );

TOOL_ACTION EE_ACTIONS::toggleLegend( "eeschema.Simulator.toggleLegend",
        AS_GLOBAL, 0, "",
        _( "Show Legend" ), "",
        BITMAPS::text );

TOOL_ACTION EE_ACTIONS::toggleDottedSecondary( "eeschema.Simulator.toggleDottedSecondary",
        AS_GLOBAL, 0, "",
        _( "Dotted Current/Phase" ),
        _( "Draw secondary signal trace (current or phase) with a dotted line" ) );

TOOL_ACTION EE_ACTIONS::toggleDarkModePlots( "eeschema.Simulator.toggleDarkModePlots",
        AS_GLOBAL, 0, "",
        _( "Dark Mode Plots" ),
        _( "Draw plots with a black background" ) );

TOOL_ACTION EE_ACTIONS::simCommand( "eeschema.Simulation.simCommand",
        AS_GLOBAL, 0, "",
        _( "Simulation Command..." ),
        _( "Edit the simulation command for the current plot tab" ),
        BITMAPS::sim_command );

TOOL_ACTION EE_ACTIONS::runSimulation( "eeschema.Simulation.runSimulation",
        AS_GLOBAL,
        'R', "",
        _( "Run Simulation" ), "",
        BITMAPS::sim_run );

TOOL_ACTION EE_ACTIONS::stopSimulation( "eeschema.Simulation.stopSimulation",
        AS_GLOBAL, 0, "",
        _( "Stop Simulation" ), "",
        BITMAPS::sim_stop );

TOOL_ACTION EE_ACTIONS::simProbe( "eeschema.Simulation.probe",
        AS_GLOBAL,
        'P', "",
        _( "Probe Schematic..." ), _( "Add a simulator probe" ),
        BITMAPS::sim_probe );

TOOL_ACTION EE_ACTIONS::simTune( "eeschema.Simulation.tune",
        AS_GLOBAL,
        'T', "",
        _( "Add Tuned Value..." ), _( "Select a value to be tuned" ),
        BITMAPS::sim_tune );

TOOL_ACTION EE_ACTIONS::showNetlist( "eeschema.Simulation.showNetlist",
        AS_GLOBAL, 0, "",
        _( "Show SPICE Netlist" ), "",
        BITMAPS::netlist );

TOOL_ACTION EE_ACTIONS::editUserDefinedSignals( "eeschema.Simulation.editUserDefinedSignals",
        AS_GLOBAL, 0, "",
        _( "User-defined Signals" ),
        _( "Add, edit or delete user-defined simulation signals" ),
        BITMAPS::sim_add_signal );

