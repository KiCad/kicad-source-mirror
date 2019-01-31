/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <wx/tooltip.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <sch_edit_frame.h>
#include <grid_tricks.h>
#include <menus_helpers.h>
#include <widgets/wx_grid.h>
#include <sch_reference_list.h>
#include <class_library.h>
#include <symbol_lib_table.h>
#include <fields_grid_table.h>

#include <dialog_edit_component_in_schematic_base.h>
#include <invoke_sch_dialog.h>

#ifdef KICAD_SPICE
#include <dialog_spice_model.h>
#include <netlist_exporter_pspice.h>
#endif /* KICAD_SPICE */



#define SymbolFieldsShownColumnsKey   wxT( "SymbolFieldsShownColumns" )


/**
 * Dialog used to edit #SCH_COMPONENT objects in a schematic.
 *
 * This is derived from DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE which is maintained by
 * wxFormBuilder.  Do not auto-generate this class or file, it is hand coded.
 */
class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC : public DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE
{
public:
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( SCH_EDIT_FRAME* aParent, SCH_COMPONENT* aComponent );
    ~DIALOG_EDIT_COMPONENT_IN_SCHEMATIC() override;

    SCH_EDIT_FRAME* GetParent() { return dynamic_cast<SCH_EDIT_FRAME*>( wxDialog::GetParent() ); }

private:
    wxConfigBase*   m_config;

    SCH_COMPONENT*  m_cmp;
    LIB_PART*       m_part;

    int             m_delayedFocusRow;
    int             m_delayedFocusColumn;
    wxString        m_shownColumns;

    FIELDS_GRID_TABLE<SCH_FIELD>* m_fields;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    // event handlers
    void UpdateFieldsFromLibrary( wxCommandEvent& event ) override;
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void OnBrowseLibrary( wxCommandEvent& event ) override;
    void OnEditSpiceModel( wxCommandEvent& event ) override;
    void OnSizeGrid( wxSizeEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnCancelButtonClick( wxCommandEvent& event ) override;

    void OnInitDlg( wxInitDialogEvent& event ) override
    {
        TransferDataToWindow();

        // Now all widgets have the size fixed, call FinishDialogSettings
        FinishDialogSettings();
    }

    void AdjustGridColumns( int aWidth );
};


void SCH_EDIT_FRAME::EditComponent( SCH_COMPONENT* aComponent )
{
    wxCHECK_RET( aComponent != nullptr && aComponent->Type() == SCH_COMPONENT_T,
                 wxT( "Invalid component object pointer.  Bad Programmer!" ) );

    m_canvas->SetIgnoreMouseEvents( true );

    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC dlg( this, aComponent );

    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    int ret = dlg.ShowQuasiModal();

    m_canvas->SetIgnoreMouseEvents( false );
    m_canvas->MoveCursorToCrossHair();

    if( ret == wxID_OK )
    {
        if( m_autoplaceFields )
            aComponent->AutoAutoplaceFields( GetScreen() );

        GetCanvas()->Refresh();
    }
}


DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( SCH_EDIT_FRAME* aParent,
                                                                        SCH_COMPONENT* aComponent ) :
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_BASE( aParent )
{
    m_config = Kiface().KifaceSettings();

    m_cmp = aComponent;
    m_part = GetParent()->GetLibPart( m_cmp->GetLibId(), true );
    m_fields = new FIELDS_GRID_TABLE<SCH_FIELD>( this, aParent, m_part );

    m_delayedFocusRow = REFERENCE;
    m_delayedFocusColumn = FDC_VALUE;

#ifndef KICAD_SPICE
    m_spiceFieldsButton->Hide();
#endif /* not KICAD_SPICE */

    // disable some options inside the edit dialog which can cause problems while dragging
    if( m_cmp->IsDragging() )
    {
        m_rbOrientation->Disable();
        m_rbMirror->Disable();
        m_libraryNameTextCtrl->Disable();
    }

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );

    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this ) );

    // Show/hide columns according to user's preference
    m_config->Read( SymbolFieldsShownColumnsKey, &m_shownColumns, wxT( "0 1 2 3 4 5 6 7" ) );
    m_grid->ShowHideColumns( m_shownColumns );

    wxToolTip::Enable( true );
    m_stdDialogButtonSizerOK->SetDefault();

    // Configure button logos
    m_buttonBrowseLibrary->SetBitmap( KiBitmap( small_library_xpm ) );
    m_bpAdd->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_bpDelete->SetBitmap( KiBitmap( trash_xpm ) );
    m_bpMoveUp->SetBitmap( KiBitmap( small_up_xpm ) );
    m_bpMoveDown->SetBitmap( KiBitmap( small_down_xpm ) );

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnGridCellChanging ), NULL, this );

    FinishDialogSettings();
}


DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::~DIALOG_EDIT_COMPONENT_IN_SCHEMATIC()
{
    m_config->Write( SymbolFieldsShownColumnsKey, m_grid->GetShownColumns() );

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnGridCellChanging ), NULL, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    std::set<wxString> defined;

    // Push a copy of each field into m_fields
    for( int i = 0; i < m_cmp->GetFieldCount(); ++i )
    {
        SCH_FIELD field( *m_cmp->GetField( i ) );

        // change offset to be symbol-relative
        field.Offset( -m_cmp->GetPosition() );

        defined.insert( field.GetName() );
        m_fields->push_back( field );
    }

    // Add in any template fieldnames not yet defined:
    for( TEMPLATE_FIELDNAME templateFieldname : GetParent()->GetTemplateFieldNames() )
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
        if( m_cmp->GetConvert() > 1 )
            m_cbAlternateSymbol->SetValue( true );
    }
    else
        m_cbAlternateSymbol->Enable( false );

    // Set the symbol orientation and mirroring.
    int orientation = m_cmp->GetOrientation() & ~( CMP_MIRROR_X | CMP_MIRROR_Y );

    if( orientation == CMP_ORIENT_90 )
        m_rbOrientation->SetSelection( 1 );
    else if( orientation == CMP_ORIENT_180 )
        m_rbOrientation->SetSelection( 2 );
    else if( orientation == CMP_ORIENT_270 )
        m_rbOrientation->SetSelection( 3 );
    else
        m_rbOrientation->SetSelection( 0 );

    int mirror = m_cmp->GetOrientation() & ( CMP_MIRROR_X | CMP_MIRROR_Y );

    if( mirror == CMP_MIRROR_X )
    {
        m_rbMirror->SetSelection( 1 );
        DBG( printf( "mirror=X,1\n" ); )
    }
    else if( mirror == CMP_MIRROR_Y )
    {
        m_rbMirror->SetSelection( 2 );
        DBG( printf( "mirror=Y,2\n" ); )
    }
    else
        m_rbMirror->SetSelection( 0 );

    // Set the component's unique ID time stamp.
    m_textCtrlTimeStamp->SetValue( wxString::Format( wxT( "%8.8lX" ),
                                   (unsigned long) m_cmp->GetTimeStamp() ) );

    // Set the component's library name.
    m_libraryNameTextCtrl->SetValue( m_cmp->GetLibId().Format() );

    Layout();

    return true;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnBrowseLibrary( wxCommandEvent& event )
{
    SCH_BASE_FRAME::HISTORY_LIST dummy;

    LIB_ID id;
    id.Parse( m_libraryNameTextCtrl->GetValue(), LIB_ID::ID_SCH );

    auto sel = GetParent()->SelectComponentFromLibTree( nullptr, dummy, true, 0, 0, false, &id );

    if( !sel.LibId.IsValid() )
        return;

    m_libraryNameTextCtrl->SetValue( sel.LibId.Format() );

    // Update the value field for Power symbols
    LIB_PART* entry = GetParent()->GetLibPart( sel.LibId );

    if( entry && entry->IsPower() )
        m_grid->SetCellValue( VALUE, FDC_VALUE, sel.LibId.GetLibItemName() );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnEditSpiceModel( wxCommandEvent& event )
{
#ifdef KICAD_SPICE
    int diff = m_fields->size();

    DIALOG_SPICE_MODEL dialog( this, *m_cmp, m_fields );

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

    if( !m_grid->CommitPendingChanges() )
        return false;

    if( !SCH_COMPONENT::IsReferenceStringValid( m_fields->at( REFERENCE ).GetText() ) )
    {
        DisplayErrorMessage( this, _( "References must start with a letter." ) );

        m_delayedFocusColumn = FDC_VALUE;
        m_delayedFocusRow = REFERENCE;

        return false;
    }

    id.Parse( m_libraryNameTextCtrl->GetValue(), LIB_ID::ID_SCH );

    if( !id.IsValid() )
    {
        DisplayErrorMessage( this, _( "Library reference is not valid." ) );

        m_libraryNameTextCtrl->SetFocus();

        return false;
    }
    else if( id != m_cmp->GetLibId() )
    {
        LIB_ALIAS* alias = nullptr;

        try
        {
            alias = Prj().SchSymbolLibTable()->LoadSymbol( id );
        }
        catch( ... )
        {
        }

        if( !alias )
        {
            msg.Printf( _( "Symbol \"%s\" not found in library \"%s\"." ),
                        id.GetLibItemName().wx_str(),
                        id.GetLibNickname().wx_str() );
            DisplayErrorMessage( this, msg );

            m_libraryNameTextCtrl->SetFocus();

            return false;
        }
    }
    m_libraryNameTextCtrl->SetValue( id.Format() );

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
    if( !Validate() )
        return false;

    // save old cmp in undo list if not already in edit, or moving ...
    // or the component to be edited is part of a block
    if( m_cmp->GetFlags() == 0 || GetParent()->GetScreen()->m_BlockLocate.GetState() != STATE_NO_BLOCK )
        GetParent()->SaveCopyInUndoList( m_cmp, UR_CHANGED );

    // Save current flags which could be modified by next change settings
    STATUS_FLAGS flags = m_cmp->GetFlags();

    // Library symbol identifier
    LIB_ID id;
    id.Parse( m_libraryNameTextCtrl->GetValue(), LIB_ID::ID_SCH, true );
    m_cmp->SetLibId( id, Prj().SchSymbolLibTable(), Prj().SchLibs()->GetCacheLibrary() );

    // For symbols with multiple shapes (De Morgan representation) Set the selected shape:
    if( m_cbAlternateSymbol->IsEnabled() )
        m_cmp->SetConvert( m_cbAlternateSymbol->GetValue() ? 2 : 1 );

    //Set the part selection in multiple part per package
    if( m_cmp->GetUnitCount() > 1 )
    {
        int unit_selection = m_unitChoice->GetCurrentSelection() + 1;

        m_cmp->SetUnitSelection( &GetParent()->GetCurrentSheet(), unit_selection );
        m_cmp->SetUnit( unit_selection );
    }

    switch( m_rbOrientation->GetSelection() )
    {
    case 0: m_cmp->SetOrientation( CMP_ORIENT_0 );   break;
    case 1: m_cmp->SetOrientation( CMP_ORIENT_90 );  break;
    case 2: m_cmp->SetOrientation( CMP_ORIENT_180 ); break;
    case 3: m_cmp->SetOrientation( CMP_ORIENT_270 ); break;
    }

    switch( m_rbMirror->GetSelection() )
    {
    case 0:                                        break;
    case 1: m_cmp->SetOrientation( CMP_MIRROR_X ); break;
    case 2: m_cmp->SetOrientation( CMP_MIRROR_Y ); break;
    }

    // Restore m_Flag modified by SetUnit() and other change settings
    m_cmp->ClearFlags();
    m_cmp->SetFlags( flags );

    // change all field positions from relative to absolute
    for( unsigned i = 0;  i < m_fields->size();  ++i )
        m_fields->at( i ).Offset( m_cmp->GetPosition() );

    LIB_PART* entry = GetParent()->GetLibPart( m_cmp->GetLibId() );

    if( entry && entry->IsPower() )
        m_fields->at( VALUE ).SetText( m_cmp->GetLibId().GetLibItemName() );

    // Remove any TEMPLATE_FIELDNAMES which were not set (given values).
    TEMPLATE_FIELDNAMES templateFieldnames = GetParent()->GetTemplateFieldNames();

    for( size_t i = MANDATORY_FIELDS; i < m_fields->size(); ++i )
    {
        SCH_FIELD& field = m_fields->at( i );

        for( auto fieldname : templateFieldnames )
        {
            if( field.GetName() == fieldname.m_Name && field.GetText().IsEmpty() )
            {
                m_fields->erase( m_fields->begin() + i );

                // grid needs to be notified about the size change,
                // as it still accesses the data on close (size event)
                wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, i, 1 );
                m_grid->ProcessTableMessage( msg );
                i--;
                break;
            }
        }
    }

    m_cmp->SetFields( *m_fields );

    // Reference has a specific initialization, depending on the current active sheet
    // because for a given component, in a complex hierarchy, there are more than one
    // reference.
    m_cmp->SetRef( &GetParent()->GetCurrentSheet(), m_fields->at( REFERENCE ).GetText() );

    // The value, footprint and datasheet fields should be kept in sync in multi-unit
    // parts.
    if( m_cmp->GetUnitCount() > 1 )
    {
        const LIB_ID   thisLibId = m_cmp->GetLibId();
        const wxString thisRef   = m_cmp->GetRef( &( GetParent()->GetCurrentSheet() ) );
        int            thisUnit  = m_cmp->GetUnit();

        SCH_REFERENCE_LIST components;
        GetParent()->GetCurrentSheet().GetComponents( components );
        for( unsigned i = 0; i < components.GetCount(); i++ )
        {
            SCH_REFERENCE component = components[i];
            if( component.GetLibPart()->GetLibId() == thisLibId
                    && component.GetRef() == thisRef
                    && component.GetUnit() != thisUnit )
            {
                SCH_COMPONENT* otherUnit = component.GetComp();
                GetParent()->SaveCopyInUndoList( otherUnit, UR_CHANGED, true /* append */);
                otherUnit->GetField( VALUE )->SetText( m_fields->at( VALUE ).GetText() );
                otherUnit->GetField( FOOTPRINT )->SetText( m_fields->at( FOOTPRINT ).GetText() );
                otherUnit->GetField( DATASHEET )->SetText( m_fields->at( DATASHEET ).GetText() );
            }
        }
    }

    m_cmp->UpdatePinCache();

    GetParent()->TestDanglingEnds();
    GetParent()->RefreshItem( m_cmp );
    GetParent()->OnModify();

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

    editor->DecRef();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnAddField( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    int       fieldID = m_fields->size();
    SCH_FIELD newField( wxPoint( 0, 0 ), fieldID, m_cmp );

    newField.SetName( TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldID ) );
    newField.SetTextAngle( m_fields->at( REFERENCE ).GetTextAngle() );

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_grid->MakeCellVisible( m_fields->size() - 1, 0 );
    m_grid->SetGridCursor( m_fields->size() - 1, 0 );

    m_grid->EnableCellEditControl();
    m_grid->ShowCellEditControl();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnDeleteField( wxCommandEvent& event )
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
        wxBell();
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
        wxBell();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::UpdateFieldsFromLibrary( wxCommandEvent& event )
{
    if( !m_grid->CommitPendingChanges() )
        return;

    SCH_COMPONENT copy( *m_cmp );
    copy.SetFields( *m_fields );

    // Update the requested fields in the component copy
    std::list<SCH_COMPONENT*> components;
    components.push_back( &copy );
    InvokeDialogUpdateFields( GetParent(), components, false );

    // Copy fields from the component copy to the dialog buffer
    m_fields->clear();
    std::set<wxString> defined;

    for( int i = 0; i < copy.GetFieldCount(); ++i )
    {
        copy.GetField( i )->SetParent( m_cmp );

        defined.insert( copy.GetField( i )->GetName() );
        m_fields->push_back( *copy.GetField( i ) );
    }

    // Add in any template fieldnames not yet defined:
    for( TEMPLATE_FIELDNAME templateFieldname : GetParent()->GetTemplateFieldNames() )
    {
        if( defined.count( templateFieldname.m_Name ) <= 0 )
        {
            SCH_FIELD field( wxPoint( 0, 0 ), -1, m_cmp, templateFieldname.m_Name );
            field.SetVisible( templateFieldname.m_Visible );
            m_fields->push_back( field );
        }
    }

    m_grid->ForceRefresh();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::AdjustGridColumns( int aWidth )
{
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
    AdjustGridColumns( event.GetSize().GetX() );

    event.Skip();
}
