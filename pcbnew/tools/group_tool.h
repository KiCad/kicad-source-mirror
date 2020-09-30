/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GROUP_TOOL_H
#define GROUP_TOOL_H

#include <math/vector2d.h>
#include <tools/pcb_tool_base.h>
#include "selection_tool.h"
#include "dialogs/dialog_group_properties_base.h"

class BOARD_COMMIT;
class BOARD_ITEM;
class SELECTION_TOOL;
class DIALOG_GROUP_PROPERTIES;

class GROUP_TOOL : public PCB_TOOL_BASE
{
public:
    GROUP_TOOL();

    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_BASE::Init()
    bool Init() override;

    int GroupProperties( const TOOL_EVENT& aEvent );

    /**
     * Function SelectNewMember()
     *
     * Invokes the picker tool to select a new member of the group.
     */
    int PickNewMember( const TOOL_EVENT& aEvent  );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    DIALOG_GROUP_PROPERTIES*      m_propertiesDialog;
    SELECTION_TOOL*               m_selectionTool;
    std::unique_ptr<BOARD_COMMIT> m_commit;
};

#endif
