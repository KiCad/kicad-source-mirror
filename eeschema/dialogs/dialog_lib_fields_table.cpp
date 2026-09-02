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
#include <functional>
#include <set>

#include <confirm.h>
#include <dialog_lib_new_symbol.h>
#include <eda_doc.h>
#include <fields_grid_table.h>
#include <fields_view_controls_grid_data_model.h>
#include <grid_tricks.h>
#include <kiface_base.h>
#include <pgm_base.h>
#include <project.h>
#include <project_sch.h>
#include <settings/common_settings.h>
#include <symbol_edit_frame.h>
#include <symbol_editor/lib_symbol_library_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <template_fieldnames.h>
#include <trace_helpers.h>
#include <validators.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/grid_text_helpers.h>
#include <widgets/std_bitmap_button.h>
#include <wx/arrstr.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>

using DIALOG_NEW_SYMBOL = DIALOG_LIB_NEW_SYMBOL;
using SCOPE = LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SCOPE;


class LIB_SYMBOL_REFERENCE_VALIDATOR : public FIELD_VALIDATOR
{
public:
    LIB_SYMBOL_REFERENCE_VALIDATOR( std::function<bool()> aAllowEmpty ) :
            FIELD_VALIDATOR( FIELD_T::REFERENCE ),
            m_allowEmpty( aAllowEmpty )
    {
    }

    LIB_SYMBOL_REFERENCE_VALIDATOR( const LIB_SYMBOL_REFERENCE_VALIDATOR& aOther ) :
            FIELD_VALIDATOR( aOther ),
            m_allowEmpty( aOther.m_allowEmpty )
    {
    }

    wxObject* Clone() const override { return new LIB_SYMBOL_REFERENCE_VALIDATOR( *this ); }

    bool Validate( wxWindow* aParent ) override
    {
        wxTextEntry* const text = GetTextEntry();

        if( text && text->GetValue().IsEmpty() && m_allowEmpty() )
            return true;

        return FIELD_VALIDATOR::Validate( aParent );
    }

private:
    std::function<bool()> m_allowEmpty;
};


static GRID_CELL_URL_EDITOR_CONTEXT getDatasheetContext( const std::vector<LIB_SYMBOL*>& aSymbols )
{
    std::vector<EMBEDDED_FILES*> embedTargets;
    std::vector<EMBEDDED_FILES*> inheritedFiles;

    for( LIB_SYMBOL* symbol : aSymbols )
    {
        if( !symbol )
            continue;

        embedTargets.push_back( symbol->GetEmbeddedFiles() );
        symbol->AppendParentEmbeddedFiles( inheritedFiles );
    }

    return MakeGridCellUrlEditorContext( embedTargets, inheritedFiles );
}


enum
{
    MYID_SELECT_FOOTPRINT = FIELDS_TABLE_GRID_TRICKS::FIRST_CLIENT_ID,
    MYID_SHOW_DATASHEET,
    MYID_CREATE_DERIVED_SYMBOL,
    MYID_INCLUDE_DNP,
    MYID_INCLUDE_EXCLUDED_FROM_BOM
};


class LIB_FIELDS_EDITOR_GRID_TRICKS : public FIELDS_TABLE_GRID_TRICKS
{
public:
    LIB_FIELDS_EDITOR_GRID_TRICKS( DIALOG_LIB_FIELDS_TABLE* aParent, WX_GRID* aGrid,
                                   VIEW_CONTROLS_GRID_DATA_MODEL*     aViewFieldsData,
                                   LIB_FIELDS_EDITOR_GRID_DATA_MODEL* aDataModel ) :
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

        wxMenuItem* deriveMenu = aMenu.Append( MYID_CREATE_DERIVED_SYMBOL, _( "Create Derived Symbol" ),
                                               _( "Create a new symbol derived from the selected one" ) );

