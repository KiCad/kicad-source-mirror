/**
 * @file pcbnew/onrightclick.cpp
 * @brief Right mouse button functions.
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "macros.h"

#include "class_board.h"
#include "class_module.h"
#include "class_track.h"
#include "class_pcb_text.h"
#include "class_zone.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "pcbnew_id.h"
#include "hotkeys.h"
#include "collectors.h"


static wxMenu* Append_Track_Width_List( BOARD* aBoard );


bool PCB_EDIT_FRAME::OnRightClick( const wxPoint& aMousePos, wxMenu* aPopMenu )
{
    wxString    msg;
    int         flags = 0;
    bool        locate_track = false;
    bool        blockActive  = (GetScreen()->m_BlockLocate.m_Command != BLOCK_IDLE);

    wxClientDC  dc( DrawPanel );

    BOARD_ITEM* item = GetCurItem();

    DrawPanel->m_CanStartBlock = -1;    // Avoid to start a block coomand when clicking on menu

    // If a command or a block is in progress:
    // Put the Cancel command (if needed) and the End command

    if( blockActive )
    {
        createPopUpBlockMenu( aPopMenu );
        aPopMenu->AppendSeparator();
        return true;
    }

    DrawPanel->CrossHairOff( &dc );

    if( GetToolId() != ID_NO_TOOL_SELECTED )
    {
        if( item && item->m_Flags )
        {
            AddMenuItem( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel" ), KiBitmap( cancel_xpm ) );
        }
        else
        {
            AddMenuItem( aPopMenu, ID_POPUP_CLOSE_CURRENT_TOOL,
                         _( "End Tool" ), KiBitmap( cancel_tool_xpm ) );
        }

        aPopMenu->AppendSeparator();
    }
    else
    {
        if( item && item->m_Flags )
        {
            AddMenuItem( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                         _( "Cancel" ), KiBitmap( cancel_xpm ) );
            aPopMenu->AppendSeparator();
        }
    }

    /* Select a proper item */

    wxPoint cursorPos = GetScreen()->GetCrossHairPosition();
    wxPoint selectPos = m_Collector->GetRefPos();

    selectPos = GetScreen()->GetNearestGridPosition( selectPos );

    /*  We can reselect another item only if there are no item being edited
     * because ALL moving functions use GetCurItem(), therefore GetCurItem()
     * must return the same item during moving. We know an item is moving
     * if( item && (item->m_Flags != 0)) is true and after calling
     * PcbGeneralLocateAndDisplay(), GetCurItem() is any arbitrary BOARD_ITEM,
     * not the current item being edited. In such case we cannot call
     * PcbGeneralLocateAndDisplay().
     */
    if( !item || (item->m_Flags == 0) )
    {
        // show "item selector" menu only if no item now or selected item was not
        // previously picked at this position
        if( !item || cursorPos != selectPos )
        {
            DrawPanel->m_AbortRequest = false;
            item = PcbGeneralLocateAndDisplay();

            if( DrawPanel->m_AbortRequest )
            {
                DrawPanel->CrossHairOn( &dc );
                return false;
            }
        }
    }

    item = GetCurItem();

    if( item )
        flags = item->m_Flags;
    else
        flags = 0;

    if( item )
    {
        switch( item->Type() )
        {
        case TYPE_MODULE:
            createPopUpMenuForFootprints( (MODULE*) item, aPopMenu );

            if( m_HTOOL_current_state == ID_TOOLBARH_PCB_MODE_MODULE )
            {
                aPopMenu->AppendSeparator();

                if( !( (MODULE*) item )->IsLocked() )
                {
                    msg = AddHotkeyName( _("Lock Module" ), g_Board_Editor_Hokeys_Descr,
                                         HK_LOCK_UNLOCK_FOOTPRINT );
                    AddMenuItem( aPopMenu, ID_POPUP_PCB_AUTOPLACE_FIXE_MODULE, msg,
                                 KiBitmap( locked_xpm ) );
                }
                else
                {
                    msg = AddHotkeyName( _( "Unlock Module" ), g_Board_Editor_Hokeys_Descr,
                                         HK_LOCK_UNLOCK_FOOTPRINT );
                    AddMenuItem( aPopMenu, ID_POPUP_PCB_AUTOPLACE_FREE_MODULE, msg,
                                 KiBitmap( unlocked_xpm ) );
                }

                if( !flags )
                    aPopMenu->Append( ID_POPUP_PCB_AUTOPLACE_CURRENT_MODULE,
                                      _( "Auto Place Module" ) );
            }

            if( m_HTOOL_current_state == ID_TOOLBARH_PCB_MODE_TRACKS )
            {
                if( !flags )
                    aPopMenu->Append( ID_POPUP_PCB_AUTOROUTE_MODULE, _( "Autoroute Module" ) );
            }
            break;

        case TYPE_PAD:
            createPopUpMenuForFpPads( (D_PAD*) item, aPopMenu );
            break;

        case TYPE_TEXTE_MODULE:
            createPopUpMenuForFpTexts( (TEXTE_MODULE*) item, aPopMenu );
            break;

        case TYPE_DRAWSEGMENT:  // Some graphic items on technical layers
            if( (flags & IS_NEW) )
            {
                AddMenuItem( aPopMenu, ID_POPUP_PCB_STOP_CURRENT_DRAWING,
                             _( "End Drawing" ), KiBitmap( apply_xpm ) );
            }

            if( !flags )
            {
                msg = AddHotkeyName( _( "Move Drawing" ), g_Board_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_MOVE_DRAWING_REQUEST,
                             msg, KiBitmap( move_xpm ) );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_EDIT_DRAWING, _( "Edit Drawing" ), KiBitmap( edit_xpm ) );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_DRAWING,
                             _( "Delete Drawing" ), KiBitmap( delete_xpm ) );

                if( item->GetLayer() > LAST_COPPER_LAYER )
                    AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_DRAWING_LAYER,
                                 _( "Delete All Drawing on Layer" ), KiBitmap( delete_body_xpm ) );
            }

            break;

        case TYPE_ZONE:      // Item used to fill a zone
            AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_ZONE,
                         _( "Delete Zone Filling" ), KiBitmap( delete_xpm ) );
            break;

        case TYPE_ZONE_CONTAINER:    // Item used to handle a zone area (outlines, holes ...)
            if( flags & IS_NEW )
            {
                AddMenuItem( aPopMenu, ID_POPUP_PCB_STOP_CURRENT_EDGE_ZONE,
                             _( "Close Zone Outline" ), KiBitmap( apply_xpm ) );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_ZONE_LAST_CREATED_CORNER,
                             _( "Delete Last Corner" ), KiBitmap( delete_xpm ) );
            }
            else
            {
                createPopUpMenuForZones( (ZONE_CONTAINER*) item, aPopMenu );
            }

            break;

        case TYPE_TEXTE:
            createPopUpMenuForTexts( (TEXTE_PCB*) item, aPopMenu );
            break;

        case TYPE_TRACK:
        case TYPE_VIA:
            locate_track = true;
            createPopupMenuForTracks( (TRACK*) item, aPopMenu );
            break;

        case TYPE_MARKER_PCB:
            createPopUpMenuForMarkers( (MARKER_PCB*) item, aPopMenu );
            break;

        case TYPE_DIMENSION:
            if( !flags )
            {
                msg = AddHotkeyName( _( "Edit Dimension" ), g_Board_Editor_Hokeys_Descr,
                                     HK_EDIT_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_EDIT_DIMENSION, msg, KiBitmap( edit_xpm ) );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_DIMENSION,
                             _( "Delete Dimension" ), KiBitmap( delete_xpm ) );
            }

            break;

        case PCB_TARGET_T:
            if( !flags )
            {
                msg = AddHotkeyName( _( "Move Target" ), g_Board_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_MOVE_MIRE_REQUEST, msg, KiBitmap( move_xpm ) );
                msg = AddHotkeyName( _( "Edit Target" ), g_Board_Editor_Hokeys_Descr,
                                     HK_EDIT_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_EDIT_MIRE, msg, KiBitmap( edit_xpm ) );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_MIRE,
                             _( "Delete Target" ), KiBitmap( delete_xpm ) );
            }

            break;

        case TYPE_EDGE_MODULE:
        case TYPE_SCREEN:
        case TYPE_NOT_INIT:
        case TYPE_PCB:
            msg.Printf( wxT( "PCB_EDIT_FRAME::OnRightClick() Error: unexpected DrawType %d" ),
                        item->Type() );
            DisplayError( this, msg );
            SetCurItem( NULL );
            break;

        default:
            msg.Printf( wxT( "PCB_EDIT_FRAME::OnRightClick() Error: unknown DrawType %d" ),
                        item->Type() );
            DisplayError( this, msg );

            // Attempt to clear error (but should no occurs )
            if( item->Type() >= MAX_STRUCT_TYPE_ID )
                SetCurItem( NULL );

            break;
        }

        aPopMenu->AppendSeparator();
    }

    if( !flags )
    {
        msg = AddHotkeyName( _( "Get and Move Footprint" ),
                             g_Board_Editor_Hokeys_Descr, HK_GET_AND_MOVE_FOOTPRINT );
        AddMenuItem( aPopMenu, ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST,
                     msg, KiBitmap( move_module_xpm ) );
    }

    /* Display context sensitive comands: */
    switch(  GetToolId() )
    {
    case ID_PCB_ZONES_BUTT:
        if(  GetBoard()->m_ZoneDescriptorList.size() > 0 )
        {
            aPopMenu->AppendSeparator();
            AddMenuItem( aPopMenu, ID_POPUP_PCB_FILL_ALL_ZONES,
                         _( "Fill or Refill All Zones" ), KiBitmap( fill_zone_xpm ) );
            AddMenuItem( aPopMenu, ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_ALL_ZONES,
                         _( "Remove Filled Areas in All Zones" ), KiBitmap( fill_zone_xpm ) );
            aPopMenu->AppendSeparator();
        }

        AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_LAYER,
                     _( "Select Working Layer" ), KiBitmap( select_w_layer_xpm ) );
        aPopMenu->AppendSeparator();
        break;

    case ID_TRACK_BUTT:
        if ( ! locate_track )   // This menu is already added when a track is located
            AddMenuItem( aPopMenu, Append_Track_Width_List( GetBoard() ),
                         ID_POPUP_PCB_SELECT_WIDTH, _( "Select Track Width" ), KiBitmap( width_track_xpm ) );

        AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_CU_LAYER,
                     _( "Select Working Layer" ), KiBitmap( select_w_layer_xpm ) );
        AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_LAYER_PAIR,
                     _( "Select Layer Pair for Vias" ), KiBitmap( select_layer_pair_xpm ) );
        aPopMenu->AppendSeparator();
        break;

    case ID_PCB_CIRCLE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_ADD_TEXT_BUTT:
    case ID_PCB_ADD_LINE_BUTT:
    case ID_PCB_DIMENSION_BUTT:
        AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_NO_CU_LAYER,
                      _( "Select Working Layer" ), KiBitmap( select_w_layer_xpm ) );
        aPopMenu->AppendSeparator();
        break;

    case ID_PCB_MODULE_BUTT:
        AddMenuItem( aPopMenu, ID_POPUP_PCB_DISPLAY_FOOTPRINT_DOC,
                     _( "Footprint Documentation" ), KiBitmap( book_xpm ) );
        aPopMenu->AppendSeparator();
        break;

    case ID_NO_TOOL_SELECTED:
        if( m_HTOOL_current_state == ID_TOOLBARH_PCB_MODE_MODULE )
        {
            wxMenu* commands = new wxMenu;
            AddMenuItem( aPopMenu, commands, ID_POPUP_PCB_AUTOPLACE_COMMANDS,
                         _( "Glob Move and Place" ), KiBitmap( move_xpm ) );
            AddMenuItem( commands, ID_POPUP_PCB_AUTOPLACE_FREE_ALL_MODULES,
                         _( "Unlock All Modules" ), KiBitmap( unlocked_xpm ) );
            AddMenuItem( commands, ID_POPUP_PCB_AUTOPLACE_FIXE_ALL_MODULES,
                         _( "Lock All Modules" ), KiBitmap( locked_xpm ) );
            commands->AppendSeparator();
            AddMenuItem( commands, ID_POPUP_PCB_AUTOMOVE_ALL_MODULES,
                         _( "Move All Modules" ), KiBitmap( move_xpm ) );
            commands->Append( ID_POPUP_PCB_AUTOMOVE_NEW_MODULES, _( "Move New Modules" ) );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOPLACE_ALL_MODULES, _( "Autoplace All Modules" ) );
            commands->Append( ID_POPUP_PCB_AUTOPLACE_NEW_MODULES, _( "Autoplace New Modules" ) );
            commands->Append( ID_POPUP_PCB_AUTOPLACE_NEXT_MODULE, _( "Autoplace Next Module" ) );
            commands->AppendSeparator();
            AddMenuItem( commands, ID_POPUP_PCB_REORIENT_ALL_MODULES,
                         _( "Orient All Modules" ), KiBitmap( rotate_module_pos_xpm ) );
            aPopMenu->AppendSeparator();
        }

        if( m_HTOOL_current_state == ID_TOOLBARH_PCB_MODE_TRACKS )
        {
            wxMenu* commands = new wxMenu;
            aPopMenu->Append( ID_POPUP_PCB_AUTOROUTE_COMMANDS, _( "Autoroute" ), commands );
            AddMenuItem( commands, ID_POPUP_PCB_SELECT_LAYER_PAIR,
                         _( "Select Layer Pair" ), KiBitmap( select_layer_pair_xpm ) );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOROUTE_ALL_MODULES, _( "Autoroute All Modules" ) );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOROUTE_RESET_UNROUTED, _( "Reset Unrouted" ) );
            aPopMenu->AppendSeparator();
        }

        if( locate_track )
            AddMenuItem( aPopMenu, Append_Track_Width_List( GetBoard() ),
                         ID_POPUP_PCB_SELECT_WIDTH, _( "Select Track Width" ),
                         KiBitmap( width_track_xpm ) );

        AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_LAYER,
                     _( "Select Working Layer" ), KiBitmap( select_w_layer_xpm ) );
        aPopMenu->AppendSeparator();
        break;
    }

    DrawPanel->CrossHairOn( &dc );
    return true;
}


