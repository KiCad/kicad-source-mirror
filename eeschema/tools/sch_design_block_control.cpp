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
#include <tool/library_editor_control.h>
#include <sch_design_block_control.h>
#include <design_block_pane.h>
#include <panel_design_block_chooser.h>
#include <dialog_design_block_properties.h>
#include <ee_actions.h>

bool SCH_DESIGN_BLOCK_CONTROL::Init()
{
    m_editFrame     = getEditFrame<SCH_EDIT_FRAME>();
    m_frame         = m_editFrame;
    m_selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();

    auto pinnedLib =
            [this]( const SELECTION& aSel )
            {
                //
                LIB_TREE_NODE* current = getCurrentTreeNode();
                return current && current->m_Type == LIB_TREE_NODE::TYPE::LIBRARY
                       && current->m_Pinned;
            };
    auto unpinnedLib =
            [this](const SELECTION& aSel )
            {
                LIB_TREE_NODE* current = getCurrentTreeNode();
                return current && current->m_Type == LIB_TREE_NODE::TYPE::LIBRARY
                       && !current->m_Pinned;
            };

    auto isInLibrary =
            [this](const SELECTION& aSel )
            {
                LIB_TREE_NODE* current = getCurrentTreeNode();
                return current
                       && ( current->m_Type == LIB_TREE_NODE::TYPE::LIBRARY
                            || current->m_Type == LIB_TREE_NODE::TYPE::ITEM );
            };

    auto isDesignBlock =
            [this](const SELECTION& aSel )
            {
                LIB_TREE_NODE* current = getCurrentTreeNode();
                return current && current->m_Type == LIB_TREE_NODE::TYPE::ITEM;
            };

    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();

    ctxMenu.AddItem( ACTIONS::pinLibrary,                    unpinnedLib, 1 );
    ctxMenu.AddItem( ACTIONS::unpinLibrary,                  pinnedLib, 1 );
    ctxMenu.AddItem( ACTIONS::newLibrary,                    !isDesignBlock, 1 );
    ctxMenu.AddSeparator( 1 );

    ctxMenu.AddItem( EE_ACTIONS::placeDesignBlock,           isDesignBlock, 50 );
    ctxMenu.AddSeparator( 50 );

    ctxMenu.AddItem( EE_ACTIONS::editDesignBlockProperties,  isDesignBlock, 100 );
    ctxMenu.AddItem( EE_ACTIONS::saveSheetAsDesignBlock,     isInLibrary, 100 );
    ctxMenu.AddItem( EE_ACTIONS::saveSelectionAsDesignBlock, isInLibrary, 100 );
    ctxMenu.AddItem( EE_ACTIONS::deleteDesignBlock,          isDesignBlock, 100 );
    ctxMenu.AddSeparator( 100 );

    ctxMenu.AddItem( ACTIONS::hideLibraryTree,               SELECTION_CONDITIONS::ShowAlways, 400 );

    return true;
}


int SCH_DESIGN_BLOCK_CONTROL::PinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( current && !current->m_Pinned )
    {
        m_frame->Prj().PinLibrary( current->m_LibId.GetLibNickname(),
                                   PROJECT::LIB_TYPE_T::DESIGN_BLOCK_LIB );
        current->m_Pinned = true;
        getDesignBlockPane()->RefreshLibs();
    }

    return 0;
}


int SCH_DESIGN_BLOCK_CONTROL::UnpinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( current && current->m_Pinned )
    {
        m_frame->Prj().UnpinLibrary( current->m_LibId.GetLibNickname(),
                                     PROJECT::LIB_TYPE_T::DESIGN_BLOCK_LIB );
        current->m_Pinned = false;
        getDesignBlockPane()->RefreshLibs();
    }

    return 0;
}


int SCH_DESIGN_BLOCK_CONTROL::NewLibrary( const TOOL_EVENT& aEvent )
{
    m_editFrame->CreateNewDesignBlockLibrary();
    return 0;
}


int SCH_DESIGN_BLOCK_CONTROL::SaveSheetAsDesignBlock( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    m_editFrame->SaveSheetAsDesignBlock( current->m_LibId.GetLibNickname(),
                                         m_editFrame->GetCurrentSheet() );

    return 0;
}


int SCH_DESIGN_BLOCK_CONTROL::SaveSelectionAsDesignBlock( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    m_editFrame->SaveSelectionAsDesignBlock( current->m_LibId.GetLibNickname() );

    return 0;
}


int SCH_DESIGN_BLOCK_CONTROL::DeleteDesignBlock( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    m_editFrame->DeleteDesignBlockFromLibrary( current->m_LibId, true );

    return 0;
}


int SCH_DESIGN_BLOCK_CONTROL::EditDesignBlockProperties( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    if( m_editFrame->EditDesignBlockProperties( current->m_LibId ) )
        return 0;

    return -1;
}


int SCH_DESIGN_BLOCK_CONTROL::HideLibraryTree( const TOOL_EVENT& aEvent )
{
    m_editFrame->ToggleLibraryTree();
    return 0;
}


void SCH_DESIGN_BLOCK_CONTROL::setTransitions()
{
    Go( &SCH_DESIGN_BLOCK_CONTROL::PinLibrary,                  ACTIONS::pinLibrary.MakeEvent() );
    Go( &SCH_DESIGN_BLOCK_CONTROL::UnpinLibrary,                ACTIONS::unpinLibrary.MakeEvent() );

    Go( &SCH_DESIGN_BLOCK_CONTROL::NewLibrary,                  ACTIONS::newLibrary.MakeEvent() );

    Go( &SCH_DESIGN_BLOCK_CONTROL::SaveSheetAsDesignBlock,      EE_ACTIONS::saveSheetAsDesignBlock.MakeEvent() );
    Go( &SCH_DESIGN_BLOCK_CONTROL::SaveSelectionAsDesignBlock,  EE_ACTIONS::saveSelectionAsDesignBlock.MakeEvent() );
    Go( &SCH_DESIGN_BLOCK_CONTROL::DeleteDesignBlock,           EE_ACTIONS::deleteDesignBlock.MakeEvent() );
    Go( &SCH_DESIGN_BLOCK_CONTROL::EditDesignBlockProperties,   EE_ACTIONS::editDesignBlockProperties.MakeEvent() );

    Go( &SCH_DESIGN_BLOCK_CONTROL::HideLibraryTree,             ACTIONS::hideLibraryTree.MakeEvent() );
}


LIB_ID SCH_DESIGN_BLOCK_CONTROL::getSelectedLibId()
{
    m_editFrame->GetDesignBlockPane()->GetSelectedLibId();

    return LIB_ID();
}


DESIGN_BLOCK_PANE* SCH_DESIGN_BLOCK_CONTROL::getDesignBlockPane()
{
    return m_editFrame->GetDesignBlockPane();
}


LIB_TREE_NODE* SCH_DESIGN_BLOCK_CONTROL::getCurrentTreeNode()
{
    LIB_TREE* libTree = getDesignBlockPane()->GetDesignBlockPanel()->GetLibTree();
    return libTree ? libTree->GetCurrentTreeNode() : nullptr;
}
