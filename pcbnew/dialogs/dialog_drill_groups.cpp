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

#include "dialog_drill_groups.h"

#include <algorithm>

#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <drill/drill_symbol_assigner.h>
#include <pad.h>
#include <pcb_edit_frame.h>
#include <pcb_track.h>
#include <widgets/wx_grid.h>

#include <wx/grid.h>


namespace
{

enum GROUP_GRID_COL
{
    COL_SYMBOL = 0,
    COL_SIZE,
    COL_PLATING,
    COL_SPAN,
    COL_OPS,
    COL_SITES
};


wxString markText( const DRILL_SYMBOL_ASSIGNMENT& aSymbol, int aDiameter )
{
    switch( aSymbol.m_MarkMode )
    {
    case DRILL_MARK_MODE::LETTER:    return aSymbol.m_Letter;
    case DRILL_MARK_MODE::SIZE_TEXT: return wxString::Format( wxT( "%.2f" ),
                                                              pcbIUScale.IUTomm( aDiameter ) );
    case DRILL_MARK_MODE::SHAPE:     break;
    }

    return wxString::Format( _( "Shape %d" ), aSymbol.m_ShapeIndex + 1 );
}

} // namespace


DIALOG_DRILL_GROUPS::DIALOG_DRILL_GROUPS( PCB_EDIT_FRAME* aFrame ) :
        DIALOG_DRILL_GROUPS_BASE( aFrame ),
        m_frame( aFrame ),
        m_loading( false ),
        m_detailRow( -1 ),
        m_reloadPending( false )
{
    m_groupGrid->SetColLabelValue( COL_SYMBOL, _( "Symbol" ) );
    m_groupGrid->SetColLabelValue( COL_SIZE, _( "Size" ) );
    m_groupGrid->SetColLabelValue( COL_PLATING, _( "Plated" ) );
    m_groupGrid->SetColLabelValue( COL_SPAN, _( "From / To" ) );
    m_groupGrid->SetColLabelValue( COL_OPS, _( "Ops" ) );
    m_groupGrid->SetColLabelValue( COL_SITES, _( "Sites" ) );

    m_groupGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_groupGrid->EnableEditing( false );

    m_groupGrid->Bind( wxEVT_GRID_SELECT_CELL, &DIALOG_DRILL_GROUPS::onGridSelect, this );

    // Modeless, so closing has to actually destroy. The default would only hide and the
    // owner's weak reference would keep pointing at a live but invisible dialog
    Bind( wxEVT_CLOSE_WINDOW,
          [this]( wxCloseEvent& aEvent )
          {
              commitDetail( m_detailRow );
              Destroy();
          } );

    m_frame->GetBoard()->AddListener( this );

    Reload();
    finishDialogSettings();
}


DIALOG_DRILL_GROUPS::~DIALOG_DRILL_GROUPS()
{
    if( m_frame && m_frame->GetBoard() )
        m_frame->GetBoard()->RemoveListener( this );
}


void DIALOG_DRILL_GROUPS::scheduleReload()
{
    if( m_reloadPending )
        return;

    // Deferred, because the notification can arrive part way through a commit and the board
    // is not necessarily coherent yet
    m_reloadPending = true;

    CallAfter(
            [this]()
            {
                m_reloadPending = false;
                Reload();
            } );
}


void DIALOG_DRILL_GROUPS::Reload()
{
    // Whatever is in the detail pane belongs to the row we are about to discard
    commitDetail( m_detailRow );

    BOARD*                      board = m_frame->GetBoard();
    const DRILL_SYMBOL_PROFILE& profile = board->GetDesignSettings().GetDrillSymbolProfile();

    m_profileName->SetLabel( profile.GetName().IsEmpty() ? _( "Default" ) : profile.GetName() );
    m_freezeAssignments->SetValue( profile.GetFreezeAssignments() );

    DRILL_CHART_MODEL model( profile );
    model.Build( *board, EnumerateDrillSpans( *board ) );

    m_groups = model.Groups();

    // Symbols come from the resolver, not the profile, so a board where no chart has ever
    // been placed still shows the marks its map would draw
    const std::shared_ptr<const DRILL_SYMBOL_CACHE> cache = board->DrillSymbolCache();

    for( DRILL_CHART_GROUP& group : m_groups )
    {
        const auto it = cache->m_ByGroup.find( group.m_SymbolKey );

        if( it != cache->m_ByGroup.end() )
            group.m_Symbol = it->second;
    }

    if( m_groupGrid->GetNumberRows() )
        m_groupGrid->DeleteRows( 0, m_groupGrid->GetNumberRows() );

    m_groupGrid->AppendRows( static_cast<int>( m_groups.size() ) );

    for( size_t row = 0; row < m_groups.size(); ++row )
    {
        const DRILL_CHART_GROUP& group = m_groups[row];
        const int                r = static_cast<int>( row );

        m_groupGrid->SetCellValue( r, COL_SYMBOL, markText( group.m_Symbol, group.m_Diameter ) );

        if( group.m_IsSlot )
        {
            m_groupGrid->SetCellValue( r, COL_SIZE,
                                       wxString::Format( wxT( "%.3f x %.3f" ),
                                                         pcbIUScale.IUTomm( group.m_SizeXY.x ),
                                                         pcbIUScale.IUTomm( group.m_SizeXY.y ) ) );
        }
        else
        {
            m_groupGrid->SetCellValue( r, COL_SIZE,
                                       wxString::Format( wxT( "%.3f" ),
                                                         pcbIUScale.IUTomm( group.m_Diameter ) ) );
        }

        m_groupGrid->SetCellValue( r, COL_PLATING,
                                   group.m_NotPlated ? _( "No" ) : _( "Yes" ) );
        m_groupGrid->SetCellValue( r, COL_SPAN,
                                   wxString::Format( wxT( "%s / %s" ),
                                                     LSET::Name( group.m_TopLayer ),
                                                     LSET::Name( group.m_BottomLayer ) ) );
        m_groupGrid->SetCellValue( r, COL_OPS,
                                   wxString::Format( wxT( "%d" ), group.m_OperationCount ) );
        m_groupGrid->SetCellValue( r, COL_SITES,
                                   wxString::Format( wxT( "%d" ), group.m_SiteCount ) );
    }

    m_groupGrid->AutoSizeColumns();

    showDetail( m_groups.empty() ? -1 : 0 );
}


