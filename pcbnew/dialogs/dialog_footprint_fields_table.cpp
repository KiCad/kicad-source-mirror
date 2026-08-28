/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Oliver Walters
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <advanced_config.h>
#include <common.h>
#include <base_units.h>
#include <confirm.h>
#include <eda_doc.h>
#include <pcbnew_settings.h>
#include <board_design_settings.h>
#include <grid_tricks.h>
#include <string_utils.h>
#include <template_fieldnames.h>
#include <kiface_base.h>
#include <pcb_edit_frame.h>
#include <footprint.h>
#include <pcb_group.h>
#include <widgets/wx_infobar.h>
#include <tools/board_editor_control.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <wx/debug.h>
#include <wx/grid.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <dialogs/eda_view_switcher.h>
#include "dialog_footprint_fields_table.h"
#include <footprint_fields_data_model.h>
#include <board_commit.h>
#include <project_pcb.h>
#include <jobs/job_export_bom.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>

wxDEFINE_EVENT( EDA_EVT_CLOSE_DIALOG_FOOTPRINT_FIELDS_TABLE, wxCommandEvent );

using SCOPE = FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE;


enum
{
    MYID_SELECT_FOOTPRINT = FIELDS_TABLE_GRID_TRICKS::FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET,
    MYID_SET_VARIANT_FOOTPRINT,
    MYID_CLEAR_VARIANT_FOOTPRINT,
    MYID_INCLUDE_DNP,
    MYID_INCLUDE_EXCLUDED_FROM_BOM,
    MYID_HIGHLIGHT_ON_CROSS_PROBE,
    MYID_SELECT_ON_CROSS_PROBE
};

class FOOTPRINT_FIELDS_EDITOR_GRID_TRICKS : public FIELDS_TABLE_GRID_TRICKS
{
public:
    FOOTPRINT_FIELDS_EDITOR_GRID_TRICKS( DIALOG_FOOTPRINT_FIELDS_TABLE* aParent, WX_GRID* aGrid,
                                         VIEW_CONTROLS_GRID_DATA_MODEL*           aViewFieldsData,
                                         FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL* aDataModel,
                                         EMBEDDED_FILES* aFiles ) :
            FIELDS_TABLE_GRID_TRICKS( aParent, aGrid, aDataModel ),
            m_dlg( aParent ),
            m_viewControlsDataModel( aViewFieldsData ),
            m_dataModel( aDataModel ),
            m_files( aFiles )
    {}

protected:
    bool toggleCell( int aRow, int aCol, bool aPreserveSelection = false ) override
    {
        if( !m_grid->IsEditable() || m_dataModel->IsCellReadOnly( aRow, aCol ) )
            return false;

        return GRID_TRICKS::toggleCell( aRow, aCol, aPreserveSelection );
    }

    void showFieldsTablePopupMenu( wxMenu& aMenu, wxGridEvent& aEvent ) override
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( row >= 0 && col >= 0 )
        {
            if( m_dataModel->GetColFieldName( col ) == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
            {
                aMenu.Append( MYID_SHOW_DATASHEET, _( "Show Datasheet" ), _( "Show datasheet in browser" ) );
                aMenu.AppendSeparator();
            }
        }

        GRID_TRICKS::showPopupMenu( aMenu, aEvent );
    }

    void doFieldsTablePopupSelection( wxCommandEvent& aEvent ) override
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( aEvent.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheetUri = m_grid->GetCellValue( row, col );
            GetAssociatedDocument( m_dlg, datasheetUri, &m_dlg->Prj(), nullptr, { m_files } );
        }
        else if( aEvent.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE )
        {
            if( !m_grid->CommitPendingChanges( false ) )
                return;

            // Pop-up column order is the order of the shown fields, not the viewControls order
            col = aEvent.GetId() - GRIDTRICKS_FIRST_SHOWHIDE;

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
            GRID_TRICKS::doPopupSelection( aEvent );
        }
    }

private:
    DIALOG_FOOTPRINT_FIELDS_TABLE*           m_dlg;
    VIEW_CONTROLS_GRID_DATA_MODEL*           m_viewControlsDataModel;
    FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
    EMBEDDED_FILES*                          m_files;
};


