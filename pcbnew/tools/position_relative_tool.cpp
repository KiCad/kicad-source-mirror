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
#include "picker_tool.h"

#include <dialogs/dialog_position_relative.h>

#include <board_commit.h>
#include <hotkeys.h>
#include <bitmaps.h>
#include <confirm.h>


// Position relative tool actions

TOOL_ACTION PCB_ACTIONS::positionRelative( "pcbnew.PositionRelative.positionRelative",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_POSITION_RELATIVE ),
        _( "Position Relative to..." ), _(
                "Positions the selected item(s) by an exact amount relative to another" ),
        move_module_xpm );


TOOL_ACTION PCB_ACTIONS::selectpositionRelativeItem(
        "pcbnew.PositionRelative.selectpositionRelativeItem",
        AS_GLOBAL,
        0,
        "",
        "",
        nullptr );


POSITION_RELATIVE_TOOL::POSITION_RELATIVE_TOOL() :
    PCB_TOOL( "pcbnew.PositionRelative" ), m_position_relative_dialog( NULL ),
    m_selectionTool( NULL ), m_anchor_item( NULL )
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
    m_selectionTool =
        static_cast<SELECTION_TOOL*>( m_toolMgr->FindTool( "pcbnew.InteractiveSelection" ) );

    if( !m_selectionTool )
    {
        DisplayError( NULL, wxT( "pcbnew.InteractiveSelection tool is not available" ) );
        return false;
    }

    return true;
}


int POSITION_RELATIVE_TOOL::PositionRelative( const TOOL_EVENT& aEvent )
{
    const auto& selection = m_selectionTool->RequestSelection();

    if( m_selectionTool->CheckLock() == SELECTION_LOCKED )
        return 0;

    if( selection.Empty() )
        return 0;


    m_position_relative_selection = selection;

    PCB_BASE_FRAME* editFrame = getEditFrame<PCB_BASE_FRAME>();
    m_position_relative_rotation = 0;

    if( !m_position_relative_dialog )
        m_position_relative_dialog = new DIALOG_POSITION_RELATIVE( editFrame,
                m_toolMgr,
                m_position_relative_translation,
                m_position_relative_rotation,
                m_anchor_position );

    m_position_relative_dialog->Show( true );

    return 0;
}


static bool selectPRitem( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition )
{
    SELECTION_TOOL* selectionTool = aToolMgr->GetTool<SELECTION_TOOL>();
    POSITION_RELATIVE_TOOL* positionRelativeTool = aToolMgr->GetTool<POSITION_RELATIVE_TOOL>();
    wxCHECK( selectionTool, false );
    wxCHECK( positionRelativeTool, false );

    aToolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    aToolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );
    selectionTool->SanitizeSelection();

    const SELECTION& selection = selectionTool->GetSelection();

    if( selection.Empty() )
        return true;

    positionRelativeTool->UpdateAnchor( static_cast<BOARD_ITEM*>( selection.Front() ) );

    return true;
}


int POSITION_RELATIVE_TOOL::RelativeItemSelectionMove( wxPoint anchorPosition,
        wxPoint relativePosition,
        double rotation )
{
    VECTOR2I rp = m_position_relative_selection.GetCenter();
    wxPoint rotPoint( rp.x, rp.y );
    wxPoint translation = anchorPosition + relativePosition - rotPoint;

    for( auto item : m_position_relative_selection )
    {
        m_commit->Modify( item );

        static_cast<BOARD_ITEM*>( item )->Rotate( rotPoint, rotation );
        static_cast<BOARD_ITEM*>( item )->Move( translation );
    }

    m_commit->Push( _( "Position Relative" ) );

    if( m_position_relative_selection.IsHover() )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionModified, true );
    return 0;
}


int POSITION_RELATIVE_TOOL::SelectPositionRelativeItem( const TOOL_EVENT& aEvent  )
{
    Activate();

    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();
    assert( picker );

    picker->SetSnapping( false );
    picker->SetClickHandler( std::bind( selectPRitem, m_toolMgr, _1 ) );
    picker->Activate();
    Wait();

    return 0;
}


void POSITION_RELATIVE_TOOL::UpdateAnchor( BOARD_ITEM* aItem )
{
    m_anchor_item = aItem;

    if( m_position_relative_dialog )
        m_position_relative_dialog->UpdateAnchor( aItem );
}


void POSITION_RELATIVE_TOOL::SetTransitions()
{
    Go( &POSITION_RELATIVE_TOOL::PositionRelative, PCB_ACTIONS::positionRelative.MakeEvent() );
    Go( &POSITION_RELATIVE_TOOL::SelectPositionRelativeItem,
            PCB_ACTIONS::selectpositionRelativeItem.MakeEvent() );
}
