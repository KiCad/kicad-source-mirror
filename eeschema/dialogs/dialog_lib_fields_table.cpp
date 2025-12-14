/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialog_lib_fields_table.h>

#include <bitmaps.h>
#include <common.h>
#include <confirm.h>
#include <eda_doc.h>
#include <lib_fields_data_model.h>
#include <grid_tricks.h>
#include <kiface_base.h>
#include <kiplatform/ui.h>
#include <kiway_player.h>
#include <string_utils.h>
#include <symbol_editor/lib_symbol_library_manager.h>
#include <project_sch.h>
#include <symbol_edit_frame.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <widgets/grid_checkbox.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <tools/sch_actions.h>
#include <tool/tool_manager.h>
#include <trace_helpers.h>
#include <fields_data_model.h>
#include <dialog_lib_new_symbol.h>
#include <wx/arrstr.h>
#include <wx/msgdlg.h>
#include <wx/srchctrl.h>

#include <wx/textdlg.h>

using DIALOG_NEW_SYMBOL = DIALOG_LIB_NEW_SYMBOL;

#ifdef __WXMAC__
#define COLUMN_MARGIN 3
#else
#define COLUMN_MARGIN 15
#endif


class VIEW_CONTROLS_GRID_TRICKS : public GRID_TRICKS
{
public:
    VIEW_CONTROLS_GRID_TRICKS( WX_GRID* aGrid ) :
            GRID_TRICKS( aGrid )
    {}

protected:
    void doPopupSelection( wxCommandEvent& event ) override
    {
        if( event.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE )
            m_grid->PostSizeEvent();

        GRID_TRICKS::doPopupSelection( event );
    }
};


enum
{
    MYID_SELECT_FOOTPRINT = GRIDTRICKS_FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET,
    MYID_REVERT_ROW,
    MYID_CLEAR_CELL,
    MYID_CREATE_DERIVED_SYMBOL
};


class LIB_FIELDS_EDITOR_GRID_TRICKS : public GRID_TRICKS
{
public:
    LIB_FIELDS_EDITOR_GRID_TRICKS( DIALOG_LIB_FIELDS_TABLE* aParent, WX_GRID* aGrid,
                                   VIEW_CONTROLS_GRID_DATA_MODEL* aViewFieldsData,
                                   LIB_FIELDS_EDITOR_GRID_DATA_MODEL* aDataModel ) :
            GRID_TRICKS( aGrid ),
            m_dlg( aParent ),
            m_viewControlsDataModel( aViewFieldsData ),
            m_dataModel( aDataModel )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override
    {
        wxMenuItem* revertMenu = menu.Append( MYID_REVERT_ROW, _( "Revert symbol" ), _( "Revert the symbol to its last saved state" ), wxITEM_NORMAL );
        wxMenuItem* clearMenu = menu.Append( MYID_CLEAR_CELL, _( "Clear cell" ), _( "Clear the cell value" ), wxITEM_NORMAL );
        menu.AppendSeparator();
        wxMenuItem* createDerivedSymbolMenu = menu.Append( MYID_CREATE_DERIVED_SYMBOL, _( "Create Derived Symbol" ), _( "Create a new symbol derived from the selected one" ), wxITEM_NORMAL );

        // Get global mouse position and convert to grid client coords
        wxPoint mousePos = wxGetMousePosition();
        wxPoint gridPt = m_grid->ScreenToClient( mousePos );

        // Offset by grid header (column label) height so header area doesn't map to a row.
        int headerHeight = m_grid->GetColLabelSize();
        gridPt.y -= headerHeight;
        if ( gridPt.y < 0 )
            gridPt.y = 0;

        int row = m_grid->YToRow( gridPt.y );
        int col = m_grid->XToCol( gridPt.x );
        m_grid->SetGridCursor( row, col );

        revertMenu->Enable( m_dataModel->IsCellEdited( row, col ) );
        clearMenu->Enable( !m_dataModel->IsCellClear( row, col ) );
        createDerivedSymbolMenu->Enable( m_dataModel->IsRowSingleSymbol( row ) );

        if( m_dataModel->GetColFieldName( col ) == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
        {
            menu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ), _( "Browse for footprint" ) );
            menu.AppendSeparator();
        }
        else if( m_dataModel->GetColFieldName( col ) == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
        {
            menu.Append( MYID_SHOW_DATASHEET, _( "Show Datasheet" ), _( "Show datasheet in browser" ) );
            menu.AppendSeparator();
        }

        GRID_TRICKS::showPopupMenu( menu, aEvent );
    }

