/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "sch_edit_tool.h"
#include <sch_actions.h>
#include <hotkeys.h>
#include <bitmaps.h>


TOOL_ACTION SCH_ACTIONS::editActivate( "eeschema.InteractiveEdit",
        AS_GLOBAL, 0,
        _( "Edit Activate" ), "", move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::move( "eeschema.InteractiveEdit.move",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_MOVE_COMPONENT_OR_ITEM ),
        _( "Move" ), _( "Moves the selected item(s)" ), move_xpm, AF_ACTIVATE );

TOOL_ACTION SCH_ACTIONS::duplicate( "eeschema.InteractiveEdit.duplicate",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_DUPLICATE_ITEM ),
        _( "Duplicate" ), _( "Duplicates the selected item(s)" ), duplicate_xpm );

TOOL_ACTION SCH_ACTIONS::rotate( "eeschema.InteractiveEdit.rotate",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ROTATE ),
        _( "Rotate" ), _( "Rotates selected item(s)" ),
        rotate_ccw_xpm, AF_NONE );

TOOL_ACTION SCH_ACTIONS::properties( "eeschema.InteractiveEdit.properties",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_EDIT ),
        _( "Properties..." ), _( "Displays item properties dialog" ), config_xpm );

