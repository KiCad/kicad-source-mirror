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
#include <dialog_design_block_properties.h>
#include <pcb_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_design_block_control.h>
#include <widgets/pcb_design_block_pane.h>
#include <widgets/panel_design_block_chooser.h>


PCB_DESIGN_BLOCK_CONTROL::~PCB_DESIGN_BLOCK_CONTROL()
{
}


bool PCB_DESIGN_BLOCK_CONTROL::Init()
{
    m_editFrame     = getEditFrame<PCB_EDIT_FRAME>();
    m_frame         = m_editFrame;
    m_framesToNotify = { FRAME_SCH };

    auto isInLibrary =
            [this](const SELECTION& aSel )
            {
                return this->selIsInLibrary(aSel);
            };

    auto isDesignBlock =
            [this](const SELECTION& aSel )
            {
                return this->selIsDesignBlock(aSel);
            };

    auto hasSelection =
            [this](const SELECTION& aSel )
            {
                return !m_editFrame->GetCurrentSelection().Empty();
            };

    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();
    AddContextMenuItems( &ctxMenu );

    ctxMenu.AddItem( PCB_ACTIONS::placeDesignBlock, isDesignBlock, 50 );
    ctxMenu.AddSeparator( 50 );

    ctxMenu.AddItem( PCB_ACTIONS::editDesignBlockProperties,  isDesignBlock, 100 );
    ctxMenu.AddItem( PCB_ACTIONS::saveBoardAsDesignBlock,     isInLibrary, 100 );
    ctxMenu.AddItem( PCB_ACTIONS::saveSelectionAsDesignBlock, isInLibrary && hasSelection, 100 );
    ctxMenu.AddItem( PCB_ACTIONS::updateDesignBlockFromBoard,     isDesignBlock, 100 );
    ctxMenu.AddItem( PCB_ACTIONS::updateDesignBlockFromSelection, isDesignBlock && hasSelection, 100 );
    ctxMenu.AddItem( PCB_ACTIONS::deleteDesignBlock,          isDesignBlock, 100 );
    ctxMenu.AddSeparator( 100 );

    return true;
}


int PCB_DESIGN_BLOCK_CONTROL::SaveBoardAsDesignBlock( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    // This can be modified as a result of the save operation so copy it
    LIB_ID libId = current->m_LibId;

    if( !m_editFrame->SaveBoardAsDesignBlock( libId.GetLibNickname() ) )
        return -1;

    notifyOtherFrames();

    return 0;
}


int PCB_DESIGN_BLOCK_CONTROL::SaveSelectionAsDesignBlock( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    // This can be modified as a result of the save operation so copy it
    LIB_ID libId = current->m_LibId;

    if( !m_editFrame->SaveSelectionAsDesignBlock( libId.GetLibNickname() ) )
        return -1;

    notifyOtherFrames();

    return 0;
}


int PCB_DESIGN_BLOCK_CONTROL::UpdateDesignBlockFromBoard( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    // This can be modified as a result of the save operation so copy it
    LIB_ID libId = current->m_LibId;

    if( !m_editFrame->UpdateDesignBlockFromBoard( libId ) )
        return -1;

    notifyOtherFrames();

    return 0;
}


int PCB_DESIGN_BLOCK_CONTROL::UpdateDesignBlockFromSelection( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    // This can be modified as a result of the save operation so copy it
    LIB_ID libId = current->m_LibId;

    if( !m_editFrame->UpdateDesignBlockFromSelection( libId ) )
        return -1;

    notifyOtherFrames();

    return 0;
}

void PCB_DESIGN_BLOCK_CONTROL::setTransitions()
{
    DESIGN_BLOCK_CONTROL::setTransitions();

    Go( &PCB_DESIGN_BLOCK_CONTROL::SaveBoardAsDesignBlock,      PCB_ACTIONS::saveBoardAsDesignBlock.MakeEvent() );
    Go( &PCB_DESIGN_BLOCK_CONTROL::SaveSelectionAsDesignBlock,  PCB_ACTIONS::saveSelectionAsDesignBlock.MakeEvent() );
    Go( &PCB_DESIGN_BLOCK_CONTROL::UpdateDesignBlockFromBoard,      PCB_ACTIONS::updateDesignBlockFromBoard.MakeEvent() );
    Go( &PCB_DESIGN_BLOCK_CONTROL::UpdateDesignBlockFromSelection,  PCB_ACTIONS::updateDesignBlockFromSelection.MakeEvent() );
    Go( &PCB_DESIGN_BLOCK_CONTROL::DeleteDesignBlock,           PCB_ACTIONS::deleteDesignBlock.MakeEvent() );
    Go( &PCB_DESIGN_BLOCK_CONTROL::EditDesignBlockProperties,   PCB_ACTIONS::editDesignBlockProperties.MakeEvent() );
}


LIB_ID PCB_DESIGN_BLOCK_CONTROL::getSelectedLibId()
{
    getDesignBlockPane()->GetSelectedLibId();

    return LIB_ID();
}


DESIGN_BLOCK_PANE* PCB_DESIGN_BLOCK_CONTROL::getDesignBlockPane()
{
    return m_editFrame->GetDesignBlockPane();
}
