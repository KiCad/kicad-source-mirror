/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * @file cvpcb_id.h
 */
/*
 * Command IDs for CvPcb.
 *
 * Please add IDs that are unique to the component library viewer here and
 * not in the global id.h file.  This will prevent the entire project from
 * being rebuilt when adding new commands to the component library viewer.
 */

// Generic IDs:
#include <id.h>

// specific IDs
enum id_cvpcb_frm
{
    ID_CVPCB_COMPONENT_LIST = ID_END_LIST,
    ID_CVPCB_FOOTPRINT_LIST,
    ID_CVPCB_LIBRARY_LIST,
    ID_CVPCB_FILTER_TEXT_EDIT,
};
