/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 <Jean-Pierre Charras>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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
 * @file gerbview/hotkeys.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <kicad_device_context.h>
#include <id.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <class_drawpanel.h>
#include <hotkeys.h>
#include <gerbview_id.h>


/* How to add a new hotkey:
 *  add a new id in the enum hotkey_id_commnand like MY_NEW_ID_FUNCTION.
 *  add a new EDA_HOTKEY entry like:
 *  static EDA_HOTKEY HkMyNewEntry(wxT("Command Label"), MY_NEW_ID_FUNCTION, default key value);
 *      "Command Label" is the name used in hotkey list display, and the identifier in the
 *      hotkey list file MY_NEW_ID_FUNCTION is an equivalent id function used in the switch
 *      in OnHotKey() function.   default key value is the default hotkey for this command.
 *      Can be overrided by the user hotkey list file add the HkMyNewEntry pointer in the
 *      s_board_edit_Hotkey_List list ( or/and the s_module_edit_Hotkey_List list)  Add the
 *      new code in the switch in OnHotKey() function.   when the variable PopupOn is true,
 *      an item is currently edited.  This can be usefull if the new function cannot be
 *      executed while an item is currently being edited
 *  ( For example, one cannot start a new wire when a component is moving.)
 *
 *  Note: If an hotkey is a special key, be sure the corresponding wxWidget keycode (WXK_XXXX)
 *  is handled in the hotkey_name_descr s_Hotkey_Name_List list (see hotkeys_basic.cpp)
 *  and see this list for some ascii keys (space ...)
 */

// local variables
// Hotkey list:
static EDA_HOTKEY   HkZoomAuto( _HKI( "Zoom Auto" ), HK_ZOOM_AUTO, WXK_HOME );
static EDA_HOTKEY   HkZoomCenter( _HKI( "Zoom Center" ), HK_ZOOM_CENTER, WXK_F4 );
static EDA_HOTKEY   HkZoomRedraw( _HKI( "Zoom Redraw" ), HK_ZOOM_REDRAW, WXK_F3 );
static EDA_HOTKEY   HkZoomOut( _HKI( "Zoom Out" ), HK_ZOOM_OUT, WXK_F2 );
static EDA_HOTKEY   HkZoomIn( _HKI( "Zoom In" ), HK_ZOOM_IN, WXK_F1 );
static EDA_HOTKEY   HkHelp( _HKI( "Help (this window)" ), HK_HELP, '?' );
static EDA_HOTKEY   HkSwitchUnits( _HKI( "Switch Units" ), HK_SWITCH_UNITS, 'U' );
static EDA_HOTKEY   HkResetLocalCoord( _HKI( "Reset Local Coordinates" ), HK_RESET_LOCAL_COORD, ' ' );

static EDA_HOTKEY   HkLinesDisplayMode( _HKI( "Gbr Lines Display Mode" ), HK_GBR_LINES_DISPLAY_MODE, 'L' );
static EDA_HOTKEY   HkFlashedDisplayMode( _HKI( "Gbr Flashed Display Mode" ), HK_GBR_FLASHED_DISPLAY_MODE, 'F' );
static EDA_HOTKEY   HkPolygonDisplayMode( _HKI( "Gbr Polygons Display Mode" ), HK_GBR_POLYGON_DISPLAY_MODE, 'P' );
static EDA_HOTKEY   HkNegativeObjDisplayMode( _HKI( "Gbr Negative Obj Display Mode" ), HK_GBR_NEGATIVE_DISPLAY_ONOFF, 'N' );
static EDA_HOTKEY   HkDCodesDisplayMode( _HKI( "DCodes Display Mode" ), HK_GBR_DCODE_DISPLAY_ONOFF, 'D' );

static EDA_HOTKEY   HkSwitch2NextCopperLayer( _HKI( "Switch to Next Layer" ), HK_SWITCH_LAYER_TO_NEXT, '+' );
static EDA_HOTKEY   HkSwitch2PreviousCopperLayer( _HKI( "Switch to Previous Layer" ), HK_SWITCH_LAYER_TO_PREVIOUS, '-' );

