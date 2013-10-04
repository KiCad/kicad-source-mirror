/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
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


/*  TODO:

*)  Grab text from any pending ChoiceEditor when OK button pressed.

*)  Test wxRE_ADVANCED on Windows.

*/


#include <fctsys.h>
#include <dialog_fp_lib_table_base.h>
#include <fp_lib_table.h>
#include <fp_lib_table_lexer.h>
#include <wx/grid.h>
#include <wx/clipbrd.h>
#include <wx/tokenzr.h>
#include <wx/arrstr.h>
#include <wx/regex.h>
#include <set>


/// grid column order is established by this sequence
enum COL_ORDER
{
    COL_NICKNAME,
    COL_URI,
    COL_TYPE,
    COL_OPTIONS,
    COL_DESCR,
    COL_COUNT       // keep as last
};


/**
 * Class FP_TBL_MODEL
 * mixes in wxGridTableBase into FP_LIB_TABLE so that the latter can be used
 * as a table within wxGrid.
 */
class FP_TBL_MODEL : public wxGridTableBase, public FP_LIB_TABLE
{
public:

    /**
     * Constructor FP_TBL_MODEL
     * is a copy constructor that builds a wxGridTableBase (table model) by wrapping
     * an FP_LIB_TABLE.
     */
    FP_TBL_MODEL( const FP_LIB_TABLE& aTableToEdit ) :
        FP_LIB_TABLE( aTableToEdit )    // copy constructor
    {
    }

    //-----<wxGridTableBase overloads>-------------------------------------------

    int         GetNumberRows()     { return rows.size(); }
    int         GetNumberCols()     { return COL_COUNT; }

    wxString    GetValue( int aRow, int aCol )
    {
        if( unsigned( aRow ) < rows.size() )
        {
            const ROW&  r  = rows[aRow];

            switch( aCol )
            {
            case COL_NICKNAME:  return r.GetNickName();
            case COL_URI:       return r.GetFullURI();
            case COL_TYPE:      return r.GetType();
            case COL_OPTIONS:   return r.GetOptions();
            case COL_DESCR:     return r.GetDescr();
            default:
                ;       // fall thru to wxEmptyString
            }
        }

        return wxEmptyString;
    }

    void    SetValue( int aRow, int aCol, const wxString &aValue )
    {
        if( unsigned( aRow ) < rows.size() )
        {
            ROW&  r  = rows[aRow];

            switch( aCol )
            {
            case COL_NICKNAME:  r.SetNickName( aValue );    break;
            case COL_URI:       r.SetFullURI( aValue );     break;
            case COL_TYPE:      r.SetType( aValue  );       break;
            case COL_OPTIONS:   r.SetOptions( aValue );     break;
            case COL_DESCR:     r.SetDescr( aValue );       break;
            }
        }
    }

    bool IsEmptyCell( int aRow, int aCol )
    {
        if( unsigned( aRow ) < rows.size() )
            return false;
        return true;
    }

    bool InsertRows( size_t aPos = 0, size_t aNumRows = 1 )
    {
        if( aPos < rows.size() )
        {
            rows.insert( rows.begin() + aPos, aNumRows, ROW() );

            // use the (wxGridStringTable) source Luke.
            if( GetView() )
            {
                wxGridTableMessage msg( this,
                                        wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                        aPos,
                                        aNumRows );

                GetView()->ProcessTableMessage( msg );
            }

            return true;
        }
        return false;
    }

    bool AppendRows( size_t aNumRows = 1 )
    {
        // do not modify aNumRows, original value needed for wxGridTableMessage below
        for( int i = aNumRows; i; --i )
            rows.push_back( ROW() );

        if( GetView() )
        {
            wxGridTableMessage msg( this,
                                    wxGRIDTABLE_NOTIFY_ROWS_APPENDED,
                                    aNumRows );

            GetView()->ProcessTableMessage( msg );
        }

        return true;
    }

    bool DeleteRows( size_t aPos, size_t aNumRows )
    {
        if( aPos + aNumRows <= rows.size() )
        {
            ROWS_ITER start = rows.begin() + aPos;
            rows.erase( start, start + aNumRows );

            if( GetView() )
            {
                wxGridTableMessage msg( this,
                                        wxGRIDTABLE_NOTIFY_ROWS_DELETED,
                                        aPos,
                                        aNumRows );

                GetView()->ProcessTableMessage( msg );
            }

            return true;
        }
        return false;
    }

    void Clear()
    {
        rows.clear();
        nickIndex.clear();
    }

