/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013-2021 CERN
 * Copyright (C) 2012-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/dir.h>
#include <wx/regex.h>
#include <wx/grid.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>

#include <project.h>
#include <env_vars.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <panel_fp_lib_table.h>
#include <lib_id.h>
#include <fp_lib_table.h>
#include <lib_table_lexer.h>
#include <invoke_pcb_dialog.h>
#include <bitmaps.h>
#include <lib_table_grid_tricks.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <confirm.h>
#include <lib_table_grid.h>
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
    PCB_IO_MGR::PCB_FILE_T m_Plugin;
};

// clang-format on

/**
 * Traverser implementation that looks to find any and all "folder" libraries by looking for files
 * with a specific extension inside folders
 */
class LIBRARY_TRAVERSER : public wxDirTraverser
{
public:
    LIBRARY_TRAVERSER( std::vector<std::string> aSearchExtensions, wxString aInitialDir ) :
            m_searchExtensions( aSearchExtensions ), m_currentDir( aInitialDir )
    {
    }

    virtual wxDirTraverseResult OnFile( const wxString& aFileName ) override
    {
        wxFileName file( aFileName );

        for( const std::string& ext : m_searchExtensions )
        {
            if( file.GetExt().IsSameAs( ext, false ) )
                m_foundDirs.insert( { m_currentDir, 1 } );
        }

        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnOpenError( const wxString& aOpenErrorName ) override
    {
        m_failedDirs.insert( { aOpenErrorName, 1 } );
        return wxDIR_IGNORE;
    }

    bool HasDirectoryOpenFailures()
    {
        return m_failedDirs.size() > 0;
    }

    virtual wxDirTraverseResult OnDir( const wxString& aDirName ) override
    {
        m_currentDir = aDirName;
        return wxDIR_CONTINUE;
    }

    void GetPaths( wxArrayString& aPathArray )
    {
        for( std::pair<const wxString, int>& foundDirsPair : m_foundDirs )
            aPathArray.Add( foundDirsPair.first );
    }

    void GetFailedPaths( wxArrayString& aPathArray )
    {
        for( std::pair<const wxString, int>& failedDirsPair : m_failedDirs )
            aPathArray.Add( failedDirsPair.first );
    }

private:
    std::vector<std::string>          m_searchExtensions;
    wxString                          m_currentDir;
    std::unordered_map<wxString, int> m_foundDirs;
    std::unordered_map<wxString, int> m_failedDirs;
};


/**
 * This class builds a wxGridTableBase by wrapping an #FP_LIB_TABLE object.
 */
class FP_LIB_TABLE_GRID : public LIB_TABLE_GRID, public FP_LIB_TABLE
{
    friend class PANEL_FP_LIB_TABLE;
    friend class FP_GRID_TRICKS;

protected:
    LIB_TABLE_ROW* at( size_t aIndex ) override { return &m_rows.at( aIndex ); }

    size_t size() const override { return m_rows.size(); }

    LIB_TABLE_ROW* makeNewRow() override
    {
        return dynamic_cast< LIB_TABLE_ROW* >( new FP_LIB_TABLE_ROW );
    }

    LIB_TABLE_ROWS_ITER begin() override { return m_rows.begin(); }

    LIB_TABLE_ROWS_ITER insert( LIB_TABLE_ROWS_ITER aIterator, LIB_TABLE_ROW* aRow ) override
    {
        return m_rows.insert( aIterator, aRow );
    }

    void push_back( LIB_TABLE_ROW* aRow ) override { m_rows.push_back( aRow ); }

    LIB_TABLE_ROWS_ITER erase( LIB_TABLE_ROWS_ITER aFirst, LIB_TABLE_ROWS_ITER aLast ) override
    {
        return m_rows.erase( aFirst, aLast );
    }

public:

    FP_LIB_TABLE_GRID( const FP_LIB_TABLE& aTableToEdit )
    {
        m_rows = aTableToEdit.m_rows;
    }

