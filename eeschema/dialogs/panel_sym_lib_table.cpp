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
#include <widgets/grid_readonly_text_helpers.h>
#include <widgets/split_button.h>
#include <widgets/std_bitmap_button.h>
#include <confirm.h>
#include <bitmaps.h>
#include <lib_table_grid_data_model.h>
#include <wildcards_and_files_ext.h>
#include <env_paths.h>
#include <functional>
#include <eeschema_id.h>
#include <env_vars.h>
#include <sch_io/sch_io.h>
#include <symbol_edit_frame.h>
#include <sch_edit_frame.h>
#include <kiway.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <project_sch.h>
#include <libraries/symbol_library_adapter.h>
#include <lib_table_notebook_panel.h>
#include <widgets/wx_aui_art_providers.h>


#define FIRST_MENU_ID 998


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
 * Special menu ID for folder-based KiCad symbol library format.
 * This ID is offset from SCH_FILE_T values to distinguish folder mode from file mode.
 */
static constexpr int ID_PANEL_SYM_LIB_KICAD_FOLDER = 1000;


class SYMBOL_LIB_TABLE_GRID_DATA_MODEL : public LIB_TABLE_GRID_DATA_MODEL
{
public:
    SYMBOL_LIB_TABLE_GRID_DATA_MODEL( DIALOG_SHIM* aParent, WX_GRID* aGrid, const LIBRARY_TABLE& aTableToEdit,
                                      SYMBOL_LIBRARY_ADAPTER* aAdapter, const wxArrayString& aPluginChoices,
                                      wxString* aMRUDirectory, const wxString& aProjectPath ) :
            LIB_TABLE_GRID_DATA_MODEL( aParent, aGrid, aTableToEdit, aAdapter, aPluginChoices, aMRUDirectory,
                                       aProjectPath )
    {
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        wxCHECK( aRow < (int) size(), /* void */ );

        LIB_TABLE_GRID_DATA_MODEL::SetValue( aRow, aCol, aValue );

        // If setting a filepath, attempt to auto-detect the format
        if( aCol == COL_URI )
        {
            LIBRARY_TABLE_ROW& row = at( static_cast<size_t>( aRow ) );
            wxString uri = LIBRARY_MANAGER::ExpandURI( row.URI(), Pgm().GetSettingsManager().Prj() );
            SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::GuessPluginTypeFromLibPath( uri );

            if( pluginType != SCH_IO_MGR::SCH_FILE_UNKNOWN )
                SetValue( aRow, COL_TYPE, SCH_IO_MGR::ShowType( pluginType ) );
        }
    }

protected:
    wxString getFileTypes( WX_GRID* aGrid, int aRow ) override
    {
        LIB_TABLE_GRID_DATA_MODEL* table = static_cast<LIB_TABLE_GRID_DATA_MODEL*>( aGrid->GetTable() );
        LIBRARY_TABLE_ROW&         tableRow = table->At( aRow );

        if( tableRow.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
        {
            wxString filter = _( "Symbol Library Tables" );
#ifndef __WXOSX__
            filter << wxString::Format( _( " (%s)|%s" ), FILEEXT::SymbolLibraryTableFileName,
                                        FILEEXT::SymbolLibraryTableFileName );
#else
            filter << wxString::Format( _( " (%s)|%s" ), wxFileSelectorDefaultWildcardStr,
                                        wxFileSelectorDefaultWildcardStr );
#endif
            return filter;
        }

        SCH_IO_MGR::SCH_FILE_T pi_type = SCH_IO_MGR::EnumFromStr( tableRow.Type() );
        IO_RELEASER<SCH_IO>    pi( SCH_IO_MGR::FindPlugin( pi_type ) );

        if( pi )
        {
            const IO_BASE::IO_FILE_DESC& desc = pi->GetLibraryDesc();

            if( desc.m_IsFile )
                return desc.FileFilter();
        }

        return wxEmptyString;
    }
};


class SYMBOL_GRID_TRICKS : public LIB_TABLE_GRID_TRICKS
{
public:
    SYMBOL_GRID_TRICKS( PANEL_SYM_LIB_TABLE* aPanel, WX_GRID* aGrid,
                        std::function<void( wxCommandEvent& )> aAddHandler ) :
            LIB_TABLE_GRID_TRICKS( aGrid, aAddHandler ),
            m_panel( aPanel )
    {
        SetTooltipEnable( COL_STATUS );
    }

protected:
    void optionsEditor( int aRow ) override
    {
        LIB_TABLE_GRID_DATA_MODEL* tbl = static_cast<LIB_TABLE_GRID_DATA_MODEL*>( m_grid->GetTable() );

        if( tbl->GetNumberRows() > aRow )
        {
            LIBRARY_TABLE_ROW& row = tbl->At( static_cast<size_t>( aRow ) );
            const wxString&    options = row.Options();
            wxString           result = options;
            std::map<std::string, UTF8> choices;

            SCH_IO_MGR::SCH_FILE_T pi_type = SCH_IO_MGR::EnumFromStr( row.Type() );
            IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( pi_type ) );
            pi->GetLibraryOptions( &choices );

            DIALOG_PLUGIN_OPTIONS dlg( wxGetTopLevelParent( m_grid ), row.Nickname(), choices, options, &result );
            dlg.ShowModal();

            if( options != result )
            {
                row.SetOptions( result );
                m_grid->Refresh();
            }
        }
    }

