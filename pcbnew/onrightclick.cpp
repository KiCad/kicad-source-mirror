/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew/onrightclick.cpp
 * @brief Right mouse button functions.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <macros.h>
#include <base_units.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_pcb_text.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <pcbnew_id.h>
#include <hotkeys.h>
#include <collectors.h>
#include <menus_helpers.h>


static wxMenu* Append_Track_Width_List( BOARD* aBoard );


bool PCB_EDIT_FRAME::OnRightClick( const wxPoint& aMousePos, wxMenu* aPopMenu )
{
    wxString    msg;
    STATUS_FLAGS flags = 0;
    bool        trackFound = false; // Flag set to true,
                                    // if a track is being the cursor, to avoid
                                    // to display menus relative to tracks twice
    bool        blockActive  = !GetScreen()->m_BlockLocate.IsIdle();

    BOARD_ITEM* item = GetCurItem();

    m_canvas->SetCanStartBlock( -1 );    // Avoid to start a block command when clicking on menu

    // If a command or a block is in progress:
    // Put the Cancel command (if needed) and the End command

    if( blockActive )
    {
        createPopUpBlockMenu( aPopMenu );
        aPopMenu->AppendSeparator();
        return true;
    }

    if( GetToolId() != ID_NO_TOOL_SELECTED )
    {
        if( item && item->GetFlags() )
        {
            AddMenuItem( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel" ),
                         KiBitmap( cancel_xpm ) );
        }
        else
        {
            AddMenuItem( aPopMenu, ID_POPUP_CLOSE_CURRENT_TOOL,
                         _( "End Tool" ), KiBitmap( cursor_xpm ) );
        }

        aPopMenu->AppendSeparator();
    }
    else
    {
        if( item && item->GetFlags() )
        {
            AddMenuItem( aPopMenu, ID_POPUP_CANCEL_CURRENT_COMMAND,
                         _( "Cancel" ), KiBitmap( cancel_xpm ) );
            aPopMenu->AppendSeparator();
        }
    }

    // Select a proper item

    wxPoint cursorPos = GetCrossHairPosition();
    wxPoint selectPos = m_Collector->GetRefPos();

    selectPos = GetNearestGridPosition( selectPos );

    /*  We can reselect another item only if there are no item being edited
     * because ALL moving functions use GetCurItem(), therefore GetCurItem()
     * must return the same item during moving. We know an item is moving
     * if( item && (item->m_Flags != 0)) is true and after calling
     * PcbGeneralLocateAndDisplay(), GetCurItem() is any arbitrary BOARD_ITEM,
     * not the current item being edited. In such case we cannot call
     * PcbGeneralLocateAndDisplay().
     */
    if( !item || (item->GetFlags() == 0) )
    {
        // show the "item selector" menu if no item selected or
        // if there is a selected item but the mouse has moved
        // (therefore a new item is perhaps under the cursor)
        if( !item || cursorPos != selectPos )
        {
            m_canvas->SetAbortRequest( false );
            PcbGeneralLocateAndDisplay();

            if( m_canvas->GetAbortRequest() )
            {
                return false;
            }
        }
    }

    item = GetCurItem();
    flags = item ? item->GetFlags() : 0;

    // Add the context menu, which depends on the picked item:
    if( item )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_T:
            createPopUpMenuForFootprints( (MODULE*) item, aPopMenu );

            if( m_mainToolBar->GetToolToggled( ID_TOOLBARH_PCB_MODE_MODULE ) )
            {
                aPopMenu->AppendSeparator();

                if( !( (MODULE*) item )->IsLocked() )
                {
                    msg = AddHotkeyName( _("Lock Footprint" ), g_Board_Editor_Hokeys_Descr,
                                         HK_LOCK_UNLOCK_FOOTPRINT );
                    AddMenuItem( aPopMenu, ID_POPUP_PCB_AUTOPLACE_FIXE_MODULE, msg,
                                 KiBitmap( locked_xpm ) );
                }
                else
                {
                    msg = AddHotkeyName( _( "Unlock Footprint" ), g_Board_Editor_Hokeys_Descr,
                                         HK_LOCK_UNLOCK_FOOTPRINT );
                    AddMenuItem( aPopMenu, ID_POPUP_PCB_AUTOPLACE_FREE_MODULE, msg,
                                 KiBitmap( unlocked_xpm ) );
                }

                if( !flags )
                    aPopMenu->Append( ID_POPUP_PCB_AUTOPLACE_CURRENT_MODULE,
                                      _( "Automatically Place Footprint" ) );
            }

            if( m_mainToolBar->GetToolToggled( ID_TOOLBARH_PCB_MODE_TRACKS ) )
            {
                if( !flags )
                    aPopMenu->Append( ID_POPUP_PCB_AUTOROUTE_MODULE,
                                      _( "Automatically Route Footprint" ) );
            }
            break;

        case PCB_PAD_T:
            createPopUpMenuForFpPads( static_cast<D_PAD*>( item ), aPopMenu );
            break;

        case PCB_MODULE_TEXT_T:
            createPopUpMenuForFpTexts( static_cast<TEXTE_MODULE*>( item ), aPopMenu );
            break;

        case PCB_LINE_T:  // Some graphic items on technical layers
            if( (flags & IS_NEW) )
            {
                AddMenuItem( aPopMenu, ID_POPUP_PCB_STOP_CURRENT_DRAWING,
                             _( "End Drawing" ), KiBitmap( checked_ok_xpm ) );
            }

            if( !flags )
            {
                msg = AddHotkeyName( _( "Move Drawing" ), g_Board_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_MOVE_DRAWING_REQUEST,
                             msg, KiBitmap( move_xpm ) );

                msg = AddHotkeyName( _( "Duplicate Drawing" ), g_Board_Editor_Hokeys_Descr,
                                     HK_DUPLICATE_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_DUPLICATE_ITEM,
                             msg, KiBitmap( duplicate_line_xpm ) );

                msg = AddHotkeyName( _("Move Drawing Exactly" ), g_Board_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM_EXACT );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_MOVE_EXACT,
                             msg, KiBitmap( move_line_xpm ) );

                msg = AddHotkeyName( _("Create Drawing Array" ), g_Board_Editor_Hokeys_Descr,
                                     HK_CREATE_ARRAY );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_CREATE_ARRAY,
                             msg, KiBitmap( array_line_xpm ) );

                msg = AddHotkeyName( _( "Edit Drawing" ), g_Board_Editor_Hokeys_Descr,
                                     HK_EDIT_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_EDIT_DRAWING,
                             msg, KiBitmap( edit_xpm ) );

                AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_DRAWING,
                             _( "Delete Drawing" ), KiBitmap( delete_xpm ) );

                if( !IsCopperLayer( item->GetLayer() ) )
                    AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_DRAWING_LAYER,
                                 _( "Delete All Drawings on Layer" ), KiBitmap( delete_xpm ) );
            }

            break;

        case PCB_ZONE_T:      // Item used to fill a zone
            AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_ZONE,
                         _( "Delete Zone Filling" ), KiBitmap( delete_xpm ) );
            break;

        case PCB_ZONE_AREA_T:    // Item used to handle a zone area (outlines, holes ...)
            if( flags & IS_NEW )
            {
                AddMenuItem( aPopMenu, ID_POPUP_PCB_STOP_CURRENT_EDGE_ZONE,
                             _( "Close Zone Outline" ), KiBitmap( checked_ok_xpm ) );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_ZONE_LAST_CREATED_CORNER,
                             _( "Delete Last Corner" ), KiBitmap( delete_xpm ) );
            }
            else
            {
                createPopUpMenuForZones( (ZONE_CONTAINER*) item, aPopMenu );
            }

            break;

        case PCB_TEXT_T:
            createPopUpMenuForTexts( (TEXTE_PCB*) item, aPopMenu );
            break;

        case PCB_TRACE_T:
        case PCB_VIA_T:
            trackFound = true;
            createPopupMenuForTracks( (TRACK*) item, aPopMenu );
            break;

        case PCB_MARKER_T:
            createPopUpMenuForMarkers( (MARKER_PCB*) item, aPopMenu );
            break;

        case PCB_DIMENSION_T:
            if( !flags )
            {
                msg = AddHotkeyName( _( "Edit Dimension" ), g_Board_Editor_Hokeys_Descr,
                                     HK_EDIT_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_EDIT_DIMENSION, msg, KiBitmap( edit_xpm ) );

                msg = AddHotkeyName( _( "Move Dimension Text" ), g_Board_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_MOVE_TEXT_DIMENSION_REQUEST,
                             msg, KiBitmap( move_text_xpm ) );

                msg = AddHotkeyName( _( "Duplicate Dimension" ), g_Board_Editor_Hokeys_Descr,
                                     HK_DUPLICATE_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_DUPLICATE_ITEM,
                             msg, KiBitmap( duplicate_text_xpm ) );

                msg = AddHotkeyName( _("Move Dimension Exactly" ), g_Board_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM_EXACT );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_MOVE_EXACT,
                             msg, KiBitmap( move_text_xpm ) );

                msg = AddHotkeyName( _( "Delete Dimension" ), g_Board_Editor_Hokeys_Descr,
                                     HK_DELETE );

                AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_DIMENSION,
                             msg, KiBitmap( delete_xpm ) );
            }
            break;

        case PCB_TARGET_T:
            if( !flags )
            {
                msg = AddHotkeyName( _( "Move Target" ), g_Board_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_MOVE_MIRE_REQUEST,
                             msg, KiBitmap( move_target_xpm ) );

                msg = AddHotkeyName( _("Move Target Exactly" ), g_Board_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM_EXACT );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_MOVE_EXACT,
                             msg, KiBitmap( move_target_xpm ) );

                msg = AddHotkeyName( _( "Duplicate Target" ), g_Board_Editor_Hokeys_Descr,
                                     HK_DUPLICATE_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_DUPLICATE_ITEM,
                             msg, KiBitmap( duplicate_target_xpm ) );

                msg = AddHotkeyName( _("Create Target Array" ), g_Board_Editor_Hokeys_Descr,
                                     HK_CREATE_ARRAY );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_CREATE_ARRAY,
                             msg, KiBitmap( array_target_xpm ) );

                msg = AddHotkeyName( _( "Edit Target" ), g_Board_Editor_Hokeys_Descr,
                                     HK_EDIT_ITEM );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_EDIT_MIRE, msg, KiBitmap( edit_xpm ) );

                msg = AddHotkeyName( _( "Delete Target" ), g_Board_Editor_Hokeys_Descr, HK_DELETE );
                AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_MIRE,
                             msg, KiBitmap( delete_xpm ) );
            }

            break;

        case PCB_MODULE_EDGE_T:
        case SCREEN_T:
        case TYPE_NOT_INIT:
        case PCB_T:
            msg.Printf( wxT( "PCB_EDIT_FRAME::OnRightClick() Error: unexpected DrawType %d" ),
                        item->Type() );
            wxMessageBox( msg );
            SetCurItem( NULL );
            break;

        default:
            msg.Printf( wxT( "PCB_EDIT_FRAME::OnRightClick() Error: unknown DrawType %d" ),
                        item->Type() );
            wxMessageBox( msg );

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

    // Display context sensitive commands:
    switch(  GetToolId() )
    {
    case ID_PCB_ZONES_BUTT:
        if(  GetBoard()->m_ZoneDescriptorList.size() > 0 )
        {
            aPopMenu->AppendSeparator();
            msg = AddHotkeyName( _( "Fill or Refill All Zones" ),
                                 g_Board_Editor_Hokeys_Descr, HK_ZONE_FILL_OR_REFILL );
            AddMenuItem( aPopMenu, ID_POPUP_PCB_FILL_ALL_ZONES,
                         msg, KiBitmap( fill_zone_xpm ) );
            msg = AddHotkeyName( _( "Remove Filled Areas in All Zones" ),
                                 g_Board_Editor_Hokeys_Descr, HK_ZONE_REMOVE_FILLED );
            AddMenuItem( aPopMenu, ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_ALL_ZONES,
                         msg, KiBitmap( zone_unfill_xpm ) );
            aPopMenu->AppendSeparator();
        }

        AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_LAYER,
                     _( "Select Working Layer" ), KiBitmap( select_w_layer_xpm ) );
        aPopMenu->AppendSeparator();
        break;

    case ID_PCB_KEEPOUT_AREA_BUTT:
        AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_LAYER,
                     _( "Select Working Layer" ), KiBitmap( select_w_layer_xpm ) );
        aPopMenu->AppendSeparator();
        break;

    case ID_TRACK_BUTT:
        if ( ! trackFound )   // This menu is already added when a track is located
        {
            aPopMenu->AppendSeparator();
            msg = AddHotkeyName( _( "Begin Track" ),
                                 g_Board_Editor_Hokeys_Descr, HK_ADD_NEW_TRACK );
            AddMenuItem( aPopMenu, ID_POPUP_PCB_BEGIN_TRACK,
                         msg, KiBitmap( add_tracks_xpm ) );

            AddMenuItem( aPopMenu, Append_Track_Width_List( GetBoard() ),
                         ID_POPUP_PCB_SELECT_WIDTH, _( "Select Track Width" ),
                         KiBitmap( width_track_xpm ) );

            AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_CU_LAYER,
                         _( "Select Working Layer" ), KiBitmap( select_w_layer_xpm ) );
            AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_LAYER_PAIR,
                         _( "Select Layer Pair for Vias" ), KiBitmap( select_layer_pair_xpm ) );
            aPopMenu->AppendSeparator();
        }
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
        if( !flags )
        {
            AddMenuItem( aPopMenu, ID_POPUP_PCB_DISPLAY_FOOTPRINT_DOC,
                         _( "Footprint Documentation" ), KiBitmap( book_xpm ) );
            aPopMenu->AppendSeparator();
        }
        break;

    case ID_NO_TOOL_SELECTED:
        if( m_mainToolBar->GetToolToggled( ID_TOOLBARH_PCB_MODE_MODULE ) )
        {
            wxMenu* commands = new wxMenu;
            AddMenuItem( aPopMenu, commands, ID_POPUP_PCB_AUTOPLACE_COMMANDS,
                         _( "Global Spread and Place" ), KiBitmap( move_xpm ) );
            AddMenuItem( commands, ID_POPUP_PCB_AUTOPLACE_FREE_ALL_MODULES,
                         _( "Unlock All Footprints" ), KiBitmap( unlocked_xpm ) );
            AddMenuItem( commands, ID_POPUP_PCB_AUTOPLACE_FIXE_ALL_MODULES,
                         _( "Lock All Footprints" ), KiBitmap( locked_xpm ) );
            commands->AppendSeparator();
            AddMenuItem( commands, ID_POPUP_PCB_SPREAD_ALL_MODULES,
                         _( "Spread out All Footprints" ), KiBitmap( move_xpm ) );
            commands->Append( ID_POPUP_PCB_SPREAD_NEW_MODULES,
                              _( "Spread out Footprints not Already on Board" ) );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOPLACE_ALL_MODULES,
                              _( "Automatically Place All Footprints" ) );
            commands->Append( ID_POPUP_PCB_AUTOPLACE_NEW_MODULES,
                              _( "Automatically Place New Footprints" ) );
            commands->Append( ID_POPUP_PCB_AUTOPLACE_NEXT_MODULE,
                              _( "Automatically Place Next Footprints" ) );
            commands->AppendSeparator();
            AddMenuItem( commands, ID_POPUP_PCB_REORIENT_ALL_MODULES,
                         _( "Orient All Footprints" ), KiBitmap( rotate_module_cw_xpm ) );
            aPopMenu->AppendSeparator();
        }

        if( m_mainToolBar->GetToolToggled( ID_TOOLBARH_PCB_MODE_TRACKS ) )
        {
            wxMenu* commands = new wxMenu;
            aPopMenu->Append( ID_POPUP_PCB_AUTOROUTE_COMMANDS, _( "Autoroute" ), commands );
            AddMenuItem( commands, ID_POPUP_PCB_SELECT_LAYER_PAIR,
                         _( "Select Layer Pair" ), KiBitmap( select_layer_pair_xpm ) );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOROUTE_ALL_MODULES,
                              _( "Automatically Route All Footprints" ) );
            commands->AppendSeparator();
            commands->Append( ID_POPUP_PCB_AUTOROUTE_RESET_UNROUTED, _( "Reset Unrouted" ) );
            aPopMenu->AppendSeparator();
        }

        if( !trackFound )
        {
            msg = AddHotkeyName( _( "Begin Track" ), g_Board_Editor_Hokeys_Descr, HK_ADD_NEW_TRACK );
            AddMenuItem( aPopMenu, ID_POPUP_PCB_BEGIN_TRACK, msg, KiBitmap( add_tracks_xpm ) );

            AddMenuItem( aPopMenu, Append_Track_Width_List( GetBoard() ),
                         ID_POPUP_PCB_SELECT_WIDTH, _( "Select Track Width" ),
                         KiBitmap( width_track_xpm ) );

            AddMenuItem( aPopMenu, ID_POPUP_PCB_SELECT_LAYER,
                         _( "Select Working Layer" ), KiBitmap( select_w_layer_xpm ) );
            aPopMenu->AppendSeparator();
        }
        break;
    }

    return true;
}


