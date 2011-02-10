/****************/
/* controle.cpp */
/****************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "eda_dde.h"
#include "wxEeschemaStruct.h"

#include "eeschema_id.h"
#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "viewlib_frame.h"
#include "lib_draw_item.h"
#include "lib_pin.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"
#include "sch_marker.h"
#include "sch_component.h"


/**
 * Function LocateAndShowItem
 * search the schematic at \a aPosition in logical (drawing) units for any item.
 * <p>
 * The search is first performed at \a aPosition which may be off grid.  If no item is
 * found at \a aPosition, the search is repeated for the nearest grid position to \a
 * aPosition.
 *
 * The search order is as follows:
 * <ul>
 * <li>Marker</li>
 * <li>No Connect</li>
 * <li>Junction</li>
 * <li>Wire, bus, or entry</li>
 * <li>Label</li>
 * <li>Pin</li>
 * <li>Component</li>
 * </ul></p>
 * @param aPosition The wxPoint on the schematic to search.
 * @param aIncludePin = true to search for pins, false to ignore them
 * @return A SCH_ITEM pointer on the item or NULL if no item found
 */
SCH_ITEM* SCH_EDIT_FRAME::LocateAndShowItem( const wxPoint& aPosition, bool aIncludePin )
{
    SCH_ITEM*      item;
    wxString       msg;
    LIB_PIN*       Pin     = NULL;
    SCH_COMPONENT* LibItem = NULL;
    wxPoint        gridPosition = GetScreen()->GetNearestGridPosition( aPosition );

    item = LocateItem( aPosition, aIncludePin );

    if( !item && aPosition != gridPosition )
        item = LocateItem( gridPosition, aIncludePin );

    if( !item )
        return NULL;

    /* Cross probing to pcbnew if a pin or a component is found */
    switch( item->Type() )
    {
    case SCH_FIELD_T:
    case LIB_FIELD_T:
        LibItem = (SCH_COMPONENT*) item->GetParent();
        SendMessageToPCBNEW( item, LibItem );
        break;

    case SCH_COMPONENT_T:
        Pin = GetScreen()->GetPin( GetScreen()->m_Curseur, &LibItem );

        if( Pin )
            break;  // Priority is probing a pin first

        LibItem = (SCH_COMPONENT*) item;
        SendMessageToPCBNEW( item, LibItem );
        break;

    default:
        Pin = GetScreen()->GetPin( GetScreen()->m_Curseur, &LibItem );
        break;

    case LIB_PIN_T:
        Pin = (LIB_PIN*) item;
        break;
    }

    if( Pin )
    {
        // Force display pin information (the previous display could be a component info)
        Pin->DisplayInfo( this );

        if( LibItem )
            AppendMsgPanel( LibItem->GetRef( GetSheet() ),
                            LibItem->GetField( VALUE )->m_Text, DARKCYAN );

        // Cross probing:2 - pin found, and send a locate pin command to pcbnew (highlight net)
        SendMessageToPCBNEW( Pin, LibItem );
    }

    return item;
}


/**
 * Function LocateItem
 * searches for an item at \a aPosition.
 * @param aPosition The wxPoint location where to search.
 * @param aIncludePin True to search for pins, false to ignore them.
 * @return The SCH_ITEM pointer of the item or NULL if no item found.
 */
SCH_ITEM* SCH_EDIT_FRAME::LocateItem( const wxPoint& aPosition, bool aIncludePin )
{
    SCH_ITEM*      item;
    LIB_PIN*       Pin;
    SCH_COMPONENT* LibItem;
    wxString       Text;
    wxString       msg;

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), MARKER_T );

    if( item )
    {
        item->DisplayInfo( this );
        return item;
    }

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), NO_CONNECT_T );

    if( item )
    {
        ClearMsgPanel();
        return item;
    }

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), JUNCTION_T );

    if( item )
    {
        ClearMsgPanel();
        return item;
    }

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), WIRE_T | BUS_T | BUS_ENTRY_T );

    if( item )  // We have found a wire: Search for a connected pin at the same location
    {
        Pin = GetScreen()->GetPin( aPosition, &LibItem );

        if( Pin )
        {
            Pin->DisplayInfo( this );

            if( LibItem )
                AppendMsgPanel( LibItem->GetRef( GetSheet() ),
                                LibItem->GetField( VALUE )->m_Text, DARKCYAN );
        }
        else
            ClearMsgPanel();

        return item;
    }

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), DRAW_ITEM_T );
    if( item )
    {
        ClearMsgPanel();
        return item;
    }

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), FIELD_T );

    if( item )
    {
        SCH_FIELD* Field = (SCH_FIELD*) item;
        LibItem = (SCH_COMPONENT*) Field->GetParent();
        LibItem->DisplayInfo( this );

        return item;
    }

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), LABEL_T | TEXT_T );

    if( item )
    {
        ClearMsgPanel();
        return item;
    }

    /* search for a pin */
    Pin = GetScreen()->GetPin( aPosition, &LibItem );

    if( Pin )
    {
        Pin->DisplayInfo( this );

        if( LibItem )
            AppendMsgPanel( LibItem->GetRef( GetSheet() ),
                            LibItem->GetField( VALUE )->m_Text, DARKCYAN );
        if( aIncludePin )
            return LibItem;
    }

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), COMPONENT_T );

    if( item )
    {
        item = LocateSmallestComponent( GetScreen() );
        LibItem    = (SCH_COMPONENT*) item;
        LibItem->DisplayInfo( this );
        return item;
    }

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), SHEET_T );

    if( item )
    {
        ( (SCH_SHEET*) item )->DisplayInfo( this );
        return item;
    }

    item = (SCH_ITEM*) PickStruct( aPosition, GetScreen(), NO_FILTER_T );

    if( item )
        return item;

    ClearMsgPanel();
    return NULL;
}


