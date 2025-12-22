/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013-2021 CERN
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


#include <set>
#include <wx/regex.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <functional>

#include <project.h>
#include <env_vars.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <panel_fp_lib_table.h>
#include <lib_id.h>
#include <footprint_library_adapter.h>
#include <lib_table_lexer.h>
#include <invoke_pcb_dialog.h>
#include <bitmaps.h>
#include <lib_table_grid_tricks.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/grid_readonly_text_helpers.h>
#include <widgets/wx_html_report_box.h>
#include <lib_table_notebook_panel.h>
#include <confirm.h>
#include <lib_table_grid_data_model.h>
#include <wildcards_and_files_ext.h>
#include <pgm_base.h>
#include <pcb_edit_frame.h>
#include <env_paths.h>
#include <dialogs/dialog_edit_library_tables.h>
#include <dialogs/dialog_plugin_options.h>
#include <footprint_viewer_frame.h>
#include <kiway.h>
#include <kiway_express.h>
#include <pcbnew_id.h>          // For ID_PCBNEW_END_LIST
#include <settings/settings_manager.h>
#include <paths.h>
#include <macros.h>
#include <project_pcb.h>
#include <common.h>
#include <dialog_HTML_reporter_base.h>
#include <libraries/library_manager.h>
#include <widgets/wx_aui_art_providers.h>


/**
 * This class builds a wxGridTableBase by wrapping an #FP_LIB_TABLE object.
 */
class FP_LIB_TABLE_GRID_DATA_MODEL : public LIB_TABLE_GRID_DATA_MODEL
{
public:
    FP_LIB_TABLE_GRID_DATA_MODEL( DIALOG_SHIM* aParent, WX_GRID* aGrid, const LIBRARY_TABLE& aTableToEdit,
                                  FOOTPRINT_LIBRARY_ADAPTER* aAdapter, const wxArrayString& aPluginChoices,
                                  wxString* aMRUDirectory, const wxString& aProjectPath,
                                  const std::map<PCB_IO_MGR::PCB_FILE_T, IO_BASE::IO_FILE_DESC>& aSupportedFiles ) :
            LIB_TABLE_GRID_DATA_MODEL( aParent, aGrid, aTableToEdit, aAdapter, aPluginChoices, aMRUDirectory,
                                       aProjectPath ),
            m_supportedFpFiles( aSupportedFiles )
    {
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        wxCHECK( aRow < (int) size(), /* void */ );

        LIB_TABLE_GRID_DATA_MODEL::SetValue( aRow, aCol, aValue );

        // If setting a filepath, attempt to auto-detect the format
        if( aCol == COL_URI )
        {
            LIBRARY_TABLE_ROW& row = at( (size_t) aRow );
            wxString uri = LIBRARY_MANAGER::ExpandURI( row.URI(), Pgm().GetSettingsManager().Prj() );
            PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::GuessPluginTypeFromLibPath( uri );

            if( pluginType != PCB_IO_MGR::FILE_TYPE_NONE )
                SetValue( aRow, COL_TYPE, PCB_IO_MGR::ShowType( pluginType ) );
        }
    }

protected:
    wxString getFileTypes( WX_GRID* aGrid, int aRow ) override
    {
        FP_LIB_TABLE_GRID_DATA_MODEL* table = static_cast<FP_LIB_TABLE_GRID_DATA_MODEL*>( aGrid->GetTable() );
        LIBRARY_TABLE_ROW&            tableRow = table->at( aRow );

        if( tableRow.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
            return wxEmptyString;

        PCB_IO_MGR::PCB_FILE_T        fileType = PCB_IO_MGR::EnumFromStr( tableRow.Type() );

        if( fileType == PCB_IO_MGR::PCB_FILE_UNKNOWN )
            return wxEmptyString;

        const IO_BASE::IO_FILE_DESC&  pluginDesc = m_supportedFpFiles.at( fileType );

        if( pluginDesc.m_IsFile )
            return pluginDesc.FileFilter();

        return wxEmptyString;
    }

private:
    const std::map<PCB_IO_MGR::PCB_FILE_T, IO_BASE::IO_FILE_DESC>& m_supportedFpFiles;
};



class FP_GRID_TRICKS : public LIB_TABLE_GRID_TRICKS
{
public:
    FP_GRID_TRICKS( PANEL_FP_LIB_TABLE* aPanel, WX_GRID* aGrid, std::function<void( wxCommandEvent& )> aAddHandler ) :
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
            const wxString& options = row.Options();
            wxString        result = options;
            std::map<std::string, UTF8> choices;