/* Create Pop sub menu for block commands
 */
void PCB_EDIT_FRAME::createPopUpBlockMenu( wxMenu* menu )
{
    AddMenuItem( menu, ID_POPUP_CANCEL_CURRENT_COMMAND, _( "Cancel Block" ),
                 KiBitmap( cancel_xpm ) );
    AddMenuItem( menu, ID_POPUP_ZOOM_BLOCK, _( "Zoom Block" ), KiBitmap( zoom_area_xpm ) );
    menu->AppendSeparator();
    AddMenuItem( menu, ID_POPUP_PLACE_BLOCK, _( "Place Block" ), KiBitmap( checked_ok_xpm ) );
    AddMenuItem( menu, ID_POPUP_COPY_BLOCK, _( "Copy Block" ), KiBitmap( copyblock_xpm ) );
    AddMenuItem( menu, ID_POPUP_FLIP_BLOCK, _( "Flip Block" ), KiBitmap( mirror_footprint_axisX_xpm ) );
    AddMenuItem( menu, ID_POPUP_ROTATE_BLOCK, _( "Rotate Block" ), KiBitmap( rotate_ccw_xpm ) );
    AddMenuItem( menu, ID_POPUP_DELETE_BLOCK, _( "Delete Block" ), KiBitmap( delete_xpm ) );
}


