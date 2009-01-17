/****************/
/* controle.cpp */
/****************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "eda_dde.h"

#include "id.h"

#include "protos.h"

/**************************************************************/
SCH_ITEM * WinEDA_SchematicFrame:: SchematicGeneralLocateAndDisplay( bool IncludePin )
/**************************************************************/

/* Routine de localisation et d'affichage des caract (si utile )
 *  de l'element pointe par la souris ou par le curseur pcb
 *  - marqueur
 *  - noconnect
 *  - jonction
 *  - wire/bus/entry
 *  - label
 *  - composant
 *  - pin
 *  retourne
 *      un pointeur sur le composant
 *      Null sinon
 */
{
    SCH_ITEM*         DrawStruct;
    wxString                msg;
    wxPoint                 mouse_position = GetScreen()->m_MousePosition;
    LibDrawPin*             Pin     = NULL;
    SCH_COMPONENT* LibItem = NULL;

    DrawStruct = SchematicGeneralLocateAndDisplay( mouse_position, IncludePin );
    if( !DrawStruct && ( mouse_position != GetScreen()->m_Curseur) )
    {
        DrawStruct = SchematicGeneralLocateAndDisplay( GetScreen()->m_Curseur, IncludePin );
    }
    if( !DrawStruct )
        return NULL;

    /* Cross probing to pcbnew if a pin or a component is found */
    switch( DrawStruct->Type() )
    {
    case DRAW_PART_TEXT_STRUCT_TYPE:
    case COMPONENT_FIELD_DRAW_TYPE:
        LibItem = (SCH_COMPONENT*) DrawStruct->GetParent();
        SendMessageToPCBNEW( DrawStruct,LibItem );
        break;

    case TYPE_SCH_COMPONENT:
        Pin = LocateAnyPin( GetScreen()->EEDrawList, GetScreen()->m_Curseur, &LibItem );
        if( Pin )
            break; // Priority is probing a pin first
        LibItem = (SCH_COMPONENT*) DrawStruct;
        SendMessageToPCBNEW( DrawStruct, LibItem );
        break;

    default:
        Pin = LocateAnyPin( GetScreen()->EEDrawList, GetScreen()->m_Curseur, &LibItem );
        break;

    case COMPONENT_PIN_DRAW_TYPE:
        Pin = (LibDrawPin*) DrawStruct;
        break;
    }

    if( Pin )
    {
        /* Force display pin infos (the previous display could be a component info) */
        Pin->Display_Infos( this );
        if( LibItem )
            Affiche_1_Parametre( this, 1,
                                 LibItem->GetRef(GetSheet()),
                                 LibItem->GetField(VALUE)->m_Text,
                                 CYAN );

        // Cross probing:2 - pin found, and send a locate pin command to pcbnew (hightlight net)
        SendMessageToPCBNEW( Pin, LibItem );
    }
    return DrawStruct;
}


/************************************************************************************/
SCH_ITEM* WinEDA_SchematicFrame:: SchematicGeneralLocateAndDisplay( const wxPoint& refpoint, bool IncludePin )
/************************************************************************************/

