/*******************************************************************************/
/*	library editor: edition of fields of lib entries (components in libraries) */
/*******************************************************************************/

#include <algorithm>

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "class_drawpanel.h"
#include "wxEeschemaStruct.h"
#include "id.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_library.h"
#include "sch_field.h"
#include "template_fieldnames.h"
#include "dialog_helpers.h"

#include "dialog_edit_libentry_fields_in_lib_base.h"

// Local variables:
static int s_SelectedRow;

#define COLUMN_FIELD_NAME   0
#define COLUMN_TEXT         1

/*****************************************************************************************/
class DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB : public DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE
/*****************************************************************************************/
{
private:
    LIB_EDIT_FRAME*    m_Parent;
    LIB_COMPONENT*     m_LibEntry;
    bool               m_skipCopyFromPanel;

    /// a copy of the edited component's LIB_FIELDs
    std::vector <LIB_FIELD> m_FieldsBuf;

public:
    DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB( LIB_EDIT_FRAME* aParent, LIB_COMPONENT* aLibEntry );
    ~DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB();

private:

    // Events handlers:
    void OnInitDialog( wxInitDialogEvent& event );

    void OnListItemDeselected( wxListEvent& event );
    void OnListItemSelected( wxListEvent& event );
    void addFieldButtonHandler( wxCommandEvent& event );
    void deleteFieldButtonHandler( wxCommandEvent& event );
    void moveUpButtonHandler( wxCommandEvent& event );
    void OnCancelButtonClick( wxCommandEvent& event );
    void OnOKButtonClick( wxCommandEvent& event );

    // internal functions:
    void setSelectedFieldNdx( int aFieldNdx );

    int  getSelectedFieldNdx();

    /**
     * Function InitBuffers
     * sets up to edit the given component.
     * @param aComponent The component to edit.
     */
    void InitBuffers();

    /**
     * Function findField
     * searches m_FieldsBuf and returns a LIB_FIELD with \a aFieldName or NULL if
     * not found.
     */
    LIB_FIELD* findField( const wxString& aFieldName );

    /**
     * Function copySelectedFieldToPanel
     * sets the values displayed on the panel according to
     * the currently selected field row
     */
    void copySelectedFieldToPanel();

    /**
     * Function copyPanelToSelectedField
     * copies the values displayed on the panel fields to the currently selected field
     * @return bool - true if all fields are OK, else false if the user has put
     *   bad data into a field, and this value can be used to deny a row change.
     */
    bool copyPanelToSelectedField();
    void setRowItem( int aFieldNdx, const LIB_FIELD& aField );

    /**
     * Function updateDisplay
     * update the listbox showing fields, according to the fields texts
     * must be called after a text change in fields, if this change is not an edition
     */
    void updateDisplay( )
    {
        for( unsigned ii = MANDATORY_FIELDS;  ii<m_FieldsBuf.size(); ii++ )
            setRowItem( ii, m_FieldsBuf[ii] );
    }
};


void LIB_EDIT_FRAME::InstallFieldsEditorDialog( wxCommandEvent& event )
{
    if( m_component == NULL )
        return;

    DrawPanel->EndMouseCapture( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor() );

    DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB dlg( this, m_component );

    int abort = dlg.ShowModal();

    if( abort )
        return;

    UpdateAliasSelectList();
    UpdatePartSelectList();
    DisplayLibInfos();
    Refresh();
}


/***********************************************************************/
DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB(
    LIB_EDIT_FRAME* aParent,
    LIB_COMPONENT*  aLibEntry ) :
    DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE( aParent )
/***********************************************************************/
{
    m_Parent   = aParent;
    m_LibEntry = aLibEntry;

    GetSizer()->SetSizeHints( this );
    Centre();
}


/***********************************************************************/
DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::~DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB()
/***********************************************************************/
{
}


