/*********************************************************************/
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
#include "kicad_device_context.h"

#include "general.h"
#include "protos.h"
#include "sch_text.h"
#include "eeschema_id.h"


static void ShowWhileMoving( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                             bool aErase );
static void ExitMoveTexte( EDA_DRAW_PANEL* panel, wxDC* DC );


static wxPoint ItemInitialPosition;
static int     OldOrient;
static wxSize  OldSize;
static int     lastGlobalLabelShape = (int) NET_INPUT;
static int     lastTextOrientation = 0;
static bool    lastTextBold = false;
static bool    lastTextItalic = false;


void SCH_EDIT_FRAME::StartMoveTexte( SCH_TEXT* aTextItem, wxDC* aDC )
{
    if( aTextItem == NULL )
        return;

    m_itemToRepeat = NULL;

    if( !aTextItem->IsNew() )
    {
        delete g_ItemToUndoCopy;
        g_ItemToUndoCopy = aTextItem->Clone();
    }

    aTextItem->SetFlags( IS_MOVED );

    switch( aTextItem->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
        ItemInitialPosition = aTextItem->m_Pos;
        OldSize = aTextItem->m_Size;
        OldOrient = aTextItem->GetOrientation();
        break;

    default:
        break;
    }

    DrawPanel->CrossHairOff( aDC );
    GetScreen()->SetCrossHairPosition( ItemInitialPosition );
    DrawPanel->MoveCursorToCrossHair();

    OnModify();
    DrawPanel->SetMouseCapture( ShowWhileMoving, ExitMoveTexte );
    GetScreen()->SetCurItem( aTextItem );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, wxDefaultPosition, true );

    DrawPanel->CrossHairOn( aDC );
}


void SCH_EDIT_FRAME::ChangeTextOrient( SCH_TEXT* aTextItem, wxDC* aDC )
{
    wxCHECK_RET( aTextItem != NULL, wxT( "Invalid schematic text item." )  );

    int orient;

    switch( aTextItem->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
        orient = ( aTextItem->GetOrientation() + 1 ) & 3;
        break;

    default:
        wxFAIL_MSG( wxT( "Invalid schematic item <" ) + aTextItem->GetClass() +
                    wxT( "> passed to SCH_EDIT_FRAME::ChangeTextOrient()" ) );
        return;
    }

    // Save current text orientation in undo list if is not already in edit.
    if( aTextItem->GetFlags() == 0 )
        SaveCopyInUndoList( aTextItem, UR_CHANGED );

    DrawPanel->CrossHairOff( aDC );
    aTextItem->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
    aTextItem->SetOrientation( orient );
    OnModify();
    aTextItem->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
    DrawPanel->CrossHairOn( aDC );
}


SCH_TEXT* SCH_EDIT_FRAME::CreateNewText( wxDC* aDC, int aType )
{
    SCH_TEXT* textItem = NULL;

    m_itemToRepeat = NULL;

    switch( aType )
    {
    case LAYER_NOTES:
        textItem = new SCH_TEXT( GetScreen()->GetCrossHairPosition() );
        break;

    case LAYER_LOCLABEL:
        textItem = new SCH_LABEL( GetScreen()->GetCrossHairPosition() );
        break;

    case LAYER_HIERLABEL:
        textItem = new SCH_HIERLABEL( GetScreen()->GetCrossHairPosition() );
        textItem->m_Shape = lastGlobalLabelShape;
        break;

    case LAYER_GLOBLABEL:
        textItem = new SCH_GLOBALLABEL( GetScreen()->GetCrossHairPosition() );
        textItem->m_Shape = lastGlobalLabelShape;
        break;

    default:
        DisplayError( this, wxT( "SCH_EDIT_FRAME::CreateNewText() Internal error" ) );
        return NULL;
    }

    textItem->m_Bold = lastTextBold;
    textItem->m_Italic = lastTextItalic;
    textItem->SetOrientation( lastTextOrientation );
    textItem->m_Size.x = textItem->m_Size.y = g_DefaultTextLabelSize;
    textItem->SetFlags( IS_NEW | IS_MOVED );

    textItem->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
    EditSchematicText( textItem );

    if( textItem->m_Text.IsEmpty() )
    {
        SAFE_DELETE( textItem );
        return NULL;
    }

    lastTextBold = textItem->m_Bold;
    lastTextItalic = textItem->m_Italic;
    lastTextOrientation = textItem->GetOrientation();

    if( aType == LAYER_GLOBLABEL  || aType == LAYER_HIERLABEL )
    {
        lastGlobalLabelShape = textItem->m_Shape;
    }

    textItem->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->SetMouseCapture( ShowWhileMoving, ExitMoveTexte );
    GetScreen()->SetCurItem( textItem );

    return textItem;
}


