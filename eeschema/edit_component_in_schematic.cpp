/********************************/
/* Scehematic component edition */
/********************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "wx/checklst.h"

#include "protos.h"


/* Fonctions exportees */

/* Fonctions locales */
static void AbortMoveCmpField( WinEDA_DrawPanel* Panel, wxDC* DC );
static void MoveCmpField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* variables locales */
static PartTextStruct* CurrentField;
static int             Multiflag;
static int             TextFieldSize = DEFAULT_SIZE_TEXT;
static wxPoint         OldPos;


/************************************/
/* class WinEDA_PartPropertiesFrame */
/************************************/
#define ID_ON_SELECT_FIELD 3000

#include "dialog_edit_component_in_schematic.cpp"

/**********************************************************************/
void InstallCmpeditFrame( WinEDA_SchematicFrame* parent, wxPoint& pos,
                          EDA_SchComponentStruct* cmp )
/*********************************************************************/

/* Create the dialog box for the current component edition
 */
{
    parent->DrawPanel->m_IgnoreMouseEvents = TRUE;
    if( cmp->Type() != DRAW_LIB_ITEM_STRUCT_TYPE )
    {
        DisplayError( parent,
                     wxT( "InstallCmpeditFrame() error: This struct is not a component" ) );
    }
    else
    {
        WinEDA_ComponentPropertiesFrame* frame =
            new WinEDA_ComponentPropertiesFrame( parent, cmp );
        frame->ShowModal(); frame->Destroy();
    }

    parent->DrawPanel->MouseToCursorSchema();
    parent->DrawPanel->m_IgnoreMouseEvents = FALSE;
}


/*****************************************************/
void WinEDA_ComponentPropertiesFrame::InitBuffers()
/*****************************************************/

