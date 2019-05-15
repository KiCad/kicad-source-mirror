/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint_edit_frame.h>
#include <pcbnew_id.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <board_design_settings.h>
#include <origin_viewitem.h>
#include <tools/pcbnew_control.h>

#include <hotkeys.h>

/* How to add a new hotkey:
 * See hotkeys.cpp
 */

EDA_HOTKEY* FOOTPRINT_EDIT_FRAME::GetHotKeyDescription( int aCommand ) const
{
    EDA_HOTKEY* HK_Descr = GetDescriptorFromCommand( aCommand, common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromCommand( aCommand, module_edit_Hotkey_List );

    return HK_Descr;
}


bool FOOTPRINT_EDIT_FRAME::OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition,
                                     EDA_ITEM* aItem )
{
    if( aHotKey == 0 )
        return false;

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
        DisplayHotkeyList( this, g_Module_Editor_Hotkeys_Descr );
        break;

    case HK_PREFERENCES:
        cmd.SetId( wxID_PREFERENCES );
        GetEventHandler()->ProcessEvent( cmd );
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
        PCBNEW_CONTROL::SetGridOrigin( GetGalCanvas()->GetView(), this,
                                       new KIGFX::ORIGIN_VIEWITEM( GetGridOrigin(), UR_TRANSIENT ),
                                       GetCrossHairPosition() );
        m_canvas->Refresh();
        break;

    case HK_RESET_GRID_ORIGIN:
        PCBNEW_CONTROL::SetGridOrigin( GetGalCanvas()->GetView(), this,
                                       new KIGFX::ORIGIN_VIEWITEM( GetGridOrigin(), UR_TRANSIENT ),
                                       wxPoint( 0, 0 ) );
        m_canvas->Refresh();
        break;

    case HK_CREATE_ARRAY:
        PostCommandMenuEvent( ID_POPUP_PCB_CREATE_ARRAY );
    }

    return true;
}


BOARD_ITEM* FOOTPRINT_EDIT_FRAME::PrepareItemForHotkey( bool aFailIfCurrentlyEdited )
{
    BOARD_ITEM* item = GetCurItem();
    bool        itemCurrentlyEdited = item && item->GetEditFlags();
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