        if( row >= 0 && col >= 0 )
        {
            deriveMenu->Enable( m_grid->IsEditable() && m_dataModel->IsRowSingleSymbol( row ) );

            if( m_dataModel->GetColFieldName( col ) == GetDefaultFieldName( FIELD_T::FOOTPRINT, UNTRANSLATED ) )
            {
                wxMenuItem* selectFootprint =
                        aMenu.Append( MYID_SELECT_FOOTPRINT, _( "Select Footprint..." ), _( "Browse for footprint" ) );
                selectFootprint->Enable( m_grid->IsEditable() );
                aMenu.AppendSeparator();
            }
            else if( m_dataModel->GetColFieldName( col ) == GetDefaultFieldName( FIELD_T::DATASHEET, UNTRANSLATED ) )
            {
                aMenu.Append( MYID_SHOW_DATASHEET, _( "Show Datasheet" ), _( "Show datasheet in browser" ) );
                aMenu.AppendSeparator();
            }
        }
        else
        {
            deriveMenu->Enable( false );
        }

        GRID_TRICKS::showPopupMenu( aMenu, aEvent );
    }

    void doFieldsTablePopupSelection( wxCommandEvent& aEvent ) override
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( aEvent.GetId() == MYID_SELECT_FOOTPRINT )
        {
            if( !m_grid->IsEditable() )
                return;

            // pick a footprint using the footprint picker.
            wxString fpid = m_grid->GetCellValue( row, col );

            wxString symbolNetlist =
                    BuildFootprintChooserSymbolNetlist( m_dataModel->GetRowReferences( row ) );

            if( SelectFootprintFromChooser( m_dlg, fpid, symbolNetlist ) )
                m_grid->SetCellValue( row, col, fpid );
        }
        else if( aEvent.GetId() == MYID_SHOW_DATASHEET )
        {
            wxString datasheetUri = m_grid->GetCellValue( row, col );
            GRID_CELL_URL_EDITOR_CONTEXT context = getDatasheetContext( m_dataModel->GetRowReferences( row ) );

            GetAssociatedDocument( m_dlg, datasheetUri, &m_dlg->Prj(), PROJECT_SCH::SchSearchS( &m_dlg->Prj() ),
                                   context.m_filesStack );
        }
        else if( aEvent.GetId() == MYID_CREATE_DERIVED_SYMBOL )
        {
            if( !m_grid->IsEditable() )
                return;

            EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( m_dlg->GetParent() );
            wxCHECK( frame, /* void */ );

            const LIB_SYMBOL* parentSymbol = m_dataModel->GetSymbolForRow( row );

            wxArrayString symbolNames;
            wxArrayString derivedSymbols;
            m_dataModel->GetSymbolNames( symbolNames, SYMBOL_NAME_FILTER::ALL );
            m_dataModel->GetSymbolNames( derivedSymbols, SYMBOL_NAME_FILTER::DERIVED_ONLY );

            auto validator =
                    [&]( const wxString& aNewName ) -> bool
                    {
                        return symbolNames.Index( UnescapeString( aNewName ) ) == wxNOT_FOUND;
                    };

            const auto styler =
                    [&]( const wxString& aItem ) -> int
                    {
                        for( wxString& candidate : derivedSymbols )
                        {
                            if( candidate.CmpNoCase( aItem ) == 0 )
                                return ITALIC;
                        }

                        return 0;
                    };

            DIALOG_NEW_SYMBOL dlg( frame, symbolNames, styler, parentSymbol->GetName(), validator );

            if( dlg.ShowModal() != wxID_OK )
                return;

            wxString derivedName = dlg.GetName();
            m_dataModel->CreateDerivedSymbolImmediate( row, col, derivedName );

            if( m_dataModel->IsEdited() )
                m_dlg->OnModify();

            m_grid->ForceRefresh();
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
                if( m_viewControlsDataModel->GetUntranslatedFieldName( row ) == fieldName )
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


DIALOG_LIB_FIELDS_TABLE::DIALOG_LIB_FIELDS_TABLE( SYMBOL_EDIT_FRAME* aParent, SCOPE aScope ) :
        DIALOG_FIELDS_TABLE( aParent, aParent->libeditconfig()->m_LibFieldEditor,
                             aParent->libeditconfig()->m_LibFieldEditorBom, nullptr ),
        m_parent( aParent )
{
    loadSymbols();

    const wxString& libName = m_parent->GetTargetLibId().GetLibNickname();
    const bool      readOnly = m_parent->GetLibManager().IsLibraryReadOnly( libName );

    m_dataModel = new LIB_FIELDS_EDITOR_GRID_DATA_MODEL( m_symbolsList );
    m_dataModel->SetScope( aScope );

    const wxString& targetSymbolName = m_parent->GetTargetLibId().GetLibItemName();

    for( LIB_SYMBOL* symbol : m_symbolsList )
    {
        if( symbol->GetName() == targetSymbolName )
        {
            if( std::shared_ptr<LIB_SYMBOL> root = symbol->GetRootSymbol() )
                m_dataModel->SetRelatedSymbolRoot( root->GetName() );

            break;
        }
    }

    m_grid->UseNativeColHeader( true );
    m_grid->SetTable( m_dataModel, true );

    // The field-list grid regroups its rows, so the dialog's position-based Ctrl+Z would shift
    // values onto the wrong field.
    ExcludeFromControlUndoRedo( m_viewControlsGrid );

    // must be done after SetTable(), which appears to re-set it
    m_grid->SetSelectionMode( wxGrid::wxGridSelectCells );

    // add Cut, Copy, and Paste to wxGrid
    m_grid->PushEventHandler( new LIB_FIELDS_EDITOR_GRID_TRICKS( this, m_grid, m_viewControlsDataModel,
                                                                 m_dataModel ) );

    m_splitter_left->Unsplit( m_variantsPanel );
    m_variantsPanel->Hide();

    m_scope->Clear();
    m_scope->Append( _( "Whole Library" ) );
    m_scope->Append( _( "Related Symbols Only" ) );
    m_scope->SetSelection( static_cast<int>( m_dataModel->GetScope() ) );
    m_filterScope->SetString( static_cast<int>( BOM_FILTER_SCOPE::REFERENCE ), _( "Symbol Names" ) );

    wxString title = wxString::Format( _( "Symbol Fields Table ('%s' Library)" ), libName );

    if( readOnly )
        title += wxS( " " ) + _( "[Read Only]" );

    SetTitle( title );
    m_buttonApply->SetLabel( _( "Apply" ) );

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
    m_grid->Bind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FIELDS_TABLE::OnColSort, this );
    m_grid->Bind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FIELDS_TABLE::OnColMove, this );
    m_grid->GetGridWindow()->Bind( wxEVT_MOTION, &DIALOG_LIB_FIELDS_TABLE::OnGridMouseMove, this );
    m_cbBomPresets->Bind( wxEVT_CHOICE, &DIALOG_LIB_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Bind( wxEVT_CHOICE, &DIALOG_LIB_FIELDS_TABLE::onBomFmtPresetChanged, this );
}


