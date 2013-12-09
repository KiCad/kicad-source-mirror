/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <tool/tool_action.h>

//class ACTION_MANAGER;

/**
 * Class COMMON_ACTIONS
 *
 * Gathers all the actions that are shared by tools. The instance of COMMON_ACTIOSN is created
 * inside of ACTION_MANAGER object and registers them.
 */
class COMMON_ACTIONS
{
public:
    /// Activation of the selection tool
    static TOOL_ACTION selectionActivate;

    /// Activation of the edit tool
    static TOOL_ACTION editActivate;

    /// Rotation of selected objects
    static TOOL_ACTION rotate;

    /// Flipping of selected objects
    static TOOL_ACTION flip;

    /// Activation of the edit tool
    static TOOL_ACTION properties;

    /// Deleting a BOARD_ITEM
    static TOOL_ACTION remove;
};
