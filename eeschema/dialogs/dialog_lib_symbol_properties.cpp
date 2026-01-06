/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_lib_symbol_properties.h"

#include <pgm_base.h>
#include <eeschema_settings.h>
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
#include <project_sch.h>
#include <refdes_utils.h>
#include <dialog_sim_model.h>
#include <vector>

#include <panel_embedded_files.h>
#include <settings/settings_manager.h>
#include <symbol_editor_settings.h>
#include <widgets/listbox_tricks.h>

#include <wx/clipbrd.h>
#include <wx/msgdlg.h>


int DIALOG_LIB_SYMBOL_PROPERTIES::m_lastOpenedPage = 0;
DIALOG_LIB_SYMBOL_PROPERTIES::LAST_LAYOUT DIALOG_LIB_SYMBOL_PROPERTIES::m_lastLayout =
        DIALOG_LIB_SYMBOL_PROPERTIES::LAST_LAYOUT::NONE;


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
        m_delayedFocusPage( -1 ),
        m_fpFilterTricks( std::make_unique<LISTBOX_TRICKS>( *this, *m_FootprintFilterListBox ) )
{
    std::vector<const EMBEDDED_FILES*> inheritedEmbeddedFiles;

    if( std::shared_ptr<LIB_SYMBOL> parent = m_libEntry->GetParent().lock() )
    {
        while( parent )
        {
            inheritedEmbeddedFiles.push_back( parent->GetEmbeddedFiles() );
            parent = parent->GetParent().lock();
        }
    }

    m_embeddedFiles = new PANEL_EMBEDDED_FILES( m_NoteBook, m_libEntry, 0,
                                                std::move( inheritedEmbeddedFiles ) );
    m_NoteBook->AddPage( m_embeddedFiles, _( "Embedded Files" ) );

    m_fields = new FIELDS_GRID_TABLE( this, aParent, m_grid, m_libEntry,
                                      { m_embeddedFiles->GetLocalFiles() } );
    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this, { m_embeddedFiles->GetLocalFiles() },
                                                      [&]( wxCommandEvent& aEvent )
                                                      {
                                                          OnAddField( aEvent );
                                                      } ) );
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );

    // Load the FIELDS_GRID_TABLE -- ensure we are calling the overloaded push_back method
    std::vector<SCH_FIELD> fields;
    m_libEntry->CopyFields( fields );

    for( const SCH_FIELD& f : fields )
        m_fields->push_back( f );

    if( std::shared_ptr<LIB_SYMBOL> parent = m_libEntry->GetParent().lock() )
        addInheritedFields( parent );

    m_grid->ShowHideColumns( "0 1 2 3 4 5 6 7" );

    m_SymbolNameCtrl->SetValidator( FIELD_VALIDATOR( FIELD_T::VALUE ) );

    m_unitNamesGrid->PushEventHandler( new GRID_TRICKS( m_unitNamesGrid ) );
    m_unitNamesGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_bodyStyleNamesGrid->PushEventHandler( new GRID_TRICKS( m_bodyStyleNamesGrid,
                                                             [this]( wxCommandEvent& aEvent )
                                                             {
                                                                 OnAddBodyStyle( aEvent );
                                                             } ) );
    m_bodyStyleNamesGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_jumperGroupsGrid->SetupColumnAutosizer( 0 );
    m_jumperGroupsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    m_jumperGroupsGrid->PushEventHandler( new GRID_TRICKS( m_jumperGroupsGrid,
                                                           [this]( wxCommandEvent& aEvent )
                                                           {
                                                               OnAddJumperGroup( aEvent );
                                                           } ) );

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_bpMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_bpDelete->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_bpAddBodyStyle->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpMoveUpBodyStyle->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_bpMoveDownBodyStyle->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_bpDeleteBodyStyle->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_addFilterButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_editFilterButton->SetBitmap( KiBitmapBundle( BITMAPS::small_edit ) );
    m_deleteFilterButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_bpAddJumperGroup->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpRemoveJumperGroup->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    SetupStandardButtons();

    if( aParent->IsSymbolFromLegacyLibrary() && !aParent->IsSymbolFromSchematic() )
    {
        m_stdSizerButtonCancel->SetDefault();
        m_stdSizerButtonOK->SetLabel( _( "Read Only" ) );
        m_stdSizerButtonOK->Enable( false );
    }

    // wxFormBuilder doesn't include this event...
    m_grid->Bind( wxEVT_GRID_CELL_CHANGING, &DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanging, this );
    m_grid->Bind( wxEVT_GRID_CELL_CHANGED, &DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanged, this );
    m_grid->GetGridWindow()->Bind( wxEVT_MOTION, &DIALOG_LIB_SYMBOL_PROPERTIES::OnGridMotion, this );


    // Forward the delete button to the tricks
    m_deleteFilterButton->Bind( wxEVT_BUTTON,
                                [&]( wxCommandEvent& aEvent )
                                {
                                    wxCommandEvent cmdEvent( EDA_EVT_LISTBOX_DELETE );
                                    m_fpFilterTricks->ProcessEvent( cmdEvent );
                                } );

    // When the filter tricks modifies something, update ourselves
    m_FootprintFilterListBox->Bind( EDA_EVT_LISTBOX_CHANGED,
                                    [&]( wxCommandEvent& aEvent )
                                    {
                                        OnModify();
                                    } );

    if( m_lastLayout != DIALOG_LIB_SYMBOL_PROPERTIES::LAST_LAYOUT::NONE )
    {
        if( ( m_lastLayout == DIALOG_LIB_SYMBOL_PROPERTIES::LAST_LAYOUT::ALIAS && aLibEntry->IsRoot() )
            || ( m_lastLayout == DIALOG_LIB_SYMBOL_PROPERTIES::LAST_LAYOUT::PARENT && aLibEntry->IsDerived() ) )
        {
            resetSize();
        }
    }

    m_lastLayout = ( aLibEntry->IsDerived() ) ? DIALOG_LIB_SYMBOL_PROPERTIES::LAST_LAYOUT::ALIAS
                                              : DIALOG_LIB_SYMBOL_PROPERTIES::LAST_LAYOUT::PARENT;

    m_grid->GetParent()->Layout();
    syncControlStates( m_libEntry->IsDerived() );
    Layout();

    finishDialogSettings();
}