void DIALOG_LIB_FIELDS_TABLE::loadSymbols()
{
    m_symbolsList.clear();

    LIB_SYMBOL_LIBRARY_MANAGER& libMgr = m_parent->GetLibManager();
    wxString                    libName = m_parent->GetTargetLibId().GetLibNickname();
    wxArrayString               symbolNames;

    libMgr.GetSymbolNames( libName, symbolNames );

    if( symbolNames.IsEmpty() )
    {
        wxMessageBox( wxString::Format( _( "No symbols found in library '%s'." ), libName ) );
        return;
    }

    for( const wxString& symbolName : symbolNames )
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
        wxMessageBox( _( "No symbols could be loaded from the library." ) );
}


DIALOG_LIB_FIELDS_TABLE::~DIALOG_LIB_FIELDS_TABLE()
{
    savePresets( true );
    SavePanelLayout();
    SaveColumnWidths();

    // Disconnect Events
    m_grid->GetGridWindow()->Unbind( wxEVT_MOTION, &DIALOG_LIB_FIELDS_TABLE::OnGridMouseMove, this );
    m_grid->Unbind( wxEVT_GRID_COL_SORT, &DIALOG_LIB_FIELDS_TABLE::OnColSort, this );
    m_grid->Unbind( wxEVT_GRID_COL_MOVE, &DIALOG_LIB_FIELDS_TABLE::OnColMove, this );
    m_cbBomPresets->Unbind( wxEVT_CHOICE, &DIALOG_LIB_FIELDS_TABLE::onBomPresetChanged, this );
    m_cbBomFmtPresets->Unbind( wxEVT_CHOICE, &DIALOG_LIB_FIELDS_TABLE::onBomFmtPresetChanged, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );

    // we gave ownership of m_viewControlsDataModel & m_dataModel to the wxGrids...
}


/**
 * Lib symbols have different rules for references. Derived symbols are allowed
 * to have a blank reference to mean inherit-from-parent.
 */