    void SetValue( int aRow, int aCol, const wxString &aValue ) override
    {
        wxCHECK( aRow < (int) size(), /* void */ );

        LIB_TABLE_GRID::SetValue( aRow, aCol, aValue );

        // If setting a filepath, attempt to auto-detect the format
        if( aCol == COL_URI )
        {
            LIB_TABLE_ROW* row = at( (size_t) aRow );
            wxString       fullURI = row->GetFullURI( true );

            PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::GuessPluginTypeFromLibPath( fullURI );

            if( pluginType == PCB_IO_MGR::FILE_TYPE_NONE )
                pluginType = PCB_IO_MGR::KICAD_SEXP;

            SetValue( aRow, COL_TYPE, PCB_IO_MGR::ShowType( pluginType ) );
        }
    }
};



class FP_GRID_TRICKS : public LIB_TABLE_GRID_TRICKS
{
public:
    FP_GRID_TRICKS( DIALOG_EDIT_LIBRARY_TABLES* aParent, WX_GRID* aGrid ) :
            LIB_TABLE_GRID_TRICKS( aGrid ),
            m_dialog( aParent )
    { }

protected:
    DIALOG_EDIT_LIBRARY_TABLES* m_dialog;

    void optionsEditor( int aRow ) override
    {
        FP_LIB_TABLE_GRID* tbl = (FP_LIB_TABLE_GRID*) m_grid->GetTable();

        if( tbl->GetNumberRows() > aRow )
        {
            LIB_TABLE_ROW*  row = tbl->at( (size_t) aRow );
            const wxString& options = row->GetOptions();
            wxString        result = options;
            STRING_UTF8_MAP choices;

            PCB_IO_MGR::PCB_FILE_T pi_type = PCB_IO_MGR::EnumFromStr( row->GetType() );
            IO_RELEASER<PCB_IO>    pi( PCB_IO_MGR::PluginFind( pi_type ) );
            pi->GetLibraryOptions( &choices );

            DIALOG_PLUGIN_OPTIONS dlg( m_dialog, row->GetNickName(), choices, options, &result );
            dlg.ShowModal();

            if( options != result )
            {
                row->SetOptions( result );
                m_grid->Refresh();
            }
        }
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

            STRING_LINE_READER  slr( TO_UTF8( cb_text ), wxT( "Clipboard" ) );
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
                    tbl->m_rows.replace( i, tmp_tbl.At( i ).clone() );
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


    bool toggleCell( int aRow, int aCol, bool aPreserveSelection ) override
    {
        if( aCol == COL_VISIBLE )
        {
            m_dialog->ShowInfoBarError( _( "Hidden footprint libraries are not yet supported." ) );
            return true;
        }

        return LIB_TABLE_GRID_TRICKS::toggleCell( aRow, aCol, aPreserveSelection );
    }
};


PANEL_FP_LIB_TABLE::PANEL_FP_LIB_TABLE( DIALOG_EDIT_LIBRARY_TABLES* aParent, PROJECT* aProject,
                                        FP_LIB_TABLE* aGlobalTable, const wxString& aGlobalTblPath,
                                        FP_LIB_TABLE* aProjectTable, const wxString& aProjectTblPath,
                                        const wxString& aProjectBasePath ) :
    PANEL_FP_LIB_TABLE_BASE( aParent ),
    m_globalTable( aGlobalTable ),
    m_projectTable( aProjectTable ),
    m_project( aProject ),
    m_projectBasePath( aProjectBasePath ),
    m_parent( aParent )
{
    m_global_grid->SetTable( new FP_LIB_TABLE_GRID( *aGlobalTable ), true );

    // add Cut, Copy, and Paste to wxGrids
    m_path_subs_grid->PushEventHandler( new GRID_TRICKS( m_path_subs_grid ) );

    populatePluginList();

    wxArrayString choices;

    for( auto& [fileType, desc] : m_supportedFpFiles )
        choices.Add( PCB_IO_MGR::ShowType( fileType ) );


    PCBNEW_SETTINGS* cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();

    if( cfg->m_lastFootprintLibDir.IsEmpty() )
        cfg->m_lastFootprintLibDir = PATHS::GetDefaultUserFootprintsPath();

    m_lastProjectLibDir = m_projectBasePath;

    auto autoSizeCol =
            [&]( WX_GRID* aGrid, int aCol )
            {
                int prevWidth = aGrid->GetColSize( aCol );

                aGrid->AutoSizeColumn( aCol, false );
                aGrid->SetColSize( aCol, std::max( prevWidth, aGrid->GetColSize( aCol ) ) );
            };

    auto setupGrid =
            [&]( WX_GRID* aGrid )
            {
                // Give a bit more room for wxChoice editors
                aGrid->SetDefaultRowSize( aGrid->GetDefaultRowSize() + 4 );

                // add Cut, Copy, and Paste to wxGrids
                aGrid->PushEventHandler( new FP_GRID_TRICKS( m_parent, aGrid ) );

                aGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

                wxGridCellAttr* attr;

                attr = new wxGridCellAttr;
                attr->SetEditor( new GRID_CELL_PATH_EDITOR( m_parent, aGrid,
                        &cfg->m_lastFootprintLibDir, true, m_projectBasePath,
                        [this]( WX_GRID* grid, int row ) -> wxString
                        {
                            auto* libTable = static_cast<FP_LIB_TABLE_GRID*>( grid->GetTable() );
                            auto* tableRow = static_cast<FP_LIB_TABLE_ROW*>( libTable->at( row ) );
                            PCB_IO_MGR::PCB_FILE_T fileType = tableRow->GetFileType();
                            const IO_BASE::IO_FILE_DESC& pluginDesc = m_supportedFpFiles.at( fileType );

                            if( pluginDesc.m_IsFile )
                                return pluginDesc.FileFilter();
                            else
                                return wxEmptyString;
                        } ) );
                aGrid->SetColAttr( COL_URI, attr );

                attr = new wxGridCellAttr;
                attr->SetEditor( new wxGridCellChoiceEditor( choices ) );
                aGrid->SetColAttr( COL_TYPE, attr );

                attr = new wxGridCellAttr;
                attr->SetRenderer( new wxGridCellBoolRenderer() );
                attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
                aGrid->SetColAttr( COL_ENABLED, attr );

                attr = new wxGridCellAttr;
                attr->SetRenderer( new wxGridCellBoolRenderer() );
                attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
                aGrid->SetColAttr( COL_VISIBLE, attr );
                // No visibility control for footprint libraries yet; this feature is primarily
                // useful for database libraries and it's only implemented for schematic symbols
                // at the moment.
                aGrid->HideCol( COL_VISIBLE );

                // all but COL_OPTIONS, which is edited with Option Editor anyways.
                autoSizeCol( aGrid, COL_NICKNAME );
                autoSizeCol( aGrid, COL_TYPE );
                autoSizeCol( aGrid, COL_URI );
                autoSizeCol( aGrid, COL_DESCR );

                // Gives a selection to each grid, mainly for delete button. wxGrid's wake up with
                // a currentCell which is sometimes not highlighted.
                if( aGrid->GetNumberRows() > 0 )
                    aGrid->SelectRow( 0 );
            };


    setupGrid( m_global_grid );
    m_global_grid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &PANEL_FP_LIB_TABLE::onGridCellLeftClickHandler, this );

