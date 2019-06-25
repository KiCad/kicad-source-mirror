/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 CERN
 * Copyright (C) 2012-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/grid.h>

#include <fctsys.h>
#include <project.h>
#include <3d_viewer/eda_3d_viewer.h>      // for KISYS3DMOD
#include <panel_fp_lib_table.h>
#include <lib_id.h>
#include <fp_lib_table.h>
#include <lib_table_lexer.h>
#include <invoke_pcb_dialog.h>
#include <bitmaps.h>
#include <grid_tricks.h>
#include <widgets/wx_grid.h>
#include <confirm.h>
#include <lib_table_grid.h>
#include <wildcards_and_files_ext.h>
#include <pgm_base.h>
#include <pcb_edit_frame.h>
#include <env_paths.h>
#include <dialogs/dialog_file_dir_picker.h>
#include <dialog_edit_library_tables.h>
#include <footprint_viewer_frame.h>
#include <footprint_edit_frame.h>
#include <kiway.h>
#include <widgets/grid_readonly_text_helpers.h>
#include <widgets/grid_text_button_helpers.h>

// Filters for the file picker
static constexpr int FILTER_COUNT = 4;
static const struct
{
    wxString m_Description; ///< Description shown in the file picker dialog
    wxString m_Extension;   ///< In case of folders it stands for extensions of files stored inside
    bool m_IsFile;          ///< Whether the library is a folder or a file
    IO_MGR::PCB_FILE_T m_Plugin;
} fileFilters[FILTER_COUNT] =
{
    // wxGenericDirCtrl does not handle regexes in wildcards
    { "KiCad (folder with .kicad_mod files)", "",    false, IO_MGR::KICAD_SEXP },
    { "Eagle 6.x (*.lbr)",                    "lbr", true,  IO_MGR::EAGLE },
    { "KiCad legacy (*.mod)",                 "mod", true,  IO_MGR::LEGACY },
    { "Geda (folder with *.fp files)",        "",    false, IO_MGR::GEDA_PCB },
};


// Returns the filter string for the file picker
static wxString getFilterString()
{
    wxString filterAll = _( "All supported library formats|" );
    bool firstFilterAll = true;
    wxString filter;

    for( int i = 0; i < FILTER_COUNT; ++i )
    {
        if( fileFilters[i].m_IsFile )
        {
            // "All supported formats" filter
            if( firstFilterAll )
                firstFilterAll = false;
            else
                filterAll += ";";

            wxASSERT( !fileFilters[i].m_Extension.IsEmpty() );
            filterAll += "*." + fileFilters[i].m_Extension;
        }


        // Individual filter strings
        filter += "|" + fileFilters[i].m_Description +
                  "|" + ( fileFilters[i].m_IsFile ? "*." + fileFilters[i].m_Extension : "" );
    }

    return filterAll + filter;
}


/**
 * This class builds a wxGridTableBase by wrapping an #FP_LIB_TABLE object.
 */
class FP_LIB_TABLE_GRID : public LIB_TABLE_GRID, public FP_LIB_TABLE
{
    friend class PANEL_FP_LIB_TABLE;
    friend class FP_GRID_TRICKS;

protected:
    LIB_TABLE_ROW* at( size_t aIndex ) override { return &rows.at( aIndex ); }

    size_t size() const override { return rows.size(); }

    LIB_TABLE_ROW* makeNewRow() override
    {
        return dynamic_cast< LIB_TABLE_ROW* >( new FP_LIB_TABLE_ROW );
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

    FP_LIB_TABLE_GRID( const FP_LIB_TABLE& aTableToEdit )
    {
        rows = aTableToEdit.rows;
    }
};


#define MYID_OPTIONS_EDITOR  15151


class FP_GRID_TRICKS : public GRID_TRICKS
{
public:
    FP_GRID_TRICKS( DIALOG_EDIT_LIBRARY_TABLES* aParent, WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid ),
            m_dialog( aParent )
    { }

protected:
    DIALOG_EDIT_LIBRARY_TABLES* m_dialog;