DIALOG_LIB_SYMBOL_PROPERTIES::~DIALOG_LIB_SYMBOL_PROPERTIES()
{
    m_lastOpenedPage = m_NoteBook->GetSelection( );

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Unbind( wxEVT_GRID_CELL_CHANGING, &DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanging, this );
    m_grid->Unbind( wxEVT_GRID_CELL_CHANGED, &DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanged, this );
    m_grid->GetGridWindow()->Unbind( wxEVT_MOTION, &DIALOG_LIB_SYMBOL_PROPERTIES::OnGridMotion, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
    m_unitNamesGrid->PopEventHandler( true );
    m_bodyStyleNamesGrid->PopEventHandler( true );
    m_jumperGroupsGrid->PopEventHandler( true );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::addInheritedFields( const std::shared_ptr<LIB_SYMBOL>& aParent )
{
    if( std::shared_ptr<LIB_SYMBOL> ancestor = aParent->GetParent().lock() )
        addInheritedFields( ancestor );

    std::vector<SCH_FIELD*> parentFields;
    aParent->GetFields( parentFields );

    for( SCH_FIELD* parentField : parentFields )
    {
        bool found = false;

        if( parentField->IsMandatory() )
            continue; // Don't inherit mandatory fields

        for( size_t ii = 0; ii < m_fields->size(); ++ii )
        {
            SCH_FIELD& field = m_fields->at( ii );

            if( field.IsMandatory() )
                continue; // Don't inherit mandatory fields

            if( field.GetCanonicalName() == parentField->GetCanonicalName() )
            {
                m_fields->SetFieldInherited( ii, *parentField );
                found = true;
                break;
            }
        }

        if( !found )
            m_fields->AddInheritedField( *parentField );
    }
}


bool DIALOG_LIB_SYMBOL_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    std::set<wxString> defined;

    for( SCH_FIELD& field : *m_fields )
        defined.insert( field.GetName() );

    // Add in any template fieldnames not yet defined:
    // Read global fieldname templates
    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        TEMPLATES templateMgr;

        if( !cfg->m_Drawing.field_names.IsEmpty() )
            templateMgr.AddTemplateFieldNames( cfg->m_Drawing.field_names );

        for( const TEMPLATE_FIELDNAME& templateFieldname : templateMgr.GetTemplateFieldNames() )
        {
            if( defined.count( templateFieldname.m_Name ) <= 0 )
            {
                SCH_FIELD field( m_libEntry, FIELD_T::USER, templateFieldname.m_Name );
                field.SetVisible( templateFieldname.m_Visible );
                m_fields->push_back( field );
                m_addedTemplateFields.insert( templateFieldname.m_Name );
            }
        }
    }

    // The Y axis for components in library file is from bottom to top while the screen axis is top
    // to bottom.However it is nowhandled by the lib file parser/writer.

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->GetNumberRows() );
    m_grid->ProcessTableMessage( msg );

    m_SymbolNameCtrl->ChangeValue( UnescapeString( m_libEntry->GetName() ) );

    m_KeywordCtrl->ChangeValue( m_libEntry->GetKeyWords() );
    m_unitSpinCtrl->SetValue( m_libEntry->GetUnitCount() );
    m_OptionPartsInterchangeable->SetValue( !m_libEntry->UnitsLocked() || m_libEntry->GetUnitCount() == 1 );

    updateUnitCount();

    for( int unit = 0; unit < m_libEntry->GetUnitCount(); unit++ )
    {
        if( m_libEntry->GetUnitDisplayNames().contains( unit + 1 ) )
            m_unitNamesGrid->SetCellValue( unit, 1, m_libEntry->GetUnitDisplayNames().at( unit + 1 ) );
    }

    if( m_libEntry->HasDeMorganBodyStyles() )
    {
        m_radioDeMorgan->SetValue( true );
    }
    else if( m_libEntry->IsMultiBodyStyle() )
    {
        m_radioCustom->SetValue( true );

        for( const wxString& name : m_libEntry->GetBodyStyleNames() )
        {
            int row = m_bodyStyleNamesGrid->GetNumberRows();
            m_bodyStyleNamesGrid->AppendRows( 1 );
            m_bodyStyleNamesGrid->SetCellValue( row, 0, name );
        }
    }
    else
    {
        m_radioSingle->SetValue( true );
    }

    m_OptionPower->SetValue( m_libEntry->IsPower() );
    m_OptionLocalPower->SetValue( m_libEntry->IsLocalPower() );

    if( m_libEntry->IsPower() )
    {
        m_spiceFieldsButton->Hide();
        m_OptionLocalPower->Enable();
    }
    else
    {
        m_OptionLocalPower->Enable( false );
    }

    m_excludeFromSimCheckBox->SetValue( m_libEntry->GetExcludedFromSim() );
    m_excludeFromBomCheckBox->SetValue( m_libEntry->GetExcludedFromBOM() );
    m_excludeFromBoardCheckBox->SetValue( m_libEntry->GetExcludedFromBoard() );
    m_excludeFromPosFilesCheckBox->SetValue( m_libEntry->GetExcludedFromPosFiles() );

    m_ShowPinNumButt->SetValue( m_libEntry->GetShowPinNumbers() );
    m_ShowPinNameButt->SetValue( m_libEntry->GetShowPinNames() );
    m_PinsNameInsideButt->SetValue( m_libEntry->GetPinNameOffset() != 0 );
    m_pinNameOffset.ChangeValue( m_libEntry->GetPinNameOffset() );

    wxArrayString tmp = m_libEntry->GetFPFilters();
    m_FootprintFilterListBox->Append( tmp );

    m_cbDuplicatePinsAreJumpers->SetValue( m_libEntry->GetDuplicatePinNumbersAreJumpers() );

    std::set<wxString> availablePins;

    for( const SCH_PIN* pin : m_libEntry->GetPins() )
        availablePins.insert( pin->GetNumber() );

    for( const std::set<wxString>& group : m_libEntry->JumperPinGroups() )
    {
        wxString groupTxt;

        for( const wxString& pinNumber : group )
        {
            if( !groupTxt.IsEmpty() )
                groupTxt << ", ";

            groupTxt << pinNumber;
        }

        m_jumperGroupsGrid->AppendRows( 1 );
        m_jumperGroupsGrid->SetCellValue( m_jumperGroupsGrid->GetNumberRows() - 1, 0, groupTxt );
    }

    // Populate the list of root parts for inherited objects.
    if( m_libEntry->IsDerived() )
    {
        wxArrayString symbolNames;
        wxString libName = m_Parent->GetCurLib();

        // Someone forgot to set the current library in the editor frame window.
        wxCHECK( !libName.empty(), false );

        m_Parent->GetLibManager().GetSymbolNames( libName, symbolNames );

        // Sort the list of symbols for easier search
        symbolNames.Sort(
                []( const wxString& a, const wxString& b ) -> int
                {
                    return StrNumCmp( a, b, true );
                } );

        // Don't allow a symbol to be derived from itself
        if( symbolNames.Index( m_libEntry->GetName() ) != wxNOT_FOUND )
            symbolNames.Remove( m_libEntry->GetName() );

        // Don't allow a symbol to be derived from any of its descendants (would create
        // circular inheritance)
        wxArrayString descendants;
        m_Parent->GetLibManager().GetDerivedSymbolNames( m_libEntry->GetName(), libName,
                                                         descendants );

        for( const wxString& descendant : descendants )
        {
            if( symbolNames.Index( descendant ) != wxNOT_FOUND )
                symbolNames.Remove( descendant );
        }

        m_inheritanceSelectCombo->Append( symbolNames );

        if( std::shared_ptr<LIB_SYMBOL> rootSymbol = m_libEntry->GetParent().lock() )
        {
            wxString parentName = UnescapeString( rootSymbol->GetName() );
            int selection = m_inheritanceSelectCombo->FindString( parentName );

            if( selection == wxNOT_FOUND )
                return false;

            m_inheritanceSelectCombo->SetSelection( selection );
        }

        m_lastOpenedPage = 0;
    }

    m_NoteBook->SetSelection( (unsigned) m_lastOpenedPage );

    m_embeddedFiles->TransferDataToWindow();

    return true;
}


