/***************/
/* hotkeys.cpp */
/***************/

#include "fctsys.h"

#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "id.h"
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
 *  when the variable PopupOn is true, an item is currently edited.
 *  This can be useful if the new function cannot be executed while an item is currently being edited
 *  ( For example, one cannot start a new wire when a component is moving.)
 *
 *  Note: If an hotkey is a special key, be sure the corresponding wxWidget keycode (WXK_XXXX)
 *  is handled in the hotkey_name_descr s_Hotkey_Name_List list (see hotkeys_basic.cpp)
 *  and see this list for some ascii keys (space ...)
 */

/* local variables */
/* Hotkey list: */
static Ki_HotkeyInfo HkSwitch2CopperLayer( wxT(
                                               "Switch to Copper layer" ),
                                           HK_SWITCH_LAYER_TO_COPPER, WXK_PAGEUP );
static Ki_HotkeyInfo HkSwitch2ComponentLayer( wxT(
                                                  "Switch to Component layer" ),
                                              HK_SWITCH_LAYER_TO_COMPONENT, WXK_PAGEDOWN );
static Ki_HotkeyInfo HkSwitch2InnerLayer1( wxT(
                                               "Switch to Inner layer 1" ),
                                           HK_SWITCH_LAYER_TO_INNER1, WXK_F5 );
static Ki_HotkeyInfo HkSwitch2InnerLayer2( wxT(
                                               "Switch to Inner layer 2" ),
                                           HK_SWITCH_LAYER_TO_INNER2, WXK_F6 );
static Ki_HotkeyInfo HkSwitch2InnerLayer3( wxT(
                                               "Switch to Inner layer 3" ),
                                           HK_SWITCH_LAYER_TO_INNER3, WXK_F7 );
static Ki_HotkeyInfo HkSwitch2InnerLayer4( wxT(
                                               "Switch to Inner layer 4" ),
                                           HK_SWITCH_LAYER_TO_INNER4, WXK_F8 );
static Ki_HotkeyInfo HkSwitch2InnerLayer5( wxT(
                                               "Switch to Inner layer 5" ),
                                           HK_SWITCH_LAYER_TO_INNER5, WXK_F9 );
static Ki_HotkeyInfo HkSwitch2InnerLayer6( wxT(
                                               "Switch to Inner layer 6" ),
                                           HK_SWITCH_LAYER_TO_INNER6, WXK_F10 );

static Ki_HotkeyInfo HkSwitch2NextCopperLayer( wxT(
                                                   "Switch to Next Layer" ),
                                               HK_SWITCH_LAYER_TO_NEXT, '+' );
static Ki_HotkeyInfo HkSwitch2PreviousCopperLayer( wxT(
                                                       "Switch to Previous Layer" ),
                                                   HK_SWITCH_LAYER_TO_PREVIOUS, '-' );

static Ki_HotkeyInfo HkSavefile( wxT( "Save board" ), HK_SAVE_BOARD, 'S' + GR_KB_CTRL );
static Ki_HotkeyInfo HkLoadfile( wxT( "Load board" ), HK_LOAD_BOARD, 'L' + GR_KB_CTRL );
static Ki_HotkeyInfo HkFindItem( wxT( "Find Item" ), HK_FIND_ITEM, 'F' + GR_KB_CTRL );
static Ki_HotkeyInfo HkBackspace( wxT( "Delete track segment" ), HK_BACK_SPACE, WXK_BACK );
static Ki_HotkeyInfo HkAddVia( wxT( "Add Via" ), HK_ADD_VIA, 'V' );
static Ki_HotkeyInfo HkEndTrack( wxT( "End Track" ), HK_END_TRACK, WXK_END );
static Ki_HotkeyInfo HkFlipFootprint( wxT( "Flip Footprint" ), HK_FLIP_FOOTPRINT, 'F' );
static Ki_HotkeyInfo HkRotateFootprint( wxT( "Rotate Footprint" ), HK_ROTATE_FOOTPRINT, 'R' );
static Ki_HotkeyInfo HkMoveFootprint( wxT( "Move Footprint" ), HK_MOVE_FOOTPRINT, 'M' );
static Ki_HotkeyInfo HkDragFootprint( wxT( "Drag Footprint" ), HK_DRAG_FOOTPRINT, 'G' );
static Ki_HotkeyInfo HkGetAndMoveFootprint( wxT(
                                                "Get and Move Footprint" ),
                                            HK_GET_AND_MOVE_FOOTPRINT, 'T' );
