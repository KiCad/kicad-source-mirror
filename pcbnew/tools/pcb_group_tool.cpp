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

#include <kiplatform/ui.h>
#include <tool/tool_manager.h>
#include <tools/pcb_group_tool.h>
#include <tools/pcb_picker_tool.h>
#include <tools/pcb_selection_tool.h>
#include <status_popup.h>
#include <board_commit.h>
#include <dialogs/dialog_group_properties.h>
#include <pcb_group.h>
#include <collectors.h>
#include <footprint.h>


std::shared_ptr<COMMIT> PCB_GROUP_TOOL::createCommit()
{
    return std::make_shared<BOARD_COMMIT>( m_toolMgr, m_frame->IsType( FRAME_PCB_EDITOR ),
                                                      m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) );
}


int PCB_GROUP_TOOL::PickNewMember( const TOOL_EVENT& aEvent )
{
    bool                isFootprintEditor = m_frame->GetFrameType() == FRAME_FOOTPRINT_EDITOR;
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    PCB_PICKER_TOOL*    picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();

    STATUS_TEXT_POPUP   statusPopup( m_frame );
    bool                done = false;

    if( m_propertiesDialog )
        m_propertiesDialog->Show( false );

    Activate();

    statusPopup.SetText( _( "Click on new member..." ) );

    picker->SetCursor( KICURSOR::BULLSEYE );
    picker->SetSnapping( false );
    picker->ClearHandlers();

    picker->SetClickHandler(
            [&]( const VECTOR2D& aPoint ) -> bool
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                const PCB_SELECTION& sel = selTool->RequestSelection(
                        []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
                        {
                        } );

                if( sel.Empty() )
                    return true; // still looking for an item

                statusPopup.Hide();

                if( m_propertiesDialog )
                {
                    EDA_ITEM* elem = sel.Front();

                    if( !isFootprintEditor )
                    {
                        while( elem->GetParent() && elem->GetParent()->Type() != PCB_T )
                            elem = elem->GetParent();
                    }

                    m_propertiesDialog->DoAddMember( elem );
                    m_propertiesDialog->Show( true );
                }

                return false; // got our item; don't need any more
            } );

    picker->SetMotionHandler(
            [&]( const VECTOR2D& aPos )
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
    m_frame->GetCanvas()->SetStatusPopup( statusPopup.GetPanel() );

    m_toolMgr->RunAction( ACTIONS::pickerTool, &aEvent );

    while( !done )
    {
        // Pass events unless we receive a null event, then we must shut down
        if( TOOL_EVENT* evt = Wait() )
            evt->SetPassEvent();
        else
            break;
    }

    picker->ClearHandlers();
    m_frame->GetCanvas()->SetStatusPopup( nullptr );

    return 0;
}


int PCB_GROUP_TOOL::Group( const TOOL_EVENT& aEvent )
{
    bool                isFootprintEditor = m_frame->GetFrameType() == FRAME_FOOTPRINT_EDITOR;
    PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION       selection;

    if( isFootprintEditor )
    {
        selection = selTool->RequestSelection(
                []( const VECTOR2I&, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* )
                {
                    // Iterate from the back so we don't have to worry about removals.
                    for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                    {
                        BOARD_ITEM* item = aCollector[i];

                        if( !item->IsGroupableType() )
                            aCollector.Remove( item );
                    }
                } );
    }
    else
    {
        selection = selTool->RequestSelection(
                []( const VECTOR2I&, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* )
                {
                    // Iterate from the back so we don't have to worry about removals.
                    for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                    {
                        BOARD_ITEM* item = aCollector[i];

                        if( item->GetParentFootprint() )
                            aCollector.Remove( item );

                        if( !item->IsGroupableType() )
                            aCollector.Remove( item );
                    }
                } );
    }

    if( selection.Empty() )
        return 0;

    BOARD*       board = getModel<BOARD>();
    PCB_GROUP*   group = nullptr;

    if( isFootprintEditor )
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

    for( EDA_ITEM* eda_item : selection )
    {
        if( eda_item->IsBOARD_ITEM() )
        {
            if( EDA_GROUP* existingGroup = eda_item->GetParentGroup() )
                m_commit->Modify( existingGroup->AsEdaItem(), nullptr, RECURSE_MODE::NO_RECURSE );

            m_commit->Modify( eda_item );
            group->AddItem( eda_item );
        }
    }

    m_commit->Add( group );
    m_commit->Push( _( "Group Items" ) );

    m_toolMgr->RunAction( ACTIONS::selectionClear );
    m_toolMgr->RunAction( ACTIONS::selectItem, group->AsEdaItem() );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->OnModify();

    return 0;
}