/* Init the buffers to a default value,
 *  or to values from Component!= NULL
 */
{
    int ii;

    m_CurrentFieldId = REFERENCE;

    /* Init default values */
    for( ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
    {
        m_FieldSize[ii]   = DEFAULT_SIZE_TEXT;
        m_FieldFlags[ii]  = 1;
        m_FieldOrient[ii] = 0;
    }

    if( m_Cmp == NULL )
        return;

    for( ii = REFERENCE; ii < NUMBER_OF_FIELDS; ii++ )
    {
        m_FieldName[ii]  = m_Cmp->ReturnFieldName( ii );
        m_FieldText[ii]  = m_Cmp->m_Field[ii].m_Text;
        m_FieldSize[ii]  = m_Cmp->m_Field[ii].m_Size.x;
        m_FieldFlags[ii] =
            (m_Cmp->m_Field[ii].m_Attributs & TEXT_NO_VISIBLE) ? 0 : 1;
        m_FieldOrient[ii] = m_Cmp->m_Field[ii].m_Orient == TEXT_ORIENT_VERT ? 1 : 0;

        if( m_Cmp->m_Field[ii].m_Text.IsEmpty() )
            continue;

        // These values have meaning only if this field is not void:
        m_FieldPosition[ii]    = m_Cmp->m_Field[ii].m_Pos;
        m_FieldPosition[ii].x -= m_Cmp->m_Pos.x;
        m_FieldPosition[ii].y -= m_Cmp->m_Pos.y;
    }
}


/****************************************************************/
void WinEDA_ComponentPropertiesFrame::CopyDataToPanelField()
/****************************************************************/

/* Set the values displayed on the panel field according to
 *  the current field number
 */
{
    int fieldId = m_CurrentFieldId;

    for( int ii = FIELD1; ii < NUMBER_OF_FIELDS; ii++ )
        m_FieldSelection->SetString( ii, m_FieldName[ii] );

    if( fieldId == VALUE && m_LibEntry && m_LibEntry->m_Options == ENTRY_POWER )
        m_FieldTextCtrl->Enable( FALSE );

    if( m_FieldFlags[fieldId] )
        m_ShowFieldTextCtrl->SetValue( TRUE );
    else
        m_ShowFieldTextCtrl->SetValue( FALSE );

    // If the field value is empty and the position is zero, we set the
    // initial position as a small offset from the ref field, and orient
    // it the same as the ref field.  That is likely to put it at least
    // close to the desired position.
    if( ( m_FieldPosition[fieldId] == wxPoint( 0, 0 ) )
       && m_FieldText[fieldId].IsEmpty() )
    {
        m_VorientFieldText->SetValue( m_FieldOrient[REFERENCE] != 0 );
        m_FieldPositionCtrl->SetValue( m_FieldPosition[REFERENCE].x + 100,
                                       m_FieldPosition[REFERENCE].y + 100 );
    }
    else
    {
        m_FieldPositionCtrl->SetValue( m_FieldPosition[fieldId].x, m_FieldPosition[fieldId].y );
        m_VorientFieldText->SetValue( m_FieldOrient[fieldId] != 0 );
    }

    m_FieldNameCtrl->SetValue( m_FieldName[fieldId] );
    if( fieldId < FIELD1 )
        m_FieldNameCtrl->Enable( FALSE );
    else
        m_FieldNameCtrl->Enable( TRUE );
    m_FieldTextCtrl->SetValue( m_FieldText[fieldId] );
    m_FieldTextCtrl->SetValue( m_FieldSize[fieldId] );
}


/****************************************************************/
void WinEDA_ComponentPropertiesFrame::CopyPanelFieldToData()
/****************************************************************/

/* Copy the values displayed on the panel field to the buffers according to
 *  the current field number
 */
{
    int id = m_CurrentFieldId;

    m_FieldFlags[id]    = m_ShowFieldTextCtrl->GetValue();
    m_FieldOrient[id]   = m_VorientFieldText->GetValue();
    m_FieldText[id]     = m_FieldTextCtrl->GetText();
    m_FieldName[id]     = m_FieldNameCtrl->GetValue();
    m_FieldPosition[id] = m_FieldPositionCtrl->GetValue();
    m_FieldSize[id] = m_FieldTextCtrl->GetTextSize();
}


/*************************************************************/
void WinEDA_ComponentPropertiesFrame::BuildPanelFields()
/*************************************************************/
{
    int     ii, FieldId;
    wxPoint field_pos;

    m_CurrentFieldId = FieldId = REFERENCE;

    // Create the box field selection:
    wxString fieldnamelist[NUMBER_OF_FIELDS];
    for( ii = 0; ii < NUMBER_OF_FIELDS; ii++ )
    {
        if( m_FieldName[ii].IsEmpty() )
            fieldnamelist[ii] = ReturnDefaultFieldName( ii );
        else
            fieldnamelist[ii] = m_FieldName[ii];
    }

    m_FieldSelection = new wxRadioBox( m_PanelField, ID_ON_SELECT_FIELD,
                                       _( "Field to edit" ), wxDefaultPosition, wxDefaultSize,
                                       NUMBER_OF_FIELDS, fieldnamelist, 2, wxRA_SPECIFY_COLS );
    m_FieldSelectionBoxSizer->Add( m_FieldSelection, 0, wxGROW | wxALL, 5 );

    // Create the box for field name display
    m_FieldNameCtrl = new WinEDA_EnterText( m_PanelField,
                                           _( "Field Name:" ), m_FieldName[FieldId],
                                           m_FieldDatasBoxSizer, wxSize( 200, -1 ) );
    if( FieldId < FIELD1 )
        m_FieldNameCtrl->Enable( FALSE );
    else
        m_FieldNameCtrl->Enable( TRUE );

    // Create the box for text editing (text, size)
    m_FieldTextCtrl = new WinEDA_GraphicTextCtrl( m_PanelField,
                                                  _(
                                                      "Value:" ),
                                                  m_FieldText[FieldId],
                                                  m_FieldSize[FieldId],
                                                  g_UnitMetric,
                                                  m_FieldDatasBoxSizer, 200,
                                                  m_Parent->m_InternalUnits );

    // Create the box for text editing (position)
    m_FieldPositionCtrl = new WinEDA_PositionCtrl( m_PanelField, _( "Pos" ),
                                                   m_FieldPosition[FieldId],
                                                   g_UnitMetric,
                                                   m_FieldDatasBoxSizer,
                                                   m_Parent->m_InternalUnits );

    CopyDataToPanelField();
}


/**********************************************************/
void WinEDA_ComponentPropertiesFrame::BuildPanelBasic()
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
void WinEDA_ComponentPropertiesFrame::SelectNewField( wxCommandEvent& event )
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
void WinEDA_ComponentPropertiesFrame::ComponentPropertiesAccept( wxCommandEvent& event )
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
            wxString msg;
            msg.Printf( _( "Component [%s] not found!" ), newname.GetData() );
            DisplayError( this, msg );
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

    //Mise a jour de la selection de l'élément dans le boitier
    if( m_Cmp->m_Multi )
        m_Cmp->m_Multi = m_SelectUnit->GetSelection() + 1;

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


    // Mise a jour des textes
    for( int ii = REFERENCE; ii < NUMBER_OF_FIELDS; ii++ )
    {
        if( ii == REFERENCE )   // la reference ne peut etre vide
        {
            if( !m_FieldText[ii].IsEmpty() )
                m_Cmp->m_Field[ii].m_Text = m_FieldText[ii];
        }
        else if( ii == VALUE )  // la valeur ne peut etre vide et ne peut etre change sur un POWER
        {
            EDA_LibComponentStruct* Entry = FindLibPart( m_Cmp->m_ChipName.GetData(
                                                             ), wxEmptyString, FIND_ROOT );
            if( Entry && (Entry->m_Options == ENTRY_POWER) )
                m_Cmp->m_Field[ii].m_Text = m_Cmp->m_ChipName;
            else if( !m_FieldText[ii].IsEmpty() )
            {
                m_Cmp->m_Field[ii].m_Text = m_FieldText[ii];
            }
        }
        else
            m_Cmp->m_Field[ii].m_Text = m_FieldText[ii];

        if( ii >= FIELD1 && m_FieldName[ii] != ReturnDefaultFieldName( ii ) )
            m_Cmp->m_Field[ii].m_Name = m_FieldName[ii];
        else
            m_Cmp->m_Field[ii].m_Name.Empty();

        m_Cmp->m_Field[ii].m_Size.x     =
            m_Cmp->m_Field[ii].m_Size.y = m_FieldSize[ii];
        if( m_FieldFlags[ii] )
            m_Cmp->m_Field[ii].m_Attributs &= ~TEXT_NO_VISIBLE;
        else
            m_Cmp->m_Field[ii].m_Attributs |= TEXT_NO_VISIBLE;
        m_Cmp->m_Field[ii].m_Orient = m_FieldOrient[ii] ? TEXT_ORIENT_VERT : TEXT_ORIENT_HORIZ;
        m_Cmp->m_Field[ii].m_Pos    = m_FieldPosition[ii];
        m_Cmp->m_Field[ii].m_Pos.x += cmp_pos.x;
        m_Cmp->m_Field[ii].m_Pos.y += cmp_pos.y;
    }

    m_Parent->m_CurrentScreen->SetModify();

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, GR_DEFAULT_DRAWMODE );
    m_Parent->TestDanglingEnds( m_Parent->m_CurrentScreen->EEDrawList, &dc );

    Close();
}


