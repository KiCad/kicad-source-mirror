/***************/
/* hotkeys.cpp */
/***************/

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
 *  add a new id in the enum hotkey_id_commnand like MY_NEW_ID_FUNCTION.
 *  add a new Ki_HotkeyInfo entry like:
 *  static Ki_HotkeyInfo HkMyNewEntry(wxT("Command Label"), MY_NEW_ID_FUNCTION, default key value);
 *      "Command Label" is the name used in hotkey list display, and the identifier in the hotkey list file
 *      MY_NEW_ID_FUNCTION is an equivalent id function used in the switch in OnHotKey() function.
 *      default key value is the default hotkey for this command. Can be overrided by the user hotkey list file
 *  add the HkMyNewEntry pointer in the s_board_edit_Hotkey_List list ( or/and the s_module_edit_Hotkey_List list)
 *  Add the new code in the switch in OnHotKey() function.
 *      Note: when the variable itemCurrentlyEdited is true, an item is currently edited.
 *      This can be useful if the new function cannot be executed while an item is currently being edited
 *      ( For example, one cannot start a new wire when a component is moving.)
 *
 *  Note: If an hotkey is a special key, be sure the corresponding wxWidget keycode (WXK_XXXX)
 *  is handled in the hotkey_name_descr s_Hotkey_Name_List list (see hotkeys_basic.cpp)
 *  and see this list for some ascii keys (space ...)
 */

/* local variables */
/* Hotkey list: */
static Ki_HotkeyInfo HkSwitch2CopperLayer( wxT( "Switch to Copper layer" ),
                                           HK_SWITCH_LAYER_TO_COPPER, WXK_PAGEDOWN );
static Ki_HotkeyInfo HkSwitch2ComponentLayer( wxT( "Switch to Component layer" ),
                                              HK_SWITCH_LAYER_TO_COMPONENT, WXK_PAGEUP );
static Ki_HotkeyInfo HkSwitch2InnerLayer1( wxT( "Switch to Inner layer 1" ),
                                           HK_SWITCH_LAYER_TO_INNER1, WXK_F5 );
static Ki_HotkeyInfo HkSwitch2InnerLayer2( wxT( "Switch to Inner layer 2" ),
                                           HK_SWITCH_LAYER_TO_INNER2, WXK_F6 );
static Ki_HotkeyInfo HkSwitch2InnerLayer3( wxT( "Switch to Inner layer 3" ),
                                           HK_SWITCH_LAYER_TO_INNER3, WXK_F7 );
static Ki_HotkeyInfo HkSwitch2InnerLayer4( wxT( "Switch to Inner layer 4" ),
                                           HK_SWITCH_LAYER_TO_INNER4, WXK_F8 );
static Ki_HotkeyInfo HkSwitch2InnerLayer5( wxT( "Switch to Inner layer 5" ),
                                           HK_SWITCH_LAYER_TO_INNER5, WXK_F9 );
static Ki_HotkeyInfo HkSwitch2InnerLayer6( wxT( "Switch to Inner layer 6" ),
                                           HK_SWITCH_LAYER_TO_INNER6, WXK_F10 );

static Ki_HotkeyInfo HkSwitch2NextCopperLayer( wxT( "Switch to Next Layer" ),
                                               HK_SWITCH_LAYER_TO_NEXT, '+' );
static Ki_HotkeyInfo HkSwitch2PreviousCopperLayer( wxT(
                                                       "Switch to Previous Layer" ),
                                                   HK_SWITCH_LAYER_TO_PREVIOUS, '-' );

static Ki_HotkeyInfo HkSavefile( wxT( "Save board" ), HK_SAVE_BOARD, 'S'
                                 + GR_KB_CTRL );
static Ki_HotkeyInfo HkLoadfile( wxT( "Load board" ), HK_LOAD_BOARD, 'L'
                                 + GR_KB_CTRL );
static Ki_HotkeyInfo HkFindItem( wxT( "Find Item" ), HK_FIND_ITEM, 'F'
                                 + GR_KB_CTRL );
static Ki_HotkeyInfo HkBackspace( wxT( "Delete track segment" ), HK_BACK_SPACE,
                                  WXK_BACK );
