/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * Please add IDs that are unique to the gerber file viewer (GerbView) here and not in
 * the global id.h file.  This will prevent the entire project from being rebuilt when
 * adding new commands to the GerbView.
 */

enum gerbview_ids
{
    ID_TOOLBARH_GERBER_SELECT_ACTIVE_DCODE = ID_END_LIST,
    ID_TOOLBARH_GERBER_DATA_TEXT_BOX,

    ID_GBR_AUX_TOOLBAR_PCB_CMP_CHOICE,
    ID_GBR_AUX_TOOLBAR_PCB_NET_CHOICE,
    ID_GBR_AUX_TOOLBAR_PCB_APERATTRIBUTES_CHOICE,

    ID_TOOLBARH_GERBVIEW_SELECT_ACTIVE_LAYER,
    ID_TB_OPTIONS_SHOW_GBR_MODE_0,
    ID_TB_OPTIONS_SHOW_GBR_MODE_1,
    ID_TB_OPTIONS_SHOW_GBR_MODE_2,

    // IDs for drill file history (ID_FILEnn is already in use)
    ID_GERBVIEW_DRILL_FILE,
    ID_GERBVIEW_DRILL_FILE1,
    ID_GERBVIEW_DRILL_FILEMAX = ID_GERBVIEW_DRILL_FILE + MAX_FILE_HISTORY_SIZE,
    ID_GERBVIEW_DRILL_FILE_LIST_CLEAR,

    // IDs for job file history (ID_FILEnn is already in use)
    ID_GERBVIEW_JOB_FILE,
    ID_GERBVIEW_JOB_FILE1,
    ID_GERBVIEW_JOB_FILEMAX = ID_GERBVIEW_JOB_FILE + MAX_FILE_HISTORY_SIZE,
    ID_GERBVIEW_JOB_FILE_LIST_CLEAR,

    // IDs for zip file history (ID_FILEnn is already in use)
    ID_GERBVIEW_ZIP_FILE,
    ID_GERBVIEW_ZIP_FILE1,
    ID_GERBVIEW_ZIP_FILEMAX = ID_GERBVIEW_ZIP_FILE + MAX_FILE_HISTORY_SIZE,
    ID_GERBVIEW_ZIP_FILE_LIST_CLEAR,

    ID_GERBER_END_LIST
};

#endif  /* __GERBVIEW_IDS_H__  */
