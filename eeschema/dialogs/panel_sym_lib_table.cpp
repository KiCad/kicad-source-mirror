/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2021 CERN
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

#include <set>
#include <wx/regex.h>

#include <build_version.h>
#include <common.h>     // For ExpandEnvVarSubstitutions
#include <dialogs/dialog_plugin_options.h>
#include <project.h>
#include <panel_sym_lib_table.h>
#include <lib_id.h>
#include <libraries/library_table.h>
#include <libraries/library_manager.h>
#include <lib_table_grid_tricks.h>
#include <widgets/wx_grid.h>
#include <confirm.h>
#include <bitmaps.h>
#include <lib_table_grid.h>
#include <wildcards_and_files_ext.h>
#include <env_paths.h>
#include <functional>
#include <eeschema_id.h>
#include <env_vars.h>
#include <sch_io/sch_io.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <sch_edit_frame.h>
#include <kiway.h>
#include <lib_table_base.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <widgets/grid_readonly_text_helpers.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <sch_file_versions.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <project_sch.h>
#include <libraries/symbol_library_manager_adapter.h>
#include <widgets/grid_button.h>


// clang-format off

/**
 * Container that describes file type info for the add a library options
 */
struct SUPPORTED_FILE_TYPE
{
    wxString m_Description;            ///< Description shown in the file picker dialog.
    wxString m_FileFilter;             ///< Filter used for file pickers if m_IsFile is true.

    /// In case of folders it stands for extensions of files stored inside.
    wxString m_FolderSearchExtension;
    bool     m_IsFile;                 ///< Whether the library is a folder or a file.
    SCH_IO_MGR::SCH_FILE_T m_Plugin;
};


/**
 * Event IDs for the menu items in the split button menu for add a library
 */
enum {
    ID_PANEL_SYM_LIB_KICAD = ID_END_EESCHEMA_ID_LIST,
    ID_PANEL_SYM_LIB_LEGACY,
};

// clang-format on

class SYMBOL_LIB_TABLE_GRID : public LIB_TABLE_GRID
{
    friend class PANEL_SYM_LIB_TABLE;
    friend class SYMBOL_GRID_TRICKS;

public:
    SYMBOL_LIB_TABLE_GRID( const LIBRARY_TABLE& aTableToEdit ) :
            LIB_TABLE_GRID( aTableToEdit )
    {
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        wxCHECK( aRow < (int) size(), /* void */ );

        LIB_TABLE_GRID::SetValue( aRow, aCol, aValue );

        // If setting a filepath, attempt to auto-detect the format
        if( aCol == COL_URI )
        {
            LIBRARY_TABLE_ROW& row = at( static_cast<size_t>( aRow ) );
            wxString uri = LIBRARY_MANAGER::ExpandURI( row.URI(), Pgm().GetSettingsManager().Prj() );

            wxFileName fn( uri );

            if( fn.GetName() == FILEEXT::SymbolLibraryTableFileName )
            {
                SetValue( aRow, COL_TYPE, _( "Table" ) );
            }
            else
            {
                SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::GuessPluginTypeFromLibPath( uri );

                if( pluginType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
                    pluginType = SCH_IO_MGR::SCH_KICAD;

                SetValue( aRow, COL_TYPE, SCH_IO_MGR::ShowType( pluginType ) );
            }
        }
    }
};


class SYMBOL_GRID_TRICKS : public LIB_TABLE_GRID_TRICKS
{
public:
    SYMBOL_GRID_TRICKS( DIALOG_EDIT_LIBRARY_TABLES* aParent, WX_GRID* aGrid, PROJECT& aProject ) :
        LIB_TABLE_GRID_TRICKS( aGrid ),
        m_dialog( aParent ),
        m_project( aProject )
    {
        SetTooltipEnable( COL_STATUS );
    }

    SYMBOL_GRID_TRICKS( DIALOG_EDIT_LIBRARY_TABLES* aParent, WX_GRID* aGrid, PROJECT& aProject,
                        std::function<void( wxCommandEvent& )> aAddHandler ) :
        LIB_TABLE_GRID_TRICKS( aGrid, aAddHandler ),
        m_dialog( aParent ),
        m_project( aProject )
    {
    }

protected:
    DIALOG_EDIT_LIBRARY_TABLES* m_dialog;

