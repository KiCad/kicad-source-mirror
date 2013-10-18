/**
 * @file pl_editor_id.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef _PL_EDITOR_ID_H_
#define _PL_EDITOR_ID_H_

#include <id.h>

/**
 * Command IDs for the printed circuit board editor.
 *
 * Please add IDs that are unique to the page layout editor (pl_editor) here and not in
 * the global id.h file.  This will prevent the entire project from being rebuilt when
 * adding new commands to the page layout editor.
 */

enum pl_editor_ids
{
    ID_MAIN_MENUBAR = ID_END_LIST,

    ID_PL_EDITOR_SHOW_SOURCE,
    ID_MENU_PL_EDITOR_SELECT_PREFERED_EDITOR,
    ID_MENU_SWITCH_BGCOLOR,
    ID_MENU_GRID_ONOFF,
    ID_DESIGN_TREE_FRAME,

    ID_SHOW_REAL_MODE,
    ID_SHOW_PL_EDITOR_MODE,
    ID_SELECT_COORDINATE_ORIGIN,
    ID_LOAD_DEFAULT_PAGE_LAYOUT,
    ID_SELECT_PAGE_NUMBER,

    ID_APPEND_DESCR_FILE,

    ID_POPUP_START_RANGE,
    ID_POPUP_ITEM_DELETE,
    ID_POPUP_DESIGN_TREE_ITEM_DELETE,
    ID_POPUP_ITEM_ADD_LINE,
    ID_POPUP_ITEM_ADD_RECT,
    ID_POPUP_ITEM_ADD_TEXT,
    ID_POPUP_ITEM_ADD_BITMAP,
    ID_POPUP_ITEM_APPEND_PAGE_LAYOUT,
    ID_POPUP_ITEM_MOVE,
    ID_POPUP_ITEM_PLACE,
    ID_POPUP_ITEM_MOVE_START_POINT,
    ID_POPUP_ITEM_MOVE_END_POINT,
    ID_POPUP_ITEM_PLACE_CANCEL,
    ID_POPUP_ITEM_EDIT_BITMAP,
    ID_POPUP_END_RANGE,

    ID_PLEDITOR_END_LIST
};

#endif  /* _PL_EDITOR_IDS_H_ */
