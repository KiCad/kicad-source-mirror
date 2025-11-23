/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lib_table_grid_tricks.h"
#include "confirm.h"
#include "lib_table_grid_data_model.h"
#include <libraries/library_manager.h>
#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <lib_id.h>


LIB_TABLE_GRID_TRICKS::LIB_TABLE_GRID_TRICKS( WX_GRID* aGrid ) :
        GRID_TRICKS( aGrid )
{
    m_grid->Disconnect( wxEVT_CHAR_HOOK );
    m_grid->Connect( wxEVT_CHAR_HOOK, wxCharEventHandler( LIB_TABLE_GRID_TRICKS::onCharHook ), nullptr, this );
}


LIB_TABLE_GRID_TRICKS::LIB_TABLE_GRID_TRICKS( WX_GRID* aGrid,
        std::function<void( wxCommandEvent& )> aAddHandler ) :
        GRID_TRICKS( aGrid, aAddHandler )
{
    m_grid->Disconnect( wxEVT_CHAR_HOOK );
    m_grid->Connect( wxEVT_CHAR_HOOK, wxCharEventHandler( LIB_TABLE_GRID_TRICKS::onCharHook ), nullptr, this );
}


void LIB_TABLE_GRID_TRICKS::onGridCellLeftClick( wxGridEvent& aEvent )
{
    if( aEvent.GetCol() == COL_STATUS )
    {
        // Status column button action depends on row:
        // Normal rows should have no button, so they are a no-op
        // Errored rows should have the warning button, so we show their error
        // Configurable libraries will have the options button, so we launch the config
        // Chained tables will have the open button, so we request the table be opened
        LIB_TABLE_GRID_DATA_MODEL* table = static_cast<LIB_TABLE_GRID_DATA_MODEL*>( m_grid->GetTable() );
        const LIBRARY_TABLE_ROW&   row = table->at( aEvent.GetRow() );
        LIBRARY_MANAGER_ADAPTER*   adapter = table->Adapter();

        wxString title = row.Type() == "Table"
                                ? wxString::Format( _( "Error loading library table '%s'" ), row.Nickname() )
                                : wxString::Format( _( "Error loading library '%s'" ), row.Nickname() );

        if( !row.IsOk() )
        {
            DisplayErrorMessage( m_grid, title, row.ErrorDescription() );
        }
        else if( std::optional<LIBRARY_ERROR> e = adapter->LibraryError( row.Nickname() ) )
        {
            DisplayErrorMessage( m_grid, title, e->message );
        }
        else if( adapter->SupportsConfigurationDialog( row.Nickname() ) )
        {
            adapter->ShowConfigurationDialog( row.Nickname(), wxGetTopLevelParent( m_grid ) );
        }
        else if( row.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
        {
            openTable( row );
        }

        aEvent.Skip();
    }
    else
    {
        GRID_TRICKS::onGridCellLeftClick( aEvent );
    }
}


void LIB_TABLE_GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    menu.Append( LIB_TABLE_GRID_TRICKS_OPTIONS_EDITOR,
                 _( "Edit Options" ),
                 _( "Edit options for this library entry" ) );

    menu.AppendSeparator();

    bool            showActivate = false;
    bool            showDeactivate = false;
    bool            showSetVisible = false;
    bool            showUnsetVisible = false;
    LIB_TABLE_GRID_DATA_MODEL* tbl = static_cast<LIB_TABLE_GRID_DATA_MODEL*>( m_grid->GetTable() );

    // Ensure selection parameters are up to date
    getSelectedArea();

    for( int row = m_sel_row_start; row < m_sel_row_start + m_sel_row_count; ++row )
    {
        if( tbl->GetValueAsBool( row, 0 ) )
            showDeactivate = true;
        else
            showActivate = true;

        if( tbl->GetValueAsBool( row, 1 ) )
            showUnsetVisible = true;
        else
            showSetVisible = true;

        if( showActivate && showDeactivate && showSetVisible && showUnsetVisible )
            break;
    }

    if( showActivate )
        menu.Append( LIB_TABLE_GRID_TRICKS_ACTIVATE_SELECTED, _( "Activate Selected" ) );

    if( showDeactivate )
        menu.Append( LIB_TABLE_GRID_TRICKS_DEACTIVATE_SELECTED, _( "Deactivate Selected" ) );

    if( supportsVisibilityColumn() )
    {
        if( showSetVisible )
            menu.Append( LIB_TABLE_GRID_TRICKS_SET_VISIBLE, _( "Set Visible Flag" ) );

        if( showUnsetVisible )
            menu.Append( LIB_TABLE_GRID_TRICKS_UNSET_VISIBLE, _( "Unset Visible Flag" ) );
    }

    bool showSettings = false;
    bool showOpen = false;

    if( LIBRARY_MANAGER_ADAPTER* adapter = tbl->Adapter() )
    {
        wxString nickname = tbl->GetValue( m_sel_row_start, COL_NICKNAME );

        if( m_sel_row_count == 1 && adapter->SupportsConfigurationDialog( nickname ) )
        {
            showSettings = true;
            menu.Append( LIB_TABLE_GRID_TRICKS_OPEN_TABLE, _( "Edit Settings" ) );
        }

        std::optional<LIBRARY_TABLE_ROW*> row = adapter->GetRow( nickname );

        if( row.has_value() && row.value()->Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
        {
            showOpen = true;
            menu.Append( LIB_TABLE_GRID_TRICKS_LIBRARY_SETTINGS, _( "Open Library Table" ) );
        }
    }

    if( showActivate || showDeactivate || showSetVisible || showUnsetVisible || showSettings || showOpen )
        menu.AppendSeparator();

    GRID_TRICKS::showPopupMenu( menu, aEvent );
}


void LIB_TABLE_GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    int menu_id = event.GetId();
    LIB_TABLE_GRID_DATA_MODEL* tbl = (LIB_TABLE_GRID_DATA_MODEL*) m_grid->GetTable();

    if( menu_id == LIB_TABLE_GRID_TRICKS_OPTIONS_EDITOR )
    {
        optionsEditor( m_grid->GetGridCursorRow() );
    }
    else if( menu_id == LIB_TABLE_GRID_TRICKS_ACTIVATE_SELECTED
            || menu_id == LIB_TABLE_GRID_TRICKS_DEACTIVATE_SELECTED )
    {
        bool selected_state = menu_id == LIB_TABLE_GRID_TRICKS_ACTIVATE_SELECTED;

        for( int row = m_sel_row_start; row < m_sel_row_start + m_sel_row_count; ++row )
            tbl->SetValueAsBool( row, 0, selected_state );

        // Ensure the new state (on/off) of the widgets is immediately shown:
        m_grid->Refresh();
    }
    else if( menu_id == LIB_TABLE_GRID_TRICKS_SET_VISIBLE
            || menu_id == LIB_TABLE_GRID_TRICKS_UNSET_VISIBLE )
    {
        bool selected_state = menu_id == LIB_TABLE_GRID_TRICKS_SET_VISIBLE;

        for( int row = m_sel_row_start; row < m_sel_row_start + m_sel_row_count; ++row )
            tbl->SetValueAsBool( row, 1, selected_state );

        // Ensure the new state (on/off) of the widgets is immediately shown:
        m_grid->Refresh();
    }
    else if( menu_id == LIB_TABLE_GRID_TRICKS_LIBRARY_SETTINGS )
    {
        // TODO(JE) library tables
#if 0
        LIB_TABLE_ROW* row = tbl->At( m_sel_row_start );
        row->Refresh();
        row->ShowSettingsDialog( m_grid->GetParent() );
#endif
    }
    else if( menu_id == LIB_TABLE_GRID_TRICKS_OPEN_TABLE )
    {
        openTable( tbl->At( m_sel_row_start ) );
    }
    else
    {
        GRID_TRICKS::doPopupSelection( event );
    }
}


