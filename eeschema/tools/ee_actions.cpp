/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#include <tool/tool_action.h>
#include <bitmaps.h>
#include <tools/ee_actions.h>
#include <core/typeinfo.h>
#include <layers_id_colors_and_visibility.h>


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
        erc_xpm );

TOOL_ACTION EE_ACTIONS::runSimulation( "eeschema.EditorControl.runSimulation",
        AS_GLOBAL, 0, "",
        _( "Simulator..." ), _( "Simulate circuit in SPICE" ),
        simulator_xpm );

TOOL_ACTION EE_ACTIONS::showDatasheet( "eeschema.InspectionTool.showDatasheet",
        AS_GLOBAL,
        'D', LEGACY_HK_NAME( "Show Datasheet" ),
        _( "Show Datasheet" ), _( "Opens the datasheet in a browser" ),
        datasheet_xpm );

TOOL_ACTION EE_ACTIONS::showMarkerInfo( "eeschema.InspectionTool.showMarkerInfo",
        AS_GLOBAL, 0, "",
        _( "Show Marker Info" ), _( "Display the marker's info in a dialog" ),
        info_xpm );


// EE_POINT_EDITOR
//
TOOL_ACTION EE_ACTIONS::pointEditorAddCorner( "eeschema.PointEditor.addCorner",
        AS_GLOBAL, 0, "",
        _( "Create Corner" ), _( "Create a corner" ),
        add_corner_xpm );

TOOL_ACTION EE_ACTIONS::pointEditorRemoveCorner( "eeschema.PointEditor.removeCorner",
        AS_GLOBAL, 0, "",
        _( "Remove Corner" ), _( "Remove corner" ),
        delete_xpm );


