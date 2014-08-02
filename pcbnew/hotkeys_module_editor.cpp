/**
 * @file hotkeys_module_editor.cpp
 */

#include <fctsys.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <module_editor_frame.h>
#include <pcbnew_id.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <class_board_design_settings.h>

#include <hotkeys.h>

/* How to add a new hotkey:
 * See hotkeys.cpp
 */


void FOOTPRINT_EDIT_FRAME::OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition,
                                     EDA_ITEM* aItem )
{
    if( aHotKey == 0 )
        return;

    bool           blockActive = GetScreen()->m_BlockLocate.GetCommand() != BLOCK_IDLE;
    BOARD_ITEM*    item     = GetCurItem();
    bool           ItemFree = (item == 0) || (item->GetFlags() == 0);
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    /* Convert lower to upper case (the usual toupper function has problem with non ascii
     * codes like function keys */
    if( (aHotKey >= 'a') && (aHotKey <= 'z') )
        aHotKey += 'A' - 'a';

    EDA_HOTKEY* HK_Descr = GetDescriptorFromHotkey( aHotKey, common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( aHotKey, module_edit_Hotkey_List );

    if( HK_Descr == NULL )
        return;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;
        break;

    case HK_HELP:                   // Display Current hotkey list
        DisplayHotkeyList( this, g_Module_Editor_Hokeys_Descr );
        break;

    case HK_RESET_LOCAL_COORD:      // set local (relative) coordinate origin
        GetScreen()->m_O_Curseur = GetCrossHairPosition();
        break;

    case HK_LEFT_CLICK:
        OnLeftClick( aDC, aPosition );
        break;

    case HK_LEFT_DCLICK:    // Simulate a double left click: generate 2 events
        OnLeftClick( aDC, aPosition );
        OnLeftDClick( aDC, aPosition );
        break;

    case HK_SET_GRID_ORIGIN:
        SetGridOrigin( GetCrossHairPosition() );
        m_canvas->Refresh();
        break;

    case HK_RESET_GRID_ORIGIN:
        SetGridOrigin( wxPoint(0,0) );
        m_canvas->Refresh();
        break;

    case HK_SWITCH_UNITS:
        cmd.SetId( (g_UserUnit == INCHES) ?
                    ID_TB_OPTIONS_SELECT_UNIT_MM : ID_TB_OPTIONS_SELECT_UNIT_INCH );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_IN:
        cmd.SetId( ID_POPUP_ZOOM_IN );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_OUT:
        cmd.SetId( ID_POPUP_ZOOM_OUT );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_REDRAW:
        cmd.SetId( ID_ZOOM_REDRAW );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_CENTER:
        cmd.SetId( ID_POPUP_ZOOM_CENTER );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_UNDO:
    case HK_REDO:
        if( ItemFree && !blockActive )
        {
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED, HK_Descr->m_IdMenuEvent );
            wxPostEvent( this, event );
        }
        break;

    case HK_ZOOM_AUTO:
        cmd.SetId( ID_ZOOM_PAGE );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_EDIT_ITEM:
        OnHotkeyEditItem( HK_EDIT_ITEM );
        break;

    case HK_DELETE:
        OnHotkeyDeleteItem( HK_DELETE );
        break;

    case HK_MOVE_ITEM:
        OnHotkeyMoveItem( HK_MOVE_ITEM );
        break;

    case HK_ROTATE_ITEM:
        OnHotkeyRotateItem( HK_ROTATE_ITEM );
        break;
    }
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyEditItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool        itemCurrentlyEdited = item && item->GetFlags();
    bool        blockActive = GetScreen()->m_BlockLocate.GetCommand() != BLOCK_IDLE;

    if( itemCurrentlyEdited || blockActive )
        return false;

    item = ModeditLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    int evt_type = 0;       // Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case PCB_MODULE_T:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_MODULE_PRMS;

        break;

    case PCB_PAD_T:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_PAD;

        break;

    case PCB_MODULE_TEXT_T:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_TEXTMODULE;

        break;

    case PCB_MODULE_EDGE_T:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_MODEDIT_EDIT_BODY_ITEM;

        break;

    default:
        break;
    }

    if( evt_type != 0 )
    {
        wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );
        evt.SetEventObject( this );
        evt.SetId( evt_type );
        wxPostEvent( this, evt );
        return true;
    }

    return false;
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyDeleteItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool        itemCurrentlyEdited = item && item->GetFlags();
    bool        blockActive = GetScreen()->m_BlockLocate.GetCommand() != BLOCK_IDLE;

    if( itemCurrentlyEdited || blockActive )
        return false;

    item = ModeditLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    int evt_type = 0;       // Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case PCB_PAD_T:
        if( aIdCommand == HK_DELETE )
            evt_type = ID_POPUP_PCB_DELETE_PAD;

        break;

    case PCB_MODULE_TEXT_T:
        if( aIdCommand == HK_DELETE )
            evt_type = ID_POPUP_PCB_DELETE_TEXTMODULE;

        break;

    case PCB_MODULE_EDGE_T:
        if( aIdCommand == HK_DELETE )
            evt_type = ID_POPUP_PCB_DELETE_EDGE;

        break;

    default:
        break;
    }

    if( evt_type != 0 )
    {
        wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );
        evt.SetEventObject( this );
        evt.SetId( evt_type );
        wxPostEvent( this, evt );
        return true;
    }

    return false;
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyMoveItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool        itemCurrentlyEdited = item && item->GetFlags();
    bool        blockActive = GetScreen()->m_BlockLocate.GetCommand() != BLOCK_IDLE;

    if( itemCurrentlyEdited || blockActive )
        return false;

    item = ModeditLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    int evt_type = 0;       // Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case PCB_PAD_T:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_PAD_REQUEST;

        break;

    case PCB_MODULE_TEXT_T:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST;

        break;

    case PCB_MODULE_EDGE_T:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_EDGE;

        break;

    default:
        break;
    }

    if( evt_type != 0 )
    {
        wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );
        evt.SetEventObject( this );
        evt.SetId( evt_type );
        wxPostEvent( this, evt );
        return true;
    }

    return false;
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyRotateItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool        itemCurrentlyEdited = item && item->GetFlags();
    int         evt_type    = 0; // Used to post a wxCommandEvent on demand
    bool        blockActive = GetScreen()->m_BlockLocate.GetCommand() != BLOCK_IDLE;

    if( blockActive )
        return false;

    if( !itemCurrentlyEdited )
        item = ModeditLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    switch( item->Type() )
    {
    case PCB_MODULE_TEXT_T:
        if( aIdCommand == HK_ROTATE_ITEM )                      // Rotation
            evt_type = ID_POPUP_PCB_ROTATE_TEXTMODULE;

        break;

    default:
        break;
    }

    if( evt_type != 0 )
    {
        wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );
        evt.SetEventObject( this );
        evt.SetId( evt_type );
        wxPostEvent( this, evt );
        return true;
    }

    return false;
}
