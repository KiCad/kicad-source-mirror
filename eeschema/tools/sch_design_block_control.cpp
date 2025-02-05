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
#include <ee_actions.h>
#include <ee_selection_tool.h>
#include <sch_design_block_control.h>
#include <sch_edit_frame.h>
#include <tool/tool_manager.h>
#include <widgets/sch_design_block_pane.h>
#include <widgets/panel_design_block_chooser.h>


SCH_DESIGN_BLOCK_CONTROL::~SCH_DESIGN_BLOCK_CONTROL()
{
}


bool SCH_DESIGN_BLOCK_CONTROL::Init()
{
    m_editFrame     = getEditFrame<SCH_EDIT_FRAME>();
    m_frame         = m_editFrame;

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

    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();
    AddContextMenuItems( &ctxMenu );

    ctxMenu.AddItem( EE_ACTIONS::placeDesignBlock,           isDesignBlock, 50 );
    ctxMenu.AddSeparator( 50 );

    ctxMenu.AddItem( EE_ACTIONS::editDesignBlockProperties,  isDesignBlock, 100 );
    ctxMenu.AddItem( EE_ACTIONS::saveSheetAsDesignBlock,     isInLibrary, 100 );
    ctxMenu.AddItem( EE_ACTIONS::saveSelectionAsDesignBlock, isInLibrary, 100 );
    ctxMenu.AddItem( EE_ACTIONS::deleteDesignBlock,          isDesignBlock, 100 );
    ctxMenu.AddSeparator( 100 );

    return true;
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


void SCH_DESIGN_BLOCK_CONTROL::setTransitions()
{
    DESIGN_BLOCK_CONTROL::setTransitions();

    Go( &SCH_DESIGN_BLOCK_CONTROL::SaveSheetAsDesignBlock,      EE_ACTIONS::saveSheetAsDesignBlock.MakeEvent() );
    Go( &SCH_DESIGN_BLOCK_CONTROL::SaveSelectionAsDesignBlock,  EE_ACTIONS::saveSelectionAsDesignBlock.MakeEvent() );
    Go( &SCH_DESIGN_BLOCK_CONTROL::DeleteDesignBlock,           EE_ACTIONS::deleteDesignBlock.MakeEvent() );
    Go( &SCH_DESIGN_BLOCK_CONTROL::EditDesignBlockProperties,   EE_ACTIONS::editDesignBlockProperties.MakeEvent() );
}


DESIGN_BLOCK_PANE* SCH_DESIGN_BLOCK_CONTROL::getDesignBlockPane()
{
    return m_editFrame->GetDesignBlockPane();
}
