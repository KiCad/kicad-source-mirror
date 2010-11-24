/*****************************/
/* hotkeys_module_editor.cpp */
/*****************************/

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "module_editor_frame.h"
#include "pcbnew_id.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_board_design_settings.h"

#include "hotkeys.h"
#include "protos.h"

/* How to add a new hotkey:
 * See hotkeys.cpp
 */


/*****************************************************************************************/
void WinEDA_ModuleEditFrame::OnHotKey( wxDC* aDC, int hotkey, EDA_BaseStruct* DrawStruct )
/*****************************************************************************************/

/* Hot keys. Some commands are relative to the item under the mouse cursor
 *  Commands are case insensitive
 */
{
    if( hotkey == 0 )
        return;

    BOARD_ITEM* item = GetCurItem();
    bool           ItemFree = (item == 0) || (item->m_Flags == 0);
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    /* Convert lower to upper case (the usual toupper function has problem with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';

    Ki_HotkeyInfo* HK_Descr = GetDescriptorFromHotkey( hotkey, common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( hotkey, module_edit_Hotkey_List );

    if( HK_Descr == NULL )
        return;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;
        break;

    case HK_HELP: // Display Current hotkey list
        DisplayHotkeyList( this, g_Module_Editor_Hokeys_Descr );
        break;

    case HK_RESET_LOCAL_COORD: /*Reset the relative coord  */
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;

    case HK_SWITCH_UNITS:
        g_UserUnit = (g_UserUnit == INCHES) ? MILLIMETRES : INCHES;
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
        if( ItemFree )
        {
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED,
                                  HK_Descr->m_IdMenuEvent );
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

bool WinEDA_ModuleEditFrame::OnHotkeyEditItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool itemCurrentlyEdited = item && item->m_Flags;

    if( itemCurrentlyEdited )
        return false;

    item = ModeditLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    int evt_type = 0;       //Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case TYPE_MODULE:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_MODULE;
        break;

    case TYPE_PAD:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_PAD;
        break;

    case TYPE_TEXTE_MODULE:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_TEXTMODULE;
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

bool WinEDA_ModuleEditFrame::OnHotkeyDeleteItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool itemCurrentlyEdited = item && item->m_Flags;

    if( itemCurrentlyEdited )
        return false;

    item = ModeditLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    int evt_type = 0;       //Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case TYPE_PAD:
        if( aIdCommand == HK_DELETE )
            evt_type = ID_POPUP_PCB_DELETE_PAD;
        break;

    case TYPE_TEXTE_MODULE:
        if( aIdCommand == HK_DELETE )
            evt_type = ID_POPUP_PCB_DELETE_TEXTMODULE;
        break;

    case TYPE_EDGE_MODULE:
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

bool WinEDA_ModuleEditFrame::OnHotkeyMoveItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool itemCurrentlyEdited = item && item->m_Flags;

    if( itemCurrentlyEdited )
        return false;

    item = ModeditLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    int evt_type = 0;       //Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case TYPE_PAD:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_PAD_REQUEST;
        break;

    case TYPE_TEXTE_MODULE:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST;
        break;

    case TYPE_EDGE_MODULE:
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

bool WinEDA_ModuleEditFrame::OnHotkeyRotateItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool        itemCurrentlyEdited = item && item->m_Flags;
    int         evt_type = 0; // Used to post a wxCommandEvent on demand

    if( !itemCurrentlyEdited )
        item = ModeditLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    switch( item->Type() )
    {
    case TYPE_TEXTE_MODULE:
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