DIALOG_FOOTPRINT_FIELDS_TABLE::DIALOG_FOOTPRINT_FIELDS_TABLE( PCB_EDIT_FRAME* aParent, JOB_EXPORT_BOM* aJob ) :
        DIALOG_FIELDS_TABLE( aParent, aParent->GetPcbNewSettings()->m_FieldEditorPanel,
                             aParent->GetBoard()->GetDesignSettings(), aJob ),
        m_parent( aParent )
{
    // Get all footprints from the list of board sheets
    for( FOOTPRINT* footprint : m_parent->GetBoard()->Footprints() )
        m_footprintsList.emplace_back( *footprint );

    m_dataModel = new FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL( m_footprintsList );

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // The field-list grid regroups its rows, so the dialog's position-based Ctrl+Z would shift
    // values onto the wrong field.
    ExcludeFromControlUndoRedo( m_viewControlsGrid );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new FOOTPRINT_FIELDS_EDITOR_GRID_TRICKS(
            this, m_grid, m_viewControlsDataModel, m_dataModel, m_parent->GetBoard()->GetEmbeddedFiles() ) );

    m_variantListBox->Set( m_parent->GetBoard()->GetVariantNamesForUI() );

    // A job keeps its own variant, otherwise follow the board.
    wxString variantToSelect;

    if( m_job )
        variantToSelect = m_job->GetSelectedVariant();
    else
        variantToSelect = m_parent->GetBoard()->GetCurrentVariant();

    if( !variantToSelect.IsEmpty() )
    {
        int toSelect = m_variantListBox->FindString( variantToSelect );

        if( toSelect == wxNOT_FOUND )
            m_variantListBox->SetSelection( 0 );
        else
            m_variantListBox->SetSelection( toSelect );
    }
    else
    {
        m_variantListBox->SetSelection( 0 );
    }

    updateVariantButtonStates();

    if( m_job )
        SetTitle( m_job->GetSettingsDialogTitle() );
    else
        SetTitle( _( "Footprint Fields Table" ) );

    m_buttonApply->SetLabel( _( "Apply, Save Board && Continue" ) );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions (which have different sizes).
    m_hash_key = TO_UTF8( GetTitle() );

    // Set the current variant for highlighting variant-specific field values
    m_dataModel->SetCurrentVariant( resolveVariant() );

    // Disable variant editing controls in for footprint fields table, variants come only from the schematic
    m_addVariantButton->Hide();
    m_copyVariantButton->Hide();
    m_renameVariantButton->Hide();
    m_editVariantDescButton->Hide();
    m_deleteVariantButton->Hide();

    SetInitialFocus( m_grid );
    m_grid->ClearSelection();

    SetupStandardButtons();

    finishDialogSettings();

    SetSize( GetDefaultDialogSize() );

    RestorePanelLayout();

    OptOut( m_outputFileName );

    if( m_job )
        m_outputFileName->SetValue( m_job->GetConfiguredOutputPath() );
    else
        m_outputFileName->SetValue( m_cfgBomSettings.m_BomExportFileName );

    Center();

    // Connect Events
    m_grid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnColSort, this );
    m_grid->Bind( wxEVT_GRID_COL_MOVE, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnColMove, this );
    m_grid->GetGridWindow()->Bind( wxEVT_MOTION, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnGridMouseMove, this );
    m_cbBomPresets->Bind( wxEVT_CHOICE, &DIALOG_FOOTPRINT_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Bind( wxEVT_CHOICE, &DIALOG_FOOTPRINT_FIELDS_TABLE::onBomFmtPresetChanged, this );

    if( !m_job )
    {
        // Start listening for board changes
        m_parent->GetBoard()->AddListener( this );
        m_parent->Bind( EDA_EVT_PCB_LAST_SCH_SHEET_CHANGED,
                        &DIALOG_FOOTPRINT_FIELDS_TABLE::OnCurrentSchematicSheetChanged, this );
    }
    else
    {
        // Don't allow editing
        m_grid->EnableEditing( false );
        m_buttonApply->Hide();
        m_buttonExport->Hide();
    }
}


