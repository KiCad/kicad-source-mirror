/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file pcbnew/hotkeys.h
 * Pcbnew hotkeys
 */
#ifndef _PCBNEW_HOTKEYS_H
#define _PCBNEW_HOTKEYS_H

#include <hotkeys_basic.h>

// List of hot keys id.
// see also enum common_hotkey_id_commnand in hotkeys_basic.h
// for shared hotkeys id
enum hotkey_id_commnand {
    HK_DELETE = HK_COMMON_END,
    HK_BACK_SPACE,
    HK_ROTATE_ITEM,
    HK_FLIP_ITEM,
    HK_COPY_ITEM,
    HK_MOVE_ITEM,
    HK_DRAG_ITEM,
    HK_GET_AND_MOVE_FOOTPRINT,
    HK_LOCK_UNLOCK_FOOTPRINT,
    HK_ADD_NEW_TRACK,
    HK_ADD_THROUGH_VIA,
    HK_SEL_LAYER_AND_ADD_THROUGH_VIA,
    HK_ADD_BLIND_BURIED_VIA,
    HK_SEL_LAYER_AND_ADD_BLIND_BURIED_VIA,
    HK_ADD_MICROVIA,
    HK_SWITCH_TRACK_POSTURE,
    HK_DRAG_TRACK_KEEP_SLOPE,
    HK_SAVE_BOARD,
    HK_SAVE_BOARD_AS,
    HK_LOAD_BOARD,
    HK_SAVE_MODULE,
    HK_SWITCH_UNITS,
    HK_SWITCH_TRACK_DISPLAY_MODE,
    HK_FIND_ITEM,
    HK_EDIT_ITEM,
    HK_PLACE_ITEM,
    HK_SWITCH_TRACK_WIDTH_TO_NEXT,
    HK_SWITCH_TRACK_WIDTH_TO_PREVIOUS,
    HK_SWITCH_GRID_TO_FASTGRID1,
    HK_SWITCH_GRID_TO_FASTGRID2,
    HK_SWITCH_GRID_TO_NEXT,
    HK_SWITCH_GRID_TO_PREVIOUS,
    HK_SWITCH_LAYER_TO_COPPER,
    HK_SWITCH_LAYER_TO_COMPONENT,
    HK_SWITCH_LAYER_TO_NEXT,
    HK_SWITCH_LAYER_TO_PREVIOUS,
    HK_SWITCH_LAYER_TO_INNER1,
    HK_SWITCH_LAYER_TO_INNER2,
    HK_SWITCH_LAYER_TO_INNER3,
    HK_SWITCH_LAYER_TO_INNER4,
    HK_SWITCH_LAYER_TO_INNER5,
    HK_SWITCH_LAYER_TO_INNER6,
    HK_SWITCH_LAYER_TO_INNER7,
    HK_SWITCH_LAYER_TO_INNER8,
    HK_SWITCH_LAYER_TO_INNER9,
    HK_SWITCH_LAYER_TO_INNER10,
    HK_SWITCH_LAYER_TO_INNER11,
    HK_SWITCH_LAYER_TO_INNER12,
    HK_SWITCH_LAYER_TO_INNER13,
    HK_SWITCH_LAYER_TO_INNER14,
    HK_ADD_MODULE,
    HK_SLIDE_TRACK,
    HK_MACRO_ID_BEGIN,
    HK_RECORD_MACROS_0,     // keep these id ordered from 0 to 9
    HK_RECORD_MACROS_1,     // because this order is used in code
    HK_RECORD_MACROS_2,
    HK_RECORD_MACROS_3,
    HK_RECORD_MACROS_4,
    HK_RECORD_MACROS_5,
    HK_RECORD_MACROS_6,
    HK_RECORD_MACROS_7,
    HK_RECORD_MACROS_8,
    HK_RECORD_MACROS_9,
    HK_CALL_MACROS_0,
    HK_CALL_MACROS_1,
    HK_CALL_MACROS_2,
    HK_CALL_MACROS_3,
    HK_CALL_MACROS_4,
    HK_CALL_MACROS_5,
    HK_CALL_MACROS_6,
    HK_CALL_MACROS_7,
    HK_CALL_MACROS_8,
    HK_CALL_MACROS_9,
    HK_MACRO_ID_END,
    HK_SWITCH_HIGHCONTRAST_MODE,
    HK_CANVAS_DEFAULT,
    HK_CANVAS_OPENGL,
    HK_CANVAS_CAIRO,
    HK_LEFT_CLICK,
    HK_LEFT_DCLICK
};

// Full list of hotkey descriptors for board editor and footprint editor
extern struct EDA_HOTKEY_CONFIG g_Pcbnew_Editor_Hokeys_Descr[];

// List of hotkey descriptors for the board editor only
extern struct EDA_HOTKEY_CONFIG g_Board_Editor_Hokeys_Descr[];

// List of hotkey descriptors for the footprint editor only
extern struct EDA_HOTKEY_CONFIG g_Module_Editor_Hokeys_Descr[];

// List of hotkey descriptors for the footprint editor only
extern struct EDA_HOTKEY_CONFIG g_Module_Viewer_Hokeys_Descr[];

// List of common hotkey descriptors
// used in hotkeys_board_editor.cpp and hotkeys_module_editor.cpp
extern EDA_HOTKEY* common_Hotkey_List[];

// List of hotkey descriptors for pcbnew
// used in hotkeys_board_editor.cpp
extern EDA_HOTKEY* board_edit_Hotkey_List[];

// List of hotkey descriptors for the module editor
// used in hotkeys_module_editor.cpp
extern EDA_HOTKEY* module_edit_Hotkey_List[];


#endif /* _PCBNEW_HOTKEYS_H_ */
