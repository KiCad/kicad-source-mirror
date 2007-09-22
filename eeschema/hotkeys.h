/***************/
/* hotkeys.h */
/***************/
#ifndef KOTKEYS_H
#define KOTKEYS_H

#include "hotkeys_basic.h"

enum hotkey_id_commnand {
    HK_NOT_FOUND = 0,
    HK_RESET_LOCAL_COORD,
    HK_HELP,
    HK_ZOOM_IN,
    HK_ZOOM_OUT,
    HK_ZOOM_REDRAW,
    HK_ZOOM_CENTER,
    HK_NEXT_SEARCH,
    HK_DELETE,
    HK_REPEAT_LAST,
	HK_UNDO,
	HK_REDO,
    HK_MOVEBLOCK_TO_DRAGBLOCK,
    HK_ROTATE_COMPONENT,
    HK_MIRROR_X_COMPONENT,
    HK_MIRROR_Y_COMPONENT,
    HK_ORIENT_NORMAL_COMPONENT,
    HK_MOVE_COMPONENT,
    HK_DRAG_COMPONENT,
    HK_ADD_NEW_COMPONENT,
    HK_BEGIN_WIRE
};

// List of hotkey descriptors for eeschema
extern struct Ki_HotkeyInfoSectionDescriptor s_Eeschema_Hokeys_Descr[];
// List of hotkey descriptors for the schematic editor only
extern struct Ki_HotkeyInfoSectionDescriptor s_Schematic_Hokeys_Descr[];
// List of hotkey descriptors for the lib editor only
extern struct Ki_HotkeyInfoSectionDescriptor s_Libedit_Hokeys_Descr[];

#endif		// KOTKEYS_H
