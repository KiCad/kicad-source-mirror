/********************************/
/* Scehematic component edition */
/********************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/* Fonctions locales */
static void AbortMoveCmpField( WinEDA_DrawPanel* Panel, wxDC* DC );
static void MoveCmpField( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


/************************************************************************************/
void WinEDA_SchematicFrame::StartMoveCmpField( SCH_CMP_FIELD* aField, wxDC* DC )
/************************************************************************************/

/* Prepare le deplacement du texte en cours d'edition
 */
{
    EDA_LibComponentStruct* Entry;

    SetCurrentField( aField );
    if( aField == NULL )
        return;

    if( aField->m_Text == wxEmptyString )
    {
        DisplayError( this, _( "No Field to move" ), 10 );
        return;
    }

    wxPoint pos, newpos;
    SCH_COMPONENT* comp = (SCH_COMPONENT*) aField->GetParent();

    SAFE_DELETE( g_ItemToUndoCopy );
    g_ItemToUndoCopy = comp->GenCopy();

    pos = comp->m_Pos;

    /* Les positions sont calculees par la matrice TRANSPOSEE de la matrice
     *  de rotation-miroir */
    newpos = aField->m_Pos - pos;

    // Empirically this is necessary.  The Y coordinate appears to be inverted
    // under some circumstances, but that inversion is not preserved by all
    // combinations of mirroring and rotation.  The following clause is true
    // when the number of rotations and the number of mirrorings are both odd.
    if( comp->m_Transform[1][0] * comp->m_Transform[0][1] < 0 )
        NEGATE (newpos.y);

    newpos = TransformCoordinate( comp->m_Transform, newpos) + pos;

    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = newpos;
    DrawPanel->MouseToCursorSchema();

    m_OldPos    = aField->m_Pos;
    m_Multiflag = 0;
    if( aField->m_FieldId == REFERENCE )
    {
        Entry = FindLibPart( comp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry  != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                m_Multiflag = 1;
        }
    }

    DrawPanel->ForceCloseManageCurseur = AbortMoveCmpField;
    DrawPanel->ManageCurseur = MoveCmpField;
    aField->m_Flags = IS_MOVED;

    DrawPanel->CursorOn( DC );
}


/**********************************************************************************/
void WinEDA_SchematicFrame::EditCmpFieldText( SCH_CMP_FIELD* Field, wxDC* DC )
/**********************************************************************************/
/* Edit the field Field (text, size)  */
{
    int fieldNdx, flag;
    EDA_LibComponentStruct* Entry;

    if( Field == NULL )
    {
        DisplayError( this, _( "No Field To Edit" ), 10 );
        return;
    }

    SCH_COMPONENT* Cmp = (SCH_COMPONENT*) Field->GetParent();

    fieldNdx = Field->m_FieldId;
    if( fieldNdx == VALUE )
    {
        Entry = FindLibPart( Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
        if( Entry && (Entry->m_Options == ENTRY_POWER) )
        {
            DisplayInfoMessage( this,
                        _(
                            "Part is a POWER, value cannot be modified!\nYou must create a new power" )
                         );
            return;
        }
    }

    flag = 0;
    if( fieldNdx == REFERENCE )
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
        SaveCopyInUndoList( Cmp, UR_CHANGED );

    wxString newtext = Field->m_Text;
    DrawPanel->m_IgnoreMouseEvents = TRUE;
    Get_Message( Field->m_Name, _("Component field text"), newtext, this );
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;

    Field->m_AddExtraText = flag;
    Field->Draw( DrawPanel, DC, wxPoint(0,0), g_XorMode );

    if( !newtext.IsEmpty() )
    {
        if( Field->m_Text.IsEmpty() )
        {
            Field->m_Pos    = Cmp->m_Pos;
            Field->m_Size.x = Field->m_Size.y = m_TextFieldSize;
        }
        Field->m_Text = newtext;
        if( fieldNdx == REFERENCE ){
            Cmp->SetRef(GetSheet(), newtext);
        }
    }
    else    /* Nouveau texte NULL */
    {
        if( fieldNdx == REFERENCE )
        {
            DisplayError( this, _( "Reference needed !, No change" ) );
        }
        else if( fieldNdx == VALUE )
        {
            DisplayError( this, _( "Value needed !, No change" ) );
        }
        else
        {
            Field->m_Text = wxT( "~" );
        }
    }

    Field->Draw( DrawPanel, DC, wxPoint(0,0), g_XorMode );
    Cmp->DisplayInfo( this );
    GetScreen()->SetModify();
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
    int     fieldNdx;

    WinEDA_SchematicFrame*  frame = (WinEDA_SchematicFrame*) panel->GetParent();
    SCH_CMP_FIELD*          currentField = frame->GetCurrentField();

    if( currentField == NULL )
        return;

    SCH_COMPONENT* component = (SCH_COMPONENT*) currentField->GetParent();
    fieldNdx = currentField->m_FieldId;

    // Effacement:
    currentField->m_AddExtraText = frame->m_Multiflag;
    if( erase )
    {
        currentField->Draw( panel, DC, wxPoint(0,0), g_XorMode );
    }

    pos = ( (SCH_COMPONENT*) currentField->GetParent() )->m_Pos;

    /* Les positions sont caculees par la matrice TRANSPOSEE de la matrice
     * de rotation-miroir
     */
    x1 = panel->GetScreen()->m_Curseur.x - pos.x;
    y1 = panel->GetScreen()->m_Curseur.y - pos.y;

    currentField->m_Pos.x = pos.x + component->m_Transform[0][0] * x1 + component->m_Transform[1][0] * y1;
    currentField->m_Pos.y = pos.y + component->m_Transform[0][1] * x1 + component->m_Transform[1][1] * y1;

    currentField->Draw( panel, DC, wxPoint(0,0), g_XorMode );
}


/******************************************************************/
static void AbortMoveCmpField( WinEDA_DrawPanel* Panel, wxDC* DC )
/******************************************************************/
{
    Panel->ForceCloseManageCurseur = NULL;
    Panel->ManageCurseur = NULL;

    WinEDA_SchematicFrame*  frame = (WinEDA_SchematicFrame*) Panel->GetParent();
    SCH_CMP_FIELD*          currentField = frame->GetCurrentField();

    if( currentField )
    {
        currentField->m_AddExtraText = frame->m_Multiflag;
        currentField->Draw( Panel, DC, wxPoint(0,0), g_XorMode );
        currentField->m_Flags = 0;
        currentField->m_Pos   = frame->m_OldPos;
        currentField->Draw( Panel, DC, wxPoint(0,0), g_XorMode );
    }

    frame->SetCurrentField( NULL );

    SAFE_DELETE( g_ItemToUndoCopy );
}


/*********************************************************************************/
void WinEDA_SchematicFrame::RotateCmpField( SCH_CMP_FIELD* Field, wxDC* DC )
/*********************************************************************************/
{
    int fieldNdx, flag;
    EDA_LibComponentStruct* Entry;

    if( Field == NULL )
        return;
    if( Field->m_Text == wxEmptyString )
        return;

    SCH_COMPONENT* Cmp = (SCH_COMPONENT*) Field->GetParent();

    fieldNdx = Field->m_FieldId;
    flag = 0;
    if( fieldNdx == REFERENCE )
    {
        Entry = FindLibPart( ( (SCH_COMPONENT*) Field->GetParent() )->m_ChipName.GetData(),
                            wxEmptyString, FIND_ROOT );
        if( Entry != NULL )
        {
            if( Entry->m_UnitCount > 1 )
                flag = 1;
        }
    }

    /* save old cmp in undo list if not already in edit, or moving ... */
    if( Field->m_Flags == 0 )
        SaveCopyInUndoList( Cmp, UR_CHANGED );

    Field->m_AddExtraText = flag;
    Field->Draw( DrawPanel, DC, wxPoint(0,0), g_XorMode );

    if( Field->m_Orient == TEXT_ORIENT_HORIZ )
        Field->m_Orient = TEXT_ORIENT_VERT;
    else
        Field->m_Orient = TEXT_ORIENT_HORIZ;
    Field->Draw( DrawPanel, DC, wxPoint(0,0), g_XorMode );

    GetScreen()->SetModify();
}


/**************************************************************************************************/
void WinEDA_SchematicFrame::EditComponentReference( SCH_COMPONENT* Cmp, wxDC* DC )
/**************************************************************************************************/
/* Edit the component text reference*/
{
    EDA_LibComponentStruct* Entry;
    int      flag = 0;

    if( Cmp == NULL )
        return;

    Entry = FindLibPart( Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return;

    if( Entry->m_UnitCount > 1 )
        flag = 1;

    wxString ref = Cmp->GetRef(GetSheet());
    Get_Message( _( "Reference" ), _("Component reference"), ref, this );

    if( !ref.IsEmpty() ) // New text entered
    {
        /* save old cmp in undo list if not already in edit, or moving ... */
        if( Cmp->m_Flags == 0 )
            SaveCopyInUndoList( Cmp, UR_CHANGED );
        Cmp->SetRef(GetSheet(), ref);

        Cmp->GetField( REFERENCE )->m_AddExtraText = flag;
        Cmp->GetField( REFERENCE )->Draw( DrawPanel, DC, wxPoint(0,0), g_XorMode );
        Cmp->SetRef(GetSheet(), ref );
        Cmp->GetField( REFERENCE )->Draw( DrawPanel, DC, wxPoint(0,0),
                       Cmp->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
        GetScreen()->SetModify();
    }
    Cmp->DisplayInfo( this );
}


/*****************************************************************************************/
void WinEDA_SchematicFrame::EditComponentValue( SCH_COMPONENT* Cmp, wxDC* DC )
/*****************************************************************************************/
/* Routine de changement du texte selectionne */
{
    wxString message;
    EDA_LibComponentStruct* Entry;

    if( Cmp == NULL )
        return;

    Entry = FindLibPart( Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return;

    SCH_CMP_FIELD* TextField = Cmp->GetField( VALUE );

    message = TextField->m_Text;
    if( Get_Message( _( "Value" ), _("Component value"), message, this ) )
        message.Empty(); //allow the user to remove the value.

    if( !message.IsEmpty() && !message.IsEmpty())
    {
        /* save old cmp in undo list if not already in edit, or moving ... */
        if( Cmp->m_Flags == 0 )
            SaveCopyInUndoList( Cmp, UR_CHANGED );

        TextField->Draw( DrawPanel, DC, wxPoint(0,0), g_XorMode );
        TextField->m_Text = message;
        TextField->Draw( DrawPanel, DC, wxPoint(0,0),
                       Cmp->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
        GetScreen()->SetModify();
    }

    Cmp->DisplayInfo( this );
}


/*****************************************************************************************/
void WinEDA_SchematicFrame::EditComponentFootprint( SCH_COMPONENT* Cmp, wxDC* DC )
/*****************************************************************************************/
{
    wxString message;
    EDA_LibComponentStruct* Entry;
    bool wasEmpty = false;

    if( Cmp == NULL )
        return;

    Entry = FindLibPart( Cmp->m_ChipName.GetData(), wxEmptyString, FIND_ROOT );
    if( Entry == NULL )
        return;

    SCH_CMP_FIELD* TextField = Cmp->GetField( FOOTPRINT );

    message = TextField->m_Text;
    if(message.IsEmpty() )
        wasEmpty = true;

    if( Get_Message( _( "Footprint" ), _("Component footprint"), message, this ) )
        message.Empty();    // allow the user to remove the value.

    // save old cmp in undo list if not already in edit, or moving ...
    if( Cmp->m_Flags == 0 )
        SaveCopyInUndoList( Cmp, UR_CHANGED );
    Cmp->GetField( FOOTPRINT )->Draw( DrawPanel, DC, wxPoint(0,0), g_XorMode );

    // move the field if it was new.
    if( wasEmpty && !message.IsEmpty() )
    {
        Cmp->GetField( FOOTPRINT )->m_Pos = Cmp->GetField( REFERENCE )->m_Pos;

        // add offset here - ? suitable heuristic below?
        Cmp->GetField( FOOTPRINT )->m_Pos.x +=
            (Cmp->GetField( REFERENCE )->m_Pos.x - Cmp->m_Pos.x) > 0 ?
            (Cmp->GetField( REFERENCE )->m_Size.x) : (-1*Cmp->GetField( REFERENCE )->m_Size.x);

        Cmp->GetField( FOOTPRINT )->m_Pos.y +=
            (Cmp->GetField( REFERENCE )->m_Pos.y - Cmp->m_Pos.y) > 0 ?
            (Cmp->GetField( REFERENCE )->m_Size.y) : (-1*Cmp->GetField( REFERENCE )->m_Size.y);

        Cmp->GetField( FOOTPRINT )->m_Orient = Cmp->GetField( REFERENCE )->m_Orient;
    }
    TextField->m_Text = message;

    Cmp->GetField( FOOTPRINT )->Draw( DrawPanel, DC, wxPoint(0,0),
                   Cmp->m_Flags ? g_XorMode : GR_DEFAULT_DRAWMODE );
    GetScreen()->SetModify();

    Cmp->DisplayInfo( this );
}