    wxString GetColLabelValue( int aCol )
    {
        switch( aCol )
        {
        case COL_NICKNAME:  return _( "Nickname" );
        case COL_URI:       return _( "Library Path" );

        // keep this text fairly long so column is sized wide enough
        case COL_TYPE:      return _( "Plugin Type" );
        case COL_OPTIONS:   return _( "Options" );
        case COL_DESCR:     return _( "Description" );
        default:            return wxEmptyString;
        }
    }

    //-----</wxGridTableBase overloads>------------------------------------------
};


// It works for table data on clipboard for an Excell spreadsheet,
// why not us too for now.
#define COL_SEP     wxT( '\t' )
#define ROW_SEP     wxT( '\n' )


inline bool isCtl( int aChar, const wxKeyEvent& e )
{
    return e.GetKeyCode() == aChar && e.ControlDown() && !e.AltDown() && !e.ShiftDown() && !e.MetaDown();
}


/**
 * Class DIALOG_FP_LIB_TABLE
 * shows and edits the PCB library tables.  Two tables are expected, one global
 * and one project specific.
 */
class DIALOG_FP_LIB_TABLE : public DIALOG_FP_LIB_TABLE_BASE
{
    typedef FP_LIB_TABLE::ROW   ROW;

    enum
    {
        MYID_CUT,     //  = wxID_HIGHEST + 1,
        MYID_COPY,
        MYID_PASTE,
        MYID_SELECT,
        MYID_SENTINEL,
    };

    // row & col "selection" acquisition
    // selected area by cell coordinate and count
    int selRowStart;
    int selColStart;
    int selRowCount;
    int selColCount;

    /// Gets the selected area into a sensible rectangle of sel{Row,Col}{Start,Count} above.
    void getSelectedArea()
    {
        wxGridCellCoordsArray topLeft  = m_cur_grid->GetSelectionBlockTopLeft();
        wxGridCellCoordsArray botRight = m_cur_grid->GetSelectionBlockBottomRight();

        wxArrayInt  cols = m_cur_grid->GetSelectedCols();
        wxArrayInt  rows = m_cur_grid->GetSelectedRows();

        D(printf("topLeft.Count():%zd botRight:Count():%zd\n", topLeft.Count(), botRight.Count() );)

        if( topLeft.Count() && botRight.Count() )
        {
            selRowStart = topLeft[0].GetRow();
            selColStart = topLeft[0].GetCol();

            selRowCount = botRight[0].GetRow() - selRowStart + 1;
            selColCount = botRight[0].GetCol() - selColStart + 1;
        }
        else if( cols.Count() )
        {
            selColStart = cols[0];
            selColCount = cols.Count();
            selRowStart = 0;
            selRowCount = m_cur_grid->GetNumberRows();
        }
        else if( rows.Count() )
        {
            selColStart = 0;
            selColCount = m_cur_grid->GetNumberCols();
            selRowStart = rows[0];
            selRowCount = rows.Count();
        }
        else
        {
            selRowStart = -1;
            selColStart = -1;
            selRowCount = 0;
            selColCount = 0;
        }

        D(printf("selRowStart:%d selColStart:%d selRowCount:%d selColCount:%d\n", selRowStart, selColStart, selRowCount, selColCount );)
    }

    void rightClickCellPopupMenu()
    {
        wxMenu      menu;

        menu.Append( MYID_CUT,    _( "Cut\tCTRL+X" ),         _( "Clear selected cells pasting original contents to clipboard" ) );
        menu.Append( MYID_COPY,   _( "Copy\tCTRL+C" ),        _( "Copy selected cells to clipboard" ) );
        menu.Append( MYID_PASTE,  _( "Paste\tCTRL+V" ),       _( "Paste clipboard cells to matrix at current cell" ) );
        menu.Append( MYID_SELECT, _( "Select All\tCTRL+A" ),  _( "Select all cells" ) );

        getSelectedArea();

        // if nothing is selected, disable cut and copy.
        if( !selRowCount && !selColCount )
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

        // if there is no current cell cursor, disable paste.
        else if( m_cur_row == -1 || m_cur_col == -1 )
            menu.Enable( MYID_PASTE, false );

        PopupMenu( &menu );

        // passOnFocus();
    }

