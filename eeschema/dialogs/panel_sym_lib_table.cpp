/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2021 CERN
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <common.h>     // For ExpandEnvVarSubstitutions
#include <project.h>
#include <panel_sym_lib_table.h>
#include <lib_id.h>
#include <symbol_lib_table.h>
#include <lib_table_lexer.h>
#include <grid_tricks.h>
#include <widgets/wx_grid.h>
#include <confirm.h>
#include <bitmaps.h>
#include <lib_table_grid.h>
#include <wildcards_and_files_ext.h>
#include <env_paths.h>
#include <eeschema_id.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <sch_edit_frame.h>
#include <kiway.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <widgets/grid_readonly_text_helpers.h>
#include <widgets/grid_text_button_helpers.h>
#include <sch_file_versions.h>
#include <wx/filedlg.h>



// clang-format off

/**
 * Container that describes file type info for the add a library options
 */
struct SUPPORTED_FILE_TYPE
{
    wxString m_Description;            ///< Description shown in the file picker dialog
    wxString m_FileFilter;             ///< Filter used for file pickers if m_IsFile is true
    wxString m_FolderSearchExtension;  ///< In case of folders it stands for extensions of files stored inside
    bool     m_IsFile;                 ///< Whether the library is a folder or a file
    SCH_IO_MGR::SCH_FILE_T m_Plugin;
};


/**
 * Event IDs for the menu items in the split button menu for add a library
 */
enum {
    ID_PANEL_SYM_LIB_KICAD = ID_END_EESCHEMA_ID_LIST,
    ID_PANEL_SYM_LIB_LEGACY,
};


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
    SYMBOL_GRID_TRICKS( DIALOG_EDIT_LIBRARY_TABLES* aParent, WX_GRID* aGrid ) :
        GRID_TRICKS( aGrid ),
        m_dialog( aParent )
    {
    }