/* Create Pop sub menu for block commands
 */
void PCB_EDIT_FRAME::createPopUpBlockMenu( wxMenu* menu )
{
    AddMenuItem( menu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel Block" ), KiBitmap( cancel_xpm ) );
    AddMenuItem( menu, ID_POPUP_ZOOM_BLOCK, _( "Zoom Block" ), KiBitmap( zoom_area_xpm ) );
    menu->AppendSeparator();
    AddMenuItem( menu, ID_POPUP_PLACE_BLOCK, _( "Place Block" ), KiBitmap( apply_xpm ) );
    AddMenuItem( menu, ID_POPUP_COPY_BLOCK, _( "Copy Block" ), KiBitmap( copyblock_xpm ) );
    AddMenuItem( menu, ID_POPUP_FLIP_BLOCK, _( "Flip Block" ), KiBitmap( invert_module_xpm ) );
    AddMenuItem( menu, ID_POPUP_ROTATE_BLOCK, _( "Rotate Block" ), KiBitmap( rotate_ccw_xpm ) );
    AddMenuItem( menu, ID_POPUP_DELETE_BLOCK, _( "Delete Block" ), KiBitmap( delete_xpm ) );
}


/* Create command lines for a popup menu, for track and via editing
 * also update Netclass selection
 */
void PCB_EDIT_FRAME::createPopupMenuForTracks( TRACK* Track, wxMenu* PopMenu )
{
    wxPoint  cursorPosition = GetScreen()->GetCrossHairPosition();
    wxString msg;

    GetBoard()->SetCurrentNetClass( Track->GetNetClassName() );
    updateTraceWidthSelectBox();
    updateViaSizeSelectBox();

    int flags = Track->m_Flags;

    if( flags == 0 )
    {
        if( Track->Type() == TYPE_VIA )
        {
            AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_TRACK_NODE, _( "Drag Via" ), KiBitmap( move_xpm ) );
        }
        else
        {
            if( Track->IsPointOnEnds( cursorPosition, -1 ) != 0 )
            {
                msg = AddHotkeyName( _( "Move Node" ), g_Board_Editor_Hokeys_Descr, HK_MOVE_ITEM );
                AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_TRACK_NODE,
                             msg, KiBitmap( move_xpm ) );
            }
            else
            {
                msg = AddHotkeyName( _( "Drag Segments, Keep Slope" ), g_Board_Editor_Hokeys_Descr,
                                     HK_DRAG_TRACK_KEEP_SLOPE );
                AddMenuItem( PopMenu, ID_POPUP_PCB_DRAG_TRACK_SEGMENT_KEEP_SLOPE,
                             msg, KiBitmap( drag_segment_withslope_xpm ) );
                msg = AddHotkeyName( _( "Drag Segment" ), g_Board_Editor_Hokeys_Descr,
                                     HK_DRAG_ITEM );
                AddMenuItem( PopMenu, ID_POPUP_PCB_DRAG_TRACK_SEGMENT,
                             msg, KiBitmap( drag_track_segment_xpm ) );
                AddMenuItem( PopMenu, ID_POPUP_PCB_BREAK_TRACK,
                             _( "Break Track" ), KiBitmap( break_line_xpm ) );
            }
        }
    }
    else if( flags & IS_DRAGGED )   // Drag via or node in progress
    {
        AddMenuItem( PopMenu, ID_POPUP_PCB_PLACE_MOVED_TRACK_NODE,
                     _( "Place Node" ), KiBitmap( apply_xpm ) );
        return;
    }
    else // Edition in progress
    {
        if( flags & IS_NEW )
        {
            msg = AddHotkeyName( _( "End Track" ), g_Board_Editor_Hokeys_Descr, HK_END_TRACK );
            AddMenuItem( PopMenu, ID_POPUP_PCB_END_TRACK, msg, KiBitmap( apply_xpm ) );
        }

        msg = AddHotkeyName( _( "Place Via" ), g_Board_Editor_Hokeys_Descr, HK_ADD_VIA );
        PopMenu->Append( ID_POPUP_PCB_PLACE_VIA, msg );

        msg = AddHotkeyName( _( "Switch Track Posture" ), g_Board_Editor_Hokeys_Descr,
                             HK_SWITCH_TRACK_POSTURE );
        PopMenu->Append( ID_POPUP_PCB_SWITCH_TRACK_POSTURE, msg );

        // See if we can place a Micro Via (4 or more layers, and start from an external layer):
        if( IsMicroViaAcceptable() )
        {
            msg = AddHotkeyName( _( "Place Micro Via" ), g_Board_Editor_Hokeys_Descr,
                                 HK_ADD_MICROVIA );
            PopMenu->Append( ID_POPUP_PCB_PLACE_MICROVIA, msg );
        }
    }

    // track Width control :
    if( !flags )
    {
        if( Track->Type() == TYPE_VIA )
        {
            msg = AddHotkeyName( _( "Change Via Size and Drill" ), g_Board_Editor_Hokeys_Descr,
                                 HK_EDIT_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_TRACKSEG, msg, KiBitmap( width_segment_xpm ) );
        }
        else
        {
            msg = AddHotkeyName( _( "Change Segment Width" ), g_Board_Editor_Hokeys_Descr,
                                 HK_EDIT_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_TRACKSEG, msg, KiBitmap( width_segment_xpm ) );
            AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_TRACK,
                         _( "Change Track Width" ), KiBitmap( width_track_xpm ) );
        }
    }

    // Allows switching to an other track/via size when routing
    AddMenuItem( PopMenu, Append_Track_Width_List( GetBoard() ), ID_POPUP_PCB_SELECT_WIDTH,
                 _( "Select Track Width" ), KiBitmap( width_track_xpm ) );

    // Delete control:
    PopMenu->AppendSeparator();
    wxMenu* track_mnu = new wxMenu;
    AddMenuItem( PopMenu, track_mnu, ID_POPUP_PCB_DELETE_TRACK_MNU, _( "Delete" ), KiBitmap( delete_xpm ) );

    msg = AddHotkeyName( Track->Type()==TYPE_VIA ?
                        _( "Delete Via" ) : _( "Delete Segment" ),
                         g_Board_Editor_Hokeys_Descr, HK_BACK_SPACE );

    AddMenuItem( track_mnu, ID_POPUP_PCB_DELETE_TRACKSEG, msg, KiBitmap( delete_line_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Delete Track" ), g_Board_Editor_Hokeys_Descr, HK_DELETE );
        AddMenuItem( track_mnu, ID_POPUP_PCB_DELETE_TRACK, msg, KiBitmap( delete_track_xpm ) );
        AddMenuItem( track_mnu, ID_POPUP_PCB_DELETE_TRACKNET, _( "Delete Net" ), KiBitmap( delete_net_xpm ) );
    }

    // Add global edition command
    if( !flags )
    {
        PopMenu->AppendSeparator();
        AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_ALL_VIAS_AND_TRACK_SIZE,
                     _( "Global Tracks and Vias Edition" ), KiBitmap( width_track_via_xpm ) );
    }

    // Add lock/unlock flags menu:
    track_mnu = new wxMenu;

    AddMenuItem( PopMenu, track_mnu, ID_POPUP_PCB_SETFLAGS_TRACK_MNU, _( "Set Flags" ), KiBitmap( flag_xpm ) );
    track_mnu->Append( ID_POPUP_PCB_LOCK_ON_TRACKSEG, _( "Locked: Yes" ), wxEmptyString, true );
    track_mnu->Append( ID_POPUP_PCB_LOCK_OFF_TRACKSEG, _( "Locked: No" ), wxEmptyString, true );

    if( Track->GetState( TRACK_LOCKED ) )
        track_mnu->Check( ID_POPUP_PCB_LOCK_ON_TRACKSEG, true );
    else
        track_mnu->Check( ID_POPUP_PCB_LOCK_OFF_TRACKSEG, true );

    if( !flags )
    {
        track_mnu->Append( ID_POPUP_PCB_LOCK_ON_TRACK, _( "Track Locked: Yes" ) );
        track_mnu->Append( ID_POPUP_PCB_LOCK_OFF_TRACK, _( "Track Locked: No" ) );
        track_mnu->AppendSeparator();
        track_mnu->Append( ID_POPUP_PCB_LOCK_ON_NET, _( "Net Locked: Yes" ) );
        track_mnu->Append( ID_POPUP_PCB_LOCK_OFF_NET, _( "Net Locked: No" ) );
    }
}


