/**************************************************/
/* onrightclick.cpp: Right mouse button functions */
/**************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "autorout.h"
#include "id.h"
#include "hotkeys.h"
#include "collectors.h"
#include "protos.h"


/* Bitmaps */
#include "bitmaps.h"


/********************************************/
static wxMenu* Append_Track_Width_List()
/********************************************/

/* create a wxMenu * which shows the last used track widths and via diameters
 *  @return a pointeur to the menu
 */
{
    #define TRACK_HISTORY_NUMBER_MAX 6
    #define VIA_HISTORY_NUMBER_MAX   4
    int      ii;
    wxString msg;
    wxMenu*  trackwidth_menu;
    double   value;

    trackwidth_menu = new wxMenu;

    trackwidth_menu->Append( ID_POPUP_PCB_SELECT_AUTO_WIDTH,
                             _( "Auto Width" ),
                             _(
                                 "Use the track width when starting on a track, otherwise the current track width" ),
                             TRUE );

    if( g_DesignSettings.m_UseConnectedTrackWidth )
        trackwidth_menu->Check( ID_POPUP_PCB_SELECT_AUTO_WIDTH, TRUE );

    for( ii = 0; (ii < HISTORY_NUMBER) && (ii < TRACK_HISTORY_NUMBER_MAX); ii++ )
    {
        if( g_DesignSettings.m_TrackWidthHistory[ii] == 0 )
            break;
        value = To_User_Unit( g_UnitMetric,
            g_DesignSettings.m_TrackWidthHistory[ii],
            PCB_INTERNAL_UNIT );
        if( g_UnitMetric == INCHES )  // Affichage en mils
            msg.Printf( _( "Track %.1f" ), value * 1000 );
        else
            msg.Printf( _( "Track %.3f" ), value );

        trackwidth_menu->Append( ID_POPUP_PCB_SELECT_WIDTH1 + ii, msg, wxEmptyString, TRUE );

        if( (g_DesignSettings.m_TrackWidthHistory[ii] == g_DesignSettings.m_CurrentTrackWidth)
           && ! g_DesignSettings.m_UseConnectedTrackWidth )
            trackwidth_menu->Check( ID_POPUP_PCB_SELECT_WIDTH1 + ii, TRUE );
    }

    trackwidth_menu->AppendSeparator();
    for( ii = 0; (ii < HISTORY_NUMBER) && (ii < VIA_HISTORY_NUMBER_MAX); ii++ )
    {
        if( g_DesignSettings.m_ViaSizeHistory[ii] == 0 )
            break;
        value = To_User_Unit( g_UnitMetric,
            g_DesignSettings.m_ViaSizeHistory[ii],
            PCB_INTERNAL_UNIT );
        if( g_UnitMetric == INCHES )
            msg.Printf( _( "Via %.1f" ), value * 1000 );
        else
            msg.Printf( _( "Via %.3f" ), value );
        trackwidth_menu->Append( ID_POPUP_PCB_SELECT_VIASIZE1 + ii, msg, wxEmptyString, TRUE );
        if( g_DesignSettings.m_ViaSizeHistory[ii] == g_DesignSettings.m_CurrentViaSize )
            trackwidth_menu->Check( ID_POPUP_PCB_SELECT_VIASIZE1 + ii, TRUE );
    }

    return trackwidth_menu;
}


