/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <symbol_library_manager.h>
#include <math/util.h> // for KiROUND
#include <pgm_base.h>
#include <sch_component.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/wx_grid.h>

#ifdef KICAD_SPICE
#include <dialog_spice_model.h>
#endif /* KICAD_SPICE */

#include <dialog_lib_symbol_properties.h>
#include <settings/settings_manager.h>
#include <symbol_editor_settings.h>


int DIALOG_LIB_SYMBOL_PROPERTIES::m_lastOpenedPage = 0;
DIALOG_LIB_SYMBOL_PROPERTIES::LAST_LAYOUT
    DIALOG_LIB_SYMBOL_PROPERTIES::m_lastLayout = DIALOG_LIB_SYMBOL_PROPERTIES::NONE;


DIALOG_LIB_SYMBOL_PROPERTIES::DIALOG_LIB_SYMBOL_PROPERTIES( SYMBOL_EDIT_FRAME* aParent,
                                                            LIB_PART* aLibEntry ) :
    DIALOG_LIB_SYMBOL_PROPERTIES_BASE( aParent ),
    m_Parent( aParent ),
    m_libEntry( aLibEntry ),
    m_pinNameOffset( aParent, m_nameOffsetLabel, m_nameOffsetCtrl, m_nameOffsetUnits, true ),
    m_delayedFocusCtrl( nullptr ),
    m_delayedFocusGrid( nullptr ),
    m_delayedFocusRow( -1 ),
    m_delayedFocusColumn( -1 ),
    m_delayedFocusPage( -1 ),
    m_width( 0 )
{
    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );
    m_fields = new FIELDS_GRID_TABLE<LIB_FIELD>( this, aParent, m_grid, m_libEntry );
    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this ) );

    // Show/hide columns according to the user's preference
    auto cfg = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
    m_grid->ShowHideColumns( cfg->m_EditComponentVisibleColumns );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_URL_EDITOR( this ) );
    m_grid->SetAttr( DATASHEET_FIELD, FDC_VALUE, attr );

    m_SymbolNameCtrl->SetValidator( SCH_FIELD_VALIDATOR( true, VALUE_FIELD ) );

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_bpDelete->SetBitmap( KiBitmap( small_trash_xpm ) );
    m_bpMoveUp->SetBitmap( KiBitmap( small_up_xpm ) );
    m_bpMoveDown->SetBitmap( KiBitmap( small_down_xpm ) );
    m_addFilterButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_deleteFilterButton->SetBitmap( KiBitmap( small_trash_xpm ) );
    m_editFilterButton->SetBitmap( KiBitmap( small_edit_xpm ) );

    if( aParent->IsSymbolFromLegacyLibrary() )
    {
        m_stdSizerButtonCancel->SetDefault();
        m_stdSizerButtonOK->SetLabel( _( "Read Only" ) );
        m_stdSizerButtonOK->Enable( false );
    }
    else
    {
        m_stdSizerButtonOK->SetDefault();
    }

#ifndef KICAD_SPICE
    m_spiceFieldsButton->Hide();
