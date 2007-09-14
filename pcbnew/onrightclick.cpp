/********************************************************************************/
/* onrightclick.cpp: fonctions de l'edition du PCB appel�es par le bouton droit */
/********************************************************************************/

#include "fctsys.h"

#include "gr_basic.h"
#include "macros.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "id.h"

#include "protos.h"

/* Bitmaps */
#include "bitmaps.h"


/* Specific bitmaps */
#include "Break_Line.xpm"
#include "Delete_Net.xpm"
#include "Delete_Line.xpm"
#include "Delete_Track.xpm"
#include "Move_Module.xpm"

//#include "Move_Track_Segment.xpm"
#include "Drag_Track_Segment.xpm"
#include "Drag_Segment_WithSlope.xpm"
#include "Drag_Module.xpm"
#include "Edit_Module.xpm"
#include "Rotate_Module+.xpm"
#include "Rotate_Module-.xpm"
#include "Delete_Module.xpm"
#include "Invert_Module.xpm"
#include "Locked.xpm"
#include "Unlocked.xpm"
#include "Move_Field.xpm"
#include "Rotate_Field.xpm"
#include "Move_Pad.xpm"
#include "Drag_Pad.xpm"
#include "Global_Options_Pad.xpm"
#include "Delete_Pad.xpm"
#include "Export_Options_Pad.xpm"
#include "Select_W_Layer.xpm"
#include "Width_Track.xpm"
#include "Width_Vias.xpm"
#include "Width_Segment.xpm"
#include "Width_Net.xpm"
#include "Width_Track_Via.xpm"
#include "Select_Layer_Pair.xpm"

#include "Flag.xpm"


/* local functions */
static void CreatePopupMenuForTracks( TRACK* Track, wxPoint CursorPosition,
                                      wxMenu* PopMenu );
static void CreatePopUpMenuForFootprints( MODULE* Footprint, wxMenu* menu, bool full_menu );
static void CreatePopUpMenuForFpTexts( TEXTE_MODULE* FpText, wxMenu* menu );
static void CreatePopUpMenuForPads( D_PAD* Pad, wxMenu* menu );
static void CreatePopUpMenuForTexts( TEXTE_PCB* Text, wxMenu* menu );
static void CreatePopUpBlockMenu( wxMenu* menu );


/*****/


/********************************************/
static wxMenu* Append_Track_Width_List()
/********************************************/

/* Ajoute au menu wxMenu * menu un sous-menu liste des epaisseurs de pistes
 *  disponibles
 */
{
    int      ii;
    wxString msg;
    wxMenu*  trackwidth;
    double   value;

    trackwidth = new wxMenu;
    for( ii = 0; ii < 6; ii++ )
    {
        if( g_DesignSettings.m_TrackWidhtHistory[ii] == 0 )
            break;
        value = To_User_Unit( g_UnitMetric,
                              g_DesignSettings.m_TrackWidhtHistory[ii],
                              PCB_INTERNAL_UNIT );
        if( g_UnitMetric == INCHES )            // Affichage en mils
            msg.Printf( _( "Track %.1f" ), value * 1000 );
        else
            msg.Printf( _( "Track %.3f" ), value );

        trackwidth->Append( ID_POPUP_PCB_SELECT_WIDTH1 + ii, msg, wxEmptyString, TRUE );
        if( g_DesignSettings.m_TrackWidhtHistory[ii] == g_DesignSettings.m_CurrentTrackWidth )
            trackwidth->Check( ID_POPUP_PCB_SELECT_WIDTH1 + ii, TRUE );
    }

    trackwidth->AppendSeparator();
    for( ii = 0; ii < 4; ii++ )
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
        trackwidth->Append( ID_POPUP_PCB_SELECT_VIASIZE1 + ii, msg, wxEmptyString, TRUE );
        if( g_DesignSettings.m_ViaSizeHistory[ii] == g_DesignSettings.m_CurrentViaSize )
            trackwidth->Check( ID_POPUP_PCB_SELECT_VIASIZE1 + ii, TRUE );
    }

    return trackwidth;
}