/******************************************************************************/
bool WinEDA_PcbFrame::OnRightClick( const wxPoint& aMousePos, wxMenu* aPopMenu )
/******************************************************************************/
{
    wxString    msg;
    int         flags = 0;
    bool        locate_track = FALSE;
    bool        BlockActive  = (GetScreen()->BlockLocate.m_Command != BLOCK_IDLE);

    wxClientDC  dc( DrawPanel );

    BOARD_ITEM* item = GetCurItem();

    DrawPanel->m_CanStartBlock = -1;    // Avoid to start a block coomand when clicking on menu


    // If a command or a block is in progress:
    // Put the Cancel command (if needed) and the End command

    if( BlockActive )
    {
        createPopUpBlockMenu( aPopMenu );
        aPopMenu->AppendSeparator();
        return true;
    }

    DrawPanel->CursorOff( &dc );

    if( m_ID_current_state )
    {
        if( item && item->m_Flags )
        {
            ADD_MENUITEM( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                _( "Cancel" ), cancel_xpm );
        }
        else
        {
            ADD_MENUITEM( aPopMenu, ID_POPUP_CLOSE_CURRENT_TOOL,
                _( "End Tool" ), cancel_tool_xpm );
        }
        aPopMenu->AppendSeparator();
    }
    else
    {
        if( item && item->m_Flags )
        {
            ADD_MENUITEM( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                _( "Cancel" ), cancel_xpm );
            aPopMenu->AppendSeparator();
        }
    }


    /* Select a proper item */

    wxPoint cursorPos = GetScreen()->m_Curseur;
    wxPoint selectPos = m_Collector->GetRefPos();

    PutOnGrid( &selectPos );

    // printf( "cursor=(%d, %d) select=(%d,%d)\n", cursorPos.x, cursorPos.y, selectPos.x, selectPos.y );

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
                DrawPanel->CursorOn( &dc );
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

            if( m_HTOOL_current_state == ID_TOOLBARH_PCB_AUTOPLACE )
            {
                aPopMenu->AppendSeparator();

                if( !( (MODULE*) item )->IsLocked() )
                {
                    msg = AddHotkeyName( _(
                            "Lock Module" ), s_Board_Editor_Hokeys_Descr,
                        HK_LOCK_UNLOCK_FOOTPRINT );
                    ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_AUTOPLACE_FIXE_MODULE, msg,
                        locked_xpm );
                }
                else
                {
                    msg = AddHotkeyName( _(
                            "Unlock Module" ), s_Board_Editor_Hokeys_Descr,
                        HK_LOCK_UNLOCK_FOOTPRINT );
                    ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_AUTOPLACE_FREE_MODULE, msg,
                        unlocked_xpm );
                }

                if( !flags )
                    aPopMenu->Append( ID_POPUP_PCB_AUTOPLACE_CURRENT_MODULE,
                        _( "Auto Place Module" ) );
            }

            if( m_HTOOL_current_state == ID_TOOLBARH_PCB_AUTOROUTE )
            {
                if( !flags )
                    aPopMenu->Append( ID_POPUP_PCB_AUTOROUTE_MODULE, _( "Autoroute" ) );
            }
            break;

        case TYPE_PAD:
            createPopUpMenuForFpPads( (D_PAD*) item, aPopMenu );
            break;

        case TYPE_TEXTE_MODULE:
            createPopUpMenuForFpTexts( (TEXTE_MODULE*) item, aPopMenu );
            break;

        case TYPE_DRAWSEGMENT:
            if( !flags )
            {
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_MOVE_DRAWING_REQUEST,
                    _( "Move Drawing" ), move_xpm );
            }
            if( flags & IS_NEW )
            {
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_STOP_CURRENT_DRAWING,
                    _( "End Drawing" ), apply_xpm );
            }
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_EDIT_DRAWING,
                _( "Edit Drawing" ), edit_xpm );
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_DRAWING,
                _( "Delete Drawing" ), delete_xpm );
            break;

        case TYPE_ZONE:      // Item used to fill a zone
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_ZONE,
                _( "Delete Zone Filling" ), delete_xpm );
            break;

        case TYPE_ZONE_CONTAINER:    // Item used to handle a zone area (outlines, holes ...)
            if( flags & IS_NEW )
            {
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_STOP_CURRENT_EDGE_ZONE,
                              _( "Close Zone Outline" ), apply_xpm );
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_ZONE_LAST_CREATED_CORNER,
                              _( "Delete Last Corner" ), delete_xpm );
            }
            else
            createPopUpMenuForZones( (ZONE_CONTAINER*) item, aPopMenu );
            break;

        case TYPE_TEXTE:
                createPopUpMenuForTexts( (TEXTE_PCB*) item, aPopMenu );
            break;

        case TYPE_TRACK:
        case TYPE_VIA:
            locate_track = TRUE;
            createPopupMenuForTracks( (TRACK*) item, aPopMenu );
            break;

        case TYPE_MARKER:
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_MARKER,
                _( "Delete Marker" ), delete_xpm );
            break;

        case TYPE_COTATION:
            if( !flags )
            {
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_EDIT_COTATION,
                    _( "Edit Dimension" ), edit_xpm );
            }
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_COTATION,
                _( "Delete Dimension" ), delete_xpm );
            break;

        case TYPE_MIRE:
            if( !flags )
            {
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_MOVE_MIRE_REQUEST,
                    _( "Move Target" ), move_xpm );
            }
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_EDIT_MIRE,
                _( "Edit Target" ), edit_xpm );
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_MIRE,
                _( "Delete Target" ), delete_xpm );
            break;

        case TYPE_EDGE_MODULE:
        case TYPE_SCREEN:
        case TYPE_NOT_INIT:
        case TYPE_PCB:
        case TYPE_EQUIPOT:
            msg.Printf(
                wxT( "WinEDA_PcbFrame::OnRightClick() Error: illegal DrawType %d" ),
                item->Type() );
            DisplayError( this, msg );
            SetCurItem( NULL );
            break;

        default:
            msg.Printf(
                wxT( "WinEDA_PcbFrame::OnRightClick() Error: unknown DrawType %d" ),
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
            s_Board_Editor_Hokeys_Descr, HK_GET_AND_MOVE_FOOTPRINT );
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST,
            msg, move_module_xpm );
    }

    /* Traitement des fonctions specifiques */
    switch(  m_ID_current_state )
    {
    case ID_PCB_ZONES_BUTT:
        if(  GetBoard()->m_ZoneDescriptorList.size() > 0 )
        {
            aPopMenu->AppendSeparator();
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_FILL_ALL_ZONES,
                _( "Fill or Refill All Zones" ), fill_zone_xpm );
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_ALL_ZONES,
                _( "Remove Filled Areas in All Zones" ), fill_zone_xpm );
            aPopMenu->AppendSeparator();
        }

        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_SELECT_LAYER,
            _( "Select Working Layer" ), select_w_layer_xpm );
        aPopMenu->AppendSeparator();
        break;

    case ID_TRACK_BUTT:
        ADD_MENUITEM_WITH_SUBMENU( aPopMenu, Append_Track_Width_List(),
            ID_POPUP_PCB_SELECT_WIDTH,
            _( "Select Track Width" ), width_track_xpm );
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_SELECT_CU_LAYER,
            _( "Select Working Layer" ), select_w_layer_xpm );
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_SELECT_LAYER_PAIR,
            _( "Select Layer Pair for Vias" ), select_layer_pair_xpm );
        aPopMenu->AppendSeparator();
        break;

    case ID_PCB_CIRCLE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_TEXT_COMMENT_BUTT:
    case ID_LINE_COMMENT_BUTT:
    case ID_PCB_COTATION_BUTT:
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_SELECT_NO_CU_LAYER,
            _( "Select Working Layer" ), select_w_layer_xpm );
        aPopMenu->AppendSeparator();
        break;

    case ID_COMPONENT_BUTT:
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DISPLAY_FOOTPRINT_DOC,
            _( "Footprint Documentation" ), book_xpm );
        aPopMenu->AppendSeparator();
        break;

    case 0:
        if( m_HTOOL_current_state == ID_TOOLBARH_PCB_AUTOPLACE )
        {
            wxMenu* commands = new wxMenu;
            ADD_MENUITEM_WITH_SUBMENU( aPopMenu, commands,
                ID_POPUP_PCB_AUTOPLACE_COMMANDS, _(
                    "Glob Move and Place" ), move_xpm );
            ADD_MENUITEM( commands, ID_POPUP_PCB_AUTOPLACE_FREE_ALL_MODULES,
                _( "Unlock All Modules" ), unlocked_xpm );
            ADD_MENUITEM( commands, ID_POPUP_PCB_AUTOPLACE_FIXE_ALL_MODULES,
                _( "Lock All Modules" ), locked_xpm );
            commands->AppendSeparator();
            ADD_MENUITEM( commands, ID_POPUP_PCB_AUTOMOVE_ALL_MODULES,
                _( "Move All Modules" ), move_xpm );
            commands->Append( ID_POPUP_PCB_AUTOMOVE_NEW_MODULES, _( "Move New Modules" ) );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOPLACE_ALL_MODULES, _( "Autoplace All Modules" ) );
            commands->Append( ID_POPUP_PCB_AUTOPLACE_NEW_MODULES, _( "Autoplace New Modules" ) );
            commands->Append( ID_POPUP_PCB_AUTOPLACE_NEXT_MODULE, _( "Autoplace Next Module" ) );
            commands->AppendSeparator();
            ADD_MENUITEM( commands, ID_POPUP_PCB_REORIENT_ALL_MODULES,
                _( "Orient All Modules" ), rotate_module_pos_xpm );
            aPopMenu->AppendSeparator();
        }

        if( m_HTOOL_current_state == ID_TOOLBARH_PCB_AUTOROUTE )
        {
            wxMenu* commands = new wxMenu;
            aPopMenu->Append( ID_POPUP_PCB_AUTOROUTE_COMMANDS, _( "Global Autoroute" ), commands );
            ADD_MENUITEM( commands, ID_POPUP_PCB_SELECT_LAYER_PAIR,
                _( "Select Layer Pair" ), select_layer_pair_xpm );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOROUTE_ALL_MODULES, _( "Autoroute All Modules" ) );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOROUTE_RESET_UNROUTED, _( "Reset Unrouted" ) );
            if( GetBoard()->m_Modules )
            {
                commands->AppendSeparator();
                commands->Append( ID_POPUP_PCB_AUTOROUTE_GET_AUTOROUTER,
                    _( "Global AutoRouter" ) );
                commands->Append( ID_POPUP_PCB_AUTOROUTE_GET_AUTOROUTER_DATA,
                    _( "Read Global AutoRouter Data" ) );
            }
            aPopMenu->AppendSeparator();
        }

        if( locate_track )
            ADD_MENUITEM_WITH_SUBMENU( aPopMenu, Append_Track_Width_List(),
                ID_POPUP_PCB_SELECT_WIDTH, _( "Select Track Width" ),
                width_track_xpm );
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_SELECT_LAYER,
            _( "Select Working Layer" ), select_w_layer_xpm );
        aPopMenu->AppendSeparator();
        break;
    }

    DrawPanel->CursorOn( &dc );
    return true;
}


