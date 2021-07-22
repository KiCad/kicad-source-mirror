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
#include <tools/pcb_picker_tool.h>
#include <pcb_base_edit_frame.h>
#include <pcb_group.h>
#include <status_popup.h>
#include <board_commit.h>
#include <bitmaps.h>
#include <dialogs/dialog_group_properties.h>


DIALOG_GROUP_PROPERTIES::DIALOG_GROUP_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                                  PCB_GROUP* aGroup ) :
        DIALOG_GROUP_PROPERTIES_BASE( aParent ),
        m_brdEditor( aParent ),
        m_toolMgr( aParent->GetToolManager() ),
        m_group( aGroup )
{
    m_bpAddMember->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpRemoveMember->SetBitmap( KiBitmap( BITMAPS::small_trash ) );

    m_nameCtrl->SetValue( m_group->GetName() );
    m_locked->SetValue( m_group->IsLocked() );

    for( BOARD_ITEM* item : m_group->GetItems() )
        m_membersList->Append( item->GetSelectMenuText( m_brdEditor->GetUserUnits() ), item );

    m_sdbSizerOK->SetDefault();

    SetInitialFocus( m_nameCtrl );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_GROUP_PROPERTIES::~DIALOG_GROUP_PROPERTIES()
{
    if( m_brdEditor->IsBeingDeleted() )
        return;

    m_brdEditor->FocusOnItem( nullptr );
    m_brdEditor->GetCanvas()->Refresh();
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

    m_group->RunOnDescendants(
            [&]( BOARD_ITEM* descendant )
            {
                commit.Modify( descendant );
            } );

    for( size_t ii = 0; ii < m_membersList->GetCount(); ++ii )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( m_membersList->GetClientData( ii ) );
        PCB_GROUP*  existingGroup = item->GetParentGroup();

        if( existingGroup != m_group )
        {
            commit.Modify( item );

            if( existingGroup )
                commit.Modify( existingGroup );
        }
    }

    m_group->SetName( m_nameCtrl->GetValue() );
    m_group->SetLocked( m_locked->GetValue() );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_group->RemoveAll();

    for( size_t ii = 0; ii < m_membersList->GetCount(); ++ii )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( m_membersList->GetClientData( ii ) );
        m_group->AddItem( item );
    }

    m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, m_group );

    commit.Push( _( "Modified group" ) );
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

    m_brdEditor->FocusOnItem( nullptr );
    m_brdEditor->GetCanvas()->Refresh();
}


