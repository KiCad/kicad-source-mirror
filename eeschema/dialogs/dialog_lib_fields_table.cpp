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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialog_lib_fields_table.h>

#include <algorithm>
#include <set>

#include <confirm.h>
#include <dialog_lib_new_symbol.h>
#include <eda_doc.h>
#include <fields_view_controls_grid_data_model.h>
#include <grid_tricks.h>
#include <kiface_base.h>
#include <kiway_player.h>
#include <project.h>
#include <project_sch.h>
#include <string_utils.h>
#include <symbol_edit_frame.h>
#include <symbol_editor/lib_symbol_library_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <template_fieldnames.h>
#include <trace_helpers.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <wx/arrstr.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>

using DIALOG_NEW_SYMBOL = DIALOG_LIB_NEW_SYMBOL;


namespace
{
enum
{
    MYID_SELECT_FOOTPRINT = GRIDTRICKS_FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET,
    MYID_REVERT_ROW,
    MYID_CLEAR_CELL,
    MYID_CREATE_DERIVED_SYMBOL,
    MYID_INCLUDE_DNP,
    MYID_INCLUDE_EXCLUDED_FROM_BOM
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
    {
    }

protected:
    void showPopupMenu( wxMenu& aMenu, wxGridEvent& aEvent ) override
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        wxMenuItem* revertMenu = aMenu.Append( MYID_REVERT_ROW, _( "Revert symbol" ),
                                               _( "Revert the symbol to its last saved state" ) );
        wxMenuItem* clearMenu = aMenu.Append( MYID_CLEAR_CELL, _( "Clear cell" ),
                                              _( "Clear the cell value" ) );
        aMenu.AppendSeparator();
        wxMenuItem* deriveMenu = aMenu.Append( MYID_CREATE_DERIVED_SYMBOL, _( "Create Derived Symbol" ),
                                               _( "Create a new symbol derived from the selected one" ) );

        if( row >= 0 && col >= 0 )
        {
            revertMenu->Enable( m_dataModel->IsCellEdited( row, col ) );
            clearMenu->Enable( !m_dataModel->IsCellClear( row, col ) );
            deriveMenu->Enable( m_dataModel->IsRowSingleSymbol( row ) );

            if( m_dataModel->GetColFieldName( col ) == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
            {
                aMenu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ), _( "Browse for footprint" ) );
                aMenu.AppendSeparator();
            }
            else if( m_dataModel->GetColFieldName( col ) == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
            {
                aMenu.Append( MYID_SHOW_DATASHEET, _( "Show Datasheet" ), _( "Show datasheet in browser" ) );
                aMenu.AppendSeparator();
            }
        }
        else
        {
            revertMenu->Enable( false );
            clearMenu->Enable( false );
            deriveMenu->Enable( false );
        }

        GRID_TRICKS::showPopupMenu( aMenu, aEvent );
    }