    void doPopupSelection( wxCommandEvent& event ) override
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( event.GetId() == MYID_REVERT_ROW )
        {
            if( m_grid->CommitPendingChanges( false ) )
                m_dataModel->RevertRow( row );

            if( m_dataModel->IsEdited() )
                m_dlg->OnModify();
            else
                m_dlg->ClearModify();

            m_grid->ForceRefresh();
        }
        else if( event.GetId() == MYID_CLEAR_CELL )
        {
            if( m_grid->CommitPendingChanges( false ) )
                m_dataModel->ClearCell( row, col );

            if( m_dataModel->IsEdited() )
                m_dlg->OnModify();
            else
                m_dlg->ClearModify();

            m_grid->ForceRefresh();
        }
        else if( event.GetId() == MYID_CREATE_DERIVED_SYMBOL )
        {
            EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( m_dlg->GetParent() );
            wxCHECK( frame, /* void */ );

            const LIB_SYMBOL* parentSymbol = m_dataModel->GetSymbolForRow( row );

            wxArrayString symbolNames;
            m_dataModel->GetSymbolNames( symbolNames );

            auto validator =
                    [&]( const wxString& newName ) -> bool
                    {
                        return symbolNames.Index( newName ) == wxNOT_FOUND;
                    };

            DIALOG_NEW_SYMBOL dlg( frame, symbolNames, parentSymbol->GetName(), validator );

            if( dlg.ShowModal() != wxID_OK )
                return;

            wxString derivedName = dlg.GetName();

            m_dataModel->CreateDerivedSymbolImmediate( row, col, derivedName );

            if( m_dataModel->IsEdited() )
                m_dlg->OnModify();

            m_grid->ForceRefresh();
        }
        else if( event.GetId() == MYID_SELECT_FOOTPRINT )
        {
            // pick a footprint using the footprint picker.
            wxString fpid = m_grid->GetCellValue( row, col );

            if( KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, m_dlg ) )
            {
                if( frame->ShowModal( &fpid, m_dlg ) )
                    m_grid->SetCellValue( row, col, fpid );

                frame->Destroy();
            }
        }
        else if (event.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheet_uri = m_grid->GetCellValue( row, col );
            GetAssociatedDocument( m_dlg, datasheet_uri, &m_dlg->Prj(), PROJECT_SCH::SchSearchS( &m_dlg->Prj() ) );
        }
        else if( event.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE )
        {
            if( !m_grid->CommitPendingChanges( false ) )
                return;

            // Pop-up column order is the order of the shown fields, not the viewControls order
            col = event.GetId() - GRIDTRICKS_FIRST_SHOWHIDE;

            bool show = !m_dataModel->GetShowColumn( col );

            m_dlg->ShowHideColumn( col, show );

            wxString fieldName = m_dataModel->GetColFieldName( col );

            for( row = 0; row < m_viewControlsDataModel->GetNumberRows(); row++ )
            {
                if( m_viewControlsDataModel->GetCanonicalFieldName( row ) == fieldName )
                    m_viewControlsDataModel->SetValueAsBool( row, SHOW_FIELD_COLUMN, show );
            }

            if( m_viewControlsDataModel->GetView() )
                m_viewControlsDataModel->GetView()->ForceRefresh();
        }
        else
        {
            GRID_TRICKS::doPopupSelection( event );
        }
    }

    DIALOG_LIB_FIELDS_TABLE*           m_dlg;
    VIEW_CONTROLS_GRID_DATA_MODEL*     m_viewControlsDataModel;
    LIB_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
};


DIALOG_LIB_FIELDS_TABLE::DIALOG_LIB_FIELDS_TABLE( SYMBOL_EDIT_FRAME* parent, DIALOG_LIB_FIELDS_TABLE::SCOPE aScope ) :
        DIALOG_LIB_FIELDS_TABLE_BASE( parent, wxID_ANY ),
        m_parent( parent ),
        m_scope( aScope ),
        m_viewControlsDataModel( nullptr ),
        m_dataModel( nullptr )
{
    m_bRefresh->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );

    m_addFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_renameFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );

    m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::left ) );

    m_viewControlsDataModel = new VIEW_CONTROLS_GRID_DATA_MODEL( false );

    m_viewControlsGrid->UseNativeColHeader( true );
    m_viewControlsGrid->SetTable( m_viewControlsDataModel, true );

    // must be done after SetTable(), which appears to re-set it
    m_viewControlsGrid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_viewControlsGrid->PushEventHandler( new VIEW_CONTROLS_GRID_TRICKS( m_viewControlsGrid ) );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( true );
    m_viewControlsDataModel->SetColAttr( attr, DISPLAY_NAME_COLUMN );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_viewControlsDataModel->SetColAttr( attr, SHOW_FIELD_COLUMN );

    attr = new wxGridCellAttr;
    attr->SetRenderer( new wxGridCellBoolRenderer() );
    attr->SetReadOnly();    // not really; we delegate interactivity to GRID_TRICKS
    attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_viewControlsDataModel->SetColAttr( attr, GROUP_BY_COLUMN );

    // Compress the view controls grid.  (We want it to look different from the fields grid.)
    m_viewControlsGrid->SetDefaultRowSize( m_viewControlsGrid->GetDefaultRowSize() - FromDIP( 4 ) );

    m_filter->SetDescriptiveText( _( "Filter" ) );

    m_dataModel = new LIB_FIELDS_EDITOR_GRID_DATA_MODEL();

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new LIB_FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_viewControlsDataModel,
                                                                 m_dataModel ) );

    SetInitialFocus( m_grid );
    m_grid->ClearSelection();

    SetupStandardButtons();

    finishDialogSettings();

    SetSize( wxSize( horizPixelsFromDU( 600 ), vertPixelsFromDU( 300 ) ) );

    SYMBOL_EDITOR_SETTINGS::PANEL_LIB_FIELDS_TABLE& cfg = m_parent->libeditconfig()->m_LibFieldEditor;

    m_viewControlsGrid->ShowHideColumns( "0 2 3" );

    CallAfter( [this, cfg]()
               {
                   if( cfg.sidebar_collapsed )
                       m_splitterMainWindow->Unsplit( m_leftPanel );
                   else
                       m_splitterMainWindow->SetSashPosition( cfg.sash_pos );

                   setSideBarButtonLook( cfg.sidebar_collapsed );
               } );

    Center();

    // Connect Events
    m_grid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FIELDS_TABLE::OnColSort, this );
    m_grid->Bind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FIELDS_TABLE::OnColMove, this );
    m_grid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &DIALOG_LIB_FIELDS_TABLE::OnTableCellClick, this );
    m_viewControlsGrid->Bind( wxEVT_GRID_CELL_CHANGED, &DIALOG_LIB_FIELDS_TABLE::OnViewControlsCellChanged, this );
}


