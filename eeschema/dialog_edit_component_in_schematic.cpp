
#include <wx/checklst.h>
#include <wx/tooltip.h>

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "dialog_edit_component_in_schematic.h"


/**********************************************************************/
void InstallCmpeditFrame( WinEDA_SchematicFrame* parent, wxPoint& pos,
                          SCH_COMPONENT* aComponent )
/*********************************************************************/
{
    parent->DrawPanel->m_IgnoreMouseEvents = TRUE;
    if( aComponent->Type() != TYPE_SCH_COMPONENT )
    {
        DisplayError( parent,
                     wxT( "InstallCmpeditFrame() error: This struct is not a component" ) );
    }
    else
    {
        wxASSERT( aComponent );     // this is no longer callable with NULL

        DIALOG_EDIT_COMPONENT_IN_SCHEMATIC* frame =
            new DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( parent );

        frame->InitBuffers( aComponent );

//        frame->Layout();

        frame->ShowModal();
        frame->Destroy();
    }

    parent->DrawPanel->MouseToCursorSchema();
    parent->DrawPanel->m_IgnoreMouseEvents = FALSE;
}


#if 0
/*********************************************************************/
void SCH_CMP_FIELD::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
/*********************************************************************/
{
    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;

    SCH_COMPONENT* component = (SCH_COMPONENT*) m_Parent;

    // save old component in undo list
    if( g_ItemToUndoCopy && g_ItemToUndoCopy->Type() == component->Type() )
    {
        component->SwapData( (SCH_COMPONENT*) g_ItemToUndoCopy );

        frame->SaveCopyInUndoList( component, IS_CHANGED );

        component->SwapData( (SCH_COMPONENT*) g_ItemToUndoCopy );
    }

    m_AddExtraText = 0;
    if( m_FieldId == REFERENCE )
    {
        EDA_LibComponentStruct* part;

        part = FindLibPart( component->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( part )
        {
            if( part->m_UnitCount > 1 )
                m_AddExtraText = 1;
        }
    }

    Draw( frame->DrawPanel, DC, wxPoint(0,0), GR_DEFAULT_DRAWMODE );
    m_Flags = 0;
    frame->GetScreen()->SetCurItem( NULL );
    frame->GetScreen()->SetModify();
}
#endif


DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::DIALOG_EDIT_COMPONENT_IN_SCHEMATIC( wxWindow* parent ) :
    DIALOG_EDIT_COMPONENT_IN_SCHEMATIC_FBP( parent )
{
    m_Parent = (WinEDA_SchematicFrame*) parent;

    m_SelectedRow = 0;

    wxListItem  columnLabel;

    columnLabel.SetImage(-1);

    columnLabel.SetText( _("Name") );
    fieldListCtrl->InsertColumn( 0, columnLabel );

    columnLabel.SetText( _("Value") );
    fieldListCtrl->InsertColumn( 1, columnLabel );

    // these must go here late in the game.
    fieldListCtrl->SetColumnWidth( 0, wxLIST_AUTOSIZE );
    fieldListCtrl->SetColumnWidth( 1, wxLIST_AUTOSIZE );

    copySelectedFieldToPanel();

    wxToolTip::Enable( true );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnListItemDeselected( wxListEvent& event )
{
    D(printf("OnListItemDeselected()\n");)
    copyPanelToSelectedField();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::OnListItemSelected( wxListEvent& event )
{
    D(printf("OnListItemSelected()\n");)

    m_SelectedRow = event.GetIndex();

    copySelectedFieldToPanel();
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::setSelectedFieldNdx( int aFieldNdx )
{
    fieldListCtrl->SetItemState( aFieldNdx, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
    fieldListCtrl->EnsureVisible( aFieldNdx );
}


int DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::getSelectedFieldNdx()
{
    return m_SelectedRow;
}


#if 0
/*****************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::SetInitCmp( wxCommandEvent& event )
/*****************************************************************************/

/* Replace le composant en position normale, dimensions et positions
 *  fields comme definies en librairie
 */
{
    EDA_LibComponentStruct* Entry;

    if( m_Cmp == NULL )
        return;

    Entry = FindLibPart( m_Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );

    if( Entry == NULL )
        return;

    wxClientDC dc( m_Parent->DrawPanel );
    m_Parent->DrawPanel->PrepareGraphicContext( &dc );

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, g_XorMode );

    /* Mise aux valeurs par defaut des champs et orientation */
    m_Cmp->GetField( REFERENCE )->m_Pos.x =
        Entry->m_Prefix.m_Pos.x + m_Cmp->m_Pos.x;

    m_Cmp->GetField( REFERENCE )->m_Pos.y =
        Entry->m_Prefix.m_Pos.y + m_Cmp->m_Pos.y;

    m_Cmp->GetField( REFERENCE )->m_Orient   = Entry->m_Prefix.m_Orient;
    m_Cmp->GetField( REFERENCE )->m_Size     = Entry->m_Prefix.m_Size;
    m_Cmp->GetField( REFERENCE )->m_HJustify = Entry->m_Prefix.m_HJustify;
    m_Cmp->GetField( REFERENCE )->m_VJustify = Entry->m_Prefix.m_VJustify;

    m_Cmp->GetField( VALUE )->m_Pos.x =
        Entry->m_Name.m_Pos.x + m_Cmp->m_Pos.x;

    m_Cmp->GetField( VALUE )->m_Pos.y =
        Entry->m_Name.m_Pos.y + m_Cmp->m_Pos.y;

    m_Cmp->GetField( VALUE )->m_Orient   = Entry->m_Name.m_Orient;
    m_Cmp->GetField( VALUE )->m_Size     = Entry->m_Name.m_Size;
    m_Cmp->GetField( VALUE )->m_HJustify = Entry->m_Name.m_HJustify;
    m_Cmp->GetField( VALUE )->m_VJustify = Entry->m_Name.m_VJustify;

    m_Cmp->SetRotationMiroir( CMP_NORMAL );

    m_Parent->GetScreen()->SetModify();

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, GR_DEFAULT_DRAWMODE );
    EndModal( 1 );
}
#endif


/*******************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::InitBuffers( SCH_COMPONENT* aComponent )
/*******************************************************************************/
{
    m_Cmp = aComponent;

    // copy all the fields to a work area
    m_FieldsBuf = aComponent->m_Fields;

    m_FieldsBuf[REFERENCE].m_Text = m_Cmp->GetRef( m_Parent->GetSheet() );

    for( int ii = 0;  ii < aComponent->GetFieldCount();  ++ii )
    {
        // make the editable field position relative to the component
        m_FieldsBuf[ii].m_Pos -= m_Cmp->m_Pos;

        setRowItem( ii, m_FieldsBuf[ii] );
    }

    setSelectedFieldNdx( REFERENCE );
}


void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::setRowItem( int aFieldNdx, const SCH_CMP_FIELD& aField )
{
    wxASSERT( aFieldNdx >= 0 );

    // insert blanks if aFieldNdx is referencing a yet to be defined row
    while( aFieldNdx >= fieldListCtrl->GetItemCount() )
    {
        long ndx = fieldListCtrl->InsertItem( fieldListCtrl->GetItemCount(),  wxEmptyString );

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

    SCH_CMP_FIELD&  field = m_FieldsBuf[fieldNdx];

    fieldNameTextCtrl->SetValue( field.m_Name );

    // if fieldNdx == REFERENCE, VALUE, FOOTPRINT, or DATASHEET, then disable editing
    fieldNameTextCtrl->Enable(  fieldNdx >= FIELD1 );

    fieldValueTextCtrl->SetValue( field.m_Text );

/*
    if( fieldNdx == VALUE && m_LibEntry && m_LibEntry->m_Options == ENTRY_POWER )
        m_FieldTextCtrl->Enable( FALSE );
*/

    showCheckBox->SetValue( !(field.m_Attributs & TEXT_NO_VISIBLE) );

    // If the field value is empty and the position is zero, we set the
    // initial position as a small offset from the ref field, and orient
    // it the same as the ref field.  That is likely to put it at least
    // close to the desired position.
    if( field.m_Pos == wxPoint( 0, 0 ) && field.m_Text.IsEmpty() )
    {
        wxString dim;

        // @todo look at the dedicated position control for this.
        dim.Printf( wxT("%d"), m_FieldsBuf[REFERENCE].m_Pos.x + (fieldNdx-FIELD1+1)*100 );
        posXTextCtrl->SetValue( dim );

        dim.Printf( wxT("%d"), m_FieldsBuf[REFERENCE].m_Pos.y + (fieldNdx-FIELD1+1)*100 );
        posYTextCtrl->SetValue( dim );
    }
    else
    {
        wxString dim;

        dim.Printf( wxT("%d"), field.m_Pos.x );
        posXTextCtrl->SetValue( dim );

        dim.Printf( wxT("%d"), field.m_Pos.y );
        posYTextCtrl->SetValue( dim );
    }

    rotateCheckBox->SetValue( field.m_Orient == TEXT_ORIENT_VERT );

#if 0

    m_FieldNameCtrl->SetValue( m_FieldName[fieldNdx] );

    m_FieldTextCtrl->SetValue( m_FieldText[fieldNdx] );
    m_FieldTextCtrl->SetValue( m_FieldSize[fieldNdx] );
#endif
}


/*****************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::copyPanelToSelectedField()
/*****************************************************************/
{
    unsigned fieldNdx = getSelectedFieldNdx();

    if( fieldNdx >= m_FieldsBuf.size() )
        return;

    SCH_CMP_FIELD&  field = m_FieldsBuf[fieldNdx];

    field.m_Name = fieldNameTextCtrl->GetValue();
    field.m_Text = fieldValueTextCtrl->GetValue();

//    field.m_Size =

//    m_FieldPosition[id] = m_FieldPositionCtrl->GetValue();
//    m_FieldSize[id] = m_FieldTextCtrl->GetTextSize();
}


#if 0

/*************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::fillTableModel()
/*************************************************************/
{
}


/**********************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::buildPanelBasic()
/**********************************************************/

/* create the basic panel for component properties editing
 */
{
    int Nb_Max_Unit = m_SelectUnit->GetCount();
    int ii;

    int nb_units = m_LibEntry ? MAX( m_LibEntry->m_UnitCount, 1 ) : 0;

    // Disable non existant units selection buttons
    for( ii = nb_units; ii < Nb_Max_Unit; ii++ )
    {
        m_SelectUnit->Enable( ii, FALSE );
    }

    if( m_Cmp->m_Multi <= Nb_Max_Unit )
        m_SelectUnit->SetSelection( m_Cmp->m_Multi - 1 );

    ii = m_Cmp->GetRotationMiroir() & ~(CMP_MIROIR_X | CMP_MIROIR_Y);

    if( ii == CMP_ORIENT_90 )
        m_OrientUnit->SetSelection( 1 );
    else if( ii == CMP_ORIENT_180 )
        m_OrientUnit->SetSelection( 2 );
    else if( ii == CMP_ORIENT_270 )
        m_OrientUnit->SetSelection( 3 );

    ii = m_Cmp->GetRotationMiroir() & (CMP_MIROIR_X | CMP_MIROIR_Y);
    if( ii == CMP_MIROIR_X )
        m_MirrorUnit->SetSelection( 1 );
    else if( ii == CMP_MIROIR_Y )
        m_MirrorUnit->SetSelection( 2 );

    // Positionnement de la selection normal/convert
    if( m_Cmp->m_Convert > 1 )
        m_ConvertButt->SetValue( TRUE );

    if( (m_LibEntry == NULL) || LookForConvertPart( m_LibEntry ) <= 1 )
    {
        m_ConvertButt->Enable( FALSE );
    }

    // Show the "Parts Locked" option:
    if( !m_LibEntry || !m_LibEntry->m_UnitSelectionLocked )
    {
        m_MsgPartLocked->Show( false );
    }

    // Positionnement de la reference en librairie
    m_RefInLib->SetValue( m_Cmp->m_ChipName );
}


/*************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::selectNewField( wxCommandEvent& event )
/*************************************************************************/

/* called when changing the current field selected
 *  Save the current field settings in buffer and display the new one
 */
{
    CopyPanelFieldToData();
    m_CurrentFieldId = m_FieldSelection->GetSelection();
    CopyDataToPanelField();
}


/***********************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_SCHEMATIC::ComponentPropertiesAccept( wxCommandEvent& event )
/***********************************************************************************/

/* Update the new parameters for the current edited component
 */
{
    wxPoint    cmp_pos = m_Cmp->m_Pos;
    wxClientDC dc( m_Parent->DrawPanel );
    wxString   newname;

    /* save old cmp in undo list if not already in edit, or moving ... */
    if( m_Cmp->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_Cmp, IS_CHANGED );

    CopyPanelFieldToData();

    m_Parent->DrawPanel->PrepareGraphicContext( &dc );


    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, g_XorMode );

    newname = m_RefInLib->GetValue();
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
        else    // Changement de composant!
        {
            m_Cmp->m_ChipName = newname;
        }
    }

    // Mise a jour de la representation:
    if( m_ConvertButt->IsEnabled() )
        (m_ConvertButt->GetValue() == TRUE) ?
        m_Cmp->m_Convert = 2 : m_Cmp->m_Convert = 1;

    //Set the part selection in multiple part per pakcage
    if( m_Cmp->m_Multi )
    {
        int unit_selection = m_SelectUnit->GetSelection() + 1;
        m_Cmp->SetUnitSelection( m_Parent->GetSheet(), unit_selection );
        m_Cmp->m_Multi = unit_selection;
    }

    //Mise a jour de l'orientation:
    switch( m_OrientUnit->GetSelection() )
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

    switch( m_MirrorUnit->GetSelection() )
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


    // Mise a jour des textes (update the texts)
    for( int ii = REFERENCE; ii < NUMBER_OF_FIELDS; ii++ )
    {
        if( ii == REFERENCE )   // la reference ne peut etre vide
        {
            if( !m_FieldText[ii].IsEmpty() )
                m_Cmp->SetRef(m_Parent->GetSheet(), m_FieldText[ii]);
        }
        else if( ii == VALUE )  // la valeur ne peut etre vide et ne peut etre change sur un POWER
        {
            EDA_LibComponentStruct* Entry = FindLibPart( m_Cmp->m_ChipName.GetData(
                                                             ), wxEmptyString, FIND_ROOT );
            if( Entry && (Entry->m_Options == ENTRY_POWER) )
                m_Cmp->GetField( ii )->m_Text = m_Cmp->m_ChipName;
            else if( !m_FieldText[ii].IsEmpty() )
            {
                m_Cmp->GetField( ii )->m_Text = m_FieldText[ii];
            }
        }
        else
            m_Cmp->GetField( ii )->m_Text = m_FieldText[ii];

        if( ii >= FIELD1 && m_FieldName[ii] != ReturnDefaultFieldName( ii ) )
            m_Cmp->GetField( ii )->m_Name = m_FieldName[ii];
        else
            m_Cmp->GetField( ii )->m_Name.Empty();

        m_Cmp->GetField( ii )->m_Size.x     =
            m_Cmp->GetField( ii )->m_Size.y = m_FieldSize[ii];
        if( m_FieldFlags[ii] )
            m_Cmp->GetField( ii )->m_Attributs &= ~TEXT_NO_VISIBLE;
        else
            m_Cmp->GetField( ii )->m_Attributs |= TEXT_NO_VISIBLE;
        m_Cmp->GetField( ii )->m_Orient = m_FieldOrient[ii] ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ;
        m_Cmp->GetField( ii )->m_Pos    = m_FieldPosition[ii];
        m_Cmp->GetField( ii )->m_Pos.x += cmp_pos.x;
        m_Cmp->GetField( ii )->m_Pos.y += cmp_pos.y;
    }

    m_Parent->GetScreen()->SetModify();

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, GR_DEFAULT_DRAWMODE );
    m_Parent->TestDanglingEnds( m_Parent->GetScreen()->EEDrawList, &dc );

    EndModal( 0 );
}

#endif