// EE_SELECTION_TOOL
//
TOOL_ACTION EE_ACTIONS::selectionActivate( "eeschema.InteractiveSelection",
        AS_GLOBAL, 0, "", "", "",      // No description, not shown anywhere
        nullptr, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::selectNode( "eeschema.InteractiveSelection.SelectNode",
        AS_GLOBAL,
        MD_ALT + '3', LEGACY_HK_NAME( "Select Node" ),
        _( "Select Node" ), _( "Select a connection item under the cursor" ) );

TOOL_ACTION EE_ACTIONS::selectConnection( "eeschema.InteractiveSelection.SelectConnection",
        AS_GLOBAL,
        MD_ALT + '4', LEGACY_HK_NAME( "Select Connection" ),
        _( "Select Connection" ), _( "Select a complete connection" ) );

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


// LIB_CONTROL
//
TOOL_ACTION EE_ACTIONS::newSymbol( "eeschema.SymbolLibraryControl.newSymbol",
        AS_GLOBAL, 0, "",
        _( "New Symbol..." ), _( "Create a new symbol" ),
        new_component_xpm );

TOOL_ACTION EE_ACTIONS::editSymbol( "eeschema.SymbolLibraryControl.editSymbol",
        AS_GLOBAL, 0, "",
        _( "Edit Symbol" ), _( "Show selected symbol on editor canvas" ),
        edit_xpm );

TOOL_ACTION EE_ACTIONS::duplicateSymbol( "eeschema.SymbolLibraryControl.duplicateSymbol",
        AS_GLOBAL, 0, "",
        _( "Duplicate Symbol" ), _( "Make a copy of the selected symbol" ),
        duplicate_xpm );

TOOL_ACTION EE_ACTIONS::deleteSymbol( "eeschema.SymbolLibraryControl.deleteSymbol",
        AS_GLOBAL, 0, "",
        _( "Delete Symbol" ), _( "Remove the selected symbol from its library" ),
        delete_xpm );

TOOL_ACTION EE_ACTIONS::cutSymbol( "eeschema.SymbolLibraryControl.cutSymbol",
        AS_GLOBAL, 0, "",
        _( "Cut Symbol" ), "",
        cut_xpm );

TOOL_ACTION EE_ACTIONS::copySymbol( "eeschema.SymbolLibraryControl.copySymbol",
        AS_GLOBAL, 0, "",
        _( "Copy Symbol" ), "",
        copy_xpm );

TOOL_ACTION EE_ACTIONS::pasteSymbol( "eeschema.SymbolLibraryControl.pasteSymbol",
        AS_GLOBAL, 0, "",
        _( "Paste Symbol" ), "",
        paste_xpm );

TOOL_ACTION EE_ACTIONS::importSymbol( "eeschema.SymbolLibraryControl.importSymbol",
        AS_GLOBAL, 0, "",
        _( "Import Symbol..." ), _( "Import a symbol to the current library" ),
        import_part_xpm );

TOOL_ACTION EE_ACTIONS::exportSymbol( "eeschema.SymbolLibraryControl.exportSymbol",
        AS_GLOBAL, 0, "",
        _( "Export Symbol..." ), _( "Export a symbol to a new library file" ),
        export_part_xpm );

TOOL_ACTION EE_ACTIONS::addSymbolToSchematic( "eeschema.SymbolLibraryControl.addSymbolToSchematic",
        AS_GLOBAL, 0, "",
        _( "Add Symbol to Schematic" ), _( "Add Symbol to Schematic" ),
        export_xpm );

TOOL_ACTION EE_ACTIONS::showElectricalTypes( "eeschema.SymbolLibraryControl.showElectricalTypes",
        AS_GLOBAL, 0, "",
        _( "Show Pin Electrical Types" ), _( "Annotate pins with their electrical types" ),
        pin_show_etype_xpm );

TOOL_ACTION EE_ACTIONS::showComponentTree( "eeschema.SymbolLibraryControl.showComponentTree",
        AS_GLOBAL, 0, "",
        _( "Show Symbol Tree" ), "",
        search_tree_xpm );

TOOL_ACTION EE_ACTIONS::exportSymbolView( "eeschema.SymbolLibraryControl.exportSymbolView",
        AS_GLOBAL, 0, "",
        _( "Export View as PNG..." ), _( "Create PNG file from the current view" ),
        plot_xpm );

TOOL_ACTION EE_ACTIONS::exportSymbolAsSVG( "eeschema.SymbolLibraryControl.exportSymbolAsSVG",
        AS_GLOBAL, 0, "",
        _( "Export Symbol as SVG..." ), _( "Create SVG file from the current symbol" ),
        plot_svg_xpm );

TOOL_ACTION EE_ACTIONS::toggleSyncedPinsMode( "eeschema.SymbolLibraryControl.toggleSyncedPinsMode",
        AS_GLOBAL, 0, "",
        _( "Synchronized Pins Edit Mode" ),
        _( "Synchronized Pins Edit Mode\n"
           "When enabled propagates all changes (except pin numbers) to other units.\n"
           "Enabled by default for multiunit parts with interchangeable units." ),
        pin2pin_xpm );


// LIB_DRAWING_TOOLS
//
TOOL_ACTION EE_ACTIONS::placeSymbolPin( "eeschema.SymbolDrawing.placeSymbolPin",
        AS_GLOBAL,
        'P', LEGACY_HK_NAME( "Create Pin" ),
        _( "Add Pin" ), _( "Add a pin" ),
        pin_xpm, AF_ACTIVATE, (void*) LIB_PIN_T );

TOOL_ACTION EE_ACTIONS::placeSymbolText( "eeschema.SymbolDrawing.placeSymbolText",
        AS_GLOBAL, 0, "",
        _( "Add Text" ), _( "Add a text item" ),
        text_xpm, AF_ACTIVATE, (void*) LIB_TEXT_T );

TOOL_ACTION EE_ACTIONS::drawSymbolRectangle( "eeschema.SymbolDrawing.drawSymbolRectangle",
        AS_GLOBAL, 0, "",
        _( "Add Rectangle" ), _( "Add a rectangle" ),
        add_rectangle_xpm, AF_ACTIVATE, (void*) LIB_RECTANGLE_T );

TOOL_ACTION EE_ACTIONS::drawSymbolCircle( "eeschema.SymbolDrawing.drawSymbolCircle",
        AS_GLOBAL, 0, "",
        _( "Add Circle" ), _( "Add a circle" ),
        add_circle_xpm, AF_ACTIVATE, (void*) LIB_CIRCLE_T );

TOOL_ACTION EE_ACTIONS::drawSymbolArc( "eeschema.SymbolDrawing.drawSymbolArc",
        AS_GLOBAL, 0, "",
        _( "Add Arc" ), _( "Add an arc" ),
        add_arc_xpm, AF_ACTIVATE, (void*) LIB_ARC_T );

TOOL_ACTION EE_ACTIONS::drawSymbolLines( "eeschema.SymbolDrawing.drawSymbolLines",
        AS_GLOBAL, 0, "",
        _( "Add Lines" ), _( "Add connected graphic lines" ),
        add_graphical_segments_xpm, AF_ACTIVATE, (void*) LIB_POLYLINE_T );

TOOL_ACTION EE_ACTIONS::placeSymbolAnchor( "eeschema.SymbolDrawing.placeSymbolAnchor",
        AS_GLOBAL, 0, "",
        _( "Move Symbol Anchor" ), _( "Specify a new location for the symbol anchor" ),
        anchor_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::finishDrawing( "eeschema.SymbolDrawing.finishDrawing",
        AS_GLOBAL, 0, "",
        _( "Finish Drawing" ), _( "Finish drawing shape" ),
        checked_ok_xpm, AF_NONE );


// LIB_PIN_TOOL
//
TOOL_ACTION EE_ACTIONS::pushPinLength( "eeschema.PinEditing.pushPinLength",
        AS_GLOBAL, 0, "",
        _( "Push Pin Length" ), _( "Copy pin length to other pins in symbol" ),
        pin_size_to_xpm );

TOOL_ACTION EE_ACTIONS::pushPinNameSize( "eeschema.PinEditing.pushPinNameSize",
        AS_GLOBAL, 0, "",
        _( "Push Pin Name Size" ), _( "Copy pin name size to other pins in symbol" ),
        pin_size_to_xpm );

TOOL_ACTION EE_ACTIONS::pushPinNumSize( "eeschema.PinEditing.pushPinNumSize",
        AS_GLOBAL, 0, "",
        _( "Push Pin Number Size" ), _( "Copy pin number size to other pins in symbol" ),
        pin_size_to_xpm );


// SCH_DRAWING_TOOLS
//
TOOL_ACTION EE_ACTIONS::placeSymbol( "eeschema.InteractiveDrawing.placeSymbol",
        AS_GLOBAL,
        'A', LEGACY_HK_NAME( "Add Symbol" ),
        _( "Add Symbol" ), _( "Add a symbol" ),
        add_component_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placePower( "eeschema.InteractiveDrawing.placePowerPort",
        AS_GLOBAL,
        'P', LEGACY_HK_NAME( "Add Power" ),
        _( "Add Power" ), _( "Add a power port" ),
        add_power_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::placeNoConnect( "eeschema.InteractiveDrawing.placeNoConnect",
        AS_GLOBAL,
        'Q', LEGACY_HK_NAME( "Add No Connect Flag" ),
        _( "Add No Connect Flag" ), _( "Add a no-connection flag" ),
        noconn_xpm, AF_ACTIVATE, (void*) SCH_NO_CONNECT_T );

TOOL_ACTION EE_ACTIONS::placeJunction( "eeschema.InteractiveDrawing.placeJunction",
        AS_GLOBAL,
        'J', LEGACY_HK_NAME( "Add Junction" ),
        _( "Add Junction" ), _( "Add a junction" ),
        add_junction_xpm, AF_ACTIVATE, (void*) SCH_JUNCTION_T );

TOOL_ACTION EE_ACTIONS::placeBusWireEntry( "eeschema.InteractiveDrawing.placeBusWireEntry",
        AS_GLOBAL,
        'Z', LEGACY_HK_NAME( "Add Wire Entry" ),
        _( "Add Wire to Bus Entry" ), _( "Add a wire entry to a bus" ),
        add_line2bus_xpm, AF_ACTIVATE, (void*) SCH_BUS_WIRE_ENTRY_T );

TOOL_ACTION EE_ACTIONS::placeBusBusEntry( "eeschema.InteractiveDrawing.placeBusBusEntry",
        AS_GLOBAL,
        '/', LEGACY_HK_NAME( "Add Bus Entry" ),
        _( "Add Bus to Bus Entry" ), _( "Add a bus entry to a bus" ),
        add_bus2bus_xpm, AF_ACTIVATE, (void*) SCH_BUS_BUS_ENTRY_T );

TOOL_ACTION EE_ACTIONS::placeLabel( "eeschema.InteractiveDrawing.placeLabel",
        AS_GLOBAL,
        'L', LEGACY_HK_NAME( "Add Label" ),
        _( "Add Label" ), _( "Add a net label" ),
        add_line_label_xpm, AF_ACTIVATE, (void*) SCH_LABEL_T );

TOOL_ACTION EE_ACTIONS::placeHierLabel( "eeschema.InteractiveDrawing.placeHierarchicalLabel",
        AS_GLOBAL,
        'H', LEGACY_HK_NAME( "Add Hierarchical Label" ),
        _( "Add Hierarchical Label" ), _( "Add a hierarchical sheet label" ),
        add_hierarchical_label_xpm, AF_ACTIVATE, (void*) SCH_HIER_LABEL_T );

TOOL_ACTION EE_ACTIONS::drawSheet( "eeschema.InteractiveDrawing.drawSheet",
        AS_GLOBAL,
        'S', LEGACY_HK_NAME( "Add Sheet" ),
        _( "Add Sheet" ), _( "Add a hierarchical sheet" ),
        add_hierarchical_subsheet_xpm, AF_ACTIVATE, (void*) SCH_SHEET_T );

TOOL_ACTION EE_ACTIONS::placeSheetPin( "eeschema.InteractiveDrawing.placeSheetPin",
        AS_GLOBAL, 0, "",
        _( "Add Sheet Pin" ), _( "Add a sheet pin" ),
        add_hierar_pin_xpm, AF_ACTIVATE, (void*) SCH_SHEET_PIN_T );

TOOL_ACTION EE_ACTIONS::importSheetPin( "eeschema.InteractiveDrawing.importSheetPin",
        AS_GLOBAL, 0, "",
        _( "Import Sheet Pin" ), _( "Import a hierarchical sheet pin" ),
        import_hierarchical_label_xpm, AF_ACTIVATE, (void*) SCH_SHEET_PIN_T );

TOOL_ACTION EE_ACTIONS::placeGlobalLabel( "eeschema.InteractiveDrawing.placeGlobalLabel",
        AS_GLOBAL,
        MD_CTRL + 'H', LEGACY_HK_NAME( "Add Global Label" ),
        _( "Add Global Label" ), _( "Add a global label" ),
        add_glabel_xpm, AF_ACTIVATE, (void*) SCH_GLOBAL_LABEL_T );

TOOL_ACTION EE_ACTIONS::placeSchematicText( "eeschema.InteractiveDrawing.placeSchematicText",
        AS_GLOBAL,
        'T', LEGACY_HK_NAME( "Add Graphic Text" ),
        _( "Add Text" ), _( "Add text" ),
        text_xpm, AF_ACTIVATE, (void*) SCH_TEXT_T );

TOOL_ACTION EE_ACTIONS::placeImage( "eeschema.InteractiveDrawing.placeImage",
        AS_GLOBAL, 0, "",
        _( "Add Image" ), _( "Add bitmap image" ),
        image_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::finishSheet( "eeschema.InteractiveDrawing.finishSheet",
        AS_GLOBAL, 0, "",
        _( "Finish Sheet" ), _( "Finish drawing sheet" ),
        checked_ok_xpm, AF_NONE );


// SCH_EDIT_TOOL
//
TOOL_ACTION EE_ACTIONS::repeatDrawItem( "eeschema.InteractiveEdit.repeatDrawItem",
        AS_GLOBAL,
        WXK_INSERT, LEGACY_HK_NAME( "Repeat Last Item" ),
        _( "Repeat Last Item" ), _( "Duplicates the last drawn item" ),
        nullptr );

TOOL_ACTION EE_ACTIONS::rotateCW( "eeschema.InteractiveEdit.rotateCW",
        AS_GLOBAL, 0, "",
        _( "Rotate Clockwise" ), _( "Rotates selected item(s) clockwise" ),
        rotate_cw_xpm );

TOOL_ACTION EE_ACTIONS::rotateCCW( "eeschema.InteractiveEdit.rotateCCW",
        AS_GLOBAL,
        'R', LEGACY_HK_NAME( "Rotate Item" ),
        _( "Rotate" ), _( "Rotates selected item(s) counter-clockwise" ),
        rotate_ccw_xpm );

TOOL_ACTION EE_ACTIONS::mirrorX( "eeschema.InteractiveEdit.mirrorX",
        AS_GLOBAL,
        'X', LEGACY_HK_NAME( "Mirror X" ),
        _( "Mirror Around Horizontal Axis" ), _( "Flips selected item(s) from top to bottom" ),
        mirror_v_xpm );

TOOL_ACTION EE_ACTIONS::mirrorY( "eeschema.InteractiveEdit.mirrorY",
        AS_GLOBAL,
        'Y', LEGACY_HK_NAME( "Mirror Y" ),
        _( "Mirror Around Vertical Axis" ), _( "Flips selected item(s) from left to right" ),
        mirror_h_xpm );

TOOL_ACTION EE_ACTIONS::properties( "eeschema.InteractiveEdit.properties",
        AS_GLOBAL,
        'E', LEGACY_HK_NAME( "Edit Item" ),
        _( "Properties..." ), _( "Displays item properties dialog" ),
        edit_xpm );

TOOL_ACTION EE_ACTIONS::editReference( "eeschema.InteractiveEdit.editReference",
        AS_GLOBAL,
        'U', LEGACY_HK_NAME( "Edit Symbol Reference" ),
        _( "Edit Reference..." ), _( "Displays reference field dialog" ),
        edit_comp_ref_xpm );

TOOL_ACTION EE_ACTIONS::editValue( "eeschema.InteractiveEdit.editValue",
        AS_GLOBAL,
        'V', LEGACY_HK_NAME( "Edit Symbol Value" ),
        _( "Edit Value..." ), _( "Displays value field dialog" ),
        edit_comp_value_xpm );

TOOL_ACTION EE_ACTIONS::editFootprint( "eeschema.InteractiveEdit.editFootprint",
        AS_GLOBAL,
        'F', LEGACY_HK_NAME( "Edit Symbol Footprint" ),
        _( "Edit Footprint..." ), _( "Displays footprint field dialog" ),
        edit_comp_footprint_xpm );

TOOL_ACTION EE_ACTIONS::autoplaceFields( "eeschema.InteractiveEdit.autoplaceFields",
        AS_GLOBAL,
        'O', LEGACY_HK_NAME( "Autoplace Fields" ),
        _( "Autoplace Fields" ), _( "Runs the automatic placement algorithm on the symbol's fields" ),
        autoplace_fields_xpm );

TOOL_ACTION EE_ACTIONS::updateFieldsFromLibrary( "eeschema.InteractiveEdit.updateFieldsFromLibrary",
        AS_GLOBAL, 0, "",
        _( "Update Fields from Library..." ), _( "Sets symbol fields to original library values" ),
        update_fields_xpm );

TOOL_ACTION EE_ACTIONS::toggleDeMorgan( "eeschema.InteractiveEdit.toggleDeMorgan",
        AS_GLOBAL, 0, "",
        _( "DeMorgan Conversion" ), _( "Switch between DeMorgan representations" ),
        morgan2_xpm );

TOOL_ACTION EE_ACTIONS::showDeMorganStandard( "eeschema.InteractiveEdit.showDeMorganStandard",
        AS_GLOBAL, 0, "",
        _( "DeMorgan Standard" ), _( "Switch to standard DeMorgan representation" ),
        morgan1_xpm );

TOOL_ACTION EE_ACTIONS::showDeMorganAlternate( "eeschema.InteractiveEdit.showDeMorganAlternate",
        AS_GLOBAL, 0, "",
        _( "DeMorgan Alternate" ), _( "Switch to alternate DeMorgan representation" ),
        morgan2_xpm );

TOOL_ACTION EE_ACTIONS::toShapeSlash( "eeschema.InteractiveEdit.toShapeSlash",
        AS_GLOBAL, 0, "",
        _( "Set Bus Entry Shape /" ), _( "Change the bus entry shape to /" ),
        change_entry_orient_xpm, AF_NONE, (void*) '/' );

TOOL_ACTION EE_ACTIONS::toShapeBackslash( "eeschema.InteractiveEdit.toShapeBackslash",
        AS_GLOBAL, 0, "",
        _( "Set Bus Entry Shape \\" ), _( "Change the bus entry shape to \\" ),
        change_entry_orient_xpm, AF_NONE, (void*) '\\' );

TOOL_ACTION EE_ACTIONS::toLabel( "eeschema.InteractiveEdit.toLabel",
        AS_GLOBAL, 0, "",
        _( "Change to Label" ), _( "Change existing item to a label" ),
        add_line_label_xpm, AF_NONE, (void*) SCH_LABEL_T );

TOOL_ACTION EE_ACTIONS::toHLabel( "eeschema.InteractiveEdit.toHLabel",
        AS_GLOBAL, 0, "",
        _( "Change to Hierarchical Label" ), _( "Change existing item to a hierarchical label" ),
        add_hierarchical_label_xpm, AF_NONE, (void*) SCH_HIER_LABEL_T );

TOOL_ACTION EE_ACTIONS::toGLabel( "eeschema.InteractiveEdit.toGLabel",
        AS_GLOBAL, 0, "",
        _( "Change to Global Label" ), _( "Change existing item to a global label" ),
        add_glabel_xpm, AF_NONE, (void*) SCH_GLOBAL_LABEL_T );

TOOL_ACTION EE_ACTIONS::toText( "eeschema.InteractiveEdit.toText",
        AS_GLOBAL, 0, "",
        _( "Change to Text" ), _( "Change existing item to a text comment" ),
        text_xpm, AF_NONE, (void*) SCH_TEXT_T );

TOOL_ACTION EE_ACTIONS::cleanupSheetPins( "eeschema.InteractiveEdit.cleanupSheetPins",
        AS_GLOBAL, 0, "",
        _( "Cleanup Sheet Pins" ), _( "Delete unreferenced sheet pins" ),
        nullptr );

TOOL_ACTION EE_ACTIONS::editTextAndGraphics( "eeschema.InteractiveEdit.editTextAndGraphics",
        AS_GLOBAL, 0, "",
        _( "Edit Text & Graphics Properties..." ),
        _( "Edit text and graphics properties globally across schematic" ),
        reset_text_xpm );

TOOL_ACTION EE_ACTIONS::symbolProperties( "eeschema.InteractiveEdit.symbolProperties",
        AS_GLOBAL, 0, "",
        _( "Symbol Properties..." ), _( "Displays symbol properties dialog" ),
        part_properties_xpm );

TOOL_ACTION EE_ACTIONS::pinTable( "eeschema.InteractiveEdit.pinTable",
        AS_GLOBAL, 0, "",
        _( "Pin Table..." ), _( "Displays pin table for bulk editing of pins" ),
        pin_table_xpm );

TOOL_ACTION EE_ACTIONS::breakWire( "eeschema.InteractiveEdit.breakWire",
        AS_GLOBAL, 0, "",
        _( "Break Wire" ), _( "Divide a wire into segments which can be dragged independently" ),
        break_line_xpm );

TOOL_ACTION EE_ACTIONS::breakBus( "eeschema.InteractiveEdit.breakBus",
        AS_GLOBAL, 0, "",
        _( "Break Bus" ), _( "Divide a bus into segments which can be dragged independently" ),
        break_line_xpm );


// SCH_EDITOR_CONTROL
//
TOOL_ACTION EE_ACTIONS::restartMove( "eeschema.EditorControl.restartMove",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::simProbe( "eeschema.Simulation.probe",
        AS_GLOBAL, 0, "",
        _( "Add a simulator probe" ), "" );

TOOL_ACTION EE_ACTIONS::simTune( "eeschema.Simulation.tune",
        AS_GLOBAL, 0, "",
        _( "Select a value to be tuned" ), "" );

TOOL_ACTION EE_ACTIONS::highlightNet( "eeschema.EditorControl.highlightNet",
        AS_GLOBAL,
        '`', "",
        _( "Highlight Net" ), _( "Highlight net under cursor" ),
        net_highlight_schematic_xpm );

TOOL_ACTION EE_ACTIONS::clearHighlight( "eeschema.EditorControl.clearHighlight",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::updateNetHighlighting( "eeschema.EditorControl.updateNetHighlighting",
        AS_GLOBAL );

TOOL_ACTION EE_ACTIONS::highlightNetTool( "eeschema.EditorControl.highlightNetTool",
        AS_GLOBAL, 0, "",
        _( "Highlight Nets" ), _( "Highlight wires and pins of a net" ),
        net_highlight_schematic_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::editWithLibEdit( "eeschema.EditorControl.editWithSymbolEditor",
        AS_GLOBAL,
        MD_CTRL + 'E', LEGACY_HK_NAME( "Edit with Symbol Editor" ),
        _( "Edit with Symbol Editor" ), _( "Open the symbol editor to edit the symbol" ),
        libedit_xpm );

TOOL_ACTION EE_ACTIONS::editSymbolFields( "eeschema.EditorControl.editSymbolFields",
        AS_GLOBAL, 0, "",
        _( "Edit Symbol Fields..." ), _( "Bulk-edit fields of all symbols in schematic" ),
        spreadsheet_xpm );

TOOL_ACTION EE_ACTIONS::editSymbolLibraryLinks( "eeschema.EditorControl.editSymbolLibraryLinks",
        AS_GLOBAL, 0, "",
        _( "Edit Symbol Library Links..." ), _( "Edit links between schematic and library symbols" ),
        edit_cmp_symb_links_xpm );

TOOL_ACTION EE_ACTIONS::assignFootprints( "eeschema.EditorControl.assignFootprints",
        AS_GLOBAL, 0, "",
        _( "Assign Footprints..." ), _( "Run Cvpcb" ),
        cvpcb_xpm );

TOOL_ACTION EE_ACTIONS::importFPAssignments( "eeschema.EditorControl.importFPAssignments",
        AS_GLOBAL, 0, "",
        _( "Import Footprint Assignments..." ),
        _( "Import symbol footprint assignments from .cmp file created by Pcbnew" ),
        import_footprint_names_xpm );

TOOL_ACTION EE_ACTIONS::annotate( "eeschema.EditorControl.annotate",
        AS_GLOBAL, 0, "",
        _( "Annotate Schematic..." ), _( "Fill in schematic symbol reference designators" ),
        annotate_xpm );

TOOL_ACTION EE_ACTIONS::showBusManager( "eeschema.EditorControl.showBusManager",
        AS_GLOBAL, 0, "",
        _( "Bus Definitions..." ), _( "Manage bus definitions" ),
        bus_definition_tool_xpm );

TOOL_ACTION EE_ACTIONS::drawSheetOnClipboard( "eeschema.EditorControl.drawSheetOnClipboard",
        AS_GLOBAL, 0, "",
        _( "Export Drawing to Clipboard" ), _( "Export drawing of current sheet to clipboard" ),
        copy_xpm );

TOOL_ACTION EE_ACTIONS::showPcbNew( "eeschema.EditorControl.showPcbNew",
        AS_GLOBAL, 0, "",
        _( "Switch to PCB Editor" ), _( "Open PCB in Pcbnew" ),
        pcbnew_xpm );

TOOL_ACTION EE_ACTIONS::exportNetlist( "eeschema.EditorControl.exportNetlist",
        AS_GLOBAL, 0, "",
        _( "Export Netlist..." ), _( "Export file containing netlist in one of several formats" ),
        netlist_xpm );

TOOL_ACTION EE_ACTIONS::generateBOM( "eeschema.EditorControl.generateBOM",
        AS_GLOBAL, 0, "",
        _( "Generate BOM..." ), _( "Generate a bill of materials for the current schematic" ),
        bom_xpm );

TOOL_ACTION EE_ACTIONS::enterSheet( "eeschema.EditorControl.enterSheet",
        AS_GLOBAL, 0, "",
        _( "Enter Sheet" ), _( "Display the selected sheet's contents in the Eeschema window" ),
        enter_sheet_xpm );

TOOL_ACTION EE_ACTIONS::leaveSheet( "eeschema.EditorControl.leaveSheet",
        AS_GLOBAL,
        MD_ALT + WXK_BACK, LEGACY_HK_NAME( "Leave Sheet" ),
        _( "Leave Sheet" ), _( "Display the parent sheet in the Eeschema window" ),
        leave_sheet_xpm );

TOOL_ACTION EE_ACTIONS::navigateHierarchy( "eeschema.EditorControl.navigateHierarchy",
        AS_GLOBAL, 0, "",
        _( "Show Hierarchy Navigator" ), "",
        hierarchy_nav_xpm );

TOOL_ACTION EE_ACTIONS::explicitCrossProbe( "eeschema.EditorControl.explicitCrossProbe",
        AS_GLOBAL, 0, "",
        _( "Highlight on PCB" ), _( "Highlight corresponding items in PCBNew" ),
        select_same_sheet_xpm );

TOOL_ACTION EE_ACTIONS::toggleHiddenPins( "eeschema.EditorControl.showHiddenPins",
        AS_GLOBAL, 0, "",
        _( "Show Hidden Pins" ), "",
        hidden_pin_xpm );

TOOL_ACTION EE_ACTIONS::toggleForceHV( "eeschema.EditorControl.forceHVLines",
        AS_GLOBAL, 0, "",
        _( "Force H/V Wires and Busses" ), "",
        lines90_xpm );


// SCH_LINE_WIRE_BUS_TOOL
//
TOOL_ACTION EE_ACTIONS::addNeededJunctions(
        "eeschema.InteractiveDrawingLineWireBus.addNeededJunctions", AS_ACTIVE, 0, "",
        _( "Add Junctions to Selection where needed" ), "", nullptr, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drawWire( "eeschema.InteractiveDrawingLineWireBus.drawWires",
        AS_GLOBAL,
        'W', LEGACY_HK_NAME( "Begin Wire" ),
        _( "Add Wire" ), _( "Add a wire" ),
        add_line_xpm, AF_ACTIVATE, (void*) LAYER_WIRE );

TOOL_ACTION EE_ACTIONS::drawBus( "eeschema.InteractiveDrawingLineWireBus.drawBusses",
        AS_GLOBAL,
        'B', LEGACY_HK_NAME( "Begin Bus" ),
        _( "Add Bus" ), _( "Add a bus" ),
        add_bus_xpm, AF_ACTIVATE, (void*) LAYER_BUS );

TOOL_ACTION EE_ACTIONS::unfoldBus( "eeschema.InteractiveDrawingLineWireBus.unfoldBus",
        AS_GLOBAL,
        'C', LEGACY_HK_NAME( "Unfold from Bus" ),
        _( "Unfold from Bus" ), _( "Break a wire out of a bus" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drawLines( "eeschema.InteractiveDrawingLineWireBus.drawLines",
        AS_GLOBAL,
        'I', LEGACY_HK_NAME( "Add Graphic PolyLine" ),
        _( "Add Lines" ), _( "Add connected graphic lines" ),
        add_dashed_line_xpm, AF_ACTIVATE, (void*) LAYER_NOTES );

TOOL_ACTION EE_ACTIONS::finishLineWireOrBus( "eeschema.InteractiveDrawingLineWireBus.finish",
        AS_GLOBAL,
        'K', LEGACY_HK_NAME( "End Line Wire Bus" ),
        _( "Finish Wire or Bus" ), _( "Complete drawing at current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishWire( "eeschema.InteractiveDrawingLineWireBus.finishWire",
        AS_GLOBAL, 0,  "",
        _( "Finish Wire" ), _( "Complete wire with current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishBus( "eeschema.InteractiveDrawingLineWireBus.finishBus",
        AS_GLOBAL, 0,  "",
        _( "Finish Bus" ), _( "Complete bus with current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishLine( "eeschema.InteractiveDrawingLineWireBus.finishLine",
        AS_GLOBAL, 0,  "",
        _( "Finish Lines" ), _( "Complete connected lines with current segment" ),
        checked_ok_xpm, AF_NONE );


// SCH_MOVE_TOOL
//
TOOL_ACTION EE_ACTIONS::move( "eeschema.InteractiveMove.move",
        AS_GLOBAL,
        'M', LEGACY_HK_NAME( "Move Item" ),
        _( "Move" ), _( "Moves the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drag( "eeschema.InteractiveMove.drag",
        AS_GLOBAL,
        'G', LEGACY_HK_NAME( "Drag Item" ),
        _( "Drag" ), _( "Drags the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::moveActivate( "eeschema.InteractiveMove",
        AS_GLOBAL, 0, "",
        _( "Move Activate" ), "", move_xpm, AF_ACTIVATE );