    void openTable( const LIBRARY_TABLE_ROW& aRow ) override
    {
        wxFileName fn( LIBRARY_MANAGER::ExpandURI( aRow.URI(), Pgm().GetSettingsManager().Prj() ) );
        std::shared_ptr<LIBRARY_TABLE> child = std::make_shared<LIBRARY_TABLE>( fn, LIBRARY_TABLE_SCOPE::GLOBAL );

        m_panel->OpenTable( child, aRow.Nickname() );
    }

    wxString getTablePreamble() override
    {
        return wxT( "(sym_lib_table" );
    }

    bool supportsVisibilityColumn() override
    {
        return true;
    }

protected:
    PANEL_SYM_LIB_TABLE* m_panel;
};


void PANEL_SYM_LIB_TABLE::OpenTable( const std::shared_ptr<LIBRARY_TABLE>& aTable, const wxString& aTitle )
{
    for( int ii = 2; ii < (int) m_notebook->GetPageCount(); ++ii )
    {
        if( m_notebook->GetPageText( ii ) == aTitle )
        {
            // Something is pretty fishy with wxAuiNotebook::ChangeSelection(); on Mac at least it
            // results in a re-entrant call where the second call is one page behind.
            for( int attempts = 0; attempts < 3; ++attempts )
                m_notebook->ChangeSelection( ii );

            return;
        }
    }

    m_nestedTables.push_back( aTable );
    AddTable( aTable.get(), aTitle, true );

    // Something is pretty fishy with wxAuiNotebook::ChangeSelection(); on Mac at least it
    // results in a re-entrant call where the second call is one page behind.
    for( int attempts = 0; attempts < 3; ++attempts )
        m_notebook->ChangeSelection( m_notebook->GetPageCount() - 1 );
}


