/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jp.charras@wanadoo.fr
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
 * @file hotkeys_board_editor.cpp
 */

#include <fctsys.h>
#include <pcb_edit_frame.h>
#include <class_drawpanel.h>
#include <confirm.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_pcb_text.h>
#include <class_pcb_target.h>
#include <class_drawsegment.h>
#include <origin_viewitem.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <hotkeys.h>
#include <class_zone.h>
#include <tool/tool_manager.h>
#include <tools/pcbnew_control.h>
#include <tools/selection_tool.h>
#include <tool/actions.h>

/* How to add a new hotkey:
 * see hotkeys.cpp
 */


EDA_HOTKEY* PCB_EDIT_FRAME::GetHotKeyDescription( int aCommand ) const
{
    EDA_HOTKEY* HK_Descr = GetDescriptorFromCommand( aCommand, common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromCommand( aCommand, board_edit_Hotkey_List );

    return HK_Descr;
}


bool PCB_EDIT_FRAME::OnHotKey( wxDC* aDC, int aHotkeyCode, const wxPoint& aPosition,
                               EDA_ITEM* aItem )
{
    if( aHotkeyCode == 0 )
        return false;

    SELECTION&  selection = GetToolManager()->GetTool<SELECTION_TOOL>()->GetSelection();
    bool        itemCurrentlyEdited = selection.Front() && selection.Front()->GetEditFlags();
    MODULE*     module = NULL;
    int         evt_type = 0;       //Used to post a wxCommandEvent on demand
    PCB_SCREEN* screen = GetScreen();
    auto        displ_opts = (PCB_DISPLAY_OPTIONS*) GetDisplayOptions();

    /* Convert lower to upper case
     * (the usual toupper function has problem with non ascii codes like function keys
     */
    if( (aHotkeyCode >= 'a') && (aHotkeyCode <= 'z') )
        aHotkeyCode += 'A' - 'a';

    EDA_HOTKEY* HK_Descr = GetDescriptorFromHotkey( aHotkeyCode, common_Hotkey_List );

    if( HK_Descr == NULL )
        HK_Descr = GetDescriptorFromHotkey( aHotkeyCode, board_edit_Hotkey_List );

    if( HK_Descr == NULL )
        return false;

    int hk_id = HK_Descr->m_Idcommand;

    // Create a wxCommandEvent that will be posted in some hot keys functions
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
    cmd.SetEventObject( this );

    LAYER_NUM  ll;

    switch( hk_id )
    {
    default:
    case HK_NOT_FOUND:
        return false;

    case HK_LEFT_CLICK:
        OnLeftClick( aDC, aPosition );
        break;

    case HK_LEFT_DCLICK:    // Simulate a double left click: generate 2 events
        OnLeftClick( aDC, aPosition );
        OnLeftDClick( aDC, aPosition );
        break;

    case HK_SWITCH_TRACK_WIDTH_TO_NEXT:
        if( GetCanvas()->IsMouseCaptured() )
            GetCanvas()->CallMouseCapture( aDC, wxDefaultPosition, false );

        if( GetDesignSettings().GetTrackWidthIndex() < GetDesignSettings().m_TrackWidthList.size() - 1 )
            GetDesignSettings().SetTrackWidthIndex( GetDesignSettings().GetTrackWidthIndex() + 1 );
        else
            GetDesignSettings().SetTrackWidthIndex( 0 );

        if( GetCanvas()->IsMouseCaptured() )
            GetCanvas()->CallMouseCapture( aDC, wxDefaultPosition, false );

        break;

    case HK_SWITCH_TRACK_WIDTH_TO_PREVIOUS:
        if( GetCanvas()->IsMouseCaptured() )
            GetCanvas()->CallMouseCapture( aDC, wxDefaultPosition, false );

        if( GetDesignSettings().GetTrackWidthIndex() <= 0 )
            GetDesignSettings().SetTrackWidthIndex( GetDesignSettings().m_TrackWidthList.size() -1 );
        else
            GetDesignSettings().SetTrackWidthIndex( GetDesignSettings().GetTrackWidthIndex() - 1 );

        if( GetCanvas()->IsMouseCaptured() )
            GetCanvas()->CallMouseCapture( aDC, wxDefaultPosition, false );

        break;

    case HK_SWITCH_GRID_TO_FASTGRID1:
        SetFastGrid1();
        break;

    case HK_SWITCH_GRID_TO_FASTGRID2:
        SetFastGrid2();
        break;

    case HK_SWITCH_GRID_TO_NEXT:
        evt_type = ID_POPUP_GRID_NEXT;
        break;

    case HK_SWITCH_GRID_TO_PREVIOUS:
        evt_type = ID_POPUP_GRID_PREV;
        break;

    case HK_SWITCH_LAYER_TO_PREVIOUS:
        ll = GetActiveLayer();

        if( !IsCopperLayer( ll ) )
            break;

        if( ll == F_Cu )
            ll = B_Cu;
        else if( ll == B_Cu )
            ll = ToLAYER_ID( GetBoard()->GetCopperLayerCount() - 2 );
        else
            ll = ll - 1;

        SwitchLayer( aDC, ToLAYER_ID( ll ) );
        break;

    case HK_SWITCH_LAYER_TO_NEXT:
        ll = GetActiveLayer();

        if( !IsCopperLayer( ll ) )
            break;

        if( ll == B_Cu )
            ll = F_Cu;
        else if( ++ll >= GetBoard()->GetCopperLayerCount() - 1 )
            ll = B_Cu;

        SwitchLayer( aDC, ToLAYER_ID( ll ) );
        break;

    case HK_SWITCH_LAYER_TO_COMPONENT:
        SwitchLayer( aDC, F_Cu );
        break;

    case HK_SWITCH_LAYER_TO_COPPER:
        SwitchLayer( aDC, B_Cu );
        break;

    case HK_SWITCH_LAYER_TO_INNER1:
        SwitchLayer( aDC, In1_Cu );
        break;

    case HK_SWITCH_LAYER_TO_INNER2:
        SwitchLayer( aDC, In2_Cu );
        break;

    case HK_SWITCH_LAYER_TO_INNER3:
        SwitchLayer( aDC, In3_Cu );
        break;

    case HK_SWITCH_LAYER_TO_INNER4:
        SwitchLayer( aDC, In4_Cu );
        break;

    case HK_SWITCH_LAYER_TO_INNER5:
        SwitchLayer( aDC, In5_Cu );
        break;

    case HK_SWITCH_LAYER_TO_INNER6:
        SwitchLayer( aDC, In6_Cu );
        break;

    case HK_HELP: // Display Current hotkey list
        DisplayHotkeyList( this, g_Board_Editor_Hotkeys_Descr );
        break;

    case HK_PREFERENCES:
        evt_type = wxID_PREFERENCES;
        break;

    case HK_ADD_MODULE:
        evt_type = ID_PCB_MODULE_BUTT;
        break;

    case HK_RESET_LOCAL_COORD:  // Set the relative coord
        GetScreen()->m_O_Curseur = GetCrossHairPosition();
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

    case HK_SWITCH_TRACK_DISPLAY_MODE:
        displ_opts->m_DisplayPcbTrackFill = !displ_opts->m_DisplayPcbTrackFill;
        m_canvas->Refresh();
        break;

    case HK_BACK_SPACE:
        m_toolManager->RunAction( ACTIONS::doDelete );
        break;

    case HK_GET_AND_MOVE_FOOTPRINT:
        if( !itemCurrentlyEdited )
            evt_type = ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST;

        break;

    case HK_OPEN:
        if( !itemCurrentlyEdited )
            evt_type = ID_LOAD_FILE ;

        break;

    case HK_SAVE:
        if( !itemCurrentlyEdited )
            evt_type = ID_SAVE_BOARD;

        break;

    case HK_ADD_MICROVIA: // Place a micro via if a track is in progress
        if( GetToolId() != ID_TRACK_BUTT )
            return true;

        if( !itemCurrentlyEdited )                         // no track in progress: nothing to do
            break;

        if( GetCurItem()->Type() != PCB_TRACE_T )           // Should not occur
            return true;

        if( !GetCurItem()->IsNew() )
            return true;

        // place micro via and switch layer
        if( IsMicroViaAcceptable() )
            evt_type = ID_POPUP_PCB_PLACE_MICROVIA;

        break;

    case HK_ADD_BLIND_BURIED_VIA:
    case HK_ADD_THROUGH_VIA: // Switch to alternate layer and Place a via if a track is in progress
        if( GetBoard()->GetDesignSettings().m_BlindBuriedViaAllowed &&
            hk_id == HK_ADD_BLIND_BURIED_VIA  )
            GetBoard()->GetDesignSettings().m_CurrentViaType = VIA_BLIND_BURIED;
        else
            GetBoard()->GetDesignSettings().m_CurrentViaType = VIA_THROUGH;

        if( !itemCurrentlyEdited ) // no track in progress: switch layer only
        {
            Other_Layer_Route( NULL, aDC );
            if( displ_opts->m_ContrastModeDisplay )
                m_canvas->Refresh();
            break;
        }

        if( GetToolId() != ID_TRACK_BUTT )
            return true;

        if( GetCurItem()->Type() != PCB_TRACE_T )
            return true;

        if( !GetCurItem()->IsNew() )
            return true;

        evt_type = hk_id == HK_ADD_BLIND_BURIED_VIA ?
            ID_POPUP_PCB_PLACE_BLIND_BURIED_VIA : ID_POPUP_PCB_PLACE_THROUGH_VIA;
        break;

    case HK_SEL_LAYER_AND_ADD_THROUGH_VIA:
    case HK_SEL_LAYER_AND_ADD_BLIND_BURIED_VIA:
        if( GetCurItem() == NULL || !GetCurItem()->IsNew() ||
            GetCurItem()->Type() != PCB_TRACE_T )
            break;

        evt_type = hk_id == HK_SEL_LAYER_AND_ADD_BLIND_BURIED_VIA ?
            ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_BLIND_BURIED_VIA :
            ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_THROUGH_VIA;
        break;

    case HK_SWITCH_TRACK_POSTURE:
        /* change the position of initial segment when creating new tracks
         * switch from _/  to -\ .
         */
        evt_type = ID_POPUP_PCB_SWITCH_TRACK_POSTURE ;
        break;

    case HK_ADD_NEW_TRACK: // Start new track, if possible
        OnHotkeyBeginRoute( aDC );
        break;

    case HK_LOCK_UNLOCK_FOOTPRINT: // toggle module "MODULE_is_LOCKED" status:
        // get any module, locked or not locked and toggle its locked status
        if( !itemCurrentlyEdited )
        {
            wxPoint pos = RefPos( true );
            module = GetBoard()->GetFootprint( pos, screen->m_Active_Layer, true );
        }
        else if( GetCurItem()->Type() == PCB_MODULE_T )
        {
            module = (MODULE*) GetCurItem();
        }

        if( module )
        {
            SetCurItem( module );
            module->SetLocked( !module->IsLocked() );
            OnModify();
            SetMsgPanel( module );
        }
        break;

    case HK_MOVE_ITEM_EXACT:
    case HK_DUPLICATE:
    case HK_DUPLICATE_ITEM_AND_INCREMENT:
    case HK_CREATE_ARRAY:
        OnHotkeyDuplicateOrArrayItem( HK_Descr->m_Idcommand );
        break;

    case HK_SWITCH_HIGHCONTRAST_MODE: // switch to high contrast mode and refresh the canvas
        displ_opts->m_ContrastModeDisplay = !displ_opts->m_ContrastModeDisplay;
        m_canvas->Refresh();
        break;

    case HK_CANVAS_CAIRO:
        evt_type = ID_MENU_CANVAS_CAIRO;
        break;

    case HK_CANVAS_OPENGL:
        evt_type = ID_MENU_CANVAS_OPENGL;
        break;

    case HK_ZONE_FILL_OR_REFILL:
        evt_type = ID_POPUP_PCB_FILL_ALL_ZONES;
        break;

    case HK_ZONE_REMOVE_FILLED:
        evt_type = ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_ALL_ZONES;
        break;
    }

    if( evt_type != 0 )
    {
        wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );
        evt.SetEventObject( this );
        evt.SetId( evt_type );
        GetEventHandler()->ProcessEvent( evt );
    }

    return true;
}


