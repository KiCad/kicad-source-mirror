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
#include <wx/dir.h>
#include <wx/regex.h>
#include <wx/grid.h>
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
#include <confirm.h>
#include <lib_table_grid_data_model.h>
#include <wildcards_and_files_ext.h>
#include <pgm_base.h>
#include <pcb_edit_frame.h>
#include <env_paths.h>
#include <dialogs/dialog_edit_library_tables.h>
#include <dialogs/dialog_plugin_options.h>
#include <footprint_viewer_frame.h>
#include <footprint_edit_frame.h>
#include <kiway.h>
#include <kiway_express.h>
#include <widgets/grid_readonly_text_helpers.h>
#include <widgets/grid_text_button_helpers.h>
#include <pcbnew_id.h>          // For ID_PCBNEW_END_LIST
#include <settings/settings_manager.h>
#include <paths.h>
#include <macros.h>
#include <project_pcb.h>
#include <common.h>
#include <dialog_HTML_reporter_base.h>
#include <libraries/library_manager.h>
#include <widgets/wx_html_report_box.h>
#include <widgets/grid_button.h>


/**
 * This class builds a wxGridTableBase by wrapping an #FP_LIB_TABLE object.
 */
class FP_LIB_TABLE_GRID_DATA_MODEL : public LIB_TABLE_GRID_DATA_MODEL
{
    friend class PANEL_FP_LIB_TABLE;
    friend class FP_GRID_TRICKS;

public:
    FP_LIB_TABLE_GRID_DATA_MODEL( DIALOG_SHIM* aParent, const LIBRARY_TABLE& aTableToEdit,
                                  FOOTPRINT_LIBRARY_ADAPTER* aAdapter, const wxArrayString& aPluginChoices,
                                  wxString* aMRUDirectory, const wxString& aProjectPath,
                                  const std::map<PCB_IO_MGR::PCB_FILE_T, IO_BASE::IO_FILE_DESC>& aSupportedFiles ) :
            LIB_TABLE_GRID_DATA_MODEL( aParent, aTableToEdit, aAdapter, aPluginChoices, aMRUDirectory, aProjectPath ),
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

            if( pluginType == PCB_IO_MGR::FILE_TYPE_NONE )
                pluginType = PCB_IO_MGR::KICAD_SEXP;

            SetValue( aRow, COL_TYPE, PCB_IO_MGR::ShowType( pluginType ) );
        }
    }

protected:
    wxString getFileTypes( WX_GRID* aGrid, int aRow ) override
    {
        FP_LIB_TABLE_GRID_DATA_MODEL*           libTable = static_cast<FP_LIB_TABLE_GRID_DATA_MODEL*>( aGrid->GetTable() );
        LIBRARY_TABLE_ROW&           tableRow = libTable->at( aRow );
        PCB_IO_MGR::PCB_FILE_T       fileType = PCB_IO_MGR::EnumFromStr( tableRow.Type() );
        const IO_BASE::IO_FILE_DESC& pluginDesc = m_supportedFpFiles.at( fileType );

        if( pluginDesc.m_IsFile )
            return pluginDesc.FileFilter();
        else
            return wxEmptyString;
    }

private:
    const std::map<PCB_IO_MGR::PCB_FILE_T, IO_BASE::IO_FILE_DESC>& m_supportedFpFiles;
};



class FP_GRID_TRICKS : public LIB_TABLE_GRID_TRICKS
{
public:
    FP_GRID_TRICKS( WX_GRID* aGrid ) :
            LIB_TABLE_GRID_TRICKS( aGrid )
    {
        SetTooltipEnable( COL_STATUS );
    }

