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
using namespace std::placeholders;

#include "position_relative_tool.h"
#include "pcb_actions.h"
#include "selection_tool.h"
#include "edit_tool.h"
#include "pcbnew_picker_tool.h"
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
        m_commit.reset( new BOARD_COMMIT( this ) );
}


bool POSITION_RELATIVE_TOOL::Init()
{
    // Find the selection tool, so they can cooperate
    m_selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    return m_selectionTool != nullptr;
}


int POSITION_RELATIVE_TOOL::PositionRelative( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME*         editFrame = getEditFrame<PCB_BASE_FRAME>();

    const auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
            { EditToolSelectionFilter( aCollector, EXCLUDE_LOCKED | EXCLUDE_TRANSIENTS ); } );

    if( selection.Empty() )
        return 0;

    m_selection = selection;

    if( !m_dialog )
        m_dialog = new DIALOG_POSITION_RELATIVE( editFrame, m_translation, m_anchor );

    m_dialog->Show( true );

    return 0;
}


int POSITION_RELATIVE_TOOL::RelativeItemSelectionMove( wxPoint aPosAnchor, wxPoint aTranslation )
{
    wxPoint aSelAnchor( INT_MAX, INT_MAX );

    // Find top-left item anchor in selection
    for( auto item : m_selection )
    {
        wxPoint itemAnchor = static_cast<BOARD_ITEM*>( item )->GetPosition();

        if( EuclideanNorm( itemAnchor ) < EuclideanNorm( aSelAnchor ) )
            aSelAnchor = itemAnchor;
    }

    wxPoint aggregateTranslation = aPosAnchor + aTranslation - aSelAnchor;

    for( auto item : m_selection )
    {
        // Don't move a pad by itself unless editing the footprint
        if( item->Type() == PCB_PAD_T && frame()->IsType( FRAME_PCB ) )
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
    std::string         tool = "pcbnew.PositionRelative.selectReferenceItem";
    PCBNEW_PICKER_TOOL* picker = m_toolMgr->GetTool<PCBNEW_PICKER_TOOL>();
    STATUS_TEXT_POPUP   statusPopup( frame() );
    bool                done = false;

    Activate();

    statusPopup.SetText( _( "Select reference item..." ) );

    picker->SetClickHandler(
        [&]( const VECTOR2D& aPoint ) -> bool
        {
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
            const PCBNEW_SELECTION& sel = m_selectionTool->RequestSelection(
                []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector )
                {
                    EditToolSelectionFilter( aCollector, EXCLUDE_TRANSIENTS );
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
        Wait();

    return 0;
}


void POSITION_RELATIVE_TOOL::setTransitions()
{
    Go( &POSITION_RELATIVE_TOOL::PositionRelative, PCB_ACTIONS::positionRelative.MakeEvent() );
    Go( &POSITION_RELATIVE_TOOL::SelectPositionRelativeItem,
            PCB_ACTIONS::selectpositionRelativeItem.MakeEvent() );
}