void PANEL_SYM_LIB_TABLE::AddTable( LIBRARY_TABLE* table, const wxString& aTitle, bool aClosable )
{
    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( m_project );
    wxString                projectPath = m_project->GetProjectPath();

    LIB_TABLE_NOTEBOOK_PANEL::AddTable( m_notebook, aTitle, aClosable );

    WX_GRID* grid = get_grid( (int) m_notebook->GetPageCount() - 1 );

    if( table->Path().StartsWith( projectPath ) )
    {
        grid->SetTable( new SYMBOL_LIB_TABLE_GRID_DATA_MODEL( m_parent, grid, *table, adapter, m_pluginChoices,
                                                              &m_lastProjectLibDir, projectPath ),
                        true /* take ownership */ );
    }
    else
    {
        wxString* lastGlobalLibDir = nullptr;

        if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
        {
            if( cfg->m_lastSymbolLibDir.IsEmpty() )
                cfg->m_lastSymbolLibDir = PATHS::GetDefaultUserSymbolsPath();

            lastGlobalLibDir = &cfg->m_lastSymbolLibDir;
        }

        grid->SetTable( new SYMBOL_LIB_TABLE_GRID_DATA_MODEL( m_parent, grid, *table, adapter, m_pluginChoices,
                                                              lastGlobalLibDir, wxEmptyString ),
                        true /* take ownership */ );
    }

    // add Cut, Copy, and Paste to wxGrids
    grid->PushEventHandler( new SYMBOL_GRID_TRICKS( this, grid,
            [this]( wxCommandEvent& event )
            {
                appendRowHandler( event );
            } ) );

    auto autoSizeCol =
            [&]( int aCol )
            {
                int prevWidth = grid->GetColSize( aCol );

                grid->AutoSizeColumn( aCol, false );
                grid->SetColSize( aCol, std::max( prevWidth, grid->GetColSize( aCol ) ) );
            };

    // all but COL_OPTIONS, which is edited with Option Editor anyways.
    autoSizeCol( COL_NICKNAME );
    autoSizeCol( COL_TYPE );
    autoSizeCol( COL_URI );
    autoSizeCol( COL_DESCR );

    if( grid->GetNumberRows() > 0 )
    {
        grid->SetGridCursor( 0, COL_NICKNAME );
        grid->SelectRow( 0 );
    }
}


PANEL_SYM_LIB_TABLE::PANEL_SYM_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* aProject ) :
        PANEL_SYM_LIB_TABLE_BASE( aParent ),
        m_project( aProject ),
        m_parent( aParent )
{
    m_lastProjectLibDir = m_project->GetProjectPath();

    populatePluginList();

    for( const SCH_IO_MGR::SCH_FILE_T& type : SCH_IO_MGR::SCH_FILE_T_vector )
    {
        if( type == SCH_IO_MGR::SCH_NESTED_TABLE )
        {
            m_pluginChoices.Add( SCH_IO_MGR::ShowType( type ) );
            continue;
        }

        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( type ) );

        if( pi )
            m_pluginChoices.Add( SCH_IO_MGR::ShowType( type ) );
    }

    std::optional<LIBRARY_TABLE*> table = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL,
                                                                           LIBRARY_TABLE_SCOPE::GLOBAL );
    wxASSERT( table.has_value() );

    AddTable( table.value(), _( "Global Libraries" ), false /* closable */ );

    std::optional<LIBRARY_TABLE*> projectTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL,
                                                                                  LIBRARY_TABLE_SCOPE::PROJECT );

    if( projectTable.has_value() )
        AddTable( projectTable.value(), _( "Project Specific Libraries" ), false /* closable */ );

    m_notebook->SetArtProvider( new WX_AUI_TAB_ART() );

    // add Cut, Copy, and Paste to wxGrids
    m_path_subs_grid->PushEventHandler( new GRID_TRICKS( m_path_subs_grid ) );

    populateEnvironReadOnlyTable();

    m_path_subs_grid->SetColLabelValue( 0, _( "Name" ) );
    m_path_subs_grid->SetColLabelValue( 1, _( "Value" ) );

    // Configure button logos
    m_append_button->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_delete_button->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_move_up_button->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_move_down_button->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    // For aesthetic reasons, we must set the size of m_browseButton to match the other bitmaps
    Layout();
    wxSize buttonSize = m_append_button->GetSize();

    m_browseButton->SetWidthPadding( 4 );
    m_browseButton->SetMinSize( buttonSize );

    // Populate the browse library options
    wxMenu* browseMenu = m_browseButton->GetSplitButtonMenu();

    auto joinExtensions =
            []( const std::vector<std::string>& aExts ) -> wxString
            {
                wxString result;

                for( const std::string& ext : aExts )
                {
                    if( !result.IsEmpty() )
                        result << wxT( ", " );

                    result << wxT( "." ) << ext;
                }

                return result;
            };

    for( auto& [type, desc] : m_supportedSymFiles )
    {
        wxString entryStr = SCH_IO_MGR::ShowType( type );

        if( !desc.m_FileExtensions.empty() )
            entryStr << wxString::Format( wxS( " (%s)" ), joinExtensions( desc.m_FileExtensions ) );

        browseMenu->Append( type + FIRST_MENU_ID, entryStr );
        browseMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_SYM_LIB_TABLE::browseLibrariesHandler, this,
                          type + FIRST_MENU_ID );

        // Add folder-based entry right after KiCad file-based entry
        if( type == SCH_IO_MGR::SCH_KICAD )
        {
            wxString folderEntry = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_KICAD );
            folderEntry << wxString::Format( wxS( " (%s)" ), _( "folder with .kicad_sym files" ) );
            browseMenu->Append( ID_PANEL_SYM_LIB_KICAD_FOLDER, folderEntry );
            browseMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_SYM_LIB_TABLE::browseLibrariesHandler, this,
                              ID_PANEL_SYM_LIB_KICAD_FOLDER );
        }
    }

    Layout();

    m_notebook->Bind( wxEVT_AUINOTEBOOK_PAGE_CLOSE, &PANEL_SYM_LIB_TABLE::onNotebookPageCloseRequest, this );
    m_browseButton->Bind( wxEVT_BUTTON, &PANEL_SYM_LIB_TABLE::browseLibrariesHandler, this );
}


