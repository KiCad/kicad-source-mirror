/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <grid_tricks.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/tokenzr.h>
#include <wx/clipbrd.h>
#include <wx/log.h>
#include <wx/stc/stc.h>
#include <widgets/grid_text_helpers.h>
#include <search_stack.h>
#include <widgets/grid_text_button_helpers.h>

// It works for table data on clipboard for an Excel spreadsheet,
// why not us too for now.
#define COL_SEP     wxT( '\t' )
#define ROW_SEP     wxT( '\n' )
#define ROW_SEP_R   wxT( '\r' )


GRID_TRICKS::GRID_TRICKS( WX_GRID* aGrid ) :
    m_grid( aGrid ),
    m_addHandler( []( wxCommandEvent& ) {} ),
    m_enableSingleClickEdit( true ),
    m_multiCellEditEnabled( true )
{
    init();
}


GRID_TRICKS::GRID_TRICKS( WX_GRID* aGrid, std::function<void( wxCommandEvent& )> aAddHandler ) :
    m_grid( aGrid ),
    m_addHandler( aAddHandler ),
    m_enableSingleClickEdit( true ),
    m_multiCellEditEnabled( true )
{
    init();
}


void GRID_TRICKS::init()
{
    m_sel_row_start = 0;
    m_sel_col_start = 0;
    m_sel_row_count = 0;
    m_sel_col_count = 0;

    m_grid->Connect( wxEVT_GRID_CELL_LEFT_CLICK,
                     wxGridEventHandler( GRID_TRICKS::onGridCellLeftClick ), nullptr, this );
    m_grid->Connect( wxEVT_GRID_CELL_LEFT_DCLICK,
                     wxGridEventHandler( GRID_TRICKS::onGridCellLeftDClick ), nullptr, this );
    m_grid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK,
                     wxGridEventHandler( GRID_TRICKS::onGridCellRightClick ), nullptr, this );
    m_grid->Connect( wxEVT_GRID_LABEL_RIGHT_CLICK,
                     wxGridEventHandler( GRID_TRICKS::onGridLabelRightClick ), nullptr, this );
    m_grid->Connect( wxEVT_GRID_LABEL_LEFT_CLICK,
                     wxGridEventHandler( GRID_TRICKS::onGridLabelLeftClick ), nullptr, this );
    m_grid->Connect( GRIDTRICKS_FIRST_ID, GRIDTRICKS_LAST_ID, wxEVT_COMMAND_MENU_SELECTED,
                     wxCommandEventHandler( GRID_TRICKS::onPopupSelection ), nullptr, this );
    m_grid->Connect( wxEVT_CHAR_HOOK,
                     wxCharEventHandler( GRID_TRICKS::onCharHook ), nullptr, this );
    m_grid->Connect( wxEVT_KEY_DOWN,
                     wxKeyEventHandler( GRID_TRICKS::onKeyDown ), nullptr, this );
    m_grid->Connect( wxEVT_UPDATE_UI,
                     wxUpdateUIEventHandler( GRID_TRICKS::onUpdateUI ), nullptr, this );

    // The handlers that control the tooltips must be on the actual grid window, not the grid
    m_grid->GetGridWindow()->Connect( wxEVT_MOTION,
                                      wxMouseEventHandler( GRID_TRICKS::onGridMotion ), nullptr,
                                      this );
}


bool GRID_TRICKS::isTextEntry( int aRow, int aCol )
{
    wxGridCellEditor* editor = m_grid->GetCellEditor( aRow, aCol );
    bool              retval = ( dynamic_cast<wxGridCellTextEditor*>( editor )
                              || dynamic_cast<GRID_CELL_STC_EDITOR*>( editor )
                              || dynamic_cast<GRID_CELL_TEXT_BUTTON*>( editor ) );

    editor->DecRef();
    return retval;
}