DIALOG_LIB_FIELDS_TABLE::~DIALOG_LIB_FIELDS_TABLE()
{
    SYMBOL_EDITOR_SETTINGS::PANEL_LIB_FIELDS_TABLE& cfg = m_parent->libeditconfig()->m_LibFieldEditor;

    if( !cfg.sidebar_collapsed )
        cfg.sash_pos = m_splitterMainWindow->GetSashPosition();

    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( m_dataModel->GetColFieldName( i ).ToUTF8() );
            cfg.field_widths[fieldName] = m_grid->GetColSize( i );
        }
    }

    // Disconnect Events
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FIELDS_TABLE::OnColSort, this );
    m_grid->Unbind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FIELDS_TABLE::OnColMove, this );
    m_grid->Unbind( wxEVT_GRID_CELL_LEFT_CLICK, &DIALOG_LIB_FIELDS_TABLE::OnTableCellClick, this );
    m_viewControlsGrid->Unbind( wxEVT_GRID_CELL_CHANGED, &DIALOG_LIB_FIELDS_TABLE::OnViewControlsCellChanged, this );

    // Delete the GRID_TRICKS.
    m_viewControlsGrid->PopEventHandler( true );
    m_grid->PopEventHandler( true );

    // we gave ownership of m_viewControlsDataModel & m_dataModel to the wxGrids...
}


void DIALOG_LIB_FIELDS_TABLE::setSideBarButtonLook( bool aIsLeftPanelCollapsed )
{
    // Set bitmap and tooltip according to left panel visibility

    if( aIsLeftPanelCollapsed )
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::right ) );
        m_sidebarButton->SetToolTip( _( "Expand left panel" ) );
    }
    else
    {
        m_sidebarButton->SetBitmap( KiBitmapBundle( BITMAPS::left ) );
        m_sidebarButton->SetToolTip( _( "Collapse left panel" ) );
    }
}


