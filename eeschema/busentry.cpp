/*****************************************************/
/* Code to handle manipulation on bus entry objects. */
/*****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "eeschema_id.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_bus_entry.h"


static int     s_LastShape = '\\';
static wxPoint ItemInitialPosition;


static void ExitBusEntry( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    /* Exit bus entry mode. */
    SCH_BUS_ENTRY* BusEntry = (SCH_BUS_ENTRY*) Panel->GetScreen()->GetCurItem();

    if( BusEntry )
    {
        BusEntry->Draw( Panel, DC, wxPoint( 0, 0 ), g_XorMode );

        if( BusEntry->m_Flags & IS_NEW )
        {
            delete BusEntry;
            Panel->GetScreen()->SetCurItem( NULL );
        }
        else
        {
            BusEntry->m_Pos = ItemInitialPosition;
            BusEntry->Draw( Panel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
            BusEntry->m_Flags = 0;
        }
    }

    SCH_EDIT_FRAME* parent = ( SCH_EDIT_FRAME* ) Panel->GetParent();

    parent->SetRepeatItem( NULL );
}


static void ShowWhileMoving( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                             bool aErase )
{
    // Draws the bus entry while moving the cursor
    BASE_SCREEN*   screen   = aPanel->GetScreen();
    SCH_BUS_ENTRY* BusEntry = (SCH_BUS_ENTRY*) screen->GetCurItem();

    if( BusEntry == NULL )
        return;

    /* Erase the last segment position. */
    if( aErase )
        BusEntry->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    /* Redraw at the new position. */
    BusEntry->m_Pos = screen->GetCrossHairPosition();
    BusEntry->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
}


SCH_BUS_ENTRY* SCH_EDIT_FRAME::CreateBusEntry( wxDC* DC, int entry_type )
{
    // Create and place a new bus entry at cursor position
    SCH_BUS_ENTRY* BusEntry = new SCH_BUS_ENTRY( GetScreen()->GetCrossHairPosition(), s_LastShape,
                                                 entry_type );
    BusEntry->m_Flags = IS_NEW;
    BusEntry->Place( this, DC );;
    OnModify();
    return BusEntry;
}


void SCH_EDIT_FRAME::StartMoveBusEntry( SCH_BUS_ENTRY* BusEntry, wxDC* DC )
{
    if( BusEntry == NULL )
        return;

    if( (BusEntry->m_Flags & IS_NEW) == 0 )    // not already in edit, save shape
    {
        delete g_ItemToUndoCopy;
        g_ItemToUndoCopy = BusEntry->Clone();
    }

    BusEntry->m_Flags |= IS_MOVED;

    ItemInitialPosition = BusEntry->m_Pos;

    DrawPanel->CrossHairOff( DC );
    GetScreen()->SetCrossHairPosition( BusEntry->m_Pos );
    DrawPanel->MoveCursorToCrossHair();

    GetScreen()->SetCurItem( BusEntry );
    DrawPanel->m_mouseCaptureCallback = ShowWhileMoving;
    DrawPanel->m_endMouseCaptureCallback = ExitBusEntry;

    DrawPanel->CrossHairOn( DC );
}


/* set the shape of BusEntry (shape = / or \ )
 */
void SCH_EDIT_FRAME::SetBusEntryShape( wxDC* DC, SCH_BUS_ENTRY* BusEntry, int entry_shape )
{
    if( BusEntry == NULL )
        return;

    if( BusEntry->Type() != SCH_BUS_ENTRY_T )
    {
        DisplayError( this, wxT( "SetBusEntryType: Bad StructType" ) );
        return;
    }

    /* Put old item in undo list if it is not currently in edit */
    if( BusEntry->m_Flags == 0 )
        SaveCopyInUndoList( BusEntry, UR_CHANGED );

    BusEntry->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );

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

    GetScreen()->TestDanglingEnds();
    BusEntry->Draw( DrawPanel, DC, wxPoint( 0, 0 ), g_XorMode );
    OnModify( );
}


int SCH_EDIT_FRAME::GetBusEntryShape( SCH_BUS_ENTRY* BusEntry )
{
    int entry_shape = '\\';

    if( BusEntry->m_Size.y < 0 )
        entry_shape = '/';
    return entry_shape;
}