protected:
    DIALOG_EDIT_LIBRARY_TABLES* m_dialog;

    /// handle specialized clipboard text, with leading "(sym_lib_table" or
    /// spreadsheet formatted text.
    virtual void paste_text( const wxString& cb_text ) override
    {
        SYMBOL_LIB_TABLE_GRID* tbl = (SYMBOL_LIB_TABLE_GRID*) m_grid->GetTable();
        size_t                 ndx = cb_text.find( "(sym_lib_table" );

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


PANEL_SYM_LIB_TABLE::PANEL_SYM_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* aProject ,
                                          SYMBOL_LIB_TABLE* aGlobalTable,
                                          const wxString& aGlobalTablePath,
                                          SYMBOL_LIB_TABLE* aProjectTable,
                                          const wxString& aProjectTablePath ) :
    PANEL_SYM_LIB_TABLE_BASE( aParent ),
    m_globalTable( aGlobalTable ),
    m_projectTable( aProjectTable ),
    m_project( aProject ),
    m_parent( aParent )
{
    // wxGrid only supports user owned tables if they exist past end of ~wxGrid(),
    // so make it a grid owned table.
    m_global_grid->SetTable( new SYMBOL_LIB_TABLE_GRID( *m_globalTable ), true );

    wxArrayString pluginChoices;

    pluginChoices.Add( SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_KICAD ) );
    pluginChoices.Add( SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY ) );

    EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();

    if( cfg->m_lastSymbolLibDir.IsEmpty() )
        cfg->m_lastSymbolLibDir = PATHS::GetDefaultUserSymbolsPath();

    m_lastProjectLibDir = m_project->GetProjectPath();

    auto setupGrid =
            [&]( WX_GRID* aGrid )
            {
                // Give a bit more room for combobox editors
                aGrid->SetDefaultRowSize( aGrid->GetDefaultRowSize() + 4 );

                // add Cut, Copy, and Paste to wxGrids
                aGrid->PushEventHandler( new SYMBOL_GRID_TRICKS( m_parent, aGrid ) );

                aGrid->SetSelectionMode( wxGrid::wxGridSelectRows );
                aGrid->AutoSizeColumns( false );

                // Set special attributes
                wxGridCellAttr* attr;

                attr = new wxGridCellAttr;

                wxString wildcards = AllSymbolLibFilesWildcard()
                                     + "|" + KiCadSymbolLibFileWildcard()
                                     + "|" + LegacySymbolLibFileWildcard();
                attr->SetEditor( new GRID_CELL_PATH_EDITOR( m_parent, aGrid,
                                                            &cfg->m_lastSymbolLibDir, wildcards,
                                                            true, m_project->GetProjectPath() ) );
                aGrid->SetColAttr( COL_URI, attr );

                attr = new wxGridCellAttr;
                attr->SetEditor( new wxGridCellChoiceEditor( pluginChoices ) );
                aGrid->SetColAttr( COL_TYPE, attr );

                attr = new wxGridCellAttr;
                attr->SetRenderer( new wxGridCellBoolRenderer() );
                attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
                aGrid->SetColAttr( COL_ENABLED, attr );

                // all but COL_OPTIONS, which is edited with Option Editor anyways.
                aGrid->AutoSizeColumn( COL_NICKNAME, false );
                aGrid->AutoSizeColumn( COL_TYPE, false );
                aGrid->AutoSizeColumn( COL_URI, false );
                aGrid->AutoSizeColumn( COL_DESCR, false );
                aGrid->AutoSizeColumn( COL_ENABLED, false );

                // would set this to width of title, if it was easily known.
                aGrid->SetColSize( COL_OPTIONS, 80 );

                // Gives a selection to each grid, mainly for delete button.  wxGrid's wake up with
                // a currentCell which is sometimes not highlighted.
                if( aGrid->GetNumberRows() > 0 )
                    aGrid->SelectRow( 0 );
            };

    setupGrid( m_global_grid );

    if( m_projectTable )
    {
        m_project_grid->SetTable( new SYMBOL_LIB_TABLE_GRID( *m_projectTable ), true );
        setupGrid( m_project_grid );
    }
    else
    {
        m_pageNdx = 0;
        m_notebook->DeletePage( 1 );
        m_project_grid = nullptr;
    }

    // add Cut, Copy, and Paste to wxGrids
    m_path_subs_grid->PushEventHandler( new GRID_TRICKS( m_path_subs_grid ) );

    populateEnvironReadOnlyTable();

    // select the last selected page
    m_notebook->SetSelection( m_pageNdx );
    m_cur_grid = ( m_pageNdx == 0 ) ? m_global_grid : m_project_grid;

    m_path_subs_grid->SetColLabelValue( 0, _( "Name" ) );
    m_path_subs_grid->SetColLabelValue( 1, _( "Value" ) );

    // for ALT+A handling, we want the initial focus to be on the first selected grid.
    m_parent->SetInitialFocus( m_cur_grid );

    // Configure button logos
    m_append_button->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_delete_button->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_move_up_button->SetBitmap( KiBitmap( BITMAPS::small_up ) );
    m_move_down_button->SetBitmap( KiBitmap( BITMAPS::small_down ) );
    m_browse_button->SetBitmap( KiBitmap( BITMAPS::small_folder ) );
}


PANEL_SYM_LIB_TABLE::~PANEL_SYM_LIB_TABLE()
{
    // When the dialog is closed it will hide the current notebook page first, which will
    // in turn select the other one.  We then end up saving its index as the "current page".
    // So flip them back again:
    m_pageNdx = m_pageNdx == 1 ? 0 : 1;

    // Delete the GRID_TRICKS.
    // Any additional event handlers should be popped before the window is deleted.
    m_global_grid->PopEventHandler( true );

    if( m_project_grid )
        m_project_grid->PopEventHandler( true );

    m_path_subs_grid->PopEventHandler( true );
}


