#include <tool/actions.h>
#include <hotkeys.h>
#include <bitmaps.h>

// These members are static in class ACTIONS: Build them here:

// Generic Actions
TOOL_ACTION ACTIONS::cancelInteractive( "common.Interactive.cancel",
        AS_GLOBAL, 0,   // ESC key is handled in the dispatcher
        _( "Cancel" ), _( "Cancel current tool" ),
        cancel_xpm, AF_NONE );

// View Controls
TOOL_ACTION ACTIONS::zoomIn( "common.Control.zoomIn",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_IN ),
        _( "Zoom In" ), "", zoom_in_xpm );

TOOL_ACTION ACTIONS::zoomOut( "common.Control.zoomOut",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_OUT ),
        _( "Zoom Out" ), "", zoom_out_xpm );

TOOL_ACTION ACTIONS::zoomInCenter( "common.Control.zoomInCenter",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION ACTIONS::zoomOutCenter( "common.Control.zoomOutCenter",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION ACTIONS::zoomCenter( "common.Control.zoomCenter",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_CENTER ),
        _( "Center" ), "", zoom_center_on_screen_xpm );

TOOL_ACTION ACTIONS::zoomFitScreen( "common.Control.zoomFitScreen",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_AUTO ),
        _( "Zoom Auto" ), "", zoom_fit_in_page_xpm );

TOOL_ACTION ACTIONS::zoomTool( "common.Control.zoomTool",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_SELECTION ),
        _( "Zoom to Selection" ), "", NULL, AF_ACTIVATE );

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
        "", "" );

TOOL_ACTION ACTIONS::gridResetOrigin( "common.Control.gridResetOrigin",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_RESET_GRID_ORIGIN ),
        "", "" );

TOOL_ACTION ACTIONS::gridPreset( "common.Control.gridPreset",
        AS_GLOBAL, 0,
        "", "" );
