/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012-18 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <grid_tricks.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>
#include <widgets/grid_readonly_text_helpers.h>


// It works for table data on clipboard for an Excell spreadsheet,
// why not us too for now.
#define COL_SEP     wxT( '\t' )
#define ROW_SEP     wxT( '\n' )


GRID_TRICKS::GRID_TRICKS( wxGrid* aGrid ):
    m_grid( aGrid )
{
    m_sel_row_start = 0;
    m_sel_col_start = 0;
    m_sel_row_count = 0;
    m_sel_col_count = 0;

    m_showEditorOnMouseUp = false;

    aGrid->Connect( wxEVT_GRID_CELL_LEFT_CLICK, wxGridEventHandler( GRID_TRICKS::onGridCellLeftClick ), NULL, this );
    aGrid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK, wxGridEventHandler( GRID_TRICKS::onGridCellLeftDClick ), NULL, this );
    aGrid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( GRID_TRICKS::onGridCellRightClick ), NULL, this );
    aGrid->Connect( wxEVT_GRID_LABEL_RIGHT_CLICK, wxGridEventHandler( GRID_TRICKS::onGridLabelRightClick ), NULL, this );
    aGrid->Connect( GRIDTRICKS_FIRST_ID, GRIDTRICKS_LAST_ID, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GRID_TRICKS::onPopupSelection ), NULL, this );
    aGrid->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( GRID_TRICKS::onKeyDown ), NULL, this );
    aGrid->GetGridWindow()->Connect( wxEVT_LEFT_UP, wxMouseEventHandler( GRID_TRICKS::onMouseUp ), NULL, this );
    aGrid->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( GRID_TRICKS::onUpdateUI ), NULL, this );
}


bool GRID_TRICKS::toggleCell( int aRow, int aCol )
{
    auto renderer = m_grid->GetCellRenderer( aRow, aCol );
    bool isCheckbox = ( dynamic_cast<wxGridCellBoolRenderer*>( renderer ) != nullptr );
    renderer->DecRef();

    if( isCheckbox )
    {
        m_grid->SetGridCursor( aRow, aCol );

        wxGridTableBase* model = m_grid->GetTable();

        if( model->CanGetValueAs( aRow, aCol, wxGRID_VALUE_BOOL )
                && model->CanSetValueAs( aRow, aCol, wxGRID_VALUE_BOOL ))
        {
            model->SetValueAsBool( aRow, aCol, !model->GetValueAsBool( aRow, aCol ));
        }
        else    // fall back to string processing
        {
            if( model->GetValue( aRow, aCol )  == wxT( "1" ) )
                model->SetValue( aRow, aCol, wxT( "0" ) );
            else
                model->SetValue( aRow, aCol, wxT( "1" ) );
        }

        // Mac needs this for the keyboard events; Linux appears to always need it.
        m_grid->ForceRefresh();

        // Let any clients know
        wxGridEvent event( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, aRow, aCol );
        event.SetString( model->GetValue( aRow, aCol ) );
        m_grid->GetEventHandler()->ProcessEvent( event );

        return true;
    }

    return false;
}


bool GRID_TRICKS::showEditor( int aRow, int aCol )
{
    m_grid->SetGridCursor( aRow, aCol );

    if( m_grid->IsEditable() && !m_grid->IsReadOnly( aRow, aCol ) )
    {
        if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
            m_grid->SelectRow( aRow );

        // For several reasons we can't enable the control here.  There's the whole
        // SetInSetFocus() issue/hack in wxWidgets, and there's also wxGrid's MouseUp
        // handler which doesn't notice it's processing a MouseUp until after it has
        // disabled the editor yet again.  So we wait for the MouseUp.
        m_showEditorOnMouseUp = true;

        return true;
    }

    return false;
}


void GRID_TRICKS::onGridCellLeftClick( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    // Activate editor only if a cursor is placed on the clicked cell
    if( !aEvent.GetModifiers() &&
        m_grid->GetGridCursorRow() == row &&
        m_grid->GetGridCursorCol() == col )
    {
        if( toggleCell( row, col ) )
            return;

        if( showEditor( row, col ) )
            return;
    }

    aEvent.Skip();
}


void GRID_TRICKS::onGridCellLeftDClick( wxGridEvent& aEvent )
{
    if( !handleDoubleClick( aEvent ) )
        onGridCellLeftClick( aEvent );
}


void GRID_TRICKS::onMouseUp( wxMouseEvent& aEvent )
{
    if( m_showEditorOnMouseUp )
    {
        // Some wxGridCellEditors don't have the SetInSetFocus() hack.  Even when they do,
        // it sometimes fails.  Activating the control here seems to avoid those issues.
        m_showEditorOnMouseUp = false;
        // Mouse button can be pressed on one cell but be released on another
        // cell (when range of cells is selecting, for example).
        // So it must be checked.
        wxGridCellCoords curCell = wxGridCellCoords( m_grid->GetGridCursorRow(),
                                                     m_grid->GetGridCursorCol() );
        wxGridCellCoords eventCell = m_grid->XYToCell( m_grid->CalcUnscrolledPosition( aEvent.GetPosition() ) );
        if( eventCell == curCell && m_grid->CanEnableCellControl() )
        {
            // Yes, the first of these also shows the control.  Well, at least sometimes.
            // The second call corrects those (as yet undefined) "other times".
            m_grid->EnableCellEditControl();
            m_grid->ShowCellEditControl();
            return;
        }
    }

    aEvent.Skip();
}


