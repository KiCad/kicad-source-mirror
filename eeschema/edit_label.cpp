/*********************************************************************/
/* EESchema											  				 */
/* edit_label.cpp: label, global label and text creation or edition  */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

/* Fonctions locales */
static void ShowWhileMoving( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void ExitMoveTexte( WinEDA_DrawPanel* panel, wxDC* DC );

/* Variables locales */
static wxPoint ItemInitialPosition;
static int     OldOrient;
static wxSize  OldSize;
static int     s_DefaultShapeGLabel  = (int) NET_INPUT;
static int     s_DefaultOrientGLabel = 0;

/************************************/
/* class WinEDA_LabelPropertiesFrame */
/************************************/

#include "dialog_edit_label.cpp"


/****************************************************************************/
void WinEDA_LabelPropertiesFrame::TextPropertiesAccept( wxCommandEvent& event )
/****************************************************************************/
{
    wxString text;
    int      value;

    /* save old text in undo list if not already in edit */
    if( m_CurrentText->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_CurrentText, IS_CHANGED );

    text = m_TextLabel->GetValue();
    if( !text.IsEmpty() )
        m_CurrentText->m_Text = text;
    else if( (m_CurrentText->m_Flags & IS_NEW) == 0 )
        DisplayError( this, _( "Empty Text!" ) );

    m_CurrentText->m_Orient = m_TextOrient->GetSelection();
    text  = m_TextSize->GetValue();
    value = ReturnValueFromString( g_UnitMetric, text, m_Parent->m_InternalUnits );
    m_CurrentText->m_Size.x = m_CurrentText->m_Size.y = value;
    if( m_TextShape )
        m_CurrentText->m_Shape = m_TextShape->GetSelection();

    m_Parent->GetScreen()->SetModify();

    /* Make the text size as new default size if it is a new text */
    if( (m_CurrentText->m_Flags & IS_NEW) != 0 )
        g_DefaultTextLabelSize = m_CurrentText->m_Size.x;

    Close( TRUE );
}


/********************************************************************************/
void WinEDA_SchematicFrame::StartMoveTexte( DrawTextStruct* TextStruct, wxDC* DC )
/********************************************************************************/
{
    if( TextStruct == NULL )
        return;

    g_ItemToRepeat = NULL;

    if( (TextStruct->m_Flags & IS_NEW) == 0 )
    {
        delete g_ItemToUndoCopy;
        g_ItemToUndoCopy = TextStruct->GenCopy();
    }

    TextStruct->m_Flags |= IS_MOVED;

    switch( TextStruct->Type() )
    {
    case DRAW_LABEL_STRUCT_TYPE:
    case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
    case DRAW_TEXT_STRUCT_TYPE:
        ItemInitialPosition = TextStruct->m_Pos;
        OldSize   = TextStruct->m_Size;
        OldOrient = TextStruct->m_Orient;
        break;

    default:
        break;
    }

    DrawPanel->CursorOff( DC );
    m_CurrentScreen->m_Curseur = ItemInitialPosition;
    DrawPanel->MouseToCursorSchema();

    GetScreen()->SetModify();
    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitMoveTexte;
	GetScreen()->SetCurItem( TextStruct );
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );

    DrawPanel->CursorOn( DC );
}


/*************************************************************************/
void WinEDA_SchematicFrame::EditSchematicText( DrawTextStruct* TextStruct,
                                               wxDC*           DC )
/*************************************************************************/

/* Edit the properties of the text (Label, Gloab label, graphic text).. )
 *  pointed by "TextStruct"
 */
{
    if( TextStruct == NULL )
        return;

    DrawPanel->CursorOff( DC );
    RedrawOneStruct( DrawPanel, DC, TextStruct, g_XorMode );

    WinEDA_LabelPropertiesFrame* frame = new WinEDA_LabelPropertiesFrame( this,
                                                                         TextStruct,
                                                                         wxPoint( 30, 30 ) );
    frame->ShowModal(); frame->Destroy();

    RedrawOneStruct( DrawPanel, DC, TextStruct, GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );
}