/* Create the wxMenuitem list for zone outlines editing and zone filling
 */
void PCB_EDIT_FRAME::createPopUpMenuForZones( ZONE_CONTAINER* edge_zone, wxMenu* aPopMenu )
{
    wxString msg;

    if( edge_zone->m_Flags == IS_DRAGGED )
    {
        AddMenuItem( aPopMenu, ID_POPUP_PCB_PLACE_DRAGGED_ZONE_OUTLINE_SEGMENT,
                     _( "Place Edge Outline" ), KiBitmap( apply_xpm ) );
    }
    else if( edge_zone->m_Flags )
    {
        if( (edge_zone->m_Flags & IN_EDIT ) )
            AddMenuItem( aPopMenu, ID_POPUP_PCB_PLACE_ZONE_CORNER,
                         _( "Place Corner" ), KiBitmap( apply_xpm ) );
        else
            AddMenuItem( aPopMenu, ID_POPUP_PCB_PLACE_ZONE_OUTLINES,
                         _( "Place Zone" ), KiBitmap( apply_xpm ) );
    }
    else
    {
        wxMenu* zones_menu = new wxMenu();

        AddMenuItem( aPopMenu, zones_menu, -1, _( "Zones" ), KiBitmap( add_zone_xpm ) );

        if( edge_zone->HitTestForCorner( GetScreen()->RefPos( true ) ) )
        {
            AddMenuItem( zones_menu, ID_POPUP_PCB_MOVE_ZONE_CORNER,
                         _( "Move Corner" ), KiBitmap( move_xpm ) );
            AddMenuItem( zones_menu, ID_POPUP_PCB_DELETE_ZONE_CORNER,
                         _( "Delete Corner" ), KiBitmap( delete_xpm ) );
        }
        else if( edge_zone->HitTestForEdge( GetScreen()->RefPos( true ) ) )
        {
            AddMenuItem( zones_menu, ID_POPUP_PCB_ADD_ZONE_CORNER,
                         _( "Create Corner" ), KiBitmap( add_corner_xpm ) );
            msg = AddHotkeyName( _( "Drag Outline Segment" ), g_Board_Editor_Hokeys_Descr,
                                 HK_DRAG_ITEM );
            AddMenuItem( zones_menu, ID_POPUP_PCB_DRAG_ZONE_OUTLINE_SEGMENT,
                         msg, KiBitmap( drag_outline_segment_xpm ) );
        }

        zones_menu->AppendSeparator();
        AddMenuItem( zones_menu, ID_POPUP_PCB_ZONE_ADD_SIMILAR_ZONE,
                     _( "Add Similar Zone" ), KiBitmap( add_zone_xpm ) );

        AddMenuItem( zones_menu, ID_POPUP_PCB_ZONE_ADD_CUTOUT_ZONE,
                     _( "Add Cutout Area" ), KiBitmap( add_zone_cutout_xpm ) );
        zones_menu->AppendSeparator();

        AddMenuItem( zones_menu, ID_POPUP_PCB_FILL_ZONE, _( "Fill Zone" ), KiBitmap( fill_zone_xpm ) );

        if( edge_zone->m_FilledPolysList.size() > 0 )
        {
            AddMenuItem( zones_menu, ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_CURRENT_ZONE,
                         _( "Remove Filled Areas in Zone" ), KiBitmap( fill_zone_xpm ) );
        }

        msg = AddHotkeyName( _( "Move Zone" ), g_Board_Editor_Hokeys_Descr, HK_MOVE_ITEM );
        AddMenuItem( zones_menu, ID_POPUP_PCB_MOVE_ZONE_OUTLINES, msg, KiBitmap( move_xpm ) );

        msg = AddHotkeyName( _( "Edit Zone Params" ), g_Board_Editor_Hokeys_Descr, HK_EDIT_ITEM );
        AddMenuItem( zones_menu, ID_POPUP_PCB_EDIT_ZONE_PARAMS,
                     msg, KiBitmap( edit_xpm ) );

        zones_menu->AppendSeparator();

        if( edge_zone->m_CornerSelection >= 0 &&
            edge_zone->m_Poly->IsCutoutContour( edge_zone->m_CornerSelection ) )
            AddMenuItem( zones_menu, ID_POPUP_PCB_DELETE_ZONE_CUTOUT,
                         _( "Delete Cutout" ), KiBitmap( delete_xpm ) );

        AddMenuItem( zones_menu, ID_POPUP_PCB_DELETE_ZONE_CONTAINER,
                     _( "Delete Zone Outline" ), KiBitmap( delete_xpm ) );
    }
}


