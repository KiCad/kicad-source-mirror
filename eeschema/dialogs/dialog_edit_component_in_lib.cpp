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
#include <class_libentry.h>
#include <confirm.h>
#include <dialog_text_entry.h>
#include <fctsys.h>
#include <kiway.h>
#include <lib_edit_frame.h>
#include <lib_manager.h>
#include <math/util.h> // for KiROUND
#include <pgm_base.h>
#include <sch_component.h>
#include <widgets/grid_text_button_helpers.h>
#include <widgets/wx_grid.h>

#ifdef KICAD_SPICE
#include <dialog_spice_model.h>
#endif /* KICAD_SPICE */

#include <dialog_edit_component_in_lib.h>
#include <settings/settings_manager.h>
#include <libedit_settings.h>


int DIALOG_EDIT_COMPONENT_IN_LIBRARY::m_lastOpenedPage = 0;
DIALOG_EDIT_COMPONENT_IN_LIBRARY::LAST_LAYOUT
    DIALOG_EDIT_COMPONENT_IN_LIBRARY::m_lastLayout = DIALOG_EDIT_COMPONENT_IN_LIBRARY::NONE;


DIALOG_EDIT_COMPONENT_IN_LIBRARY::DIALOG_EDIT_COMPONENT_IN_LIBRARY( LIB_EDIT_FRAME* aParent,
                                                                    LIB_PART* aLibEntry ) :
    DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE( aParent ),
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
    m_fields = new FIELDS_GRID_TABLE<LIB_FIELD>( this, aParent, m_libEntry );
    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this ) );

    // Show/hide columns according to the user's preference
    auto cfg = Pgm().GetSettingsManager().GetAppSettings<LIBEDIT_SETTINGS>();
    m_grid->ShowHideColumns( cfg->m_EditComponentVisibleColumns );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_URL_EDITOR( this ) );
    m_grid->SetAttr( DATASHEET, FDC_VALUE, attr );

    m_SymbolNameCtrl->SetValidator( SCH_FIELD_VALIDATOR( true, VALUE ) );

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_bpDelete->SetBitmap( KiBitmap( trash_xpm ) );
    m_bpMoveUp->SetBitmap( KiBitmap( small_up_xpm ) );
    m_bpMoveDown->SetBitmap( KiBitmap( small_down_xpm ) );
    m_addFilterButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_deleteFilterButton->SetBitmap( KiBitmap( trash_xpm ) );
    m_editFilterButton->SetBitmap( KiBitmap( small_edit_xpm ) );

    m_stdSizerButtonOK->SetDefault();

#ifndef KICAD_SPICE
    m_spiceFieldsButton->Hide();
#endif

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING,
                     wxGridEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnGridCellChanging ),
                     NULL, this );

    if( m_lastLayout != DIALOG_EDIT_COMPONENT_IN_LIBRARY::NONE )
    {
        if( ( m_lastLayout == DIALOG_EDIT_COMPONENT_IN_LIBRARY::ALIAS && aLibEntry->IsRoot() )
          || ( m_lastLayout == DIALOG_EDIT_COMPONENT_IN_LIBRARY::PARENT && aLibEntry->IsAlias() ) )
        {
            ResetSize();
        }
    }

    m_lastLayout = ( aLibEntry->IsAlias() ) ? DIALOG_EDIT_COMPONENT_IN_LIBRARY::ALIAS :
                   DIALOG_EDIT_COMPONENT_IN_LIBRARY::PARENT;

    m_grid->GetParent()->Layout();
    syncControlStates( m_libEntry->IsAlias() );
    Layout();

    FinishDialogSettings();
}


DIALOG_EDIT_COMPONENT_IN_LIBRARY::~DIALOG_EDIT_COMPONENT_IN_LIBRARY()
{
    m_lastOpenedPage = m_NoteBook->GetSelection( );

    auto cfg = Pgm().GetSettingsManager().GetAppSettings<LIBEDIT_SETTINGS>();
    cfg->m_EditComponentVisibleColumns = m_grid->GetShownColumns();

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                        wxGridEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnGridCellChanging ),
                        NULL, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::TransferDataToWindow()
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
    m_OptionPartsLocked->SetValue( m_libEntry->UnitsLocked() && m_libEntry->GetUnitCount() > 1 );
    m_AsConvertButt->SetValue( m_libEntry->HasConversion() );
    m_OptionPower->SetValue( m_libEntry->IsPower() );

    m_ShowPinNumButt->SetValue( m_libEntry->ShowPinNumbers() );
    m_ShowPinNameButt->SetValue( m_libEntry->ShowPinNames() );
    m_PinsNameInsideButt->SetValue( m_libEntry->GetPinNameOffset() != 0 );
    m_pinNameOffset.SetValue( m_libEntry->GetPinNameOffset() );

    wxArrayString tmp = m_libEntry->GetFootprints();
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


bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::Validate()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    // Alias symbol reference can be empty because it inherits from the parent symbol.
    if( m_libEntry->IsRoot() &&
        !SCH_COMPONENT::IsReferenceStringValid( m_fields->at( REFERENCE ).GetText() ) )
    {
        if( m_NoteBook->GetSelection() != 0 )
            m_NoteBook->SetSelection( 0 );

        m_delayedErrorMessage = _( "References must start with a letter." );
        m_delayedFocusGrid = m_grid;
        m_delayedFocusColumn = FDC_VALUE;
        m_delayedFocusRow = REFERENCE;
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


bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )
        return false;

    // We need to keep the name and the value the same at the moment!
    wxString   newName = m_fields->at( VALUE ).GetText();
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

        m_Parent->SaveCopyInUndoList( m_libEntry, UR_LIB_RENAME );
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

    // We need to keep the name and the value the same at the moment!
    m_libEntry->SetName( newName );
    m_libEntry->SetDescription( m_DescCtrl->GetValue() );
    m_libEntry->SetKeyWords( m_KeywordCtrl->GetValue() );
    m_libEntry->SetUnitCount( m_SelNumberOfUnits->GetValue() );
    m_libEntry->LockUnits( m_libEntry->GetUnitCount() > 1 && m_OptionPartsLocked->GetValue() );
    m_libEntry->SetConversion( m_AsConvertButt->GetValue() );

    if( m_OptionPower->GetValue() )
        m_libEntry->SetPower();
    else
        m_libEntry->SetNormal();

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

    m_libEntry->SetFootprintFilters( m_FootprintFilterListBox->GetStrings() );

    if( oldName != newName )
        m_Parent->UpdateAfterSymbolProperties( &oldName );
    else
        m_Parent->RebuildView();

    // It's possible that the symbol being edited has no pins, in which case there may be no
    // alternate body style objects causing #LIB_PART::HasCoversion() to always return false.
    // This allows the user to edit the alternate body style just in case this condition occurs.
    m_Parent->SetShowDeMorgan( m_AsConvertButt->GetValue() );

    return true;
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnGridCellChanging( wxGridEvent& event )
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
    else if( event.GetRow() == VALUE && event.GetCol() == FDC_VALUE )
        m_SymbolNameCtrl->ChangeValue( event.GetString() );

    editor->DecRef();
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnSymbolNameText( wxCommandEvent& event )
{
    m_grid->SetCellValue( VALUE, FDC_VALUE, m_SymbolNameCtrl->GetValue() );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnSymbolNameKillFocus( wxFocusEvent& event )
{
    if( !m_delayedFocusCtrl && !m_SymbolNameCtrl->GetValidator()->Validate( m_SymbolNameCtrl ) )
    {
        m_delayedFocusCtrl = m_SymbolNameCtrl;
        m_delayedFocusPage = 0;
    }

    event.Skip();
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnAddField( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    LIBEDIT_SETTINGS* settings = Pgm().GetSettingsManager().GetAppSettings<LIBEDIT_SETTINGS>();
    int               fieldID = m_fields->size();
    LIB_FIELD         newField( m_libEntry, fieldID );

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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnDeleteField( wxCommandEvent& event )
{
    int curRow = m_grid->GetGridCursorRow();

    if( curRow < 0 )
        return;
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnMoveUp( wxCommandEvent& event )
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnMoveDown( wxCommandEvent& event )
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnEditSpiceModel( wxCommandEvent& event )
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnFilterDClick( wxMouseEvent& event)
{
    int idx = m_FootprintFilterListBox->HitTest( event.GetPosition() );
    wxCommandEvent dummy;

    if( idx >= 0 )
        OnEditFootprintFilter( dummy );
    else
        OnAddFootprintFilter( dummy );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnCancelButtonClick( wxCommandEvent& event )
{
    // Running the Footprint Browser gums up the works and causes the automatic cancel
    // stuff to no longer work.  So we do it here ourselves.
    EndQuasiModal( wxID_CANCEL );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnAddFootprintFilter( wxCommandEvent& event )
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnDeleteFootprintFilter( wxCommandEvent& event )
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnEditFootprintFilter( wxCommandEvent& event )
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::adjustGridColumns( int aWidth )
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnUpdateUI( wxUpdateUIEvent& event )
{
    m_OptionPartsLocked->Enable( m_SelNumberOfUnits->GetValue() > 1 );
    m_pinNameOffset.Enable( m_PinsNameInsideButt->GetValue() );

    if( m_grid->IsCellEditControlShown() )
    {
        int row = m_grid->GetGridCursorRow();
        int col = m_grid->GetGridCursorCol();

        if( row == VALUE && col == FDC_VALUE )
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnSizeGrid( wxSizeEvent& event )
{
    auto new_size = event.GetSize().GetX();

    if( new_size != m_width )
    {
        adjustGridColumns( event.GetSize().GetX() );
    }

    // Always propagate a wxSizeEvent:
    event.Skip();
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::syncControlStates( bool aIsAlias )
{
    // Remove the not wanted notebook page.
    // *Do not use* Hide(), it is suitable to hide a widget,
    // but it is not suitable to hide a notebook page (that is not a widget)
    if( aIsAlias )
        m_NoteBook->RemovePage( 1 );

    bSizerLowerBasicPanel->Show( !aIsAlias );
    // bButtonSize->Show( !aIsAlias );

#ifdef KICAD_SPICE
    m_spiceFieldsButton->Show( !aIsAlias );
#endif

    m_inheritanceSelectCombo->Enable( aIsAlias );
    m_inheritsStaticText->Enable( aIsAlias );
    m_grid->ForceRefresh();
}