    populateEnvironReadOnlyTable();

    if( aProjectTable )
    {
        m_project_grid->SetTable( new FP_LIB_TABLE_GRID( *aProjectTable ), true );
        setupGrid( m_project_grid );
        m_project_grid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &PANEL_FP_LIB_TABLE::onGridCellLeftClickHandler, this );
    }
    else
    {
        m_pageNdx = 0;
        m_notebook->DeletePage( 1 );
        m_project_grid = nullptr;
    }

    m_path_subs_grid->SetColLabelValue( 0, _( "Name" ) );
    m_path_subs_grid->SetColLabelValue( 1, _( "Value" ) );

    // select the last selected page
    m_notebook->SetSelection( m_pageNdx );
    m_cur_grid = ( m_pageNdx == 0 ) ? m_global_grid : m_project_grid;

    // for ALT+A handling, we want the initial focus to be on the first selected grid.
    m_parent->SetInitialFocus( m_cur_grid );

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

    auto joinExts = []( const std::vector<std::string>& aExts )
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
            entryStr << wxString::Format( wxS( " (%s)" ),
                                          joinExts( desc.m_FileExtensions ) );
        }
        else if( !desc.m_IsFile && !desc.m_ExtensionsInDir.empty() )
        {
            wxString midPart = wxString::Format( _( "folder with %s files" ),
                                                 joinExts( desc.m_ExtensionsInDir ) );

            entryStr << wxString::Format( wxS( " (%s)" ), midPart );
        }

        browseMenu->Append( type, entryStr );

        browseMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_FP_LIB_TABLE::browseLibrariesHandler,
                          this, type );
    }

    Layout();

    // This is the button only press for the browse button instead of the menu
    m_browseButton->Bind( wxEVT_BUTTON, &PANEL_FP_LIB_TABLE::browseLibrariesHandler, this );
}


