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
#include <schematic_settings.h>
#include <general.h>
#include <grid_tricks.h>
#include <string_utils.h>
#include <template_fieldnames.h>
#include <kiface_base.h>
#include <sch_edit_frame.h>
#include <widgets/wx_infobar.h>
#include <sch_reference_list.h>
#include <tools/sch_editor_control.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <wx/debug.h>
#include <wx/grid.h>
#include <wx/textdlg.h>
#include <wx/msgdlg.h>
#include <dialogs/eda_view_switcher.h>
#include "dialog_symbol_fields_table.h"
#include "dialog_resolve_field_case_conflicts.h"
#include <symbol_fields_data_model.h>
#include <project_sch.h>
#include <jobs/job_export_bom.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>
#include <sch_sheet_path.h>

wxDEFINE_EVENT( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, wxCommandEvent );

using SCOPE = SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE;


enum
{
    MYID_SELECT_FOOTPRINT = GRIDTRICKS_FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET,
    MYID_SET_VARIANT_SYMBOL,
    MYID_CLEAR_VARIANT_SYMBOL
};

class FIELDS_EDITOR_GRID_TRICKS : public GRID_TRICKS
{
public:
    FIELDS_EDITOR_GRID_TRICKS( DIALOG_SYMBOL_FIELDS_TABLE* aParent, WX_GRID* aGrid,
                               VIEW_CONTROLS_GRID_DATA_MODEL*        aViewFieldsData,
                               SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL* aDataModel, EMBEDDED_FILES* aFiles ) :
            GRID_TRICKS( aGrid ),
            m_dlg( aParent ),
            m_viewControlsDataModel( aViewFieldsData ),
            m_dataModel( aDataModel ),
            m_files( aFiles )
    {}

protected:
    void showPopupMenu( wxMenu& menu, wxGridEvent& aEvent ) override
    {
        int col = m_grid->GetGridCursorCol();

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

        SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_dlg->GetParent() );

        if( frame && !frame->Schematic().GetCurrentVariant().IsEmpty() )
        {
            int row = m_grid->GetGridCursorRow();
            std::vector<SCH_REFERENCE> refs = m_dataModel->GetRowReferences( row );

            if( refs.size() == 1 && refs[0].GetSymbol() )
            {
                menu.AppendSeparator();
                menu.Append( MYID_SET_VARIANT_SYMBOL, _( "Set Variant Symbol..." ) );

                const SCH_SYMBOL* sym = refs[0].GetSymbol();
                wxString variantName = frame->Schematic().GetCurrentVariant();
                auto variant = sym->GetVariant( refs[0].GetSheetPath(), variantName );

                if( variant && variant->m_SymbolOverride )
                    menu.Append( MYID_CLEAR_VARIANT_SYMBOL, _( "Clear Variant Symbol" ) );
            }
        }

        GRID_TRICKS::showPopupMenu( menu, aEvent );
    }

    void doPopupSelection( wxCommandEvent& event ) override
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( event.GetId() == MYID_SELECT_FOOTPRINT )
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
            GetAssociatedDocument( m_dlg, datasheet_uri, &m_dlg->Prj(), PROJECT_SCH::SchSearchS( &m_dlg->Prj() ),
                                   { m_files } );
        }
        else if( event.GetId() == MYID_SET_VARIANT_SYMBOL
                 || event.GetId() == MYID_CLEAR_VARIANT_SYMBOL )
        {
            SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_dlg->GetParent() );

            if( !frame )
                return;

            std::vector<SCH_REFERENCE> refs = m_dataModel->GetRowReferences( row );

            if( refs.size() != 1 || !refs[0].GetSymbol() )
                return;

            SCH_SELECTION_TOOL* selTool =
                    frame->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
            std::vector<SCH_ITEM*> items = { refs[0].GetSymbol() };
            selTool->SyncSelection( refs[0].GetSheetPath(), nullptr, items );

            if( event.GetId() == MYID_SET_VARIANT_SYMBOL )
                frame->GetToolManager()->RunAction( SCH_ACTIONS::setVariantSymbol );
            else
                frame->GetToolManager()->RunAction( SCH_ACTIONS::clearVariantSymbol );
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

