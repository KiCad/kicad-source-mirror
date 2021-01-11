/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <functional>
#include <memory>
using namespace std::placeholders;

#include "position_relative_tool.h"
#include "pcb_actions.h"
#include "pcb_selection_tool.h"
#include "edit_tool.h"
#include "pcb_picker_tool.h"
#include <dialogs/dialog_position_relative.h>
#include <status_popup.h>
#include <board_commit.h>
#include <bitmaps.h>
#include <confirm.h>


POSITION_RELATIVE_TOOL::POSITION_RELATIVE_TOOL() :
    PCB_TOOL_BASE( "pcbnew.PositionRelative" ),
    m_dialog( NULL ),
    m_selectionTool( NULL ),
    m_anchor_item( NULL )
{
}


void POSITION_RELATIVE_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason != RUN )
        m_commit = std::make_unique<BOARD_COMMIT>( this );
}


bool POSITION_RELATIVE_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    return m_selectionTool != nullptr;
}


int POSITION_RELATIVE_TOOL::PositionRelative( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();

    const auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( item->Type() == PCB_MARKER_T )
                        aCollector.Remove( item );
                }
            },
            !m_isFootprintEditor /* prompt user regarding locked items */ );

    if( selection.Empty() )
        return 0;

    m_selection = selection;

    // The dialog is not modal and not deleted between calls.
    // It means some options can have changed since the last call.
    // Therefore we need to rebuild it in case UI units have changed since the last call.
    if( m_dialog && m_dialog->GetUserUnits() != editFrame->GetUserUnits() )
    {
        m_dialog->Destroy();
        m_dialog = nullptr;
    }

    if( !m_dialog )
        m_dialog = new DIALOG_POSITION_RELATIVE( editFrame, m_translation, m_anchor );

    m_dialog->Show( true );

    return 0;
}

wxPoint POSITION_RELATIVE_TOOL::GetSelectionAnchorPosition() const
{
    return m_selection.GetTopLeftItem()->GetPosition();
}


int POSITION_RELATIVE_TOOL::RelativeItemSelectionMove( wxPoint aPosAnchor, wxPoint aTranslation )
{
    wxPoint aggregateTranslation = aPosAnchor + aTranslation - GetSelectionAnchorPosition();

    for( auto item : m_selection )
    {
        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && frame()->IsType( FRAME_PCB_EDITOR ) )
            item = item->GetParent();

        m_commit->Modify( item );
        static_cast<BOARD_ITEM*>( item )->Move( aggregateTranslation );
    }

    m_commit->Push( _( "Position Relative" ) );

    if( m_selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    canvas()->Refresh();
    return 0;
}


int POSITION_RELATIVE_TOOL::SelectPositionRelativeItem( const TOOL_EVENT& aEvent  )
{
    std::string       tool = "pcbnew.PositionRelative.selectReferenceItem";
    PCB_PICKER_TOOL*  picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    STATUS_TEXT_POPUP statusPopup( frame() );
    bool              done = false;

    Activate();

    statusPopup.SetText( _( "Click on reference item..." ) );

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

            m_anchor_item = sel.Front();
            statusPopup.Hide();

            if( m_dialog )
                m_dialog->UpdateAnchor( sel.Front() );

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
            statusPopup.Hide();

            if( m_dialog )
                m_dialog->UpdateAnchor( m_anchor_item );
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


void POSITION_RELATIVE_TOOL::setTransitions()
{
    Go( &POSITION_RELATIVE_TOOL::PositionRelative, PCB_ACTIONS::positionRelative.MakeEvent() );
    Go( &POSITION_RELATIVE_TOOL::SelectPositionRelativeItem,
            PCB_ACTIONS::selectpositionRelativeItem.MakeEvent() );
}
