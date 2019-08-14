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

#include <fctsys.h>
#include <tool/actions.h>
#include <bitmaps.h>
#include <frame_type.h>


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

TOOL_ACTION ACTIONS::doNew( "common.Control.new",
        AS_GLOBAL, 
        MD_CTRL + 'N', LEGACY_HK_NAME( "New" ),
        _( "New..." ), _( "Create a new document in the editor" ),
        new_generic_xpm );

TOOL_ACTION ACTIONS::newLibrary( "common.Control.newLibrary",
        AS_GLOBAL, 
        0, "",
        _( "New Library..." ), _( "Create a new library folder" ),
        new_generic_xpm );

TOOL_ACTION ACTIONS::addLibrary( "common.Control.addLibrary",
        AS_GLOBAL, 
        0, "",
        _( "Add Library..." ), _( "Add an existing library folder" ),
        add_library_xpm );

TOOL_ACTION ACTIONS::open( "common.Control.open",
        AS_GLOBAL, 
        MD_CTRL + 'O', LEGACY_HK_NAME( "Open" ),
        _( "Open..." ), _( "Open existing document" ),
        directory_xpm );

TOOL_ACTION ACTIONS::save( "common.Control.save",
        AS_GLOBAL, 
        MD_CTRL + 'S', LEGACY_HK_NAME( "Save" ),
        _( "Save" ), _( "Save changes" ),
        save_xpm );

TOOL_ACTION ACTIONS::saveAs( "common.Control.saveAs",
        AS_GLOBAL, 
        MD_SHIFT + MD_CTRL + 'S', LEGACY_HK_NAME( "Save As" ),
        _( "Save As..." ), _( "Save current document to another location" ),
        save_as_xpm );

TOOL_ACTION ACTIONS::saveCopyAs( "common.Control.saveCopyAs",
        AS_GLOBAL, 
        0, "",
        _( "Save Copy As..." ), _( "Save a copy of the current document to another location" ),
        save_as_xpm );

TOOL_ACTION ACTIONS::saveAll( "common.Control.saveAll",
        AS_GLOBAL, 
        0, "",
        _( "Save All" ), _( "Save all changes" ),
        save_xpm );

TOOL_ACTION ACTIONS::revert( "common.Control.revert",
        AS_GLOBAL, 
        0, "",
        _( "Revert" ), _( "Throw away changes" ),
        undo_xpm );

TOOL_ACTION ACTIONS::pageSettings( "common.Control.pageSettings",
        AS_GLOBAL, 
        0, "",
        _( "Page Settings..." ), _( "Settings for paper size and title block info" ),
        sheetset_xpm );

TOOL_ACTION ACTIONS::print( "common.Control.print",
        AS_GLOBAL, 
        MD_CTRL + 'P', LEGACY_HK_NAME( "Print" ),
        _( "Print..." ), _( "Print" ),
        print_button_xpm );

TOOL_ACTION ACTIONS::plot( "common.Control.plot",
        AS_GLOBAL, 
        0, "",
        _( "Plot..." ), _( "Plot" ),
        plot_xpm );

TOOL_ACTION ACTIONS::quit( "common.Control.quit",
        AS_GLOBAL, 
        0, "",   // Not currently in use due to wxWidgets crankiness
        _( "Quit" ), _( "Close the current editor" ),
        exit_xpm );

// Generic Edit Actions
TOOL_ACTION ACTIONS::cancelInteractive( "common.Interactive.cancel",
        AS_GLOBAL, 
        0, "",   // ESC key is handled in the dispatcher
        _( "Cancel" ), _( "Cancel current tool" ),
        cancel_xpm, AF_NONE );

TOOL_ACTION ACTIONS::showContextMenu( "common.Control.showContextMenu",
        AS_GLOBAL,
        0, "",
        _( "Show Context Menu" ), _( "Perform the right-mouse-button action" ),
        nullptr, AF_NONE, (void*) CURSOR_RIGHT_CLICK );

TOOL_ACTION ACTIONS::updateMenu( "common.Interactive.updateMenu",
        AS_GLOBAL );