private:
    DIALOG_SYMBOL_FIELDS_TABLE*           m_dlg;
    VIEW_CONTROLS_GRID_DATA_MODEL*        m_viewControlsDataModel;
    SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
    EMBEDDED_FILES*                       m_files;
};


DIALOG_SYMBOL_FIELDS_TABLE::DIALOG_SYMBOL_FIELDS_TABLE( SCH_EDIT_FRAME* parent, JOB_EXPORT_BOM* aJob ) :
        DIALOG_FIELDS_TABLE( parent, parent->eeconfig()->m_FieldEditorPanel, parent->Schematic().Settings() ),
        m_parent( parent ),
        m_schSettings( parent->Schematic().Settings() ),
        m_job( aJob )
{
    // Get all symbols from the list of schematic sheets
    m_parent->Schematic().Hierarchy().GetSymbols( m_symbolsList, SYMBOL_FILTER_NON_POWER );

    if( auto conflicts = DetectFieldCaseConflicts( m_symbolsList ); !conflicts.empty() )
    {
        DIALOG_RESOLVE_FIELD_CASE_CONFLICTS resolver( this, m_parent, std::move( conflicts ) );

        if( resolver.ShowModal() != wxID_OK )
        {
            m_aborted = true;
            return;
        }

        m_symbolsList.Clear();
        m_parent->Schematic().Hierarchy().GetSymbols( m_symbolsList, SYMBOL_FILTER_NON_POWER );
    }

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( createDatasheetEditor() );
    m_dataModel = new SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL( m_symbolsList, attr );

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // The field-list grid regroups its rows, so the dialog's position-based Ctrl+Z would shift
    // values onto the wrong field.
    ExcludeFromControlUndoRedo( m_viewControlsGrid );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_viewControlsDataModel, m_dataModel,
                                                             &m_parent->Schematic() ) );

    m_variantListBox->Set( parent->Schematic().GetVariantNamesForUI() );

    // A job keeps its own variant, otherwise follow the schematic.
    wxString variantToSelect;

    if( m_job )
        variantToSelect = m_job->GetSelectedVariant();
    else
        variantToSelect = m_parent->Schematic().GetCurrentVariant();

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
        SetTitle( _( "Symbol Fields Table" ) );

    // DIALOG_SHIM needs a unique hash_key because classname will be the same for both job and
    // non-job versions (which have different sizes).
    m_hash_key = TO_UTF8( GetTitle() );

    // Set the current variant for highlighting variant-specific field values
    m_dataModel->SetCurrentVariant( resolveVariant() );

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
    m_grid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_SYMBOL_FIELDS_TABLE::OnColSort, this );
    m_grid->Bind( wxEVT_GRID_COL_MOVE, &DIALOG_SYMBOL_FIELDS_TABLE::OnColMove, this );
    m_grid->GetGridWindow()->Bind( wxEVT_MOTION, &DIALOG_SYMBOL_FIELDS_TABLE::OnGridMouseMove, this );
    m_cbBomPresets->Bind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Bind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomFmtPresetChanged, this );

    if( !m_job )
    {
        // Start listening for schematic changes
        m_parent->Schematic().AddListener( this );
    }
    else
    {
        // Don't allow editing
        m_grid->EnableEditing( false );
        m_buttonApply->Hide();
        m_buttonExport->Hide();
    }
}


DIALOG_SYMBOL_FIELDS_TABLE::~DIALOG_SYMBOL_FIELDS_TABLE()
{
    if( m_aborted )
        return;

    if( savePresets( !m_job ) )
    {
        m_parent->OnModify();
    }

    SavePanelLayout();
    SaveColumnWidths();

    // Disconnect Events
    m_grid->GetGridWindow()->Unbind( wxEVT_MOTION, &DIALOG_SYMBOL_FIELDS_TABLE::OnGridMouseMove, this );
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_SYMBOL_FIELDS_TABLE::OnColSort, this );
    m_grid->Unbind( wxEVT_GRID_COL_MOVE, &DIALOG_SYMBOL_FIELDS_TABLE::OnColMove, this );
    m_cbBomPresets->Unbind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Unbind( wxEVT_CHOICE, &DIALOG_SYMBOL_FIELDS_TABLE::onBomFmtPresetChanged, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // we gave ownership of m_viewControlsDataModel & m_dataModel to the wxGrids...
}