/* Create command lines for a popup menu, for track and via editing
 * also update Netclass selection
 */
void PCB_EDIT_FRAME::createPopupMenuForTracks( TRACK* Track, wxMenu* PopMenu )
{
    wxPoint  cursorPosition = GetCrossHairPosition();
    wxString msg;

    SetCurrentNetClass( Track->GetNetClassName() );

    int flags = Track->GetFlags();

    if( flags == 0 )
    {
        msg = AddHotkeyName( _( "Begin Track" ),
                             g_Board_Editor_Hokeys_Descr, HK_ADD_NEW_TRACK );
        AddMenuItem( PopMenu, ID_POPUP_PCB_BEGIN_TRACK,
                     msg, KiBitmap( add_tracks_xpm ) );

        if( Track->Type() == PCB_VIA_T )
        {
            msg = AddHotkeyName( _( "Drag Via" ), g_Board_Editor_Hokeys_Descr,
                                 HK_DRAG_ITEM );
            AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_TRACK_NODE, msg,
                         KiBitmap( move_xpm ) );
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

                msg = AddHotkeyName( _( "Duplicate Track" ), g_Board_Editor_Hokeys_Descr,
                                     HK_DUPLICATE_ITEM );
                AddMenuItem( PopMenu, ID_POPUP_PCB_DUPLICATE_ITEM,
                             msg, KiBitmap( duplicate_line_xpm ) );

                msg = AddHotkeyName( _("Move Track Exactly" ), g_Board_Editor_Hokeys_Descr,
                                     HK_MOVE_ITEM_EXACT );
                AddMenuItem( PopMenu, ID_POPUP_PCB_MOVE_EXACT,
                             msg, KiBitmap( move_line_xpm ) );

                msg = AddHotkeyName( _("Create Track Array" ), g_Board_Editor_Hokeys_Descr,
                                     HK_CREATE_ARRAY );
                AddMenuItem( PopMenu, ID_POPUP_PCB_CREATE_ARRAY,
                             msg, KiBitmap( array_line_xpm ) );

                AddMenuItem( PopMenu, ID_POPUP_PCB_BREAK_TRACK,
                             _( "Break Track" ), KiBitmap( break_line_xpm ) );
            }
        }

        AddMenuItem( PopMenu, ID_POPUP_PCB_SELECT_CU_LAYER,
                     _( "Select Working Layer" ), KiBitmap( select_w_layer_xpm ) );
    }
    else if( flags & IS_DRAGGED )   // Drag via or node in progress
    {
        AddMenuItem( PopMenu, ID_POPUP_PCB_PLACE_MOVED_TRACK_NODE,
                     _( "Place Node" ), KiBitmap( checked_ok_xpm ) );
        return;
    }
    else // Edition in progress
    {
        if( flags & IS_NEW )
        {
            msg = AddHotkeyName( _( "End Track" ), g_Board_Editor_Hokeys_Descr, HK_LEFT_DCLICK );
            AddMenuItem( PopMenu, ID_POPUP_PCB_END_TRACK, msg, KiBitmap( checked_ok_xpm ) );
        }

        msg = AddHotkeyName( _( "Place Through Via" ), g_Board_Editor_Hokeys_Descr, HK_ADD_THROUGH_VIA );
        AddMenuItem( PopMenu, ID_POPUP_PCB_PLACE_THROUGH_VIA, msg, KiBitmap( via_xpm ) );

        msg = AddHotkeyName( _( "Select Layer and Place Through Via" ),
                             g_Board_Editor_Hokeys_Descr, HK_SEL_LAYER_AND_ADD_THROUGH_VIA );
        AddMenuItem( PopMenu, ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_THROUGH_VIA,
                     msg, KiBitmap( select_w_layer_xpm ) );

        if( GetDesignSettings().m_BlindBuriedViaAllowed )
        {
            msg = AddHotkeyName( _( "Place Blind/Buried Via" ),
                                 g_Board_Editor_Hokeys_Descr, HK_ADD_BLIND_BURIED_VIA );
            AddMenuItem( PopMenu, ID_POPUP_PCB_PLACE_BLIND_BURIED_VIA, msg, KiBitmap( via_buried_xpm ) );

            msg = AddHotkeyName( _( "Select Layer and Place Blind/Buried Via" ),
                                 g_Board_Editor_Hokeys_Descr, HK_SEL_LAYER_AND_ADD_BLIND_BURIED_VIA );
            AddMenuItem( PopMenu, ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_BLIND_BURIED_VIA,
                         msg, KiBitmap( select_w_layer_xpm ) );
        }

        msg = AddHotkeyName( _( "Switch Track Posture" ), g_Board_Editor_Hokeys_Descr,
                             HK_SWITCH_TRACK_POSTURE );
        AddMenuItem( PopMenu, ID_POPUP_PCB_SWITCH_TRACK_POSTURE, msg,
                             KiBitmap( change_entry_orient_xpm ) );

        // See if we can place a Micro Via (4 or more layers, and start from an external layer):
        if( IsMicroViaAcceptable() )
        {
            msg = AddHotkeyName( _( "Place Micro Via" ), g_Board_Editor_Hokeys_Descr,
                                 HK_ADD_MICROVIA );
            AddMenuItem( PopMenu, ID_POPUP_PCB_PLACE_MICROVIA, msg, KiBitmap( via_microvia_xpm ) );
        }
    }

    // track Width control :
    if( !flags )
    {
        if( Track->Type() == PCB_VIA_T )
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
    wxMenu* trackdel_mnu = new wxMenu;
    AddMenuItem( PopMenu, trackdel_mnu, ID_POPUP_PCB_DELETE_TRACK_MNU, _( "Delete" ),
                 KiBitmap( delete_xpm ) );

    msg = AddHotkeyName( Track->Type()==PCB_VIA_T ?
                        _( "Delete Via" ) : _( "Delete Segment" ),
                         g_Board_Editor_Hokeys_Descr, HK_BACK_SPACE );

    AddMenuItem( trackdel_mnu, ID_POPUP_PCB_DELETE_TRACKSEG, msg, KiBitmap( delete_line_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Delete Track" ), g_Board_Editor_Hokeys_Descr, HK_DELETE );
        AddMenuItem( trackdel_mnu, ID_POPUP_PCB_DELETE_TRACK, msg, KiBitmap( delete_track_xpm ) );
        AddMenuItem( trackdel_mnu, ID_POPUP_PCB_DELETE_TRACKNET, _( "Delete Net" ),
                     KiBitmap( delete_net_xpm ) );
    }

    // Add global edition command
    if( !flags )
    {
        PopMenu->AppendSeparator();
        AddMenuItem( PopMenu, ID_POPUP_PCB_EDIT_ALL_VIAS_AND_TRACK_SIZE,
                     _( "Edit All Tracks and Vias" ), KiBitmap( width_track_via_xpm ) );
    }

    // Add lock/unlock flags menu:
    wxMenu* trackflg_mnu = new wxMenu;

    AddMenuItem( PopMenu, trackflg_mnu, ID_POPUP_PCB_SETFLAGS_TRACK_MNU, _( "Set Flags" ),
                 KiBitmap( flag_xpm ) );
    trackflg_mnu->Append( ID_POPUP_PCB_LOCK_ON_TRACKSEG, _( "Locked: Yes" ), wxEmptyString, true );
    trackflg_mnu->Append( ID_POPUP_PCB_LOCK_OFF_TRACKSEG, _( "Locked: No" ), wxEmptyString, true );

    if( Track->GetState( TRACK_LOCKED ) )
        trackflg_mnu->Check( ID_POPUP_PCB_LOCK_ON_TRACKSEG, true );
    else
        trackflg_mnu->Check( ID_POPUP_PCB_LOCK_OFF_TRACKSEG, true );

    if( !flags )
    {
        trackflg_mnu->Append( ID_POPUP_PCB_LOCK_ON_TRACK, _( "Track Locked: Yes" ) );
        trackflg_mnu->Append( ID_POPUP_PCB_LOCK_OFF_TRACK, _( "Track Locked: No" ) );
        trackflg_mnu->AppendSeparator();
        trackflg_mnu->Append( ID_POPUP_PCB_LOCK_ON_NET, _( "Net Locked: Yes" ) );
        trackflg_mnu->Append( ID_POPUP_PCB_LOCK_OFF_NET, _( "Net Locked: No" ) );
    }
}


/* Create the wxMenuitem list for zone outlines editing and zone filling
 */
void PCB_EDIT_FRAME::createPopUpMenuForZones( ZONE_CONTAINER* edge_zone, wxMenu* aPopMenu )
{
    wxString msg;

    if( edge_zone->GetFlags() == IS_DRAGGED )
    {
        AddMenuItem( aPopMenu, ID_POPUP_PCB_PLACE_DRAGGED_ZONE_OUTLINE_SEGMENT,
                     _( "Place Edge Outline" ), KiBitmap( checked_ok_xpm ) );
    }
    else if( edge_zone->GetFlags() )
    {
        if( (edge_zone->GetFlags() & IN_EDIT ) )
            AddMenuItem( aPopMenu, ID_POPUP_PCB_PLACE_ZONE_CORNER,
                         _( "Place Corner" ), KiBitmap( checked_ok_xpm ) );
        else
            AddMenuItem( aPopMenu, ID_POPUP_PCB_PLACE_ZONE_OUTLINES,
                         _( "Place Zone" ), KiBitmap( checked_ok_xpm ) );
    }
    else
    {
        wxMenu* zones_menu = new wxMenu();

        AddMenuItem( aPopMenu, zones_menu, -1,
                    edge_zone->GetIsKeepout() ? _("Keepout Area") : _( "Zones" ),
                    KiBitmap( add_zone_xpm ) );

        if( edge_zone->HitTestForCorner( RefPos( true ) ) >= 0 )
        {
            AddMenuItem( zones_menu, ID_POPUP_PCB_MOVE_ZONE_CORNER,
                         _( "Move Corner" ), KiBitmap( move_xpm ) );
            AddMenuItem( zones_menu, ID_POPUP_PCB_DELETE_ZONE_CORNER,
                         _( "Delete Corner" ), KiBitmap( delete_xpm ) );
        }
        else if( edge_zone->HitTestForEdge( RefPos( true ) ) >= 0 )
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

        AddMenuItem( zones_menu, ID_POPUP_PCB_ZONE_DUPLICATE,
                     _( "Duplicate Zone Onto Layer" ), KiBitmap( zone_duplicate_xpm ) );

        zones_menu->AppendSeparator();

        if( ! edge_zone->GetIsKeepout() )
            AddMenuItem( zones_menu, ID_POPUP_PCB_FILL_ZONE, _( "Fill Zone" ),
                         KiBitmap( fill_zone_xpm ) );

        if( !edge_zone->GetFilledPolysList().IsEmpty() )
        {
            AddMenuItem( zones_menu, ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_CURRENT_ZONE,
                         _( "Remove Filled Areas in Zone" ), KiBitmap( zone_unfill_xpm ) );
        }

        msg = AddHotkeyName( _( "Move Zone" ), g_Board_Editor_Hokeys_Descr, HK_MOVE_ITEM );
        AddMenuItem( zones_menu, ID_POPUP_PCB_MOVE_ZONE_OUTLINES, msg, KiBitmap( move_xpm ) );

        msg = AddHotkeyName( _("Move Zone Exactly" ), g_Board_Editor_Hokeys_Descr,
                             HK_MOVE_ITEM_EXACT );
        AddMenuItem( zones_menu, ID_POPUP_PCB_MOVE_EXACT,
                     msg, KiBitmap( move_zone_xpm ) );

        msg = AddHotkeyName( _( "Edit Zone Properties" ), g_Board_Editor_Hokeys_Descr,
                             HK_EDIT_ITEM );
        AddMenuItem( zones_menu, ID_POPUP_PCB_EDIT_ZONE_PARAMS,
                     msg, KiBitmap( edit_xpm ) );

        zones_menu->AppendSeparator();

        if( edge_zone->GetSelectedCorner() >= 0 &&
            edge_zone->Outline()->IsCutoutContour( edge_zone->GetSelectedCorner() ) )
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
    int      flags = aModule->GetFlags();
    wxString msg;

    sub_menu_footprint = new wxMenu;

    msg = aModule->GetSelectMenuText();
    AddMenuItem( menu, sub_menu_footprint, -1, msg, KiBitmap( module_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Move" ), g_Board_Editor_Hokeys_Descr, HK_MOVE_ITEM );
        AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_MOVE_MODULE_REQUEST,
                     msg, KiBitmap( move_module_xpm ) );

        msg = AddHotkeyName( _( "Duplicate Footprint" ), g_Board_Editor_Hokeys_Descr,
                             HK_DUPLICATE_ITEM );
        AddMenuItem( menu, ID_POPUP_PCB_DUPLICATE_ITEM,
                     msg, KiBitmap( duplicate_module_xpm ) );

        msg = AddHotkeyName( _("Move Footprint Exactly" ), g_Board_Editor_Hokeys_Descr,
                             HK_MOVE_ITEM_EXACT );
        AddMenuItem( menu, ID_POPUP_PCB_MOVE_EXACT,
                     msg, KiBitmap( move_module_xpm ) );

        msg = AddHotkeyName( _("Create Footprint Array" ), g_Board_Editor_Hokeys_Descr,
                             HK_CREATE_ARRAY );
        AddMenuItem( menu, ID_POPUP_PCB_CREATE_ARRAY,
                     msg, KiBitmap( array_module_xpm ) );

        msg = AddHotkeyName( _( "Drag" ), g_Board_Editor_Hokeys_Descr, HK_DRAG_ITEM  );
        AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_DRAG_MODULE_REQUEST,
                     msg, KiBitmap( drag_module_xpm ) );
    }

    msg = AddHotkeyName( _( "Rotate +" ), g_Board_Editor_Hokeys_Descr, HK_ROTATE_ITEM );
    AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE,
                 msg, KiBitmap( rotate_module_ccw_xpm ) );
    AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE,
                 _( "Rotate -" ), KiBitmap( rotate_module_cw_xpm ) );
    msg = AddHotkeyName( _( "Flip" ), g_Board_Editor_Hokeys_Descr, HK_FLIP_ITEM );
    AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_CHANGE_SIDE_MODULE,
                 msg, KiBitmap( mirror_footprint_axisX_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Edit Parameters" ),
                             g_Board_Editor_Hokeys_Descr, HK_EDIT_ITEM );
        AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_EDIT_MODULE_PRMS, msg,
                     KiBitmap( edit_module_xpm ) );

        msg = AddHotkeyName( _( "Edit with Footprint Editor" ),
                             g_Board_Editor_Hokeys_Descr, HK_EDIT_MODULE_WITH_MODEDIT );
        AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_EDIT_MODULE_WITH_MODEDIT,
                     msg, KiBitmap( module_editor_xpm ) );

        sub_menu_footprint->AppendSeparator();

        msg = AddHotkeyName( _( "Delete Footprint" ),
                             g_Board_Editor_Hokeys_Descr, HK_DELETE );
        AddMenuItem( sub_menu_footprint, ID_POPUP_PCB_DELETE_MODULE,
                     msg, KiBitmap( delete_module_xpm ) );
    }
}


