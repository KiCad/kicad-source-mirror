/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "dialog_edit_component_in_schematic.h"

#include <wx/tooltip.h>

#include <confirm.h>
#include <kiface_i.h>
#include <menus_helpers.h>
#include <widgets/wx_grid.h>
#include <settings/settings_manager.h>
#include <ee_collectors.h>
#include <class_library.h>
#include <fields_grid_table.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <schematic.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <dialog_sch_pin_table.h>

#ifdef KICAD_SPICE
#include <dialog_spice_model.h>
#endif /* KICAD_SPICE */


DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( SCH_EDIT_FRAME* aParent,
                                                                        SCH_COMPONENT* aComponent ) :
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE( aParent )
{
    m_cmp = aComponent;
    m_part = m_cmp->GetPartRef().get();
    m_fields = new FIELDS_GRID_TABLE<SCH_FIELD>( this, aParent, m_part );

    m_width = 0;
    m_delayedFocusRow = REFERENCE;
    m_delayedFocusColumn = FDC_VALUE;

#ifndef KICAD_SPICE
    m_spiceFieldsButton->Hide();
#endif /* not KICAD_SPICE */

    // disable some options inside the edit dialog which can cause problems while dragging
    if( m_cmp->IsDragging() )
    {
        m_orientationLabel->Disable();
        m_orientationCtrl->Disable();
        m_mirrorLabel->Disable();
        m_mirrorCtrl->Disable();
    }

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this ) );

    // Show/hide columns according to user's preference
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    if( cfg )
    {
        m_shownColumns = cfg->m_Appearance.edit_component_visible_columns;
        m_grid->ShowHideColumns( m_shownColumns );
    }

    // Set font size for items showing long strings:
    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );

    m_libraryIDLabel->SetFont( infoFont );
    m_tcLibraryID->SetFont( infoFont );

    wxToolTip::Enable( true );
    m_stdDialogButtonSizerOK->SetDefault();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_bpDelete->SetBitmap( KiBitmap( trash_xpm ) );
    m_bpMoveUp->SetBitmap( KiBitmap( small_up_xpm ) );
    m_bpMoveDown->SetBitmap( KiBitmap( small_down_xpm ) );

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING,
                     wxGridEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnGridCellChanging ),
                     NULL, this );

    FinishDialogSettings();
}


DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::~DIALOG_EDIT_COMPONENT_IN_SCHEMATIC()
{
    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    if( cfg )
        cfg->m_Appearance.edit_component_visible_columns = m_grid->GetShownColumns();

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING,
                        wxGridEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnGridCellChanging ),
                        NULL, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


SCH_EDIT_FRAME* DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::GetParent()
{
    return dynamic_cast<SCH_EDIT_FRAME*>( wxDialog::GetParent() );
}