/* Create the wxMenuitem list for footprint editing
 */
void PCB_EDIT_FRAME::createPopUpMenuForFootprints( MODULE* aModule, wxMenu* menu )
{
    wxMenu*  sub_menu_footprint;
    int      flags = aModule->m_Flags;
    wxString msg;

    sub_menu_footprint = new wxMenu;

    msg = aModule->GetSelectMenuText();
    AddMenuItem( menu, sub_menu_footprint, -1, msg, KiBitmap( module_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Move" ), g_Board_Editor_Hokeys_Descr, HK_MOVE_ITEM );
        AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_MOVE_MODULE_REQUEST,
                     msg, KiBitmap( move_module_xpm ) );
        msg = AddHotkeyName( _( "Drag" ), g_Board_Editor_Hokeys_Descr, HK_DRAG_ITEM  );
        AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_DRAG_MODULE_REQUEST,
                     msg, KiBitmap( drag_module_xpm ) );
    }

    msg = AddHotkeyName( _( "Rotate +" ), g_Board_Editor_Hokeys_Descr, HK_ROTATE_ITEM );
    AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE,
                 msg, KiBitmap( rotate_module_pos_xpm ) );
    AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE,
                 _( "Rotate -" ), KiBitmap( rotate_module_neg_xpm ) );
    msg = AddHotkeyName( _( "Flip" ), g_Board_Editor_Hokeys_Descr, HK_FLIP_FOOTPRINT );
    AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_CHANGE_SIDE_MODULE,
                 msg, KiBitmap( invert_module_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Edit" ), g_Board_Editor_Hokeys_Descr, HK_EDIT_ITEM );
        AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_EDIT_MODULE, msg, KiBitmap( edit_module_xpm ) );
        sub_menu_footprint->AppendSeparator();
        AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_DELETE_MODULE,
                     _( "Delete Module" ), KiBitmap( delete_module_xpm ) );
    }
}


