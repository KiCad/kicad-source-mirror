/**
 * @file dialog_edit_component_in_schematic.cpp
 */

#include <wx/tooltip.h>

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "class_library.h"
#include "sch_component.h"
#include "dialog_helpers.h"

#include "dialog_edit_component_in_schematic.h"


int DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_SelectedRow;


wxSize DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize = wxDefaultSize;


void InstallCmpeditFrame( SCH_EDIT_FRAME* aParent, SCH_COMPONENT* aComponent )
{
    if( aComponent == NULL )    // Null component not accepted
        return;

    aParent->DrawPanel->m_IgnoreMouseEvents = TRUE;

    if( aComponent->Type() != SCH_COMPONENT_T )
    {
        DisplayError( aParent,
                      wxT( "InstallCmpeditFrame() error: This item is not a component" ) );
        return;
    }

    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC dialog( aParent );

    dialog.InitBuffers( aComponent );

    wxSize sizeNow = dialog.GetSize();

    // this relies on wxDefaultSize being -1,-1, be careful here.
    if( sizeNow.GetWidth() < DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize.GetWidth()
        || sizeNow.GetHeight() < DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize.GetHeight() )
    {
        dialog.SetSize( DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize );
    }

    // make sure the chipnameTextCtrl is wide enough to hold any
    // unusually long chip names:
    EnsureTextCtrlWidth( dialog.chipnameTextCtrl );

    dialog.ShowModal();

    // Some of the field values are long and are not always fully visible because the
    // window comes up too narrow.  Remember user's manual window resizing efforts here
    // so it comes up wide enough next time.
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::s_LastSize = dialog.GetSize();

    aParent->DrawPanel->MouseToCursorSchema();
    aParent->DrawPanel->m_IgnoreMouseEvents = false;
}


DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( wxWindow* parent ) :
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP( parent )
{
    m_Parent = (SCH_EDIT_FRAME*) parent;

    m_LibEntry = NULL;
    m_skipCopyFromPanel = false;

    wxListItem columnLabel;

    columnLabel.SetImage( -1 );

    columnLabel.SetText( _( "Name" ) );
    fieldListCtrl->InsertColumn( 0, columnLabel );

    columnLabel.SetText( _( "Value" ) );
    fieldListCtrl->InsertColumn( 1, columnLabel );

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

    copySelectedFieldToPanel();

    wxToolTip::Enable( true );

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
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

#ifndef KICAD_KEEPCASE
    newname.MakeUpper();
#endif
    newname.Replace( wxT( " " ), wxT( "_" ) );

    if( newname.IsEmpty() )
        DisplayError( this, _( "No Component Name!" ) );

    else if( newname.CmpNoCase( m_Cmp->m_ChipName ) )
    {
        if( CMP_LIBRARY::FindLibraryEntry( newname ) == NULL )
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

    // For components with multiple shapes (De Morgan representation) Set the
    // selected shape:
    if( convertCheckBox->IsEnabled() )
    {
        m_Cmp->SetConvert( convertCheckBox->GetValue() ? 2 : 1 );
    }

    //Set the part selection in multiple part per pakcage
    if( m_Cmp->GetUnit() )
    {
        int unit_selection = unitChoice->GetCurrentSelection() + 1;
        m_Cmp->SetUnitSelection( m_Parent->GetSheet(), unit_selection );
        m_Cmp->SetUnit( unit_selection );
    }

    switch( orientationRadioBox->GetSelection() )
    {
    case 0:
        m_Cmp->SetOrientation( CMP_ORIENT_0 );
        break;

    case 1:
        m_Cmp->SetOrientation( CMP_ORIENT_90 );
        break;

    case 2:
        m_Cmp->SetOrientation( CMP_ORIENT_180 );
        break;

    case 3:
        m_Cmp->SetOrientation( CMP_ORIENT_270 );
        break;
    }

    int mirror = mirrorRadioBox->GetSelection();

    switch( mirror )
    {
    case 0:
        break;

    case 1:
        m_Cmp->SetOrientation( CMP_MIRROR_X );
        break;

    case 2:
        m_Cmp->SetOrientation( CMP_MIRROR_Y );
        break;
    }
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnOKButtonClick( wxCommandEvent& event )
{
    if( !copyPanelToSelectedField() )
        return;

    // save old cmp in undo list if not already in edit, or moving ...
    if( m_Cmp->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_Cmp, UR_CHANGED );

    copyPanelToOptions();

    // change all field positions from relative to absolute
    for( unsigned i = 0;  i<m_FieldsBuf.size();  ++i )
    {
        m_FieldsBuf[i].m_Pos += m_Cmp->m_Pos;
    }

    // delete any fields with no name or no value before we copy all of m_FieldsBuf
    // back into the component
    for( unsigned i = MANDATORY_FIELDS;  i<m_FieldsBuf.size(); )
    {
        if( m_FieldsBuf[i].m_Name.IsEmpty() || m_FieldsBuf[i].m_Text.IsEmpty() )
        {
            m_FieldsBuf.erase( m_FieldsBuf.begin() + i );
            continue;
        }

        ++i;
    }

    LIB_COMPONENT* entry = CMP_LIBRARY::FindLibraryComponent( m_Cmp->m_ChipName );

    if( entry &&  entry->IsPower() )
        m_FieldsBuf[VALUE].m_Text = m_Cmp->m_ChipName;

    // copy all the fields back, and change the length of m_Fields.
    m_Cmp->SetFields( m_FieldsBuf );

    // Reference has a specific initialisation, depending on the current active sheet
    // because for a given component, in a complexe hierarchy, there are more than one
    // reference.
    m_Cmp->SetRef( m_Parent->GetSheet(), m_FieldsBuf[REFERENCE].m_Text );

    m_Parent->OnModify();
    m_Parent->GetScreen()->TestDanglingEnds();
    m_Parent->DrawPanel->Refresh( TRUE );

    EndModal( 0 );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::addFieldButtonHandler( wxCommandEvent& event )
{
    // in case m_FieldsBuf[REFERENCE].m_Orient has changed on screen only, grab
    // screen contents.
    if( !copyPanelToSelectedField() )
        return;

    unsigned  fieldNdx = m_FieldsBuf.size();

    SCH_FIELD blank( wxPoint(), fieldNdx, m_Cmp );

    blank.m_Orient = m_FieldsBuf[REFERENCE].m_Orient;

    m_FieldsBuf.push_back( blank );
    m_FieldsBuf[fieldNdx].m_Name = TEMPLATE_FIELDNAME::GetDefaultFieldName( fieldNdx );

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

    updateDisplay( );

    setSelectedFieldNdx( fieldNdx );
    m_skipCopyFromPanel = false;
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

    D( printf( "tmp.m_Text=\"%s\" tmp.m_Name=\"%s\"\n",
               CONV_TO_UTF8( tmp.m_Text ), CONV_TO_UTF8( tmp.m_Name ) ); )

    m_FieldsBuf[fieldNdx - 1] = m_FieldsBuf[fieldNdx];
    setRowItem( fieldNdx - 1, m_FieldsBuf[fieldNdx] );

    m_FieldsBuf[fieldNdx] = tmp;
    setRowItem( fieldNdx, tmp );

    updateDisplay( );

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
        if( aFieldName == m_FieldsBuf[i].m_Name )
            return &m_FieldsBuf[i];
    }
    return NULL;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::InitBuffers( SCH_COMPONENT* aComponent )
{
    m_Cmp = aComponent;

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

    m_LibEntry = CMP_LIBRARY::FindLibraryComponent( m_Cmp->m_ChipName );

#if 0 && defined(DEBUG)
    for( int i = 0;  i<aComponent->GetFieldCount();  ++i )
    {
        printf( "Orig[%d] (x=%d, y=%d)\n", i, aComponent->m_Fields[i].m_Pos.x,
                aComponent->m_Fields[i].m_Pos.y );
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
        m_FieldsBuf[i].m_Pos -= m_Cmp->m_Pos;
    }

    // Add template fieldnames:
    // Now copy in the template fields, in the order that they are present in the
    // template field editor UI.
    const TEMPLATE_FIELDNAMES& tfnames = m_Parent->GetTemplateFieldNames();
    for( TEMPLATE_FIELDNAMES::const_iterator it = tfnames.begin();  it!=tfnames.end();  ++it )
    {
        // add a new field unconditionally to the UI only
        SCH_FIELD   fld( wxPoint(0,0), -1 /* id is a relic */, m_Cmp, it->m_Name );

        // See if field by same name already exists in component.
        SCH_FIELD* schField = aComponent->FindField( it->m_Name );

        // If the field does not already exist in the component, then we
        // use defaults from the template fieldname, otherwise the original
        // values from the component will be set.
        if( !schField )
        {
            if( !it->m_Visible )
                fld.m_Attributs |= TEXT_NO_VISIBLE;
            else
                fld.m_Attributs &= ~TEXT_NO_VISIBLE;

            fld.m_Text = it->m_Value;   // empty? ok too.
        }
        else
        {
            fld = *schField;

            // make the editable field position relative to the component
            fld.m_Pos -= m_Cmp->m_Pos;
        }

        m_FieldsBuf.push_back( fld );
    }

    // Lastly, append any original fields from the component which were not added
    // from the set of fixed fields nor from the set of template fields.
    for( unsigned i=MANDATORY_FIELDS;  i<aComponent->m_Fields.size();  ++i )
    {
        SCH_FIELD*  cmp = &aComponent->m_Fields[i];
        SCH_FIELD*  buf = findField( cmp->m_Name );

        if( !buf )
        {
            int newNdx = m_FieldsBuf.size();
            m_FieldsBuf.push_back( *cmp );

            // make the editable field position relative to the component
            m_FieldsBuf[newNdx].m_Pos -= m_Cmp->m_Pos;
        }
    }


#if 0 && defined(DEBUG)
    for( unsigned i = 0;  i<m_FieldsBuf.size();  ++i )
    {
        printf( "m_FieldsBuf[%d] (x=%-3d, y=%-3d) name:%s\n", i, m_FieldsBuf[i].m_Pos.x,
                m_FieldsBuf[i].m_Pos.y, CONV_TO_UTF8(m_FieldsBuf[i].m_Name) );
    }
#endif

    m_FieldsBuf[REFERENCE].m_Text = m_Cmp->GetRef( m_Parent->GetSheet() );

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

    // put focus on the list ctrl
    fieldListCtrl->SetFocus();

    // resume editing at the last row edited, last time dialog was up.
    setSelectedFieldNdx( s_SelectedRow );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::setRowItem( int              aFieldNdx,
                                                     const SCH_FIELD& aField )
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


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copySelectedFieldToPanel()
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )    // traps the -1 case too
        return;

    SCH_FIELD& field = m_FieldsBuf[fieldNdx];

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


    fieldNameTextCtrl->SetValue( field.m_Name );

    // the names of the fixed fields are not editable, others are.
    fieldNameTextCtrl->Enable(  fieldNdx >= MANDATORY_FIELDS );
    fieldNameTextCtrl->SetEditable( fieldNdx >= MANDATORY_FIELDS );

    // only user defined fields may be moved, and not the top most user defined
    // field since it would be moving up into the fixed fields, > not >=
    moveUpButton->Enable( fieldNdx > MANDATORY_FIELDS );

    // may only delete user defined fields
    deleteFieldButton->Enable( fieldNdx >= MANDATORY_FIELDS );

    fieldValueTextCtrl->SetValue( field.m_Text );

    // For power symbols, the value is NOR editable, because value and pin
    // name must be same and can be edited only in library editor
    if( fieldNdx == VALUE && m_LibEntry && m_LibEntry->IsPower() )
        fieldValueTextCtrl->Enable( false );
    else
        fieldValueTextCtrl->Enable( true );

    textSizeTextCtrl->SetValue(
        WinEDA_GraphicTextCtrl::FormatSize( EESCHEMA_INTERNAL_UNIT,
                                            g_UserUnit, field.m_Size.x ) );

    wxPoint coord = field.m_Pos;
    wxPoint zero  = -m_Cmp->m_Pos;  // relative zero

    // If the field value is empty and the position is at relative zero, we
    // set the initial position as a small offset from the ref field, and
    // orient it the same as the ref field.  That is likely to put it at least
    // close to the desired position.
    if( coord == zero && field.m_Text.IsEmpty() )
    {
        rotateCheckBox->SetValue( m_FieldsBuf[REFERENCE].m_Orient == TEXT_ORIENT_VERT );

        coord.x = m_FieldsBuf[REFERENCE].m_Pos.x
            + ( fieldNdx - MANDATORY_FIELDS + 1 ) * 100;

        coord.y = m_FieldsBuf[REFERENCE].m_Pos.y
            + ( fieldNdx - MANDATORY_FIELDS + 1 ) * 100;

        // coord can compute negative if field is < MANDATORY_FIELDS, e.g. FOOTPRINT.
        // That is ok, we basically don't want all the new empty fields on
        // top of each other.
    }

    wxString coordText = ReturnStringFromValue( g_UserUnit, coord.x,
                                                EESCHEMA_INTERNAL_UNIT );
    posXTextCtrl->SetValue( coordText );

    coordText = ReturnStringFromValue( g_UserUnit, coord.y,
                                       EESCHEMA_INTERNAL_UNIT );
    posYTextCtrl->SetValue( coordText );
}


bool DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copyPanelToSelectedField()
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )        // traps the -1 case too
        return true;

    SCH_FIELD& field = m_FieldsBuf[fieldNdx];

    if( showCheckBox->GetValue() )
        field.m_Attributs &= ~TEXT_NO_VISIBLE;
    else
        field.m_Attributs |= TEXT_NO_VISIBLE;

    if( rotateCheckBox->GetValue() )
        field.m_Orient = TEXT_ORIENT_VERT;
    else
        field.m_Orient = TEXT_ORIENT_HORIZ;

    rotateCheckBox->SetValue( field.m_Orient == TEXT_ORIENT_VERT );

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

    field.m_Name = fieldNameTextCtrl->GetValue();
    /* Void fields texts for REFERENCE and VALUE (value is the name of the
     * compinent in lib ! ) are not allowed
     * change them only for a new non void value
     * When woid, usually netlists are broken
     */
    if( !fieldValueTextCtrl->GetValue().IsEmpty() || fieldNdx > VALUE )
        field.m_Text = fieldValueTextCtrl->GetValue();

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

    return true;
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copyOptionsToPanel()
{
    int choiceCount = unitChoice->GetCount();

    // Remove non existing choices (choiceCount must be <= number for parts)
    int unitcount = m_LibEntry ? m_LibEntry->GetPartCount() : 1;

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

    if( m_Cmp->GetUnit() <= choiceCount )
        unitChoice->SetSelection( m_Cmp->GetUnit() - 1 );

    // Disable unit selection if only one unit exists:
    if( choiceCount <= 1 )
        unitChoice->Enable( false );

    int orientation = m_Cmp->GetOrientation()
        & ~( CMP_MIRROR_X | CMP_MIRROR_Y );

    if( orientation == CMP_ORIENT_90 )
        orientationRadioBox->SetSelection( 1 );
    else if( orientation == CMP_ORIENT_180 )
        orientationRadioBox->SetSelection( 2 );
    else if( orientation == CMP_ORIENT_270 )
        orientationRadioBox->SetSelection( 3 );
    else
        orientationRadioBox->SetSelection( 0 );

    int mirror = m_Cmp->GetOrientation() & ( CMP_MIRROR_X | CMP_MIRROR_Y );

    if( mirror == CMP_MIRROR_X )
    {
        mirrorRadioBox->SetSelection( 1 );
        D( printf( "mirror=X,1\n" ); )
    }
    else if( mirror == CMP_MIRROR_Y )
    {
        mirrorRadioBox->SetSelection( 2 );
        D( printf( "mirror=Y,2\n" ); )
    }
    else
        mirrorRadioBox->SetSelection( 0 );

    // Activate/Desactivate the normal/convert option ? (activated only if
    // the component has more than one shape)
    if( m_Cmp->GetConvert() > 1 )
    {
        convertCheckBox->SetValue( true );
    }

    if( m_LibEntry == NULL || !m_LibEntry->HasConversion() )
    {
        convertCheckBox->Enable( false );
    }

    // Show the "Parts Locked" option?
    if( !m_LibEntry || !m_LibEntry->UnitsLocked() )
    {
        D( printf( "partsAreLocked->false\n" ); )
        partsAreLockedLabel->Show( false );
    }

    // Positionnement de la reference en librairie
    chipnameTextCtrl->SetValue( m_Cmp->m_ChipName );
}

#include "kicad_device_context.h"

/* reinitialize components parametres to default values found in lib
 */
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::SetInitCmp( wxCommandEvent& event )
{
    LIB_COMPONENT* entry;

    if( m_Cmp == NULL )
        return;

    entry = CMP_LIBRARY::FindLibraryComponent( m_Cmp->m_ChipName );

    if( entry == NULL )
        return;

    // save old cmp in undo list if not already in edit, or moving ...
    if( m_Cmp->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_Cmp, UR_CHANGED );

    INSTALL_UNBUFFERED_DC( dc, m_Parent->DrawPanel );
    m_Cmp->Draw( m_Parent->DrawPanel, &dc, wxPoint( 0, 0 ), g_XorMode );

    // Initialize field values to default values found in library:
    LIB_FIELD& refField = entry->GetReferenceField();
    m_Cmp->GetField( REFERENCE )->m_Pos = refField.m_Pos + m_Cmp->m_Pos;
    m_Cmp->GetField( REFERENCE )->ImportValues( refField );

    LIB_FIELD& valField = entry->GetValueField();
    m_Cmp->GetField( VALUE )->m_Pos = valField.m_Pos + m_Cmp->m_Pos;
    m_Cmp->GetField( VALUE )->ImportValues( valField );

    m_Cmp->SetOrientation( CMP_NORMAL );

    m_Parent->OnModify( );

    m_Cmp->Draw( m_Parent->DrawPanel, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    EndModal( 1 );
}
