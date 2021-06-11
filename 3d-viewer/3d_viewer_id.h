/**
 * @file 3d_viewer_id.h
 */

/**
 * Command IDs for the 3D viewer.
 *
 * Please add IDs that are unique to the 3D viewer here and not in the global
 * id.h file.  This will prevent the entire project from being rebuilt when
 * adding new commands to the 3D viewer.
 * However the number of IDs should be < ROOM_FOR_3D_VIEWER, defined in id.h
 * Please change the value of ROOM_FOR_3D_VIEWER if too small.
 */

#include <id.h>        // Generic Id.

enum id_3dview_frm
{
    ID_START_COMMAND_3D = ID_KICAD_3D_VIEWER_START,
    ID_RELOAD3D_BOARD,
    ID_VIEW3D_TOP,
    ID_VIEW3D_BOTTOM,
    ID_VIEW3D_LEFT,
    ID_VIEW3D_RIGHT,
    ID_VIEW3D_FRONT,
    ID_VIEW3D_BACK,
    ID_VIEW3D_RESET,
    ID_VIEW3D_FLIP,
    ID_TOOL_SCREENCOPY_TOCLIBBOARD,

    ID_MENU_SCREENCOPY_PNG,
    ID_MENU_SCREENCOPY_JPEG,
    ID_MENU_SCREENCOPY_TOCLIBBOARD,

    ID_MENU3D_RESET_DEFAULTS,

    // Help
    ID_MENU_COMMAND_END,

    ID_RENDER_CURRENT_VIEW,

    ID_DISABLE_RAY_TRACING,

    ID_CUSTOM_EVENT_1,      // A id for a custom event (canvas refresh request)

    ID_END_COMMAND_3D = ID_KICAD_3D_VIEWER_END,
};
