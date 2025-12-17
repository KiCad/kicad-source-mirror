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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef PCB_DESIGN_BLOCK_CONTROL_H
#define PCB_DESIGN_BLOCK_CONTROL_H

#include <tool/design_block_control.h>

class DESIGN_BLOCK_PANE;
class PCB_EDIT_FRAME;

/**
 * Handle design block actions in the PCB editor.
 */
class PCB_DESIGN_BLOCK_CONTROL : public DESIGN_BLOCK_CONTROL
{
public:
    PCB_DESIGN_BLOCK_CONTROL() : DESIGN_BLOCK_CONTROL( "pcbnew.PcbDesignBlockControl" ) {}
    virtual ~PCB_DESIGN_BLOCK_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int SaveBoardAsDesignBlock( const TOOL_EVENT& aEvent );
    int SaveSelectionAsDesignBlock( const TOOL_EVENT& aEvent );

    int UpdateDesignBlockFromBoard( const TOOL_EVENT& aEvent );
    int UpdateDesignBlockFromSelection( const TOOL_EVENT& aEvent );

private:
    LIB_ID getSelectedLibId();
    ///< Set up handlers for various events.
    void setTransitions() override;

    DESIGN_BLOCK_PANE* getDesignBlockPane() override;

    PCB_EDIT_FRAME* m_editFrame = nullptr;
};


#endif