bool DIALOG_LIB_SYMBOL_PROPERTIES::Validate()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Symbol reference can be empty because it inherits from the parent symbol.
    if( m_libEntry->IsRoot() )
    {
        SCH_FIELD* field = m_fields->GetField( FIELD_T::REFERENCE );

        if( UTIL::GetRefDesPrefix( field->GetText() ).IsEmpty() )
        {
            if( m_NoteBook->GetSelection() != 0 )
                m_NoteBook->SetSelection( 0 );

            m_delayedErrorMessage = _( "References must start with a letter." );
            m_delayedFocusGrid = m_grid;
            m_delayedFocusColumn = FDC_VALUE;
            m_delayedFocusRow = m_fields->GetFieldRow( FIELD_T::REFERENCE );
            m_delayedFocusPage = 0;

            return false;
        }
    }

    // Check for missing field names.
    for( int ii = 0; ii < (int) m_fields->size(); ++ii )
    {
        SCH_FIELD& field = m_fields->at( ii );

        if( field.IsMandatory() )
            continue;

        wxString fieldName = field.GetName( false );

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
    if( m_libEntry->IsDerived() )
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

    if( m_unitSpinCtrl->GetValue() < m_libEntry->GetUnitCount() )
    {
        if( !IsOK( this, _( "Delete extra units from symbol?" ) ) )
            return false;
    }

    int bodyStyleCount = 0;

    if( m_radioSingle->GetValue() )
    {
        bodyStyleCount = 1;
    }
    if( m_radioDeMorgan->GetValue() )
    {
        bodyStyleCount = 2;
    }
    else if( m_radioCustom->GetValue() )
    {
        for( int ii = 0; ii < m_bodyStyleNamesGrid->GetNumberRows(); ++ii )
        {
            if( !m_bodyStyleNamesGrid->GetCellValue( ii, 0 ).IsEmpty() )
                bodyStyleCount++;
        }
    }

    if( bodyStyleCount == 0 )
    {
        m_delayedErrorMessage = _( "Symbol must have at least 1 body style" );
        return false;
    }

    if( bodyStyleCount < m_libEntry->GetBodyStyleCount() )
    {
        if( !IsOK( this, _( "Delete extra body styles from symbol?" ) ) )
            return false;
    }

    return true;
}


