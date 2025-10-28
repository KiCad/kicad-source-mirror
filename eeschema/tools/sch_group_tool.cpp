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
#include <frame_type.h>
#include <eda_draw_frame.h>
#include <symbol_edit_frame.h>
#include <sch_edit_frame.h>
#include <sch_screen.h>
#include <kiplatform/ui.h>
#include <tool/tool_manager.h>
#include <tool/picker_tool.h>
#include <tools/sch_group_tool.h>
#include <tools/sch_selection_tool.h>
#include <status_popup.h>
#include <sch_commit.h>
#include <dialogs/dialog_group_properties.h>
#include <sch_group.h>
#include <symbol.h>


int SCH_GROUP_TOOL::PickNewMember( const TOOL_EVENT& aEvent )
{
    bool                isSymbolEditor = m_frame->GetFrameType() == FRAME_SCH_SYMBOL_EDITOR;
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    PICKER_TOOL*        picker = m_toolMgr->GetTool<PICKER_TOOL>();

    STATUS_TEXT_POPUP   statusPopup( m_frame );
    bool                done = false;

    if( m_propertiesDialog )
        m_propertiesDialog->Show( false );

    Activate();

    statusPopup.SetText( _( "Click on new member..." ) );

    picker->SetClickHandler(
            [&]( const VECTOR2D& aPoint ) -> bool
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                const SCH_SELECTION& sel = selTool->RequestSelection();

                if( sel.Empty() )
                    return true; // still looking for an item

                statusPopup.Hide();

                if( m_propertiesDialog )
                {
                    EDA_ITEM* elem = sel.Front();

                    if( !isSymbolEditor )
                    {
                        while( elem->GetParent() && elem->GetParent()->Type() != SCH_SCREEN_T )
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


int SCH_GROUP_TOOL::Group( const TOOL_EVENT& aEvent )
{
    bool                isSymbolEditor = m_frame->GetFrameType() == FRAME_SCH_SYMBOL_EDITOR;
    SCH_SELECTION_TOOL* selTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION       selection = selTool->RequestSelection();

    // Iterate from the back so we don't have to worry about removals.
    for( int ii = selection.GetSize() - 1; ii >= 0; --ii )
    {
        if( !selection[ii]->IsSCH_ITEM() )
        {
            selection.Remove( selection[ii] );
            continue;
        }

        SCH_ITEM* schItem = static_cast<SCH_ITEM*>( selection[ii] );

        if( schItem->GetParentSymbol() )
            selection.Remove( schItem );

        if( !schItem->IsGroupableType() )
            selection.Remove( schItem );
    }

    if( selection.Empty() )
        return 0;

    SCH_GROUP*  group = new SCH_GROUP;
    SCH_SCREEN* screen = static_cast<SCH_BASE_FRAME*>( m_frame )->GetScreen();

    if( isSymbolEditor )
        group->SetParent( static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol() );
    else
        group->SetParent( screen );

    for( EDA_ITEM* eda_item : selection )
    {
        if( EDA_GROUP* existingGroup = eda_item->GetParentGroup() )
            m_commit->Modify( existingGroup->AsEdaItem(), screen, RECURSE_MODE::NO_RECURSE );

        m_commit->Modify( eda_item, screen, RECURSE_MODE::NO_RECURSE );
        group->AddItem( eda_item );
    }

    m_commit->Add( group, screen );
    m_commit->Push( _( "Group Items" ) );

    m_toolMgr->RunAction( ACTIONS::selectionClear );
    m_toolMgr->RunAction( ACTIONS::selectItem, group->AsEdaItem() );

    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
    m_frame->OnModify();

    return 0;
}