/**********************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnInitDialog( wxInitDialogEvent& event )
/**********************************************************************************/
{
    m_skipCopyFromPanel = false;
    wxListItem columnLabel;

    columnLabel.SetImage( -1 );

    columnLabel.SetText( _( "Name" ) );
    fieldListCtrl->InsertColumn( COLUMN_FIELD_NAME, columnLabel );

    columnLabel.SetText( _( "Value" ) );
    fieldListCtrl->InsertColumn( COLUMN_TEXT, columnLabel );

    wxString label = _( "Size" ) + ReturnUnitSymbol( g_UserUnit );
    textSizeLabel->SetLabel( label );

    label  = _( "Pos " );
    label += _( "X" );
    label += ReturnUnitSymbol( g_UserUnit );
    posXLabel->SetLabel( label );

    label  = _( "Pos " );
    label += _( "Y" );
    label += ReturnUnitSymbol( g_UserUnit );
    posYLabel->SetLabel( label );

    InitBuffers();
    copySelectedFieldToPanel();

    stdDialogButtonSizerOK->SetDefault();
}


/**********************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnListItemDeselected( wxListEvent& event )
/**********************************************************************************/
{
    if( !m_skipCopyFromPanel )
    {
        if( !copyPanelToSelectedField() )
            event.Skip();   // do not go to the next row
    }
}


/**********************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnListItemSelected( wxListEvent& event )
/**********************************************************************************/
{
    // remember the selected row, statically
    s_SelectedRow = event.GetIndex();

    copySelectedFieldToPanel();
}


/***********************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnCancelButtonClick( wxCommandEvent& event )
/***********************************************************************************/
{
    EndModal( 1 );
}


/**********************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::OnOKButtonClick( wxCommandEvent& event )
/**********************************************************************************/
{
    if( !copyPanelToSelectedField() )
        return;

    /* Note: this code is now (2010-dec-04) not used, because the value field is no more editable
     * because changing the value is equivalent to create a new component or alias.
     * This is now handled in libedit main frame, and no more in this dialog
     * but this code is not removed, just in case
     */
    /* If a new name entered in the VALUE field, that it not an existing alias name
     * or root alias of the component */
    wxString newvalue = m_FieldsBuf[VALUE].m_Text;

    if( m_LibEntry->HasAlias( newvalue ) && !m_LibEntry->GetAlias( newvalue )->IsRoot() )
    {
        wxString msg;
        msg.Printf( _( "A new name is entered for this component\n\
An alias %s already exists!\nCannot update this component" ),
                    GetChars( newvalue ) );
        DisplayError( this, msg );
        return;
    }
    /* End unused code */

    /* save old cmp in undo list */
    m_Parent->SaveCopyInUndoList( m_LibEntry, IS_CHANGED );

    // delete any fields with no name or no value before we copy all of m_FieldsBuf
    // back into the component
    for( unsigned i = MANDATORY_FIELDS; i < m_FieldsBuf.size(); )
    {
        if( m_FieldsBuf[i].GetName().IsEmpty() || m_FieldsBuf[i].m_Text.IsEmpty() )
        {
            m_FieldsBuf.erase( m_FieldsBuf.begin() + i );
            continue;
        }

        ++i;
    }

#if defined(DEBUG)
    for( unsigned i=0;  i<m_FieldsBuf.size();  ++i )
    {
        printf( "save[%d].name:'%s' value:'%s'\n", i,
                TO_UTF8( m_FieldsBuf[i].GetName() ),
                TO_UTF8( m_FieldsBuf[i].m_Text ) );
    }
#endif

    // copy all the fields back, fully replacing any previous fields
    m_LibEntry->SetFields( m_FieldsBuf );

    // We need to keep the name and the value the same at the moment!
    SetName( m_LibEntry->GetValueField().m_Text );

    m_Parent->OnModify();

    EndModal( 0 );
}


void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::addFieldButtonHandler( wxCommandEvent& event )
{
    // in case m_FieldsBuf[REFERENCE].m_Orient has changed on screen only, grab
    // screen contents.
    if( !copyPanelToSelectedField() )
        return;

    unsigned fieldNdx = m_FieldsBuf.size();

    LIB_FIELD blank( fieldNdx );

    m_FieldsBuf.push_back( blank );
    m_FieldsBuf[fieldNdx].SetName( TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldNdx ) );

    setRowItem( fieldNdx, m_FieldsBuf[fieldNdx] );

    m_skipCopyFromPanel = true;
    setSelectedFieldNdx( fieldNdx );
    m_skipCopyFromPanel = false;
}