/************************************************************************************/
void WinEDA_SchematicFrame::StartMoveCmpField( PartTextStruct* Field, wxDC* DC )
/************************************************************************************/

/* Prepare le deplacement du texte en cours d'edition
 */
{
    EDA_LibComponentStruct* Entry;


    CurrentField = Field;
    if( Field == NULL )
        return;

    if( Field->m_Text == wxEmptyString )
    {
        DisplayError( this, _( "No Field to move" ), 10 );
        return;
    }

    wxPoint pos, newpos;
    int     x1, y1;
    EDA_SchComponentStruct* Cmp = (EDA_SchComponentStruct*) CurrentField->m_Parent;

    delete g_ItemToUndoCopy;
    g_ItemToUndoCopy = Cmp->GenCopy();

    pos = Cmp->m_Pos;

    /* Les positions sont calculees par la matrice TRANSPOSEE de la matrice
     *  de rotation-miroir */
    x1 = Field->m_Pos.x - pos.x;
    y1 = Field->m_Pos.y - pos.y;

    // Empirically this is necessary.  The Y coordinate appears to be inverted
    // under some circumstances, but that inversion is not preserved by all
    // combinations of mirroring and rotation.  The following clause is true
    // when the number of rotations and the number of mirrorings are both odd.
    if( Cmp->m_Transform[1][0] * Cmp->m_Transform[0][1] < 0 )
    {
        y1 = -y1;
    }
    newpos.x = pos.x + Cmp->m_Transform[0][0] * x1 + Cmp->m_Transform[1][0] * y1;
    newpos.y = pos.y + Cmp->m_Transform[0][1] * x1 + Cmp->m_Transform[1][1] * y1;

    DrawPanel->CursorOff( DC );
    m_CurrentScreen->m_Curseur = newpos;
    DrawPanel->MouseToCursorSchema();

    OldPos    = Field->m_Pos;
    Multiflag = 0;
    if( Field->m_FieldId == REFERENCE )
    {
        Entry = FindLibPart( Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry  != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                Multiflag = 1;
        }
    }

    DrawPanel->ForceCloseManageCurseur = AbortMoveCmpField;
    DrawPanel->ManageCurseur = MoveCmpField;
    Field->m_Flags = IS_MOVED;

    DrawPanel->CursorOn( DC );
}