bool GRID_TRICKS::handleDoubleClick( wxGridEvent& aEvent )
{
    // Double-click processing must be handled by specific sub-classes
    return false;
}


void GRID_TRICKS::getSelectedArea()
{
    wxGridCellCoordsArray topLeft  = m_grid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray botRight = m_grid->GetSelectionBlockBottomRight();

    wxArrayInt  cols = m_grid->GetSelectedCols();
    wxArrayInt  rows = m_grid->GetSelectedRows();

    if( topLeft.Count() && botRight.Count() )
    {
        m_sel_row_start = topLeft[0].GetRow();
        m_sel_col_start = topLeft[0].GetCol();

        m_sel_row_count = botRight[0].GetRow() - m_sel_row_start + 1;
        m_sel_col_count = botRight[0].GetCol() - m_sel_col_start + 1;
    }
    else if( cols.Count() )
    {
        m_sel_col_start = cols[0];
        m_sel_col_count = cols.Count();
        m_sel_row_start = 0;
        m_sel_row_count = m_grid->GetNumberRows();
    }
    else if( rows.Count() )
    {
        m_sel_col_start = 0;
        m_sel_col_count = m_grid->GetNumberCols();
        m_sel_row_start = rows[0];
        m_sel_row_count = rows.Count();
    }
    else
    {
        m_sel_row_start = m_grid->GetGridCursorRow();
        m_sel_col_start = m_grid->GetGridCursorCol();
        m_sel_row_count = m_sel_row_start >= 0 ? 1 : 0;
        m_sel_col_count = m_sel_col_start >= 0 ? 1 : 0;
    }
}


void GRID_TRICKS::onGridCellRightClick( wxGridEvent&  )
{
    wxMenu menu;

    showPopupMenu( menu );
}


void GRID_TRICKS::onGridLabelRightClick( wxGridEvent&  )
{
    wxMenu menu;

    for( int i = 0; i < m_grid->GetNumberCols(); ++i )
    {
        int id = GRIDTRICKS_FIRST_SHOWHIDE + i;
        menu.AppendCheckItem( id, m_grid->GetColLabelValue( i ) );
        menu.Check( id, m_grid->IsColShown( i ) );
    }

    m_grid->PopupMenu( &menu );
}


void GRID_TRICKS::showPopupMenu( wxMenu& menu )
{
    menu.Append( GRIDTRICKS_ID_CUT,    _( "Cut\tCTRL+X" ),         _( "Clear selected cells placing original contents on clipboard" ) );
    menu.Append( GRIDTRICKS_ID_COPY,   _( "Copy\tCTRL+C" ),        _( "Copy selected cells to clipboard" ) );
    menu.Append( GRIDTRICKS_ID_PASTE,  _( "Paste\tCTRL+V" ),       _( "Paste clipboard cells to matrix at current cell" ) );
    menu.Append( GRIDTRICKS_ID_SELECT, _( "Select All\tCTRL+A" ),  _( "Select all cells" ) );

    getSelectedArea();

    // if nothing is selected, disable cut and copy.
    if( !m_sel_row_count && !m_sel_col_count )
    {
        menu.Enable( GRIDTRICKS_ID_CUT,  false );
        menu.Enable( GRIDTRICKS_ID_COPY, false );
    }

    menu.Enable( GRIDTRICKS_ID_PASTE, false );

    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
            menu.Enable( GRIDTRICKS_ID_PASTE, true );

        wxTheClipboard->Close();
    }

    m_grid->PopupMenu( &menu );
}


void GRID_TRICKS::onPopupSelection( wxCommandEvent& event )
{
    doPopupSelection( event );
}


void GRID_TRICKS::doPopupSelection( wxCommandEvent& event )
{
    int     menu_id = event.GetId();

    // assume getSelectedArea() was called by rightClickPopupMenu() and there's
    // no way to have gotten here without that having been called.

    switch( menu_id )
    {
    case GRIDTRICKS_ID_CUT:
    case GRIDTRICKS_ID_COPY:
        cutcopy( menu_id == GRIDTRICKS_ID_CUT );
        break;

    case GRIDTRICKS_ID_PASTE:
        paste_clipboard();
        break;

    case GRIDTRICKS_ID_SELECT:
        m_grid->SelectAll();
        break;

    default:
        if( menu_id >= GRIDTRICKS_FIRST_SHOWHIDE )
        {
            int col = menu_id - GRIDTRICKS_FIRST_SHOWHIDE;

            if( m_grid->IsColShown( col ) )
                m_grid->HideCol( col );
            else
                m_grid->ShowCol( col );
        }
    }
}


