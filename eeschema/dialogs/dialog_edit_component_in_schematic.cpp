/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file dialog_edit_component_in_schematic.cpp
 */

#include <wx/tooltip.h>
#include <wx/hyperlink.h>

#include <fctsys.h>
#include <pgm_base.h>
#include <kiway.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <class_sch_screen.h>
#include <schframe.h>
#include <base_units.h>

#include <general.h>
#include <sch_base_frame.h>
#include <class_library.h>
#include <sch_component.h>
#include <dialog_helpers.h>
#include <dialog_edit_component_in_schematic_fbp.h>


/**
 * class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC
 * is hand coded and implements DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP which
 * is maintained by wxFormBuilder.  Do not auto-generate this class or file,
 * it is hand coded.
 */
class DIALOG_EDIT_COMPONENT_IN_SCHEMATIC : public DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP
{
public:
    /** Constructor */
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( wxWindow* aParent );

    /**
     * Function InitBuffers
     * sets up to edit the given component.
     * @param aComponent The component to edit.
     */
    void InitBuffers( SCH_COMPONENT* aComponent );

private:

    friend class SCH_EDIT_FRAME;

    SCH_EDIT_FRAME* m_parent;
    SCH_COMPONENT*  m_cmp;
    LIB_PART*       m_part;
    bool            m_skipCopyFromPanel;

    static int      s_SelectedRow;

    /// a copy of the edited component's SCH_FIELDs
    SCH_FIELDS      m_FieldsBuf;

    void setSelectedFieldNdx( int aFieldNdx );

    int getSelectedFieldNdx();

    /**
     * Function copySelectedFieldToPanel
     * sets the values displayed on the panel according to
     * the currently selected field row
     */
    void copySelectedFieldToPanel();


    /**
     * Function copyPanelToSelectedField
     * copies the values displayed on the panel fields to the currently
     * selected field
     * @return bool - true if all fields are OK, else false if the user has put
     *   bad data into a field, and this value can be used to deny a row change.
     */
    bool copyPanelToSelectedField();

    void copyOptionsToPanel();

    void copyPanelToOptions();

    void setRowItem( int aFieldNdx, const SCH_FIELD& aField );

    // event handlers
    void OnCloseDialog( wxCloseEvent& event );
    void OnListItemDeselected( wxListEvent& event );
    void OnListItemSelected( wxListEvent& event );
    void OnCancelButtonClick( wxCommandEvent& event );
    void OnOKButtonClick( wxCommandEvent& event );
    void SetInitCmp( wxCommandEvent& event );
    void addFieldButtonHandler( wxCommandEvent& event );
    void deleteFieldButtonHandler( wxCommandEvent& event );
    void moveUpButtonHandler( wxCommandEvent& event );
    void showButtonHandler( wxCommandEvent& event );
    void OnTestChipName( wxCommandEvent& event );
    void OnSelectChipName( wxCommandEvent& event );

    SCH_FIELD* findField( const wxString& aFieldName );

    /**
     * Function updateDisplay
     * update the listbox showing fields, according to the fields texts
     * must be called after a text change in fields, if this change is not an edition
     */
    void updateDisplay( )
    {
        for( unsigned ii = FIELD1;  ii<m_FieldsBuf.size(); ii++ )
            setRowItem( ii, m_FieldsBuf[ii] );
    }
};


int DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_SelectedRow;


void SCH_EDIT_FRAME::EditComponent( SCH_COMPONENT* aComponent )
{
    wxCHECK_RET( aComponent != NULL && aComponent->Type() == SCH_COMPONENT_T,
                 wxT( "Invalid component object pointer.  Bad Programmer!" ) );

    m_canvas->SetIgnoreMouseEvents( true );

    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC* dlg = new DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( this );

    dlg->InitBuffers( aComponent );

    // make sure the chipnameTextCtrl is wide enough to hold any unusually long chip names:
    EnsureTextCtrlWidth( dlg->chipnameTextCtrl );

    // This dialog itself subsequently can invoke a KIWAY_PLAYER as a quasimodal
    // frame. Therefore this dialog as a modal frame parent, MUST be run under
    // quasimodal mode for the quasimodal frame support to work.  So don't use
    // the QUASIMODAL macros here.
    int ret = dlg->ShowQuasiModal();

    m_canvas->SetIgnoreMouseEvents( false );
    m_canvas->MoveCursorToCrossHair();
    dlg->Destroy();

    if( ret == wxID_OK )
        GetCanvas()->Refresh();
}


DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( wxWindow* aParent ) :
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP( aParent )
{
    m_parent = (SCH_EDIT_FRAME*) aParent;

    m_cmp = NULL;
    m_part = NULL;
    m_skipCopyFromPanel = false;

    wxListItem columnLabel;

    columnLabel.SetImage( -1 );

    columnLabel.SetText( _( "Name" ) );
    fieldListCtrl->InsertColumn( 0, columnLabel );

    columnLabel.SetText( _( "Value" ) );
    fieldListCtrl->InsertColumn( 1, columnLabel );

    m_staticTextUnitSize->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_staticTextUnitPosX->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_staticTextUnitPosY->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );

    wxToolTip::Enable( true );

    GetSizer()->SetSizeHints( this );
    Center();

    stdDialogButtonSizerOK->SetDefault();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnListItemDeselected( wxListEvent& event )
{
    if( !m_skipCopyFromPanel )
    {
        if( !copyPanelToSelectedField() )
            event.Skip();   // do not go to the next row
    }
}

void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnTestChipName( wxCommandEvent& event )
{
    wxString partname = chipnameTextCtrl->GetValue();
    LIB_PART* entry = Prj().SchLibs()->FindLibPart( partname );

    wxString msg;

    if( entry )
    {
        msg.Printf( _( "Component '%s' found in library '%s'" ),
                    GetChars( partname ), GetChars( entry->GetLibraryName() ) );
        wxMessageBox( msg );
        return;
    }

    msg.Printf( _( "Component '%s' not found in any library" ), GetChars( partname ) );

    // Try to find components which have a name "near" the current chip name,
    // i.e. the same name when the comparison is case insensitive.
    // Could be helpful for old designs when lower cases and upper case were
    // equivalent.
    std::vector<LIB_ALIAS*> candidates;
    Prj().SchLibs()->FindLibraryNearEntries( candidates, partname );

    if( candidates.size() == 0 )
    {
        wxMessageBox( msg );
        return;
    }

    // Some candidates are found. Show them:
    msg << wxT("\n") << _( "However, some candidates are found:" );

    // add candidate names:
    for( unsigned ii = 0; ii < candidates.size(); ii++ )
    {
        msg << wxT("\n") <<
            wxString::Format( _( "'%s' found in library '%s'" ),
                              GetChars( candidates[ii]->GetName() ),
                              GetChars( candidates[ii]->GetLibraryName() ) );
    }

    wxMessageBox( msg );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnSelectChipName( wxCommandEvent& event )
{
    wxArrayString dummy;
    int dummyunit = 1;
    wxString chipname = m_parent->SelectComponentFromLibrary( NULL, dummy, dummyunit,
                                                              true, NULL, NULL );
    if( chipname.IsEmpty() )
        return;

    chipnameTextCtrl->SetValue( chipname );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnListItemSelected( wxListEvent& event )
{
    DBG( printf( "OnListItemSelected()\n" ); )

    // remember the selected row, statically
    s_SelectedRow = event.GetIndex();

    copySelectedFieldToPanel();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnCloseDialog( wxCloseEvent& event )
{
    // On wxWidgets 2.8, and on Linux, calling EndQuasiModal here is mandatory
    // Otherwise, the main event loop is never restored, and Eeschema does not
    // respond to any event, because the DIALOG_SHIM destructor is never called.
    // On wxWidgets 3.0, or on Windows, the DIALOG_SHIM destructor is called,
    // and calls EndQuasiModal.
    // therefore calling EndQuasiModal here is not always mandatory but it creates no issues
    EndQuasiModal( wxID_CANCEL );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnCancelButtonClick( wxCommandEvent& event )
{
    EndQuasiModal( wxID_CANCEL );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copyPanelToOptions()
{
    wxString newname = chipnameTextCtrl->GetValue();

    // Save current flags which could be modified by next change settings
    STATUS_FLAGS flags = m_cmp->GetFlags();

    newname.Replace( wxT( " " ), wxT( "_" ) );

    if( newname.IsEmpty() )
    {
        DisplayError( NULL, _( "No Component Name!" ) );
    }
    else if( Cmp_KEEPCASE( newname, m_cmp->m_part_name ) )
    {
        PART_LIBS* libs = Prj().SchLibs();

        if( libs->FindLibraryEntry( newname ) == NULL )
        {
            wxString msg = wxString::Format( _(
                "Component '%s' not found!" ),  GetChars( newname ) );
            DisplayError( this, msg );
        }
        else    // Change component from lib!
        {
            m_cmp->SetPartName( newname, libs );
        }
    }

    // For components with multiple shapes (De Morgan representation) Set the selected shape:
    if( convertCheckBox->IsEnabled() )
    {
        m_cmp->SetConvert( convertCheckBox->GetValue() ? 2 : 1 );
    }

    //Set the part selection in multiple part per package
    if( m_cmp->GetUnit() )
    {
        int unit_selection = unitChoice->GetCurrentSelection() + 1;

        m_cmp->SetUnitSelection( &m_parent->GetCurrentSheet(), unit_selection );
        m_cmp->SetUnit( unit_selection );
    }

    switch( orientationRadioBox->GetSelection() )
    {
    case 0:
        m_cmp->SetOrientation( CMP_ORIENT_0 );
        break;

    case 1:
        m_cmp->SetOrientation( CMP_ORIENT_90 );
        break;

    case 2:
        m_cmp->SetOrientation( CMP_ORIENT_180 );
        break;

    case 3:
        m_cmp->SetOrientation( CMP_ORIENT_270 );
        break;
    }

    int mirror = mirrorRadioBox->GetSelection();

    switch( mirror )
    {
    case 0:
        break;

    case 1:
        m_cmp->SetOrientation( CMP_MIRROR_X );
        break;

    case 2:
        m_cmp->SetOrientation( CMP_MIRROR_Y );
        break;
    }

    // Restore m_Flag modified by SetUnit() and other change settings
    m_cmp->ClearFlags();
    m_cmp->SetFlags( flags );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnOKButtonClick( wxCommandEvent& event )
{
    bool removeRemainingFields = false;

    if( !copyPanelToSelectedField() )
        return;

    if( ! SCH_COMPONENT::IsReferenceStringValid( m_FieldsBuf[REFERENCE].GetText() ) )
    {
        DisplayError( NULL, _( "Illegal reference. A reference must start with a letter" ) );
        return;
    }

    // save old cmp in undo list if not already in edit, or moving ...
    // or the component to be edited is part of a block
    if( m_cmp->m_Flags == 0
      || m_parent->GetScreen()->m_BlockLocate.GetState() != STATE_NO_BLOCK )
        m_parent->SaveCopyInUndoList( m_cmp, UR_CHANGED );

    copyPanelToOptions();

    // Delete any fields with no name before we copy all of m_FieldsBuf back into the component.
    for( unsigned i = MANDATORY_FIELDS;  i<m_FieldsBuf.size(); )
    {
        if( m_FieldsBuf[i].GetName( false ).IsEmpty() || m_FieldsBuf[i].GetText().IsEmpty() )
        {
            // If a field has no value and is not it the field template list, warn the user
            // that it will be remove from the component.  This gives the user a chance to
            // correct the problem before removing the undefined fields.  It should also
            // resolve most of the bug reports and questions regarding missing fields.
            if( !m_FieldsBuf[i].GetName( false ).IsEmpty() && m_FieldsBuf[i].GetText().IsEmpty()
                && !m_parent->GetTemplates().HasFieldName( m_FieldsBuf[i].GetName( false ) )
                && !removeRemainingFields )
            {
                wxString msg = wxString::Format(
                    _( "The field name <%s> does not have a value and is not defined in "
                       "the field template list.  Empty field values are invalid an will "
                       "be removed from the component.  Do you wish to remove this and "
                       "all remaining undefined fields?" ),
                    GetChars( m_FieldsBuf[i].GetName( false ) )
                    );

                wxMessageDialog dlg( this, msg, _( "Remove Fields" ), wxYES_NO | wxNO_DEFAULT );

                if( dlg.ShowModal() == wxID_NO )
                    return;

                removeRemainingFields = true;
            }

            m_FieldsBuf.erase( m_FieldsBuf.begin() + i );
            continue;
        }

        ++i;
    }

    // change all field positions from relative to absolute
    for( unsigned i = 0;  i<m_FieldsBuf.size();  ++i )
    {
        m_FieldsBuf[i].SetTextPosition( m_FieldsBuf[i].GetTextPosition() + m_cmp->m_Pos );
    }

    LIB_PART* entry = Prj().SchLibs()->FindLibPart( m_cmp->m_part_name );

    if( entry && entry->IsPower() )
        m_FieldsBuf[VALUE].SetText( m_cmp->m_part_name );

    // copy all the fields back, and change the length of m_Fields.
    m_cmp->SetFields( m_FieldsBuf );

    // Reference has a specific initialization, depending on the current active sheet
    // because for a given component, in a complex hierarchy, there are more than one
    // reference.
    m_cmp->SetRef( &m_parent->GetCurrentSheet(), m_FieldsBuf[REFERENCE].GetText() );

    m_parent->OnModify();
    m_parent->GetScreen()->TestDanglingEnds();

    EndQuasiModal( wxID_OK );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::addFieldButtonHandler( wxCommandEvent& event )
{
    // in case m_FieldsBuf[REFERENCE].m_Orient has changed on screen only, grab
    // screen contents.
    if( !copyPanelToSelectedField() )
        return;

    unsigned  fieldNdx = m_FieldsBuf.size();

    SCH_FIELD blank( wxPoint(), fieldNdx, m_cmp );

    blank.SetOrientation( m_FieldsBuf[REFERENCE].GetOrientation() );

    m_FieldsBuf.push_back( blank );
    m_FieldsBuf[fieldNdx].SetName( TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldNdx ) );

    m_skipCopyFromPanel = true;
    setRowItem( fieldNdx, m_FieldsBuf[fieldNdx] );

    setSelectedFieldNdx( fieldNdx );
    m_skipCopyFromPanel = false;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::deleteFieldButtonHandler( wxCommandEvent& event )
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )    // traps the -1 case too
        return;

    if( fieldNdx < MANDATORY_FIELDS )
    {
        wxBell();
        return;
    }

    m_skipCopyFromPanel = true;
    m_FieldsBuf.erase( m_FieldsBuf.begin() + fieldNdx );
    fieldListCtrl->DeleteItem( fieldNdx );

    if( fieldNdx >= m_FieldsBuf.size() )
        --fieldNdx;

    updateDisplay();

    setSelectedFieldNdx( fieldNdx );
    m_skipCopyFromPanel = false;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::showButtonHandler( wxCommandEvent& event )
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx == DATASHEET )
    {
        wxString datasheet_uri = fieldValueTextCtrl->GetValue();
        ::wxLaunchDefaultBrowser( datasheet_uri );
    }
    else if( fieldNdx == FOOTPRINT )
    {
        // pick a footprint using the footprint picker.
        wxString fpid;

        KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true );

        if( frame->ShowModal( &fpid, this ) )
        {
            // DBG( printf( "%s: %s\n", __func__, TO_UTF8( fpid ) ); )
            fieldValueTextCtrl->SetValue( fpid );
        }

        frame->Destroy();
    }
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::moveUpButtonHandler( wxCommandEvent& event )
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )    // traps the -1 case too
        return;

    if( fieldNdx <= MANDATORY_FIELDS )
    {
        wxBell();
        return;
    }

    if( !copyPanelToSelectedField() )
        return;

    // swap the fieldNdx field with the one before it, in both the vector
    // and in the fieldListCtrl
    SCH_FIELD tmp = m_FieldsBuf[fieldNdx - 1];

    DBG( printf( "tmp.m_Text=\"%s\" tmp.m_Name=\"%s\"\n",
                 TO_UTF8( tmp.GetText() ), TO_UTF8( tmp.GetName( false ) ) ); )

    m_FieldsBuf[fieldNdx - 1] = m_FieldsBuf[fieldNdx];
    setRowItem( fieldNdx - 1, m_FieldsBuf[fieldNdx] );

    m_FieldsBuf[fieldNdx] = tmp;
    setRowItem( fieldNdx, tmp );

    updateDisplay();

    m_skipCopyFromPanel = true;
    setSelectedFieldNdx( fieldNdx - 1 );
    m_skipCopyFromPanel = false;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::setSelectedFieldNdx( int aFieldNdx )
{
    /* deselect old selection, but I think this is done by single selection
     * flag within fieldListCtrl.
     * fieldListCtrl->SetItemState( s_SelectedRow, 0,
     *                              wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
     */

    if( aFieldNdx >= (int) m_FieldsBuf.size() )
        aFieldNdx = m_FieldsBuf.size() - 1;

    if( aFieldNdx < 0 )
        aFieldNdx = 0;

    fieldListCtrl->SetItemState( aFieldNdx, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
    fieldListCtrl->EnsureVisible( aFieldNdx );

    s_SelectedRow = aFieldNdx;
}


int DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::getSelectedFieldNdx()
{
    return s_SelectedRow;
}


SCH_FIELD* DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::findField( const wxString& aFieldName )
{
    for( unsigned i=0;  i<m_FieldsBuf.size();  ++i )
    {
        if( aFieldName == m_FieldsBuf[i].GetName( false ) )
            return &m_FieldsBuf[i];
    }

    return NULL;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::InitBuffers( SCH_COMPONENT* aComponent )
{
    m_cmp = aComponent;

    /*  We have 3 component related field lists to be aware of: 1) UI
        presentation, 2) fields in component ram copy, and 3) fields recorded
        with component on disk. m_FieldsBuf is the list of UI fields, and this
        list is not the same as the list which is in the component, which is
        also not the same as the list on disk. All 3 lists are potentially
        different. In the UI we choose to preserve the order of the first
        MANDATORY_FIELDS which are sometimes called fixed fields. Then we append
        the template fieldnames in the exact same order as the template
        fieldname editor shows them. Then we append any user defined fieldnames
        which came from the component.
    */

    m_part = Prj().SchLibs()->FindLibPart( m_cmp->m_part_name );

#if 0 && defined(DEBUG)
    for( int i = 0;  i<aComponent->GetFieldCount();  ++i )
    {
        printf( "Orig[%d] (x=%d, y=%d)\n", i, aComponent->m_Fields[i].GetTextPosition().x,
                aComponent->m_Fields[i].GetTextPosition().y );
    }

#endif

    // When this code was written, all field constructors ensure that the fixed fields
    // are all present within a component.  So we can knowingly copy them over
    // in the normal order.  Copy only the fixed fields at first.
    // Please do not break the field constructors.

    m_FieldsBuf.clear();

    for( int i=0;  i<MANDATORY_FIELDS;  ++i )
    {
        m_FieldsBuf.push_back(  aComponent->m_Fields[i] );

        // make the editable field position relative to the component
        m_FieldsBuf[i].SetTextPosition( m_FieldsBuf[i].GetTextPosition() - m_cmp->m_Pos );
    }

    // Add template fieldnames:
    // Now copy in the template fields, in the order that they are present in the
    // template field editor UI.
    const TEMPLATE_FIELDNAMES& tfnames = m_parent->GetTemplateFieldNames();

    for( TEMPLATE_FIELDNAMES::const_iterator it = tfnames.begin();  it!=tfnames.end();  ++it )
    {
        // add a new field unconditionally to the UI only
        SCH_FIELD   fld( wxPoint(0,0), -1 /* id is a relic */, m_cmp, it->m_Name );

        // See if field by same name already exists in component.
        SCH_FIELD* schField = aComponent->FindField( it->m_Name );

        // If the field does not already exist in the component, then we
        // use defaults from the template fieldname, otherwise the original
        // values from the component will be set.
        if( !schField )
        {
            if( !it->m_Visible )
                fld.SetVisible( false );
            else
                fld.SetVisible( true );

            fld.SetText( it->m_Value );   // empty? ok too.
        }
        else
        {
            fld = *schField;

            // make the editable field position relative to the component
            fld.SetTextPosition( fld.GetTextPosition() - m_cmp->m_Pos );
        }

        m_FieldsBuf.push_back( fld );
    }

    // Lastly, append any original fields from the component which were not added
    // from the set of fixed fields nor from the set of template fields.
    for( unsigned i=MANDATORY_FIELDS;  i<aComponent->m_Fields.size();  ++i )
    {
        SCH_FIELD*  cmp = &aComponent->m_Fields[i];
        SCH_FIELD*  buf = findField( cmp->GetName( false ) );

        if( !buf )
        {
            int newNdx = m_FieldsBuf.size();
            m_FieldsBuf.push_back( *cmp );

            // make the editable field position relative to the component
            m_FieldsBuf[newNdx].SetTextPosition( m_FieldsBuf[newNdx].GetTextPosition() -
                                                 m_cmp->m_Pos );
        }
    }


#if 0 && defined(DEBUG)
    for( unsigned i = 0;  i<m_FieldsBuf.size();  ++i )
    {
        printf( "m_FieldsBuf[%d] (x=%-3d, y=%-3d) name:%s\n", i, m_FieldsBuf[i].m_Pos.x,
                m_FieldsBuf[i].m_Pos.y, TO_UTF8(m_FieldsBuf[i].GetName( false ) ) );
    }
#endif

    m_FieldsBuf[REFERENCE].SetText( m_cmp->GetRef( &m_parent->GetCurrentSheet() ) );

    for( unsigned i = 0;  i<m_FieldsBuf.size();  ++i )
    {
        setRowItem( i, m_FieldsBuf[i] );
    }

#if 0 && defined(DEBUG)
    for( unsigned i = 0;  i<m_FieldsBuf.size();  ++i )
    {
        printf( "after[%d] (x=%d, y=%d)\n", i, m_FieldsBuf[i].m_Pos.x,
                m_FieldsBuf[i].m_Pos.y );
    }

#endif

    copyOptionsToPanel();

    // disable some options inside the edit dialog
    // which can cause problems while dragging
    if( m_cmp->IsDragging() )
    {
        orientationRadioBox->Disable();
        mirrorRadioBox->Disable();
        chipnameTextCtrl->Disable();
    }

    // put focus on the list ctrl
    fieldListCtrl->SetFocus();

    // resume editing at the last row edited, last time dialog was up.
    setSelectedFieldNdx( s_SelectedRow );

    copySelectedFieldToPanel();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::setRowItem( int aFieldNdx, const SCH_FIELD& aField )
{
    wxASSERT( aFieldNdx >= 0 );

    // insert blanks if aFieldNdx is referencing a "yet to be defined" row
    while( aFieldNdx >= fieldListCtrl->GetItemCount() )
    {
        long ndx = fieldListCtrl->InsertItem( fieldListCtrl->GetItemCount(), wxEmptyString );

        wxASSERT( ndx >= 0 );

        fieldListCtrl->SetItem( ndx, 1, wxEmptyString );
    }

    fieldListCtrl->SetItem( aFieldNdx, 0, aField.GetName( false ) );
    fieldListCtrl->SetItem( aFieldNdx, 1, aField.GetText() );

    // recompute the column widths here, after setting texts
    fieldListCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    fieldListCtrl->SetColumnWidth( 1, wxLIST_AUTOSIZE );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copySelectedFieldToPanel()
{
    wxCHECK_RET( m_cmp != NULL, wxT( "Component pointer not initialized." ) );

    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )    // traps the -1 case too
        return;

    SCH_FIELD& field = m_FieldsBuf[fieldNdx];

    showCheckBox->SetValue( field.IsVisible() );

    rotateCheckBox->SetValue( field.GetOrientation() == TEXT_ORIENT_VERT );

    int style = 0;

    if( field.IsItalic() )
        style = 1;

    if( field.IsBold() )
        style |= 2;

    m_StyleRadioBox->SetSelection( style );

    // Select the right text justification
    if( field.GetHorizJustify() == GR_TEXT_HJUSTIFY_LEFT )
        m_FieldHJustifyCtrl->SetSelection( 0 );
    else if( field.GetHorizJustify() == GR_TEXT_HJUSTIFY_RIGHT )
        m_FieldHJustifyCtrl->SetSelection( 2 );
    else
        m_FieldHJustifyCtrl->SetSelection( 1 );

    if( field.GetVertJustify() == GR_TEXT_VJUSTIFY_BOTTOM )
        m_FieldVJustifyCtrl->SetSelection( 0 );
    else if( field.GetVertJustify() == GR_TEXT_VJUSTIFY_TOP )
        m_FieldVJustifyCtrl->SetSelection( 2 );
    else
        m_FieldVJustifyCtrl->SetSelection( 1 );


    fieldNameTextCtrl->SetValue( field.GetName( false ) );

    // the names of the fixed fields are not editable, others are.
    fieldNameTextCtrl->Enable(  fieldNdx >= MANDATORY_FIELDS );
    fieldNameTextCtrl->SetEditable( fieldNdx >= MANDATORY_FIELDS );

    // only user defined fields may be moved, and not the top most user defined
    // field since it would be moving up into the fixed fields, > not >=
    moveUpButton->Enable( fieldNdx > MANDATORY_FIELDS );

    // may only delete user defined fields
    deleteFieldButton->Enable( fieldNdx >= MANDATORY_FIELDS );

    fieldValueTextCtrl->SetValue( field.GetText() );

    m_show_datasheet_button->Enable( fieldNdx == DATASHEET || fieldNdx == FOOTPRINT );

    if( fieldNdx == DATASHEET )
        m_show_datasheet_button->SetLabel( _( "Show in Browser" ) );
    else if( fieldNdx == FOOTPRINT )
        m_show_datasheet_button->SetLabel( _( "Assign Footprint" ) );
    else
        m_show_datasheet_button->SetLabel( wxEmptyString );

    // For power symbols, the value is NOR editable, because value and pin
    // name must be same and can be edited only in library editor
    if( fieldNdx == VALUE && m_part && m_part->IsPower() )
        fieldValueTextCtrl->Enable( false );
    else
        fieldValueTextCtrl->Enable( true );

    textSizeTextCtrl->SetValue( EDA_GRAPHIC_TEXT_CTRL::FormatSize( g_UserUnit, field.GetSize().x ) );

    wxPoint coord = field.GetTextPosition();
    wxPoint zero  = -m_cmp->m_Pos;  // relative zero

    // If the field value is empty and the position is at relative zero, we
    // set the initial position as a small offset from the ref field, and
    // orient it the same as the ref field.  That is likely to put it at least
    // close to the desired position.
    if( coord == zero && field.GetText().IsEmpty() )
    {
        rotateCheckBox->SetValue( m_FieldsBuf[REFERENCE].GetOrientation() == TEXT_ORIENT_VERT );

        coord.x = m_FieldsBuf[REFERENCE].GetTextPosition().x
            + ( fieldNdx - MANDATORY_FIELDS + 1 ) * 100;

        coord.y = m_FieldsBuf[REFERENCE].GetTextPosition().y
            + ( fieldNdx - MANDATORY_FIELDS + 1 ) * 100;

        // coord can compute negative if field is < MANDATORY_FIELDS, e.g. FOOTPRINT.
        // That is ok, we basically don't want all the new empty fields on
        // top of each other.
    }

    wxString coordText = StringFromValue( g_UserUnit, coord.x );
    posXTextCtrl->SetValue( coordText );

    coordText = StringFromValue( g_UserUnit, coord.y );
    posYTextCtrl->SetValue( coordText );
}


bool DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copyPanelToSelectedField()
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )        // traps the -1 case too
        return true;

    SCH_FIELD& field = m_FieldsBuf[fieldNdx];

    field.SetVisible( showCheckBox->GetValue() );

    if( rotateCheckBox->GetValue() )
        field.SetOrientation( TEXT_ORIENT_VERT );
    else
        field.SetOrientation( TEXT_ORIENT_HORIZ );

    rotateCheckBox->SetValue( field.GetOrientation() == TEXT_ORIENT_VERT );

    // Copy the text justification
    static const EDA_TEXT_HJUSTIFY_T hjustify[] = {
        GR_TEXT_HJUSTIFY_LEFT,
        GR_TEXT_HJUSTIFY_CENTER,
        GR_TEXT_HJUSTIFY_RIGHT
    };

    static const EDA_TEXT_VJUSTIFY_T vjustify[] = {
        GR_TEXT_VJUSTIFY_BOTTOM,
        GR_TEXT_VJUSTIFY_CENTER,
        GR_TEXT_VJUSTIFY_TOP
    };

    field.SetHorizJustify( hjustify[m_FieldHJustifyCtrl->GetSelection()] );
    field.SetVertJustify( vjustify[m_FieldVJustifyCtrl->GetSelection()] );

    field.SetName( fieldNameTextCtrl->GetValue() );

    /* Void fields texts for REFERENCE and VALUE (value is the name of the
     * component in lib ! ) are not allowed
     * change them only for a new non void value
     * When void, usually netlists are broken
     */
    if( !fieldValueTextCtrl->GetValue().IsEmpty() || fieldNdx > VALUE )
        field.SetText( fieldValueTextCtrl->GetValue() );

    setRowItem( fieldNdx, field );  // update fieldListCtrl

    int tmp = EDA_GRAPHIC_TEXT_CTRL::ParseSize( textSizeTextCtrl->GetValue(), g_UserUnit );
    field.SetSize( wxSize( tmp, tmp ) );
    int style = m_StyleRadioBox->GetSelection();

    field.SetItalic( (style & 1 ) != 0 );
    field.SetBold( (style & 2 ) != 0 );

    wxPoint pos;
    pos.x = ValueFromString( g_UserUnit, posXTextCtrl->GetValue() );
    pos.y = ValueFromString( g_UserUnit, posYTextCtrl->GetValue() );
    field.SetTextPosition( pos );

    return true;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copyOptionsToPanel()
{
    // Remove non existing choices (choiceCount must be <= number for parts)
    int unitcount = m_part ? m_part->GetUnitCount() : 1;

    if( unitcount < 1 )
        unitcount = 1;

    unitChoice->Clear();

    for( int ii = 1; ii <= unitcount; ii++ )
    {
        unitChoice->Append( LIB_PART::SubReference(  ii, false ) );
    }

    // For components with multiple parts per package, set the unit selection
    if( m_cmp->GetUnit() <= (int)unitChoice->GetCount() )
        unitChoice->SetSelection( m_cmp->GetUnit() - 1 );

    // Disable unit selection if only one unit exists:
    if( m_cmp->GetUnitCount() <= 1 )
    {
        unitChoice->Enable( false );
        unitsInterchageableLabel->Show( false );
        unitsInterchageableText->Show( false );
    }
    else
    {
        // Show the "Units are not interchangeable" message option?
        if( !m_part || !m_part->UnitsLocked() )
            unitsInterchageableLabel->SetLabel( _( "Yes" ) );
        else
            unitsInterchageableLabel->SetLabel( _( "No" ) );
    }

    int orientation = m_cmp->GetOrientation() & ~( CMP_MIRROR_X | CMP_MIRROR_Y );

    if( orientation == CMP_ORIENT_90 )
        orientationRadioBox->SetSelection( 1 );
    else if( orientation == CMP_ORIENT_180 )
        orientationRadioBox->SetSelection( 2 );
    else if( orientation == CMP_ORIENT_270 )
        orientationRadioBox->SetSelection( 3 );
    else
        orientationRadioBox->SetSelection( 0 );

    int mirror = m_cmp->GetOrientation() & ( CMP_MIRROR_X | CMP_MIRROR_Y );

    if( mirror == CMP_MIRROR_X )
    {
        mirrorRadioBox->SetSelection( 1 );
        DBG( printf( "mirror=X,1\n" ); )
    }
    else if( mirror == CMP_MIRROR_Y )
    {
        mirrorRadioBox->SetSelection( 2 );
        DBG( printf( "mirror=Y,2\n" ); )
    }
    else
        mirrorRadioBox->SetSelection( 0 );

    // Activate/Desactivate the normal/convert option ? (activated only if
    // the component has more than one shape)
    if( m_cmp->GetConvert() > 1 )
        convertCheckBox->SetValue( true );

    if( m_part == NULL || !m_part->HasConversion() )
        convertCheckBox->Enable( false );

    // Set the component's library name.
    chipnameTextCtrl->SetValue( m_cmp->m_part_name );

    // Set the component's unique ID time stamp.
    m_textCtrlTimeStamp->SetValue( wxString::Format( wxT( "%8.8lX" ),
                                   (unsigned long) m_cmp->GetTimeStamp() ) );
}


#include <kicad_device_context.h>


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::SetInitCmp( wxCommandEvent& event )
{
    if( !m_cmp )
        return;

    if( LIB_PART* part = Prj().SchLibs()->FindLibPart( m_cmp->m_part_name ) )
    {
        // save old cmp in undo list if not already in edit, or moving ...
        if( m_cmp->m_Flags == 0 )
            m_parent->SaveCopyInUndoList( m_cmp, UR_CHANGED );

        INSTALL_UNBUFFERED_DC( dc, m_parent->GetCanvas() );
        m_cmp->Draw( m_parent->GetCanvas(), &dc, wxPoint( 0, 0 ), g_XorMode );

        // Initialize fixed field values to default values found in library
        // Note: the field texts are not modified because they are set in schematic,
        // the text from libraries is most of time a dummy text
        // Only VALUE, REFERENCE , FOOTPRINT and DATASHEET are re-initialized
        LIB_FIELD& refField = part->GetReferenceField();

        m_cmp->GetField( REFERENCE )->SetTextPosition( refField.GetTextPosition() + m_cmp->m_Pos );
        m_cmp->GetField( REFERENCE )->ImportValues( refField );

        LIB_FIELD& valField = part->GetValueField();

        m_cmp->GetField( VALUE )->SetTextPosition( valField.GetTextPosition() + m_cmp->m_Pos );
        m_cmp->GetField( VALUE )->ImportValues( valField );

        LIB_FIELD* field = part->GetField(FOOTPRINT);

        if( field && m_cmp->GetField( FOOTPRINT ) )
        {
            m_cmp->GetField( FOOTPRINT )->SetTextPosition( field->GetTextPosition() + m_cmp->m_Pos );
            m_cmp->GetField( FOOTPRINT )->ImportValues( *field );
        }

        field = part->GetField(DATASHEET);

        if( field && m_cmp->GetField( DATASHEET ) )
        {
            m_cmp->GetField( DATASHEET )->SetTextPosition( field->GetTextPosition() + m_cmp->m_Pos );
            m_cmp->GetField( DATASHEET )->ImportValues( *field );
        }

        m_cmp->SetOrientation( CMP_NORMAL );

        m_parent->OnModify();

        m_cmp->Draw( m_parent->GetCanvas(), &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

        EndQuasiModal( wxID_OK );
    }
}
