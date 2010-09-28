/***************/
/* hotkeys.h */
/***************/
#ifndef KOTKEYS_H
#define KOTKEYS_H

#include "hotkeys_basic.h"

// List of hot keys id.
// see also enum common_hotkey_id_commnand in hotkeys_basic.h
// for shared hotkeys id
enum hotkey_id_commnand {
	HK_SWITCH_UNITS = HK_COMMON_END,
	HK_SWITCH_GBR_ITEMS_DISPLAY_MODE,
    HK_SWITCH_LAYER_TO_NEXT,
    HK_SWITCH_LAYER_TO_PREVIOUS
};

// List of hotkey descriptors for pcbnew
extern struct Ki_HotkeyInfoSectionDescriptor s_Gerbview_Hokeys_Descr[];

#endif		// KOTKEYS_H
