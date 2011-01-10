/*********************************************************************/
/* EESchema											                 */
/* edit_label.cpp: label, global label and text creation or edition  */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "base_struct.h"
#include "drawtxt.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_text.h"


static void ShowWhileMoving( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void ExitMoveTexte( WinEDA_DrawPanel* panel, wxDC* DC );


static wxPoint ItemInitialPosition;
static int     OldOrient;
static wxSize  OldSize;
static int     lastGlobalLabelShape = (int) NET_INPUT;
static int     lastTextOrientation = 0;
static bool    lastTextBold = false;
static bool    lastTextItalic = false;


void SCH_EDIT_FRAME::StartMoveTexte( SCH_TEXT* TextStruct, wxDC* DC )
{
    if( TextStruct == NULL )
        return;

    g_ItemToRepeat = NULL;

    if( (TextStruct->m_Flags & IS_NEW) == 0 )
    {
        delete g_ItemToUndoCopy;
        g_ItemToUndoCopy = TextStruct->Clone();
    }

    TextStruct->m_Flags |= IS_MOVED;

    switch( TextStruct->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
        ItemInitialPosition = TextStruct->m_Pos;
        OldSize   = TextStruct->m_Size;
        OldOrient = TextStruct->GetSchematicTextOrientation();
        break;

    default:
        break;
    }

    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = ItemInitialPosition;
    DrawPanel->MouseToCursorSchema();

    OnModify( );
    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitMoveTexte;
    GetScreen()->SetCurItem( TextStruct );
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );

    DrawPanel->CursorOn( DC );
}


void SCH_EDIT_FRAME::ChangeTextOrient( SCH_TEXT* TextStruct, wxDC* DC )
{
    if( TextStruct == NULL )
        TextStruct = (SCH_TEXT*) PickStruct( GetScreen()->m_Curseur,
                                             GetScreen(), TEXTITEM | LABELITEM );
    if( TextStruct == NULL )
        return;

    /* save old text in undo list if is not already in edit */
    if( TextStruct->m_Flags == 0 )
        SaveCopyInUndoList( TextStruct, UR_CHANGED );

    /* Erase old text */
    DrawPanel->CursorOff( DC );
    TextStruct->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

    int orient;

    switch( TextStruct->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
        orient  = TextStruct->GetSchematicTextOrientation() + 1;
        orient &= 3;
        TextStruct->SetSchematicTextOrientation( orient );
        break;

    default:
        break;
    }

    OnModify( );
    TextStruct->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
    DrawPanel->CursorOn( DC );
}


/* Routine to create new text struct (GraphicText, label or Glabel).
 */
SCH_TEXT* SCH_EDIT_FRAME::CreateNewText( wxDC* DC, int type )
{
    SCH_TEXT* NewText = NULL;

    g_ItemToRepeat = NULL;

    switch( type )
    {
    case LAYER_NOTES:
        NewText = new SCH_TEXT( GetScreen()->m_Curseur );
        break;

    case LAYER_LOCLABEL:
        NewText = new SCH_LABEL( GetScreen()->m_Curseur );
        break;

    case LAYER_HIERLABEL:
        NewText = new SCH_HIERLABEL( GetScreen()->m_Curseur );
        NewText->m_Shape = lastGlobalLabelShape;
        break;

    case LAYER_GLOBLABEL:
        NewText = new SCH_GLOBALLABEL( GetScreen()->m_Curseur );
        NewText->m_Shape = lastGlobalLabelShape;
        break;

    default:
        DisplayError( this, wxT( "SCH_EDIT_FRAME::CreateNewText() Internal error" ) );
        return NULL;
    }

    NewText->m_Bold = lastTextBold;
    NewText->m_Italic = lastTextItalic;
    NewText->SetSchematicTextOrientation( lastTextOrientation );
    NewText->m_Size.x = NewText->m_Size.y = g_DefaultTextLabelSize;
    NewText->m_Flags  = IS_NEW | IS_MOVED;

    NewText->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
    EditSchematicText( NewText );

    if( NewText->m_Text.IsEmpty() )
    {
        SAFE_DELETE( NewText );
        return NULL;
    }

    lastTextBold = NewText->m_Bold;
    lastTextItalic = NewText->m_Italic;
    lastTextOrientation = NewText->GetSchematicTextOrientation();

    if( type == LAYER_GLOBLABEL  || type == LAYER_HIERLABEL )
    {
        lastGlobalLabelShape = NewText->m_Shape;
    }

    NewText->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitMoveTexte;

    GetScreen()->SetCurItem( NewText );

    return NewText;
}


