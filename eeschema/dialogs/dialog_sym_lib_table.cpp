/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <set>
#include <wx/regex.h>

#include <fctsys.h>
#include <project.h>
#include <dialog_sym_lib_table.h>
#include <lib_id.h>
#include <symbol_lib_table.h>
#include <lib_table_lexer.h>
#include <grid_tricks.h>
#include <confirm.h>
#include <lib_table_grid.h>
#include <wildcards_and_files_ext.h>
#include <env_paths.h>


/**
 * Build a wxGridTableBase by wrapping an #SYMBOL_LIB_TABLE object.
 */
class SYMBOL_LIB_TABLE_GRID : public LIB_TABLE_GRID, public SYMBOL_LIB_TABLE
{
    friend class SYMBOL_GRID_TRICKS;

protected:
    LIB_TABLE_ROW* at( size_t aIndex ) override { return &rows.at( aIndex ); }

    size_t size() const override { return rows.size(); }

    LIB_TABLE_ROW* makeNewRow() override
    {
        return dynamic_cast< LIB_TABLE_ROW* >( new SYMBOL_LIB_TABLE_ROW );
    }

    LIB_TABLE_ROWS_ITER begin() override { return rows.begin(); }

    LIB_TABLE_ROWS_ITER insert( LIB_TABLE_ROWS_ITER aIterator, LIB_TABLE_ROW* aRow ) override
    {
        return rows.insert( aIterator, aRow );
    }

    void push_back( LIB_TABLE_ROW* aRow ) override { rows.push_back( aRow ); }

    LIB_TABLE_ROWS_ITER erase( LIB_TABLE_ROWS_ITER aFirst, LIB_TABLE_ROWS_ITER aLast ) override
    {
        return rows.erase( aFirst, aLast );
    }

public:

    SYMBOL_LIB_TABLE_GRID( const SYMBOL_LIB_TABLE& aTableToEdit )
    {
        rows = aTableToEdit.rows;
    }
};


class SYMBOL_GRID_TRICKS : public GRID_TRICKS
{
public:
    SYMBOL_GRID_TRICKS( wxGrid* aGrid ) :
        GRID_TRICKS( aGrid )
    {
    }

protected:

    /// handle specialized clipboard text, with leading "(sym_lib_table" or
    /// spreadsheet formatted text.
    virtual void paste_text( const wxString& cb_text ) override
    {
        SYMBOL_LIB_TABLE_GRID*       tbl = (SYMBOL_LIB_TABLE_GRID*) m_grid->GetTable();

        size_t  ndx = cb_text.find( "(sym_lib_table" );

        if( ndx != std::string::npos )
        {
            // paste the SYMBOL_LIB_TABLE_ROWs of s-expression (sym_lib_table), starting
            // at column 0 regardless of current cursor column.

            STRING_LINE_READER  slr( TO_UTF8( cb_text ), "Clipboard" );
            LIB_TABLE_LEXER     lexer( &slr );
            SYMBOL_LIB_TABLE    tmp_tbl;
            bool                parsed = true;

            try
            {
                tmp_tbl.Parse( &lexer );
            }
            catch( PARSE_ERROR& pe )
            {
                DisplayError( NULL, pe.What() );
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
                    tbl->rows.replace( cur_row+i, tmp_tbl.At( i ) );
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


DIALOG_SYMBOL_LIB_TABLE::DIALOG_SYMBOL_LIB_TABLE( wxTopLevelWindow* aParent,
                                                  SYMBOL_LIB_TABLE* aGlobal,
                                                  SYMBOL_LIB_TABLE* aProject ) :
    DIALOG_SYMBOL_LIB_TABLE_BASE( aParent ),
    m_global( aGlobal ),
    m_project( aProject )
{
    // For user info, shows the table filenames:
    m_PrjTableFilename->SetLabel( Prj().SymbolLibTableName() );
    m_GblTableFilename->SetLabel( SYMBOL_LIB_TABLE::GetGlobalTableFileName() );

    // wxGrid only supports user owned tables if they exist past end of ~wxGrid(),
    // so make it a grid owned table.
    m_global_grid->SetTable(  new SYMBOL_LIB_TABLE_GRID( *aGlobal ),  true );
    m_project_grid->SetTable( new SYMBOL_LIB_TABLE_GRID( *aProject ), true );

    // add Cut, Copy, and Paste to wxGrids
    m_global_grid->PushEventHandler( new SYMBOL_GRID_TRICKS( m_global_grid ) );
    m_project_grid->PushEventHandler( new SYMBOL_GRID_TRICKS( m_project_grid ) );

    m_global_grid->AutoSizeColumns( false );
    m_project_grid->AutoSizeColumns( false );

    wxArrayString pluginChoices;

//    pluginChoices.Add( SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_KICAD ) );
    pluginChoices.Add( SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY ) );

    populateEnvironReadOnlyTable();

    for( int i=0; i<2; ++i )
    {
        wxGrid* g = i==0 ? m_global_grid : m_project_grid;

        // Set special attributes
        wxGridCellAttr* attr;

        attr = new wxGridCellAttr;
        attr->SetEditor( new wxGridCellChoiceEditor( pluginChoices ) );
        g->SetColAttr( COL_TYPE, attr );

        attr = new wxGridCellAttr;
        attr->SetEditor( new wxGridCellBoolEditor() );
        attr->SetRenderer( new wxGridCellBoolRenderer() );
        g->SetColAttr( COL_ENABLED, attr );

        // all but COL_OPTIONS, which is edited with Option Editor anyways.
        g->AutoSizeColumn( COL_NICKNAME, false );
        g->AutoSizeColumn( COL_TYPE, false );
        g->AutoSizeColumn( COL_URI, false );
        g->AutoSizeColumn( COL_DESCR, false );
        g->AutoSizeColumn( COL_ENABLED, false );

        // would set this to width of title, if it was easily known.
        g->SetColSize( COL_OPTIONS, 80 );
    }

    // select the last selected page
    m_auinotebook->SetSelection( m_pageNdx );

    // Gives a selection for each grid, mainly for delete lib button.
    // Without that, we do not see what lib will be deleted
    m_global_grid->SelectRow( 0 );
    m_project_grid->SelectRow( 0 );

    // for ALT+A handling, we want the initial focus to be on the first selected grid.
    if( m_pageNdx == 0 )
    {
        m_global_grid->SetFocus();
        m_cur_grid = m_global_grid;
    }
    else
    {
        m_project_grid->SetFocus();
        m_cur_grid = m_project_grid;
    }

    // On some window managers (Unity, XFCE), this dialog is
    // not always raised, depending on this dialog is run.
    // Force it to be raised
    Raise();
}


DIALOG_SYMBOL_LIB_TABLE::~DIALOG_SYMBOL_LIB_TABLE()
{
    // Delete the GRID_TRICKS.
    // Any additional event handlers should be popped before the window is deleted.
    m_global_grid->PopEventHandler( true );
    m_project_grid->PopEventHandler( true );
}


int DIALOG_SYMBOL_LIB_TABLE::getCursorCol() const
{
    return m_cur_grid->GetGridCursorCol();
}


int DIALOG_SYMBOL_LIB_TABLE::getCursorRow() const
{
    return m_cur_grid->GetGridCursorRow();
}


bool DIALOG_SYMBOL_LIB_TABLE::verifyTables()
{
    for( int t=0; t<2; ++t )
    {
        SYMBOL_LIB_TABLE_GRID& model = t==0 ? *global_model() : *project_model();

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
            else if( nick.find( ':' ) != size_t( -1 ) )
            {
                wxString msg = wxString::Format(
                    _( "Illegal character \"%s\" found in Nickname: \"%s\" in row %d" ),
                    ":", GetChars( nick ), r );

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
        SYMBOL_LIB_TABLE_GRID& model = t==0 ? *global_model() : *project_model();

        for( int r1 = 0; r1 < model.GetNumberRows() - 1;  ++r1 )
        {
            wxString    nick1 = model.GetValue( r1, COL_NICKNAME );

            for( int r2=r1+1; r2 < model.GetNumberRows();  ++r2 )
            {
                wxString    nick2 = model.GetValue( r2, COL_NICKNAME );

                if( nick1 == nick2 )
                {
                    wxString msg = wxString::Format(
                        _( "Duplicate Nickname: \"%s\" in rows %d and %d" ),
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


void DIALOG_SYMBOL_LIB_TABLE::pageChangedHandler( wxAuiNotebookEvent& event )
{
    m_pageNdx = m_auinotebook->GetSelection();
    m_cur_grid = ( m_pageNdx == 0 ) ? m_global_grid : m_project_grid;
}


void DIALOG_SYMBOL_LIB_TABLE::browseLibrariesHandler( wxCommandEvent& event )
{
    wxFileDialog dlg( this, _( "Select Library" ), Prj().GetProjectPath(),
            wxEmptyString, SchematicLibraryFileWildcard(),
            wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxArrayString files;
    dlg.GetFilenames( files );

    for( const auto& file : files )
    {
        wxString filePath = dlg.GetDirectory() + wxFileName::GetPathSeparator() + file;

        if( m_cur_grid->AppendRows( 1 ) )
        {
            int last_row = m_cur_grid->GetNumberRows() - 1;
            wxFileName fn( filePath );

            m_cur_grid->SetCellValue( last_row, COL_NICKNAME, fn.GetName() );

            // TODO the following code can detect only schematic types, not libs
            // SCH_IO_MGR needs to provide file extension information for libraries too

            // auto detect the plugin type
            /*for( auto pluginType : SCH_IO_MGR::SCH_FILE_T_vector )
            {
                if( SCH_IO_MGR::GetFileExtension( pluginType ).Lower() == fn.GetExt().Lower() )
                {
                    m_cur_grid->SetCellValue( last_row, COL_TYPE,
                            SCH_IO_MGR::ShowType( pluginType ) );
                    break;
                }
            }*/
            m_cur_grid->SetCellValue( last_row, COL_TYPE,
                    SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY ) );

            // try to use path normalized to an environmental variable or project path
            wxString normalizedPath = NormalizePath( filePath, &Pgm().GetLocalEnvVariables(), &Prj() );
            m_cur_grid->SetCellValue( last_row, COL_URI,
                    normalizedPath.IsEmpty() ? fn.GetFullPath() : normalizedPath );
        }
    }

    if( !files.IsEmpty() )
        scrollToRow( m_cur_grid->GetNumberRows() - 1 );  // scroll to the new libraries
}


void DIALOG_SYMBOL_LIB_TABLE::appendRowHandler( wxCommandEvent& event )
{
    if( m_cur_grid->AppendRows( 1 ) )
    {
        int row = m_cur_grid->GetNumberRows() - 1;
        // Gives a default type (currently, only one type exists):
        m_cur_grid->SetCellValue( row, COL_TYPE, SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY ) );
        scrollToRow( row );
    }
}


void DIALOG_SYMBOL_LIB_TABLE::deleteRowHandler( wxCommandEvent& event )
{
    int currRow = getCursorRow();

    // In a wxGrid, collect rows that have a selected cell, or are selected
    // is not so easy: it depend on the way the selection was made.
    // Here, we collect row selected by clicking on a row label, and
    // row that contain a cell previously selected.
    // If no candidate, just delete the row with the grid cursor.
    wxArrayInt selectedRows	= m_cur_grid->GetSelectedRows();
    wxGridCellCoordsArray cells = m_cur_grid->GetSelectedCells();

    // Add all row having cell selected to list:
    for( unsigned ii = 0; ii < cells.GetCount(); ii++ )
        selectedRows.Add( cells[ii].GetRow() );

    // Use the row having the grid cursor only if we have no candidate:
    if( selectedRows.size() == 0 && getCursorRow() >= 0 )
        selectedRows.Add( getCursorRow() );

    std::sort( selectedRows.begin(), selectedRows.end() );

    // Remove selected rows (note: a row can be stored more than once in list)
    int last_row = -1;

    for( int ii = selectedRows.GetCount()-1; ii >= 0; ii-- )
    {
        int row = selectedRows[ii];

        if( row != last_row )
        {
            last_row = row;
            m_cur_grid->DeleteRows( row, 1 );
        }
    }

    if( currRow >= m_cur_grid->GetNumberRows() )
        m_cur_grid->SetGridCursor(m_cur_grid->GetNumberRows()-1, getCursorCol() );

    m_cur_grid->SelectRow( m_cur_grid->GetGridCursorRow() );
}


void DIALOG_SYMBOL_LIB_TABLE::moveUpHandler( wxCommandEvent& event )
{
    wxArrayInt rowsSelected = m_cur_grid->GetSelectedRows();

    if( rowsSelected.GetCount() == 0 )
        return;

    // @todo: add multiple selection moves.
    int curRow = rowsSelected[0];

    if( curRow >= 1 )
    {
        int curCol = getCursorCol();

        SYMBOL_LIB_TABLE_GRID* tbl = cur_model();

        boost::ptr_vector< LIB_TABLE_ROW >::auto_type move_me =
            tbl->rows.release( tbl->rows.begin() + curRow );

        --curRow;
        tbl->rows.insert( tbl->rows.begin() + curRow, move_me.release() );

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


void DIALOG_SYMBOL_LIB_TABLE::moveDownHandler( wxCommandEvent& event )
{
    wxArrayInt rowsSelected = m_cur_grid->GetSelectedRows();

    if( rowsSelected.GetCount() == 0 )
        return;

    SYMBOL_LIB_TABLE_GRID* tbl = cur_model();

    // @todo: add multiple selection moves.
    int curRow = rowsSelected[0];

    if( unsigned( curRow + 1 ) < tbl->rows.size() )
    {
        int curCol = getCursorCol();

        boost::ptr_vector< LIB_TABLE_ROW >::auto_type move_me =
            tbl->rows.release( tbl->rows.begin() + curRow );

        ++curRow;
        tbl->rows.insert( tbl->rows.begin() + curRow, move_me.release() );

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


bool DIALOG_SYMBOL_LIB_TABLE::TransferDataFromWindow()
{
    // stuff any pending cell editor text into the table.
    m_cur_grid->SaveEditControlValue();

    if( !wxDialog::TransferDataFromWindow() || !verifyTables() )
        return false;

    if( *global_model() != *m_global )
    {
        m_global->Clear();
        m_global->rows.transfer( m_global->rows.end(), global_model()->rows.begin(),
                                 global_model()->rows.end(), global_model()->rows );
        m_global->reindex();
    }

    if( *project_model() != *m_project )
    {
        m_project->Clear();
        m_project->rows.transfer( m_project->rows.end(), project_model()->rows.begin(),
                                  project_model()->rows.end(), project_model()->rows );
        m_project->reindex();
    }

    return true;
}


void DIALOG_SYMBOL_LIB_TABLE::populateEnvironReadOnlyTable()
{
    wxRegEx re( ".*?\\$\\{(.+?)\\}.*?", wxRE_ADVANCED );
    wxASSERT( re.IsValid() );   // wxRE_ADVANCED is required.

    std::set< wxString >        unique;
    typedef std::set<wxString>::const_iterator      SET_CITER;

    // clear the table
    m_path_subs_grid->DeleteRows( 0, m_path_subs_grid->GetNumberRows() );

    SYMBOL_LIB_TABLE_GRID*   gbl = global_model();
    SYMBOL_LIB_TABLE_GRID*   prj = project_model();

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
    unique.insert( SYMBOL_LIB_TABLE::GlobalPathEnvVariableName() );

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


void DIALOG_SYMBOL_LIB_TABLE::scrollToRow( int aRowNumber )
{
    // wx documentation is wrong, SetGridCursor does not make visible.
    m_cur_grid->MakeCellVisible( aRowNumber, 0 );
    m_cur_grid->SetGridCursor( aRowNumber, 0 );
    m_cur_grid->SelectRow( m_cur_grid->GetGridCursorRow() );
}


SYMBOL_LIB_TABLE_GRID* DIALOG_SYMBOL_LIB_TABLE::global_model() const
{
    return (SYMBOL_LIB_TABLE_GRID*) m_global_grid->GetTable();
}


SYMBOL_LIB_TABLE_GRID* DIALOG_SYMBOL_LIB_TABLE::project_model() const
{
    return (SYMBOL_LIB_TABLE_GRID*) m_project_grid->GetTable();
}


SYMBOL_LIB_TABLE_GRID* DIALOG_SYMBOL_LIB_TABLE::cur_model() const
{
    return (SYMBOL_LIB_TABLE_GRID*) m_cur_grid->GetTable();
}


int DIALOG_SYMBOL_LIB_TABLE::m_pageNdx = 0;