/* Create the wxMenuitem list for editing texts on footprints
 */
void PCB_EDIT_FRAME::createPopUpMenuForFpTexts( TEXTE_MODULE* FpText, wxMenu* menu )
{
    wxMenu*  sub_menu_Fp_text;
    int      flags = FpText->m_Flags;

    wxString msg = FpText->GetSelectMenuText();

    sub_menu_Fp_text = new wxMenu;

    AddMenuItem( menu, sub_menu_Fp_text, -1, msg, KiBitmap( footprint_text_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Move" ), g_Board_Editor_Hokeys_Descr, HK_MOVE_ITEM );
        AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST,
                     msg, KiBitmap( move_field_xpm ) );
    }

    msg = AddHotkeyName( _( "Rotate" ), g_Board_Editor_Hokeys_Descr, HK_ROTATE_ITEM );
    AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_ROTATE_TEXTMODULE,
                 msg, KiBitmap( rotate_field_xpm ) );
    if( !flags )
    {
        msg = AddHotkeyName( _( "Edit" ), g_Board_Editor_Hokeys_Descr, HK_EDIT_ITEM );
        AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_EDIT_TEXTMODULE,
                     msg, KiBitmap( edit_text_xpm ) );
        AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_RESET_TEXT_SIZE,
                     _( "Reset Size" ), KiBitmap( reset_text_xpm ) );
    }

    // Graphic texts can be deleted only if are not currently edited.
    if( !flags && FpText->m_Type == TEXT_is_DIVERS )
    {
        AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_DELETE_TEXTMODULE,
                     _( "Delete" ), KiBitmap( delete_xpm ) );
    }

    if( !flags )
    {
        MODULE* module = (MODULE*) FpText->GetParent();

        if( module )
        {
            menu->AppendSeparator();
            createPopUpMenuForFootprints( module, menu );
        }
    }
}