TOOL_ACTION ACTIONS::undo( "common.Interactive.undo",
        AS_GLOBAL, 
        MD_CTRL + 'Z', LEGACY_HK_NAME( "Undo" ),
        _( "Undo" ), _( "Undo last edit" ),
        undo_xpm );

TOOL_ACTION ACTIONS::redo( "common.Interactive.redo",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_SHIFT + MD_CTRL + 'Z',
#else
        MD_CTRL + 'Y',
#endif
        LEGACY_HK_NAME( "Redo" ),
        _( "Redo" ), _( "Redo last edit" ),
        redo_xpm );

TOOL_ACTION ACTIONS::cut( "common.Interactive.cut",
        AS_GLOBAL, 
        MD_CTRL + 'X', LEGACY_HK_NAME( "Cut" ),
        _( "Cut" ), _( "Cut selected item(s) to clipboard" ),
        cut_xpm );

TOOL_ACTION ACTIONS::copy( "common.Interactive.copy",
        AS_GLOBAL, 
        MD_CTRL + 'C', LEGACY_HK_NAME( "Copy" ),
        _( "Copy" ), _( "Copy selected item(s) to clipboard" ),
        copy_xpm );

TOOL_ACTION ACTIONS::paste( "common.Interactive.paste",
        AS_GLOBAL, 
        MD_CTRL + 'V', LEGACY_HK_NAME( "Paste" ),
        _( "Paste" ), _( "Paste clipboard into schematic" ),
        paste_xpm );

TOOL_ACTION ACTIONS::duplicate( "common.Interactive.duplicate",
        AS_GLOBAL, 
        MD_CTRL + 'D', LEGACY_HK_NAME( "Duplicate" ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ),
        duplicate_xpm );

TOOL_ACTION ACTIONS::doDelete( "common.Interactive.delete",
        AS_GLOBAL, 
        WXK_DELETE, LEGACY_HK_NAME( "Delete Item" ),
        _( "Delete" ), _( "Deletes selected item(s)" ),
        delete_xpm );

TOOL_ACTION ACTIONS::deleteTool( "common.Interactive.deleteTool",
        AS_GLOBAL, 0, "",
        _( "Interactive Delete Tool" ), _( "Delete clicked items" ),
        delete_xpm, AF_ACTIVATE );

TOOL_ACTION ACTIONS::activatePointEditor( "common.Control.activatePointEditor",
        AS_GLOBAL );

TOOL_ACTION ACTIONS::find( "common.Interactive.find",
        AS_GLOBAL, 
        MD_CTRL + 'F', LEGACY_HK_NAME( "Find" ),
        _( "Find" ), _( "Find text" ),
        find_xpm );

TOOL_ACTION ACTIONS::findAndReplace( "common.Interactive.findAndReplace",
        AS_GLOBAL, 
        MD_CTRL + MD_ALT + 'F', LEGACY_HK_NAME( "Find and Replace" ),
        _( "Find and Replace" ), _( "Find and replace text" ),
        find_replace_xpm );

TOOL_ACTION ACTIONS::findNext( "common.Interactive.findNext",
        AS_GLOBAL, 
        WXK_F5, LEGACY_HK_NAME( "Find Next" ),
        _( "Find Next" ), _( "Find next match" ),
        find_xpm );

TOOL_ACTION ACTIONS::findNextMarker( "common.Interactive.findNextMarker",
        AS_GLOBAL, 
        MD_SHIFT + WXK_F5, LEGACY_HK_NAME( "Find Next Marker" ),
        _( "Find Next Marker" ), "",
        find_xpm );

TOOL_ACTION ACTIONS::replaceAndFindNext( "common.Interactive.replaceAndFindNext",
        AS_GLOBAL, 
        0, "",
        _( "Replace and Find Next" ), _( "Replace current match and find next" ),
        find_replace_xpm );

TOOL_ACTION ACTIONS::replaceAll( "common.Interactive.replaceAll",
        AS_GLOBAL, 
        0, "",
        _( "Replace All" ), _( "Replace all matches" ),
        find_replace_xpm );

