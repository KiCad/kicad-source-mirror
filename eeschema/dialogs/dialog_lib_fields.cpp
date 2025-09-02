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

#include <dialog_lib_fields.h>

#include <bitmaps.h>
#include <common.h>
#include <confirm.h>
#include <eda_doc.h>
#include <lib_fields_data_model.h>
#include <grid_tricks.h>
#include <kiface_base.h>
#include <kiplatform/ui.h>
#include <kiway_player.h>
#include <symbol_editor/lib_symbol_library_manager.h>
#include <project_sch.h>
#include <symbol_edit_frame.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <widgets/grid_checkbox.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <tools/sch_actions.h>
#include <tool/tool_manager.h>
#include <trace_helpers.h>

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
    LIB_FIELDS_EDITOR_GRID_TRICKS( DIALOG_SHIM*                       aParent,
                                   WX_GRID*                           aGrid,
                                   wxDataViewListCtrl*                aFieldsCtrl,
                                   LIB_FIELDS_EDITOR_GRID_DATA_MODEL* aDataModel ) :
            GRID_TRICKS( aGrid ),
            m_dlg( aParent ),
            m_fieldsCtrl( aFieldsCtrl ),
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
            menu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ),
                         _( "Browse for footprint" ) );
            menu.AppendSeparator();
        }
        else if( m_dataModel->GetColFieldName( col ) == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
        {
            menu.Append( MYID_SHOW_DATASHEET, _( "Show Datasheet" ),
                         _( "Show datasheet in browser" ) );
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
            const LIB_SYMBOL* parentSymbol = m_dataModel->GetSymbolForRow( row );

            wxArrayString symbolNames;
            m_dataModel->GetSymbolNames( symbolNames );

            auto validator = [&]( wxString newName ) -> bool
            {
                return symbolNames.Index( newName ) == wxNOT_FOUND;
            };

            EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( m_dlg->GetParent() );
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

            if( KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true,
                                                             m_dlg ) )
            {
                if( frame->ShowModal( &fpid, m_dlg ) )
                    m_grid->SetCellValue( row, col, fpid );

                frame->Destroy();
            }
        }
        else if (event.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheet_uri = m_grid->GetCellValue( row, col );
            GetAssociatedDocument( m_dlg, datasheet_uri, &m_dlg->Prj(),
                                   PROJECT_SCH::SchSearchS( &m_dlg->Prj() ) );
        }
        else
        {
            // We have grid tricks events to show/hide the columns from the popup menu
            // and we need to make sure the data model is updated to match the grid,
            // so do it through our code instead
            if( event.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE )
            {
                // Pop-up column order is the order of the shown fields, not the
                // fieldsCtrl order
                col = event.GetId() - GRIDTRICKS_FIRST_SHOWHIDE;

                bool show = !m_dataModel->GetShowColumn( col );

                // Convert data model column to by iterating over m_fieldsCtrl rows
                // and finding the matching field name
                wxString fieldName = m_dataModel->GetColFieldName( col );

                for( row = 0; row < m_fieldsCtrl->GetItemCount(); row++ )
                {
                    if( m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN ) == fieldName )
                    {
                        if( m_grid->CommitPendingChanges( false ) )
                            m_fieldsCtrl->SetToggleValue( show, row, SHOW_FIELD_COLUMN );

                        break;
                    }
                }
            }
            else
            {
                GRID_TRICKS::doPopupSelection( event );
            }
        }
    }

    DIALOG_SHIM*                       m_dlg;
    wxDataViewListCtrl*                m_fieldsCtrl;
    LIB_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
};


