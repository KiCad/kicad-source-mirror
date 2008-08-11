/*****************************************/
/*	EESchema - libedit_onleftclick.cpp	*/
/*****************************************/

/* Library editor commands created by a mouse left button simple or double click
 */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "bitmaps.h"
#include "protos.h"

#include "id.h"


/************************************************************************/
void WinEDA_LibeditFrame::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
/************************************************************************/
{
    LibEDA_BaseStruct* DrawEntry = CurrentDrawItem;

    if( CurrentLibEntry == NULL )
        return;

    if( m_ID_current_state == 0 )
    {
        if( DrawEntry && DrawEntry->m_Flags )
        {
            SaveCopyInUndoList( CurrentLibEntry );

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
			DrawEntry = LocatePin( GetScreen()->m_MousePosition, CurrentLibEntry,
                                   CurrentUnit, CurrentConvert );
            if( DrawEntry == NULL )
            {
				DrawEntry = LocateDrawItem( (SCH_SCREEN*)GetScreen(), GetScreen()->m_MousePosition,
                                            CurrentLibEntry, CurrentUnit,
                                            CurrentConvert, LOCATE_ALL_DRAW_ITEM );
            }

            if( DrawEntry == NULL )
				DrawEntry = LocatePin( GetScreen()->m_Curseur, CurrentLibEntry,
                                       CurrentUnit, CurrentConvert );
            if( DrawEntry == NULL )
            {
				DrawEntry = LocateDrawItem( (SCH_SCREEN*)GetScreen(), GetScreen()->m_Curseur,
                                            CurrentLibEntry, CurrentUnit,
                                            CurrentConvert, LOCATE_ALL_DRAW_ITEM );
            }

            if( DrawEntry )
                DrawEntry->Display_Infos_DrawEntry( this );

            else
            {
                EraseMsgBox();
                AfficheDoc( this, CurrentLibEntry->m_Doc.GetData(),
                           CurrentLibEntry->m_KeyWord.GetData() );
            }
        }
    }

    if( m_ID_current_state )
    {
        switch( m_ID_current_state )
        {
        case ID_NO_SELECT_BUTT:
            break;

        case ID_LIBEDIT_PIN_BUTT:
            if( CurrentDrawItem == NULL )
            {
                CreatePin( DC );
            }
            else
            {
                SaveCopyInUndoList( CurrentLibEntry );
                PlacePin( DC );
            }
            break;

        case ID_LIBEDIT_BODY_LINE_BUTT:
        case ID_LIBEDIT_BODY_ARC_BUTT:
        case ID_LIBEDIT_BODY_CIRCLE_BUTT:
        case ID_LIBEDIT_BODY_RECT_BUTT:
        case ID_LIBEDIT_BODY_TEXT_BUTT:
            if( CurrentDrawItem == NULL )
            {
                CurrentDrawItem = CreateGraphicItem( DC );
            }
            else
            {
                if( CurrentDrawItem->m_Flags & IS_NEW )
                    GraphicItemBeginDraw( DC );
                else
                {
                    SaveCopyInUndoList( CurrentLibEntry );
                    EndDrawGraphicItem( DC );
                }
            }
            break;

        case ID_LIBEDIT_DELETE_ITEM_BUTT:
			DrawEntry = LocatePin( GetScreen()->m_MousePosition, CurrentLibEntry,
                                   CurrentUnit, CurrentConvert );
            if( DrawEntry == NULL )
            {
				DrawEntry = LocateDrawItem( (SCH_SCREEN*)GetScreen(),
											 GetScreen()->m_MousePosition,
                                            CurrentLibEntry, CurrentUnit,
                                            CurrentConvert, LOCATE_ALL_DRAW_ITEM );
            }

            if( DrawEntry == NULL )
				DrawEntry = LocatePin( GetScreen()->m_Curseur, CurrentLibEntry,
                                       CurrentUnit, CurrentConvert );
            if( DrawEntry == NULL )
            {
				DrawEntry = LocateDrawItem( (SCH_SCREEN*)GetScreen(),
											 GetScreen()->m_Curseur,
                                            CurrentLibEntry, CurrentUnit,
                                            CurrentConvert, LOCATE_ALL_DRAW_ITEM );
            }
            if( DrawEntry == NULL )
            {
                AfficheDoc( this, CurrentLibEntry->m_Doc.GetData(),
                           CurrentLibEntry->m_KeyWord.GetData() );
                break;
            }
            SaveCopyInUndoList( CurrentLibEntry );
            if( DrawEntry->Type() == COMPONENT_PIN_DRAW_TYPE )
                DeletePin( DC, CurrentLibEntry, (LibDrawPin*) DrawEntry );
            else
                DeleteOneLibraryDrawStruct( DrawPanel, DC, CurrentLibEntry, DrawEntry, TRUE );
            DrawEntry = NULL;
			GetScreen()->SetModify();
            break;

        case ID_LIBEDIT_ANCHOR_ITEM_BUTT:
            SaveCopyInUndoList( CurrentLibEntry );
            PlaceAncre();
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
            break;


        default:
            DisplayError( this, wxT( "WinEDA_LibeditFrame::OnLeftClick error" ) );
            SetToolID( 0, wxCURSOR_ARROW, wxEmptyString );
            break;
        }
    }
}


