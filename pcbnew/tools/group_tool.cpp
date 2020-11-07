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
#include <tools/selection_tool.h>
#include <tools/group_tool.h>
#include <tools/pcbnew_picker_tool.h>
#include <status_popup.h>
#include <board_commit.h>
#include <bitmaps.h>
#include <dialogs/dialog_group_properties_base.h>
#include "edit_tool.h"

class DIALOG_GROUP_PROPERTIES : public DIALOG_GROUP_PROPERTIES_BASE
{
private:
    PCB_BASE_EDIT_FRAME* m_brdEditor;
    TOOL_MANAGER*        m_toolMgr;
    PCB_GROUP*           m_group;

public:
    DIALOG_GROUP_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_GROUP* aTarget );
    ~DIALOG_GROUP_PROPERTIES() { }

    void OnMemberSelected( wxCommandEvent& event ) override;
    void OnAddMember( wxCommandEvent& event ) override;
    void OnRemoveMember( wxCommandEvent& event ) override;

    void DoAddMember( EDA_ITEM* aItem );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};


DIALOG_GROUP_PROPERTIES::DIALOG_GROUP_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                                  PCB_GROUP* aGroup ) :
        DIALOG_GROUP_PROPERTIES_BASE( aParent ),
        m_brdEditor( aParent ),
        m_toolMgr( aParent->GetToolManager() ),
        m_group( aGroup )
{
    m_bpAddMember->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_bpRemoveMember->SetBitmap( KiBitmap( trash_xpm ) );

    m_nameCtrl->SetValue( m_group->GetName() );

    for( BOARD_ITEM* item : m_group->GetItems() )
        m_membersList->Append( item->GetSelectMenuText( m_brdEditor->GetUserUnits() ), item );

    m_sdbSizerOK->SetDefault();

    SetInitialFocus( m_nameCtrl );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


bool DIALOG_GROUP_PROPERTIES::TransferDataToWindow()
{
    // Don't do anything here; it gets called every time we re-show the dialog after
    // picking a new member.
    return true;
}


bool DIALOG_GROUP_PROPERTIES::TransferDataFromWindow()
{
    BOARD_COMMIT commit( m_brdEditor );
    commit.Modify( m_group );

    m_group->SetName( m_nameCtrl->GetValue() );


    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_group->RemoveAll();

    for( size_t ii = 0; ii < m_membersList->GetCount(); ++ii )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( m_membersList->GetClientData( ii ) );
        PCB_GROUP*  existingGroup = item->GetParentGroup();

        if( existingGroup )
        {
            commit.Modify( existingGroup );
            existingGroup->RemoveItem( item );
        }

        m_group->AddItem( item );
    }

    commit.Push( _( "Modified group" ) );
    m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, m_group );

    return true;
}


void DIALOG_GROUP_PROPERTIES::OnMemberSelected( wxCommandEvent& aEvent )
{
    int selected = m_membersList->GetSelection();

    if( selected >= 0 )
    {
        WINDOW_THAWER thawer( m_brdEditor );
        BOARD_ITEM*   item = static_cast<BOARD_ITEM*>( m_membersList->GetClientData( selected ) );

        m_brdEditor->FocusOnItem( item );
        m_brdEditor->GetCanvas()->Refresh();
    }

    aEvent.Skip();
}


void DIALOG_GROUP_PROPERTIES::OnAddMember( wxCommandEvent& event )
{
    m_toolMgr->RunAction( PCB_ACTIONS::pickNewGroupMember, true );
}


void DIALOG_GROUP_PROPERTIES::DoAddMember( EDA_ITEM* aItem )
{
    for( size_t ii = 0; ii < m_membersList->GetCount(); ++ii )
    {
        if( aItem == static_cast<BOARD_ITEM*>( m_membersList->GetClientData( ii ) ) )
            return;
    }

    if( aItem == m_group )
        return;

    m_membersList->Append( aItem->GetSelectMenuText( m_brdEditor->GetUserUnits() ), aItem );
}


void DIALOG_GROUP_PROPERTIES::OnRemoveMember( wxCommandEvent& event )
{
    int selected = m_membersList->GetSelection();

    if( selected >= 0 )
        m_membersList->Delete( selected );
}


class GROUP_CONTEXT_MENU : public ACTION_MENU
{
public:
    GROUP_CONTEXT_MENU( ) : ACTION_MENU( true )
    {
        SetIcon( locked_xpm ); // fixme
        SetTitle( _( "Grouping" ) );

        Add( PCB_ACTIONS::groupCreate );
        Add( PCB_ACTIONS::groupUngroup );
        Add( PCB_ACTIONS::groupRemoveItems );
        Add( PCB_ACTIONS::groupEnter );
    }