/***********************************************************************************/
void WinEDA_SchematicFrame::ChangeTextOrient( DrawTextStruct* TextStruct, wxDC* DC )
/***********************************************************************************/
{
    if( TextStruct == NULL )
        TextStruct = (DrawTextStruct*) PickStruct( GetScreen()->m_Curseur,
                                                   GetScreen()->EEDrawList, TEXTITEM | LABELITEM );
    if( TextStruct == NULL )
        return;

    /* save old text in undo list if is not already in edit */
    if( TextStruct->m_Flags == 0 )
        SaveCopyInUndoList( TextStruct, IS_CHANGED );

    /* Effacement du texte en cours */
    DrawPanel->CursorOff( DC );
    RedrawOneStruct( DrawPanel, DC, TextStruct, g_XorMode );

    /* Rotation du texte */
    switch( TextStruct->Type() )
    {
    case DRAW_LABEL_STRUCT_TYPE:
    case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
    case DRAW_TEXT_STRUCT_TYPE:
        TextStruct->m_Orient++;
        TextStruct->m_Orient &= 3;
        break;

    default:
        break;
    }

    GetScreen()->SetModify();

    /* Reaffichage */
    RedrawOneStruct( DrawPanel, DC, TextStruct, g_XorMode );
    DrawPanel->CursorOn( DC );
}


/*************************************************************************/
EDA_BaseStruct* WinEDA_SchematicFrame::CreateNewText( wxDC* DC, int type )
/*************************************************************************/

/* Routine to create new text struct (GraphicText, label or Glabel).
 */
{
    DrawTextStruct* NewText = NULL;

    g_ItemToRepeat = NULL;

    switch( type )
    {
    case LAYER_NOTES:
        NewText = new DrawTextStruct( m_CurrentScreen->m_Curseur );
        break;

    case LAYER_LOCLABEL:
        NewText = new DrawLabelStruct( m_CurrentScreen->m_Curseur );
        break;

    case LAYER_GLOBLABEL:
        NewText = new DrawGlobalLabelStruct( m_CurrentScreen->m_Curseur );
        NewText->m_Shape  = s_DefaultShapeGLabel;
        NewText->m_Orient = s_DefaultOrientGLabel;
        break;

    default:
        DisplayError( this, wxT( "WinEDA_SchematicFrame::CreateNewText() Internal error" ) );
        return NULL;
    }

    NewText->m_Size.x = NewText->m_Size.y = g_DefaultTextLabelSize;
    NewText->m_Flags  = IS_NEW | IS_MOVED;

    RedrawOneStruct( DrawPanel, DC, NewText, g_XorMode );
    EditSchematicText( NewText, DC );

    if( NewText->m_Text.IsEmpty() )
    {
        delete NewText;
        return NULL;
    }

    if( type == LAYER_GLOBLABEL )
    {
        s_DefaultShapeGLabel  = NewText->m_Shape;
        s_DefaultOrientGLabel = NewText->m_Orient;
    }

    RedrawOneStruct( DrawPanel, DC, NewText, GR_DEFAULT_DRAWMODE );
    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitMoveTexte;

    m_CurrentScreen->SetCurItem( NewText );

    return NewText;
}


/************************************/
/*		Redraw a Texte while moving	*/
/************************************/
static void ShowWhileMoving( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    EDA_BaseStruct* TextStruct = panel->GetScreen()->GetCurItem();

    /* effacement ancienne position */
    if( erase )
        RedrawOneStruct( panel, DC, TextStruct, g_XorMode );

    /* Redessin du texte */
    switch( TextStruct->Type() )
    {
    case DRAW_LABEL_STRUCT_TYPE:
    case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
    case DRAW_TEXT_STRUCT_TYPE:
        ( (DrawTextStruct*) TextStruct )->m_Pos = panel->GetScreen()->m_Curseur;
        break;

    default:
        break;
    }

    RedrawOneStruct( panel, DC, TextStruct, g_XorMode );
}