bool GRID_TRICKS::isCheckbox( int aRow, int aCol )
{
    wxGridCellRenderer* renderer = m_grid->GetCellRenderer( aRow, aCol );
    bool                retval = ( dynamic_cast<wxGridCellBoolRenderer*>( renderer ) );

    renderer->DecRef();
    return retval;
}


bool GRID_TRICKS::isReadOnly( int aRow, int aCol )
{
    return !m_grid->IsEditable() || m_grid->IsReadOnly( aRow, aCol );
}


bool GRID_TRICKS::toggleCell( int aRow, int aCol, bool aPreserveSelection )
{
    if( isCheckbox( aRow, aCol ) )
    {
        if( !aPreserveSelection )
        {
            m_grid->ClearSelection();
            m_grid->SetGridCursor( aRow, aCol );
        }

        wxGridTableBase* model = m_grid->GetTable();

        if( model->CanGetValueAs( aRow, aCol, wxGRID_VALUE_BOOL )
                && model->CanSetValueAs( aRow, aCol, wxGRID_VALUE_BOOL ) )
        {
            model->SetValueAsBool( aRow, aCol, !model->GetValueAsBool( aRow, aCol ) );
        }
        else    // fall back to string processing
        {
            if( model->GetValue( aRow, aCol ) == wxT( "1" ) )
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
    if( m_grid->GetGridCursorRow() != aRow || m_grid->GetGridCursorCol() != aCol )
        m_grid->SetGridCursor( aRow, aCol );

    if( !isReadOnly( aRow, aCol ) )
    {
        m_grid->ClearSelection();

        m_sel_row_start = aRow;
        m_sel_col_start = aCol;
        m_sel_row_count = 1;
        m_sel_col_count = 1;

        if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
        {
            wxArrayInt rows = m_grid->GetSelectedRows();

            if( rows.size() != 1 || rows.Item( 0 ) != aRow )
                m_grid->SelectRow( aRow );
        }

        // For several reasons we can't enable the control here.  There's the whole
        // SetInSetFocus() issue/hack in wxWidgets, and there's also wxGrid's MouseUp
        // handler which doesn't notice it's processing a MouseUp until after it has
        // disabled the editor yet again.  So we re-use wxWidgets' slow-click hack,
        // which is processed later in the MouseUp handler.
        //
        // It should be pointed out that the fact that it's wxWidgets' hack doesn't
        // make it any less of a hack.  Be extra careful with any modifications here.
        // See, in particular, https://bugs.launchpad.net/kicad/+bug/1817965.
        m_grid->ShowEditorOnMouseUp();

        return true;
    }

    return false;
}


void GRID_TRICKS::onGridCellLeftClick( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    // Don't make users click twice to toggle a checkbox or edit a text cell
    if( !aEvent.GetModifiers() )
    {
        bool toggled = false;

        if( toggleCell( row, col, true ) )
            toggled = true;
        else if( m_enableSingleClickEdit && showEditor( row, col ) )
            return;

        // Apply checkbox changes to multi-selection.
        // Non-checkbox changes handled elsewhere
        if( toggled )
        {
            getSelectedArea();

            // We only want to apply this to whole rows.  If the grid allows selecting individual
            // cells, and the selection contains dijoint cells, skip this logic.
            if( !m_grid->GetSelectedCells().IsEmpty() || m_sel_row_count < 2 )
            {
                // We preserved the selection in toggleCell above; so clear it now that we know
                // we aren't doing a multi-select edit
                m_grid->ClearSelection();
                return;
            }

            wxString newVal = m_grid->GetCellValue( row, col );

            for( int otherRow = m_sel_row_start; otherRow < m_sel_row_start + m_sel_row_count; ++otherRow )
            {
                if( otherRow == row )
                    continue;

                m_grid->SetCellValue( otherRow, col, newVal );
            }

            return;
        }
    }

    aEvent.Skip();
}


void GRID_TRICKS::onGridCellLeftDClick( wxGridEvent& aEvent )
{
    if( !handleDoubleClick( aEvent ) )
        onGridCellLeftClick( aEvent );
}


void GRID_TRICKS::onGridMotion( wxMouseEvent& aEvent )
{
    // Always skip the event
    aEvent.Skip();

    wxPoint pt  = aEvent.GetPosition();
    wxPoint pos = m_grid->CalcScrolledPosition( wxPoint( pt.x, pt.y ) );

    int col = m_grid->XToCol( pos.x );
    int row = m_grid->YToRow( pos.y );

    // Empty tooltip if the cell doesn't exist or the column doesn't have tooltips
    if( ( col == wxNOT_FOUND ) || ( row == wxNOT_FOUND ) || !m_tooltipEnabled[col] )
    {
        m_grid->GetGridWindow()->SetToolTip( wxS( "" ) );
        return;
    }

    // Set the tooltip to the string contained in the cell
    m_grid->GetGridWindow()->SetToolTip( m_grid->GetCellValue( row, col ) );
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


void GRID_TRICKS::onGridCellRightClick( wxGridEvent& aEvent  )
{
    wxMenu menu;

    showPopupMenu( menu, aEvent );
}


void GRID_TRICKS::onGridLabelLeftClick( wxGridEvent& aEvent )
{
    m_grid->CommitPendingChanges();

    aEvent.Skip();
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


void GRID_TRICKS::showPopupMenu( wxMenu& menu, wxGridEvent& aEvent )
{
    menu.Append( GRIDTRICKS_ID_CUT, _( "Cut" ) + "\tCtrl+X",
                 _( "Clear selected cells placing original contents on clipboard" ) );
    menu.Append( GRIDTRICKS_ID_COPY, _( "Copy" ) + "\tCtrl+C",
                 _( "Copy selected cells to clipboard" ) );

    if( m_multiCellEditEnabled )
    {
        menu.Append( GRIDTRICKS_ID_PASTE, _( "Paste" ) + "\tCtrl+V",
                     _( "Paste clipboard cells to matrix at current cell" ) );
        menu.Append( GRIDTRICKS_ID_DELETE, _( "Delete" ) + "\tDel",
                     _( "Clear contents of selected cells" ) );
    }

    menu.Append( GRIDTRICKS_ID_SELECT, _( "Select All" ) + "\tCtrl+A",
                 _( "Select all cells" ) );

    menu.Enable( GRIDTRICKS_ID_CUT,  false );
    menu.Enable( GRIDTRICKS_ID_DELETE, false );
    menu.Enable( GRIDTRICKS_ID_PASTE, false );

    getSelectedArea();

    auto anyCellsWritable =
            [&]()
            {
                for( int row = m_sel_row_start; row < m_sel_row_start + m_sel_row_count; ++row )
                {
                    for( int col = m_sel_col_start; col < m_sel_col_start + m_sel_col_count; ++col )
                    {
                        if( !isReadOnly( row, col ) && isTextEntry( row, col ) )
                            return true;
                    }
                }

                return false;
            };

    if( anyCellsWritable() )
    {
        menu.Enable( GRIDTRICKS_ID_CUT,  true );
        menu.Enable( GRIDTRICKS_ID_DELETE, true );
    }

    // Paste can overflow the selection, so don't depend on the particular cell being writeable.

    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT )
            || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
        {
            if( m_grid->IsEditable() )
                menu.Enable( GRIDTRICKS_ID_PASTE, true );
        }

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
        cutcopy( true, true );
        break;

    case GRIDTRICKS_ID_COPY:
        cutcopy( true, false );
        break;

    case GRIDTRICKS_ID_DELETE:
        cutcopy( false, true );
        break;

    case GRIDTRICKS_ID_PASTE:
        paste_clipboard();
        break;

    case GRIDTRICKS_ID_SELECT:
        m_grid->SelectAll();
        break;

    default:
        if( menu_id >= GRIDTRICKS_FIRST_SHOWHIDE && m_grid->CommitPendingChanges( false ) )
        {
            int col = menu_id - GRIDTRICKS_FIRST_SHOWHIDE;

            if( m_grid->IsColShown( col ) )
                m_grid->HideCol( col );
            else
                m_grid->ShowCol( col );
        }
    }
}


void GRID_TRICKS::onCharHook( wxKeyEvent& ev )
{
    bool handled = false;

    if( ( ev.GetKeyCode() == WXK_RETURN || ev.GetKeyCode() == WXK_NUMPAD_ENTER )
        && ev.GetModifiers() == wxMOD_NONE
        && m_grid->GetGridCursorRow() == m_grid->GetNumberRows() - 1 )
    {
        if( m_grid->IsCellEditControlShown() )
        {
            if( m_grid->CommitPendingChanges() )
                handled = true;
        }
        else
        {
            wxCommandEvent dummy;
            m_addHandler( dummy );
            handled = true;
        }
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'V' )
    {
        if( m_grid->IsCellEditControlShown() && wxTheClipboard->Open() )
        {
            if( wxTheClipboard->IsSupported( wxDF_TEXT )
                || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
            {
                wxTextDataObject data;
                wxTheClipboard->GetData( data );

                if( data.GetText().Contains( COL_SEP ) || data.GetText().Contains( ROW_SEP ) )
                {
                    wxString stripped( data.GetText() );
                    stripped.Replace( ROW_SEP, " " );
                    stripped.Replace( ROW_SEP_R, " " );
                    stripped.Replace( COL_SEP, " " );

                    // Write to the CellEditControl if we can
                    wxTextEntry* te = dynamic_cast<wxTextEntry*>( ev.GetEventObject() );

                    if( te && te->IsEditable() )
                        te->WriteText( stripped );
                    else
                        paste_text( stripped );

                    handled = true;
                }
            }

            wxTheClipboard->Close();
            m_grid->ForceRefresh();
        }
    }
    else if( ev.GetKeyCode() == WXK_ESCAPE )
    {
        if( m_grid->IsCellEditControlShown() )
        {
            m_grid->CancelPendingChanges();
            handled = true;
        }
    }

    if( !handled )
        ev.Skip( true );
}


void GRID_TRICKS::onKeyDown( wxKeyEvent& ev )
{
    if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'A' )
    {
        m_grid->SelectAll();
        return;
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'C' )
    {
        getSelectedArea();
        cutcopy( true, false );
        return;
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'V' )
    {
        getSelectedArea();
        paste_clipboard();
        return;
    }
    else if( ev.GetModifiers() == wxMOD_CONTROL && ev.GetKeyCode() == 'X' )
    {
        getSelectedArea();
        cutcopy( true, true );
        return;
    }
    else if( !ev.GetModifiers() && ev.GetKeyCode() == WXK_DELETE )
    {
        getSelectedArea();
        cutcopy( false, true );
        return;
    }

    // space-bar toggling of checkboxes
    if( m_grid->IsEditable() && ev.GetKeyCode() == ' ' )
    {
        bool retVal = false;

        // If only rows can be selected, only toggle the first cell in a row
        if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectRows )
        {
            wxArrayInt rowSel = m_grid->GetSelectedRows();

            for( unsigned int rowInd = 0; rowInd < rowSel.GetCount(); rowInd++ )
                retVal |= toggleCell( rowSel[rowInd], 0, true );
        }

        // If only columns can be selected, only toggle the first cell in a column
        else if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectColumns )
        {
            wxArrayInt colSel = m_grid->GetSelectedCols();

            for( unsigned int colInd = 0; colInd < colSel.GetCount(); colInd++ )
                retVal |= toggleCell( 0, colSel[colInd], true );
        }

        // If the user can select the individual cells, toggle each cell selected
        else if( m_grid->GetSelectionMode() == wxGrid::wxGridSelectCells )
        {
            wxArrayInt            rowSel   = m_grid->GetSelectedRows();
            wxArrayInt            colSel   = m_grid->GetSelectedCols();
            wxGridCellCoordsArray cellSel  = m_grid->GetSelectedCells();
            wxGridCellCoordsArray topLeft  = m_grid->GetSelectionBlockTopLeft();
            wxGridCellCoordsArray botRight = m_grid->GetSelectionBlockBottomRight();

            // Iterate over every individually selected cell and try to toggle it
            for( unsigned int cellInd = 0; cellInd < cellSel.GetCount(); cellInd++ )
            {
                retVal |= toggleCell( cellSel[cellInd].GetRow(), cellSel[cellInd].GetCol(), true );
            }

            // Iterate over every column and try to toggle each cell in it
            for( unsigned int colInd = 0; colInd < colSel.GetCount(); colInd++ )
            {
                for( int row = 0; row < m_grid->GetNumberRows(); row++ )
                    retVal |= toggleCell( row, colSel[colInd], true );
            }

            // Iterate over every row and try to toggle each cell in it
            for( unsigned int rowInd = 0; rowInd < rowSel.GetCount(); rowInd++ )
            {
                for( int col = 0; col < m_grid->GetNumberCols(); col++ )
                    retVal |= toggleCell( rowSel[rowInd], col, true );
            }

            // Iterate over the selection blocks
            for( unsigned int blockInd = 0; blockInd < topLeft.GetCount(); blockInd++ )
            {
                wxGridCellCoords start = topLeft[blockInd];
                wxGridCellCoords end   = botRight[blockInd];

                for( int row = start.GetRow(); row <= end.GetRow(); row++ )
                {
                    for( int col = start.GetCol(); col <= end.GetCol(); col++ )
                        retVal |= toggleCell( row, col, true );
                }
            }
        }

        // Return if there were any cells toggled
        if( retVal )
            return;
    }

    // ctrl-tab for exit grid
#ifdef __APPLE__
    bool ctrl = ev.RawControlDown();
#else
    bool ctrl = ev.ControlDown();
#endif

    if( ctrl && ev.GetKeyCode() == WXK_TAB )
    {
        wxWindow* test = m_grid->GetNextSibling();

        if( !test )
            test = m_grid->GetParent()->GetNextSibling();

        while( test && !test->IsTopLevel() )
        {
            test->SetFocus();

            if( test->HasFocus() )
                break;

            if( !test->GetChildren().empty() )
            {
                test = test->GetChildren().front();
            }
            else if( test->GetNextSibling() )
            {
                test = test->GetNextSibling();
            }
            else
            {
                while( test )
                {
                    test = test->GetParent();

                    if( test && test->IsTopLevel() )
                    {
                        break;
                    }
                    else if( test && test->GetNextSibling() )
                    {
                        test = test->GetNextSibling();
                        break;
                    }
                }
            }
        }

        return;
    }

    ev.Skip( true );
}


