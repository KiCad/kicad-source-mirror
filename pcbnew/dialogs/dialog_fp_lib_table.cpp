/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 CERN
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

*)  After any change to uri, reparse the environment variables.

*/


#include <set>
#include <wx/regex.h>

#include <fctsys.h>
#include <project.h>
#include <3d_viewer.h>      // for KISYS3DMOD
#include <dialog_fp_lib_table_base.h>
#include <fp_lib_table.h>
#include <fp_lib_table_lexer.h>
#include <invoke_pcb_dialog.h>
#include <grid_tricks.h>
#include <confirm.h>
#include <wizard_add_fplib.h>


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
 * mixes in FP_LIB_TABLE into wxGridTableBase so the result can be used
 * as a table within wxGrid.
 */
class FP_TBL_MODEL : public wxGridTableBase, public FP_LIB_TABLE
{
    friend class FP_GRID_TRICKS;

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
        return !GetValue( aRow, aCol );
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
        // aPos may be a large positive, e.g. size_t(-1), and the sum of
        // aPos+aNumRows may wrap here, so both ends of the range are tested.
        if( aPos < rows.size() && aPos + aNumRows <= rows.size() )
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

        // keep this "Plugin Type" text fairly long so column is sized wide enough
        case COL_TYPE:      return _( "Plugin Type" );
        case COL_OPTIONS:   return _( "Options" );
        case COL_DESCR:     return _( "Description" );
        default:            return wxEmptyString;
        }
    }

    //-----</wxGridTableBase overloads>------------------------------------------
};


class FP_GRID_TRICKS : public GRID_TRICKS
{
public:
    FP_GRID_TRICKS( wxGrid* aGrid ) :
        GRID_TRICKS( aGrid )
    {
    }

protected:

    /// handle specialized clipboard text, with leading "(fp_lib_table", OR
    /// spreadsheet formatted text.
    virtual void paste_text( const wxString& cb_text )
    {
        FP_TBL_MODEL*       tbl = (FP_TBL_MODEL*) m_grid->GetTable();

        size_t  ndx = cb_text.find( wxT( "(fp_lib_table" ) );

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
                DisplayError( NULL, pe.errorText );
                parsed = false;
            }

            if( parsed )
            {
                const int cur_row = std::max( getCursorRow(), 0 );

                // if clipboard rows would extend past end of current table size...
                if( tmp_tbl.GetCount() > tbl->GetNumberRows() - cur_row )
                {
                    int newRowsNeeded = tmp_tbl.GetCount() - ( tbl->GetNumberRows() - cur_row );
                    tbl->AppendRows( newRowsNeeded );
                }

                for( int i = 0;  i < tmp_tbl.GetCount();  ++i )
                {
                    tbl->At( cur_row+i ) = tmp_tbl.At( i );
                }
            }
            m_grid->AutoSizeColumns( false );
        }
        else
        {
            // paste spreadsheet formatted text.
            GRID_TRICKS::paste_text( cb_text );
        }
    }
};


/**
 * Class DIALOG_FP_LIB_TABLE
 * shows and edits the PCB library tables.  Two tables are expected, one global
 * and one project specific.
 */
