/*************************************/
/* Modules de creations de Bus Entry */
/*************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "eeschema_id.h"
#include "confirm.h"

#include "program.h"
#include "general.h"
#include "protos.h"

/* Routines Locales */

/* Variables locales */
static int     s_LastShape = '\\';
static wxPoint ItemInitialPosition;

/**************************************************************/
static void ExitBusEntry( WinEDA_DrawPanel* Panel, wxDC* DC )
/**************************************************************/
/* Routine de sortie des menus de trace */
{
    DrawBusEntryStruct* BusEntry =
        (DrawBusEntryStruct*) Panel->GetScreen()->GetCurItem();

    if( BusEntry )  /* trace en cours */
    {
        RedrawOneStruct( Panel, DC, BusEntry, g_XorMode );
        if( BusEntry->m_Flags & IS_NEW )
        {
            delete BusEntry;
            Panel->GetScreen()->SetCurItem( NULL );
        }
        else
        {
            BusEntry->m_Pos = ItemInitialPosition;
            RedrawOneStruct( Panel, DC, BusEntry, GR_DEFAULT_DRAWMODE );
            BusEntry->m_Flags = 0;
        }
    }

    g_ItemToRepeat = NULL;
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
}


/************************************************************************/
static void ShowWhileMoving( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/************************************************************************/

/*  Dessin du Segment "BusEntry" lors des deplacements du curseur
 */
{
    BASE_SCREEN*        screen   = panel->GetScreen();
    DrawBusEntryStruct* BusEntry = (DrawBusEntryStruct*) screen->GetCurItem();

    if( BusEntry == NULL )
        return;

    /* effacement apres deplacement curseur */
    if( erase )
        RedrawOneStruct( panel, DC, BusEntry, g_XorMode );

    /* Reaffichage au bon endroit */
    BusEntry->m_Pos = screen->m_Curseur;
    RedrawOneStruct( panel, DC, BusEntry, g_XorMode );
}


/**********************************************************************************/
DrawBusEntryStruct* WinEDA_SchematicFrame::CreateBusEntry( wxDC* DC, int entry_type )
/**********************************************************************************/

/* Create a new bus entry, and prepare moving function (for later place it)
 */
{
    DrawBusEntryStruct* BusEntry = new DrawBusEntryStruct( GetScreen()->m_Curseur,
                                                           s_LastShape, entry_type );

    BusEntry->m_Flags = IS_NEW;

    DrawPanel->CursorOff( DC );     // Erase schematic cursor
    RedrawOneStruct( DrawPanel, DC, BusEntry, g_XorMode );
    DrawPanel->CursorOn( DC );      // Display schematic cursor

    GetScreen()->SetModify();

    StartMoveBusEntry( BusEntry, DC );
    return BusEntry;
}


/**************************************************************************/
void WinEDA_SchematicFrame::StartMoveBusEntry( DrawBusEntryStruct* BusEntry,
                                               wxDC*               DC )
/**************************************************************************/
{
    if( BusEntry == NULL )
        return;

    if( (BusEntry->m_Flags & IS_NEW) == 0 )    // => not already in edit, save shape */
    {
        delete g_ItemToUndoCopy;
        g_ItemToUndoCopy = BusEntry->GenCopy();
    }

    BusEntry->m_Flags |= IS_MOVED;

    ItemInitialPosition = BusEntry->m_Pos;

    DrawPanel->CursorOff( DC );
    GetScreen()->m_Curseur = BusEntry->m_Pos;
    DrawPanel->MouseToCursorSchema();

    GetScreen()->SetCurItem( BusEntry );
    DrawPanel->ManageCurseur = ShowWhileMoving;
    DrawPanel->ForceCloseManageCurseur = ExitBusEntry;

    DrawPanel->CursorOn( DC );
}


/************************************************************/
void WinEDA_SchematicFrame::SetBusEntryShape( wxDC* DC,
                                              DrawBusEntryStruct* BusEntry, int entry_shape )
/************************************************************/

/* set the shape of BusEntry (shape = / or \ )
 */
{
    if( BusEntry == NULL )
        return;

    if( BusEntry->Type() != DRAW_BUSENTRY_STRUCT_TYPE )
    {
        DisplayError( this, wxT( "SetBusEntryType: Bad StructType" ) );
        return;
    }

    /* Put old item in undo list if it is not currently in edit */
    if( BusEntry->m_Flags == 0 )
        SaveCopyInUndoList( BusEntry, UR_CHANGED );

    RedrawOneStruct( DrawPanel, DC, BusEntry, g_XorMode );

    switch( entry_shape )
    {
    case '\\':
        s_LastShape = '\\';
        BusEntry->m_Size.y = 100;
        break;

    case '/':
        s_LastShape = '/';
        BusEntry->m_Size.y = -100;
        break;
    }

    TestDanglingEnds( GetScreen()->EEDrawList, NULL );
    RedrawOneStruct( DrawPanel, DC, BusEntry, g_XorMode );
    GetScreen()->SetModify();
}


/************************************************************************/
int WinEDA_SchematicFrame::GetBusEntryShape( DrawBusEntryStruct* BusEntry )
/************************************************************************/
{
    int entry_shape = '\\';

    if( BusEntry->m_Size.y < 0 )
        entry_shape = '/';
    return entry_shape;
}