/**********************************************************************************/
void WinEDA_SchematicFrame::EditCmpFieldText( PartTextStruct* Field, wxDC* DC )
/**********************************************************************************/
/* Edit the field Field (text, size)  */
{
    int FieldNumber, flag;
    EDA_LibComponentStruct* Entry;

    if( Field == NULL )
    {
        DisplayError( this, _( "No Field To Edit" ), 10 );
        return;
    }

    EDA_SchComponentStruct* Cmp = (EDA_SchComponentStruct*) Field->m_Parent;

    FieldNumber = Field->m_FieldId;
    if( FieldNumber == VALUE )
    {
        Entry = FindLibPart( Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry && (Entry->m_Options == ENTRY_POWER) )
        {
            DisplayInfo( this,
                        _(
                            "Part is a POWER, value cannot be modified!\nYou must create a new power" )
                         );
            return;
        }
    }

    flag = 0;
    if( FieldNumber == REFERENCE )
    {
        Entry = FindLibPart( Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                flag = 1;
        }
    }


    /* save old cmp in undo list if not already in edit, or moving ... */
    if( Field->m_Flags == 0 )
        SaveCopyInUndoList( Cmp, IS_CHANGED );

    wxString newtext = Field->m_Text;
    DrawPanel->m_IgnoreMouseEvents = TRUE;
    Get_Message( Field->m_Name, newtext, this );
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    DrawTextField( DrawPanel, DC, Field, flag, g_XorMode );

    if( !newtext.IsEmpty() )
    {
        if( Field->m_Text.IsEmpty() )
        {
            Field->m_Pos    = Cmp->m_Pos;
            Field->m_Size.x = Field->m_Size.y = TextFieldSize;
        }
        Field->m_Text = newtext;
    }
    else    /* Nouveau texte NULL */
    {
        if( FieldNumber == REFERENCE )
        {
            DisplayError( this, _( "Reference needed !, No change" ) );
        }
        else if( FieldNumber == VALUE )
        {
            DisplayError( this, _( "Value needed !, No change" ) );
        }
        else
        {
            Field->m_Text = wxT( "~" );
        }
    }

    DrawTextField( DrawPanel, DC, Field, flag, g_XorMode );
    Cmp->Display_Infos( this );
    m_CurrentScreen->SetModify();
}


/************************************************************************/
static void MoveCmpField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/************************************************************************/

/* Routine de deplacement d'un texte type Field.
 *  Celle routine est normalement attachee au deplacement du curseur
 */
{
    wxPoint pos;
    int     x1, y1;
    int     FieldNumber;

    if( CurrentField == NULL )
        return;

    EDA_SchComponentStruct* Cmp = (EDA_SchComponentStruct*) CurrentField->m_Parent;
    FieldNumber = CurrentField->m_FieldId;

    /* Effacement: */
    if( erase )
        DrawTextField( panel, DC, CurrentField, Multiflag, g_XorMode );

    pos = ( (EDA_SchComponentStruct*) CurrentField->m_Parent )->m_Pos;

    /* Les positions sont caculees par la matrice TRANSPOSEE de la matrice
     *  de rotation-miroir */
    x1 = panel->GetScreen()->m_Curseur.x - pos.x;
    y1 = panel->GetScreen()->m_Curseur.y - pos.y;
    CurrentField->m_Pos.x = pos.x + Cmp->m_Transform[0][0] * x1 + Cmp->m_Transform[1][0] * y1;
    CurrentField->m_Pos.y = pos.y + Cmp->m_Transform[0][1] * x1 + Cmp->m_Transform[1][1] * y1;

    DrawTextField( panel, DC, CurrentField, Multiflag, g_XorMode );
}


