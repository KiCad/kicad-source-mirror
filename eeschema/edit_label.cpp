/*********************************************************************/
/* edit_label.cpp: label, global label and text creation or edition  */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"
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
    SCH_TEXT* item = (SCH_TEXT*)screen->GetCurItem();
    SCH_EDIT_FRAME* parent = ( SCH_EDIT_FRAME* ) aPanel->GetParent();

    parent->SetRepeatItem( NULL );
    screen->SetCurItem( NULL );

    if( item == NULL )  /* no current item */
        return;

    if( item->IsNew() )
    {
        delete item;
        item = NULL;
    }
    else    // Move command on an existing text item, restore the values of the original.
    {
        SCH_TEXT* olditem = (SCH_TEXT* )parent->GetUndoItem();
        screen->SetCurItem( item );

        wxCHECK_RET( olditem != NULL && item->Type() == olditem->Type(),
            wxT( "Cannot restore undefined or bad last text item." ) );
        // Never delete existing item, because it can be referenced by an undo/redo command
        // Just restore its data
        item->SwapData(olditem);
    }

    aPanel->Refresh();
}


void SCH_EDIT_FRAME::MoveText( SCH_TEXT* aTextItem, wxDC* aDC )
{
    wxCHECK_RET( (aTextItem != NULL) && aTextItem->CanIncrementLabel(),
                 wxT( "Cannot move invalid text item" ) );

    m_itemToRepeat = NULL;

    aTextItem->SetFlags( IS_MOVED );

    SetUndoItem( aTextItem );

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

/*
 * OnConvertTextType is a command event handler to change a text type to an other one.
 * The new text, label, hierarchical label, or global label is created from the old text
 * The old text is deleted.
 * A tricky case is when the 'old" text is being edited (i.e. moving)
 * because we must create a new text, and prepare the undo/redo command data for this
 * change and the current move/edit command
 */
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
    m_itemToRepeat = NULL;
    OnModify();
    newtext->Draw( DrawPanel, &dc, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    DrawPanel->CrossHairOn( &dc );    // redraw schematic cursor

    if( text->IsNew() )
    {
        // if the previous text is new, no undo command to prepare here
        // just delete this previous text.
        delete text;
        return;
    }

    // previous text is not new and we replace text by new text.
    // So this is equivalent to delete text and add newtext
    // If text if being currently edited (i.e. moved)
    // we also save the initial copy of text, and prepare undo command for new text modifications.
    // we must save it as modified text (if currently beeing edited), then deleted text,
    // and replace text with newtext
    PICKED_ITEMS_LIST pickList;
    ITEM_PICKER picker( text, UR_CHANGED );
    if( text->GetFlags() )
    {
        // text is being edited, save initial text for undo command
        picker.SetLink( GetUndoItem() );
        pickList.PushItem( picker );
        // the owner of undoItem is no more "this", it is now "picker":
        SetUndoItem( NULL );
        // save current newtext copy for undo/abort current command
        SetUndoItem( newtext );
    }

    // Prepare undo command for delete old text
    picker.m_UndoRedoStatus = UR_DELETED;
    picker.SetLink( NULL );
    pickList.PushItem( picker );

    // Prepare undo command for new text
    picker.m_UndoRedoStatus = UR_NEW;
    picker.SetItem(newtext);
    pickList.PushItem( picker );

    SaveCopyInUndoList( pickList, UR_UNSPECIFIED );
}


/* Function to increment bus label members numbers,
 * i.e. when a text is ending with a number, adds
 * <RepeatDeltaLabel> to this number
 */
void IncrementLabelMember( wxString& name )
{
    int  ii, nn;
    long number = 0;

    ii = name.Len() - 1; nn = 0;

    if( !isdigit( name.GetChar( ii ) ) )
        return;

    while( (ii >= 0) && isdigit( name.GetChar( ii ) ) )
    {
        ii--; nn++;
    }

    ii++;   /* digits are starting at ii position */
    wxString litt_number = name.Right( nn );

    if( litt_number.ToLong( &number ) )
    {
        number += g_RepeatDeltaLabel;
        name.Remove( ii ); name << number;
    }
}
