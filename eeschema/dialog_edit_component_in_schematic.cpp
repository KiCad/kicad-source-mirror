
#include <wx/tooltip.h>
#include <algorithm>

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"

#include "dialog_edit_component_in_schematic.h"


int DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_SelectedRow;


wxSize DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize = wxDefaultSize;


/**********************************************************************/
void InstallCmpeditFrame( WinEDA_SchematicFrame* parent, wxPoint& pos,
                          SCH_COMPONENT* aComponent )
/*********************************************************************/
{
    if( aComponent == NULL )    // Null component not accepted
        return;

    parent->DrawPanel->m_IgnoreMouseEvents = TRUE;
    if( aComponent->Type() != TYPE_SCH_COMPONENT )
    {
        DisplayError( parent,
                     wxT( "InstallCmpeditFrame() error: This item is not a component" ) );
    }
    else
    {
        DIALOG_EDIT_COMPONENT_IN_SCHEMATIC* dialog =
            new DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( parent );

        dialog->InitBuffers( aComponent );

        wxSize sizeNow = dialog->GetSize();

        // this relies on wxDefaultSize being -1,-1, be careful here.
        if(  sizeNow.GetWidth()  < DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize.GetWidth()
          || sizeNow.GetHeight() < DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize.GetHeight() )
        {
            dialog->SetSize( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize );
        }

        // make sure the chipnameTextCtrl is wide enough to hold any unusually long chip names:
        EnsureTextCtrlWidth( dialog->chipnameTextCtrl );

        dialog->ShowModal();

        // Some of the field values are long and are not always fully visible
        // because the window comes up too narrow.
        // Remember user's manual window resizing efforts here so it comes up wide enough next time.
        DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize = dialog->GetSize();

        dialog->Destroy();
    }

    parent->DrawPanel->MouseToCursorSchema();
    parent->DrawPanel->m_IgnoreMouseEvents = FALSE;
}


DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( wxWindow* parent ) :
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP( parent )
{
    m_Parent = (WinEDA_SchematicFrame*) parent;

    m_LibEntry = NULL;
    m_skipCopyFromPanel = false;

    wxListItem columnLabel;

    columnLabel.SetImage( -1 );

    columnLabel.SetText( _( "Name" ) );
    fieldListCtrl->InsertColumn( 0, columnLabel );

    columnLabel.SetText( _( "Value" ) );
    fieldListCtrl->InsertColumn( 1, columnLabel );

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

    copySelectedFieldToPanel();

    wxToolTip::Enable( true );
    
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }

}