PANEL_SYM_LIB_TABLE::~PANEL_SYM_LIB_TABLE()
{
    wxMenu* browseMenu = m_browseButton->GetSplitButtonMenu();

    for( auto& [type, desc] : m_supportedSymFiles )
        browseMenu->Unbind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_SYM_LIB_TABLE::browseLibrariesHandler, this, type );

    browseMenu->Unbind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_SYM_LIB_TABLE::browseLibrariesHandler,
                        this, ID_PANEL_SYM_LIB_KICAD_FOLDER );
    m_browseButton->Unbind( wxEVT_BUTTON, &PANEL_SYM_LIB_TABLE::browseLibrariesHandler, this );

    // Delete the GRID_TRICKS.
    // (Notebook page GRID_TRICKS are deleted by LIB_TABLE_NOTEBOOK_PANEL.)
    m_path_subs_grid->PopEventHandler( true );
}


void PANEL_SYM_LIB_TABLE::populatePluginList()
{
    for( const SCH_IO_MGR::SCH_FILE_T& type : SCH_IO_MGR::SCH_FILE_T_vector )
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( type ) );

        if( !pi )
            continue;

        if( const IO_BASE::IO_FILE_DESC& desc = pi->GetLibraryDesc() )
        {
            if( !desc.m_FileExtensions.empty() )
                m_supportedSymFiles.emplace( type, desc );
        }
    }

    m_supportedSymFiles.emplace( SCH_IO_MGR::SCH_NESTED_TABLE,
                                 IO_BASE::IO_FILE_DESC( _( "Table (nested library table)" ), {} ) );
}


SYMBOL_LIB_TABLE_GRID_DATA_MODEL* PANEL_SYM_LIB_TABLE::get_model( int aPage ) const
{
    return static_cast<SYMBOL_LIB_TABLE_GRID_DATA_MODEL*>( get_grid( aPage )->GetTable() );
}


WX_GRID* PANEL_SYM_LIB_TABLE::get_grid( int aPage ) const
{
    return static_cast<LIB_TABLE_NOTEBOOK_PANEL*>( m_notebook->GetPage( aPage ) )->GetGrid();
}


bool PANEL_SYM_LIB_TABLE::TransferDataToWindow()
{
    // for ALT+A handling, we want the initial focus to be on the first selected grid.
    m_parent->SetInitialFocus( cur_grid() );

    return true;
}