DIALOG_LIB_FIELDS::DIALOG_LIB_FIELDS( SYMBOL_EDIT_FRAME* parent, wxString libId, const wxArrayString& aSymbolNames ) :
        DIALOG_LIB_FIELDS_BASE( parent, wxID_ANY, wxString::Format( _( "Symbol Library Fields: %s" ), libId ) ),
        m_libId( libId ),
        m_parent( parent )
{
    // Get all symbols from the library
    loadSymbols( aSymbolNames );

    m_bRefresh->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );

    m_addFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_renameFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );

    m_removeFieldButton->Enable( false );
    m_renameFieldButton->Enable( false );

    m_fieldsCtrl->AppendTextColumn( _( "Field" ), wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, wxCOL_HIDDEN );
    m_fieldsCtrl->AppendTextColumn( _( "Label" ), wxDATAVIEW_CELL_EDITABLE, 0, wxALIGN_LEFT, 0 );
    m_fieldsCtrl->AppendToggleColumn( _( "Show" ), wxDATAVIEW_CELL_ACTIVATABLE, 0,
                                      wxALIGN_CENTER, 0 );
    m_fieldsCtrl->AppendToggleColumn( _( "Group By" ), wxDATAVIEW_CELL_ACTIVATABLE, 0,
                                      wxALIGN_CENTER, 0 );

    // GTK asserts if the number of columns doesn't match the data, but we still don't want
    // to display the canonical names.  So we'll insert a column for them, but keep it 0 width.
    m_fieldsCtrl->AppendTextColumn( _( "Name" ), wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );

    // SetWidth( wxCOL_WIDTH_AUTOSIZE ) fails here on GTK, so we calculate the title sizes and
    // set the column widths ourselves.
    wxDataViewColumn* column = m_fieldsCtrl->GetColumn( SHOW_FIELD_COLUMN );
    m_showColWidth = KIUI::GetTextSize( column->GetTitle(), m_fieldsCtrl ).x + COLUMN_MARGIN;
    column->SetMinWidth( m_showColWidth );

    column = m_fieldsCtrl->GetColumn( GROUP_BY_COLUMN );
    m_groupByColWidth = KIUI::GetTextSize( column->GetTitle(), m_fieldsCtrl ).x + COLUMN_MARGIN;
    column->SetMinWidth( m_groupByColWidth );

    // The fact that we're a list should keep the control from reserving space for the
    // expander buttons... but it doesn't.  Fix by forcing the indent to 0.
    m_fieldsCtrl->SetIndent( 0 );

    m_filter->SetDescriptiveText( _( "Filter" ) );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_URL_EDITOR( this, PROJECT_SCH::SchSearchS( &Prj() ) ) );
    m_dataModel = new LIB_FIELDS_EDITOR_GRID_DATA_MODEL( m_symbolsList );

    // Now that the fields are loaded we can set the initial location of the splitter
    // based on the list width.  Again, SetWidth( wxCOL_WIDTH_AUTOSIZE ) fails us on GTK.
    m_fieldNameColWidth = 0;
    m_labelColWidth = 0;

    int colWidth = 0;

    for( int row = 0; row < m_fieldsCtrl->GetItemCount(); ++row )
    {
        const wxString& displayName = m_fieldsCtrl->GetTextValue( row, DISPLAY_NAME_COLUMN );
        colWidth = std::max( colWidth, KIUI::GetTextSize( displayName, m_fieldsCtrl ).x );

        const wxString& label = m_fieldsCtrl->GetTextValue( row, LABEL_COLUMN );
        colWidth = std::max( colWidth, KIUI::GetTextSize( label, m_fieldsCtrl ).x );
    }

    m_fieldNameColWidth = colWidth + 20;
    m_labelColWidth = colWidth + 20;

    int fieldsMinWidth = m_fieldNameColWidth + m_labelColWidth + m_groupByColWidth + m_showColWidth;

    m_fieldsCtrl->GetColumn( DISPLAY_NAME_COLUMN )->SetWidth( m_fieldNameColWidth );
    m_fieldsCtrl->GetColumn( LABEL_COLUMN )->SetWidth( m_labelColWidth );

    // This is used for data only.  Don't show it to the user.
    m_fieldsCtrl->GetColumn( FIELD_NAME_COLUMN )->SetHidden( true );

    m_splitterMainWindow->SetMinimumPaneSize( fieldsMinWidth );
    m_splitterMainWindow->SetSashPosition( fieldsMinWidth + 40 );

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new LIB_FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_fieldsCtrl,
                                                             m_dataModel ) );

    // give a bit more room for comboboxes
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );


    SetInitialFocus( m_grid );
    m_grid->ClearSelection();

    SetupStandardButtons();

    finishDialogSettings();

    SetSize( wxSize( horizPixelsFromDU( 600 ), vertPixelsFromDU( 300 ) ) );

    Center();

    // Connect Events
    m_grid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FIELDS::OnColSort, this );
    m_grid->Bind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FIELDS::OnColMove, this );
    m_grid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &DIALOG_LIB_FIELDS::OnTableCellClick, this );
    m_fieldsCtrl->Bind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DIALOG_LIB_FIELDS::OnColLabelChange, this );

    OnInit();
}