void GRID_TRICKS::paste_clipboard()
{
    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( m_grid->IsEditable() && ( wxTheClipboard->IsOpened() || wxTheClipboard->Open() ) )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT )
            || wxTheClipboard->IsSupported( wxDF_UNICODETEXT ) )
        {
            wxTextDataObject    data;

            wxTheClipboard->GetData( data );

            wxString text = data.GetText();

#ifdef __WXMAC__
            // Some editors use windows linefeeds (\r\n), which wx re-writes to \n\n
            text.Replace( "\n\n", "\n" );
#endif
            m_grid->CommitPendingChanges( true );
            paste_text( text );
        }

        wxTheClipboard->Close();
        m_grid->ForceRefresh();
    }
}


void GRID_TRICKS::paste_text( const wxString& cb_text )
{
    if( !m_multiCellEditEnabled )
        return;

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
    else if( m_sel_col_count > 1 || m_sel_row_count > 1 )
    {
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
        {
            if( m_addHandler )
            {
                for( int ii = end_row - tbl->GetNumberRows(); ii > 0; --ii )
                {
                    wxCommandEvent dummy;
                    m_addHandler( dummy );
                }
            }

            end_row = tbl->GetNumberRows();
        }

        start_col = cur_col;
        end_col = start_col; // end_col actual value calculates later
    }

    for( int row = start_row;  row < end_row;  ++row )
    {
        // If number of selected rows is larger than the count of rows on the clipboard, paste
        // again and again until the end of the selection is reached.
        if( !rows.HasMoreTokens() )
            rows.SetString( cb_text, ROW_SEP, wxTOKEN_RET_EMPTY );

        wxString rowTxt = rows.GetNextToken();

        wxStringTokenizer cols( rowTxt, COL_SEP, wxTOKEN_RET_EMPTY );

        if( !is_selection )
            end_col = cur_col + cols.CountTokens();

        for( int col = start_col;  col < end_col && col < tbl->GetNumberCols();  ++col )
        {
            // Skip hidden columns
            if( !m_grid->IsColShown( col ) )
            {
                end_col++;
                continue;
            }

            // If number of selected cols is larger than the count of cols on the clipboard,
            // paste again and again until the end of the selection is reached.
            if( !cols.HasMoreTokens() )
                cols.SetString( rowTxt, COL_SEP, wxTOKEN_RET_EMPTY );

            wxString cellTxt = cols.GetNextToken();

            // Allow paste to anything that can take a string, including things like color
            // swatches and checkboxes
            if( tbl->CanSetValueAs( row, col, wxGRID_VALUE_STRING ) && !isReadOnly( row, col ) )
            {
                tbl->SetValue( row, col, cellTxt );

                wxGridEvent evt( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, row, col );
                m_grid->GetEventHandler()->ProcessEvent( evt );
            }
            // Allow paste to any cell that can accept a boolean value
            else if( tbl->CanSetValueAs( row, col, wxGRID_VALUE_BOOL ) )
            {
                tbl->SetValueAsBool( row, col, cellTxt == wxT( "1" ) );

                wxGridEvent evt( m_grid->GetId(), wxEVT_GRID_CELL_CHANGED, m_grid, row, col );
                m_grid->GetEventHandler()->ProcessEvent( evt );
            }
        }
    }
}