static Ki_HotkeyInfo HkLock_Unlock_Footprint( wxT(
                                                  "Lock/Unlock Footprint" ),
                                              HK_LOCK_UNLOCK_FOOTPRINT, 'L' );
static Ki_HotkeyInfo HkDelete( wxT( "Delete Track or Footprint" ), HK_DELETE, WXK_DELETE );
static Ki_HotkeyInfo HkResetLocalCoord( wxT( "Reset local coord." ), HK_RESET_LOCAL_COORD, ' ' );
static Ki_HotkeyInfo HkZoomCenter( wxT( "Zoom Center" ), HK_ZOOM_CENTER, WXK_F4 );
static Ki_HotkeyInfo HkZoomRedraw( wxT( "Zoom Redraw" ), HK_ZOOM_REDRAW, WXK_F3 );
static Ki_HotkeyInfo HkZoomOut( wxT( "Zoom Out" ), HK_ZOOM_OUT, WXK_F2 );
static Ki_HotkeyInfo HkZoomIn( wxT( "Zoom In" ), HK_ZOOM_IN, WXK_F1 );
static Ki_HotkeyInfo HkHelp( wxT( "Help: this message" ), HK_HELP, '?' );
static Ki_HotkeyInfo HkSwitchUnits( wxT( "Switch Units" ), HK_SWITCH_UNITS, 'U' + GR_KB_CTRL );
static Ki_HotkeyInfo HkTrackDisplayMode( wxT(
                                             "Track Display Mode" ),
                                         HK_SWITCH_TRACK_DISPLAY_MODE, 'K' );

// List of common hotkey descriptors
Ki_HotkeyInfo* s_Common_Hotkey_List[] = {
    &HkHelp,
    &HkZoomIn,      &HkZoomOut,         &HkZoomRedraw, &HkZoomCenter,
    &HkSwitchUnits, &HkResetLocalCoord,
    NULL
};

// List of hotkey descriptors for pcbnew
Ki_HotkeyInfo* s_board_edit_Hotkey_List[] = {
    &HkTrackDisplayMode,
    &HkDelete,                     &HkBackspace,
    &HkAddVia,                     &HkEndTrack,
    &HkMoveFootprint,              &HkFlipFootprint,
    &HkRotateFootprint,            &HkDragFootprint,
    &HkGetAndMoveFootprint,
    &HkLock_Unlock_Footprint,
    &HkSavefile,                   &HkLoadfile,      &HkFindItem,
    &HkSwitch2CopperLayer,
    &HkSwitch2InnerLayer1,
    &HkSwitch2InnerLayer2,
    &HkSwitch2InnerLayer3,
    &HkSwitch2InnerLayer4,
    &HkSwitch2InnerLayer5,
    &HkSwitch2InnerLayer6,
    &HkSwitch2ComponentLayer,
    &HkSwitch2NextCopperLayer,
    &HkSwitch2PreviousCopperLayer,
    NULL
};

// List of hotkey descriptors for the module editor
Ki_HotkeyInfo* s_module_edit_Hotkey_List[] = {
    NULL
};


// list of sections and corresponding hotkey list for pcbnew (used to create an hotkey config file)
struct Ki_HotkeyInfoSectionDescriptor s_Pcbnew_Editor_Hokeys_Descr[] =
{
    { &g_CommonSectionTag, s_Common_Hotkey_List, "Common keys"           },
    { &g_BoardEditorSectionTag, s_board_edit_Hotkey_List, "Board editor keys"     },
    { &g_ModuleEditSectionTag, s_module_edit_Hotkey_List, "Footprint editor keys" },
    { NULL, NULL, NULL }
};

