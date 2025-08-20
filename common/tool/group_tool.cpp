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
#include "tool/group_tool.h"

#include <eda_draw_frame.h>
#include <kiplatform/ui.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tool/picker_tool.h>
#include <tool/selection.h>
#include <tool/selection_tool.h>
#include <status_popup.h>
#include <commit.h>
#include <bitmaps.h>
#include <dialogs/dialog_group_properties.h>
#include <eda_group.h>

class GROUP_CONTEXT_MENU : public ACTION_MENU
{
public:
    GROUP_CONTEXT_MENU() : ACTION_MENU( true )
    {
        SetIcon( BITMAPS::group ); // fixme
        SetTitle( _( "Grouping" ) );

        Add( ACTIONS::group );
        Add( ACTIONS::ungroup );
        Add( ACTIONS::addToGroup );
        Add( ACTIONS::removeFromGroup );
    }

    ACTION_MENU* create() const override
    {
        GROUP_CONTEXT_MENU* menu = new GROUP_CONTEXT_MENU();
        menu->SetSelectionTool( m_selectionTool );
        return menu;
    }

    void SetSelectionTool( SELECTION_TOOL* aTool ) { m_selectionTool = aTool; }

private:
    void update() override
    {
        bool canGroup = false;
        bool hasGroup = false;
        bool hasMember = false;
        bool onlyOneGroup = false;
        bool hasUngroupedItems = false;

        if( m_selectionTool != nullptr )
        {
            for( EDA_ITEM* item : m_selectionTool->GetSelection() )
            {
                canGroup = true;

                if( item->Type() == PCB_GROUP_T || item->Type() == SCH_GROUP_T )
                {
                    // Only allow one group to be selected for adding to existing group
                    if( hasGroup )
                        onlyOneGroup = false;
                    else
                    {
                        onlyOneGroup = true;
                        hasGroup = true;
                    }
                }
                else if( !item->GetParentGroup() )
                    hasUngroupedItems = true;

                if( item->GetParentGroup() )
                    hasMember = true;
            }
        }

        Enable( ACTIONS::group.GetUIId(),           canGroup );
        Enable( ACTIONS::ungroup.GetUIId(),         hasGroup );
        Enable( ACTIONS::addToGroup.GetUIId(),      onlyOneGroup && hasUngroupedItems );
        Enable( ACTIONS::removeFromGroup.GetUIId(), hasMember );
    }

private:
    SELECTION_TOOL* m_selectionTool = nullptr;
};


GROUP_TOOL::GROUP_TOOL() : TOOL_INTERACTIVE( "common.Groups" )
{
}


void GROUP_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_DRAW_FRAME>();
    m_commit = createCommit();
}


bool GROUP_TOOL::Init()
{
    m_frame = getEditFrame<EDA_DRAW_FRAME>();
    m_commit = createCommit();

    // Find the selection tool, so they can cooperate
    m_selectionTool = static_cast<SELECTION_TOOL*>( m_toolMgr->FindTool( "common.InteractiveSelection" ) );
    wxCHECK( m_selectionTool, false );

    TOOL_MENU& selToolMenu = m_selectionTool->GetToolMenu();

    std::shared_ptr<GROUP_CONTEXT_MENU> groupMenu = std::make_shared<GROUP_CONTEXT_MENU>();
    groupMenu->SetTool( this );
    groupMenu->SetSelectionTool( m_selectionTool );
    selToolMenu.RegisterSubMenu( groupMenu );

    selToolMenu.GetMenu().AddMenu( groupMenu.get(), SELECTION_CONDITIONS::NotEmpty, 100 );

    return true;
}


int GROUP_TOOL::GroupProperties( const TOOL_EVENT& aEvent )
{
    EDA_GROUP* group = aEvent.Parameter<EDA_GROUP*>();

    if( m_propertiesDialog )
        m_propertiesDialog->Destroy();

    m_propertiesDialog = new DIALOG_GROUP_PROPERTIES( m_frame, group, m_commit );

    m_propertiesDialog->Show( true );

    return 0;
}