void DIALOG_LIB_FIELDS_TABLE::SetupColumnProperties( int aCol )
{
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( false );

    // Set some column types to specific editors
    if( m_dataModel->GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
    {
        // Create symbol netlist for footprint picker
        wxString symbolNetlist;

        if( !m_symbolsList.empty() )
        {
            // Use the first symbol's netlist (all symbols in lib should have similar pin structure)
            LIB_SYMBOL* symbol = m_symbolsList[0];
            wxArrayString pins;

            for( SCH_PIN* pin : symbol->GetGraphicalPins( 0 /* all units */, 1 /* single bodyStyle */ ) )
                pins.push_back( pin->GetNumber() + ' ' + pin->GetShownName() );

            if( !pins.IsEmpty() )
                symbolNetlist << EscapeString( wxJoin( pins, '\t' ), CTX_LINE );

            symbolNetlist << wxS( "\r" );

            wxArrayString fpFilters = symbol->GetFPFilters();
            if( !fpFilters.IsEmpty() )
                symbolNetlist << EscapeString( wxJoin( fpFilters, ' ' ), CTX_LINE );

            symbolNetlist << wxS( "\r" );
        }

        attr->SetEditor( new GRID_CELL_FPID_EDITOR( this, symbolNetlist ) );
        m_dataModel->SetColAttr( attr, aCol );
    }
    else if( m_dataModel->GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
    {
        // set datasheet column viewer button
        attr->SetEditor( new GRID_CELL_URL_EDITOR( this, PROJECT_SCH::SchSearchS( &Prj() ) ) );
        m_dataModel->SetColAttr( attr, aCol );
    }
    else if( m_dataModel->ColIsCheck( aCol ) )
    {
        attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
        attr->SetRenderer( new GRID_CELL_CHECKBOX_RENDERER() );
        m_dataModel->SetColAttr( attr, aCol );
    }
    else if( IsGeneratedField( m_dataModel->GetColFieldName( aCol ) ) )
    {
        attr->SetReadOnly();
        m_dataModel->SetColAttr( attr, aCol );
    }
    else
    {
        attr->SetRenderer( new GRID_CELL_TEXT_RENDERER() );
        attr->SetEditor( m_grid->GetDefaultEditor() );
        m_dataModel->SetColAttr( attr, aCol );
    }
}


void DIALOG_LIB_FIELDS_TABLE::SetupAllColumnProperties()
{
    SYMBOL_EDITOR_SETTINGS* cfg = m_parent->GetSettings();
    wxSize defaultDlgSize = ConvertDialogToPixels( wxSize( 600, 300 ) );

    // Restore column sorting order and widths
    m_grid->AutoSizeColumns( false );
    int  sortCol = 0;
    bool sortAscending = true;

    // Find the symbol name column for initial sorting
    int nameCol = m_dataModel->GetFieldNameCol( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME );

    // Set initial sort to VALUE field (ascending) if no previous sort preference exists
    if( m_dataModel->GetSortCol() == 0 && nameCol != -1 )
    {
        sortCol = nameCol;
        sortAscending = true;
        m_dataModel->SetSorting( sortCol, sortAscending );
    }

    for( int col = 0; col < m_grid->GetNumberCols(); ++col )
    {
        SetupColumnProperties( col );

        if( col == m_dataModel->GetSortCol() )
        {
            sortCol = col;
            sortAscending = m_dataModel->GetSortAsc();
        }
    }

    // sync m_grid's column visibilities to Show checkboxes in m_viewControlsGrid
    for( int i = 0; i < m_viewControlsDataModel->GetNumberRows(); ++i )
    {
        int col = m_dataModel->GetFieldNameCol( m_viewControlsDataModel->GetCanonicalFieldName( i ) );

        if( col == -1 )
            continue;

        bool show = m_viewControlsDataModel->GetValueAsBool( i, SHOW_FIELD_COLUMN );
        m_dataModel->SetShowColumn( col, show );

        if( show )
        {
            m_grid->ShowCol( col );

            std::string key( m_dataModel->GetColFieldName( col ).ToUTF8() );

            if( cfg->m_LibFieldEditor.field_widths.count( key )
                && ( cfg->m_LibFieldEditor.field_widths.at( key ) > 0 ) )
            {
                m_grid->SetColSize( col, cfg->m_LibFieldEditor.field_widths.at( key ) );
            }
            else
            {
                int textWidth = m_dataModel->GetDataWidth( col ) + COLUMN_MARGIN;
                int maxWidth = defaultDlgSize.x / 3;

                m_grid->SetColSize( col, std::clamp( textWidth, 100, maxWidth ) );
            }
        }
        else
        {
            m_grid->HideCol( col );
        }
    }

    m_dataModel->SetSorting( sortCol, sortAscending );
    m_grid->SetSortingColumn( sortCol, sortAscending );
}


bool DIALOG_LIB_FIELDS_TABLE::TransferDataToWindow()
{
    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );

    switch( m_scope )
    {
    case SCOPE::SCOPE_LIBRARY:         m_choiceScope->SetSelection( 0 ); break;
    case SCOPE::SCOPE_RELATED_SYMBOLS: m_choiceScope->SetSelection( 1 ); break;
    }

    setScope( m_scope );

    return true;
}


void DIALOG_LIB_FIELDS_TABLE::loadSymbols( const wxArrayString& aSymbolNames )
{
    // Clear any existing data
    m_symbolsList.clear();

    wxString libName = m_parent->GetTreeLIBID().GetLibNickname();

    if( aSymbolNames.IsEmpty() )
    {
        if( m_scope == SCOPE_RELATED_SYMBOLS )
            wxMessageBox( wxString::Format( _( "No related symbols found in library '%s'." ), libName ) );
        else
            wxMessageBox( wxString::Format( _( "No symbols found in library '%s'." ), libName ) );

        return;
    }

    // Load each symbol from the library manager and add it to our list
    for( const wxString& symbolName : aSymbolNames )
    {
        LIB_SYMBOL* canvasSymbol = m_parent->GetCurSymbol();

        if( canvasSymbol && canvasSymbol->GetLibraryName() == libName && canvasSymbol->GetName() == symbolName )
        {
            m_symbolsList.push_back( canvasSymbol );
        }
        else
        {
            try
            {
                if( LIB_SYMBOL* symbol = m_parent->GetLibManager().GetSymbol( symbolName, libName ) )
                    m_symbolsList.push_back( symbol );
            }
            catch( const IO_ERROR& ioe )
            {
                // Log the error and continue
                wxLogWarning( wxString::Format( _( "Error loading symbol '%s': %s" ), symbolName, ioe.What() ) );
            }
        }
    }

    if( m_symbolsList.empty() )
    {
        if( m_scope == SCOPE_RELATED_SYMBOLS )
            wxMessageBox( _( "No related symbols could be loaded from the library." ) );
        else
            wxMessageBox( _( "No symbols could be loaded from the library." ) );
    }

    m_dataModel->SetSymbols( m_symbolsList );
}


bool DIALOG_LIB_FIELDS_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    bool updateCanvas = false;

    m_dataModel->ApplyData(
            [&]( LIB_SYMBOL* symbol )
            {
                m_parent->GetLibManager().UpdateSymbol( symbol, symbol->GetLibNickname() );

                if( m_parent->GetCurSymbol() == symbol )
                    updateCanvas = true;
            },
            [&]()
            {
                // Handle newly created derived symbols
                auto createdSymbols = m_dataModel->GetAndClearCreatedDerivedSymbols();

                wxLogTrace( traceLibFieldTable, "Post-apply handler: found %zu created derived symbols",
                            createdSymbols.size() );

                for( const auto& [symbol, libraryName] : createdSymbols )
                {
                    if( !libraryName.IsEmpty() )
                    {
                        wxLogTrace( traceLibFieldTable, "Updating symbol '%s' (UUID: %s) in library '%s'",
                                    symbol->GetName(), symbol->m_Uuid.AsString(), libraryName );
                        // Update the symbol in the library manager to properly register it
                        m_parent->GetLibManager().UpdateSymbol( symbol, libraryName );
                    }
                }

                // Sync libraries and refresh tree if there were changes
                if( !createdSymbols.empty() )
                {
                    wxLogTrace( traceLibFieldTable, "Syncing libraries due to %zu new symbols", createdSymbols.size() );

                    // Store references to the created symbols before sync
                    std::vector<LIB_SYMBOL*> symbolsToPreserve;
                    for( const auto& [symbol, libraryName] : createdSymbols )
                    {
                        symbolsToPreserve.push_back( symbol );
                    }

                    // Synchronize libraries to update the tree view
                    m_parent->SyncLibraries( false );

                    // Ensure created symbols are still in the symbol list after sync
                    for( LIB_SYMBOL* symbol : symbolsToPreserve )
                    {
                        bool found = false;
                        for( LIB_SYMBOL* existingSymbol : m_symbolsList )
                        {
                            if( existingSymbol->m_Uuid == symbol->m_Uuid )
                            {
                                found = true;
                                break;
                            }
                        }
                        if( !found )
                        {
                            wxLogTrace( traceLibFieldTable, "Re-adding symbol '%s' to list after sync", symbol->GetName() );
                            m_symbolsList.push_back( symbol );
                        }
                    }
                }

                wxLogTrace( traceLibFieldTable, "Dialog symbol list size after processing: %zu", m_symbolsList.size() );
            } );

    ClearModify();

    wxLogTrace( traceLibFieldTable, "About to rebuild grid rows to include new symbols" );
    RegroupSymbols();
    wxLogTrace( traceLibFieldTable, "Grid rebuild completed" );

    m_parent->RefreshLibraryTree();

    if( updateCanvas )
    {
        m_parent->OnModify();
        m_parent->HardRedraw();
    }

    return true;
}