/* Create the wxMenuitem list for editing texts on footprints
 */
void PCB_EDIT_FRAME::createPopUpMenuForFpTexts( TEXTE_MODULE* FpText, wxMenu* menu )
{
    wxMenu*  sub_menu_Fp_text;
    int      flags = FpText->GetFlags();

    wxString msg = FpText->GetSelectMenuText();

    sub_menu_Fp_text = new wxMenu;

    AddMenuItem( menu, sub_menu_Fp_text, -1, msg, KiBitmap( footprint_text_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Move Text" ), g_Board_Editor_Hokeys_Descr, HK_MOVE_ITEM );
        AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST,
                     msg, KiBitmap( move_field_xpm ) );

        msg = AddHotkeyName( _("Move Text Exactly" ), g_Board_Editor_Hokeys_Descr,
                             HK_MOVE_ITEM_EXACT );
        AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_MOVE_EXACT,
                     msg, KiBitmap( move_text_xpm ) );
    }

    msg = AddHotkeyName( _( "Rotate Text" ), g_Board_Editor_Hokeys_Descr, HK_ROTATE_ITEM );
    AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_ROTATE_TEXTMODULE,
                 msg, KiBitmap( rotate_field_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Edit Text" ), g_Board_Editor_Hokeys_Descr, HK_EDIT_ITEM );
        AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_EDIT_TEXTMODULE,
                     msg, KiBitmap( edit_text_xpm ) );
        AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_RESET_TEXT_SIZE,
                     _( "Reset Size" ), KiBitmap( reset_text_xpm ) );
    }

    // Graphic texts can be deleted only if are not currently edited.
    if( !flags && FpText->GetType() == TEXTE_MODULE::TEXT_is_DIVERS )
    {
        AddMenuItem( sub_menu_Fp_text, ID_POPUP_PCB_DELETE_TEXTMODULE,
                     _( "Delete Text" ), KiBitmap( delete_xpm ) );
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
    int     flags = Pad->GetFlags();

    if( flags )     // Currently in edit, no others commands possible
        return;

    SetCurrentNetClass( Pad->GetNetClassName() );

    wxString msg = Pad->GetSelectMenuText();

    sub_menu_Pad = new wxMenu;
    AddMenuItem( menu, sub_menu_Pad, -1, msg, KiBitmap( pad_xpm ) );

    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_MOVE_PAD_REQUEST, _( "Move Pad" ),
                 KiBitmap( move_pad_xpm ) );
    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_DRAG_PAD_REQUEST, _( "Drag Pad" ),
                 KiBitmap( drag_pad_xpm ) );

    msg = AddHotkeyName( _( "Edit Pad" ), g_Board_Editor_Hokeys_Descr, HK_EDIT_ITEM );
    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_EDIT_PAD, msg, KiBitmap( options_pad_xpm ) );
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
                 _( "Edit All Pads" ),
                 _( "Copy this pad's settings to all pads in this footprint (or similar footprints)" ),
                 KiBitmap( global_options_pad_xpm ) );
    sub_menu_Pad->AppendSeparator();

    AddMenuItem( sub_menu_Pad, ID_POPUP_PCB_DELETE_PAD, _( "Delete" ), KiBitmap( delete_pad_xpm ) );

    if( m_mainToolBar->GetToolToggled( ID_TOOLBARH_PCB_MODE_TRACKS ) )
    {
        menu->Append( ID_POPUP_PCB_AUTOROUTE_PAD, _( "Automatically Route Pad" ) );
        menu->Append( ID_POPUP_PCB_AUTOROUTE_NET, _( "Automatically Route Net" ) );
    }

    MODULE* module = (MODULE*) Pad->GetParent();

    if( module )
    {
        menu->AppendSeparator();
        createPopUpMenuForFootprints( module, menu );
    }
}