/* Find the schematic item at position "refpoint"
 *  the priority order is:
 *  - marker
 *  - noconnect
 *  - junction
 *  - wire/bus/entry
 *  - label
 *  - pin
 *  - component
 *  return:
 *      an EDA_BaseStruct pointer on the item
 *      a Null pointer if no item found
 *
 *  For some items, caracteristics are displayed on the screen.
 */
{
    SCH_ITEM*         DrawStruct;
    LibDrawPin*             Pin;
    SCH_COMPONENT* LibItem;
    wxString Text;
    wxString msg;
    int      ii;

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), MARKERITEM );
    if( DrawStruct )
    {
        DrawMarkerStruct* Marker = (DrawMarkerStruct*) DrawStruct;
        ii   = Marker->m_Type;
        Text = Marker->GetComment();
        if( Text.IsEmpty() )
            Text = wxT( "NoComment" );
        msg = NameMarqueurType[ii]; msg << wxT( " << " ) << Text;
        Affiche_Message( msg );
        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), NOCONNECTITEM );
    if( DrawStruct )
    {
        MsgPanel->EraseMsgBox();
        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), JUNCTIONITEM );
    if( DrawStruct )
    {
        MsgPanel->EraseMsgBox();
        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), WIREITEM | BUSITEM | RACCORDITEM );
    if( DrawStruct )    // Search for a pin
    {
        Pin = LocateAnyPin( (SCH_ITEM*) m_CurrentSheet->LastDrawList(), refpoint, &LibItem );
        if( Pin )
        {
            Pin->Display_Infos( this );
            if( LibItem )
                Affiche_1_Parametre( this, 1,
                                     LibItem->GetRef(GetSheet()),
                                     LibItem->GetField(VALUE)->m_Text,
                                     CYAN );
        }
        else
            MsgPanel->EraseMsgBox();
        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), FIELDCMPITEM );
    if( DrawStruct )
    {
        SCH_CMP_FIELD* Field = (SCH_CMP_FIELD*) DrawStruct;
        LibItem = (SCH_COMPONENT*) Field->GetParent();
        LibItem->Display_Infos( this );

        return DrawStruct;
    }

    /* search for a pin */
    Pin = LocateAnyPin( (SCH_ITEM*) m_CurrentSheet->LastDrawList(), refpoint, &LibItem );
    if( Pin )
    {
        Pin->Display_Infos( this );
        if( LibItem )
            Affiche_1_Parametre( this, 1,
                                 LibItem->GetRef(GetSheet()),
                                 LibItem->GetField(VALUE)->m_Text,
                                 CYAN );
        if( IncludePin == TRUE )
            return LibItem;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), LIBITEM );
    if( DrawStruct )
    {
        DrawStruct = LocateSmallestComponent( (SCH_SCREEN*)GetScreen() );
        LibItem    = (SCH_COMPONENT*) DrawStruct;
        LibItem->Display_Infos( this );
        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*)PickStruct( refpoint, GetScreen(), SHEETITEM );
    if( DrawStruct )
    {
        ( (DrawSheetStruct*) DrawStruct )->Display_Infos( this );
        return DrawStruct;
    }

    // Recherche des autres elements
    DrawStruct = (SCH_ITEM*)PickStruct( refpoint, GetScreen(), SEARCHALL );
    if( DrawStruct )
    {
        return DrawStruct;
    }

    MsgPanel->EraseMsgBox();
    return NULL;
}


