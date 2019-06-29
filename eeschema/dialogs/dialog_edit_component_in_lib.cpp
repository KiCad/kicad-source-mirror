/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiway.h>
#include <common.h>
#include <confirm.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <dialog_text_entry.h>

#include <general.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_text_button_helpers.h>
#include <lib_edit_frame.h>
#include <class_library.h>
#include <symbol_lib_table.h>
#include <sch_item.h>
#include <sch_component.h>
#include <dialog_helpers.h>
#include <bitmaps.h>

#ifdef KICAD_SPICE
#include <dialog_spice_model.h>
#include <netlist_exporter_pspice.h>
#endif /* KICAD_SPICE */

#include <dialog_edit_component_in_lib.h>


#define LibEditFieldsShownColumnsKey   wxT( "LibEditFieldsShownColumns" )

int DIALOG_EDIT_COMPONENT_IN_LIBRARY::m_lastOpenedPage = 0;


DIALOG_EDIT_COMPONENT_IN_LIBRARY::DIALOG_EDIT_COMPONENT_IN_LIBRARY( LIB_EDIT_FRAME* aParent,
                                                                    LIB_PART* aLibEntry ) :
    DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE( aParent ),
    m_Parent( aParent ),
    m_libEntry( aLibEntry ),
    m_currentAlias( wxNOT_FOUND ),
    m_pinNameOffset( aParent, m_nameOffsetLabel, m_nameOffsetCtrl, m_nameOffsetUnits, true ),
    m_delayedFocusCtrl( nullptr ),
    m_delayedFocusGrid( nullptr ),
    m_delayedFocusRow( -1 ),
    m_delayedFocusColumn( -1 ),
    m_delayedFocusPage( -1 ),
    m_width( 0 )
{
    m_config = Kiface().KifaceSettings();

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );
    m_aliasGrid->SetDefaultRowSize( m_aliasGrid->GetDefaultRowSize() + 4 );

    // Work around a bug in wxWidgets where it fails to recalculate the grid height
    // after changing the default row size
    m_aliasGrid->AppendRows( 1 );
    m_aliasGrid->DeleteRows( m_grid->GetNumberRows() - 1, 1 );

    m_fields = new FIELDS_GRID_TABLE<LIB_FIELD>( this, aParent, m_libEntry );
    m_grid->SetTable( m_fields );

    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this ) );
    m_aliasGrid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this ) );

    // Show/hide columns according to the user's preference
    m_config->Read( LibEditFieldsShownColumnsKey, &m_shownColumns, wxT( "0 1 2 3 4 5 6 7" ) );
    m_grid->ShowHideColumns( m_shownColumns );

    // Hide non-overridden rows in aliases grid
    m_aliasGrid->HideRow( REFERENCE );
    m_aliasGrid->HideRow( FOOTPRINT );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetReadOnly();
    m_aliasGrid->SetColAttr( FDC_NAME, attr );
    m_aliasGrid->SetCellValue( VALUE, FDC_NAME, TEMPLATE_FIELDNAME::GetDefaultFieldName( VALUE ) );
    m_aliasGrid->SetCellValue( DATASHEET, FDC_NAME,
                               TEMPLATE_FIELDNAME::GetDefaultFieldName( DATASHEET ) );

    attr = new wxGridCellAttr;
    attr->SetEditor( new GRID_CELL_URL_EDITOR( this ) );
    m_aliasGrid->SetAttr( DATASHEET, FDC_VALUE, attr );

    m_SymbolNameCtrl->SetValidator( SCH_FIELD_VALIDATOR( true, VALUE ) );

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_bpDelete->SetBitmap( KiBitmap( trash_xpm ) );
    m_bpMoveUp->SetBitmap( KiBitmap( small_up_xpm ) );
    m_bpMoveDown->SetBitmap( KiBitmap( small_down_xpm ) );
    m_addAliasButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_deleteAliasButton->SetBitmap( KiBitmap( trash_xpm ) );
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
    m_aliasGrid->Connect( wxEVT_GRID_CELL_CHANGING,
                          wxGridEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnAliasGridCellChanging ),
                          NULL, this );

    m_grid->GetParent()->Layout();
    m_aliasGrid->GetParent()->Layout();
    Layout();

    FinishDialogSettings();
}


