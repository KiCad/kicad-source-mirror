/**
 * @file pagelayout_editor/hotkeys.h
 */

#ifndef PL_EDITOR_KOTKEYS_H_
#define PL_EDITOR_KOTKEYS_H_

#include <hotkeys_basic.h>

// List of hot keys id.
// see also enum common_hotkey_id_commnand in hotkeys_basic.h
// for shared hotkeys id
enum hotkey_id_commnand {
    HK_SWITCH_UNITS = HK_COMMON_END,
    HK_MOVE_ITEM,
    HK_MOVE_START_POINT,
    HK_MOVE_END_POINT,
    HK_DELETE_ITEM
};

// List of hotkey descriptors for PlEditor.
extern struct EDA_HOTKEY_CONFIG s_PlEditor_Hokeys_Descr[];

#endif		// PL_EDITOR_KOTKEYS_H_
