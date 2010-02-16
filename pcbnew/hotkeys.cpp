/***************/
/* hotkeys.cpp */
/***************/

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
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
static Ki_HotkeyInfo HkAddMicroVia( wxT( "Add MicroVia" ), HK_ADD_MICROVIA, 'V'
                                    + GR_KB_CTRL );
static Ki_HotkeyInfo HkEndTrack( wxT( "End Track" ), HK_END_TRACK, WXK_END );
static Ki_HotkeyInfo HkFlipFootprint( wxT( "Flip Footprint" ), HK_FLIP_FOOTPRINT,
                                      'F' );
static Ki_HotkeyInfo HkRotateFootprint( wxT( "Rotate Footprint" ),
                                        HK_ROTATE_FOOTPRINT, 'R' );
static Ki_HotkeyInfo HkMoveFootprint( wxT( "Move Footprint" ), HK_MOVE_FOOTPRINT,
                                      'M' );
static Ki_HotkeyInfo HkDragFootprint( wxT( "Drag Footprint" ), HK_DRAG_FOOTPRINT,
                                      'G' );
static Ki_HotkeyInfo HkGetAndMoveFootprint( wxT( "Get and Move Footprint" ),
                                            HK_GET_AND_MOVE_FOOTPRINT, 'T' );
static Ki_HotkeyInfo HkLock_Unlock_Footprint( wxT( "Lock/Unlock Footprint" ),
                                              HK_LOCK_UNLOCK_FOOTPRINT, 'L' );