    FP_GRID_TRICKS( WX_GRID* aGrid, std::function<void( wxCommandEvent& )> aAddHandler ) :
            LIB_TABLE_GRID_TRICKS( aGrid, aAddHandler )
    {
        SetTooltipEnable( COL_STATUS );
    }

protected:
    void optionsEditor( int aRow ) override
    {
        FP_LIB_TABLE_GRID_DATA_MODEL* tbl = (FP_LIB_TABLE_GRID_DATA_MODEL*) m_grid->GetTable();

        if( tbl->GetNumberRows() > aRow )
        {
            LIBRARY_TABLE_ROW& row = tbl->at( static_cast<size_t>( aRow ) );
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

    /// handle specialized clipboard text, with leading "(fp_lib_table", OR
    /// spreadsheet formatted text.
    void paste_text( const wxString& cb_text ) override
    {
        // TODO(JE) library tables
        FP_LIB_TABLE_GRID_DATA_MODEL* tbl = static_cast<FP_LIB_TABLE_GRID_DATA_MODEL*>( m_grid->GetTable() );

        if( size_t ndx = cb_text.find( "(fp_lib_table" ); ndx != std::string::npos )
        {
            // paste the FP_LIB_TABLE_ROWs of s-expression (fp_lib_table), starting
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
                DisplayError( wxGetTopLevelParent( m_grid ), tempTable.ErrorDescription() );
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
};


void PANEL_FP_LIB_TABLE::setupGrid( WX_GRID* aGrid )
{
    // add Cut, Copy, and Paste to wxGrids
    aGrid->PushEventHandler( new FP_GRID_TRICKS( aGrid,
            [this]( wxCommandEvent& event )
            {
                appendRowHandler( event );
            } ) );

    aGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // Gives a selection to each grid, mainly for delete button. wxGrid's wake up with
    // a currentCell which is sometimes not highlighted.
    if( aGrid->GetNumberRows() > 0 )
        aGrid->SelectRow( 0 );
};


PANEL_FP_LIB_TABLE::PANEL_FP_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* aProject ) :
        PANEL_FP_LIB_TABLE_BASE( aParent ),
        m_project( aProject ),
        m_parent( aParent )
{
    wxString* lastGlobalLibDir = nullptr;

    if( PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" ) )
    {
        if( cfg->m_LastFootprintLibDir.IsEmpty() )
            cfg->m_LastFootprintLibDir = PATHS::GetDefaultUserFootprintsPath();

        lastGlobalLibDir = &cfg->m_LastFootprintLibDir;
    }

    m_lastProjectLibDir = m_project->GetProjectPath();

    populatePluginList();

    for( auto& [fileType, desc] : m_supportedFpFiles )
        m_pluginChoices.Add( PCB_IO_MGR::ShowType( fileType ) );

    std::optional<LIBRARY_TABLE*> table = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
                                                                           LIBRARY_TABLE_SCOPE::GLOBAL );
    wxASSERT( table );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( m_project );

    m_global_grid->SetTable( new FP_LIB_TABLE_GRID_DATA_MODEL( m_parent, *table.value(), adapter, m_pluginChoices,
                                                               lastGlobalLibDir, wxEmptyString, m_supportedFpFiles ),
                             true /* take ownership */ );

    // add Cut, Copy, and Paste to wxGrids
    m_path_subs_grid->PushEventHandler( new GRID_TRICKS( m_path_subs_grid ) );

    setupGrid( m_global_grid );

    populateEnvironReadOnlyTable();

    std::optional<LIBRARY_TABLE*> projectTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
                                                                                  LIBRARY_TABLE_SCOPE::PROJECT );

    if( projectTable )
    {
        m_project_grid->SetTable( new FP_LIB_TABLE_GRID_DATA_MODEL( m_parent, *projectTable.value(), adapter,
                                                                    m_pluginChoices, &m_lastProjectLibDir,
                                                                    m_project->GetProjectPath(), m_supportedFpFiles ),
                                  true /* take ownership */ );
        setupGrid( m_project_grid );
    }
    else
    {
        m_notebook->DeletePage( 1 );
        m_project_grid = nullptr;
    }

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

    auto joinExts =
            []( const std::vector<std::string>& aExts )
            {
                wxString joined;

                for( const std::string& ext : aExts )
                {
                    if( !joined.empty() )
                        joined << wxS( ", " );

                    joined << wxS( "*." ) << ext;
                }

                return joined;
            };

    for( auto& [type, desc] : m_supportedFpFiles )
    {
        wxString entryStr = PCB_IO_MGR::ShowType( type );

        if( desc.m_IsFile && !desc.m_FileExtensions.empty() )
        {
            entryStr << wxString::Format( wxS( " (%s)" ), joinExts( desc.m_FileExtensions ) );
        }
        else if( !desc.m_IsFile && !desc.m_ExtensionsInDir.empty() )
        {
            wxString midPart = wxString::Format( _( "folder with %s files" ),
                                                 joinExts( desc.m_ExtensionsInDir ) );

            entryStr << wxString::Format( wxS( " (%s)" ), midPart );
        }

        browseMenu->Append( type, entryStr );

        browseMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_FP_LIB_TABLE::browseLibrariesHandler, this, type );
    }

    Layout();

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
    // Any additional event handlers should be popped before the window is deleted.
    m_global_grid->PopEventHandler( true );

    if( m_project_grid )
        m_project_grid->PopEventHandler( true );

    m_path_subs_grid->PopEventHandler( true );
}


bool PANEL_FP_LIB_TABLE::TransferDataToWindow()
{
    // No visibility control for footprint libraries yet; this feature is primarily
    // useful for database libraries and it's only implemented for schematic symbols
    // at the moment.
    m_global_grid->HideCol( COL_VISIBLE );
    m_project_grid->HideCol( COL_VISIBLE );

    m_cur_grid = ( m_notebook->GetSelection() == 0 ) ? m_global_grid : m_project_grid;

    // for ALT+A handling, we want the initial focus to be on the first selected grid.
    m_parent->SetInitialFocus( m_cur_grid );

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
}


bool PANEL_FP_LIB_TABLE::verifyTables()
{
    wxString msg;

    for( FP_LIB_TABLE_GRID_DATA_MODEL* model : { global_model(), project_model() } )
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
                if( !nick && !uri )
                    msg = _( "A library table row nickname and path cells are empty." );
                else if( !nick )
                    msg = _( "A library table row nickname cell is empty." );
                else
                    msg = _( "A library table row path cell is empty." );

                wxWindow* topLevelParent = wxGetTopLevelParent( this );

                wxMessageDialog badCellDlg( topLevelParent, msg, _( "Invalid Row Definition" ),
                                            wxYES_NO | wxCENTER | wxICON_QUESTION | wxYES_DEFAULT );
                badCellDlg.SetExtendedMessage( _( "Empty cells will result in all rows that are "
                                                  "invalid to be removed from the table." ) );
                badCellDlg.SetYesNoLabels( wxMessageDialog::ButtonLabel( _( "Remove Invalid Cells" ) ),
                                           wxMessageDialog::ButtonLabel( _( "Cancel Table Update" ) ) );

                if( badCellDlg.ShowModal() == wxID_NO )
                    return false;

                // Delete the "empty" row, where empty means missing nick or uri.
                // This also updates the UI which could be slow, but there should only be a few
                // rows to delete, unless the user fell asleep on the Add Row
                // button.
                model->DeleteRows( r, 1 );
            }
            else if( ( illegalCh = LIB_ID::FindIllegalLibraryNameChar( nick ) ) )
            {
                msg = wxString::Format( _( "Illegal character '%c' in nickname '%s'." ),
                                        illegalCh,
                                        nick );

                // show the tabbed panel holding the grid we have flunked:
                if( model != cur_model() )
                    m_notebook->SetSelection( model == global_model() ? 0 : 1 );

                m_cur_grid->MakeCellVisible( r, 0 );
                m_cur_grid->SetGridCursor( r, 1 );

                wxWindow* topLevelParent = wxGetTopLevelParent( this );

                wxMessageDialog errdlg( topLevelParent, msg, _( "Library Nickname Error" ) );
                errdlg.ShowModal();
                return false;
            }
            else
            {
                // set the trimmed values back into the table so they get saved to disk.
                model->SetValue( r, COL_NICKNAME, nick );
                model->SetValue( r, COL_URI, uri );

                // Make sure to not save a hidden flag
                model->SetValue( r, COL_VISIBLE, wxS( "1" ) );

                ++r;        // this row was OK.
            }
        }
    }

    // check for duplicate nickNames, separately in each table.
    for( FP_LIB_TABLE_GRID_DATA_MODEL* model : { global_model(), project_model() } )
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
                    m_cur_grid->SetGridCursor( r2, 1 );

                    wxWindow* topLevelParent = wxGetTopLevelParent( this );

                    wxMessageDialog errdlg( topLevelParent, msg, _( "Library Nickname Error" ) );
                    errdlg.ShowModal();
                    return false;
                }
            }
        }
    }

    return true;
}


