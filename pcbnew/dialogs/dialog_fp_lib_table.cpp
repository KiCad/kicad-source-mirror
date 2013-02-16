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

*)  Check for duplicate nicknames per table

*)  Grab text from any pending ChoiceEditor when OK button pressed.

*)  Test wxRE_ADVANCED on Windows.

*)  Do environment variable substitution on lookup

*/



#include <fctsys.h>
#include <dialog_fp_lib_table_base.h>
#include <fp_lib_table.h>
#include <wx/grid.h>
#include <wx/clipbrd.h>
#include <wx/tokenzr.h>
#include <wx/arrstr.h>
#include <wx/regex.h>
#include <set>

/**
 * Class FP_TBL_MODEL
 * mixes in wxGridTableBase into FP_LIB_TABLE so that the latter can be used
 * as a table within wxGrid.
 */
class FP_TBL_MODEL : public wxGridTableBase, public FP_LIB_TABLE
{
public:

    enum COL_ORDER      ///<  grid column order, established by this sequence
    {
        COL_NICKNAME,
        COL_URI,
        COL_TYPE,
        COL_OPTIONS,
        COL_DESCR,
        COL_COUNT       // keep as last
    };

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

    int         GetNumberRows () { return rows.size(); }
    int         GetNumberCols () { return COL_COUNT; }

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
        ID_CUT,     //  = wxID_HIGHEST + 1,
        ID_COPY,
        ID_PASTE,
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

        // D(printf("selRowStart:%d selColStart:%d selRowCount:%d selColCount:%d\n", selRowStart, selColStart, selRowCount, selColCount );)
    }

    void rightClickCellPopupMenu()
    {
        wxMenu      menu;

        menu.Append( ID_CUT, _( "Cut" ),      _( "Clear selected cells" ) );
        menu.Append( ID_COPY, _( "Copy" ),    _( "Copy selected cells to clipboard" ) );
        menu.Append( ID_PASTE, _( "Paste" ),  _( "Paste clipboard cells to matrix at current cell" ) );

        getSelectedArea();

        // if nothing is selected, diable cut and copy.
        if( !selRowCount && !selColCount )
        {
            menu.Enable( ID_CUT,  false );
            menu.Enable( ID_COPY, false );
        }

        // if there is no current cell cursor, disable paste.
        if( m_cur_row == -1 || m_cur_col == -1 )
            menu.Enable( ID_PASTE, false );

        PopupMenu( &menu );

        // passOnFocus();
    }

    // the user clicked on a popup menu choice:
    void onPopupSelection( wxCommandEvent& event )
    {
        int     menuId = event.GetId();

        // assume getSelectedArea() was called by rightClickPopupMenu() and there's
        // no way to have gotten here without that having been called.

        switch( menuId )
        {
        case ID_CUT:
        case ID_COPY:
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

                        if( menuId == ID_CUT )
                            tbl->SetValue( row, col, wxEmptyString );
                    }
                    txt += ROW_SEP;
                }

                wxTheClipboard->SetData( new wxTextDataObject( txt ) );
                wxTheClipboard->Close();
                m_cur_grid->ForceRefresh();
            }
            break;

        case ID_PASTE:
            D(printf( "paste\n" );)
            // assume format came from a spreadsheet or us.
            if( wxTheClipboard->Open() )
            {
                if( wxTheClipboard->IsSupported( wxDF_TEXT ) )
                {
                    wxGridTableBase*    tbl = m_cur_grid->GetTable();
                    wxTextDataObject    data;

                    wxTheClipboard->GetData( data );

                    wxStringTokenizer   rows( data.GetText(), ROW_SEP, wxTOKEN_RET_EMPTY );

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

                wxTheClipboard->Close();
                m_cur_grid->ForceRefresh();
            }
            break;
        }
    }

    //-----<event handlers>----------------------------------

    void pageChangedHandler( wxAuiNotebookEvent& event )
    {
        int pageNdx = m_auinotebook->GetSelection();
        m_cur_grid = ( pageNdx == 0 ) ? m_global_grid : m_project_grid;
    }

    void appendRowHandler( wxMouseEvent& event )
    {
        m_cur_grid->AppendRows( 1 );
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

    void onCancelButtonClick( wxCommandEvent& event )
    {
        EndModal( 0 );
    }

    void onOKButtonClick( wxCommandEvent& event )
    {
        int dialogRet = 0;

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
            wxString uri = m_global_model.GetValue( row, FP_TBL_MODEL::COL_URI );

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
            wxString uri = m_project_model.GetValue( row, FP_TBL_MODEL::COL_URI );

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
        m_cur_row( -1 ),
        m_cur_col( -1 )
    {
        m_global_grid->SetTable( (wxGridTableBase*) &m_global_model );
        m_project_grid->SetTable( (wxGridTableBase*) &m_project_model );

        m_global_grid->AutoSizeColumns( false );
        m_project_grid->AutoSizeColumns( false );

        wxArrayString choices;
        choices.Add( IO_MGR::ShowType( IO_MGR::KICAD ) );
        choices.Add( IO_MGR::ShowType( IO_MGR::LEGACY ) );
        choices.Add( IO_MGR::ShowType( IO_MGR::EAGLE ) );
        choices.Add( IO_MGR::ShowType( IO_MGR::GEDA_PCB ) );

        wxGridCellAttr* attr;

        attr = new wxGridCellAttr;
        attr->SetEditor( new wxGridCellChoiceEditor( choices ) );
        m_project_grid->SetColAttr( FP_TBL_MODEL::COL_TYPE, attr );

        attr = new wxGridCellAttr;
        attr->SetEditor( new wxGridCellChoiceEditor( choices ) );
        m_global_grid->SetColAttr(  FP_TBL_MODEL::COL_TYPE, attr );

        m_global_grid->AutoSizeColumns();
        m_project_grid->AutoSizeColumns();

        Connect( ID_CUT, ID_PASTE, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler( DIALOG_FP_LIB_TABLE::onPopupSelection ), NULL, this );

        populateEnvironReadOnlyTable();

        /* This scrunches the dialog hideously
        Fit();
        */

        // fire pageChangedHandler() so m_cur_grid gets set
        wxAuiNotebookEvent uneventful;
        pageChangedHandler( uneventful );
    }

    ~DIALOG_FP_LIB_TABLE()
    {
        Disconnect( ID_CUT, ID_PASTE, wxEVT_COMMAND_MENU_SELECTED,
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