wxGridCellEditor* DIALOG_LIB_FIELDS_TABLE::createReferenceEditor()
{
    auto rowAllowsEmptyReference =
            [this]()
            {
                int row = m_grid->GetGridCursorRow();

                if( row < 0 || row >= m_dataModel->GetNumberRows() )
                    return false;

                std::vector<LIB_SYMBOL*> symbols = m_dataModel->GetRowReferences( row );

                if( symbols.empty() )
                    return false;

                // Can't have any parent symbols with empty references
                for( LIB_SYMBOL* symbol : symbols )
                {
                    if( symbol->IsRoot() )
                        return false;
                }

                return true;
            };

    GRID_CELL_TEXT_EDITOR* editor = new GRID_CELL_TEXT_EDITOR;
    editor->SetValidator( LIB_SYMBOL_REFERENCE_VALIDATOR( rowAllowsEmptyReference ) );
    return editor;
}


wxGridCellEditor* DIALOG_LIB_FIELDS_TABLE::createDatasheetEditor()
{
    return new GRID_CELL_URL_EDITOR(
            this, PROJECT_SCH::SchSearchS( &Prj() ),
            [this]( int aRow )
            {
                return getDatasheetContext( m_dataModel->GetRowReferences( aRow ) );
            } );
}


wxGridCellEditor* DIALOG_LIB_FIELDS_TABLE::createFootprintEditor()
{
    return new GRID_CELL_FPID_EDITOR(
            this,
            [this]( int aRow )
            {
                return BuildFootprintChooserSymbolNetlist( m_dataModel->GetRowReferences( aRow ) );
            } );
}


bool DIALOG_LIB_FIELDS_TABLE::TransferDataToWindow()
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


bool DIALOG_LIB_FIELDS_TABLE::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    wxString symbolName;
    wxString errorMessage;

    if( !m_dataModel->ValidateReferences( symbolName, errorMessage ) )
    {
        DisplayErrorMessage( this, wxString::Format( _( "Invalid reference for symbol '%s'." ), symbolName ),
                             errorMessage );
        return false;
    }

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    std::set<KIID_PATH> savedSelection = SaveGridSelection();
    bool                updateCanvas = false;

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
    RestoreGridSelection( savedSelection );
    m_parent->RefreshLibraryTree();

    if( updateCanvas )
    {
        m_parent->OnModify();
        m_parent->HardRedraw();
    }

    return true;
}


void DIALOG_LIB_FIELDS_TABLE::LoadFieldNames()
{
    auto addMandatoryField =
            [&]( FIELD_T aFieldId, bool aShow, bool aGroupBy )
            {
                m_mandatoryFieldListIndexes[aFieldId] = m_viewControlsDataModel->GetNumberRows();

                AddField( GetDefaultFieldName( aFieldId, UNTRANSLATED ), GetDefaultFieldName( aFieldId, TRANSLATED ),
                          aShow, aGroupBy );
            };

    AddField( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME, _( "Symbol Name" ), true, false );

    // Add mandatory fields first            show   groupBy
    addMandatoryField( FIELD_T::REFERENCE,   false,  false  );
    addMandatoryField( FIELD_T::VALUE,       true,   false  );
    addMandatoryField( FIELD_T::FOOTPRINT,   true,   false  );
    addMandatoryField( FIELD_T::DATASHEET,   true,   false  );
    addMandatoryField( FIELD_T::DESCRIPTION, false,  false  );

    // Generated fields present only in the fields table
    AddField( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_KEYWORDS, _( "Keywords" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_BOM}" ), _( "Exclude From BOM" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_SIM}" ), _( "Exclude From Simulation" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_BOARD}" ), _( "Exclude From Board" ), true, false );
    AddField( wxS( "${EXCLUDE_FROM_POS_FILES}" ), _( "Exclude From Position Files" ), true, false );
    AddField( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_IS_POWER, _( "Power Symbol" ), true, false );
    AddField( LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_IS_LOCAL_POWER, _( "Local Power Symbol" ), true, false );

    // User field names are stored and matched case-sensitively (see issue #24021), so each
    // distinct name gets its own column rather than collapsing case variants together.
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

    // Add any global template field names which aren't already present.
    for( const TEMPLATE_FIELDNAME& templateField :
         Pgm().GetCommonSettings()->m_FieldNameTemplates.GetTemplateFieldNames(
                 TEMPLATES::SCOPE::GLOBAL ) )
    {
        if( userFieldNames.count( templateField.m_Name ) == 0 )
            AddField( templateField.m_Name, GetGeneratedFieldDisplayName( templateField.m_Name ), false, false );
    }
}