static Ki_HotkeyInfo HkAddNewTrack( wxT( "Add new track" ), HK_ADD_NEW_TRACK, 'X' );
static Ki_HotkeyInfo HkAddVia( wxT( "Add Via" ), HK_ADD_VIA, 'V' );
static Ki_HotkeyInfo HkSwitchTrackPosture( wxT( "Switch Track Posture" ),
                                           HK_SWITCH_TRACK_POSTURE, '/' );
static Ki_HotkeyInfo HkAddMicroVia( wxT( "Add MicroVia" ), HK_ADD_MICROVIA, 'V'
                                    + GR_KB_CTRL );
static Ki_HotkeyInfo HkEndTrack( wxT( "End Track" ), HK_END_TRACK, WXK_END );
static Ki_HotkeyInfo HkEditBoardItem( wxT( "Edit Item" ), HK_EDIT_ITEM, 'E' );
static Ki_HotkeyInfo HkFlipFootprint( wxT( "Flip Footprint" ), HK_FLIP_FOOTPRINT,
                                      'F' );
static Ki_HotkeyInfo HkRotateItem( wxT( "Rotate Item" ), HK_ROTATE_ITEM, 'R' );
static Ki_HotkeyInfo HkMoveItem( wxT( "Move Item" ), HK_MOVE_ITEM, 'M' );
static Ki_HotkeyInfo HkDragFootprint( wxT( "Drag Footprint" ), HK_DRAG_ITEM,
                                      'G' );
static Ki_HotkeyInfo HkGetAndMoveFootprint( wxT( "Get and Move Footprint" ),
                                            HK_GET_AND_MOVE_FOOTPRINT, 'T' );
static Ki_HotkeyInfo HkLock_Unlock_Footprint( wxT( "Lock/Unlock Footprint" ),
                                              HK_LOCK_UNLOCK_FOOTPRINT, 'L' );
static Ki_HotkeyInfo HkDelete( wxT( "Delete Track or Footprint" ), HK_DELETE, WXK_DELETE );
static Ki_HotkeyInfo HkResetLocalCoord( wxT( "Reset local coord." ),
                                        HK_RESET_LOCAL_COORD, ' ' );

/* Fit on Screen */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkZoomAuto( wxT( "Zoom Auto" ), HK_ZOOM_AUTO, WXK_HOME );
#else
static Ki_HotkeyInfo HkZoomAuto( wxT( "Zoom Auto" ), HK_ZOOM_AUTO, GR_KB_CTRL + '0' );
#endif

static Ki_HotkeyInfo HkZoomCenter( wxT( "Zoom Center" ), HK_ZOOM_CENTER, WXK_F4 );

/* Refresh Screen */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW, WXK_F3 );
#else
static Ki_HotkeyInfo HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW, GR_KB_CTRL + 'R' );
#endif

/* Zoom In */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkZoomIn( wxT( "Zoom In" ), HK_ZOOM_IN, WXK_F1 );
#else
static Ki_HotkeyInfo HkZoomIn( wxT( "Zoom In" ), HK_ZOOM_IN, GR_KB_CTRL + '+' );
#endif

/* Zoom Out */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkZoomOut( wxT( "Zoom Out" ), HK_ZOOM_OUT, WXK_F2 );
#else
static Ki_HotkeyInfo HkZoomOut( wxT( "Zoom Out" ), HK_ZOOM_OUT, GR_KB_CTRL + '-' );
#endif

static Ki_HotkeyInfo HkHelp( wxT( "Help: this message" ), HK_HELP, '?' );


/* Undo */
static Ki_HotkeyInfo HkUndo( wxT( "Undo" ), HK_UNDO, GR_KB_CTRL + 'Z',
                             (int) wxID_UNDO );

/* Redo */
#if !defined( __WXMAC__ )
static Ki_HotkeyInfo HkRedo( wxT( "Redo" ), HK_REDO, GR_KB_CTRL + 'Y',
                             (int) wxID_REDO );
#else
static Ki_HotkeyInfo HkRedo( wxT( "Redo" ), HK_REDO,
                             GR_KB_SHIFT + GR_KB_CTRL + 'Z',
                             (int) wxID_REDO );