bool DIALOG_LIB_SYMBOL_PROPERTIES::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges()
            || !m_unitNamesGrid->CommitPendingChanges()
            || !m_bodyStyleNamesGrid->CommitPendingChanges()
            || !m_jumperGroupsGrid->CommitPendingChanges()
            || !m_embeddedFiles->TransferDataFromWindow() )
    {
        return false;
    }

    wxString   newName = EscapeString( m_SymbolNameCtrl->GetValue(), CTX_LIBID );
    wxString   oldName = m_libEntry->GetName();

    if( newName.IsEmpty() )
    {
        wxMessageBox( _( "Symbol must have a name." ) );
        return false;
    }

    UNDO_REDO opType = UNDO_REDO::LIBEDIT;

    if( oldName != newName )
    {
        wxString libName = m_Parent->GetCurLib();

        if( m_Parent->GetLibManager().SymbolNameInUse( newName, libName ) )
        {
            wxString msg;

            msg.Printf( _( "Symbol name '%s' already in use in library '%s'." ),
                        UnescapeString( newName ),
                        libName );
            DisplayErrorMessage( this, msg );
            return false;
        }

        opType = UNDO_REDO::LIB_RENAME;
    }

    m_Parent->SaveCopyInUndoList( _( "Edit Symbol Properties" ), m_libEntry, opType );

    // The Y axis for components in lib is from bottom to top while the screen axis is top
    // to bottom: we must change the y coord sign when writing back to the library
    std::vector<SCH_FIELD> fieldsToSave;
    int ordinal = 42;   // Arbitrarily larger than any mandatory FIELD_T ids.

    for( size_t ii = 0; ii < m_fields->size(); ++ii )
    {
        SCH_FIELD& field = m_fields->at( ii );

        if( !field.IsMandatory() )
            field.SetOrdinal( ordinal++ );

        wxString fieldName = field.GetCanonicalName();

        if( m_fields->IsInherited( ii ) && field == m_fields->ParentField( ii ) )
            continue; // Skip inherited fields

        if( field.GetText().IsEmpty() )
        {
            if( fieldName.IsEmpty() || m_addedTemplateFields.contains( fieldName ) )
                continue; // Skip empty fields that are not mandatory or template fields
        }
        else if( fieldName.IsEmpty() )
        {
            field.SetName( _( "untitled" ) ); // Set a default name for unnamed fields
        }

        fieldsToSave.push_back( field );
    }

    m_libEntry->SetFields( fieldsToSave );

    // Update the parent for inherited symbols
    if( m_libEntry->IsDerived() )
    {
        wxString parentName = EscapeString( m_inheritanceSelectCombo->GetValue(), CTX_LIBID );

        // The parentName was verified to be non-empty in the Validator
        wxString libName = m_Parent->GetCurLib();

        // Get the parent from the libManager based on the name set in the inheritance combo box.
        LIB_SYMBOL* newParent = m_Parent->GetLibManager().GetSymbol( parentName, libName );

        // Verify that the requested parent exists
        wxCHECK( newParent, false );

        m_libEntry->SetParent( newParent );
    }

    m_libEntry->SetName( newName );
    m_libEntry->SetKeyWords( m_KeywordCtrl->GetValue() );
    m_libEntry->SetUnitCount( m_unitSpinCtrl->GetValue(), true );
    m_libEntry->LockUnits( m_libEntry->GetUnitCount() > 1 && !m_OptionPartsInterchangeable->GetValue() );

    m_libEntry->GetUnitDisplayNames().clear();

    for( int row = 0; row < m_unitNamesGrid->GetNumberRows(); row++ )
    {
        if( !m_unitNamesGrid->GetCellValue( row, 1 ).IsEmpty() )
            m_libEntry->GetUnitDisplayNames()[row+1] = m_unitNamesGrid->GetCellValue( row, 1 );
    }

    if( m_radioSingle->GetValue() )
    {
        m_libEntry->SetHasDeMorganBodyStyles( false );
        m_libEntry->SetBodyStyleCount( 1, false, false );
        m_libEntry->SetBodyStyleNames( {} );
    }
    else if( m_radioDeMorgan->GetValue() )
    {
        m_libEntry->SetHasDeMorganBodyStyles( true );
        m_libEntry->SetBodyStyleCount( 2, false, true );
        m_libEntry->SetBodyStyleNames( {} );
    }
    else
    {
        std::vector<wxString> bodyStyleNames;

        for( int row = 0; row < m_bodyStyleNamesGrid->GetNumberRows(); ++row )
        {
            if( !m_bodyStyleNamesGrid->GetCellValue( row, 0 ).IsEmpty() )
                bodyStyleNames.push_back( m_bodyStyleNamesGrid->GetCellValue( row, 0 ) );
        }

        m_libEntry->SetHasDeMorganBodyStyles( false );
        m_libEntry->SetBodyStyleCount( bodyStyleNames.size(), true, true );
        m_libEntry->SetBodyStyleNames( bodyStyleNames );
    }

    if( m_OptionPower->GetValue() )
    {
        if( m_OptionLocalPower->GetValue() )
            m_libEntry->SetLocalPower();
        else
            m_libEntry->SetGlobalPower();

        // Power symbols must have value matching name for now
        m_libEntry->GetValueField().SetText( newName );
    }
    else
    {
        m_libEntry->SetNormal();
    }

    m_libEntry->SetExcludedFromSim( m_excludeFromSimCheckBox->GetValue() );
    m_libEntry->SetExcludedFromBOM( m_excludeFromBomCheckBox->GetValue() );
    m_libEntry->SetExcludedFromBoard( m_excludeFromBoardCheckBox->GetValue() );
    m_libEntry->SetExcludedFromPosFiles( m_excludeFromPosFilesCheckBox->GetValue() );

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

    m_libEntry->SetDuplicatePinNumbersAreJumpers( m_cbDuplicatePinsAreJumpers->GetValue() );

    std::vector<std::set<wxString>>& jumpers = m_libEntry->JumperPinGroups();
    jumpers.clear();

    for( int ii = 0; ii < m_jumperGroupsGrid->GetNumberRows(); ++ii )
    {
        wxStringTokenizer tokenizer( m_jumperGroupsGrid->GetCellValue( ii, 0 ), ", \t\r\n", wxTOKEN_STRTOK );
        std::set<wxString>& group = jumpers.emplace_back();

        while( tokenizer.HasMoreTokens() )
        {
            if( wxString token = tokenizer.GetNextToken(); !token.IsEmpty() )
                group.insert( token );
        }
    }

    m_Parent->UpdateAfterSymbolProperties( &oldName );

    return true;
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnBodyStyle( wxCommandEvent& event )
{
    m_bodyStyleNamesGrid->Enable( m_radioCustom->GetValue() );

    m_bpAddBodyStyle->Enable( m_radioCustom->GetValue() );
    m_bpMoveUpBodyStyle->Enable( m_radioCustom->GetValue() );
    m_bpMoveDownBodyStyle->Enable( m_radioCustom->GetValue() );
    m_bpDeleteBodyStyle->Enable( m_radioCustom->GetValue() );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnGridMotion( wxMouseEvent& aEvent )
{
    aEvent.Skip();

    wxPoint pos = aEvent.GetPosition();
    wxPoint unscolled_pos = m_grid->CalcUnscrolledPosition( pos );
    int row = m_grid->YToRow( unscolled_pos.y );
    int col = m_grid->XToCol( unscolled_pos.x );

    if( row == wxNOT_FOUND || col == wxNOT_FOUND || !m_fields->IsInherited( row ) )
    {
        m_grid->SetToolTip( "" );
        return;
    }

    m_grid->SetToolTip( wxString::Format( _( "This field is inherited from '%s'." ),
                                          m_fields->ParentField( row ).GetName() ) );
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
                DisplayError( this, wxString::Format( _( "The name '%s' is already in use." ), newName ) );
                event.Veto();
                m_delayedFocusRow = event.GetRow();
                m_delayedFocusColumn = event.GetCol();
            }
        }
    }

    editor->DecRef();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanged( wxGridEvent& event )
{
    m_grid->ForceRefresh();
    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnSymbolNameText( wxCommandEvent& event )
{
    if( m_OptionPower->IsChecked() )
        m_grid->SetCellValue( m_fields->GetFieldRow( FIELD_T::VALUE ), FDC_VALUE, m_SymbolNameCtrl->GetValue() );

    OnModify();
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
    m_grid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                SYMBOL_EDITOR_SETTINGS* settings = m_Parent->GetSettings();
                SCH_FIELD newField( m_libEntry, FIELD_T::USER, GetUserFieldName( m_fields->size(), DO_TRANSLATE ) );

                newField.SetTextSize( VECTOR2I( schIUScale.MilsToIU( settings->m_Defaults.text_size ),
                                                schIUScale.MilsToIU( settings->m_Defaults.text_size ) ) );
                newField.SetVisible( false );

                m_fields->push_back( newField );

                // notify the grid
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
                m_grid->ProcessTableMessage( msg );
                OnModify();

                return { m_fields->size() - 1, FDC_NAME };
            } );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    m_grid->OnDeleteRows(
            [&]( int row )
            {
                if( row < m_fields->GetMandatoryRowCount() )
                {
                    DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                                          m_fields->GetMandatoryRowCount() ) );
                    return false;
                }

                return true;
            },
            [&]( int row )
            {
                if( !m_fields->EraseRow( row ) )
                    return;

                // notify the grid
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, row, 1 );
                m_grid->ProcessTableMessage( msg );
            } );

    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnMoveUp( wxCommandEvent& event )
{
    m_grid->OnMoveRowUp(
            [&]( int row )
            {
                return row > m_fields->GetMandatoryRowCount();
            },
            [&]( int row )
            {
                m_fields->SwapRows( row, row - 1 );
                m_grid->ForceRefresh();
                OnModify();
            } );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnMoveDown( wxCommandEvent& event )
{
    m_grid->OnMoveRowDown(
            [&]( int row )
            {
                return row >= m_fields->GetMandatoryRowCount();
            },
            [&]( int row )
            {
                m_fields->SwapRows( row, row + 1 );
                m_grid->ForceRefresh();
                OnModify();
            } );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnAddBodyStyle( wxCommandEvent& event )
{
    m_bodyStyleNamesGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_bodyStyleNamesGrid->AppendRows( 1 );
                OnModify();

                return { m_bodyStyleNamesGrid->GetNumberRows() - 1, 0 };
            } );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnDeleteBodyStyle( wxCommandEvent& event )
{
    m_bodyStyleNamesGrid->OnDeleteRows(
            [&]( int row )
            {
                m_bodyStyleNamesGrid->DeleteRows( row );
            } );

    OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnBodyStyleMoveUp( wxCommandEvent& event )
{
    m_bodyStyleNamesGrid->OnMoveRowUp(
            [&]( int row )
            {
                m_bodyStyleNamesGrid->SwapRows( row, row - 1 );
                OnModify();
            } );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnBodyStyleMoveDown( wxCommandEvent& event )
{
    m_bodyStyleNamesGrid->OnMoveRowDown(
            [&]( int row )
            {
                m_bodyStyleNamesGrid->SwapRows( row, row + 1 );
                OnModify();
            } );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnEditSpiceModel( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    m_grid->ClearSelection();

    std::vector<SCH_FIELD> fields;

    for( const SCH_FIELD& field : *m_fields )
        fields.emplace_back( field );

    DIALOG_SIM_MODEL dialog( this, m_parentFrame, *m_libEntry, fields );

    if( dialog.ShowModal() != wxID_OK )
        return;

    // Add in any new fields
    for( const SCH_FIELD& editedField : fields )
    {
        bool found = false;

        for( SCH_FIELD& existingField : *m_fields )
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
    for( int ii = (int) m_fields->size() - 1; ii >= 0; --ii )
    {
        SCH_FIELD& existingField = m_fields->at( ii );
        bool       found = false;

        for( SCH_FIELD& editedField : fields )
        {
            if( editedField.GetName() == existingField.GetName() )
            {
                found = true;
                break;
            }
        }

        if( !found )
        {
            m_grid->ClearSelection();
            m_fields->erase( m_fields->begin() + ii );

            wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, ii, 1 );
            m_grid->ProcessTableMessage( msg );
        }
    }

    OnModify();
    m_grid->ForceRefresh();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnFpFilterDClick( wxMouseEvent& event )
{
    int            idx = m_FootprintFilterListBox->HitTest( event.GetPosition() );
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


void DIALOG_LIB_SYMBOL_PROPERTIES::OnEditFootprintFilter( wxCommandEvent& event )
{
    wxArrayInt selections;
    int n = m_FootprintFilterListBox->GetSelections( selections );

    if( n > 0 )
    {
        // Just edit the first one
        int idx = selections[0];
        wxString filter = m_FootprintFilterListBox->GetString( idx );

        WX_TEXT_ENTRY_DIALOG dlg( this, _( "Filter:" ), _( "Edit Footprint Filter" ), filter );

        if( dlg.ShowModal() == wxID_OK && !dlg.GetValue().IsEmpty() )
        {
            m_FootprintFilterListBox->SetString( (unsigned) idx, dlg.GetValue() );
            OnModify();
        }
    }
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    m_OptionPartsInterchangeable->Enable( m_unitSpinCtrl->GetValue() > 1 );
    m_pinNameOffset.Enable( m_PinsNameInsideButt->GetValue() );

    if( m_grid->IsCellEditControlShown() )
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( row == m_fields->GetFieldRow( FIELD_T::VALUE ) && col == FDC_VALUE && m_OptionPower->IsChecked() )
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
            m_grid->SetGridWidthsDirty();
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

        if( wxTextEntry* textEntry = dynamic_cast<wxTextEntry*>( m_delayedFocusCtrl ) )
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
        m_excludeFromSimCheckBox->SetValue( true );
        m_excludeFromBomCheckBox->SetValue( true );
        m_excludeFromBoardCheckBox->SetValue( true );
        m_excludeFromPosFilesCheckBox->SetValue( true );
        m_excludeFromBomCheckBox->Enable( false );
        m_excludeFromBoardCheckBox->Enable( false );
        m_excludeFromSimCheckBox->Enable( false );
        m_excludeFromPosFilesCheckBox->Enable( false );
        m_spiceFieldsButton->Show( false );
        m_OptionLocalPower->Enable( true );
    }
    else
    {
        m_excludeFromBomCheckBox->Enable( true );
        m_excludeFromBoardCheckBox->Enable( true );
        m_excludeFromSimCheckBox->Enable( true );
        m_excludeFromPosFilesCheckBox->Enable( true );
        m_spiceFieldsButton->Show( true );
        m_OptionLocalPower->Enable( false );
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


bool DIALOG_LIB_SYMBOL_PROPERTIES::updateUnitCount()
{
    m_unitNamesGrid->CommitPendingChanges( true /* aQuietMode */ );

    int extra = m_unitNamesGrid->GetNumberRows() - m_unitSpinCtrl->GetValue();
    int needed = m_unitSpinCtrl->GetValue() - m_unitNamesGrid->GetNumberRows();

    if( extra > 0 )
    {
        m_unitNamesGrid->DeleteRows( m_unitNamesGrid->GetNumberRows() - extra, extra );
        return true;
    }

    if( needed > 0 )
    {
        m_unitNamesGrid->AppendRows( needed );

        for( int row = m_unitNamesGrid->GetNumberRows() - needed; row < m_unitNamesGrid->GetNumberRows(); ++row )
            m_unitNamesGrid->SetCellValue( row, 0, LIB_SYMBOL::LetterSubReference( row + 1, 'A' ) );

        return true;
    }

    return false;
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnUnitSpinCtrl( wxSpinEvent& event )
{
    if( updateUnitCount() )
        OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnUnitSpinCtrlText( wxCommandEvent& event )
{
    // wait for kill focus to update unit count
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnUnitSpinCtrlEnter( wxCommandEvent& event )
{
    if( updateUnitCount() )
        OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnUnitSpinCtrlKillFocus( wxFocusEvent& event )
{
    if( updateUnitCount() )
        OnModify();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnPageChanging( wxBookCtrlEvent& aEvent )
{
    if( !m_grid->CommitPendingChanges() )
        aEvent.Veto();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnAddJumperGroup( wxCommandEvent& event )
{
    m_jumperGroupsGrid->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_jumperGroupsGrid->AppendRows( 1 );
                OnModify();

                return { m_jumperGroupsGrid->GetNumberRows() - 1, 0 };
            } );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnRemoveJumperGroup( wxCommandEvent& event )
{
    m_jumperGroupsGrid->OnDeleteRows(
            [&]( int row )
            {
                m_jumperGroupsGrid->DeleteRows( row, 1 );
            } );

    OnModify();
}