const DRILL_CHART_GROUP* DIALOG_DRILL_GROUPS::selectedGroup() const
{
    const int row = m_groupGrid->GetGridCursorRow();

    if( row < 0 || row >= static_cast<int>( m_groups.size() ) )
        return nullptr;

    return &m_groups[row];
}


void DIALOG_DRILL_GROUPS::showDetail( int aRow )
{
    m_loading = true;

    const bool valid = aRow >= 0 && aRow < static_cast<int>( m_groups.size() );

    m_markCtrl->Enable( valid );
    m_shapeCtrl->Enable( valid );
    m_letterCtrl->Enable( valid );
    m_descriptionCtrl->Enable( valid );
    m_flashButton->Enable( valid );

    if( valid )
    {
        const DRILL_CHART_GROUP& group = m_groups[aRow];

        m_markCtrl->SetSelection( static_cast<int>( group.m_Symbol.m_MarkMode ) );
        m_shapeCtrl->SetValue( group.m_Symbol.m_ShapeIndex );
        m_letterCtrl->SetValue( group.m_Symbol.m_Letter );
        m_descriptionCtrl->SetValue( group.m_Symbol.m_Description );

        // The cursor column is -1 until the grid has one, and moving to an invalid cell leaves
        // the grid re-sending the selection
        m_groupGrid->SetGridCursor( aRow, std::max( 0, m_groupGrid->GetGridCursorCol() ) );
        m_groupGrid->SelectRow( aRow );
    }

    m_detailRow = valid ? aRow : -1;
    m_loading = false;
}


void DIALOG_DRILL_GROUPS::commitDetail( int aRow )
{
    if( m_loading || aRow < 0 || aRow >= static_cast<int>( m_groups.size() ) )
        return;

    const DRILL_CHART_GROUP* group = &m_groups[aRow];

    DRILL_SYMBOL_ASSIGNMENT assignment;
    assignment.m_MarkMode = static_cast<DRILL_MARK_MODE>( m_markCtrl->GetSelection() );
    assignment.m_ShapeIndex = m_shapeCtrl->GetValue();
    assignment.m_Letter = m_letterCtrl->GetValue();
    assignment.m_Description = m_descriptionCtrl->GetValue();

    BOARD* board = m_frame->GetBoard();

    if( const DRILL_SYMBOL_ASSIGNMENT* existing =
                board->GetDesignSettings().GetDrillSymbolProfile().GetAssignment( group->m_SymbolKey ) )
    {
        const bool same = existing->m_MarkMode == assignment.m_MarkMode
                          && existing->m_ShapeIndex == assignment.m_ShapeIndex
                          && existing->m_Letter == assignment.m_Letter
                          && existing->m_Description == assignment.m_Description;

        if( same )
            return;
    }

    board->GetDesignSettings().GetDrillSymbolProfile().SetAssignment( group->m_SymbolKey, assignment );
    board->BumpDrillModelGeneration();

    // The profile lives in board design settings, which KiCad does not put on the undo
    // stack, so the least this can do is mark the board dirty
    m_frame->OnModify();
    m_frame->RefreshDrillSymbols( KIGFX::REPAINT );
}


void DIALOG_DRILL_GROUPS::onGridSelect( wxGridEvent& aEvent )
{
    // showDetail() moves the cursor itself and lands back here, so without m_loading telling
    // our own selection from the user's the two recurse until the stack runs out
    if( !m_loading )
    {
        // The row being left owns whatever is in the detail pane. Without this its edits are
        // simply overwritten by the incoming row
        commitDetail( m_detailRow );
        showDetail( aEvent.GetRow() );
    }

    aEvent.Skip();
}


void DIALOG_DRILL_GROUPS::onFreezeChanged( wxCommandEvent& aEvent )
{
    BOARD* board = m_frame->GetBoard();
    board->GetDesignSettings().GetDrillSymbolProfile().SetFreezeAssignments(
            m_freezeAssignments->GetValue() );
    board->BumpDrillModelGeneration();

    m_frame->OnModify();
    m_frame->RefreshDrillSymbols( KIGFX::REPAINT );
}


void DIALOG_DRILL_GROUPS::onFlash( wxCommandEvent& aEvent )
{
    const DRILL_CHART_GROUP* group = selectedGroup();

    if( !group )
        return;

    commitDetail( m_detailRow );

    BOARD*                   board = m_frame->GetBoard();
    std::vector<BOARD_ITEM*> items;

    for( const DRILL_OPERATION_ID& id : group->m_Members )
    {
        if( BOARD_ITEM* item = board->ResolveItem( id.m_Owner, true ) )
            items.push_back( item );
    }

    // Brightens rather than selects, so the canvas selection the user already had survives
    if( !items.empty() )
        m_frame->FocusOnItems( items );
}



void DIALOG_DRILL_GROUPS::onClose( wxCommandEvent& aEvent )
{
    commitDetail( m_detailRow );
    Close();
}
