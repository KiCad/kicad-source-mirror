/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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

#ifndef __GERBVIEW_ID_H__
#define __GERBVIEW_ID_H__

#include <id.h>

/**
 * Command IDs for the printed circuit board editor.
 *
 * Please add IDs that are unique to the gerber file viewer (GerbView) here and not in
 * the global id.h file.  This will prevent the entire project from being rebuilt when
 * adding new commands to the GerbView.
 */

enum gerbview_ids
{
    ID_MAIN_MENUBAR = ID_END_LIST,

    ID_GERBVIEW_SHOW_LIST_DCODES,
    ID_GERBVIEW_LOAD_DRILL_FILE,
    ID_GERBVIEW_ERASE_ALL,
    ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE,
    ID_GERBVIEW_SHOW_SOURCE,
    ID_GERBVIEW_EXPORT_TO_PCBNEW,

    ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
    ID_MENU_GERBVIEW_SELECT_PREFERED_EDITOR,

    // IDs for drill file history (wxID_FILEnn is already in use)
    ID_GERBVIEW_DRILL_FILE,
    ID_GERBVIEW_DRILL_FILE1,
    ID_GERBVIEW_DRILL_FILE2,
    ID_GERBVIEW_DRILL_FILE3,
    ID_GERBVIEW_DRILL_FILE4,
    ID_GERBVIEW_DRILL_FILE5,
    ID_GERBVIEW_DRILL_FILE6,
    ID_GERBVIEW_DRILL_FILE7,
    ID_GERBVIEW_DRILL_FILE8,
    ID_GERBVIEW_DRILL_FILE9,

    ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
    ID_GERBVIEW_GLOBAL_DELETE,
    ID_GERBVIEW_OPTIONS_SETUP,
    ID_GERBVIEW_SET_PAGE_BORDER,
    ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
    ID_TB_OPTIONS_SHOW_DCODES,
    ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH,
    ID_TB_OPTIONS_SHOW_LINES_SKETCH,
    ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH,
    ID_TB_OPTIONS_SHOW_NEGATIVE_ITEMS,
    ID_TB_OPTIONS_SHOW_GBR_MODE_0,
    ID_TB_OPTIONS_SHOW_GBR_MODE_1,
    ID_TB_OPTIONS_SHOW_GBR_MODE_2,

    ID_GERBER_END_LIST
};

#endif  /* __GERBVIEW_IDS_H__  */