/******************************************************************************/
void WinEDA_PcbFrame::OnRightClick( const wxPoint& aMousePos, wxMenu* aPopMenu )
/******************************************************************************/
{
    wxString        msg;
    int             flags = 0;
    bool            locate_track = FALSE;
    bool            BlockActive  = (m_CurrentScreen->BlockLocate.m_Command != BLOCK_IDLE);
    wxClientDC      dc( DrawPanel );

    BOARD_ITEM*     item = GetCurItem();
    
    DrawPanel->CursorOff( &dc );
    DrawPanel->m_CanStartBlock = -1;    // Ne pas engager un debut de bloc sur validation menu

    
    /*  The user must now left click to first make the selection.  OnRightClick()
        is now only an action mechanism, not a selection mechanism.  The selection
        mechanism sometimes involves a popup menu, so it is too complex to try
        and do that here.
        
    // Only offer user a new selection if there is currently none.
    if( item == NULL )
    {
        item = PcbGeneralLocateAndDisplay();
    }
    */

    // If command in progress: Put the Cancel command (if needed) and End command
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
        if( (item && item->m_Flags) || BlockActive )
        {
            if( BlockActive )
                CreatePopUpBlockMenu( aPopMenu );
            else
            {
                ADD_MENUITEM( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                              _( "Cancel" ), cancel_xpm );
            }
            aPopMenu->AppendSeparator();
        }
    }

    if( BlockActive )
    {
        DrawPanel->CursorOn( &dc ); 
        return;
    }

    if( item )
        flags = item->m_Flags;
    else
        flags = 0;
    
    if( item )
    {
        switch( item->Type() )
        {
        case TYPEMODULE:
            if( !flags )
                CreatePopUpMenuForFootprints( (MODULE*) item, aPopMenu, TRUE );
            else
                CreatePopUpMenuForFootprints( (MODULE*) item, aPopMenu, FALSE );

            if( m_HTOOL_current_state == ID_TOOLBARH_PCB_AUTOPLACE )
            {
                aPopMenu->AppendSeparator();
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_AUTOPLACE_FIXE_MODULE, _( "Lock Module" ),
                              Locked_xpm );
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_AUTOPLACE_FREE_MODULE, _( "Unlock Module" ),
                              Unlocked_xpm );
                if( !flags )
                    aPopMenu->Append( ID_POPUP_PCB_AUTOPLACE_CURRENT_MODULE,
                                    _( "Auto place Module" ) );
            }

            if( m_HTOOL_current_state == ID_TOOLBARH_PCB_AUTOROUTE )
            {
                if( !flags )
                    aPopMenu->Append( ID_POPUP_PCB_AUTOROUTE_MODULE, _( "Autoroute" ) );
            }
            break;

        case TYPEPAD:
            CreatePopUpMenuForPads( (D_PAD*) item, aPopMenu );
            if( m_HTOOL_current_state == ID_TOOLBARH_PCB_AUTOROUTE )
            {
                if( !flags )
                {
                    aPopMenu->Append( ID_POPUP_PCB_AUTOROUTE_PAD, _( "Autoroute Pad" ) );
                    aPopMenu->Append( ID_POPUP_PCB_AUTOROUTE_NET, _( "Autoroute Net" ) );
                }
            }
            if( !flags )
            {
                MODULE* module = (MODULE*) item->m_Parent;
                if( module )
                {
                    aPopMenu->AppendSeparator();
                    CreatePopUpMenuForFootprints( module, aPopMenu, TRUE );
                }
            }
            break;

        case TYPETEXTEMODULE:
            CreatePopUpMenuForFpTexts( (TEXTE_MODULE*) item, aPopMenu );
            if( !flags )
            {
                MODULE* module = (MODULE*) item->m_Parent;
                if( module )
                {
                    aPopMenu->AppendSeparator();
                    CreatePopUpMenuForFootprints( module, aPopMenu, TRUE );
                }
            }
            break;

        case TYPEDRAWSEGMENT:
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
            aPopMenu->Append( ID_POPUP_PCB_EDIT_DRAWING, _( "Edit Drawing" ) );
            aPopMenu->Append( ID_POPUP_PCB_DELETE_DRAWING, _( "Delete Drawing" ) );
            break;

        case TYPEEDGEZONE:
            if( flags & IS_NEW )
            {
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_STOP_CURRENT_EDGE_ZONE,
                              _( "End edge zone" ), apply_xpm );
            }
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_EDGE_ZONE,
                          _( "Delete edge zone" ), delete_xpm );
            break;

        case TYPETEXTE:
            CreatePopUpMenuForTexts( (TEXTE_PCB*) item, aPopMenu );
            break;

        case TYPETRACK:
        case TYPEVIA:
            locate_track = TRUE;
            CreatePopupMenuForTracks( (TRACK*) item, GetScreen()->m_Curseur,
                                     aPopMenu );
            break;

        case TYPEZONE:
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_EDIT_ZONE,
                          _( "Edit Zone" ), edit_xpm );
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_ZONE,
                          _( "Delete Zone" ), delete_xpm );
            break;

        case TYPEMARQUEUR:
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_MARKER,
                          _( "Delete Marker" ), delete_xpm );
            break;

        case TYPECOTATION:
            if( !flags )
            {
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_EDIT_COTATION,
                              _( "Edit Cotation" ), edit_xpm );
            }
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_COTATION,
                          _( "Delete Cotation" ), delete_xpm );
            break;

        case TYPEMIRE:
            if( !flags )
            {
                ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_MOVE_MIRE_REQUEST,
                              _( "Move Mire" ), move_xpm );
            }
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_EDIT_MIRE,
                          _( "Edit Mire" ), edit_xpm );
            ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DELETE_MIRE,
                          _( "Delete Mire" ), delete_xpm );
            break;

        case TYPEEDGEMODULE:
        case TYPESCREEN:
        case TYPE_NOT_INIT:
        case TYPEPCB:
        case PCB_EQUIPOT_STRUCT_TYPE:
            msg.Printf(
                wxT( "WinEDA_PcbFrame::OnRightClick() Error: illegal DrawType %d" ),
                item->Type() );
            DisplayError( this, msg );
            break;

        default:
            msg.Printf(
                wxT( "WinEDA_PcbFrame::OnRightClick() Error: unknown DrawType %d" ),
                item->Type() );
            DisplayError( this, msg );
            break;
        }

        aPopMenu->AppendSeparator();
    }

    if( !flags )
    {
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST,
                      _( "Get and Move Footprint" ), Move_Module_xpm );
    }
    
    /* Traitement des fonctions specifiques */
    switch(  m_ID_current_state )
    {
    case ID_PCB_ZONES_BUTT:
    {
        bool add_separator = FALSE;
        aPopMenu->Append( ID_POPUP_PCB_FILL_ZONE, _( "Fill zone" ) );

        if( item
           && ( (item->Type() == TYPEPAD)
             || (item->Type() == TYPETRACK)
             || (item->Type() == TYPEVIA) ) )
        {
            add_separator = TRUE;
            aPopMenu->Append( ID_POPUP_PCB_SELECT_NET_ZONE, _( "Select Net" ) );
        }
        if( m_Pcb->m_CurrentLimitZone )
        {
            add_separator = TRUE;
            aPopMenu->Append( ID_POPUP_PCB_DELETE_ZONE_LIMIT, _( "Delete Zone Limit" ) );
        }
        if( add_separator )
            aPopMenu->AppendSeparator();
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_SELECT_LAYER,
                      _( "Select Working Layer" ), Select_W_Layer_xpm );
        aPopMenu->AppendSeparator();
    }
        break;

    case ID_TRACK_BUTT:
    {
        ADD_MENUITEM_WITH_SUBMENU( aPopMenu, Append_Track_Width_List(),
                                   ID_POPUP_PCB_SELECT_WIDTH,
                                   _( "Select Track Width" ), width_track_xpm );
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_SELECT_CU_LAYER,
                      _( "Select Working Layer" ), Select_W_Layer_xpm );
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_SELECT_LAYER_PAIR,
                      _( "Select layer pair for vias" ), select_layer_pair_xpm );
        aPopMenu->AppendSeparator();
    }
        break;

    case ID_PCB_CIRCLE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_TEXT_COMMENT_BUTT:
    case ID_LINE_COMMENT_BUTT:
    case ID_PCB_COTATION_BUTT:
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_SELECT_NO_CU_LAYER,
                      _( "Select Working Layer" ), Select_W_Layer_xpm );
        aPopMenu->AppendSeparator();
        break;

    case ID_COMPONENT_BUTT:
        ADD_MENUITEM( aPopMenu, ID_POPUP_PCB_DISPLAY_FOOTPRINT_DOC,
                      _( "Footprint documentation" ), book_xpm );
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
                          _( "Unlock All Modules" ), Unlocked_xpm );
            ADD_MENUITEM( commands, ID_POPUP_PCB_AUTOPLACE_FIXE_ALL_MODULES,
                          _( "Lock All Modules" ), Locked_xpm );
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
                          _( "Select layer pair" ), select_layer_pair_xpm );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOROUTE_ALL_MODULES, _( "Autoroute All Modules" ) );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOROUTE_RESET_UNROUTED, _( "Reset Unrouted" ) );
            if( m_Pcb->m_Modules )
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
                      _( "Select Working Layer" ), Select_W_Layer_xpm );
        aPopMenu->AppendSeparator();
        break;
    }

    DrawPanel->CursorOn( &dc );
}


