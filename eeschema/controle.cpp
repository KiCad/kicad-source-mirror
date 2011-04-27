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
#include "hotkeys.h"
#include "protos.h"
#include "libeditframe.h"
#include "viewlib_frame.h"
#include "lib_draw_item.h"
#include "lib_pin.h"
#include "sch_sheet.h"
#include "sch_sheet_path.h"
#include "sch_marker.h"
#include "sch_component.h"


SCH_ITEM* SCH_EDIT_FRAME::LocateAndShowItem( const wxPoint& aPosition, const KICAD_T aFilterList[],
                                             int aHotKeyCommandId )
{
    SCH_ITEM*      item;
    wxString       msg;
    LIB_PIN*       Pin     = NULL;
    SCH_COMPONENT* LibItem = NULL;
    wxPoint        gridPosition = GetScreen()->GetNearestGridPosition( aPosition );

    // Check the on grid position first.  There is more likely to be multple items on
    // grid than off grid.
    item = LocateItem( gridPosition, aFilterList, aHotKeyCommandId );

    // If the user aborted the clarification context menu, don't show it again at the
    // off grid position.
    if( !item && DrawPanel->m_AbortRequest )
    {
        DrawPanel->m_AbortRequest = false;
        return NULL;
    }

    if( !item && (aPosition != gridPosition) )
        item = LocateItem( aPosition, aFilterList, aHotKeyCommandId );

    if( !item )
    {
        DrawPanel->m_AbortRequest = false;  // Just in case the user aborted the context menu.
        return NULL;
    }

    /* Cross probing to pcbnew if a pin or a component is found */
    switch( item->Type() )
    {
    case SCH_FIELD_T:
    case LIB_FIELD_T:
        LibItem = (SCH_COMPONENT*) item->GetParent();
        SendMessageToPCBNEW( item, LibItem );
        break;

    case SCH_COMPONENT_T:
        LibItem = (SCH_COMPONENT*) item;
        SendMessageToPCBNEW( item, LibItem );
        break;

    case LIB_PIN_T:
        Pin = (LIB_PIN*) item;
        break;

    default:
        ;
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


SCH_ITEM* SCH_EDIT_FRAME::LocateItem( const wxPoint& aPosition, const KICAD_T aFilterList[],
                                      int aHotKeyCommandId )
{
    SCH_ITEM* item = NULL;

    m_collectedItems.Collect( GetScreen()->GetDrawItems(), aFilterList, aPosition );

    if( m_collectedItems.GetCount() == 0 )
    {
        ClearMsgPanel();
    }
    else if( m_collectedItems.GetCount() == 1 )
    {
        item = m_collectedItems[0];
        GetScreen()->SetCurItem( item );
    }
    else
    {
        // There are certain combinations of items that do not need clarification such as
        // a corner were two lines meet or all the items form a junction.
        if( aHotKeyCommandId )
        {
            switch( aHotKeyCommandId )
            {
            case HK_DRAG:
                if( m_collectedItems.IsCorner() || m_collectedItems.IsNode( false ) )
                {
                    item = m_collectedItems[0];
                    GetScreen()->SetCurItem( item );
                }
            default:
                ;
            }
        }

        if( item == NULL )
        {
            wxASSERT_MSG( m_collectedItems.GetCount() <= MAX_SELECT_ITEM_IDS,
                          wxT( "Select item clarification context menu size limit exceeded." ) );

            wxMenu selectMenu;
            wxMenuItem* title = new wxMenuItem( &selectMenu, wxID_NONE, _( "Clarify Selection" ) );

            selectMenu.Append( title );
            selectMenu.AppendSeparator();

            for( int i = 0;  i < m_collectedItems.GetCount() && i < MAX_SELECT_ITEM_IDS;  i++ )
            {
                wxString text = m_collectedItems[i]->GetSelectMenuText();
                const char** xpm = m_collectedItems[i]->GetMenuImage();
                ADD_MENUITEM( &selectMenu, ID_SELECT_ITEM_START + i, text, xpm );
            }

            // Set to NULL in case user aborts the clarification context menu.
            GetScreen()->SetCurItem( NULL );
            DrawPanel->m_AbortRequest = true;   // Changed to false if an item is selected
            PopupMenu( &selectMenu );
            DrawPanel->MoveCursorToCrossHair();
            item = GetScreen()->GetCurItem();
        }
    }

    if( item )
        item->DisplayInfo( this );
    else
        ClearMsgPanel();

    return item;
}


void SCH_EDIT_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    wxRealPoint gridSize;
    SCH_SCREEN* screen = GetScreen();
    wxPoint     oldpos;
    wxPoint     pos = aPosition;

    pos = screen->GetNearestGridPosition( pos );
    oldpos = screen->GetCrossHairPosition();
    gridSize = screen->GetGridSize();

    switch( aHotKey )
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
        break;
    }

    // Update cursor position.
    screen->SetCrossHairPosition( pos );

    if( oldpos != screen->GetCrossHairPosition() )
    {
        pos = screen->GetCrossHairPosition();
        screen->SetCrossHairPosition( oldpos );
        DrawPanel->CrossHairOff( aDC );
        screen->SetCrossHairPosition( pos );
        DrawPanel->CrossHairOn( aDC );

        if( DrawPanel->IsMouseCaptured() )
        {
            DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, aPosition, true );
        }
    }

    if( aHotKey )
    {
        if( screen->GetCurItem() && screen->GetCurItem()->GetFlags() )
            OnHotKey( aDC, aHotKey, aPosition, screen->GetCurItem() );
        else
            OnHotKey( aDC, aHotKey, aPosition, NULL );
    }

    UpdateStatusBar();    /* Display cursor coordinates info */
}