    void onGridCellLeftClick( wxGridEvent& aEvent ) override
    {
        if( aEvent.GetCol() == COL_STATUS )
        {
            // Status column button action depends on row:
            // Normal rows should have no button, so they are a no-op
            // Errored rows should have the warning button, so we show their error
            // Configurable libraries will have the options button, so we launch the config
            // Chained tables will have the open button, so we request the table be opened
            SYMBOL_LIBRARY_MANAGER_ADAPTER* adapter = PROJECT_SCH::SymbolLibManager( &m_project );

            auto libTable = static_cast<SYMBOL_LIB_TABLE_GRID*>( m_grid->GetTable() );
            const LIBRARY_TABLE_ROW& row = libTable->at( aEvent.GetRow() );

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
                adapter->ShowConfigurationDialog( row.Nickname(), m_dialog );
            }

            aEvent.Skip();
        }
        else
        {
            GRID_TRICKS::onGridCellLeftClick( aEvent );
        }
    }

    void optionsEditor( int aRow ) override
    {
        auto tbl = static_cast<SYMBOL_LIB_TABLE_GRID*>( m_grid->GetTable() );

        if( tbl->GetNumberRows() > aRow )
        {
            LIBRARY_TABLE_ROW& row = tbl->at( static_cast<size_t>( aRow ) );
            const wxString&    options = row.Options();
            wxString           result = options;
            std::map<std::string, UTF8> choices;

            SCH_IO_MGR::SCH_FILE_T pi_type = SCH_IO_MGR::EnumFromStr( row.Type() );
            IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( pi_type ) );
            pi->GetLibraryOptions( &choices );

            DIALOG_PLUGIN_OPTIONS dlg( m_dialog, row.Nickname(), choices, options, &result );
            dlg.ShowModal();

            if( options != result )
            {
                row.SetOptions( result );
                m_grid->Refresh();
            }
        }

    }

    /// handle specialized clipboard text, with leading "(sym_lib_table" or
    /// spreadsheet formatted text.
    void paste_text( const wxString& cb_text ) override
    {
        SYMBOL_LIB_TABLE_GRID* tbl = static_cast<SYMBOL_LIB_TABLE_GRID*>( m_grid->GetTable() );

        if( size_t ndx = cb_text.find( "(sym_lib_table" ); ndx != std::string::npos )
        {
            // paste the SYMBOL_LIB_TABLE_ROWs of s-expression (sym_lib_table), starting
            // at column 0 regardless of current cursor column.

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
                DisplayError( m_dialog, tempTable.ErrorDescription() );
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

    bool supportsVisibilityColumn() override
    {
        return true;
    }

private:
    PROJECT& m_project;
};


void PANEL_SYM_LIB_TABLE::setupGrid( WX_GRID* aGrid )
{
    auto autoSizeCol =
            [&]( WX_GRID* aCurrGrid, int aCol )
            {
                int prevWidth = aCurrGrid->GetColSize( aCol );

                aCurrGrid->AutoSizeColumn( aCol, false );
                aCurrGrid->SetColSize( aCol, std::max( prevWidth, aCurrGrid->GetColSize( aCol ) ) );
            };

    SYMBOL_LIBRARY_MANAGER_ADAPTER* adapter = PROJECT_SCH::SymbolLibManager( m_project );

    for( int ii = 0; ii < aGrid->GetNumberRows(); ++ii )
    {
        // Give a bit more room for combobox editors
        aGrid->SetRowSize( ii, aGrid->GetDefaultRowSize() + 4 );

        auto libTable = static_cast<SYMBOL_LIB_TABLE_GRID*>( aGrid->GetTable() );

        if( LIBRARY_TABLE_ROW& tableRow = libTable->at( ii ); tableRow.IsOk() )
        {
            if( std::optional<LIBRARY_ERROR> error = adapter->LibraryError( tableRow.Nickname() ) )
            {
                aGrid->SetCellValue( ii, COL_STATUS, error->message );
                aGrid->SetCellRenderer( ii, COL_STATUS,
                                        new GRID_BITMAP_BUTTON_RENDERER( KiBitmapBundle( BITMAPS::small_warning ) ) );
            }
            else if( adapter->SupportsConfigurationDialog( tableRow.Nickname() ) )
            {
                aGrid->SetCellValue( ii, COL_STATUS,
                                     wxString::Format( _( "Library settings for %s..." ),
                                                       tableRow.Nickname() ) );
                aGrid->SetCellRenderer( ii, COL_STATUS,
                                        new GRID_BITMAP_BUTTON_RENDERER( KiBitmapBundle( BITMAPS::config ) ) );
            }
        }
        else
        {
            aGrid->SetCellValue( ii, COL_STATUS, tableRow.ErrorDescription() );
            aGrid->SetCellRenderer( ii, COL_STATUS,
                                    new GRID_BITMAP_BUTTON_RENDERER( KiBitmapBundle( BITMAPS::small_warning ) ) );
        }
    }

    // add Cut, Copy, and Paste to wxGrids
    aGrid->PushEventHandler( new SYMBOL_GRID_TRICKS( m_parent, aGrid, m_parent->Kiway().Prj(),
            [this]( wxCommandEvent& event ) { appendRowHandler( event ); } ) );

    aGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // Set special attributes
    wxGridCellAttr* attr = new wxGridCellAttr;

    wxString fileFiltersStr;
    wxString allWildcardsStr;

    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        attr->SetEditor( new GRID_CELL_PATH_EDITOR(
                m_parent, aGrid, &cfg->m_lastSymbolLibDir, true, m_project->GetProjectPath(),
                []( WX_GRID* grid, int row ) -> wxString
                {
                    auto libTable = static_cast<SYMBOL_LIB_TABLE_GRID*>( grid->GetTable() );
                    LIBRARY_TABLE_ROW& tableRow = libTable->at( row );
                    SCH_IO_MGR::SCH_FILE_T pi_type = SCH_IO_MGR::EnumFromStr( tableRow.Type() );

                    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( pi_type ) );

                    if( pi )
                    {
                        const IO_BASE::IO_FILE_DESC& desc = pi->GetLibraryDesc();

                        if( desc.m_IsFile )
                            return desc.FileFilter();
                    }
                    else if( tableRow.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
                    {
                        // TODO(JE) library tables - wxWidgets doesn't allow filtering on no-extension filenames
                        return wxString::Format( _( "Symbol Library Tables (%s)|*" ),
                                                 FILEEXT::SymbolLibraryTableFileName,
                                                 FILEEXT::SymbolLibraryTableFileName );
                    }

                    return wxEmptyString;
                } ) );
    }

    aGrid->SetColAttr( COL_URI, attr );

    attr = new wxGridCellAttr;
    attr->SetEditor( new wxGridCellChoiceEditor( m_pluginChoices ) );
    aGrid->SetColAttr( COL_TYPE, attr );

    attr = new wxGridCellAttr;
    attr->SetReadOnly();
    aGrid->SetColAttr( COL_STATUS, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
    aGrid->SetColAttr( COL_ENABLED, attr );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTRE, wxALIGN_CENTRE );
    aGrid->SetColAttr( COL_VISIBLE, attr );

    aGrid->DisableColResize( COL_STATUS );
    aGrid->DisableColResize( COL_VISIBLE );
    aGrid->DisableColResize( COL_ENABLED );

    aGrid->AutoSizeColumn( COL_STATUS, true );
    aGrid->AutoSizeColumn( COL_VISIBLE, true );
    aGrid->AutoSizeColumn( COL_ENABLED, true );

    // all but COL_OPTIONS, which is edited with Option Editor anyways.
    autoSizeCol( aGrid, COL_NICKNAME );
    autoSizeCol( aGrid, COL_TYPE );
    autoSizeCol( aGrid, COL_URI );
    autoSizeCol( aGrid, COL_DESCR );

    // Gives a selection to each grid, mainly for delete button.  wxGrid's wake up with
    // a currentCell which is sometimes not highlighted.
    if( aGrid->GetNumberRows() > 0 )
        aGrid->SelectRow( 0 );
};


