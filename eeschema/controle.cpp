/****************/
/* controle.cpp */
/****************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "eda_dde.h"
#include "eeschema_id.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "viewlib_frame.h"
#include "classes_body_items.h"
#include "class_pin.h"
#include "class_marker_sch.h"


/** Function SchematicGeneralLocateAndDisplay
 * Overlaid function
 *  Find the schematic item at cursor position
 *  the priority order is:
 *  - marker
 *  - noconnect
 *  - junction
 *  - wire/bus/entry
 *  - label
 *  - pin
 *  - component
 * @return  an EDA_BaseStruct pointer on the item or NULL if no item found
 * @param IncludePin = true to search for pins, false to ignore them
 *
 *  For some items, characteristics are displayed on the screen.
 */
SCH_ITEM* WinEDA_SchematicFrame:: SchematicGeneralLocateAndDisplay(
    bool IncludePin )
{
    SCH_ITEM*      DrawStruct;
    wxString       msg;
    wxPoint        mouse_position = GetScreen()->m_MousePosition;
    LIB_PIN*       Pin     = NULL;
    SCH_COMPONENT* LibItem = NULL;

    DrawStruct = SchematicGeneralLocateAndDisplay( mouse_position, IncludePin );
    if( !DrawStruct && ( mouse_position != GetScreen()->m_Curseur) )
    {
        DrawStruct = SchematicGeneralLocateAndDisplay( GetScreen()->m_Curseur,
                                                       IncludePin );
    }
    if( !DrawStruct )
        return NULL;

    /* Cross probing to pcbnew if a pin or a component is found */
    switch( DrawStruct->Type() )
    {
    case DRAW_PART_TEXT_STRUCT_TYPE:
    case COMPONENT_FIELD_DRAW_TYPE:
        LibItem = (SCH_COMPONENT*) DrawStruct->GetParent();
        SendMessageToPCBNEW( DrawStruct, LibItem );
        break;

    case TYPE_SCH_COMPONENT:
        Pin = LocateAnyPin( GetScreen()->EEDrawList, GetScreen()->m_Curseur,
                            &LibItem );
        if( Pin )
            break;  // Priority is probing a pin first
        LibItem = (SCH_COMPONENT*) DrawStruct;
        SendMessageToPCBNEW( DrawStruct, LibItem );
        break;

    default:
        Pin = LocateAnyPin( GetScreen()->EEDrawList, GetScreen()->m_Curseur,
                            &LibItem );
        break;

    case COMPONENT_PIN_DRAW_TYPE:
        Pin = (LIB_PIN*) DrawStruct;
        break;
    }

    if( Pin )
    {
        /* Force display pin information (the previous display could be a
         * component info) */
        Pin->DisplayInfo( this );
        if( LibItem )
            AppendMsgPanel( LibItem->GetRef( GetSheet() ),
                            LibItem->GetField( VALUE )->m_Text, DARKCYAN );

        // Cross probing:2 - pin found, and send a locate pin command to
        // pcbnew (highlight net)
        SendMessageToPCBNEW( Pin, LibItem );
    }
    return DrawStruct;
}


/** Function SchematicGeneralLocateAndDisplay
 * Overlaid function
 *  Find the schematic item at a given position
 *  the priority order is:
 *  - marker
 *  - noconnect
 *  - junction
 *  - wire/bus/entry
 *  - label
 *  - pin
 *  - component
 * @return  an EDA_BaseStruct pointer on the item or NULL if no item found
 * @param refpoint = the wxPoint location where to search
 * @param IncludePin = true to search for pins, false to ignore them
 *
 *  For some items, characteristics are displayed on the screen.
 */
SCH_ITEM* WinEDA_SchematicFrame::SchematicGeneralLocateAndDisplay(
    const wxPoint& refpoint,
    bool
    IncludePin )
{
    SCH_ITEM*      DrawStruct;
    LIB_PIN*       Pin;
    SCH_COMPONENT* LibItem;
    wxString       Text;
    wxString       msg;

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), MARKERITEM );
    if( DrawStruct )
    {
        ClearMsgPanel();
        return DrawStruct;
    }
    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), NOCONNECTITEM );
    if( DrawStruct )
    {
        ClearMsgPanel();
        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), JUNCTIONITEM );
    if( DrawStruct )
    {
        ClearMsgPanel();
        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint,
                                         GetScreen(), WIREITEM | BUSITEM |
                                         RACCORDITEM );
    if( DrawStruct )    // We have found a wire: Search for a connected pin at
                        // the same location
    {
        Pin = LocateAnyPin( (SCH_ITEM*) m_CurrentSheet->LastDrawList(),
                            refpoint, &LibItem );
        if( Pin )
        {
            Pin->DisplayInfo( this );
            if( LibItem )
                AppendMsgPanel( LibItem->GetRef( GetSheet() ),
                                LibItem->GetField( VALUE )->m_Text, DARKCYAN );
        }
        else
            ClearMsgPanel();

        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), FIELDCMPITEM );
    if( DrawStruct )
    {
        SCH_FIELD* Field = (SCH_FIELD*) DrawStruct;
        LibItem = (SCH_COMPONENT*) Field->GetParent();
        LibItem->DisplayInfo( this );

        return DrawStruct;
    }

    /* search for a pin */
    Pin = LocateAnyPin( (SCH_ITEM*) m_CurrentSheet->LastDrawList(), refpoint,
                        &LibItem );
    if( Pin )
    {
        Pin->DisplayInfo( this );
        if( LibItem )
            AppendMsgPanel( LibItem->GetRef( GetSheet() ),
                            LibItem->GetField( VALUE )->m_Text, DARKCYAN );
        if( IncludePin )
            return LibItem;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), LIBITEM );
    if( DrawStruct )
    {
        DrawStruct = LocateSmallestComponent( (SCH_SCREEN*) GetScreen() );
        LibItem    = (SCH_COMPONENT*) DrawStruct;
        LibItem->DisplayInfo( this );
        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), SHEETITEM );
    if( DrawStruct )
    {
        ( (SCH_SHEET*) DrawStruct )->DisplayInfo( this );
        return DrawStruct;
    }

    DrawStruct = (SCH_ITEM*) PickStruct( refpoint, GetScreen(), SEARCHALL );
    if( DrawStruct )
    {
        return DrawStruct;
    }

    ClearMsgPanel();
    return NULL;
}