    void cutcopy( bool doCut )
    {
        // this format is compatible with most spreadsheets
        if( wxTheClipboard->Open() )
        {
            wxGridTableBase*    tbl = m_cur_grid->GetTable();
            wxString            txt;

            for( int row = selRowStart;  row < selRowStart + selRowCount;  ++row )
            {
                for( int col = selColStart;  col < selColStart + selColCount; ++col )
                {
                    txt += tbl->GetValue( row, col );

                    if( col < selColStart + selColCount - 1 )   // that was not last column
                        txt += COL_SEP;

                    if( doCut )
                        tbl->SetValue( row, col, wxEmptyString );
                }
                txt += ROW_SEP;
            }

            wxTheClipboard->SetData( new wxTextDataObject( txt ) );
            wxTheClipboard->Close();
            m_cur_grid->ForceRefresh();
        }
    }

    void paste()
    {
        D(printf( "paste\n" );)
        // assume format came from a spreadsheet or us.
        if( wxTheClipboard->Open() )
        {
            if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
            {
                wxTextDataObject    data;
                FP_TBL_MODEL*       tbl = (FP_TBL_MODEL*) m_cur_grid->GetTable();

                wxTheClipboard->GetData( data );

                wxString    cb_text = data.GetText();
                size_t      ndx = cb_text.find( wxT( "(fp_lib_table " ) );

                if( ndx != std::string::npos )
                {
                    // paste the ROWs of s-expression (fp_lib_table), starting
                    // at column 0 regardless of current cursor column.

                    STRING_LINE_READER  slr( TO_UTF8( cb_text ), wxT( "Clipboard" ) );
                    FP_LIB_TABLE_LEXER  lexer( &slr );
                    FP_LIB_TABLE        tmp_tbl;
                    bool                parsed = true;

                    try
                    {
                        tmp_tbl.Parse( &lexer );
                    }
                    catch( PARSE_ERROR& pe )
                    {
                        // @todo tell what line and offset
                        parsed = false;
                    }

                    if( parsed )
                    {
                        // if clipboard rows would extend past end of current table size...
                        if( int( tmp_tbl.rows.size() ) > tbl->GetNumberRows() - m_cur_row )
                        {
                            int newRowsNeeded = tmp_tbl.rows.size() - ( tbl->GetNumberRows() - m_cur_row );
                            tbl->AppendRows( newRowsNeeded );
                        }

                        for( int i = 0;  i < (int) tmp_tbl.rows.size();  ++i )
                        {
                            tbl->rows[m_cur_row+i] = tmp_tbl.rows[i];
                        }
                    }
                    m_cur_grid->AutoSizeColumns();
                }
                else
                {
                    wxStringTokenizer   rows( cb_text, ROW_SEP, wxTOKEN_RET_EMPTY );

                    // if clipboard rows would extend past end of current table size...
                    if( int( rows.CountTokens() ) > tbl->GetNumberRows() - m_cur_row )
                    {
                        int newRowsNeeded = rows.CountTokens() - ( tbl->GetNumberRows() - m_cur_row );
                        tbl->AppendRows( newRowsNeeded );
                    }

                    for( int row = m_cur_row;  rows.HasMoreTokens();  ++row )
                    {
                        wxString rowTxt = rows.GetNextToken();

                        wxStringTokenizer   cols( rowTxt, COL_SEP, wxTOKEN_RET_EMPTY );

                        for( int col = m_cur_col; cols.HasMoreTokens();  ++col )
                        {
                            wxString cellTxt = cols.GetNextToken();
                            tbl->SetValue( row, col, cellTxt );
                        }
                    }
                }
            }

            wxTheClipboard->Close();
            m_cur_grid->ForceRefresh();
        }
    }

    // the user clicked on a popup menu choice:
    void onPopupSelection( wxCommandEvent& event )
    {
        int     menuId = event.GetId();

        // assume getSelectedArea() was called by rightClickPopupMenu() and there's
        // no way to have gotten here without that having been called.

        switch( menuId )
        {
        case MYID_CUT:
        case MYID_COPY:
            cutcopy( menuId == MYID_CUT );
            break;

        case MYID_PASTE:
            paste();
            break;

        case MYID_SELECT:
            m_cur_grid->SelectAll();
            break;
        }
    }