PANEL_SYM_LIB_TABLE::PANEL_SYM_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* aProject ) :
        PANEL_SYM_LIB_TABLE_BASE( aParent ),
        m_project( aProject ),
        m_parent( aParent )
{
    std::optional<LIBRARY_TABLE*> table =
        Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::GLOBAL );
    wxASSERT( table );
    // wxGrid only supports user owned tables if they exist past end of ~wxGrid(),
    // so make it a grid owned table.
    m_global_grid->SetTable( new SYMBOL_LIB_TABLE_GRID( *table.value() ) );

    // TODO(JE) should use translated string here but type is stored as untranslated string
    // Maybe type storage needs to be enum?
    m_pluginChoices.Add( wxT( "Table" ) );

    for( const SCH_IO_MGR::SCH_FILE_T& type : SCH_IO_MGR::SCH_FILE_T_vector )
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( type ) );

        if( !pi )
            continue;

        if( const IO_BASE::IO_FILE_DESC& desc = pi->GetLibraryDesc() )
            m_pluginChoices.Add( SCH_IO_MGR::ShowType( type ) );
    }

    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        if( cfg->m_lastSymbolLibDir.IsEmpty() )
            cfg->m_lastSymbolLibDir = PATHS::GetDefaultUserSymbolsPath();
    }

    m_lastProjectLibDir = m_project->GetProjectPath();

    setupGrid( m_global_grid );

    std::optional<LIBRARY_TABLE*> projectTable =
            Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::PROJECT );

    if( projectTable )
    {
        m_project_grid->SetTable( new SYMBOL_LIB_TABLE_GRID( *projectTable.value() ), true );
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
    m_append_button->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_delete_button->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_move_up_button->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_move_down_button->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_browse_button->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );
}