TRACK * PCB_EDIT_FRAME::OnHotkeyBeginRoute( wxDC* aDC )
{
    if( !IsCopperLayer( GetActiveLayer() ) )
        return NULL;

    bool itemCurrentlyEdited = GetCurItem() && GetCurItem()->GetEditFlags();

    // Ensure the track tool is active
    if( GetToolId() != ID_TRACK_BUTT && !itemCurrentlyEdited )
    {
        wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );
        cmd.SetEventObject( this );
        cmd.SetId( ID_TRACK_BUTT );
        GetEventHandler()->ProcessEvent( cmd );
    }

    if( GetToolId() != ID_TRACK_BUTT )
        return NULL;

    TRACK* track = NULL;

    if( !itemCurrentlyEdited )     // no track in progress:
    {
        track = Begin_Route( NULL, aDC );
        SetCurItem( track );

        if( track )
            m_canvas->SetAutoPanRequest( true );
    }
    else if( GetCurItem()->IsNew() )
    {
        track = Begin_Route( (TRACK*) GetCurItem(), aDC );

        // SetCurItem() must not write to the msg panel
        // because a track info is displayed while moving the mouse cursor
        if( track )      // A new segment was created
            SetCurItem( track, false );

        m_canvas->SetAutoPanRequest( true );
    }

    return track;
}


