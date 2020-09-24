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
    PCB_EDIT_FRAME* m_brdEditor;
    TOOL_MANAGER*   m_toolMgr;
    PCB_GROUP*      m_group;

public:
    DIALOG_GROUP_PROPERTIES( PCB_EDIT_FRAME* aParent, PCB_GROUP* aTarget );
    ~DIALOG_GROUP_PROPERTIES() { }

    void OnMemberSelected( wxCommandEvent& event ) override;
    void OnAddMember( wxCommandEvent& event ) override;
    void OnRemoveMember( wxCommandEvent& event ) override;

    void DoAddMember( EDA_ITEM* aItem );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};


DIALOG_GROUP_PROPERTIES::DIALOG_GROUP_PROPERTIES( PCB_EDIT_FRAME* aParent, PCB_GROUP* aGroup ) :
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
        PCB_GROUP*  existingGroup = item->GetGroup();

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


GROUP_TOOL::GROUP_TOOL() :
    PCB_TOOL_BASE( "pcbnew.Groups" ),
    m_propertiesDialog( nullptr ),
    m_selectionTool( nullptr )
{
}


void GROUP_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason != RUN )
        m_commit = std::make_unique<BOARD_COMMIT>( this );
}


bool GROUP_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    return m_selectionTool != nullptr;
}


int GROUP_TOOL::GroupProperties( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    PCB_GROUP*      group = aEvent.Parameter<PCB_GROUP*>();

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
        Wait()->SetPassEvent();

    return 0;
}


void GROUP_TOOL::setTransitions()
{
    Go( &GROUP_TOOL::GroupProperties,         PCB_ACTIONS::groupProperties.MakeEvent() );
    Go( &GROUP_TOOL::PickNewMember,           PCB_ACTIONS::pickNewGroupMember.MakeEvent() );
}