    void optionsEditor( int aRow )
    {
        FP_LIB_TABLE_GRID* tbl = (FP_LIB_TABLE_GRID*) m_grid->GetTable();

        if( tbl->GetNumberRows() > aRow )
        {
            LIB_TABLE_ROW*  row = tbl->at( (size_t) aRow );
            const wxString& options = row->GetOptions();
            wxString        result = options;

            InvokePluginOptionsEditor( m_dialog, row->GetNickName(), row->GetType(), options,
                                       &result );

            if( options != result )
            {
                row->SetOptions( result );
                m_grid->Refresh();
            }
        }
    }

    bool handleDoubleClick( wxGridEvent& aEvent ) override
    {
        if( aEvent.GetCol() == COL_OPTIONS )
        {
            optionsEditor( aEvent.GetRow() );
            return true;
        }

        return false;
    }

    void showPopupMenu( wxMenu& menu ) override
    {
        if( m_grid->GetGridCursorCol() == COL_OPTIONS )
        {
            menu.Append( MYID_OPTIONS_EDITOR, _( "Options Editor..." ), _( "Edit options" ) );
            menu.AppendSeparator();
        }

        GRID_TRICKS::showPopupMenu( menu );
    }

    void doPopupSelection( wxCommandEvent& event ) override
    {
        if( event.GetId() == MYID_OPTIONS_EDITOR )
            optionsEditor( m_grid->GetGridCursorRow() );
        else
            GRID_TRICKS::doPopupSelection( event );
    }