class DIALOG_FP_LIB_TABLE : public DIALOG_FP_LIB_TABLE_BASE
{

public:
    DIALOG_FP_LIB_TABLE( wxTopLevelWindow* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject ) :
        DIALOG_FP_LIB_TABLE_BASE( aParent ),
        m_global( aGlobal ),
        m_project( aProject )
    {
        // For user info, shows the table filenames:
        m_PrjTableFilename->SetLabel( Prj().FootprintLibTblName() );
        m_GblTableFilename->SetLabel( FP_LIB_TABLE::GetGlobalTableFileName() );

        // wxGrid only supports user owned tables if they exist past end of ~wxGrid(),
        // so make it a grid owned table.
        m_global_grid->SetTable(  new FP_TBL_MODEL( *aGlobal ),  true );
        m_project_grid->SetTable( new FP_TBL_MODEL( *aProject ), true );

        // add Cut, Copy, and Paste to wxGrids
        m_global_grid->PushEventHandler( new FP_GRID_TRICKS( m_global_grid ) );
        m_project_grid->PushEventHandler( new FP_GRID_TRICKS( m_project_grid ) );

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

        populateEnvironReadOnlyTable();

        for( int i=0; i<2; ++i )
        {
            wxGrid* g = i==0 ? m_global_grid : m_project_grid;

            // all but COL_OPTIONS, which is edited with Option Editor anyways.
            g->AutoSizeColumn( COL_NICKNAME, false );
            g->AutoSizeColumn( COL_TYPE, false );
            g->AutoSizeColumn( COL_URI, false );
            g->AutoSizeColumn( COL_DESCR, false );

            // would set this to width of title, if it was easily known.
            g->SetColSize( COL_OPTIONS, 80 );
        }

        // This scrunches the dialog hideously, probably due to wxAUI container.
        // Fit();
        // We derive from DIALOG_SHIM so prior size will be used anyways.

        // select the last selected page
        m_auinotebook->SetSelection( m_pageNdx );

        // fire pageChangedHandler() so m_cur_grid gets set
        // m_auinotebook->SetSelection will generate a pageChangedHandler()
        // event call later, but too late.
        wxAuiNotebookEvent uneventful;
        pageChangedHandler( uneventful );

        // Gives a selection for each grid, mainly for delete lib button.
        // Without that, we do not see what lib will be deleted
        m_global_grid->SelectRow(0);
        m_project_grid->SelectRow(0);

        // for ALT+A handling, we want the initial focus to be on the first selected grid.
        m_cur_grid->SetFocus();
    }

    ~DIALOG_FP_LIB_TABLE()
    {
        // Delete the GRID_TRICKS.
        // Any additional event handlers should be popped before the window is deleted.
        m_global_grid->PopEventHandler( true );
        m_project_grid->PopEventHandler( true );
    }


private:
    typedef FP_LIB_TABLE::ROW   ROW;

    /// If the cursor is not on a valid cell, because there are no rows at all, return -1,
    /// else return a 0 based column index.
    int getCursorCol() const
    {
        return m_cur_grid->GetGridCursorCol();
    }

    /// If the cursor is not on a valid cell, because there are no rows at all, return -1,
    /// else return a 0 based row index.
    int getCursorRow() const
    {
        return m_cur_grid->GetGridCursorRow();
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
            FP_TBL_MODEL& model = t==0 ? *global_model() : *project_model();

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
                    if( &model != cur_model() )
                    {
                        m_auinotebook->SetSelection( &model == global_model() ? 0 : 1 );
                    }

                    // go to the problematic row
                    m_cur_grid->SetGridCursor( r, 0 );
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
            FP_TBL_MODEL& model = t==0 ? *global_model() : *project_model();

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
                        if( &model != cur_model() )
                        {
                            m_auinotebook->SetSelection( &model == global_model() ? 0 : 1 );
                        }

                        // go to the lower of the two rows, it is technically the duplicate:
                        m_cur_grid->SetGridCursor( r2, 0 );
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
#if 0
        // send the key to the current grid
        ((wxEvtHandler*)m_cur_grid)->ProcessEvent( ev );
#else
        // or no:
        // m_cur_grid has the focus most of the time anyways, so above not needed.
        ev.Skip();
#endif
    }

    void pageChangedHandler( wxAuiNotebookEvent& event )
    {
        m_pageNdx = m_auinotebook->GetSelection();
        m_cur_grid = ( m_pageNdx == 0 ) ? m_global_grid : m_project_grid;
    }

    void appendRowHandler( wxCommandEvent& event )
    {
        if( m_cur_grid->AppendRows( 1 ) )
        {
            int last_row = m_cur_grid->GetNumberRows() - 1;

            // wx documentation is wrong, SetGridCursor does not make visible.
            m_cur_grid->MakeCellVisible( last_row, 0 );
            m_cur_grid->SetGridCursor( last_row, 0 );
            m_cur_grid->SelectRow( m_cur_grid->GetGridCursorRow() );
        }
    }

