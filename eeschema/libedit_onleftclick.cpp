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

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_libentry.h"


void WinEDA_LibeditFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
    LIB_DRAW_ITEM* DrawEntry = m_drawItem;

    if( m_component == NULL )   // No component loaded !
        return;

    if( m_ID_current_state == 0 )
    {
        if( DrawEntry && DrawEntry->m_Flags )
        {
        	// Don't put copy in undo list while resizing (because it's already done)
        	if (!(DrawEntry->m_Flags & IS_RESIZED))
        		SaveCopyInUndoList( m_component );

            switch( DrawEntry->Type() )
            {
            case COMPONENT_FIELD_DRAW_TYPE:
                PlaceField( DC, (LIB_FIELD*) DrawEntry );
                DrawEntry = NULL;
                break;

            case COMPONENT_PIN_DRAW_TYPE:
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
            DrawEntry =
                m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT,
                                             GetScreen()->m_MousePosition );

            if( DrawEntry == NULL )
            {
                DrawEntry =
                    m_component->LocateDrawItem( m_unit, m_convert,
                                                 TYPE_NOT_INIT,
                                                 GetScreen()->m_Curseur );
            }

            if( DrawEntry )
                DrawEntry->DisplayInfo( this );

            else
                DisplayCmpDoc();
        }
    }

    if( m_ID_current_state )
    {
        switch( m_ID_current_state )
        {
        case ID_NO_SELECT_BUTT:
            break;

        case ID_LIBEDIT_PIN_BUTT:
            if( m_drawItem == NULL || m_drawItem->m_Flags == 0 )
            {
                CreatePin( DC );
            }
            else
            {
                SaveCopyInUndoList( m_component );
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
                if( m_drawItem->m_Flags & IS_NEW )
                    GraphicItemBeginDraw( DC );
                else
                {
                    SaveCopyInUndoList( m_component );
                    EndDrawGraphicItem( DC );
                }
            }
            break;

        case ID_LIBEDIT_DELETE_ITEM_BUTT:
            DrawEntry =
                m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT,
                                             GetScreen()->m_MousePosition );

            if( DrawEntry == NULL )
            {
                DrawEntry =
                    m_component->LocateDrawItem( m_unit, m_convert,
                                                 TYPE_NOT_INIT,
                                                 GetScreen()->m_Curseur );
            }
            if( DrawEntry == NULL )
            {
                DisplayCmpDoc();
                break;
            }
            SaveCopyInUndoList( m_component );
            if( DrawEntry->Type() == COMPONENT_PIN_DRAW_TYPE )
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
            DisplayError( this,
                          wxT( "WinEDA_LibeditFrame::OnLeftClick error" ) );
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
void WinEDA_LibeditFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
{
    wxPoint        pos = GetPosition();

    if( m_component == NULL )
        return;

    if( ( m_drawItem == NULL ) || ( m_drawItem->m_Flags == 0 ) )
    {   // We can locate an item
        m_drawItem =
            m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT,
                                         GetScreen()->m_MousePosition );
        if( m_drawItem == NULL )
        {
            m_drawItem =
                m_component->LocateDrawItem( m_unit, m_convert, TYPE_NOT_INIT,
                                             GetScreen()->m_Curseur );
        }
        if( m_drawItem == NULL )
        {
            EditComponentProperties();
        }
    }

    if( m_drawItem )
        m_drawItem->DisplayInfo( this );
    else
        return;

    DrawPanel->m_IgnoreMouseEvents = TRUE;

    switch( m_drawItem->Type() )
    {
    case COMPONENT_PIN_DRAW_TYPE:
        if( m_drawItem->m_Flags == 0 )
        {
            wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
            cmd.SetId( ID_LIBEDIT_EDIT_PIN );
            GetEventHandler()->ProcessEvent( cmd );
        }
        break;

    case COMPONENT_ARC_DRAW_TYPE:
    case COMPONENT_CIRCLE_DRAW_TYPE:
    case COMPONENT_RECT_DRAW_TYPE:
        if( m_drawItem->m_Flags == 0 )
        {
            EditGraphicSymbol( DC, m_drawItem );
        }
        break;

    case COMPONENT_LINE_DRAW_TYPE:
    case COMPONENT_POLYLINE_DRAW_TYPE:
        if( m_drawItem->m_Flags == 0 )
        {
            EditGraphicSymbol( DC, m_drawItem );
        }
        else if( m_drawItem->m_Flags & IS_NEW )
        {
            EndDrawGraphicItem( DC );
        }
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        if( m_drawItem->m_Flags == 0 )
        {
            EditSymbolText( DC, m_drawItem );
        }
        break;

    case COMPONENT_FIELD_DRAW_TYPE:
        if( m_drawItem->m_Flags == 0 )
        {
            EditField( DC, (LIB_FIELD*) m_drawItem );
        }
        break;


    default:
        wxString msg;
        msg.Printf( wxT( "WinEDA_LibeditFrame::OnLeftDClick Error: unknown \
StructType %d" ),
                    m_drawItem->Type() );
        DisplayError( this, msg );
        break;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;
}