#endif


static Ki_HotkeyInfo HkSwitchUnits( wxT( "Switch Units" ), HK_SWITCH_UNITS, 'U'
                                    + GR_KB_CTRL );
static Ki_HotkeyInfo HkTrackDisplayMode( wxT( "Track Display Mode" ),
                                         HK_SWITCH_TRACK_DISPLAY_MODE, 'K' );
static Ki_HotkeyInfo HkAddModule( wxT( "Add Module" ), HK_ADD_MODULE, 'O' );

// List of common hotkey descriptors
Ki_HotkeyInfo*       s_Common_Hotkey_List[] =
{
    &HkHelp,        &HkZoomIn,          &HkZoomOut,
    &HkZoomRedraw,  &HkZoomCenter,      &HkZoomAuto,
    &HkSwitchUnits, &HkResetLocalCoord,
    &HkUndo,        &HkRedo,
    NULL
};

// List of hotkey descriptors for pcbnew
Ki_HotkeyInfo* s_board_edit_Hotkey_List[] =
{
    &HkTrackDisplayMode,       &HkDelete,
    &HkBackspace,
    &HkAddNewTrack,            &HkAddVia,                    &HkAddMicroVia,
    &HkSwitchTrackPosture,
    &HkEndTrack,               &HkMoveItem,
    &HkFlipFootprint,          &HkRotateItem,                &HkDragFootprint,
    &HkGetAndMoveFootprint,    &HkLock_Unlock_Footprint,     &HkSavefile,
    &HkLoadfile,               &HkFindItem,                  &HkEditBoardItem,
    &HkSwitch2CopperLayer,     &HkSwitch2InnerLayer1,
    &HkSwitch2InnerLayer2,     &HkSwitch2InnerLayer3,        &HkSwitch2InnerLayer4,
    &HkSwitch2InnerLayer5,     &HkSwitch2InnerLayer6,        &HkSwitch2ComponentLayer,
    &HkSwitch2NextCopperLayer, &HkSwitch2PreviousCopperLayer,&HkAddModule,
    NULL
};

// List of hotkey descriptors for the module editor
Ki_HotkeyInfo* s_module_edit_Hotkey_List[] = { NULL };

// list of sections and corresponding hotkey list for pcbnew (used to create an hotkey config file)
struct Ki_HotkeyInfoSectionDescriptor s_Pcbnew_Editor_Hokeys_Descr[] =
{ {
      &g_CommonSectionTag, s_Common_Hotkey_List, L"Common keys"
  },
  {
      &g_BoardEditorSectionTag, s_board_edit_Hotkey_List, L"Board editor keys"
  },{
      &g_ModuleEditSectionTag, s_module_edit_Hotkey_List, L"Footprint editor keys"
  },{
      NULL, NULL, NULL
  } };

// list of sections and corresponding hotkey list for the board editor (used to list current hotkeys)
struct Ki_HotkeyInfoSectionDescriptor s_Board_Editor_Hokeys_Descr[] =
{ {
      &g_CommonSectionTag, s_Common_Hotkey_List,
      NULL
  },{
      &g_BoardEditorSectionTag, s_board_edit_Hotkey_List, NULL
  },{
      NULL, NULL, NULL
  } };

// list of sections and corresponding hotkey list for the footprint editor (used to list current hotkeys)
struct Ki_HotkeyInfoSectionDescriptor s_Module_Editor_Hokeys_Descr[] =
{ {
      &g_CommonSectionTag, s_Common_Hotkey_List, NULL
  },{
      &g_ModuleEditSectionTag, s_module_edit_Hotkey_List, NULL
  },{
      NULL, NULL, NULL
  } };

/** Function OnHotKey.
 *  ** Commands are case insensitive **
 *  Some commands are relatives to the item under the mouse cursor
 *  @param aDC = current device context
 *  @param hotkey = hotkey code (ascii or wxWidget code for special keys)
 *  @param aItem = NULL or pointer on a EDA_BaseStruct under the mouse cursor
 */
