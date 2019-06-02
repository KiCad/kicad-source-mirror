#include <tool/actions.h>
#include <hotkeys.h>
#include <bitmaps.h>

// These members are static in class ACTIONS: Build them here:

TOOL_ACTION ACTIONS::doNew( "common.Control.new",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_NEW ),
        _( "New..." ), _( "Create a new document in the editor" ),
        new_generic_xpm );

TOOL_ACTION ACTIONS::newLibrary( "common.Control.newLibrary",
        AS_GLOBAL, 0,
        _( "New Library..." ), _( "Create a new library folder" ),
        new_generic_xpm );

TOOL_ACTION ACTIONS::addLibrary( "common.Control.addLibrary",
        AS_GLOBAL, 0,
        _( "Add Library..." ), _( "Add an existing library folder" ),
        add_library_xpm );

TOOL_ACTION ACTIONS::open( "common.Control.open",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_OPEN ),
        _( "Open..." ), _( "Open existing document" ),
        directory_xpm );

TOOL_ACTION ACTIONS::save( "common.Control.save",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SAVE ),
        _( "Save" ), _( "Save changes" ),
        save_xpm );

TOOL_ACTION ACTIONS::saveAs( "common.Control.saveAs",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SAVEAS ),
        _( "Save As..." ), _( "Save current document to another location" ),
        save_as_xpm );

TOOL_ACTION ACTIONS::saveCopyAs( "common.Control.saveCopyAs",
        AS_GLOBAL, 0,
        _( "Save Copy As..." ), _( "Save a copy of the current document to another location" ),
        save_as_xpm );

TOOL_ACTION ACTIONS::saveAll( "common.Control.saveAll",
        AS_GLOBAL, 0,
        _( "Save All" ), _( "Save all changes" ),
        save_xpm );

TOOL_ACTION ACTIONS::revert( "common.Control.revert",
        AS_GLOBAL, 0,
        _( "Revert" ), _( "Throw away changes" ),
        undo_xpm );

TOOL_ACTION ACTIONS::pageSettings( "common.Control.pageSettings",
        AS_GLOBAL, 0,
        _( "Page Settings..." ), _( "Settings for paper size and frame references" ),
        sheetset_xpm );

TOOL_ACTION ACTIONS::print( "common.Control.print",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_PRINT ),
        _( "Print..." ), _( "Print" ),
        print_button_xpm );

TOOL_ACTION ACTIONS::plot( "common.Control.plot",
        AS_GLOBAL, 0,
        _( "Plot..." ), _( "Plot" ),
        plot_xpm );

TOOL_ACTION ACTIONS::quit( "common.Control.quit",
        AS_GLOBAL, 0,
        _( "Quit" ), _( "Close the current editor" ),
        exit_xpm );

// Generic Edit Actions
TOOL_ACTION ACTIONS::cancelInteractive( "common.Interactive.cancel",
        AS_GLOBAL, 0,   // ESC key is handled in the dispatcher
        _( "Cancel" ), _( "Cancel current tool" ),
        cancel_xpm, AF_NONE );

TOOL_ACTION ACTIONS::updateMenu( "common.Interactive.updateMenu",
        AS_GLOBAL, 0, "", "" );   // This is an internal event

TOOL_ACTION ACTIONS::undo( "common.Interactive.undo",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_UNDO ),
        _( "Undo" ), _( "Undo last edit" ),
        undo_xpm );

TOOL_ACTION ACTIONS::redo( "common.Interactive.redo",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_REDO ),
        _( "Redo" ), _( "Redo last edit" ),
        redo_xpm );

TOOL_ACTION ACTIONS::cut( "common.Interactive.cut",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_CUT ),
        _( "Cut" ), _( "Cut selected item(s) to clipboard" ),
        cut_xpm );

TOOL_ACTION ACTIONS::copy( "common.Interactive.copy",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_COPY ),
        _( "Copy" ), _( "Copy selected item(s) to clipboard" ),
        copy_xpm );