void DIALOG_LIB_FIELDS_TABLE::OnAddField(wxCommandEvent& event)
{
    wxTextEntryDialog dlg( this, _( "New field name:" ), _( "Add Field" ) );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString fieldName = dlg.GetValue();

    if( fieldName.IsEmpty() )
    {
        DisplayError( this, _( "Field must have a name." ) );
        return;
    }

    for( int i = 0; i < m_dataModel->GetNumberCols(); ++i )
    {
        if( fieldName == m_dataModel->GetColFieldName( i ) )
        {
            DisplayError( this, wxString::Format( _( "Field name '%s' already in use." ), fieldName ) );
            return;
        }
    }

    AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false, true );

    SetupColumnProperties( m_dataModel->GetColsCount() - 1 );

    OnModify();
}


void DIALOG_LIB_FIELDS_TABLE::OnRemoveField(wxCommandEvent& event)
{
    m_viewControlsGrid->OnDeleteRows(
            [&]( int row )
            {
                return IsOK( this, wxString::Format( _( "Are you sure you want to remove the field '%s'?" ),
                                                     m_viewControlsDataModel->GetValue( row, DISPLAY_NAME_COLUMN ) ) );
            },
            [&]( int row )
            {
                wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );

                RemoveField( fieldName );

                m_viewControlsDataModel->DeleteRow( row );
                OnModify();
            } );
}


void DIALOG_LIB_FIELDS_TABLE::OnRenameField(wxCommandEvent& event)
{
    wxArrayInt selectedRows = m_viewControlsGrid->GetSelectedRows();

    if( selectedRows.empty() && m_viewControlsGrid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_viewControlsGrid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    int row = selectedRows[0];

    wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );

    int col = m_dataModel->GetFieldNameCol( fieldName );
    wxCHECK_RET( col != -1, wxS( "Existing field name missing from data model" ) );

    wxTextEntryDialog dlg( this, _( "New field name:" ), _( "Rename Field" ), fieldName );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxString newFieldName = dlg.GetValue();

    if( newFieldName == fieldName )
        return;

    if( m_dataModel->GetFieldNameCol( newFieldName ) != -1 )
    {
        DisplayError( this, wxString::Format( _( "Field name '%s' already exists." ), newFieldName ) );
        return;
    }

    RenameField( fieldName, newFieldName );

    m_viewControlsDataModel->SetCanonicalFieldName( row, newFieldName );
    m_viewControlsDataModel->SetValue( row, DISPLAY_NAME_COLUMN, newFieldName );
    m_viewControlsDataModel->SetValue( row, LABEL_COLUMN, GetGeneratedFieldDisplayName( newFieldName ) );

    SetupColumnProperties( col );
    OnModify();
}


