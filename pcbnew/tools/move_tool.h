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

#ifndef __MOVE_TOOL_H
#define __MOVE_TOOL_H

#include <math/vector2d.h>
#include <tool/tool_interactive.h>
#include <view/view_group.h>
#include "item_state.h"

class BOARD_ITEM;
class SELECTION_TOOL;

namespace KIGFX
{
class VIEW_GROUP;
}

/**
 * Class MOVE_TOOL
 *
 * Our sample move tool. Allows to move, rotate and flip items selected by
 * pcbnew.InteractiveSelection tool.
 */

class MOVE_TOOL : public TOOL_INTERACTIVE
{
public:
    MOVE_TOOL();
    ~MOVE_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init();

    /**
     * Function Main()
     *
     * Main loop in which events are handled.
     */
    int Main( TOOL_EVENT& aEvent );

private:
    /// Saves the state of items and allows to restore them
    ITEM_STATE m_state;

    /// Selection tool used for obtaining selected items
    SELECTION_TOOL* m_selectionTool;
};

#endif