    /// handle specialized clipboard text, with leading "(fp_lib_table", OR
    /// spreadsheet formatted text.
    void paste_text( const wxString& cb_text ) override
    {
        FP_LIB_TABLE_GRID* tbl = (FP_LIB_TABLE_GRID*) m_grid->GetTable();
        size_t             ndx = cb_text.find( "(fp_lib_table" );

        if( ndx != std::string::npos )
        {
            // paste the FP_LIB_TABLE_ROWs of s-expression (fp_lib_table), starting
            // at column 0 regardless of current cursor column.

            STRING_LINE_READER  slr( TO_UTF8( cb_text ), "Clipboard" );
            LIB_TABLE_LEXER     lexer( &slr );
            FP_LIB_TABLE        tmp_tbl;
            bool                parsed = true;

            try
            {
                tmp_tbl.Parse( &lexer );
            }
            catch( PARSE_ERROR& pe )
            {
                DisplayError( m_dialog, pe.What() );
                parsed = false;
            }

            if( parsed )
            {
                // make sure the table is big enough...
                if( tmp_tbl.GetCount() > (unsigned) tbl->GetNumberRows() )
                    tbl->AppendRows( tmp_tbl.GetCount() - tbl->GetNumberRows() );

                for( unsigned i = 0;  i < tmp_tbl.GetCount();  ++i )
                    tbl->rows.replace( i, tmp_tbl.At( i ).clone() );
            }

            m_grid->AutoSizeColumns( false );
        }
        else
        {
            // paste spreadsheet formatted text.
            GRID_TRICKS::paste_text( cb_text );

            m_grid->AutoSizeColumns( false );
        }
    }
};


PANEL_FP_LIB_TABLE::PANEL_FP_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent,
                                        FP_LIB_TABLE* aGlobal, const wxString& aGlobalTblPath,
                                        FP_LIB_TABLE* aProject, const wxString& aProjectTblPath,
                                        const wxString& aProjectBasePath ) :
    PANEL_FP_LIB_TABLE_BASE( aParent ),
    m_global( aGlobal ),
    m_project( aProject ),
    m_projectBasePath( aProjectBasePath ),
    m_parent( aParent )
{
    // For user info, shows the table filenames:
    m_GblTableFilename->SetLabel( aGlobalTblPath );
    m_PrjTableFilename->SetLabel( aProjectTblPath );

    m_global_grid->SetTable(  new FP_LIB_TABLE_GRID( *aGlobal ),  true );
    m_project_grid->SetTable( new FP_LIB_TABLE_GRID( *aProject ), true );

    // Give a bit more room for wxChoice editors
    m_global_grid->SetDefaultRowSize( m_global_grid->GetDefaultRowSize() + 4 );
    m_project_grid->SetDefaultRowSize( m_project_grid->GetDefaultRowSize() + 4 );

    // add Cut, Copy, and Paste to wxGrids
    m_global_grid->PushEventHandler( new FP_GRID_TRICKS( m_parent, m_global_grid ) );
    m_project_grid->PushEventHandler( new FP_GRID_TRICKS( m_parent, m_project_grid ) );
    m_path_subs_grid->PushEventHandler( new GRID_TRICKS( m_path_subs_grid ) );

    m_global_grid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_project_grid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_global_grid->AutoSizeColumns( false );
    m_project_grid->AutoSizeColumns( false );

    wxArrayString choices;

    choices.Add( IO_MGR::ShowType( IO_MGR::KICAD_SEXP ) );
#if defined(BUILD_GITHUB_PLUGIN)
    choices.Add( IO_MGR::ShowType( IO_MGR::GITHUB ) );
#endif
    choices.Add( IO_MGR::ShowType( IO_MGR::LEGACY ) );
    choices.Add( IO_MGR::ShowType( IO_MGR::EAGLE ) );
    choices.Add( IO_MGR::ShowType( IO_MGR::GEDA_PCB ) );

    /* PCAD_PLUGIN does not support Footprint*() functions
    choices.Add( IO_MGR::ShowType( IO_MGR::PCAD ) );
    */

    populateEnvironReadOnlyTable();

    for( wxGrid* g : { m_global_grid, m_project_grid } )
    {
        wxGridCellAttr* attr;

        attr = new wxGridCellAttr;
        attr->SetEditor( new GRID_CELL_PATH_EDITOR( m_parent, &m_lastBrowseDir ) );
        g->SetColAttr( COL_URI, attr );

        attr = new wxGridCellAttr;
        attr->SetEditor( new wxGridCellChoiceEditor( choices ) );
        g->SetColAttr( COL_TYPE, attr );

        attr = new wxGridCellAttr;
        attr->SetRenderer( new wxGridCellBoolRenderer() );
        attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
        g->SetColAttr( COL_ENABLED, attr );

        // all but COL_OPTIONS, which is edited with Option Editor anyways.
        g->AutoSizeColumn( COL_NICKNAME, false );
        g->AutoSizeColumn( COL_TYPE, false );
        g->AutoSizeColumn( COL_URI, false );
        g->AutoSizeColumn( COL_DESCR, false );

        // would set this to width of title, if it was easily known.
        g->SetColSize( COL_OPTIONS, 80 );
    }

    m_path_subs_grid->SetColLabelValue( 0, _( "Name" ) );
    m_path_subs_grid->SetColLabelValue( 1, _( "Value" ) );

    // select the last selected page
    m_auinotebook->SetSelection( m_pageNdx );
    m_cur_grid = ( m_pageNdx == 0 ) ? m_global_grid : m_project_grid;

    // for ALT+A handling, we want the initial focus to be on the first selected grid.
    m_parent->SetInitialFocus( m_cur_grid );

    // Configure button logos
    m_append_button->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_browse_button->SetBitmap( KiBitmap( folder_xpm ) );
    m_delete_button->SetBitmap( KiBitmap( trash_xpm ) );
    m_move_up_button->SetBitmap( KiBitmap( small_up_xpm ) );
    m_move_down_button->SetBitmap( KiBitmap( small_down_xpm ) );

    // Gives a selection to each grid, mainly for delete button.  wxGrid's wake up with
    // a currentCell which is sometimes not highlighted.
    if( m_global_grid->GetNumberRows() > 0 )
        m_global_grid->SelectRow( 0 );

    if( m_project_grid->GetNumberRows() > 0 )
        m_project_grid->SelectRow( 0 );
}