void WinEDA_PcbFrame::OnHotKey( wxDC* aDC, int aHotkeyCode, EDA_BaseStruct* aItem )
{
    if( aHotkeyCode == 0 )
        return;

    bool    itemCurrentlyEdited = (GetCurItem() && GetCurItem()->m_Flags);

    MODULE* module = NULL;
    int evt_type = 0;       //Used to post a wxCommandEvent on demand

    /* Convert lower to upper case
     * (the usual toupper function has problem with non ascii codes like function keys
     */
    if( (aHotkeyCode >= 'a') && (aHotkeyCode <= 'z') )
        aHotkeyCode += 'A' - 'a';

    Ki_HotkeyInfo* HK_Descr = GetDescriptorFromHotkey( aHotkeyCode, s_Common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( aHotkeyCode, s_board_edit_Hotkey_List );

    if( HK_Descr == NULL )
        return;

    // Create a wxCommandEvent that will be posted in some hot keys functions
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    int            ll;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;
        break;

    case HK_SWITCH_LAYER_TO_PREVIOUS:
        ll = getActiveLayer();
        if( (ll <= LAYER_N_BACK) || (ll > LAYER_N_FRONT) )
            break;

        if( GetBoard()->GetCopperLayerCount() < 2 ) // Single layer
            ll = LAYER_N_BACK;
        else if( ll == LAYER_N_FRONT )
            ll = MAX( LAYER_N_BACK,
                      GetBoard()->GetCopperLayerCount() - 2 );
        else
            ll--;
        SwitchLayer( aDC, ll );
        break;

    case HK_SWITCH_LAYER_TO_NEXT:
        ll = getActiveLayer();
        if( (ll < LAYER_N_BACK) || (ll >= LAYER_N_FRONT) )
            break;
        if( GetBoard()->GetCopperLayerCount() < 2 ) // Single layer
            ll = LAYER_N_BACK;
        else if( ll >= GetBoard()->GetCopperLayerCount() - 2 )
            ll = LAYER_N_FRONT;
        else
            ll++;
        SwitchLayer( aDC, ll );
        break;

    case HK_SWITCH_LAYER_TO_COMPONENT:
        SwitchLayer( aDC, LAYER_N_FRONT );
        break;

    case HK_SWITCH_LAYER_TO_COPPER:
        SwitchLayer( aDC, LAYER_N_BACK );
        break;

    case HK_SWITCH_LAYER_TO_INNER1:
        SwitchLayer( aDC, LAYER_N_2 );
        break;

    case HK_SWITCH_LAYER_TO_INNER2:
        SwitchLayer( aDC, LAYER_N_3 );
        break;

    case HK_SWITCH_LAYER_TO_INNER3:
        SwitchLayer( aDC, LAYER_N_4 );
        break;

    case HK_SWITCH_LAYER_TO_INNER4:
        SwitchLayer( aDC, LAYER_N_5 );
        break;

    case HK_SWITCH_LAYER_TO_INNER5:
        SwitchLayer( aDC, LAYER_N_6 );
        break;

    case HK_SWITCH_LAYER_TO_INNER6:
        SwitchLayer( aDC, LAYER_N_7 );
        break;

    case HK_HELP: // Display Current hotkey list
        DisplayHotkeyList( this, s_Board_Editor_Hokeys_Descr );
        break;

    case HK_ZOOM_IN:
        evt_type = ID_POPUP_ZOOM_IN;
        break;

    case HK_ZOOM_OUT:
        evt_type = ID_POPUP_ZOOM_OUT;
        break;

    case HK_ZOOM_REDRAW:
        evt_type = ID_ZOOM_REDRAW;
        break;

    case HK_ZOOM_AUTO:
        evt_type = ID_ZOOM_PAGE;
        break;

    case HK_ZOOM_CENTER:
        evt_type = ID_POPUP_ZOOM_CENTER;
        break;

    case HK_ADD_MODULE:
        evt_type = ID_COMPONENT_BUTT;
        break;

    case HK_UNDO:
    case HK_REDO:
        if( !itemCurrentlyEdited )
        {
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED,
                                  HK_Descr->m_IdMenuEvent );
            wxPostEvent( this, event );
        }
        break;

    case HK_RESET_LOCAL_COORD: /*Reset the relative coord  */
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;

    case HK_SWITCH_UNITS:
        g_UserUnit = (g_UserUnit == INCHES) ? MILLIMETRES : INCHES;
        break;

    case HK_SWITCH_TRACK_DISPLAY_MODE:
        DisplayOpt.DisplayPcbTrackFill ^= 1;
        DisplayOpt.DisplayPcbTrackFill &= 1;
        m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
        DrawPanel->Refresh();
        break;

    case HK_DELETE:
        OnHotkeyDeleteItem( aDC );
        break;

    case HK_BACK_SPACE:
        if( m_ID_current_state == ID_TRACK_BUTT && (getActiveLayer() <= LAYER_N_FRONT) )
        {
            if( !itemCurrentlyEdited )
            {
                // no track is currently being edited - select a segment and remove it.
                // @todo: possibly? pass the HK command code to PcbGeneralLocateAndDisplay()
                // so it can restrict its search to specific item types.
                aItem = PcbGeneralLocateAndDisplay();

                // don't let backspace delete modules!!
                if( aItem && (aItem->Type() == TYPE_TRACK
                              || aItem->Type() == TYPE_VIA) )
                {
                    Delete_Segment( aDC, (TRACK*) aItem );
                    SetCurItem( NULL );
                }
                OnModify();
            }
            else if( GetCurItem()->Type() == TYPE_TRACK )
            {
                // then an element is being edited - remove the last segment.
                // simple lines for debugger:
                TRACK* track = (TRACK*) GetCurItem();
                track = Delete_Segment( aDC, track );
                SetCurItem( track );
                OnModify();
            }
        }
        break;

    case HK_END_TRACK:
        if( itemCurrentlyEdited && (GetCurItem()->Type() == TYPE_TRACK)
           && ( (GetCurItem()->m_Flags & IS_NEW) != 0 ) )
        {
            // A new track is in progress: call to End_Route()
            DrawPanel->MouseToCursorSchema();
            End_Route( (TRACK*) GetCurItem(), aDC );
        }
        break;

    case HK_GET_AND_MOVE_FOOTPRINT:
        if( !itemCurrentlyEdited )
            evt_type = ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST;
        break;

    case HK_FIND_ITEM:
        if( !itemCurrentlyEdited )
            evt_type = ID_FIND_ITEMS;
        break;

    case HK_LOAD_BOARD:
        if( !itemCurrentlyEdited )
            evt_type = ID_LOAD_FILE ;
        break;

    case HK_SAVE_BOARD:
        if( !itemCurrentlyEdited )
            evt_type = ID_SAVE_BOARD;
        break;

    case HK_ADD_MICROVIA: // Place a micro via if a track is in progress
        if( m_ID_current_state != ID_TRACK_BUTT )
            return;
        if( !itemCurrentlyEdited )                              // no track in progress: nothing to do
            break;
        if( GetCurItem()->Type() != TYPE_TRACK )                // Should not occur
            return;
        if( (GetCurItem()->m_Flags & IS_NEW) == 0 )
            return;

        // place micro via and switch layer
        if( IsMicroViaAcceptable() )
            evt_type = ID_POPUP_PCB_PLACE_MICROVIA;
        break;

    case HK_ADD_VIA: // Switch to alternate layer and Place a via if a track is in progress
        if( m_ID_current_state != ID_TRACK_BUTT )
            return;
        if( !itemCurrentlyEdited ) // no track in progress: switch layer only
        {
            Other_Layer_Route( NULL, aDC );
            break;
        }
        if( GetCurItem()->Type() != TYPE_TRACK )
            return;
        if( (GetCurItem()->m_Flags & IS_NEW) == 0 )
            return;
        evt_type = ID_POPUP_PCB_PLACE_VIA;
        break;

    case HK_SWITCH_TRACK_POSTURE:
        /* change the position of initial segment when creating new tracks
         * switch from _/  to -\ .
         */
        evt_type = ID_POPUP_PCB_SWITCH_TRACK_POSTURE ;
        break;

    case HK_ADD_NEW_TRACK: // Start new track
        if( getActiveLayer() > LAYER_N_FRONT )
            break;

        if( m_ID_current_state != ID_TRACK_BUTT && !itemCurrentlyEdited )
        {
            cmd.SetId( ID_TRACK_BUTT );
            GetEventHandler()->ProcessEvent( cmd );
        }

        if( m_ID_current_state != ID_TRACK_BUTT )
            break;

        if( !itemCurrentlyEdited )     // no track in progress:
        {
            TRACK* track = Begin_Route( NULL, aDC );
            SetCurItem( track );
            if( track )
                DrawPanel->m_AutoPAN_Request = true;
        }
        else if( GetCurItem()->m_Flags & IS_NEW )
        {
            TRACK* track = Begin_Route( (TRACK*) GetCurItem(), aDC );

            // SetCurItem() must not write to the msg panel
            // because a track info is displayed while moving the mouse cursor
            if( track )      // A new segment was created
                SetCurItem( track, false );
            DrawPanel->m_AutoPAN_Request = true;
        }
        break;

    case HK_EDIT_ITEM:      // Edit board item
        if( !itemCurrentlyEdited )
        {
            BOARD_ITEM* item = PcbGeneralLocateAndDisplay();
            if( item == NULL )
                break;

            //An item is found, and some can be edited:
            OnEditItemRequest( aDC, item );
        }
        break;

    // Footprint edition:
    case HK_LOCK_UNLOCK_FOOTPRINT: // toggle module "MODULE_is_LOCKED" status:
        // get any module, locked or not locked and toggle its locked status
        if( !itemCurrentlyEdited )
            module = Locate_Prefered_Module( GetBoard(), CURSEUR_OFF_GRILLE
                                             | VISIBLE_ONLY );
        else if( GetCurItem()->Type() == TYPE_MODULE )
            module = (MODULE*) GetCurItem();
        if( module )
        {
            SetCurItem( module );
            module->SetLocked( !module->IsLocked() );
            module->DisplayInfo( this );
        }
        break;

    case HK_DRAG_ITEM:    // Start drag module or track segment
        OnHotkeyMoveItem( HK_DRAG_ITEM );
        break;

    case HK_MOVE_ITEM:                  // Start move item
        OnHotkeyMoveItem( HK_MOVE_ITEM );
        break;

    case HK_ROTATE_ITEM:        // Rotation
        OnHotkeyRotateItem( HK_ROTATE_ITEM );
        break;

    case HK_FLIP_FOOTPRINT:     // move to other side
        OnHotkeyRotateItem( HK_FLIP_FOOTPRINT );
        break;
    }

    if( evt_type != 0 )
    {
        wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );
        evt.SetEventObject( this );
        evt.SetId( evt_type );
        wxPostEvent( this, evt );
    }
}


