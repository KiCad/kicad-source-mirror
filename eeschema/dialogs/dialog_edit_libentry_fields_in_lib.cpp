/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007-2018 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <algorithm>

#include <fctsys.h>
#include <pgm_base.h>
#include <kiway.h>
#include <confirm.h>
#include <class_drawpanel.h>
#include <sch_edit_frame.h>
#include <kiface_i.h>
#include <widgets/wx_grid.h>
#include <lib_edit_frame.h>
#include <class_library.h>
#include <sch_component.h>
#include <dialog_helpers.h>
#include <bitmaps.h>
#include <fields_grid_table.h>

#include <dialog_edit_libentry_fields_in_lib_base.h>

#ifdef KICAD_SPICE
#include <dialog_spice_model.h>
#include <netlist_exporter_pspice.h>
#endif /* KICAD_SPICE */


#define LibEditFieldsShownColumnsKey   wxT( "LibEditFieldsShownColumns" )



class DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB : public DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE
{
public:
    DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB( LIB_EDIT_FRAME* aParent, LIB_PART* aLibEntry );
    ~DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB() override;

private:
    wxConfigBase*   m_config;

    LIB_EDIT_FRAME* m_parent;
    LIB_PART*       m_libEntry;

    FIELDS_GRID_TABLE<LIB_FIELD>* m_fields;

    int             m_delayedFocusRow;
    int             m_delayedFocusColumn;
    wxString        m_shownColumns;

    bool TransferDataToWindow() override;

    bool Validate() override;

    // event handlers:
    void OnOKButtonClick( wxCommandEvent& event ) override;
    void OnAddField( wxCommandEvent& event ) override;
    void OnDeleteField( wxCommandEvent& event ) override;
    void OnMoveUp( wxCommandEvent& event ) override;
    void OnMoveDown( wxCommandEvent& event ) override;
    void OnEditSpiceModel( wxCommandEvent& event ) override;
    void OnSizeGrid( wxSizeEvent& event ) override;
    void OnGridCellChanging( wxGridEvent& event );
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    void AdjustGridColumns( int aWidth );
};


void LIB_EDIT_FRAME::InstallFieldsEditorDialog( wxCommandEvent& event )
{
    if( !GetCurPart() )
        return;

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB dlg( this, GetCurPart() );

    if( GetDrawItem() && GetDrawItem()->Type() == LIB_FIELD_T )
        SetDrawItem( nullptr );     // selected LIB_FIELD might be deleted

    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    if( dlg.ShowQuasiModal() != wxID_OK )
        return;

    UpdateAliasSelectList();
    UpdatePartSelectList();
    DisplayLibInfos();
    Refresh();
}


DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB( LIB_EDIT_FRAME* aParent,
                                                                        LIB_PART* aLibEntry ) :
    DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE( aParent )
{
    m_config = Kiface().KifaceSettings();

    m_parent   = aParent;
    m_libEntry = aLibEntry;
    m_fields = new FIELDS_GRID_TABLE<LIB_FIELD>( true, g_UserUnit, m_libEntry );

    m_delayedFocusRow = REFERENCE;
    m_delayedFocusColumn = FDC_VALUE;

#ifndef KICAD_SPICE
    m_spiceFieldsButton->Hide();
#endif

    // Give a bit more room for combobox editors
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 2 );

    m_grid->SetTable( m_fields );
    m_grid->PushEventHandler( new FIELDS_GRID_TRICKS( m_grid, this ) );

    stdDialogButtonSizerOK->SetDefault();

    // Configure button logos
    m_bpAdd->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_bpDelete->SetBitmap( KiBitmap( trash_xpm ) );
    m_bpMoveUp->SetBitmap( KiBitmap( small_up_xpm ) );
    m_bpMoveDown->SetBitmap( KiBitmap( small_down_xpm ) );

    // wxFormBuilder doesn't include this event...
    m_grid->Connect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnGridCellChanging ), NULL, this );

    FinishDialogSettings();
}


DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::~DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB()
{
    m_config->Write( LibEditFieldsShownColumnsKey, m_grid->GetShownColumns() );

    // Prevents crash bug in wxGrid's d'tor
    m_grid->DestroyTable( m_fields );

    m_grid->Disconnect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnGridCellChanging ), NULL, this );

    // Delete the GRID_TRICKS.
    m_grid->PopEventHandler( true );
}


bool DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    // Push a copy of each field into m_fields
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

    // Show/hide columns according to the user's preference
    m_config->Read( LibEditFieldsShownColumnsKey, &m_shownColumns, wxT( "0 1 2 3 4 5 6 7" ) );
    m_grid->ShowHideColumns( m_shownColumns );

    Layout();

    return true;
}