wxGridCellEditor* DIALOG_SYMBOL_FIELDS_TABLE::createDatasheetEditor()
{
    return new GRID_CELL_URL_EDITOR( this, PROJECT_SCH::SchSearchS( &Prj() ), { &m_parent->Schematic() } );
}


bool DIALOG_SYMBOL_FIELDS_TABLE::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    LoadFieldNames();   // loads rows into m_viewControlsDataModel and columns into m_dataModel

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

    TOOL_MANAGER*       toolMgr = m_parent->GetToolManager();
    SCH_SELECTION_TOOL* selectionTool = toolMgr->GetTool<SCH_SELECTION_TOOL>();
    SCH_SELECTION&      selection = selectionTool->GetSelection();
    SCH_SYMBOL*         symbol = nullptr;

    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );

    setScope( static_cast<SCOPE>( m_scope->GetSelection() ) );

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

    if( m_job )
    {
        // and exit, don't even dream of saving changes from the data model
        return true;
    }

    SCH_COMMIT     commit( m_parent );
    SCH_SHEET_PATH currentSheet = m_parent->GetCurrentSheet();
    wxString       currentVariant = m_parent->Schematic().GetCurrentVariant();

    m_dataModel->ApplyData( commit, m_schSettings.m_TemplateFieldNames, currentVariant );

    if( !commit.Empty() )
    {
        commit.Push( wxS( "Symbol Fields Table Edit" ) );  // Push clears the commit buffer.
        m_parent->OnModify();
    }

    // Reset the view to where we left the user
    m_parent->SetCurrentSheet( currentSheet );
    m_parent->SyncView();
    m_parent->Refresh();

    return true;
}


