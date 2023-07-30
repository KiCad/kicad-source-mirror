/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <confirm.h>
#include <dialogs/dialog_text_entry.h>
#include <kiway.h>
#include <symbol_edit_frame.h>
#include <lib_symbol_library_manager.h>
#include <math/util.h> // for KiROUND
#include <sch_symbol.h>
#include <kiplatform/ui.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/wx_grid.h>
#include <widgets/std_bitmap_button.h>
#include <string_utils.h>

#include <dialog_sim_model.h>

#include <dialog_lib_symbol_properties.h>
#include <settings/settings_manager.h>
#include <symbol_editor_settings.h>


int DIALOG_LIB_SYMBOL_PROPERTIES::m_lastOpenedPage = 0;
DIALOG_LIB_SYMBOL_PROPERTIES::LAST_LAYOUT
    DIALOG_LIB_SYMBOL_PROPERTIES::m_lastLayout = DIALOG_LIB_SYMBOL_PROPERTIES::NONE;


DIALOG_LIB_SYMBOL_PROPERTIES::DIALOG_LIB_SYMBOL_PROPERTIES( SYMBOL_EDIT_FRAME* aParent,
                                                            LIB_SYMBOL* aLibEntry ) :
    DIALOG_LIB_SYMBOL_PROPERTIES_BASE( aParent ),
    m_Parent( aParent ),
    m_libEntry( aLibEntry ),
    m_pinNameOffset( aParent, m_nameOffsetLabel, m_nameOffsetCtrl, m_nameOffsetUnits, true ),
    m_delayedFocusCtrl( nullptr ),
    m_delayedFocusGrid( nullptr ),
    m_delayedFocusRow( -1 ),
    m_delayedFocusColumn( -1 ),
    m_delayedFocusPage( -1 )
{
    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );
    m_fields = new FIELDS_GRID_TABLE<LIB_FIELD>( this, aParent, m_grid, m_libEntry );
    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this,
                                                      [&]( wxCommandEvent& aEvent )
                                                      {
                                                          OnAddField( aEvent );
                                                      } ) );
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // Show/hide columns according to the user's preference
    SYMBOL_EDITOR_SETTINGS* cfg = m_Parent->GetSettings();
    m_grid->ShowHideColumns( cfg->m_EditSymbolVisibleColumns );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_URL_EDITOR( this, Prj().SchSearchS() ) );
    m_grid->SetAttr( DATASHEET_FIELD, FDC_VALUE, attr );

    m_SymbolNameCtrl->SetValidator( FIELD_VALIDATOR( VALUE_FIELD ) );

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_bpDelete->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_bpMoveUp->SetBitmap( KiBitmap( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmap( BITMAPS::small_down ) );
    m_addFilterButton->SetBitmap( KiBitmap( BITMAPS::small_plus ) );
    m_deleteFilterButton->SetBitmap( KiBitmap( BITMAPS::small_trash ) );
    m_editFilterButton->SetBitmap( KiBitmap( BITMAPS::small_edit ) );

    SetupStandardButtons();

    if( aParent->IsSymbolFromLegacyLibrary() && !aParent->IsSymbolFromSchematic() )
    {
        m_stdSizerButtonCancel->SetDefault();
        m_stdSizerButtonOK->SetLabel( _( "Read Only" ) );
        m_stdSizerButtonOK->Enable( false );
    }

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING,
                     wxGridEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanging ),
                     nullptr, this );

    if( m_lastLayout != DIALOG_LIB_SYMBOL_PROPERTIES::NONE )
    {
        if( ( m_lastLayout == DIALOG_LIB_SYMBOL_PROPERTIES::ALIAS && aLibEntry->IsRoot() )
          || ( m_lastLayout == DIALOG_LIB_SYMBOL_PROPERTIES::PARENT && aLibEntry->IsAlias() ) )
        {
            resetSize();
        }
    }

    m_lastLayout = ( aLibEntry->IsAlias() ) ? DIALOG_LIB_SYMBOL_PROPERTIES::ALIAS
                                            : DIALOG_LIB_SYMBOL_PROPERTIES::PARENT;

    m_grid->GetParent()->Layout();
    syncControlStates( m_libEntry->IsAlias() );
    Layout();

    finishDialogSettings();
}


