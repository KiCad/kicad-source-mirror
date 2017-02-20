#include <tool/actions.h>
#include <hotkeys.h>
#include <bitmaps.h>

// These members are static in class ACTIONS: Build them here:

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

TOOL_ACTION ACTIONS::zoomPreset( "common.Control.zoomPreset",
        AS_GLOBAL, 0,
        "", "" );

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
