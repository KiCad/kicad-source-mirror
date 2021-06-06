/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <common.h>
#include <eda_units.h>
#include <frame_type.h>
#include <tool/actions.h>
#include <tool/tool_action.h>
#include <tool/tool_event.h>

// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

TOOL_ACTION ACTIONS::doNew( "common.Control.new",
        AS_GLOBAL,
        MD_CTRL + 'N', LEGACY_HK_NAME( "New" ),
        _( "New..." ), _( "Create a new document in the editor" ),
        BITMAPS::new_generic );

TOOL_ACTION ACTIONS::newLibrary( "common.Control.newLibrary",
        AS_GLOBAL,
        0, "",
        _( "New Library..." ), _( "Create a new library folder" ),
        BITMAPS::new_library );

TOOL_ACTION ACTIONS::addLibrary( "common.Control.addLibrary",
        AS_GLOBAL,
        0, "",
        _( "Add Library..." ), _( "Add an existing library folder" ),
        BITMAPS::add_library );

TOOL_ACTION ACTIONS::open( "common.Control.open",
        AS_GLOBAL,
        MD_CTRL + 'O', LEGACY_HK_NAME( "Open" ),
        _( "Open..." ), _( "Open existing document" ),
        BITMAPS::directory_open );

TOOL_ACTION ACTIONS::save( "common.Control.save",
        AS_GLOBAL,
        MD_CTRL + 'S', LEGACY_HK_NAME( "Save" ),
        _( "Save" ), _( "Save changes" ),
        BITMAPS::save );

TOOL_ACTION ACTIONS::saveAs( "common.Control.saveAs",
        AS_GLOBAL,
        MD_SHIFT + MD_CTRL + 'S', LEGACY_HK_NAME( "Save As" ),
        _( "Save As..." ), _( "Save current document to another location" ),
        BITMAPS::save_as );

TOOL_ACTION ACTIONS::saveCopyAs( "common.Control.saveCopyAs",
        AS_GLOBAL,
        0, "",
        _( "Save Copy As..." ), _( "Save a copy of the current document to another location" ),
        BITMAPS::save_as );

TOOL_ACTION ACTIONS::saveAll( "common.Control.saveAll",
        AS_GLOBAL,
        0, "",
        _( "Save All" ), _( "Save all changes" ),
        BITMAPS::save );

TOOL_ACTION ACTIONS::revert( "common.Control.revert",
        AS_GLOBAL,
        0, "",
        _( "Revert" ), _( "Throw away changes" ),
        BITMAPS::undo );

TOOL_ACTION ACTIONS::pageSettings( "common.Control.pageSettings",
        AS_GLOBAL,
        0, "",
        _( "Page Settings..." ), _( "Settings for paper size and title block info" ),
        BITMAPS::sheetset );

TOOL_ACTION ACTIONS::print( "common.Control.print",
        AS_GLOBAL,
        MD_CTRL + 'P', LEGACY_HK_NAME( "Print" ),
        _( "Print..." ), _( "Print" ),
        BITMAPS::print_button );

TOOL_ACTION ACTIONS::plot( "common.Control.plot",
        AS_GLOBAL,
        0, "",
        _( "Plot..." ), _( "Plot" ),
        BITMAPS::plot );

TOOL_ACTION ACTIONS::quit( "common.Control.quit",
        AS_GLOBAL,
        0, "",   // Not currently in use due to wxWidgets crankiness
        _( "Quit" ), _( "Close the current editor" ),
        BITMAPS::exit );

// Generic Edit Actions
TOOL_ACTION ACTIONS::cancelInteractive( "common.Interactive.cancel",
        AS_GLOBAL,
        0, "",   // ESC key is handled in the dispatcher
        _( "Cancel" ), _( "Cancel current tool" ),
        BITMAPS::cancel, AF_NONE );