static Ki_HotkeyInfo HkDelete( wxT( "Delete Track or Footprint" ), HK_DELETE,
                               WXK_DELETE );
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
    &HkEndTrack,               &HkMoveFootprint,
    &HkFlipFootprint,          &HkRotateFootprint,           &HkDragFootprint,
    &HkGetAndMoveFootprint,    &HkLock_Unlock_Footprint,     &HkSavefile,
    &HkLoadfile,               &HkFindItem,                  &HkSwitch2CopperLayer,
    &HkSwitch2InnerLayer1,
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
      &g_CommonSectionTag, s_Common_Hotkey_List, "Common keys"
  },
  {
      &g_BoardEditorSectionTag, s_board_edit_Hotkey_List, "Board editor keys"
  },{
      &g_ModuleEditSectionTag, s_module_edit_Hotkey_List, "Footprint editor keys"
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

/***********************************************************/
void WinEDA_PcbFrame::OnHotKey( wxDC* DC, int hotkey, EDA_BaseStruct* DrawStruct )
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

    bool PopupOn = (GetCurItem() && GetCurItem()->m_Flags);

    bool ItemFree = (GetCurItem() == 0 || GetCurItem()->m_Flags == 0);

    if( hotkey == 0 )
        return;

    MODULE* module = NULL;

    /* Convert lower to upper case
     * (the usual toupper function has problem with non ascii codes like function keys
     */
    if( (hotkey >= 'a') && (hotkey <= 'z') )
        hotkey += 'A' - 'a';

    Ki_HotkeyInfo* HK_Descr = GetDescriptorFromHotkey( hotkey, s_Common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( hotkey, s_board_edit_Hotkey_List );

    if( HK_Descr == NULL )
        return;

    int ll;

    switch( HK_Descr->m_Idcommand )
    {
    default:
    case HK_NOT_FOUND:
        return;
        break;

    case HK_SWITCH_LAYER_TO_PREVIOUS:
        ll = GetScreen()->m_Active_Layer;
        if( (ll <= LAYER_N_BACK) || (ll > LAYER_N_FRONT) )
            break;

        if( GetBoard()->GetCopperLayerCount() < 2 ) // Single layer
            ll = LAYER_N_BACK;
        else if( ll == LAYER_N_FRONT )
            ll = MAX( LAYER_N_BACK,
                      GetBoard()->GetCopperLayerCount() - 2 );
        else
            ll--;
        SwitchLayer( DC, ll );
        break;

    case HK_SWITCH_LAYER_TO_NEXT:
        ll = GetScreen()->m_Active_Layer;
        if( (ll < LAYER_N_BACK) || (ll >= LAYER_N_FRONT) )
            break;
        if( GetBoard()->GetCopperLayerCount() < 2 ) // Single layer
            ll = LAYER_N_BACK;
        else if( ll >= GetBoard()->GetCopperLayerCount() - 2 )
            ll = LAYER_N_FRONT;
        else
            ll++;
        SwitchLayer( DC, ll );
        break;

    case HK_SWITCH_LAYER_TO_COMPONENT:
        SwitchLayer( DC, LAYER_N_FRONT );
        break;

    case HK_SWITCH_LAYER_TO_COPPER:
        SwitchLayer( DC, LAYER_N_BACK );
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

    case HK_HELP: // Display Current hotkey list
        DisplayHotkeyList( this, s_Board_Editor_Hokeys_Descr );
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

    case HK_ADD_MODULE:
        cmd.SetId( ID_COMPONENT_BUTT );
        GetEventHandler()->ProcessEvent( cmd );
        break;

    case HK_ZOOM_AUTO:
        cmd.SetId( ID_ZOOM_PAGE );
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

    case HK_RESET_LOCAL_COORD: /*Reset the relative coord  */
        GetScreen()->m_O_Curseur = GetScreen()->m_Curseur;
        break;

    case HK_SWITCH_UNITS:
        g_UnitMetric = (g_UnitMetric == INCHES) ? MILLIMETRE : INCHES;
        break;

    case HK_SWITCH_TRACK_DISPLAY_MODE:
        DisplayOpt.DisplayPcbTrackFill ^= 1;
        DisplayOpt.DisplayPcbTrackFill &= 1;
        m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
        GetScreen()->SetRefreshReq();
        break;

    case HK_DELETE:
        OnHotkeyDeleteItem( DC, DrawStruct );
        break;

    case HK_BACK_SPACE:
        if( m_ID_current_state == ID_TRACK_BUTT && GetScreen()->m_Active_Layer
            <= LAYER_N_FRONT )
        {
            if( ItemFree )
            {
                // no track is currently being edited - select a segment and remove it.
                // @todo: possibly? pass the HK command code to PcbGeneralLocateAndDisplay() so it can restrict its search to specific item types.
                // @todo: use PcbGeneralLocateAndDisplay() everywhere in this source file.

                DrawStruct = PcbGeneralLocateAndDisplay();

                // don't let backspace delete modules!!
                if( DrawStruct && (DrawStruct->Type() == TYPE_TRACK
                                   || DrawStruct->Type() == TYPE_VIA) )
                {
                    Delete_Segment( DC, (TRACK*) DrawStruct );
                    SetCurItem( NULL );
                }
                GetScreen()->SetModify();
            }
            else if( GetCurItem()->Type() == TYPE_TRACK )
            {
                // then an element is being edited - remove the last segment.
                // simple lines for debugger:
                TRACK* track = (TRACK*) GetCurItem();
                track = Delete_Segment( DC, track );
                SetCurItem( track );
                GetScreen()->SetModify();
            }
        }
        break;

    case HK_END_TRACK:
        if( !ItemFree && (GetCurItem()->Type() == TYPE_TRACK)
           && ( (GetCurItem()->m_Flags & IS_NEW) != 0 ) )
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

    case HK_ADD_MICROVIA: // Place a micro via if a track is in progress
        if( m_ID_current_state != ID_TRACK_BUTT )
            return;
        if( ItemFree )                              // no track in progress: nothing to do
            break;
        if( GetCurItem()->Type() != TYPE_TRACK )    // Should not occur
            return;
        if( (GetCurItem()->m_Flags & IS_NEW) == 0 )
            return;

        // place micro via and switch layer
        if( IsMicroViaAcceptable() )
        {
            int v_type = GetBoard()->GetBoardDesignSettings()->m_CurrentViaType;
            GetBoard()->GetBoardDesignSettings()->m_CurrentViaType = VIA_MICROVIA;
            Other_Layer_Route( (TRACK*) GetCurItem(), DC );
            GetBoard()->GetBoardDesignSettings()->m_CurrentViaType = v_type;
            if( DisplayOpt.ContrastModeDisplay )
                GetScreen()->SetRefreshReq();
        }
        break;

    case HK_ADD_VIA: // Switch to alternate layer and Place a via if a track is in progress
        if( m_ID_current_state != ID_TRACK_BUTT )
            return;
        if( ItemFree ) // no track in progress: switch layer only
        {
            Other_Layer_Route( NULL, DC );
            break;
        }
        if( GetCurItem()->Type() != TYPE_TRACK )
            return;
        if( (GetCurItem()->m_Flags & IS_NEW) == 0 )
            return;
        Other_Layer_Route( (TRACK*) GetCurItem(), DC ); // place via and switch layer
        if( DisplayOpt.ContrastModeDisplay )
            GetScreen()->SetRefreshReq();
        break;

    case HK_ADD_NEW_TRACK: // Start new track
        if( GetScreen()->m_Active_Layer > LAYER_N_FRONT )
            break;

        if( m_ID_current_state != ID_TRACK_BUTT && ItemFree )
        {
            cmd.SetId( ID_TRACK_BUTT );
            GetEventHandler()->ProcessEvent( cmd );
        }

        if( m_ID_current_state != ID_TRACK_BUTT )
            break;

        if( ItemFree )     // no track in progress:
        {
            TRACK* track = Begin_Route( NULL, DC );
            SetCurItem( track );
            if( track )
                DrawPanel->m_AutoPAN_Request = true;
        }
        else if( GetCurItem()->m_Flags & IS_NEW )
        {
            TRACK* track = Begin_Route( (TRACK*) GetCurItem(), DC );

            // SetCurItem() must not write to the msg panel
            // because a track info is displayed while moving the mouse cursor
            if( track )      // A new segment was created
                SetCurItem( track, false );
            DrawPanel->m_AutoPAN_Request = true;
        }
        break;

    // Footprint edition:
    case HK_LOCK_UNLOCK_FOOTPRINT: // toggle module "MODULE_is_LOCKED" status:
        // get any module, locked or not locked and toggle its locked status
        if( ItemFree )
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

    case HK_DRAG_FOOTPRINT: // Start move (and drag) module
    case HK_MOVE_FOOTPRINT: // Start move module
        if( PopupOn )
            break;

    case HK_ROTATE_FOOTPRINT:   // Rotation
    case HK_FLIP_FOOTPRINT:     // move to other side
        int exit = 0;
        if( m_ID_current_state == ID_TRACK_BUTT )
        {
            if( ItemFree )
                DrawStruct = PcbGeneralLocateAndDisplay();
            else
                DrawStruct = GetCurItem();

            if( DrawStruct && (DrawStruct->Type() == TYPE_TRACK
                               || DrawStruct->Type() == TYPE_VIA) )
                switch( HK_Descr->m_Idcommand )
                {
                case HK_DRAG_FOOTPRINT: // Start move (and drag) module
                    DrawPanel->MouseToCursorSchema();

                    //Start_DragTrackSegmentAndKeepSlope( (TRACK*) DrawStruct,DC );
                    Start_MoveOneNodeOrSegment( (TRACK*) DrawStruct, DC,
                                               ID_POPUP_PCB_DRAG_TRACK_SEGMENT );
                    break;

                // fall through
                case HK_MOVE_FOOTPRINT: // Start move module
                    DrawPanel->MouseToCursorSchema();
                    Start_MoveOneNodeOrSegment( (TRACK*) DrawStruct, DC,
                                               ID_POPUP_PCB_MOVE_TRACK_NODE );
                    break;
                }

            else
                exit = 1;
        }
        else if( !exit )
        {
            if( ItemFree )
            {
                module = Locate_Prefered_Module( GetBoard(), CURSEUR_OFF_GRILLE
                                                 | IGNORE_LOCKED | VISIBLE_ONLY
#if defined(USE_MATCH_LAYER)
                                                 | MATCH_LAYER
#endif
                                                 );

                if( module == NULL ) // no footprint found
                {
                    module = Locate_Prefered_Module( GetBoard(),
                                                     CURSEUR_OFF_GRILLE | VISIBLE_ONLY );
                    if( module )
                    {
                        // a footprint is found, but locked or on an other layer
                        if( module->IsLocked() )
                        {
                            wxString msg;

                            msg.Printf( _( "Footprint %s found, but locked" ),
                                       module->m_Reference->m_Text.GetData() );

                            DisplayInfoMessage( this, msg );
                        }
                        module = NULL;
                    }
                }
            }
            else if( GetCurItem()->Type() == TYPE_MODULE )
            {
                module = (MODULE*) GetCurItem();

                // @todo: might need to add a layer check in if() below
                if( (GetCurItem()->m_Flags == 0) && module->IsLocked() )
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
            case HK_ROTATE_FOOTPRINT:           // Rotation
                if( module->m_Flags == 0 )      // not currently in edit, prepare undo command
                    SaveCopyInUndoList( module, UR_ROTATED, module->m_Pos );
                Rotate_Module( DC, module, 900, TRUE );
                break;

            case HK_FLIP_FOOTPRINT:             // move to other side
                if( module->m_Flags == 0 )      // not currently in edit, prepare undo command
                    SaveCopyInUndoList( module, UR_FLIPPED, module->m_Pos );
                Change_Side_Module( module, DC );
                break;

            case HK_DRAG_FOOTPRINT: // Start move (and drag) module
                g_Drag_Pistes_On = TRUE;

            // fall through
            case HK_MOVE_FOOTPRINT: // Start move module
                StartMove_Module( module, DC );
                break;
            }

            module->DisplayInfo( this );
            break;
        }
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

    bool           ItemFree = (GetCurItem() == 0 || GetCurItem()->m_Flags == 0);
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
        g_UnitMetric = (g_UnitMetric == INCHES) ? MILLIMETRE : INCHES;
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
    bool ItemFree = (GetCurItem() == NULL) || (GetCurItem()->m_Flags == 0);

    switch( m_ID_current_state )
    {
    case ID_TRACK_BUTT:
        if( GetScreen()->m_Active_Layer > LAYER_N_FRONT )
            return FALSE;
        if( ItemFree )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
            if( DrawStruct && DrawStruct->Type() != TYPE_TRACK )
                return FALSE;
            Delete_Track( DC, (TRACK*) DrawStruct );
        }
        else if( GetCurItem()->Type() == TYPE_TRACK )
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
            MODULE* module = Locate_Prefered_Module( GetBoard(),
                                                     CURSEUR_ON_GRILLE );
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