    /**
     * Function verifyTables
     * trims important fields, removes blank row entries, and checks for duplicates.
     * @return bool - true if tables are OK, else false.
     */
    bool verifyTables()
    {
        for( int t=0; t<2; ++t )
        {
            FP_TBL_MODEL& model = t==0 ? m_global_model : m_project_model;

            for( int r = 0; r < model.GetNumberRows(); )
            {
                wxString nick = model.GetValue( r, COL_NICKNAME ).Trim( false ).Trim();
                wxString uri  = model.GetValue( r, COL_URI ).Trim( false ).Trim();

                if( !nick || !uri )
                {
                    // Delete the "empty" row, where empty means missing nick or uri.
                    // This also updates the UI which could be slow, but there should only be a few
                    // rows to delete, unless the user fell asleep on the Add Row
                    // button.
                    model.DeleteRows( r, 1 );
                }
                else if( nick.find(':') != size_t(-1) )
                {
                    wxString msg = wxString::Format(
                        _( "Illegal character '%s' found in Nickname: '%s' in row %d" ),
                        wxT( ":" ), GetChars( nick ), r );

                    // show the tabbed panel holding the grid we have flunked:
                    if( &model != (FP_TBL_MODEL*) m_cur_grid->GetTable() )
                    {
                        m_auinotebook->SetSelection( &model == &m_global_model ? 0 : 1 );
                    }

                    // go to the problematic row
                    m_cur_grid->SelectBlock( r, 0, r, 0 );
                    m_cur_grid->MakeCellVisible( r, 0 );

                    wxMessageDialog errdlg( this, msg, _( "No Colon in Nicknames" ) );
                    errdlg.ShowModal();
                    return false;
                }
                else
                {
                    // set the trimmed values back into the table so they get saved to disk.
                    model.SetValue( r, COL_NICKNAME, nick );
                    model.SetValue( r, COL_URI, uri );
                    ++r;        // this row was OK.
                }
            }
        }

        // check for duplicate nickNames, separately in each table.
        for( int t=0; t<2; ++t )
        {
            FP_TBL_MODEL& model = t==0 ? m_global_model : m_project_model;

            for( int r1 = 0; r1 < model.GetNumberRows() - 1;  ++r1 )
            {
                wxString    nick1 = model.GetValue( r1, COL_NICKNAME );

                for( int r2=r1+1; r2 < model.GetNumberRows();  ++r2 )
                {
                    wxString    nick2 = model.GetValue( r2, COL_NICKNAME );

                    if( nick1 == nick2 )
                    {
                        wxString msg = wxString::Format(
                            _( "Duplicate Nickname: '%s' in rows %d and %d" ),
                            GetChars( nick1 ), r1+1, r2+1
                            );

                        // show the tabbed panel holding the grid we have flunked:
                        if( &model != (FP_TBL_MODEL*) m_cur_grid->GetTable() )
                        {
                            m_auinotebook->SetSelection( &model == &m_global_model ? 0 : 1 );
                        }

                        // go to the lower of the two rows, it is technically the duplicate:
                        m_cur_grid->SelectBlock( r2, 0, r2, 0 );
                        m_cur_grid->MakeCellVisible( r2, 0 );

                        wxMessageDialog errdlg( this, msg, _( "Please Delete or Modify One" ) );
                        errdlg.ShowModal();
                        return false;
                    }
                }
            }
        }

        return true;
    }

    //-----<event handlers>----------------------------------

    void onKeyDown( wxKeyEvent& ev )
    {
        if( isCtl( 'A', ev ) )
        {
            m_cur_grid->SelectAll();
        }
        else if( isCtl( 'C', ev ) )
        {
            getSelectedArea();
            cutcopy( false );
        }
        else if( isCtl( 'V', ev ) )
        {
            getSelectedArea();
            paste();
        }
        else if( isCtl( 'X', ev ) )
        {
            getSelectedArea();
            cutcopy( true );
        }
        else
            ev.Skip();
    }

    void pageChangedHandler( wxAuiNotebookEvent& event )
    {
        int pageNdx = m_auinotebook->GetSelection();
        m_cur_grid = ( pageNdx == 0 ) ? m_global_grid : m_project_grid;
    }

    void appendRowHandler( wxMouseEvent& event )
    {
        if( m_cur_grid->AppendRows( 1 ) )
        {
            int last_row = m_cur_grid->GetNumberRows() - 1;

            m_cur_grid->SelectBlock( last_row, 0, last_row, 0 );
            m_cur_grid->MakeCellVisible( last_row, 0 );
        }
    }

    void deleteRowHandler( wxMouseEvent& event )
    {
        int curRow = m_cur_grid->GetGridCursorRow();
        m_cur_grid->DeleteRows( curRow );
    }