#endif

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING,
                     wxGridEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanging ),
                     NULL, this );

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

    auto cfg = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
    cfg->m_EditComponentVisibleColumns = m_grid->GetShownColumns();

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                        wxGridEventHandler( DIALOG_LIB_SYMBOL_PROPERTIES::OnGridCellChanging ),
                        NULL, this );

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
        wxPoint pos = m_fields->at( i ).GetPosition();
        pos.y = -pos.y;
        m_fields->at( i ).SetPosition( pos );
    }

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->GetNumberRows() );
    m_grid->ProcessTableMessage( msg );
    adjustGridColumns( m_grid->GetRect().GetWidth() );

    m_SymbolNameCtrl->SetValue( m_libEntry->GetName() );

    m_DescCtrl->SetValue( m_libEntry->GetDescription() );
    m_KeywordCtrl->SetValue( m_libEntry->GetKeyWords() );
    m_SelNumberOfUnits->SetValue( m_libEntry->GetUnitCount() );
    m_OptionPartsInterchangeable->SetValue( !m_libEntry->UnitsLocked() || m_libEntry->GetUnitCount() == 1 );
    m_AsConvertButt->SetValue( m_libEntry->HasConversion() );
    m_OptionPower->SetValue( m_libEntry->IsPower() );
    m_excludeFromBomCheckBox->SetValue( !m_libEntry->GetIncludeInBom() );
    m_excludeFromBoardCheckBox->SetValue( !m_libEntry->GetIncludeOnBoard() );

    m_ShowPinNumButt->SetValue( m_libEntry->ShowPinNumbers() );
    m_ShowPinNameButt->SetValue( m_libEntry->ShowPinNames() );
    m_PinsNameInsideButt->SetValue( m_libEntry->GetPinNameOffset() != 0 );
    m_pinNameOffset.SetValue( m_libEntry->GetPinNameOffset() );

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

        PART_SPTR rootPart = m_libEntry->GetParent().lock();

        wxCHECK( rootPart, false );

        wxString parentName = rootPart->GetName();
        int selection = m_inheritanceSelectCombo->FindString( parentName );

        wxCHECK( selection != wxNOT_FOUND, false );
        m_inheritanceSelectCombo->SetSelection( selection );

        m_lastOpenedPage = 0;
    }

    m_NoteBook->SetSelection( (unsigned) m_lastOpenedPage );

    return true;
}


bool DIALOG_LIB_SYMBOL_PROPERTIES::Validate()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Alias symbol reference can be empty because it inherits from the parent symbol.
    if( m_libEntry->IsRoot() &&
        !SCH_COMPONENT::IsReferenceStringValid( m_fields->at( REFERENCE_FIELD ).GetText() ) )
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
    for( size_t i = MANDATORY_FIELDS;  i < m_fields->size(); ++i )
    {
        LIB_FIELD& field = m_fields->at( i );
        wxString   fieldName = field.GetName( false );

        if( fieldName.IsEmpty() )
        {
            if( m_NoteBook->GetSelection() != 0 )
                m_NoteBook->SetSelection( 0 );

            m_delayedErrorMessage = _( "Fields must have a name." );
            m_delayedFocusGrid = m_grid;
            m_delayedFocusColumn = FDC_NAME;
            m_delayedFocusRow = i;
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
            m_delayedErrorMessage = _( "Aliased symbol must have a parent selected" );

            return false;
        }
    }

    if( m_SelNumberOfUnits->GetValue() < m_libEntry->GetUnitCount() )
    {
        if( !IsOK( this, _( "Delete extra units from symbol?" ) ) )
            return false;
    }

    if( m_AsConvertButt->GetValue() && !m_libEntry->HasConversion() )
    {
        if( !IsOK( this, _( "Add new pins for alternate body style (DeMorgan) to symbol?" ) ) )
            return false;
    }
    else if( !m_AsConvertButt->GetValue() && m_libEntry->HasConversion() )
    {
        if( !IsOK( this, _( "Delete alternate body style (DeMorgan) draw items from symbol?" ) ) )
            return false;
    }

    return true;
}