static void ShowWhileMoving( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                             bool aErase )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_ITEM* textItem = screen->GetCurItem();

    // Erase the current text at its current position.
    if( aErase )
        textItem->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    // Draw the text item at it's new position.
    switch( textItem->Type() )
    {
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIERARCHICAL_LABEL_T:
    case SCH_TEXT_T:
        ( (SCH_TEXT*) textItem )->m_Pos = screen->GetCrossHairPosition();
        break;

    default:
        break;
    }

    textItem->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
}


/* Abort function for the command move text */
static void ExitMoveTexte( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_ITEM* item = screen->GetCurItem();
    SCH_EDIT_FRAME* parent = ( SCH_EDIT_FRAME* ) aPanel->GetParent();

    parent->SetRepeatItem( NULL );

    if( item == NULL )  /* no current item */
        return;

    // Erase the text item and delete it if new (i.e. it was being just created).
    item->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    if( item->IsNew() )
    {
        SAFE_DELETE( item );
        screen->SetCurItem( NULL );
    }
    else    // this was a move command on "old" text: restore its old settings.
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
        case SCH_TEXT_T:
        {
            SCH_TEXT* text = (SCH_TEXT*) item;
            text->m_Pos = ItemInitialPosition;
            text->m_Size = OldSize;
            text->SetOrientation( OldOrient );
        }
        break;

        default:
            break;
        }

        item->Draw( aPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        item->ClearFlags();
    }
}


void SCH_EDIT_FRAME::OnConvertTextType( wxCommandEvent& aEvent )
{
    SCH_SCREEN* screen = GetScreen();
    SCH_TEXT* text = (SCH_TEXT*) screen->GetCurItem();

    wxCHECK_RET( (text != NULL) && text->CanIncrementLabel(),
                 wxT( "Cannot convert text type." ) );

    KICAD_T type;

    switch( aEvent.GetId() )
    {
    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_LABEL:
        type = SCH_LABEL_T;
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_GLABEL:
        type = SCH_GLOBAL_LABEL_T;
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_HLABEL:
        type = SCH_HIERARCHICAL_LABEL_T;
        break;

    case ID_POPUP_SCH_CHANGE_TYPE_TEXT_TO_COMMENT:
        type = SCH_TEXT_T;
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "Invalid text type command ID %d." ),
                                      aEvent.GetId() ) );
        return;
    }

    if( text->Type() == type )
        return;

    SCH_TEXT* newtext;

    switch( type )
    {
    case SCH_LABEL_T:
        newtext = new SCH_LABEL( text->m_Pos, text->m_Text );
        break;

    case SCH_GLOBAL_LABEL_T:
        newtext = new SCH_GLOBALLABEL( text->m_Pos, text->m_Text );
        break;

    case SCH_HIERARCHICAL_LABEL_T:
        newtext = new SCH_HIERLABEL( text->m_Pos, text->m_Text );
        break;

    case SCH_TEXT_T:
        newtext = new SCH_TEXT( text->m_Pos, text->m_Text );
        break;

    default:
        newtext = NULL;
        wxFAIL_MSG( wxString::Format( wxT( "Cannot convert text type to %d" ), type ) );
        return;
    }

    /* Copy the old text item settings to the new one.  Justifications are not copied because
     * they are not used in labels.  Justifications will be set to default value in the new
     * text item type.
     */
    newtext->m_Shape = text->m_Shape;
    newtext->SetOrientation( text->GetOrientation() );
    newtext->m_Size = text->m_Size;
    newtext->m_Thickness = text->m_Thickness;
    newtext->m_Italic = text->m_Italic;
    newtext->m_Bold = text->m_Bold;

    // save current text flag:
    int flags = text->GetFlags();

    /* add the new text in linked list if old text is in list */
    if( (flags & IS_NEW) == 0 )
    {
        newtext->SetNext( GetScreen()->GetDrawItems() );
        GetScreen()->SetDrawItems( newtext );
        OnModify();
    }

    INSTALL_UNBUFFERED_DC( dc, DrawPanel );

    /* Delete the old text item.  If it is a text flagged as new it will be deleted by
     * ending the mouse capture.
     */
    if( DrawPanel->IsMouseCaptured() )
        DrawPanel->EndMouseCapture();

    if( (flags & IS_NEW) == 0 )  // Remove old text from current list and save it in undo list
    {
        text->ClearFlags();
        DeleteItem( text );      // old text is really saved in undo list
        GetScreen()->SetCurItem( NULL );
        m_itemToRepeat = NULL;
    }

    GetScreen()->SetCurItem( NULL );

    delete g_ItemToUndoCopy;
    g_ItemToUndoCopy = NULL;

    DrawPanel->CrossHairOff( &dc );   // Erase schematic cursor

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
        newtext->SetFlags( IS_NEW );
    }

    if( (flags & IS_MOVED) != 0 )
    {
        GetScreen()->SetCurItem( newtext );
        StartMoveTexte( newtext, &dc );
    }

    newtext->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CrossHairOn( &dc );    // redraw schematic cursor
}