/****************************************/
void CreatePopUpBlockMenu( wxMenu* menu )
/****************************************/

/* Create Pop sub menu for block commands
 */
{
    ADD_MENUITEM( menu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                  _( "Cancel Block" ), cancel_xpm );
    ADD_MENUITEM( menu, ID_POPUP_ZOOM_BLOCK,
                  _( "Zoom Block (Midd butt drag)" ), zoom_selected_xpm );
    menu->AppendSeparator();
    ADD_MENUITEM( menu, ID_POPUP_PLACE_BLOCK,
                  _( "Place Block" ), apply_xpm );
    ADD_MENUITEM( menu, ID_POPUP_COPY_BLOCK,
                  _( "Copy Block (shift + drag mouse)" ), copyblock_xpm );
    ADD_MENUITEM( menu, ID_POPUP_INVERT_BLOCK,
                  _( "Flip Block (alt + drag mouse)" ), invert_module_xpm );
    ADD_MENUITEM( menu, ID_POPUP_ROTATE_BLOCK,
                  _( "Rotate Block (ctrl + drag mouse)" ), rotate_pos_xpm );
    ADD_MENUITEM( menu, ID_POPUP_DELETE_BLOCK,
                  _( "Delete Block (shift+ctrl + drag mouse)" ), delete_xpm );
}