void LIB_TABLE_GRID_TRICKS::onCharHook( wxKeyEvent& ev )
{
    if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'V' && m_grid->IsCellEditControlShown() )
    {
        wxLogNull doNotLog;

        if( wxTheClipboard->Open() )
        {
            if( wxTheClipboard->IsSupported( wxDF_TEXT ) || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
            {
                wxTextDataObject data;
                wxTheClipboard->GetData( data );

                wxString text = data.GetText();

                if( !text.Contains( '\t' ) && text.Contains( ',' ) )
                    text.Replace( ',', '\t' );

                if( text.Contains( '\t' ) || text.Contains( '\n' ) || text.Contains( '\r' ) )
                {
                    m_grid->CancelPendingChanges();
                    int row = m_grid->GetGridCursorRow();

                    // Check if the current row already has data (has a nickname)
                    wxGridTableBase* table = m_grid->GetTable();
                    if( table && row >= 0 && row < table->GetNumberRows() )
                    {
                        // Check if the row has a nickname (indicating it has existing data)
                        wxString nickname = table->GetValue( row, COL_NICKNAME );
                        if( !nickname.IsEmpty() )
                        {
                            // Row already has data, don't allow pasting over it
                            wxTheClipboard->Close();
                            wxBell(); // Provide audio feedback
                            return;
                        }
                    }

                    m_grid->ClearSelection();
                    m_grid->SelectRow( row );
                    m_grid->SetGridCursor( row, 0 );
                    getSelectedArea();
                    paste_text( text );
                    wxTheClipboard->Close();
                    m_grid->ForceRefresh();
                    return;
                }
            }

            wxTheClipboard->Close();
        }
    }

    GRID_TRICKS::onCharHook( ev );
}


