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

#include <kiplatform/ui.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/group_tool.h>
#include <tools/pcb_picker_tool.h>
#include <status_popup.h>
#include <board_commit.h>
#include <bitmaps.h>
#include <dialogs/dialog_group_properties.h>
#include <pcb_group.h>
#include <collectors.h>
#include <footprint.h>

class GROUP_CONTEXT_MENU : public ACTION_MENU
{
public:
    GROUP_CONTEXT_MENU( ) : ACTION_MENU( true )
    {
        SetIcon( BITMAPS::group ); // fixme
        SetTitle( _( "Grouping" ) );

        Add( PCB_ACTIONS::group );
        Add( PCB_ACTIONS::ungroup );
        Add( PCB_ACTIONS::removeFromGroup );
    }

    ACTION_MENU* create() const override
    {
        return new GROUP_CONTEXT_MENU();
    }

private:
    void update() override
    {
        PCB_SELECTION_TOOL* selTool = getToolManager()->GetTool<PCB_SELECTION_TOOL>();
        BOARD*              board = static_cast<BOARD*>( getToolManager()->GetModel() );

        const auto& selection = selTool->GetSelection();

        wxString check = board->GroupsSanityCheck();
        wxCHECK_RET( check == wxEmptyString, _( "Group is in inconsistent state:" ) + wxS( " " ) + check );

        BOARD::GroupLegalOpsField legalOps = board->GroupLegalOps( selection );

        Enable( PCB_ACTIONS::group.GetUIId(),           legalOps.create );
        Enable( PCB_ACTIONS::ungroup.GetUIId(),         legalOps.ungroup );
        Enable( PCB_ACTIONS::removeFromGroup.GetUIId(), legalOps.removeItems );
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

    // Add the group control menus to relevant other tools
    if( m_selectionTool )
    {
        TOOL_MENU& selToolMenu = m_selectionTool->GetToolMenu();

        std::shared_ptr<GROUP_CONTEXT_MENU> groupMenu = std::make_shared<GROUP_CONTEXT_MENU>();
        groupMenu->SetTool( this );
        selToolMenu.RegisterSubMenu( groupMenu );

        selToolMenu.GetMenu().AddMenu( groupMenu.get(), SELECTION_CONDITIONS::NotEmpty, 100 );
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
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

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
                    EDA_ITEM* elem = sel.Front();

                    if( !m_isFootprintEditor )
                    {
                        while( elem->GetParent() && elem->GetParent()->Type() != PCB_T )
                            elem = elem->GetParent();
                    }

                    m_propertiesDialog->DoAddMember( elem );
                    m_propertiesDialog->Show( true );
                }

                return false;       // got our item; don't need any more
            } );

    picker->SetMotionHandler(
            [&] ( const VECTOR2D& aPos )
            {
                statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
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

    statusPopup.Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, -50 ) );
    statusPopup.Popup();
    canvas()->SetStatusPopup( statusPopup.GetPanel() );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    while( !done )
    {
        // Pass events unless we receive a null event, then we must shut down
        if( TOOL_EVENT* evt = Wait() )
            evt->SetPassEvent();
        else
            break;
    }

    canvas()->SetStatusPopup( nullptr );

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

                        if( item->GetParentFootprint() )
                            aCollector.Remove( item );
                    }
                } );
    }

    if( selection.Empty() )
        return 0;

    BOARD*       board = getModel<BOARD>();
    BOARD_COMMIT commit( m_toolMgr );
    PCB_GROUP*   group = nullptr;

    if( m_isFootprintEditor )
        group = new PCB_GROUP( board->GetFirstFootprint() );
    else
        group = new PCB_GROUP( board );

    for( EDA_ITEM* eda_item : selection )
    {
        if( eda_item->IsBOARD_ITEM() )
        {
            if( static_cast<BOARD_ITEM*>( eda_item )->IsLocked() )
                group->SetLocked( true );
        }
    }

    commit.Add( group );

    for( EDA_ITEM* eda_item : selection )
    {
        if( eda_item->IsBOARD_ITEM() )
            commit.Stage( static_cast<BOARD_ITEM*>( eda_item ), CHT_GROUP );
    }

    commit.Push( _( "Group Items" ) );

    selTool->ClearSelection();
    selTool->select( group );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->OnModify();

    return 0;
}


int GROUP_TOOL::Ungroup( const TOOL_EVENT& aEvent )
{
    const PCB_SELECTION& selection = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetSelection();
    BOARD_COMMIT         commit( m_toolMgr );
    EDA_ITEMS            toSelect;

    if( selection.Empty() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor );

    PCB_SELECTION selCopy = selection;
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    for( EDA_ITEM* item : selCopy )
    {
        PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( item );

        if( group )
        {
            for( BOARD_ITEM* member : group->GetItems() )
            {
                commit.Stage( member, CHT_UNGROUP );
                toSelect.push_back( member );
            }

            group->SetSelected();
            commit.Remove( group );
        }
    }

    commit.Push( _( "Ungroup Items" ) );

    m_toolMgr->RunAction<EDA_ITEMS*>( PCB_ACTIONS::selectItems, &toSelect );

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
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor );

    for( EDA_ITEM* item : selection )
    {
        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );
        PCB_GROUP*  group = boardItem->GetParentGroup();

        if( group )
            commit.Stage( boardItem, CHT_UNGROUP );
    }

    commit.Push( wxT( "Remove Group Items" ) );

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