PANEL_SYM_LIB_TABLE::~PANEL_SYM_LIB_TABLE()
{
    // Delete the GRID_TRICKS.
    // Any additional event handlers should be popped before the window is deleted.
    m_global_grid->PopEventHandler( true );

    if( m_project_grid )
        m_project_grid->PopEventHandler( true );

    m_path_subs_grid->PopEventHandler( true );
}


bool PANEL_SYM_LIB_TABLE::allowAutomaticPluginTypeSelection( wxString& aLibraryPath )
{
    // When the plugin type depends only of the file extension, return true.
    // if it needs to read the actual file (that can be not available), return false

    wxFileName fn( aLibraryPath );
    wxString   ext = fn.GetExt().Lower();

    // Currently, only the extension .lib is common to legacy libraries and Cadstar libraries
    // so return false in this case
    if( ext == FILEEXT::LegacySymbolLibFileExtension )
        return false;

    return true;
}


bool PANEL_SYM_LIB_TABLE::verifyTables()
{
    wxString                      msg;
    int                           cursorCol;
    std::unique_ptr<wxBusyCursor> wait;
    wait.reset( new wxBusyCursor );

    for( SYMBOL_LIB_TABLE_GRID* model : { global_model(), project_model() } )
    {
        if( !model )
            continue;

        for( int r = 0; r < model->GetNumberRows(); ++r )
        {
            wxString nick = model->GetValue( r, COL_NICKNAME ).Trim( false ).Trim();
            wxString uri  = model->GetValue( r, COL_URI ).Trim( false ).Trim();
            unsigned illegalCh = 0;

            if( !nick || !uri )
            {
                if( !nick && !uri )
                {
                    msg = _( "Nickname and path cannot be empty." );
                    cursorCol = COL_NICKNAME;
                }
                else if( !nick )
                {
                    msg = _( "Nickname cannot be empty." );
                    cursorCol = COL_NICKNAME;
                }
                else
                {
                    msg = _( "Path cannot be empty." );
                    cursorCol = COL_URI;
                }

                // show the tabbed panel holding the grid we have flunked:
                if( model != cur_model() )
                    m_notebook->SetSelection( model == global_model() ? 0 : 1 );

                m_cur_grid->MakeCellVisible( r, 0 );
                m_cur_grid->SetGridCursor( r, cursorCol );

                wxWindow* topLevelParent = wxGetTopLevelParent( this );

                wxMessageDialog errdlg( topLevelParent, msg, _( "Library Table Error" ) );

                wait.reset();
                errdlg.ShowModal();
                return false;
            }
            else if( ( illegalCh = LIB_ID::FindIllegalLibraryNameChar( nick ) ) )
            {
                msg = wxString::Format( _( "Illegal character '%c' in nickname '%s'" ),
                                        illegalCh,
                                        nick );

                // show the tabbed panel holding the grid we have flunked:
                if( model != cur_model() )
                    m_notebook->SetSelection( model == global_model() ? 0 : 1 );

                m_cur_grid->MakeCellVisible( r, 0 );
                m_cur_grid->SetGridCursor( r, COL_NICKNAME );

                wxWindow* topLevelParent = wxGetTopLevelParent( this );

                wxMessageDialog errdlg( topLevelParent, msg, _( "Library Nickname Error" ) );

                wait.reset();
                errdlg.ShowModal();
                return false;
            }
            else
            {
                // set the trimmed values back into the table so they get saved to disk.
                model->SetValue( r, COL_NICKNAME, nick );

                if( allowAutomaticPluginTypeSelection( uri ) )
                {
                    model->SetValue( r, COL_URI, uri );
                }
                else
                {
                    wxString ltype = model->GetValue( r, COL_TYPE );
                    model->LIB_TABLE_GRID::SetValue( r, COL_URI, uri );
                    model->SetValue( r, COL_TYPE, ltype );
                }
            }
        }
    }

    // check for duplicate nickNames, separately in each table.
    for( SYMBOL_LIB_TABLE_GRID* model : { global_model(), project_model() } )
    {
        if( !model )
            continue;

        for( int r1 = 0; r1 < model->GetNumberRows() - 1; ++r1 )
        {
            wxString nick1 = model->GetValue( r1, COL_NICKNAME );

            for( int r2 = r1 + 1; r2 < model->GetNumberRows(); ++r2 )
            {
                wxString nick2 = model->GetValue( r2, COL_NICKNAME );

                if( nick1 == nick2 )
                {
                    msg = wxString::Format( _( "Multiple libraries cannot share the same nickname ('%s')." ),
                                            nick1 );

                    // show the tabbed panel holding the grid we have flunked:
                    if( model != cur_model() )
                        m_notebook->SetSelection( model == global_model() ? 0 : 1 );

                    // go to the lower of the two rows, it is technically the duplicate:
                    m_cur_grid->MakeCellVisible( r2, 0 );
                    m_cur_grid->SetGridCursor( r2, COL_NICKNAME );

                    wxWindow* topLevelParent = wxGetTopLevelParent( this );

                    wait.reset();
                    wxMessageDialog errdlg( topLevelParent, msg, _( "Library Nickname Error" ) );
                    errdlg.ShowModal();

                    return false;
                }
            }
        }
    }

    return true;
}