    ACTION_MENU* create() const override
    {
        return new GROUP_CONTEXT_MENU();
    }

private:
    void update() override
    {
        SELECTION_TOOL* selTool = getToolManager()->GetTool<SELECTION_TOOL>();
        BOARD* board = selTool->GetBoard();

        const auto& selection = selTool->GetSelection();

        wxString check = board->GroupsSanityCheck();
        wxCHECK_RET( check == wxEmptyString, _( "Group is in inconsistent state:" ) + wxS( " " )+ check );

        BOARD::GroupLegalOpsField legalOps = board->GroupLegalOps( selection );

        Enable( PCB_ACTIONS::groupCreate.GetUIId(),      legalOps.create );
        Enable( PCB_ACTIONS::groupUngroup.GetUIId(),     legalOps.ungroup );
        Enable( PCB_ACTIONS::groupRemoveItems.GetUIId(), legalOps.removeItems );
        Enable( PCB_ACTIONS::groupEnter.GetUIId(),       legalOps.enter );
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
    m_selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

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
    std::string         tool = "pcbnew.EditGroups.selectNewMember";
    PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();
    STATUS_TEXT_POPUP   statusPopup( frame() );
    bool                done = false;

    if( m_propertiesDialog )
        m_propertiesDialog->Show( false );

    Activate();

    statusPopup.SetText( _( "Click on new member..." ) );

    picker->SetClickHandler(
        [&]( const VECTOR2D& aPoint ) -> bool
        {
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

            const PCBNEW_SELECTION& sel = m_selectionTool->RequestSelection(
                    []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, SELECTION_TOOL* sTool )
                    {
                        EditToolSelectionFilter( aCollector, EXCLUDE_TRANSIENTS, sTool );
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
    SELECTION_TOOL*         selTool   = m_toolMgr->GetTool<SELECTION_TOOL>();
    const PCBNEW_SELECTION& selection = selTool->GetSelection();
    BOARD*                  board     = getModel<BOARD>();
    PCB_GROUP*              group     = nullptr;

    if( selection.Empty() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );

    if( m_isFootprintEditor )
    {
        MODULE* module = board->GetFirstModule();

        m_frame->SaveCopyInUndoList( module, UNDO_REDO::CHANGED );

        group = new PCB_GROUP( module );
        module->Add( group );

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
    const PCBNEW_SELECTION&  selection = m_toolMgr->GetTool<SELECTION_TOOL>()->GetSelection();
    BOARD*                   board     = getModel<BOARD>();
    std::vector<BOARD_ITEM*> members;

    if( selection.Empty() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );

    PCBNEW_SELECTION selCopy = selection;
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    for( EDA_ITEM* item : selCopy )
    {
        PCB_GROUP* group = dynamic_cast<PCB_GROUP*>( item );

        if( group )
        {
            if( m_isFootprintEditor )
            {
                MODULE* module = board->GetFirstModule();

                m_frame->SaveCopyInUndoList( module, UNDO_REDO::CHANGED );

                group->RemoveAll();
                module->Remove( group );
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
    SELECTION_TOOL*         selTool   = m_toolMgr->GetTool<SELECTION_TOOL>();
    const PCBNEW_SELECTION& selection = selTool->GetSelection();
    BOARD_COMMIT            commit( m_frame );

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
    SELECTION_TOOL*         selTool   = m_toolMgr->GetTool<SELECTION_TOOL>();
    const PCBNEW_SELECTION& selection = selTool->GetSelection();

    if( selection.GetSize() == 1 && selection[0]->Type() == PCB_GROUP_T )
        selTool->EnterGroup();

    return 0;
}


int GROUP_TOOL::LeaveGroup( const TOOL_EVENT& aEvent )
{
    m_toolMgr->GetTool<SELECTION_TOOL>()->ExitGroup( true /* Select the group */ );
    return 0;
}


void GROUP_TOOL::setTransitions()
{
    Go( &GROUP_TOOL::GroupProperties,         PCB_ACTIONS::groupProperties.MakeEvent() );
    Go( &GROUP_TOOL::PickNewMember,           PCB_ACTIONS::pickNewGroupMember.MakeEvent() );

    Go( &GROUP_TOOL::Group,                   PCB_ACTIONS::groupCreate.MakeEvent() );
    Go( &GROUP_TOOL::Ungroup,                 PCB_ACTIONS::groupUngroup.MakeEvent() );
    Go( &GROUP_TOOL::RemoveFromGroup,         PCB_ACTIONS::groupRemoveItems.MakeEvent() );
    Go( &GROUP_TOOL::EnterGroup,              PCB_ACTIONS::groupEnter.MakeEvent() );
    Go( &GROUP_TOOL::LeaveGroup,              PCB_ACTIONS::groupLeave.MakeEvent() );
}