    void moveUpHandler( wxMouseEvent& event )
    {
        int curRow = m_cur_grid->GetGridCursorRow();
        if( curRow >= 1 )
        {
            int curCol = m_cur_grid->GetGridCursorCol();

            FP_TBL_MODEL* tbl = (FP_TBL_MODEL*) m_cur_grid->GetTable();

            ROW move_me = tbl->rows[curRow];

            tbl->rows.erase( tbl->rows.begin() + curRow );
            --curRow;
            tbl->rows.insert( tbl->rows.begin() + curRow, move_me );

            if( tbl->GetView() )
            {
                // fire a msg to cause redrawing
                wxGridTableMessage msg( tbl,
                                        wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                        curRow,
                                        0 );

                tbl->GetView()->ProcessTableMessage( msg );
            }

            m_cur_grid->SetGridCursor( curRow, curCol );
        }
    }

    void moveDownHandler( wxMouseEvent& event )
    {
        FP_TBL_MODEL* tbl = (FP_TBL_MODEL*) m_cur_grid->GetTable();

        int curRow = m_cur_grid->GetGridCursorRow();
        if( unsigned( curRow + 1 ) < tbl->rows.size() )
        {
            int curCol  = m_cur_grid->GetGridCursorCol();

            ROW move_me = tbl->rows[curRow];

            tbl->rows.erase( tbl->rows.begin() + curRow );
             ++curRow;
            tbl->rows.insert( tbl->rows.begin() + curRow, move_me );

            if( tbl->GetView() )
            {
                // fire a msg to cause redrawing
                wxGridTableMessage msg( tbl,
                                        wxGRIDTABLE_NOTIFY_ROWS_INSERTED,
                                        curRow - 1,
                                        0 );

                tbl->GetView()->ProcessTableMessage( msg );
            }

            m_cur_grid->SetGridCursor( curRow, curCol );
        }
        D(printf("%s\n", __func__);)
    }

    void optionsEditor( wxMouseEvent& event )
    {
        // @todo: write the options editor, and pass the options to the Footprint*() calls.
        //D(printf("%s:%d\n", __func__, (int) m_cur_grid->GetNumberRows() );)
    }

    void onCancelButtonClick( wxCommandEvent& event )
    {
        EndModal( 0 );
    }

    void onOKButtonClick( wxCommandEvent& event )
    {
        int dialogRet = 0;

        if( verifyTables() )
        {
            if( m_global_model != *m_global )
            {
                dialogRet |= 1;

                *m_global  = m_global_model;
                m_global->reindex();
            }

            if( m_project_model != *m_project )
            {
                dialogRet |= 2;

                *m_project = m_project_model;
                m_project->reindex();
            }

            EndModal( dialogRet );
        }
    }

    void onGridCellLeftClick( wxGridEvent& event )
    {
        event.Skip();
    }

    void onGridCellLeftDClick( wxGridEvent& event )
    {
        event.Skip();
    }

    void onGridCellRightClick( wxGridEvent& event )
    {
        rightClickCellPopupMenu();
    }

    void onGridCmdSelectCell( wxGridEvent& event )
    {
        m_cur_row = event.GetRow();
        m_cur_col = event.GetCol();

        D(printf("change cursor(%d,%d)\n", m_cur_row, m_cur_col );)

        // somebody else wants this
        event.Skip();
    }

    /// Populate the readonly environment variable table with names and values
    /// by examining all the full_uri columns.
    void populateEnvironReadOnlyTable()
    {
        wxRegEx re( wxT( ".*?\\$\\{(.+?)\\}.*?" ), wxRE_ADVANCED );
        wxASSERT( re.IsValid() );   // wxRE_ADVANCED is required.

        std::set< wxString >        unique;
        typedef std::set<wxString>::const_iterator      SET_CITER;

        m_path_subs_grid->DeleteRows( 0, m_path_subs_grid->GetNumberRows() );

        int gblRowCount = m_global_model.GetNumberRows();
        int prjRowCount = m_project_model.GetNumberRows();
        int row;

        for( row = 0;  row < gblRowCount;  ++row )
        {
            wxString uri = m_global_model.GetValue( row, COL_URI );

            while( re.Matches( uri ) )
            {
                wxString envvar = re.GetMatch( uri, 1 );

                // ignore duplicates
                unique.insert( envvar );

                // delete the last match and search again
                uri.Replace( re.GetMatch( uri, 0 ), wxEmptyString );
            }
        }

        for( row = 0;  row < prjRowCount;  ++row )
        {
            wxString uri = m_project_model.GetValue( row, COL_URI );

            while( re.Matches( uri ) )
            {
                wxString envvar = re.GetMatch( uri, 1 );

                // ignore duplicates
                unique.insert( envvar );

                // delete the last match and search again
                uri.Replace( re.GetMatch( uri, 0 ), wxEmptyString );
            }
        }

        m_path_subs_grid->AppendRows( unique.size() );

        row = 0;
        for( SET_CITER it = unique.begin();  it != unique.end();  ++it, ++row )
        {
            wxString    evName = *it;
            wxString    evValue;

            m_path_subs_grid->SetCellValue( row, 0, evName );

            if( wxGetEnv( evName, &evValue ) )
                m_path_subs_grid->SetCellValue( row, 1, evValue );
        }

        m_path_subs_grid->AutoSizeColumns();
    }