bool PANEL_SYM_LIB_TABLE::verifyTables()
{
    for( SYMBOL_LIB_TABLE_GRID* model : { global_model(), project_model() } )
    {
        if( !model )
            continue;

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
            else if( ( illegalCh = LIB_ID::FindIllegalLibraryNameChar( nick ) ) )
            {
                wxString msg = wxString::Format( _( "Illegal character '%c' in nickname '%s'" ),
                                                 illegalCh,
                                                 nick );

                // show the tabbed panel holding the grid we have flunked:
                if( model != cur_model() )
                    m_notebook->SetSelection( model == global_model() ? 0 : 1 );

                m_cur_grid->MakeCellVisible( r, 0 );
                m_cur_grid->SetGridCursor( r, 1 );

                wxMessageDialog errdlg( this, msg, _( "Library Nickname Error" ) );
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
    for( SYMBOL_LIB_TABLE_GRID* model : { global_model(), project_model() } )
    {
        if( !model )
            continue;

        for( int r1 = 0; r1 < model->GetNumberRows() - 1;  ++r1 )
        {
            wxString    nick1 = model->GetValue( r1, COL_NICKNAME );

            for( int r2=r1+1; r2 < model->GetNumberRows();  ++r2 )
            {
                wxString    nick2 = model->GetValue( r2, COL_NICKNAME );

                if( nick1 == nick2 )
                {
                    wxString msg = wxString::Format( _( "Multiple libraries cannot share the same "
                                                        "nickname ('%s')." ),
                                                     nick1 );

                    // show the tabbed panel holding the grid we have flunked:
                    if( model != cur_model() )
                        m_notebook->SetSelection( model == global_model() ? 0 : 1 );

                    // go to the lower of the two rows, it is technically the duplicate:
                    m_cur_grid->MakeCellVisible( r2, 0 );
                    m_cur_grid->SetGridCursor( r2, 1 );

                    wxMessageDialog errdlg( this, msg, _( "Library Nickname Error" ) );
                    errdlg.ShowModal();

                    return false;
                }
            }
        }
    }

    for( SYMBOL_LIB_TABLE* table : { global_model(), project_model() } )
    {
        if( !table )
            continue;

        for( unsigned int r = 0; r < table->GetCount(); ++r )
        {
            SYMBOL_LIB_TABLE_ROW& row = dynamic_cast<SYMBOL_LIB_TABLE_ROW&>( table->At( r ) );

            if( !row.GetIsEnabled() )
                continue;

            try
            {
                if( row.Refresh() )
                {
                    if( table == global_model() )
                        m_parent->m_GlobalTableChanged = true;
                    else
                        m_parent->m_ProjectTableChanged = true;
                }
            }
            catch( const IO_ERROR& ioe )
            {
                wxString msg = wxString::Format( _( "Symbol library \"%s\" failed to load.\n %s" ),
                                                 row.GetNickName(),
                                                 ioe.What() );

                wxMessageDialog errdlg( this, msg, _( "Error Loading Library" ) );
                errdlg.ShowModal();

                return false;
            }
        }
    }

    return true;
}


void PANEL_SYM_LIB_TABLE::OnUpdateUI( wxUpdateUIEvent& event )
{
    m_pageNdx = (unsigned) std::max( 0, m_notebook->GetSelection() );
    m_cur_grid = m_pageNdx == 0 ? m_global_grid : m_project_grid;
}


void PANEL_SYM_LIB_TABLE::browseLibrariesHandler( wxCommandEvent& event )
{
    wxString wildcards = AllSymbolLibFilesWildcard()
                            + "|" + KiCadSymbolLibFileWildcard()
                            + "|" + LegacySymbolLibFileWildcard();

    EESCHEMA_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<EESCHEMA_SETTINGS>();

    wxString openDir = cfg->m_lastSymbolLibDir;
    if( m_cur_grid == m_project_grid )
    {
        openDir = m_lastProjectLibDir;
    }

    wxFileDialog dlg( this, _( "Select Library" ),
                      openDir, wxEmptyString, wildcards,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( m_cur_grid == m_global_grid )
    {
        cfg->m_lastSymbolLibDir = dlg.GetPath();
    }
    else
    {
        m_lastProjectLibDir = dlg.GetPath();
    }

    const ENV_VAR_MAP& envVars       = Pgm().GetLocalEnvVariables();
    bool               addDuplicates = false;
    bool               applyToAll    = false;
    wxString           warning       = _( "Warning: Duplicate Nickname" );
    wxString           msg           = _( "A library nicknamed '%s' already exists." );
    wxString           detailedMsg   = _( "One of the nicknames will need to be changed after "
                                          "adding this library." );

    wxArrayString files;
    dlg.GetFilenames( files );

    for( const wxString& file : files )
    {
        wxString   filePath = dlg.GetDirectory() + wxFileName::GetPathSeparator() + file;
        wxFileName fn( filePath );
        wxString   nickname = LIB_ID::FixIllegalChars( fn.GetName() );
        bool       doAdd = true;

        if( cur_model()->ContainsNickname( nickname ) )
        {
            if( !applyToAll )
            {
                // The cancel button adds the library to the table anyway
                addDuplicates = OKOrCancelDialog( this, warning, wxString::Format( msg, nickname ),
                                                  detailedMsg, _( "Skip" ), _( "Add Anyway" ),
                                                  &applyToAll ) == wxID_CANCEL;
            }

            doAdd = addDuplicates;
        }

        if( doAdd && m_cur_grid->AppendRows( 1 ) )
        {
            int last_row = m_cur_grid->GetNumberRows() - 1;

            m_cur_grid->SetCellValue( last_row, COL_NICKNAME, nickname );

            // TODO the following code can detect only schematic types, not libs
            // SCH_IO_MGR needs to provide file extension information for libraries too

            // auto detect the plugin type
            for( SCH_IO_MGR::SCH_FILE_T piType : SCH_IO_MGR::SCH_FILE_T_vector )
            {
                if( SCH_IO_MGR::GetLibraryFileExtension( piType ).Lower() == fn.GetExt().Lower() )
                {
                    m_cur_grid->SetCellValue( last_row, COL_TYPE, SCH_IO_MGR::ShowType( piType ) );
                    break;
                }
            }

            // try to use path normalized to an environmental variable or project path
            wxString path = NormalizePath( filePath, &envVars, m_project->GetProjectPath() );

            // Do not use the project path in the global library table.  This will almost
            // assuredly be wrong for a different project.
            if( path.IsEmpty() || (m_pageNdx == 0 && path.Contains( "${KIPRJMOD}" )) )
                path = fn.GetFullPath();

            m_cur_grid->SetCellValue( last_row, COL_URI, path );
        }
    }

    if( !files.IsEmpty() )
    {
        m_cur_grid->MakeCellVisible( m_cur_grid->GetNumberRows() - 1, 0 );
        m_cur_grid->SetGridCursor( m_cur_grid->GetNumberRows() - 1, 1 );
    }
}


void PANEL_SYM_LIB_TABLE::appendRowHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    if( m_cur_grid->AppendRows( 1 ) )
    {
        int row = m_cur_grid->GetNumberRows() - 1;

        // wx documentation is wrong, SetGridCursor does not make visible.
        m_cur_grid->MakeCellVisible( row, 0 );
        m_cur_grid->SetGridCursor( row, 1 );
        m_cur_grid->EnableCellEditControl( true );
        m_cur_grid->ShowCellEditControl();
    }
}


void PANEL_SYM_LIB_TABLE::deleteRowHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    int curRow = m_cur_grid->GetGridCursorRow();
    int curCol = m_cur_grid->GetGridCursorCol();

    // In a wxGrid, collect rows that have a selected cell, or are selected
    // It is not so easy: it depends on the way the selection was made.
    // Here, we collect rows selected by clicking on a row label, and rows that contain
    // previously-selected cells.
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

    if( m_cur_grid->GetNumberRows() > 0 && curRow >= 0 )
        m_cur_grid->SetGridCursor( std::min( curRow, m_cur_grid->GetNumberRows() - 1 ), curCol );
}


void PANEL_SYM_LIB_TABLE::moveUpHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    SYMBOL_LIB_TABLE_GRID* tbl = cur_model();
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


void PANEL_SYM_LIB_TABLE::moveDownHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    SYMBOL_LIB_TABLE_GRID* tbl = cur_model();
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


void PANEL_SYM_LIB_TABLE::onConvertLegacyLibraries( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = m_cur_grid->GetSelectedRows();

    if( selectedRows.empty() && m_cur_grid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_cur_grid->GetGridCursorRow() );

    wxArrayInt legacyRows;
    wxString   legacyType = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY );
    wxString   kicadType = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_KICAD );
    wxString   msg;

    for( int row : selectedRows )
    {
        if( m_cur_grid->GetCellValue( row, COL_TYPE ) == legacyType )
            legacyRows.push_back( row );
    }

    if( legacyRows.size() <= 0 )
    {
        wxMessageBox( _( "Select one or more table rows containing legacy libraries to save as "
                         "current format (*.kicad_sym)." ) );
        return;
    }
    else
    {
        if( legacyRows.size() == 1 )
        {
            msg.Printf( _( "Save '%s' as current format (*.kicad_sym) and "
                           "replace legacy entry in table?" ),
                        m_cur_grid->GetCellValue( legacyRows[0], COL_NICKNAME ) );
        }
        else
        {
            msg.Printf( _( "Save %d legacy libraries as current format (*.kicad_sym) and "
                           "replace legacy entries in table?" ),
                        legacyRows.size() );
        }

        if( !IsOK( m_parent, msg ) )
            return;
    }

    for( int row : legacyRows )
    {
        wxString   libName = m_cur_grid->GetCellValue( row, COL_NICKNAME );
        wxString   relPath = m_cur_grid->GetCellValue( row, COL_URI );
        wxString   resolvedPath = ExpandEnvVarSubstitutions( relPath, m_project );
        wxFileName legacyLib( resolvedPath );

        if( !legacyLib.Exists() )
        {
            msg.Printf( _( "Library '%s' not found." ), relPath );
            DisplayErrorMessage( this, msg );
            continue;
        }

        wxFileName newLib( resolvedPath );
        newLib.SetExt( "kicad_sym" );

        if( convertLibrary( libName, legacyLib.GetFullPath(), newLib.GetFullPath() ) )
        {
            relPath = NormalizePath( newLib.GetFullPath(), &Pgm().GetLocalEnvVariables(),
                                     m_project );

            // Do not use the project path in the global library table.  This will almost
            // assuredly be wrong for a different project.
            if( relPath.IsEmpty() || (m_cur_grid == m_global_grid && relPath.Contains( "${KIPRJMOD}" ) ) )
                relPath = newLib.GetFullPath();

            m_cur_grid->SetCellValue( row, COL_URI, relPath );
            m_cur_grid->SetCellValue( row, COL_TYPE, kicadType );
        }
        else
        {
            msg.Printf( _( "Failed to save symbol library file '%s'." ), newLib.GetFullPath() );
            DisplayErrorMessage( this, msg );
        }
    }
}