void DIALOG_SYMBOL_FIELDS_TABLE::LoadFieldNames()
{
    auto addMandatoryField =
            [&]( FIELD_T fieldId, bool show, bool groupBy )
            {
                m_mandatoryFieldListIndexes[fieldId] = m_viewControlsDataModel->GetNumberRows();

                AddField( GetCanonicalFieldName( fieldId ), GetDefaultFieldName( fieldId, DO_TRANSLATE ),
                          show, groupBy );
            };

    // Add mandatory fields first            show   groupBy
    addMandatoryField( FIELD_T::REFERENCE,   true,   true   );
    addMandatoryField( FIELD_T::VALUE,       true,   true   );
    addMandatoryField( FIELD_T::FOOTPRINT,   true,   true   );
    addMandatoryField( FIELD_T::DATASHEET,   true,   false  );
    addMandatoryField( FIELD_T::DESCRIPTION, false,  false  );

    // Generated fields present only in the fields table
    AddField( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::QUANTITY_VARIABLE, _( "Qty" ), true, false );
    AddField( SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::ITEM_NUMBER_VARIABLE, _( "#" ), true, false );

    // User field names are stored and matched case-sensitively (see issue #24021), so each
    // distinct name gets its own column rather than collapsing case variants together.
    std::set<wxString> userFieldNames;

    for( int ii = 0; ii < (int) m_symbolsList.GetCount(); ++ii )
    {
        SCH_SYMBOL* symbol = m_symbolsList[ii].GetSymbol();

        for( const SCH_FIELD& field : symbol->GetFields() )
        {
            if( !field.IsMandatory() && !field.IsPrivate() )
                userFieldNames.insert( field.GetName() );
        }
    }

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false );

    // Add any templateFieldNames which aren't already present.
    for( const TEMPLATE_FIELDNAME& tfn : m_schSettings.m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( userFieldNames.count( tfn.m_Name ) == 0 )
            AddField( tfn.m_Name, GetGeneratedFieldDisplayName( tfn.m_Name ), false, false );
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::setScope( SCOPE aScope )
{
    m_dataModel->SetPath( m_parent->GetCurrentSheet() );
    m_dataModel->SetScope( aScope );
    m_dataModel->RebuildRows();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnScope( wxCommandEvent& aEvent )
{
    switch( aEvent.GetSelection() )
    {
    case 0: setScope( SCOPE::SCOPE_ALL );             break;
    case 1: setScope( SCOPE::SCOPE_SHEET );           break;
    case 2: setScope( SCOPE::SCOPE_SHEET_RECURSIVE ); break;
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnMenu( wxCommandEvent& event )
{
    // Build a pop menu:
    wxMenu menu;

    menu.Append( 4204, _( "Include 'DNP' Symbols" ),
                 _( "Show symbols marked 'DNP' in the table.  This setting also controls whether or not 'DNP' "
                    "symbols are included on export." ),
                 wxITEM_CHECK );
    menu.Check( 4204, !m_dataModel->GetExcludeDNP() );

    menu.Append( 4205, _( "Include 'Exclude from BOM' Symbols" ),
                 _( "Show symbols marked 'Exclude from BOM' in the table.  Symbols marked 'Exclude from BOM' "
                    "are never included on export." ),
                 wxITEM_CHECK );
    menu.Check( 4205, m_dataModel->GetIncludeExcludedFromBOM() );

    menu.AppendSeparator();

    menu.Append( 4206, _( "Highlight on Cross-probe" ),
                 _( "Highlight corresponding item on canvas when it is selected in the table" ),
                 wxITEM_CHECK );
    menu.Check( 4206, m_cfgDialogSettings.selection_mode == 0 );

    menu.Append( 4207, _( "Select on Cross-probe" ),
                 _( "Select corresponding item on canvas when it is selected in the table" ),
                 wxITEM_CHECK );
    menu.Check( 4207, m_cfgDialogSettings.selection_mode == 1 );

    // menu_id is the selected submenu id from the popup menu or wxID_NONE
    int menu_id = m_bMenu->GetPopupMenuSelectionFromUser( menu );

    if( menu_id == 0 || menu_id == 4204 )
    {
        m_dataModel->SetExcludeDNP( !m_dataModel->GetExcludeDNP() );
        m_dataModel->RebuildRows();
        m_grid->ForceRefresh();

        syncBomPresetSelection();
    }
    else if( menu_id == 1 || menu_id == 4205 )
    {
        m_dataModel->SetIncludeExcludedFromBOM( !m_dataModel->GetIncludeExcludedFromBOM() );
        m_dataModel->RebuildRows();
        m_grid->ForceRefresh();

        syncBomPresetSelection();
    }
    else if( menu_id == 3 || menu_id == 4206 )
    {
        if( m_cfgDialogSettings.selection_mode != 0 )
            m_cfgDialogSettings.selection_mode = 0;
        else
            m_cfgDialogSettings.selection_mode = 2;
    }
    else if( menu_id == 4 || menu_id == 4207 )
    {
        if( m_cfgDialogSettings.selection_mode != 1 )
            m_cfgDialogSettings.selection_mode = 1;
        else
            m_cfgDialogSettings.selection_mode = 2;
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnTableCellClick( wxGridEvent& event )
{
    if( m_dataModel->IsExpanderColumn( event.GetCol() ) )
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

    if( m_cfgDialogSettings.selection_mode == 0 )
    {
        SCH_EDITOR_CONTROL* editor = m_parent->GetToolManager()->GetTool<SCH_EDITOR_CONTROL>();

        if( refs.size() > 0 )
        {
            // Use of full path based on UUID allows select of not yet annotated or duplicated
            // symbols
            wxString symbol_path = refs.begin()->GetFullPath();

            // Focus only handles one item at this time
            editor->FindSymbolAndItem( &symbol_path, nullptr, true, HIGHLIGHT_SYMBOL, wxEmptyString );
        }
        else
        {
            m_parent->ClearFocus();
        }
    }
    else if( m_cfgDialogSettings.selection_mode == 1 )
    {
        SCH_SELECTION_TOOL*    selTool = m_parent->GetToolManager()->GetTool<SCH_SELECTION_TOOL>();
        std::vector<SCH_ITEM*> items( symbols.begin(), symbols.end() );

        if( refs.size() > 0 )
            selTool->SyncSelection( refs.begin()->GetSheetPath(), nullptr, items );
        else
            selTool->ClearSelection();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSaveAndContinue( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
    {
        m_cfgBomSettings.m_BomExportFileName = m_outputFileName->GetValue();
        m_parent->SaveProject();
        ClearModify();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnCancel( wxCommandEvent& aEvent )
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


void DIALOG_SYMBOL_FIELDS_TABLE::OnOk( wxCommandEvent& aEvent )
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


void DIALOG_SYMBOL_FIELDS_TABLE::OnClose( wxCloseEvent& aEvent )
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

    // Stop listening to schematic events
    m_parent->Schematic().RemoveListener( this );
    m_parent->ClearFocus();

    wxCommandEvent* evt = new wxCommandEvent( EDA_EVT_CLOSE_DIALOG_SYMBOL_FIELDS_TABLE, wxID_ANY );

    if( wxWindow* parent = GetParent() )
        wxQueueEvent( parent, evt );
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchItemsAdded( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem )
{
    std::set<wxString> savedSelection = SaveGridSelection();

    SCH_REFERENCE_LIST allRefs;
    m_parent->Schematic().Hierarchy().GetSymbols( allRefs, SYMBOL_FILTER_ALL );

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
            {
                if( !field.IsMandatory() && !field.IsPrivate() )
                    AddField( field.GetCanonicalName(), field.GetName(), true, false, false );
            }

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
                {
                    if( !field.IsMandatory() && !field.IsPrivate() )
                        AddField( field.GetCanonicalName(), field.GetName(), true, false, false );
                }
            }

            m_dataModel->AddReferences( refs );
        }
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    RestoreGridSelection( savedSelection );
    EnableSelectionEvents();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchItemsRemoved( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem )
{
    std::set<wxString> savedSelection = SaveGridSelection();

    for( SCH_ITEM* item : aSchItem )
    {
        if( item->Type() == SCH_SYMBOL_T )
            m_dataModel->RemoveSymbol( *static_cast<SCH_SYMBOL*>( item ) );
        else if( item->Type() == SCH_SHEET_T )
            m_dataModel->RemoveReferences( getSheetSymbolReferences( *static_cast<SCH_SHEET*>( item ) ) );
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    RestoreGridSelection( savedSelection );
    EnableSelectionEvents();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchItemsChanged( SCHEMATIC& aSch, std::vector<SCH_ITEM*>& aSchItem )
{
    std::set<wxString> savedSelection = SaveGridSelection();

    SCH_REFERENCE_LIST allRefs;
    m_parent->Schematic().Hierarchy().GetSymbols( allRefs, SYMBOL_FILTER_ALL );

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
            {
                if( !field.IsMandatory() && !field.IsPrivate() )
                    AddField( field.GetCanonicalName(), field.GetName(), true, false, false );
            }

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
                {
                    if( !field.IsMandatory() && !field.IsPrivate() )
                        AddField( field.GetCanonicalName(), field.GetName(), true, false, false );
                }
            }

            m_dataModel->UpdateReferences( refs );
        }
    }

    DisableSelectionEvents();
    m_dataModel->RebuildRows();
    RestoreGridSelection( savedSelection );
    EnableSelectionEvents();
}


void DIALOG_SYMBOL_FIELDS_TABLE::OnSchSheetChanged( SCHEMATIC& aSch )
{
    m_dataModel->SetPath( aSch.CurrentSheet() );

    if( m_dataModel->GetScope() != SYMBOL_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE::SCOPE_ALL )
    {
        std::set<wxString> savedSelection = SaveGridSelection();

        DisableSelectionEvents();
        m_dataModel->RebuildRows();
        RestoreGridSelection( savedSelection );
        EnableSelectionEvents();
    }
}


void DIALOG_SYMBOL_FIELDS_TABLE::EnableSelectionEvents()
{
    m_grid->Connect( wxEVT_GRID_RANGE_SELECTED,
                     wxGridRangeSelectEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnTableRangeSelected ),
                     nullptr, this );
}


void DIALOG_SYMBOL_FIELDS_TABLE::DisableSelectionEvents()
{
    m_grid->Disconnect( wxEVT_GRID_RANGE_SELECTED,
                        wxGridRangeSelectEventHandler( DIALOG_SYMBOL_FIELDS_TABLE::OnTableRangeSelected ),
                        nullptr, this );
}


std::set<wxString> DIALOG_SYMBOL_FIELDS_TABLE::SaveGridSelection()
{
    std::set<wxString> selectedFullPaths;

    wxGridCellCoordsArray topLeft = m_grid->GetSelectionBlockTopLeft();
    wxGridCellCoordsArray bottomRight = m_grid->GetSelectionBlockBottomRight();

    for( size_t i = 0; i < topLeft.size(); ++i )
    {
        for( int row = topLeft[i].GetRow(); row <= bottomRight[i].GetRow(); ++row )
        {
            for( const SCH_REFERENCE& ref : m_dataModel->GetRowReferences( row ) )
                selectedFullPaths.insert( ref.GetFullPath() );
        }
    }

    wxArrayInt selectedRows = m_grid->GetSelectedRows();

    for( int row : selectedRows )
    {
        for( const SCH_REFERENCE& ref : m_dataModel->GetRowReferences( row ) )
            selectedFullPaths.insert( ref.GetFullPath() );
    }

    int cursorRow = m_grid->GetGridCursorRow();

    if( cursorRow >= 0 && selectedFullPaths.empty() )
    {
        for( const SCH_REFERENCE& ref : m_dataModel->GetRowReferences( cursorRow ) )
            selectedFullPaths.insert( ref.GetFullPath() );
    }

    return selectedFullPaths;
}


void DIALOG_SYMBOL_FIELDS_TABLE::RestoreGridSelection( const std::set<wxString>& aFullPaths )
{
    if( aFullPaths.empty() )
        return;

    m_grid->ClearSelection();

    bool firstSelection = true;

    for( int row = 0; row < m_dataModel->GetNumberRows(); ++row )
    {
        std::vector<SCH_REFERENCE> refs = m_dataModel->GetRowReferences( row );

        for( const SCH_REFERENCE& ref : refs )
        {
            if( aFullPaths.count( ref.GetFullPath() ) )
            {
                m_grid->SelectRow( row, true );

                if( firstSelection )
                {
                    m_grid->SetGridCursor( row, m_grid->GetGridCursorCol() );
                    firstSelection = false;
                }

                break;
            }
        }
    }
}


SCH_REFERENCE_LIST DIALOG_SYMBOL_FIELDS_TABLE::getSymbolReferences( SCH_SYMBOL* aSymbol,
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

                subSheets.GetSymbolsWithinPath( sheetRefs, sheetPath, SYMBOL_FILTER_NON_POWER, false );
                break;
            }
        }
    }

    for( SCH_REFERENCE& ref : sheetRefs )
        ref.Split();

    return sheetRefs;
}


void DIALOG_SYMBOL_FIELDS_TABLE::onAddVariant( wxCommandEvent& aEvent )
{
    if( !m_parent->ShowAddVariantDialog( this ) )
        return;

    wxArrayString ctrlContents;
    ctrlContents.Add( GetDefaultVariantName() );

    for( const wxString& variant : m_parent->Schematic().GetVariantNames() )
        ctrlContents.Add( variant );

    ctrlContents.Sort( SortVariantNames );
    m_variantListBox->Set( ctrlContents );

    wxString currentVariant = m_parent->Schematic().GetCurrentVariant();
    int      newSelection = m_variantListBox->FindString(
                currentVariant.IsEmpty() ? GetDefaultVariantName() : currentVariant );

    if( newSelection != wxNOT_FOUND )
        m_variantListBox->SetSelection( newSelection );

    updateVariantButtonStates();
}


void DIALOG_SYMBOL_FIELDS_TABLE::onDeleteVariant( wxCommandEvent& aEvent )
{
    int selection = m_variantListBox->GetSelection();

    // An empty or default selection cannot be deleted.
    if( ( selection == wxNOT_FOUND ) || ( selection == 0 ) )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Cannot delete the default variant." ),
                                                 10000, wxICON_ERROR );
        return;
    }

    wxString variantName = m_variantListBox->GetString( selection );
    m_variantListBox->Delete( selection );

    SCH_COMMIT commit( m_parent );

    m_parent->Schematic().DeleteVariant( variantName, &commit );

    if( !commit.Empty() )
        commit.Push( wxString::Format( wxS( "Delete Variant '%s'" ), variantName ) );

    m_parent->OnModify();

    int newSelection = std::max( 0, selection - 1 );
    m_variantListBox->SetSelection( newSelection );

    wxString selectedVariant = getSelectedVariant();
    m_parent->SetCurrentVariant( selectedVariant );

    if( m_grid->CommitPendingChanges( true ) )
    {
        m_dataModel->SetCurrentVariant( selectedVariant );
        m_dataModel->UpdateReferences( m_dataModel->GetReferenceList() );
        m_dataModel->RebuildRows();

        if( m_nbPages->GetSelection() == 1 )
            PreviewRefresh();
        else
            m_grid->ForceRefresh();
    }

    updateVariantButtonStates();
    m_parent->UpdateVariantSelectionCtrl( m_parent->Schematic().GetVariantNamesForUI() );
}


void DIALOG_SYMBOL_FIELDS_TABLE::onRenameVariant( wxCommandEvent& aEvent )
{
    int selection = m_variantListBox->GetSelection();

    // An empty or default selection cannot be renamed.
    if( ( selection == wxNOT_FOUND ) || ( selection == 0 ) )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Cannot rename the default variant." ),
                                                 10000, wxICON_ERROR );
        return;
    }

    wxString oldVariantName = m_variantListBox->GetString( selection );

    wxTextEntryDialog dlg( this, _( "Enter new variant name:" ), _( "Rename Design Variant" ),
                           oldVariantName, wxOK | wxCANCEL | wxCENTER );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString newVariantName = dlg.GetValue().Trim().Trim( false );

    // Empty name is not allowed.
    if( newVariantName.IsEmpty() )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Variant name cannot be empty." ), 10000, wxICON_ERROR );
        return;
    }

    // Reserved name is not allowed (case-insensitive).
    if( newVariantName.CmpNoCase( GetDefaultVariantName() ) == 0 )
    {
        m_parent->GetInfoBar()->ShowMessageFor( wxString::Format( _( "'%s' is a reserved variant name." ),
                                                                  GetDefaultVariantName() ),
                                                10000, wxICON_ERROR );
        return;
    }

    // Same name (exact match) - nothing to do
    if( newVariantName == oldVariantName )
        return;

    // Duplicate name is not allowed (case-insensitive).
    for( const wxString& existingName : m_parent->Schematic().GetVariantNames() )
    {
        if( existingName.CmpNoCase( newVariantName ) == 0
            && existingName.CmpNoCase( oldVariantName ) != 0 )
        {
            m_parent->GetInfoBar()->ShowMessageFor( wxString::Format( _( "Variant '%s' already exists." ),
                                                                      existingName ),
                                                    0000, wxICON_ERROR );
            return;
        }
    }

    m_parent->Schematic().RenameVariant( oldVariantName, newVariantName );
    m_parent->OnModify();

    wxArrayString ctrlContents = m_variantListBox->GetStrings();
    ctrlContents.Remove( oldVariantName );
    ctrlContents.Add( newVariantName );
    ctrlContents.Sort( SortVariantNames );
    m_variantListBox->Set( ctrlContents );

    int newSelection = m_variantListBox->FindString( newVariantName );

    if( newSelection != wxNOT_FOUND )
        m_variantListBox->SetSelection( newSelection );

    updateVariantButtonStates();
    m_parent->UpdateVariantSelectionCtrl( m_parent->Schematic().GetVariantNamesForUI() );
}


