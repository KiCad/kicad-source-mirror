/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (c) 2013-2021 3Dconnexion.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file V3DCMD.hpp
 * @brief Virtual 3D Commands.
 */

/*----------------------------------------------------------------------
 *  V3DCMD.h -- Virtual 3D Commands
 *
 *  enums for all current V3DCMDs
 *  These are functions that all applications should respond to if they are
 *  at all applicable.  These cmds are generated from any number of places
 *  including hard and soft buttons.  They don't necessarily represent a
 *  hardware button
 *
 * Written:     January 2013
 * Original author:      Jim Wick
 *
 */
#ifndef _V3DCMD_H_
#define _V3DCMD_H_


/*
 * Constants
 */


/*
 * Virtual 3D Commands
 *
 * These function numbers will never change, but the list will be amended as more
 * V3DCMDs are created.
 * For use with SI_CMD_EVENT.
 * Most of these don't have a separate press and release of these events.
 * Some keys do have press and release (Esc, Shift, Ctrl) as expected of keyboard keys.
 */
typedef enum
{
    V3DCMD_NOOP = 0,
    V3DCMD_MENU_OPTIONS = 1,
    V3DCMD_VIEW_FIT = 2,
    V3DCMD_VIEW_TOP = 3,
    V3DCMD_VIEW_LEFT = 4,
    V3DCMD_VIEW_RIGHT = 5,
    V3DCMD_VIEW_FRONT = 6,
    V3DCMD_VIEW_BOTTOM = 7,
    V3DCMD_VIEW_BACK = 8,
    V3DCMD_VIEW_ROLLCW = 9,
    V3DCMD_VIEW_ROLLCCW = 10,
    V3DCMD_VIEW_ISO1 = 11,
    V3DCMD_VIEW_ISO2 = 12,
    V3DCMD_KEY_F1 = 13,
    V3DCMD_KEY_F2 = 14,
    V3DCMD_KEY_F3 = 15,
    V3DCMD_KEY_F4 = 16,
    V3DCMD_KEY_F5 = 17,
    V3DCMD_KEY_F6 = 18,
    V3DCMD_KEY_F7 = 19,
    V3DCMD_KEY_F8 = 20,
    V3DCMD_KEY_F9 = 21,
    V3DCMD_KEY_F10 = 22,
    V3DCMD_KEY_F11 = 23,
    V3DCMD_KEY_F12 = 24,
    V3DCMD_KEY_ESC = 25,
    V3DCMD_KEY_ALT = 26,
    V3DCMD_KEY_SHIFT = 27,
    V3DCMD_KEY_CTRL = 28,
    V3DCMD_FILTER_ROTATE = 29,
    V3DCMD_FILTER_PANZOOM = 30,
    V3DCMD_FILTER_DOMINANT = 31,
    V3DCMD_SCALE_PLUS = 32,
    V3DCMD_SCALE_MINUS = 33,
    V3DCMD_VIEW_SPINCW = 34,
    V3DCMD_VIEW_SPINCCW = 35,
    V3DCMD_VIEW_TILTCW = 36,
    V3DCMD_VIEW_TILTCCW = 37,
    V3DCMD_MENU_POPUP = 38,
    V3DCMD_MENU_BUTTONMAPPINGEDITOR = 39,
    V3DCMD_MENU_ADVANCEDSETTINGSEDITOR = 40,
    V3DCMD_MOTIONMACRO_ZOOM = 41,
    V3DCMD_MOTIONMACRO_ZOOMOUT_CURSORTOCENTER = 42,
    V3DCMD_MOTIONMACRO_ZOOMIN_CURSORTOCENTER = 43,
    V3DCMD_MOTIONMACRO_ZOOMOUT_CENTERTOCENTER = 44,
    V3DCMD_MOTIONMACRO_ZOOMIN_CENTERTOCENTER = 45,
    V3DCMD_MOTIONMACRO_ZOOMOUT_CURSORTOCURSOR = 46,
    V3DCMD_MOTIONMACRO_ZOOMIN_CURSORTOCURSOR = 47,
    V3DCMD_VIEW_QZ_IN = 48,
    V3DCMD_VIEW_QZ_OUT = 49,
    V3DCMD_KEY_ENTER = 50,
    V3DCMD_KEY_DELETE = 51,
    V3DCMD_KEY_F13 = 52,
    V3DCMD_KEY_F14 = 53,
    V3DCMD_KEY_F15 = 54,
    V3DCMD_KEY_F16 = 55,
    V3DCMD_KEY_F17 = 56,
    V3DCMD_KEY_F18 = 57,
    V3DCMD_KEY_F19 = 58,
    V3DCMD_KEY_F20 = 59,
    V3DCMD_KEY_F21 = 60,
    V3DCMD_KEY_F22 = 61,
    V3DCMD_KEY_F23 = 62,
    V3DCMD_KEY_F24 = 63,
    V3DCMD_KEY_F25 = 64,
    V3DCMD_KEY_F26 = 65,
    V3DCMD_KEY_F27 = 66,
    V3DCMD_KEY_F28 = 67,
    V3DCMD_KEY_F29 = 68,
    V3DCMD_KEY_F30 = 69,
    V3DCMD_KEY_F31 = 70,
    V3DCMD_KEY_F32 = 71,
    V3DCMD_KEY_F33 = 72,
    V3DCMD_KEY_F34 = 73,
    V3DCMD_KEY_F35 = 74,
    V3DCMD_KEY_F36 = 75,
    V3DCMD_VIEW_1 = 76,
    V3DCMD_VIEW_2 = 77,
    V3DCMD_VIEW_3 = 78,
    V3DCMD_VIEW_4 = 79,
    V3DCMD_VIEW_5 = 80,
    V3DCMD_VIEW_6 = 81,
    V3DCMD_VIEW_7 = 82,
    V3DCMD_VIEW_8 = 83,
    V3DCMD_VIEW_9 = 84,
    V3DCMD_VIEW_10 = 85,
    V3DCMD_VIEW_11 = 86,
    V3DCMD_VIEW_12 = 87,
    V3DCMD_VIEW_13 = 88,
    V3DCMD_VIEW_14 = 89,
    V3DCMD_VIEW_15 = 90,
    V3DCMD_VIEW_16 = 91,
    V3DCMD_VIEW_17 = 92,
    V3DCMD_VIEW_18 = 93,
    V3DCMD_VIEW_19 = 94,
    V3DCMD_VIEW_20 = 95,
    V3DCMD_VIEW_21 = 96,
    V3DCMD_VIEW_22 = 97,
    V3DCMD_VIEW_23 = 98,
    V3DCMD_VIEW_24 = 99,
    V3DCMD_VIEW_25 = 100,
    V3DCMD_VIEW_26 = 101,
    V3DCMD_VIEW_27 = 102,
    V3DCMD_VIEW_28 = 103,
    V3DCMD_VIEW_29 = 104,
    V3DCMD_VIEW_30 = 105,
    V3DCMD_VIEW_31 = 106,
    V3DCMD_VIEW_32 = 107,
    V3DCMD_VIEW_33 = 108,
    V3DCMD_VIEW_34 = 109,
    V3DCMD_VIEW_35 = 110,
    V3DCMD_VIEW_36 = 111,
    V3DCMD_SAVE_VIEW_1 = 112,
    V3DCMD_SAVE_VIEW_2 = 113,
    V3DCMD_SAVE_VIEW_3 = 114,
    V3DCMD_SAVE_VIEW_4 = 115,
    V3DCMD_SAVE_VIEW_5 = 116,
    V3DCMD_SAVE_VIEW_6 = 117,
    V3DCMD_SAVE_VIEW_7 = 118,
    V3DCMD_SAVE_VIEW_8 = 119,
    V3DCMD_SAVE_VIEW_9 = 120,
    V3DCMD_SAVE_VIEW_10 = 121,
    V3DCMD_SAVE_VIEW_11 = 122,
    V3DCMD_SAVE_VIEW_12 = 123,
    V3DCMD_SAVE_VIEW_13 = 124,
    V3DCMD_SAVE_VIEW_14 = 125,
    V3DCMD_SAVE_VIEW_15 = 126,
    V3DCMD_SAVE_VIEW_16 = 127,
    V3DCMD_SAVE_VIEW_17 = 128,
    V3DCMD_SAVE_VIEW_18 = 129,
    V3DCMD_SAVE_VIEW_19 = 130,
    V3DCMD_SAVE_VIEW_20 = 131,
    V3DCMD_SAVE_VIEW_21 = 132,
    V3DCMD_SAVE_VIEW_22 = 133,
    V3DCMD_SAVE_VIEW_23 = 134,
    V3DCMD_SAVE_VIEW_24 = 135,
    V3DCMD_SAVE_VIEW_25 = 136,
    V3DCMD_SAVE_VIEW_26 = 137,
    V3DCMD_SAVE_VIEW_27 = 138,
    V3DCMD_SAVE_VIEW_28 = 139,
    V3DCMD_SAVE_VIEW_29 = 140,
    V3DCMD_SAVE_VIEW_30 = 141,
    V3DCMD_SAVE_VIEW_31 = 142,
    V3DCMD_SAVE_VIEW_32 = 143,
    V3DCMD_SAVE_VIEW_33 = 144,
    V3DCMD_SAVE_VIEW_34 = 145,
    V3DCMD_SAVE_VIEW_35 = 146,
    V3DCMD_SAVE_VIEW_36 = 147,
    V3DCMD_KEY_TAB = 148,
    V3DCMD_KEY_SPACE = 149,
    V3DCMD_MENU_1 = 150,
    V3DCMD_MENU_2 = 151,
    V3DCMD_MENU_3 = 152,
    V3DCMD_MENU_4 = 153,
    V3DCMD_MENU_5 = 154,
    V3DCMD_MENU_6 = 155,
    V3DCMD_MENU_7 = 156,
    V3DCMD_MENU_8 = 157,
    V3DCMD_MENU_9 = 158,
    V3DCMD_MENU_10 = 159,
    V3DCMD_MENU_11 = 160,
    V3DCMD_MENU_12 = 161,
    V3DCMD_MENU_13 = 162,
    V3DCMD_MENU_14 = 163,
    V3DCMD_MENU_15 = 164,
    V3DCMD_MENU_16 = 165,
    /* Add here as needed. Don't change any values that may be in use */
} V3DCMD;

#endif /* _V3DCMD_H_ */