/***********************************************************/
void WinEDA_ModuleEditFrame::OnHotKey( wxDC* aDC, int hotkey,
                                       EDA_BaseStruct* DrawStruct )
/***********************************************************/

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

    Ki_HotkeyInfo* HK_Descr = GetDescriptorFromHotkey( hotkey, s_Common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( hotkey, s_module_edit_Hotkey_List );

    if( HK_Descr == NULL )
        return;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;
        break;

    case HK_HELP: // Display Current hotkey list
        DisplayHotkeyList( this, s_Module_Editor_Hokeys_Descr );
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
    }
}


/** Function OnHotkeyDeleteItem
 * Delete the item found under the mouse cursor
 *  Depending on the current active tool::
 *      Tool track
 *          if a track is in progress: Delete the last segment
 *			else delete the entire track
 *      Tool module (footprint):
 *          Delete the module.
 * @param aDC = current device context
 * @return true if an item was deleted
 */
bool WinEDA_PcbFrame::OnHotkeyDeleteItem( wxDC* aDC )
{
    BOARD_ITEM* item = GetCurItem();
    bool ItemFree = (item == NULL) || (item->m_Flags == 0);

    switch( m_ID_current_state )
    {
    case ID_TRACK_BUTT:
        if( getActiveLayer() > LAYER_N_FRONT )
            return false;
        if( ItemFree )
        {
            item = PcbGeneralLocateAndDisplay();
            if( item && item->Type() != TYPE_TRACK )
                return false;
            Delete_Track( aDC, (TRACK*) item );
        }
        else if( item->Type() == TYPE_TRACK )
        {
            // simple lines for debugger:
            TRACK* track = (TRACK*) item;
            track = Delete_Segment( aDC, track );
            SetCurItem( track );
            OnModify();
            return true;
        }
        break;

    case ID_COMPONENT_BUTT:
        if( ItemFree )
        {
            MODULE* module = Locate_Prefered_Module( GetBoard(),
                                                     CURSEUR_ON_GRILLE );
            if( module == NULL )
                return false;
            if( !IsOK( this, _( "Delete module?" ) ) )
                return false;
            RemoveStruct( module, aDC );
        }
        else
            return false;
        break;

    default:
        return false;
    }

    OnModify();
    SetCurItem( NULL );
    return true;
}


