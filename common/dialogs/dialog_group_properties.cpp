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

#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_picker_tool.h>
#include <pcb_base_edit_frame.h>
#include <pcb_group.h>
#include <eda_group.h>
#include <status_popup.h>
#include <board_commit.h>
#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>
#include <dialogs/dialog_group_properties.h>


DIALOG_GROUP_PROPERTIES::DIALOG_GROUP_PROPERTIES( EDA_DRAW_FRAME* aParent,
                                                  EDA_GROUP* aGroup ) :
        DIALOG_GROUP_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_toolMgr( aParent->GetToolManager() ),
        m_group( aGroup )
{
    m_bpAddMember->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpRemoveMember->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_nameCtrl->SetValue( m_group->GetName() );

    if( aGroup->AsEdaItem()->Type() == PCB_GROUP_T )
    {
        m_locked->SetValue( static_cast<PCB_GROUP*>( aGroup )->IsLocked() );
        m_locked->Show( dynamic_cast<PCB_EDIT_FRAME*>( aParent ) != nullptr );
    }
    else
    {
        m_locked->SetValue( false );
        m_locked->Hide();
    }

    for( EDA_ITEM* item : m_group->GetItems() )
        m_membersList->Append( item->GetItemDescription( m_frame, true ), item );

    SetupStandardButtons();

    SetInitialFocus( m_nameCtrl );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_GROUP_PROPERTIES::~DIALOG_GROUP_PROPERTIES()
{
    if( m_frame->IsBeingDeleted() )
        return;

    m_frame->ClearFocus();
    m_frame->GetCanvas()->Refresh();
}


bool DIALOG_GROUP_PROPERTIES::TransferDataToWindow()
{
    // Don't do anything here; it gets called every time we re-show the dialog after
    // picking a new member.
    return true;
}


bool DIALOG_GROUP_PROPERTIES::TransferDataFromWindow()
{
    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_group->AsEdaItem() );

    for( size_t ii = 0; ii < m_membersList->GetCount(); ++ii )
    {
        EDA_ITEM*  item = static_cast<EDA_ITEM*>( m_membersList->GetClientData( ii ) );
        EDA_GROUP* existingGroup = item->GetParentGroup();

        if( existingGroup != m_group )
        {
            commit.Modify( item );

            if( existingGroup )
                commit.Modify( existingGroup->AsEdaItem() );
        }
    }

    m_group->SetName( m_nameCtrl->GetValue() );

    if( m_group->AsEdaItem()->Type() == PCB_GROUP_T )
    {
        static_cast<PCB_GROUP*>( m_group )->SetLocked( m_locked->GetValue() );
    }

    m_toolMgr->RunAction( ACTIONS::selectionClear );
    m_group->RemoveAll();

    for( size_t ii = 0; ii < m_membersList->GetCount(); ++ii )
    {
        EDA_ITEM* item = static_cast<EDA_ITEM*>( m_membersList->GetClientData( ii ) );
        m_group->AddItem( item );
    }

    m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, m_group->AsEdaItem() );

    commit.Push( _( "Edit Group Properties" ) );
    return true;
}


void DIALOG_GROUP_PROPERTIES::OnMemberSelected( wxCommandEvent& aEvent )
{
    int selected = m_membersList->GetSelection();

    if( selected >= 0 )
    {
        WINDOW_THAWER thawer( m_frame );
        EDA_ITEM*     item = static_cast<EDA_ITEM*>( m_membersList->GetClientData( selected ) );

        m_frame->FocusOnItem( item );
        m_frame->GetCanvas()->Refresh();
    }

    aEvent.Skip();
}


void DIALOG_GROUP_PROPERTIES::OnAddMember( wxCommandEvent& event )
{
    m_toolMgr->RunAction( ACTIONS::pickNewGroupMember );
}


void DIALOG_GROUP_PROPERTIES::DoAddMember( EDA_ITEM* aItem )
{

    for( size_t ii = 0; ii < m_membersList->GetCount(); ++ii )
    {
        if( aItem == static_cast<EDA_ITEM*>( m_membersList->GetClientData( ii ) ) )
            return;
    }

    if( aItem == m_group->AsEdaItem() )
        return;

    m_membersList->Append( aItem->GetItemDescription( m_frame, true ), aItem );
}


void DIALOG_GROUP_PROPERTIES::OnRemoveMember( wxCommandEvent& event )
{
    int selected = m_membersList->GetSelection();

    if( selected >= 0 )
        m_membersList->Delete( selected );

    m_frame->ClearFocus();
    m_frame->GetCanvas()->Refresh();
}


