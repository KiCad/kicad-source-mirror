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

#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/group_tool.h>
#include <tools/pcb_picker_tool.h>
#include <status_popup.h>
#include <board_commit.h>
#include <bitmaps.h>
#include <dialogs/dialog_group_properties.h>


class GROUP_CONTEXT_MENU : public ACTION_MENU
{
public:
    GROUP_CONTEXT_MENU( ) : ACTION_MENU( true )
    {
        SetIcon( group_xpm ); // fixme
        SetTitle( _( "Grouping" ) );

        Add( PCB_ACTIONS::group );
        Add( PCB_ACTIONS::ungroup );
        Add( PCB_ACTIONS::removeFromGroup );
        Add( PCB_ACTIONS::groupEnter );
    }

    ACTION_MENU* create() const override
    {
        return new GROUP_CONTEXT_MENU();
    }

private:
    void update() override
    {
        PCB_SELECTION_TOOL* selTool = getToolManager()->GetTool<PCB_SELECTION_TOOL>();
        BOARD*              board = selTool->GetBoard();

        const auto& selection = selTool->GetSelection();

        wxString check = board->GroupsSanityCheck();
        wxCHECK_RET( check == wxEmptyString, _( "Group is in inconsistent state:" ) + wxS( " " )+ check );

        BOARD::GroupLegalOpsField legalOps = board->GroupLegalOps( selection );

        Enable( PCB_ACTIONS::group.GetUIId(),           legalOps.create );
        Enable( PCB_ACTIONS::ungroup.GetUIId(),         legalOps.ungroup );
        Enable( PCB_ACTIONS::removeFromGroup.GetUIId(), legalOps.removeItems );
        Enable( PCB_ACTIONS::groupEnter.GetUIId(),      legalOps.enter );
    }
};


GROUP_TOOL::GROUP_TOOL() :
    PCB_TOOL_BASE( "pcbnew.Groups" ),
    m_frame( nullptr ),
    m_propertiesDialog( nullptr ),
    m_selectionTool( nullptr )
{
}


void GROUP_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    if( aReason != RUN )
        m_commit = std::make_unique<BOARD_COMMIT>( this );
}


bool GROUP_TOOL::Init()
{
    m_frame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    auto groupMenu = std::make_shared<GROUP_CONTEXT_MENU>();
    groupMenu->SetTool( this );

    // Add the group control menus to relevant other tools
    if( m_selectionTool )
    {
        auto& toolMenu = m_selectionTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        toolMenu.AddSubMenu( groupMenu );

        menu.AddMenu( groupMenu.get(), SELECTION_CONDITIONS::NotEmpty, 100 );
    }

    return true;
}


int GROUP_TOOL::GroupProperties( const TOOL_EVENT& aEvent )
{
    PCB_BASE_EDIT_FRAME* editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    PCB_GROUP*           group = aEvent.Parameter<PCB_GROUP*>();

    if( m_propertiesDialog )
        m_propertiesDialog->Destroy();

    m_propertiesDialog = new DIALOG_GROUP_PROPERTIES( editFrame, group );

    m_propertiesDialog->Show( true );

    return 0;
}


int GROUP_TOOL::PickNewMember( const TOOL_EVENT& aEvent  )
{
    std::string       tool = "pcbnew.EditGroups.selectNewMember";
    PCB_PICKER_TOOL*  picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    STATUS_TEXT_POPUP statusPopup( frame() );
    bool              done = false;

    if( m_propertiesDialog )
        m_propertiesDialog->Show( false );

    Activate();

    statusPopup.SetText( _( "Click on new member..." ) );

    picker->SetClickHandler(
        [&]( const VECTOR2D& aPoint ) -> bool
        {
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

            const PCB_SELECTION& sel = m_selectionTool->RequestSelection(
                    []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector,
                        PCB_SELECTION_TOOL* sTool )
                    {
                    } );

            if( sel.Empty() )
                return true;    // still looking for an item

            statusPopup.Hide();

            if( m_propertiesDialog )
            {
                m_propertiesDialog->DoAddMember( sel.Front() );
                m_propertiesDialog->Show( true );
            }

            return false;       // got our item; don't need any more
        } );

    picker->SetMotionHandler(
        [&] ( const VECTOR2D& aPos )
        {
            statusPopup.Move( wxGetMousePosition() + wxPoint( 20, -50 ) );
        } );

    picker->SetCancelHandler(
        [&]()
        {
            if( m_propertiesDialog )
                m_propertiesDialog->Show( true );

            statusPopup.Hide();
        } );

    picker->SetFinalizeHandler(
        [&]( const int& aFinalState )
        {
            done = true;
        } );

    statusPopup.Move( wxGetMousePosition() + wxPoint( 20, -50 ) );
    statusPopup.Popup();

    m_toolMgr->RunAction( ACTIONS::pickerTool, true, &tool );

    while( !done )
    {
        // Pass events unless we receive a null event, then we must shut down
        if( TOOL_EVENT* evt = Wait() )
            evt->SetPassEvent();
        else
            break;
    }

    return 0;
}