void PANEL_SYM_LIB_TABLE::OnUpdateUI( wxUpdateUIEvent& event )
{
}


void PANEL_SYM_LIB_TABLE::browseLibrariesHandler( wxCommandEvent& event )
{
    wxString fileFiltersStr;
    wxString allWildcardsStr;

    for( const SCH_IO_MGR::SCH_FILE_T& fileType : SCH_IO_MGR::SCH_FILE_T_vector )
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( fileType ) );

        if( !pi )
            continue;

        const IO_BASE::IO_FILE_DESC& desc = pi->GetLibraryDesc();

        if( desc.m_FileExtensions.empty() )
            continue;

        if( !fileFiltersStr.IsEmpty() )
            fileFiltersStr += wxChar( '|' );

        fileFiltersStr += desc.FileFilter();

        for( const std::string& ext : desc.m_FileExtensions )
            allWildcardsStr << wxT( "*." ) << formatWildcardExt( ext ) << wxT( ";" );
    }

    fileFiltersStr = _( "All supported formats" ) + wxT( "|" ) + allWildcardsStr + wxT( "|" )
                     + fileFiltersStr;

    EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
    wxString           dummy;
    wxString*          lastDir;

    if( m_cur_grid == m_project_grid )
        lastDir = &m_lastProjectLibDir;
    else
        lastDir = cfg ? &cfg->m_lastSymbolLibDir : &dummy;

    wxWindow* topLevelParent = wxGetTopLevelParent( this );

    wxFileDialog dlg( topLevelParent, _( "Add Library" ), *lastDir, wxEmptyString, fileFiltersStr,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    *lastDir = dlg.GetDirectory();

    const ENV_VAR_MAP& envVars       = Pgm().GetLocalEnvVariables();
    bool               addDuplicates = false;
    bool               applyToAll    = false;

    wxArrayString filePathsList;
    dlg.GetPaths( filePathsList );

    for( const wxString& filePath : filePathsList )
    {
        wxFileName fn( filePath );
        wxString   nickname = LIB_ID::FixIllegalChars( fn.GetName(), true );
        bool       doAdd = true;

        if( cur_model()->ContainsNickname( nickname ) )
        {
            if( !applyToAll )
            {
                // The cancel button adds the library to the table anyway
                addDuplicates = OKOrCancelDialog( wxGetTopLevelParent( this ), _( "Warning: Duplicate Nickname" ),
                                                  wxString::Format( _( "A library nicknamed '%s' already exists." ),
                                                                    nickname ),
                                                  _( "One of the nicknames will need to be changed after adding "
                                                     "this library." ),
                                                  _( "Skip" ), _( "Add Anyway" ),
                                                  &applyToAll ) == wxID_CANCEL;
            }

            doAdd = addDuplicates;
        }

        if( doAdd && m_cur_grid->AppendRows( 1 ) )
        {
            int last_row = m_cur_grid->GetNumberRows() - 1;

            m_cur_grid->SetCellValue( last_row, COL_NICKNAME, nickname );

            // attempt to auto-detect the plugin type
            SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::GuessPluginTypeFromLibPath( filePath );

            if( pluginType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
                pluginType = SCH_IO_MGR::SCH_KICAD;

            m_cur_grid->SetCellValue( last_row, COL_TYPE, SCH_IO_MGR::ShowType( pluginType ) );

            // try to use path normalized to an environmental variable or project path
            wxString path = NormalizePath( filePath, &envVars, m_project->GetProjectPath() );

            // Do not use the project path in the global library table.  This will almost
            // assuredly be wrong for a different project.
            if( m_pageNdx == 0 && path.Contains( "${KIPRJMOD}" ) )
                path = fn.GetFullPath();

            m_cur_grid->SetCellValue( last_row, COL_URI, path );
        }
    }

    if( !filePathsList.IsEmpty() )
    {
        m_cur_grid->MakeCellVisible( m_cur_grid->GetNumberRows() - 1, COL_ENABLED );
        m_cur_grid->SetGridCursor( m_cur_grid->GetNumberRows() - 1, COL_NICKNAME );
    }
}


void PANEL_SYM_LIB_TABLE::appendRowHandler( wxCommandEvent& event )
{
    m_cur_grid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_cur_grid->AppendRows( 1 );
                return { m_cur_grid->GetNumberRows() - 1, COL_NICKNAME };
            } );
}