int GROUP_TOOL::Ungroup( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();
    EDA_ITEMS        toSelect;

    if( selection.Empty() )
        m_toolMgr->RunAction( ACTIONS::selectionCursor );

    SELECTION selCopy = selection;
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    for( EDA_ITEM* item : selCopy )
    {
        if( EDA_GROUP* group = dynamic_cast<EDA_GROUP*>( item ) )
        {
            group->AsEdaItem()->SetSelected();
            m_commit->Remove( group->AsEdaItem(), m_frame->GetScreen() );

            for( EDA_ITEM* member : group->GetItems() )
            {
                m_commit->Modify( member, m_frame->GetScreen(), RECURSE_MODE::NO_RECURSE );
                toSelect.push_back( member );
            }

            group->RemoveAll();
        }
    }

    m_commit->Push( _( "Ungroup Items" ) );

    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &toSelect );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->OnModify();

    return 0;
}


int GROUP_TOOL::AddToGroup( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    EDA_GROUP* group = nullptr;
    EDA_ITEMS  toAdd;

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == PCB_GROUP_T || item->Type() == SCH_GROUP_T )
        {
            // Only allow one group to be selected for adding to existing group
            if( group != nullptr )
                return 0;

            group = dynamic_cast<EDA_GROUP*>( item );
        }
        else if( !item->GetParentGroup() )
        {
            toAdd.push_back( item );
        }
    }

    if( !group || toAdd.empty() )
        return 0;

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_commit->Modify( group->AsEdaItem(), m_frame->GetScreen(), RECURSE_MODE::NO_RECURSE );

    for( EDA_ITEM* item : toAdd )
    {
        EDA_GROUP* existingGroup = item->GetParentGroup();

        if( existingGroup != group )
        {
            m_commit->Modify( item, m_frame->GetScreen() );

            if( existingGroup )
                m_commit->Modify( existingGroup->AsEdaItem(), m_frame->GetScreen(), RECURSE_MODE::NO_RECURSE );

            group->AddItem( item );
        }
    }

    m_commit->Push( _( "Add Items to Group" ) );

    m_selectionTool->AddItemToSel( group->AsEdaItem() );
    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->OnModify();

    return 0;
}


int GROUP_TOOL::RemoveFromGroup( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Empty() )
        m_toolMgr->RunAction( ACTIONS::selectionCursor );

    for( EDA_ITEM* item : selection )
    {
        if( EDA_GROUP* group = item->GetParentGroup() )
        {
            m_commit->Modify( group->AsEdaItem(), m_frame->GetScreen(), RECURSE_MODE::NO_RECURSE );
            m_commit->Modify( item, m_frame->GetScreen() );
            group->RemoveItem( item );
        }
    }

    m_commit->Push( _( "Remove Group Items" ) );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->OnModify();

    return 0;
}


int GROUP_TOOL::EnterGroup( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.GetSize() == 1 &&
            (selection[0]->Type() == SCH_GROUP_T || selection[0]->Type() == PCB_GROUP_T) )
    {
        m_selectionTool->EnterGroup();
    }

    return 0;
}


int GROUP_TOOL::LeaveGroup( const TOOL_EVENT& aEvent )
{
    m_selectionTool->ExitGroup( true /* Select the group */ );
    return 0;
}


void GROUP_TOOL::setTransitions()
{
    Go( &GROUP_TOOL::GroupProperties,         ACTIONS::groupProperties.MakeEvent() );
    Go( &GROUP_TOOL::PickNewMember,           ACTIONS::pickNewGroupMember.MakeEvent() );

    Go( &GROUP_TOOL::Group,                   ACTIONS::group.MakeEvent() );
    Go( &GROUP_TOOL::Ungroup,                 ACTIONS::ungroup.MakeEvent() );
    Go( &GROUP_TOOL::AddToGroup,              ACTIONS::addToGroup.MakeEvent() );
    Go( &GROUP_TOOL::RemoveFromGroup,         ACTIONS::removeFromGroup.MakeEvent() );
    Go( &GROUP_TOOL::EnterGroup,              ACTIONS::groupEnter.MakeEvent() );
    Go( &GROUP_TOOL::LeaveGroup,              ACTIONS::groupLeave.MakeEvent() );
}
