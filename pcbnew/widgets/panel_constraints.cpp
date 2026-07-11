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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <widgets/panel_constraints.h>

#include <wx/listctrl.h>

#include <board.h>
#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/constraint_edit_tool.h>
#include <constraints/pcb_constraint.h>
#include <constraints/board_constraint_adapter.h>
#include <widgets/constraint_list_view.h>


PANEL_CONSTRAINTS::PANEL_CONSTRAINTS( PCB_BASE_EDIT_FRAME* aFrame ) :
        PANEL_CONSTRAINTS_BASE( aFrame ),
        m_frame( aFrame )
{
    AddConstraintListColumns( m_list );

    // Item-cell clicks highlight a single member; the base only wires item-activated/buttons.
    m_list->Bind( wxEVT_LEFT_DOWN, &PANEL_CONSTRAINTS::onLeftDown, this );

    RefreshList();
}


void PANEL_CONSTRAINTS::RefreshList()
{
    BOARD* board = m_frame->GetBoard();

    RefreshList( board ? DiagnoseBoardConstraints( board ) : BOARD_CONSTRAINT_DIAGNOSTICS() );
}


void PANEL_CONSTRAINTS::RefreshList( const BOARD_CONSTRAINT_DIAGNOSTICS& aDiag )
{
    m_rows.clear();

    PopulateConstraintList( m_list, m_frame->GetBoard(), m_frame, aDiag,
                            [&]( long aRow, PCB_CONSTRAINT* aConstraint )
                            {
                                m_rows.push_back( aConstraint->m_Uuid );
                            } );
}


void PANEL_CONSTRAINTS::SelectConstraint( const KIID& aConstraint )
{
    for( long row = 0; row < static_cast<long>( m_rows.size() ); ++row )
    {
        if( m_rows[row] != aConstraint )
            continue;

        m_list->SetItemState( row, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                              wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
        m_list->EnsureVisible( row );
        return;
    }
}


CONSTRAINT_EDIT_TOOL* PANEL_CONSTRAINTS::constraintTool() const
{
    return m_frame->GetToolManager()->GetTool<CONSTRAINT_EDIT_TOOL>();
}


const KIID& PANEL_CONSTRAINTS::rowConstraint( long aRow ) const
{
    if( aRow < 0 || aRow >= static_cast<long>( m_rows.size() ) )
        return niluuid;

    return m_rows[aRow];
}


long PANEL_CONSTRAINTS::columnAt( long aRow, const wxPoint& aPos ) const
{
    for( int col = 0; col < m_list->GetColumnCount(); ++col )
    {
        wxRect rect;

        if( m_list->GetSubItemRect( aRow, col, rect ) && aPos.x >= rect.GetLeft()
            && aPos.x <= rect.GetRight() )
        {
            return col;
        }
    }

    return -1;
}


void PANEL_CONSTRAINTS::onLeftDown( wxMouseEvent& aEvent )
{
    int  flags = 0;
    long row = m_list->HitTest( aEvent.GetPosition(), flags );

    if( row >= 0 && row < static_cast<long>( m_rows.size() ) && constraintTool() )
    {
        // Clicking an item cell highlights just that member; any other cell highlights all.
        long column = columnAt( row, aEvent.GetPosition() );
        int  memberIndex = ( column == CONSTRAINT_COL_ITEM_1 || column == CONSTRAINT_COL_ITEM_2 )
                                   ? static_cast<int>( column )
                                   : -1;
        constraintTool()->HighlightConstraintMembers( m_rows[row], memberIndex );
    }

    aEvent.Skip();
}


void PANEL_CONSTRAINTS::onRowActivated( wxListEvent& aEvent )
{
    // Editing the value is a board mutation, so hand it to the tool; it commits and refreshes us.
    if( CONSTRAINT_EDIT_TOOL* tool = constraintTool() )
        tool->EditConstraintById( rowConstraint( aEvent.GetIndex() ) );
}


void PANEL_CONSTRAINTS::onDelete( wxCommandEvent& aEvent )
{
    long row = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( CONSTRAINT_EDIT_TOOL* tool = constraintTool() )
        tool->RemoveConstraintById( rowConstraint( row ) );

    // Select the last row so pressing Delete again removes the next constraint.
    if( long count = m_list->GetItemCount(); count > 0 )
    {
        m_list->SetItemState( count - 1, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED,
                              wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED );
        m_list->EnsureVisible( count - 1 );
    }
}


void PANEL_CONSTRAINTS::onRefresh( wxCommandEvent& aEvent )
{
    RefreshList();
}