bool DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::Validate()
{
    // Commit any pending in-place edits and close the editor
    m_grid->DisableCellEditControl();

    if( !SCH_COMPONENT::IsReferenceStringValid( m_fields->at( REFERENCE ).GetText() ) )
    {
        DisplayErrorMessage( nullptr, _( "References must start with a letter." ) );

        m_delayedFocusColumn = FDC_VALUE;
        m_delayedFocusRow = REFERENCE;

        return false;
    }

    // Check for missing field names.
    for( size_t i = MANDATORY_FIELDS;  i < m_fields->size(); ++i )
    {
        LIB_FIELD& field = m_fields->at( i );
        wxString   fieldName = field.GetName( false );

        if( fieldName.IsEmpty() )
        {
            DisplayErrorMessage( nullptr, _( "Fields must have a name." ) );

            m_delayedFocusColumn = FDC_NAME;
            m_delayedFocusRow = i;

            return false;
        }
    }

    return true;
}


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnOKButtonClick( wxCommandEvent& event )
{
    if( !Validate() )
        return;

    // save old cmp in undo list
    m_parent->SaveCopyInUndoList( m_libEntry );

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
    SetName( m_libEntry->GetValueField().GetText() );

    m_parent->OnModify();

    EndQuasiModal( wxID_OK );
}


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnEditSpiceModel( wxCommandEvent& event )
{
#ifdef KICAD_SPICE
    // DIALOG_SPICE_MODEL expects a SCH_COMPONENT,
    // and a list of SCH_FIELDS to create/edit/delete Spice fields.
    SCH_COMPONENT component;    // This dummy component

    // Build fields list from the m_FieldsBuf fields buffer dialog
    // to be sure to use the current fields.
    SCH_FIELDS schFields;

    for( unsigned ii = 0; ii < m_fields->size(); ++ii )
    {
        LIB_FIELD& libField = m_fields->at( ii );
        SCH_FIELD schField( libField.GetTextPos(), libField.GetId(),
                            &component,  libField.GetName() );
        schField.ImportValues( m_fields->at( ii ) );
        schField.SetText( m_fields->at( ii ).GetText() );

        schFields.push_back( schField );
    }

    component.SetFields( schFields );
    int diff = schFields.size();

    DIALOG_SPICE_MODEL dialog( this, component, schFields );

    if( dialog.ShowModal() != wxID_OK )
        return;

    // Transfer sch fields to the m_FieldsBuf fields buffer dialog:
    m_fields->clear();

    for( auto& schField : schFields )
    {
        LIB_FIELD libField;
        schField.ExportValues( libField );
        m_fields->push_back( libField );
    }

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


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnGridCellChanging( wxGridEvent& event )
{
    if( event.GetCol() == FDC_NAME && event.GetString().IsEmpty() )
    {
        DisplayErrorMessage( this, _( "Fields must have a name." ) );
        event.Veto();

        m_delayedFocusRow = event.GetRow();
        m_delayedFocusColumn = event.GetCol();
    }
}


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnAddField( wxCommandEvent& event )
{
    int        fieldID = m_fields->size();
    LIB_FIELD& refField = m_fields->at( REFERENCE );
    LIB_FIELD  newField( fieldID );

    newField.SetName( TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldID ) );

    // Give new fields a slight offset so they don't all end up on top of each other
    newField.SetPosition( refField.GetTextPos()
                          + wxPoint( (fieldID - MANDATORY_FIELDS + 1) * 100,
                                     (fieldID - MANDATORY_FIELDS + 1) * 100 ) );

    newField.SetTextAngle( refField.GetTextAngle() );

    m_fields->push_back( newField );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_APPENDED, 1 );
    m_grid->ProcessTableMessage( msg );

    m_grid->MakeCellVisible( m_fields->size() - 1, 0 );
    m_grid->SetGridCursor( m_fields->size() - 1, 0 );

    m_grid->EnableCellEditControl( true );
    m_grid->ShowCellEditControl();
}


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnDeleteField( wxCommandEvent& event )
{
    int rowCount = m_grid->GetNumberRows();
    int curRow   = m_grid->GetGridCursorRow();

    if( curRow < 0 || curRow >= (int) m_fields->size() )
        return;

    if( curRow < MANDATORY_FIELDS )
    {
        DisplayError( nullptr, wxString::Format( _( "The first %d fields are mandatory." ),
                                                 MANDATORY_FIELDS ) );
        return;
    }

    auto start = m_fields->begin() + curRow;
    m_fields->erase( start, start + 1 );

    // notify the grid
    wxGridTableMessage msg( m_fields, wxGRIDTABLE_NOTIFY_ROWS_DELETED, curRow, 1 );
    m_grid->ProcessTableMessage( msg );

    if( curRow == rowCount - 1 )
    {
        m_grid->MakeCellVisible( curRow-1, m_grid->GetGridCursorCol() );
        m_grid->SetGridCursor( curRow-1, m_grid->GetGridCursorCol() );
    }
}


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnMoveUp( wxCommandEvent& event )
{
    int i = m_grid->GetGridCursorRow();

    m_grid->DisableCellEditControl();

    if( i > MANDATORY_FIELDS && i < (int) m_fields->size() )
    {
        LIB_FIELD tmp = m_fields->at( i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i - 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i - 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    }
    else
        wxBell();
}


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnMoveDown( wxCommandEvent& event )
{
    int i = m_grid->GetGridCursorRow();

    m_grid->DisableCellEditControl();

    if( i >= MANDATORY_FIELDS && i < (int) m_fields->size() - 1 )
    {
        LIB_FIELD tmp = m_fields->at( i );
        m_fields->erase( m_fields->begin() + i, m_fields->begin() + i + 1 );
        m_fields->insert( m_fields->begin() + i + 1, tmp );
        m_grid->ForceRefresh();

        m_grid->SetGridCursor( i + 1, m_grid->GetGridCursorCol() );
        m_grid->MakeCellVisible( m_grid->GetGridCursorRow(), m_grid->GetGridCursorCol() );
    }
    else
        wxBell();
}


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::AdjustGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

    m_grid->AutoSizeColumn( 0 );

    int fixedColsWidth = m_grid->GetColSize( 0 );

    for( int i = 2; i < m_grid->GetNumberCols(); i++ )
        fixedColsWidth += m_grid->GetColSize( i );

    m_grid->SetColSize( 1, aWidth - fixedColsWidth );
}


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnUpdateUI( wxUpdateUIEvent& event )
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


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnSizeGrid( wxSizeEvent& event )
{
    AdjustGridColumns( event.GetSize().GetX() );

    event.Skip();
}