            PCB_IO_MGR::PCB_FILE_T pi_type = PCB_IO_MGR::EnumFromStr( row.Type() );
            IO_RELEASER<PCB_IO>    pi( PCB_IO_MGR::FindPlugin( pi_type ) );
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
        wxFileName uri = LIBRARY_MANAGER::ExpandURI( aRow.URI(), Pgm().GetSettingsManager().Prj() );
        auto       nestedTable = std::make_unique<LIBRARY_TABLE>( uri, LIBRARY_TABLE_SCOPE::GLOBAL );

        m_panel->AddTable( nestedTable.get(), aRow.Nickname(), true );
    }

    wxString getTablePreamble() override
    {
        return wxT( "(fp_lib_table" );
    }

protected:
    PANEL_FP_LIB_TABLE* m_panel;
};


void PANEL_FP_LIB_TABLE::AddTable( LIBRARY_TABLE* aTable, const wxString& aTitle, bool aClosable )
{
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( m_project );
    wxString                   projectPath = m_project->GetProjectPath();

    LIB_TABLE_NOTEBOOK_PANEL::AddTable( m_notebook, aTitle, aClosable );

    WX_GRID* grid = get_grid( (int) m_notebook->GetPageCount() - 1 );

    if( aTable->Path().StartsWith( projectPath ) )
    {
        grid->SetTable( new FP_LIB_TABLE_GRID_DATA_MODEL( m_parent, grid, *aTable, adapter, m_pluginChoices,
                                                          &m_lastProjectLibDir, projectPath, m_supportedFpFiles ),
                        true /* take ownership */ );
    }
    else
    {
        wxString* lastGlobalLibDir = nullptr;

        if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
        {
            if( cfg->m_LastFootprintLibDir.IsEmpty() )
                cfg->m_LastFootprintLibDir = PATHS::GetDefaultUserFootprintsPath();

            lastGlobalLibDir = &cfg->m_LastFootprintLibDir;
        }

        grid->SetTable( new FP_LIB_TABLE_GRID_DATA_MODEL( m_parent, grid, *aTable, adapter, m_pluginChoices,
                                                          lastGlobalLibDir, wxEmptyString, m_supportedFpFiles ),
                        true /* take ownership */ );
    }

    // add Cut, Copy, and Paste to wxGrids
    grid->PushEventHandler( new FP_GRID_TRICKS( this, grid,
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


PANEL_FP_LIB_TABLE::PANEL_FP_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* aProject ) :
        PANEL_FP_LIB_TABLE_BASE( aParent ),
        m_project( aProject ),
        m_parent( aParent )
{
    m_lastProjectLibDir = m_project->GetProjectPath();

    populatePluginList();

    for( auto& [fileType, desc] : m_supportedFpFiles )
        m_pluginChoices.Add( PCB_IO_MGR::ShowType( fileType ) );

    std::optional<LIBRARY_TABLE*> table = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
                                                                           LIBRARY_TABLE_SCOPE::GLOBAL );
    wxASSERT( table.has_value() );

    AddTable( table.value(), _( "Global Libraries" ), false /* closable */ );

    std::optional<LIBRARY_TABLE*> projectTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
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
    // manually (for instance m_append_button)
    Layout();   // Needed at least on MSW to compute the actual buttons sizes, after initializing
                // their bitmaps
    wxSize buttonSize = m_append_button->GetSize();

    m_browseButton->SetWidthPadding( 4 );
    m_browseButton->SetMinSize( buttonSize );

    // Populate the browse library options
    wxMenu* browseMenu = m_browseButton->GetSplitButtonMenu();

    for( auto& [type, desc] : m_supportedFpFiles )
    {
        wxString entryStr = PCB_IO_MGR::ShowType( type );
        wxString midPart;

        if( desc.m_IsFile && !desc.m_FileExtensions.empty() )
        {
            entryStr << wxString::Format( wxS( " (%s)" ), JoinExtensions( desc.m_FileExtensions ) );
        }
        else if( !desc.m_IsFile && !desc.m_ExtensionsInDir.empty() )
        {
            midPart = wxString::Format( _( "folder with %s files" ), JoinExtensions( desc.m_ExtensionsInDir ) );
            entryStr << wxString::Format( wxS( " (%s)" ), midPart );
        }

        browseMenu->Append( type, entryStr );
        browseMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_FP_LIB_TABLE::browseLibrariesHandler, this, type );
    }

    Layout();

    m_notebook->Bind( wxEVT_AUINOTEBOOK_PAGE_CLOSE, &PANEL_FP_LIB_TABLE::onNotebookPageCloseRequest, this );
    // This is the button only press for the browse button instead of the menu
    m_browseButton->Bind( wxEVT_BUTTON, &PANEL_FP_LIB_TABLE::browseLibrariesHandler, this );
}