/************************************/
/*		Redraw a Text while moving	*/
/************************************/
static void ShowWhileMoving( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    SCH_ITEM* TextStruct = (SCH_ITEM*) panel->GetScreen()->GetCurItem();

    /* "Undraw" the current text at its old position*/
    if( erase )
        TextStruct->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode );

    /* redraw the text */
    switch( TextStruct->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
        ( (SCH_TEXT*) TextStruct )->m_Pos = panel->GetScreen()->m_Curseur;
        break;

    default:
        break;
    }

    TextStruct->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode );
}


/* Abort function for the command move text */
static void ExitMoveTexte( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    BASE_SCREEN* screen = Panel->GetScreen();
    SCH_ITEM*    Struct = (SCH_ITEM*) screen->GetCurItem();

    g_ItemToRepeat = NULL;
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;

    if( Struct == NULL )  /* no current item */
    {
        return;
    }

    /* "Undraw" the text, and delete it if new (i.e. it was being just
     * created)*/
    Struct->Draw( Panel, DC, wxPoint( 0, 0 ), g_XorMode );

    if( Struct->m_Flags & IS_NEW )
    {
        SAFE_DELETE( Struct );
        screen->SetCurItem( NULL );
    }
    else /* this was a move command on "old" text: restore its old settings. */
    {
        switch( Struct->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
        case SCH_TEXT_T:
        {
            SCH_TEXT* Text = (SCH_TEXT*) Struct;
            Text->m_Pos  = ItemInitialPosition;
            Text->m_Size = OldSize;
            Text->SetSchematicTextOrientation( OldOrient );
        }
        break;

        default:
            break;
        }

        Struct->Draw( Panel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        Struct->m_Flags = 0;
    }
}


/* Routine to change a text type to an other one (GraphicText, label or Glabel).
 * A new test, label or hierarchical or global label is created from the old text.
 * the old text is deleted
 */
void SCH_EDIT_FRAME::ConvertTextType( SCH_TEXT* Text, wxDC* DC, int newtype )
{
    if( Text == NULL )
        return;

    SCH_TEXT* newtext;

    switch( newtype )
    {
    case SCH_LABEL_T:
        newtext = new SCH_LABEL( Text->m_Pos, Text->m_Text );
        break;

    case SCH_GLOBAL_LABEL_T:
        newtext = new SCH_GLOBALLABEL( Text->m_Pos, Text->m_Text );
        break;

    case SCH_HIERARCHICAL_LABEL_T:
        newtext = new SCH_HIERLABEL( Text->m_Pos, Text->m_Text );
        break;

    case SCH_TEXT_T:
        newtext = new SCH_TEXT( Text->m_Pos, Text->m_Text );
        break;

    default:
        newtext = NULL;
        DisplayError( this, wxT( "ConvertTextType: Internal error" ) );
        return;
    }

    /* copy the old text settings
     * Justifications are not copied because they are not used in labels,
     *  and can be used in texts
     *  So they will be set to default in conversion.
     */
    newtext->m_Shape = Text->m_Shape;
    newtext->SetSchematicTextOrientation( Text->GetSchematicTextOrientation() );
    newtext->m_Size   = Text->m_Size;
    newtext->m_Thickness  = Text->m_Thickness;
    newtext->m_Italic = Text->m_Italic;
    newtext->m_Bold   = Text->m_Bold;


    // save current text flag:
    int flags = Text->m_Flags;

    /* add the new text in linked list if old text is in list */
    if( (flags & IS_NEW) == 0 )
    {
        newtext->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( newtext );
        OnModify();
    }

    /* now delete the old text
     *  If it is a text flagged IS_NEW it will be deleted by ForceCloseManageCurseur()
     *  If not, we must delete it.
     */
    if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
    {
        DrawPanel->ForceCloseManageCurseur( DrawPanel, DC );
    }

    if( (flags & IS_NEW) == 0 )    // Remove old text from current list and
                                   // save it in undo list
    {
        Text->m_Flags = 0;
        DeleteStruct( DrawPanel, DC, Text );    // old text is really saved in
                                                // undo list
        GetScreen()->SetCurItem( NULL );
        g_ItemToRepeat = NULL;
    }

    GetScreen()->SetCurItem( NULL );

    delete g_ItemToUndoCopy;
    g_ItemToUndoCopy = NULL;

    DrawPanel->CursorOff( DC );   // Erase schematic cursor

    /* Save the new text in undo list if the old text was not itself a "new created text"
     * In this case, the old text is already in undo list as a deleted item.
     * Of course if the old text was a "new created text" the new text will be
     * put in undo list later, at the end of the current command (if not aborted)
     */
    if( (flags & IS_NEW) == 0 )
    {
        SaveCopyInUndoList( newtext, UR_NEW );
    }
    else
    {
        GetScreen()->SetCurItem( newtext );
        newtext->m_Flags = IS_NEW;
    }


    if( (flags & IS_MOVED) != 0 )
    {
        GetScreen()->SetCurItem( newtext );
        StartMoveTexte( newtext, DC );
    }

    newtext->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );    // redraw schematic cursor
}