/********************************************************************/
void CreatePopupMenuForTracks( TRACK* Track, wxPoint CursorPosition,
                               wxMenu* PopMenu )
/*******************************************************************/

/* Create command lines for a popup menu, for track editing
 */
{
    int flags = Track->m_Flags;

    if( flags == 0 )
    {
        if( Track->Type() == TYPEVIA )
        {
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_TRACK_NODE, _( "Drag Via" ), move_xpm );
            wxMenu* via_mnu = new wxMenu();
            ADD_MENUITEM_WITH_SUBMENU( PopMenu, via_mnu,
                                       ID_POPUP_PCB_VIA_EDITING, _( "Edit Via" ), edit_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_TO_DEFAULT,
                          _( "Set via hole to Default" ), apply_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_TO_VALUE, _(
                              "Set via hole to alt value" ), options_new_pad_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_ENTER_VALUE,
                          _( "Set the via hole alt value" ), edit_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_EXPORT, _(
                              "Export Via hole to alt value" ), Export_Options_Pad_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_EXPORT_TO_OTHERS,
                          _( "Export via hole to others id vias" ), global_options_pad_xpm );
            ADD_MENUITEM( via_mnu, ID_POPUP_PCB_VIA_HOLE_RESET_TO_DEFAULT,
                          _( "Set ALL via holes to default" ), apply_xpm );
            if( Track->m_Drill <= 0 )
            {
                via_mnu->Enable( ID_POPUP_PCB_VIA_HOLE_EXPORT, FALSE );
            }
            if( g_ViaHoleLastValue <= 0 )
                via_mnu->Enable( ID_POPUP_PCB_VIA_HOLE_TO_VALUE, FALSE );
        }
        else
        {
            if( Track->IsPointOnEnds( CursorPosition, -1 ) != 0 )
            {
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_TRACK_NODE,
                              _( "Move Node" ), move_xpm );
            }
            else
            {
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_DRAG_TRACK_SEGMENT_KEEP_SLOPE,
                              _( "Drag Segments, keep slope" ), drag_segment_withslope_xpm );
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_DRAG_TRACK_SEGMENT,
                              _( "Drag Segment" ), drag_track_segment_xpm );