void DIALOG_SYMBOL_FIELDS_TABLE::onCopyVariant( wxCommandEvent& aEvent )
{
    int selection = m_variantListBox->GetSelection();

    // An empty or default selection cannot be copied.
    if( ( selection == wxNOT_FOUND ) || ( selection == 0 ) )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Cannot copy the default variant." ),
                                                 10000, wxICON_ERROR );
        return;
    }

    wxString sourceVariantName = m_variantListBox->GetString( selection );

    wxTextEntryDialog dlg( this, _( "Enter name for the copied variant:" ), _( "Copy Design Variant" ),
                           sourceVariantName + wxS( "_copy" ), wxOK | wxCANCEL | wxCENTER );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString newVariantName = dlg.GetValue().Trim().Trim( false );

    // Empty name is not allowed.
    if( newVariantName.IsEmpty() )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Variant name cannot be empty." ), 10000, wxICON_ERROR );
        return;
    }

    // Duplicate name is not allowed.
    if( m_variantListBox->FindString( newVariantName ) != wxNOT_FOUND )
    {
        m_parent->GetInfoBar()->ShowMessageFor( wxString::Format( _( "Variant '%s' already exists." ),
                                                                  newVariantName ),
                                                10000, wxICON_ERROR );
        return;
    }

    m_parent->Schematic().CopyVariant( sourceVariantName, newVariantName );
    m_parent->OnModify();

    wxArrayString ctrlContents = m_variantListBox->GetStrings();
    ctrlContents.Add( newVariantName );
    ctrlContents.Sort( SortVariantNames );
    m_variantListBox->Set( ctrlContents );

    int newSelection = m_variantListBox->FindString( newVariantName );

    if( newSelection != wxNOT_FOUND )
        m_variantListBox->SetSelection( newSelection );

    updateVariantButtonStates();
    m_parent->UpdateVariantSelectionCtrl( m_parent->Schematic().GetVariantNamesForUI() );
}


