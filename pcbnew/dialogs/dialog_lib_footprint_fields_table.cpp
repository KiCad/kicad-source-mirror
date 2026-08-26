/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialog_lib_footprint_fields_table.h>
#include <footprint_fields_data_model.h>

#include <algorithm>
#include <set>

#include <confirm.h>
#include <eda_doc.h>
#include <fields_view_controls_grid_data_model.h>
#include <footprint.h>
#include <footprint_editor_settings.h>
#include <grid_tricks.h>
#include <kiface_base.h>
#include <pgm_base.h>
#include <project.h>
#include <project_pcb.h>
#include <settings/settings_manager.h>
#include <footprint_edit_frame.h>
#include <footprint_library_adapter.h>
#include <template_fieldnames.h>
#include <trace_helpers.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>

using SCOPE = LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE;


static GRID_CELL_URL_EDITOR_CONTEXT getDatasheetContext( const std::vector<FOOTPRINT_REF>& aFootprints )
{
    std::vector<EMBEDDED_FILES*> embedTargets;
    std::vector<EMBEDDED_FILES*> inheritedFiles;

    for( FOOTPRINT_REF footprint : aFootprints )
    {
        embedTargets.push_back( footprint.GetFootprint().GetEmbeddedFiles() );
    }

    return MakeGridCellUrlEditorContext( embedTargets, inheritedFiles );
}


enum
{
    MYID_SHOW_DATASHEET = FIELDS_TABLE_GRID_TRICKS::FIRST_CLIENT_ID,
    MYID_INCLUDE_DNP,
    MYID_INCLUDE_EXCLUDED_FROM_BOM
};


class LIB_FOOTPRINT_FIELDS_EDITOR_GRID_TRICKS : public FIELDS_TABLE_GRID_TRICKS
{
public:
    LIB_FOOTPRINT_FIELDS_EDITOR_GRID_TRICKS( DIALOG_LIB_FOOTPRINT_FIELDS_TABLE* aParent, WX_GRID* aGrid,
                                   VIEW_CONTROLS_GRID_DATA_MODEL*     aViewFieldsData,
                                   LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL* aDataModel ) :
            FIELDS_TABLE_GRID_TRICKS( aParent, aGrid, aDataModel ),
            m_dlg( aParent ),
            m_viewControlsDataModel( aViewFieldsData ),
            m_dataModel( aDataModel )
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
            GetAssociatedDocument( m_dlg, datasheetUri, &m_dlg->Prj(), nullptr, { } );
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
    DIALOG_LIB_FOOTPRINT_FIELDS_TABLE*           m_dlg;
    VIEW_CONTROLS_GRID_DATA_MODEL*               m_viewControlsDataModel;
    LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
};


DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::DIALOG_LIB_FOOTPRINT_FIELDS_TABLE( FOOTPRINT_EDIT_FRAME* aParent, SCOPE aScope ) :
        DIALOG_FIELDS_TABLE( aParent, aParent->GetSettings()->m_LibFieldEditor,
                             aParent->GetSettings()->m_LibFieldEditorBom, nullptr ),
        m_parent( aParent )
{
    loadFootprints();

    const wxString& libName = m_parent->GetTargetFPID().GetLibNickname();
    const bool      readOnly = !PROJECT_PCB::FootprintLibAdapter( &Prj() )->IsFootprintLibWritable( libName );

    m_dataModel = new LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL( m_footprintsList );
    m_dataModel->SetScope( aScope );

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // The field-list grid regroups its rows, so the dialog's position-based Ctrl+Z would shift
    // values onto the wrong field.
    ExcludeFromControlUndoRedo( m_viewControlsGrid );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler(
            new LIB_FOOTPRINT_FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_viewControlsDataModel, m_dataModel ) );

    m_splitter_left->Unsplit( m_variantsPanel );
    m_variantsPanel->Hide();

    // Footprint libraries don't have a concept of related or derived footprints like symbols do.
    m_scope->Clear();
    m_scope->Append( _( "Whole Library" ) );
    m_scope->SetSelection( static_cast<int>( m_dataModel->GetScope() ) );
    m_filterScope->SetString( static_cast<int>( BOM_FILTER_SCOPE::REFERENCE ), _( "Footprint Names" ) );

    wxString title = wxString::Format( _( "Footprint Fields Table ('%s' Library)" ), libName );

    if( readOnly )
        title += wxS( " " ) + _( "[Read Only]" );

    SetTitle( title );
    m_buttonApply->SetLabel( _( "Save" ) );

    SetInitialFocus( m_grid );
    m_grid->ClearSelection();

    SetupStandardButtons();
    SetReadOnly( readOnly );

    finishDialogSettings();

    SetSize( GetDefaultDialogSize() );

    RestorePanelLayout();

    OptOut( m_outputFileName );

    m_outputFileName->SetValue( m_cfgBomSettings.m_BomExportFileName );

    Center();

    // Connect Events
    m_grid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnColSort, this );
    m_grid->Bind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnColMove, this );
    m_grid->GetGridWindow()->Bind( wxEVT_MOTION, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnGridMouseMove, this );
    m_cbBomPresets->Bind( wxEVT_CHOICE, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Bind( wxEVT_CHOICE, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::onBomFmtPresetChanged, this );
}


void DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::loadFootprints()
{
    m_footprintsList.clear();
    m_loadedFootprints.clear();

    FOOTPRINT_LIBRARY_ADAPTER* libMgr = PROJECT_PCB::FootprintLibAdapter( &Prj() );
    wxString                   libName = m_parent->GetTargetFPID().GetLibNickname();
    std::vector<wxString>      footprintNames = libMgr->GetFootprintNames( libName );

    if( footprintNames.empty() )
    {
        wxMessageBox( wxString::Format( _( "No footprints found in library '%s'." ), libName ) );
        return;
    }

    // Saving replaces the library manager's cached footprint object, so keep independent copies
    // whose addresses remain stable for the lifetime of the data model.
    for( const wxString& footprintName : footprintNames )
    {
        try
        {
            // LoadFootprint returns a caller-owned clone.
            std::unique_ptr<FOOTPRINT> footprint( libMgr->LoadFootprint( libName, footprintName, true ) );

            if( footprint )
            {
                m_loadedFootprints.push_back( std::move( footprint ) );
                m_footprintsList.emplace_back( *m_loadedFootprints.back() );
            }
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogWarning( wxString::Format( _( "Error loading footprint '%s': %s" ), footprintName, ioe.What() ) );
        }
    }

    if( m_footprintsList.empty() )
        wxMessageBox( _( "No footprints could be loaded from the library." ) );
}


DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::~DIALOG_LIB_FOOTPRINT_FIELDS_TABLE()
{
    savePresets( true );
    SavePanelLayout();
    SaveColumnWidths();

    // Disconnect Events
    m_grid->GetGridWindow()->Unbind( wxEVT_MOTION, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnGridMouseMove, this );
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnColSort, this );
    m_grid->Unbind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnColMove, this );
    m_cbBomPresets->Unbind( wxEVT_CHOICE, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Unbind( wxEVT_CHOICE, &DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::onBomFmtPresetChanged, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // we gave ownership of m_viewControlsDataModel & m_dataModel to the wxGrids...
}


wxGridCellEditor* DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::createDatasheetEditor()
{
    return new GRID_CELL_URL_EDITOR(
            this, nullptr,
            [this]( int aRow )
            {
                return getDatasheetContext( m_dataModel->GetRowReferences( aRow ) );
            } );
}


bool DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    LoadFieldNames(); // loads rows into m_viewControlsDataModel and columns into m_dataModel

    m_scope->SetSelection( static_cast<int>( m_dataModel->GetScope() ) );

    // Load our BOM view presets
    SetUserBomPresets( m_cfgBomSettings.m_BomPresets );

    BOM_PRESET preset = m_cfgBomSettings.m_BomSettings;

    ApplyBomPreset( preset );
    syncBomPresetSelection();

    // Load BOM export format presets
    SetUserBomFmtPresets( m_cfgBomSettings.m_BomFmtPresets );
    ApplyBomFmtPreset( m_cfgBomSettings.m_BomFmtSettings );
    syncBomFmtPresetSelection();

    m_outputFileName->SetValue( m_cfgBomSettings.m_BomExportFileName );

    m_dataModel->SetGroupingEnabled( m_groupSymbolsBox->GetValue() );

    setScope( static_cast<SCOPE>( m_scope->GetSelection() ) );

    return true;
}


bool DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    std::set<KIID_PATH> savedSelection = SaveGridSelection();
    bool                libraryChanged = false;

    bool allChangesApplied = m_dataModel->ApplyData(
            [&]( FOOTPRINT& aFootprint )
            {
                bool saved =
                        m_parent->SaveFootprintInLibrary( &aFootprint, aFootprint.GetFPID().GetUniStringLibNickname() );

                libraryChanged |= saved;

                if( saved )
                    m_parent->RefreshLibraryFootprintTab( aFootprint );

                return saved;
            } );

    m_dataModel->RebuildRows();
    RestoreGridSelection( savedSelection );

    if( libraryChanged )
        m_parent->SyncLibraryTree( true );

    if( !allChangesApplied )
        return false;

    ClearModify();
    return true;
}


void DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::LoadFieldNames()
{
    auto addMandatoryField =
            [&]( FIELD_T aFieldId, bool aShow, bool aGroupBy )
            {
                m_mandatoryFieldListIndexes[aFieldId] = m_viewControlsDataModel->GetNumberRows();

                AddField( GetCanonicalFieldName( aFieldId ), GetDefaultFieldName( aFieldId, DO_TRANSLATE ),
                          aShow, aGroupBy );
            };

    AddField( wxS( "${FOOTPRINT_NAME}" ), _( "Footprint Name" ), true, false );

    // Add mandatory fields first            show   groupBy
    addMandatoryField( FIELD_T::REFERENCE,   false,  false  );
    addMandatoryField( FIELD_T::VALUE,       true,   false  );
    addMandatoryField( FIELD_T::FOOTPRINT,   true,   false  );
    addMandatoryField( FIELD_T::DATASHEET,   true,   false  );
    addMandatoryField( FIELD_T::DESCRIPTION, false,  false  );

    // Generated fields present only in the fields table
    AddField( LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::FOOTPRINT_KEYWORDS, _( "Keywords" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_BOM}" ), _( "Exclude From BOM" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_SIM}" ), _( "Exclude From Simulation" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_BOARD}" ), _( "Exclude From Board" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_POS_FILES}" ), _( "Exclude From Position Files" ), true, false );

    // User field names are stored and matched case-sensitively (see issue #24021), so each
    // distinct name gets its own column rather than collapsing case variants together.
    std::set<wxString> userFieldNames;

    for( FOOTPRINT_REF ref : m_footprintsList )
    {
        for( PCB_FIELD* field : ref.GetFootprint().GetFields() )
        {
            if( !field->IsMandatory() && !field->IsPrivate() )
                userFieldNames.insert( field->GetName() );
        }
    }

    for( const wxString& fieldName : userFieldNames )
        AddField( fieldName, GetGeneratedFieldDisplayName( fieldName ), true, false );

    // TODO: template fieldnames aren't implement for boards/footprints
}


void DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::setScope( SCOPE aScope )
{
    m_dataModel->SetScope( aScope );
    m_dataModel->RebuildRows();
}


void DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnScope( wxCommandEvent& aEvent )
{
    switch( aEvent.GetSelection() )
    {
    case 0: setScope( SCOPE::SCOPE_ALL );         break;
    }
}


void DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnMenu( wxCommandEvent& aEvent )
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
}


void DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnSaveAndContinue( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
    {
        m_cfgBomSettings.m_BomExportFileName = m_outputFileName->GetValue();
        ClearModify();
    }
}


void DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnCancel( wxCommandEvent& aEvent )
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
            return;
        }
    }

    EndModal( wxID_CANCEL );
}


void DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnOk( wxCommandEvent& aEvent )
{
    if( !TransferDataFromWindow() )
        return;

    m_cfgBomSettings.m_BomExportFileName = m_outputFileName->GetValue();
    EndModal( wxID_OK );
}


void DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::OnClose( wxCloseEvent& aEvent )
{
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

    aEvent.Skip();
}


std::vector<BOM_PRESET> DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::getBuiltInBomPresets() const
{
    std::vector<BOM_PRESET> presets = BOM_PRESET::BuiltInPresets();

    for( BOM_PRESET& preset : presets )
    {
        if( preset.sortField == GetDefaultFieldName( FIELD_T::REFERENCE, DO_TRANSLATE ) )
            preset.sortField = LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::FOOTPRINT_NAME;

        for( BOM_FIELD& field : preset.fieldsOrdered )
        {
            if( field.name == GetCanonicalFieldName( FIELD_T::REFERENCE ) )
            {
                field.name = LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::FOOTPRINT_NAME;
                field.label = wxS( "Footprint Name" );
                field.groupBy = false;
            }
        }

        if( preset.name == BOM_PRESET::DefaultEditing().name )
        {
            preset.groupSymbols = false;
            preset.fieldsOrdered = {
                { LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::FOOTPRINT_NAME, wxS( "Footprint Name" ), true, false },
                { GetCanonicalFieldName( FIELD_T::REFERENCE ), wxS( "Reference" ), false, false },
                { GetCanonicalFieldName( FIELD_T::VALUE ), wxS( "Value" ), true, false },
                { GetCanonicalFieldName( FIELD_T::FOOTPRINT ), wxS( "Footprint" ), true, false },
                { GetCanonicalFieldName( FIELD_T::DATASHEET ), wxS( "Datasheet" ), true, false },
                { GetCanonicalFieldName( FIELD_T::DESCRIPTION ), wxS( "Description" ), false, false },
                { LIB_FOOTPRINT_FIELDS_EDITOR_GRID_DATA_MODEL::FOOTPRINT_KEYWORDS, wxS( "Keywords" ), true, false },
                { wxS( "${EXCLUDE_FROM_BOM}" ), wxS( "Exclude From BOM" ), true, false },
                { wxS( "${EXCLUDE_FROM_SIM}" ), wxS( "Exclude From Simulation" ), true, false },
                { wxS( "${EXCLUDE_FROM_BOARD}" ), wxS( "Exclude From Board" ), true, false },
                { wxS( "${EXCLUDE_FROM_POS_FILES}" ), wxS( "Exclude From Position Files" ), true, false },
            };
        }
    }

    return presets;
}


wxString DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::resolveVariant() const
{
    return wxEmptyString;
}


bool DIALOG_LIB_FOOTPRINT_FIELDS_TABLE::resolveTextVar( wxString* aToken ) const
{
    return Prj().TextVarResolver( aToken );
}