/******************************************************************/
static void AbortMoveCmpField( WinEDA_DrawPanel* Panel, wxDC* DC )
/******************************************************************/
{
    Panel->ForceCloseManageCurseur = NULL;
    Panel->ManageCurseur = NULL;
    if( CurrentField )
    {
        DrawTextField( Panel, DC, CurrentField, Multiflag, g_XorMode );
        CurrentField->m_Flags = 0;
        CurrentField->m_Pos   = OldPos;
        DrawTextField( Panel, DC, CurrentField, Multiflag, GR_DEFAULT_DRAWMODE );
    }
    CurrentField = NULL;
    delete g_ItemToUndoCopy; g_ItemToUndoCopy = NULL;
}


/*********************************************************************************/
void WinEDA_SchematicFrame::RotateCmpField( PartTextStruct* Field, wxDC* DC )
/*********************************************************************************/
{
    int FieldNumber, flag;
    EDA_LibComponentStruct* Entry;

    if( Field == NULL )
        return;
    if( Field->m_Text == wxEmptyString )
        return;

    EDA_SchComponentStruct* Cmp = (EDA_SchComponentStruct*) Field->m_Parent;

    FieldNumber = Field->m_FieldId;
    flag = 0;
    if( FieldNumber == REFERENCE )
    {
        Entry = FindLibPart( ( (EDA_SchComponentStruct*) Field->m_Parent )->m_ChipName.GetData(),
                            wxEmptyString, FIND_ROOT );
        if( Entry != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                flag = 1;
        }
    }

    /* save old cmp in undo list if not already in edit, or moving ... */
    if( Field->m_Flags == 0 )
        SaveCopyInUndoList( Cmp, IS_CHANGED );

    DrawTextField( DrawPanel, DC, Field, flag, g_XorMode );

    if( Field->m_Orient == TEXT_ORIENT_HORIZ )
        Field->m_Orient = TEXT_ORIENT_VERT;
    else
        Field->m_Orient = TEXT_ORIENT_HORIZ;
    DrawTextField( DrawPanel, DC, Field, flag, g_XorMode );

    GetScreen()->SetModify();
}


/*********************************************************************/
void PartTextStruct::Place( WinEDA_DrawFrame* frame, wxDC* DC )
/*********************************************************************/
{
    int FieldNumber, flag;
    EDA_LibComponentStruct* Entry;

    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;

    EDA_SchComponentStruct* Cmp = (EDA_SchComponentStruct*) m_Parent;
    /* save old cmp in undo list */
    if( g_ItemToUndoCopy && ( g_ItemToUndoCopy->Type() == Cmp->Type()) )
    {
        Cmp->SwapData( (EDA_SchComponentStruct*) g_ItemToUndoCopy );
        ( (WinEDA_SchematicFrame*) frame )->SaveCopyInUndoList( Cmp, IS_CHANGED );
        Cmp->SwapData( (EDA_SchComponentStruct*) g_ItemToUndoCopy );
    }

    FieldNumber = m_FieldId;
    flag = 0;
    if( FieldNumber == REFERENCE )
    {
        Entry = FindLibPart( Cmp->m_ChipName.GetData(),
                             wxEmptyString, FIND_ROOT );
        if( Entry != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                flag = 1;
        }
    }

    DrawTextField( frame->DrawPanel, DC, this, flag, GR_DEFAULT_DRAWMODE );
    m_Flags = 0;
    frame->GetScreen()->SetCurItem( NULL );
    frame->GetScreen()->SetModify();
    CurrentField = NULL;
}


