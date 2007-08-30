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
    HK_DELETE,
    HK_BACK_SPACE,
    HK_ROTATE_FOOTPRINT,
    HK_MOVE_FOOTPRINT,
    HK_DRAG_FOOTPRINT,
    HK_FLIP_FOOTPRINT,
    HK_LOCK_UNLOCK_FOOTPRINT,
    HK_ADD_VIA, HK_END_TRACK,
    HK_SAVE_BOARD, HK_LOAD_BOARD,
    HK_SWITCH_UNITS, HK_SWITCH_TRACK_DISPLAY_MODE,
    HK_FIND_ITEM,
    HK_SWITCH_LAYER_TO_COPPER,
    HK_SWITCH_LAYER_TO_COMPONENT,
    HK_SWITCH_LAYER_TO_NEXT,
    HK_SWITCH_LAYER_TO_PREVIOUS,
    HK_SWITCH_LAYER_TO_INNER1,
    HK_SWITCH_LAYER_TO_INNER2,
    HK_SWITCH_LAYER_TO_INNER3,
    HK_SWITCH_LAYER_TO_INNER4,
    HK_SWITCH_LAYER_TO_INNER5,
    HK_SWITCH_LAYER_TO_INNER6
};

// List of hotkey descriptors for pcbnew
extern Ki_HotkeyInfo* s_board_edit_Hotkey_List[];
// List of hotkey descriptors for the module editor
extern Ki_HotkeyInfo* s_module_edit_Hotkey_List[];

#endif		// KOTKEYS_H