/*****************************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::deleteFieldButtonHandler( wxCommandEvent& event )
/*****************************************************************************************/
/* Delete a field.
 * MANDATORY_FIELDS cannot be deleted.
 * If a field is empty, it is removed.
 * if not empty, the text is removed.
 */
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )    // traps the -1 case too
        return;

    if( fieldNdx <= VALUE )
    {
        wxBell();
        return;
    }

    m_skipCopyFromPanel = true;

    if( m_FieldsBuf[fieldNdx].m_Text.IsEmpty() )
    {
        m_FieldsBuf.erase( m_FieldsBuf.begin() + fieldNdx );
        fieldListCtrl->DeleteItem( fieldNdx );

        if( fieldNdx >= m_FieldsBuf.size() )
            --fieldNdx;
    }
    else
    {
        m_FieldsBuf[fieldNdx].m_Text.Empty();
        copySelectedFieldToPanel();
    }

    updateDisplay( );

    setRowItem( fieldNdx, m_FieldsBuf[fieldNdx] );
    setSelectedFieldNdx( fieldNdx );
    m_skipCopyFromPanel = false;
}


/*************************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB:: moveUpButtonHandler( wxCommandEvent& event )
/*************************************************************************************/
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
    LIB_FIELD tmp = m_FieldsBuf[fieldNdx - 1];

    m_FieldsBuf[fieldNdx - 1] = m_FieldsBuf[fieldNdx];
    setRowItem( fieldNdx - 1, m_FieldsBuf[fieldNdx] );

    m_FieldsBuf[fieldNdx] = tmp;
    setRowItem( fieldNdx, tmp );

    updateDisplay( );

    m_skipCopyFromPanel = true;
    setSelectedFieldNdx( fieldNdx - 1 );
    m_skipCopyFromPanel = false;
}