PANEL_FP_LIB_TABLE::~PANEL_FP_LIB_TABLE()
{
    wxMenu* browseMenu = m_browseButton->GetSplitButtonMenu();

    for( auto& [type, desc] : m_supportedFpFiles )
        browseMenu->Unbind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_FP_LIB_TABLE::browseLibrariesHandler, this, type );

    m_browseButton->Unbind( wxEVT_BUTTON, &PANEL_FP_LIB_TABLE::browseLibrariesHandler, this );

    // Delete the GRID_TRICKS.
    // (Notebook page GRID_TRICKS are deleted by LIB_TABLE_NOTEBOOK_PANEL.)
    m_path_subs_grid->PopEventHandler( true );
}


FP_LIB_TABLE_GRID_DATA_MODEL* PANEL_FP_LIB_TABLE::get_model( int aPage ) const
{
    return static_cast<FP_LIB_TABLE_GRID_DATA_MODEL*>( get_grid( aPage )->GetTable() );
}


WX_GRID* PANEL_FP_LIB_TABLE::get_grid( int aPage ) const
{
    return static_cast<LIB_TABLE_NOTEBOOK_PANEL*>( m_notebook->GetPage( aPage ) )->GetGrid();
}


bool PANEL_FP_LIB_TABLE::TransferDataToWindow()
{
    // No visibility control for footprint libraries yet; this feature is primarily
    // useful for database libraries and it's only implemented for schematic symbols
    // at the moment.
    for( int page = 0 ; page < (int) m_notebook->GetPageCount(); ++page )
    {
        WX_GRID* grid = get_grid( page );
        grid->HideCol( COL_VISIBLE );
    }

    // for ALT+A handling, we want the initial focus to be on the first selected grid.
    m_parent->SetInitialFocus( cur_grid() );

    return true;
}


void PANEL_FP_LIB_TABLE::populatePluginList()
{
    for( const auto& plugin : PCB_IO_MGR::PLUGIN_REGISTRY::Instance()->AllPlugins() )
    {
        IO_RELEASER<PCB_IO> pi( plugin.m_createFunc() );

        if( !pi )
            continue;

        if( const IO_BASE::IO_FILE_DESC& desc = pi->GetLibraryDesc() )
            m_supportedFpFiles.emplace( plugin.m_type, desc );
    }

    m_supportedFpFiles.emplace( PCB_IO_MGR::NESTED_TABLE,
                                IO_BASE::IO_FILE_DESC( _( "Table (nested library table)" ), {} ) );
}


