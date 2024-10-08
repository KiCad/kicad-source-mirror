/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
 * Copyright (C) 2017-2023, 2024 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <common.h>
#include <base_units.h>
#include <bitmaps.h>
#include <symbol_library.h>
#include <confirm.h>
#include <eda_doc.h>
#include <wildcards_and_files_ext.h>
#include <schematic_settings.h>
#include <general.h>
#include <grid_tricks.h>
#include <string_utils.h>
#include <kiface_base.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <tools/sch_editor_control.h>
#include <kiplatform/ui.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/bitmap_button.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <wx/debug.h>
#include <wx/ffile.h>
#include <wx/grid.h>
#include <wx/textdlg.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <dialogs/eda_view_switcher.h>
#include "dialog_symbol_fields_table.h"
#include <fields_data_model.h>
#include <eda_list_dialog.h>
#include <project_sch.h>

wxDEFINE_EVENT( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, wxCommandEvent );

#ifdef __WXMAC__
#define COLUMN_MARGIN 3
#else
#define COLUMN_MARGIN 15
#endif

using SCOPE = FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE;


enum
{
    MYID_SELECT_FOOTPRINT = GRIDTRICKS_FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET
};

class FIELDS_EDITOR_GRID_TRICKS : public GRID_TRICKS
{
public:
    FIELDS_EDITOR_GRID_TRICKS( DIALOG_SHIM* aParent, WX_GRID* aGrid,
                               wxDataViewListCtrl*            aFieldsCtrl,
                               FIELDS_EDITOR_GRID_DATA_MODEL* aDataModel ) :
            GRID_TRICKS( aGrid ),
            m_dlg( aParent ),
            m_fieldsCtrl( aFieldsCtrl ),
            m_dataModel( aDataModel )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override
    {
        if( m_grid->GetGridCursorCol() == FOOTPRINT_FIELD )
        {
            menu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ),
                         _( "Browse for footprint" ) );
            menu.AppendSeparator();
        }
        else if( m_grid->GetGridCursorCol() == DATASHEET_FIELD )
        {
            menu.Append( MYID_SHOW_DATASHEET, _( "Show Datasheet" ),
                         _( "Show datasheet in browser" ) );
            menu.AppendSeparator();
        }

        GRID_TRICKS::showPopupMenu( menu, aEvent );
    }

    void doPopupSelection( wxCommandEvent& event ) override
    {
        if( event.GetId() == MYID_SELECT_FOOTPRINT )
        {
            // pick a footprint using the footprint picker.
            wxString fpid = m_grid->GetCellValue( m_grid->GetGridCursorRow(), FOOTPRINT_FIELD );

            if( KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, m_dlg ) )
            {
                if( frame->ShowModal( &fpid, m_dlg ) )
                    m_grid->SetCellValue( m_grid->GetGridCursorRow(), FOOTPRINT_FIELD, fpid );

                frame->Destroy();
            }
        }
        else if (event.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheet_uri = m_grid->GetCellValue( m_grid->GetGridCursorRow(),
                                                           DATASHEET_FIELD );
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
                int col = event.GetId() - GRIDTRICKS_FIRST_SHOWHIDE;

                bool show = !m_dataModel->GetShowColumn( col );

                // Convert data model column to by iterating over m_fieldsCtrl rows
                // and finding the matching field name
                wxString fieldName = m_dataModel->GetColFieldName( col );

                for( int row = 0; row < m_fieldsCtrl->GetItemCount(); row++ )
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

    DIALOG_SHIM*        m_dlg;
    wxDataViewListCtrl* m_fieldsCtrl;
    FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
};