// list of sections and corresponding hotkey list for the board editor (used to list current hotkeys)
struct Ki_HotkeyInfoSectionDescriptor s_Board_Editor_Hokeys_Descr[] =
{
    { &g_CommonSectionTag, s_Common_Hotkey_List, NULL },
    { &g_BoardEditorSectionTag, s_board_edit_Hotkey_List, NULL },
    { NULL, NULL, NULL }
};

// list of sections and corresponding hotkey list for the footprint editor (used to list current hotkeys)
struct Ki_HotkeyInfoSectionDescriptor s_Module_Editor_Hokeys_Descr[] =
{
    { &g_CommonSectionTag, s_Common_Hotkey_List, NULL },
    { &g_ModuleEditSectionTag, s_module_edit_Hotkey_List, NULL },
    { NULL, NULL, NULL }
};


/***********************************************************/
void WinEDA_PcbFrame::OnHotKey( wxDC* DC, int hotkey,
                                EDA_BaseStruct* DrawStruct )
/***********************************************************/

/* Hot keys. Some commands are relatives to the item under the mouse cursor
 *  Commands are case insensitive
 *  @param DC = current device context
 *  @param hotkey = hotkey code (ascii or wxWidget code for special keys)
 *  @param DrawStruct = NULL or pointer on a EDA_BaseStruct under the mouse cursor
 */