/*************************************************************************/
void WinEDA_LibeditFrame::OnLeftDClick( wxDC* DC, const wxPoint& MousePos )
/*************************************************************************/

/* Called on a double click:
 *  If an editable item  (field, pin, graphic):
 *      Call the suitable dialog editor.
 */
{
    wxPoint            pos = GetPosition();
    LibEDA_BaseStruct* DrawEntry = CurrentDrawItem;

    if( CurrentLibEntry == NULL )
        return;

    if( (DrawEntry == NULL) || (DrawEntry->m_Flags == 0) )
    {   // We can locate an item
		DrawEntry = LocatePin( GetScreen()->m_MousePosition, CurrentLibEntry,
                               CurrentUnit, CurrentConvert );
        if( DrawEntry == NULL )
			DrawEntry = LocatePin( GetScreen()->m_Curseur, CurrentLibEntry,
                                   CurrentUnit, CurrentConvert );
        if( DrawEntry == NULL )
        {
			DrawEntry = CurrentDrawItem = LocateDrawItem( (SCH_SCREEN*) GetScreen(),
														 GetScreen()->m_MousePosition,
                                                         CurrentLibEntry, CurrentUnit,
                                                         CurrentConvert,
                                                         LOCATE_ALL_DRAW_ITEM );
        }
        if( DrawEntry == NULL )
        {
			DrawEntry = CurrentDrawItem = LocateDrawItem( (SCH_SCREEN*) GetScreen(),
														 GetScreen()->m_Curseur,
                                                         CurrentLibEntry, CurrentUnit,
                                                         CurrentConvert,
                                                         LOCATE_ALL_DRAW_ITEM );
        }
        if( DrawEntry == NULL )
        {
            DrawEntry = CurrentDrawItem = (LibEDA_BaseStruct*)
                                          LocateField( CurrentLibEntry );
        }
        if( DrawEntry == NULL )
        {
            wxPoint mpos;
            wxGetMousePosition( &mpos.x, &mpos.y );
            InstallLibeditFrame( mpos );
        }
    }

    // Si Commande en cours: affichage commande d'annulation
    if( m_ID_current_state )
    {
    }
    else
    {
    }

    if( DrawEntry )
        DrawEntry->Display_Infos_DrawEntry( this );
    else
        return;

    CurrentDrawItem = DrawEntry;

    DrawPanel->m_IgnoreMouseEvents = TRUE;

    switch( DrawEntry->Type() )
    {
    case  COMPONENT_PIN_DRAW_TYPE:
        if( DrawEntry->m_Flags == 0 )       // Item localis� et non en edition: placement commande move
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
        msg.Printf(
            wxT( "WinEDA_LibeditFrame::OnLeftDClick Error: unknown StructType %d" ),
            DrawEntry->Type() );
        DisplayError( this, msg );
        break;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;
}