/*********************************************************/
void WinEDA_PcbFrame::createPopUpBlockMenu( wxMenu* menu )
/*********************************************************/

/* Create Pop sub menu for block commands
 */
{
    ADD_MENUITEM( menu, ID_POPUP_CANCEL_CURRENT_COMMAND,
        _( "Cancel Block" ), cancel_xpm );
    ADD_MENUITEM( menu, ID_POPUP_ZOOM_BLOCK,
        _( "Zoom Block" ), zoom_selected_xpm );
    menu->AppendSeparator();
    ADD_MENUITEM( menu, ID_POPUP_PLACE_BLOCK,
        _( "Place Block" ), apply_xpm );
    ADD_MENUITEM( menu, ID_POPUP_COPY_BLOCK,
        _( "Copy Block" ), copyblock_xpm );
    ADD_MENUITEM( menu, ID_POPUP_INVERT_BLOCK,
        _( "Flip Block" ), invert_module_xpm );
    ADD_MENUITEM( menu, ID_POPUP_ROTATE_BLOCK,
        _( "Rotate Block" ), rotate_pos_xpm );
    ADD_MENUITEM( menu, ID_POPUP_DELETE_BLOCK,
        _( "Delete Block" ), delete_xpm );
}


/******************************************************************************/
void WinEDA_PcbFrame::createPopupMenuForTracks( TRACK* Track, wxMenu* PopMenu )
/******************************************************************************/