DIALOG_LIB_FIELDS::~DIALOG_LIB_FIELDS()
{
    // Disconnect Events
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FIELDS::OnColSort, this );
    m_grid->Unbind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FIELDS::OnColMove, this );
    m_grid->Unbind( wxEVT_GRID_CELL_LEFT_CLICK, &DIALOG_LIB_FIELDS::OnTableCellClick, this );
    m_fieldsCtrl->Unbind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DIALOG_LIB_FIELDS::OnColLabelChange, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}

void DIALOG_LIB_FIELDS::OnInit()
{
    // Update the field list and refresh the grid
    UpdateFieldList();

    int colWidth = 0;

    for( int row = 0; row < m_fieldsCtrl->GetItemCount(); ++row )
    {
        const wxString& displayName = m_fieldsCtrl->GetTextValue( row, DISPLAY_NAME_COLUMN );
        colWidth = std::max( colWidth, KIUI::GetTextSize( displayName, m_fieldsCtrl ).x );

        const wxString& label = m_fieldsCtrl->GetTextValue( row, LABEL_COLUMN );
        colWidth = std::max( colWidth, KIUI::GetTextSize( label, m_fieldsCtrl ).x );
    }

    m_fieldNameColWidth = colWidth + 20;
    m_labelColWidth = colWidth + 20;

    int fieldsMinWidth = m_fieldNameColWidth + m_labelColWidth + m_groupByColWidth + m_showColWidth;

    m_fieldsCtrl->GetColumn( DISPLAY_NAME_COLUMN )->SetWidth( m_fieldNameColWidth );
    m_fieldsCtrl->GetColumn( LABEL_COLUMN )->SetWidth( m_labelColWidth );

    m_splitterMainWindow->SetMinimumPaneSize( fieldsMinWidth );
    m_splitterMainWindow->SetSashPosition( fieldsMinWidth + 40 );

    SetupAllColumnProperties();
    RegroupSymbols();
}


void DIALOG_LIB_FIELDS::loadSymbols( const wxArrayString& aSymbolNames )
{
    // Clear any existing data
    m_symbolsList.clear();

    try
    {
        if( aSymbolNames.IsEmpty() )
        {
            wxMessageBox( wxString::Format( _( "No symbols found in library %s." ), m_libId ) );
            return;
        }

        // Load each symbol from the library manager and add it to our list
        for( const wxString& symbolName : aSymbolNames )
        {
            LIB_SYMBOL* symbol = nullptr;

            try
            {
                symbol = m_parent->GetLibManager().GetSymbol( symbolName, m_libId );

                if( symbol )
                    m_symbolsList.push_back( symbol );
            }
            catch( const IO_ERROR& ioe )
            {
                // Log the error and continue
                wxLogWarning( wxString::Format( _( "Error loading symbol %s: %s" ), symbolName, ioe.What() ) );
            }
        }
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error accessing library %s.\n\n%s" ), m_libId, ioe.What() );
        DisplayErrorMessage( this, msg );
        return;
    }

    if( m_symbolsList.empty() )
    {
        wxMessageBox( _( "No symbols could be loaded from the library." ) );
        return;
    }
}

void DIALOG_LIB_FIELDS::OnClose(wxCloseEvent& aEvent)
{
    // This is a cancel, so commit quietly as we're going to throw the results away anyway.
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

    // Save all our settings since we're really closing
    SYMBOL_EDITOR_SETTINGS* cfg = m_parent->GetSettings();

    cfg->m_LibFieldEditor.width = GetSize().x;
    cfg->m_LibFieldEditor.height = GetSize().y;

    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( m_dataModel->GetColFieldName( i ).ToUTF8() );
            cfg->m_LibFieldEditor.field_widths[fieldName] = m_grid->GetColSize( i );
        }
    }

    aEvent.Skip();
}

