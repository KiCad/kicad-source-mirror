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
    HK_MOVEBLOCK_TO_DRAGBLOCK,
    HK_ROTATE_COMPONENT,
    HK_MIRROR_X_COMPONENT,
    HK_MIRROR_Y_COMPONENT,
    HK_ORIENT_NORMAL_COMPONENT,
    HK_MOVE_COMPONENT,
    HK_ADD_NEW_COMPONENT,
    HK_BEGIN_WIRE
};

// List of hotkey descriptors for schematic
extern Ki_HotkeyInfo* s_Schematic_Hotkey_List[];
// List of hotkey descriptors for libray editor
extern Ki_HotkeyInfo* s_LibEdit_Hotkey_List[];

#endif		// KOTKEYS_H
