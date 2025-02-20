/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 <author>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * Command IDs for the 3D viewer.
 *
 * Please add IDs that are unique to the 3D viewer here and not in the global
 * id.h file.  This will prevent the entire project from being rebuilt when
 * adding new commands to the 3D viewer.
 * However the number of IDs should be < ROOM_FOR_3D_VIEWER, defined in id.h
 * Please change the value of ROOM_FOR_3D_VIEWER if too small.
 */
#pragma once

#include <id.h>        // Generic Id.

enum id_3dview_frm
{
    ID_START_COMMAND_3D = ID_KICAD_3D_VIEWER_START,

    ID_MENU3D_RESET_DEFAULTS,

    // Help
    ID_MENU_COMMAND_END,

    ID_DISABLE_RAY_TRACING,

    ID_CUSTOM_EVENT_1,      // A id for a custom event (canvas refresh request)

    ID_END_COMMAND_3D = ID_KICAD_3D_VIEWER_END,
};