/* Create pop menu for pads
 * also update Netclass selection
 */
void PCB_EDIT_FRAME::createPopUpMenuForFpPads( D_PAD* Pad, wxMenu* menu )
{
    wxMenu* sub_menu_Pad;
    int     flags = Pad->m_Flags;

    if( flags )     // Currently in edit, no others commands possible
        return;

    GetBoard()->SetCurrentNetClass( Pad->GetNetClassName() );
    updateTraceWidthSelectBox();
    updateViaSizeSelectBox();

    wxString msg = Pad->GetSelectMenuText();

    sub_menu_Pad = new wxMenu;
    AddMenuItem( menu, sub_menu_Pad, -1, msg, KiBitmap( pad_xpm ) );

    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_MOVE_PAD_REQUEST, _( "Move" ), KiBitmap( move_pad_xpm ) );
    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_DRAG_PAD_REQUEST, _( "Drag" ), KiBitmap( drag_pad_xpm ) );

    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_EDIT_PAD, _( "Edit" ), KiBitmap( options_pad_xpm ) );
    sub_menu_Pad->AppendSeparator();

    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_IMPORT_PAD_SETTINGS,
                 _( "Copy Current Settings to this Pad" ),
                 wxEmptyString,
                 KiBitmap( options_new_pad_xpm ) );
    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_EXPORT_PAD_SETTINGS,
                 _( "Copy this Pad Settings to Current Settings" ),
                 wxEmptyString,
                 KiBitmap( export_options_pad_xpm ) );

    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS,
                 _( "Global Pads Edition" ),
                 _( "Copy this pad settings to all pads in this footprint (or similar footprints)" ),
                 KiBitmap( global_options_pad_xpm ) );
    sub_menu_Pad->AppendSeparator();

    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_DELETE_PAD, _( "Delete" ), KiBitmap( delete_pad_xpm ) );

    if( m_HTOOL_current_state == ID_TOOLBARH_PCB_MODE_TRACKS )
    {
        menu->Append( ID_POPUP_PCB_AUTOROUTE_PAD, _( "Autoroute Pad" ) );
        menu->Append( ID_POPUP_PCB_AUTOROUTE_NET, _( "Autoroute Net" ) );
    }

    MODULE* module = (MODULE*) Pad->GetParent();

    if( module )
    {
        menu->AppendSeparator();
        createPopUpMenuForFootprints( module, menu );
    }
}