void DIALOG_SYMBOL_FIELDS_TABLE::onEditVariantDescription( wxCommandEvent& aEvent )
{
    int selection = m_variantListBox->GetSelection();

    if( ( selection == wxNOT_FOUND ) || ( selection == 0 ) )
    {
        m_parent->GetInfoBar()->ShowMessageFor( _( "Cannot edit the default variant description." ), 10000,
                                                wxICON_ERROR );
        return;
    }

    wxString variantName = m_variantListBox->GetString( selection );
    wxString currentDesc = m_parent->Schematic().GetVariantDescription( variantName );

    wxDialog dlg( this, wxID_ANY, wxString::Format( _( "Edit Description for '%s'" ), variantName ), wxDefaultPosition,
                  wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER );

    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    wxStaticText* label = new wxStaticText( &dlg, wxID_ANY, _( "Description:" ) );
    mainSizer->Add( label, 0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 10 );

    mainSizer->AddSpacer( 3 );

    wxTextCtrl* descCtrl =
            new wxTextCtrl( &dlg, wxID_ANY, currentDesc, wxDefaultPosition, wxSize( 300, 60 ), wxTE_MULTILINE );
    mainSizer->Add( descCtrl, 1, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10 );

    wxStdDialogButtonSizer* btnSizer = new wxStdDialogButtonSizer();
    btnSizer->AddButton( new wxButton( &dlg, wxID_OK ) );
    btnSizer->AddButton( new wxButton( &dlg, wxID_CANCEL ) );
    btnSizer->Realize();
    mainSizer->Add( btnSizer, 0, wxALL | wxALIGN_RIGHT, 5 );

    dlg.SetSizer( mainSizer );
    dlg.Fit();
    dlg.Centre();

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString newDesc = descCtrl->GetValue().Trim().Trim( false );

    m_parent->Schematic().SetVariantDescription( variantName, newDesc );
    m_parent->OnModify();
}