    void deleteRowHandler( wxCommandEvent& event )
    {
#if 1
        int currRow = getCursorRow();
        wxArrayInt selectedRows	= m_cur_grid->GetSelectedRows();

        if( selectedRows.size() == 0 && getCursorRow() >= 0 )
            selectedRows.Add( getCursorRow() );

        std::sort( selectedRows.begin(), selectedRows.end() );

        for( int ii = selectedRows.GetCount()-1; ii >= 0; ii-- )
        {
            int row = selectedRows[ii];
            m_cur_grid->DeleteRows( row, 1 );
        }

        if( currRow >= m_cur_grid->GetNumberRows() )
            m_cur_grid->SetGridCursor(m_cur_grid->GetNumberRows()-1, getCursorCol() );

        m_cur_grid->SelectRow( m_cur_grid->GetGridCursorRow() );
#else
        int rowCount = m_cur_grid->GetNumberRows();
        int curRow   = getCursorRow();

        if( curRow >= 0 )
        {
            m_cur_grid->DeleteRows( curRow );

            if( curRow && curRow == rowCount - 1 )
            {
                m_cur_grid->SetGridCursor( curRow-1, getCursorCol() );
            }
        }
#endif
    }

    void moveUpHandler( wxCommandEvent& event )
    {
        int curRow = getCursorRow();
        if( curRow >= 1 )
        {
            int curCol = getCursorCol();

            FP_TBL_MODEL* tbl = cur_model();

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

            m_cur_grid->MakeCellVisible( curRow, curCol );
            m_cur_grid->SetGridCursor( curRow, curCol );
            m_cur_grid->SelectRow( getCursorRow() );
        }
    }

    void moveDownHandler( wxCommandEvent& event )
    {
        FP_TBL_MODEL* tbl = cur_model();

        int curRow = getCursorRow();
        if( unsigned( curRow + 1 ) < tbl->rows.size() )
        {
            int curCol  = getCursorCol();

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

            m_cur_grid->MakeCellVisible( curRow, curCol );
            m_cur_grid->SetGridCursor( curRow, curCol );
            m_cur_grid->SelectRow( getCursorRow() );
        }
    }

    void optionsEditor( wxCommandEvent& event )
    {
        FP_TBL_MODEL*   tbl = cur_model();

        if( tbl->GetNumberRows() )
        {
            int     curRow = getCursorRow();
            ROW&    row    = tbl->rows[curRow];

            wxString        result;
            const wxString& options = row.GetOptions();

            InvokePluginOptionsEditor( this, row.GetNickName(), row.GetType(), options, &result );

            if( options != result )
            {
                row.SetOptions( result );

                // all but options:
                m_cur_grid->AutoSizeColumn( COL_NICKNAME, false );
                m_cur_grid->AutoSizeColumn( COL_URI, false );
                m_cur_grid->AutoSizeColumn( COL_TYPE, false );

                // On Windows, the grid is not refresh,
                // so force resfresh after a change
#ifdef __WINDOWS__
                Refresh();
#endif
            }
        }
    }

	void OnClickLibraryWizard( wxCommandEvent& event );

    void onCancelButtonClick( wxCommandEvent& event )
    {
        EndModal( 0 );
    }

    void onCancelCaptionButtonClick( wxCloseEvent& event )
    {
        EndModal( 0 );
    }

    void onOKButtonClick( wxCommandEvent& event )
    {
        int dialogRet = 0;

        // stuff any pending cell editor text into the table.
        m_cur_grid->SaveEditControlValue();

        if( verifyTables() )
        {
            if( *global_model() != *m_global )
            {
                dialogRet |= 1;

                *m_global  = *global_model();
                m_global->reindex();
            }

            if( *project_model() != *m_project )
            {
                dialogRet |= 2;

                *m_project = *project_model();
                m_project->reindex();
            }

            EndModal( dialogRet );
        }
    }

