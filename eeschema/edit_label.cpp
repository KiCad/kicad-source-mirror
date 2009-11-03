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

#include "program.h"
#include "general.h"
#include "protos.h"

#include "dialog_edit_label.h"


static void ShowWhileMoving( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void ExitMoveTexte( WinEDA_DrawPanel* panel, wxDC* DC );


static wxPoint ItemInitialPosition;
static int     OldOrient;
static wxSize  OldSize;
static int     s_DefaultShapeGLabel  = (int) NET_INPUT;
static int     s_DefaultOrientGLabel = 0;


/****************************************************************************/
void DialogLabelEditor::TextPropertiesAccept( wxCommandEvent& event )
{
/****************************************************************************/
    wxString text;
    int      value;

    /* save old text in undo list if not already in edit */
    if( m_CurrentText->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_CurrentText, UR_CHANGED );

    text = m_TextLabel->GetValue();
    if( !text.IsEmpty() )
        m_CurrentText->m_Text = text;
    else if( (m_CurrentText->m_Flags & IS_NEW) == 0 )
        DisplayError( this, _( "Empty Text!" ) );

    m_CurrentText->SetSchematicTextOrientation( m_TextOrient->GetSelection() );
    text  = m_TextSize->GetValue();
    value = ReturnValueFromString( g_UnitMetric, text,
                                   m_Parent->m_InternalUnits );
    m_CurrentText->m_Size.x = m_CurrentText->m_Size.y = value;
    if( m_TextShape )
        m_CurrentText->m_Shape = m_TextShape->GetSelection();

    int style = m_TextStyle->GetSelection();
    if( ( style & 1 ) )
        m_CurrentText->m_Italic = 1;
    else
        m_CurrentText->m_Italic = 0;

    if( ( style & 2 ) )
    {
        m_CurrentText->m_Bold  = true;
        m_CurrentText->m_Width = GetPenSizeForBold( m_CurrentText->m_Size.x );
    }
    else
    {
        m_CurrentText->m_Bold  = false;
        m_CurrentText->m_Width = 0;
    }

    m_Parent->GetScreen()->SetModify();

    /* Make the text size as new default size if it is a new text */
    if( (m_CurrentText->m_Flags & IS_NEW) != 0 )
        g_DefaultTextLabelSize = m_CurrentText->m_Size.x;

    m_Parent->DrawPanel->MouseToCursorSchema();
    EndModal( 0 );
}


/*****************************************************************************/
void WinEDA_SchematicFrame::StartMoveTexte( SCH_TEXT* TextStruct, wxDC* DC )
{
/*****************************************************************************/
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
    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_TEXT:
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

    GetScreen()->SetModify();
    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitMoveTexte;
    GetScreen()->SetCurItem( TextStruct );
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );

    DrawPanel->CursorOn( DC );
}


/*************************************************************************/
void WinEDA_SchematicFrame::EditSchematicText( SCH_TEXT* TextStruct,
                                               wxDC*     DC )
{
/*************************************************************************/
/* Edit the properties of the text (Label, Global label, graphic text).. )
 *  pointed by "TextStruct"
 */
    if( TextStruct == NULL )
        return;

    // Erase old text on screen
    DrawPanel->CursorOff( DC );
    RedrawOneStruct( DrawPanel, DC, TextStruct, g_XorMode );

    DialogLabelEditor::ShowModally( this, TextStruct );

    // Redraw nex text, if exists
    if( ! TextStruct->m_Text.IsEmpty() )
        RedrawOneStruct( DrawPanel, DC, TextStruct, GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );
}


/*****************************************************************************/
void WinEDA_SchematicFrame::ChangeTextOrient( SCH_TEXT* TextStruct, wxDC* DC )
{
/*****************************************************************************/
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
    RedrawOneStruct( DrawPanel, DC, TextStruct, g_XorMode );

    int orient;

    switch( TextStruct->Type() )
    {
    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_TEXT:
        orient  = TextStruct->GetSchematicTextOrientation() + 1;
        orient &= 3;
        TextStruct->SetSchematicTextOrientation( orient );
        break;

    default:
        break;
    }

    GetScreen()->SetModify();
    RedrawOneStruct( DrawPanel, DC, TextStruct, g_XorMode );
    DrawPanel->CursorOn( DC );
}