TOOL_ACTION ACTIONS::updateFind( "common.Control.updateFind",
        AS_GLOBAL );

// View Controls
TOOL_ACTION ACTIONS::zoomRedraw( "common.Control.zoomRedraw",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_CTRL + 'R',
#else
        WXK_F3,
#endif
        LEGACY_HK_NAME( "Zoom Redraw" ),
        _( "Refresh" ), _( "Refresh" ),
        zoom_redraw_xpm );

TOOL_ACTION ACTIONS::zoomFitScreen( "common.Control.zoomFitScreen",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_CTRL + '0',
#else
        WXK_HOME,
#endif
        LEGACY_HK_NAME( "Zoom Auto" ),
        _( "Zoom to Fit" ), _( "Zoom to Fit" ),
        zoom_fit_in_page_xpm );

TOOL_ACTION ACTIONS::zoomIn( "common.Control.zoomIn",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_CTRL + '+',
#else
        WXK_F1, 
#endif
        LEGACY_HK_NAME( "Zoom In" ),
        _( "Zoom In at Cursor" ), _( "Zoom In at Cursor" ),
        zoom_in_xpm );

TOOL_ACTION ACTIONS::zoomOut( "common.Control.zoomOut",
        AS_GLOBAL,
#if defined( __WXMAC__ )
        MD_CTRL + '-',
#else
        WXK_F2,
#endif
        LEGACY_HK_NAME( "Zoom Out" ),
        _( "Zoom Out at Cursor" ), _( "Zoom Out at Cursor" ),
        zoom_out_xpm );

TOOL_ACTION ACTIONS::zoomInCenter( "common.Control.zoomInCenter",
        AS_GLOBAL, 
        0, "",
        _( "Zoom In" ), _( "Zoom In" ),
        zoom_in_xpm );

TOOL_ACTION ACTIONS::zoomOutCenter( "common.Control.zoomOutCenter",
        AS_GLOBAL, 
        0, "",
        _( "Zoom Out" ), _( "Zoom Out" ),
        zoom_out_xpm );

TOOL_ACTION ACTIONS::zoomCenter( "common.Control.zoomCenter",
        AS_GLOBAL, 
        WXK_F4, LEGACY_HK_NAME( "Zoom Center" ),
        _( "Center" ), _( "Center" ),
        zoom_center_on_screen_xpm );

TOOL_ACTION ACTIONS::zoomTool( "common.Control.zoomTool",
        AS_GLOBAL, 
        MD_CTRL + WXK_F5, LEGACY_HK_NAME( "Zoom to Selection" ),
        _( "Zoom to Selection" ), _( "Zoom to Selection" ),
        zoom_area_xpm, AF_ACTIVATE );

TOOL_ACTION ACTIONS::zoomPreset( "common.Control.zoomPreset",
        AS_GLOBAL );

TOOL_ACTION ACTIONS::centerContents( "common.Control.centerContents",
        AS_GLOBAL );

// Cursor control
TOOL_ACTION ACTIONS::cursorUp( "common.Control.cursorUp",
        AS_GLOBAL, 
        WXK_UP, "",
        _( "Cursor Up" ), "",
        nullptr, AF_NONE, (void*) CURSOR_UP );

TOOL_ACTION ACTIONS::cursorDown( "common.Control.cursorDown",
        AS_GLOBAL, 
        WXK_DOWN, "",
        _( "Cursor Down" ), "" ,
        nullptr, AF_NONE, (void*) CURSOR_DOWN );

TOOL_ACTION ACTIONS::cursorLeft( "common.Control.cursorLeft",
        AS_GLOBAL, 
        WXK_LEFT, "",
        _( "Cursor Left" ), "" ,
        nullptr, AF_NONE, (void*) CURSOR_LEFT );

TOOL_ACTION ACTIONS::cursorRight( "common.Control.cursorRight",
        AS_GLOBAL, 
        WXK_RIGHT, "",
        _( "Cursor Right" ), "" ,
        nullptr, AF_NONE, (void*) CURSOR_RIGHT );