/*************************************************************************************/
void WinEDA_SchematicFrame::GeneralControle( wxDC* DC, wxPoint MousePositionInPixels )
/*************************************************************************************/
{
    wxSize          delta;
    SCH_SCREEN*     screen = GetScreen();
    int             zoom = screen->GetZoom();
    wxPoint         curpos, oldpos;
    int             hotkey = 0;

    ActiveScreen = screen;

    curpos = screen->m_MousePosition;
    oldpos = screen->m_Curseur;

    delta = screen->GetGrid() / zoom;

    if( delta.x <= 0 )
        delta.x = 1;
    if( delta.y <= 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case 0:
        break;

    case WXK_NUMPAD8:       /* Deplacement curseur vers le haut */
    case WXK_UP:
        MousePositionInPixels.y -= delta.y;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD2:       /* Deplacement curseur vers le bas */
    case WXK_DOWN:
        MousePositionInPixels.y += delta.y;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD4:       /* Deplacement curseur vers la gauche */
    case WXK_LEFT:
        MousePositionInPixels.x -= delta.x;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD6:      /* Deplacement curseur vers la droite */
    case WXK_RIGHT:
        MousePositionInPixels.x += delta.x;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    /* Recalcul de la position du curseur schema */
    screen->m_Curseur = curpos;

    /* Placement sur la grille generale */
    PutOnGrid( &(screen->m_Curseur) );

    if( screen->IsRefreshReq() )
    {
        RedrawActiveWindow( DC, TRUE );
    }

    if( oldpos != screen->m_Curseur )
    {
        curpos = screen->m_Curseur;
        screen->m_Curseur = oldpos;
        DrawPanel->CursorOff( DC );
        screen->m_Curseur = curpos;
        DrawPanel->CursorOn( DC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
        }
    }

    if( hotkey )
    {
        if( screen->GetCurItem() && screen->GetCurItem()->m_Flags )
            OnHotKey( DC, hotkey, screen->GetCurItem() );
        else
            OnHotKey( DC, hotkey, NULL );
    }

    Affiche_Status_Box();    /* Affichage des coord curseur */
    SetToolbars();
}


/*************************************************************************************/
void WinEDA_LibeditFrame::GeneralControle( wxDC* DC, wxPoint MousePositionInPixels )
/*************************************************************************************/
{
    wxSize          delta;
    SCH_SCREEN*     screen = GetScreen();
    int             zoom = screen->GetZoom();
    wxPoint         curpos, oldpos;
    int             hotkey = 0;

    ActiveScreen = screen;

    curpos = screen->m_MousePosition;
    oldpos = screen->m_Curseur;

    delta = screen->GetGrid() / zoom;

    if( delta.x <= 0 )
        delta.x = 1;
    if( delta.y <= 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case 0:
        break;

    case WXK_NUMPAD8:       /* Deplacement curseur vers le haut */
    case WXK_UP:
        MousePositionInPixels.y -= delta.y;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD2:       /* Deplacement curseur vers le bas */
    case WXK_DOWN:
        MousePositionInPixels.y += delta.y;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD4:       /* Deplacement curseur vers la gauche */
    case WXK_LEFT:
        MousePositionInPixels.x -= delta.x;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD6:      /* Deplacement curseur vers la droite */
    case WXK_RIGHT:
        MousePositionInPixels.x += delta.x;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    /* Recalcul de la position du curseur schema */
    screen->m_Curseur = curpos;

    /* Placement sur la grille generale */
    PutOnGrid( &(screen->m_Curseur) );

    if( screen->IsRefreshReq() )
    {
        RedrawActiveWindow( DC, TRUE );
    }

    if( oldpos != screen->m_Curseur )
    {
        curpos = screen->m_Curseur;
        screen->m_Curseur = oldpos;
        DrawPanel->CursorOff( DC );
        screen->m_Curseur = curpos;
        DrawPanel->CursorOn( DC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
        }
    }

    if( hotkey )
    {
        if( screen->GetCurItem() && screen->GetCurItem()->m_Flags )
            OnHotKey( DC, hotkey, screen->GetCurItem() );
        else
            OnHotKey( DC, hotkey, NULL );
    }

    Affiche_Status_Box();    /* Affichage des coord curseur */
    SetToolbars();
}

/*************************************************************************************/
void WinEDA_ViewlibFrame::GeneralControle( wxDC* DC, wxPoint MousePositionInPixels )
/*************************************************************************************/
{
    wxSize          delta;
    SCH_SCREEN*     screen = GetScreen();
    int             zoom = screen->GetZoom();
    wxPoint         curpos, oldpos;
    int             hotkey = 0;

    ActiveScreen = screen;

    curpos = screen->m_MousePosition;
    oldpos = screen->m_Curseur;

    delta = screen->GetGrid() / zoom;

    if( delta.x <= 0 )
        delta.x = 1;
    if( delta.y <= 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case 0:
        break;

    case WXK_NUMPAD8:       /* Deplacement curseur vers le haut */
    case WXK_UP:
        MousePositionInPixels.y -= delta.y;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD2:       /* Deplacement curseur vers le bas */
    case WXK_DOWN:
        MousePositionInPixels.y += delta.y;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD4:       /* Deplacement curseur vers la gauche */
    case WXK_LEFT:
        MousePositionInPixels.x -= delta.x;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD6:      /* Deplacement curseur vers la droite */
    case WXK_RIGHT:
        MousePositionInPixels.x += delta.x;
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    /* Recalcul de la position du curseur schema */
    screen->m_Curseur = curpos;

    /* Placement sur la grille generale */
    PutOnGrid( &(screen->m_Curseur) );

    if( screen->IsRefreshReq() )
    {
        RedrawActiveWindow( DC, TRUE );
    }

    if( oldpos != screen->m_Curseur )
    {
        curpos = screen->m_Curseur;
        screen->m_Curseur = oldpos;
        DrawPanel->CursorOff( DC );
        screen->m_Curseur = curpos;
        DrawPanel->CursorOn( DC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
        }
    }

    if( hotkey )
    {
        if( screen->GetCurItem() && screen->GetCurItem()->m_Flags )
            OnHotKey( DC, hotkey, screen->GetCurItem() );
        else
            OnHotKey( DC, hotkey, NULL );
    }

    Affiche_Status_Box();    /* Affichage des coord curseur */
    SetToolbars();
}