DIALOG_FOOTPRINT_FIELDS_TABLE::~DIALOG_FOOTPRINT_FIELDS_TABLE()
{
    if( m_aborted )
        return;

    if( !m_job )
    {
        m_parent->Unbind( EDA_EVT_PCB_LAST_SCH_SHEET_CHANGED,
                          &DIALOG_FOOTPRINT_FIELDS_TABLE::OnCurrentSchematicSheetChanged, this );
    }

    if( savePresets( !m_job ) )
    {
        m_parent->OnModify();
    }

    SavePanelLayout();
    SaveColumnWidths();

    // Disconnect Events
    m_grid->GetGridWindow()->Unbind( wxEVT_MOTION, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnGridMouseMove, this );
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnColSort, this );
    m_grid->Unbind( wxEVT_GRID_COL_MOVE, &DIALOG_FOOTPRINT_FIELDS_TABLE::OnColMove, this );
    m_cbBomPresets->Unbind( wxEVT_CHOICE, &DIALOG_FOOTPRINT_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Unbind( wxEVT_CHOICE, &DIALOG_FOOTPRINT_FIELDS_TABLE::onBomFmtPresetChanged, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // we gave ownership of m_viewControlsDataModel & m_dataModel to the wxGrids...
}


wxGridCellEditor* DIALOG_FOOTPRINT_FIELDS_TABLE::createDatasheetEditor()
{
    return new GRID_CELL_URL_EDITOR( this, nullptr, { m_parent->GetBoard() } );
}


bool DIALOG_FOOTPRINT_FIELDS_TABLE::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    LoadFieldNames(); // loads rows into m_viewControlsDataModel and columns into m_dataModel

    // Load our BOM view presets
    SetUserBomPresets( m_cfgBomSettings.m_BomPresets );

    BOM_PRESET preset = m_cfgBomSettings.m_BomSettings;

    if( m_job )
        loadJobBomPreset( *m_job, preset );

    ApplyBomPreset( preset );
    syncBomPresetSelection();

    // Load BOM export format presets
    SetUserBomFmtPresets( m_cfgBomSettings.m_BomFmtPresets );
    BOM_FMT_PRESET fmtPreset = m_cfgBomSettings.m_BomFmtSettings;

    if( m_job )
        loadJobBomFmtPreset( *m_job, fmtPreset );

    ApplyBomFmtPreset( fmtPreset );
    syncBomFmtPresetSelection();

    if( !m_job )
        m_outputFileName->SetValue( m_cfgBomSettings.m_BomExportFileName );

    TOOL_MANAGER*       toolManager = m_parent->GetToolManager();
    PCB_SELECTION_TOOL* selectionTool = toolManager->GetTool<PCB_SELECTION_TOOL>();
    PCB_SELECTION&      selection = selectionTool->GetSelection();
    FOOTPRINT*          footprint = nullptr;

    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );

    setScope( static_cast<SCOPE>( m_scope->GetSelection() ) );

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = selection.Front();

        if( item->Type() == PCB_FOOTPRINT_T )
            footprint = static_cast<FOOTPRINT*>( item );
        else if( item->GetParent() && item->GetParent()->Type() == PCB_FOOTPRINT_T )
            footprint = static_cast<FOOTPRINT*>( item->GetParent() );
    }

    if( footprint )
    {
        for( int row = 0; row < m_dataModel->GetNumberRows(); ++row )
        {
            std::vector<FOOTPRINT_REF> references = m_dataModel->GetRowReferences( row );
            bool                       found = false;

            for( const FOOTPRINT_REF& ref : references )
            {
                if( &ref.GetFootprint() == footprint )
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


bool DIALOG_FOOTPRINT_FIELDS_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( m_job )
    {
        // and exit, don't even dream of saving changes from the data model
        return true;
    }

    BOARD_COMMIT commit( m_parent );
    wxString     currentVariant = m_parent->GetBoard()->GetCurrentVariant();

    // TODO: board settings don't have template field names, ideally we would sync these from schematic
    // or more likely move them up the project level since they should be the same for symbols/footprints.
    TEMPLATES notImplemented;
    m_dataModel->ApplyData( commit, notImplemented, currentVariant );

    if( !commit.Empty() )
    {
        commit.Push( wxS( "Footprint Fields Table Edit" ) ); // Push clears the commit buffer.
        m_parent->OnModify();
    }

    // Reset the view to where we left the user
    m_parent->Refresh();

    return true;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::LoadFieldNames()
{
    auto addMandatoryField =
            [&]( FIELD_T aFieldId, bool aShow, bool aGroupBy )
            {
                m_mandatoryFieldListIndexes[aFieldId] = m_viewControlsDataModel->GetNumberRows();

                AddField( GetCanonicalFieldName( aFieldId ), GetDefaultFieldName( aFieldId, DO_TRANSLATE ),
                          aShow, aGroupBy );
            };

    // Add mandatory fields first            show   groupBy
    addMandatoryField( FIELD_T::REFERENCE,   true,   true   );
    addMandatoryField( FIELD_T::VALUE,       true,   true   );
    addMandatoryField( FIELD_T::FOOTPRINT,   true,   true   );
    addMandatoryField( FIELD_T::DATASHEET,   true,   false  );
    addMandatoryField( FIELD_T::DESCRIPTION, false,  false  );

    // Generated fields present only in the fields table
    AddField( FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE, _( "Qty" ), true, false );
    AddField( FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE, _( "#" ), true, false );

    // User field names are stored and matched case-sensitively (see issue #24021), so each
    // distinct name gets its own column rather than collapsing case variants together.
    std::set<wxString> userFieldNames;

    for( const FOOTPRINT_REF& ref : m_footprintsList )
    {
        FOOTPRINT& footprint = ref.GetFootprint();

        for( const PCB_FIELD* field : footprint.GetFields() )
        {
            if( !field->IsMandatory() && !field->IsPrivate() )
                userFieldNames.insert( field->GetName() );
        }
    }

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false );

    // Add any templateFieldNames which aren't already present.
    // TODO: no template fieldnames in board settings
    TEMPLATES notImplemented;
    for( const TEMPLATE_FIELDNAME& templateField : notImplemented.GetResolvedTemplateFieldNames() )
    {
        if( userFieldNames.count( templateField.m_Name ) == 0 )
            AddField( templateField.m_Name, GetGeneratedFieldDisplayName( templateField.m_Name ), false, false );
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::setScope( SCOPE aScope )
{
    m_dataModel->SetPath( m_parent->GetLastSchematicSheetPath() );
    m_dataModel->SetScope( aScope );

    if( aScope == SCOPE::SCOPE_SELECTION )
        updateSelectionItems();

    m_dataModel->RebuildRows();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::updateSelectionItems()
{
    std::unordered_set<KIID_PATH> selectionItems;
    PCB_SELECTION_TOOL*           selectionTool = m_parent->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION&          selection = selectionTool->GetSelection();

    auto addSelectionItem = [&]( EDA_ITEM* aItem )
    {
        FOOTPRINT* footprint = nullptr;

        if( aItem->Type() == PCB_FOOTPRINT_T )
            footprint = static_cast<FOOTPRINT*>( aItem );
        else if( aItem->GetParent() && aItem->GetParent()->Type() == PCB_FOOTPRINT_T )
            footprint = static_cast<FOOTPRINT*>( aItem->GetParent() );

        if( footprint )
        {
            KIID_PATH path;
            path.push_back( footprint->m_Uuid );
            selectionItems.insert( path );
        }
    };

    for( EDA_ITEM* item : selection )
    {
        if( item->Type() == PCB_GROUP_T )
            static_cast<PCB_GROUP*>( item )->RunOnChildren( addSelectionItem, RECURSE_MODE::RECURSE );
        else
            addSelectionItem( item );
    }

    m_dataModel->SetSelectionItems( selectionItems );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnScope( wxCommandEvent& aEvent )
{
    switch( aEvent.GetSelection() )
    {
    case 0: setScope( SCOPE::SCOPE_ALL );             break;
    case 1: setScope( SCOPE::SCOPE_SHEET );           break;
    case 2: setScope( SCOPE::SCOPE_SHEET_RECURSIVE ); break;
    case 3: setScope( SCOPE::SCOPE_SELECTION );       break;
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnMenu( wxCommandEvent& aEvent )
{
    // Build a pop menu:
    wxMenu menu;

    menu.Append( MYID_INCLUDE_DNP, _( "Include 'DNP' Footprints" ),
                 _( "Show footprints marked 'DNP' in the table.  This setting also controls whether or not 'DNP' "
                    "footprints are included on export." ),
                 wxITEM_CHECK );
    menu.Check( MYID_INCLUDE_DNP, !m_dataModel->GetExcludeDNP() );

    menu.Append( MYID_INCLUDE_EXCLUDED_FROM_BOM, _( "Include 'Exclude from BOM' Footprints" ),
                 _( "Show footprints marked 'Exclude from BOM' in the table.  Footprints marked 'Exclude from BOM' "
                    "are never included on export." ),
                 wxITEM_CHECK );
    menu.Check( MYID_INCLUDE_EXCLUDED_FROM_BOM, m_dataModel->GetIncludeExcludedFromBOM() );

    menu.AppendSeparator();

    menu.Append( MYID_HIGHLIGHT_ON_CROSS_PROBE, _( "Highlight on Cross-probe" ),
                 _( "Highlight corresponding item on canvas when it is selected in the table" ),
                 wxITEM_CHECK );
    menu.Check( MYID_HIGHLIGHT_ON_CROSS_PROBE, m_cfgDialogSettings.selection_mode == 0 );

    menu.Append( MYID_SELECT_ON_CROSS_PROBE, _( "Select on Cross-probe" ),
                 _( "Select corresponding item on canvas when it is selected in the table" ),
                 wxITEM_CHECK );
    menu.Check( MYID_SELECT_ON_CROSS_PROBE, m_cfgDialogSettings.selection_mode == 1 );

    // menuId is the selected submenu id from the popup menu or wxID_NONE
    int menuId = m_bMenu->GetPopupMenuSelectionFromUser( menu );

    if( menuId == 0 || menuId == MYID_INCLUDE_DNP )
    {
        m_dataModel->SetExcludeDNP( !m_dataModel->GetExcludeDNP() );
        m_dataModel->RebuildRows();
        m_grid->ForceRefresh();

        syncBomPresetSelection();
    }
    else if( menuId == 1 || menuId == MYID_INCLUDE_EXCLUDED_FROM_BOM )
    {
        m_dataModel->SetIncludeExcludedFromBOM( !m_dataModel->GetIncludeExcludedFromBOM() );
        m_dataModel->RebuildRows();
        m_grid->ForceRefresh();

        syncBomPresetSelection();
    }
    else if( menuId == 3 || menuId == MYID_HIGHLIGHT_ON_CROSS_PROBE )
    {
        if( m_cfgDialogSettings.selection_mode != 0 )
            m_cfgDialogSettings.selection_mode = 0;
        else
            m_cfgDialogSettings.selection_mode = 2;
    }
    else if( menuId == 4 || menuId == MYID_SELECT_ON_CROSS_PROBE )
    {
        if( m_cfgDialogSettings.selection_mode != 1 )
            m_cfgDialogSettings.selection_mode = 1;
        else
            m_cfgDialogSettings.selection_mode = 2;
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnTableSelectionChanged( const std::set<int>& aRows )
{
    // Cross-probing should only work in Edit page
    if( m_nbPages->GetSelection() != 0 )
        return;

    // Cross-probing is disabled when we're in selection scope mode, otherwise
    // we're just in a loop
    if( m_dataModel->GetScope() == SCOPE::SCOPE_SELECTION )
        return;

    // Multi-select can grab the rows that are expanded child refs, and also the row
    // containing the list of all child refs. Make sure we add refs/footprints uniquely
    std::set<BOARD_ITEM*> footprints;

    for( int row : aRows )
    {
        for( const FOOTPRINT_REF& ref : m_dataModel->GetRowReferences( row ) )
            footprints.insert( &ref.GetFootprint() );
    }

    std::vector<BOARD_ITEM*> focusItems( footprints.begin(), footprints.end() );

    if( m_cfgDialogSettings.selection_mode == 0 )
    {
        m_parent->FocusOnItems( focusItems );
    }
    else if( m_cfgDialogSettings.selection_mode == 1 )
    {
        m_parent->GetToolManager()->RunAction( PCB_ACTIONS::syncSelection, &focusItems );
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnSaveAndContinue( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
    {
        m_cfgBomSettings.m_BomExportFileName = m_outputFileName->GetValue();
        m_parent->SaveBoard();
        ClearModify();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnCancel( wxCommandEvent& aEvent )
{
    if( m_job )
    {
        EndModal( wxID_CANCEL );
    }
    else
    {
        // Discard any unsaved edit in the output filename field
        m_outputFileName->SetValue( m_cfgBomSettings.m_BomExportFileName );
        Close();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnOk( wxCommandEvent& aEvent )
{
    if( !TransferDataFromWindow() )
        return;

    if( m_job )
    {
        saveJobSettings( *m_job );
        EndModal( wxID_OK );
    }
    else
    {
        if( m_cfgBomSettings.m_BomExportFileName != m_outputFileName->GetValue() )
        {
            m_cfgBomSettings.m_BomExportFileName = m_outputFileName->GetValue();
            m_parent->OnModify();
        }

        Close();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnClose( wxCloseEvent& aEvent )
{
    if( m_job )
    {
        aEvent.Skip();
        return;
    }

    m_grid->CommitPendingChanges( true );

    if( m_dataModel->IsEdited() && aEvent.CanVeto() )
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

    // Stop listening to board events
    m_parent->GetBoard()->RemoveListener( this );
    m_parent->ClearFocus();

    wxCommandEvent* event = new wxCommandEvent( EDA_EVT_CLOSE_DIALOG_FOOTPRINT_FIELDS_TABLE, wxID_ANY );

    if( wxWindow* parentWindow = GetParent() )
        wxQueueEvent( parentWindow, event );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnBoardItemsAdded( BOARD& aPcb, std::vector<BOARD_ITEM*>& aPcbItem )
{
    FOOTPRINT_REFERENCE_LIST addedRefs;

    for( BOARD_ITEM* item : aPcbItem )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            addedRefs.push_back( FOOTPRINT_REF( *static_cast<FOOTPRINT*>( item ) ) );
    }

    bool selectionScope = m_dataModel->GetScope() == SCOPE::SCOPE_SELECTION;

    if( addedRefs.empty() && !selectionScope )
        return;

    std::set<KIID_PATH> savedSelection = SaveGridSelection();

    for( FOOTPRINT_REF& ref : addedRefs )
    {
        // Add all fields again in case this footprint has a new one
        for( PCB_FIELD* field : ref.GetFootprint().GetFields() )
        {
            if( !field->IsMandatory() && !field->IsPrivate() )
                AddField( field->GetCanonicalName(), field->GetName(), true, false, true );
        }
    }


    m_dataModel->AddReferences( addedRefs );
    rebuildRowsPreservingSelection( savedSelection );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnBoardItemsRemoved( BOARD& aPcb, std::vector<BOARD_ITEM*>& aPcbItem )
{
    std::set<KIID_PATH> savedSelection = SaveGridSelection();

    for( BOARD_ITEM* item : aPcbItem )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            m_dataModel->RemoveFootprint( FOOTPRINT_REF( *static_cast<FOOTPRINT*>( item ) ) );
    }

    rebuildRowsPreservingSelection( savedSelection );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnBoardItemsChanged( BOARD& aPcb, std::vector<BOARD_ITEM*>& aPcbItem )
{
    FOOTPRINT_REFERENCE_LIST changedRefs;

    for( BOARD_ITEM* item : aPcbItem )
    {
        if( item->Type() == PCB_FOOTPRINT_T )
            changedRefs.push_back( FOOTPRINT_REF( *static_cast<FOOTPRINT*>( item ) ) );
    }

    bool selectionScope = m_dataModel->GetScope() == SCOPE::SCOPE_SELECTION;

    if( changedRefs.empty() && !selectionScope )
        return;

    std::set<KIID_PATH> savedSelection = SaveGridSelection();

    for( FOOTPRINT_REF& ref : changedRefs )
    {
        // Add all fields again in case this footprint has a new one
        for( PCB_FIELD* field : ref.GetFootprint().GetFields() )
        {
            if( !field->IsMandatory() && !field->IsPrivate() )
                AddField( field->GetCanonicalName(), field->GetName(), true, false, true );
        }
    }


    m_dataModel->UpdateReferences( changedRefs );
    rebuildRowsPreservingSelection( savedSelection );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnCurrentSchematicSheetChanged( wxCommandEvent& aEvent )
{
    wxUnusedVar( aEvent );

    m_dataModel->SetPath( m_parent->GetLastSchematicSheetPath() );

    if( m_dataModel->GetScope() != SCOPE::SCOPE_ALL )
        rebuildRowsPreservingSelection();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::OnBoardSelectionChanged( BOARD& aPcb )
{
    if( m_dataModel->GetScope() == SCOPE::SCOPE_SELECTION )
        rebuildRowsPreservingSelection();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::rebuildRowsPreservingSelection()
{
    rebuildRowsPreservingSelection( SaveGridSelection() );
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::rebuildRowsPreservingSelection( const std::set<KIID_PATH>& aSavedSelection )
{
    DisableSelectionEvents();

    if( m_dataModel->GetScope() == SCOPE::SCOPE_SELECTION )
        updateSelectionItems();

    m_dataModel->RebuildRows();
    RestoreGridSelection( aSavedSelection );

    EnableSelectionEvents();
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onAddVariant( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onDeleteVariant( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onRenameVariant( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onCopyVariant( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onEditVariantDescription( wxCommandEvent& aEvent )
{
    return;
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::onVariantSelectionChange( wxCommandEvent& aEvent )
{
    wxString currentVariant;
    wxString selectedVariant = getSelectedVariant();

    updateVariantButtonStates();

    if( m_job )
    {
        m_grid->CommitPendingChanges( true );

        if( m_parent )
            m_parent->SetCurrentVariant( selectedVariant );

        m_dataModel->SetCurrentVariant( selectedVariant );
        m_dataModel->UpdateReferences( m_dataModel->GetReferenceList() );
        m_dataModel->RebuildRows();

        if( m_nbPages->GetSelection() == 1 )
            PreviewRefresh();
        else
            m_grid->ForceRefresh();

        syncBomFmtPresetSelection();
        return;
    }

    if( m_parent )
    {
        currentVariant = m_parent->GetBoard()->GetCurrentVariant();

        if( currentVariant != selectedVariant )
            m_parent->SetCurrentVariant( selectedVariant );
    }

    // TODO: this is probably the wrong method in both the symbol and footprint fields tables,
    // changing the variant should ask the user whether to apply the changes to the board.
    // The rest of the time in the fields table, no data is pushed to the sch/board until the user
    // explicity clicks Apply or Ok.
    if( currentVariant != selectedVariant )
    {
        m_grid->CommitPendingChanges( true );

        BOARD_COMMIT commit( m_parent );

        TEMPLATES notImplemented;
        m_dataModel->ApplyData( commit, notImplemented, currentVariant );

        if( !commit.Empty() )
        {
            commit.Push( wxS( "Footprint Fields Table Edit" ) ); // Push clears the commit buffer.
            m_parent->OnModify();
        }

        // Update the data model's current variant for field highlighting
        m_dataModel->SetCurrentVariant( selectedVariant );
        m_dataModel->UpdateReferences( m_dataModel->GetReferenceList() );
        m_dataModel->RebuildRows();

        if( m_nbPages->GetSelection() == 1 )
            PreviewRefresh();
        else
            m_grid->ForceRefresh();

        syncBomFmtPresetSelection();
    }
}


void DIALOG_FOOTPRINT_FIELDS_TABLE::updateVariantButtonStates()
{
    // All variant modifications are disabled for the footprint fields table
    m_addVariantButton->Enable( false );
    m_copyVariantButton->Enable( false );
    m_renameVariantButton->Enable( false );
    m_editVariantDescButton->Enable( false );
    m_deleteVariantButton->Enable( false );
}


wxString DIALOG_FOOTPRINT_FIELDS_TABLE::resolveVariant() const
{
    // A job keeps its own variant, otherwise follow the board.
    if( m_job )
        return getSelectedVariant();

    return m_parent->GetBoard()->GetCurrentVariant();
}


bool DIALOG_FOOTPRINT_FIELDS_TABLE::resolveTextVar( wxString* aToken ) const
{
    return m_parent->GetBoard()->ResolveTextVar( aToken, 0 );
}