bool PCB_EDIT_FRAME::OnHotkeyDuplicateOrArrayItem( int aIdCommand )
{
    BOARD_ITEM* item = GetCurItem();
    bool itemCurrentlyEdited = item && item->GetEditFlags();

    if( itemCurrentlyEdited )
        return false;

    item = PcbGeneralLocateAndDisplay();

    if( item == NULL )
        return false;

    SetCurItem( item );

    int evt_type = 0;       // Used to post a wxCommandEvent on demand

    bool canDuplicate = true;

    switch( item->Type() )
    {
    // Only handle items we know we can handle
    case PCB_PAD_T:
        canDuplicate = false;
        // no break
    case PCB_MODULE_T:
    case PCB_LINE_T:
    case PCB_TEXT_T:
    case PCB_TRACE_T:
    case PCB_ZONE_AREA_T:
    case PCB_TARGET_T:
    case PCB_DIMENSION_T:
        switch( aIdCommand )
        {
        case HK_CREATE_ARRAY:
            if( canDuplicate )
                evt_type = ID_POPUP_PCB_CREATE_ARRAY;
            break;

        case HK_MOVE_ITEM_EXACT:
            evt_type = ID_POPUP_PCB_MOVE_EXACT;
            break;

        default:
            // We don't handle other commands here
            break;
        }
        break;

    default:
        evt_type = 0;
        break;
    }

    return PostCommandMenuEvent( evt_type );
}