DIALOG_LIB_SYMBOL_PROPERTIES::~DIALOG_LIB_SYMBOL_PROPERTIES()
{
    m_lastOpenedPage = m_NoteBook->GetSelection( );

    if( SYMBOL_EDITOR_SETTINGS* cfg = m_Parent->GetSettings() )
        cfg->m_EditSymbolVisibleColumns = m_grid->GetShownColumnsAsString();

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                        wxGridEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanging ),
                        nullptr, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool DIALOG_LIB_SYMBOL_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    // Push a copy of each field into m_updateFields
    m_libEntry->GetFields( *m_fields );

    // The Y axis for components in lib is from bottom to top while the screen axis is top
    // to bottom: we must change the y coord sign for editing
    for( size_t i = 0; i < m_fields->size(); ++i )
    {
        VECTOR2I pos = m_fields->at( i ).GetPosition();
        pos.y = -pos.y;
        m_fields->at( i ).SetPosition( pos );
    }

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->GetNumberRows() );
    m_grid->ProcessTableMessage( msg );
    adjustGridColumns();

    m_SymbolNameCtrl->ChangeValue( UnescapeString( m_libEntry->GetName() ) );

    m_KeywordCtrl->ChangeValue( m_libEntry->GetKeyWords() );
    m_SelNumberOfUnits->SetValue( m_libEntry->GetUnitCount() );
    m_OptionPartsInterchangeable->SetValue( !m_libEntry->UnitsLocked() ||
                                            m_libEntry->GetUnitCount() == 1 );

    // If a symbol contains no conversion-specific pins or graphic items, symbol->HasConversion()
    // will return false.  But when editing a symbol with DeMorgan option set, we don't want to
    // keep turning it off just because there aren't any conversion-specific items yet, so we force
    // it to on if the parent frame has it enabled.
    m_AsConvertButt->SetValue( m_Parent->GetShowDeMorgan() );

    m_OptionPower->SetValue( m_libEntry->IsPower() );

    if( m_libEntry->IsPower() )
        m_spiceFieldsButton->Hide();

    LIB_FIELD* simEnableField = m_libEntry->FindField( SIM_ENABLE_FIELD );
    m_excludeFromSim->SetValue( simEnableField && simEnableField->GetText() == wxT( "0" ) );

    m_excludeFromBomCheckBox->SetValue( m_libEntry->GetExcludedFromBOM() );
    m_excludeFromBoardCheckBox->SetValue( m_libEntry->GetExcludedFromBoard() );

    m_ShowPinNumButt->SetValue( m_libEntry->ShowPinNumbers() );
    m_ShowPinNameButt->SetValue( m_libEntry->ShowPinNames() );
    m_PinsNameInsideButt->SetValue( m_libEntry->GetPinNameOffset() != 0 );
    m_pinNameOffset.ChangeValue( m_libEntry->GetPinNameOffset() );

    wxArrayString tmp = m_libEntry->GetFPFilters();
    m_FootprintFilterListBox->Append( tmp );

    // Populate the list of root parts for inherited objects.
    if( m_libEntry->IsAlias() )
    {
        wxArrayString rootSymbolNames;
        wxString libName = m_Parent->GetCurLib();

        // Someone forgot to set the current library in the editor frame window.
        wxCHECK( !libName.empty(), false );

        m_Parent->GetLibManager().GetRootSymbolNames( libName, rootSymbolNames );
        m_inheritanceSelectCombo->Append( rootSymbolNames );

        LIB_SYMBOL_SPTR rootSymbol = m_libEntry->GetParent().lock();

        wxCHECK( rootSymbol, false );

        wxString parentName = UnescapeString( rootSymbol->GetName() );
        int selection = m_inheritanceSelectCombo->FindString( parentName );

        wxCHECK( selection != wxNOT_FOUND, false );
        m_inheritanceSelectCombo->SetSelection( selection );

        m_lastOpenedPage = 0;
    }

    m_NoteBook->SetSelection( (unsigned) m_lastOpenedPage );

    return true;
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnExcludeFromSimulation( wxCommandEvent& event )
{
    int simEnableFieldRow = -1;

    for( int ii = MANDATORY_FIELDS; ii < m_grid->GetNumberRows(); ++ii )
    {
        if( m_grid->GetCellValue( ii, FDC_NAME ) == SIM_ENABLE_FIELD )
            simEnableFieldRow = ii;
    }

    if( event.IsChecked() )
    {
        if( simEnableFieldRow == -1 )
        {
            simEnableFieldRow = (int) m_fields->size();
            m_fields->emplace_back( m_libEntry, simEnableFieldRow, SIM_ENABLE_FIELD );

            // notify the grid
            wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
            m_grid->ProcessTableMessage( msg );
        }

        m_grid->SetCellValue( simEnableFieldRow, FDC_VALUE, wxT( "0" ) );
        m_grid->SetCellValue( simEnableFieldRow, FDC_SHOWN, wxT( "0" ) );
        m_grid->SetCellValue( simEnableFieldRow, FDC_SHOW_NAME, wxT( "0" ) );
    }
    else if( simEnableFieldRow >= 0 )
    {
        m_fields->erase( m_fields->begin() + simEnableFieldRow );

        // notify the grid
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, simEnableFieldRow, 1 );
        m_grid->ProcessTableMessage( msg );
    }

    OnModify();
}


