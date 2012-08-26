/**
 * @file 3d_viewer_id.h
 */

/**
 * Command IDs for the 3D viewer.
 *
 * Please add IDs that are unique to the 3D viewer here and not in the global
 * id.h file.  This will prevent the entire project from being rebuilt when
 * adding new commands to the 3D viewer.
 */

 #include <id.h>        // Generic Id.

enum id_3dview_frm
{
    ID_START_COMMAND_3D = ID_END_LIST,
    ID_ROTATE3D_X_NEG,
    ID_ROTATE3D_X_POS,
    ID_ROTATE3D_Y_NEG,
    ID_ROTATE3D_Y_POS,
    ID_ROTATE3D_Z_NEG,
    ID_ROTATE3D_Z_POS,
    ID_RELOAD3D_BOARD,
    ID_TOOL_SCREENCOPY_TOCLIBBOARD,
    ID_MOVE3D_LEFT,
    ID_MOVE3D_RIGHT,
    ID_MOVE3D_UP,
    ID_MOVE3D_DOWN,
    ID_ORTHO,
    ID_MENU3D_BGCOLOR_SELECTION,
    ID_MENU3D_USE_COPPER_THICKNESS,
    ID_MENU3D_AXIS_ONOFF,
    ID_MENU3D_MODULE_ONOFF,
    ID_MENU3D_ZONE_ONOFF,
    ID_MENU3D_DRAWINGS_ONOFF,
    ID_MENU3D_COMMENTS_ONOFF,
    ID_MENU3D_ECO1_ONOFF,
    ID_MENU3D_ECO2_ONOFF,
    ID_END_COMMAND_3D,

    ID_MENU3D_GRID,
    ID_MENU3D_GRID_NOGRID,
    ID_MENU3D_GRID_10_MM,
    ID_MENU3D_GRID_5_MM,
    ID_MENU3D_GRID_2P5_MM,
    ID_MENU3D_GRID_1_MM,
    ID_MENU3D_GRID_END,

    ID_MENU_SCREENCOPY_PNG,
    ID_MENU_SCREENCOPY_JPEG,
    ID_MENU_SCREENCOPY_TOCLIBBOARD,

    ID_POPUP_3D_VIEW_START,
    ID_POPUP_ZOOMIN,
    ID_POPUP_ZOOMOUT,
    ID_POPUP_VIEW_XPOS,
    ID_POPUP_VIEW_XNEG,
    ID_POPUP_VIEW_YPOS,
    ID_POPUP_VIEW_YNEG,
    ID_POPUP_VIEW_ZPOS,
    ID_POPUP_VIEW_ZNEG,
    ID_POPUP_MOVE3D_LEFT,
    ID_POPUP_MOVE3D_RIGHT,
    ID_POPUP_MOVE3D_UP,
    ID_POPUP_MOVE3D_DOWN,
    ID_POPUP_3D_VIEW_END
};