void GRID_TRICKS::cutcopy( bool doCopy, bool doDelete )
{
    wxLogNull doNotLog; // disable logging of failed clipboard actions

    if( doCopy && !wxTheClipboard->Open() )
        return;

    wxGridTableBase*    tbl = m_grid->GetTable();
    wxString            txt;

    // fill txt with a format that is compatible with most spreadsheets
    for( int row = m_sel_row_start;  row < m_sel_row_start + m_sel_row_count;  ++row )
    {
        if( !txt.IsEmpty() )
            txt += ROW_SEP;

        for( int col = m_sel_col_start;  col < m_sel_col_start + m_sel_col_count; ++col )
        {
            if( !m_grid->IsColShown( col ) )
                continue;

            txt += tbl->GetValue( row, col );

            if( col < m_sel_col_start + m_sel_col_count - 1 )   // that was not last column
                txt += COL_SEP;

            if( doDelete )
            {
                // Do NOT allow clear of things that can take strings but aren't textEntries
                // (ie: color swatches, textboxes, etc.).
                if( isTextEntry( row, col ) && !isReadOnly( row, col ) )
                    tbl->SetValue( row, col, wxEmptyString );
            }
        }
    }

    if( doCopy )
    {
        wxTheClipboard->SetData( new wxTextDataObject( txt ) );
        wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
        wxTheClipboard->Close();
    }

    if( doDelete )
        m_grid->ForceRefresh();
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

        if( !cursorInSelectedRow && cursorRow >= 0 )
            m_grid->SelectRow( cursorRow );
    }

    event.Skip();
}