bool DIALOG_LIB_FIELDS::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    m_dataModel->ApplyData(
            [&]( LIB_SYMBOL* )
            {
                m_parent->OnModify();
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
    return true;
}

void DIALOG_LIB_FIELDS::OnColumnItemToggled(wxDataViewEvent& event)
{
    wxDataViewItem item = event.GetItem();
    int            row = m_fieldsCtrl->ItemToRow( item );
    int            col = event.GetColumn();

    switch ( col )
    {
    case SHOW_FIELD_COLUMN:
    {
        wxString name = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
        bool     value = m_fieldsCtrl->GetToggleValue( row, col );
        int      dataCol = m_dataModel->GetFieldNameCol( name );

        m_dataModel->SetShowColumn( dataCol, value );

        if( dataCol != -1 )
        {
            if( value )
                m_grid->ShowCol( dataCol );
            else
                m_grid->HideCol( dataCol );
        }

        break;
    }

    case GROUP_BY_COLUMN:
    {
        m_dataModel->SetGroupingEnabled( false );
        // Check if any columns are grouped.  We don't keep a separate UI state for this
        // so check all rows anytime we change a grouping column.

        for( int i = 0; i < m_fieldsCtrl->GetItemCount(); ++i )
        {
            if( m_fieldsCtrl->GetToggleValue( i, GROUP_BY_COLUMN ) )
            {
                m_dataModel->SetGroupingEnabled( true );
                break;
            }
        }

        wxString name = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
        bool     value = m_fieldsCtrl->GetToggleValue( row, GROUP_BY_COLUMN );

        wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );

        m_dataModel->SetGroupColumn( m_dataModel->GetFieldNameCol( fieldName ), value );
        RegroupSymbols();
        break;
    }

    default:
        break;
    }
}

void DIALOG_LIB_FIELDS::OnFieldsCtrlSelectionChanged(wxDataViewEvent& event)
{
    // Enable/disable Rename/Remove buttons based on selection
    int row = m_fieldsCtrl->GetSelectedRow();
    bool enable = ( row != -1 ); // Basic check, could add mandatory field check later

    // Check if the selected field is mandatory (cannot be removed/renamed)
    if( enable )
    {
        wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
        if( fieldName == GetCanonicalFieldName( FIELD_T::REFERENCE ) ||
            fieldName == GetCanonicalFieldName( FIELD_T::VALUE ) ||
            fieldName == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) ||
            fieldName == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
        {
            enable = false;
        }
    }

    m_removeFieldButton->Enable( enable );
    m_renameFieldButton->Enable( enable );

    event.Skip(); // Allow default processing
}

void DIALOG_LIB_FIELDS::OnSizeFieldList(wxSizeEvent& event)
{
    int width = KIPLATFORM::UI::GetUnobscuredSize( m_fieldsCtrl ).x
                    - m_showColWidth
                    - m_groupByColWidth;
#ifdef __WXMAC__
    // TODO: something in wxWidgets 3.1.x pads checkbox columns with extra space.  (It used to
    // also be that the width of the column would get set too wide (to 30), but that's patched in
    // our local wxWidgets fork.)
    width -= 50;
#endif

    m_fieldNameColWidth = width / 2;
    m_labelColWidth = width = m_fieldNameColWidth;

    // GTK loses its head and messes these up when resizing the splitter bar:
    m_fieldsCtrl->GetColumn( SHOW_FIELD_COLUMN )->SetWidth( m_showColWidth );
    m_fieldsCtrl->GetColumn( GROUP_BY_COLUMN )->SetWidth( m_groupByColWidth );

    m_fieldsCtrl->GetColumn( FIELD_NAME_COLUMN )->SetHidden( true );
    m_fieldsCtrl->GetColumn( DISPLAY_NAME_COLUMN )->SetWidth( m_fieldNameColWidth );
    m_fieldsCtrl->GetColumn( LABEL_COLUMN )->SetWidth( m_labelColWidth );

    m_fieldsCtrl->Refresh(); // To refresh checkboxes on Windows.

    event.Skip();
}

void DIALOG_LIB_FIELDS::OnAddField(wxCommandEvent& event)
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
            DisplayError( this, wxString::Format( _( "Field name '%s' already in use." ),
                                                  fieldName ) );
            return;
        }
    }

    AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false, true );

    SetupColumnProperties( m_dataModel->GetColsCount() - 1 );

    OnModify();
}

void DIALOG_LIB_FIELDS::OnRenameField(wxCommandEvent& event)
{
    int row = m_fieldsCtrl->GetSelectedRow();

    if( row == -1 )
    {
        wxBell();
        return;
    }

    wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );

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

    m_fieldsCtrl->SetTextValue( newFieldName, row, DISPLAY_NAME_COLUMN );
    m_fieldsCtrl->SetTextValue( newFieldName, row, FIELD_NAME_COLUMN );
    m_fieldsCtrl->SetTextValue( newFieldName, row, LABEL_COLUMN );

    SetupColumnProperties( col );
    OnModify();
}