/****************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::setSelectedFieldNdx( int aFieldNdx )
/****************************************************************************/
{
    /* deselect old selection, but I think this is done by single selection flag within fieldListCtrl
     *  fieldListCtrl->SetItemState( s_SelectedRow, 0, wxLIST_STATE_SELECTED|wxLIST_STATE_FOCUSED);
     */

    if( aFieldNdx >= (int) m_FieldsBuf.size() )
        aFieldNdx = m_FieldsBuf.size() - 1;

    if( aFieldNdx < 0 )
        aFieldNdx = 0;

    fieldListCtrl->SetItemState( aFieldNdx, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
    fieldListCtrl->EnsureVisible( aFieldNdx );

    s_SelectedRow = aFieldNdx;
}


int DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::getSelectedFieldNdx()
{
    return s_SelectedRow;
}


/**
 * Function findfield
 * searches a LIB_FIELD_LIST for aFieldName.
 */
static LIB_FIELD* findfield( const LIB_FIELDS& aList, const wxString& aFieldName )
{
    const LIB_FIELD*  field = NULL;

    for( unsigned i=0;  i<aList.size();  ++i )
    {
        if( aFieldName == aList[i].GetName() )
        {
            field = &aList[i];  // best to avoid casting here.
            break;
        }
    }
    return (LIB_FIELD*) field;  // remove const-ness last
}


LIB_FIELD* DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::findField( const wxString& aFieldName )
{
    for( unsigned i=0;  i<m_FieldsBuf.size();  ++i )
    {
        if( aFieldName == m_FieldsBuf[i].GetName() )
            return &m_FieldsBuf[i];
    }
    return NULL;
}


/***********************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::InitBuffers()
/***********************************************************/
{
    LIB_FIELDS cmpFields;

    m_LibEntry->GetFields( cmpFields );

#if defined(DEBUG)
    for( unsigned i=0; i<cmpFields.size();  ++i )
    {
        printf( "cmpFields[%d].name:%s\n", i, TO_UTF8( cmpFields[i].GetName() ) );
    }
#endif

    /*  We have 3 component related field lists to be aware of: 1) UI
        presentation (m_FieldsBuf), 2) fields in component ram copy, and 3)
        fields recorded with component on disk. m_FieldsBuf is the list of UI
        fields, and this list is not the same as the list which is in the
        component, which is also not the same as the list on disk. All 3 lists
        are potentially different. In the UI we choose to preserve the order of
        the first MANDATORY_FIELDS which are sometimes called fixed fields. Then
        we append the template fieldnames in the exact same order as the
        template fieldname editor shows them. Then we append any user defined
        fieldnames which came from the component, and user can modify it during
        editing, but cannot delete or move a fixed field.
    */

    m_FieldsBuf.clear();

    /*  When this code was written, all field constructors ensured that the
        MANDATORY_FIELDS are all present within a component (in ram only). So we can
        knowingly copy them over in the normal order. Copy only the fixed fields
        at first. Please do not break the field constructors.
    */

    // fixed fields:
    for( int i=0; i<MANDATORY_FIELDS; ++i )
    {
        D( printf( "add fixed:%s\n", TO_UTF8( cmpFields[i].GetName() ) ); )
        m_FieldsBuf.push_back( cmpFields[i] );
    }

    // Add template fieldnames:
    // Now copy in the template fields, in the order that they are present in the
    // template field editor UI.
    const TEMPLATE_FIELDNAMES& tfnames =
        ((SCH_EDIT_FRAME*)m_Parent->GetParent())->GetTemplateFieldNames();

    for( TEMPLATE_FIELDNAMES::const_iterator it = tfnames.begin();  it!=tfnames.end();  ++it )
    {
        // add a new field unconditionally to the UI only for this template fieldname

        // field id must not be in range 0 - MANDATORY_FIELDS, set before saving to disk
        LIB_FIELD   fld(-1);

        // See if field by same name already exists in component.
        LIB_FIELD* libField = findfield( cmpFields, it->m_Name );

        // If the field does not already exist in the component, then we
        // use defaults from the template fieldname, otherwise the original
        // values from the component will be set.
        if( !libField )
        {
            D( printf( "add template:%s\n", TO_UTF8( it->m_Name ) ); )

                fld.SetName( it->m_Name );
            fld.m_Text = it->m_Value;   // empty? ok too.

            if( !it->m_Visible )
                fld.m_Attributs |= TEXT_NO_VISIBLE;
            else
                fld.m_Attributs &= ~TEXT_NO_VISIBLE;
        }
        else
        {
            D( printf( "match template:%s\n", TO_UTF8( libField->GetName() ) ); )
            fld = *libField;    // copy values from component, m_Name too
        }

        m_FieldsBuf.push_back( fld );
    }

    // Lastly, append any original fields from the component which were not added
    // from the set of fixed fields nor from the set of template fields.
    for( unsigned i=MANDATORY_FIELDS;  i<cmpFields.size();  ++i )
    {
        LIB_FIELD*  cmp = &cmpFields[i];
        LIB_FIELD*  buf = findField( cmp->GetName() );

        if( !buf )
        {
            D( printf( "add cmp:%s\n", TO_UTF8( cmp->GetName() ) ); )
            m_FieldsBuf.push_back( *cmp );
        }
    }

    /*  field names have become more important than field ids, so we cannot
        mangle the names in the buffer, but can do so in the panel, see elsewhere.
    m_FieldsBuf[VALUE].m_Name << wxT( "/" ) << _( "Chip Name" );
    */

    for( unsigned ii = 0; ii < m_FieldsBuf.size();  ++ii )
    {
        setRowItem( ii, m_FieldsBuf[ii] );
    }

    // put focus on the list ctrl
    fieldListCtrl->SetFocus();

    // resume editing at the last row edited, last time dialog was up.
    if ( s_SelectedRow < (int) m_FieldsBuf.size() )
        s_SelectedRow = 0;

    setSelectedFieldNdx( s_SelectedRow );
}


/***********************************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::setRowItem( int aFieldNdx, const LIB_FIELD& aField )
/***********************************************************************************************/
{
    wxASSERT( aFieldNdx >= 0 );

    // insert blanks if aFieldNdx is referencing a "yet to be defined" row
    while( aFieldNdx >= fieldListCtrl->GetItemCount() )
    {
        long ndx = fieldListCtrl->InsertItem( fieldListCtrl->GetItemCount(), wxEmptyString );

        wxASSERT( ndx >= 0 );

        fieldListCtrl->SetItem( ndx, COLUMN_TEXT, wxEmptyString );
    }

    fieldListCtrl->SetItem( aFieldNdx, COLUMN_FIELD_NAME, aField.GetName() );
    fieldListCtrl->SetItem( aFieldNdx, COLUMN_TEXT, aField.m_Text );

    // recompute the column widths here, after setting texts
    fieldListCtrl->SetColumnWidth( COLUMN_FIELD_NAME, wxLIST_AUTOSIZE );
    fieldListCtrl->SetColumnWidth( COLUMN_TEXT, wxLIST_AUTOSIZE );
}


/****************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::copySelectedFieldToPanel()
/****************************************************************/
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )    // traps the -1 case too
        return;

    LIB_FIELD& field = m_FieldsBuf[fieldNdx];

    showCheckBox->SetValue( !(field.m_Attributs & TEXT_NO_VISIBLE) );

    rotateCheckBox->SetValue( field.m_Orient == TEXT_ORIENT_VERT );

    int style = 0;
    if( field.m_Italic )
        style = 1;

    if( field.m_Bold )
        style |= 2;

    m_StyleRadioBox->SetSelection( style );

    // Select the right text justification
    if( field.m_HJustify == GR_TEXT_HJUSTIFY_LEFT )
        m_FieldHJustifyCtrl->SetSelection(0);
    else if( field.m_HJustify == GR_TEXT_HJUSTIFY_RIGHT )
        m_FieldHJustifyCtrl->SetSelection(2);
    else
        m_FieldHJustifyCtrl->SetSelection(1);

    if( field.m_VJustify == GR_TEXT_VJUSTIFY_BOTTOM )
        m_FieldVJustifyCtrl->SetSelection(0);
    else if( field.m_VJustify == GR_TEXT_VJUSTIFY_TOP )
        m_FieldVJustifyCtrl->SetSelection(2);
    else
        m_FieldVJustifyCtrl->SetSelection(1);


    // Field names have become more important than field ids, so we cannot
    // mangle the names in the buffer but we can do so in the panel.
    if( field.GetId() == VALUE )
    {
        // This field is the lib name and the default value when loading this component in
        // schematic.  The value is now not editable here (in this dialog) because changing
        // it is equivalent to create a new component or alias. This is handles in libedir,
        // not in this dialog.
        fieldNameTextCtrl->SetValue( field.GetName() + wxT( " / " ) + _( "Chip Name" ) );
        fieldValueTextCtrl->Enable( false );
    }
    else
    {
        fieldValueTextCtrl->Enable( true );
        fieldNameTextCtrl->SetValue( field.GetName() );
    }

    // if fieldNdx == REFERENCE, VALUE, FOOTPRINT, or DATASHEET, then disable field name editing
    fieldNameTextCtrl->Enable(  fieldNdx >= MANDATORY_FIELDS );
    fieldNameTextCtrl->SetEditable( fieldNdx >= MANDATORY_FIELDS );

    // only user defined fields may be moved, and not the top most user defined
    // field since it would be moving up into the fixed fields, > not >=
    moveUpButton->Enable( fieldNdx > MANDATORY_FIELDS );

    // if fieldNdx == REFERENCE, VALUE, then disable delete button
    deleteFieldButton->Enable( fieldNdx > VALUE );

    fieldValueTextCtrl->SetValue( field.m_Text );

    textSizeTextCtrl->SetValue(
        WinEDA_GraphicTextCtrl::FormatSize( EESCHEMA_INTERNAL_UNIT, g_UserUnit, field.m_Size.x ) );

    wxPoint coord = field.m_Pos;
    wxPoint zero;

    // If the field value is empty and the position is at relative zero, we set the
    // initial position as a small offset from the ref field, and orient
    // it the same as the ref field.  That is likely to put it at least
    // close to the desired position.
    if( coord == zero && field.m_Text.IsEmpty() )
    {
        rotateCheckBox->SetValue( m_FieldsBuf[REFERENCE].m_Orient == TEXT_ORIENT_VERT );

        coord.x = m_FieldsBuf[REFERENCE].m_Pos.x + (fieldNdx - MANDATORY_FIELDS + 1) * 100;
        coord.y = m_FieldsBuf[REFERENCE].m_Pos.y + (fieldNdx - MANDATORY_FIELDS + 1) * 100;

        // coord can compute negative if field is < MANDATORY_FIELDS, e.g. FOOTPRINT.
        // That is ok, we basically don't want all the new empty fields on
        // top of each other.
    }

    wxString coordText = ReturnStringFromValue( g_UserUnit, coord.x, EESCHEMA_INTERNAL_UNIT );
    posXTextCtrl->SetValue( coordText );

    // Note: the Y axis for components in lib is from bottom to top
    // and the screen axis is top to bottom: we must change the y coord sign for editing
    NEGATE( coord.y );
    coordText = ReturnStringFromValue( g_UserUnit, coord.y, EESCHEMA_INTERNAL_UNIT );
    posYTextCtrl->SetValue( coordText );
}


