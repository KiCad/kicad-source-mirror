/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 KiCad Developers, see change_log.txt for contributors.
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
#include <wx/arrstr.h>
#include <wx/clipbrd.h>


 // It works for table data on clipboard for an Excell spreadsheet,
// why not us too for now.
#define COL_SEP     wxT( '\t' )
#define ROW_SEP     wxT( '\n' )


enum
{
    MYID_FIRST = -1,
    MYID_CUT,
    MYID_COPY,
    MYID_PASTE,
    MYID_SELECT,
    MYID_LAST,
};


GRID_TRICKS::GRID_TRICKS( wxGrid* aGrid ):
    m_grid( aGrid )
{
    aGrid->Connect( wxEVT_GRID_CELL_RIGHT_CLICK, wxGridEventHandler( GRID_TRICKS::onGridCellRightClick ), NULL, this );
    aGrid->Connect( MYID_FIRST, MYID_LAST, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( GRID_TRICKS::onPopupSelection ), NULL, this );
    aGrid->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( GRID_TRICKS::onKeyDown ), NULL, this );
    aGrid->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( GRID_TRICKS::onRightDown ), NULL, this );
}


void GRID_TRICKS::getSelectedArea()
{
    wxGridCellCoordsArray topLeft  = m_grid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray botRight = m_grid->GetSelectionBlockBottomRight();

    wxArrayInt  cols = m_grid->GetSelectedCols();
    wxArrayInt  rows = m_grid->GetSelectedRows();

    DBG(printf("topLeft.Count():%d botRight:Count():%d\n", int( topLeft.Count() ), int( botRight.Count() ) );)

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
        m_sel_row_start = -1;
        m_sel_col_start = -1;
        m_sel_row_count = 0;
        m_sel_col_count = 0;
    }

    //DBG(printf("m_sel_row_start:%d m_sel_col_start:%d m_sel_row_count:%d m_sel_col_count:%d\n", m_sel_row_start, m_sel_col_start, m_sel_row_count, m_sel_col_count );)
}


void GRID_TRICKS::showPopupMenu()
{
    wxMenu      menu;

    menu.Append( MYID_CUT,    _( "Cut\tCTRL+X" ),         _( "Clear selected cells pasting original contents to clipboard" ) );
    menu.Append( MYID_COPY,   _( "Copy\tCTRL+C" ),        _( "Copy selected cells to clipboard" ) );
    menu.Append( MYID_PASTE,  _( "Paste\tCTRL+V" ),       _( "Paste clipboard cells to matrix at current cell" ) );
    menu.Append( MYID_SELECT, _( "Select All\tCTRL+A" ),  _( "Select all cells" ) );

    getSelectedArea();

    // if nothing is selected, disable cut and copy.
    if( !m_sel_row_count && !m_sel_col_count )
    {
        menu.Enable( MYID_CUT,  false );
        menu.Enable( MYID_COPY, false );
    }

    bool have_cb_text = false;
    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
            have_cb_text = true;

        wxTheClipboard->Close();
    }

    if( !have_cb_text )
    {
        // if nothing on clipboard, disable paste.
        menu.Enable( MYID_PASTE, false );
    }

    m_grid->PopupMenu( &menu );
}


void GRID_TRICKS::onPopupSelection( wxCommandEvent& event )
{
    int     menu_id = event.GetId();

    // assume getSelectedArea() was called by rightClickPopupMenu() and there's
    // no way to have gotten here without that having been called.

    switch( menu_id )
    {
    case MYID_CUT:
    case MYID_COPY:
        cutcopy( menu_id == MYID_CUT );
        break;

    case MYID_PASTE:
        paste_clipboard();
        break;

    case MYID_SELECT:
        m_grid->SelectAll();
        break;

    default:
        ;
    }
}


void GRID_TRICKS::onKeyDown( wxKeyEvent& ev )
{
    if( isCtl( 'A', ev ) )
    {
        m_grid->SelectAll();
    }
    else if( isCtl( 'C', ev ) )
    {
        getSelectedArea();
        cutcopy( false );
    }
    else if( isCtl( 'V', ev ) )
    {
        getSelectedArea();
        paste_clipboard();
    }
    else if( isCtl( 'X', ev ) )
    {
        getSelectedArea();
        cutcopy( true );
    }
    else
    {
        ev.Skip( true );
    }
}


void GRID_TRICKS::paste_clipboard()
{
    if( wxTheClipboard->Open() )
    {
        if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
        {
            wxTextDataObject    data;

            wxTheClipboard->GetData( data );

            wxString    cb_text = data.GetText();

            paste_text( cb_text );
        }

        wxTheClipboard->Close();
        m_grid->ForceRefresh();
    }
}


void GRID_TRICKS::paste_text( const wxString& cb_text )
{
    wxGridTableBase*   tbl = m_grid->GetTable();

    const int cur_row = std::max( getCursorRow(), 0 );    // no -1
    const int cur_col = std::max( getCursorCol(), 0 );

    wxStringTokenizer   rows( cb_text, ROW_SEP, wxTOKEN_RET_EMPTY );

    // if clipboard rows would extend past end of current table size...
    if( int( rows.CountTokens() ) > tbl->GetNumberRows() - cur_row )
    {
        int newRowsNeeded = rows.CountTokens() - ( tbl->GetNumberRows() - cur_row );

        tbl->AppendRows( newRowsNeeded );
    }

    for( int row = cur_row;  rows.HasMoreTokens();  ++row )
    {
        wxString rowTxt = rows.GetNextToken();

        wxStringTokenizer   cols( rowTxt, COL_SEP, wxTOKEN_RET_EMPTY );

        for( int col = cur_col;  cols.HasMoreTokens();  ++col )
        {
            wxString cellTxt = cols.GetNextToken();
            tbl->SetValue( row, col, cellTxt );
        }
    }
    m_grid->AutoSizeColumns( false );
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
        {
            m_grid->AutoSizeColumns( false );
            m_grid->ForceRefresh();
        }
    }
}