/* Create command lines for a popup menu, for track editing
 */
{
    wxPoint  cursorPosition = GetScreen()->m_Curseur;
    wxString msg;
    int      flags = Track->m_Flags;

    if( flags == 0 )
    {
        if( Track->Type() == TYPE_VIA )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_TRACK_NODE, _( "Drag Via" ), move_xpm );
            wxMenu* via_mnu = new wxMenu();

            ADD_MENUITEM_WITH_SUBMENU( PopMenu, via_mnu,
                ID_POPUP_PCB_VIA_EDITING, _( "Edit Via Drill" ), edit_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_TO_DEFAULT,
                          _( "Set Via Hole to Default" ), apply_xpm );
            msg = _( "Set via hole to a specific value. This specific value is currently" );
            msg << wxT(" ") << ReturnStringFromValue( g_UnitMetric, g_DesignSettings.m_ViaDrillCustomValue, m_InternalUnits );
            ADD_MENUITEM_WITH_HELP( via_mnu, ID_POPUP_PCB_VIA_HOLE_TO_VALUE,
                                _( "Set Via Hole to Alt Value" ), msg,
                                options_new_pad_xpm );
            msg = _( "Set a specific via hole value. This value is currently" );
            msg  << wxT(" ") << ReturnStringFromValue( g_UnitMetric, g_DesignSettings.m_ViaDrillCustomValue, m_InternalUnits );
            ADD_MENUITEM_WITH_HELP( via_mnu, ID_POPUP_PCB_VIA_HOLE_ENTER_VALUE,
                _( "Set the Via Hole Alt Value" ), msg, edit_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_EXPORT, _(
                    "Export Via Hole to Alt Value" ), export_options_pad_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_EXPORT_TO_OTHERS,
                _( "Export Via Hole to Others id Vias" ), global_options_pad_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_RESET_TO_DEFAULT,
                _( "Set ALL Via Holes to Default" ), apply_xpm );
            if( Track->IsDrillDefault() )   // Can't export the drill value, because this value is 0
            {
                via_mnu->Enable( ID_POPUP_PCB_VIA_HOLE_EXPORT, FALSE );
            }
            if( g_DesignSettings.m_ViaDrillCustomValue <= 0 )
                via_mnu->Enable( ID_POPUP_PCB_VIA_HOLE_TO_VALUE, FALSE );
        }
        else
        {
            if( Track->IsPointOnEnds( cursorPosition, -1 ) != 0 )
            {
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_TRACK_NODE,
                    _( "Move Node" ), move_xpm );
            }
            else
            {
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_DRAG_TRACK_SEGMENT_KEEP_SLOPE,
                    _( "Drag Segments, Keep Slope" ), drag_segment_withslope_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_DRAG_TRACK_SEGMENT,
                    _( "Drag Segment" ), drag_track_segment_xpm );