void DIALOG_SYMBOL_FIELDS_TABLE::onVariantSelectionChange( wxCommandEvent& aEvent )
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
        currentVariant = m_parent->Schematic().GetCurrentVariant();

        if( currentVariant != selectedVariant )
            m_parent->SetCurrentVariant( selectedVariant );
    }

    if( currentVariant != selectedVariant )
    {
        m_grid->CommitPendingChanges( true );

        SCH_COMMIT     commit( m_parent );

        m_dataModel->ApplyData( commit, m_schSettings.m_TemplateFieldNames, currentVariant );

        if( !commit.Empty() )
        {
            commit.Push( wxS( "Symbol Fields Table Edit" ) );  // Push clears the commit buffer.
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


void DIALOG_SYMBOL_FIELDS_TABLE::updateVariantButtonStates()
{
    int selection = m_variantListBox->GetSelection();

    // Copy, rename, and delete are only enabled for non-default variant selections
    bool canModify = ( selection != wxNOT_FOUND ) && ( selection != 0 );

    m_copyVariantButton->Enable( canModify );
    m_renameVariantButton->Enable( canModify );
    m_editVariantDescButton->Enable( canModify );
    m_deleteVariantButton->Enable( canModify );
}


wxString DIALOG_SYMBOL_FIELDS_TABLE::resolveVariant() const
{
    // A job keeps its own variant, otherwise follow the schematic.
    if( m_job )
        return getSelectedVariant();

    return m_parent->Schematic().GetCurrentVariant();
}


bool DIALOG_SYMBOL_FIELDS_TABLE::resolveTextVar( wxString* aToken ) const
{
    SCHEMATIC& schematic = m_parent->Schematic();

    return schematic.ResolveTextVar( &schematic.CurrentSheet(), aToken, 0 );
}
