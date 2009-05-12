/*******************************************************************************/
/*	library editor: edition of fields of lib entries (components in libraries) */
/*******************************************************************************/

#include "fctsys.h"

#include <algorithm>

#include "common.h"
#include "confirm.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "dialog_edit_libentry_fields_in_lib_base.h"

// Local variables:
static int s_SelectedRow;

#define COLUMN_FIELD_NAME 0
#define COLUMN_TEXT 1

/*****************************************************************************************/
class DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB : public DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE
/*****************************************************************************************/
{
private:
    WinEDA_LibeditFrame*    m_Parent;
    EDA_LibComponentStruct* m_LibEntry;
    bool m_skipCopyFromPanel;

    /// a copy of the edited component's LibDrawFields
    std::vector <LibDrawField> m_FieldsBuf;

public:
    DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB( WinEDA_LibeditFrame*    aParent,
                                        EDA_LibComponentStruct* aLibEntry );
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
    void InitBuffers( void );

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
    void setRowItem( int aFieldNdx, const LibDrawField& aField );

    /** Function updateDisplay
     * update the listbox showing fields, according to the fields texts
     * must be called after a text change in fields, if this change is not an edition
     */
    void updateDisplay( )
    {
        for( unsigned ii = FIELD1;  ii<m_FieldsBuf.size(); ii++ )
            setRowItem( ii, m_FieldsBuf[ii] );
    }

    /** Function reinitializeFieldsIdAndDefaultNames
     * Calculates  the field id and default name, after deleting a field
     * or moving a field
    */
    void reinitializeFieldsIdAndDefaultNames();
};

/*****************************************************************/
void WinEDA_LibeditFrame::InstallFieldsEditorDialog( void )
/*****************************************************************/
{
    if( CurrentLibEntry == NULL )
        return;

    DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB* frame =
        new DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB( this, CurrentLibEntry );

    int abort = frame->ShowModal(); frame->Destroy();

    if( ! abort )
    {
        ReCreateHToolbar();
        Refresh();
    }

    DisplayLibInfos();
}


/***********************************************************************/
DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB(
    WinEDA_LibeditFrame*    aParent,
    EDA_LibComponentStruct* aLibEntry ) :
    DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB_BASE( aParent )
/***********************************************************************/
{
    m_Parent   = aParent;
    m_LibEntry = aLibEntry;
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

    wxString label = _( "Size" ) + ReturnUnitSymbol( g_UnitMetric );
    textSizeLabel->SetLabel( label );

    label  = _( "Pos " );
    label += _( "X" );
    label += ReturnUnitSymbol( g_UnitMetric );
    posXLabel->SetLabel( label );

    label  = _( "Pos " );
    label += _( "Y" );
    label += ReturnUnitSymbol( g_UnitMetric );
    posYLabel->SetLabel( label );

    InitBuffers();
    copySelectedFieldToPanel();

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
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

    /* A new name could be entered in VALUE field.
     *  Must not be an existing alias name in alias list box */
    wxString* newvalue = &m_FieldsBuf[VALUE].m_Text;
    for( unsigned ii = 0; ii < m_LibEntry->m_AliasList.GetCount(); ii += ALIAS_NEXT  )
    {
        wxString* libname = &(m_LibEntry->m_AliasList[ii + ALIAS_NAME]);
        if( newvalue->CmpNoCase( *libname ) == 0 )
        {
            wxString msg;
            msg.Printf(
                _(
                    "A new name is entered for this component\nAn alias %s already exists!\nCannot update this component" ),
                newvalue->GetData() );
            DisplayError( this, msg );
            return;
        }
    }

    /* save old cmp in undo list */
    m_Parent->SaveCopyInUndoList( m_LibEntry, IS_CHANGED );

    // delete any fields with no name
    for( unsigned i = FIELD1;  i<m_FieldsBuf.size(); )
    {
        if( m_FieldsBuf[i].m_Name.IsEmpty() )
        {
            m_FieldsBuf.erase( m_FieldsBuf.begin() + i );
            continue;
        }

        ++i;
    }

    // copy all the fields back, and change the length of m_Fields.
    m_LibEntry->SetFields( m_FieldsBuf );

    m_Parent->GetScreen()->SetModify();

    EndModal( 0 );
}


/******************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::reinitializeFieldsIdAndDefaultNames( )
/*****************************************************************************/
{
    for( unsigned new_id = FIELD1; new_id < m_FieldsBuf.size(); new_id++ )
    {
        unsigned old_id = m_FieldsBuf[new_id].m_FieldId;
        if ( old_id != new_id )
        {
            if ( m_FieldsBuf[new_id].m_Name == ReturnDefaultFieldName( old_id ) )
                 m_FieldsBuf[new_id].m_Name = ReturnDefaultFieldName( new_id );
            m_FieldsBuf[new_id].m_FieldId = new_id;
        }
    }
}