void SCH_EDIT_FRAME::GeneralControle( wxDC* aDC, const wxPoint& aPosition )
{
    wxRealPoint gridSize;
    SCH_SCREEN* screen = GetScreen();
    wxPoint     oldpos;
    int         hotkey = 0;
    wxPoint     pos = aPosition;

    PutOnGrid( &pos );
    oldpos = screen->m_Curseur;
    gridSize = screen->GetGridSize();

    switch( g_KeyPressed )
    {
    case 0:
        break;

    case WXK_NUMPAD8:
    case WXK_UP:
        pos.y -= wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        pos.y += wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        pos.x -= wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        pos.x += wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    // Update cursor position.
    screen->m_Curseur = pos;

    if( screen->IsRefreshReq() )
    {
        DrawPanel->Refresh( );
        wxSafeYield();
    }

    if( oldpos != screen->m_Curseur )
    {
        pos = screen->m_Curseur;
        screen->m_Curseur = oldpos;
        DrawPanel->CursorOff( aDC );
        screen->m_Curseur = pos;
        DrawPanel->CursorOn( aDC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, aDC, aPosition, TRUE );
        }
    }

    if( hotkey )
    {
        if( screen->GetCurItem() && screen->GetCurItem()->m_Flags )
            OnHotKey( aDC, hotkey, aPosition, screen->GetCurItem() );
        else
            OnHotKey( aDC, hotkey, aPosition, NULL );
    }

    UpdateStatusBar();    /* Display cursor coordinates info */
    SetToolbars();
}


void LIB_EDIT_FRAME::GeneralControle( wxDC* aDC, const wxPoint& aPosition )
{
    wxRealPoint gridSize;
    SCH_SCREEN* screen = GetScreen();
    wxPoint     oldpos;
    int         hotkey = 0;
    wxPoint     pos = aPosition;

    PutOnGrid( &pos );
    oldpos = screen->m_Curseur;
    gridSize = screen->GetGridSize();

    switch( g_KeyPressed )
    {
    case 0:
        break;

    case WXK_NUMPAD8:
    case WXK_UP:
        pos.y -= wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        pos.y += wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        pos.x -= wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        pos.x += wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    // Update the cursor position.
    screen->m_Curseur = pos;

    if( screen->IsRefreshReq() )
    {
        DrawPanel->Refresh( );
        wxSafeYield();
    }

    if( oldpos != screen->m_Curseur )
    {
        pos = screen->m_Curseur;
        screen->m_Curseur = oldpos;
        DrawPanel->CursorOff( aDC );
        screen->m_Curseur = pos;
        DrawPanel->CursorOn( aDC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, aDC, aPosition, TRUE );
        }
    }

    if( hotkey )
    {
        if( screen->GetCurItem() && screen->GetCurItem()->m_Flags )
            OnHotKey( aDC, hotkey, aPosition, screen->GetCurItem() );
        else
            OnHotKey( aDC, hotkey, aPosition, NULL );
    }

    UpdateStatusBar();
}


void LIB_VIEW_FRAME::GeneralControle( wxDC* aDC, const wxPoint& aPosition )
{
    wxRealPoint gridSize;
    SCH_SCREEN* screen = GetScreen();
    wxPoint     oldpos;
    int         hotkey = 0;
    wxPoint     pos = aPosition;

    PutOnGrid( &pos );
    oldpos = screen->m_Curseur;
    gridSize = screen->GetGridSize();

    switch( g_KeyPressed )
    {
    case 0:
        break;

    case WXK_NUMPAD8:
    case WXK_UP:
        pos.y -= wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        pos.y += wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        pos.x -= wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        pos.x += wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    // Update cursor position.
    screen->m_Curseur = pos;

    if( screen->IsRefreshReq() )
    {
        DrawPanel->Refresh( );
        wxSafeYield();
    }

    if( oldpos != screen->m_Curseur )
    {
        pos = screen->m_Curseur;
        screen->m_Curseur = oldpos;
        DrawPanel->CursorOff( aDC );
        screen->m_Curseur = pos;
        DrawPanel->CursorOn( aDC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, aDC, aPosition, TRUE );
        }
    }

    if( hotkey )
    {
        if( screen->GetCurItem() && screen->GetCurItem()->m_Flags )
            OnHotKey( aDC, hotkey, aPosition, screen->GetCurItem() );
        else
            OnHotKey( aDC, hotkey, aPosition, NULL );
    }

    UpdateStatusBar();
    SetToolbars();
}