/* Create pop menu for pcb texts */
void PCB_EDIT_FRAME::createPopUpMenuForTexts( TEXTE_PCB* Text, wxMenu* menu )
{
    wxMenu*  sub_menu_Text;
    int      flags = Text->m_Flags;

    wxString msg = Text->GetSelectMenuText();

    sub_menu_Text = new wxMenu;

    AddMenuItem( menu, sub_menu_Text, -1, msg, KiBitmap( add_text_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Move" ), g_Board_Editor_Hokeys_Descr, HK_MOVE_ITEM );
        AddMenuItem( sub_menu_Text, ID_POPUP_PCB_MOVE_TEXTEPCB_REQUEST,
                     msg, KiBitmap( move_text_xpm ) );
    }
    msg = AddHotkeyName( _( "Rotate" ), g_Board_Editor_Hokeys_Descr, HK_ROTATE_ITEM );
    AddMenuItem( sub_menu_Text, ID_POPUP_PCB_ROTATE_TEXTEPCB, msg, KiBitmap( rotate_ccw_xpm ) );
    msg = AddHotkeyName( _( "Edit" ), g_Board_Editor_Hokeys_Descr, HK_EDIT_ITEM );
    AddMenuItem( sub_menu_Text, ID_POPUP_PCB_EDIT_TEXTEPCB, msg, KiBitmap( edit_text_xpm ) );
    AddMenuItem( sub_menu_Text, ID_POPUP_PCB_RESET_TEXT_SIZE,
                 _( "Reset Size" ), KiBitmap( reset_text_xpm ) );

    sub_menu_Text->AppendSeparator();
    AddMenuItem( sub_menu_Text, ID_POPUP_PCB_DELETE_TEXTEPCB, _( "Delete" ), KiBitmap( delete_text_xpm ) );
}


void PCB_EDIT_FRAME::createPopUpMenuForMarkers( MARKER_PCB* aMarker, wxMenu* aPopMenu )
{
    AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_MARKER, _( "Delete Marker" ), KiBitmap( delete_xpm ) );
    AddMenuItem( aPopMenu, ID_POPUP_PCB_GETINFO_MARKER, _( "Marker Error Info" ), KiBitmap( info_xpm ) );
}


