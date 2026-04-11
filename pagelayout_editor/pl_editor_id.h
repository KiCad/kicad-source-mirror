/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _PL_EDITOR_ID_H_
#define _PL_EDITOR_ID_H_

#include <id.h>

/**
 * Drawing sheet editor IDs.
 */

enum pl_editor_ids
{
    ID_SELECT_COORDINATE_ORIGIN = ID_END_LIST,
    ID_SELECT_PAGE_NUMBER,

    ID_APPEND_DESCR_FILE,

    ID_PLEDITOR_END_LIST
};

#endif  /* _PL_EDITOR_IDS_H_ */