{
    bool PopupOn = (GetCurItem() && GetCurItem()->m_Flags);

    bool ItemFree = (GetCurItem()==0  || GetCurItem()->m_Flags==0);

    if( hotkey == 0 )
        return;

    MODULE* module = NULL;

    // Remap the control key Ctrl A (0x01) to GR_KB_CTRL + 'A' (just easier to handle...)
    if( (hotkey & GR_KB_CTRL) != 0 )
        hotkey += 'A' - 1;
    
    /* Convert lower to upper case (the usual toupper function has problem with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';

    Ki_HotkeyInfo * HK_Descr = GetDescriptorFromHotkey( hotkey, s_Common_Hotkey_List );
    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( hotkey, s_board_edit_Hotkey_List );
    if( HK_Descr == NULL ) return;

    int ll;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;
        break;

    case HK_SWITCH_LAYER_TO_PREVIOUS:
        ll = GetScreen()->m_Active_Layer;
        if( (ll <= COPPER_LAYER_N) || (ll > CMP_N) )
            break;
        if( m_Pcb->m_BoardSettings->m_CopperLayerCount < 2 )  // Single layer
            ll = COPPER_LAYER_N;
        else if( ll == CMP_N )
            ll = MAX( COPPER_LAYER_N, m_Pcb->m_BoardSettings->m_CopperLayerCount - 2 );
        else
            ll--;
        SwitchLayer( DC, ll );
        break;

    case HK_SWITCH_LAYER_TO_NEXT:
        ll = GetScreen()->m_Active_Layer;
        if( (ll < COPPER_LAYER_N) || (ll >= CMP_N) )
            break;
        if( m_Pcb->m_BoardSettings->m_CopperLayerCount < 2 )  // Single layer
            ll = COPPER_LAYER_N;
        else if( ll >= m_Pcb->m_BoardSettings->m_CopperLayerCount - 2 )
            ll = CMP_N;
        else
            ll++;
        SwitchLayer( DC, ll );
        break;

    case HK_SWITCH_LAYER_TO_COMPONENT:
        SwitchLayer( DC, CMP_N );
        break;

    case HK_SWITCH_LAYER_TO_COPPER:
        SwitchLayer( DC, COPPER_LAYER_N );
        break;

    case HK_SWITCH_LAYER_TO_INNER1:
        SwitchLayer( DC, LAYER_N_2 );
        break;

    case HK_SWITCH_LAYER_TO_INNER2:
        SwitchLayer( DC, LAYER_N_3 );
        break;

    case HK_SWITCH_LAYER_TO_INNER3:
        SwitchLayer( DC, LAYER_N_4 );
        break;

    case HK_SWITCH_LAYER_TO_INNER4:
        SwitchLayer( DC, LAYER_N_5 );
        break;

    case HK_SWITCH_LAYER_TO_INNER5:
        SwitchLayer( DC, LAYER_N_6 );
        break;

    case HK_SWITCH_LAYER_TO_INNER6:
        SwitchLayer( DC, LAYER_N_7 );
        break;

    case HK_HELP:       // Display Current hotkey list
        DisplayHotkeyList( this, s_Board_Editor_Hokeys_Descr );
        break;

    case HK_ZOOM_IN:
        OnZoom( ID_ZOOM_PLUS_KEY );
        break;

    case HK_ZOOM_OUT:
        OnZoom( ID_ZOOM_MOINS_KEY );
        break;

    case HK_ZOOM_REDRAW:
        OnZoom( ID_ZOOM_REDRAW_KEY );
        break;

    case HK_ZOOM_CENTER:
        OnZoom( ID_ZOOM_CENTER_KEY );
        break;


    case HK_RESET_LOCAL_COORD:         /*Reset the relative coord  */
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;


    case HK_SWITCH_UNITS:
        g_UnitMetric = (g_UnitMetric == INCHES ) ? MILLIMETRE : INCHES;
        break;

    case HK_SWITCH_TRACK_DISPLAY_MODE:
        DisplayOpt.DisplayPcbTrackFill ^= 1; DisplayOpt.DisplayPcbTrackFill &= 1;
		m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
        GetScreen()->SetRefreshReq();
        break;

    case HK_DELETE:
        OnHotkeyDeleteItem( DC, DrawStruct );
        break;

    case HK_BACK_SPACE:
        if( m_ID_current_state == ID_TRACK_BUTT && GetScreen()->m_Active_Layer <= CMP_N )
        {
            if( ItemFree )
            {
                // no track is currently being edited - select a segment and remove it.

                // @todo: possibly? pass the HK command code to PcbGeneralLocateAndDisplay() so it can restrict its search to specific item types.

                // @todo: use PcbGeneralLocateAndDisplay() everywhere in this source file.

                DrawStruct = PcbGeneralLocateAndDisplay();

                // don't let backspace delete modules!!
                if( DrawStruct && (DrawStruct->Type() == TYPETRACK
                                   || DrawStruct->Type() == TYPEVIA) )
                {
                    Delete_Segment( DC, (TRACK*) DrawStruct );
                    SetCurItem(NULL);
                }
                GetScreen()->SetModify();
            }
            else if( GetCurItem()->Type() == TYPETRACK  )
            {
                // then an element is being edited - remove the last segment.
                // simple lines for debugger:
                TRACK* track = (TRACK*) GetCurItem() ;
                track = Delete_Segment( DC, track );
                SetCurItem( track );
                GetScreen()->SetModify();
            }
        }
        break;

    case HK_END_TRACK:
		if( ! ItemFree && (GetCurItem()->Type() == TYPETRACK) && ((GetCurItem()->m_Flags & IS_NEW) != 0) )
		{	
            // A new track is in progress: call to End_Route()
			DrawPanel->MouseToCursorSchema();
			End_Route( (TRACK*) GetCurItem(), DC );
		}
        break;

    case HK_GET_AND_MOVE_FOOTPRINT:
        if( ItemFree )
        {
            wxCommandEvent evt;
            evt.SetId( ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST );
            Process_Special_Functions( evt );
        }
        break;

    case HK_FIND_ITEM:
        if( ItemFree )
        {
            wxCommandEvent evt;
            evt.SetId( ID_FIND_ITEMS );
            Process_Special_Functions( evt );
        }
        break;

    case HK_LOAD_BOARD:
        if( ItemFree )
        {
            // try not to duplicate save, load code etc.
            wxCommandEvent evt;
            evt.SetId( ID_LOAD_FILE );
            Files_io( evt );
        }
        break;

    case HK_SAVE_BOARD:
        if( ItemFree )
        {
            // try not to duplicate save, load code etc.
            wxCommandEvent evt;
            evt.SetId( ID_SAVE_BOARD );
            Files_io( evt );
        }
        break;

    case HK_ADD_VIA:          // Switch to alternate layer and Place a via if a track is in progress
        if( m_ID_current_state != ID_TRACK_BUTT )
            return;
        if( ItemFree )		// no track in progress: switch layer only
        {
            Other_Layer_Route( NULL, DC );
            break;
        }
        if( GetCurItem()->Type() != TYPETRACK )
            return;
        if( (GetCurItem()->m_Flags & IS_NEW) == 0 )
            return;
        Other_Layer_Route( (TRACK*) GetCurItem(), DC );		// place via and switch layer
        if( DisplayOpt.ContrastModeDisplay )
            GetScreen()->SetRefreshReq();
        break;

        // Footprint edition:
    case HK_LOCK_UNLOCK_FOOTPRINT:          // toggle module "MODULE_is_LOCKED" status:
        // get any module, locked or not locked and toggle its locked status
        if( ItemFree )
            module = Locate_Prefered_Module( m_Pcb, CURSEUR_OFF_GRILLE | VISIBLE_ONLY );
        else if( GetCurItem()->Type() == TYPEMODULE )
            module = (MODULE*) GetCurItem();
        if( module )
        {
            SetCurItem( module );
            module->SetLocked( !module->IsLocked() );
            module->Display_Infos( this );
        }
        break;

    case HK_DRAG_FOOTPRINT:             // Start move (and drag) module
    case HK_MOVE_FOOTPRINT:             // Start move module
        if( PopupOn )
            break;

    case HK_ROTATE_FOOTPRINT:           // Rotation
    case HK_FLIP_FOOTPRINT:             // move to other side
        if( ItemFree )
        {
            module = Locate_Prefered_Module( m_Pcb,
                                             CURSEUR_OFF_GRILLE | IGNORE_LOCKED | VISIBLE_ONLY
    #if defined (USE_MATCH_LAYER)
                                             | MATCH_LAYER
    #endif
                                             );
            
            if( module == NULL )          // no footprint found
            {
                module = Locate_Prefered_Module( m_Pcb, CURSEUR_OFF_GRILLE | VISIBLE_ONLY );
                if( module )
                {
                    // a footprint is found, but locked or on an other layer
                    if( module->IsLocked() )
                    {
                        wxString msg;

                        msg.Printf( _( "Footprint %s found, but locked" ),
                                   module->m_Reference->m_Text.GetData() );

                        DisplayInfo( this, msg );
                    }
                    module = NULL;
                }
            }
        }
        else if( GetCurItem()->Type() == TYPEMODULE )
        {
            module = (MODULE*) GetCurItem();

            // @todo: might need to add a layer check in if() below
            if( (GetCurItem()->m_Flags == 0)
               && module->IsLocked() )
                module = NULL; // do not move, rotate ... it.
        }
        if( module == NULL )
            break;

        /*  I'd like to make sending to EESCHEMA edge triggered, but the
         *  simple mouse click on a module when the arrow icon is in play
         *  does not set GetCurItem() at this time, nor does a mouse click
         *  when the local ratsnest icon is in play set GetCurItem(), and these
         *  actions also call SendMessageToEESCHEMA().
         *  if( GetCurItem() != module )
         */
        {
            // Send the module via socket to EESCHEMA's search facility.
            SendMessageToEESCHEMA( module );

            SetCurItem( module );
        }

        switch( HK_Descr->m_Idcommand )
        {
        case HK_ROTATE_FOOTPRINT:                  // Rotation
            Rotate_Module( DC, module, 900, TRUE );
            break;

        case HK_FLIP_FOOTPRINT:                  // move to other side
            Change_Side_Module( module, DC );
            break;

        case HK_DRAG_FOOTPRINT:                  // Start move (and drag) module
            g_Drag_Pistes_On = TRUE;

            // fall through
        case HK_MOVE_FOOTPRINT:                  // Start move module
            StartMove_Module( module, DC );
            break;
        }

        module->Display_Infos( this );
        break;
    }
}