void DIALOG_LIB_FIELDS_TABLE::setScope( SCOPE aScope )
{
    m_dataModel->SetScope( aScope );
    m_dataModel->RebuildRows();
}


void DIALOG_LIB_FIELDS_TABLE::OnScope( wxCommandEvent& aEvent )
{
    switch( aEvent.GetSelection() )
    {
    case 0: setScope( SCOPE::SCOPE_LIBRARY );         break;
    case 1: setScope( SCOPE::SCOPE_RELATED_SYMBOLS ); break;
    }
}


void DIALOG_LIB_FIELDS_TABLE::OnMenu( wxCommandEvent& aEvent )
{
    // Build a pop menu:
    wxMenu menu;

    menu.Append( MYID_INCLUDE_DNP, _( "Include 'DNP' Symbols" ),
                 _( "Show symbols marked 'DNP' in the table.  This setting also controls whether or not 'DNP' "
                    "symbols are included on export." ),
                 wxITEM_CHECK );
    menu.Check( MYID_INCLUDE_DNP, !m_dataModel->GetExcludeDNP() );

    menu.Append( MYID_INCLUDE_EXCLUDED_FROM_BOM, _( "Include 'Exclude from BOM' Symbols" ),
                 _( "Show symbols marked 'Exclude from BOM' in the table.  Symbols marked 'Exclude from BOM' "
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


std::vector<BOM_PRESET> DIALOG_LIB_FIELDS_TABLE::getBuiltInBomPresets() const
{
    std::vector<BOM_PRESET> presets = BOM_PRESET::BuiltInPresets();

    for( BOM_PRESET& preset : presets )
    {
        if( preset.sortField == GetDefaultFieldName( FIELD_T::REFERENCE, TRANSLATED ) )
            preset.sortField = LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME;

        for( BOM_FIELD& field : preset.fieldsOrdered )
        {
            if( field.name == GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ) )
            {
                field.name = LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME;
                field.label = wxS( "Symbol Name" );
                field.groupBy = false;
            }
        }

        if( preset.name == BOM_PRESET::DefaultEditing().name )
        {
            preset.groupSymbols = false;
            preset.fieldsOrdered = {
                { LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_NAME, wxS( "Symbol Name" ), true, false },
                { GetDefaultFieldName( FIELD_T::REFERENCE, UNTRANSLATED ), wxS( "Reference" ), false, false },
                { GetDefaultFieldName( FIELD_T::VALUE, UNTRANSLATED ), wxS( "Value" ), true, false },
                { GetDefaultFieldName( FIELD_T::FOOTPRINT, UNTRANSLATED ), wxS( "Footprint" ), true, false },
                { GetDefaultFieldName( FIELD_T::DATASHEET, UNTRANSLATED ), wxS( "Datasheet" ), true, false },
                { GetDefaultFieldName( FIELD_T::DESCRIPTION, UNTRANSLATED ), wxS( "Description" ), false, false },
                { LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_KEYWORDS, wxS( "Keywords" ), true, false },
                { wxS( "${EXCLUDE_FROM_BOM}" ), wxS( "Exclude From BOM" ), true, false },
                { wxS( "${EXCLUDE_FROM_SIM}" ), wxS( "Exclude From Simulation" ), true, false },
                { wxS( "${EXCLUDE_FROM_BOARD}" ), wxS( "Exclude From Board" ), true, false },
                { wxS( "${EXCLUDE_FROM_POS_FILES}" ), wxS( "Exclude From Position Files" ), true, false },
                { LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_IS_POWER, wxS( "Power Symbol" ), true, false },
                { LIB_FIELDS_EDITOR_GRID_DATA_MODEL::SYMBOL_IS_LOCAL_POWER, wxS( "Local Power Symbol" ), true, false },
            };
        }
    }

    return presets;
}


wxString DIALOG_LIB_FIELDS_TABLE::resolveVariant() const
{
    return wxEmptyString;
}


bool DIALOG_LIB_FIELDS_TABLE::resolveTextVar( wxString* aToken ) const
{
    return Prj().TextVarResolver( aToken );
}
