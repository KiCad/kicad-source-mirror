/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

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


bool FOOTPRINT_EDIT_FRAME::OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition,
                                     EDA_ITEM* aItem )
{
    if( aHotKey == 0 )
        return false;

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
        return false;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return false;

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

    case HK_MOVE_ITEM_EXACT:
        if( blockActive )
        {
            cmd.SetId( ID_POPUP_MOVE_BLOCK_EXACT );
            GetEventHandler()->ProcessEvent( cmd );
        }
        else
        {
            OnHotkeyMoveItemExact();
        }
        break;

    case HK_ROTATE_ITEM:
        OnHotkeyRotateItem( HK_ROTATE_ITEM );
        break;

    case HK_DUPLICATE_ITEM:
    case HK_DUPLICATE_ITEM_AND_INCREMENT:
        OnHotkeyDuplicateItem( HK_Descr->m_Idcommand );
        break;

    case HK_CREATE_ARRAY:
        PostCommandMenuEvent( ID_POPUP_PCB_CREATE_ARRAY );
    }

    return true;
}


BOARD_ITEM* FOOTPRINT_EDIT_FRAME::PrepareItemForHotkey( bool aFailIfCurrentlyEdited )
{
    BOARD_ITEM* item = GetCurItem();
    bool        itemCurrentlyEdited = item && item->GetFlags();
    bool        blockActive = GetScreen()->m_BlockLocate.GetCommand() != BLOCK_IDLE;

    if( aFailIfCurrentlyEdited )
    {
        if( itemCurrentlyEdited || blockActive )
            return NULL;

        item = ModeditLocateAndDisplay();
    }
    else
    {
        if( blockActive )
            return NULL;

        if( !itemCurrentlyEdited )
            item = ModeditLocateAndDisplay();
    }

    // set item if we can, but don't clear if not
    if( item )
        SetCurItem( item );

    return item;
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyEditItem( int aIdCommand )
{
    BOARD_ITEM* item = PrepareItemForHotkey( true );

    if( item == NULL )
        return false;

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

    return PostCommandMenuEvent( evt_type );
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyDeleteItem( int aIdCommand )
{
    BOARD_ITEM* item = PrepareItemForHotkey( true );

    if( item == NULL )
        return false;

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

    return PostCommandMenuEvent( evt_type );
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyMoveItem( int aIdCommand )
{
    BOARD_ITEM* item = PrepareItemForHotkey( true );

    if( item == NULL )
        return false;

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

    return PostCommandMenuEvent( evt_type );
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyMoveItemExact()
{
    BOARD_ITEM* item = PrepareItemForHotkey( false );

    if( item == NULL )
        return false;

    int evt_type = 0;       // Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case PCB_PAD_T:
    case PCB_MODULE_EDGE_T:
    case PCB_MODULE_TEXT_T:
        evt_type = ID_POPUP_PCB_MOVE_EXACT;
        break;
    default:
        break;
    }

    return PostCommandMenuEvent( evt_type );
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyDuplicateItem( int aIdCommand )
{
    BOARD_ITEM* item = PrepareItemForHotkey( true );

    if( item == NULL )
        return false;

    int evt_type = 0;       // Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case PCB_PAD_T:
    case PCB_MODULE_EDGE_T:
    case PCB_MODULE_TEXT_T:
        if( aIdCommand == HK_DUPLICATE_ITEM )
            evt_type = ID_POPUP_PCB_DUPLICATE_ITEM;
        else
            evt_type = ID_POPUP_PCB_DUPLICATE_ITEM_AND_INCREMENT;

        break;

    default:
        break;
    }

    return PostCommandMenuEvent( evt_type );
}


bool FOOTPRINT_EDIT_FRAME::OnHotkeyRotateItem( int aIdCommand )
{
    BOARD_ITEM* item = PrepareItemForHotkey( false );

    if( item == NULL )
        return false;

    int evt_type = 0;       // Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case PCB_MODULE_TEXT_T:
        if( aIdCommand == HK_ROTATE_ITEM )                      // Rotation
            evt_type = ID_POPUP_PCB_ROTATE_TEXTMODULE;

        break;

    default:
        break;
    }

    return PostCommandMenuEvent( evt_type );
}