#if 0
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_TRACK_SEGMENT,
                    _( "Move Segment" ), move_track_segment_xpm );
#endif
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_BREAK_TRACK,
                    _( "Break Track" ), break_line_xpm );
            }
        }
    }
    else if( flags & IS_DRAGGED )   // Drag via or node in progress
    {
        ADD_MENUITEM( PopMenu, ID_POPUP_PCB_PLACE_MOVED_TRACK_NODE,
            _( "Place Node" ), apply_xpm );
        return;
    }
    else // Edition in progress
    {
        if( flags & IS_NEW )
        {
            msg = AddHotkeyName( _( "End Track" ), s_Board_Editor_Hokeys_Descr, HK_END_TRACK );
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_END_TRACK, msg, apply_xpm );
        }
        msg = AddHotkeyName( _( "Place Via" ), s_Board_Editor_Hokeys_Descr, HK_ADD_VIA );
        PopMenu->Append( ID_POPUP_PCB_PLACE_VIA, msg );

        // See if we can place a Micro Via (4 or more layers, and start from an external layer):
        if( ( (PCB_SCREEN*) GetScreen() )->IsMicroViaAcceptable() )
        {
            msg = AddHotkeyName( _(
                    "Place Micro Via" ), s_Board_Editor_Hokeys_Descr,
                HK_ADD_MICROVIA );
            PopMenu->Append( ID_POPUP_PCB_PLACE_MICROVIA, msg );
        }
    }

    // track Width control :
    wxMenu* track_mnu;
    if( !flags )    // track Width control :
    {
        track_mnu = new wxMenu;
        ADD_MENUITEM_WITH_SUBMENU( PopMenu, track_mnu,
                                   ID_POPUP_PCB_EDIT_TRACK_MNU, _( "Change Width" ), width_track_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_TRACKSEG,
                 Track->Type()==TYPE_VIA ? _( "Change Via Size" ) : _( "Change Segment Width" ), width_segment_xpm );

        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_TRACK,
            _( "Change Track Width" ), width_track_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_NET,
            _( "Change Net" ), width_net_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_ALL_VIAS_AND_TRACK_SIZE,
            _( "Change ALL Tracks and Vias" ), width_track_via_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_ALL_VIAS_SIZE,
            _( "Change ALL Vias (No Track)" ), width_vias_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_ALL_TRACK_SIZE,
            _( "Change ALL Tracks (No Via)" ), width_track_xpm );
    }

    // Delete control:
    track_mnu = new wxMenu;
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, track_mnu,
        ID_POPUP_PCB_DELETE_TRACK_MNU, _( "Delete" ), delete_xpm );

    msg = AddHotkeyName( Track->Type()==TYPE_VIA ? _( "Delete Via" ) : _( "Delete Segment" ),
        s_Board_Editor_Hokeys_Descr, HK_BACK_SPACE );

    ADD_MENUITEM( track_mnu, ID_POPUP_PCB_DELETE_TRACKSEG,
        msg, delete_line_xpm );
    if( !flags )
    {
        msg = AddHotkeyName( _( "Delete Track" ), s_Board_Editor_Hokeys_Descr, HK_DELETE );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_DELETE_TRACK,
            msg, delete_track_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_DELETE_TRACKNET,
            _( "Delete Net" ), delete_net_xpm );
    }
    track_mnu = new wxMenu;

    ADD_MENUITEM_WITH_SUBMENU( PopMenu, track_mnu,
        ID_POPUP_PCB_SETFLAGS_TRACK_MNU, _( "Set Flags" ), flag_xpm );
    track_mnu->Append( ID_POPUP_PCB_LOCK_ON_TRACKSEG, _( "Locked: Yes" ), wxEmptyString, TRUE );
    track_mnu->Append( ID_POPUP_PCB_LOCK_OFF_TRACKSEG, _( "Locked: No" ), wxEmptyString, TRUE );

    if( Track->GetState( SEGM_FIXE ) )
        track_mnu->Check( ID_POPUP_PCB_LOCK_ON_TRACKSEG, TRUE );
    else
        track_mnu->Check( ID_POPUP_PCB_LOCK_OFF_TRACKSEG, TRUE );

    if( !flags )
    {
        track_mnu->AppendSeparator();
        track_mnu->Append( ID_POPUP_PCB_LOCK_ON_TRACK, _( "Track Locked: Yes" ) );
        track_mnu->Append( ID_POPUP_PCB_LOCK_OFF_TRACK, _( "Track Locked: No" ) );
        track_mnu->AppendSeparator();
        track_mnu->Append( ID_POPUP_PCB_LOCK_ON_NET, _( "Net Locked: Yes" ) );
        track_mnu->Append( ID_POPUP_PCB_LOCK_OFF_NET, _( "Net Locked: No" ) );
    }
}