bool PANEL_SYM_LIB_TABLE::convertLibrary( const wxString& aLibrary, const wxString& legacyFilepath,
                                          const wxString& newFilepath )
{
    SCH_PLUGIN::SCH_PLUGIN_RELEASER legacyPI( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );
    SCH_PLUGIN::SCH_PLUGIN_RELEASER kicadPI( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
    std::vector<LIB_PART*>          parts;
    std::vector<LIB_PART*>          newParts;
    std::map<LIB_PART*, LIB_PART*>  partMap;

    try
    {
        // Write a stub file; SaveSymbol() expects something to be there already.
        FILE_OUTPUTFORMATTER* formatter = new FILE_OUTPUTFORMATTER( newFilepath );

        formatter->Print( 0, "(kicad_symbol_lib (version %d) (generator kicad_converter))",
                          SEXPR_SYMBOL_LIB_FILE_VERSION );

        // This will write the file
        delete formatter;

        legacyPI->EnumerateSymbolLib( parts, legacyFilepath );

        // Copy non-aliases first so we can build a map from parts to newParts
        for( LIB_PART* part : parts )
        {
            if( part->IsAlias() )
                continue;

            newParts.push_back( new LIB_PART( *part ) );
            partMap[part] = newParts.back();
        }

        // Now do the aliases using the map to hook them up to their newPart parents
        for( LIB_PART* part : parts )
        {
            if( !part->IsAlias() )
                continue;

            newParts.push_back( new LIB_PART( *part ) );
            newParts.back()->SetParent( partMap[ part->GetParent().lock().get() ] );
        }

        // Finally write out newParts
        for( LIB_PART* part : newParts )
        {
            kicadPI->SaveSymbol( newFilepath, part );
        }
    }
    catch( ... )
    {
        return false;
    }

    return true;
}


bool PANEL_SYM_LIB_TABLE::TransferDataFromWindow()
{
    if( !m_cur_grid->CommitPendingChanges() )
        return false;

    if( !verifyTables() )
        return false;

    if( *global_model() != *m_globalTable )
    {
        m_parent->m_GlobalTableChanged = true;

        m_globalTable->Clear();
        m_globalTable->rows.transfer( m_globalTable->rows.end(), global_model()->rows.begin(),
                                      global_model()->rows.end(), global_model()->rows );
        m_globalTable->reindex();
    }

    if( project_model() && *project_model() != *m_projectTable )
    {
        m_parent->m_ProjectTableChanged = true;

        m_projectTable->Clear();
        m_projectTable->rows.transfer( m_projectTable->rows.end(), project_model()->rows.begin(),
                                       project_model()->rows.end(), project_model()->rows );
        m_projectTable->reindex();
    }

    return true;
}


void PANEL_SYM_LIB_TABLE::populateEnvironReadOnlyTable()
{
    wxRegEx re( ".*?(\\$\\{(.+?)\\})|(\\$\\((.+?)\\)).*?", wxRE_ADVANCED );
    wxASSERT( re.IsValid() );   // wxRE_ADVANCED is required.

    std::set< wxString > unique;

    // clear the table
    m_path_subs_grid->DeleteRows( 0, m_path_subs_grid->GetNumberRows() );

    for( SYMBOL_LIB_TABLE_GRID* tbl : { global_model(), project_model() } )
    {
        if( !tbl )
            continue;

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
    unique.insert( SYMBOL_LIB_TABLE::GlobalPathEnvVariableName() );

    for( const wxString& evName : unique )
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


void PANEL_SYM_LIB_TABLE::adjustPathSubsGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_path_subs_grid->GetSize().x - m_path_subs_grid->GetClientSize().x );

    m_path_subs_grid->AutoSizeColumn( 0 );

    int remaining_width = aWidth - m_path_subs_grid->GetColSize( 0 );

    if( remaining_width < 0 )
        m_path_subs_grid->SetColSize( 1, -1 );
    else
        m_path_subs_grid->SetColSize( 1, remaining_width );
}


void PANEL_SYM_LIB_TABLE::onSizeGrid( wxSizeEvent& event )
{
    adjustPathSubsGridColumns( event.GetSize().GetX() );

    event.Skip();
}


SYMBOL_LIB_TABLE_GRID* PANEL_SYM_LIB_TABLE::global_model() const
{
    return (SYMBOL_LIB_TABLE_GRID*) m_global_grid->GetTable();
}


SYMBOL_LIB_TABLE_GRID* PANEL_SYM_LIB_TABLE::project_model() const
{
    return m_project_grid ? (SYMBOL_LIB_TABLE_GRID*) m_project_grid->GetTable() : nullptr;
}


SYMBOL_LIB_TABLE_GRID* PANEL_SYM_LIB_TABLE::cur_model() const
{
    return (SYMBOL_LIB_TABLE_GRID*) m_cur_grid->GetTable();
}


size_t PANEL_SYM_LIB_TABLE::m_pageNdx = 0;


void InvokeSchEditSymbolLibTable( KIWAY* aKiway, wxWindow *aParent )
{
    auto* schEditor = (SCH_EDIT_FRAME*) aKiway->Player( FRAME_SCH, false );
    auto* symbolEditor = (SYMBOL_EDIT_FRAME*) aKiway->Player( FRAME_SCH_SYMBOL_EDITOR, false );
    auto* symbolViewer = (SYMBOL_VIEWER_FRAME*) aKiway->Player( FRAME_SCH_VIEWER, false );

    SYMBOL_LIB_TABLE* globalTable = &SYMBOL_LIB_TABLE::GetGlobalLibTable();
    wxString          globalTablePath = SYMBOL_LIB_TABLE::GetGlobalTableFileName();
    SYMBOL_LIB_TABLE* projectTable = nullptr;
    wxString          projectPath = aKiway->Prj().GetProjectPath();
    wxFileName        projectTableFn( projectPath, SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );
    wxString          msg;
    wxString          currentLib;

    // Don't allow editing project tables if no project is open
    if( !aKiway->Prj().IsNullProject() )
        projectTable = aKiway->Prj().SchSymbolLibTable();

    if( symbolEditor )
    {
        currentLib = symbolEditor->GetCurLib();

        // This prevents an ugly crash on OSX (https://bugs.launchpad.net/kicad/+bug/1765286)
        symbolEditor->FreezeLibraryTree();

        if( symbolEditor->HasLibModifications() )
        {
            msg = _( "Modifications have been made to one or more symbol libraries.\n"
                     "Changes must be saved or discarded before the symbol library "
                     "table can be modified." );

            switch( UnsavedChangesDialog( aParent, msg ) )
            {
            case wxID_YES:    symbolEditor->SaveAll();         break;
            case wxID_NO:     symbolEditor->RevertAll();       break;
            default:
            case wxID_CANCEL: symbolEditor->ThawLibraryTree(); return;
            }
        }
    }

    DIALOG_EDIT_LIBRARY_TABLES dlg( aParent, _( "Symbol Libraries" ) );
    dlg.SetKiway( &dlg, aKiway );

    dlg.InstallPanel( new PANEL_SYM_LIB_TABLE( &dlg, &aKiway->Prj(), globalTable, globalTablePath,
                                               projectTable, projectTableFn.GetFullPath() ) );

    if( dlg.ShowModal() == wxID_CANCEL )
    {
        if( symbolEditor )
            symbolEditor->ThawLibraryTree();

        return;
    }

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

    if( projectTable && dlg.m_ProjectTableChanged )
    {
        try
        {
            projectTable->Save( projectTableFn.GetFullPath() );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error saving project-specific library table:\n\n%s" ), ioe.What() );
            wxMessageBox( msg, _( "File Save Error" ), wxOK | wxICON_ERROR );
        }
    }

    if( schEditor )
        schEditor->SyncView();

    if( symbolEditor )
    {
        // Check if the currently selected symbol library been removed or disabled.
        if( !currentLib.empty() && projectTable && !projectTable->HasLibrary( currentLib, true ) )
        {
            symbolEditor->SetCurLib( wxEmptyString );
            symbolEditor->emptyScreen();
        }

        symbolEditor->SyncLibraries( true );
        symbolEditor->ThawLibraryTree();
        symbolEditor->RefreshLibraryTree();
    }

    if( symbolViewer )
        symbolViewer->ReCreateLibList();
}
