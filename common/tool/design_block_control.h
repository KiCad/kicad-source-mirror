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


#ifndef DESIGN_BLOCK_CONTROL_H
#define DESIGN_BLOCK_CONTROL_H

#include <eda_draw_frame.h>
#include <tool/tool_interactive.h>

class DESIGN_BLOCK_PANE;
class LIB_TREE_NODE;

/**
 * Handle schematic design block actions in the schematic editor.
 */
class DESIGN_BLOCK_CONTROL : public TOOL_INTERACTIVE, public wxEvtHandler
{
public:
    DESIGN_BLOCK_CONTROL( const std::string& aName );
    virtual ~DESIGN_BLOCK_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    void AddContextMenuItems( CONDITIONAL_MENU* aMenu );

    int PinLibrary( const TOOL_EVENT& aEvent );
    int UnpinLibrary( const TOOL_EVENT& aEvent );

    int NewLibrary( const TOOL_EVENT& aEvent );
    int DeleteLibrary( const TOOL_EVENT& aEvent );

    int DeleteDesignBlock( const TOOL_EVENT& aEvent );
    int EditDesignBlockProperties( const TOOL_EVENT& aEvent );

    int HideLibraryTree( const TOOL_EVENT& aEvent );

protected:
    bool selIsInLibrary( const SELECTION& aSel );
    bool selIsDesignBlock( const SELECTION& aSel );

    LIB_ID getSelectedLibId();
    ///< Set up handlers for various events.
    void setTransitions() override;

    virtual DESIGN_BLOCK_PANE* getDesignBlockPane() = 0;
    LIB_TREE_NODE*             getCurrentTreeNode();

    /// Notify other frames that the design block lib table has changed
    std::vector<FRAME_T> m_framesToNotify;
    void                 notifyOtherFrames();

    EDA_DRAW_FRAME* m_frame = nullptr;
};


#endif