void DIALOG_LIB_FIELDS_TABLE::OnFilterText( wxCommandEvent& event )
{
    m_dataModel->SetFilter( m_filter->GetValue() );
    RegroupSymbols();
}


void DIALOG_LIB_FIELDS_TABLE::OnFilterMouseMoved(wxMouseEvent& aEvent)
{
#if defined( __WXOSX__ ) // Doesn't work properly on other ports
    wxPoint pos = aEvent.GetPosition();
    wxRect  ctrlRect = m_filter->GetScreenRect();
    int     buttonWidth = ctrlRect.GetHeight();         // Presume buttons are square

    // TODO: restore cursor when mouse leaves the filter field (or is it a MSW bug?)
    if( m_filter->IsSearchButtonVisible() && pos.x < buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else if( m_filter->IsCancelButtonVisible() && pos.x > ctrlRect.GetWidth() - buttonWidth )
        SetCursor( wxCURSOR_ARROW );
    else
        SetCursor( wxCURSOR_IBEAM );
#endif
}


void DIALOG_LIB_FIELDS_TABLE::setScope( SCOPE aScope )
{
    LIB_SYMBOL_LIBRARY_MANAGER& libMgr = m_parent->GetLibManager();
    wxString                    targetLib = m_parent->GetTargetLibId().GetLibNickname();
    wxString                    targetSymbol = m_parent->GetTargetLibId().GetLibItemName();
    wxArrayString               symbolNames;

    SetTitle( wxString::Format( _( "Symbol Fields Table ('%s' Library)" ), targetLib ) );

    m_scope = aScope;

    if( m_scope == SCOPE::SCOPE_RELATED_SYMBOLS )
    {
        const LIB_SYMBOL*           symbol = libMgr.GetBufferedSymbol( targetSymbol, targetLib );
        std::shared_ptr<LIB_SYMBOL> root = symbol ? symbol->GetRootSymbol() : nullptr;

        if( root )
        {
            symbolNames.Add( root->GetName() );

            // Now we have the root symbol, collect all its derived symbols
            libMgr.GetDerivedSymbolNames( root->GetName(), targetLib, symbolNames );
        }
    }
    else
    {
        // Get all symbol names from the library manager
        libMgr.GetSymbolNames( targetLib, symbolNames );
    }

    // Get all symbols from the library
    loadSymbols( symbolNames );

    // Update the field list and refresh the grid
    UpdateFieldList();
    SetupAllColumnProperties();
    RegroupSymbols();
}


void DIALOG_LIB_FIELDS_TABLE::OnScope( wxCommandEvent& aEvent )
{
    switch( aEvent.GetSelection() )
    {
    case 0: setScope( SCOPE::SCOPE_LIBRARY );         break;
    case 1: setScope( SCOPE::SCOPE_RELATED_SYMBOLS ); break;
    }
}


void DIALOG_LIB_FIELDS_TABLE::OnRegroupSymbols( wxCommandEvent& event )
{
    RegroupSymbols();
}


void DIALOG_LIB_FIELDS_TABLE::OnTableValueChanged( wxGridEvent& event )
{
    m_grid->ForceRefresh();
    OnModify();
}


void DIALOG_LIB_FIELDS_TABLE::OnTableCellClick( wxGridEvent& event )
{
    if( m_dataModel->IsExpanderColumn( event.GetCol() ) )
    {
        m_grid->ClearSelection();
        m_dataModel->ExpandCollapseRow( event.GetRow() );
        m_grid->SetGridCursor( event.GetRow(), event.GetCol() );
        // Don't call event.Skip() - we've handled this event
    }
    else
    {
        event.Skip(); // Let normal cell editing proceed
    }
}


void DIALOG_LIB_FIELDS_TABLE::OnTableItemContextMenu( wxGridEvent& event )
{
    if( m_grid )
    {
        // Get global mouse position and convert to grid client coords
        wxPoint mousePos = wxGetMousePosition();
        wxPoint gridPt = m_grid->ScreenToClient( mousePos );

        int row = m_grid->YToRow( gridPt.y );
        int col = m_grid->XToCol( gridPt.x );

        if ( row != -1 && col != -1 )
            m_grid->SetGridCursor( row, col );
    }

    event.Skip();
}


void DIALOG_LIB_FIELDS_TABLE::OnGroupSymbolsToggled( wxCommandEvent& event )
{
    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();
}


void DIALOG_LIB_FIELDS_TABLE::OnTableColSize(wxGridSizeEvent& aEvent)
{
    aEvent.Skip();

    m_grid->ForceRefresh();
}


void DIALOG_LIB_FIELDS_TABLE::OnSizeViewControlsGrid( wxSizeEvent& event )
{
    const wxString& showColLabel = m_viewControlsGrid->GetColLabelValue( SHOW_FIELD_COLUMN );
    const wxString& groupByColLabel = m_viewControlsGrid->GetColLabelValue( GROUP_BY_COLUMN );
    int             showColWidth = KIUI::GetTextSize( showColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             groupByColWidth = KIUI::GetTextSize( groupByColLabel, m_viewControlsGrid ).x + COLUMN_MARGIN;
    int             remainingWidth = m_viewControlsGrid->GetSize().GetX() - showColWidth - groupByColWidth;

    m_viewControlsGrid->SetColSize( showColWidth, SHOW_FIELD_COLUMN );
    m_viewControlsGrid->SetColSize( groupByColWidth, GROUP_BY_COLUMN );

    if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) && m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth / 2, 60 ) );
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth - ( remainingWidth / 2 ), 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( DISPLAY_NAME_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( DISPLAY_NAME_COLUMN, std::max( remainingWidth, 60 ) );
    }
    else if( m_viewControlsGrid->IsColShown( LABEL_COLUMN ) )
    {
        m_viewControlsGrid->SetColSize( LABEL_COLUMN, std::max( remainingWidth, 60 ) );
    }

    event.Skip();
}


