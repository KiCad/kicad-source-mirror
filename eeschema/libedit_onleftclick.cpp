/*****************************************/
/*  EESchema - libedit_onleftclick.cpp  */
/*****************************************/

/* Library editor commands created by a mouse left button simple or double click
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eeschema_id.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_libentry.h"


void LIB_EDIT_FRAME::OnLeftClick( wxDC* DC, const wxPoint& aPosition )
{
    LIB_ITEM* item = m_drawItem;

    if( m_component == NULL )   // No component loaded !
        return;

    if( item == NULL || item->m_Flags == 0 )
    {
        item = LocateItemUsingCursor( aPosition );

        if( item )
            item->DisplayInfo( this );
        else
        {
            DisplayCmpDoc();

            if( DrawPanel->m_AbortRequest )
                DrawPanel->m_AbortRequest = false;
        }
    }

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        if( item && item->m_Flags )   // moved object
        {
            switch( item->Type() )
            {
            case LIB_PIN_T:
                PlacePin( DC );
                break;

            default:
                EndDrawGraphicItem( DC );
                break;
            }
        }
        break;

    case ID_LIBEDIT_PIN_BUTT:
        if( m_drawItem == NULL || m_drawItem->m_Flags == 0 )
        {
            CreatePin( DC );
        }
        else
        {
            PlacePin( DC );
        }
        break;

    case ID_LIBEDIT_BODY_LINE_BUTT:
    case ID_LIBEDIT_BODY_ARC_BUTT:
    case ID_LIBEDIT_BODY_CIRCLE_BUTT:
    case ID_LIBEDIT_BODY_RECT_BUTT:
    case ID_LIBEDIT_BODY_TEXT_BUTT:
        if( m_drawItem == NULL || m_drawItem->m_Flags == 0 )
        {
            m_drawItem = CreateGraphicItem( m_component, DC );
        }
        else if( m_drawItem )
        {
            if( m_drawItem->IsNew() )
                GraphicItemBeginDraw( DC );
            else
                EndDrawGraphicItem( DC );
        }
        break;

    case ID_LIBEDIT_DELETE_ITEM_BUTT:
        m_drawItem = LocateItemUsingCursor( aPosition );

        if( m_drawItem )
            deleteItem( DC );
        else
            DisplayCmpDoc();

        break;

    case ID_LIBEDIT_ANCHOR_ITEM_BUTT:
        SaveCopyInUndoList( m_component );
        PlaceAncre();
        SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );
        break;

    default:
        wxFAIL_MSG( wxString::Format( wxT( "Unhandled command ID %d" ), GetToolId() ) );
        SetToolID( ID_NO_TOOL_SELECTED, DrawPanel->GetDefaultCursor(), wxEmptyString );
        break;
    }
}


/*
 * Called on a double click:
 *  If an editable item  (field, pin, graphic):
 *      Call the suitable dialog editor.
 */
void LIB_EDIT_FRAME::OnLeftDClick( wxDC* DC, const wxPoint& aPosition )
{
    wxPoint pos = GetPosition();

    if( m_component == NULL )
        return;

    if( ( m_drawItem == NULL ) || ( m_drawItem->m_Flags == 0 ) )
    {   // We can locate an item
        m_drawItem = LocateItemUsingCursor( aPosition );

        if( m_drawItem == NULL )
        {
            wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
            cmd.SetId( ID_LIBEDIT_GET_FRAME_EDIT_PART );
            GetEventHandler()->ProcessEvent( cmd );
        }
    }

    if( m_drawItem )
        m_drawItem->DisplayInfo( this );
    else
        return;

    DrawPanel->m_IgnoreMouseEvents = true;

    switch( m_drawItem->Type() )
    {
    case LIB_PIN_T:
        if( m_drawItem->m_Flags == 0 )
        {
            wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
            cmd.SetId( ID_LIBEDIT_EDIT_PIN );
            GetEventHandler()->ProcessEvent( cmd );
        }
        break;

    case LIB_ARC_T:
    case LIB_CIRCLE_T:
    case LIB_RECTANGLE_T:
        if( m_drawItem->m_Flags == 0 )
        {
            EditGraphicSymbol( DC, m_drawItem );
        }
        break;

    case LIB_POLYLINE_T:
        if( m_drawItem->m_Flags == 0 )
        {
            EditGraphicSymbol( DC, m_drawItem );
        }
        else if( m_drawItem->IsNew() )
        {
            EndDrawGraphicItem( DC );
        }
        break;

    case LIB_TEXT_T:
        if( m_drawItem->m_Flags == 0 )
        {
            EditSymbolText( DC, m_drawItem );
        }
        break;

    case LIB_FIELD_T:
        if( m_drawItem->m_Flags == 0 )
        {
            EditField( DC, (LIB_FIELD*) m_drawItem );
        }
        break;

    default:
        wxFAIL_MSG( wxT( "Unhandled item <" ) + m_drawItem->GetClass() + wxT( ">" ) );
        break;
    }

    DrawPanel->MoveCursorToCrossHair();
    DrawPanel->m_IgnoreMouseEvents = false;
}