TOOL_ACTION ACTIONS::cursorUpFast( "common.Control.cursorUpFast",
        AS_GLOBAL, 
        MD_CTRL + WXK_UP, "",
        _( "Cursor Up Fast" ), "",
        nullptr, AF_NONE, (void*) ( CURSOR_UP | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorDownFast( "common.Control.cursorDownFast",
        AS_GLOBAL, 
        MD_CTRL + WXK_DOWN, "",
        _( "Cursor Down Fast" ), "" ,
        nullptr, AF_NONE, (void*) ( CURSOR_DOWN | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorLeftFast( "common.Control.cursorLeftFast",
        AS_GLOBAL, 
        MD_CTRL + WXK_LEFT, "",
        _( "Cursor Left Fast" ), "" ,
        nullptr, AF_NONE, (void*) ( CURSOR_LEFT | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorRightFast( "common.Control.cursorRightFast",
        AS_GLOBAL, 
        MD_CTRL + WXK_RIGHT, "",
        _( "Cursor Right Fast" ), "" ,
        nullptr, AF_NONE, (void*) ( CURSOR_RIGHT | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorClick( "common.Control.cursorClick",
        AS_GLOBAL, 
        WXK_RETURN, LEGACY_HK_NAME( "Mouse Left Click" ),
        _( "Click" ), "Performs left mouse button click",
        nullptr, AF_NONE, (void*) CURSOR_CLICK );

TOOL_ACTION ACTIONS::cursorDblClick( "common.Control.cursorDblClick",
        AS_GLOBAL, 
        WXK_END, LEGACY_HK_NAME( "Mouse Left Double Click" ),
        _( "Double-click" ), "Performs left mouse button double-click",
        nullptr, AF_NONE, (void*) CURSOR_DBL_CLICK );

TOOL_ACTION ACTIONS::refreshPreview( "common.Control.refreshPreview",
         AS_GLOBAL );

TOOL_ACTION ACTIONS::panUp( "common.Control.panUp",
        AS_GLOBAL, 
        MD_SHIFT + WXK_UP, "",
        _( "Pan Up" ), "",
        nullptr, AF_NONE, (void*) CURSOR_UP );

TOOL_ACTION ACTIONS::panDown( "common.Control.panDown",
        AS_GLOBAL, 
        MD_SHIFT + WXK_DOWN, "",
        _( "Pan Down" ), "" ,
        nullptr, AF_NONE, (void*) CURSOR_DOWN );

TOOL_ACTION ACTIONS::panLeft( "common.Control.panLeft",
        AS_GLOBAL, 
        MD_SHIFT + WXK_LEFT, "",
        _( "Pan Left" ), "" ,
        nullptr, AF_NONE, (void*) CURSOR_LEFT );

TOOL_ACTION ACTIONS::panRight( "common.Control.panRight",
        AS_GLOBAL, 
        MD_SHIFT + WXK_RIGHT, "",
        _( "Pan Right" ), "" ,
        nullptr, AF_NONE, (void*) CURSOR_RIGHT );

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
        grid_select_axis_xpm );

TOOL_ACTION ACTIONS::gridResetOrigin( "common.Control.gridResetOrigin",
        AS_GLOBAL, 
        'Z', LEGACY_HK_NAME( "Reset Grid Origin" ),
        _( "Reset Grid Origin" ), "" );

TOOL_ACTION ACTIONS::gridPreset( "common.Control.gridPreset",
        AS_GLOBAL );

TOOL_ACTION ACTIONS::toggleGrid( "common.Control.toggleGrid",
        AS_GLOBAL, 0, "",
        _( "Show Grid" ), _( "Display grid dots or lines in the edit window" ),
        grid_xpm );

TOOL_ACTION ACTIONS::gridProperties( "common.Control.gridProperties",
        AS_GLOBAL, 0, "",
        _( "Grid Properties..." ), _( "Set grid dimensions" ),
        grid_select_xpm );

TOOL_ACTION ACTIONS::imperialUnits( "common.Control.imperialUnits",
        AS_GLOBAL, 0, "",
        _( "Imperial" ), _( "Use inches and mils" ),
        unit_inch_xpm );

TOOL_ACTION ACTIONS::metricUnits( "common.Control.metricUnits",
        AS_GLOBAL, 0, "",
        _( "Metric" ), _( "Use millimeters" ),
        unit_mm_xpm );

TOOL_ACTION ACTIONS::toggleUnits( "common.Control.toggleUnits",
        AS_GLOBAL, 
        MD_CTRL + 'U', LEGACY_HK_NAME( "Switch Units" ),
        _( "Switch units" ), _( "Switch between inches and millimeters" ),
        unit_mm_xpm );

TOOL_ACTION ACTIONS::togglePolarCoords( "common.Control.togglePolarCoords",
        AS_GLOBAL, 0, "",
        _( "Polar Coordinates" ), _( "Switch between polar and cartesian coordinate systems" ),
        polar_coord_xpm );

TOOL_ACTION ACTIONS::resetLocalCoords( "common.Control.resetLocalCoords",
        AS_GLOBAL, 
        ' ', LEGACY_HK_NAME( "Reset Local Coordinates" ),
        _( "Reset Local Coordinates" ), "" );

TOOL_ACTION ACTIONS::toggleCursor( "common.Control.toggleCursor",
        AS_GLOBAL, 
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        MD_CTRL + MD_SHIFT + 'X', LEGACY_HK_NAME( "Toggle Cursor Display (Modern Toolset only)" ),
        _( "Always Show Cursor" ), _( "Display crosshairs even in selection tool" ),
        cursor_xpm );

TOOL_ACTION ACTIONS::toggleCursorStyle( "common.Control.toggleCursorStyle",
        AS_GLOBAL, 0, "",
        _( "Full-Window Crosshairs" ), _( "Switch display of full-window crosshairs" ),
        cursor_shape_xpm );

TOOL_ACTION ACTIONS::highContrastMode( "common.Control.highContrastMode",
        AS_GLOBAL,
        MD_CTRL + 'H', LEGACY_HK_NAME( "Toggle High Contrast Mode" ),
        _( "High Contrast Mode" ), _( "Use high contrast display mode" ),
        contrast_mode_xpm );

TOOL_ACTION ACTIONS::selectionTool( "common.InteractiveSelection.selectionTool",
        AS_GLOBAL, 0, "", 
        _( "Select item(s)" ), "",
        cursor_xpm, AF_ACTIVATE );

TOOL_ACTION ACTIONS::measureTool( "common.InteractiveEdit.measureTool",
        AS_GLOBAL,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        MD_CTRL + MD_SHIFT + 'M', LEGACY_HK_NAME( "Measure Distance (Modern Toolset only)" ),
        _( "Measure Tool" ), _( "Interactively measure distance between points" ),
        measurement_xpm, AF_ACTIVATE );

TOOL_ACTION ACTIONS::pickerTool( "common.InteractivePicker.pickerTool",
        AS_GLOBAL, 0, "", "", "", NULL, AF_ACTIVATE );

TOOL_ACTION ACTIONS::show3DViewer( "common.Control.show3DViewer",
        AS_GLOBAL,
        MD_ALT + '3', LEGACY_HK_NAME( "3D Viewer" ),
        _( "3D Viewer" ), _( "Show 3D viewer window" ),
        three_d_xpm );

TOOL_ACTION ACTIONS::showSymbolBrowser( "common.Control.showSymbolBrowser",
        AS_GLOBAL, 0, "",
        _( "Symbol Library Browser" ), _( "Browse symbol libraries" ),
        library_browse_xpm, AF_NONE, (void*) FRAME_SCH_VIEWER );

TOOL_ACTION ACTIONS::showSymbolEditor( "common.Control.showSymbolEditor",
        AS_GLOBAL, 0, "",
        _( "Symbol Editor" ), _( "Create, delete and edit symbols" ),
        libedit_xpm, AF_NONE, (void*) FRAME_SCH_LIB_EDITOR );

TOOL_ACTION ACTIONS::showFootprintBrowser( "common.Control.showFootprintBrowser",
        AS_GLOBAL, 0, "",
        _( "Footprint Library Browser" ), _( "Browse footprint libraries" ),
        modview_icon_xpm, AF_NONE, (void*) FRAME_PCB_MODULE_VIEWER );

TOOL_ACTION ACTIONS::showFootprintEditor( "common.Control.showFootprintEditor",
        AS_GLOBAL, 0, "",
        _( "Footprint Editor" ), _( "Create, delete and edit footprints" ),
        module_editor_xpm, AF_NONE, (void*) FRAME_PCB_MODULE_EDITOR );

TOOL_ACTION ACTIONS::updatePcbFromSchematic( "common.Control.updatePcbFromSchematic",
        AS_GLOBAL,
        WXK_F8, LEGACY_HK_NAME( "Update PCB from Schematic" ),
        _( "Update PCB from Schematic..." ), _( "Push changes from schematic to PCB" ),
        update_pcb_from_sch_xpm );

TOOL_ACTION ACTIONS::acceleratedGraphics( "common.Control.acceleratedGraphics",
        AS_GLOBAL,
#ifdef __WXMAC__
        MD_ALT + WXK_F11,
#else
        WXK_F11, 
#endif
        LEGACY_HK_NAME( "Switch to Modern Toolset with hardware-accelerated graphics (recommended)" ),
        _( "Accelerated Graphics" ), _( "Use hardware-accelerated graphics (recommended)" ),
        tools_xpm );

TOOL_ACTION ACTIONS::standardGraphics( "common.Control.standardGraphics",
        AS_GLOBAL,
#ifdef __WXMAC__
        MD_ALT + WXK_F12,
#else
        WXK_F12,
#endif
        LEGACY_HK_NAME( "Switch to Modern Toolset with software graphics (fall-back)" ),
        _( "Standard Graphics" ), _( "Use software graphics (fall-back)" ),
        tools_xpm );

TOOL_ACTION ACTIONS::configurePaths( "common.SuiteControl.configurePaths",
        AS_GLOBAL, 0, "",
        _( "Configure Paths..." ), _( "Edit path configuration environment variables" ),
        path_xpm );

TOOL_ACTION ACTIONS::showSymbolLibTable( "common.SuiteControl.showSymbolLibTable",
        AS_GLOBAL, 0, "",
        _( "Manage Symbol Libraries..." ),
        _( "Edit the global and project symbol library lists" ),
        library_table_xpm );

TOOL_ACTION ACTIONS::showFootprintLibTable( "common.SuiteControl.showFootprintLibTable",
        AS_GLOBAL, 0, "",
        _( "Manage Footprint Libraries..." ),
        _( "Edit the global and project footprint library lists" ),
        library_table_xpm );

TOOL_ACTION ACTIONS::gettingStarted( "common.SuiteControl.gettingStarted",
        AS_GLOBAL, 0, "",
        _( "Getting Started with KiCad" ),
        _( "Open \"Getting Started in KiCad\" guide for beginners" ),
        help_xpm );

TOOL_ACTION ACTIONS::help( "common.SuiteControl.help",
        AS_GLOBAL, 0, "",
        _( "Help" ),
        _( "Open product documentation in a web browser" ),
        online_help_xpm );

TOOL_ACTION ACTIONS::listHotKeys( "common.SuiteControl.listHotKeys",
        AS_GLOBAL,
        MD_CTRL + WXK_F1, LEGACY_HK_NAME( "List Hotkeys" ),
        _( "List Hotkeys..." ),
        _( "Displays current hotkeys table and corresponding commands" ),
        hotkeys_xpm );
        
TOOL_ACTION ACTIONS::getInvolved( "common.SuiteControl.getInvolved",
        AS_GLOBAL, 0, "",
        _( "Get Involved" ),
        _( "Open \"Contribute to KiCad\" in a web browser" ),
        info_xpm );
        

// System-wide selection Events

const TOOL_EVENT EVENTS::SelectedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.selected" );
const TOOL_EVENT EVENTS::UnselectedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.unselected" );
const TOOL_EVENT EVENTS::ClearedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.cleared" );

const TOOL_EVENT EVENTS::SelectedItemsModified( TC_MESSAGE, TA_ACTION, "common.Interactive.modified" );