void PANEL_SYM_LIB_TABLE::deleteRowHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    wxGridUpdateLocker noUpdates( m_cur_grid );

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

    // Needed to avoid a wxWidgets alert if the row to delete is the last row
    // at least on wxMSW 3.2
    m_cur_grid->ClearSelection();

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
    m_cur_grid->OnMoveRowUp(
            [&]( int row )
            {
                SYMBOL_LIB_TABLE_GRID* tbl = cur_model();
                int curRow = m_cur_grid->GetGridCursorRow();

                std::vector<LIBRARY_TABLE_ROW>& rows = tbl->Table().Rows();

                auto current = rows.begin() + curRow;
                auto prev    = rows.begin() + curRow - 1;

                std::iter_swap( current, prev );

                // Update the wxGrid
                wxGridTableMessage msg( cur_model(), wxGRIDTABLE_NOTIFY_ROWS_INSERTED, row - 1, 0 );
                cur_model()->GetView()->ProcessTableMessage( msg );
            } );
}


void PANEL_SYM_LIB_TABLE::moveDownHandler( wxCommandEvent& event )
{
    m_cur_grid->OnMoveRowDown(
            [&]( int row )
            {
                SYMBOL_LIB_TABLE_GRID* tbl = cur_model();
                int curRow = m_cur_grid->GetGridCursorRow();
                std::vector<LIBRARY_TABLE_ROW>& rows = tbl->Table().Rows();

                auto current = rows.begin() + curRow;
                auto next    = rows.begin() + curRow + 1;

                std::iter_swap( current, next );

                // Update the wxGrid
                wxGridTableMessage msg( cur_model(), wxGRIDTABLE_NOTIFY_ROWS_INSERTED, row, 0 );
                cur_model()->GetView()->ProcessTableMessage( msg );
            } );
}


void PANEL_SYM_LIB_TABLE::onReset( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    // No need to prompt to preserve an empty table
    if( m_global_grid->GetNumberRows() > 0 &&
        !IsOK( this, wxString::Format( _( "This action will reset your global library table on "
                                          "disk and cannot be undone." ) ) ) )
    {
        return;
    }

    // TODO(JE) library tables
#if 0
    DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG dlg( m_parent );

    if( dlg.ShowModal() == wxID_OK )
    {
        m_global_grid->Freeze();

        wxGridTableBase* table = m_global_grid->GetTable();
        m_global_grid->DestroyTable( table );

        std::optional<LIBRARY_TABLE*> newTable =
                Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL,
                                                 LIBRARY_TABLE_SCOPE::GLOBAL );
        wxASSERT( newTable );

        m_global_grid->SetTable( new SYMBOL_LIB_TABLE_GRID( *newTable.value() ) );
        m_global_grid->PopEventHandler( true );
        setupGrid( m_global_grid );
        m_parent->m_GlobalTableChanged = true;

        m_global_grid->Thaw();
    }
#endif
}