/********************************************************************************************/
void WinEDA_PcbFrame::createPopUpMenuForZones( ZONE_CONTAINER* edge_zone, wxMenu* aPopMenu )
/********************************************************************************************/

/* Create the wxMenuitem list for zone outlines editing and zone filling
 */
{
    if( edge_zone->m_Flags == IS_DRAGGED )
    {
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_PLACE_DRAGGED_ZONE_OUTLINE_SEGMENT,
            _( "Place Edge Outline" ), apply_xpm );
    }
    else if( edge_zone->m_Flags )
    {
        if( (edge_zone->m_Flags & IN_EDIT ) )
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_PLACE_ZONE_CORNER,
                _( "Place Corner" ), apply_xpm );
        else
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_PLACE_ZONE_OUTLINES,
                _( "Place Zone" ), apply_xpm );
    }
    else
    {
        wxMenu* zones_menu = new wxMenu();

        ADD_MENUITEM_WITH_SUBMENU( aPopMenu, zones_menu,
            -1, _( "Zones" ), add_zone_xpm );
        int index;
        if( ( index = edge_zone->HitTestForCorner( GetScreen()->RefPos( true ) ) ) >= 0 )
        {
            ADD_MENUITEM( zones_menu, ID_POPUP_PCB_MOVE_ZONE_CORNER,
                _( "Move Corner" ), move_xpm );
            ADD_MENUITEM( zones_menu, ID_POPUP_PCB_DELETE_ZONE_CORNER,
                _( "Delete Corner" ), delete_xpm );
        }
        else if( ( index = edge_zone->HitTestForEdge( GetScreen()->RefPos( true ) ) ) >= 0 )
        {
            ADD_MENUITEM( zones_menu, ID_POPUP_PCB_ADD_ZONE_CORNER,
                _( "Create Corner" ), add_corner_xpm );
            ADD_MENUITEM( zones_menu, ID_POPUP_PCB_DRAG_ZONE_OUTLINE_SEGMENT,
                _( "Drag Outline Segment" ), drag_outline_segment_xpm );
        }

        zones_menu->AppendSeparator();
        ADD_MENUITEM( zones_menu, ID_POPUP_PCB_ZONE_ADD_SIMILAR_ZONE,
            _( "Add Similar Zone" ), add_zone_xpm );

        ADD_MENUITEM( zones_menu, ID_POPUP_PCB_ZONE_ADD_CUTOUT_ZONE,
            _( "Add Cutout Area" ), add_zone_cutout );
        zones_menu->AppendSeparator();

        ADD_MENUITEM( zones_menu, ID_POPUP_PCB_FILL_ZONE,
            _( "Fill Zone" ), fill_zone_xpm );

        if (edge_zone->m_FilledPolysList.size() > 0 )
        {
            ADD_MENUITEM( zones_menu, ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_CURRENT_ZONE,
                _( "Remove Filled Areas in Zone" ), fill_zone_xpm );
        }

        ADD_MENUITEM( zones_menu, ID_POPUP_PCB_MOVE_ZONE_OUTLINES,
            _( "Move Zone" ), move_xpm );

        ADD_MENUITEM( zones_menu, ID_POPUP_PCB_EDIT_ZONE_PARAMS,
            _( "Edit Zone Params" ), edit_xpm );

        zones_menu->AppendSeparator();
        if( index >= 0 && edge_zone->m_Poly->IsCutoutContour( edge_zone->m_CornerSelection ) )
            ADD_MENUITEM( zones_menu, ID_POPUP_PCB_DELETE_ZONE_CUTOUT,
                _( "Delete Cutout" ), delete_xpm );

        ADD_MENUITEM( zones_menu, ID_POPUP_PCB_DELETE_ZONE_CONTAINER,
            _( "Delete Zone Outline" ), delete_xpm );
    }
}


