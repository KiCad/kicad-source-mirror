/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file edit.cpp
 * @brief Edit PCB implementation.
 */

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <eda_doc.h>
#include <gestfich.h>
#include <kicad_device_context.h>
#include <wxPcbStruct.h>
#include <pcbcommon.h>

#include <pcbnew_id.h>
#include <pcbnew.h>
#include <module_editor_frame.h>
#include <protos.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>

#include <dialog_drc.h>

#include <dialog_global_edit_tracks_and_vias.h>

// Uncomment following line to enable wxBell() command (which beeps speaker)
// #include <wx/utils.h>

/* Handles the selection of command events. */
void PCB_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int         id = event.GetId();
    wxPoint     pos;

    int         itmp;
    INSTALL_UNBUFFERED_DC( dc, m_canvas );
    MODULE* module;

    m_canvas->CrossHairOff( &dc );

    wxGetMousePosition( &pos.x, &pos.y );

    pos.y += 20;

    switch( id )   // Some (not all ) edit commands must be finished or aborted
    {
    case wxID_CUT:
    case wxID_COPY:
    case ID_PCB_USER_GRID_SETUP:
    case ID_TOOLBARH_PCB_SELECT_LAYER:
    case ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR:
    case ID_POPUP_PCB_ROTATE_TEXTEPCB:
    case ID_POPUP_PCB_EDIT_TEXTEPCB:
    case ID_POPUP_PCB_EDIT_MIRE:
    case ID_POPUP_PCB_ROTATE_TEXTMODULE:
    case ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE:
    case ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE:
    case ID_POPUP_PCB_CHANGE_SIDE_MODULE:
    case ID_POPUP_PCB_EDIT_MODULE:
    case ID_POPUP_PCB_EDIT_TEXTMODULE:
    case ID_POPUP_PCB_STOP_CURRENT_DRAWING:
    case ID_POPUP_PCB_END_TRACK:
    case ID_POPUP_PCB_PLACE_VIA:
    case ID_POPUP_PCB_SWITCH_TRACK_POSTURE:
    case ID_POPUP_PCB_PLACE_MICROVIA:
    case ID_POPUP_PCB_IMPORT_PAD_SETTINGS:
    case ID_POPUP_PCB_EXPORT_PAD_SETTINGS:
    case ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS:
    case ID_POPUP_PCB_STOP_CURRENT_EDGE_ZONE:
    case ID_POPUP_PCB_DELETE_ZONE_LAST_CREATED_CORNER:
    case ID_POPUP_PCB_FILL_ALL_ZONES:
    case ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_ALL_ZONES:
    case ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_CURRENT_ZONE:
    case ID_POPUP_PCB_PLACE_ZONE_CORNER:
    case ID_POPUP_PCB_PLACE_ZONE_OUTLINES:
    case ID_POPUP_PCB_EDIT_ZONE_PARAMS:
    case ID_POPUP_PCB_DELETE_ZONE:
    case ID_POPUP_PCB_MOVE_ZONE_CORNER:
    case ID_POPUP_PCB_DRAG_ZONE_OUTLINE_SEGMENT:
    case ID_POPUP_PCB_MOVE_ZONE_OUTLINES:
    case ID_POPUP_PCB_ADD_ZONE_CORNER:
    case ID_POPUP_PCB_DELETE_TRACKSEG:
    case ID_POPUP_PCB_DELETE_TRACK:
    case ID_POPUP_PCB_DELETE_TRACKNET:
    case ID_POPUP_PCB_FILL_ZONE:
    case ID_POPUP_PCB_SELECT_LAYER:
    case ID_POPUP_PCB_SELECT_CU_LAYER:
    case ID_POPUP_PCB_SELECT_LAYER_PAIR:
    case ID_POPUP_PCB_SELECT_NO_CU_LAYER:
    case ID_POPUP_PCB_MOVE_TRACK_NODE:
    case ID_POPUP_PCB_DRAG_TRACK_SEGMENT_KEEP_SLOPE:
    case ID_POPUP_PCB_DRAG_TRACK_SEGMENT:
    case ID_POPUP_PCB_MOVE_TRACK_SEGMENT:
    case ID_POPUP_PCB_PLACE_MOVED_TRACK_NODE:
    case ID_POPUP_PCB_BREAK_TRACK:
    case ID_POPUP_PCB_EDIT_NET:
    case ID_POPUP_PCB_EDIT_TRACK:
    case ID_POPUP_PCB_EDIT_TRACKSEG:
    case ID_POPUP_PCB_LOCK_ON_TRACKSEG:
    case ID_POPUP_PCB_LOCK_OFF_TRACKSEG:
    case ID_POPUP_PCB_LOCK_ON_TRACK:
    case ID_POPUP_PCB_LOCK_OFF_TRACK:
    case ID_POPUP_PCB_LOCK_ON_NET:
    case ID_POPUP_PCB_LOCK_OFF_NET:
    case ID_POPUP_DELETE_BLOCK:
    case ID_POPUP_PLACE_BLOCK:
    case ID_POPUP_ZOOM_BLOCK:
    case ID_POPUP_FLIP_BLOCK:
    case ID_POPUP_ROTATE_BLOCK:
    case ID_POPUP_COPY_BLOCK:
    case ID_POPUP_PCB_EDIT_DRAWING:
    case ID_POPUP_PCB_GETINFO_MARKER:
    case ID_POPUP_PCB_MOVE_TEXT_DIMENSION_REQUEST:
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->EndMouseCapture();
        }

        /* Should not be executed, just in case */
        if( GetScreen()->m_BlockLocate.m_Command != BLOCK_IDLE )
        {
            GetScreen()->m_BlockLocate.m_Command = BLOCK_IDLE;
            GetScreen()->m_BlockLocate.m_State   = STATE_NO_BLOCK;
            GetScreen()->m_BlockLocate.ClearItemsList();
        }

        if( GetToolId() == ID_NO_TOOL_SELECTED )
            SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        else
            SetCursor( m_canvas->GetDefaultCursor() );

        break;

    default:        // Finish (abort) the command
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallEndMouseCapture( &dc );

        if( GetToolId() != id )
        {
            if( m_lastDrawToolId != GetToolId() )
                m_lastDrawToolId = GetToolId();

            SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        }
        break;
    }

    switch( id )   // Execute command
    {
    case 0:
        break;

    case ID_OPEN_MODULE_EDITOR:
        if( m_ModuleEditFrame == NULL )
        {
            m_ModuleEditFrame = new FOOTPRINT_EDIT_FRAME( this,
                                                          _( "Module Editor" ),
                                                          wxPoint( -1, -1 ),
                                                          wxSize( 600, 400 ) );
            m_ModuleEditFrame->Show( true );
            m_ModuleEditFrame->Zoom_Automatique( false );
        }
        else
        {
            if( m_ModuleEditFrame->IsIconized() )
                 m_ModuleEditFrame->Iconize( false );

            m_ModuleEditFrame->Raise();

            // Raising the window does not set the focus on Linux.  This should work on
            // any platform.
            if( wxWindow::FindFocus() != m_ModuleEditFrame )
                m_ModuleEditFrame->SetFocus();
        }

        break;

    case ID_PCB_GLOBAL_DELETE:
        InstallPcbGlobalDeleteFrame( pos );
        break;

    case ID_POPUP_PLACE_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_MOVE;
        m_canvas->SetAutoPanRequest( false );
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_COPY_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_COPY;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        m_canvas->SetAutoPanRequest( false );
        HandleBlockPlace( &dc );
        break;

    case ID_POPUP_ZOOM_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_ZOOM;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_DELETE_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_DELETE;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_ROTATE_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_ROTATE;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_POPUP_FLIP_BLOCK:
        GetScreen()->m_BlockLocate.m_Command = BLOCK_FLIP;
        GetScreen()->m_BlockLocate.SetMessageBlock( this );
        HandleBlockEnd( &dc );
        break;

    case ID_DRC_CONTROL:
        m_drc->ShowDialog();
        break;

    case ID_GET_NETLIST:
        InstallNetlistFrame( &dc );
        break;

    case ID_GET_TOOLS:

        // InstalloolsFrame(this, wxPoint(-1,-1) );
        break;

    case ID_FIND_ITEMS:
        InstallFindFrame( pos, &dc );
        break;

    case ID_POPUP_CLOSE_CURRENT_TOOL:
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        break;

    case ID_POPUP_CANCEL_CURRENT_COMMAND:
        break;

    case ID_POPUP_PCB_END_LINE:
        m_canvas->MoveCursorToCrossHair();

        //  EndSegment(&dc);
        break;

    case ID_POPUP_PCB_EDIT_TRACK:
        if( GetCurItem() == NULL )
            break;
        Edit_Track_Width( &dc, (TRACK*) GetCurItem() );
        m_canvas->MoveCursorToCrossHair();
        OnModify();
        break;

    case ID_POPUP_PCB_EDIT_TRACKSEG:
        if( GetCurItem() == NULL )
            break;
        Edit_TrackSegm_Width( &dc, (TRACK*) GetCurItem() );
        m_canvas->MoveCursorToCrossHair();
        OnModify();
        break;

    case ID_POPUP_PCB_EDIT_ALL_VIAS_AND_TRACK_SIZE:
        if( GetCurItem() == NULL )
            break;
        {
        int type = GetCurItem()->Type();

        if( type == PCB_TRACE_T || type == PCB_VIA_T )
        {
            BOARD_CONNECTED_ITEM*item = (BOARD_CONNECTED_ITEM*) GetCurItem();
            DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS dlg( this, item->GetNet() );
            dlg.ShowModal();
        }

        }
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_END_TRACK:
        m_canvas->MoveCursorToCrossHair();
        End_Route( (TRACK*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_PLACE_MOVED_TRACK_NODE:
        m_canvas->MoveCursorToCrossHair();

        if( GetCurItem()->IsDragging() )
        {
            PlaceDraggedOrMovedTrackSegment( (TRACK*) GetCurItem(), &dc );
        }

        break;

    case ID_POPUP_PCB_SWITCH_TRACK_POSTURE:
        /* change the position of initial segment when creating new tracks
         * switch from _/  to -\ .
         * If a track is in progress, it will be redrawn
        */
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallMouseCapture( &dc, wxDefaultPosition, false );

        g_Alternate_Track_Posture = !g_Alternate_Track_Posture;

        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallMouseCapture( &dc, wxDefaultPosition, false );

        break;

    case ID_POPUP_PCB_PLACE_MICROVIA:
        if( !IsMicroViaAcceptable() )
            break;

    case ID_POPUP_PCB_PLACE_VIA:
        m_canvas->MoveCursorToCrossHair();

        if( GetCurItem()->IsDragging() )
        {
            PlaceDraggedOrMovedTrackSegment( (TRACK*) GetCurItem(), &dc );
        }
        else
        {
            int v_type = GetDesignSettings().m_CurrentViaType;

             // place micro via and switch layer.
            if( id == ID_POPUP_PCB_PLACE_MICROVIA )
                GetDesignSettings().m_CurrentViaType = VIA_MICROVIA;

            Other_Layer_Route( (TRACK*) GetCurItem(), &dc );
            GetDesignSettings().m_CurrentViaType = v_type;

            if( DisplayOpt.ContrastModeDisplay )
                m_canvas->Refresh();
        }
        break;

    case ID_POPUP_PCB_DELETE_TRACKSEG:
        if( GetCurItem() == NULL )
            break;

        m_canvas->MoveCursorToCrossHair();
        SetCurItem( Delete_Segment( &dc, (TRACK*) GetCurItem() ) );
        OnModify();
        break;

    case ID_POPUP_PCB_DELETE_TRACK:
        if( GetCurItem() == NULL )
            break;
        m_canvas->MoveCursorToCrossHair();
        Delete_Track( &dc, (TRACK*) GetCurItem() );
        SetCurItem( NULL );
        OnModify();
        break;

    case ID_POPUP_PCB_DELETE_TRACKNET:
        m_canvas->MoveCursorToCrossHair();
        Delete_net( &dc, (TRACK*) GetCurItem() );
        SetCurItem( NULL );
        OnModify();
        break;

    case ID_POPUP_PCB_LOCK_ON_TRACKSEG:
        Attribut_Segment( (TRACK*) GetCurItem(), &dc, true );
        break;

    case ID_POPUP_PCB_LOCK_OFF_TRACKSEG:
        Attribut_Segment( (TRACK*) GetCurItem(), &dc, false );
        break;

    case ID_POPUP_PCB_LOCK_ON_TRACK:
        Attribut_Track( (TRACK*) GetCurItem(), &dc, true );
        break;

    case ID_POPUP_PCB_LOCK_OFF_TRACK:
        Attribut_Track( (TRACK*) GetCurItem(), &dc, false );
        break;

    case ID_POPUP_PCB_LOCK_ON_NET:
        Attribut_net( &dc, ( (TRACK*) GetCurItem() )->GetNet(), true );
        break;

    case ID_POPUP_PCB_LOCK_OFF_NET:
        Attribut_net( &dc, ( (TRACK*) GetCurItem() )->GetNet(), false );
        break;

    case ID_POPUP_PCB_SETFLAGS_TRACK_MNU:
        break;

    case ID_POPUP_PCB_DELETE_ZONE:
        m_canvas->MoveCursorToCrossHair();

        if( GetCurItem() == NULL )
            break;

        {
            SEGZONE* zsegm   = (SEGZONE*) GetCurItem();
            int      netcode = zsegm->GetNet();
            Delete_OldZone_Fill( zsegm );
            SetCurItem( NULL );
            TestNetConnection( NULL, netcode );
            OnModify();
            GetBoard()->DisplayInfo( this );
        }
        break;

    case ID_POPUP_PCB_EDIT_ZONE_PARAMS:
        Edit_Zone_Params( &dc, (ZONE_CONTAINER*) GetCurItem() );
        SetCurItem( NULL ); // Outlines can have changed
        break;

    case ID_POPUP_PCB_ZONE_ADD_SIMILAR_ZONE:
        m_canvas->MoveCursorToCrossHair();
        m_canvas->SetAutoPanRequest( true );
        Add_Similar_Zone( &dc, (ZONE_CONTAINER*) GetCurItem() );
        break;

    case ID_POPUP_PCB_ZONE_ADD_CUTOUT_ZONE:
        m_canvas->MoveCursorToCrossHair();
        m_canvas->SetAutoPanRequest( true );
        Add_Zone_Cutout( &dc, (ZONE_CONTAINER*) GetCurItem() );
        break;

    case ID_POPUP_PCB_DELETE_ZONE_CONTAINER:
    case ID_POPUP_PCB_DELETE_ZONE_CUTOUT:
        m_canvas->MoveCursorToCrossHair();
        {
            int netcode = ( (ZONE_CONTAINER*) GetCurItem() )->GetNet();
            Delete_Zone_Contour( &dc, (ZONE_CONTAINER*) GetCurItem() );
            SetCurItem( NULL );
            TestNetConnection( NULL, netcode );
            GetBoard()->DisplayInfo( this );
        }
        break;

    case ID_POPUP_PCB_DELETE_ZONE_CORNER:
        Remove_Zone_Corner( &dc, (ZONE_CONTAINER*) GetCurItem() );
        SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_MOVE_ZONE_CORNER:
    {
        m_canvas->MoveCursorToCrossHair();
        ZONE_CONTAINER* zone_cont = (ZONE_CONTAINER*) GetCurItem();
        m_canvas->SetAutoPanRequest( true );
        Start_Move_Zone_Corner( &dc, zone_cont, zone_cont->m_CornerSelection, false );
        break;
    }

    case ID_POPUP_PCB_DRAG_ZONE_OUTLINE_SEGMENT:
    {
        m_canvas->MoveCursorToCrossHair();
        ZONE_CONTAINER* zone_cont = (ZONE_CONTAINER*) GetCurItem();
        m_canvas->SetAutoPanRequest( true );
        Start_Move_Zone_Drag_Outline_Edge( &dc, zone_cont, zone_cont->m_CornerSelection );
        break;
    }

    case ID_POPUP_PCB_MOVE_ZONE_OUTLINES:
    {
        m_canvas->MoveCursorToCrossHair();
        ZONE_CONTAINER* zone_cont = (ZONE_CONTAINER*) GetCurItem();
        m_canvas->SetAutoPanRequest( true );
        Start_Move_Zone_Outlines( &dc, zone_cont );
        break;
    }

    case ID_POPUP_PCB_ADD_ZONE_CORNER:
    {
        m_canvas->MoveCursorToCrossHair();
        ZONE_CONTAINER* zone_cont = (ZONE_CONTAINER*) GetCurItem();
        wxPoint         pos = GetScreen()->GetCrossHairPosition();

        /* add corner between zone_cont->m_CornerSelection
         * and zone_cont->m_CornerSelection+1
         * and start move the new corner
         */
        zone_cont->Draw( m_canvas, &dc, GR_XOR );
        zone_cont->m_Poly->InsertCorner( zone_cont->m_CornerSelection, pos.x, pos.y );
        zone_cont->m_CornerSelection++;
        zone_cont->Draw( m_canvas, &dc, GR_XOR );
        m_canvas->SetAutoPanRequest( true );
        Start_Move_Zone_Corner( &dc, zone_cont, zone_cont->m_CornerSelection, true );
        break;
    }

    case ID_POPUP_PCB_PLACE_ZONE_OUTLINES:
    case ID_POPUP_PCB_PLACE_ZONE_CORNER:
    {
        m_canvas->MoveCursorToCrossHair();
        ZONE_CONTAINER* zone_cont = (ZONE_CONTAINER*) GetCurItem();
        End_Move_Zone_Corner_Or_Outlines( &dc, zone_cont );
        m_canvas->SetAutoPanRequest( false );
        break;
    }

    case ID_POPUP_PCB_FILL_ALL_ZONES:
        m_canvas->MoveCursorToCrossHair();
        Fill_All_Zones( this );
        m_canvas->Refresh();
        GetBoard()->DisplayInfo( this );
        break;

    case ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_CURRENT_ZONE:
        if( ( GetCurItem() )->Type() == PCB_ZONE_AREA_T )
        {
            ZONE_CONTAINER* zone_container = (ZONE_CONTAINER*) GetCurItem();
            zone_container->UnFill();
            TestNetConnection( NULL, zone_container->GetNet() );
            OnModify();
            GetBoard()->DisplayInfo( this );
            m_canvas->Refresh();
        }
        SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_ALL_ZONES: // Remove all zones :
        GetBoard()->m_Zone.DeleteAll();                 // remove zone segments used to fill zones.

        for( int ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
        {
            // Remove filled areas in zone
            ZONE_CONTAINER* zone_container = GetBoard()->GetArea( ii );
            zone_container->m_FilledPolysList.clear();
        }

        SetCurItem( NULL );        // CurItem might be deleted by this command, clear the pointer
        TestConnections();
        TestForActiveLinksInRatsnest( 0 );   // Recalculate the active ratsnest, i.e. the unconnected links
        OnModify();
        GetBoard()->DisplayInfo( this );
        m_canvas->Refresh();
        break;

    case ID_POPUP_PCB_FILL_ZONE:
        m_canvas->MoveCursorToCrossHair();
        Fill_Zone( (ZONE_CONTAINER*) GetCurItem() );
        TestNetConnection( NULL, ( (ZONE_CONTAINER*) GetCurItem() )->GetNet() );
        GetBoard()->DisplayInfo( this );
        m_canvas->Refresh();
        break;

    case ID_POPUP_PCB_MOVE_TEXTEPCB_REQUEST:
        StartMoveTextePcb( (TEXTE_PCB*) GetCurItem(), &dc );
        m_canvas->SetAutoPanRequest( true );
        break;

    case ID_POPUP_PCB_DRAG_MODULE_REQUEST:
        g_Drag_Pistes_On = true;

    case ID_POPUP_PCB_MOVE_MODULE_REQUEST:
        if( GetCurItem() == NULL )
            break;

        // If the current Item is a pad, text module ...: Get its parent
        if( GetCurItem()->Type() != PCB_MODULE_T )
            SetCurItem( GetCurItem()->GetParent() );

        if( !GetCurItem() || GetCurItem()->Type() != PCB_MODULE_T )
        {
            g_Drag_Pistes_On = false;
            break;
        }

        module = (MODULE*) GetCurItem();

        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "Footprint %s found, but it is locked" ),
                        module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }

        SendMessageToEESCHEMA( module );
        GetScreen()->SetCrossHairPosition( module->m_Pos );
        m_canvas->MoveCursorToCrossHair();
        StartMove_Module( module, &dc );
        break;

    case ID_POPUP_PCB_GET_AND_MOVE_MODULE_REQUEST:      /* get module by name and move it */
        SetCurItem( GetModuleByName() );
        module = (MODULE*) GetCurItem();

        if( module == NULL )
            break;

        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "Footprint %s found, but it is locked" ),
                        module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }

        SendMessageToEESCHEMA( module );
        m_canvas->MoveCursorToCrossHair();
        StartMove_Module( module, &dc );
        break;

    case ID_POPUP_PCB_DELETE_MODULE:
        m_canvas->MoveCursorToCrossHair();

        // If the current Item is a pad, text module ...: Get its parent
        if( GetCurItem()->Type() != PCB_MODULE_T )
            SetCurItem( GetCurItem()->GetParent() );

        if( !GetCurItem() || GetCurItem()->Type() != PCB_MODULE_T )
            break;

        module = (MODULE*) GetCurItem();

        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "Footprint %s found, but it is locked" ),
                        module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }

        if( Delete_Module( (MODULE*) GetCurItem(), &dc, true ) )
        {
            SetCurItem( NULL );
        }

        break;

    case ID_POPUP_PCB_ROTATE_MODULE_COUNTERCLOCKWISE:
        m_canvas->MoveCursorToCrossHair();

        // If the current Item is a pad, text module ...: Get its parent
        if( GetCurItem()->Type() != PCB_MODULE_T )
            SetCurItem( GetCurItem()->GetParent() );

        if( !GetCurItem() || GetCurItem()->Type() != PCB_MODULE_T )
            break;

        module = (MODULE*) GetCurItem();

        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "Footprint %s found, but it is locked" ),
                        module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }

        /* This is a simple rotation, no other editing in progress */
        if( !GetCurItem()->IsMoving() )
            SaveCopyInUndoList(GetCurItem(), UR_ROTATED, ((MODULE*)GetCurItem())->m_Pos);

        Rotate_Module( &dc, (MODULE*) GetCurItem(), g_RotationAngle, true );
        break;

    case ID_POPUP_PCB_ROTATE_MODULE_CLOCKWISE:
        m_canvas->MoveCursorToCrossHair();

        // If the current Item is a pad, text module ...: Get its parent
        if( GetCurItem()->Type() != PCB_MODULE_T )
            SetCurItem( GetCurItem()->GetParent() );

        if( !GetCurItem() || GetCurItem()->Type() != PCB_MODULE_T )
            break;

        module = (MODULE*) GetCurItem();

        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "Footprint %s found, but it is locked" ),
                        module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }

        /* This is a simple rotation, no other editing in progress */
        if( !GetCurItem()->IsMoving() )
            SaveCopyInUndoList( GetCurItem(), UR_ROTATED_CLOCKWISE,
                                ((MODULE*)GetCurItem())->m_Pos );

        Rotate_Module( &dc, (MODULE*) GetCurItem(), -g_RotationAngle, true );
        break;

    case ID_POPUP_PCB_CHANGE_SIDE_MODULE:
        m_canvas->MoveCursorToCrossHair();

        // If the current Item is a pad, text module ...: Get its parent
        if( GetCurItem()->Type() != PCB_MODULE_T )
            SetCurItem( GetCurItem()->GetParent() );

        if( !GetCurItem() || GetCurItem()->Type() != PCB_MODULE_T )
            break;

        module = (MODULE*) GetCurItem();

        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "Footprint %s found, but it is locked" ),
                        module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }

        /* This is a simple flip, no other editing in progress */
        if( !GetCurItem()->IsMoving() )
            SaveCopyInUndoList(GetCurItem(), UR_FLIPPED, ((MODULE*)GetCurItem())->m_Pos);

        Change_Side_Module( (MODULE*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_EDIT_MODULE:

        // If the current Item is a pad, text module ...: Get its parent
        if( GetCurItem()->Type() != PCB_MODULE_T )
            SetCurItem( GetCurItem()->GetParent() );

        if( !GetCurItem() || GetCurItem()->Type() != PCB_MODULE_T )
            break;

        InstallModuleOptionsFrame( (MODULE*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_DRAG_PAD_REQUEST:
        module = (MODULE*) GetCurItem()->GetParent();

        if( !module || module->Type() != PCB_MODULE_T )
            break;

        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "The parent (%s) of the pad is locked" ),
                        module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }

        g_Drag_Pistes_On = true;
        m_canvas->MoveCursorToCrossHair();
        StartMovePad( (D_PAD*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_MOVE_PAD_REQUEST:
        module = (MODULE*) GetCurItem()->GetParent();

        if( !module || module->Type() != PCB_MODULE_T )
            break;

        if( module->IsLocked() )
        {
            wxString msg;
            msg.Printf( _( "The parent (%s) of the pad is locked" ),
                        module->m_Reference->m_Text.GetData() );
            DisplayInfoMessage( this, msg );
            break;
        }

        g_Drag_Pistes_On = false;
        m_canvas->MoveCursorToCrossHair();
        StartMovePad( (D_PAD*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_EDIT_PAD:
        InstallPadOptionsFrame( (D_PAD*) GetCurItem() );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_IMPORT_PAD_SETTINGS:
        m_canvas->MoveCursorToCrossHair();
        SaveCopyInUndoList( GetCurItem()->GetParent(), UR_CHANGED );
        Import_Pad_Settings( (D_PAD*) GetCurItem(), true );
        break;

    case ID_POPUP_PCB_GLOBAL_IMPORT_PAD_SETTINGS:
        m_canvas->MoveCursorToCrossHair();
        DlgGlobalChange_PadSettings( (D_PAD*) GetCurItem(), true );
        break;

    case ID_POPUP_PCB_EXPORT_PAD_SETTINGS:
        m_canvas->MoveCursorToCrossHair();
        Export_Pad_Settings( (D_PAD*) GetCurItem() );
        break;

    case ID_POPUP_PCB_DELETE_PAD:
        SaveCopyInUndoList( GetCurItem()->GetParent(), UR_CHANGED );
        DeletePad( (D_PAD*) GetCurItem() );
        SetCurItem( NULL );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_EDIT_TEXTMODULE:
        InstallTextModOptionsFrame( (TEXTE_MODULE*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_RESET_TEXT_SIZE:
        ResetTextSize( GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_MOVE_TEXTMODULE_REQUEST:
        m_canvas->MoveCursorToCrossHair();
        StartMoveTexteModule( (TEXTE_MODULE*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_ROTATE_TEXTMODULE:
        RotateTextModule( (TEXTE_MODULE*) GetCurItem(),
                         &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_DELETE_TEXTMODULE:
        DeleteTextModule( (TEXTE_MODULE*) GetCurItem() );
        SetCurItem( NULL );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_SELECT_LAYER:
        itmp = SelectLayer( getActiveLayer(), -1, -1 );

        if( itmp >= 0 )
            setActiveLayer( itmp );

        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR:
        SelectLayerPair();
        break;

    case ID_POPUP_PCB_SELECT_NO_CU_LAYER:
        itmp = SelectLayer( getActiveLayer(), FIRST_NO_COPPER_LAYER, -1 );

        if( itmp >= 0 )
            setActiveLayer( itmp );

        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_SELECT_CU_LAYER:
        itmp = SelectLayer( getActiveLayer(), -1, LAST_COPPER_LAYER );

        if( itmp >= 0 )
            setActiveLayer( itmp );

        break;

    case ID_POPUP_PCB_SELECT_LAYER_PAIR:
        SelectLayerPair();
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_TOOLBARH_PCB_SELECT_LAYER:
        setActiveLayer( (size_t) m_SelLayerBox->GetLayerSelection());

        if( DisplayOpt.ContrastModeDisplay )
            m_canvas->Refresh( true );

        break;

    case ID_POPUP_PCB_EDIT_TEXTEPCB:
        InstallTextPCBOptionsFrame( (TEXTE_PCB*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_ROTATE_TEXTEPCB:
        Rotate_Texte_Pcb( (TEXTE_PCB*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_DELETE_TEXTEPCB:
        Delete_Texte_Pcb( (TEXTE_PCB*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_MOVE_MIRE_REQUEST:
        BeginMoveTarget( (PCB_TARGET*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_EDIT_MIRE:
        ShowTargetOptionsDialog( (PCB_TARGET*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_DELETE_MIRE:
        m_canvas->MoveCursorToCrossHair();
        DeleteTarget( (PCB_TARGET*) GetCurItem(), &dc );
        SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_DELETE_DIMENSION:
        m_canvas->MoveCursorToCrossHair();
        DeleteDimension( (DIMENSION*) GetCurItem(), &dc );
        SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_EDIT_DIMENSION:
        ShowDimensionPropertyDialog( (DIMENSION*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_MOVE_TEXT_DIMENSION_REQUEST:
        BeginMoveDimensionText( (DIMENSION*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_DELETE_DRAWING:
        Delete_Segment_Edge( (DRAWSEGMENT*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_DELETE_MARKER:
        RemoveStruct( GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_GETINFO_MARKER:
        if( GetCurItem() && GetCurItem()->Type() == PCB_MARKER_T )
            ( (MARKER_PCB*) GetCurItem() )->DisplayMarkerInfo( this );

        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_DELETE_DRAWING_LAYER:
        if( GetCurItem()->GetFlags() != 0 )
            break;

        Delete_Drawings_All_Layer( GetCurItem()->GetLayer() );
        SetCurItem( NULL );
        m_canvas->MoveCursorToCrossHair();
        m_canvas->Refresh();
        break;

    case ID_POPUP_PCB_EDIT_DRAWING:
        InstallGraphicItemPropertiesDialog( (DRAWSEGMENT*) GetCurItem(), &dc );
        m_canvas->MoveCursorToCrossHair();
        break;

    case ID_POPUP_PCB_MOVE_DRAWING_REQUEST:
        m_canvas->MoveCursorToCrossHair();
        Start_Move_DrawItem( (DRAWSEGMENT*) GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_STOP_CURRENT_DRAWING:
        m_canvas->MoveCursorToCrossHair();

        if( GetCurItem() && (GetCurItem()->IsNew()) )
        {
            End_Edge( (DRAWSEGMENT*) GetCurItem(), &dc );
            SetCurItem( NULL );
        }

        break;

    case ID_POPUP_PCB_STOP_CURRENT_EDGE_ZONE:
        m_canvas->MoveCursorToCrossHair();

        if( GetCurItem() && (GetCurItem()->IsNew()) )
        {
            if( End_Zone( &dc ) )
                SetCurItem( NULL );
        }

        m_canvas->SetAutoPanRequest( false );
        break;

    case ID_POPUP_PCB_DELETE_ZONE_LAST_CREATED_CORNER:
        m_canvas->MoveCursorToCrossHair();

        if( GetCurItem() && (GetCurItem()->IsNew()) )
        {
            if( Delete_LastCreatedCorner( &dc ) == 0 )  // No more segment in outline,
                SetCurItem( NULL );
        }

        break;


    case ID_POPUP_PCB_MOVE_TRACK_SEGMENT:
        m_canvas->MoveCursorToCrossHair();
        StartMoveOneNodeOrSegment( (TRACK*) GetScreen()->GetCurItem(), &dc, id );
        break;

    case ID_POPUP_PCB_DRAG_TRACK_SEGMENT:
    case ID_POPUP_PCB_MOVE_TRACK_NODE:
        m_canvas->MoveCursorToCrossHair();
        StartMoveOneNodeOrSegment( (TRACK*) GetScreen()->GetCurItem(), &dc, id );
        break;

    case ID_POPUP_PCB_DRAG_TRACK_SEGMENT_KEEP_SLOPE:
        m_canvas->MoveCursorToCrossHair();
        Start_DragTrackSegmentAndKeepSlope( (TRACK*) GetScreen()->GetCurItem(), &dc );
        break;

    case ID_POPUP_PCB_BREAK_TRACK:
        m_canvas->MoveCursorToCrossHair();
        {
            TRACK*  track = (TRACK*) GetScreen()->GetCurItem();
            wxPoint pos   = GetScreen()->GetCrossHairPosition();
            track->Draw( m_canvas, &dc, GR_XOR );
            PICKED_ITEMS_LIST itemsListPicker;
            TRACK*  newtrack = GetBoard()->CreateLockPoint( pos, track, &itemsListPicker );
            SaveCopyInUndoList( itemsListPicker, UR_UNSPECIFIED );
            track->Draw( m_canvas, &dc, GR_XOR );
            newtrack->Draw( m_canvas, &dc, GR_XOR );
            /* compute the new ratsnest, because connectivity could change */
            TestNetConnection( &dc, track->GetNet() );
        }
        break;

    case ID_MENU_PCB_CLEAN:
        Clean_Pcb( &dc );
        break;

    case ID_MENU_PCB_SWAP_LAYERS:
        Swap_Layers( event );
        break;

    case ID_MENU_PCB_RESET_TEXTMODULE_REFERENCE_SIZES:
        ResetModuleTextSizes( TEXT_is_REFERENCE, &dc );
        break;

    case ID_MENU_PCB_RESET_TEXTMODULE_VALUE_SIZES:
        ResetModuleTextSizes( TEXT_is_VALUE, &dc );
        break;

    case ID_PCB_USER_GRID_SETUP:
        InstallGridFrame( pos );
        break;

    case ID_POPUP_PCB_DISPLAY_FOOTPRINT_DOC:
    {
        wxConfig* cfg = wxGetApp().GetCommonSettings();
        cfg->Read( wxT( "module_doc_file" ), g_DocModulesFileName );
        GetAssociatedDocument( this, g_DocModulesFileName, &wxGetApp().GetLibraryPathList() );
    }
    break;

    case ID_MENU_ARCHIVE_NEW_MODULES:
        ArchiveModulesOnBoard( wxEmptyString, true );
        break;

    case ID_MENU_ARCHIVE_ALL_MODULES:
        ArchiveModulesOnBoard( wxEmptyString, false );
        break;

    default:
        wxString msg;
        msg.Printf( wxT( "PCB_EDIT_FRAME::Process_Special_Functions() unknown event id %d" ),
                    id );
        DisplayError( this, msg );
        break;
    }

    m_canvas->CrossHairOn( &dc );
    m_canvas->SetIgnoreMouseEvents( false );
}


void PCB_EDIT_FRAME::RemoveStruct( BOARD_ITEM* Item, wxDC* DC )
{
    if( Item == NULL )
        return;

    switch( Item->Type() )
    {
    case PCB_MODULE_T:
        Delete_Module( (MODULE*) Item, DC, true );
        break;

    case PCB_DIMENSION_T:
        DeleteDimension( (DIMENSION*) Item, DC );
        break;

    case PCB_TARGET_T:
        DeleteTarget( (PCB_TARGET*) Item, DC );
        break;

    case PCB_LINE_T:
        Delete_Segment_Edge( (DRAWSEGMENT*) Item, DC );
        break;

    case PCB_TEXT_T:
        Delete_Texte_Pcb( (TEXTE_PCB*) Item, DC );
        break;

    case PCB_TRACE_T:
        Delete_Track( DC, (TRACK*) Item );
        break;

    case PCB_VIA_T:
        Delete_Segment( DC, (TRACK*) Item );
        break;

    case PCB_ZONE_T:
        Delete_OldZone_Fill( (SEGZONE*) Item );
        break;

    case PCB_ZONE_AREA_T:
    {
        SetCurItem( NULL );
        int netcode = ( (ZONE_CONTAINER*) Item )->GetNet();
        Delete_Zone_Contour( DC, (ZONE_CONTAINER*) Item );
        TestNetConnection( NULL, netcode );
        GetBoard()->DisplayInfo( this );
    }

    break;

    case PCB_MARKER_T:
        if( Item == GetCurItem() )
            SetCurItem( NULL );

        ( (MARKER_PCB*) Item )->Draw( m_canvas, DC, GR_XOR );

        // delete the marker, and free memory.  Don't use undo stack.
        GetBoard()->Delete( Item );
        break;

    case PCB_PAD_T:
    case PCB_MODULE_TEXT_T:
    case PCB_MODULE_EDGE_T:
        break;

    case TYPE_NOT_INIT:
    case PCB_T:
    default:
    {
        wxString Line;
        Line.Printf( wxT( "Remove: item type %d unknown." ), Item->Type() );
        DisplayError( this, Line );
    }
    break;
    }
}


void PCB_EDIT_FRAME::SwitchLayer( wxDC* DC, int layer )
{
    int curLayer = getActiveLayer();

    // Check if the specified layer matches the present layer
    if( layer == curLayer )
        return;

    // Copper layers cannot be selected unconditionally; how many
    // of those layers are currently enabled needs to be checked.
    if( IsValidCopperLayerIndex( layer ) )
    {
        // If only one copper layer is enabled, the only such layer
        // that can be selected to is the "Back" layer (so the
        // selection of any other copper layer is disregarded).
        if( GetBoard()->GetCopperLayerCount() < 2 )
        {
            if( layer != LAYER_N_BACK )
            {
                // Uncomment following command (and line 17) to beep
                // the speaker. (Doing that would provide feedback to
                // the user that the (layer-switching) command has been
                // "acknowledged", but is unable to be acted upon.)
//              wxBell();
                return;
            }
        }
        // If more than one copper layer is enabled, the "Copper"
        // and "Component" layers can be selected, but the total
        // number of copper layers determines which internal
        // layers are also capable of being selected.
        else
        {
            if( ( layer != LAYER_N_BACK ) && ( layer != LAYER_N_FRONT )
               && ( layer >= GetBoard()->GetCopperLayerCount() - 1 ) )
            {
                // Uncomment following command (and line 17) to beep
                // the speaker. (Doing that would provide feedback to
                // the user that the (layer-switching) command has been
                // "acknowledged", but is unable to be acted upon.)
//              wxBell();
                return;
            }
        }

        EDA_ITEM* current = GetScreen()->GetCurItem();

        // See if we are drawing a segment; if so, add a via?
        if( GetToolId() == ID_TRACK_BUTT && current != NULL )
        {
            if( current->Type() == PCB_TRACE_T && ( current->IsNew() ) )
            {
                // Want to set the routing layers so that it switches properly -
                // see the implementation of Other_Layer_Route - the working
                // layer is used to 'start' the via and set the layer masks appropriately.
                GetScreen()->m_Route_Layer_TOP    = curLayer;
                GetScreen()->m_Route_Layer_BOTTOM = layer;

                setActiveLayer( curLayer );

                if( Other_Layer_Route( (TRACK*) GetScreen()->GetCurItem(), DC ) )
                {
                    if( DisplayOpt.ContrastModeDisplay )
                        m_canvas->Refresh();
                }

                // if the via was allowed by DRC, then the layer swap has already
                // been done by Other_Layer_Route(). if via not allowed, then
                // return now so assignment to setActiveLayer() below doesn't happen.
                return;
            }
        }
    }

    // Is yet more checking required? E.g. when the layer to be selected
    // is a non-copper layer, or when switching between a copper layer
    // and a non-copper layer, or vice-versa?
    // ...

    setActiveLayer( layer );

    if( DisplayOpt.ContrastModeDisplay )
        m_canvas->Refresh();
}


void PCB_EDIT_FRAME::OnSelectTool( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();

    if( GetToolId() == id )
        return;

    INSTALL_UNBUFFERED_DC( dc, m_canvas );

    // Stop the current command and deselect the current tool.
    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    switch( id )
    {
    case ID_NO_TOOL_SELECTED:
        SetToolID( id, m_canvas->GetDefaultCursor(), wxEmptyString );
        break;

    case ID_TRACK_BUTT:
        if( Drc_On )
            SetToolID( id, wxCURSOR_PENCIL, _( "Add tracks" ) );
        else
            SetToolID( id, wxCURSOR_QUESTION_ARROW, _( "Add tracks" ) );

        if( (GetBoard()->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK) == 0 )
        {
            Compile_Ratsnest( &dc, true );
        }

        break;

    case ID_PCB_MODULE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add module" ) );
        break;

    case ID_PCB_ZONES_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add zones" ) );

        if( DisplayOpt.DisplayZonesMode != 0 )
            DisplayInfoMessage( this, _( "Warning: zone display is OFF!!!" ) );

        if( !GetBoard()->IsHighLightNetON() && (GetBoard()->GetHighLightNetCode() > 0 ) )
            HighLight( &dc );

        break;

    case ID_PCB_MIRE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add layer alignment target" ) );
        break;

    case ID_PCB_PLACE_OFFSET_COORD_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Adjust zero" ) );
        break;

    case ID_PCB_PLACE_GRID_COORD_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Adjust grid origin" ) );
        break;

    case ID_PCB_ADD_LINE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add graphic line" ) );
        break;

    case ID_PCB_ARC_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add graphic arc" ) );
        break;

    case ID_PCB_CIRCLE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add graphic circle" ) );
        break;

    case ID_PCB_ADD_TEXT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add text" ) );
        break;

    case ID_COMPONENT_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Add module" ) );
        break;

    case ID_PCB_DIMENSION_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add dimension" ) );
        break;

    case ID_PCB_DELETE_ITEM_BUTT:
        SetToolID( id, wxCURSOR_BULLSEYE, _( "Delete item" ) );
        break;

    case ID_PCB_HIGHLIGHT_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Highlight net" ) );
        break;

    case ID_PCB_SHOW_1_RATSNEST_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Select rats nest" ) );

        if( ( GetBoard()->m_Status_Pcb & LISTE_RATSNEST_ITEM_OK ) == 0 )
            Compile_Ratsnest( &dc, true );

        break;
    }
}
