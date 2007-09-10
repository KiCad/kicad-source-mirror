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
	HK_SWITCH_UNITS,
	HK_SWITCH_TRACK_DISPLAY_MODE,
    HK_ZOOM_IN,
    HK_ZOOM_OUT,
    HK_ZOOM_REDRAW,
    HK_ZOOM_CENTER,
    HK_SWITCH_LAYER_TO_NEXT,
    HK_SWITCH_LAYER_TO_PREVIOUS
};

// List of hotkey descriptors for pcbnew
extern struct Ki_HotkeyInfoSectionDescriptor s_Gerbview_Hokeys_Descr[];

#endif		// KOTKEYS_H