/*********************************************************************************/
void WinEDA_PcbFrame::createPopUpMenuForFootprints( MODULE* aModule, wxMenu* menu )
/*********************************************************************************/

/* Create the wxMenuitem list for footprint editing
 */
{
    wxMenu*  sub_menu_footprint;
    int      flags = aModule->m_Flags;
    wxString msg;

    sub_menu_footprint = new wxMenu;

    msg = aModule->MenuText( GetBoard() );
    ADD_MENUITEM_WITH_SUBMENU( menu, sub_menu_footprint, -1, msg, module_xpm );
    if( !flags )
    {
        msg = AddHotkeyName( _( "Move" ), s_Board_Editor_Hokeys_Descr, HK_MOVE_FOOTPRINT );
        ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_MOVE_MODULE_REQUEST,
            msg, move_module_xpm );
        msg = AddHotkeyName( _( "Drag" ), s_Board_Editor_Hokeys_Descr, HK_DRAG_FOOTPRINT );
        ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_DRAG_MODULE_REQUEST,
            msg, drag_module_xpm );
    }
    msg = AddHotkeyName( _( "Rotate  +" ), s_Board_Editor_Hokeys_Descr, HK_ROTATE_FOOTPRINT );
    ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE,
        msg, rotate_module_pos_xpm );
    ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE,
        _( "Rotate -" ), rotate_module_neg_xpm );
    msg = AddHotkeyName( _( "Flip" ), s_Board_Editor_Hokeys_Descr, HK_FLIP_FOOTPRINT );
    ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_CHANGE_SIDE_MODULE,
        msg, invert_module_xpm );
    ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_EDIT_MODULE,
        _( "Edit" ), edit_module_xpm );

    if( !flags )
    {
        sub_menu_footprint->AppendSeparator();
        ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_DELETE_MODULE,
            _( "Delete Module" ), delete_module_xpm );
    }
}


/********************************************************************/
void WinEDA_PcbFrame::createPopUpMenuForFpTexts( TEXTE_MODULE* FpText, wxMenu* menu )
/********************************************************************/