void PANEL_SYM_LIB_TABLE::onPageChange( wxBookCtrlEvent& event )
{
    m_pageNdx = (unsigned) std::max( 0, m_notebook->GetSelection() );

    if( m_pageNdx == 0 )
    {
        m_cur_grid = m_global_grid;
        m_resetGlobal->Enable();
    }
    else
    {
        m_cur_grid = m_project_grid;
        m_resetGlobal->Disable();
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
    wxString   databaseType = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_DATABASE );
    wxString   kicadType = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_KICAD );
    wxString   msg;

    for( int row : selectedRows )
    {
        if( m_cur_grid->GetCellValue( row, COL_TYPE ) != databaseType &&
            m_cur_grid->GetCellValue( row, COL_TYPE ) != kicadType )
        {
            legacyRows.push_back( row );
        }
    }

    if( legacyRows.size() <= 0 )
    {
        wxMessageBox( _( "Select one or more rows containing libraries "
                         "to save as current KiCad format (*.kicad_sym)." ) );
        return;
    }
    else
    {
        if( legacyRows.size() == 1 )
        {
            msg.Printf( _( "Save '%s' as current KiCad format (*.kicad_sym) "
                           "and replace legacy entry in table?" ),
                        m_cur_grid->GetCellValue( legacyRows[0], COL_NICKNAME ) );
        }
        else
        {
            msg.Printf( _( "Save %d libraries as current KiCad format (*.kicad_sym) "
                           "and replace legacy entries in table?" ),
                        (int) legacyRows.size() );
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

            wxWindow* topLevelParent = wxGetTopLevelParent( this );

            DisplayErrorMessage( topLevelParent, msg );
            continue;
        }

        wxFileName newLib( resolvedPath );
        newLib.SetExt( "kicad_sym" );

        if( newLib.Exists() )
        {
            msg.Printf( _( "File '%s' already exists. Do you want overwrite this file?" ),
                        newLib.GetFullPath() );

            switch( wxMessageBox( msg, _( "Migrate Library" ),
                                  wxYES_NO | wxCANCEL | wxICON_QUESTION, m_parent ) )
            {
            case wxYES:    break;
            case wxNO:     continue;
            case wxCANCEL: return;
            }
        }

        wxString options = m_cur_grid->GetCellValue( row, COL_OPTIONS );
        std::map<std::string, UTF8> props( LIB_TABLE::ParseOptions( options.ToStdString() ) );

        if( SCH_IO_MGR::ConvertLibrary( &props, legacyLib.GetFullPath(), newLib.GetFullPath() ) )
        {
            relPath = NormalizePath( newLib.GetFullPath(), &Pgm().GetLocalEnvVariables(), m_project );

            // Do not use the project path in the global library table.  This will almost
            // assuredly be wrong for a different project.
            if( m_cur_grid == m_global_grid && relPath.Contains( "${KIPRJMOD}" ) )
                relPath = newLib.GetFullPath();

            m_cur_grid->SetCellValue( row, COL_URI, relPath );
            m_cur_grid->SetCellValue( row, COL_TYPE, kicadType );
            m_cur_grid->SetCellValue( row, COL_OPTIONS, wxEmptyString );
        }
        else
        {
            msg.Printf( _( "Failed to save symbol library file '%s'." ), newLib.GetFullPath() );

            wxWindow* topLevelParent = wxGetTopLevelParent( this );

            DisplayErrorMessage( topLevelParent, msg );
        }
    }
}


bool PANEL_SYM_LIB_TABLE::TransferDataFromWindow()
{
    if( !m_cur_grid->CommitPendingChanges() )
        return false;

    if( !verifyTables() )
        return false;

    std::optional<LIBRARY_TABLE*> optTable =
        Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::GLOBAL );
    wxCHECK( optTable, false );
    LIBRARY_TABLE* globalTable = *optTable;

    if( global_model()->Table() != *globalTable )
    {
        m_parent->m_GlobalTableChanged = true;
        *globalTable = global_model()->Table();
    }

    optTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL,
                                                LIBRARY_TABLE_SCOPE::PROJECT );

    if( optTable && project_model() )
    {
        LIBRARY_TABLE* projectTable = *optTable;

        if( project_model()->Table() != *projectTable )
        {
            m_parent->m_ProjectTableChanged = true;
            *projectTable = project_model()->Table();
        }
    }

    return true;
}


void PANEL_SYM_LIB_TABLE::populateEnvironReadOnlyTable()
{
    wxRegEx re( ".*?(\\$\\{(.+?)\\})|(\\$\\((.+?)\\)).*?", wxRE_ADVANCED );
    wxASSERT( re.IsValid() );   // wxRE_ADVANCED is required.

    std::set< wxString > unique;

    // clear the table
    m_path_subs_grid->ClearRows();

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
    unique.insert( ENV_VAR::GetVersionedEnvVarName( wxS( "SYMBOL_DIR" ) ) );

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

    adjustPathSubsGridColumns( m_path_subs_grid->GetRect().GetWidth() );
}


