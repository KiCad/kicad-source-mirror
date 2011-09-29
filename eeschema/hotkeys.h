/**
 * eeschema/hotkeys.h
 */
#ifndef KOTKEYS_H
#define KOTKEYS_H

#include "hotkeys_basic.h"

// List of hot keys id.
// see also enum common_hotkey_id_commnand in hotkeys_basic.h
// for shared hotkeys id
enum hotkey_id_commnand {
    HK_FIND_NEXT_ITEM = HK_COMMON_END,
    HK_FIND_NEXT_DRC_MARKER,
    HK_FIND_ITEM,
    HK_DELETE,
    HK_REPEAT_LAST,
    HK_LIBEDIT_MOVE_GRAPHIC_ITEM,
    HK_MOVEBLOCK_TO_DRAGBLOCK,
    HK_LIBEDIT_CREATE_PIN,
    HK_DELETE_PIN,
    HK_ROTATE,
    HK_EDIT,
    HK_EDIT_COMPONENT_VALUE,
    HK_EDIT_COMPONENT_FOOTPRINT,
    HK_MIRROR_X_COMPONENT,
    HK_MIRROR_Y_COMPONENT,
    HK_ORIENT_NORMAL_COMPONENT,
    HK_MOVE_COMPONENT_OR_ITEM,
    HK_COPY_COMPONENT_OR_LABEL,
    HK_DRAG,
    HK_ADD_NEW_COMPONENT,
    HK_ADD_NEW_POWER,
    HK_BEGIN_WIRE,
    HK_BEGIN_BUS,
    HK_ADD_WIRE_ENTRY,
    HK_ADD_BUS_ENTRY,
    HK_ADD_LABEL,
    HK_ADD_HLABEL,
    HK_ADD_GLABEL,
    HK_ADD_JUNCTION,
    HK_ADD_HIER_SHEET,
    HK_ADD_GRAPHIC_TEXT,
    HK_ADD_GRAPHIC_POLYLINE,
    HK_ADD_NOCONN_FLAG
};

// List of hotkey descriptors for eeschema
extern struct EDA_HOTKEY_CONFIG s_Eeschema_Hokeys_Descr[];

// List of hotkey descriptors for the schematic editor only
extern struct EDA_HOTKEY_CONFIG s_Schematic_Hokeys_Descr[];

// List of hotkey descriptors for the lib editor only
extern struct EDA_HOTKEY_CONFIG s_Libedit_Hokeys_Descr[];

// List of hotkey descriptors for the lib browser only
extern struct EDA_HOTKEY_CONFIG s_Viewlib_Hokeys_Descr[];

#endif      // KOTKEYS_H