/**************************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::addFieldButtonHandler( wxCommandEvent& event )
/**************************************************************************************/
{
    // in case m_FieldsBuf[REFERENCE].m_Orient has changed on screen only, grab
    // screen contents.
    if( !copyPanelToSelectedField() )
        return;

    unsigned     fieldNdx = m_FieldsBuf.size();

    LibDrawField blank( fieldNdx );

    m_FieldsBuf.push_back( blank );
    m_FieldsBuf[fieldNdx].m_Name = ReturnDefaultFieldName(fieldNdx);

    setRowItem( fieldNdx, m_FieldsBuf[fieldNdx] );

    m_skipCopyFromPanel = true;
    setSelectedFieldNdx( fieldNdx );
    m_skipCopyFromPanel = false;
}


/*****************************************************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::deleteFieldButtonHandler( wxCommandEvent& event )
/*****************************************************************************************/
/* Delete a field.
 * Fields REFERENCE and VALUE are mandatory, and cannot be deleted.
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

        // Reinitialize fields IDs and default names:
        reinitializeFieldsIdAndDefaultNames();
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

    if( fieldNdx <= FIELD1 )
    {
        wxBell();
        return;
    }

    if( !copyPanelToSelectedField() )
        return;

    // swap the fieldNdx field with the one before it, in both the vector
    // and in the fieldListCtrl
    LibDrawField tmp = m_FieldsBuf[fieldNdx - 1];

    m_FieldsBuf[fieldNdx - 1] = m_FieldsBuf[fieldNdx];
    setRowItem( fieldNdx - 1, m_FieldsBuf[fieldNdx] );

    m_FieldsBuf[fieldNdx] = tmp;
    setRowItem( fieldNdx, tmp );

    // Reinitialize fields IDs and default names:
    reinitializeFieldsIdAndDefaultNames();
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


static bool SortFieldsById(const LibDrawField& item1, const LibDrawField& item2)
{
    return item1.m_FieldId < item2.m_FieldId;
}

/***********************************************************/
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::InitBuffers( void )
/***********************************************************/
{
    // copy all the fields to a work area
    m_FieldsBuf.reserve(NUMBER_OF_FIELDS);

    m_FieldsBuf.push_back( m_LibEntry->m_Prefix );
    m_FieldsBuf.push_back( m_LibEntry->m_Name );

    // Creates a working copy of fields
    for( LibDrawField* field = m_LibEntry->m_Fields; field != NULL; field = field->Next() )
        m_FieldsBuf.push_back( *field );

    // Display 12 fields (or more), and add missing fields
    LibDrawField blank( 2 );
    unsigned fcount = m_FieldsBuf.size();
    for( unsigned ii = 2; ii < NUMBER_OF_FIELDS; ii++ )
    {
        unsigned jj;
        for ( jj = 2; jj < fcount; jj ++ )
            if ( m_FieldsBuf[jj].m_FieldId == (int)ii )     // Field id already exists, ok.
                break;
        if ( jj < fcount ) continue;
        // Field id not found: add this field
        blank.m_FieldId = ii;
        m_FieldsBuf.push_back( blank );
    }

    m_FieldsBuf[VALUE].m_Name << wxT( "/" ) << _( "Chip Name" );

    // Sort files by field id, because they are not entered by id
    sort(m_FieldsBuf.begin(), m_FieldsBuf.end(), SortFieldsById);

    // Now, all fields with Id  0 to NUMBER_OF_FIELDS-1 exist
    // init default fields names
    for( int ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
    {
        if( m_FieldsBuf[ii].m_Name.IsEmpty() || ii < FIELD1 )
            m_FieldsBuf[ii].m_Name = ReturnDefaultFieldName( ii );
    }

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
void DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::setRowItem( int aFieldNdx, const LibDrawField& aField )
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

    fieldListCtrl->SetItem( aFieldNdx, COLUMN_FIELD_NAME, aField.m_Name );
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

    LibDrawField& field = m_FieldsBuf[fieldNdx];

    showCheckBox->SetValue( !(field.m_Attributs & TEXT_NO_VISIBLE) );

    rotateCheckBox->SetValue( field.m_Orient == TEXT_ORIENT_VERT );

    int style = 0;
    if( field.m_Italic )
        style = 1;
    if( field.m_Width > 1 )
        style |= 2;
    m_StyleRadioBox->SetSelection( style );

    // Copy the text justification
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

    fieldNameTextCtrl->SetValue( field.m_Name );

    // if fieldNdx == REFERENCE, VALUE, FOOTPRINT, or DATASHEET, then disable filed name editing
    fieldNameTextCtrl->Enable(  fieldNdx >= FIELD1 );
    fieldNameTextCtrl->SetEditable( fieldNdx >= FIELD1 );
    moveUpButton->Enable( fieldNdx >= FIELD1 );   // disable move up button for non moveable fields
    // if fieldNdx == REFERENCE, VALUE, then disable delete button
    deleteFieldButton->Enable( fieldNdx > VALUE );

    fieldValueTextCtrl->SetValue( field.m_Text );

    if( fieldNdx == VALUE && m_LibEntry && m_LibEntry->m_Options == ENTRY_POWER )
        fieldValueTextCtrl->Enable( FALSE );

    textSizeTextCtrl->SetValue(
        WinEDA_GraphicTextCtrl::FormatSize( EESCHEMA_INTERNAL_UNIT, g_UnitMetric, field.m_Size.x ) );

    wxPoint coord = field.m_Pos;
    wxPoint zero;

    // If the field value is empty and the position is at relative zero, we set the
    // initial position as a small offset from the ref field, and orient
    // it the same as the ref field.  That is likely to put it at least
    // close to the desired position.
    if( coord == zero && field.m_Text.IsEmpty() )
    {
        rotateCheckBox->SetValue( m_FieldsBuf[REFERENCE].m_Orient == TEXT_ORIENT_VERT );

        coord.x = m_FieldsBuf[REFERENCE].m_Pos.x + (fieldNdx - FIELD1 + 1) * 100;
        coord.y = m_FieldsBuf[REFERENCE].m_Pos.y + (fieldNdx - FIELD1 + 1) * 100;

        // coord can compute negative if field is < FIELD1, e.g. FOOTPRINT.
        // That is ok, we basically don't want all the new empty fields on
        // top of each other.
    }

    wxString coordText = ReturnStringFromValue( g_UnitMetric, coord.x, EESCHEMA_INTERNAL_UNIT );
    posXTextCtrl->SetValue( coordText );

    // Note: the Y axis for components in lib is from bottom to top
    // and the screen axis is top to bottom: we must change the y coord sign for editing
    NEGATE( coord.y );
    coordText = ReturnStringFromValue( g_UnitMetric, coord.y, EESCHEMA_INTERNAL_UNIT );
    posYTextCtrl->SetValue( coordText );
}


/*****************************************************************/
bool DIALOG_EDIT_LIBENTRY_FIELDS_IN_LIB::copyPanelToSelectedField()
/*****************************************************************/
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )        // traps the -1 case too
        return true;

    LibDrawField& field = m_FieldsBuf[fieldNdx];

    if( showCheckBox->GetValue() )
        field.m_Attributs &= ~TEXT_NO_VISIBLE;
    else
        field.m_Attributs |= TEXT_NO_VISIBLE;

    if( rotateCheckBox->GetValue() )
        field.m_Orient = TEXT_ORIENT_VERT;
    else
        field.m_Orient = TEXT_ORIENT_HORIZ;

    // Copy the text justification
    GRTextHorizJustifyType hjustify[3] = {
        GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_HJUSTIFY_CENTER,
        GR_TEXT_HJUSTIFY_RIGHT
    };
    GRTextVertJustifyType vjustify[3] = {
        GR_TEXT_VJUSTIFY_BOTTOM, GR_TEXT_VJUSTIFY_CENTER,
        GR_TEXT_VJUSTIFY_TOP
    };
    field.m_HJustify = hjustify[m_FieldHJustifyCtrl->GetSelection()];
    field.m_VJustify = vjustify[m_FieldVJustifyCtrl->GetSelection()];

    /* Void fields texts for REFERENCE and VALUE (value is the name of the component in lib ! ) are not allowed
     * change them only for a new non void value
     */
    if( !fieldValueTextCtrl->GetValue().IsEmpty() || fieldNdx > VALUE )
        field.m_Text = fieldValueTextCtrl->GetValue();

    field.m_Name = fieldNameTextCtrl->GetValue();

    setRowItem( fieldNdx, field );  // update fieldListCtrl

    field.m_Size.x = WinEDA_GraphicTextCtrl::ParseSize(
        textSizeTextCtrl->GetValue(), EESCHEMA_INTERNAL_UNIT, g_UnitMetric );
    field.m_Size.y = field.m_Size.x;

    int style = m_StyleRadioBox->GetSelection();
    if( (style & 1 ) != 0 )
        field.m_Italic = true;
    else
        field.m_Italic = false;

    if( (style & 2 ) != 0 )
        field.m_Width = field.m_Size.x / 4;
    else
        field.m_Width = 0;

    double value;

    posXTextCtrl->GetValue().ToDouble( &value );
    field.m_Pos.x = From_User_Unit( g_UnitMetric, value, EESCHEMA_INTERNAL_UNIT );

    posYTextCtrl->GetValue().ToDouble( &value );
    field.m_Pos.y = From_User_Unit( g_UnitMetric, value, EESCHEMA_INTERNAL_UNIT );

    // Note: the Y axis for components in lib is from bottom to top
    // and the screen axis is top to bottom: we must change the y coord sign for editing
    NEGATE( field.m_Pos.y );

    return true;
}