TOOL_ACTION ACTIONS::paste( "common.Interactive.paste",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_PASTE ),
        _( "Paste" ), _( "Paste clipboard into schematic" ),
        paste_xpm );

TOOL_ACTION ACTIONS::duplicate( "common.Interactive.duplicate",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DUPLICATE ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ),
        duplicate_xpm );

TOOL_ACTION ACTIONS::doDelete( "common.Interactive.delete",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DELETE ),
        _( "Delete" ), _( "Deletes selected item(s)" ),
        delete_xpm );

TOOL_ACTION ACTIONS::activatePointEditor( "common.Control.activatePointEditor",
        AS_GLOBAL, 0, "", "" );   // This is an internal event

TOOL_ACTION ACTIONS::find( "common.Interactive.find",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_FIND ),
        _( "Find" ), _( "Find text" ),
        find_xpm );

TOOL_ACTION ACTIONS::findAndReplace( "common.Interactive.findAndReplace",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_REPLACE ),
        _( "Find and Replace" ), _( "Find and replace text" ),
        find_replace_xpm );

TOOL_ACTION ACTIONS::findNext( "common.Interactive.findNext",
        AS_GLOBAL, 0,
        _( "Find Next" ), _( "Find next match" ),
        find_xpm );

TOOL_ACTION ACTIONS::findNextMarker( "common.Interactive.findNextMarker",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_FIND_NEXT_MARKER ),
        _( "Find Next Marker" ), "",
        find_xpm );

TOOL_ACTION ACTIONS::replaceAndFindNext( "common.Interactive.replaceAndFindNext",
        AS_GLOBAL, 0,
        _( "Replace and Find Next" ), _( "Replace current match and find next" ),
        find_replace_xpm );

TOOL_ACTION ACTIONS::replaceAll( "common.Interactive.replaceAll",
        AS_GLOBAL, 0,
        _( "Replace All" ), _( "Replace all matches" ),
        find_replace_xpm );

TOOL_ACTION ACTIONS::updateFind( "common.Control.updateFind",
        AS_GLOBAL, 0, "", "" );   // This is an internal event

// View Controls
TOOL_ACTION ACTIONS::zoomRedraw( "common.Control.zoomRedraw",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_REDRAW ),
        _( "Refresh" ), "",
        zoom_redraw_xpm );

TOOL_ACTION ACTIONS::zoomIn( "common.Control.zoomIn",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_IN ),
        _( "Zoom In" ), "",
        zoom_in_xpm );

TOOL_ACTION ACTIONS::zoomOut( "common.Control.zoomOut",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_OUT ),
        _( "Zoom Out" ), "",
        zoom_out_xpm );

TOOL_ACTION ACTIONS::zoomInCenter( "common.Control.zoomInCenter",
        AS_GLOBAL, 0,
        _( "Zoom In" ), "",
        zoom_in_xpm );

TOOL_ACTION ACTIONS::zoomOutCenter( "common.Control.zoomOutCenter",
        AS_GLOBAL, 0,
        _( "Zoom Out" ), "",
        zoom_out_xpm );

TOOL_ACTION ACTIONS::zoomCenter( "common.Control.zoomCenter",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_CENTER ),
        _( "Center" ), "",
        zoom_center_on_screen_xpm );

TOOL_ACTION ACTIONS::zoomFitScreen( "common.Control.zoomFitScreen",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_AUTO ),
        _( "Zoom to Fit" ), "",
        zoom_fit_in_page_xpm );

TOOL_ACTION ACTIONS::zoomTool( "common.Control.zoomTool",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_SELECTION ),
        _( "Zoom to Selection" ), "",
        zoom_area_xpm, AF_ACTIVATE );