// Create pop menu for pcb texts
void PCB_EDIT_FRAME::createPopUpMenuForTexts( TEXTE_PCB* Text, wxMenu* menu )
{
    wxMenu*  sub_menu_Text;
    int      flags = Text->GetFlags();

    wxString msg = Text->GetSelectMenuText();

    sub_menu_Text = new wxMenu;

    AddMenuItem( menu, sub_menu_Text, -1, msg, KiBitmap( add_text_xpm ) );

    if( !flags )
    {
        msg = AddHotkeyName( _( "Move" ), g_Board_Editor_Hokeys_Descr, HK_MOVE_ITEM );
        AddMenuItem( sub_menu_Text, ID_POPUP_PCB_MOVE_TEXTEPCB_REQUEST,
                     msg, KiBitmap( move_text_xpm ) );
        msg = AddHotkeyName( _( "Copy" ), g_Board_Editor_Hokeys_Descr, HK_COPY_ITEM );
        AddMenuItem( sub_menu_Text, ID_POPUP_PCB_COPY_TEXTEPCB,
                     msg, KiBitmap( copyblock_xpm ) );
    }

    msg = AddHotkeyName( _( "Rotate" ), g_Board_Editor_Hokeys_Descr, HK_ROTATE_ITEM );
    AddMenuItem( sub_menu_Text, ID_POPUP_PCB_ROTATE_TEXTEPCB, msg, KiBitmap( rotate_ccw_xpm ) );
    msg = AddHotkeyName( _( "Flip" ), g_Board_Editor_Hokeys_Descr, HK_FLIP_ITEM );
    AddMenuItem( sub_menu_Text, ID_POPUP_PCB_FLIP_TEXTEPCB, msg, KiBitmap( mirror_h_xpm ) );
    msg = AddHotkeyName( _( "Edit" ), g_Board_Editor_Hokeys_Descr, HK_EDIT_ITEM );
    AddMenuItem( sub_menu_Text, ID_POPUP_PCB_EDIT_TEXTEPCB, msg, KiBitmap( edit_text_xpm ) );
    if( !flags )
    {
        AddMenuItem( sub_menu_Text, ID_POPUP_PCB_RESET_TEXT_SIZE,
                     _( "Reset Size" ), KiBitmap( reset_text_xpm ) );

        sub_menu_Text->AppendSeparator();
        msg = AddHotkeyName( _( "Delete" ), g_Board_Editor_Hokeys_Descr, HK_DELETE );
        AddMenuItem( sub_menu_Text, ID_POPUP_PCB_DELETE_TEXTEPCB, msg, KiBitmap( delete_text_xpm ) );
    }
}