// List of common hotkey descriptors
EDA_HOTKEY* gerbviewHotkeyList[] = {
    &HkHelp,
    &HkZoomIn,                      &HkZoomOut,         &HkZoomRedraw,  &HkZoomCenter,
    &HkZoomAuto,    &HkSwitchUnits, &HkResetLocalCoord,
    &HkLinesDisplayMode, &HkFlashedDisplayMode, &HkPolygonDisplayMode,
    &HkDCodesDisplayMode, &HkNegativeObjDisplayMode,
    &HkSwitch2NextCopperLayer,
    &HkSwitch2PreviousCopperLayer,
    NULL
};


// list of sections and corresponding hotkey list for GerbView (used to create an hotkey
// config file)
static wxString gerbviewSectionTag( wxT( "[gerbview]" ) );
static wxString gerbviewSectionTitle( _HKI( "Gerbview Hotkeys" ) );

struct EDA_HOTKEY_CONFIG GerbviewHokeysDescr[] =
{
    { &gerbviewSectionTag, gerbviewHotkeyList, &gerbviewSectionTitle  },
    { NULL,                NULL,               NULL  }
};


bool GERBVIEW_FRAME::OnHotKey( wxDC* aDC, int aHotkeyCode, const wxPoint& aPosition, EDA_ITEM* aItem )
{
    #define CHANGE( x ) ( x ) = not (x )

    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    /* Convert lower to upper case (the usual toupper function has problem with non ascii
     * codes like function keys */
    if( (aHotkeyCode >= 'a') && (aHotkeyCode <= 'z') )
        aHotkeyCode += 'A' - 'a';

    EDA_HOTKEY * HK_Descr = GetDescriptorFromHotkey( aHotkeyCode, gerbviewHotkeyList );

    if( HK_Descr == NULL )
        return false;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return false;

    case HK_HELP:       // Display Current hotkey list
        DisplayHotkeyList( this, GerbviewHokeysDescr );
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

    case HK_RESET_LOCAL_COORD:         // Reset the relative coord
        GetScreen()->m_O_Curseur = GetCrossHairPosition();
        break;

    case HK_SWITCH_UNITS:
        g_UserUnit = (g_UserUnit == INCHES ) ? MILLIMETRES : INCHES;
        break;

    case HK_GBR_LINES_DISPLAY_MODE:
        CHANGE(  m_DisplayOptions.m_DisplayLinesFill );
        m_canvas->Refresh();
        break;

    case HK_GBR_FLASHED_DISPLAY_MODE:
        CHANGE( m_DisplayOptions.m_DisplayFlashedItemsFill );
        m_canvas->Refresh( true );
        break;

    case HK_GBR_POLYGON_DISPLAY_MODE:
        CHANGE( m_DisplayOptions.m_DisplayPolygonsFill );
        m_canvas->Refresh();
        break;

    case HK_GBR_NEGATIVE_DISPLAY_ONOFF:
        SetElementVisibility( NEGATIVE_OBJECTS_VISIBLE, not IsElementVisible( NEGATIVE_OBJECTS_VISIBLE ) );
        m_canvas->Refresh();
        break;

    case HK_GBR_DCODE_DISPLAY_ONOFF:
        SetElementVisibility( DCODES_VISIBLE, not IsElementVisible( DCODES_VISIBLE ) );
        m_canvas->Refresh();
        break;

    case HK_SWITCH_LAYER_TO_PREVIOUS:
        if( getActiveLayer() > 0 )
        {
            setActiveLayer( getActiveLayer() - 1 );
            m_canvas->Refresh();
        }
        break;

    case HK_SWITCH_LAYER_TO_NEXT:
        if( getActiveLayer() < 31 )
        {
            setActiveLayer( getActiveLayer() + 1 );
            m_canvas->Refresh();
        }
        break;
    }

    return true;
}