TOOL_ACTION ACTIONS::showContextMenu( "common.Control.showContextMenu",
        AS_GLOBAL,
        0, "",
        _( "Show Context Menu" ), _( "Perform the right-mouse-button action" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_RIGHT_CLICK );

TOOL_ACTION ACTIONS::updateMenu( "common.Interactive.updateMenu",
        AS_GLOBAL );

TOOL_ACTION ACTIONS::undo( "common.Interactive.undo",
        AS_GLOBAL,
        MD_CTRL + 'Z', LEGACY_HK_NAME( "Undo" ),
        _( "Undo" ), _( "Undo last edit" ),
        BITMAPS::undo );

TOOL_ACTION ACTIONS::redo( "common.Interactive.redo",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_SHIFT + MD_CTRL + 'Z',
#else
        MD_CTRL + 'Y',
#endif
        LEGACY_HK_NAME( "Redo" ),
        _( "Redo" ), _( "Redo last edit" ),
        BITMAPS::redo );

TOOL_ACTION ACTIONS::cut( "common.Interactive.cut",
        AS_GLOBAL,
        MD_CTRL + 'X', LEGACY_HK_NAME( "Cut" ),
        _( "Cut" ), _( "Cut selected item(s) to clipboard" ),
        BITMAPS::cut );

TOOL_ACTION ACTIONS::copy( "common.Interactive.copy",
        AS_GLOBAL,
        MD_CTRL + 'C', LEGACY_HK_NAME( "Copy" ),
        _( "Copy" ), _( "Copy selected item(s) to clipboard" ),
        BITMAPS::copy );

TOOL_ACTION ACTIONS::paste( "common.Interactive.paste",
        AS_GLOBAL,
        MD_CTRL + 'V', LEGACY_HK_NAME( "Paste" ),
        _( "Paste" ), _( "Paste items(s) from clipboard" ),
        BITMAPS::paste );

TOOL_ACTION ACTIONS::selectAll( "common.Interactive.selectAll",
        AS_GLOBAL,
        MD_CTRL + 'A', "",
        _( "Select All" ), _( "Select all items on screen" ) );

TOOL_ACTION ACTIONS::pasteSpecial( "common.Interactive.pasteSpecial",
        AS_GLOBAL, 0, "",
        _( "Paste Special..." ), _( "Paste item(s) from clipboard with options" ) );

TOOL_ACTION ACTIONS::duplicate( "common.Interactive.duplicate",
        AS_GLOBAL,
        MD_CTRL + 'D', LEGACY_HK_NAME( "Duplicate" ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ),
        BITMAPS::duplicate );

TOOL_ACTION ACTIONS::doDelete( "common.Interactive.delete",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        WXK_BACK,
#else
        WXK_DELETE,
#endif
        LEGACY_HK_NAME( "Delete Item" ),
        _( "Delete" ), _( "Deletes selected item(s)" ),
        BITMAPS::trash );

TOOL_ACTION ACTIONS::deleteTool( "common.Interactive.deleteTool",
        AS_GLOBAL, 0, "",
        _( "Interactive Delete Tool" ), _( "Delete clicked items" ),
        BITMAPS::delete_cursor, AF_ACTIVATE );

TOOL_ACTION ACTIONS::activatePointEditor( "common.Control.activatePointEditor",
        AS_GLOBAL );

TOOL_ACTION ACTIONS::changeEditMethod( "common.Interactive.changeEditMethod", AS_GLOBAL,
        MD_CTRL + ' ', "", _( "Change Edit Method" ), _( "Change edit method constraints" ) );

TOOL_ACTION ACTIONS::find( "common.Interactive.find",
        AS_GLOBAL,
        MD_CTRL + 'F', LEGACY_HK_NAME( "Find" ),
        _( "Find" ), _( "Find text" ),
        BITMAPS::find );

TOOL_ACTION ACTIONS::findAndReplace( "common.Interactive.findAndReplace",
        AS_GLOBAL,
        MD_CTRL + MD_ALT + 'F', LEGACY_HK_NAME( "Find and Replace" ),
        _( "Find and Replace" ), _( "Find and replace text" ),
        BITMAPS::find_replace );

TOOL_ACTION ACTIONS::findNext( "common.Interactive.findNext",
        AS_GLOBAL,
        WXK_F3, LEGACY_HK_NAME( "Find Next" ),
        _( "Find Next" ), _( "Find next match" ),
        BITMAPS::find );

TOOL_ACTION ACTIONS::findNextMarker( "common.Interactive.findNextMarker",
        AS_GLOBAL,
        MD_SHIFT + WXK_F3, LEGACY_HK_NAME( "Find Next Marker" ),
        _( "Find Next Marker" ), "",
        BITMAPS::find );

TOOL_ACTION ACTIONS::replaceAndFindNext( "common.Interactive.replaceAndFindNext",
        AS_GLOBAL,
        0, "",
        _( "Replace and Find Next" ), _( "Replace current match and find next" ),
        BITMAPS::find_replace );

TOOL_ACTION ACTIONS::replaceAll( "common.Interactive.replaceAll",
        AS_GLOBAL,
        0, "",
        _( "Replace All" ), _( "Replace all matches" ),
        BITMAPS::find_replace );

TOOL_ACTION ACTIONS::updateFind( "common.Control.updateFind",
        AS_GLOBAL );


// Marker Controls
TOOL_ACTION ACTIONS::prevMarker( "common.Checker.prevMarker",
        AS_GLOBAL,
        0, "",
        _( "Previous Marker" ), _( "Go to previous marker in Checker window" ) );

TOOL_ACTION ACTIONS::nextMarker( "common.Checker.nextMarker",
        AS_GLOBAL,
        0, "",
        _( "Next Marker" ), _( "Go to next marker in Checker window" ) );

TOOL_ACTION ACTIONS::excludeMarker( "common.Checker.excludeMarker",
        AS_GLOBAL,
        0, "",
        _( "Exclude Marker" ), _( "Mark current violation in Checker window as an exclusion" ) );

// View Controls
TOOL_ACTION ACTIONS::zoomRedraw( "common.Control.zoomRedraw",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_CTRL + 'R',
#else
        WXK_F5,
#endif
        LEGACY_HK_NAME( "Zoom Redraw" ),
        _( "Refresh" ), _( "Refresh" ),
        BITMAPS::refresh );

TOOL_ACTION ACTIONS::zoomFitScreen( "common.Control.zoomFitScreen",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_CTRL + '0',
#else
        WXK_HOME,
#endif
        LEGACY_HK_NAME( "Zoom Auto" ),
        _( "Zoom to Fit" ), _( "Zoom to Fit" ),
        BITMAPS::zoom_fit_in_page );

TOOL_ACTION ACTIONS::zoomFitObjects( "common.Control.zoomFitObjects",
        AS_GLOBAL, MD_CTRL + WXK_HOME, "",
        _( "Zoom to Objects" ), _( "Zoom to Objects" ),
        BITMAPS::zoom_fit_to_objects );

TOOL_ACTION ACTIONS::zoomIn( "common.Control.zoomIn",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_CTRL + '+',
#else
        WXK_F1,
#endif
        LEGACY_HK_NAME( "Zoom In" ),
        _( "Zoom In at Cursor" ), _( "Zoom In at Cursor" ),
        BITMAPS::zoom_in );

TOOL_ACTION ACTIONS::zoomOut( "common.Control.zoomOut",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_CTRL + '-',
#else
        WXK_F2,
#endif
        LEGACY_HK_NAME( "Zoom Out" ),
        _( "Zoom Out at Cursor" ), _( "Zoom Out at Cursor" ),
        BITMAPS::zoom_out );

TOOL_ACTION ACTIONS::zoomInCenter( "common.Control.zoomInCenter",
        AS_GLOBAL,
        0, "",
        _( "Zoom In" ), _( "Zoom In" ),
        BITMAPS::zoom_in );

TOOL_ACTION ACTIONS::zoomOutCenter( "common.Control.zoomOutCenter",
        AS_GLOBAL,
        0, "",
        _( "Zoom Out" ), _( "Zoom Out" ),
        BITMAPS::zoom_out );

TOOL_ACTION ACTIONS::zoomCenter( "common.Control.zoomCenter",
        AS_GLOBAL,
        WXK_F4, LEGACY_HK_NAME( "Zoom Center" ),
        _( "Center" ), _( "Center" ),
        BITMAPS::zoom_center_on_screen );

TOOL_ACTION ACTIONS::zoomTool( "common.Control.zoomTool",
        AS_GLOBAL,
        MD_CTRL + WXK_F5, LEGACY_HK_NAME( "Zoom to Selection" ),
        _( "Zoom to Selection" ), _( "Zoom to Selection" ),
        BITMAPS::zoom_area, AF_ACTIVATE );

TOOL_ACTION ACTIONS::zoomPreset( "common.Control.zoomPreset",
        AS_GLOBAL );

TOOL_ACTION ACTIONS::centerContents( "common.Control.centerContents",
        AS_GLOBAL );

// Cursor control
TOOL_ACTION ACTIONS::cursorUp( "common.Control.cursorUp",
        AS_GLOBAL,
        WXK_UP, "",
        _( "Cursor Up" ), "",
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_UP );

TOOL_ACTION ACTIONS::cursorDown( "common.Control.cursorDown",
        AS_GLOBAL,
        WXK_DOWN, "",
        _( "Cursor Down" ), "" ,
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_DOWN );

TOOL_ACTION ACTIONS::cursorLeft( "common.Control.cursorLeft",
        AS_GLOBAL,
        WXK_LEFT, "",
        _( "Cursor Left" ), "" ,
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_LEFT );

TOOL_ACTION ACTIONS::cursorRight( "common.Control.cursorRight",
        AS_GLOBAL,
        WXK_RIGHT, "",
        _( "Cursor Right" ), "" ,
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_RIGHT );


TOOL_ACTION ACTIONS::cursorUpFast( "common.Control.cursorUpFast",
        AS_GLOBAL,
        MD_CTRL + WXK_UP, "",
        _( "Cursor Up Fast" ), "",
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) ( CURSOR_UP | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorDownFast( "common.Control.cursorDownFast",
        AS_GLOBAL,
        MD_CTRL + WXK_DOWN, "",
        _( "Cursor Down Fast" ), "" ,
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) ( CURSOR_DOWN | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorLeftFast( "common.Control.cursorLeftFast",
        AS_GLOBAL,
        MD_CTRL + WXK_LEFT, "",
        _( "Cursor Left Fast" ), "" ,
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) ( CURSOR_LEFT | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorRightFast( "common.Control.cursorRightFast",
        AS_GLOBAL,
        MD_CTRL + WXK_RIGHT, "",
        _( "Cursor Right Fast" ), "" ,
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) ( CURSOR_RIGHT | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorClick( "common.Control.cursorClick",
        AS_GLOBAL,
        WXK_RETURN, LEGACY_HK_NAME( "Mouse Left Click" ),
        _( "Click" ), _( "Performs left mouse button click" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_CLICK );

TOOL_ACTION ACTIONS::cursorDblClick( "common.Control.cursorDblClick",
        AS_GLOBAL,
        WXK_END, LEGACY_HK_NAME( "Mouse Left Double Click" ),
        _( "Double-click" ), _( "Performs left mouse button double-click" ),
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_DBL_CLICK );

TOOL_ACTION ACTIONS::refreshPreview( "common.Control.refreshPreview",
         AS_GLOBAL );

TOOL_ACTION ACTIONS::pinLibrary( "common.Control.pinLibrary", AS_GLOBAL, 0, "", _( "Pin Library" ),
                                 _( "Keep the library at the top of the list" ) );

TOOL_ACTION ACTIONS::unpinLibrary( "common.Control.unpinLibrary", AS_GLOBAL, 0, "",
                                   _( "Unpin Library" ),
                                   _( "No longer keep the library at the top of the list" ) );

TOOL_ACTION ACTIONS::panUp( "common.Control.panUp",
        AS_GLOBAL,
        MD_SHIFT + WXK_UP, "",
        _( "Pan Up" ), "",
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_UP );

TOOL_ACTION ACTIONS::panDown( "common.Control.panDown",
        AS_GLOBAL,
        MD_SHIFT + WXK_DOWN, "",
        _( "Pan Down" ), "" ,
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_DOWN );

TOOL_ACTION ACTIONS::panLeft( "common.Control.panLeft",
        AS_GLOBAL,
        MD_SHIFT + WXK_LEFT, "",
        _( "Pan Left" ), "" ,
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_LEFT );

TOOL_ACTION ACTIONS::panRight( "common.Control.panRight",
        AS_GLOBAL,
        MD_SHIFT + WXK_RIGHT, "",
        _( "Pan Right" ), "" ,
        BITMAPS::INVALID_BITMAP, AF_NONE, (void*) CURSOR_RIGHT );

// Grid control
TOOL_ACTION ACTIONS::gridFast1( "common.Control.gridFast1",
        AS_GLOBAL,
        MD_ALT + '1', LEGACY_HK_NAME( "Switch Grid To Fast Grid1" ),
        _( "Switch to Fast Grid 1" ), "" );

TOOL_ACTION ACTIONS::gridFast2( "common.Control.gridFast2",
        AS_GLOBAL,
        MD_ALT + '2', LEGACY_HK_NAME( "Switch Grid To Fast Grid2" ),
        _( "Switch to Fast Grid 2" ), "" );

TOOL_ACTION ACTIONS::gridNext( "common.Control.gridNext",
        AS_GLOBAL,
        'N', LEGACY_HK_NAME( "Switch Grid To Next" ),
        _("Switch to Next Grid" ), "" );

TOOL_ACTION ACTIONS::gridPrev( "common.Control.gridPrev",
        AS_GLOBAL, MD_SHIFT + 'N', LEGACY_HK_NAME( "Switch Grid To Previous" ),
        _( "Switch to Previous Grid" ), "" );

TOOL_ACTION ACTIONS::gridSetOrigin( "common.Control.gridSetOrigin",
        AS_GLOBAL,
        'S', LEGACY_HK_NAME( "Set Grid Origin" ),
        _( "Grid Origin" ), _( "Set the grid origin point" ),
        BITMAPS::grid_select_axis );

TOOL_ACTION ACTIONS::gridResetOrigin( "common.Control.gridResetOrigin",
        AS_GLOBAL,
        'Z', LEGACY_HK_NAME( "Reset Grid Origin" ),
        _( "Reset Grid Origin" ), "" );

TOOL_ACTION ACTIONS::gridPreset( "common.Control.gridPreset",
        AS_GLOBAL );

TOOL_ACTION ACTIONS::toggleGrid( "common.Control.toggleGrid",
        AS_GLOBAL, 0, "",
        _( "Show Grid" ), _( "Display grid dots or lines in the edit window" ),
        BITMAPS::grid );

TOOL_ACTION ACTIONS::gridProperties( "common.Control.gridProperties",
        AS_GLOBAL, 0, "",
        _( "Grid Properties..." ), _( "Set grid dimensions" ),
        BITMAPS::grid_select );

TOOL_ACTION ACTIONS::inchesUnits( "common.Control.imperialUnits",
        AS_GLOBAL, 0, "",
        _( "Inches" ), _( "Use inches" ),
        BITMAPS::unit_inch, AF_NONE, (void*) EDA_UNITS::INCHES );

TOOL_ACTION ACTIONS::milsUnits( "common.Control.mils",
        AS_GLOBAL, 0, "",
        _( "Mils" ), _( "Use mils" ),
        BITMAPS::unit_mil, AF_NONE, (void*) EDA_UNITS::MILS );

TOOL_ACTION ACTIONS::millimetersUnits( "common.Control.metricUnits",
        AS_GLOBAL, 0, "",
        _( "Millimeters" ), _( "Use millimeters" ),
        BITMAPS::unit_mm, AF_NONE, (void*) EDA_UNITS::MILLIMETRES );

TOOL_ACTION ACTIONS::updateUnits( "common.Control.updateUnits",
        AS_GLOBAL );

TOOL_ACTION ACTIONS::toggleUnits( "common.Control.toggleUnits",
        AS_GLOBAL,
        MD_CTRL + 'U', LEGACY_HK_NAME( "Switch Units" ),
        _( "Switch units" ), _( "Switch between imperial and metric units" ),
        BITMAPS::unit_mm );

TOOL_ACTION ACTIONS::togglePolarCoords( "common.Control.togglePolarCoords",
        AS_GLOBAL, 0, "",
        _( "Polar Coordinates" ), _( "Switch between polar and cartesian coordinate systems" ),
        BITMAPS::polar_coord );

TOOL_ACTION ACTIONS::resetLocalCoords( "common.Control.resetLocalCoords",
        AS_GLOBAL,
        ' ', LEGACY_HK_NAME( "Reset Local Coordinates" ),
        _( "Reset Local Coordinates" ), "" );

TOOL_ACTION ACTIONS::toggleCursor( "common.Control.toggleCursor",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        MD_CTRL + MD_SHIFT + 'X', LEGACY_HK_NAME( "Toggle Cursor Display (Modern Toolset only)" ),
        _( "Always Show Cursor" ), _( "Display crosshairs even in selection tool" ),
        BITMAPS::cursor );

TOOL_ACTION ACTIONS::toggleCursorStyle( "common.Control.toggleCursorStyle",
        AS_GLOBAL, 0, "",
        _( "Full-Window Crosshairs" ), _( "Switch display of full-window crosshairs" ),
        BITMAPS::cursor_shape );

TOOL_ACTION ACTIONS::highContrastMode( "common.Control.highContrastMode",
        AS_GLOBAL, 0, LEGACY_HK_NAME( "Toggle High Contrast Mode" ),
        _( "Single Layer View Mode" ), _( "Toggle inactive layers between normal and dimmed" ),
        BITMAPS::contrast_mode );

TOOL_ACTION ACTIONS::highContrastModeCycle( "common.Control.highContrastModeCycle",
        AS_GLOBAL, MD_CTRL + 'H', "",  _( "Single Layer View Mode (3-state)" ),
        _( "Toggle inactive layers between normal, dimmed, and hidden" ), BITMAPS::contrast_mode );

TOOL_ACTION ACTIONS::selectionTool( "common.InteractiveSelection.selectionTool",
        AS_GLOBAL, 0, "",
        _( "Select item(s)" ), _( "Select item(s)" ),
        BITMAPS::cursor, AF_ACTIVATE );

TOOL_ACTION ACTIONS::measureTool( "common.InteractiveEdit.measureTool",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        MD_CTRL + MD_SHIFT + 'M', LEGACY_HK_NAME( "Measure Distance (Modern Toolset only)" ),
        _( "Measure Tool" ), _( "Interactively measure distance between points" ),
        BITMAPS::measurement, AF_ACTIVATE );

TOOL_ACTION ACTIONS::pickerTool( "common.InteractivePicker.pickerTool",
        AS_GLOBAL, 0, "", "", "", BITMAPS::INVALID_BITMAP, AF_ACTIVATE );

TOOL_ACTION ACTIONS::show3DViewer( "common.Control.show3DViewer",
        AS_GLOBAL,
        MD_ALT + '3', LEGACY_HK_NAME( "3D Viewer" ),
        _( "3D Viewer" ), _( "Show 3D viewer window" ),
        BITMAPS::three_d );

TOOL_ACTION ACTIONS::showSymbolBrowser( "common.Control.showSymbolBrowser",
        AS_GLOBAL, 0, "",
        _( "Symbol Library Browser" ), _( "Browse symbol libraries" ),
        BITMAPS::library_browser, AF_NONE, (void*) FRAME_SCH_VIEWER );

TOOL_ACTION ACTIONS::showSymbolEditor( "common.Control.showSymbolEditor",
        AS_GLOBAL, 0, "",
        _( "Symbol Editor" ), _( "Create, delete and edit symbols" ),
        BITMAPS::libedit, AF_NONE, (void*) FRAME_SCH_SYMBOL_EDITOR );

TOOL_ACTION ACTIONS::showFootprintBrowser( "common.Control.showFootprintBrowser",
        AS_GLOBAL, 0, "",
        _( "Footprint Library Browser" ), _( "Browse footprint libraries" ),
        BITMAPS::library_browser, AF_NONE, (void*) FRAME_FOOTPRINT_VIEWER );

TOOL_ACTION ACTIONS::showFootprintEditor( "common.Control.showFootprintEditor",
        AS_GLOBAL, 0, "",
        _( "Footprint Editor" ), _( "Create, delete and edit footprints" ),
        BITMAPS::module_editor, AF_NONE, (void*) FRAME_FOOTPRINT_EDITOR );

TOOL_ACTION ACTIONS::updatePcbFromSchematic( "common.Control.updatePcbFromSchematic",
        AS_GLOBAL,
        WXK_F8, LEGACY_HK_NAME( "Update PCB from Schematic" ),
        _( "Update PCB from Schematic..." ), _( "Update PCB with changes made to schematic" ),
        BITMAPS::update_pcb_from_sch );

TOOL_ACTION ACTIONS::updateSchematicFromPcb( "common.Control.updateSchematicFromPCB",
        AS_GLOBAL, 0, "",
        _( "Update Schematic from PCB..." ), _( "Update schematic with changes made to PCB" ),
        BITMAPS::update_sch_from_pcb );

TOOL_ACTION ACTIONS::openPreferences( "common.SuiteControl.openPreferences",
        AS_GLOBAL, MD_CTRL + ',', "",
        _( "Preferences..." ), _( "Show preferences for all open tools" ),
        BITMAPS::preference );

TOOL_ACTION ACTIONS::configurePaths( "common.SuiteControl.configurePaths",
        AS_GLOBAL, 0, "",
        _( "Configure Paths..." ), _( "Edit path configuration environment variables" ),
        BITMAPS::path );

TOOL_ACTION ACTIONS::showSymbolLibTable( "common.SuiteControl.showSymbolLibTable",
        AS_GLOBAL, 0, "",
        _( "Manage Symbol Libraries..." ),
        _( "Edit the global and project symbol library lists" ),
        BITMAPS::library_table );

TOOL_ACTION ACTIONS::showFootprintLibTable( "common.SuiteControl.showFootprintLibTable",
        AS_GLOBAL, 0, "",
        _( "Manage Footprint Libraries..." ),
        _( "Edit the global and project footprint library lists" ),
        BITMAPS::library_table );

TOOL_ACTION ACTIONS::gettingStarted( "common.SuiteControl.gettingStarted",
        AS_GLOBAL, 0, "",
        _( "Getting Started with KiCad" ),
        _( "Open \"Getting Started in KiCad\" guide for beginners" ),
        BITMAPS::help );

TOOL_ACTION ACTIONS::help( "common.SuiteControl.help",
        AS_GLOBAL, 0, "",
        _( "Help" ),
        _( "Open product documentation in a web browser" ),
        BITMAPS::help_online );

TOOL_ACTION ACTIONS::listHotKeys( "common.SuiteControl.listHotKeys",
        AS_GLOBAL,
        MD_CTRL + WXK_F1, LEGACY_HK_NAME( "List Hotkeys" ),
        _( "List Hotkeys..." ),
        _( "Displays current hotkeys table and corresponding commands" ),
        BITMAPS::hotkeys );

TOOL_ACTION ACTIONS::getInvolved( "common.SuiteControl.getInvolved",
        AS_GLOBAL, 0, "",
        _( "Get Involved" ),
        _( "Open \"Contribute to KiCad\" in a web browser" ),
        BITMAPS::info );

TOOL_ACTION ACTIONS::donate( "common.SuiteControl.donate",
        AS_GLOBAL, 0, "",
        _( "Donate" ),
        _( "Open \"Donate to KiCad\" in a web browser" ) );

TOOL_ACTION ACTIONS::reportBug( "common.SuiteControl.reportBug",
        AS_GLOBAL, 0, "",
        _( "Report Bug" ),
        _( "Report a problem with KiCad" ),
        BITMAPS::bug );


// System-wide selection Events

const TOOL_EVENT EVENTS::SelectedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.selected" );
const TOOL_EVENT EVENTS::UnselectedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.unselected" );
const TOOL_EVENT EVENTS::ClearedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.cleared" );

const TOOL_EVENT EVENTS::SelectedItemsModified( TC_MESSAGE, TA_ACTION, "common.Interactive.modified" );
const TOOL_EVENT EVENTS::SelectedItemsMoved( TC_MESSAGE, TA_ACTION, "common.Interactive.moved" );
const TOOL_EVENT EVENTS::InhibitSelectionEditing( TC_MESSAGE, TA_ACTION, "common.Interactive.inhibit" );
const TOOL_EVENT EVENTS::UninhibitSelectionEditing( TC_MESSAGE, TA_ACTION, "common.Interactive.uninhibit" );

