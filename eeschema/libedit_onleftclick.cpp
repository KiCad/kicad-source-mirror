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
#include "libcmp.h"
#include "protos.h"
#include "libeditfrm.h"
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
            SaveCopyInUndoList( m_component );

            switch( DrawEntry->Type() )
            {
            case COMPONENT_PIN_DRAW_TYPE:
                PlacePin( DC );
                break;

            case COMPONENT_FIELD_DRAW_TYPE:
                PlaceField( DC, (LibDrawField*) DrawEntry );
                DrawEntry = NULL;
                break;

            default:
                EndDrawGraphicItem( DC );
                break;
            }
        }
        else
        {
            DrawEntry = LocatePin( GetScreen()->m_MousePosition, m_component,
                                   m_unit, m_convert );
            if( DrawEntry == NULL )
            {
                DrawEntry = LocateDrawItem( (SCH_SCREEN*)GetScreen(),
                                            GetScreen()->m_MousePosition,
                                            m_component, m_unit, m_convert,
                                            LOCATE_ALL_DRAW_ITEM );
            }

            if( DrawEntry == NULL )
                DrawEntry = LocatePin( GetScreen()->m_Curseur, m_component,
                                       m_unit, m_convert );
            if( DrawEntry == NULL )
            {
                DrawEntry = LocateDrawItem( (SCH_SCREEN*)GetScreen(),
                                            GetScreen()->m_Curseur,
                                            m_component, m_unit, m_convert,
                                            LOCATE_ALL_DRAW_ITEM );
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
            DrawEntry = LocatePin( GetScreen()->m_MousePosition, m_component,
                                   m_unit, m_convert );
            if( DrawEntry == NULL )
            {
                DrawEntry = LocateDrawItem( (SCH_SCREEN*)GetScreen(),
                                            GetScreen()->m_MousePosition,
                                            m_component, m_unit, m_convert,
                                            LOCATE_ALL_DRAW_ITEM );
            }

            if( DrawEntry == NULL )
                DrawEntry = LocatePin( GetScreen()->m_Curseur,
                                       m_component, m_unit,
                                       m_convert );
            if( DrawEntry == NULL )
            {
                DrawEntry = LocateDrawItem( (SCH_SCREEN*)GetScreen(),
                                            GetScreen()->m_Curseur,
                                            m_component, m_unit, m_convert,
                                            LOCATE_ALL_DRAW_ITEM );
            }
            if( DrawEntry == NULL )
                DisplayCmpDoc();
            SaveCopyInUndoList( m_component );
            if( DrawEntry->Type() == COMPONENT_PIN_DRAW_TYPE )
                DeletePin( DC, m_component, (LibDrawPin*) DrawEntry );
            else
                m_component->RemoveDrawItem( DrawEntry, DrawPanel, DC );
            DrawEntry = NULL;
            GetScreen()->SetModify();
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
    LIB_DRAW_ITEM* DrawEntry = m_drawItem;

    if( m_component == NULL )
        return;

    if( ( DrawEntry == NULL ) || ( DrawEntry->m_Flags == 0 ) )
    {   // We can locate an item
        DrawEntry = LocatePin( GetScreen()->m_MousePosition, m_component,
                               m_unit, m_convert );
        if( DrawEntry == NULL )
            DrawEntry = LocatePin( GetScreen()->m_Curseur, m_component,
                                   m_unit, m_convert );
        if( DrawEntry == NULL )
        {
            DrawEntry = m_drawItem =
                LocateDrawItem( (SCH_SCREEN*) GetScreen(),
                                GetScreen()->m_MousePosition, m_component,
                                m_unit, m_convert, LOCATE_ALL_DRAW_ITEM );
        }
        if( DrawEntry == NULL )
        {
            DrawEntry = m_drawItem =
                LocateDrawItem( (SCH_SCREEN*) GetScreen(),
                                GetScreen()->m_Curseur, m_component, m_unit,
                                m_convert, LOCATE_ALL_DRAW_ITEM );
        }
        if( DrawEntry == NULL )
        {
            DrawEntry = m_drawItem =
                (LIB_DRAW_ITEM*) LocateField( m_component );
        }
        if( DrawEntry == NULL )
        {
            EditComponentProperties();
        }
    }

    if( DrawEntry )
        DrawEntry->DisplayInfo( this );
    else
        return;

    m_drawItem = DrawEntry;

    DrawPanel->m_IgnoreMouseEvents = TRUE;

    switch( DrawEntry->Type() )
    {
    case  COMPONENT_PIN_DRAW_TYPE:
        if( DrawEntry->m_Flags == 0 )
        {
            InstallPineditFrame( this, DC, pos );
        }
        break;

    case COMPONENT_ARC_DRAW_TYPE:
    case COMPONENT_CIRCLE_DRAW_TYPE:
    case COMPONENT_RECT_DRAW_TYPE:
        if( DrawEntry->m_Flags == 0 )
        {
            EditGraphicSymbol( DC, DrawEntry );
        }
        break;

    case COMPONENT_LINE_DRAW_TYPE:
    case COMPONENT_POLYLINE_DRAW_TYPE:
        if( DrawEntry->m_Flags == 0 )
        {
            EditGraphicSymbol( DC, DrawEntry );
        }
        else if( DrawEntry->m_Flags & IS_NEW )
        {
            EndDrawGraphicItem( DC );
        }
        break;

    case COMPONENT_GRAPHIC_TEXT_DRAW_TYPE:
        if( DrawEntry->m_Flags == 0 )
        {
            EditSymbolText( DC, DrawEntry );
        }
        break;

    case COMPONENT_FIELD_DRAW_TYPE:
        if( DrawEntry->m_Flags == 0 )
        {
            EditField( DC, (LibDrawField*) DrawEntry );
        }
        break;


    default:
        wxString msg;
        msg.Printf( wxT( "WinEDA_LibeditFrame::OnLeftDClick Error: unknown \
StructType %d" ),
                    DrawEntry->Type() );
        DisplayError( this, msg );
        break;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;
}