DIALOG_EDIT_COMPONENT_IN_LIBRARY::~DIALOG_EDIT_COMPONENT_IN_LIBRARY()
{
    m_lastOpenedPage = m_NoteBook->GetSelection( );

    m_config->Write( LibEditFieldsShownColumnsKey, m_grid->GetShownColumns() );

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                        wxGridEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnGridCellChanging ),
                        NULL, this );
    m_aliasGrid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                             wxGridEventHandler( DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnAliasGridCellChanging ),
                             NULL, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
    m_aliasGrid->PopEventHandler( true );

    // An OK will have transferred these and cleared the buffer, but we have to delete them
    // on a Cancel.
    for( LIB_ALIAS* alias : m_aliasesBuffer )
        delete alias;
}


bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    LIB_ALIAS* rootAlias = m_libEntry->GetAlias( m_libEntry->GetName() );

    // Push a copy of each field into m_fields
    m_libEntry->GetFields( *m_fields );

    // The datasheet field is special.  Grab its value from the LIB_ALIAS document file
    // member except for old libraries that saved the root alias document file in the
    // datasheet field in the LIB_PART object.
    if( rootAlias->GetDocFileName().IsEmpty() )
    {
        m_fields->at( DATASHEET ).SetText( m_libEntry->GetField( DATASHEET )->GetText() );
        rootAlias->SetDocFileName( m_libEntry->GetField( DATASHEET )->GetText() );
    }
    else
    {
        m_fields->at( DATASHEET ).SetText( rootAlias->GetDocFileName() );
    }

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
    adjustGridColumns( m_grid->GetRect().GetWidth());

    m_SymbolNameCtrl->SetValue( m_libEntry->GetName() );

    m_DescCtrl->SetValue( rootAlias->GetDescription() );
    m_KeywordCtrl->SetValue( rootAlias->GetKeyWords() );

    m_SelNumberOfUnits->SetValue( m_libEntry->GetUnitCount() );
    m_OptionPartsLocked->SetValue( m_libEntry->UnitsLocked() && m_libEntry->GetUnitCount() > 1 );
    m_AsConvertButt->SetValue( m_libEntry->HasConversion() );
    m_OptionPower->SetValue( m_libEntry->IsPower() );

    m_ShowPinNumButt->SetValue( m_libEntry->ShowPinNumbers() );
    m_ShowPinNameButt->SetValue( m_libEntry->ShowPinNames() );
    m_PinsNameInsideButt->SetValue( m_libEntry->GetPinNameOffset() != 0 );
    m_pinNameOffset.SetValue( Mils2iu( m_libEntry->GetPinNameOffset() ) );

    const LIB_ALIASES aliases = m_libEntry->GetAliases();

    for( LIB_ALIAS* alias : aliases )
    {
        if( alias->IsRoot() )
            continue;

        m_aliasesBuffer.push_back( new LIB_ALIAS( *alias, m_libEntry ) );
        m_aliasListBox->Append( alias->GetName() );
    }

    if( m_aliasListBox->GetCount() )
        m_aliasListBox->SetSelection( 0 );

    wxCommandEvent dummy;
    OnSelectAlias( dummy );

    adjustAliasGridColumns( m_aliasGrid->GetClientRect().GetWidth() - 4 );

    m_FootprintFilterListBox->Append( m_libEntry->GetFootprints() );

    m_NoteBook->SetSelection( (unsigned) m_lastOpenedPage );

    return true;
}


bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::Validate()
{
    if( !m_grid->CommitPendingChanges() || !m_aliasGrid->CommitPendingChanges() )
        return false;

    if( !SCH_COMPONENT::IsReferenceStringValid( m_fields->at( REFERENCE ).GetText() ) )
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

    LIB_ALIAS* rootAlias = m_libEntry->GetAlias( m_libEntry->GetName() );
    // We need to keep the name and the value the same at the moment!
    wxString   newName = m_fields->at( VALUE ).GetText();

    if( m_libEntry->GetName() != newName )
        m_Parent->SaveCopyInUndoList( m_libEntry, UR_LIB_RENAME );
    else
        m_Parent->SaveCopyInUndoList( m_libEntry );

    // The Y axis for components in lib is from bottom to top while the screen axis is top
    // to bottom: we must change the y coord sign when writing back to the library
    for( size_t i = 0; i < m_fields->size(); ++i )
    {
        wxPoint pos = m_fields->at( i ).GetPosition();
        pos.y = -pos.y;
        m_fields->at( i ).SetPosition( pos );
    }

    // Datasheet field is special; copy it to the root alias docfilename
    rootAlias->SetDocFileName( m_fields->at( DATASHEET ).GetText() );
    m_fields->at( DATASHEET ).SetText( wxEmptyString );

    m_libEntry->SetFields( *m_fields );

    // We need to keep the name and the value the same at the moment!
    m_libEntry->SetName( newName );

    rootAlias->SetDescription( m_DescCtrl->GetValue() );
    rootAlias->SetKeyWords( m_KeywordCtrl->GetValue() );

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
        int offset = KiROUND( (double) m_pinNameOffset.GetValue() / IU_PER_MILS );

        // We interpret an offset of 0 as "outside", so make sure it's non-zero
        m_libEntry->SetPinNameOffset( offset == 0 ? 20 : offset );
    }
    else
    {
        m_libEntry->SetPinNameOffset( 0 );   // pin text outside the body (name is on the pin)
    }

    transferAliasDataToBuffer();
    m_libEntry->RemoveAllAliases();

    for( LIB_ALIAS* alias : m_aliasesBuffer )
        m_libEntry->AddAlias( alias );       // Transfers ownership; no need to delete

    m_aliasesBuffer.clear();

    m_libEntry->GetFootprints().Clear();
    m_libEntry->GetFootprints() = m_FootprintFilterListBox->GetStrings();

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

    int       fieldID = m_fields->size();
    LIB_FIELD newField( m_libEntry, fieldID );

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_grid->MakeCellVisible( m_fields->size() - 1, 0 );
    m_grid->SetGridCursor( m_fields->size() - 1, 0 );

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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::updateAliasName( bool aFromGrid, const wxString& aName )
{
    int idx = m_aliasListBox->GetSelection();

    if( idx >= 0 )
    {
        m_aliasListBox->SetString( (unsigned) idx, aName );
        m_aliasesBuffer[ idx ]->SetName( aName );

        if( aFromGrid )
            m_AliasNameCtrl->ChangeValue( aName );
        else
            m_aliasGrid->SetCellValue( VALUE, FDC_VALUE, aName );
    }
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnAliasGridCellChanging( wxGridEvent& event )
{
    if( event.GetRow() == VALUE )
    {
        int idx = m_aliasListBox->GetSelection();
        wxString newName = event.GetString();

        if( idx < 0 || !checkAliasName( newName ) )
        {
            event.Veto();

            m_delayedFocusGrid = m_aliasGrid;
            m_delayedFocusRow = event.GetRow();
            m_delayedFocusColumn = event.GetCol();
            m_delayedFocusPage = 1;
        }
        else
            updateAliasName( true, newName );
    }
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnAliasNameText( wxCommandEvent& event )
{
    updateAliasName( false, m_AliasNameCtrl->GetValue() );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnAliasNameKillFocus( wxFocusEvent& event )
{
    if( !m_delayedFocusCtrl && !checkAliasName( m_AliasNameCtrl->GetValue() ) )
    {
        m_delayedFocusCtrl = m_AliasNameCtrl;
        m_delayedFocusPage = 1;
    }

    event.Skip();
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::transferAliasDataToBuffer()
{
    if( m_currentAlias >= 0 )
    {
        LIB_ALIAS* alias = m_aliasesBuffer[ m_currentAlias ];

        alias->SetName( m_aliasGrid->GetCellValue( VALUE, FDC_VALUE ) );
        alias->SetDocFileName( m_aliasGrid->GetCellValue( DATASHEET, FDC_VALUE ) );
        alias->SetDescription( m_AliasDescCtrl->GetValue() );
        alias->SetKeyWords( m_AliasKeywordsCtrl->GetValue() );
    }
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnEditSpiceModel( wxCommandEvent& event )
{
#ifdef KICAD_SPICE
    int diff = m_fields->size();
    auto cmp = SCH_COMPONENT( *m_libEntry, m_libEntry->GetLibId(), nullptr );

    DIALOG_SPICE_MODEL dialog( this, cmp, m_fields );

    if( dialog.ShowModal() != wxID_OK )
        return;

    diff = m_fields->size() - diff;

    if( diff > 0 )
    {
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, diff );
        m_grid->ProcessTableMessage( msg );
    }
    else if( diff < 0 )
    {
        wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, 0, diff );
        m_grid->ProcessTableMessage( msg );
    }

    m_grid->ForceRefresh();
#endif /* KICAD_SPICE */
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnSelectAlias( wxCommandEvent& event )
{
    if( m_delayedFocusCtrl || !m_aliasGrid->CommitPendingChanges() )
    {
        m_aliasListBox->SetSelection( m_currentAlias );  // veto selection change
        return;
    }

    // Copy any pending changes back into the buffer
    transferAliasDataToBuffer();

    LIB_ALIAS* alias = nullptr;
    int newIdx = m_aliasListBox->GetSelection();

    if( newIdx >= 0 )
    {
        alias = m_aliasesBuffer[ newIdx ];

        m_aliasGrid->SetCellValue( VALUE, FDC_VALUE, alias->GetName() );
        m_aliasGrid->SetCellValue( DATASHEET, FDC_VALUE, alias->GetDocFileName() );
        m_aliasGrid->Enable( true );

        // Use ChangeValue() so we don't generate events
        m_AliasNameCtrl->ChangeValue( alias->GetName() );
        m_AliasNameCtrl->Enable( true );
        m_AliasDescCtrl->ChangeValue( alias->GetDescription() );
        m_AliasDescCtrl->Enable( true );
        m_AliasKeywordsCtrl->ChangeValue( alias->GetKeyWords() );
        m_AliasKeywordsCtrl->Enable( true );
    }
    else
    {
        m_aliasGrid->SetCellValue( VALUE, FDC_VALUE, wxEmptyString );
        m_aliasGrid->SetCellValue( DATASHEET, FDC_VALUE, wxEmptyString );
        m_aliasGrid->Enable( false );

        // Use ChangeValue() so we don't generate events
        m_AliasNameCtrl->ChangeValue( wxEmptyString );
        m_AliasNameCtrl->Enable( false );
        m_AliasDescCtrl->ChangeValue( wxEmptyString );
        m_AliasDescCtrl->Enable( false );
        m_AliasKeywordsCtrl->ChangeValue( wxEmptyString );
        m_AliasKeywordsCtrl->Enable( false );
    }

    m_currentAlias = newIdx;
}


bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::checkAliasName( const wxString& aName )
{
    if( aName.IsEmpty() )
        return false;

    if( m_SymbolNameCtrl->GetValue().CmpNoCase( aName ) == 0 )
    {
        wxString msg;
        msg.Printf( _( "Alias can not have same name as symbol." ) );
        DisplayInfoMessage( this, msg );
        return false;
    }

    for( int i = 0; i < (int)m_aliasListBox->GetCount(); ++i )
    {
        if( i == m_aliasListBox->GetSelection() )
            continue;

        if( m_aliasListBox->GetString( i ).CmpNoCase( aName ) == 0 )
        {
            wxString msg;
            msg.Printf( _( "Alias \"%s\" already exists." ), aName );
            DisplayInfoMessage( this, msg );
            return false;
        }
    }

    wxString  library = m_Parent->GetCurLib();

    if( !library.empty() )
    {
        LIB_ALIAS* existing = Prj().SchSymbolLibTable()->LoadSymbol( library, aName );

        if( existing && existing->GetPart()->GetName() != m_libEntry->GetName() )
        {
            wxString msg;
            msg.Printf( _( "Symbol name \"%s\" already exists in library \"%s\"." ),
                        aName, library );
            DisplayErrorMessage( this, msg );
            return false;
        }
    }

    return true;
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnAddAlias( wxCommandEvent& event )
{
    if( m_delayedFocusCtrl || !m_aliasGrid->CommitPendingChanges() )
        return;

    wxCommandEvent dummy;
    wxString       aliasname = _( "untitled" );
    int            suffix = 1;

    while( m_aliasListBox->FindString( aliasname ) != wxNOT_FOUND )
        aliasname = wxString::Format( _( "untitled%i" ), suffix++ );

    LIB_ALIAS* alias = new LIB_ALIAS( aliasname, m_libEntry );

    // Initialize with parent's data
    alias->SetDescription( m_DescCtrl->GetValue() );
    alias->SetKeyWords( m_KeywordCtrl->GetValue() );
    alias->SetDocFileName( m_grid->GetCellValue( DATASHEET, FDC_VALUE ) );

    m_aliasesBuffer.push_back( alias );     // transfers ownership of alias to aliasesBuffer

    m_aliasListBox->Append( aliasname );
    m_aliasListBox->SetSelection( m_aliasListBox->GetCount() - 1 );
    OnSelectAlias( dummy );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnDeleteAlias( wxCommandEvent& event )
{
    if( m_delayedFocusCtrl || !m_aliasGrid->CommitPendingChanges() )
        return;

    int sel = m_aliasListBox->GetSelection();

    if( sel == wxNOT_FOUND )
        return;

    m_aliasListBox->Delete( (unsigned) sel );
    m_aliasesBuffer.erase( m_aliasesBuffer.begin() + sel );
    m_currentAlias = wxNOT_FOUND;

    if( m_aliasListBox->GetCount() == 0 )
        m_aliasListBox->SetSelection( wxNOT_FOUND );
    else
        m_aliasListBox->SetSelection( std::max( 0, sel - 1 ) );

    wxCommandEvent dummy;
    OnSelectAlias( dummy );
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
    LIB_PART* component = m_Parent->GetCurPart();

    if( component == NULL )
        return;

    WX_TEXT_ENTRY_DIALOG dlg( this, _( "Filter:" ), _( "Add Footprint Filter" ), filterLine );

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetValue().IsEmpty() )
        return;

    filterLine = dlg.GetValue();
    filterLine.Replace( wxT( " " ), wxT( "_" ) );

    // duplicate filters do no harm, so don't be a nanny.

    m_FootprintFilterListBox->Append( filterLine );
    m_FootprintFilterListBox->SetSelection( m_FootprintFilterListBox->GetCount() - 1 );
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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::adjustAliasGridColumns( int aWidth )
{
    m_aliasGrid->AutoSizeColumn( FDC_NAME );
    m_aliasGrid->SetColSize( FDC_VALUE, aWidth - m_aliasGrid->GetColSize( FDC_NAME ) - 2 );
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

    // Synthesize a Select event when the selection is cleared
    if( m_aliasListBox->GetSelection() == wxNOT_FOUND && m_currentAlias != wxNOT_FOUND )
    {
        wxCommandEvent dummy;
        OnSelectAlias( dummy );
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

        if( dynamic_cast<wxTextEntry*>( m_delayedFocusCtrl ) )
            dynamic_cast<wxTextEntry*>( m_delayedFocusCtrl )->SelectAll();

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


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnSizeAliasGrid( wxSizeEvent& event )
{
    adjustAliasGridColumns( event.GetSize().GetX() );

    event.Skip();
}
