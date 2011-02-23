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
    LIB_DRAW_ITEM* DrawEntry = m_drawItem;

    if( m_component == NULL )   // No component loaded !
        return;

    if( DrawEntry && DrawEntry->m_Flags )
    {
        switch( DrawEntry->Type() )
        {
        case LIB_PIN_T:
            PlacePin( DC );
            DrawEntry = NULL;
            break;

        default:
            EndDrawGraphicItem( DC );
            break;
        }
    }
    else
    {
        DrawEntry = m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT, aPosition );

        if( DrawEntry == NULL )
        {
            DrawEntry = m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT,
                                                     GetScreen()->GetCrossHairPosition() );
        }

        if( DrawEntry )
            DrawEntry->DisplayInfo( this );

        else
            DisplayCmpDoc();
    }

    if( m_ID_current_state )
    {
        switch( m_ID_current_state )
        {
        case 0:
        case ID_LIBEDIT_NO_TOOL:
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
            DrawEntry = m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT, aPosition );

            if( DrawEntry == NULL )
            {
                DrawEntry = m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT,
                                                         GetScreen()->GetCrossHairPosition() );
            }

            if( DrawEntry == NULL )
            {
                DisplayCmpDoc();
                break;
            }

            SaveCopyInUndoList( m_component );

            if( DrawEntry->Type() == LIB_PIN_T )
                DeletePin( DC, m_component, (LIB_PIN*) DrawEntry );
            else
                m_component->RemoveDrawItem( DrawEntry, DrawPanel, DC );

            DrawEntry = NULL;
            OnModify( );
            break;

        case ID_LIBEDIT_ANCHOR_ITEM_BUTT:
            SaveCopyInUndoList( m_component );
            PlaceAncre();
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
            break;

        default:
            DisplayError( this, wxT( "LIB_EDIT_FRAME::OnLeftClick error" ) );
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
            break;
        }
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
        m_drawItem = m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT, aPosition );
        if( m_drawItem == NULL )
        {
            m_drawItem = m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT,
                                                      GetScreen()->GetCrossHairPosition() );
        }
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

    DrawPanel->m_IgnoreMouseEvents = TRUE;

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
        wxString msg;
        msg.Printf( wxT( "LIB_EDIT_FRAME::OnLeftDClick Error: unknown StructType %d" ),
                    m_drawItem->Type() );
        DisplayError( this, msg );
        break;
    }

    DrawPanel->MoveCursorToCrossHair();
    DrawPanel->m_IgnoreMouseEvents = FALSE;
}
