/**
 * @file pagelayout_editor/hotkeys.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <fctsys.h>
#include <common.h>
#include <kicad_device_context.h>
#include <id.h>

#include <class_drawpanel.h>
#include <pl_editor_frame.h>
#include <design_tree_frame.h>
#include <class_worksheet_dataitem.h>
#include <hotkeys.h>
#include <pl_editor_id.h>


// Remark: the hotkey message info is used as keyword in hotkey config files and
// as comments in help windows, therefore translated only when displayed
// they are marked _HKI to be extracted by translation tools
// See hotkeys_basic.h for more info


/* How to add a new hotkey:
 * add a new id in the enum hotkey_id_commnand like MY_NEW_ID_FUNCTION.
 * add a new EDA_HOTKEY entry like:
 * static EDA_HOTKEY HkMyNewEntry(_HKI("Command Label"), MY_NEW_ID_FUNCTION, default key value);
 * 'Command Label' is the name used in hotkey list display, and the identifier in the
 * hotkey list file
 * 'MY_NEW_ID_FUNCTION' is the id event function used in the switch in OnHotKey() function.
 * 'Default key value' is the default hotkey for this command.
 * Can be overrided by the user hotkey list
 * Add the 'HkMyNewEntry' pointer in the  s_PlEditor_Hotkey_List list
 * Add the new code in the switch in OnHotKey() function.
 *
 *  Note: If an hotkey is a special key, be sure the corresponding wxWidget keycode (WXK_XXXX)
 *  is handled in the hotkey_name_descr s_Hotkey_Name_List list (see hotkeys_basic.cpp)
 *  and see this list for some ascii keys (space ...)
 */

// Hotkey list:

// mouse click command:
static EDA_HOTKEY HkMouseLeftClick( _HKI( "Mouse Left Click" ), HK_LEFT_CLICK, WXK_RETURN, 0 );
static EDA_HOTKEY HkMouseLeftDClick( _HKI( "Mouse Left Double Click" ), HK_LEFT_DCLICK, WXK_END, 0 );

static EDA_HOTKEY    HkResetLocalCoord( _HKI( "Reset Local Coordinates" ),
                                        HK_RESET_LOCAL_COORD, ' ' );
static EDA_HOTKEY    HkZoomAuto( _HKI( "Zoom Auto" ), HK_ZOOM_AUTO, WXK_HOME, ID_ZOOM_PAGE );
static EDA_HOTKEY    HkZoomCenter( _HKI( "Zoom Center" ), HK_ZOOM_CENTER, WXK_F4,
                                   ID_POPUP_ZOOM_CENTER );
static EDA_HOTKEY    HkZoomRedraw( _HKI( "Zoom Redraw" ), HK_ZOOM_REDRAW, WXK_F3, ID_ZOOM_REDRAW );
static EDA_HOTKEY    HkZoomOut( _HKI( "Zoom Out" ), HK_ZOOM_OUT, WXK_F2, ID_POPUP_ZOOM_OUT );
static EDA_HOTKEY    HkZoomIn( _HKI( "Zoom In" ), HK_ZOOM_IN, WXK_F1, ID_POPUP_ZOOM_IN );
static EDA_HOTKEY    HkHelp( _HKI( "Help (this window)" ), HK_HELP, '?' );
static EDA_HOTKEY    HkMoveItem( _HKI( "Move Item" ), HK_MOVE_ITEM, 'M', ID_POPUP_ITEM_MOVE );
static EDA_HOTKEY    HkPlaceItem( _HKI( "Place Item" ), HK_PLACE_ITEM, 'P', ID_POPUP_ITEM_PLACE );
static EDA_HOTKEY    HkMoveStartPoint( _HKI( "Move Start Point" ), HK_MOVE_START_POINT, 'S',
                                       ID_POPUP_ITEM_MOVE_START_POINT );
static EDA_HOTKEY    HkMoveEndPoint( _HKI( "Move End Point" ), HK_MOVE_END_POINT, 'E',
                                     ID_POPUP_ITEM_MOVE_END_POINT );
static EDA_HOTKEY    HkDeleteItem( _HKI( "Delete Item" ), HK_DELETE_ITEM, WXK_DELETE,
                                   ID_POPUP_ITEM_DELETE );