/*
 * Handle specialized clipboard text, either s-expr syntax starting with a lib table preamble
 * (such as "(fp_lib_table"), or spreadsheet formatted text.
 */
void LIB_TABLE_GRID_TRICKS::paste_text( const wxString& cb_text )
{
    LIB_TABLE_GRID_DATA_MODEL* tbl = static_cast<LIB_TABLE_GRID_DATA_MODEL*>( m_grid->GetTable() );

    if( size_t ndx = cb_text.find( getTablePreamble() ); ndx != std::string::npos )
    {
        // paste the LIB_TABLE_ROWs of s-expr, starting at column 0 regardless of current cursor column.

        if( LIBRARY_TABLE tempTable( cb_text, tbl->Table().Scope() ); tempTable.IsOk() )
        {
            std::ranges::copy( tempTable.Rows(),
                               std::inserter( tbl->Table().Rows(), tbl->Table().Rows().begin() ) );

            if( tbl->GetView() )
            {
                wxGridTableMessage msg( tbl, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, 0, 0 );
                tbl->GetView()->ProcessTableMessage( msg );
            }
        }
        else
        {
            DisplayError( wxGetTopLevelParent( m_grid ), tempTable.ErrorDescription() );
        }
    }
    else
    {
        wxString text = cb_text;

        if( !text.Contains( '\t' ) && text.Contains( ',' ) )
            text.Replace( ',', '\t' );

        if( text.Contains( '\t' ) )
        {
            int row = m_grid->GetGridCursorRow();
            m_grid->ClearSelection();
            m_grid->SelectRow( row );
            m_grid->SetGridCursor( row, 0 );
            getSelectedArea();
        }

        GRID_TRICKS::paste_text( text );

        m_grid->AutoSizeColumns( false );
    }

    m_grid->AutoSizeColumns( false );
}


bool LIB_TABLE_GRID_TRICKS::handleDoubleClick( wxGridEvent& aEvent )
{
    if( aEvent.GetCol() == COL_OPTIONS )
    {
        optionsEditor( aEvent.GetRow() );
        return true;
    }

    return false;
}


void LIB_TABLE_GRID_TRICKS::AppendRowHandler( WX_GRID* aGrid )
{
    aGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                    aGrid->AppendRows( 1 );
                return { aGrid->GetNumberRows() - 1, COL_NICKNAME };
            } );
}


void LIB_TABLE_GRID_TRICKS::DeleteRowHandler( WX_GRID* aGrid )
{
    if( !aGrid->CommitPendingChanges() )
        return;

    wxGridUpdateLocker noUpdates( aGrid );

    int curRow = aGrid->GetGridCursorRow();
    int curCol = aGrid->GetGridCursorCol();

    // In a wxGrid, collect rows that have a selected cell, or are selected
    // It is not so easy: it depends on the way the selection was made.
    // Here, we collect rows selected by clicking on a row label, and rows that contain
    // previously-selected cells.
    // If no candidate, just delete the row with the grid cursor.
    wxArrayInt selectedRows	= aGrid->GetSelectedRows();
    wxGridCellCoordsArray cells = aGrid->GetSelectedCells();
    wxGridCellCoordsArray blockTopLeft = aGrid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray blockBotRight = aGrid->GetSelectionBlockBottomRight();

    // Add all row having cell selected to list:
    for( unsigned ii = 0; ii < cells.GetCount(); ii++ )
        selectedRows.Add( cells[ii].GetRow() );

    // Handle block selection
    if( !blockTopLeft.IsEmpty() && !blockBotRight.IsEmpty() )
    {
        for( int i = blockTopLeft[0].GetRow(); i <= blockBotRight[0].GetRow(); ++i )
            selectedRows.Add( i );
    }

    // Use the row having the grid cursor only if we have no candidate:
    if( selectedRows.size() == 0 && aGrid->GetGridCursorRow() >= 0 )
        selectedRows.Add( aGrid->GetGridCursorRow() );

    if( selectedRows.size() == 0 )
    {
        wxBell();
        return;
    }

    std::sort( selectedRows.begin(), selectedRows.end() );

    // Remove selected rows (note: a row can be stored more than once in list)
    int last_row = -1;

    // Needed to avoid a wxWidgets alert if the row to delete is the last row
    // at least on wxMSW 3.2
    aGrid->ClearSelection();

    for( int ii = selectedRows.GetCount()-1; ii >= 0; ii-- )
    {
        int row = selectedRows[ii];

        if( row != last_row )
        {
            last_row = row;
            aGrid->DeleteRows( row, 1 );
        }
    }

    if( aGrid->GetNumberRows() > 0 && curRow >= 0 )
        aGrid->SetGridCursor( std::min( curRow, aGrid->GetNumberRows() - 1 ), curCol );
}