    //-----</event handlers>---------------------------------

    // caller's tables are modified only on OK button.
    FP_LIB_TABLE*       m_global;
    FP_LIB_TABLE*       m_project;

    // local copies which are edited, but aborted if Cancel button.
    FP_TBL_MODEL        m_global_model;
    FP_TBL_MODEL        m_project_model;

    wxGrid*             m_cur_grid;     ///< changed based on tab choice

    // wxGrid makes it difficult to know if the cursor is yet visible,
    // use this to solve that, initial values are -1
    int                 m_cur_row;      ///< cursor position
    int                 m_cur_col;

public:
    DIALOG_FP_LIB_TABLE( wxFrame* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject ) :
        DIALOG_FP_LIB_TABLE_BASE( aParent ),
        m_global( aGlobal ),
        m_project( aProject ),
        m_global_model( *aGlobal ),
        m_project_model( *aProject ),
        m_cur_row( 0 ),
        m_cur_col( 0 )
    {
        m_global_grid->SetTable( (wxGridTableBase*) &m_global_model );
        m_project_grid->SetTable( (wxGridTableBase*) &m_project_model );

        m_global_grid->AutoSizeColumns( false );
        m_project_grid->AutoSizeColumns( false );

        wxArrayString choices;

        choices.Add( IO_MGR::ShowType( IO_MGR::KICAD ) );
        choices.Add( IO_MGR::ShowType( IO_MGR::GITHUB ) );
        choices.Add( IO_MGR::ShowType( IO_MGR::LEGACY ) );
        choices.Add( IO_MGR::ShowType( IO_MGR::EAGLE ) );
        choices.Add( IO_MGR::ShowType( IO_MGR::GEDA_PCB ) );

        /* PCAD_PLUGIN does not support Footprint*() functions
        choices.Add( IO_MGR::ShowType( IO_MGR::GITHUB ) );
        */

        wxGridCellAttr* attr;

        attr = new wxGridCellAttr;
        attr->SetEditor( new wxGridCellChoiceEditor( choices ) );
        m_project_grid->SetColAttr( COL_TYPE, attr );

        attr = new wxGridCellAttr;
        attr->SetEditor( new wxGridCellChoiceEditor( choices ) );
        m_global_grid->SetColAttr( COL_TYPE, attr );

        m_global_grid->AutoSizeColumns();
        m_project_grid->AutoSizeColumns();

        Connect( MYID_CUT, MYID_SENTINEL-1, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler( DIALOG_FP_LIB_TABLE::onPopupSelection ), NULL, this );

        populateEnvironReadOnlyTable();

        /* This scrunches the dialog hideously
        Fit();
        */

        // fire pageChangedHandler() so m_cur_grid gets set
        wxAuiNotebookEvent uneventful;
        pageChangedHandler( uneventful );

        // for ALT+A handling, we want the initial focus to be on the first selected grid.
        m_cur_grid->SetFocus();
    }

    ~DIALOG_FP_LIB_TABLE()
    {
        Disconnect( MYID_CUT, MYID_SENTINEL-1, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler( DIALOG_FP_LIB_TABLE::onPopupSelection ), NULL, this );

        // ~wxGrid() examines its table, and the tables will have been destroyed before
        // the wxGrids are, so remove the tables from the wxGrids' awareness.
        // Otherwise there is a segfault.
        m_global_grid->SetTable( NULL );
        m_project_grid->SetTable( NULL );
    }
};


int InvokePcbLibTableEditor( wxFrame* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject )
{
    DIALOG_FP_LIB_TABLE dlg( aParent, aGlobal, aProject );

    int dialogRet = dlg.ShowModal();    // returns value passed to EndModal() above

    return dialogRet;
}
