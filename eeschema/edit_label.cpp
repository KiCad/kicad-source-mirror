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


static int       lastGlobalLabelShape = (int) NET_INPUT;
static int       lastTextOrientation = 0;
static bool      lastTextBold = false;
static bool      lastTextItalic = false;


static void moveText( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition, bool aErase )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_TEXT* textItem = (SCH_TEXT*) screen->GetCurItem();

    wxCHECK_RET( (textItem != NULL) && textItem->CanIncrementLabel(),
                 wxT( "Cannot move invalid text type." ) );

    // Erase the current text at its current position.
    if( aErase )
        textItem->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    textItem->m_Pos = screen->GetCrossHairPosition();

    // Draw the text item at it's new position.
    textItem->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
}


static void abortMoveText( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
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
    }
    else    // Move command on an existing text item, restore the copy of the original.
    {
        screen->RemoveFromDrawList( item );
        delete item;

        item = parent->GetUndoItem();

        wxCHECK_RET( item != NULL, wxT( "Cannot restore undefined last text item." ) );

        screen->AddToDrawList( item );
        parent->SetUndoItem( NULL );
        item->Draw( aPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        item->ClearFlags();
    }

    screen->SetCurItem( NULL );
}


void SCH_EDIT_FRAME::MoveText( SCH_TEXT* aTextItem, wxDC* aDC )
{
    wxCHECK_RET( (aTextItem != NULL) && aTextItem->CanIncrementLabel(),
                 wxT( "Cannot move invalid text item" ) );

    m_itemToRepeat = NULL;

    aTextItem->SetFlags( IS_MOVED );

    SetUndoItem( (SCH_ITEM*) aTextItem );

    DrawPanel->CrossHairOff( aDC );
    GetScreen()->SetCrossHairPosition( aTextItem->m_Pos );
    DrawPanel->MoveCursorToCrossHair();

    OnModify();
    DrawPanel->SetMouseCapture( moveText, abortMoveText );
    GetScreen()->SetCurItem( aTextItem );
    moveText( DrawPanel, aDC, wxDefaultPosition, true );

    DrawPanel->CrossHairOn( aDC );
}


void SCH_EDIT_FRAME::ChangeTextOrient( SCH_TEXT* aTextItem, wxDC* aDC )
{
    wxCHECK_RET( (aTextItem != NULL) && aTextItem->CanIncrementLabel(),
                 wxT( "Invalid schematic text item." )  );

    int orient = ( aTextItem->GetOrientation() + 1 ) & 3;

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

    if( (aType == SCH_GLOBAL_LABEL_T) || (aType == SCH_HIERARCHICAL_LABEL_T) )
    {
        lastGlobalLabelShape = textItem->m_Shape;
    }

    textItem->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->SetMouseCapture( moveText, abortMoveText );
    GetScreen()->SetCurItem( textItem );

    return textItem;
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
    newtext->SetFlags( text->GetFlags() );
    newtext->m_Shape = text->m_Shape;
    newtext->SetOrientation( text->GetOrientation() );
    newtext->m_Size = text->m_Size;
    newtext->m_Thickness = text->m_Thickness;
    newtext->m_Italic = text->m_Italic;
    newtext->m_Bold = text->m_Bold;

    /* Save the new text in undo list if the old text was not itself a "new created text"
     * In this case, the old text is already in undo list as a deleted item.
     * Of course if the old text was a "new created text" the new text will be
     * put in undo list later, at the end of the current command (if not aborted)
     */

    INSTALL_UNBUFFERED_DC( dc, DrawPanel );
    DrawPanel->CrossHairOff( &dc );   // Erase schematic cursor
    text->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), g_XorMode );

    screen->RemoveFromDrawList( text );
    screen->AddToDrawList( newtext );
    GetScreen()->SetCurItem( newtext );
    OnModify();
    newtext->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CrossHairOn( &dc );    // redraw schematic cursor

    if( text->GetFlags() == 0 )
    {
        m_itemToRepeat = NULL;
        text->ClearFlags();
        text->SetNext( NULL );
        text->SetBack( NULL );
        newtext->ClearFlags();
        PICKED_ITEMS_LIST pickList;
        ITEM_PICKER picker( newtext, UR_EXCHANGE_T );
        picker.SetLink( text );
        pickList.PushItem( picker );
        SaveCopyInUndoList( pickList, UR_EXCHANGE_T );
    }
    else
    {
        delete text;
    }
}
