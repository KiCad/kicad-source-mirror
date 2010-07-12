/***************/
/* hotkeys.cpp */
/***************/

#include "fctsys.h"
#include "common.h"
#include "hotkeys.h"

#include "gerbview.h"
#include "class_drawpanel.h"

/* How to add a new hotkey:
 *  add a new id in the enum hotkey_id_commnand like MY_NEW_ID_FUNCTION.
 *  add a new Ki_HotkeyInfo entry like:
 *  static Ki_HotkeyInfo HkMyNewEntry(wxT("Command Label"), MY_NEW_ID_FUNCTION, default key value);
 *      "Command Label" is the name used in hotkey list display, and the identifier in the hotkey list file
 *      MY_NEW_ID_FUNCTION is an equivalent id function used in the switch in OnHotKey() function.
 *      default key value is the default hotkey for this command. Can be overrided by the user hotkey list file
 *  add the HkMyNewEntry pointer in the s_board_edit_Hotkey_List list ( or/and the s_module_edit_Hotkey_List list)
 *  Add the new code in the switch in OnHotKey() function.
 *  when the variable PopupOn is true, an item is currently edited.
 *  This can be usefull if the new function cannot be executed while an item is currently being edited
 *  ( For example, one cannot start a new wire when a component is moving.)
 *
 *  Note: If an hotkey is a special key, be sure the corresponding wxWidget keycode (WXK_XXXX)
 *  is handled in the hotkey_name_descr s_Hotkey_Name_List list (see hotkeys_basic.cpp)
 *  and see this list for some ascii keys (space ...)
 */

/* local variables */
/* Hotkey list: */
static Ki_HotkeyInfo    HkResetLocalCoord( wxT( "Reset local coord." ), HK_RESET_LOCAL_COORD, ' ' );
static Ki_HotkeyInfo    HkZoomAuto( wxT( "Zoom Auto" ), HK_ZOOM_AUTO, WXK_HOME );
static Ki_HotkeyInfo    HkZoomCenter( wxT( "Zoom Center" ), HK_ZOOM_CENTER, WXK_F4 );
static Ki_HotkeyInfo    HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW, WXK_F3 );
static Ki_HotkeyInfo    HkZoomOut( wxT( "Zoom Out" ), HK_ZOOM_OUT, WXK_F2 );
static Ki_HotkeyInfo    HkZoomIn( wxT( "Zoom In" ), HK_ZOOM_IN, WXK_F1 );
static Ki_HotkeyInfo    HkHelp( wxT( "Help: this message" ), HK_HELP, '?' );
static Ki_HotkeyInfo    HkSwitchUnits( wxT( "Switch Units" ), HK_SWITCH_UNITS, 'U' );
static Ki_HotkeyInfo    HkTrackDisplayMode( wxT(
                                                "Track Display Mode" ),
                                            HK_SWITCH_TRACK_DISPLAY_MODE, 'F' );

static Ki_HotkeyInfo    HkSwitch2NextCopperLayer( wxT(
                                                      "Switch to Next Layer" ),
                                                  HK_SWITCH_LAYER_TO_NEXT, '+' );
static Ki_HotkeyInfo    HkSwitch2PreviousCopperLayer( wxT(
                                                          "Switch to Previous Layer" ),
                                                      HK_SWITCH_LAYER_TO_PREVIOUS, '-' );

// List of common hotkey descriptors
Ki_HotkeyInfo* s_Gerbview_Hotkey_List[] = {
    &HkHelp,
    &HkZoomIn,                     &HkZoomOut,         &HkZoomRedraw, &HkZoomCenter,
    &HkZoomAuto,  &HkSwitchUnits,  &HkResetLocalCoord,
    &HkTrackDisplayMode,
    &HkSwitch2NextCopperLayer,
    &HkSwitch2PreviousCopperLayer,
    NULL
};


// list of sections and corresponding hotkey list for pcbnew (used to create an hotkey config file)
struct Ki_HotkeyInfoSectionDescriptor s_Gerbview_Hokeys_Descr[] =
{
    { &g_CommonSectionTag, s_Gerbview_Hotkey_List, NULL  },
    { NULL,                NULL,                   NULL  }
};


/***********************************************************/
void WinEDA_GerberFrame::OnHotKey( wxDC* DC, int hotkey,
                                   EDA_BaseStruct* DrawStruct )
/***********************************************************/

/* Hot keys. Some commands are relatives to the item under the mouse cursor
 *  Commands are case insensitive
 *  @param DC = current device context
 *  @param hotkey = hotkey code (ascii or wxWidget code for special keys)
 *  @param DrawStruct = NULL or pointer on a EDA_BaseStruct under the mouse cursor
 */

{
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    /* Convert lower to upper case (the usual toupper function has problem with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';

    Ki_HotkeyInfo * HK_Descr = GetDescriptorFromHotkey( hotkey, s_Gerbview_Hotkey_List );
    if( HK_Descr == NULL ) return;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;

    case HK_HELP:       // Display Current hotkey list
        DisplayHotkeyList( this, s_Gerbview_Hokeys_Descr );
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

    case HK_RESET_LOCAL_COORD:         /*Reset the relative coord  */
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;

    case HK_SWITCH_UNITS:
        g_UserUnit = (g_UserUnit == INCHES ) ? MILLIMETRES : INCHES;
        break;

    case HK_SWITCH_TRACK_DISPLAY_MODE:
        DisplayOpt.DisplayPcbTrackFill ^= 1; DisplayOpt.DisplayPcbTrackFill &= 1;
        DrawPanel->Refresh();
        break;

    case HK_SWITCH_LAYER_TO_PREVIOUS:
        if( ((PCB_SCREEN*)GetScreen())->m_Active_Layer > 0 )
            ((PCB_SCREEN*)GetScreen())->m_Active_Layer--;
        break;

    case HK_SWITCH_LAYER_TO_NEXT:
        if( ((PCB_SCREEN*)GetScreen())->m_Active_Layer < 31 )
            ((PCB_SCREEN*)GetScreen())->m_Active_Layer++;
        break;
    }
}