    /// Populate the readonly environment variable table with names and values
    /// by examining all the full_uri columns.
    void populateEnvironReadOnlyTable()
    {
        wxRegEx re( wxT( ".*?\\$\\{(.+?)\\}.*?" ), wxRE_ADVANCED );
        wxASSERT( re.IsValid() );   // wxRE_ADVANCED is required.

        std::set< wxString >        unique;
        typedef std::set<wxString>::const_iterator      SET_CITER;

        // clear the table
        m_path_subs_grid->DeleteRows( 0, m_path_subs_grid->GetNumberRows() );

        FP_TBL_MODEL*   gbl = global_model();
        FP_TBL_MODEL*   prj = project_model();

        int gblRowCount = gbl->GetNumberRows();
        int prjRowCount = prj->GetNumberRows();
        int row;

        for( row = 0;  row < gblRowCount;  ++row )
        {
            wxString uri = gbl->GetValue( row, COL_URI );

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
            wxString uri = prj->GetValue( row, COL_URI );

            while( re.Matches( uri ) )
            {
                wxString envvar = re.GetMatch( uri, 1 );

                // ignore duplicates
                unique.insert( envvar );

                // delete the last match and search again
                uri.Replace( re.GetMatch( uri, 0 ), wxEmptyString );
            }
        }

        // Make sure this special environment variable shows up even if it was
        // not used yet.  It is automatically set by KiCad to the directory holding
        // the current project.
        unique.insert( PROJECT_VAR_NAME );
        unique.insert( FP_LIB_TABLE::GlobalPathEnvVariableName() );
        // This special environment variable is used to locate 3d shapes
        unique.insert( KISYS3DMOD );
        unique.insert( FP_LIB_TABLE::GlobalPathEnvVariableName() );

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

    // caller's tables are modified only on OK button and successful verification.
    FP_LIB_TABLE*       m_global;
    FP_LIB_TABLE*       m_project;

    FP_TBL_MODEL*       global_model()  const   { return (FP_TBL_MODEL*) m_global_grid->GetTable(); }
    FP_TBL_MODEL*       project_model() const   { return (FP_TBL_MODEL*) m_project_grid->GetTable(); }
    FP_TBL_MODEL*       cur_model() const       { return (FP_TBL_MODEL*) m_cur_grid->GetTable(); }

    wxGrid*             m_cur_grid;     ///< changed based on tab choice
    static int          m_pageNdx;      ///< Remember the last notebook page selected during a session
};

int DIALOG_FP_LIB_TABLE::m_pageNdx = 0;


void DIALOG_FP_LIB_TABLE::OnClickLibraryWizard( wxCommandEvent& event )
{
    wxArrayString envVariableList;

    // Build the environment variables in use:
    for( int ii = 0; ii <  m_path_subs_grid->GetTable()->GetRowsCount(); ii ++ )
        envVariableList.Add( m_path_subs_grid->GetCellValue( wxGridCellCoords( ii, 0 ) ) );

    WIZARD_FPLIB_TABLE dlg( this, envVariableList );

    if( ! dlg.RunWizard( dlg.GetFirstPage() ) )
        return;     // Aborted by user

    wxGrid* libgrid = m_cur_grid;
    FP_TBL_MODEL*  tbl = (FP_TBL_MODEL*) libgrid->GetTable();

    // Import fp library list
    int idx = 0;
    wxArrayString libDescr;   // Will contain nickname, URI, plugin

    while( dlg.GetLibDescr( idx++, libDescr ) )
    {
        if( ! libDescr[0].IsEmpty() && m_cur_grid->AppendRows( 1 ) )
        {
            int last_row = libgrid->GetNumberRows() - 1;

            // Add the nickname: currently make it from filename
            tbl->SetValue( last_row, COL_NICKNAME, libDescr[0] );
            // Add the full path:
            tbl->SetValue( last_row, COL_URI, libDescr[1] );
            // Add the plugin name:
            tbl->SetValue( last_row, COL_TYPE, libDescr[2] );

            libgrid->MakeCellVisible( last_row, 0 );
            libgrid->SetGridCursor( last_row, 0 );
        }

        libDescr.Clear();
    }

    libgrid->SelectRow( libgrid->GetGridCursorRow() );
}


int InvokePcbLibTableEditor( wxTopLevelWindow* aParent, FP_LIB_TABLE* aGlobal, FP_LIB_TABLE* aProject )
{
    DIALOG_FP_LIB_TABLE dlg( aParent, aGlobal, aProject );

    int dialogRet = dlg.ShowModal();    // returns value passed to EndModal() above

    return dialogRet;
}