/**************************************************************************************************/
void WinEDA_SchematicFrame::EditComponentReference( EDA_SchComponentStruct* Cmp, wxDC* DC )
/**************************************************************************************************/
/* Edit the component text reference*/
{
    wxString msg;
    EDA_LibComponentStruct* Entry;
    int      flag = 0;

    if( Cmp == NULL )
        return;

    Entry = FindLibPart( Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return;

    if( Entry->m_UnitCount > 1 )
        flag = 1;

    PartTextStruct* TextField = &Cmp->m_Field[REFERENCE];

    msg = TextField->m_Text;
    Get_Message( _( "Reference" ), msg, this );

    if( !msg.IsEmpty() ) // New text entered
    {
        /* save old cmp in undo list if not already in edit, or moving ... */
        if( Cmp->m_Flags == 0 )
            SaveCopyInUndoList( Cmp, IS_CHANGED );

        DrawTextField( DrawPanel, DC, &Cmp->m_Field[REFERENCE], flag, g_XorMode );
        TextField->m_Text = msg;
        DrawTextField( DrawPanel, DC, &Cmp->m_Field[REFERENCE], flag,
                       Cmp->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
        GetScreen()->SetModify();
    }
    Cmp->Display_Infos( this );
}


/*****************************************************************************************/
void WinEDA_SchematicFrame::EditComponentValue( EDA_SchComponentStruct* Cmp, wxDC* DC )
/*****************************************************************************************/
/* Routine de changement du texte selectionne */
{
    wxString msg;
    EDA_LibComponentStruct* Entry;
    int      flag = 0;

    if( Cmp == NULL )
        return;

    Entry = FindLibPart( Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return;
    if( Entry->m_UnitCount > 1 )
        flag = 1;

    PartTextStruct* TextField = &Cmp->m_Field[VALUE];

    msg = TextField->m_Text;
    Get_Message( _( "Value" ), msg, this );

    if( !msg.IsEmpty() )
    {
        /* save old cmp in undo list if not already in edit, or moving ... */
        if( Cmp->m_Flags == 0 )
            SaveCopyInUndoList( Cmp, IS_CHANGED );

        DrawTextField( DrawPanel, DC, &Cmp->m_Field[VALUE], flag, g_XorMode );
        TextField->m_Text = msg;
        DrawTextField( DrawPanel, DC, &Cmp->m_Field[VALUE], flag,
                       Cmp->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
        m_CurrentScreen->SetModify();
    }

    Cmp->Display_Infos( this );
}


/*****************************************************************************/
void WinEDA_ComponentPropertiesFrame::SetInitCmp( wxCommandEvent& event )
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
    m_Cmp->m_Field[REFERENCE].m_Pos.x =
        Entry->m_Prefix.m_Pos.x + m_Cmp->m_Pos.x;
    m_Cmp->m_Field[REFERENCE].m_Pos.y =
        Entry->m_Prefix.m_Pos.y + m_Cmp->m_Pos.y;
    m_Cmp->m_Field[REFERENCE].m_Orient   = Entry->m_Prefix.m_Orient;
    m_Cmp->m_Field[REFERENCE].m_Size     = Entry->m_Prefix.m_Size;
    m_Cmp->m_Field[REFERENCE].m_HJustify = Entry->m_Prefix.m_HJustify;
    m_Cmp->m_Field[REFERENCE].m_VJustify = Entry->m_Prefix.m_VJustify;

    m_Cmp->m_Field[VALUE].m_Pos.x =
        Entry->m_Name.m_Pos.x + m_Cmp->m_Pos.x;
    m_Cmp->m_Field[VALUE].m_Pos.y =
        Entry->m_Name.m_Pos.y + m_Cmp->m_Pos.y;
    m_Cmp->m_Field[VALUE].m_Orient   = Entry->m_Name.m_Orient;
    m_Cmp->m_Field[VALUE].m_Size     = Entry->m_Name.m_Size;
    m_Cmp->m_Field[VALUE].m_HJustify = Entry->m_Name.m_HJustify;
    m_Cmp->m_Field[VALUE].m_VJustify = Entry->m_Name.m_VJustify;

    m_Cmp->SetRotationMiroir( CMP_NORMAL );

    m_Parent->m_CurrentScreen->SetModify();

    RedrawOneStruct( m_Parent->DrawPanel, &dc, m_Cmp, GR_DEFAULT_DRAWMODE );
    Close();
}