void DIALOG_LIB_FIELDS_TABLE::OnSidebarToggle( wxCommandEvent& event )
{
    SYMBOL_EDITOR_SETTINGS::PANEL_LIB_FIELDS_TABLE& cfg = m_parent->libeditconfig()->m_LibFieldEditor;

    if( cfg.sidebar_collapsed )
    {
        cfg.sidebar_collapsed = false;
        m_splitterMainWindow->SplitVertically( m_leftPanel, m_rightPanel, cfg.sash_pos );
    }
    else
    {
        cfg.sash_pos = m_splitterMainWindow->GetSashPosition();
        cfg.sidebar_collapsed = true;
        m_splitterMainWindow->Unsplit( m_leftPanel );
    }

    setSideBarButtonLook( cfg.sidebar_collapsed );
}


void DIALOG_LIB_FIELDS_TABLE::OnApply(wxCommandEvent& event)
{
    TransferDataFromWindow();
}


void DIALOG_LIB_FIELDS_TABLE::OnCancel(wxCommandEvent& event)
{
    m_grid->CommitPendingChanges( true );

    if( m_dataModel->IsEdited() )
    {
        if( !HandleUnsavedChanges( this, _( "Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return TransferDataFromWindow();
                                   } ) )
            return;
    }

    EndModal( wxID_CANCEL );
}


void DIALOG_LIB_FIELDS_TABLE::OnOk(wxCommandEvent& event)
{
    if( TransferDataFromWindow() )
        EndModal( wxID_OK );
}


void DIALOG_LIB_FIELDS_TABLE::OnClose( wxCloseEvent& aEvent )
{
    m_grid->CommitPendingChanges( true );

    if( m_dataModel->IsEdited() )
    {
        if( !HandleUnsavedChanges( this, _( "Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return TransferDataFromWindow();
                                   } ) )
        {
            aEvent.Veto();
            return;
        }
    }

    aEvent.Skip();
}


void DIALOG_LIB_FIELDS_TABLE::UpdateFieldList()
{
    auto addMandatoryField =
            [&]( FIELD_T fieldId, bool show, bool groupBy )
            {
                AddField( GetCanonicalFieldName( fieldId ),
                            GetDefaultFieldName( fieldId, DO_TRANSLATE ), show, groupBy );
            };

    AddField( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME, _( "Symbol Name" ), true, false );

    // Add mandatory fields first            show   groupBy
    addMandatoryField( FIELD_T::REFERENCE,   false,  false   );
    addMandatoryField( FIELD_T::VALUE,       true,   false   );
    addMandatoryField( FIELD_T::FOOTPRINT,   true,   false   );
    addMandatoryField( FIELD_T::DATASHEET,   true,   false  );
    addMandatoryField( FIELD_T::DESCRIPTION, false,  false  );

    AddField( wxS( "Keywords" ), _( "Keywords" ), true, false );

    // Add attribute fields as checkboxes                                     show  groupBy  user   checkbox
    AddField( wxS( "${EXCLUDE_FROM_BOM}" ),   _( "Exclude From BOM" ),        true,  false,  false,  true );
    AddField( wxS( "${EXCLUDE_FROM_SIM}" ),   _( "Exclude From Simulation" ), true,  false,  false,  true );
    AddField( wxS( "${EXCLUDE_FROM_BOARD}" ), _( "Exclude From Board" ),      true,  false,  false,  true );

    AddField( wxS( "Power" ),      _( "Power Symbol" ),       true, false, false, true );
    AddField( wxS( "LocalPower" ), _( "Local Power Symbol" ), true, false, false, true );

    // User fields next
    std::set<wxString> userFieldNames;

    for( LIB_SYMBOL* symbol : m_symbolsList )
    {
        std::vector< SCH_FIELD* > fields;
        symbol->GetFields( fields );

        for( SCH_FIELD* field : fields )
        {
            if( !field->IsMandatory() && !field->IsPrivate() )
                userFieldNames.insert( field->GetName() );
        }
    }

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false );
}


