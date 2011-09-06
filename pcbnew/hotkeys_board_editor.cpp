/***************/
/* hotkeys_board_editor.cpp */
/***************/

#include "fctsys.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbnew_id.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_board_design_settings.h"

#include "hotkeys.h"
#include "protos.h"

/* How to add a new hotkey:
 * see hotkeys.cpp
 */


/**
 * Function OnHotKey.
 *  ** Commands are case insensitive **
 * Some commands are relatives to the item under the mouse cursor
 * @param aDC = current device context
 * @param aHotkeyCode = hotkey code (ascii or wxWidget code for special keys)
 * @param aPosition The current cursor position in logical (drawing) units.
 * @param aItem = NULL or pointer on a EDA_ITEM under the mouse cursor
 */
void PCB_EDIT_FRAME::OnHotKey( wxDC* aDC, int aHotkeyCode, const wxPoint& aPosition,
                               EDA_ITEM* aItem )
{
    if( aHotkeyCode == 0 )
        return;

    wxPoint pos;
    bool    itemCurrentlyEdited = (GetCurItem() && GetCurItem()->m_Flags);
    MODULE* module = NULL;
    int evt_type = 0;       //Used to post a wxCommandEvent on demand
    PCB_SCREEN* screen = GetScreen();

    /* Convert lower to upper case
     * (the usual toupper function has problem with non ascii codes like function keys
     */
    if( (aHotkeyCode >= 'a') && (aHotkeyCode <= 'z') )
        aHotkeyCode += 'A' - 'a';

    Ki_HotkeyInfo* HK_Descr = GetDescriptorFromHotkey( aHotkeyCode, common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( aHotkeyCode, board_edit_Hotkey_List );

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
        DisplayHotkeyList( this, g_Board_Editor_Hokeys_Descr );
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
        evt_type = ID_PCB_MODULE_BUTT;
        break;

    case HK_UNDO:
    case HK_REDO:
        if( !itemCurrentlyEdited )
        {
            wxCommandEvent event( wxEVT_COMMAND_TOOL_CLICKED, HK_Descr->m_IdMenuEvent );
            wxPostEvent( this, event );
        }
        break;

    case HK_RESET_LOCAL_COORD: /*Reset the relative coord  */
        GetScreen()->m_O_Curseur = GetScreen()->GetCrossHairPosition();
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
        if( /*m_ID_current_state == ID_TRACK_BUTT &&*/ (getActiveLayer() <= LAYER_N_FRONT) )
        {
            if( !itemCurrentlyEdited )
            {
                // no track is currently being edited - select a segment and remove it.
                // @todo: possibly? pass the HK command code to PcbGeneralLocateAndDisplay()
                // so it can restrict its search to specific item types.
                BOARD_ITEM * item = PcbGeneralLocateAndDisplay();

                // don't let backspace delete modules!!
                if( item && item->IsTrack() )
                {
                    Delete_Segment( aDC, (TRACK*) item );
                    SetCurItem( NULL );
                }
                OnModify();
            }
            else if( GetCurItem()->IsTrack() )
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
        if( itemCurrentlyEdited && GetCurItem()->IsTrack() && GetCurItem()->IsNew() )
        {
            // A new track is in progress: call to End_Route()
            DrawPanel->MoveCursorToCrossHair();
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
        if( GetToolId() != ID_TRACK_BUTT )
            return;
        if( !itemCurrentlyEdited )                         // no track in progress: nothing to do
            break;
        if( GetCurItem()->Type() != TYPE_TRACK )           // Should not occur
            return;
        if( !GetCurItem()->IsNew() )
            return;

        // place micro via and switch layer
        if( IsMicroViaAcceptable() )
            evt_type = ID_POPUP_PCB_PLACE_MICROVIA;
        break;

    case HK_ADD_VIA: // Switch to alternate layer and Place a via if a track is in progress
        if( !itemCurrentlyEdited ) // no track in progress: switch layer only
        {
            Other_Layer_Route( NULL, aDC );
            break;
        }
        if( GetToolId() != ID_TRACK_BUTT )
            return;
        if( GetCurItem()->Type() != TYPE_TRACK )
            return;
        if( !GetCurItem()->IsNew() )
            return;
        evt_type = ID_POPUP_PCB_PLACE_VIA;
        break;

    case HK_SWITCH_TRACK_POSTURE:
        /* change the position of initial segment when creating new tracks
         * switch from _/  to -\ .
         */
        evt_type = ID_POPUP_PCB_SWITCH_TRACK_POSTURE ;
        break;

    case HK_DRAG_TRACK_KEEP_SLOPE:
        OnHotkeyMoveItem( HK_DRAG_TRACK_KEEP_SLOPE );
        break;

    case HK_ADD_NEW_TRACK: // Start new track
        if( getActiveLayer() > LAYER_N_FRONT )
            break;

        if( GetToolId() != ID_TRACK_BUTT && !itemCurrentlyEdited )
        {
            cmd.SetId( ID_TRACK_BUTT );
            GetEventHandler()->ProcessEvent( cmd );
        }

        if( GetToolId() != ID_TRACK_BUTT )
            break;

        if( !itemCurrentlyEdited )     // no track in progress:
        {
            TRACK* track = Begin_Route( NULL, aDC );
            SetCurItem( track );
            if( track )
                DrawPanel->m_AutoPAN_Request = true;
        }
        else if( GetCurItem()->IsNew() )
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
        OnHotkeyEditItem( HK_EDIT_ITEM );
        break;

    // Footprint edition:
    case HK_LOCK_UNLOCK_FOOTPRINT: // toggle module "MODULE_is_LOCKED" status:
        // get any module, locked or not locked and toggle its locked status
        if( !itemCurrentlyEdited )
        {
            pos = screen->RefPos( true );
            module = Locate_Prefered_Module( GetBoard(), pos, screen->m_Active_Layer, true );
        }
        else if( GetCurItem()->Type() == TYPE_MODULE )
        {
            module = (MODULE*) GetCurItem();
        }

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


/**
 * Function OnHotkeyDeleteItem
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
bool PCB_EDIT_FRAME::OnHotkeyDeleteItem( wxDC* aDC )
{
    BOARD_ITEM* item = GetCurItem();
    bool ItemFree = (item == NULL) || (item->m_Flags == 0);

    switch( GetToolId() )
    {
    case ID_TRACK_BUTT:
        if( getActiveLayer() > LAYER_N_FRONT )
            return false;
        if( ItemFree )
        {
            item = PcbGeneralLocateAndDisplay();
            if( item && !item->IsTrack( ) )
                return false;
            Delete_Track( aDC, (TRACK*) item );
        }
        else if( item->IsTrack( ) )
        {
            // simple lines for debugger:
            TRACK* track = (TRACK*) item;
            track = Delete_Segment( aDC, track );
            SetCurItem( track );
            OnModify();
            return true;
        }
        break;

    case ID_PCB_MODULE_BUTT:
        if( ItemFree )
        {
            wxPoint pos = GetScreen()->RefPos( false );
            MODULE* module = Locate_Prefered_Module( GetBoard(), pos, ALL_LAYERS, false );

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
        if( ItemFree )
        {
            item = PcbGeneralLocateAndDisplay();
            if( item == NULL )
                return false;
            if( (item->Type() == TYPE_MODULE) && !IsOK( this, _( "Delete module?" ) ) )
                return false;
            RemoveStruct( item, aDC );
        }
        else
        return false;
    }

    OnModify();
    SetCurItem( NULL );
    return true;
}

bool PCB_EDIT_FRAME::OnHotkeyEditItem( int aIdCommand )
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
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_TRACKSEG;
        break;

    case TYPE_TEXTE:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_TEXTEPCB;
        break;

    case TYPE_MODULE:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_MODULE;
        break;

    case TYPE_PAD:
        // Post a EDIT_MODULE event here to prevent pads
        // from being edited by hotkeys.
        // Process_Special_Functions takes care of finding
        // the parent.
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_MODULE;
        break;

    case TYPE_MIRE:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_MIRE;
        break;

    case TYPE_DIMENSION:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_DIMENSION;
        break;

    case TYPE_TEXTE_MODULE:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_TEXTMODULE;
        break;

    case TYPE_DRAWSEGMENT:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_DRAWING;
        break;

    case TYPE_ZONE_CONTAINER:
        if( aIdCommand == HK_EDIT_ITEM )
            evt_type = ID_POPUP_PCB_EDIT_ZONE_PARAMS;
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

/**
 * Function OnHotkeyMoveItem
 * Move or drag the item (footprint, track, text .. ) found under the mouse cursor
 * An item can be moved (or dragged) only if there is no item currently edited
 * Only a footprint, a pad or a track can be dragged
 * @param aIdCommand = the hotkey command id
 * @return true if an item was moved
 */
bool PCB_EDIT_FRAME::OnHotkeyMoveItem( int aIdCommand )
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
        if( aIdCommand == HK_DRAG_TRACK_KEEP_SLOPE )
            evt_type = ID_POPUP_PCB_DRAG_TRACK_SEGMENT_KEEP_SLOPE;
        break;

    case TYPE_MODULE:
    {
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_MODULE_REQUEST;
        if( aIdCommand == HK_DRAG_ITEM )
            evt_type = ID_POPUP_PCB_DRAG_MODULE_REQUEST;
    }
    break;

    case TYPE_PAD:
        // Post MODULE_REQUEST events here to prevent pads
        // from being moved or dragged by hotkeys.
        // Process_Special_Functions takes care of finding
        // the parent.
        if( aIdCommand == HK_MOVE_ITEM )
            evt_type = ID_POPUP_PCB_MOVE_MODULE_REQUEST;
        if( aIdCommand == HK_DRAG_ITEM )
            evt_type = ID_POPUP_PCB_DRAG_MODULE_REQUEST;
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


/**
 * Function OnHotkeyRotateItem
 * Rotate the item (text or footprint) found under the mouse cursor
 * Note:
 *     this command can be used with an item currently in edit
 *     Only some items can be rotated (footprints and texts)
 * @param aIdCommand = the hotkey command id
 * @return true if an item was moved
 */
bool PCB_EDIT_FRAME::OnHotkeyRotateItem( int aIdCommand )
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