void PANEL_FP_LIB_TABLE::appendRowHandler( wxCommandEvent& event )
{
    m_cur_grid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_cur_grid->AppendRows( 1 );
                return { m_cur_grid->GetNumberRows() - 1, COL_NICKNAME };
            } );
}


void PANEL_FP_LIB_TABLE::deleteRowHandler( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    wxGridUpdateLocker noUpdates( m_cur_grid );

    int curRow = m_cur_grid->GetGridCursorRow();
    int curCol = m_cur_grid->GetGridCursorCol();

    // In a wxGrid, collect rows that have a selected cell, or are selected
    // It is not so easy: it depends on the way the selection was made.
    // Here, we collect rows selected by clicking on a row label, and rows that contain any
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


void PANEL_FP_LIB_TABLE::moveUpHandler( wxCommandEvent& event )
{
    m_cur_grid->OnMoveRowUp(
            [&]( int row )
            {
                FP_LIB_TABLE_GRID_DATA_MODEL* tbl = cur_model();
                int curRow = m_cur_grid->GetGridCursorRow();

                std::vector<LIBRARY_TABLE_ROW>& rows = tbl->Table().Rows();

                auto current = rows.begin() + curRow;
                auto prev    = rows.begin() + curRow - 1;

                std::iter_swap( current, prev );

                // Update the wxGrid
                wxGridTableMessage msg( tbl, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, row - 1, 0 );
                tbl->GetView()->ProcessTableMessage( msg );
            } );
}


void PANEL_FP_LIB_TABLE::moveDownHandler( wxCommandEvent& event )
{
    m_cur_grid->OnMoveRowDown(
            [&]( int row )
            {
                FP_LIB_TABLE_GRID_DATA_MODEL* tbl = cur_model();
                int curRow = m_cur_grid->GetGridCursorRow();
                std::vector<LIBRARY_TABLE_ROW>& rows = tbl->Table().Rows();

                auto current = rows.begin() + curRow;
                auto next    = rows.begin() + curRow + 1;

                std::iter_swap( current, next );

                // Update the wxGrid
                wxGridTableMessage msg( tbl, wxGRIDTABLE_NOTIFY_ROWS_INSERTED, row, 0 );
                tbl->GetView()->ProcessTableMessage( msg );
            } );
}


// @todo refactor this function into single location shared with PANEL_SYM_LIB_TABLE
void PANEL_FP_LIB_TABLE::onMigrateLibraries( wxCommandEvent& event )
{
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    wxArrayInt selectedRows = m_cur_grid->GetSelectedRows();

    if( selectedRows.empty() && m_cur_grid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_cur_grid->GetGridCursorRow() );

    wxArrayInt rowsToMigrate;
    wxString   kicadType = PCB_IO_MGR::ShowType( PCB_IO_MGR::KICAD_SEXP );
    wxString   msg;
    DIALOG_HTML_REPORTER errorReporter( this );

    for( int row : selectedRows )
    {
        if( m_cur_grid->GetCellValue( row, COL_TYPE ) != kicadType )
            rowsToMigrate.push_back( row );
    }

    if( rowsToMigrate.size() <= 0 )
    {
        wxMessageBox( wxString::Format( _( "Select one or more rows containing libraries to save as "
                                           "current KiCad format." ) ) );
        return;
    }
    else
    {
        if( rowsToMigrate.size() == 1 )
        {
            msg.Printf( _( "Save '%s' as current KiCad format and replace entry in table?" ),
                        m_cur_grid->GetCellValue( rowsToMigrate[0], COL_NICKNAME ) );
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
        wxString   relPath = m_cur_grid->GetCellValue( row, COL_URI );
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

        wxString options = m_cur_grid->GetCellValue( row, COL_OPTIONS );
        std::map<std::string, UTF8> props( LIBRARY_TABLE::ParseOptions( options.ToStdString() ) );

        if( PCB_IO_MGR::ConvertLibrary( props, legacyLib.GetFullPath(), newLib.GetFullPath(),
                                        errorReporter.m_Reporter ) )
        {
            relPath = NormalizePath( newLib.GetFullPath(), &Pgm().GetLocalEnvVariables(), m_project );

            // Do not use the project path in the global library table.  This will almost
            // assuredly be wrong for a different project.
            if( m_cur_grid == m_global_grid && relPath.Contains( "${KIPRJMOD}" ) )
                relPath = newLib.GetFullPath();

            m_cur_grid->SetCellValue( row, COL_URI, relPath );
            m_cur_grid->SetCellValue( row, COL_TYPE, kicadType );
        }
        else
        {
            msg.Printf( _( "Failed to save footprint library file '%s'." ), newLib.GetFullPath() );
            DisplayErrorMessage( wxGetTopLevelParent( this ), msg );
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
    if( !m_cur_grid->CommitPendingChanges() )
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
    {
        wxLogWarning( wxT( "File type selection event received but could not find the file type in the table" ) );
        return;
    }

    const ENV_VAR_MAP&           envVars = Pgm().GetLocalEnvVariables();
    const IO_BASE::IO_FILE_DESC& fileDesc = m_supportedFpFiles.at( fileType );
    PCBNEW_SETTINGS*             cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );

    wxString  title = wxString::Format( _( "Select %s Library" ), PCB_IO_MGR::ShowType( fileType ) );
    wxString  dummy;
    wxString* lastDir;

    if( m_cur_grid == m_project_grid )
        lastDir = &m_lastProjectLibDir;
    else
        lastDir = cfg ? &cfg->m_LastFootprintLibDir : &dummy;

    wxArrayString files;

    wxWindow* topLevelParent = wxGetTopLevelParent( this );

    if( fileDesc.m_IsFile )
    {
        wxFileDialog dlg( topLevelParent, title, *lastDir, wxEmptyString, fileDesc.FileFilter(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        dlg.GetPaths( files );
        *lastDir = dlg.GetDirectory();
    }
    else
    {
        wxDirDialog dlg( topLevelParent, title, *lastDir,
                         wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_MULTIPLE );

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
    wxString msg           = _( "A library nicknamed '%s' already exists." );
    wxString detailedMsg   = _( "One of the nicknames will need to be changed after adding this library." );

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
                addDuplicates = OKOrCancelDialog( wxGetTopLevelParent( this ), warning,
                                                  wxString::Format( msg, nickname ),
                                                  detailedMsg, _( "Skip" ), _( "Add Anyway" ),
                                                  &applyToAll ) == wxID_CANCEL;
            }

            doAdd = addDuplicates;
        }

        if( doAdd && m_cur_grid->AppendRows( 1 ) )
        {
            int last_row = m_cur_grid->GetNumberRows() - 1;

            m_cur_grid->SetCellValue( last_row, COL_NICKNAME, nickname );

            m_cur_grid->SetCellValue( last_row, COL_TYPE, PCB_IO_MGR::ShowType( fileType ) );

            // try to use path normalized to an environmental variable or project path
            wxString path = NormalizePath( filePath, &envVars, m_project->GetProjectPath() );

            // Do not use the project path in the global library table.  This will almost
            // assuredly be wrong for a different project.
            if( m_notebook->GetSelection() == 0 && path.Contains( wxT( "${KIPRJMOD}" ) ) )
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
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    // No need to prompt to preserve an empty table
    if( m_global_grid->GetNumberRows() > 0
            && !IsOK( this, wxString::Format( _( "This action will reset your global library table on "
                                                 "disk and cannot be undone." ) ) ) )
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

    m_global_grid->Freeze();

    wxGridTableBase* table = m_global_grid->GetTable();
    m_global_grid->DestroyTable( table );

    std::optional<LIBRARY_TABLE*> newTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
                                                                              LIBRARY_TABLE_SCOPE::GLOBAL );
    wxASSERT( newTable );

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( m_project );

    m_global_grid->SetTable( new FP_LIB_TABLE_GRID_DATA_MODEL( m_parent, *newTable.value(), adapter, m_pluginChoices,
                                                               lastGlobalLibDir, wxEmptyString, m_supportedFpFiles ),
                             true /* take ownership */ );
    m_global_grid->PopEventHandler( true );
    setupGrid( m_global_grid );
    m_parent->m_GlobalTableChanged = true;

    m_global_grid->Thaw();
}


void PANEL_FP_LIB_TABLE::onPageChange( wxBookCtrlEvent& event )
{
    if( m_notebook->GetSelection() == 0 )
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


bool PANEL_FP_LIB_TABLE::TransferDataFromWindow()
{
    if( !m_cur_grid->CommitPendingChanges() )
        return false;

    if( !verifyTables() )
        return false;

    std::optional<LIBRARY_TABLE*> optTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
                                                                              LIBRARY_TABLE_SCOPE::GLOBAL );
    wxCHECK( optTable, false );
    LIBRARY_TABLE* globalTable = *optTable;

    if( global_model()->Table() != *globalTable )
    {
        m_parent->m_GlobalTableChanged = true;
        *globalTable = global_model()->Table();
    }

    optTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT, LIBRARY_TABLE_SCOPE::PROJECT );

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


/// Populate the readonly environment variable table with names and values by examining all
/// the full_uri columns.
void PANEL_FP_LIB_TABLE::populateEnvironReadOnlyTable()
{
    wxRegEx re( ".*?(\\$\\{(.+?)\\})|(\\$\\((.+?)\\)).*?", wxRE_ADVANCED );
    wxASSERT( re.IsValid() );   // wxRE_ADVANCED is required.

    std::set< wxString > unique;

    // clear the table
    m_path_subs_grid->ClearRows();

    for( FP_LIB_TABLE_GRID_DATA_MODEL* tbl : { global_model(), project_model() } )
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
    wxString      msg;

    DIALOG_EDIT_LIBRARY_TABLES dlg( aCaller, _( "Footprint Libraries" ) );
    dlg.SetKiway( &dlg, aKiway );

    dlg.InstallPanel( new PANEL_FP_LIB_TABLE( &dlg, &aKiway->Prj() ) );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( dlg.m_GlobalTableChanged )
    {
        std::optional<LIBRARY_TABLE*> optTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
                                                                                  LIBRARY_TABLE_SCOPE::GLOBAL );
        wxCHECK( optTable, /* void */ );
        LIBRARY_TABLE* globalTable = *optTable;

        globalTable->Save().map_error(
                []( const LIBRARY_ERROR& aError )
                {
                    wxMessageBox( wxString::Format( _( "Error saving global library table:\n\n%s" ), aError.message ),
                                  _( "File Save Error" ), wxOK | wxICON_ERROR );
                } );

        Pgm().GetLibraryManager().LoadGlobalTables( { LIBRARY_TABLE_TYPE::FOOTPRINT } );
    }

    std::optional<LIBRARY_TABLE*> projectTable = Pgm().GetLibraryManager().Table( LIBRARY_TABLE_TYPE::FOOTPRINT,
                                                                                  LIBRARY_TABLE_SCOPE::PROJECT );

    if( projectTable && dlg.m_ProjectTableChanged )
    {
        ( *projectTable )->Save().map_error(
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
    if( KIFACE *face = aKiway->KiFACE( KIWAY::FACE_PCB ) )
        face->PreloadLibraries( aKiway );

    std::string payload = "";
    aKiway->ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_RELOAD_LIB, payload );
    aKiway->ExpressMail( FRAME_FOOTPRINT_VIEWER, MAIL_RELOAD_LIB, payload );
    aKiway->ExpressMail( FRAME_CVPCB, MAIL_RELOAD_LIB, payload );
}