void DIALOG_LIB_FIELDS_TABLE::AddField( const wxString& aFieldName, const wxString& aLabelValue, bool show,
                                        bool groupBy, bool addedByUser, bool aIsCheckbox )
{
    // Users can add fields with variable names that match the special names in the grid,
    // e.g. ${QUANTITY} so make sure we don't add them twice
    for( int row = 0; row < m_viewControlsDataModel->GetNumberRows(); row++ )
    {
        if( m_viewControlsDataModel->GetCanonicalFieldName( row ) == aFieldName )
            return;
    }

    m_dataModel->AddColumn( aFieldName, aLabelValue, addedByUser, aIsCheckbox );

    wxGridTableMessage msg( m_dataModel, wxGRIDTABLE_NOTIFY_COLS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_viewControlsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_viewControlsDataModel->AppendRow( aFieldName, aLabelValue, show, groupBy );

                return { m_viewControlsDataModel->GetNumberRows() - 1, -1 };
            } );
}


void DIALOG_LIB_FIELDS_TABLE::RemoveField(const wxString& fieldName)
{
    int col = m_dataModel->GetFieldNameCol( fieldName );
    wxCHECK_RET( col != -1, wxS( "Field name not found" ) );

    m_dataModel->RemoveColumn( col );
}


void DIALOG_LIB_FIELDS_TABLE::RenameField(const wxString& oldName, const wxString& newName)
{
    int col = m_dataModel->GetFieldNameCol( oldName );
    wxCHECK_RET( col != -1, wxS( "Existing field name missing from data model" ) );

    m_dataModel->RenameColumn( col, newName );
}


void DIALOG_LIB_FIELDS_TABLE::RegroupSymbols()
{
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();
}


void DIALOG_LIB_FIELDS_TABLE::OnColSort(wxGridEvent& aEvent)
{
    int         sortCol = aEvent.GetCol();
    std::string key( m_dataModel->GetColFieldName( sortCol ).ToUTF8() );
    bool        ascending;

    // This is bonkers, but wxWidgets doesn't tell us ascending/descending in the event, and
    // if we ask it will give us pre-event info.
    if( m_grid->IsSortingBy( sortCol ) )
    {
        // same column; invert ascending
        ascending = !m_grid->IsSortOrderAscending();
    }
    else
    {
        // different column; start with ascending
        ascending = true;
    }

    m_dataModel->SetSorting( sortCol, ascending );
    RegroupSymbols();
}


void DIALOG_LIB_FIELDS_TABLE::OnColMove(wxGridEvent& aEvent)
{
    int origPos = aEvent.GetCol();

    // Save column widths since the setup function uses the saved config values
    SYMBOL_EDITOR_SETTINGS* cfg = m_parent->GetSettings();

    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( m_dataModel->GetColFieldName( i ).ToUTF8() );
            cfg->m_LibFieldEditor.field_widths[fieldName] = m_grid->GetColSize( i );
        }
    }

    CallAfter(
            [origPos, this]()
            {
                int newPos = m_grid->GetColPos( origPos );

#ifdef __WXMAC__
                if( newPos < origPos )
                    newPos += 1;
#endif

                m_dataModel->MoveColumn( origPos, newPos );

                // "Unmove" the column since we've moved the column internally
                m_grid->ResetColPos();

                // We need to reset all the column attr's to the correct column order
                SetupAllColumnProperties();

                m_grid->ForceRefresh();
            } );
}


void DIALOG_LIB_FIELDS_TABLE::ShowHideColumn( int aCol, bool aShow )
{
    if( aShow )
        m_grid->ShowCol( aCol );
    else
        m_grid->HideCol( aCol );

    m_dataModel->SetShowColumn( aCol, aShow );

    m_grid->ForceRefresh();
    OnModify();
}


void DIALOG_LIB_FIELDS_TABLE::OnViewControlsCellChanged( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();

    wxCHECK( row < m_viewControlsGrid->GetNumberRows(), /* void */ );

    switch( aEvent.GetCol() )
    {
    case DISPLAY_NAME_COLUMN:
    {
        wxString label = m_viewControlsDataModel->GetValue( row, DISPLAY_NAME_COLUMN );
        wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
        int      dataCol = m_dataModel->GetFieldNameCol( fieldName );

        if( dataCol != -1 )
        {
            m_dataModel->SetColLabelValue( dataCol, label );
            m_grid->SetColLabelValue( dataCol, label );

            m_grid->ForceRefresh();
            OnModify();
        }

        break;
    }

    case SHOW_FIELD_COLUMN:
    {
        wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
        bool     value = m_viewControlsDataModel->GetValueAsBool( row, SHOW_FIELD_COLUMN );
        int      dataCol = m_dataModel->GetFieldNameCol( fieldName );

        if( dataCol != -1 )
            ShowHideColumn( dataCol, value );

        break;
    }

    case GROUP_BY_COLUMN:
    {
        wxString fieldName = m_viewControlsDataModel->GetCanonicalFieldName( row );
        bool     value = m_viewControlsDataModel->GetValueAsBool( row, GROUP_BY_COLUMN );
        int      dataCol = m_dataModel->GetFieldNameCol( fieldName );

        m_dataModel->SetGroupColumn( dataCol, value );
        m_dataModel->RebuildRows();

        m_grid->ForceRefresh();
        OnModify();
        break;
    }

    default:
        break;
    }
}


