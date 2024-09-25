/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2024 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef SCH_DESIGN_BLOCK_CONTROL_H
#define SCH_DESIGN_BLOCK_CONTROL_H

#include <sch_base_frame.h>
#include <tools/ee_tool_base.h>

/**
 * Handle schematic design block actions in the schematic editor.
 */
class SCH_DESIGN_BLOCK_CONTROL : public wxEvtHandler, public EE_TOOL_BASE<SCH_BASE_FRAME>
{
public:
    SCH_DESIGN_BLOCK_CONTROL() : EE_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.SchDesignBlockControl" ) {}

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int PinLibrary( const TOOL_EVENT& aEvent );
    int UnpinLibrary( const TOOL_EVENT& aEvent );

    int NewLibrary( const TOOL_EVENT& aEvent );
    int DeleteLibrary( const TOOL_EVENT& aEvent );

    int SaveSheetAsDesignBlock( const TOOL_EVENT& aEvent );
    int SaveSelectionAsDesignBlock( const TOOL_EVENT& aEvent );
    int DeleteDesignBlock( const TOOL_EVENT& aEvent );
    int EditDesignBlockProperties( const TOOL_EVENT& aEvent );

    int HideLibraryTree( const TOOL_EVENT& aEvent );

private:
    LIB_ID getSelectedLibId();
    ///< Set up handlers for various events.
    void setTransitions() override;

    DESIGN_BLOCK_PANE* getDesignBlockPane();
    LIB_TREE_NODE*     getCurrentTreeNode();

    SCH_EDIT_FRAME*    m_editFrame = nullptr;
};


#endif