/***********************************************************/
void WinEDA_ModuleEditFrame::OnHotKey( wxDC* DC, int hotkey,
                                       EDA_BaseStruct* DrawStruct )
/***********************************************************/

/* Hot keys. Some commands are relative to the item under the mouse cursor
 *  Commands are case insensitive
 */
{
    if( hotkey == 0 )
        return;

    /* Convert lower to upper case (the usual toupper function has problem with non ascii codes like function keys */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';

    Ki_HotkeyInfo * HK_Descr = GetDescriptorFromHotkey( hotkey, s_Common_Hotkey_List );
    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( hotkey, s_module_edit_Hotkey_List );
    if( HK_Descr == NULL ) return;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;
        break;

    case HK_HELP:       // Display Current hotkey list
        DisplayHotkeyList( this, s_Module_Editor_Hokeys_Descr );
        break;

    case HK_RESET_LOCAL_COORD:         /*Reset the relative coord  */
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;


    case HK_SWITCH_UNITS:
        g_UnitMetric = (g_UnitMetric == INCHES ) ? MILLIMETRE : INCHES;
        break;

    case HK_ZOOM_IN:
        OnZoom( ID_ZOOM_PLUS_KEY );
        break;

    case HK_ZOOM_OUT:
        OnZoom( ID_ZOOM_MOINS_KEY );
        break;

    case HK_ZOOM_REDRAW:
        OnZoom( ID_ZOOM_REDRAW_KEY );
        break;

    case HK_ZOOM_CENTER:
        OnZoom( ID_ZOOM_CENTER_KEY );
        break;
    }
}