int GROUP_TOOL::Group( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION       selection;

    if( m_isFootprintEditor )
    {
        selection = selTool->RequestSelection(
                []( const VECTOR2I& , GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL*  )
                {
                } );
    }
    else
    {
        selection = selTool->RequestSelection(
                []( const VECTOR2I& , GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL*  )
                {
                    // Iterate from the back so we don't have to worry about removals.
                    for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                    {
                        BOARD_ITEM* item = aCollector[ i ];

                        switch( item->Type() )
                        {
                        case PCB_FP_TEXT_T:
                        case PCB_FP_SHAPE_T:
                        case PCB_FP_ZONE_T:
                            aCollector.Remove( item );
                            break;

                        default:
                            break;
                        }
                    }
                } );
    }

    if( selection.Empty() )
        return 0;

    BOARD*     board = getModel<BOARD>();
    PCB_GROUP* group = nullptr;

    if( m_isFootprintEditor )
    {
        FOOTPRINT* parentFootprint = board->GetFirstFootprint();

        m_frame->SaveCopyInUndoList( parentFootprint, UNDO_REDO::CHANGED );

        group = new PCB_GROUP( parentFootprint );
        parentFootprint->Add( group );

        for( EDA_ITEM* item : selection )
            group->AddItem( static_cast<BOARD_ITEM*>( item ) );
    }
    else
    {
        PICKED_ITEMS_LIST undoList;

        group = new PCB_GROUP( board );
        board->Add( group );

        undoList.PushItem( ITEM_PICKER( nullptr, group, UNDO_REDO::NEWITEM ) );

        for( EDA_ITEM* item : selection )
        {
            group->AddItem( static_cast<BOARD_ITEM*>( item ) );
            undoList.PushItem( ITEM_PICKER( nullptr, item, UNDO_REDO::GROUP ) );
        }

        m_frame->SaveCopyInUndoList( undoList, UNDO_REDO::GROUP );
    }

    selTool->ClearSelection();
    selTool->select( group );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->OnModify();

    return 0;
}


int GROUP_TOOL::Ungroup( const TOOL_EVENT& aEvent )
{
    const PCB_SELECTION&     selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();
    BOARD*                   board     = getModel<BOARD>();
    std::vector<BOARD_ITEM*> members;

    if( selection.Empty() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );

    PCB_SELECTION selCopy = selection;
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    for( EDA_ITEM* item : selCopy )
    {
        PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( item );

        if( group )
        {
            if( m_isFootprintEditor )
            {
                FOOTPRINT* parentFootprint = board->GetFirstFootprint();

                m_frame->SaveCopyInUndoList( parentFootprint, UNDO_REDO::CHANGED );

                group->RemoveAll();
                parentFootprint->Remove( group );
            }
            else
            {
                PICKED_ITEMS_LIST undoList;

                for( BOARD_ITEM* member : group->GetItems() )
                {
                    undoList.PushItem( ITEM_PICKER( nullptr, member, UNDO_REDO::UNGROUP ) );
                    members.push_back( member );
                }

                group->RemoveAll();
                board->Remove( group );

                undoList.PushItem( ITEM_PICKER( nullptr, group, UNDO_REDO::DELETED ) );

                m_frame->SaveCopyInUndoList( undoList, UNDO_REDO::UNGROUP );
            }

            group->SetSelected();
        }
    }

    m_toolMgr->RunAction( PCB_ACTIONS::selectItems, true, &members );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->OnModify();

    return 0;
}


int GROUP_TOOL::RemoveFromGroup( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selTool   = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();
    BOARD_COMMIT         commit( m_frame );

    if( selection.Empty() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );

    std::map<PCB_GROUP*, std::vector<BOARD_ITEM*>> groupMap;

    for( EDA_ITEM* item : selection )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
        PCB_GROUP*  group = boardItem->GetParentGroup();

        if( group )
            groupMap[ group ].push_back( boardItem );
    }

    for( std::pair<PCB_GROUP*, std::vector<BOARD_ITEM*>> pair : groupMap )
    {
        commit.Modify( pair.first );

        for( BOARD_ITEM* item : pair.second )
            pair.first->RemoveItem( item );
    }

    commit.Push( "Remove Group Items" );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->OnModify();

    return 0;
}


int GROUP_TOOL::EnterGroup( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selTool   = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();

    if( selection.GetSize() == 1 && selection[0]->Type() == PCB_GROUP_T )
        selTool->EnterGroup();

    return 0;
}


int GROUP_TOOL::LeaveGroup( const TOOL_EVENT& aEvent )
{
    m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->ExitGroup( true /* Select the group */ );
    return 0;
}


void GROUP_TOOL::setTransitions()
{
    Go( &GROUP_TOOL::GroupProperties,         PCB_ACTIONS::groupProperties.MakeEvent() );
    Go( &GROUP_TOOL::PickNewMember,           PCB_ACTIONS::pickNewGroupMember.MakeEvent() );

    Go( &GROUP_TOOL::Group,                   PCB_ACTIONS::group.MakeEvent() );
    Go( &GROUP_TOOL::Ungroup,                 PCB_ACTIONS::ungroup.MakeEvent() );
    Go( &GROUP_TOOL::RemoveFromGroup,         PCB_ACTIONS::removeFromGroup.MakeEvent() );
    Go( &GROUP_TOOL::EnterGroup,              PCB_ACTIONS::groupEnter.MakeEvent() );
    Go( &GROUP_TOOL::LeaveGroup,              PCB_ACTIONS::groupLeave.MakeEvent() );
}