TOOL_ACTION ACTIONS::zoomPreset( "common.Control.zoomPreset",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION ACTIONS::centerContents( "common.Control.centerContents",
        AS_GLOBAL, 0,
        "", "" );

// Cursor control
TOOL_ACTION ACTIONS::cursorUp( "common.Control.cursorUp",
        AS_GLOBAL, WXK_UP,
        "", "", NULL, AF_NONE, (void*) CURSOR_UP );

TOOL_ACTION ACTIONS::cursorDown( "common.Control.cursorDown",
        AS_GLOBAL, WXK_DOWN,
        "", "" , NULL, AF_NONE, (void*) CURSOR_DOWN );

TOOL_ACTION ACTIONS::cursorLeft( "common.Control.cursorLeft",
        AS_GLOBAL, WXK_LEFT,
        "", "" , NULL, AF_NONE, (void*) CURSOR_LEFT );

TOOL_ACTION ACTIONS::cursorRight( "common.Control.cursorRight",
        AS_GLOBAL, WXK_RIGHT,
        "", "" , NULL, AF_NONE, (void*) CURSOR_RIGHT );


TOOL_ACTION ACTIONS::cursorUpFast( "common.Control.cursorUpFast",
        AS_GLOBAL, MD_CTRL + WXK_UP,
        "", "", NULL, AF_NONE, (void*) ( CURSOR_UP | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorDownFast( "common.Control.cursorDownFast",
        AS_GLOBAL, MD_CTRL + WXK_DOWN,
        "", "" , NULL, AF_NONE, (void*) ( CURSOR_DOWN | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorLeftFast( "common.Control.cursorLeftFast",
        AS_GLOBAL, MD_CTRL + WXK_LEFT,
        "", "" , NULL, AF_NONE, (void*) ( CURSOR_LEFT | CURSOR_FAST_MOVE ) );

TOOL_ACTION ACTIONS::cursorRightFast( "common.Control.cursorRightFast",
        AS_GLOBAL, MD_CTRL + WXK_RIGHT,
        "", "" , NULL, AF_NONE, (void*) ( CURSOR_RIGHT | CURSOR_FAST_MOVE ) );


TOOL_ACTION ACTIONS::cursorClick( "common.Control.cursorClick",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_LEFT_CLICK ),
        "", "", NULL, AF_NONE, (void*) CURSOR_CLICK );

TOOL_ACTION ACTIONS::cursorDblClick( "common.Control.cursorDblClick",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_LEFT_DCLICK ),
        "", "", NULL, AF_NONE, (void*) CURSOR_DBL_CLICK );

TOOL_ACTION ACTIONS::panUp( "common.Control.panUp",
        AS_GLOBAL, MD_SHIFT + WXK_UP,
        "", "", NULL, AF_NONE, (void*) CURSOR_UP );

TOOL_ACTION ACTIONS::panDown( "common.Control.panDown",
        AS_GLOBAL, MD_SHIFT + WXK_DOWN,
        "", "" , NULL, AF_NONE, (void*) CURSOR_DOWN );

TOOL_ACTION ACTIONS::panLeft( "common.Control.panLeft",
        AS_GLOBAL, MD_SHIFT + WXK_LEFT,
        "", "" , NULL, AF_NONE, (void*) CURSOR_LEFT );

TOOL_ACTION ACTIONS::panRight( "common.Control.panRight",
        AS_GLOBAL, MD_SHIFT + WXK_RIGHT,
        "", "" , NULL, AF_NONE, (void*) CURSOR_RIGHT );

// Grid control
TOOL_ACTION ACTIONS::gridFast1( "common.Control.gridFast1",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_GRID_TO_FASTGRID1 ),
        "", "" );

TOOL_ACTION ACTIONS::gridFast2( "common.Control.gridFast2",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_GRID_TO_FASTGRID2 ),
        "", "" );

TOOL_ACTION ACTIONS::gridNext( "common.Control.gridNext",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_GRID_TO_NEXT ),
        "", "" );

TOOL_ACTION ACTIONS::gridPrev( "common.Control.gridPrev",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_GRID_TO_PREVIOUS ),
        "", "" );

TOOL_ACTION ACTIONS::gridSetOrigin( "common.Control.gridSetOrigin",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SET_GRID_ORIGIN ),
        _( "Grid Origin" ), _( "Set the grid origin point" ),
        grid_select_axis_xpm );