bool PANEL_FP_LIB_TABLE::verifyTables()
{
    wxString msg;

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


void PANEL_FP_LIB_TABLE::appendRowHandler( wxCommandEvent& event )
{
    LIB_TABLE_GRID_TRICKS::AppendRowHandler( cur_grid() );
}


void PANEL_FP_LIB_TABLE::deleteRowHandler( wxCommandEvent& event )
{
    LIB_TABLE_GRID_TRICKS::DeleteRowHandler( cur_grid() );
}


void PANEL_FP_LIB_TABLE::moveUpHandler( wxCommandEvent& event )
{
    LIB_TABLE_GRID_TRICKS::MoveUpHandler( cur_grid() );
}


void PANEL_FP_LIB_TABLE::moveDownHandler( wxCommandEvent& event )
{
    LIB_TABLE_GRID_TRICKS::MoveDownHandler( cur_grid() );
}


// @todo refactor this function into single location shared with PANEL_SYM_LIB_TABLE
void PANEL_FP_LIB_TABLE::onMigrateLibraries( wxCommandEvent& event )
{
    if( !cur_grid()->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = cur_grid()->GetSelectedRows();

    if( selectedRows.empty() && cur_grid()->GetGridCursorRow() >= 0 )
        selectedRows.push_back( cur_grid()->GetGridCursorRow() );

    wxArrayInt rowsToMigrate;
    wxString   kicadType = PCB_IO_MGR::ShowType( PCB_IO_MGR::KICAD_SEXP );
    wxString   msg;
    DIALOG_HTML_REPORTER errorReporter( this );

    for( int row : selectedRows )
    {
        if( cur_grid()->GetCellValue( row, COL_TYPE ) != kicadType )
            rowsToMigrate.push_back( row );
    }

    if( rowsToMigrate.size() <= 0 )
    {
        wxMessageBox( _( "Select one or more rows containing libraries to save as current KiCad format." ) );
        return;
    }
    else
    {
        if( rowsToMigrate.size() == 1 )
        {
            msg.Printf( _( "Save '%s' as current KiCad format and replace entry in table?" ),
                        cur_grid()->GetCellValue( rowsToMigrate[0], COL_NICKNAME ) );
        }
        else
        {
            msg.Printf( _( "Save %d libraries as current KiCad format and replace entries in table?" ),
                        (int) rowsToMigrate.size() );
        }

        if( !IsOK( m_parent, msg ) )
            return;
    }

    for( int row : rowsToMigrate )
    {
        wxString   relPath = cur_grid()->GetCellValue( row, COL_URI );
        wxString   resolvedPath = ExpandEnvVarSubstitutions( relPath, m_project );
        wxFileName legacyLib( resolvedPath );

        if( !legacyLib.Exists() )
        {
            msg.Printf( _( "Library '%s' not found." ), relPath );
            DisplayErrorMessage( wxGetTopLevelParent( this ), msg );
            continue;
        }

        wxFileName newLib( resolvedPath );
        newLib.AppendDir( newLib.GetName() + "." + FILEEXT::KiCadFootprintLibPathExtension );
        newLib.SetName( "" );
        newLib.ClearExt();

        if( newLib.DirExists() )
        {
            msg.Printf( _( "Folder '%s' already exists. Do you want overwrite any existing footprints?" ),
                        newLib.GetFullPath() );

            switch( wxMessageBox( msg, _( "Migrate Library" ), wxYES_NO|wxCANCEL|wxICON_QUESTION, m_parent ) )
            {
            case wxYES:    break;
            case wxNO:     continue;
            case wxCANCEL: return;
            }
        }

        wxString options = cur_grid()->GetCellValue( row, COL_OPTIONS );
        std::map<std::string, UTF8> props( LIBRARY_TABLE::ParseOptions( options.ToStdString() ) );

        if( PCB_IO_MGR::ConvertLibrary( props, legacyLib.GetFullPath(), newLib.GetFullPath(),
                                        errorReporter.m_Reporter ) )
        {
            relPath = NormalizePath( newLib.GetFullPath(), &Pgm().GetLocalEnvVariables(), m_project );

            cur_grid()->SetCellValue( row, COL_URI, relPath );
            cur_grid()->SetCellValue( row, COL_TYPE, kicadType );
        }
        else
        {
            DisplayErrorMessage( m_parent, wxString::Format( _( "Failed to save footprint library file '%s'." ),
                                                             newLib.GetFullPath() ) );
        }
    }

    if( errorReporter.m_Reporter->HasMessage() )
    {
        errorReporter.m_Reporter->Flush(); // Build HTML messages
        errorReporter.ShowModal();
    }
}


void PANEL_FP_LIB_TABLE::browseLibrariesHandler( wxCommandEvent& event )
{
    if( !cur_grid()->CommitPendingChanges() )
        return;

    PCB_IO_MGR::PCB_FILE_T fileType = PCB_IO_MGR::FILE_TYPE_NONE;

    // We are bound both to the menu and button with this one handler
    // So we must set the file type based on it
    if( event.GetEventType() == wxEVT_BUTTON )
    {
        // Let's default to adding a kicad footprint file for just the footprint
        fileType = PCB_IO_MGR::KICAD_SEXP;
    }
    else
    {
        fileType = static_cast<PCB_IO_MGR::PCB_FILE_T>( event.GetId() );
    }

    if( fileType == PCB_IO_MGR::FILE_TYPE_NONE )
        return;

    const ENV_VAR_MAP&           envVars = Pgm().GetLocalEnvVariables();
    const IO_BASE::IO_FILE_DESC& fileDesc = m_supportedFpFiles.at( fileType );
    PCBNEW_SETTINGS*             cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    wxString  title;
    wxString  dummy;
    wxString* lastDir;

    if( fileType == PCB_IO_MGR::NESTED_TABLE )
        title = _( "Select Library Table" );
    else
        title = wxString::Format( _( "Select %s Library" ), PCB_IO_MGR::ShowType( fileType ) );

    if( m_notebook->GetSelection() == 0 )
        lastDir = cfg ? &cfg->m_LastFootprintLibDir : &dummy;
    else
        lastDir = &m_lastProjectLibDir;

    wxArrayString files;

    if( fileDesc.m_IsFile )
    {
        wxFileDialog dlg( m_parent, title, *lastDir, wxEmptyString, fileDesc.FileFilter(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        dlg.GetPaths( files );
        *lastDir = dlg.GetDirectory();
    }
    else
    {
        wxDirDialog dlg( m_parent, title, *lastDir, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_MULTIPLE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        dlg.GetPaths( files );

        if( !files.IsEmpty() )
        {
            wxFileName first( files.front() );
            *lastDir = first.GetPath();
        }
    }

    // Drop the last directory if the path is a .pretty folder
    if( cfg && cfg->m_LastFootprintLibDir.EndsWith( FILEEXT::KiCadFootprintLibPathExtension ) )
        cfg->m_LastFootprintLibDir = cfg->m_LastFootprintLibDir.BeforeLast( wxFileName::GetPathSeparator() );


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

        if( fileType == PCB_IO_MGR::KICAD_SEXP
            && fn.GetExt() != FILEEXT::KiCadFootprintLibPathExtension )
        {
            nickname = LIB_ID::FixIllegalChars( fn.GetFullName(), true ).wx_str();
        }

        if( cur_model()->ContainsNickname( nickname ) )
        {
            if( !applyToAll )
            {
                // The cancel button adds the library to the table anyway
                addDuplicates = OKOrCancelDialog( m_parent, warning, wxString::Format( msg, nickname ), detailedMsg,
                                                  _( "Skip" ), _( "Add Anyway" ), &applyToAll ) == wxID_CANCEL;
            }

            doAdd = addDuplicates;
        }

        if( doAdd && cur_grid()->AppendRows( 1 ) )
        {
            int last_row = cur_grid()->GetNumberRows() - 1;

            cur_grid()->SetCellValue( last_row, COL_NICKNAME, nickname );
            cur_grid()->SetCellValue( last_row, COL_TYPE, PCB_IO_MGR::ShowType( fileType ) );

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


void PANEL_FP_LIB_TABLE::onNotebookPageCloseRequest( wxAuiNotebookEvent& aEvent )
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


void PANEL_FP_LIB_TABLE::adjustPathSubsGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_path_subs_grid->GetSize().x - m_path_subs_grid->GetClientSize().x );

    m_path_subs_grid->AutoSizeColumn( 0 );
    m_path_subs_grid->SetColSize( 0, std::max( 72, m_path_subs_grid->GetColSize( 0 ) ) );
    m_path_subs_grid->SetColSize( 1, std::max( 120, aWidth - m_path_subs_grid->GetColSize( 0 ) ) );
}


void PANEL_FP_LIB_TABLE::onSizeGrid( wxSizeEvent& event )
{
    adjustPathSubsGridColumns( event.GetSize().GetX() );

    event.Skip();
}


void PANEL_FP_LIB_TABLE::onReset( wxCommandEvent& event )
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

    if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
    {
        if( cfg->m_LastFootprintLibDir.IsEmpty() )
            cfg->m_LastFootprintLibDir = PATHS::GetDefaultUserFootprintsPath();

        lastGlobalLibDir = &cfg->m_LastFootprintLibDir;
    }

    LIBRARY_MANAGER::CreateGlobalTable( LIBRARY_TABLE_TYPE::FOOTPRINT, true );

    // Go ahead and reload here because this action takes place even if the dialog is canceled
    Pgm().GetLibraryManager().LoadGlobalTables( { LIBRARY_TABLE_TYPE::FOOTPRINT } );

    if( KIFACE *face = m_parent->Kiway().KiFACE( KIWAY::FACE_PCB ) )
        face->PreloadLibraries( &m_parent->Kiway() );

    grid->Freeze();

    wxGridTableBase* table = grid->GetTable();
    grid->DestroyTable( table );

    std::optional<LIBRARY_TABLE*> newTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
                                                                              LIBRARY_TABLE_SCOPE::GLOBAL );
    wxASSERT( newTable );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( m_project );

    grid->SetTable( new FP_LIB_TABLE_GRID_DATA_MODEL( m_parent, grid, *newTable.value(), adapter, m_pluginChoices,
                                                      lastGlobalLibDir, wxEmptyString, m_supportedFpFiles ),
                    true /* take ownership */ );

    m_parent->m_GlobalTableChanged = true;

    grid->Thaw();

    if( grid->GetNumberRows() > 0 )
    {
        grid->SetGridCursor( 0, COL_NICKNAME );
        grid->SelectRow( 0 );
    }
}


void PANEL_FP_LIB_TABLE::onPageChange( wxAuiNotebookEvent& event )
{
    m_resetGlobal->Enable( m_notebook->GetSelection() == 0 );
}


bool PANEL_FP_LIB_TABLE::TransferDataFromWindow()
{
    if( !cur_grid()->CommitPendingChanges() )
        return false;

    if( !verifyTables() )
        return false;

    std::optional<LIBRARY_TABLE*> optTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
                                                                              LIBRARY_TABLE_SCOPE::GLOBAL );
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

    optTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT, LIBRARY_TABLE_SCOPE::PROJECT );

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


/// Populate the readonly environment variable table with names and values by examining all
/// the full_uri columns.
void PANEL_FP_LIB_TABLE::populateEnvironReadOnlyTable()
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

    // Make sure this special environment variable shows up even if it was not used yet.  It is
    // automatically set by KiCad to the directory holding the current project.
    unique.insert( PROJECT_VAR_NAME );
    unique.insert( ENV_VAR::GetVersionedEnvVarName( wxS( "FOOTPRINT_DIR" ) ) );

    // This special environment variable is used to locate 3d shapes
    unique.insert( ENV_VAR::GetVersionedEnvVarName( wxS( "3DMODEL_DIR" ) ) );

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


//-----</event handlers>---------------------------------



void InvokePcbLibTableEditor( KIWAY* aKiway, wxWindow* aCaller )
{
    DIALOG_EDIT_LIBRARY_TABLES dlg( aCaller, _( "Footprint Libraries" ) );
    dlg.SetKiway( &dlg, aKiway );

    dlg.InstallPanel( new PANEL_FP_LIB_TABLE( &dlg, &aKiway->Prj() ) );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( dlg.m_GlobalTableChanged )
        Pgm().GetLibraryManager().LoadGlobalTables( { LIBRARY_TABLE_TYPE::FOOTPRINT } );

    if( dlg.m_ProjectTableChanged )
    {
        // Trigger a reload of the table and cancel an in-progress background load
        Pgm().GetLibraryManager().ProjectChanged();
    }

    // Trigger a reload in case any libraries have been added or removed
    if( KIFACE *face = aKiway->KiFACE( KIWAY::FACE_PCB ) )
        face->PreloadLibraries( aKiway );

    std::string payload = "";
    aKiway->ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_RELOAD_LIB, payload );
    aKiway->ExpressMail( FRAME_FOOTPRINT_VIEWER, MAIL_RELOAD_LIB, payload );
    aKiway->ExpressMail( FRAME_CVPCB, MAIL_RELOAD_LIB, payload );
}