/******************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::reinitializeFieldsIdAndDefaultNames( )
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


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnListItemDeselected( wxListEvent& event )
{
    D( printf( "OnListItemDeselected()\n" ); )

    if( !m_skipCopyFromPanel )
    {
        if( !copyPanelToSelectedField() )
            event.Skip();   // do not go to the next row
    }
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnListItemSelected( wxListEvent& event )
{
    D( printf( "OnListItemSelected()\n" ); )

    // remember the selected row, statically
    s_SelectedRow = event.GetIndex();

    copySelectedFieldToPanel();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnCancelButtonClick( wxCommandEvent& event )
{
    EndModal( 1 );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copyPanelToOptions()
{
    wxString newname = chipnameTextCtrl->GetValue();

    newname.MakeUpper();
    newname.Replace( wxT( " " ), wxT( "_" ) );

    if( newname.IsEmpty() )
        DisplayError( this, _( "No Component Name!" ) );

    else if( newname.CmpNoCase( m_Cmp->m_ChipName ) )
    {
        if( FindLibPart( newname.GetData(), wxEmptyString, FIND_ALIAS ) == NULL )
        {
            wxString message;
            message.Printf( _( "Component [%s] not found!" ), newname.GetData() );
            DisplayError( this, message );
        }
        else    // Change component from lib!
        {
            m_Cmp->m_ChipName = newname;
        }
    }

    // For components with multiple shapes (De Morgan representation) Set the selected shape:
    if( convertCheckBox->IsEnabled() )
    {
        m_Cmp->m_Convert = convertCheckBox->GetValue() ? 2 : 1;
    }

    //Set the part selection in multiple part per pakcage
    if( m_Cmp->m_Multi )
    {
        int unit_selection = unitChoice->GetCurrentSelection() + 1;
        m_Cmp->SetUnitSelection( m_Parent->GetSheet(), unit_selection );
        m_Cmp->m_Multi = unit_selection;
    }

    switch( orientationRadioBox->GetSelection() )
    {
    case 0:
        m_Cmp->SetRotationMiroir( CMP_ORIENT_0 );
        break;

    case 1:
        m_Cmp->SetRotationMiroir( CMP_ORIENT_90 );
        break;

    case 2:
        m_Cmp->SetRotationMiroir( CMP_ORIENT_180 );
        break;

    case 3:
        m_Cmp->SetRotationMiroir( CMP_ORIENT_270 );
        break;
    }

    int mirror = mirrorRadioBox->GetSelection();

    switch( mirror )
    {
    case 0:
        break;

    case 1:
        m_Cmp->SetRotationMiroir( CMP_MIROIR_X );
        break;

    case 2:
        m_Cmp->SetRotationMiroir( CMP_MIROIR_Y );
        break;
    }
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnOKButtonClick( wxCommandEvent& event )
{
    if( !copyPanelToSelectedField() )
        return;

    copyPanelToOptions();

    /* save old cmp in undo list if not already in edit, or moving ... */
    if( m_Cmp->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_Cmp, IS_CHANGED );

    // change all field positions from relative to absolute
    for( unsigned i = 0;  i<m_FieldsBuf.size();  ++i )
    {
        m_FieldsBuf[i].m_Pos += m_Cmp->m_Pos;
    }

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

    EDA_LibComponentStruct* entry = FindLibPart(
        m_Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

    if( entry &&  entry->m_Options == ENTRY_POWER )
        m_FieldsBuf[VALUE].m_Text = m_Cmp->m_ChipName;

    // copy all the fields back, and change the length of m_Fields.
    m_Cmp->SetFields( m_FieldsBuf );

    m_Parent->GetScreen()->SetModify();

    m_Parent->TestDanglingEnds( m_Parent->GetScreen()->EEDrawList, NULL );

    m_Parent->DrawPanel->Refresh( TRUE );

    EndModal( 0 );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::addFieldButtonHandler( wxCommandEvent& event )
{
    // in case m_FieldsBuf[REFERENCE].m_Orient has changed on screen only, grab
    // screen contents.
    if( !copyPanelToSelectedField() )
        return;

    unsigned      fieldNdx = m_FieldsBuf.size();

    SCH_CMP_FIELD blank( wxPoint(), fieldNdx, m_Cmp );

    blank.m_Orient = m_FieldsBuf[REFERENCE].m_Orient;

    m_FieldsBuf.push_back( blank );
    m_FieldsBuf[fieldNdx].m_Name = ReturnDefaultFieldName(fieldNdx);

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

    if( fieldNdx < FIELD1 )
    {
        wxBell();
        return;
    }

    m_skipCopyFromPanel = true;
    m_FieldsBuf.erase( m_FieldsBuf.begin() + fieldNdx );
    fieldListCtrl->DeleteItem( fieldNdx );

    if( fieldNdx >= m_FieldsBuf.size() )
        --fieldNdx;

    // Reinitialize fields IDs and default names:
    reinitializeFieldsIdAndDefaultNames();
    updateDisplay( );

    setSelectedFieldNdx( fieldNdx );
    m_skipCopyFromPanel = false;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC:: moveUpButtonHandler( wxCommandEvent& event )
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
    SCH_CMP_FIELD tmp = m_FieldsBuf[fieldNdx - 1];

    D( printf( "tmp.m_Text=\"%s\" tmp.m_Name=\"%s\"\n",
              CONV_TO_UTF8( tmp.m_Text ), CONV_TO_UTF8( tmp.m_Name ) ); )

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


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::setSelectedFieldNdx( int aFieldNdx )
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


int DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::getSelectedFieldNdx()
{
    return s_SelectedRow;
}



static bool SortFieldsById(const SCH_CMP_FIELD& item1, const SCH_CMP_FIELD& item2)
{
    return item1.m_FieldId < item2.m_FieldId;
}

/*******************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::InitBuffers( SCH_COMPONENT* aComponent )
/*******************************************************************************/
{
    m_Cmp = aComponent;

    m_LibEntry = FindLibPart( m_Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

#if 0 && defined(DEBUG)
    for( int i = 0;  i<aComponent->GetFieldCount();  ++i )
    {
        printf( "Orig[%d] (x=%d, y=%d)\n", i, aComponent->m_Fields[i].m_Pos.x,
                aComponent->m_Fields[i].m_Pos.y );
    }

#endif

    // copy all the fields to a work area
    m_FieldsBuf = aComponent->m_Fields;

    // Sort files by field id,if they are not entered by id
    sort(m_FieldsBuf.begin(), m_FieldsBuf.end(), SortFieldsById);

#if 0 && defined(DEBUG)
    for( unsigned i = 0;  i<m_FieldsBuf.size();  ++i )
    {
        printf( "m_FieldsBuf[%d] (x=%d, y=%d)\n", i, m_FieldsBuf[i].m_Pos.x,
                m_FieldsBuf[i].m_Pos.y );
    }

#endif

    m_FieldsBuf[REFERENCE].m_Text = m_Cmp->GetRef( m_Parent->GetSheet() );

    for( unsigned i = 0;  i<m_FieldsBuf.size();  ++i )
    {
        // make the editable field position relative to the component
        m_FieldsBuf[i].m_Pos -= m_Cmp->m_Pos;

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

    // put focus on the list ctrl
    fieldListCtrl->SetFocus();

    // resume editing at the last row edited, last time dialog was up.
    setSelectedFieldNdx( s_SelectedRow );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::setRowItem( int aFieldNdx, const SCH_CMP_FIELD& aField )
{
    wxASSERT( aFieldNdx >= 0 );

    // insert blanks if aFieldNdx is referencing a "yet to be defined" row
    while( aFieldNdx >= fieldListCtrl->GetItemCount() )
    {
        long ndx = fieldListCtrl->InsertItem( fieldListCtrl->GetItemCount(), wxEmptyString );

        wxASSERT( ndx >= 0 );

        fieldListCtrl->SetItem( ndx, 1, wxEmptyString );
    }

    fieldListCtrl->SetItem( aFieldNdx, 0, aField.m_Name );
    fieldListCtrl->SetItem( aFieldNdx, 1, aField.m_Text );

    // recompute the column widths here, after setting texts
    fieldListCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    fieldListCtrl->SetColumnWidth( 1, wxLIST_AUTOSIZE );
}


/****************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copySelectedFieldToPanel()
/****************************************************************/
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )    // traps the -1 case too
        return;

    SCH_CMP_FIELD& field = m_FieldsBuf[fieldNdx];

    showCheckBox->SetValue( !(field.m_Attributs & TEXT_NO_VISIBLE) );

    rotateCheckBox->SetValue( field.m_Orient == TEXT_ORIENT_VERT );

    int style = 0;
    if( field.m_Italic )
        style = 1;
    if( field.m_Bold )
        style |= 2;
    m_StyleRadioBox->SetSelection( style );

    fieldNameTextCtrl->SetValue( field.m_Name );

    // if fieldNdx == REFERENCE, VALUE, FOOTPRINT, or DATASHEET, then disable editing
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
    wxPoint zero  = -m_Cmp->m_Pos;  // relative zero

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

    coordText = ReturnStringFromValue( g_UnitMetric, coord.y, EESCHEMA_INTERNAL_UNIT );
    posYTextCtrl->SetValue( coordText );
}


/*****************************************************************/
bool DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copyPanelToSelectedField()
/*****************************************************************/
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )        // traps the -1 case too
        return true;

    SCH_CMP_FIELD& field = m_FieldsBuf[fieldNdx];

    if( showCheckBox->GetValue() )
        field.m_Attributs &= ~TEXT_NO_VISIBLE;
    else
        field.m_Attributs |= TEXT_NO_VISIBLE;

    if( rotateCheckBox->GetValue() )
        field.m_Orient = TEXT_ORIENT_VERT;
    else
        field.m_Orient = TEXT_ORIENT_HORIZ;

    rotateCheckBox->SetValue( field.m_Orient == TEXT_ORIENT_VERT );


    field.m_Name = fieldNameTextCtrl->GetValue();
    /* Void fields texts for REFERENCE and VALUE (value is the name of the compinent in lib ! ) are not allowed
     * change them only for a new non void value
     * When woid, usually netlists are broken
     */
    if( !fieldValueTextCtrl->GetValue().IsEmpty() || fieldNdx > VALUE )
        field.m_Text = fieldValueTextCtrl->GetValue();

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
        field.m_Bold = true;
    else
        field.m_Bold = false;

    double value;

    posXTextCtrl->GetValue().ToDouble( &value );
    field.m_Pos.x = From_User_Unit( g_UnitMetric, value, EESCHEMA_INTERNAL_UNIT );

    posYTextCtrl->GetValue().ToDouble( &value );
    field.m_Pos.y = From_User_Unit( g_UnitMetric, value, EESCHEMA_INTERNAL_UNIT );

    return true;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copyOptionsToPanel()
{
    int choiceCount = unitChoice->GetCount();

    // Remove non existing choices (choiceCount must be <= number for parts)
    int unitcount = m_LibEntry ? m_LibEntry->m_UnitCount : 1;

    if( unitcount < 1 )
        unitcount = 1;

    if( unitcount < choiceCount )
    {
        while( unitcount < choiceCount )
        {
            choiceCount--;
            unitChoice->Delete( choiceCount );
        }
    }

    // For components with multiple parts per package, set the unit selection
    choiceCount = unitChoice->GetCount();
    if( m_Cmp->m_Multi <= choiceCount )
        unitChoice->SetSelection( m_Cmp->m_Multi - 1 );

    // Disable unit selection if only one unit exists:
    if( choiceCount <= 1 )
        unitChoice->Enable( false );

    int orientation = m_Cmp->GetRotationMiroir() & ~(CMP_MIROIR_X | CMP_MIROIR_Y);

    if( orientation == CMP_ORIENT_90 )
        orientationRadioBox->SetSelection( 1 );
    else if( orientation == CMP_ORIENT_180 )
        orientationRadioBox->SetSelection( 2 );
    else if( orientation == CMP_ORIENT_270 )
        orientationRadioBox->SetSelection( 3 );
    else
        orientationRadioBox->SetSelection( 0 );

    int mirror = m_Cmp->GetRotationMiroir() & (CMP_MIROIR_X | CMP_MIROIR_Y);

    if( mirror == CMP_MIROIR_X )
    {
        mirrorRadioBox->SetSelection( 1 );
        D( printf( "mirror=X,1\n" ); )
    }
    else if( mirror == CMP_MIROIR_Y )
    {
        mirrorRadioBox->SetSelection( 2 );
        D( printf( "mirror=Y,2\n" ); )
    }
    else
        mirrorRadioBox->SetSelection( 0 );

    // Activate/Desactivate the normal/convert option ? (activated only if the component has more than one shape)
    if( m_Cmp->m_Convert > 1 )
    {
        convertCheckBox->SetValue( true );
    }

    if( m_LibEntry == NULL || LookForConvertPart( m_LibEntry ) <= 1 )
    {
        convertCheckBox->Enable( false );
    }

    // Show the "Parts Locked" option?
    if( !m_LibEntry || !m_LibEntry->m_UnitSelectionLocked )
    {
        D( printf( "partsAreLocked->false\n" ); )
        partsAreLockedLabel->Show( false );
    }

    // Positionnement de la reference en librairie
    chipnameTextCtrl->SetValue( m_Cmp->m_ChipName );
}


/*****************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::SetInitCmp( wxCommandEvent& event )
/*****************************************************************************/

/* reinitialise components parametres to default values found in lib
 */
{
    EDA_LibComponentStruct* entry;

    if( m_Cmp == NULL )
        return;

    entry = FindLibPart( m_Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

    if( entry == NULL )
        return;

    wxClientDC dc( m_Parent->DrawPanel );
    m_Parent->DrawPanel->PrepareGraphicContext( &dc );

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, g_XorMode );

    /* Initialise fields values to default values found in library:  */
    m_Cmp->GetField( REFERENCE )->m_Pos = entry->m_Prefix.m_Pos + m_Cmp->m_Pos;
    m_Cmp->GetField( REFERENCE )->ImportValues( entry->m_Prefix );

    m_Cmp->GetField( VALUE )->m_Pos = entry->m_Name.m_Pos + m_Cmp->m_Pos;
    m_Cmp->GetField( VALUE )->ImportValues( entry->m_Name );

    m_Cmp->SetRotationMiroir( CMP_NORMAL );

    m_Parent->GetScreen()->SetModify();

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, GR_DEFAULT_DRAWMODE );
    EndModal( 1 );
}