    void doPopupSelection( wxCommandEvent& aEvent ) override
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( aEvent.GetId() == MYID_REVERT_ROW )
        {
            if( m_grid->CommitPendingChanges( false ) )
                m_dataModel->RevertRow( row );

            if( m_dataModel->IsEdited() )
                m_dlg->OnModify();
            else
                m_dlg->ClearModify();

            m_grid->ForceRefresh();
        }
        else if( aEvent.GetId() == MYID_CLEAR_CELL )
        {
            if( m_grid->CommitPendingChanges( false ) )
                m_dataModel->ClearCell( row, col );

            if( m_dataModel->IsEdited() )
                m_dlg->OnModify();
            else
                m_dlg->ClearModify();

            m_grid->ForceRefresh();
        }
        else if( aEvent.GetId() == MYID_CREATE_DERIVED_SYMBOL )
        {
            EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( m_dlg->GetParent() );
            wxCHECK( frame, /* void */ );

            const LIB_SYMBOL* parentSymbol = m_dataModel->GetSymbolForRow( row );

            wxArrayString symbolNames;
            m_dataModel->GetSymbolNames( symbolNames );

            auto validator =
                    [&]( const wxString& aNewName ) -> bool
                    {
                        return symbolNames.Index( aNewName ) == wxNOT_FOUND;
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
        else if( aEvent.GetId() == MYID_SELECT_FOOTPRINT )
        {
            wxString fpid = m_grid->GetCellValue( row, col );

            if( KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, m_dlg ) )
            {
                if( frame->ShowModal( &fpid, m_dlg ) )
                    m_grid->SetCellValue( row, col, fpid );

                frame->Destroy();
            }
        }
        else if( aEvent.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheetUri = m_grid->GetCellValue( row, col );
            GetAssociatedDocument( m_dlg, datasheetUri, &m_dlg->Prj(),
                                   PROJECT_SCH::SchSearchS( &m_dlg->Prj() ) );
        }
        else if( aEvent.GetId() >= GRIDTRICKS_FIRST_SHOWHIDE )
        {
            if( !m_grid->CommitPendingChanges( false ) )
                return;

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
    DIALOG_LIB_FIELDS_TABLE*           m_dlg;
    VIEW_CONTROLS_GRID_DATA_MODEL*     m_viewControlsDataModel;
    LIB_FIELDS_EDITOR_GRID_DATA_MODEL* m_dataModel;
};
} // namespace


DIALOG_LIB_FIELDS_TABLE::DIALOG_LIB_FIELDS_TABLE( SYMBOL_EDIT_FRAME* aParent, SCOPE aScope ) :
        DIALOG_FIELDS_TABLE( aParent, aParent->libeditconfig()->m_LibFieldEditor,
                             aParent->libeditconfig()->m_LibFieldEditorBom, nullptr ),
        m_parent( aParent ),
        m_symbolScope( aScope )
{
    m_dataModel = new LIB_FIELDS_EDITOR_GRID_DATA_MODEL();

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // The field-list grid regroups its rows, so the dialog's position-based Ctrl+Z would shift
    // values onto the wrong field.
    ExcludeFromControlUndoRedo( m_viewControlsGrid );

    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );
    m_grid->PushEventHandler( new LIB_FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_viewControlsDataModel,
                                                                 m_dataModel ) );

    m_splitter_left->Unsplit( m_variantsPanel );
    m_variantsPanel->Hide();

    m_scope->Clear();
    m_scope->Append( _( "Whole Library" ) );
    m_scope->Append( _( "Related Symbols Only" ) );
    m_scope->SetSelection( static_cast<int>( m_symbolScope ) );
    m_filterScope->SetString( static_cast<int>( BOM_FILTER_SCOPE::REFERENCE ), _( "Symbol Names" ) );

    SetTitle( wxString::Format( _( "Symbol Fields Table ('%s' Library)" ),
                                wxString::FromUTF8( m_parent->GetTargetLibId().GetLibNickname() ) ) );
    m_buttonApply->SetLabel( _( "Apply" ) );

    SetInitialFocus( m_grid );
    m_grid->ClearSelection();

    SetupStandardButtons();
    finishDialogSettings();
    SetSize( GetDefaultDialogSize() );
    RestorePanelLayout();

    OptOut( m_outputFileName );
    m_outputFileName->SetValue( m_cfgBomSettings.m_BomExportFileName );

    Center();

    m_grid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FIELDS_TABLE::OnColSort, this );
    m_grid->Bind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FIELDS_TABLE::OnColMove, this );
    m_grid->GetGridWindow()->Bind( wxEVT_MOTION, &DIALOG_LIB_FIELDS_TABLE::OnGridMouseMove, this );
    m_cbBomPresets->Bind( wxEVT_CHOICE, &DIALOG_LIB_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Bind( wxEVT_CHOICE, &DIALOG_LIB_FIELDS_TABLE::onBomFmtPresetChanged, this );
}


DIALOG_LIB_FIELDS_TABLE::~DIALOG_LIB_FIELDS_TABLE()
{
    savePresets( true );
    SavePanelLayout();
    SaveColumnWidths();

    m_grid->GetGridWindow()->Unbind( wxEVT_MOTION, &DIALOG_LIB_FIELDS_TABLE::OnGridMouseMove, this );
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FIELDS_TABLE::OnColSort, this );
    m_grid->Unbind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FIELDS_TABLE::OnColMove, this );
    m_cbBomPresets->Unbind( wxEVT_CHOICE, &DIALOG_LIB_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Unbind( wxEVT_CHOICE, &DIALOG_LIB_FIELDS_TABLE::onBomFmtPresetChanged, this );

    m_grid->PopEventHandler( true );
}