void DIALOG_LIB_FIELDS::OnRemoveField(wxCommandEvent& event)
{
    int row = m_fieldsCtrl->GetSelectedRow();

    if( row == -1 )
    {
        wxBell();
        return;
    }

    wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
    wxString displayName = m_fieldsCtrl->GetTextValue( row, DISPLAY_NAME_COLUMN );

    wxString confirm_msg = wxString::Format( _( "Are you sure you want to remove the field '%s'?" ),
                                             displayName );

    if( !IsOK( this, confirm_msg ) )
        return;

    int col = m_dataModel->GetFieldNameCol( fieldName );

    RemoveField( fieldName );

    m_fieldsCtrl->DeleteItem( row );

    if( row > 0 )
        m_fieldsCtrl->SelectRow( row - 1 );

    wxGridTableMessage msg( m_dataModel, wxGRIDTABLE_NOTIFY_COLS_DELETED, col, 1 );
    m_grid->ProcessTableMessage( msg );

    OnModify();
}

void DIALOG_LIB_FIELDS::OnFilterMouseMoved(wxMouseEvent& aEvent)
{
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
}

void DIALOG_LIB_FIELDS::OnFilterText( wxCommandEvent& event )
{
    m_dataModel->SetFilter( m_filter->GetValue() );
    RegroupSymbols();
}

void DIALOG_LIB_FIELDS::OnRegroupSymbols( wxCommandEvent& event )
{
    RegroupSymbols();
}

void DIALOG_LIB_FIELDS::OnTableValueChanged( wxGridEvent& event )
{
    m_grid->ForceRefresh();
    OnModify();
}