bool DIALOG_LIB_SYMBOL_PROPERTIES::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // We need to keep the name and the value the same at the moment!
    wxString   newName = m_fields->at( VALUE_FIELD ).GetText();
    wxString   oldName = m_libEntry->GetName();

    if( oldName != newName )
    {
        wxString libName = m_Parent->GetCurLib();

        if( m_Parent->GetLibManager().PartExists( newName, libName ) )
        {
            wxString msg;

            msg.Printf( _( "The name '%s' conflicts with an existing entry in the library '%s'." ),
                        newName, libName );
            DisplayErrorMessage( this, msg );
            return false;
        }

        m_Parent->SaveCopyInUndoList( m_libEntry, UNDO_REDO::LIB_RENAME );
    }
    else
    {
        m_Parent->SaveCopyInUndoList( m_libEntry );
    }

    // The Y axis for components in lib is from bottom to top while the screen axis is top
    // to bottom: we must change the y coord sign when writing back to the library
    for( size_t i = 0; i < m_fields->size(); ++i )
    {
        wxPoint pos = m_fields->at( i ).GetPosition();
        pos.y = -pos.y;
        m_fields->at( i ).SetPosition( pos );
    }

    m_libEntry->SetFields( *m_fields );

    // Update the parent for inherited symbols
    if( m_libEntry->IsAlias() )
    {
        wxString parentName = m_inheritanceSelectCombo->GetValue();

        // The parentName was verified to be non-empty in the Validator
        wxString libName = m_Parent->GetCurLib();

        // Get the parent from the libManager based on the name set in the inheritance combo box.
        LIB_PART* newParent = m_Parent->GetLibManager().GetAlias( parentName, libName );

        // Verify that the requested parent exists
        wxCHECK( newParent, false );

        // Verify that the new parent is not an alias.
        wxCHECK( !newParent->IsAlias(), false );

        m_libEntry->SetParent( newParent );
    }

    // We need to keep the name and the value the same at the moment!
    m_libEntry->SetName( newName );
    m_libEntry->SetDescription( m_DescCtrl->GetValue() );
    m_libEntry->SetKeyWords( m_KeywordCtrl->GetValue() );
    m_libEntry->SetUnitCount( m_SelNumberOfUnits->GetValue() );
    m_libEntry->LockUnits( m_libEntry->GetUnitCount() > 1 && !m_OptionPartsInterchangeable->GetValue() );
    m_libEntry->SetConversion( m_AsConvertButt->GetValue() );

    if( m_OptionPower->GetValue() )
        m_libEntry->SetPower();
    else
        m_libEntry->SetNormal();

    m_libEntry->SetIncludeInBom( !m_excludeFromBomCheckBox->GetValue() );
    m_libEntry->SetIncludeOnBoard( !m_excludeFromBoardCheckBox->GetValue() );

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
    // alternate body style objects causing #LIB_PART::HasCoversion() to always return false.
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
    else if( event.GetRow() == VALUE_FIELD && event.GetCol() == FDC_VALUE )
        m_SymbolNameCtrl->ChangeValue( event.GetString() );

    editor->DecRef();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnSymbolNameText( wxCommandEvent& event )
{
    m_grid->SetCellValue( VALUE_FIELD, FDC_VALUE, m_SymbolNameCtrl->GetValue() );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnSymbolNameKillFocus( wxFocusEvent& event )
{
    if( !m_delayedFocusCtrl && !m_SymbolNameCtrl->GetValidator()->Validate( m_SymbolNameCtrl ) )
    {
        m_delayedFocusCtrl = m_SymbolNameCtrl;
        m_delayedFocusPage = 0;
    }

    event.Skip();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnAddField( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    auto*     settings = Pgm().GetSettingsManager().GetAppSettings<SYMBOL_EDITOR_SETTINGS>();
    int       fieldID = m_fields->size();
    LIB_FIELD newField( m_libEntry, fieldID );

    newField.SetTextSize( wxSize( Mils2iu( settings->m_Defaults.text_size ),
                                  Mils2iu( settings->m_Defaults.text_size ) ) );

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_grid->MakeCellVisible( (int) m_fields->size() - 1, 0 );
    m_grid->SetGridCursor( (int) m_fields->size() - 1, 0 );

    m_grid->EnableCellEditControl();
    m_grid->ShowCellEditControl();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnDeleteField( wxCommandEvent& event )
{
    int curRow = m_grid->GetGridCursorRow();

    if( curRow < 0 )
    {
        return;
    }
    else if( curRow < MANDATORY_FIELDS )
    {
        DisplayError( this, wxString::Format( _( "The first %d fields are mandatory." ),
                                              MANDATORY_FIELDS ) );
        return;
    }

    m_grid->CommitPendingChanges( true /* quiet mode */ );

    m_fields->erase( m_fields->begin() + curRow );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, curRow, 1 );
    m_grid->ProcessTableMessage( msg );

    if( m_grid->GetNumberRows() > 0 )
    {
        m_grid->MakeCellVisible( std::max( 0, curRow-1 ), m_grid->GetGridCursorCol() );
        m_grid->SetGridCursor( std::max( 0, curRow-1 ), m_grid->GetGridCursorCol() );
    }
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
    }
    else
        wxBell();
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
    }
    else
        wxBell();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnEditSpiceModel( wxCommandEvent& event )
{
#ifdef KICAD_SPICE
    int diff = m_fields->size();
    auto cmp = SCH_COMPONENT( *m_libEntry, m_libEntry->GetLibId(), nullptr );

    DIALOG_SPICE_MODEL dialog( this, cmp, m_fields );

    if( dialog.ShowModal() != wxID_OK )
        return;

    diff = (int) m_fields->size() - diff;

    if( diff > 0 )
    {
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, diff );
        m_grid->ProcessTableMessage( msg );
    }
    else if( diff < 0 )
    {
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, -diff );
        m_grid->ProcessTableMessage( msg );
    }

    m_grid->ForceRefresh();