/** Function OnHotkeyMoveItem
 * Move or drag the item (footprint, track, text .. ) found under the mouse cursor
 * An item can be moved (or dragged) only if there is no item currently edited
 * Only a footprint, a pad or a track can be dragged
 * @param aIdCommand = the hotkey command id
 * @return true if an item was moved
 */
bool WinEDA_PcbFrame::OnHotkeyMoveItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool itemCurrentlyEdited = item && item->m_Flags;

    if( itemCurrentlyEdited )
        return false;

    item = PcbGeneralLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    int evt_type = 0;       //Used to post a wxCommandEvent on demand

    switch( item->Type() )
    {
    case TYPE_TRACK:
    case TYPE_VIA:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_TRACK_NODE;
        if( aIdCommand == HK_DRAG_ITEM )
            evt_type = ID_POPUP_PCB_DRAG_TRACK_SEGMENT;
        break;

    case TYPE_MODULE:
    {
        MODULE* module = (MODULE*) item;

        // a footprint is found, but locked or on an other layer
        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "Footprint %s found, but locked" ),
                       module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_MODULE_REQUEST;
        if( aIdCommand == HK_DRAG_ITEM )
            evt_type = ID_POPUP_PCB_DRAG_MODULE_REQUEST;
    }
    break;

    case TYPE_PAD:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_PAD_REQUEST;
        if( aIdCommand == HK_DRAG_ITEM )
            evt_type = ID_POPUP_PCB_DRAG_PAD_REQUEST;
        break;

    case TYPE_TEXTE:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_TEXTEPCB_REQUEST;
        break;

    case TYPE_MIRE:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_MIRE_REQUEST;
        break;

    case TYPE_ZONE_CONTAINER:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_ZONE_OUTLINES;
        if( aIdCommand == HK_DRAG_ITEM )
            evt_type = ID_POPUP_PCB_DRAG_ZONE_OUTLINE_SEGMENT;
        break;

    case TYPE_TEXTE_MODULE:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST;
        break;

    case TYPE_DRAWSEGMENT:
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_DRAWING_REQUEST;
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


/** Function OnHotkeyRotateItem
 * Rotate the item (text or footprint) found under the mouse cursor
 * Note:
 *     this command can be used with an item currently in edit
 *     Only some items can be rotated (footprints and texts)
 * @param aIdCommand = the hotkey command id
 * @return true if an item was moved
 */
bool WinEDA_PcbFrame::OnHotkeyRotateItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool        itemCurrentlyEdited = item && item->m_Flags;
    int         evt_type = 0; // Used to post a wxCommandEvent on demand

    if( !itemCurrentlyEdited )
        item = PcbGeneralLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    switch( item->Type() )
    {
    case TYPE_MODULE:
    {
         MODULE* module = (MODULE*) item;
        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "Footprint %s is locked" ),
                       module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }
        if( aIdCommand == HK_ROTATE_ITEM )                      // Rotation
            evt_type = ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE;
        if( aIdCommand == HK_FLIP_FOOTPRINT )                   // move to other side
            evt_type = ID_POPUP_PCB_CHANGE_SIDE_MODULE;
    }
        break;

    case TYPE_TEXTE:
        if( aIdCommand == HK_ROTATE_ITEM )                      // Rotation
            evt_type = ID_POPUP_PCB_ROTATE_TEXTEPCB;
        break;

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