PANEL_FP_LIB_TABLE::~PANEL_FP_LIB_TABLE()
{
    // When the dialog is closed it will hide the current notebook page first, which will
    // in turn select the other one.  We then end up saving its index as the "current page".
    // So flip them back again:
    m_pageNdx = m_pageNdx == 1 ? 0 : 1;

    // Delete the GRID_TRICKS.
    // Any additional event handlers should be popped before the window is deleted.
    m_global_grid->PopEventHandler( true );
    m_project_grid->PopEventHandler( true );
    m_path_subs_grid->PopEventHandler( true );
}


bool PANEL_FP_LIB_TABLE::verifyTables()
{
    for( FP_LIB_TABLE_GRID* model : { global_model(), project_model() } )
    {
        for( int r = 0; r < model->GetNumberRows(); )
        {
            wxString nick = model->GetValue( r, COL_NICKNAME ).Trim( false ).Trim();
            wxString uri  = model->GetValue( r, COL_URI ).Trim( false ).Trim();
            unsigned illegalCh = 0;

            if( !nick || !uri )
            {
                // Delete the "empty" row, where empty means missing nick or uri.
                // This also updates the UI which could be slow, but there should only be a few
                // rows to delete, unless the user fell asleep on the Add Row
                // button.
                model->DeleteRows( r, 1 );
            }
        else if( ( illegalCh = LIB_ID::FindIllegalLibNicknameChar( nick, LIB_ID::ID_PCB ) ) )
            {
                wxString msg = wxString::Format( _( "Illegal character '%c' in Nickname: \"%s\"" ),
                                                 illegalCh,
                                                 nick );

                // show the tabbed panel holding the grid we have flunked:
                if( model != cur_model() )
                    m_auinotebook->SetSelection( model == global_model() ? 0 : 1 );

                m_cur_grid->MakeCellVisible( r, 0 );
                m_cur_grid->SetGridCursor( r, 1 );

                wxMessageDialog errdlg( this, msg, _( "No Colon in Nicknames" ) );
                errdlg.ShowModal();
                return false;
            }
            else
            {
                // set the trimmed values back into the table so they get saved to disk.
                model->SetValue( r, COL_NICKNAME, nick );
                model->SetValue( r, COL_URI, uri );
                ++r;        // this row was OK.
            }
        }
    }

    // check for duplicate nickNames, separately in each table.
    for( FP_LIB_TABLE_GRID* model : { global_model(), project_model() } )
    {
        for( int r1 = 0; r1 < model->GetNumberRows() - 1; ++r1 )
        {
            wxString nick1 = model->GetValue( r1, COL_NICKNAME );

            for( int r2 = r1 + 1; r2 < model->GetNumberRows(); ++r2 )
            {
                wxString nick2 = model->GetValue( r2, COL_NICKNAME );

                if( nick1 == nick2 )
                {
                    wxString msg = wxString::Format( _( "Duplicate Nicknames \"%s\"." ), nick1 );

                    // show the tabbed panel holding the grid we have flunked:
                    if( model != cur_model() )
                        m_auinotebook->SetSelection( model == global_model() ? 0 : 1 );

                    // go to the lower of the two rows, it is technically the duplicate:
                    m_cur_grid->MakeCellVisible( r2, 0 );
                    m_cur_grid->SetGridCursor( r2, 1 );

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

void PANEL_FP_LIB_TABLE::pageChangedHandler( wxAuiNotebookEvent& event )
{
    m_pageNdx = (unsigned) std::max( 0, m_auinotebook->GetSelection() );
    m_cur_grid = m_pageNdx == 0 ? m_global_grid : m_project_grid;
}


void PANEL_FP_LIB_TABLE::appendRowHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    if( m_cur_grid->AppendRows( 1 ) )
    {
        int last_row = m_cur_grid->GetNumberRows() - 1;

        // wx documentation is wrong, SetGridCursor does not make visible.
        m_cur_grid->MakeCellVisible( last_row, 0 );
        m_cur_grid->SetGridCursor( last_row, 1 );
        m_cur_grid->EnableCellEditControl( true );
        m_cur_grid->ShowCellEditControl();
    }
}


void PANEL_FP_LIB_TABLE::deleteRowHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    int curRow = m_cur_grid->GetGridCursorRow();
    int curCol = m_cur_grid->GetGridCursorCol();

    // In a wxGrid, collect rows that have a selected cell, or are selected
    // is not so easy: it depend on the way the selection was made.
    // Here, we collect row selected by clicking on a row label, and
    // row that contain a cell previously selected.
    // If no candidate, just delete the row with the grid cursor.
    wxArrayInt selectedRows	= m_cur_grid->GetSelectedRows();
    wxGridCellCoordsArray cells = m_cur_grid->GetSelectedCells();
    wxGridCellCoordsArray blockTopLeft = m_cur_grid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray blockBotRight = m_cur_grid->GetSelectionBlockBottomRight();

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
    if( selectedRows.size() == 0 && m_cur_grid->GetGridCursorRow() >= 0 )
        selectedRows.Add( m_cur_grid->GetGridCursorRow() );

    if( selectedRows.size() == 0 )
    {
        wxBell();
        return;
    }

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

    m_cur_grid->SetGridCursor( std::min( curRow, m_cur_grid->GetNumberRows() - 1 ), curCol );
}


void PANEL_FP_LIB_TABLE::moveUpHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    FP_LIB_TABLE_GRID* tbl = cur_model();
    int curRow = m_cur_grid->GetGridCursorRow();

    // @todo: add multiple selection moves.
    if( curRow >= 1 )
    {
        boost::ptr_vector< LIB_TABLE_ROW >::auto_type move_me =
            tbl->rows.release( tbl->rows.begin() + curRow );

        --curRow;
        tbl->rows.insert( tbl->rows.begin() + curRow, move_me.release() );

        if( tbl->GetView() )
        {
            // Update the wxGrid
            wxGridTableMessage msg( tbl, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, curRow, 0 );
            tbl->GetView()->ProcessTableMessage( msg );
        }

        m_cur_grid->MakeCellVisible( curRow, m_cur_grid->GetGridCursorCol() );
        m_cur_grid->SetGridCursor( curRow, m_cur_grid->GetGridCursorCol() );
    }
}


void PANEL_FP_LIB_TABLE::moveDownHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    FP_LIB_TABLE_GRID* tbl = cur_model();
    int curRow = m_cur_grid->GetGridCursorRow();

    // @todo: add multiple selection moves.
    if( unsigned( curRow + 1 ) < tbl->rows.size() )
    {
        boost::ptr_vector< LIB_TABLE_ROW >::auto_type move_me =
            tbl->rows.release( tbl->rows.begin() + curRow );

        ++curRow;
        tbl->rows.insert( tbl->rows.begin() + curRow, move_me.release() );

        if( tbl->GetView() )
        {
            // Update the wxGrid
            wxGridTableMessage msg( tbl, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, curRow - 1, 0 );
            tbl->GetView()->ProcessTableMessage( msg );
        }

        m_cur_grid->MakeCellVisible( curRow, m_cur_grid->GetGridCursorCol() );
        m_cur_grid->SetGridCursor( curRow, m_cur_grid->GetGridCursorCol() );
    }
}


void PANEL_FP_LIB_TABLE::browseLibrariesHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    if( m_lastBrowseDir.IsEmpty() )
        m_lastBrowseDir = m_projectBasePath;

    DIALOG_FILE_DIR_PICKER dlg( this, _( "Select Library" ), m_lastBrowseDir,
                                getFilterString(), FD_MULTIPLE );

    auto result = dlg.ShowModal();

    if( result == wxID_CANCEL )
        return;

    m_lastBrowseDir = dlg.GetDirectory();

    // Drop the last directory if the path is a .pretty folder
    if( m_lastBrowseDir.EndsWith( KiCadFootprintLibPathExtension ) )
        m_lastBrowseDir = m_lastBrowseDir.BeforeLast( wxFileName::GetPathSeparator() );

    const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();
    bool addDuplicates = false;
    bool applyToAll = false;
    wxString warning = _( "Warning: Duplicate Nickname" );
    wxString msg = _( "A library nicknamed \"%s\" already exists." );
    wxArrayString files;
    dlg.GetFilenames( files );

    for( const auto& filePath : files )
    {
        wxFileName fn( filePath );
        wxString nickname = LIB_ID::FixIllegalChars( fn.GetName(), LIB_ID::ID_PCB );
        bool doAdd = true;

        if( cur_model()->ContainsNickname( nickname ) )
        {
            if( !applyToAll )
            {
                int ret = OKOrCancelDialog( this, warning, wxString::Format( msg, nickname ),
                                            _( "Skip" ), _( "Add Anyway" ), &applyToAll );
                addDuplicates = (ret == wxID_CANCEL );
            }

            doAdd = addDuplicates;
        }

        if( doAdd && m_cur_grid->AppendRows( 1 ) )
        {
            int last_row = m_cur_grid->GetNumberRows() - 1;

            m_cur_grid->SetCellValue( last_row, COL_NICKNAME, nickname );

            auto type = IO_MGR::GuessPluginTypeFromLibPath( filePath );
            m_cur_grid->SetCellValue( last_row, COL_TYPE, IO_MGR::ShowType( type ) );

            // try to use path normalized to an environmental variable or project path
            wxString path = NormalizePath( filePath, &envVars, m_projectBasePath );

            // Do not use the project path in the global library table.  This will almost
            // assuredly be wrong for a different project.
            if( path.IsEmpty() || (m_pageNdx == 0 && path.Contains( "${KIPRJMOD}" )) )
                path = fn.GetFullPath();

            m_cur_grid->SetCellValue( last_row, COL_URI, path );
        }
    }

    if( !files.IsEmpty() )
    {
        int new_row = m_cur_grid->GetNumberRows() - 1;
        m_cur_grid->MakeCellVisible( new_row, m_cur_grid->GetGridCursorCol() );
        m_cur_grid->SetGridCursor( new_row, m_cur_grid->GetGridCursorCol() );
    }
}