bool DIALOG_LIB_SYMBOL_PROPERTIES::Validate()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Alias symbol reference can be empty because it inherits from the parent symbol.
    if( m_libEntry->IsRoot() &&
        !SCH_SYMBOL::IsReferenceStringValid( m_fields->at( REFERENCE_FIELD ).GetText() ) )
    {
        if( m_NoteBook->GetSelection() != 0 )
            m_NoteBook->SetSelection( 0 );

        m_delayedErrorMessage = _( "References must start with a letter." );
        m_delayedFocusGrid = m_grid;
        m_delayedFocusColumn = FDC_VALUE;
        m_delayedFocusRow = REFERENCE_FIELD;
        m_delayedFocusPage = 0;

        return false;
    }

    // Check for missing field names.
    for( int ii = MANDATORY_FIELDS; ii < (int) m_fields->size(); ++ii )
    {
        LIB_FIELD& field = m_fields->at( ii );
        wxString   fieldName = field.GetName( false );

        if( fieldName.IsEmpty() && !field.GetText().IsEmpty() )
        {
            if( m_NoteBook->GetSelection() != 0 )
                m_NoteBook->SetSelection( 0 );

            m_delayedErrorMessage = _( "Fields must have a name." );
            m_delayedFocusGrid = m_grid;
            m_delayedFocusColumn = FDC_NAME;
            m_delayedFocusRow = ii;
            m_delayedFocusPage = 0;

            return false;
        }
    }

    // Verify that the parent name is set if the symbol is inherited
    if( m_libEntry->IsAlias() )
    {
        wxString parentName = m_inheritanceSelectCombo->GetValue();

        if( parentName.IsEmpty() )
        {
            m_delayedErrorMessage = _( "Derived symbol must have a parent selected" );

            return false;
        }
    }

    /*
     * Confirm destructive actions.
     */

    if( m_SelNumberOfUnits->GetValue() < m_libEntry->GetUnitCount() )
    {
        if( !IsOK( this, _( "Delete extra units from symbol?" ) ) )
            return false;
    }

    if( !m_AsConvertButt->GetValue() && m_libEntry->HasConversion() )
    {
        if( !IsOK( this, _( "Delete alternate body style (De Morgan) from symbol?" ) ) )
            return false;
    }

    return true;
}