/*************************************************************************/
SCH_TEXT* WinEDA_SchematicFrame::CreateNewText( wxDC* DC, int type )
{
/*************************************************************************/
/* Routine to create new text struct (GraphicText, label or Glabel).
 */
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
        NewText->m_Shape = s_DefaultShapeGLabel;
        NewText->SetSchematicTextOrientation( s_DefaultOrientGLabel );
        break;

    case LAYER_GLOBLABEL:
        NewText = new SCH_GLOBALLABEL( GetScreen()->m_Curseur );
        NewText->m_Shape = s_DefaultShapeGLabel;
        NewText->SetSchematicTextOrientation( s_DefaultOrientGLabel );
        break;

    default:
        DisplayError( this,
                      wxT( "WinEDA_SchematicFrame::CreateNewText() Internal error" ) );
        return NULL;
    }

    NewText->m_Size.x = NewText->m_Size.y = g_DefaultTextLabelSize;
    NewText->m_Flags  = IS_NEW | IS_MOVED;

    RedrawOneStruct( DrawPanel, DC, NewText, g_XorMode );
    EditSchematicText( NewText, DC );

    if( NewText->m_Text.IsEmpty() )
    {
        SAFE_DELETE( NewText );
        return NULL;
    }

    if( type == LAYER_GLOBLABEL  || type == LAYER_HIERLABEL )
    {
        s_DefaultShapeGLabel  = NewText->m_Shape;
        s_DefaultOrientGLabel = NewText->GetSchematicTextOrientation();
    }

    RedrawOneStruct( DrawPanel, DC, NewText, GR_DEFAULT_DRAWMODE );
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
        RedrawOneStruct( panel, DC, TextStruct, g_XorMode );

    /* redraw the text */
    switch( TextStruct->Type() )
    {
    case TYPE_SCH_LABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_TEXT:
        ( (SCH_TEXT*) TextStruct )->m_Pos = panel->GetScreen()->m_Curseur;
        break;

    default:
        break;
    }

    RedrawOneStruct( panel, DC, TextStruct, g_XorMode );
}


/*************************************************************/
static void ExitMoveTexte( WinEDA_DrawPanel* Panel, wxDC* DC )
{
/*************************************************************/
/* Abort function for the command move text */
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
    RedrawOneStruct( Panel, DC, Struct, g_XorMode );

    if( Struct->m_Flags & IS_NEW )
    {
        SAFE_DELETE( Struct );
        screen->SetCurItem( NULL );
    }
    else /* this was a move command on "old" text: restore its old settings. */
    {
        switch( Struct->Type() )
        {
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
        case TYPE_SCH_TEXT:
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

        RedrawOneStruct( Panel, DC, Struct, GR_DEFAULT_DRAWMODE );
        Struct->m_Flags = 0;
    }
}


/*****************************************************************************/
void WinEDA_SchematicFrame::ConvertTextType( SCH_TEXT* Text,
                                             wxDC* DC, int newtype )
{
/*****************************************************************************/
/* Routine to change a text type to an other one (GraphicText, label or
 * Glabel).
 * A new test, label or hierarchical or global label is created from the old
 * text.
 * the old text is deleted
 */
    if( Text == NULL )
        return;

    SCH_TEXT* newtext;

    switch( newtype )
    {
    case TYPE_SCH_LABEL:
        newtext = new SCH_LABEL( Text->m_Pos, Text->m_Text );
        break;

    case TYPE_SCH_GLOBALLABEL:
        newtext = new SCH_GLOBALLABEL( Text->m_Pos, Text->m_Text );
        break;

    case TYPE_SCH_HIERLABEL:
        newtext = new SCH_HIERLABEL( Text->m_Pos, Text->m_Text );
        break;

    case TYPE_SCH_TEXT:
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
    newtext->m_Width  = Text->m_Width;
    newtext->m_Italic = Text->m_Italic;
    newtext->m_Bold   = Text->m_Bold;


    // save current text flag:
    int flags = Text->m_Flags;

    /* add the new text in linked list if old text is in list */
    if( (flags & IS_NEW) == 0 )
    {
        newtext->SetNext( GetScreen()->EEDrawList );
        GetScreen()->EEDrawList = newtext;
        GetScreen()->SetModify();
    }

    /* now delete the old text
     *  If it is a text flagged IS_NEW it will be deleted by
     * ForceCloseManageCurseur()
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

    SAFE_DELETE( g_ItemToUndoCopy );

    DrawPanel->CursorOff( DC );   // Erase schematic cursor

    /* Save the new text in undo list if the old text was not itself a "new
     * created text"
     * In this case, the old text is already in undo list as a deleted item
     * Of course if the old text was a "new created text" the new text will be
     * put in undo list
     * later, at the end of the current command (if not aborted)
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

    RedrawOneStruct( DrawPanel, DC, newtext, GR_DEFAULT_DRAWMODE );
    DrawPanel->CursorOn( DC );    // redraw schematic cursor
}