DIALOG_SYMBOL_FIELDS_TABLE::DIALOG_SYMBOL_FIELDS_TABLE( SCH_EDIT_FRAME* parent ) :
        DIALOG_SYMBOL_FIELDS_TABLE_BASE( parent ),
        m_currentBomPreset( nullptr ),
        m_lastSelectedBomPreset( nullptr ),
        m_parent( parent ),
        m_schSettings( parent->Schematic().Settings() )
{
    // Get all symbols from the list of schematic sheets
    m_parent->Schematic().Hierarchy().GetSymbols( m_symbolsList, false );

    m_bRefresh->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );
    m_bRefreshPreview->SetBitmap( KiBitmapBundle( BITMAPS::small_refresh ) );
    m_browseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_folder ) );

    m_addFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_removeFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_renameFieldButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );

    m_removeFieldButton->Enable( false );
    m_renameFieldButton->Enable( false );

    m_bomPresetsLabel->SetFont( KIUI::GetInfoFont( this ) );
    m_labelBomExportPresets->SetFont( KIUI::GetInfoFont( this ) );

    m_fieldsCtrl->AppendTextColumn( _( "Field" ), wxDATAVIEW_CELL_INERT, 0, wxALIGN_LEFT, 0 );
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
    m_dataModel = new FIELDS_EDITOR_GRID_DATA_MODEL( m_symbolsList );

    LoadFieldNames();   // loads rows into m_fieldsCtrl and columns into m_dataModel

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
    m_grid->PushEventHandler( new FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_fieldsCtrl,
                                                             m_dataModel ) );

    // give a bit more room for comboboxes
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    // Load our BOM view presets
    SetUserBomPresets( m_schSettings.m_BomPresets );
    ApplyBomPreset( m_schSettings.m_BomSettings );
    syncBomPresetSelection();

    // Load BOM export format presets
    SetUserBomFmtPresets( m_schSettings.m_BomFmtPresets );
    ApplyBomFmtPreset( m_schSettings.m_BomFmtSettings );
    syncBomFmtPresetSelection();

    SetInitialFocus( m_grid );
    m_grid->ClearSelection();

    SetupStandardButtons();

    finishDialogSettings();

    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();
    EESCHEMA_SETTINGS::PANEL_FIELD_EDITOR& panelCfg = cfg->m_FieldEditorPanel;

    wxSize dlgSize( panelCfg.width > 0 ? panelCfg.width : horizPixelsFromDU( 600 ),
                    panelCfg.height > 0 ? panelCfg.height : vertPixelsFromDU( 300 ) );
    SetSize( dlgSize );

    m_nbPages->SetSelection( cfg->m_FieldEditorPanel.page );

    switch( cfg->m_FieldEditorPanel.selection_mode )
    {
    case 0: m_radioHighlight->SetValue( true ); break;
    case 1: m_radioSelect->SetValue( true );    break;
    case 2: m_radioOff->SetValue( true );       break;
    }

    switch( cfg->m_FieldEditorPanel.scope )
    {
    case SCOPE::SCOPE_ALL:             m_radioProject->SetValue( true );      break;
    case SCOPE::SCOPE_SHEET:           m_radioCurrentSheet->SetValue( true ); break;
    case SCOPE::SCOPE_SHEET_RECURSIVE: m_radioRecursive->SetValue( true );    break;
    }

    m_outputFileName->SetValue( m_schSettings.m_BomExportFileName );

    Center();

    // Connect Events
    m_grid->Connect( wxEVT_GRID_COL_SORT,
                     wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnColSort ), nullptr, this );
    m_grid->Connect( wxEVT_GRID_COL_MOVE,
                     wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnColMove ), nullptr, this );
    m_cbBomPresets->Bind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Bind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomFmtPresetChanged, this );
    m_fieldsCtrl->Bind( wxEVT_DATAVIEW_ITEM_VALUE_CHANGED,
                        &DIALOG_SYMBOL_FIELDS_TABLE::OnColLabelChange, this );

    // Start listening for schematic changes
    m_parent->Schematic().AddListener( this );
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetupColumnProperties( int aCol )
{
    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly( false );

    // Set some column types to specific editors
    if( m_dataModel->ColIsReference( aCol ) )
    {
        attr->SetReadOnly();
        m_grid->SetColAttr( aCol, attr );
    }
    else if( m_dataModel->GetColFieldName( aCol ) == GetCanonicalFieldName( FOOTPRINT_FIELD ) )
    {
        attr->SetEditor( new GRID_CELL_FPID_EDITOR( this, wxEmptyString ) );
        m_grid->SetColAttr( aCol, attr );
    }
    else if( m_dataModel->GetColFieldName( aCol ) == GetCanonicalFieldName( DATASHEET_FIELD ) )
    {
        // set datasheet column viewer button
        attr->SetEditor( new GRID_CELL_URL_EDITOR( this, PROJECT_SCH::SchSearchS( &Prj() ),
                                                   &m_parent->Schematic() ) );
        m_grid->SetColAttr( aCol, attr );
    }
    else if( m_dataModel->ColIsQuantity( aCol ) || m_dataModel->ColIsItemNumber( aCol ) )
    {
        attr->SetReadOnly();
        m_grid->SetColAttr( aCol, attr );
        m_grid->SetColFormatNumber( aCol );
    }
    else if( m_dataModel->ColIsAttribute( aCol ) )
    {
        attr->SetAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
        m_grid->SetColAttr( aCol, attr );
        m_grid->SetColFormatBool( aCol );
    }
    else if( IsTextVar( m_dataModel->GetColFieldName( aCol ) ) )
    {
        attr->SetReadOnly();
        m_grid->SetColAttr( aCol, attr );
    }
    else
    {
        attr->SetEditor( m_grid->GetDefaultEditor() );
        m_grid->SetColAttr( aCol, attr );
        m_grid->SetColFormatCustom( aCol, wxGRID_VALUE_STRING );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetupAllColumnProperties()
{
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();
    wxSize defaultDlgSize = ConvertDialogToPixels( wxSize( 600, 300 ) );

    // Restore column sorting order and widths
    m_grid->AutoSizeColumns( false );
    int  sortCol = 0;
    bool sortAscending = true;

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

            if( cfg->m_FieldEditorPanel.field_widths.count( key )
                && ( cfg->m_FieldEditorPanel.field_widths.at( key ) > 0 ) )
            {
                m_grid->SetColSize( col, cfg->m_FieldEditorPanel.field_widths.at( key ) );
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


DIALOG_SYMBOL_FIELDS_TABLE::~DIALOG_SYMBOL_FIELDS_TABLE()
{
    // Disconnect Events
    m_grid->Disconnect( wxEVT_GRID_COL_SORT,
                        wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnColSort ), nullptr,
                        this );
    m_grid->Disconnect( wxEVT_GRID_COL_SORT,
                        wxGridEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnColMove ), nullptr,
                        this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // we gave ownership of m_dataModel to the wxGrid...
}


bool DIALOG_SYMBOL_FIELDS_TABLE::TransferDataToWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    TOOL_MANAGER*      toolMgr = m_parent->GetToolManager();
    EE_SELECTION_TOOL* selectionTool = toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selectionTool->GetSelection();
    SCH_SYMBOL*        symbol = nullptr;

    UpdateScope();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = selection.Front();

        if( item->Type() == SCH_SYMBOL_T )
            symbol = (SCH_SYMBOL*) item;
        else if( item->GetParent() && item->GetParent()->Type() == SCH_SYMBOL_T )
            symbol = (SCH_SYMBOL*) item->GetParent();
    }

    if( symbol )
    {
        for( int row = 0; row < m_dataModel->GetNumberRows(); ++row )
        {
            std::vector<SCH_REFERENCE> references = m_dataModel->GetRowReferences( row );
            bool                       found = false;

            for( const SCH_REFERENCE& ref : references )
            {
                if( ref.GetSymbol() == symbol )
                {
                    found = true;
                    break;
                }
            }

            if( found )
            {
                // Find the value column and the reference column if they're shown
                int valueCol = -1;
                int refCol = -1;
                int anyCol = -1;

                for( int col = 0; col < m_dataModel->GetNumberCols(); col++ )
                {
                    if( m_dataModel->ColIsValue( col ) )
                        valueCol = col;
                    else if( m_dataModel->ColIsReference( col ) )
                        refCol = col;
                    else if( anyCol == -1 && m_dataModel->GetShowColumn( col ) )
                        anyCol = col;
                }

                if( valueCol != -1 && m_dataModel->GetShowColumn( valueCol ) )
                    m_grid->GoToCell( row, valueCol );
                else if( refCol != -1 && m_dataModel->GetShowColumn( refCol ) )
                    m_grid->GoToCell( row, refCol );
                else if( anyCol != -1 )
                    m_grid->GoToCell( row, anyCol );

                break;
            }
        }
    }

    // We don't want table range selection events to happen until we've loaded the data or we
    // we'll clear our selection as the grid is built before the code above can get the
    // user's current selection.
    EnableSelectionEvents();

    return true;
}


bool DIALOG_SYMBOL_FIELDS_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    SCH_COMMIT     commit( m_parent );
    SCH_SHEET_PATH currentSheet = m_parent->GetCurrentSheet();

    std::function<void( SCH_SYMBOL&, SCH_SHEET_PATH & aPath )> changeHandler =
            [&commit]( SCH_SYMBOL& aSymbol, SCH_SHEET_PATH& aPath ) -> void
            {
                commit.Modify( &aSymbol, aPath.LastScreen() );
            };

    m_dataModel->ApplyData( changeHandler );

    commit.Push( wxS( "Symbol Fields Table Edit" ) );

    // Reset the view to where we left the user
    m_parent->SetCurrentSheet( currentSheet );
    m_parent->SyncView();
    m_parent->Refresh();

    m_parent->OnModify();

    return true;
}