/* Create the wxMenuitem list for editing texts on footprints
 */
{
    wxMenu*  sub_menu_Fp_text;
    int      flags = FpText->m_Flags;

    wxString msg = FpText->MenuText( GetBoard() );

    sub_menu_Fp_text = new wxMenu;

    ADD_MENUITEM_WITH_SUBMENU( menu, sub_menu_Fp_text, -1, msg, footprint_text_xpm );

    if( !flags )
        ADD_MENUITEM( sub_menu_Fp_text, ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST,
            _( "Move" ), move_field_xpm );

    ADD_MENUITEM( sub_menu_Fp_text, ID_POPUP_PCB_ROTATE_TEXTMODULE,
        _( "Rotate" ), rotate_field_xpm );
    ADD_MENUITEM( sub_menu_Fp_text, ID_POPUP_PCB_EDIT_TEXTMODULE,
        _( "Edit" ), edit_text_xpm );

    if( FpText->m_Type == TEXT_is_DIVERS )
        ADD_MENUITEM( sub_menu_Fp_text, ID_POPUP_PCB_DELETE_TEXTMODULE,
            _( "Delete" ), delete_xpm );

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


/************************************************************************/
void WinEDA_PcbFrame::createPopUpMenuForFpPads( D_PAD* Pad, wxMenu* menu )
/************************************************************************/
/* Create pop menu for pads */
{
    wxMenu*  sub_menu_Pad;
    int      flags = Pad->m_Flags;

    wxString msg = Pad->MenuText( GetBoard() );

    sub_menu_Pad = new wxMenu;
    ADD_MENUITEM_WITH_SUBMENU( menu, sub_menu_Pad, -1, msg, pad_xpm );
    if( !flags )
    {
        ADD_MENUITEM( sub_menu_Pad, ID_POPUP_PCB_MOVE_PAD_REQUEST,
            _( "Move" ), move_pad_xpm );
        ADD_MENUITEM( sub_menu_Pad, ID_POPUP_PCB_DRAG_PAD_REQUEST,
            _( "Drag" ), drag_pad_xpm );
    }
    ADD_MENUITEM( sub_menu_Pad, ID_POPUP_PCB_EDIT_PAD, _( "Edit Pad" ), options_pad_xpm );
    sub_menu_Pad->AppendSeparator();

    ADD_MENUITEM_WITH_HELP( sub_menu_Pad, ID_POPUP_PCB_IMPORT_PAD_SETTINGS,
        _( "New Pad Settings" ),
        _( "Copy current pad settings to this pad" ),
        options_new_pad_xpm );
    ADD_MENUITEM_WITH_HELP( sub_menu_Pad, ID_POPUP_PCB_EXPORT_PAD_SETTINGS,
        _( "Export Pad Settings" ),
        _( "Copy this pad settings to current pad settings" ),
        export_options_pad_xpm );

    if( !flags )
    {
        ADD_MENUITEM_WITH_HELP( sub_menu_Pad, ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS,
            _( "Global Pad Settings" ),
            _(
                "Copy this pad settings to all pads in this footprint (or similar footprints)" ),
            global_options_pad_xpm );
        sub_menu_Pad->AppendSeparator();

        ADD_MENUITEM( sub_menu_Pad, ID_POPUP_PCB_DELETE_PAD,
            _( "Delete" ), delete_pad_xpm );
    }

    if( m_HTOOL_current_state == ID_TOOLBARH_PCB_AUTOROUTE )
    {
        if( !flags )
        {
            menu->Append( ID_POPUP_PCB_AUTOROUTE_PAD, _( "Autoroute Pad" ) );
            menu->Append( ID_POPUP_PCB_AUTOROUTE_NET, _( "Autoroute Net" ) );
        }
    }
    if( !flags )
    {
        MODULE* module = (MODULE*) Pad->GetParent();
        if( module )
        {
            menu->AppendSeparator();
            createPopUpMenuForFootprints( module, menu );
        }
    }
}


/*****************************************************************************/
void WinEDA_PcbFrame::createPopUpMenuForTexts( TEXTE_PCB* Text, wxMenu* menu )
/*****************************************************************************/
/* Create pop menu for pcb texts */
{
    wxMenu*  sub_menu_Text;
    int      flags = Text->m_Flags;

    wxString msg = Text->MenuText( GetBoard() );

    sub_menu_Text = new wxMenu;

    ADD_MENUITEM_WITH_SUBMENU( menu, sub_menu_Text, -1, msg, add_text_xpm );

    if( !flags )
    {
        ADD_MENUITEM( sub_menu_Text, ID_POPUP_PCB_MOVE_TEXTEPCB_REQUEST,
            _( "Move" ), move_text_xpm );
    }
    ADD_MENUITEM( sub_menu_Text, ID_POPUP_PCB_ROTATE_TEXTEPCB,
        _( "Rotate" ), rotate_pos_xpm );
    ADD_MENUITEM( sub_menu_Text, ID_POPUP_PCB_EDIT_TEXTEPCB,
        _( "Edit" ), edit_text_xpm );

    sub_menu_Text->AppendSeparator();
    ADD_MENUITEM( sub_menu_Text, ID_POPUP_PCB_DELETE_TEXTEPCB,
        _( "Delete" ), delete_text_xpm );
}