bool PANEL_SYM_LIB_TABLE::verifyTables()
{
    for( int page = 0 ; page < (int) m_notebook->GetPageCount(); ++page )
    {
        WX_GRID* grid = get_grid( page );

        if( !LIB_TABLE_GRID_TRICKS::VerifyTable( grid,
                [&]( int aRow, int aCol )
                {
                    // show the tabbed panel holding the grid we have flunked:
                    if( m_notebook->GetSelection() != page )
                        m_notebook->SetSelection( page );

                    grid->MakeCellVisible( aRow, 0 );
                    grid->SetGridCursor( aRow, aCol );
                } ) )
        {
            return false;
        }
    }

    return true;
}


void PANEL_SYM_LIB_TABLE::browseLibrariesHandler( wxCommandEvent& event )
{
    if( !cur_grid()->CommitPendingChanges() )
        return;

    SCH_IO_MGR::SCH_FILE_T fileType = SCH_IO_MGR::SCH_FILE_UNKNOWN;
    bool                   selectingFolder = false;

    // We are bound both to the menu and button with this one handler
    if( event.GetEventType() == wxEVT_BUTTON )
    {
        // Default to KiCad file format when clicking the button directly
        fileType = SCH_IO_MGR::SCH_KICAD;
    }
    else if( event.GetId() == ID_PANEL_SYM_LIB_KICAD_FOLDER )
    {
        // Special case for folder-based KiCad library
        fileType = SCH_IO_MGR::SCH_KICAD;
        selectingFolder = true;
    }
    else
    {
        fileType = static_cast<SCH_IO_MGR::SCH_FILE_T>( event.GetId() - FIRST_MENU_ID );
    }

    if( fileType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
        return;

    const ENV_VAR_MAP& envVars = Pgm().GetLocalEnvVariables();

    EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" );
    wxString           dummy;
    wxString*          lastDir;

    if( m_notebook->GetSelection() == 0 )
        lastDir = cfg ? &cfg->m_lastSymbolLibDir : &dummy;
    else
        lastDir = &m_lastProjectLibDir;

    wxString       title = wxString::Format( _( "Select %s Library" ), SCH_IO_MGR::ShowType( fileType ) );
    wxWindow*      topLevelParent = wxGetTopLevelParent( this );
    wxArrayString  files;

    if( selectingFolder )
    {
        wxDirDialog dlg( topLevelParent, title, *lastDir, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_MULTIPLE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        dlg.GetPaths( files );

        if( !files.IsEmpty() )
        {
            wxFileName first( files.front() );
            *lastDir = first.GetPath();
        }
    }
    else
    {
        auto it = m_supportedSymFiles.find( fileType );

        if( it == m_supportedSymFiles.end() )
            return;

        const IO_BASE::IO_FILE_DESC& fileDesc = it->second;

        wxFileDialog dlg( topLevelParent, title, *lastDir, wxEmptyString, fileDesc.FileFilter(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        dlg.GetPaths( files );
        *lastDir = dlg.GetDirectory();
    }

    bool     addDuplicates = false;
    bool     applyToAll    = false;
    wxString warning       = _( "Warning: Duplicate Nicknames" );
    wxString msg           = _( "An item nicknamed '%s' already exists." );
    wxString detailedMsg   = _( "One of the nicknames will need to be changed." );

    for( const wxString& filePath : files )
    {
        wxFileName fn( filePath );
        wxString   nickname = LIB_ID::FixIllegalChars( fn.GetName(), true );
        bool       doAdd    = true;

        if( cur_model()->ContainsNickname( nickname ) )
        {
            if( !applyToAll )
            {
                addDuplicates = OKOrCancelDialog( topLevelParent, warning,
                                                  wxString::Format( msg, nickname ), detailedMsg,
                                                  _( "Skip" ), _( "Add Anyway" ),
                                                  &applyToAll ) == wxID_CANCEL;
            }

            doAdd = addDuplicates;
        }

        if( doAdd && cur_grid()->AppendRows( 1 ) )
        {
            int last_row = cur_grid()->GetNumberRows() - 1;

            cur_grid()->SetCellValue( last_row, COL_NICKNAME, nickname );
            cur_grid()->SetCellValue( last_row, COL_TYPE, SCH_IO_MGR::ShowType( fileType ) );

            // try to use path normalized to an environmental variable or project path
            wxString path = NormalizePath( filePath, &envVars, m_project->GetProjectPath() );

            // Do not use the project path in the global library table.  This will almost
            // assuredly be wrong for a different project.
            if( m_notebook->GetSelection() == 0 && path.Contains( wxT( "${KIPRJMOD}" ) ) )
                path = fn.GetFullPath();

            cur_grid()->SetCellValue( last_row, COL_URI, path );
        }
    }

    if( !files.IsEmpty() )
    {
        cur_grid()->MakeCellVisible( cur_grid()->GetNumberRows() - 1, COL_ENABLED );
        cur_grid()->SetGridCursor( cur_grid()->GetNumberRows() - 1, COL_NICKNAME );
    }
}


void PANEL_SYM_LIB_TABLE::appendRowHandler( wxCommandEvent& event )
{
    LIB_TABLE_GRID_TRICKS::AppendRowHandler( cur_grid() );
}


void PANEL_SYM_LIB_TABLE::deleteRowHandler( wxCommandEvent& event )
{
    LIB_TABLE_GRID_TRICKS::DeleteRowHandler( cur_grid() );
}


void PANEL_SYM_LIB_TABLE::moveUpHandler( wxCommandEvent& event )
{
    LIB_TABLE_GRID_TRICKS::MoveUpHandler( cur_grid() );
}


void PANEL_SYM_LIB_TABLE::moveDownHandler( wxCommandEvent& event )
{
    LIB_TABLE_GRID_TRICKS::MoveDownHandler( cur_grid() );
}


void PANEL_SYM_LIB_TABLE::onReset( wxCommandEvent& event )
{
    if( !cur_grid()->CommitPendingChanges() )
        return;

    WX_GRID* grid = get_grid( 0 );

    // No need to prompt to preserve an empty table
    if( grid->GetNumberRows() > 0 && !IsOK( this, wxString::Format( _( "This action will reset your global library "
                                                                       "table on disk and cannot be undone." ) ) ) )
    {
        return;
    }

    wxString* lastGlobalLibDir = nullptr;

    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        if( cfg->m_lastSymbolLibDir.IsEmpty() )
            cfg->m_lastSymbolLibDir = PATHS::GetDefaultUserSymbolsPath();

        lastGlobalLibDir = &cfg->m_lastSymbolLibDir;
    }

    LIBRARY_MANAGER::CreateGlobalTable( LIBRARY_TABLE_TYPE::SYMBOL, true );

    // Go ahead and reload here because this action takes place even if the dialog is canceled
    Pgm().GetLibraryManager().LoadGlobalTables( { LIBRARY_TABLE_TYPE::SYMBOL } );

    if( KIFACE *schface = m_parent->Kiway().KiFACE( KIWAY::FACE_SCH ) )
        schface->PreloadLibraries( &m_parent->Kiway() );

    grid->Freeze();

    wxGridTableBase* table = grid->GetTable();
    grid->DestroyTable( table );

    std::optional<LIBRARY_TABLE*> newTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::SYMBOL,
                                                                              LIBRARY_TABLE_SCOPE::GLOBAL );
    wxASSERT( newTable );

    SYMBOL_LIBRARY_ADAPTER* adapter = PROJECT_SCH::SymbolLibAdapter( &m_parent->Kiway().Prj() );

    grid->SetTable( new SYMBOL_LIB_TABLE_GRID_DATA_MODEL( m_parent, grid, *newTable.value(), adapter, m_pluginChoices,
                                                          lastGlobalLibDir, wxEmptyString ),
                    true /* take ownership */ );

    m_parent->m_GlobalTableChanged = true;

    grid->Thaw();

    if( grid->GetNumberRows() > 0 )
    {
        grid->SetGridCursor( 0, COL_NICKNAME );
        grid->SelectRow( 0 );
    }
}


void PANEL_SYM_LIB_TABLE::onPageChange( wxAuiNotebookEvent& event )
{
    m_resetGlobal->Enable( m_notebook->GetSelection() == 0 );
}


void PANEL_SYM_LIB_TABLE::onNotebookPageCloseRequest( wxAuiNotebookEvent& aEvent )
{
    wxAuiNotebook* notebook = (wxAuiNotebook*) aEvent.GetEventObject();
    wxWindow*      page = notebook->GetPage( aEvent.GetSelection() );

    if( LIB_TABLE_NOTEBOOK_PANEL* panel = dynamic_cast<LIB_TABLE_NOTEBOOK_PANEL*>( page ) )
    {
        if( panel->GetClosable() )
        {
            if( !panel->GetCanClose() )
                aEvent.Veto();
        }
        else
        {
            aEvent.Veto();
        }
    }
}


void PANEL_SYM_LIB_TABLE::onConvertLegacyLibraries( wxCommandEvent& event )
{
    if( !cur_grid()->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = cur_grid()->GetSelectedRows();

    if( selectedRows.empty() && cur_grid()->GetGridCursorRow() >= 0 )
        selectedRows.push_back( cur_grid()->GetGridCursorRow() );

    wxArrayInt legacyRows;
    wxString   databaseType = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_DATABASE );
    wxString   kicadType = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_KICAD );
    wxString   msg;

    for( int row : selectedRows )
    {
        if( cur_grid()->GetCellValue( row, COL_TYPE ) != databaseType
                && cur_grid()->GetCellValue( row, COL_TYPE ) != kicadType )
        {
            legacyRows.push_back( row );
        }
    }

    if( legacyRows.size() <= 0 )
    {
        wxMessageBox( _( "Select one or more rows containing libraries to save as current KiCad format." ) );
        return;
    }
    else
    {
        if( legacyRows.size() == 1 )
        {
            msg.Printf( _( "Save '%s' as current KiCad format (*.kicad_sym) and replace legacy entry in table?" ),
                        cur_grid()->GetCellValue( legacyRows[0], COL_NICKNAME ) );
        }
        else
        {
            msg.Printf( _( "Save %d libraries as current KiCad format (*.kicad_sym) and replace legacy entries "
                           "in table?" ),
                        (int) legacyRows.size() );
        }

        if( !IsOK( m_parent, msg ) )
            return;
    }

    for( int row : legacyRows )
    {
        wxString   relPath = cur_grid()->GetCellValue( row, COL_URI );
        wxString   resolvedPath = ExpandEnvVarSubstitutions( relPath, m_project );
        wxFileName legacyLib( resolvedPath );

        if( !legacyLib.Exists() )
        {
            DisplayErrorMessage( m_parent, wxString::Format( _( "Library '%s' not found." ), relPath ) );
            continue;
        }

        wxFileName newLib( resolvedPath );
        newLib.SetExt( "kicad_sym" );

        if( newLib.Exists() )
        {
            msg.Printf( _( "File '%s' already exists. Do you want overwrite this file?" ), newLib.GetFullPath() );

            switch( wxMessageBox( msg, _( "Migrate Library" ), wxYES_NO | wxCANCEL | wxICON_QUESTION, m_parent ) )
            {
            case wxYES:    break;
            case wxNO:     continue;
            case wxCANCEL: return;
            }
        }

        wxString                    options = cur_grid()->GetCellValue( row, COL_OPTIONS );
        std::map<std::string, UTF8> props( LIBRARY_TABLE::ParseOptions( options.ToStdString() ) );

        if( SCH_IO_MGR::ConvertLibrary( &props, legacyLib.GetFullPath(), newLib.GetFullPath() ) )
        {
            relPath = NormalizePath( newLib.GetFullPath(), &Pgm().GetLocalEnvVariables(), m_project );

            cur_grid()->SetCellValue( row, COL_URI, relPath );
            cur_grid()->SetCellValue( row, COL_TYPE, kicadType );
            cur_grid()->SetCellValue( row, COL_OPTIONS, wxEmptyString );
        }
        else
        {
            DisplayErrorMessage( m_parent, wxString::Format( _( "Failed to save symbol library file '%s'." ),
                                                             newLib.GetFullPath() ) );
        }
    }
}


bool PANEL_SYM_LIB_TABLE::TransferDataFromWindow()
{
    if( !cur_grid()->CommitPendingChanges() )
        return false;

    if( !verifyTables() )
        return false;

    LIBRARY_MANAGER&              manager = Pgm().GetLibraryManager();
    std::optional<LIBRARY_TABLE*> optTable = manager.Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::GLOBAL );
    wxCHECK( optTable, false );
    LIBRARY_TABLE* globalTable = *optTable;

    if( get_model( 0 )->Table() != *globalTable )
    {
        m_parent->m_GlobalTableChanged = true;
        *globalTable = get_model( 0 )->Table();

        globalTable->Save().map_error(
                []( const LIBRARY_ERROR& aError )
                {
                    wxMessageBox( _( "Error saving global library table:\n\n" ) + aError.message,
                                  _( "File Save Error" ), wxOK | wxICON_ERROR );
                } );
    }

    optTable = manager.Table( LIBRARY_TABLE_TYPE::SYMBOL, LIBRARY_TABLE_SCOPE::PROJECT );

    if( optTable.has_value() && get_model( 1 )->Table().Path() == optTable.value()->Path() )
    {
        LIBRARY_TABLE* projectTable = *optTable;

        if( get_model( 1 )->Table() != *projectTable )
        {
            m_parent->m_ProjectTableChanged = true;
            *projectTable = get_model( 1 )->Table();

            projectTable->Save().map_error(
                    []( const LIBRARY_ERROR& aError )
                    {
                        wxMessageBox( _( "Error saving project library table:\n\n" ) + aError.message,
                                      _( "File Save Error" ), wxOK | wxICON_ERROR );
                    } );
        }
    }

    for( int ii = 0; ii < (int) m_notebook->GetPageCount(); ++ii )
    {
        LIB_TABLE_NOTEBOOK_PANEL* panel = static_cast<LIB_TABLE_NOTEBOOK_PANEL*>( m_notebook->GetPage( ii ) );

        if( panel->GetClosable() && panel->TableModified() )
        {
            panel->SaveTable();
            m_parent->m_GlobalTableChanged = true;
            m_parent->m_ProjectTableChanged = true;
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

    for( int page = 0 ; page < (int) m_notebook->GetPageCount(); ++page )
    {
        LIB_TABLE_GRID_DATA_MODEL* model = get_model( page );

        for( int row = 0; row < model->GetNumberRows(); ++row )
        {
            wxString uri = model->GetValue( row, COL_URI );

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


void InvokeSchEditSymbolLibTable( KIWAY* aKiway, wxWindow *aParent )
{
    auto symbolEditor = static_cast<SYMBOL_EDIT_FRAME*>( aKiway->Player( FRAME_SCH_SYMBOL_EDITOR, false ) );
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
        Pgm().GetLibraryManager().LoadGlobalTables( { LIBRARY_TABLE_TYPE::SYMBOL } );

    if( dlg.m_ProjectTableChanged )
    {
        // Trigger a reload of the table and cancel an in-progress background load
        Pgm().GetLibraryManager().ProjectChanged();
    }

    // Trigger a reload in case any libraries have been added or removed
    if( KIFACE *schface = aKiway->KiFACE( KIWAY::FACE_SCH ) )
        schface->PreloadLibraries( aKiway );

    if( symbolEditor )
        symbolEditor->ThawLibraryTree();
}
