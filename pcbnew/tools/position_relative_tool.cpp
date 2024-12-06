/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiplatform/ui.h>
#include <tools/position_relative_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_picker_tool.h>
#include <dialogs/dialog_position_relative.h>
#include <status_popup.h>
#include <board_commit.h>
#include <confirm.h>
#include <collectors.h>
#include <pad.h>
#include <footprint.h>
#include <pcb_group.h>


POSITION_RELATIVE_TOOL::POSITION_RELATIVE_TOOL() :
    PCB_TOOL_BASE( "pcbnew.PositionRelative" ),
    m_dialog( nullptr ),
    m_selectionTool( nullptr ),
    m_anchor_item( nullptr )
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
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForMarkers( aCollector );
            },
            !m_isFootprintEditor /* prompt user regarding locked items */ );

    if( selection.Empty() )
        return 0;

    m_selection = selection;

    // We prefer footprints, then pads, then anything else here.
    EDA_ITEM* preferredItem = m_selection.GetTopLeftItem( true );

    if( !preferredItem && m_selection.HasType( PCB_PAD_T ) )
    {
        PCB_SELECTION padsOnly = m_selection;
        std::deque<EDA_ITEM*>& items = padsOnly.Items();
        items.erase( std::remove_if( items.begin(), items.end(),
                                     []( const EDA_ITEM* aItem )
                                     {
                                         return aItem->Type() != PCB_PAD_T;
                                     } ), items.end() );

        preferredItem = padsOnly.GetTopLeftItem();
    }

    if( preferredItem )
        m_selectionAnchor = preferredItem->GetPosition();
    else
        m_selectionAnchor = m_selection.GetTopLeftItem()->GetPosition();

    // The dialog is not modal and not deleted between calls.
    // It means some options can have changed since the last call.
    // Therefore we need to rebuild it in case UI units have changed since the last call.
    if( m_dialog && m_dialog->GetUserUnits() != editFrame->GetUserUnits() )
    {
        m_dialog->Destroy();
        m_dialog = nullptr;
    }

    if( !m_dialog )
        m_dialog = new DIALOG_POSITION_RELATIVE( editFrame );

    m_dialog->Show( true );

    return 0;
}


int POSITION_RELATIVE_TOOL::RelativeItemSelectionMove( const VECTOR2I& aPosAnchor,
                                                       const VECTOR2I& aTranslation )
{
    VECTOR2I aggregateTranslation = aPosAnchor + aTranslation - GetSelectionAnchorPosition();

    for( EDA_ITEM* item : m_selection )
    {
        if( !item->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

        // Don't move a pad by itself unless editing the footprint
        if( boardItem->Type() == PCB_PAD_T
            && !frame()->GetPcbNewSettings()->m_AllowFreePads
            && frame()->IsType( FRAME_PCB_EDITOR ) )
        {
            boardItem = boardItem->GetParent();
        }

        m_commit->Modify( boardItem );
        boardItem->Move( aggregateTranslation );
    }

    m_commit->Push( _( "Position Relative" ) );

    if( m_selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    canvas()->Refresh();
    return 0;
}


int POSITION_RELATIVE_TOOL::SelectPositionRelativeItem( const TOOL_EVENT& aEvent  )
{
    PCB_PICKER_TOOL*  picker = m_toolMgr->GetTool<PCB_PICKER_TOOL>();
    STATUS_TEXT_POPUP statusPopup( frame() );
    bool              done = false;

    Activate();

    statusPopup.SetText( _( "Click on reference item..." ) );

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

            m_anchor_item = sel.Front();
            statusPopup.Hide();

            if( m_dialog )
                m_dialog->UpdateAnchor( sel.Front() );

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
            statusPopup.Hide();

            if( m_dialog )
                m_dialog->UpdateAnchor( m_anchor_item );
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


void POSITION_RELATIVE_TOOL::setTransitions()
{
    Go( &POSITION_RELATIVE_TOOL::PositionRelative, PCB_ACTIONS::positionRelative.MakeEvent() );
    Go( &POSITION_RELATIVE_TOOL::SelectPositionRelativeItem,
        PCB_ACTIONS::selectpositionRelativeItem.MakeEvent() );
}