/*************************************************************/
static void ExitMoveTexte( WinEDA_DrawPanel* Panel, wxDC* DC )
/*************************************************************/
/* Routine de sortie des menus de Texte */
{
    SCH_SCREEN*     screen = (SCH_SCREEN*) Panel->m_Parent->m_CurrentScreen;
    EDA_BaseStruct* Struct = screen->GetCurItem();

    g_ItemToRepeat = NULL;
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;

    if( Struct == NULL )  /* Pas de trace en cours  */
    {
        return;
    }

    /* ici : trace en cours */

    /* Effacement du trace en cours et suppression eventuelle de la structure */
    RedrawOneStruct( Panel, DC, Struct, g_XorMode );

    if( Struct->m_Flags & IS_NEW )  /* Suppression du nouveau texte en cours de placement */
    {
        delete Struct;
        screen->SetCurItem( NULL );
    }
    else    /* Remise a jour des anciens parametres du texte */
    {
        switch( Struct->Type() )
        {
        case DRAW_LABEL_STRUCT_TYPE:
        case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
        case DRAW_TEXT_STRUCT_TYPE:
        {
            DrawTextStruct* Text = (DrawTextStruct*) Struct;
            Text->m_Pos    = ItemInitialPosition;
            Text->m_Size   = OldSize;
            Text->m_Orient = OldOrient;
        }
            break;

        default:
            break;
        }

        RedrawOneStruct( Panel, DC, Struct, GR_DEFAULT_DRAWMODE );
        Struct->m_Flags = 0;
    }
}


/*****************************************************************************/
void WinEDA_SchematicFrame::ConvertTextType( DrawTextStruct* Text,
                                             wxDC* DC, int newtype )
/*****************************************************************************/

/* Routine to change a text type to an other one (GraphicText, label or Glabel).
 * A new test, label or global label is created from the old text.
 * the old text is deleted
 */
{
    if( Text == NULL )
        return;

    DrawTextStruct* newtext;

    switch( newtype )
    {
    case DRAW_LABEL_STRUCT_TYPE:
        newtext = new DrawLabelStruct( Text->m_Pos, Text->m_Text );
        break;

    case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
        newtext = new DrawGlobalLabelStruct( Text->m_Pos, Text->m_Text );
        break;

    case DRAW_TEXT_STRUCT_TYPE:
        newtext = new DrawTextStruct( Text->m_Pos, Text->m_Text );
        break;

    default:
        newtext = NULL;
        DisplayError( this, wxT( "ConvertTextType: Internal error" ) );
        return;
    }

    /* copy the old text settings */
    newtext->m_Shape      = Text->m_Shape;
    newtext->m_Orient     = Text->m_Orient;
    newtext->m_Size       = Text->m_Size;
    newtext->m_Width      = Text->m_Width;
    newtext->m_HJustify   = Text->m_HJustify;
    newtext->m_VJustify   = Text->m_VJustify;
    newtext->m_IsDangling = Text->m_IsDangling;

    // save current text flag:
    int flags = Text->m_Flags;

    /* add the new text in linked list if old text is in list */
    if( (flags & IS_NEW) == 0 )
    {
        newtext->Pnext = GetScreen()->EEDrawList;
        GetScreen()->EEDrawList = newtext;
        GetScreen()->SetModify();
    }

    /* now delete the old text
     *  If it is a text flagged IS_NEW it will be deleted by ForceCloseManageCurseur()
     *  If not, we must delete it.
     */
    if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
    {
        DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );
    }
    if( (flags & IS_NEW) == 0 )    // Delete old text and save it in undo list
    {
        Text->m_Flags = 0;
        DeleteStruct( DrawPanel, DC, Text );
        m_CurrentScreen->SetCurItem( NULL );
        g_ItemToRepeat = NULL;
    }
    GetScreen()->SetCurItem( newtext );

    delete g_ItemToUndoCopy;
    g_ItemToUndoCopy = NULL;

    DrawPanel->CursorOff( DC );   // Erase schematic cursor

    /* Save the new text in undo list if the old text was not itself a "new created text"
     * In this case, the old text is already in undo list as a deleted item
     * Of course if the old text was a "new created text" the new text will be put in undo list
     * later, at the end of the current command (if not aborted)
     */
    if( (flags & IS_NEW) == 0 )
    {
        SaveCopyInUndoList( newtext, IS_NEW );
    }
    else
        newtext->m_Flags = IS_NEW;


    if( (flags & IS_MOVED) != 0 )
    {
        StartMoveTexte( newtext, DC );
    }

    RedrawOneStruct( DrawPanel, DC, newtext, GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );    // redraw schematic cursor
}