#endif /* KICAD_SPICE */
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnFilterDClick( wxMouseEvent& event)
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
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnEditFootprintFilter( wxCommandEvent& event )
{
    int idx = m_FootprintFilterListBox->GetSelection();

    if( idx >= 0 )
    {
        wxString filter = m_FootprintFilterListBox->GetStringSelection();

        WX_TEXT_ENTRY_DIALOG dlg( this, _( "Filter:" ), _( "Edit Footprint Filter" ), filter );

        if( dlg.ShowModal() == wxID_OK && !dlg.GetValue().IsEmpty() )
            m_FootprintFilterListBox->SetString( (unsigned) idx, dlg.GetValue() );
    }
}


void DIALOG_LIB_SYMBOL_PROPERTIES::adjustGridColumns( int aWidth )
{
    m_width = aWidth;

    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

    m_grid->AutoSizeColumn( FDC_NAME );

    int fixedColsWidth = m_grid->GetColSize( FDC_NAME );

    for( int i = 2; i < m_grid->GetNumberCols(); i++ )
        fixedColsWidth += m_grid->GetColSize( i );

    m_grid->SetColSize( FDC_VALUE, aWidth - fixedColsWidth );
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    m_OptionPartsInterchangeable->Enable( m_SelNumberOfUnits->GetValue() > 1 );
    m_pinNameOffset.Enable( m_PinsNameInsideButt->GetValue() );

    if( m_grid->IsCellEditControlShown() )
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( row == VALUE_FIELD && col == FDC_VALUE )
        {
            wxGridCellEditor* editor = m_grid->GetCellEditor( row, col );
            m_SymbolNameCtrl->ChangeValue( editor->GetValue() );
            editor->DecRef();
        }
    }

    // Handle shown columns changes
    wxString shownColumns = m_grid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_grid->IsCellEditControlShown() )
            adjustGridColumns( m_grid->GetRect().GetWidth() );
    }

    // Handle a delayed focus.  The delay allows us to:
    // a) change focus when the error was triggered from within a killFocus handler
    // b) show the correct notebook page in the background before the error dialog comes up
    //    when triggered from an OK or a notebook page change

    if( m_delayedFocusPage >= 0 && m_NoteBook->GetSelection() != m_delayedFocusPage )
    {
        m_NoteBook->SetSelection( (unsigned) m_delayedFocusPage );
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
}


void DIALOG_LIB_SYMBOL_PROPERTIES::OnSizeGrid( wxSizeEvent& event )
{
    auto new_size = event.GetSize().GetX();

    if( new_size != m_width )
    {
        adjustGridColumns( event.GetSize().GetX() );
    }

    // Always propagate a wxSizeEvent:
    event.Skip();
}


void DIALOG_LIB_SYMBOL_PROPERTIES::syncControlStates( bool aIsAlias )
{
    bSizerLowerBasicPanel->Show( !aIsAlias );

#ifdef KICAD_SPICE
    m_spiceFieldsButton->Show( !aIsAlias );
#endif

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
    }
    else
    {
        m_excludeFromBomCheckBox->Enable( true );
        m_excludeFromBoardCheckBox->Enable( true );
    }
}