#if 0
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_MOVE_TRACK_SEGMENT,
                              _( "Move Segment" ), move_track_segment_xpm );
#endif
                ADD_MENUITEM( PopMenu, ID_POPUP_PCB_BREAK_TRACK,
                              _( "Break Track" ), Break_Line_xpm );
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
            ADD_MENUITEM( PopMenu, ID_POPUP_PCB_END_TRACK, _( "End Track (end)" ), apply_xpm );
        PopMenu->Append( ID_POPUP_PCB_PLACE_VIA, _( "Place Via (V)" ) );
    }

    // track Width control :
    wxMenu* track_mnu = new wxMenu;
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, track_mnu,
                               ID_POPUP_PCB_EDIT_TRACK_MNU, _( "Change Width" ), width_track_xpm );
    ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_TRACKSEG,
                  _( "Edit Segment" ), width_segment_xpm );
    if( !flags )
    {
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_TRACK,
                      _( "Edit Track" ), width_track_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_NET,
                      _( "Edit Net" ), width_net_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_ALL_VIAS_AND_TRACK_SIZE,
                      _( "Edit ALL Tracks and Vias" ), width_track_via_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_ALL_VIAS_SIZE,
                      _( "Edit ALL Vias (no track)" ), width_vias_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_EDIT_ALL_TRACK_SIZE,
                      _( "Edit ALL Tracks (no via)" ), width_track_xpm );
    }

    // Delete control:
    track_mnu = new wxMenu;
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, track_mnu,
                               ID_POPUP_PCB_DELETE_TRACK_MNU, _( "Delete" ), delete_xpm );
    ADD_MENUITEM( track_mnu, ID_POPUP_PCB_DELETE_TRACKSEG,
                  _( "Delete Segment (backspace)" ), Delete_Line_xpm );
    if( !flags )
    {
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_DELETE_TRACK,
                      _( "Delete Track (delete)" ), Delete_Track_xpm );
        ADD_MENUITEM( track_mnu, ID_POPUP_PCB_DELETE_TRACKNET,
                      _( "Delete Net" ), Delete_Net_xpm );
    }
    track_mnu = new wxMenu;
    
    ADD_MENUITEM_WITH_SUBMENU( PopMenu, track_mnu,
                               ID_POPUP_PCB_SETFLAGS_TRACK_MNU, _( "Set Flags" ), Flag_xpm );
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


/*********************************************************************************/
void CreatePopUpMenuForFootprints( MODULE* aModule, wxMenu* menu, bool full_menu )
/*********************************************************************************/