void DIALOG_SYMBOL_FIELDS_TABLE::AddField( const wxString& aFieldName, const wxString& aLabelValue,
                                           bool show, bool groupBy, bool addedByUser )
{
    // Users can add fields with variable names that match the special names in the grid,
    // e.g. ${QUANTITY} so make sure we don't add them twice
    for( int i = 0; i < m_fieldsCtrl->GetItemCount(); i++ )
    {
        if( m_fieldsCtrl->GetTextValue( i, FIELD_NAME_COLUMN ) == aFieldName )
            return;
    }

    m_dataModel->AddColumn( aFieldName, aLabelValue, addedByUser );

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


void DIALOG_SYMBOL_FIELDS_TABLE::LoadFieldNames()
{
    // Add mandatory fields first
    for( int i = 0; i < MANDATORY_FIELDS; ++i )
    {
        bool show = false;
        bool groupBy = false;

        switch( i )
        {
        case REFERENCE_FIELD:
        case VALUE_FIELD:
        case FOOTPRINT_FIELD:
            show = true;
            groupBy = true;
            break;
        case DATASHEET_FIELD:
            show = true;
            groupBy = false;
            break;
        }

        AddField( TEMPLATE_FIELDNAME::GetDefaultFieldName( i ),
                  TEMPLATE_FIELDNAME::GetDefaultFieldName( i, true ), show, groupBy );
    }

    // Generated fields present only in the fields table
    AddField( FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE, _( "Qty" ), true, false );
    AddField( FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE, _( "#" ), true, false );

    // User fields next
    std::set<wxString> userFieldNames;

    for( unsigned i = 0; i < m_symbolsList.GetCount(); ++i )
    {
        SCH_SYMBOL* symbol = m_symbolsList[ i ].GetSymbol();

        for( int j = MANDATORY_FIELDS; j < symbol->GetFieldCount(); ++j )
            userFieldNames.insert( symbol->GetFields()[j].GetName() );
    }

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, GetTextVars( fieldName ), true, false );

    // Add any templateFieldNames which aren't already present in the userFieldNames
    for( const TEMPLATE_FIELDNAME& templateFieldname :
         m_schSettings.m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( userFieldNames.count( templateFieldname.m_Name ) == 0 )
        {
            AddField( templateFieldname.m_Name, GetTextVars( templateFieldname.m_Name ), false,
                      false );
        }
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnAddField( wxCommandEvent& event )
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

    AddField( fieldName, GetTextVars( fieldName ), true, false, true );

    SetupColumnProperties( m_dataModel->GetColsCount() - 1 );

    syncBomPresetSelection();
    OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnRemoveField( wxCommandEvent& event )
{
    int col = -1;
    int row = m_fieldsCtrl->GetSelectedRow();

   // Should never occur: "Remove Field..." button should be disabled if invalid selection
   // via OnFieldsCtrlSelectionChanged()
    wxCHECK_RET( row != -1, wxS( "Some user defined field must be selected first" ) );
    wxCHECK_RET( row >= MANDATORY_FIELDS, wxS( "Mandatory fields cannot be removed" ) );

    wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
    wxString displayName = m_fieldsCtrl->GetTextValue( row, DISPLAY_NAME_COLUMN );

    wxString confirm_msg = wxString::Format( _( "Are you sure you want to remove the field '%s'?" ),
                                             displayName );

    if( !IsOK( this, confirm_msg ) )
        return;

    for( int i = 0; i < m_dataModel->GetNumberCols(); ++i )
    {
        if( fieldName == m_dataModel->GetColFieldName( i ) )
            col = i;
    }

    m_fieldsCtrl->DeleteItem( row );
    m_dataModel->RemoveColumn( col );

    // Make selection and update the state of "Remove field..." button via OnFieldsCtrlSelectionChanged()
    // Safe to decrement row index because we always have mandatory fields
    m_fieldsCtrl->SelectRow( --row );

    if( row < MANDATORY_FIELDS )
    {
         m_removeFieldButton->Enable( false );
         m_renameFieldButton->Enable( false );
    }

    wxGridTableMessage msg( m_dataModel, wxGRIDTABLE_NOTIFY_COLS_DELETED, col, 1 );

    m_grid->ProcessTableMessage( msg );

    syncBomPresetSelection();
    OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnRenameField( wxCommandEvent& event )
{
    int row = m_fieldsCtrl->GetSelectedRow();
    wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );

    // Should never occur: "Rename Field..." button should be disabled if invalid selection
    // via OnFieldsCtrlSelectionChanged()
    wxCHECK_RET( row != -1, wxS( "Some user defined field must be selected first" ) );
    wxCHECK_RET( row >= MANDATORY_FIELDS, wxS( "Mandatory fields cannot be renamed" ) );
    wxCHECK_RET( !fieldName.IsEmpty(), wxS( "Field must have a name" ) );

    int col = m_dataModel->GetFieldNameCol( fieldName );
    wxCHECK_RET( col != -1, wxS( "Existing field name missing from data model" ) );

    wxTextEntryDialog dlg( this, _( "New field name:" ), _( "Rename Field" ) );

    if( dlg.ShowModal() != wxID_OK )
         return;

    wxString newFieldName = dlg.GetValue();

    // No change, no-op
    if( newFieldName == fieldName )
         return;

    // New field name already exists
    if( m_dataModel->GetFieldNameCol( newFieldName ) != -1 )
    {
         wxString confirm_msg = wxString::Format( _( "Field name %s already exists." ),
                                                  newFieldName );
         DisplayError( this, confirm_msg );
         return;
    }

    m_dataModel->RenameColumn( col, newFieldName );
    m_fieldsCtrl->SetTextValue( newFieldName, row, DISPLAY_NAME_COLUMN );
    m_fieldsCtrl->SetTextValue( newFieldName, row, FIELD_NAME_COLUMN );
    m_fieldsCtrl->SetTextValue( newFieldName, row, LABEL_COLUMN );

    syncBomPresetSelection();
    OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnFilterText( wxCommandEvent& aEvent )
{
    m_dataModel->SetFilter( m_filter->GetValue() );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnFilterMouseMoved( wxMouseEvent& aEvent )
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


void DIALOG_SYMBOL_FIELDS_TABLE::OnFieldsCtrlSelectionChanged( wxDataViewEvent& event )
{
    int row = m_fieldsCtrl->GetSelectedRow();

    if( row >= MANDATORY_FIELDS )
    {
        m_removeFieldButton->Enable( true );
        m_renameFieldButton->Enable( true );
    }
    else
    {
        m_removeFieldButton->Enable( false );
        m_renameFieldButton->Enable( false );
    }
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnColumnItemToggled( wxDataViewEvent& event )
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
        wxString name = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
        bool     value = m_fieldsCtrl->GetToggleValue( row, col );
        int      dataCol = m_dataModel->GetFieldNameCol( name );

        if( m_dataModel->ColIsQuantity( dataCol ) && value )
        {
            DisplayError( this, _( "The Quantity column cannot be grouped by." ) );

            value = false;
            m_fieldsCtrl->SetToggleValue( value, row, col );
        }

        if( m_dataModel->ColIsItemNumber( dataCol ) && value )
        {
            DisplayError( this, _( "The Item Number column cannot be grouped by." ) );

            value = false;
            m_fieldsCtrl->SetToggleValue( value, row, col );
        }

        wxString fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );

        m_dataModel->SetGroupColumn( m_dataModel->GetFieldNameCol( fieldName ), value );
        m_dataModel->RebuildRows();
        m_grid->ForceRefresh();
        break;
    }

    default:
        break;
    }

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnGroupSymbolsToggled( wxCommandEvent& event )
{
    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnExcludeDNPToggled( wxCommandEvent& event )
{
    m_dataModel->SetExcludeDNP( m_checkExcludeDNP->GetValue() );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnShowExcludedToggled( wxCommandEvent& event )
{
    m_dataModel->SetIncludeExcludedFromBOM( m_checkShowExcluded->GetValue() );
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnColSort( wxGridEvent& aEvent )
{
    int         sortCol = aEvent.GetCol();
    std::string key( m_dataModel->GetColFieldName( sortCol ).ToUTF8() );
    bool        ascending;

    // Don't sort by item number, it is generated by the sort
    if( m_dataModel->ColIsItemNumber( sortCol ) )
    {
        aEvent.Veto();
        return;
    }

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
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnColMove( wxGridEvent& aEvent )
{
    int origPos = aEvent.GetCol();

    // Save column widths since the setup function uses the saved config values
    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( m_dataModel->GetColFieldName( i ).ToUTF8() );
            cfg->m_FieldEditorPanel.field_widths[fieldName] = m_grid->GetColSize( i );
        }
    }

    CallAfter(
            [origPos, this]()
            {
                int newPos = m_grid->GetColPos( origPos );

                m_dataModel->MoveColumn( origPos, newPos );

                // "Unmove" the column since we've moved the column internally
                m_grid->ResetColPos();

                // We need to reset all the column attr's to the correct column order
                SetupAllColumnProperties();

                m_grid->ForceRefresh();
            } );

    syncBomPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnColLabelChange( wxDataViewEvent& aEvent )
{
    wxDataViewItem item = aEvent.GetItem();
    int            row = m_fieldsCtrl->ItemToRow( item );
    wxString       label = m_fieldsCtrl->GetTextValue( row, LABEL_COLUMN );
    wxString       fieldName = m_fieldsCtrl->GetTextValue( row, FIELD_NAME_COLUMN );
    int            col = m_dataModel->GetFieldNameCol( fieldName );

    if( col != -1 )
        m_dataModel->SetColLabelValue( col, label );

    syncBomPresetSelection();

    aEvent.Skip();

    m_grid->ForceRefresh();
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnTableValueChanged( wxGridEvent& aEvent )
{
    m_grid->ForceRefresh();
    OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnTableColSize( wxGridSizeEvent& aEvent )
{
    int         col = aEvent.GetRowOrCol();
    std::string key( m_dataModel->GetColFieldName( col ).ToUTF8() );

    aEvent.Skip();

    m_grid->ForceRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnRegroupSymbols( wxCommandEvent& aEvent )
{
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnScopeChanged( wxCommandEvent& aEvent )
{
    UpdateScope();
}

void DIALOG_SYMBOL_FIELDS_TABLE::UpdateScope()
{
    m_dataModel->SetPath( m_parent->GetCurrentSheet() );

    if( m_radioProject->GetValue() )
        m_dataModel->SetScope( FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE::SCOPE_ALL );
    else if( m_radioCurrentSheet->GetValue() )
        m_dataModel->SetScope( FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE::SCOPE_SHEET );
    else if( m_radioRecursive->GetValue() )
        m_dataModel->SetScope( FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE::SCOPE_SHEET_RECURSIVE );

    m_dataModel->RebuildRows();
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnTableCellClick( wxGridEvent& event )
{
    if( m_dataModel->ColIsReference( event.GetCol() ) )
    {
        m_grid->ClearSelection();

        m_dataModel->ExpandCollapseRow( event.GetRow() );
        m_grid->SetGridCursor( event.GetRow(), event.GetCol() );
    }
    else
    {
        event.Skip();
    }
}

void DIALOG_SYMBOL_FIELDS_TABLE::OnTableRangeSelected( wxGridRangeSelectEvent& aEvent )
{
    // Cross-probing should only work in Edit page
    if( m_nbPages->GetSelection() != 0 )
        return;

    // Multi-select can grab the rows that are expanded child refs, and also the row
    // containing the list of all child refs. Make sure we add refs/symbols uniquely
    std::set<SCH_REFERENCE> refs;
    std::set<SCH_ITEM*>     symbols;

    // This handler handles selecting and deselecting
    if( aEvent.Selecting() )
    {
        for( int i = aEvent.GetTopRow(); i <= aEvent.GetBottomRow(); i++ )
        {
            for( const SCH_REFERENCE& ref : m_dataModel->GetRowReferences( i ) )
                refs.insert( ref );
        }

        for( const SCH_REFERENCE& ref : refs )
            symbols.insert( ref.GetSymbol() );
    }

    if( m_radioHighlight->GetValue() )
    {
        SCH_EDITOR_CONTROL* editor = m_parent->GetToolManager()->GetTool<SCH_EDITOR_CONTROL>();

        if( refs.size() > 0 )
        {
            // Use of full path based on UUID allows select of not yet annotated or duplicated symbols
            wxString symbol_path = refs.begin()->GetFullPath();

            // Focus only handles one item at this time
            editor->FindSymbolAndItem( &symbol_path, nullptr, true, HIGHLIGHT_SYMBOL,
                                       wxEmptyString );
        }
        else
        {
            m_parent->FocusOnItem( nullptr );
        }
    }
    else if( m_radioSelect->GetValue() )
    {
        EE_SELECTION_TOOL* selTool = m_parent->GetToolManager()->GetTool<EE_SELECTION_TOOL>();

        std::vector<SCH_ITEM*> items( symbols.begin(), symbols.end() );

        if( refs.size() > 0 )
            selTool->SyncSelection( refs.begin()->GetSheetPath(), nullptr, items );
        else
            selTool->ClearSelection();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnTableItemContextMenu( wxGridEvent& event )
{
    // TODO: Option to select footprint if FOOTPRINT column selected

    event.Skip();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSizeFieldList( wxSizeEvent& event )
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


void DIALOG_SYMBOL_FIELDS_TABLE::OnSaveAndContinue( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
    {
        m_parent->SaveProject();
        ClearModify();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnPageChanged( wxNotebookEvent& event )
{
    PreviewRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnPreviewRefresh( wxCommandEvent& event )
{
    PreviewRefresh();
    syncBomFmtPresetSelection();
}


void DIALOG_SYMBOL_FIELDS_TABLE::PreviewRefresh()
{
    m_dataModel->RebuildRows();
    m_textOutput->SetValue( m_dataModel->Export( GetCurrentBomFmtSettings() ) );
}


BOM_FMT_PRESET DIALOG_SYMBOL_FIELDS_TABLE::GetCurrentBomFmtSettings()
{
    BOM_FMT_PRESET current;

    current.name = m_cbBomFmtPresets->GetStringSelection();
    current.fieldDelimiter = m_textFieldDelimiter->GetValue();
    current.stringDelimiter = m_textStringDelimiter->GetValue();
    current.refDelimiter = m_textRefDelimiter->GetValue();
    current.refRangeDelimiter = m_textRefRangeDelimiter->GetValue();
    current.keepTabs = m_checkKeepTabs->GetValue();
    current.keepLineBreaks = m_checkKeepLineBreaks->GetValue();

    return current;
}


void DIALOG_SYMBOL_FIELDS_TABLE::ShowEditTab()
{
    m_nbPages->SetSelection( 0 );
}


void DIALOG_SYMBOL_FIELDS_TABLE::ShowExportTab()
{
    m_nbPages->SetSelection( 1 );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnOutputFileBrowseClicked( wxCommandEvent& event )
{
    // Build the absolute path of current output directory to preselect it in the file browser.
    wxString path = ExpandEnvVarSubstitutions( m_outputFileName->GetValue(), &Prj() );
    path = Prj().AbsolutePath( path );


    // Calculate the export filename
    wxFileName fn( Prj().AbsolutePath( m_parent->Schematic().GetFileName() ) );
    fn.SetExt( FILEEXT::CsvFileExtension );

    wxFileDialog saveDlg( this, _( "Bill of Materials Output File" ), path, fn.GetFullName(),
                          FILEEXT::CsvFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( saveDlg.ShowModal() == wxID_CANCEL )
        return;


    wxFileName file = wxFileName( saveDlg.GetPath() );
    wxString   defaultPath = fn.GetPathWithSep();
    wxString   msg;
    msg.Printf( _( "Do you want to use a path relative to\n'%s'?" ), defaultPath );

    wxMessageDialog dialog( this, msg, _( "BOM Output File" ),
                            wxYES_NO | wxICON_QUESTION | wxYES_DEFAULT );

    if( dialog.ShowModal() == wxID_YES )
    {
        if( !file.MakeRelativeTo( defaultPath ) )
        {
            wxMessageBox( _( "Cannot make path relative (target volume different from schematic "
                             "file volume)!" ),
                          _( "BOM Output File" ), wxOK | wxICON_ERROR );
        }
    }

    m_outputFileName->SetValue( file.GetFullPath() );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnExport( wxCommandEvent& aEvent )
{
    if( m_dataModel->IsEdited() )
    {
        if( OKOrCancelDialog( nullptr, _( "Unsaved data" ),
                              _( "Changes have not yet been saved. Export unsaved data?" ), "",
                              _( "OK" ), _( "Cancel" ) )
            == wxID_CANCEL )
        {
            return;
        }
    }

    // Create output directory if it does not exist (also transform it in absolute form).
    // Bail if it fails.

    std::function<bool( wxString* )> textResolver =
            [&]( wxString* token ) -> bool
            {
                SCHEMATIC& schematic = m_parent->Schematic();

                // Handles m_board->GetTitleBlock() *and* m_board->GetProject()
                return schematic.ResolveTextVar( &schematic.CurrentSheet(), token, 0 );
            };

    wxString path = m_outputFileName->GetValue();

    if( path.IsEmpty() )
    {
        DisplayError( this, _( "No output file specified in Export tab." ) );
        return;
    }

    path = ExpandTextVars( path, &textResolver );
    path = ExpandEnvVarSubstitutions( path, nullptr );

    wxFileName outputFile = wxFileName::FileName( path );
    wxString msg;

    if( !EnsureFileDirectoryExists( &outputFile,
                                    Prj().AbsolutePath( m_parent->Schematic().GetFileName() ),
                                    &NULL_REPORTER::GetInstance() ) )
    {
        msg.Printf( _( "Could not open/create path '%s'." ), outputFile.GetPath() );
        DisplayError( this, msg );
        return;
    }

    wxFFile out( outputFile.GetFullPath(), "wb" );

    if( !out.IsOpened() )
    {
        msg.Printf( _( "Could not create BOM output '%s'." ), outputFile.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    PreviewRefresh();

    if( !out.Write( m_textOutput->GetValue() ) )
    {
        msg.Printf( _( "Could not write BOM output '%s'." ), outputFile.GetFullPath() );
        DisplayError( this, msg );
        return;
    }

    out.Close(); // close the file before we tell the user it's done with the info modal :workflow meme:
    msg.Printf( _( "Wrote BOM output to '%s'" ), outputFile.GetFullPath() );
    DisplayInfoMessage( this, msg );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnCancel( wxCommandEvent& aEvent )
{
    Close();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnOk( wxCommandEvent& aEvent )
{
    TransferDataFromWindow();
    Close();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnClose( wxCloseEvent& aEvent )
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

    // Stop listening to schematic events
    m_parent->Schematic().RemoveListener( this );

    // Save all our settings since we're really closing
    savePresetsToSchematic();
    m_schSettings.m_BomExportFileName = m_outputFileName->GetValue();

    EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();

    cfg->m_FieldEditorPanel.width = GetSize().x;
    cfg->m_FieldEditorPanel.height = GetSize().y;
    cfg->m_FieldEditorPanel.page = m_nbPages->GetSelection();

    if( m_radioHighlight->GetValue() )
        cfg->m_FieldEditorPanel.selection_mode = 0;
    else if( m_radioSelect->GetValue() )
        cfg->m_FieldEditorPanel.selection_mode = 1;
    else if( m_radioOff->GetValue() )
        cfg->m_FieldEditorPanel.selection_mode = 2;

    if( m_radioProject->GetValue() )
        cfg->m_FieldEditorPanel.scope = SCOPE::SCOPE_ALL;
    else if( m_radioCurrentSheet->GetValue() )
        cfg->m_FieldEditorPanel.scope = SCOPE::SCOPE_SHEET;
    else if( m_radioRecursive->GetValue() )
        cfg->m_FieldEditorPanel.scope = SCOPE::SCOPE_SHEET_RECURSIVE;

    for( int i = 0; i < m_grid->GetNumberCols(); i++ )
    {
        if( m_grid->IsColShown( i ) )
        {
            std::string fieldName( m_dataModel->GetColFieldName( i ).ToUTF8() );
            cfg->m_FieldEditorPanel.field_widths[fieldName] = m_grid->GetColSize( i );
        }
    }

    m_parent->FocusOnItem( nullptr );

    wxCommandEvent* evt = new wxCommandEvent( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, wxID_ANY );

    if( wxWindow* parent = GetParent() )
        wxQueueEvent( parent, evt );
}


std::vector<BOM_PRESET> DIALOG_SYMBOL_FIELDS_TABLE::GetUserBomPresets() const
{
    std::vector<BOM_PRESET> ret;

    for( const std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        if( !pair.second.readOnly )
            ret.emplace_back( pair.second );
    }

    return ret;
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetUserBomPresets( std::vector<BOM_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultBomPresets();

    for( const BOM_PRESET& preset : aPresetList )
    {
        if( m_bomPresets.count( preset.name ) )
            continue;

        m_bomPresets[preset.name] = preset;

        m_bomPresetMRU.Add( preset.name );
    }

    rebuildBomPresetsWidget();
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomPreset( const wxString& aPresetName )
{
    updateBomPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onBomPresetChanged( dummy );
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomPreset( const BOM_PRESET& aPreset )
{
    if( m_bomPresets.count( aPreset.name ) )
        m_currentBomPreset = &m_bomPresets[aPreset.name];
    else
        m_currentBomPreset = nullptr;

    if( m_currentBomPreset && !m_currentBomPreset->readOnly )
        m_lastSelectedBomPreset = m_currentBomPreset;
    else
        m_lastSelectedBomPreset = nullptr;

    updateBomPresetSelection( aPreset.name );
    doApplyBomPreset( aPreset );
}


void DIALOG_SYMBOL_FIELDS_TABLE::loadDefaultBomPresets()
{
    m_bomPresets.clear();
    m_bomPresetMRU.clear();

    // Load the read-only defaults
    for( const BOM_PRESET& preset : BOM_PRESET::BuiltInPresets() )
    {
        m_bomPresets[preset.name] = preset;
        m_bomPresets[preset.name].readOnly = true;

        m_bomPresetMRU.Add( preset.name );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::rebuildBomPresetsWidget()
{
    m_cbBomPresets->Clear();

    // Build the layers preset list.
    // By default, the presetAllLayers will be selected
    int idx = 0;
    int default_idx = 0;

    for( std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        m_cbBomPresets->Append( wxGetTranslation( pair.first ),
                                static_cast<void*>( &pair.second ) );

        if( pair.first == BOM_PRESET::DefaultEditing().name )
            default_idx = idx;

        idx++;
    }

    m_cbBomPresets->Append( wxT( "---" ) );
    m_cbBomPresets->Append( _( "Save preset..." ) );
    m_cbBomPresets->Append( _( "Delete preset..." ) );

    // At least the built-in presets should always be present
    wxASSERT( !m_bomPresets.empty() );

    // Default preset: all Boms
    m_cbBomPresets->SetSelection( default_idx );
    m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( default_idx ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::syncBomPresetSelection()
{
    BOM_PRESET current = m_dataModel->GetBomSettings();

    auto it = std::find_if( m_bomPresets.begin(), m_bomPresets.end(),
                            [&]( const std::pair<const wxString, BOM_PRESET>& aPair )
                            {
                                const BOM_PRESET& preset = aPair.second;

                                // Check the simple settings first
                                if( !( preset.sortAsc == current.sortAsc
                                       && preset.filterString == current.filterString
                                       && preset.groupSymbols == current.groupSymbols
                                       && preset.excludeDNP == current.excludeDNP
                                       && preset.includeExcludedFromBOM
                                                  == current.includeExcludedFromBOM ) )
                                {
                                    return false;
                                }

                                // We should compare preset.name and current.name.
                                // unfortunately current.name is empty because
                                // m_dataModel->GetBomSettings() does not store the .name member
                                // So use sortField member as a (not very efficient) auxiliary filter.
                                // sortField can be translated in m_bomPresets list,
                                // so current.sortField needs to be translated
                                // Probably this not efficient and error prone test should be removed (JPC).
                                if( preset.sortField != wxGetTranslation( current.sortField ) )
                                    return false;

                                // Only compare shown or grouped fields
                                std::vector<BOM_FIELD> A, B;

                                for( const BOM_FIELD& field : preset.fieldsOrdered )
                                {
                                    if( field.show || field.groupBy )
                                        A.emplace_back( field );
                                }

                                for( const BOM_FIELD& field : current.fieldsOrdered )
                                {
                                    if( field.show || field.groupBy )
                                        B.emplace_back( field );
                                }

                                return A == B;
                            } );

    if( it != m_bomPresets.end() )
    {
        // Select the right m_cbBomPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;
        m_cbBomPresets->SetStringSelection( text );
    }
    else
    {
        m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 ); // separator
    }

    m_currentBomPreset = static_cast<BOM_PRESET*>(
            m_cbBomPresets->GetClientData( m_cbBomPresets->GetSelection() ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::updateBomPresetSelection( const wxString& aName )
{
    // look at m_userBomPresets to know if aName is a read only preset, or a user preset.
    // Read only presets have translated names in UI, so we have to use
    // a translated name in UI selection.
    // But for a user preset name we should search for aName (not translated)
    wxString ui_label = aName;

    for( std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        if( pair.first != aName )
            continue;

        if( pair.second.readOnly == true )
            ui_label = wxGetTranslation( aName );

        break;
    }

    int idx = m_cbBomPresets->FindString( ui_label );

    if( idx >= 0 && m_cbBomPresets->GetSelection() != idx )
    {
        m_cbBomPresets->SetSelection( idx );
        m_currentBomPreset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 ); // separator
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::onBomPresetChanged( wxCommandEvent& aEvent )
{
    int count = m_cbBomPresets->GetCount();
    int index = m_cbBomPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentBomPreset )
                    m_cbBomPresets->SetStringSelection( m_currentBomPreset->name );
                else
                    m_cbBomPresets->SetSelection( m_cbBomPresets->GetCount() - 3 );
            };

    if( index == count - 3 )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedBomPreset )
            name = m_lastSelectedBomPreset->name;

        wxTextEntryDialog dlg( this, _( "BOM preset name:" ), _( "Save BOM Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_bomPresets.count( name );

        if( !exists )
        {
            m_bomPresets[name] = m_dataModel->GetBomSettings();
            m_bomPresets[name].readOnly = false;
            m_bomPresets[name].name = name;
        }

        BOM_PRESET* preset = &m_bomPresets[name];

        if( !exists )
        {
            index = m_cbBomPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else if( preset->readOnly )
        {
            wxMessageBox( _( "Default presets cannot be modified.\nPlease use a different name." ),
                          _( "Error" ), wxOK | wxICON_ERROR, this );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( this, _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            *preset = m_dataModel->GetBomSettings();
            preset->name = name;

            index = m_cbBomPresets->FindString( name );
            m_bomPresetMRU.Remove( name );
        }

        m_currentBomPreset = preset;
        m_cbBomPresets->SetSelection( index );
        m_bomPresetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
        {
            if( !pair.second.readOnly )
            {
                wxArrayString item;
                item.Add( pair.first );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbBomPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_bomPresets.erase( presetName );

                m_cbBomPresets->Delete( idx );
                m_currentBomPreset = nullptr;

                m_bomPresetMRU.Remove( presetName );
            }
        }

        resetSelection();
        return;
    }

    BOM_PRESET* preset = static_cast<BOM_PRESET*>( m_cbBomPresets->GetClientData( index ) );
    m_currentBomPreset = preset;

    m_lastSelectedBomPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        doApplyBomPreset( *preset );
        syncBomPresetSelection();
        m_currentBomPreset = preset;

        if( !m_currentBomPreset->name.IsEmpty() )
        {
            m_bomPresetMRU.Remove( preset->name );
            m_bomPresetMRU.Insert( preset->name, 0 );
        }
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::doApplyBomPreset( const BOM_PRESET& aPreset )
{
    // Disable rebuilds while we're applying the preset otherwise we'll be
    // rebuilding the model constantly while firing off wx events
    m_dataModel->DisableRebuilds();

    // Basically, we apply the BOM preset to the data model and then
    // update our UI to reflect resulting the data model state, not the preset.
    m_dataModel->ApplyBomPreset( aPreset );

    // BOM Presets can add, but not remove, columns, so make sure the field control
    // grid has all of them before starting
    for( int i = 0; i < m_dataModel->GetColsCount(); i++ )
    {
        const wxString& fieldName( m_dataModel->GetColFieldName( i ) );
        bool            found = false;

        for( int j = 0; j < m_fieldsCtrl->GetItemCount(); j++ )
        {
            if( m_fieldsCtrl->GetTextValue( j, FIELD_NAME_COLUMN ) == fieldName )
            {
                found = true;
                break;
            }
        }

        // Properties like label, etc. will be added in the next loop
        if( !found )
            AddField( fieldName, GetTextVars( fieldName ), false, false );
    }

    // Sync all fields
    for( int i = 0; i < m_fieldsCtrl->GetItemCount(); i++ )
    {
        const wxString& fieldName( m_fieldsCtrl->GetTextValue( i, FIELD_NAME_COLUMN ) );
        int             col = m_dataModel->GetFieldNameCol( fieldName );

        if( col == -1 )
        {
            wxASSERT_MSG( true, "Fields control has a field not found in the data model." );
            continue;
        }

        EESCHEMA_SETTINGS* cfg = m_parent->eeconfig();
        std::string        fieldNameStr( fieldName.ToUTF8() );

        // Set column labels
        const wxString& label = m_dataModel->GetColLabelValue( col );
        m_fieldsCtrl->SetTextValue( label, i, LABEL_COLUMN );
        m_grid->SetColLabelValue( col, label );

        if( cfg->m_FieldEditorPanel.field_widths.count( fieldNameStr ) )
            m_grid->SetColSize( col, cfg->m_FieldEditorPanel.field_widths.at( fieldNameStr ) );

        // Set shown colums
        bool show = m_dataModel->GetShowColumn( col );
        m_fieldsCtrl->SetToggleValue( show, i, SHOW_FIELD_COLUMN );

        if( show )
            m_grid->ShowCol( col );
        else
            m_grid->HideCol( col );

        // Set grouped columns
        bool groupBy = m_dataModel->GetGroupColumn( col );
        m_fieldsCtrl->SetToggleValue( groupBy, i, GROUP_BY_COLUMN );
    }

    m_grid->SetSortingColumn( m_dataModel->GetSortCol(), m_dataModel->GetSortAsc() );
    m_groupSymbolsBox->SetValue( m_dataModel->GetGroupingEnabled() );
    m_filter->ChangeValue( m_dataModel->GetFilter() );
    m_checkExcludeDNP->SetValue( m_dataModel->GetExcludeDNP() );
    m_checkShowExcluded->SetValue( m_dataModel->GetIncludeExcludedFromBOM() );

    SetupAllColumnProperties();

    // This will rebuild all rows and columns in the model such that the order
    // and labels are right, then we refresh the shown grid data to match
    m_dataModel->EnableRebuilds();
    m_dataModel->RebuildRows();
    m_grid->ForceRefresh();
}


std::vector<BOM_FMT_PRESET> DIALOG_SYMBOL_FIELDS_TABLE::GetUserBomFmtPresets() const
{
    std::vector<BOM_FMT_PRESET> ret;

    for( const std::pair<const wxString, BOM_FMT_PRESET>& pair : m_bomFmtPresets )
    {
        if( !pair.second.readOnly )
            ret.emplace_back( pair.second );
    }

    return ret;
}


void DIALOG_SYMBOL_FIELDS_TABLE::SetUserBomFmtPresets( std::vector<BOM_FMT_PRESET>& aPresetList )
{
    // Reset to defaults
    loadDefaultBomFmtPresets();

    for( const BOM_FMT_PRESET& preset : aPresetList )
    {
        if( m_bomFmtPresets.count( preset.name ) )
            continue;

        m_bomFmtPresets[preset.name] = preset;

        m_bomFmtPresetMRU.Add( preset.name );
    }

    rebuildBomFmtPresetsWidget();
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomFmtPreset( const wxString& aPresetName )
{
    updateBomFmtPresetSelection( aPresetName );

    wxCommandEvent dummy;
    onBomFmtPresetChanged( dummy );
}


void DIALOG_SYMBOL_FIELDS_TABLE::ApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset )
{
    if( m_bomFmtPresets.count( aPreset.name ) )
        m_currentBomFmtPreset = &m_bomFmtPresets[aPreset.name];
    else
        m_currentBomFmtPreset = nullptr;

    m_lastSelectedBomFmtPreset = ( m_currentBomFmtPreset
                                    && !m_currentBomFmtPreset->readOnly ) ? m_currentBomFmtPreset
                                                                          : nullptr;

    updateBomFmtPresetSelection( aPreset.name );
    doApplyBomFmtPreset( aPreset );
}


void DIALOG_SYMBOL_FIELDS_TABLE::loadDefaultBomFmtPresets()
{
    m_bomFmtPresets.clear();
    m_bomFmtPresetMRU.clear();

    // Load the read-only defaults
    for( const BOM_FMT_PRESET& preset : BOM_FMT_PRESET::BuiltInPresets() )
    {
        m_bomFmtPresets[preset.name] = preset;
        m_bomFmtPresets[preset.name].readOnly = true;

        m_bomFmtPresetMRU.Add( preset.name );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::rebuildBomFmtPresetsWidget()
{
    m_cbBomFmtPresets->Clear();

    // Build the layers preset list.
    // By default, the presetAllLayers will be selected
    int idx = 0;
    int default_idx = 0;

    for( std::pair<const wxString, BOM_FMT_PRESET>& pair : m_bomFmtPresets )
    {
        m_cbBomFmtPresets->Append( wxGetTranslation( pair.first ),
                                   static_cast<void*>( &pair.second ) );

        if( pair.first == BOM_FMT_PRESET::CSV().name )
            default_idx = idx;

        idx++;
    }

    m_cbBomFmtPresets->Append( wxT( "---" ) );
    m_cbBomFmtPresets->Append( _( "Save preset..." ) );
    m_cbBomFmtPresets->Append( _( "Delete preset..." ) );

    // At least the built-in presets should always be present
    wxASSERT( !m_bomFmtPresets.empty() );

    // Default preset: all Boms
    m_cbBomFmtPresets->SetSelection( default_idx );
    m_currentBomFmtPreset =
            static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( default_idx ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::syncBomFmtPresetSelection()
{
    BOM_FMT_PRESET current = GetCurrentBomFmtSettings();

    auto it = std::find_if( m_bomFmtPresets.begin(), m_bomFmtPresets.end(),
                            [&]( const std::pair<const wxString, BOM_FMT_PRESET>& aPair )
                            {
                                return ( aPair.second.fieldDelimiter == current.fieldDelimiter
                                         && aPair.second.stringDelimiter == current.stringDelimiter
                                         && aPair.second.refDelimiter == current.refDelimiter
                                         && aPair.second.refRangeDelimiter == current.refRangeDelimiter
                                         && aPair.second.keepTabs == current.keepTabs
                                         && aPair.second.keepLineBreaks == current.keepLineBreaks );
                            } );

    if( it != m_bomFmtPresets.end() )
    {
        // Select the right m_cbBomFmtPresets item.
        // but these items are translated if they are predefined items.
        bool     do_translate = it->second.readOnly;
        wxString text = do_translate ? wxGetTranslation( it->first ) : it->first;

        m_cbBomFmtPresets->SetStringSelection( text );
    }
    else
    {
        m_cbBomFmtPresets->SetSelection( m_cbBomFmtPresets->GetCount() - 3 ); // separator
    }

    m_currentBomFmtPreset = static_cast<BOM_FMT_PRESET*>(
            m_cbBomFmtPresets->GetClientData( m_cbBomFmtPresets->GetSelection() ) );
}


void DIALOG_SYMBOL_FIELDS_TABLE::updateBomFmtPresetSelection( const wxString& aName )
{
    // look at m_userBomFmtPresets to know if aName is a read only preset, or a user preset.
    // Read only presets have translated names in UI, so we have to use
    // a translated name in UI selection.
    // But for a user preset name we should search for aName (not translated)
    wxString ui_label = aName;

    for( std::pair<const wxString, BOM_FMT_PRESET>& pair : m_bomFmtPresets )
    {
        if( pair.first != aName )
            continue;

        if( pair.second.readOnly == true )
            ui_label = wxGetTranslation( aName );

        break;
    }

    int idx = m_cbBomFmtPresets->FindString( ui_label );

    if( idx >= 0 && m_cbBomFmtPresets->GetSelection() != idx )
    {
        m_cbBomFmtPresets->SetSelection( idx );
        m_currentBomFmtPreset =
                static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( idx ) );
    }
    else if( idx < 0 )
    {
        m_cbBomFmtPresets->SetSelection( m_cbBomFmtPresets->GetCount() - 3 ); // separator
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::onBomFmtPresetChanged( wxCommandEvent& aEvent )
{
    int count = m_cbBomFmtPresets->GetCount();
    int index = m_cbBomFmtPresets->GetSelection();

    auto resetSelection =
            [&]()
            {
                if( m_currentBomFmtPreset )
                    m_cbBomFmtPresets->SetStringSelection( m_currentBomFmtPreset->name );
                else
                    m_cbBomFmtPresets->SetSelection( m_cbBomFmtPresets->GetCount() - 3 );
            };

    if( index == count - 3 )
    {
        // Separator: reject the selection
        resetSelection();
        return;
    }
    else if( index == count - 2 )
    {
        // Save current state to new preset
        wxString name;

        if( m_lastSelectedBomFmtPreset )
            name = m_lastSelectedBomFmtPreset->name;

        wxTextEntryDialog dlg( this, _( "BOM preset name:" ), _( "Save BOM Preset" ), name );

        if( dlg.ShowModal() != wxID_OK )
        {
            resetSelection();
            return;
        }

        name = dlg.GetValue();
        bool exists = m_bomFmtPresets.count( name );

        if( !exists )
        {
            m_bomFmtPresets[name] = GetCurrentBomFmtSettings();
            m_bomFmtPresets[name].readOnly = false;
            m_bomFmtPresets[name].name = name;
        }

        BOM_FMT_PRESET* preset = &m_bomFmtPresets[name];

        if( !exists )
        {
            index = m_cbBomFmtPresets->Insert( name, index - 1, static_cast<void*>( preset ) );
        }
        else if( preset->readOnly )
        {
            wxMessageBox( _( "Default presets cannot be modified.\nPlease use a different name." ),
                          _( "Error" ), wxOK | wxICON_ERROR, this );
            resetSelection();
            return;
        }
        else
        {
            // Ask the user if they want to overwrite the existing preset
            if( !IsOK( this, _( "Overwrite existing preset?" ) ) )
            {
                resetSelection();
                return;
            }

            *preset = GetCurrentBomFmtSettings();
            preset->name = name;

            index = m_cbBomFmtPresets->FindString( name );
            m_bomFmtPresetMRU.Remove( name );
        }

        m_currentBomFmtPreset = preset;
        m_cbBomFmtPresets->SetSelection( index );
        m_bomFmtPresetMRU.Insert( name, 0 );

        return;
    }
    else if( index == count - 1 )
    {
        // Delete a preset
        wxArrayString              headers;
        std::vector<wxArrayString> items;

        headers.Add( _( "Presets" ) );

        for( std::pair<const wxString, BOM_FMT_PRESET>& pair : m_bomFmtPresets )
        {
            if( !pair.second.readOnly )
            {
                wxArrayString item;
                item.Add( pair.first );
                items.emplace_back( item );
            }
        }

        EDA_LIST_DIALOG dlg( this, _( "Delete Preset" ), headers, items );
        dlg.SetListLabel( _( "Select preset:" ) );

        if( dlg.ShowModal() == wxID_OK )
        {
            wxString presetName = dlg.GetTextSelection();
            int      idx = m_cbBomFmtPresets->FindString( presetName );

            if( idx != wxNOT_FOUND )
            {
                m_bomFmtPresets.erase( presetName );

                m_cbBomFmtPresets->Delete( idx );
                m_currentBomFmtPreset = nullptr;

                m_bomFmtPresetMRU.Remove( presetName );
            }
        }

        resetSelection();
        return;
    }

    auto* preset = static_cast<BOM_FMT_PRESET*>( m_cbBomFmtPresets->GetClientData( index ) );
    m_currentBomFmtPreset = preset;

    m_lastSelectedBomFmtPreset = ( !preset || preset->readOnly ) ? nullptr : preset;

    if( preset )
    {
        doApplyBomFmtPreset( *preset );
        syncBomFmtPresetSelection();
        m_currentBomFmtPreset = preset;

        if( !m_currentBomFmtPreset->name.IsEmpty() )
        {
            m_bomFmtPresetMRU.Remove( preset->name );
            m_bomFmtPresetMRU.Insert( preset->name, 0 );
        }
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::doApplyBomFmtPreset( const BOM_FMT_PRESET& aPreset )
{
    m_textFieldDelimiter->ChangeValue( aPreset.fieldDelimiter );
    m_textStringDelimiter->ChangeValue( aPreset.stringDelimiter );
    m_textRefDelimiter->ChangeValue( aPreset.refDelimiter );
    m_textRefRangeDelimiter->ChangeValue( aPreset.refRangeDelimiter );
    m_checkKeepTabs->SetValue( aPreset.keepTabs );
    m_checkKeepLineBreaks->SetValue( aPreset.keepLineBreaks );


    // Refresh the preview if that's the current page
    if( m_nbPages->GetSelection() == 1 )
        PreviewRefresh();
}


void DIALOG_SYMBOL_FIELDS_TABLE::savePresetsToSchematic()
{
    bool modified = false;

    // Save our BOM presets
    std::vector<BOM_PRESET> presets;

    for( const std::pair<const wxString, BOM_PRESET>& pair : m_bomPresets )
    {
        if( !pair.second.readOnly )
            presets.emplace_back( pair.second );
    }

    if( m_schSettings.m_BomPresets != presets )
    {
        modified = true;
        m_schSettings.m_BomPresets = presets;
    }

    if( m_schSettings.m_BomSettings != m_dataModel->GetBomSettings() )
    {
        modified = true;
        m_schSettings.m_BomSettings = m_dataModel->GetBomSettings();
    }


    // Save our BOM Format presets
    std::vector<BOM_FMT_PRESET> fmts;

    for( const std::pair<const wxString, BOM_FMT_PRESET>& pair : m_bomFmtPresets )
    {
        if( !pair.second.readOnly )
            fmts.emplace_back( pair.second );
    }

    if( m_schSettings.m_BomFmtPresets != fmts )
    {
        modified = true;
        m_schSettings.m_BomFmtPresets = fmts;
    }

    if( m_schSettings.m_BomFmtSettings != GetCurrentBomFmtSettings() )
    {
        modified = true;
        m_schSettings.m_BomFmtSettings = GetCurrentBomFmtSettings();
    }

    if( modified )
        m_parent->OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchItemsAdded( SCHEMATIC&              aSch,
                                                  std::vector<SCH_ITEM*>& aSchItem )
{
    SCH_REFERENCE_LIST allRefs;
    m_parent->Schematic().Hierarchy().GetSymbols( allRefs );

    for( SCH_ITEM* item : aSchItem )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            // Don't add power symbols
            if( !symbol->IsMissingLibSymbol() && symbol->IsPower() )
                continue;

            // Add all fields again in case this symbol has a new one
            for( SCH_FIELD& field : symbol->GetFields() )
                AddField( field.GetCanonicalName(), field.GetName(), true, false, true );

            m_dataModel->AddReferences( getSymbolReferences( symbol, allRefs ) );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            std::set<SCH_SYMBOL*> symbols;
            SCH_REFERENCE_LIST refs = getSheetSymbolReferences( *static_cast<SCH_SHEET*>( item ) );

            for( SCH_REFERENCE& ref : refs )
                symbols.insert( ref.GetSymbol() );

            for( SCH_SYMBOL* symbol : symbols )
            {
                // Add all fields again in case this symbol has a new one
                for( SCH_FIELD& field : symbol->GetFields() )
                    AddField( field.GetCanonicalName(), field.GetName(), true, false, true );
            }

            m_dataModel->AddReferences( refs );
        }
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    EnableSelectionEvents();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchItemsRemoved( SCHEMATIC&              aSch,
                                                    std::vector<SCH_ITEM*>& aSchItem )
{
    for( SCH_ITEM* item : aSchItem )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            m_dataModel->RemoveSymbol( *static_cast<SCH_SYMBOL*>( item ) );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            m_dataModel->RemoveReferences(
                    getSheetSymbolReferences( *static_cast<SCH_SHEET*>( item ) ) );
        }
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    EnableSelectionEvents();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchItemsChanged( SCHEMATIC&              aSch,
                                                    std::vector<SCH_ITEM*>& aSchItem )
{
    SCH_REFERENCE_LIST allRefs;
    m_parent->Schematic().Hierarchy().GetSymbols( allRefs );

    for( SCH_ITEM* item : aSchItem )
    {
        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            // Don't add power symbols
            if( !symbol->IsMissingLibSymbol() && symbol->IsPower() )
                continue;

            // Add all fields again in case this symbol has a new one
            for( SCH_FIELD& field : symbol->GetFields() )
                AddField( field.GetCanonicalName(), field.GetName(), true, false, true );

            m_dataModel->UpdateReferences( getSymbolReferences( symbol, allRefs ) );
        }
        else if( item->Type() == SCH_SHEET_T )
        {
            std::set<SCH_SYMBOL*> symbols;
            SCH_REFERENCE_LIST refs = getSheetSymbolReferences( *static_cast<SCH_SHEET*>( item ) );

            for( SCH_REFERENCE& ref : refs )
                symbols.insert( ref.GetSymbol() );

            for( SCH_SYMBOL* symbol : symbols )
            {
                // Add all fields again in case this symbol has a new one
                for( SCH_FIELD& field : symbol->GetFields() )
                    AddField( field.GetCanonicalName(), field.GetName(), true, false, true );
            }

            m_dataModel->UpdateReferences( refs );
        }
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    EnableSelectionEvents();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchSheetChanged( SCHEMATIC& aSch )
{
    m_dataModel->SetPath( aSch.CurrentSheet() );

    if( m_dataModel->GetScope() != FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE::SCOPE_ALL )
    {
        DisableSelectionEvents();
        m_dataModel->RebuildRows();
        EnableSelectionEvents();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::EnableSelectionEvents()
{
    m_grid->Connect(
            wxEVT_GRID_RANGE_SELECTED,
            wxGridRangeSelectEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnTableRangeSelected ),
            nullptr, this );
}


void DIALOG_SYMBOL_FIELDS_TABLE::DisableSelectionEvents()
{
    m_grid->Disconnect(
            wxEVT_GRID_RANGE_SELECTED,
            wxGridRangeSelectEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnTableRangeSelected ),
            nullptr, this );
}


SCH_REFERENCE_LIST
DIALOG_SYMBOL_FIELDS_TABLE::getSymbolReferences( SCH_SYMBOL*         aSymbol,
                                                 SCH_REFERENCE_LIST& aCachedRefs )
{
    SCH_REFERENCE_LIST symbolRefs;

    for( size_t i = 0; i < aCachedRefs.GetCount(); i++ )
    {
        SCH_REFERENCE& ref = aCachedRefs[i];

        if( ref.GetSymbol() == aSymbol )
        {
            ref.Split(); // Figures out if we are annotated or not
            symbolRefs.AddItem( ref );
        }
    }

    return symbolRefs;
}


SCH_REFERENCE_LIST DIALOG_SYMBOL_FIELDS_TABLE::getSheetSymbolReferences( SCH_SHEET& aSheet )
{
    SCH_SHEET_LIST     allSheets = m_parent->Schematic().Hierarchy();
    SCH_REFERENCE_LIST sheetRefs;

    // We need to operate on all instances of the sheet
    for( const SCH_SHEET_INSTANCE& instance : aSheet.GetInstances() )
    {
        // For every sheet instance we need to get the current schematic sheet
        // instance that matches that particular sheet path from the root
        for( SCH_SHEET_PATH& basePath : allSheets )
        {
            if( basePath.Path() == instance.m_Path )
            {
                SCH_SHEET_PATH sheetPath = basePath;
                sheetPath.push_back( &aSheet );

                // Create a list of all sheets in this path, starting with the path
                // of the sheet that we just deleted, then all of its subsheets
                SCH_SHEET_LIST subSheets;
                subSheets.push_back( sheetPath );
                allSheets.GetSheetsWithinPath( subSheets, sheetPath );

                subSheets.GetSymbolsWithinPath( sheetRefs, sheetPath, false, false );
                break;
            }
        }
    }

    for( SCH_REFERENCE& ref : sheetRefs )
        ref.Split();

    return sheetRefs;
}