void GRID_TRICKS::onKeyDown( wxKeyEvent& ev )
{
    if( isCtl( 'A', ev ) )
    {
        m_grid->SelectAll();
        return;
    }
    else if( isCtl( 'C', ev ) )
    {
        getSelectedArea();
        cutcopy( false );
        return;
    }
    else if( isCtl( 'V', ev ) )
    {
        getSelectedArea();
        paste_clipboard();
        return;
    }
    else if( isCtl( 'X', ev ) )
    {
        getSelectedArea();
        cutcopy( true );
        return;
    }

    // space-bar toggling of checkboxes
    if( ev.GetKeyCode() == ' ' )
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( m_grid->IsVisible( row, col ) && toggleCell( row, col ) )
            return;
    }

    // shift-return for OK
    if( ev.GetKeyCode() == WXK_RETURN && ev.ShiftDown() )
    {
        wxPostEvent( this, wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK ) );
        return;
    }

    ev.Skip( true );
}


void GRID_TRICKS::paste_clipboard()
{
    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
        {
            wxTextDataObject    data;

            wxTheClipboard->GetData( data );

            paste_text( data.GetText() );
        }

        wxTheClipboard->Close();
        m_grid->ForceRefresh();
    }
}


void GRID_TRICKS::paste_text( const wxString& cb_text )
{
    wxGridTableBase*   tbl = m_grid->GetTable();

    const int cur_row = m_grid->GetGridCursorRow();
    const int cur_col = m_grid->GetGridCursorCol();
    int start_row;
    int end_row;
    int start_col;
    int end_col;
    bool is_selection = false;

    if( cur_row < 0 || cur_col < 0 )
    {
        wxBell();
        return;
    }

    if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
    {
        if( m_sel_row_count > 1 )
            is_selection = true;
    }
    else
    {
        if( m_grid->IsSelection() )
            is_selection = true;
    }

    wxStringTokenizer rows( cb_text, ROW_SEP, wxTOKEN_RET_EMPTY );

    // If selection of cells is present
    // then a clipboard pastes to selected cells only.
    if( is_selection )
    {
        start_row = m_sel_row_start;
        end_row = m_sel_row_start + m_sel_row_count;
        start_col = m_sel_col_start;
        end_col = m_sel_col_start + m_sel_col_count;
    }
    // Otherwise, paste whole clipboard
    // starting from cell with cursor.
    else
    {
        start_row = cur_row;
        end_row = cur_row + rows.CountTokens();

        if( end_row > tbl->GetNumberRows() )
            end_row = tbl->GetNumberRows();

        start_col = cur_col;
        end_col = start_col; // end_col actual value calculates later
    }

    for( int row = start_row;  row < end_row;  ++row )
    {
        // If number of selected rows bigger than count of rows in
        // the clipboard, paste from the clipboard again and again
        // while end of the selection is reached.
        if( !rows.HasMoreTokens() )
            rows.SetString( cb_text, ROW_SEP, wxTOKEN_RET_EMPTY );

        wxString rowTxt = rows.GetNextToken();

        wxStringTokenizer cols( rowTxt, COL_SEP, wxTOKEN_RET_EMPTY );

        if( !is_selection )
        {
            end_col = cur_col + cols.CountTokens();

            if( end_col > tbl->GetNumberCols() )
                end_col = tbl->GetNumberCols();
        }

        for( int col = start_col;  col < end_col;  ++col )
        {
            // If number of selected columns bigger than count of columns in
            // the clipboard, paste from the clipboard again and again while
            // end of the selection is reached.
            if( !cols.HasMoreTokens() )
                cols.SetString( rowTxt, COL_SEP, wxTOKEN_RET_EMPTY );

            wxString cellTxt = cols.GetNextToken();
            tbl->SetValue( row, col, cellTxt );
        }
    }
}


void GRID_TRICKS::cutcopy( bool doCut )
{
    if( wxTheClipboard->Open() )
    {
        wxGridTableBase*    tbl = m_grid->GetTable();
        wxString            txt;

        // fill txt with a format that is compatible with most spreadsheets
        for( int row = m_sel_row_start;  row < m_sel_row_start + m_sel_row_count;  ++row )
        {
            for( int col = m_sel_col_start;  col < m_sel_col_start + m_sel_col_count; ++col )
            {
                txt += tbl->GetValue( row, col );

                if( col < m_sel_col_start + m_sel_col_count - 1 )   // that was not last column
                    txt += COL_SEP;

                if( doCut )
                    tbl->SetValue( row, col, wxEmptyString );
            }
            txt += ROW_SEP;
        }

        wxTheClipboard->SetData( new wxTextDataObject( txt ) );
        wxTheClipboard->Close();

        if( doCut )
            m_grid->ForceRefresh();
    }
}


void GRID_TRICKS::onUpdateUI( wxUpdateUIEvent& event )
{
    // Respect ROW selectionMode when moving cursor

    if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
    {
        int cursorRow = m_grid->GetGridCursorRow();
        bool cursorInSelectedRow = false;

        for( int row : m_grid->GetSelectedRows() )
        {
            if( row == cursorRow )
            {
                cursorInSelectedRow = true;
                break;
            }
        }

        if( !cursorInSelectedRow )
            m_grid->SelectRow( cursorRow );
    }
}