// Undo Redo
static EDA_HOTKEY HkUndo( _HKI( "Undo" ), HK_UNDO, GR_KB_CTRL + 'Z', (int) wxID_UNDO );
static EDA_HOTKEY HkRedo( _HKI( "Redo" ), HK_REDO, GR_KB_CTRL + 'Y', (int) wxID_REDO );

// List of common hotkey descriptors
EDA_HOTKEY* s_Common_Hotkey_List[] =
{
    &HkHelp,
    &HkZoomIn,    &HkZoomOut,      &HkZoomRedraw, &HkZoomCenter,
    &HkZoomAuto,  &HkResetLocalCoord,
    &HkUndo, &HkRedo,
    &HkMouseLeftClick,
    &HkMouseLeftDClick,
    NULL
};

EDA_HOTKEY* s_PlEditor_Hotkey_List[] =
{
    &HkMoveItem,    &HkMoveStartPoint,
    &HkMoveEndPoint,
    &HkPlaceItem,
    &HkDeleteItem,
    NULL
};

// list of sections and corresponding hotkey list for Pl_Editor
// (used to create an hotkey config file)
wxString s_PlEditorSectionTag( wxT( "[pl_editor]" ) );
wxString s_PlEditorSectionTitle( wxT( "Part Layout Editor" ) );

struct EDA_HOTKEY_CONFIG s_PlEditor_Hokeys_Descr[] =
{
    { &g_CommonSectionTag,    s_Common_Hotkey_List,     &g_CommonSectionTitle    },
    { &s_PlEditorSectionTag,  s_PlEditor_Hotkey_List,   &s_PlEditorSectionTitle  },
    { NULL,                   NULL,                     NULL                     }
};


bool PL_EDITOR_FRAME::OnHotKey( wxDC* aDC, int aHotkeyCode,
                                const wxPoint& aPosition, EDA_ITEM* aItem )
{
    bool busy = GetScreen()->GetCurItem() != NULL;
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    /* Convert lower to upper case (the usual toupper function has problem with non ascii
     * codes like function keys */
    if( (aHotkeyCode >= 'a') && (aHotkeyCode <= 'z') )
        aHotkeyCode += 'A' - 'a';

    EDA_HOTKEY * HK_Descr = GetDescriptorFromHotkey( aHotkeyCode, s_PlEditor_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( aHotkeyCode, s_Common_Hotkey_List );

    if( HK_Descr == NULL )
        return false;

    WORKSHEET_DATAITEM* item;

    switch( HK_Descr->m_Idcommand )
    {
    case HK_NOT_FOUND:
        return false;

    case HK_LEFT_CLICK:
        OnLeftClick( aDC, aPosition );
        break;

    case HK_LEFT_DCLICK:    // Simulate a double left click: generate 2 events
        OnLeftClick( aDC, aPosition );
        OnLeftDClick( aDC, aPosition );
        break;

    case HK_HELP:       // Display Current hotkey list
        DisplayHotkeyList( this, s_PlEditor_Hokeys_Descr );
        break;

    case HK_UNDO:
    case HK_REDO:
        if( busy )
            break;
        cmd.SetId( HK_Descr->m_IdMenuEvent );
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

    case HK_ZOOM_AUTO:
        cmd.SetId( ID_ZOOM_PAGE );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_RESET_LOCAL_COORD:      // Reset the relative coord
        GetScreen()->m_O_Curseur = GetCrossHairPosition();
        break;

    case HK_SET_GRID_ORIGIN:
        SetGridOrigin( GetCrossHairPosition() );
        break;

    case HK_MOVE_ITEM:
    case HK_MOVE_START_POINT:
    case HK_MOVE_END_POINT:
    case HK_DELETE_ITEM:
        if( busy )
            break;

        if( (item = Locate( aPosition )) == NULL )
            break;

        // Only rect and lines have a end point.
        if( HK_Descr->m_Idcommand == HK_MOVE_END_POINT && !item->HasEndPoint() )
            break;

        if( m_treePagelayout->GetPageLayoutSelectedItem() != item )
            m_treePagelayout->SelectCell( item );

        cmd.SetId( HK_Descr->m_IdMenuEvent );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_PLACE_ITEM:
        if( busy )
        {
            cmd.SetId( HK_Descr->m_IdMenuEvent );
            GetEventHandler()->ProcessEvent( cmd );
        }
        break;

    default:
        wxMessageBox( wxT( "Unknown hotkey" ) );
        return false;
    }

    return true;
}
