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
 * Page layout editor IDs.
 */

enum pl_editor_ids
{
    ID_MAIN_MENUBAR = ID_END_LIST,

    ID_SHOW_REAL_MODE,
    ID_SHOW_PL_EDITOR_MODE,
    ID_SELECT_COORDINATE_ORIGIN,
    ID_SELECT_PAGE_NUMBER,

    ID_APPEND_DESCR_FILE,

    ID_PLEDITOR_END_LIST
};

#endif  /* _PL_EDITOR_IDS_H_ */