bool DIALOG_LIB_SYMBOL_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( !m_grid->CommitPendingChanges() )
        return false;

    wxString   newName = EscapeString( m_SymbolNameCtrl->GetValue(), CTX_LIBID );
    wxString   oldName = m_libEntry->GetName();

    if( oldName != newName )
    {
        wxString libName = m_Parent->GetCurLib();

        if( m_Parent->GetLibManager().SymbolExists( newName, libName ) )
        {
            wxString msg;

            msg.Printf( _( "Symbol name '%s' already in use in library '%s'." ),
                        UnescapeString( newName ),
                        libName );
            DisplayErrorMessage( this, msg );
            return false;
        }

        m_Parent->SaveCopyInUndoList( _( "Edit Symbol Properties" ), m_libEntry,
                                      UNDO_REDO::LIB_RENAME );
    }
    else
    {
        m_Parent->SaveCopyInUndoList( _( "Edit Symbol Properties" ), m_libEntry );
    }

    // The Y axis for components in lib is from bottom to top while the screen axis is top
    // to bottom: we must change the y coord sign when writing back to the library
    for( int ii = 0; ii < (int) m_fields->size(); ++ii )
    {
        VECTOR2I pos = m_fields->at( ii ).GetPosition();
        pos.y = -pos.y;
        m_fields->at( ii ).SetPosition( pos );
        m_fields->at( ii ).SetId( ii );
    }

    for( int ii = m_fields->GetNumberRows() - 1; ii >= MANDATORY_FIELDS; ii-- )
    {
        LIB_FIELD&      field = m_fields->at( ii );
        const wxString& fieldName = field.GetCanonicalName();

        if( fieldName.IsEmpty() && field.GetText().IsEmpty() )
            m_fields->erase( m_fields->begin() + ii );
        else if( fieldName.IsEmpty() )
            field.SetName( _( "untitled" ) );
    }

    m_libEntry->SetFields( *m_fields );

    // Update the parent for inherited symbols
    if( m_libEntry->IsAlias() )
    {
        wxString parentName = EscapeString( m_inheritanceSelectCombo->GetValue(), CTX_LIBID );

        // The parentName was verified to be non-empty in the Validator
        wxString libName = m_Parent->GetCurLib();

        // Get the parent from the libManager based on the name set in the inheritance combo box.
        LIB_SYMBOL* newParent = m_Parent->GetLibManager().GetAlias( parentName, libName );

        // Verify that the requested parent exists
        wxCHECK( newParent, false );

        // Verify that the new parent is not an alias.
        wxCHECK( !newParent->IsAlias(), false );

        m_libEntry->SetParent( newParent );
    }

    m_libEntry->SetName( newName );
    m_libEntry->SetKeyWords( m_KeywordCtrl->GetValue() );
    m_libEntry->SetUnitCount( m_SelNumberOfUnits->GetValue() );
    m_libEntry->LockUnits( m_libEntry->GetUnitCount() > 1 &&
                           !m_OptionPartsInterchangeable->GetValue() );
    m_libEntry->SetConversion( m_AsConvertButt->GetValue() );
    m_Parent->SetShowDeMorgan( m_AsConvertButt->GetValue() );

    if( m_OptionPower->GetValue() )
    {
        m_libEntry->SetPower();
        // Power symbols must have value matching name for now
        m_libEntry->GetValueField().SetText( newName );
    }
    else
    {
        m_libEntry->SetNormal();
    }

    m_libEntry->SetExcludedFromBOM( m_excludeFromBomCheckBox->GetValue() );
    m_libEntry->SetExcludedFromBoard( m_excludeFromBoardCheckBox->GetValue() );

    m_libEntry->SetShowPinNumbers( m_ShowPinNumButt->GetValue() );
    m_libEntry->SetShowPinNames( m_ShowPinNameButt->GetValue() );

    if( m_PinsNameInsideButt->GetValue() )
    {
        int offset = KiROUND( (double) m_pinNameOffset.GetValue() );

        // We interpret an offset of 0 as "outside", so make sure it's non-zero
        m_libEntry->SetPinNameOffset( offset == 0 ? 20 : offset );
    }
    else
    {
        m_libEntry->SetPinNameOffset( 0 );   // pin text outside the body (name is on the pin)
    }

    m_libEntry->SetFPFilters( m_FootprintFilterListBox->GetStrings());

    m_Parent->UpdateAfterSymbolProperties( &oldName );

    // It's possible that the symbol being edited has no pins, in which case there may be no
    // alternate body style objects causing #LIB_SYMBOL::HasCoversion() to always return false.
    // This allows the user to edit the alternate body style just in case this condition occurs.
    m_Parent->SetShowDeMorgan( m_AsConvertButt->GetValue() );

    return true;
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanging( wxGridEvent& event )
{
    wxGridCellEditor* editor = m_grid->GetCellEditor( event.GetRow(), event.GetCol() );
    wxControl* control = editor->GetControl();

    if( control && control->GetValidator() && !control->GetValidator()->Validate( control ) )
    {
        event.Veto();

        m_delayedFocusGrid = m_grid;
        m_delayedFocusRow = event.GetRow();
        m_delayedFocusColumn = event.GetCol();
        m_delayedFocusPage = 0;
    }
    else if( event.GetCol() == FDC_NAME )
    {
        wxString newName = event.GetString();

        for( int i = 0; i < m_grid->GetNumberRows(); ++i )
        {
            if( i == event.GetRow() )
                continue;

            if( newName.CmpNoCase( m_grid->GetCellValue( i, FDC_NAME ) ) == 0 )
            {
                DisplayError( this, wxString::Format( _( "The name '%s' is already in use." ),
                                                      newName ) );
                event.Veto();
                m_delayedFocusRow = event.GetRow();
                m_delayedFocusColumn = event.GetCol();
            }
        }
    }

    editor->DecRef();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnSymbolNameText( wxCommandEvent& event )
{
    if( m_OptionPower->IsChecked() )
        m_grid->SetCellValue( VALUE_FIELD, FDC_VALUE, m_SymbolNameCtrl->GetValue() );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnSymbolNameKillFocus( wxFocusEvent& event )
{
    if( !m_delayedFocusCtrl )
    {
        // If the validation fails and we throw up a dialog then GTK will give us another
        // KillFocus event and we end up in infinite recursion.  So we use m_delayedFocusCtrl
        // as a re-entrancy block and then clear it again if validation passes.
        m_delayedFocusCtrl = m_SymbolNameCtrl;
        m_delayedFocusPage = 0;

        if( m_SymbolNameCtrl->GetValidator()->Validate( m_SymbolNameCtrl ) )
        {
            m_delayedFocusCtrl = nullptr;
            m_delayedFocusPage = -1;
        }
    }

    event.Skip();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnAddField( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    SYMBOL_EDITOR_SETTINGS* settings = m_Parent->GetSettings();
    int       fieldID = (int) m_fields->size();
    LIB_FIELD newField( m_libEntry, fieldID );

    newField.SetTextSize( VECTOR2I( schIUScale.MilsToIU( settings->m_Defaults.text_size ),
                                    schIUScale.MilsToIU( settings->m_Defaults.text_size ) ) );

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_grid->MakeCellVisible( (int) m_fields->size() - 1, 0 );
    m_grid->SetGridCursor( (int) m_fields->size() - 1, 0 );

    m_grid->EnableCellEditControl();
    m_grid->ShowCellEditControl();

    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    wxArrayInt selectedRows = m_grid->GetSelectedRows();

    if( selectedRows.empty() && m_grid->GetGridCursorRow() >= 0 )
        selectedRows.push_back( m_grid->GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    for( int row : selectedRows )
    {
        if( row < MANDATORY_FIELDS )
        {
            DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                                  MANDATORY_FIELDS ) );
            return;
        }
    }

    m_grid->CommitPendingChanges( true /* quiet mode */ );

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort( []( int* first, int* second ) { return *second - *first; } );

    for( int row : selectedRows )
    {
        m_fields->erase( m_fields->begin() + row );

        // notify the grid
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1 );
        m_grid->ProcessTableMessage( msg );

        if( m_grid->GetNumberRows() > 0 )
        {
            m_grid->MakeCellVisible( std::max( 0, row-1 ), m_grid->GetGridCursorCol() );
            m_grid->SetGridCursor( std::max( 0, row-1 ), m_grid->GetGridCursorCol() );
        }
    }

    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnMoveUp( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int i = m_grid->GetGridCursorRow();

    if( i > MANDATORY_FIELDS )
    {
        LIB_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i - 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i - 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );

        OnModify();
    }
    else
    {
        wxBell();
    }
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnMoveDown( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int i = m_grid->GetGridCursorRow();

    if( i >= MANDATORY_FIELDS && i + 1 < m_fields->GetNumberRows() )
    {
        LIB_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i + 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i + 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );

        OnModify();
    }
    else
    {
        wxBell();
    }
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnEditSpiceModel( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    std::vector<LIB_FIELD> fields;

    for( const LIB_FIELD& field : *m_fields )
        fields.emplace_back( field );

    DIALOG_SIM_MODEL dialog( this, *m_libEntry, fields );

    if( dialog.ShowModal() != wxID_OK )
        return;

    // Add in any new fields
    for( const LIB_FIELD& editedField : fields )
    {
        bool found = false;

        for( LIB_FIELD& existingField : *m_fields )
        {
            if( existingField.GetName() == editedField.GetName() )
            {
                found = true;
                existingField.SetText( editedField.GetText() );
                break;
            }
        }

        if( !found )
        {
            m_fields->emplace_back( editedField );
            wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
            m_grid->ProcessTableMessage( msg );
        }
    }

    // Remove any deleted fields
    for( int ii = (int) m_fields->size() - 1; ii >= 0; /* advance in loop */ )
    {
        LIB_FIELD& existingField = m_fields->at( ii );
        bool       found = false;

        for( LIB_FIELD& editedField : fields )
        {
            if( editedField.GetName() == existingField.GetName() )
            {
                found = true;
                break;
            }
        }

        if( found )
        {
            ii--;
        }
        else
        {
            m_fields->erase( m_fields->begin() + ii );
            wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, ii, 1 );
            m_grid->ProcessTableMessage( msg );
        }
    }

    OnModify();
    m_grid->ForceRefresh();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnFilterDClick( wxMouseEvent& event )
{
    int idx = m_FootprintFilterListBox->HitTest( event.GetPosition() );
    wxCommandEvent dummy;

    if( idx >= 0 )
        OnEditFootprintFilter( dummy );
    else
        OnAddFootprintFilter( dummy );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnCancelButtonClick( wxCommandEvent& event )
{
    // Running the Footprint Browser gums up the works and causes the automatic cancel
    // stuff to no longer work.  So we do it here ourselves.
    EndQuasiModal( wxID_CANCEL );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnAddFootprintFilter( wxCommandEvent& event )
{
    wxString  filterLine;
    WX_TEXT_ENTRY_DIALOG dlg( this, _( "Filter:" ), _( "Add Footprint Filter" ), filterLine );

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetValue().IsEmpty() )
        return;

    filterLine = dlg.GetValue();
    filterLine.Replace( wxT( " " ), wxT( "_" ) );

    // duplicate filters do no harm, so don't be a nanny.

    m_FootprintFilterListBox->Append( filterLine );
    m_FootprintFilterListBox->SetSelection( (int) m_FootprintFilterListBox->GetCount() - 1 );

    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnDeleteFootprintFilter( wxCommandEvent& event )
{
    int ii = m_FootprintFilterListBox->GetSelection();

    if( ii >= 0 )
    {
        m_FootprintFilterListBox->Delete( (unsigned) ii );

        if( m_FootprintFilterListBox->GetCount() == 0 )
            m_FootprintFilterListBox->SetSelection( wxNOT_FOUND );
        else
            m_FootprintFilterListBox->SetSelection( std::max( 0, ii - 1 ) );
    }

    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnEditFootprintFilter( wxCommandEvent& event )
{
    int idx = m_FootprintFilterListBox->GetSelection();

    if( idx >= 0 )
    {
        wxString filter = m_FootprintFilterListBox->GetStringSelection();

        WX_TEXT_ENTRY_DIALOG dlg( this, _( "Filter:" ), _( "Edit Footprint Filter" ), filter );

        if( dlg.ShowModal() == wxID_OK && !dlg.GetValue().IsEmpty() )
        {
            m_FootprintFilterListBox->SetString( (unsigned) idx, dlg.GetValue() );
            OnModify();
        }
    }
}


void DIALOG_LIB_SYMBOL_PROPERTIES::adjustGridColumns()
{
    // Account for scroll bars
    int width = KIPLATFORM::UI::GetUnobscuredSize( m_grid ).x;

    m_grid->AutoSizeColumn( FDC_NAME );
    m_grid->SetColSize( FDC_NAME, std::max( 72, m_grid->GetColSize( FDC_NAME ) ) );

    int fixedColsWidth = m_grid->GetColSize( FDC_NAME );

    for( int i = 2; i < m_grid->GetNumberCols(); i++ )
        fixedColsWidth += m_grid->GetColSize( i );

    m_grid->SetColSize( FDC_VALUE, std::max( 120, width - fixedColsWidth ) );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    m_OptionPartsInterchangeable->Enable( m_SelNumberOfUnits->GetValue() > 1 );
    m_pinNameOffset.Enable( m_PinsNameInsideButt->GetValue() );

    if( m_grid->IsCellEditControlShown() )
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( row == VALUE_FIELD && col == FDC_VALUE && m_OptionPower->IsChecked() )
        {
            wxGridCellEditor* editor = m_grid->GetCellEditor( row, col );
            m_SymbolNameCtrl->ChangeValue( editor->GetValue() );
            editor->DecRef();
        }
    }

    // Handle shown columns changes
    std::bitset<64> shownColumns = m_grid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_grid->IsCellEditControlShown() )
            adjustGridColumns();
    }

    // Handle a delayed focus.  The delay allows us to:
    // a) change focus when the error was triggered from within a killFocus handler
    // b) show the correct notebook page in the background before the error dialog comes up
    //    when triggered from an OK or a notebook page change

    if( m_delayedFocusPage >= 0 && m_NoteBook->GetSelection() != m_delayedFocusPage )
    {
        m_NoteBook->ChangeSelection( (unsigned) m_delayedFocusPage );
        m_delayedFocusPage = -1;
    }

    if( !m_delayedErrorMessage.IsEmpty() )
    {
        // We will re-enter this routine when the error dialog is displayed, so make
        // sure we don't keep putting up more dialogs.
        wxString msg = m_delayedErrorMessage;
        m_delayedErrorMessage = wxEmptyString;

        // Do not use DisplayErrorMessage(); it screws up window order on Mac
        DisplayError( nullptr, msg );
    }

    if( m_delayedFocusCtrl )
    {
        m_delayedFocusCtrl->SetFocus();

        if( auto textEntry = dynamic_cast<wxTextEntry*>( m_delayedFocusCtrl ) )
            textEntry->SelectAll();

        m_delayedFocusCtrl = nullptr;
    }
    else if( m_delayedFocusGrid )
    {
        m_delayedFocusGrid->SetFocus();
        m_delayedFocusGrid->MakeCellVisible( m_delayedFocusRow, m_delayedFocusColumn );
        m_delayedFocusGrid->SetGridCursor( m_delayedFocusRow, m_delayedFocusColumn );

        m_delayedFocusGrid->EnableCellEditControl( true );
        m_delayedFocusGrid->ShowCellEditControl();

        m_delayedFocusGrid = nullptr;
        m_delayedFocusRow = -1;
        m_delayedFocusColumn = -1;
    }

    wxString simEnable;

    for( int ii = MANDATORY_FIELDS; ii < m_fields->GetNumberRows(); ++ii )
    {
        if( m_fields->GetValue( ii, FDC_NAME ) == SIM_ENABLE_FIELD )
        {
            simEnable = m_fields->GetValue( ii, FDC_VALUE );
            break;
        }
    }

    m_excludeFromSim->SetValue( simEnable == wxS( "0" ) );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnSizeGrid( wxSizeEvent& event )
{
    auto new_size = event.GetSize();

    if( new_size != m_size )
    {
        m_size = new_size;

        adjustGridColumns();
    }

    // Always propagate a wxSizeEvent:
    event.Skip();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::syncControlStates( bool aIsAlias )
{
    bSizerLowerBasicPanel->Show( !aIsAlias );
    m_inheritanceSelectCombo->Enable( aIsAlias );
    m_inheritsStaticText->Enable( aIsAlias );
    m_grid->ForceRefresh();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::onPowerCheckBox( wxCommandEvent& aEvent )
{
    if( m_OptionPower->IsChecked() )
    {
        m_excludeFromBomCheckBox->SetValue( true );
        m_excludeFromBoardCheckBox->SetValue( true );
        m_excludeFromBomCheckBox->Enable( false );
        m_excludeFromBoardCheckBox->Enable( false );
        m_spiceFieldsButton->Show( false );
    }
    else
    {
        m_excludeFromBomCheckBox->Enable( true );
        m_excludeFromBoardCheckBox->Enable( true );
        m_spiceFieldsButton->Show( true );
    }

    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnText( wxCommandEvent& event )
{
    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnCombobox( wxCommandEvent& event )
{
    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnCheckBox( wxCommandEvent& event )
{
    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnSpinCtrl( wxSpinEvent& event )
{
    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnSpinCtrlText( wxCommandEvent& event )
{
    OnModify();
}