PANEL_FP_LIB_TABLE::~PANEL_FP_LIB_TABLE()
{
    wxMenu* browseMenu = m_browseButton->GetSplitButtonMenu();
    for( auto& [type, desc] : m_supportedFpFiles )
    {
        browseMenu->Unbind( wxEVT_COMMAND_MENU_SELECTED,
                            &PANEL_FP_LIB_TABLE::browseLibrariesHandler, this, type );
    }
    m_browseButton->Unbind( wxEVT_BUTTON, &PANEL_FP_LIB_TABLE::browseLibrariesHandler, this );

    // Delete the GRID_TRICKS.
    // Any additional event handlers should be popped before the window is deleted.
    m_global_grid->PopEventHandler( true );
    m_global_grid->Unbind( wxEVT_GRID_CELL_LEFT_CLICK, &PANEL_FP_LIB_TABLE::onGridCellLeftClickHandler, this );

    if( m_project_grid )
    {
        m_project_grid->PopEventHandler( true );
        m_project_grid->Unbind( wxEVT_GRID_CELL_LEFT_CLICK, &PANEL_FP_LIB_TABLE::onGridCellLeftClickHandler, this );
    }

    m_path_subs_grid->PopEventHandler( true );
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

    for( FP_LIB_TABLE_GRID* model : { global_model(), project_model() } )
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
    for( FP_LIB_TABLE_GRID* model : { global_model(), project_model() } )
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
                    msg = wxString::Format( _( "Multiple libraries cannot share the same "
                                               "nickname ('%s')." ),
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


void PANEL_FP_LIB_TABLE::OnUpdateUI( wxUpdateUIEvent& event )
{
    m_pageNdx = (unsigned) std::max( 0, m_notebook->GetSelection() );
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
        m_cur_grid->MakeCellVisible( last_row, COL_ENABLED );
        m_cur_grid->SetGridCursor( last_row, COL_NICKNAME );
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
    if( !m_cur_grid->CommitPendingChanges() )
        return;

    FP_LIB_TABLE_GRID* tbl = cur_model();
    int curRow = m_cur_grid->GetGridCursorRow();

    // @todo: add multiple selection moves.
    if( curRow >= 1 )
    {
        boost::ptr_vector< LIB_TABLE_ROW >::auto_type move_me =
            tbl->m_rows.release( tbl->m_rows.begin() + curRow );

        --curRow;
        tbl->m_rows.insert( tbl->m_rows.begin() + curRow, move_me.release() );

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
    if( unsigned( curRow + 1 ) < tbl->m_rows.size() )
    {
        boost::ptr_vector< LIB_TABLE_ROW >::auto_type move_me =
            tbl->m_rows.release( tbl->m_rows.begin() + curRow );

        ++curRow;
        tbl->m_rows.insert( tbl->m_rows.begin() + curRow, move_me.release() );

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

    for( int row : selectedRows )
    {
        if( m_cur_grid->GetCellValue( row, COL_TYPE ) != kicadType )
            rowsToMigrate.push_back( row );
    }

    if( rowsToMigrate.size() <= 0 )
    {
        wxMessageBox( wxString::Format( _( "Select one or more rows containing libraries "
                                           "to save as current KiCad format." ) ) );
        return;
    }
    else
    {
        if( rowsToMigrate.size() == 1 )
        {
            msg.Printf( _( "Save '%s' as current KiCad format "
                           "and replace entry in table?" ),
                        m_cur_grid->GetCellValue( rowsToMigrate[0], COL_NICKNAME ) );
        }
        else
        {
            msg.Printf( _( "Save %d libraries as current KiCad format "
                           "and replace entries in table?" ),
                        (int) rowsToMigrate.size() );
        }

        if( !IsOK( m_parent, msg ) )
            return;
    }

    for( int row : rowsToMigrate )
    {
        wxString   libName = m_cur_grid->GetCellValue( row, COL_NICKNAME );
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

            switch( wxMessageBox( msg, _( "Migrate Library" ),
                                  wxYES_NO | wxCANCEL | wxICON_QUESTION, m_parent ) )
            {
            case wxYES:    break;
            case wxNO:     continue;
            case wxCANCEL: return;
            }
        }

        wxString options = m_cur_grid->GetCellValue( row, COL_OPTIONS );
        std::unique_ptr<STRING_UTF8_MAP> props( LIB_TABLE::ParseOptions( options.ToStdString() ) );

        if( PCB_IO_MGR::ConvertLibrary( props.get(), legacyLib.GetFullPath(), newLib.GetFullPath() ) )
        {
            relPath = NormalizePath( newLib.GetFullPath(), &Pgm().GetLocalEnvVariables(),
                                     m_project );

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
        wxLogWarning( wxT( "File type selection event received but could not find the file type "
                           "in the table" ) );
        return;
    }

    const IO_BASE::IO_FILE_DESC& fileDesc = m_supportedFpFiles.at( fileType );
    PCBNEW_SETTINGS*        cfg = Pgm().GetSettingsManager().GetAppSettings<PCBNEW_SETTINGS>();

    wxString title = wxString::Format( _( "Select %s Library" ), PCB_IO_MGR::ShowType( fileType ) );
    wxString openDir = cfg->m_lastFootprintLibDir;

    if( m_cur_grid == m_project_grid )
        openDir = m_lastProjectLibDir;

    wxArrayString files;

    wxWindow* topLevelParent = wxGetTopLevelParent( this );

    if( fileDesc.m_IsFile )
    {
        wxFileDialog dlg( topLevelParent, title, openDir, wxEmptyString, fileDesc.FileFilter(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE );

        int result = dlg.ShowModal();

        if( result == wxID_CANCEL )
            return;

        dlg.GetPaths( files );

        if( m_cur_grid == m_global_grid )
            cfg->m_lastFootprintLibDir = dlg.GetDirectory();
        else
            m_lastProjectLibDir = dlg.GetDirectory();
    }
    else
    {
        wxDirDialog dlg( topLevelParent, title, openDir,
                         wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST | wxDD_MULTIPLE );

        int result = dlg.ShowModal();

        if( result == wxID_CANCEL )
            return;

        dlg.GetPaths( files );

        if( !files.IsEmpty() )
        {
            wxFileName first( files.front() );

            if( m_cur_grid == m_global_grid )
                cfg->m_lastFootprintLibDir = first.GetPath();
            else
                m_lastProjectLibDir = first.GetPath();
        }
    }

    // Drop the last directory if the path is a .pretty folder
    if( cfg->m_lastFootprintLibDir.EndsWith( FILEEXT::KiCadFootprintLibPathExtension ) )
        cfg->m_lastFootprintLibDir = cfg->m_lastFootprintLibDir.BeforeLast( wxFileName::GetPathSeparator() );

    const ENV_VAR_MAP& envVars       = Pgm().GetLocalEnvVariables();
    bool               addDuplicates = false;
    bool               applyToAll    = false;
    wxString           warning       = _( "Warning: Duplicate Nicknames" );
    wxString           msg           = _( "A library nicknamed '%s' already exists." );
    wxString           detailedMsg   = _( "One of the nicknames will need to be changed after "
                                          "adding this library." );

    for( const wxString& filePath : files )
    {
        wxFileName fn( filePath );
        wxString   nickname = LIB_ID::FixIllegalChars( fn.GetName(), true );
        bool       doAdd    = true;

        if( fileType == PCB_IO_MGR::KICAD_SEXP
            && fn.GetExt() != FILEEXT::KiCadFootprintLibPathExtension )
            nickname = LIB_ID::FixIllegalChars( fn.GetFullName(), true ).wx_str();

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
            wxString path = NormalizePath( filePath, &envVars, m_projectBasePath );

            // Do not use the project path in the global library table.  This will almost
            // assuredly be wrong for a different project.
            if( m_pageNdx == 0 && path.Contains( wxT( "${KIPRJMOD}" ) ) )
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


bool PANEL_FP_LIB_TABLE::TransferDataFromWindow()
{
    if( !m_cur_grid->CommitPendingChanges() )
        return false;

    if( verifyTables() )
    {
        if( *global_model() != *m_globalTable )
        {
            m_parent->m_GlobalTableChanged = true;

            m_globalTable->Clear();
            m_globalTable->TransferRows( global_model()->m_rows );
        }

        if( project_model() && *project_model() != *m_projectTable )
        {
            m_parent->m_ProjectTableChanged = true;

            m_projectTable->Clear();
            m_projectTable->TransferRows( project_model()->m_rows );
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
    m_path_subs_grid->ClearRows();

    for( FP_LIB_TABLE_GRID* tbl : { global_model(), project_model() } )
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
    unique.insert( FP_LIB_TABLE::GlobalPathEnvVariableName() );

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

    // No combobox editors here, but it looks better if its consistent with the other
    // grids in the dialog.
    m_path_subs_grid->SetDefaultRowSize( m_path_subs_grid->GetDefaultRowSize() + 2 );

    adjustPathSubsGridColumns( m_path_subs_grid->GetRect().GetWidth() );
}


void PANEL_FP_LIB_TABLE::onGridCellLeftClickHandler( wxGridEvent& event )
{
    // Get the grid object that triggered the event
    wxGrid* grid = dynamic_cast<wxGrid*>( event.GetEventObject() );

    // If the event object is a wxGrid, proceed with the handling
    if( grid )
    {
        int row = event.GetRow();
        int col = event.GetCol();

        // Get the cell renderer for the clicked cell
        wxGridCellRenderer* renderer = grid->GetCellRenderer( row, col );

        // Check if the renderer is a wxGridCellBoolRenderer using dynamic_cast
        if( dynamic_cast<wxGridCellBoolRenderer*>( renderer ) )
        {
            // Set the grid cursor to the clicked boolean cell
            grid->SetGridCursor( row, col );
        }
    }

    // Allow the default behavior to continue (toggle the bool)
    event.Skip();
}

//-----</event handlers>---------------------------------



size_t   PANEL_FP_LIB_TABLE::m_pageNdx = 0;


void InvokePcbLibTableEditor( KIWAY* aKiway, wxWindow* aCaller )
{
    FP_LIB_TABLE* globalTable = &GFootprintTable;
    wxString      globalTablePath = FP_LIB_TABLE::GetGlobalTableFileName();
    FP_LIB_TABLE* projectTable = PROJECT_PCB::PcbFootprintLibs( &aKiway->Prj() );
    wxString      projectTablePath = aKiway->Prj().FootprintLibTblName();
    wxString      msg;

    DIALOG_EDIT_LIBRARY_TABLES dlg( aCaller, _( "Footprint Libraries" ) );
    dlg.SetKiway( &dlg, aKiway );

    if( aKiway->Prj().IsNullProject() )
        projectTable = nullptr;

    dlg.InstallPanel( new PANEL_FP_LIB_TABLE( &dlg, &aKiway->Prj(), globalTable, globalTablePath,
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

    if( projectTable && dlg.m_ProjectTableChanged )
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

    std::string payload = "";
    aKiway->ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_RELOAD_LIB, payload );
    aKiway->ExpressMail( FRAME_FOOTPRINT_VIEWER, MAIL_RELOAD_LIB, payload );
    aKiway->ExpressMail( FRAME_CVPCB, MAIL_RELOAD_LIB, payload );
}