/**
 * Function Append_Track_Width_List
 * creates a wxMenu * which shows the last used track widths and via diameters
 * @return a pointeur to the menu
 */
static wxMenu* Append_Track_Width_List( BOARD* aBoard )
{
    wxString msg;
    wxMenu*  trackwidth_menu;
    wxString value;

    trackwidth_menu = new wxMenu;

    trackwidth_menu->Append( ID_POPUP_PCB_SELECT_AUTO_WIDTH, _( "Auto Width" ),
                             _( "Use the track width when starting on a track, otherwise the current track width" ),
                             true );

    if( aBoard->GetBoardDesignSettings()->m_UseConnectedTrackWidth )
        trackwidth_menu->Check( ID_POPUP_PCB_SELECT_AUTO_WIDTH, true );

    if( aBoard->m_ViaSizeSelector != 0
        || aBoard->m_TrackWidthSelector != 0
        || aBoard->GetBoardDesignSettings()->m_UseConnectedTrackWidth )
        trackwidth_menu->Append( ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES,
                                 _( "Use Netclass Values" ),
                                 _( "Use track and via sizes from their Netclass values" ),
                                 true );

    for( unsigned ii = 0; ii < aBoard->m_TrackWidthList.size(); ii++ )
    {
        value = ReturnStringFromValue( g_UserUnit, aBoard->m_TrackWidthList[ii],
                                       PCB_INTERNAL_UNIT, true );
        msg.Printf( _( "Track %s" ), GetChars( value ) );

        if( ii == 0 )
            msg << _( " (use NetClass)" );

        trackwidth_menu->Append( ID_POPUP_PCB_SELECT_WIDTH1 + ii, msg, wxEmptyString, true );
    }

    trackwidth_menu->AppendSeparator();

    for( unsigned ii = 0; ii < aBoard->m_ViasDimensionsList.size(); ii++ )
    {
        value = ReturnStringFromValue( g_UserUnit, aBoard->m_ViasDimensionsList[ii].m_Diameter,
                                       PCB_INTERNAL_UNIT, true );
        wxString drill = ReturnStringFromValue( g_UserUnit,
                                                aBoard->m_ViasDimensionsList[ii].m_Drill,
                                                PCB_INTERNAL_UNIT,  true );

        if( aBoard->m_ViasDimensionsList[ii].m_Drill <= 0 )
        {
            msg.Printf( _( "Via %s" ), GetChars( value ) );
        }
        else
        {
            msg.Printf( _( "Via %s; (drl %s)" ), GetChars( value ), GetChars( drill ) );
        }

        if( ii == 0 )
            msg << _( " (use NetClass)" );

        trackwidth_menu->Append( ID_POPUP_PCB_SELECT_VIASIZE1 + ii, msg, wxEmptyString, true );
    }

    return trackwidth_menu;
}