/*****************************************************************/
bool DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::copyPanelToSelectedField()
/*****************************************************************/
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )        // traps the -1 case too
        return true;

    LIB_FIELD& field = m_FieldsBuf[fieldNdx];

    if( showCheckBox->GetValue() )
        field.m_Attributs &= ~TEXT_NO_VISIBLE;
    else
        field.m_Attributs |= TEXT_NO_VISIBLE;

    if( rotateCheckBox->GetValue() )
        field.m_Orient = TEXT_ORIENT_VERT;
    else
        field.m_Orient = TEXT_ORIENT_HORIZ;

    // Copy the text justification
    static const GRTextHorizJustifyType hjustify[3] = {
        GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_HJUSTIFY_CENTER,
        GR_TEXT_HJUSTIFY_RIGHT
    };

    static const GRTextVertJustifyType vjustify[3] = {
        GR_TEXT_VJUSTIFY_BOTTOM, GR_TEXT_VJUSTIFY_CENTER,
        GR_TEXT_VJUSTIFY_TOP
    };

    field.m_HJustify = hjustify[m_FieldHJustifyCtrl->GetSelection()];
    field.m_VJustify = vjustify[m_FieldVJustifyCtrl->GetSelection()];

    // Blank/empty field texts for REFERENCE and VALUE are not allowed.
    // (Value is the name of the component in lib!)
    // Change them only if user provided a non blank value
    if( !fieldValueTextCtrl->GetValue().IsEmpty() || fieldNdx > VALUE )
        field.m_Text = fieldValueTextCtrl->GetValue();

    // FieldNameTextCtrl has a tricked value in it for VALUE index, do not copy it back.
    // It has the "Chip Name" appended.
    if( field.GetId() >= MANDATORY_FIELDS )
    {
        wxString name = fieldNameTextCtrl->GetValue();
        D( printf("name:%s\n", TO_UTF8( name ) ); )
        field.SetName( name );
    }

    D( printf("setname:%s\n", TO_UTF8( field.GetName() ) ); )

    setRowItem( fieldNdx, field );  // update fieldListCtrl

    field.m_Size.x = WinEDA_GraphicTextCtrl::ParseSize(
        textSizeTextCtrl->GetValue(), EESCHEMA_INTERNAL_UNIT, g_UserUnit );

    field.m_Size.y = field.m_Size.x;

    int style = m_StyleRadioBox->GetSelection();
    if( (style & 1 ) != 0 )
        field.m_Italic = true;
    else
        field.m_Italic = false;

    if( (style & 2 ) != 0 )
        field.m_Bold = true;
    else
        field.m_Bold = false;

    field.m_Pos.x = ReturnValueFromString( g_UserUnit, posXTextCtrl->GetValue(),
            EESCHEMA_INTERNAL_UNIT );
    field.m_Pos.y = ReturnValueFromString( g_UserUnit, posYTextCtrl->GetValue(),
            EESCHEMA_INTERNAL_UNIT );

    // Note: the Y axis for components in lib is from bottom to top
    // and the screen axis is top to bottom: we must change the y coord sign for editing
    NEGATE( field.m_Pos.y );

    return true;
}