/******************************************************************************/
bool WinEDA_PcbFrame::OnHotkeyDeleteItem( wxDC* DC, EDA_BaseStruct* DrawStruct )
/******************************************************************************/

/* Delete the item foun under the mouse cursor
 *  Depending on the current active tool::
 *      Tool track
 *          if a track is in progress: Delete the last segment
 *			else delete the entire track
 *      Tool module (footprint):
 *          Delete the module.
 */
{
    bool ItemFree = (GetCurItem() == NULL )
                    || (GetCurItem()->m_Flags == 0);

    switch( m_ID_current_state )
    {
    case ID_TRACK_BUTT:
        if( GetScreen()->m_Active_Layer > CMP_N )
            return FALSE;
        if( ItemFree )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
            if( DrawStruct && DrawStruct->Type() != TYPETRACK )
                return FALSE;
            Delete_Track( DC, (TRACK*) DrawStruct );
        }
        else if( GetCurItem()->Type() == TYPETRACK  )
        {
            // simple lines for debugger:
            TRACK* track = (TRACK*) GetCurItem();
            track = Delete_Segment( DC, track ); 
            SetCurItem( track );
            GetScreen()->SetModify();
            return TRUE;
        }
        break;

    case ID_COMPONENT_BUTT:
        if( ItemFree )
        {
            MODULE* module = Locate_Prefered_Module( m_Pcb, CURSEUR_ON_GRILLE );
            if( module == NULL )
                return FALSE;
            if( !IsOK( this, _( "Delete module?" ) ) )
                return FALSE;
            RemoveStruct( module, DC );
        }
        else
            return FALSE;
        break;

    default:
        return FALSE;
    }

    GetScreen()->SetModify();
    SetCurItem( NULL );
    return TRUE;
}