void PANEL_FP_LIB_TABLE::adjustPathSubsGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_path_subs_grid->GetSize().x - m_path_subs_grid->GetClientSize().x );

    m_path_subs_grid->AutoSizeColumn( 0 );
    m_path_subs_grid->SetColSize( 1, aWidth - m_path_subs_grid->GetColSize( 0 ) );
}


void PANEL_FP_LIB_TABLE::onSizeGrid( wxSizeEvent& event )
{
    adjustPathSubsGridColumns( event.GetSize().GetX() );

    event.Skip();
}


bool PANEL_FP_LIB_TABLE::TransferDataFromWindow()
{
    if( !m_cur_grid->CommitPendingChanges() )
        return false;

    if( verifyTables() )
    {
        if( *global_model() != *m_global )
        {
            m_parent->m_GlobalTableChanged = true;

            m_global->Clear();
            m_global->rows.transfer( m_global->rows.end(), global_model()->rows.begin(),
                                     global_model()->rows.end(), global_model()->rows );
            m_global->reindex();
        }

        if( *project_model() != *m_project )
        {
            m_parent->m_ProjectTableChanged = true;

            m_project->Clear();
            m_project->rows.transfer( m_project->rows.end(), project_model()->rows.begin(),
                                      project_model()->rows.end(), project_model()->rows );
            m_project->reindex();
        }

        return true;
    }

    return false;
}