wxGridCellEditor* DIALOG_LIB_FIELDS_TABLE::createFootprintEditor()
{
    wxString symbolNetlist;

    if( !m_symbolsList.empty() )
    {
        LIB_SYMBOL* symbol = m_symbolsList.front();
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

    return new GRID_CELL_FPID_EDITOR( this, symbolNetlist );
}


wxGridCellEditor* DIALOG_LIB_FIELDS_TABLE::createDatasheetEditor()
{
    return new GRID_CELL_URL_EDITOR( this, PROJECT_SCH::SchSearchS( &Prj() ) );
}


bool DIALOG_LIB_FIELDS_TABLE::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    m_scope->SetSelection( static_cast<int>( m_symbolScope ) );
    setScope( m_symbolScope );

    SetUserBomPresets( m_cfgBomSettings.m_BomPresets );

    BOM_PRESET preset = m_cfgBomSettings.m_BomSettings;

    if( preset.fieldsOrdered.empty() )
    {
        for( const BOM_PRESET& builtIn : getBuiltInBomPresets() )
        {
            if( builtIn.name == BOM_PRESET::DefaultEditing().name )
            {
                preset = builtIn;
                break;
            }
        }
    }

    ApplyBomPreset( preset );
    syncBomPresetSelection();

    SetUserBomFmtPresets( m_cfgBomSettings.m_BomFmtPresets );
    ApplyBomFmtPreset( m_cfgBomSettings.m_BomFmtSettings );
    syncBomFmtPresetSelection();

    m_outputFileName->SetValue( m_cfgBomSettings.m_BomExportFileName );

    return true;
}


bool DIALOG_LIB_FIELDS_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    bool updateCanvas = false;

    m_dataModel->ApplyData(
            [&]( LIB_SYMBOL* aSymbol )
            {
                m_parent->GetLibManager().UpdateSymbol( aSymbol, aSymbol->GetLibNickname() );

                if( m_parent->GetCurSymbol() == aSymbol )
                    updateCanvas = true;
            },
            [&]()
            {
                auto createdSymbols = m_dataModel->GetAndClearCreatedDerivedSymbols();

                wxLogTrace( traceLibFieldTable, "Post-apply handler: found %zu created derived symbols",
                            createdSymbols.size() );

                for( const auto& [symbol, libraryName] : createdSymbols )
                {
                    if( !libraryName.IsEmpty() )
                    {
                        wxLogTrace( traceLibFieldTable, "Updating symbol '%s' (UUID: %s) in library '%s'",
                                    symbol->GetName(), symbol->m_Uuid.AsString(), libraryName );
                        m_parent->GetLibManager().UpdateSymbol( symbol, libraryName );
                    }
                }

                if( !createdSymbols.empty() )
                {
                    wxLogTrace( traceLibFieldTable, "Syncing libraries due to %zu new symbols",
                                createdSymbols.size() );

                    std::vector<LIB_SYMBOL*> symbolsToPreserve;

                    for( const auto& [symbol, libraryName] : createdSymbols )
                        symbolsToPreserve.push_back( symbol );

                    m_parent->SyncLibraries( false );

                    for( LIB_SYMBOL* symbol : symbolsToPreserve )
                    {
                        bool found = std::any_of( m_symbolsList.begin(), m_symbolsList.end(),
                                [&]( LIB_SYMBOL* aExistingSymbol )
                                {
                                    return aExistingSymbol->m_Uuid == symbol->m_Uuid;
                                } );

                        if( !found )
                        {
                            wxLogTrace( traceLibFieldTable, "Re-adding symbol '%s' to list after sync",
                                        symbol->GetName() );
                            m_symbolsList.push_back( symbol );
                        }
                    }
                }

                wxLogTrace( traceLibFieldTable, "Dialog symbol list size after processing: %zu",
                            m_symbolsList.size() );
            } );

    ClearModify();
    m_dataModel->RebuildRows();
    m_parent->RefreshLibraryTree();

    if( updateCanvas )
    {
        m_parent->OnModify();
        m_parent->HardRedraw();
    }

    return true;
}