bool DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    std::set<wxString> defined;

    // Push a copy of each field into m_updateFields
    for( int i = 0; i < m_cmp->GetFieldCount(); ++i )
    {
        SCH_FIELD field( *m_cmp->GetField( i ) );

        // change offset to be symbol-relative
        field.Offset( -m_cmp->GetPosition() );

        defined.insert( field.GetName() );
        m_fields->push_back( field );
    }

    // Add in any template fieldnames not yet defined:
    for( const TEMPLATE_FIELDNAME& templateFieldname :
            GetParent()->Schematic().Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
    {
        if( defined.count( templateFieldname.m_Name ) <= 0 )
        {
            SCH_FIELD field( wxPoint( 0, 0 ), -1, m_cmp, templateFieldname.m_Name );
            field.SetVisible( templateFieldname.m_Visible );
            m_fields->push_back( field );
        }
    }

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, m_fields->size() );
    m_grid->ProcessTableMessage( msg );
    AdjustGridColumns( m_grid->GetRect().GetWidth() );

    // If a multi-unit component, set up the unit selector and interchangeable checkbox.
    if( m_cmp->GetUnitCount() > 1 )
    {
        for( int ii = 1; ii <= m_cmp->GetUnitCount(); ii++ )
            m_unitChoice->Append( LIB_PART::SubReference( ii, false ) );

        if( m_cmp->GetUnit() <= ( int )m_unitChoice->GetCount() )
            m_unitChoice->SetSelection( m_cmp->GetUnit() - 1 );
    }
    else
    {
        m_unitLabel->Enable( false );
        m_unitChoice->Enable( false );
    }

    if( m_part != nullptr && m_part->HasConversion() )
    {
        if( m_cmp->GetConvert() > LIB_ITEM::LIB_CONVERT::BASE )
            m_cbAlternateSymbol->SetValue( true );
    }
    else
        m_cbAlternateSymbol->Enable( false );

    // Set the symbol orientation and mirroring.
    int orientation = m_cmp->GetOrientation() & ~( CMP_MIRROR_X | CMP_MIRROR_Y );

    switch( orientation )
    {
    default:
    case CMP_ORIENT_0:   m_orientationCtrl->SetSelection( 0 ); break;
    case CMP_ORIENT_90:  m_orientationCtrl->SetSelection( 1 ); break;
    case CMP_ORIENT_270: m_orientationCtrl->SetSelection( 2 ); break;
    case CMP_ORIENT_180: m_orientationCtrl->SetSelection( 3 ); break;
    }

    int mirror = m_cmp->GetOrientation() & ( CMP_MIRROR_X | CMP_MIRROR_Y );

    switch( mirror )
    {
    default:           m_mirrorCtrl->SetSelection( 0 ) ; break;
    case CMP_MIRROR_X: m_mirrorCtrl->SetSelection( 1 ); break;
    case CMP_MIRROR_Y: m_mirrorCtrl->SetSelection( 2 ); break;
    }

    m_cbExcludeFromBom->SetValue( !m_cmp->GetIncludeInBom() );
    m_cbExcludeFromBoard->SetValue( !m_cmp->GetIncludeOnBoard() );

    m_ShowPinNumButt->SetValue( m_part->ShowPinNumbers() );
    m_ShowPinNameButt->SetValue( m_part->ShowPinNames() );

    // Set the component's library name.
    m_tcLibraryID->SetValue( m_cmp->GetLibId().Format() );

    Layout();

    return true;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnEditSpiceModel( wxCommandEvent& event )
{
#ifdef KICAD_SPICE
    int diff = m_fields->size();

    DIALOG_SPICE_MODEL dialog( this, *m_cmp, m_fields );

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


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnEditPinTable( wxCommandEvent& event )
{
    DIALOG_SCH_PIN_TABLE dialog( GetParent(), m_cmp );

    dialog.ShowModal();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnCancelButtonClick( wxCommandEvent& event )
{
    // Running the Footprint Browser gums up the works and causes the automatic cancel
    // stuff to no longer work.  So we do it here ourselves.
    EndQuasiModal( wxID_CANCEL );
}


bool DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::Validate()
{
    wxString msg;
    LIB_ID   id;

    if( !m_grid->CommitPendingChanges() || !m_grid->Validate() )
        return false;

    if( !SCH_COMPONENT::IsReferenceStringValid( m_fields->at( REFERENCE ).GetText() ) )
    {
        DisplayErrorMessage( this, _( "References must start with a letter." ) );

        m_delayedFocusColumn = FDC_VALUE;
        m_delayedFocusRow = REFERENCE;

        return false;
    }

    // Check for missing field names.
    for( size_t i = MANDATORY_FIELDS;  i < m_fields->size(); ++i )
    {
        SCH_FIELD& field = m_fields->at( i );
        wxString   fieldName = field.GetName( false );

        if( fieldName.IsEmpty() )
        {
            DisplayErrorMessage( this, _( "Fields must have a name." ) );

            m_delayedFocusColumn = FDC_NAME;
            m_delayedFocusRow = i;

            return false;
        }
    }

    return true;
}


bool DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::TransferDataFromWindow()
{
    if( !wxDialog::TransferDataFromWindow() )  // Calls our Validate() method.
        return false;

    SCH_SCREEN* currentScreen = GetParent()->GetScreen();
    SCHEMATIC&  schematic = GetParent()->Schematic();

    wxCHECK( currentScreen, false );

    // This needs to be done before the LIB_ID is changed to prevent stale library symbols in
    // the schematic file.
    currentScreen->Remove( m_cmp );

    wxString msg;

    // save old cmp in undo list if not already in edit, or moving ...
    if( m_cmp->GetEditFlags() == 0 )
        GetParent()->SaveCopyInUndoList( currentScreen, m_cmp, UNDO_REDO::CHANGED, false );

    // Save current flags which could be modified by next change settings
    STATUS_FLAGS flags = m_cmp->GetFlags();

    // For symbols with multiple shapes (De Morgan representation) Set the selected shape:
    if( m_cbAlternateSymbol->IsEnabled() && m_cbAlternateSymbol->GetValue() )
        m_cmp->SetConvert( LIB_ITEM::LIB_CONVERT::DEMORGAN );
    else
        m_cmp->SetConvert( LIB_ITEM::LIB_CONVERT::BASE );

    //Set the part selection in multiple part per package
    int unit_selection = m_unitChoice->IsEnabled() ? m_unitChoice->GetSelection() + 1 : 1;
    m_cmp->SetUnitSelection( &GetParent()->GetCurrentSheet(), unit_selection );
    m_cmp->SetUnit( unit_selection );

    switch( m_orientationCtrl->GetSelection() )
    {
    case 0: m_cmp->SetOrientation( CMP_ORIENT_0 );   break;
    case 1: m_cmp->SetOrientation( CMP_ORIENT_90 );  break;
    case 2: m_cmp->SetOrientation( CMP_ORIENT_270 ); break;
    case 3: m_cmp->SetOrientation( CMP_ORIENT_180 ); break;
    }

    switch( m_mirrorCtrl->GetSelection() )
    {
    case 0:                                        break;
    case 1: m_cmp->SetOrientation( CMP_MIRROR_X ); break;
    case 2: m_cmp->SetOrientation( CMP_MIRROR_Y ); break;
    }

    m_part->SetShowPinNames( m_ShowPinNameButt->GetValue() );
    m_part->SetShowPinNumbers( m_ShowPinNumButt->GetValue() );

    // Restore m_Flag modified by SetUnit() and other change settings
    m_cmp->ClearFlags();
    m_cmp->SetFlags( flags );

    // change all field positions from relative to absolute
    for( unsigned i = 0;  i < m_fields->size();  ++i )
        m_fields->at( i ).Offset( m_cmp->GetPosition() );

    LIB_PART* entry = GetParent()->GetLibPart( m_cmp->GetLibId() );

    if( entry && entry->IsPower() )
        m_fields->at( VALUE ).SetText( m_cmp->GetLibId().GetLibItemName() );

    // Push all fields to the component -except- for those which are TEMPLATE_FIELDNAMES
    // with empty values.
    SCH_FIELDS& fields = m_cmp->GetFields();

    fields.clear();

    for( size_t i = 0; i < m_fields->size(); ++i )
    {
        SCH_FIELD& field = m_fields->at( i );
        bool       emptyTemplateField = false;

        if( i >= MANDATORY_FIELDS )
        {
            for( const TEMPLATE_FIELDNAME& fieldname :
                    schematic.Settings().m_TemplateFieldNames.GetTemplateFieldNames() )
            {
                if( field.GetName() == fieldname.m_Name && field.GetText().IsEmpty() )
                {
                    emptyTemplateField = true;
                    break;
                }
            }
        }

        if( !emptyTemplateField )
            fields.push_back( field );
    }

    // Reference has a specific initialization, depending on the current active sheet
    // because for a given component, in a complex hierarchy, there are more than one
    // reference.
    m_cmp->SetRef( &GetParent()->GetCurrentSheet(), m_fields->at( REFERENCE ).GetText() );

    m_cmp->SetIncludeInBom( !m_cbExcludeFromBom->IsChecked() );
    m_cmp->SetIncludeOnBoard( !m_cbExcludeFromBoard->IsChecked() );

    // The value, footprint and datasheet fields and exclude from bill of materials setting
    // should be kept in sync in multi-unit parts.
    if( m_cmp->GetUnitCount() > 1 )
    {
        for( SCH_SHEET_PATH& sheet : GetParent()->Schematic().GetSheets() )
        {
            SCH_SCREEN*                 screen = sheet.LastScreen();
            std::vector<SCH_COMPONENT*> otherUnits;

            CollectOtherUnits( sheet, m_cmp, &otherUnits );

            for( SCH_COMPONENT* otherUnit : otherUnits )
            {
                GetParent()->SaveCopyInUndoList( screen, otherUnit, UNDO_REDO::CHANGED, true /* append */);
                otherUnit->GetField( VALUE )->SetText( m_fields->at( VALUE ).GetText() );
                otherUnit->GetField( FOOTPRINT )->SetText( m_fields->at( FOOTPRINT ).GetText() );
                otherUnit->GetField( DATASHEET )->SetText( m_fields->at( DATASHEET ).GetText() );
                otherUnit->SetIncludeInBom( !m_cbExcludeFromBom->IsChecked() );
                otherUnit->SetIncludeOnBoard( !m_cbExcludeFromBoard->IsChecked() );
                GetParent()->UpdateItem( otherUnit );
            }
        }
    }

    currentScreen->Append( m_cmp );
    GetParent()->TestDanglingEnds();
    GetParent()->UpdateItem( m_cmp );
    GetParent()->OnModify();

    // This must go after OnModify() so that the connectivity graph will have been updated.
    GetParent()->GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );

    return true;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnGridCellChanging( wxGridEvent& event )
{
    wxGridCellEditor* editor = m_grid->GetCellEditor( event.GetRow(), event.GetCol() );
    wxControl* control = editor->GetControl();

    if( control && control->GetValidator() && !control->GetValidator()->Validate( control ) )
    {
        event.Veto();
        m_delayedFocusRow = event.GetRow();
        m_delayedFocusColumn = event.GetCol();
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


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnAddField( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    SCHEMATIC_SETTINGS& settings = m_cmp->Schematic()->Settings();
    int                 fieldID = m_fields->size();
    SCH_FIELD           newField( wxPoint( 0, 0 ), fieldID, m_cmp,
                                  TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldID ) );

    newField.SetTextAngle( m_fields->at( REFERENCE ).GetTextAngle() );
    newField.SetTextSize( wxSize( settings.m_DefaultTextSize, settings.m_DefaultTextSize ) );

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_grid->MakeCellVisible( (int) m_fields->size() - 1, 0 );
    m_grid->SetGridCursor( (int) m_fields->size() - 1, 0 );

    m_grid->EnableCellEditControl();
    m_grid->ShowCellEditControl();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnDeleteField( wxCommandEvent& event )
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


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnMoveUp( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int i = m_grid->GetGridCursorRow();

    if( i > MANDATORY_FIELDS )
    {
        SCH_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i - 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i - 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnMoveDown( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int i = m_grid->GetGridCursorRow();

    if( i >= MANDATORY_FIELDS && i < m_grid->GetNumberRows() - 1 )
    {
        SCH_FIELD tmp = m_fields->at( (unsigned) i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i + 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i + 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnEditSymbol( wxCommandEvent&  )
{
    EndQuasiModal( SYMBOL_PROPS_EDIT_SCHEMATIC_SYMBOL );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnEditLibrarySymbol( wxCommandEvent&  )
{
    EndQuasiModal( SYMBOL_PROPS_EDIT_LIBRARY_SYMBOL );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnUpdateSymbol( wxCommandEvent&  )
{
    EndQuasiModal( SYMBOL_PROPS_WANT_UPDATE_SYMBOL );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnExchangeSymbol( wxCommandEvent&  )
{
    EndQuasiModal( SYMBOL_PROPS_WANT_EXCHANGE_SYMBOL );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::AdjustGridColumns( int aWidth )
{
    m_width = aWidth;
    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

    m_grid->AutoSizeColumn( 0 );

    int fixedColsWidth = m_grid->GetColSize( 0 );

    for( int i = 2; i < m_grid->GetNumberCols(); i++ )
        fixedColsWidth += m_grid->GetColSize( i );

    m_grid->SetColSize( 1, aWidth - fixedColsWidth );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnUpdateUI( wxUpdateUIEvent& event )
{
    wxString shownColumns = m_grid->GetShownColumns();

    if( shownColumns != m_shownColumns )
    {
        m_shownColumns = shownColumns;

        if( !m_grid->IsCellEditControlShown() )
            AdjustGridColumns( m_grid->GetRect().GetWidth() );
    }

    // Handle a delayed focus
    if( m_delayedFocusRow >= 0 )
    {
        m_grid->SetFocus();
        m_grid->MakeCellVisible( m_delayedFocusRow, m_delayedFocusColumn );
        m_grid->SetGridCursor( m_delayedFocusRow, m_delayedFocusColumn );


        m_grid->EnableCellEditControl( true );
        m_grid->ShowCellEditControl();

        m_delayedFocusRow = -1;
        m_delayedFocusColumn = -1;
    }
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnSizeGrid( wxSizeEvent& event )
{
    auto new_size = event.GetSize().GetX();

    if( m_width != new_size )
    {
        AdjustGridColumns( new_size );
    }

    // Always propagate for a grid repaint (needed if the height changes, as well as width)
    event.Skip();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnInitDlg( wxInitDialogEvent& event )
{
    TransferDataToWindow();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}