/* Create the wxMenuitem list for footprint editing
 */
{
    wxMenu*  sub_menu_footprint;
    int      flags = aModule->m_Flags;

    wxString msg = aModule->MenuText();

    sub_menu_footprint = new wxMenu;
    
    ADD_MENUITEM_WITH_SUBMENU( menu, sub_menu_footprint, -1, msg, module_xpm );
    if( full_menu )
    {
        ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_MOVE_MODULE_REQUEST,
                      _( "Move (M)" ), Move_Module_xpm );
        ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_DRAG_MODULE_REQUEST,
                      _( "Drag (G)" ), Drag_Module_xpm );
    }
    ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE,
                  _( "Rotate  + (R)" ), rotate_module_pos_xpm );
    ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE,
                  _( "Rotate -" ), rotate_module_neg_xpm );
    ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_CHANGE_SIDE_MODULE,
                  _( "Flip (S)" ), invert_module_xpm );
    ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_EDIT_MODULE,
                  _( "Edit" ), Edit_Module_xpm );

    if( !flags )
    {
        sub_menu_footprint->AppendSeparator();
        ADD_MENUITEM( sub_menu_footprint, ID_POPUP_PCB_DELETE_MODULE,
                      _( "Delete Module" ), Delete_Module_xpm );
    }
}


/********************************************************************/
void CreatePopUpMenuForFpTexts( TEXTE_MODULE* FpText, wxMenu* menu )
/********************************************************************/

/* Create the wxMenuitem list for editing texts on footprints
 */
{
    wxMenu*  sub_menu_Fp_text;
    int      flags = FpText->m_Flags;

    wxString msg = FpText->MenuText();
    
    sub_menu_Fp_text = new wxMenu;
    
    ADD_MENUITEM_WITH_SUBMENU( menu, sub_menu_Fp_text, -1, msg, footprint_text_xpm );

    if( !flags )
        ADD_MENUITEM( sub_menu_Fp_text, ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST,
                      _( "Move" ), Move_Field_xpm );

    ADD_MENUITEM( sub_menu_Fp_text, ID_POPUP_PCB_ROTATE_TEXTMODULE,
                  _( "Rotate" ), Rotate_Field_xpm );
    ADD_MENUITEM( sub_menu_Fp_text, ID_POPUP_PCB_EDIT_TEXTMODULE,
                  _( "Edit" ), edit_text_xpm );
    
    if( FpText->m_Type == TEXT_is_DIVERS )
        ADD_MENUITEM( sub_menu_Fp_text, ID_POPUP_PCB_DELETE_TEXTMODULE,
                      _( "Delete" ), delete_xpm );
}


/***************************************************************/
void CreatePopUpMenuForPads( D_PAD* Pad, wxMenu* menu )
/***************************************************************/
/* Create pop menu for pads */
{
    wxMenu*  sub_menu_Pad;
    int      flags = Pad->m_Flags;

    wxString msg = Pad->MenuText();

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
    
    ADD_MENUITEM( sub_menu_Pad, ID_POPUP_PCB_IMPORT_PAD_SETTINGS,
                  _( "New Pad Settings" ), options_new_pad_xpm );
    ADD_MENUITEM( sub_menu_Pad, ID_POPUP_PCB_EXPORT_PAD_SETTINGS,
                  _( "Export Pad Settings" ), Export_Options_Pad_xpm );

    if( !flags )
    {
        ADD_MENUITEM( sub_menu_Pad, ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS,
                      _( "Global Pad Settings" ), global_options_pad_xpm );
        sub_menu_Pad->AppendSeparator();
        
        ADD_MENUITEM( sub_menu_Pad, ID_POPUP_PCB_DELETE_PAD,
                      _( "delete" ), Delete_Pad_xpm );
    }
}


/*************************************************************/
void CreatePopUpMenuForTexts( TEXTE_PCB* Text, wxMenu* menu )
/*************************************************************/
/* Create pop menu for pcb texts */
{
    wxMenu*  sub_menu_Text;
    int      flags = Text->m_Flags;

    wxString msg = Text->MenuText();

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