TOOL_ACTION ACTIONS::gridResetOrigin( "common.Control.gridResetOrigin",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_RESET_GRID_ORIGIN ),
        "", "" );

TOOL_ACTION ACTIONS::gridPreset( "common.Control.gridPreset",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION ACTIONS::toggleGrid( "common.Control.toggleGrid",
        AS_GLOBAL, 0,
        _( "Show Grid" ), _( "Display grid dots or lines in the edit window" ),
        grid_xpm );

TOOL_ACTION ACTIONS::gridProperties( "common.Control.gridProperties",
        AS_GLOBAL, 0,
        _( "Grid Properties..." ), _( "Set grid dimensions" ),
        grid_select_xpm );

TOOL_ACTION ACTIONS::imperialUnits( "common.Control.imperialUnits",
        AS_GLOBAL, 0,
        _( "Imperial" ), _( "Use inches and mils" ),
        unit_inch_xpm );

TOOL_ACTION ACTIONS::metricUnits( "common.Control.metricUnits",
        AS_GLOBAL, 0,
        _( "Metric" ), _( "Use millimeters" ),
        unit_mm_xpm );

TOOL_ACTION ACTIONS::toggleUnits( "common.Control.toggleUnits",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_UNITS ),
        _( "Switch units" ), _( "Switch between inches and millimeters" ),
        unit_mm_xpm );

TOOL_ACTION ACTIONS::togglePolarCoords( "common.Control.togglePolarCoords",
        AS_GLOBAL, 0,
        _( "Polar Coordinates" ), _( "Switch between polar and cartesian coordinate systems" ),
        polar_coord_xpm );

TOOL_ACTION ACTIONS::resetLocalCoords( "common.Control.resetLocalCoords",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_RESET_LOCAL_COORD ),
        "", "" );

TOOL_ACTION ACTIONS::toggleCursor( "common.Control.toggleCursor",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_TOGGLE_CURSOR ),
        _( "Always Show Cursor" ), _( "Display crosshairs even in selection tool" ),
        cursor_xpm );

TOOL_ACTION ACTIONS::toggleCursorStyle( "common.Control.toggleCursorStyle",
        AS_GLOBAL, 0,
        _( "Full-Window Crosshairs" ), _( "Switch display of full-window crosshairs" ),
        cursor_shape_xpm );

TOOL_ACTION ACTIONS::highContrastMode( "common.Control.highContrastMode",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_HIGHCONTRAST_MODE ),
        _( "High Contrast Mode" ), _( "Use high contrast display mode" ),
        contrast_mode_xpm );

TOOL_ACTION ACTIONS::highContrastInc( "common.Control.highContrastInc",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_HIGHCONTRAST_INC ),
        "", "" );

TOOL_ACTION ACTIONS::highContrastDec( "common.Control.highContrastDec",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_HIGHCONTRAST_DEC ),
        "", "" );

TOOL_ACTION ACTIONS::acceleratedGraphics( "common.Control.acceleratedGraphics",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_CANVAS_OPENGL ),
        _( "Accelerated Graphics" ), _( "Use hardware-accelerated graphics (recommended)" ),
        tools_xpm );

TOOL_ACTION ACTIONS::standardGraphics( "common.Control.standardGraphics",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_CANVAS_CAIRO ),
        _( "Standard Graphics" ), _( "Use software graphics (fall-back)" ),
        tools_xpm );

TOOL_ACTION ACTIONS::show3DViewer( "pcbnew.Control.show3DViewer",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_3D_VIEWER ),
        _( "3D Viewer" ), _( "Show 3D viewer window" ),
        three_d_xpm );



// System-wide selection Events

///> Event sent after an item is selected.
const TOOL_EVENT EVENTS::SelectedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.selected" );

///> Event sent after an item is unselected.
const TOOL_EVENT EVENTS::UnselectedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.unselected" );

///> Event sent after selection is cleared.
const TOOL_EVENT EVENTS::ClearedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.cleared" );

const TOOL_EVENT EVENTS::SelectedItemsModified( TC_MESSAGE, TA_ACTION, "common.Interactive.modified" );

