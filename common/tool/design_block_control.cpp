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
#include <tool/actions.h>
#include <tool/library_editor_control.h>
#include <tool/design_block_control.h>
#include <widgets/design_block_pane.h>
#include <widgets/panel_design_block_chooser.h>
#include <dialog_design_block_properties.h>
#include <mail_type.h>
#include <kiway.h>


DESIGN_BLOCK_CONTROL::~DESIGN_BLOCK_CONTROL()
{
}

DESIGN_BLOCK_CONTROL::DESIGN_BLOCK_CONTROL( const std::string& aName ) : TOOL_INTERACTIVE( aName )
{
}


void DESIGN_BLOCK_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_DRAW_FRAME>();
}


void DESIGN_BLOCK_CONTROL::AddContextMenuItems( CONDITIONAL_MENU* aMenu )
{
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

    aMenu->AddItem( ACTIONS::pinLibrary, unpinnedLib, 1 );
    aMenu->AddItem( ACTIONS::unpinLibrary, pinnedLib, 1 );
    aMenu->AddItem( ACTIONS::newLibrary, SELECTION_CONDITIONS::ShowAlways, 1 );
    aMenu->AddSeparator( 2 );

    aMenu->AddSeparator( 400 );
    aMenu->AddItem( ACTIONS::hideLibraryTree, SELECTION_CONDITIONS::ShowAlways, 400 );
}


int DESIGN_BLOCK_CONTROL::PinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( current && !current->m_Pinned )
    {
        m_frame->Prj().PinLibrary( current->m_LibId.GetLibNickname(), PROJECT::LIB_TYPE_T::DESIGN_BLOCK_LIB );
        current->m_Pinned = true;
        getDesignBlockPane()->RefreshLibs();
        notifyOtherFrames();

        return 0;
    }

    return -1;
}


int DESIGN_BLOCK_CONTROL::UnpinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( current && current->m_Pinned )
    {
        m_frame->Prj().UnpinLibrary( current->m_LibId.GetLibNickname(), PROJECT::LIB_TYPE_T::DESIGN_BLOCK_LIB );
        current->m_Pinned = false;
        getDesignBlockPane()->RefreshLibs();
        notifyOtherFrames();

        return 0;
    }

    return -1;
}


int DESIGN_BLOCK_CONTROL::NewLibrary( const TOOL_EVENT& aEvent )
{
    if( !getDesignBlockPane()->CreateNewDesignBlockLibrary( _( "New Design Block Library" ) ).IsEmpty() )
    {
        notifyOtherFrames();
        return 0;
    }

    return -1;
}


int DESIGN_BLOCK_CONTROL::DeleteDesignBlock( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    if( getDesignBlockPane()->DeleteDesignBlockFromLibrary( current->m_LibId, true ) )
    {
        notifyOtherFrames();
        return 0;
    }

    return -1;
}


int DESIGN_BLOCK_CONTROL::EditDesignBlockProperties( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();

    if( !current )
        return -1;

    if( getDesignBlockPane()->EditDesignBlockProperties( current->m_LibId ) )
    {
        notifyOtherFrames();
        return 0;
    }

    return -1;
}


int DESIGN_BLOCK_CONTROL::HideLibraryTree( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleLibraryTree();
    return 0;
}


bool DESIGN_BLOCK_CONTROL::selIsInLibrary( const SELECTION& aSel )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();
    return current
           && ( current->m_Type == LIB_TREE_NODE::TYPE::LIBRARY
                || current->m_Type == LIB_TREE_NODE::TYPE::ITEM );
}


bool DESIGN_BLOCK_CONTROL::selIsDesignBlock( const SELECTION& aSel )
{
    LIB_TREE_NODE* current = getCurrentTreeNode();
    return current && current->m_Type == LIB_TREE_NODE::TYPE::ITEM;
}


void DESIGN_BLOCK_CONTROL::setTransitions()
{
    Go( &DESIGN_BLOCK_CONTROL::PinLibrary,      ACTIONS::pinLibrary.MakeEvent() );
    Go( &DESIGN_BLOCK_CONTROL::UnpinLibrary,    ACTIONS::unpinLibrary.MakeEvent() );
    Go( &DESIGN_BLOCK_CONTROL::NewLibrary,      ACTIONS::newLibrary.MakeEvent() );
    Go( &DESIGN_BLOCK_CONTROL::HideLibraryTree, ACTIONS::hideLibraryTree.MakeEvent() );
}


LIB_ID DESIGN_BLOCK_CONTROL::getSelectedLibId()
{
    getDesignBlockPane()->GetSelectedLibId();

    return LIB_ID();
}


LIB_TREE_NODE* DESIGN_BLOCK_CONTROL::getCurrentTreeNode()
{
    LIB_TREE* libTree = getDesignBlockPane()->GetDesignBlockPanel()->GetLibTree();
    return libTree ? libTree->GetCurrentTreeNode() : nullptr;
}


void DESIGN_BLOCK_CONTROL::notifyOtherFrames()
{
    std::string payload = "";

    for( FRAME_T frame : m_framesToNotify )
        m_frame->Kiway().ExpressMail( frame, MAIL_RELOAD_LIB, payload );
}