void LIB_EDIT_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    wxRealPoint gridSize;
    SCH_SCREEN* screen = GetScreen();
    wxPoint     oldpos;
    wxPoint     pos = aPosition;

    pos = screen->GetNearestGridPosition( pos );
    oldpos = screen->GetCrossHairPosition();
    gridSize = screen->GetGridSize();

    switch( aHotKey )
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
        break;
    }

    // Update the cursor position.
    screen->SetCrossHairPosition( pos );

    if( oldpos != screen->GetCrossHairPosition() )
    {
        pos = screen->GetCrossHairPosition();
        screen->SetCrossHairPosition( oldpos );
        DrawPanel->CrossHairOff( aDC );
        screen->SetCrossHairPosition( pos );
        DrawPanel->CrossHairOn( aDC );

        if( DrawPanel->IsMouseCaptured() )
        {
            DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, aPosition, true );
        }
    }

    if( aHotKey )
    {
        OnHotKey( aDC, aHotKey, aPosition, NULL );
    }

    UpdateStatusBar();
}


void LIB_VIEW_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    wxRealPoint gridSize;
    SCH_SCREEN* screen = GetScreen();
    wxPoint     oldpos;
    wxPoint     pos = aPosition;

    pos = screen->GetNearestGridPosition( pos );
    oldpos = screen->GetCrossHairPosition();
    gridSize = screen->GetGridSize();

    switch( aHotKey )
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
        break;
    }

    // Update cursor position.
    screen->SetCrossHairPosition( pos );

    if( oldpos != screen->GetCrossHairPosition() )
    {
        pos = screen->GetCrossHairPosition();
        screen->SetCrossHairPosition( oldpos );
        DrawPanel->CrossHairOff( aDC );
        screen->SetCrossHairPosition( pos );
        DrawPanel->CrossHairOn( aDC );

        if( DrawPanel->IsMouseCaptured() )
        {
            DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, aPosition, true );
        }
    }

    if( aHotKey )
    {
        if( screen->GetCurItem() && screen->GetCurItem()->GetFlags() )
            OnHotKey( aDC, aHotKey, aPosition, screen->GetCurItem() );
        else
            OnHotKey( aDC, aHotKey, aPosition, NULL );
    }

    UpdateStatusBar();
}