void DIALOG_LIB_FIELDS::OnTableCellClick( wxGridEvent& event )
{
    wxString cellValue = m_grid->GetCellValue( event.GetRow(), event.GetCol() );

    if( cellValue.StartsWith( wxS( ">  " ) ) || cellValue.StartsWith( wxS( "v  " ) ) )
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

void DIALOG_LIB_FIELDS::OnTableItemContextMenu( wxGridEvent& event )
{
    // Try to use the mouse position to determine the actual cell under the cursor.
    // Fall back to the event's row/col if mapping fails.
    int col = event.GetCol();
    int row = event.GetRow();

    if ( m_grid )
    {
        // Get global mouse position and convert to grid client coords
        wxPoint mousePos = wxGetMousePosition();
        wxPoint gridPt = m_grid->ScreenToClient( mousePos );

        row = m_grid->YToRow( gridPt.y );
        col = m_grid->XToCol( gridPt.x );
    }

    if ( row != -1 && col != -1 )
        m_grid->SetGridCursor( row, col );

    event.Skip();
}

void DIALOG_LIB_FIELDS::OnTableColSize(wxGridSizeEvent& aEvent)
{
    int         col = aEvent.GetRowOrCol();
    std::string key( m_dataModel->GetColFieldName( col ).ToUTF8() );

    aEvent.Skip();

    m_grid->ForceRefresh();
}

void DIALOG_LIB_FIELDS::OnCancel(wxCommandEvent& event)
{
    EndModal( wxID_CANCEL );
}

void DIALOG_LIB_FIELDS::OnOk(wxCommandEvent& event)
{
    if( TransferDataFromWindow() )
    {
        EndModal( wxID_OK );
    }
}

void DIALOG_LIB_FIELDS::OnApply(wxCommandEvent& event)
{
    TransferDataFromWindow();
}

void DIALOG_LIB_FIELDS::UpdateFieldList()
{
    auto addMandatoryField =
        [&]( FIELD_T fieldId, bool show, bool groupBy )
        {
            AddField( GetCanonicalFieldName( fieldId ),
                        GetDefaultFieldName( fieldId, DO_TRANSLATE ), show, groupBy );
        };

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

void DIALOG_LIB_FIELDS::AddField( const wxString& aFieldName, const wxString& aLabelValue,
    bool show, bool groupBy, bool addedByUser, bool aIsCheckbox )
{
    // Users can add fields with variable names that match the special names in the grid,
    // e.g. ${QUANTITY} so make sure we don't add them twice
    for( int i = 0; i < m_fieldsCtrl->GetItemCount(); i++ )
    {
        if( m_fieldsCtrl->GetTextValue( i, FIELD_NAME_COLUMN ) == aFieldName )
            return;
    }

    m_dataModel->AddColumn( aFieldName, aLabelValue, addedByUser, aIsCheckbox );

    wxVector<wxVariant> fieldsCtrlRow;
    std::string         key( aFieldName.ToUTF8() );

    // Don't change these to emplace_back: some versions of wxWidgets don't support it
    fieldsCtrlRow.push_back( wxVariant( aFieldName ) );
    fieldsCtrlRow.push_back( wxVariant( aLabelValue ) );
    fieldsCtrlRow.push_back( wxVariant( show ) );
    fieldsCtrlRow.push_back( wxVariant( groupBy ) );
    fieldsCtrlRow.push_back( wxVariant( aFieldName ) );

    m_fieldsCtrl->AppendItem( fieldsCtrlRow );

    wxGridTableMessage msg( m_dataModel, wxGRIDTABLE_NOTIFY_COLS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );
}

void DIALOG_LIB_FIELDS::RemoveField(const wxString& fieldName)
{
    int col = m_dataModel->GetFieldNameCol( fieldName );
    wxCHECK_RET( col != -1, wxS( "Field name not found" ) );

    m_dataModel->RemoveColumn( col );
}

void DIALOG_LIB_FIELDS::RenameField(const wxString& oldName, const wxString& newName)
{
    int col = m_dataModel->GetFieldNameCol( oldName );
    wxCHECK_RET( col != -1, wxS( "Existing field name missing from data model" ) );

    m_dataModel->RenameColumn( col, newName );
}

void DIALOG_LIB_FIELDS::RegroupSymbols()
{
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();
}

void DIALOG_LIB_FIELDS::OnColSort(wxGridEvent& aEvent)
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

void DIALOG_LIB_FIELDS::OnColMove(wxGridEvent& aEvent)
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

void DIALOG_LIB_FIELDS::OnColLabelChange(wxDataViewEvent& aEvent)
{
    wxDataViewItem item = aEvent.GetItem();
    int            row = m_fieldsCtrl->ItemToRow( item );
    wxString       label = m_fieldsCtrl->GetTextValue( row, LABEL_COLUMN );
    wxString       fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
    int            col = m_dataModel->GetFieldNameCol( fieldName );

    if( col != -1 )
        m_dataModel->SetColLabelValue( col, label );

    aEvent.Skip();

    m_grid->ForceRefresh();
}


void DIALOG_LIB_FIELDS::SetupColumnProperties( int aCol )
{
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( false );

    // Set some column types to specific editors
    if( m_dataModel->GetColFieldName( aCol ) == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
    {
        attr->SetEditor( new GRID_CELL_FPID_EDITOR( this, wxEmptyString ) );
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
        attr->SetEditor( m_grid->GetDefaultEditor() );
        m_dataModel->SetColAttr( attr, aCol );
    }
}


void DIALOG_LIB_FIELDS::SetupAllColumnProperties()
{
    SYMBOL_EDITOR_SETTINGS* cfg = m_parent->GetSettings();
    wxSize defaultDlgSize = ConvertDialogToPixels( wxSize( 600, 300 ) );

    // Restore column sorting order and widths
    m_grid->AutoSizeColumns( false );
    int  sortCol = 0;
    bool sortAscending = true;

    // Find the VALUE column for initial sorting
    int valueCol = m_dataModel->GetFieldNameCol( GetCanonicalFieldName( FIELD_T::VALUE ) );

    // Set initial sort to VALUE field (ascending) if no previous sort preference exists
    if( m_dataModel->GetSortCol() == 0 && valueCol != -1 )
    {
        sortCol = valueCol;
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

    // sync m_grid's column visibilities to Show checkboxes in m_fieldsCtrl
    for( int i = 0; i < m_fieldsCtrl->GetItemCount(); ++i )
    {
        int col = m_dataModel->GetFieldNameCol( m_fieldsCtrl->GetTextValue( i, FIELD_NAME_COLUMN ) );

        if( col == -1 )
            continue;

        bool show = m_fieldsCtrl->GetToggleValue( i, SHOW_FIELD_COLUMN );
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
