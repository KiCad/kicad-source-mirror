/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2008-2021 KiCad Developers, see change_log.txt for contributors.
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

#ifndef __EESCHEMA_ID_H__
#define __EESCHEMA_ID_H__

#include <id.h>


/**
 * The maximum number of items in the clarify selection context menu.  While in
 * most cases it is highly unlikely that there would ever be more than 10 items
 * at the current cursor, there are some exceptions  (a bunch of pins created at
 * once, for instance).  The current setting of 200 is arbitrary.
 */
#define MAX_SELECT_ITEM_IDS 200

/**
 * The maximum number of units per package.
 * While counts approaching 100 start to make the unit-selection popup menu
 * difficult to use, the limit is currently 'ZZ' (26 * 26).
 */
#define MAX_UNIT_COUNT_PER_PACKAGE 676


/**
 * Command IDs for the schematic editor.
 *
 * Please add IDs that are unique to the schematic editor (Eeschema) here and
 * not in the global id.h file.  This will prevent the entire project from
 * being rebuilt when adding new command to Eeschema.
 */

enum id_eeschema_frm
{
    ID_IMPORT_NON_KICAD_SCH = ID_END_LIST,

    /* Library editor horizontal toolbar IDs. */
    ID_LIBEDIT_SELECT_UNIT_NUMBER,

    /* Library viewer horizontal toolbar IDs */
    ID_LIBVIEW_SELECT_PART,
    ID_LIBVIEW_NEXT,
    ID_LIBVIEW_PREVIOUS,
    ID_LIBVIEW_SELECT_UNIT_NUMBER,
    ID_LIBVIEW_LIB_LIST,
    ID_LIBVIEW_SYM_LIST,

    ID_SIM_RUN,
    ID_SIM_TUNE,
    ID_SIM_PROBE,
    ID_SIM_ADD_SIGNALS,

    ID_END_EESCHEMA_ID_LIST,    // End of IDs specific to Eeschema

    // These ID are used in context menus,
    // and must not clash with any other menu ID inside Kicad
    // So used ID inside the reserved popup ID
    //
    // Dynamically bound in AddMenusForBus():
    ID_POPUP_SCH_UNFOLD_BUS = ID_POPUP_MENU_START,
    ID_POPUP_SCH_UNFOLD_BUS_END = ID_POPUP_SCH_UNFOLD_BUS + 128,

    // Unit select context menus command IDs.
    ID_POPUP_SCH_SELECT_UNIT_CMP,
    ID_POPUP_SCH_SELECT_UNIT1,
    // ... leave room for MAX_UNIT_COUNT_PER_PACKAGE IDs ,
    // to select one unit among MAX_UNIT_COUNT_PER_PACKAGE in popup menu
    ID_POPUP_SCH_SELECT_UNIT_SYM_MAX = ID_POPUP_SCH_SELECT_UNIT1 + MAX_UNIT_COUNT_PER_PACKAGE
};


#endif  /* __EESCHEMA_ID_H__ */