void LIB_TABLE_GRID_TRICKS::MoveUpHandler( WX_GRID* aGrid )
{
    aGrid->OnMoveRowUp(
            [&]( int row )
            {
                LIB_TABLE_GRID_DATA_MODEL* tbl = static_cast<LIB_TABLE_GRID_DATA_MODEL*>( aGrid->GetTable() );
                int curRow = aGrid->GetGridCursorRow();

                std::vector<LIBRARY_TABLE_ROW>& rows = tbl->Table().Rows();

                auto current = rows.begin() + curRow;
                auto prev    = rows.begin() + curRow - 1;

                std::iter_swap( current, prev );

                // Update the wxGrid
                wxGridTableMessage msg( tbl, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, row - 1, 0 );
                tbl->GetView()->ProcessTableMessage( msg );
            } );
}


void LIB_TABLE_GRID_TRICKS::MoveDownHandler( WX_GRID* aGrid )
{
    aGrid->OnMoveRowDown(
            [&]( int row )
            {
                LIB_TABLE_GRID_DATA_MODEL* tbl = static_cast<LIB_TABLE_GRID_DATA_MODEL*>( aGrid->GetTable() );
                int curRow = aGrid->GetGridCursorRow();
                std::vector<LIBRARY_TABLE_ROW>& rows = tbl->Table().Rows();

                auto current = rows.begin() + curRow;
                auto next    = rows.begin() + curRow + 1;

                std::iter_swap( current, next );

                // Update the wxGrid
                wxGridTableMessage msg( tbl, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, row, 0 );
                tbl->GetView()->ProcessTableMessage( msg );
            } );
}


bool LIB_TABLE_GRID_TRICKS::VerifyTable( WX_GRID* aGrid, std::function<void( int aRow, int aCol )> aErrorHandler )
{
    wxWindow*                  topLevelParent = wxGetTopLevelParent( aGrid );
    LIB_TABLE_GRID_DATA_MODEL* model = static_cast<LIB_TABLE_GRID_DATA_MODEL*>( aGrid->GetTable() );
    wxString                   msg;

    for( int r = 0; r < model->GetNumberRows(); )
    {
        wxString nick = model->GetValue( r, COL_NICKNAME ).Trim( false ).Trim();
        wxString uri  = model->GetValue( r, COL_URI ).Trim( false ).Trim();
        unsigned illegalCh = 0;

        if( !uri )
        {
            // Silently nuke rows that have no libraray URI
            model->DeleteRows( r, 1 );
        }
        else if( !nick || ( illegalCh = LIB_ID::FindIllegalLibraryNameChar( nick ) ) )
        {
            if( !nick )
                msg = _( "Library must have a nickname." );
            else
                msg = wxString::Format( _( "Illegal character '%c' in nickname '%s'." ), illegalCh, nick );

            aErrorHandler( r, COL_NICKNAME );

            wxMessageDialog errdlg( topLevelParent, msg, _( "Library Nickname Error" ) );
            errdlg.ShowModal();
            return false;
        }
        else
        {
            // set the trimmed values back into the table so they get saved to disk.
            model->SetValue( r, COL_NICKNAME, nick );
            model->SetValue( r, COL_URI, uri );

            // Make sure to not save a hidden flag
            model->SetValue( r, COL_VISIBLE, wxS( "1" ) );

            ++r;        // this row was OK.
        }
    }

    // check for duplicate nickNames
    for( int r1 = 0; r1 < model->GetNumberRows() - 1; ++r1 )
    {
        wxString nick1 = model->GetValue( r1, COL_NICKNAME );

        for( int r2 = r1 + 1; r2 < model->GetNumberRows(); ++r2 )
        {
            wxString nick2 = model->GetValue( r2, COL_NICKNAME );

            if( nick1 == nick2 )
            {
                msg = wxString::Format( _( "Multiple libraries cannot share the same nickname ('%s')." ), nick1 );

                // go to the lower of the two rows, it is technically the duplicate:
                aErrorHandler( r2, 1 );

                wxMessageDialog errdlg( topLevelParent, msg, _( "Library Nickname Error" ) );
                errdlg.ShowModal();
                return false;
            }
        }
    }

    return true;
}