void PANEL_SYM_LIB_TABLE::adjustPathSubsGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_path_subs_grid->GetSize().x - m_path_subs_grid->GetClientSize().x );

    m_path_subs_grid->AutoSizeColumn( 0 );
    m_path_subs_grid->SetColSize( 0, std::max( 72, m_path_subs_grid->GetColSize( 0 ) ) );
    m_path_subs_grid->SetColSize( 1, std::max( 120, aWidth - m_path_subs_grid->GetColSize( 0 ) ) );
}


void PANEL_SYM_LIB_TABLE::onSizeGrid( wxSizeEvent& event )
{
    adjustPathSubsGridColumns( event.GetSize().GetX() );

    event.Skip();
}


SYMBOL_LIB_TABLE_GRID* PANEL_SYM_LIB_TABLE::global_model() const
{
    return static_cast<SYMBOL_LIB_TABLE_GRID*>( m_global_grid->GetTable() );
}


SYMBOL_LIB_TABLE_GRID* PANEL_SYM_LIB_TABLE::project_model() const
{
    return m_project_grid ? static_cast<SYMBOL_LIB_TABLE_GRID*>( m_project_grid->GetTable() ) : nullptr;
}


SYMBOL_LIB_TABLE_GRID* PANEL_SYM_LIB_TABLE::cur_model() const
{
    return static_cast<SYMBOL_LIB_TABLE_GRID*>( m_cur_grid->GetTable() );
}


size_t PANEL_SYM_LIB_TABLE::m_pageNdx = 0;


void InvokeSchEditSymbolLibTable( KIWAY* aKiway, wxWindow *aParent )
{
    auto symbolEditor = static_cast<SYMBOL_EDIT_FRAME*>( aKiway->Player( FRAME_SCH_SYMBOL_EDITOR,
                                                                         false ) );
    wxString msg;

    if( symbolEditor )
    {
        // This prevents an ugly crash on OSX (https://bugs.launchpad.net/kicad/+bug/1765286)
        symbolEditor->FreezeLibraryTree();

        if( symbolEditor->HasLibModifications() )
        {
            msg = _( "Modifications have been made to one or more symbol libraries.\n"
                     "Changes must be saved or discarded before the symbol library table can be modified." );

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

    dlg.InstallPanel( new PANEL_SYM_LIB_TABLE( &dlg, &aKiway->Prj() ) );

    if( dlg.ShowModal() == wxID_CANCEL )
    {
        if( symbolEditor )
            symbolEditor->ThawLibraryTree();

        return;
    }

    if( dlg.m_GlobalTableChanged )
    {
        std::optional<LIBRARY_TABLE*> optTable =
                Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::GLOBAL );
        wxCHECK( optTable, /* void */ );
        LIBRARY_TABLE* globalTable = *optTable;

        Pgm().GetLibraryManager().Save( globalTable ).map_error(
            []( const LIBRARY_ERROR& aError )
            {
                wxMessageBox( wxString::Format( _( "Error saving global library table:\n\n%s" ), aError.message ),
                              _( "File Save Error" ), wxOK | wxICON_ERROR );
            } );

        Pgm().GetLibraryManager().LoadGlobalTables();
    }

    std::optional<LIBRARY_TABLE*> projectTable =
            Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::PROJECT );

    if( projectTable && dlg.m_ProjectTableChanged )
    {
        Pgm().GetLibraryManager().Save( *projectTable ).map_error(
            []( const LIBRARY_ERROR& aError )
            {
                wxMessageBox( wxString::Format( _( "Error saving project-specific library table:\n\n%s" ),
                                                aError.message ),
                              _( "File Save Error" ), wxOK | wxICON_ERROR );
            } );

        // Trigger a reload of the table and cancel an in-progress background load
        Pgm().GetLibraryManager().ProjectChanged();
    }

    // Trigger a reload in case any libraries have been added or removed
    if( KIFACE *schface = aKiway->KiFACE( KIWAY::FACE_SCH ) )
        schface->PreloadLibraries( &aKiway->Prj() );

    if( symbolEditor )
        symbolEditor->ThawLibraryTree();

    std::string payload = "";
    aKiway->ExpressMail( FRAME_SCH, MAIL_RELOAD_LIB, payload );
    aKiway->ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_RELOAD_LIB, payload );
    aKiway->ExpressMail( FRAME_SCH_VIEWER, MAIL_RELOAD_LIB, payload );
}