/// Populate the readonly environment variable table with names and values
/// by examining all the full_uri columns.
void PANEL_FP_LIB_TABLE::populateEnvironReadOnlyTable()
{
    wxRegEx re( ".*?(\\$\\{(.+?)\\})|(\\$\\((.+?)\\)).*?", wxRE_ADVANCED );
    wxASSERT( re.IsValid() );   // wxRE_ADVANCED is required.

    std::set< wxString > unique;

    // clear the table
    m_path_subs_grid->DeleteRows( 0, m_path_subs_grid->GetNumberRows() );

    for( FP_LIB_TABLE_GRID* tbl : { global_model(), project_model() } )
    {
        for( int row = 0; row < tbl->GetNumberRows(); ++row )
        {
            wxString uri = tbl->GetValue( row, COL_URI );

            while( re.Matches( uri ) )
            {
                wxString envvar = re.GetMatch( uri, 2 );

                // if not ${...} form then must be $(...)
                if( envvar.IsEmpty() )
                    envvar = re.GetMatch( uri, 4 );

                // ignore duplicates
                unique.insert( envvar );

                // delete the last match and search again
                uri.Replace( re.GetMatch( uri, 0 ), wxEmptyString );
            }
        }
    }

    // Make sure this special environment variable shows up even if it was
    // not used yet.  It is automatically set by KiCad to the directory holding
    // the current project.
    unique.insert( PROJECT_VAR_NAME );
    unique.insert( FP_LIB_TABLE::GlobalPathEnvVariableName() );
    // This special environment variable is used to locate 3d shapes
    unique.insert( KISYS3DMOD );

    for( wxString evName : unique )
    {
        int row = m_path_subs_grid->GetNumberRows();
        m_path_subs_grid->AppendRows( 1 );

        m_path_subs_grid->SetCellValue( row, 0, wxT( "${" ) + evName + wxT( "}" ) );
        m_path_subs_grid->SetCellEditor( row, 0, new GRID_CELL_READONLY_TEXT_EDITOR() );

        wxString evValue;
        wxGetEnv( evName, &evValue );
        m_path_subs_grid->SetCellValue( row, 1, evValue );
        m_path_subs_grid->SetCellEditor( row, 1, new GRID_CELL_READONLY_TEXT_EDITOR() );
    }

    // No combobox editors here, but it looks better if its consistent with the other
    // grids in the dialog.
    m_path_subs_grid->SetDefaultRowSize( m_path_subs_grid->GetDefaultRowSize() + 2 );

    adjustPathSubsGridColumns( m_path_subs_grid->GetRect().GetWidth() );
}