void WinEDA_SchematicFrame::GeneralControle( wxDC* DC, wxPoint MousePositionInPixels )
{
    wxRealPoint delta;
    SCH_SCREEN* screen = GetScreen();
    wxPoint     curpos, oldpos;
    int         hotkey = 0;
    double      scalar = screen->GetScalingFactor();

    ActiveScreen = screen;

    curpos = screen->m_MousePosition;
    oldpos = screen->m_Curseur;

    delta = screen->GetGridSize();

    delta.x *= scalar;
    delta.y *= scalar;

    if( delta.x <= 0 )
        delta.x = 1;
    if( delta.y <= 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case 0:
        break;

    case WXK_NUMPAD8:
    case WXK_UP:
        MousePositionInPixels.y -= wxRound( delta.y );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        MousePositionInPixels.y += wxRound( delta.y );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        MousePositionInPixels.x -= wxRound( delta.x );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        MousePositionInPixels.x += wxRound( delta.x );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    /* Update cursor position. */
    screen->m_Curseur = curpos;

    /* Snap cursor to grid. */
    PutOnGrid( &(screen->m_Curseur) );

    if( screen->IsRefreshReq() )
    {
        DrawPanel->Refresh( );
        wxSafeYield();
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

    UpdateStatusBar();    /* Display cursor coordinates info */
    SetToolbars();
}


void WinEDA_LibeditFrame::GeneralControle( wxDC* DC, wxPoint MousePositionInPixels )
{
    wxRealPoint delta;
    SCH_SCREEN* screen = GetScreen();
    wxPoint     curpos, oldpos;
    int         hotkey = 0;
    double      scalar = screen->GetScalingFactor();

    ActiveScreen = screen;

    curpos = screen->m_MousePosition;
    oldpos = screen->m_Curseur;

    delta = screen->GetGridSize();

    delta.x *= scalar;
    delta.y *= scalar;

    if( delta.x <= 0 )
        delta.x = 1;
    if( delta.y <= 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case 0:
        break;

    case WXK_NUMPAD8:
    case WXK_UP:
        MousePositionInPixels.y -= wxRound( delta.y );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        MousePositionInPixels.y += wxRound( delta.y );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        MousePositionInPixels.x -= wxRound( delta.x );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        MousePositionInPixels.x += wxRound( delta.x );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    /* Update the cursor position. */
    screen->m_Curseur = curpos;

    /* Snap cursor to grid. */
    PutOnGrid( &(screen->m_Curseur) );

    if( screen->IsRefreshReq() )
    {
        DrawPanel->Refresh( );
        wxSafeYield();
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

    UpdateStatusBar();
}


void WinEDA_ViewlibFrame::GeneralControle( wxDC* DC, wxPoint MousePositionInPixels )
{
    wxRealPoint delta;
    SCH_SCREEN* screen = GetScreen();
    wxPoint     curpos, oldpos;
    int         hotkey = 0;
    double      scalar = screen->GetScalingFactor();

    ActiveScreen = screen;

    curpos = screen->m_MousePosition;
    oldpos = screen->m_Curseur;

    delta = screen->GetGridSize();

    delta.x *= scalar;
    delta.y *= scalar;

    if( delta.x <= 0 )
        delta.x = 1;
    if( delta.y <= 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case 0:
        break;

    case WXK_NUMPAD8:
    case WXK_UP:
        MousePositionInPixels.y -= wxRound( delta.y );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        MousePositionInPixels.y += wxRound( delta.y );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        MousePositionInPixels.x -= wxRound( delta.x );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        MousePositionInPixels.x += wxRound( delta.x );
        DrawPanel->MouseTo( MousePositionInPixels );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    /* Update cursor position. */
    screen->m_Curseur = curpos;

    /* Snap cursor to grid. */
    PutOnGrid( &screen->m_Curseur );

    if( screen->IsRefreshReq() )
    {
        DrawPanel->Refresh( );
        wxSafeYield();
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

    UpdateStatusBar();
    SetToolbars();
}
