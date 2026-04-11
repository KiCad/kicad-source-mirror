/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef SCH_DESIGN_BLOCK_CONTROL_H
#define SCH_DESIGN_BLOCK_CONTROL_H

#include <tool/design_block_control.h>

class DESIGN_BLOCK_PANE;
class SCH_EDIT_FRAME;

/**
 * Handle design block actions in the schematic editor.
 */
class SCH_DESIGN_BLOCK_CONTROL : public DESIGN_BLOCK_CONTROL
{
public:
    SCH_DESIGN_BLOCK_CONTROL() : DESIGN_BLOCK_CONTROL( "eeschema.SchDesignBlockControl" ) {}
    virtual ~SCH_DESIGN_BLOCK_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int SaveSheetAsDesignBlock( const TOOL_EVENT& aEvent );
    int SaveSelectionAsDesignBlock( const TOOL_EVENT& aEvent );

    int UpdateDesignBlockFromSheet( const TOOL_EVENT& aEvent );
    int UpdateDesignBlockFromSelection( const TOOL_EVENT& aEvent );

private:
    LIB_ID getSelectedLibId();
    ///< Set up handlers for various events.
    void setTransitions() override;

    DESIGN_BLOCK_PANE* getDesignBlockPane() override;

    SCH_EDIT_FRAME*    m_editFrame = nullptr;
};


#endif