void PCB_EDIT_FRAME::createPopUpMenuForMarkers( MARKER_PCB* aMarker, wxMenu* aPopMenu )
{
    AddMenuItem( aPopMenu, ID_POPUP_PCB_DELETE_MARKER, _( "Delete Marker" ),
                 KiBitmap( delete_xpm ) );
    AddMenuItem( aPopMenu, ID_POPUP_PCB_GETINFO_MARKER, _( "Marker Error Info" ),
                 KiBitmap( info_xpm ) );
}


/**
 * Function Append_Track_Width_List
 * creates a wxMenu * which shows the last used track widths and via diameters
 * @return a pointer to the menu
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

    if( aBoard->GetDesignSettings().m_UseConnectedTrackWidth )
        trackwidth_menu->Check( ID_POPUP_PCB_SELECT_AUTO_WIDTH, true );

    if(  aBoard->GetDesignSettings().GetViaSizeIndex() != 0
      || aBoard->GetDesignSettings().GetTrackWidthIndex() != 0
      || aBoard->GetDesignSettings().m_UseConnectedTrackWidth )
        trackwidth_menu->Append( ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES,
                                 _( "Use Netclass Values" ),
                                 _( "Use track and via sizes from their Netclass values" ),
                                 true );

    for( unsigned ii = 0; ii < aBoard->GetDesignSettings().m_TrackWidthList.size(); ii++ )
    {
        value = StringFromValue( g_UserUnit, aBoard->GetDesignSettings().m_TrackWidthList[ii], true );
        msg.Printf( _( "Track %s" ), GetChars( value ) );

        if( ii == 0 )
            msg << _( " uses NetClass" );

        trackwidth_menu->Append( ID_POPUP_PCB_SELECT_WIDTH1 + ii, msg, wxEmptyString, true );
    }

    trackwidth_menu->AppendSeparator();

    for( unsigned ii = 0; ii < aBoard->GetDesignSettings().m_ViasDimensionsList.size(); ii++ )
    {
        value = StringFromValue( g_UserUnit,
                                 aBoard->GetDesignSettings().m_ViasDimensionsList[ii].m_Diameter,
                                 true );
        wxString drill = StringFromValue( g_UserUnit,
                                          aBoard->GetDesignSettings().m_ViasDimensionsList[ii].m_Drill,
                                          true );

        if( aBoard->GetDesignSettings().m_ViasDimensionsList[ii].m_Drill <= 0 )
        {
            msg.Printf( _( "Via %s" ), GetChars( value ) );
        }
        else
        {
            msg.Printf( _( "Via %s, drill %s" ), GetChars( value ), GetChars( drill ) );
        }

        if( ii == 0 )
            msg << _( " uses NetClass" );

        trackwidth_menu->Append( ID_POPUP_PCB_SELECT_VIASIZE1 + ii, msg, wxEmptyString, true );
    }

    return trackwidth_menu;
}