//-----</event handlers>---------------------------------



size_t   PANEL_FP_LIB_TABLE::m_pageNdx = 0;

wxString PANEL_FP_LIB_TABLE::m_lastBrowseDir;


void InvokePcbLibTableEditor( KIWAY* aKiway, wxWindow* aCaller )
{
    FP_LIB_TABLE* globalTable = &GFootprintTable;
    wxString      globalTablePath = FP_LIB_TABLE::GetGlobalTableFileName();
    FP_LIB_TABLE* projectTable = aKiway->Prj().PcbFootprintLibs();
    wxString      projectTablePath = aKiway->Prj().FootprintLibTblName();
    wxString      msg;

    DIALOG_EDIT_LIBRARY_TABLES dlg( aCaller, _( "Footprint Libraries" ) );
    dlg.SetKiway( &dlg, aKiway );

    dlg.InstallPanel( new PANEL_FP_LIB_TABLE( &dlg, globalTable, globalTablePath,
                                              projectTable, projectTablePath,
                                              aKiway->Prj().GetProjectPath() ) );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( dlg.m_GlobalTableChanged )
    {
        try
        {
            globalTable->Save( globalTablePath );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error saving global library table:\n\n%s" ), ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    if( dlg.m_ProjectTableChanged )
    {
        try
        {
            projectTable->Save( projectTablePath );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error saving project-specific library table:\n\n%s" ), ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    auto editor = (FOOTPRINT_EDIT_FRAME*) aKiway->Player( FRAME_PCB_MODULE_EDITOR, false );

    if( editor )
        editor->SyncLibraryTree( true );

    auto viewer = (FOOTPRINT_VIEWER_FRAME*) aKiway->Player( FRAME_PCB_MODULE_VIEWER, false );

    if( viewer )
        viewer->ReCreateLibraryList();
}