void DIALOG_LIB_FIELDS_TABLE::loadSymbols( const wxArrayString& aSymbolNames )
{
    m_symbolsList.clear();

    wxString libName = m_parent->GetTreeLIBID().GetLibNickname();

    if( aSymbolNames.IsEmpty() )
    {
        if( m_symbolScope == SCOPE_RELATED_SYMBOLS )
            wxMessageBox( wxString::Format( _( "No related symbols found in library '%s'." ), libName ) );
        else
            wxMessageBox( wxString::Format( _( "No symbols found in library '%s'." ), libName ) );

        m_dataModel->SetSymbols( m_symbolsList );
        return;
    }

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
                wxLogWarning( wxString::Format( _( "Error loading symbol '%s': %s" ), symbolName, ioe.What() ) );
            }
        }
    }

    if( m_symbolsList.empty() )
    {
        if( m_symbolScope == SCOPE_RELATED_SYMBOLS )
            wxMessageBox( _( "No related symbols could be loaded from the library." ) );
        else
            wxMessageBox( _( "No symbols could be loaded from the library." ) );
    }

    m_dataModel->SetSymbols( m_symbolsList );
}


void DIALOG_LIB_FIELDS_TABLE::loadFieldNames()
{
    auto addMandatoryField =
            [&]( FIELD_T aFieldId, bool aShow, bool aGroupBy )
            {
                wxString fieldName = GetCanonicalFieldName( aFieldId );
                int      row = -1;

                for( int i = 0; i < m_viewControlsDataModel->GetNumberRows(); ++i )
                {
                    if( m_viewControlsDataModel->GetCanonicalFieldName( i ) == fieldName )
                    {
                        row = i;
                        break;
                    }
                }

                if( row == -1 )
                {
                    row = m_viewControlsDataModel->GetNumberRows();
                    AddField( fieldName, GetDefaultFieldName( aFieldId, DO_TRANSLATE ), aShow, aGroupBy );
                }

                m_mandatoryFieldListIndexes[aFieldId] = row;
            };

    AddField( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME, _( "Symbol Name" ), true, false );

    addMandatoryField( FIELD_T::REFERENCE, false, false );
    addMandatoryField( FIELD_T::VALUE, true, false );
    addMandatoryField( FIELD_T::FOOTPRINT, true, false );
    addMandatoryField( FIELD_T::DATASHEET, true, false );
    addMandatoryField( FIELD_T::DESCRIPTION, false, false );

    AddField( wxS( "Keywords" ), _( "Keywords" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_BOM}" ), _( "Exclude From BOM" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_SIM}" ), _( "Exclude From Simulation" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_BOARD}" ), _( "Exclude From Board" ), true, false );
    AddField( wxS( "${SYMBOL_IS_POWER}" ), _( "Power Symbol" ), true, false );
    AddField( wxS( "${SYMBOL_IS_LOCAL_POWER}" ), _( "Local Power Symbol" ), true, false );

    std::set<wxString> userFieldNames;

    for( LIB_SYMBOL* symbol : m_symbolsList )
    {
        std::vector<SCH_FIELD*> fields;
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


void DIALOG_LIB_FIELDS_TABLE::setScope( SCOPE aScope )
{
    LIB_SYMBOL_LIBRARY_MANAGER& libMgr = m_parent->GetLibManager();
    wxString                    targetLib = m_parent->GetTargetLibId().GetLibNickname();
    wxString                    targetSymbol = m_parent->GetTargetLibId().GetLibItemName();
    wxArrayString               symbolNames;

    m_symbolScope = aScope;

    if( m_symbolScope == SCOPE_RELATED_SYMBOLS )
    {
        const LIB_SYMBOL*           symbol = libMgr.GetBufferedSymbol( targetSymbol, targetLib );
        std::shared_ptr<LIB_SYMBOL> root = symbol ? symbol->GetRootSymbol() : nullptr;

        if( root )
        {
            symbolNames.Add( root->GetName() );
            libMgr.GetDerivedSymbolNames( root->GetName(), targetLib, symbolNames );
        }
    }
    else
    {
        libMgr.GetSymbolNames( targetLib, symbolNames );
    }

    loadSymbols( symbolNames );
    loadFieldNames();
    m_dataModel->RebuildRows();
    SetupAllColumnProperties();
}


void DIALOG_LIB_FIELDS_TABLE::OnScope( wxCommandEvent& aEvent )
{
    switch( aEvent.GetSelection() )
    {
    case 0: setScope( SCOPE_LIBRARY );         break;
    case 1: setScope( SCOPE_RELATED_SYMBOLS ); break;
    }
}


void DIALOG_LIB_FIELDS_TABLE::OnMenu( wxCommandEvent& aEvent )
{
    wxMenu menu;

    menu.Append( MYID_INCLUDE_DNP, _( "Include 'DNP' Symbols" ),
                 _( "Show symbols marked 'DNP' in the table and include them on export." ), wxITEM_CHECK );
    menu.Check( MYID_INCLUDE_DNP, !m_dataModel->GetExcludeDNP() );

    menu.Append( MYID_INCLUDE_EXCLUDED_FROM_BOM, _( "Include 'Exclude from BOM' Symbols" ),
                 _( "Show symbols marked 'Exclude from BOM' in the table. They are never included on export." ),
                 wxITEM_CHECK );
    menu.Check( MYID_INCLUDE_EXCLUDED_FROM_BOM, m_dataModel->GetIncludeExcludedFromBOM() );

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


std::vector<BOM_PRESET> DIALOG_LIB_FIELDS_TABLE::getBuiltInBomPresets() const
{
    std::vector<BOM_PRESET> presets = BOM_PRESET::BuiltInPresets();

    for( BOM_PRESET& preset : presets )
    {
        if( preset.sortField == GetDefaultFieldName( FIELD_T::REFERENCE, DO_TRANSLATE ) )
            preset.sortField = LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME;

        for( BOM_FIELD& field : preset.fieldsOrdered )
        {
            if( field.name == GetCanonicalFieldName( FIELD_T::REFERENCE ) )
            {
                field.name = LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME;
                field.label = wxS( "Symbol Name" );
                field.groupBy = false;
            }
        }

        std::erase_if( preset.fieldsOrdered,
                       []( const BOM_FIELD& aField )
                       {
                           return aField.name == wxS( "${EXCLUDE_FROM_POS_FILES}" );
                       } );

        if( preset.name == BOM_PRESET::DefaultEditing().name )
        {
            preset.groupSymbols = false;
            preset.fieldsOrdered = {
                { LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME, wxS( "Symbol Name" ), true, false },
                { GetCanonicalFieldName( FIELD_T::REFERENCE ), wxS( "Reference" ), false, false },
                { GetCanonicalFieldName( FIELD_T::VALUE ), wxS( "Value" ), true, false },
                { GetCanonicalFieldName( FIELD_T::FOOTPRINT ), wxS( "Footprint" ), true, false },
                { GetCanonicalFieldName( FIELD_T::DATASHEET ), wxS( "Datasheet" ), true, false },
                { GetCanonicalFieldName( FIELD_T::DESCRIPTION ), wxS( "Description" ), false, false },
                { wxS( "Keywords" ), wxS( "Keywords" ), true, false },
                { wxS( "${EXCLUDE_FROM_BOM}" ), wxS( "Exclude From BOM" ), true, false },
                { wxS( "${EXCLUDE_FROM_SIM}" ), wxS( "Exclude From Simulation" ), true, false },
                { wxS( "${EXCLUDE_FROM_BOARD}" ), wxS( "Exclude From Board" ), true, false },
                { wxS( "${SYMBOL_IS_POWER}" ), wxS( "Power Symbol" ), true, false },
                { wxS( "${SYMBOL_IS_LOCAL_POWER}" ), wxS( "Local Power Symbol" ), true, false },
            };
        }
    }

    return presets;
}


void DIALOG_LIB_FIELDS_TABLE::OnSaveAndContinue( wxCommandEvent& aEvent )
{
    if( TransferDataFromWindow() )
    {
        m_cfgBomSettings.m_BomExportFileName = m_outputFileName->GetValue();
        ClearModify();
    }
}


void DIALOG_LIB_FIELDS_TABLE::OnCancel( wxCommandEvent& aEvent )
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


void DIALOG_LIB_FIELDS_TABLE::OnOk( wxCommandEvent& aEvent )
{
    if( !TransferDataFromWindow() )
        return;

    m_cfgBomSettings.m_BomExportFileName = m_outputFileName->GetValue();
    EndModal( wxID_OK );
}


void DIALOG_LIB_FIELDS_TABLE::OnClose( wxCloseEvent& aEvent )
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


bool DIALOG_LIB_FIELDS_TABLE::resolveTextVar( wxString* aToken ) const
{
    return Prj().TextVarResolver( aToken );
}
