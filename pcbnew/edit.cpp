/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pgm_base.h>
#include <kiface_i.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <eda_doc.h>
#include <gestfich.h>
#include <kicad_device_context.h>
#include <pcb_edit_frame.h>

#include <pcbnew_id.h>
#include <pcbnew.h>
#include <footprint_edit_frame.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_pcb_text.h>
#include <footprint_viewer_frame.h>
#include <pcb_layer_box_selector.h>
#include <dialog_drc.h>
#include <invoke_pcb_dialog.h>
#include <array_creator.h>
#include <connectivity/connectivity_data.h>

#include <zone_filler.h>

#include <dialog_move_exact.h>

#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>

// Handles the selection of command events.
void PCB_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    int         id = event.GetId();

    INSTALL_UNBUFFERED_DC( dc, m_canvas );
    MODULE* module;
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();

    m_canvas->CrossHairOff( &dc );

    switch( id )   // Some (not all ) edit commands must be finished or aborted
    {
    case wxID_CUT:
    case wxID_COPY:
    case ID_TOOLBARH_PCB_SELECT_LAYER:
    case ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR:
    case ID_POPUP_PCB_PLACE_THROUGH_VIA:
    case ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_THROUGH_VIA:
    case ID_POPUP_PCB_PLACE_BLIND_BURIED_VIA:
    case ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_BLIND_BURIED_VIA:
    case ID_POPUP_PCB_PLACE_MICROVIA:
    case ID_POPUP_PCB_SWITCH_TRACK_POSTURE:
    case ID_POPUP_PCB_FILL_ALL_ZONES:
    case ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_ALL_ZONES:
    case ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_CURRENT_ZONE:
    case ID_POPUP_PCB_FILL_ZONE:
        break;

    default:        // Finish (abort) the command
        if( m_canvas->IsMouseCaptured() )
            m_canvas->CallEndMouseCapture( &dc );

        if( GetToolId() != id )
        {
            if( m_lastDrawToolId != GetToolId() )
                m_lastDrawToolId = GetToolId();

            SetNoToolSelected();
        }
        break;
    }

    switch( id )   // Execute command
    {
    case 0:
        break;

    case ID_OPEN_MODULE_EDITOR:
        {
            FOOTPRINT_EDIT_FRAME* editor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_PCB_MODULE_EDITOR, false );

            if( !editor )
            {
                editor = (FOOTPRINT_EDIT_FRAME*) Kiway().Player( FRAME_PCB_MODULE_EDITOR, true );
                editor->Zoom_Automatique( false );
            }
            else
            {
                // Needed on Windows, other platforms do not use it,
                // but it creates no issue
                if( editor->IsIconized() )
                     editor->Iconize( false );

                editor->Raise();

                // Raising the window does not set the focus on Linux.  This should work on
                // any platform.
                if( wxWindow::FindFocus() != editor )
                    editor->SetFocus();
            }

            editor->PushPreferences( m_canvas );
        }
        break;

    case ID_OPEN_MODULE_VIEWER:
        {
            FOOTPRINT_VIEWER_FRAME* viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_PCB_MODULE_VIEWER, false );

            if( !viewer )
            {
                viewer = (FOOTPRINT_VIEWER_FRAME*) Kiway().Player( FRAME_PCB_MODULE_VIEWER, true );
            }
            else
            {
                // Needed on Windows, other platforms do not use it,
                // but it creates no issue
                if( viewer->IsIconized() )
                     viewer->Iconize( false );

                viewer->Raise();

                // Raising the window does not set the focus on Linux.  This should work on
                // any platform.
                if( wxWindow::FindFocus() != viewer )
                    viewer->SetFocus();
            }

            viewer->PushPreferences( m_canvas );
        }
        break;

    case ID_PCB_GLOBAL_DELETE:
        InstallPcbGlobalDeleteFrame( wxDefaultPosition );
        break;

    case ID_DRC_CONTROL:
        // Shows the DRC dialog in non modal mode, to allows board editing
        // with the DRC dialog opened and showing errors.
        m_drc->ShowDRCDialog();
        break;

    case ID_GET_NETLIST:
        InstallNetlistFrame( &dc );
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
        // fall through
    case ID_POPUP_PCB_PLACE_BLIND_BURIED_VIA:
    case ID_POPUP_PCB_PLACE_THROUGH_VIA:
    case ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_THROUGH_VIA:
    case ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_BLIND_BURIED_VIA:
        m_canvas->MoveCursorToCrossHair();

        if( GetCurItem()->IsDragging() )
        {
            // JEY TODO: reachable?
            PlaceDraggedOrMovedTrackSegment( (TRACK*) GetCurItem(), &dc );
        }
        else
        {
            BOARD_DESIGN_SETTINGS &settings = GetDesignSettings();
            VIATYPE_T v_type = settings.m_CurrentViaType;
            switch( id )
            {
            case ID_POPUP_PCB_PLACE_BLIND_BURIED_VIA:
            case ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_BLIND_BURIED_VIA:
                settings.m_CurrentViaType = VIA_BLIND_BURIED;
                break;

            case ID_POPUP_PCB_PLACE_MICROVIA:
                settings.m_CurrentViaType = VIA_MICROVIA;
                break;

            default:
                settings.m_CurrentViaType = VIA_THROUGH;
                break;
            }

            // place via and switch layer.
            if( id == ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_THROUGH_VIA ||
                id == ID_POPUP_PCB_SELECT_CU_LAYER_AND_PLACE_BLIND_BURIED_VIA )
            {
                m_canvas->SetIgnoreMouseEvents( true );

                wxPoint dlgPosition;

                wxGetMousePosition( &dlgPosition.x, &dlgPosition.y );

                PCB_LAYER_ID layer = SelectLayer( GetActiveLayer(), LSET::AllNonCuMask(), dlgPosition );

                m_canvas->SetIgnoreMouseEvents( false );
                m_canvas->MoveCursorToCrossHair();

                if( GetActiveLayer() != layer )
                {
                    GetScreen()->m_Route_Layer_TOP    = GetActiveLayer();
                    GetScreen()->m_Route_Layer_BOTTOM = layer;
                    Other_Layer_Route( (TRACK*) GetCurItem(), &dc );
                }
            }

            else
                Other_Layer_Route( (TRACK*) GetCurItem(), &dc );

            settings.m_CurrentViaType = v_type;

            if( displ_opts->m_ContrastModeDisplay )
                m_canvas->Refresh();
        }
        break;

    case ID_POPUP_PCB_FILL_ALL_ZONES:
        m_canvas->MoveCursorToCrossHair();
        Fill_All_Zones();
        m_canvas->Refresh();
        SetMsgPanel( GetBoard() );
        break;

    case ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_CURRENT_ZONE:
        if( ( GetCurItem() )->Type() == PCB_ZONE_AREA_T )
        {
            ZONE_CONTAINER* zone_container = (ZONE_CONTAINER*) GetCurItem();
            zone_container->UnFill();
            GetBoard()->GetConnectivity()->Update( zone_container );
            OnModify();
            SetMsgPanel( GetBoard() );
            m_canvas->Refresh();
        }

        Compile_Ratsnest( &dc, false );
        SetCurItem( NULL );
        break;

    case ID_POPUP_PCB_REMOVE_FILLED_AREAS_IN_ALL_ZONES: // Remove all zones :
        for( int ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
        {
            // Remove filled areas in zone
            ZONE_CONTAINER* zone_container = GetBoard()->GetArea( ii );
            zone_container->UnFill();
            GetBoard()->GetConnectivity()->Update( zone_container );
        }

        Compile_Ratsnest( &dc, false );
        SetCurItem( NULL );        // CurItem might be deleted by this command, clear the pointer
        OnModify();
        SetMsgPanel( GetBoard() );
        m_canvas->Refresh();
        break;

    case ID_POPUP_PCB_FILL_ZONE:
    {
        m_canvas->MoveCursorToCrossHair();
        ZONE_FILLER filler( GetBoard() );
        filler.Fill( { (ZONE_CONTAINER*) GetCurItem() } );
        SetMsgPanel( GetBoard() );
        m_canvas->Refresh();
        break;
    }

    case ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR:
        SelectCopperLayerPair();
        break;

    case ID_TOOLBARH_PCB_SELECT_LAYER:
        SetActiveLayer( ToLAYER_ID( m_SelLayerBox->GetLayerSelection() ) );

        if( displ_opts->m_ContrastModeDisplay )
            m_canvas->Refresh( true );
        break;

    case ID_POPUP_PCB_MOVE_EXACT:
        moveExact();
        break;

    case ID_POPUP_PCB_CREATE_ARRAY:
        createArray();
        break;

    case ID_MENU_PCB_CLEAN:
        Clean_Pcb();
        break;

    case ID_MENU_PCB_UPDATE_FOOTPRINTS:
        InstallExchangeModuleFrame( dynamic_cast<MODULE*>( GetCurItem() ), true, false );
        break;

    case ID_MENU_PCB_EXCHANGE_FOOTPRINTS:
        InstallExchangeModuleFrame( dynamic_cast<MODULE*>( GetCurItem() ), false, false );
        break;

    case ID_MENU_PCB_SWAP_LAYERS:
        Swap_Layers( event );
        break;

    case ID_MENU_ARCHIVE_MODULES_IN_LIBRARY:
        ArchiveModulesOnBoard( false );
        break;

    case ID_MENU_CREATE_LIBRARY_AND_ARCHIVE_MODULES:
        ArchiveModulesOnBoard( true );
        break;

    case ID_GEN_IMPORT_GRAPHICS_FILE:
        InvokeDialogImportGfxBoard( this );
        m_canvas->Refresh();
        break;


    default:
        wxLogDebug( wxT( "PCB_EDIT_FRAME::Process_Special_Functions() unknown event id %d" ), id );
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
        Delete_Module( (MODULE*) Item, DC );
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

    case PCB_ZONE_AREA_T:
        {
            SetCurItem( NULL );
            int netcode = ( (ZONE_CONTAINER*) Item )->GetNetCode();
            Delete_Zone_Contour( DC, (ZONE_CONTAINER*) Item );
            TestNetConnection( NULL, netcode );
            SetMsgPanel( GetBoard() );
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
        wxLogDebug( wxT( "Remove: item type %d unknown." ), Item->Type() );
        break;
    }
}


void PCB_EDIT_FRAME::SwitchLayer( wxDC* DC, PCB_LAYER_ID layer )
{
    PCB_LAYER_ID curLayer = GetActiveLayer();
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();

    // Check if the specified layer matches the present layer
    if( layer == curLayer )
        return;

    // Copper layers cannot be selected unconditionally; how many
    // of those layers are currently enabled needs to be checked.
    if( IsCopperLayer( layer ) )
    {
        // If only one copper layer is enabled, the only such layer
        // that can be selected to is the "Back" layer (so the
        // selection of any other copper layer is disregarded).
        if( GetBoard()->GetCopperLayerCount() < 2 )
        {
            if( layer != B_Cu )
                return;
        }
        // If more than one copper layer is enabled, the "Copper"
        // and "Component" layers can be selected, but the total
        // number of copper layers determines which internal
        // layers are also capable of being selected.
        else
        {
            if( layer != B_Cu  &&  layer != F_Cu  && layer >= GetBoard()->GetCopperLayerCount() - 1 )
                return;
        }

        EDA_ITEM* current = GetScreen()->GetCurItem();

        // See if we are drawing a segment; if so, add a via?
        if( GetToolId() == ID_TRACK_BUTT && current )
        {
            if( current->Type() == PCB_TRACE_T && current->IsNew() )
            {
                // Want to set the routing layers so that it switches properly -
                // see the implementation of Other_Layer_Route - the working
                // layer is used to 'start' the via and set the layer masks appropriately.
                GetScreen()->m_Route_Layer_TOP    = curLayer;
                GetScreen()->m_Route_Layer_BOTTOM = layer;

                SetActiveLayer( curLayer );

                if( Other_Layer_Route( (TRACK*) GetScreen()->GetCurItem(), DC ) )
                {
                    if( displ_opts->m_ContrastModeDisplay )
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

    SetActiveLayer( layer );

    if( displ_opts->m_ContrastModeDisplay )
        m_canvas->Refresh();
}


void PCB_EDIT_FRAME::OnSelectTool( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();
    int lastToolID = GetToolId();

    INSTALL_UNBUFFERED_DC( dc, m_canvas );
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)GetDisplayOptions();

    // Stop the current command and deselect the current tool.
    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    switch( id )
    {
    case ID_NO_TOOL_SELECTED:
        SetNoToolSelected();
        break;

    case ID_ZOOM_SELECTION:
        // This tool is located on the main toolbar: switch it on or off on click on it
        if( lastToolID != ID_ZOOM_SELECTION )
            SetToolID( ID_ZOOM_SELECTION, wxCURSOR_MAGNIFIER, _( "Zoom to selection" ) );
        else
            SetNoToolSelected();
        break;

    case ID_TRACK_BUTT:
        if( Settings().m_legacyDrcOn )
            SetToolID( id, wxCURSOR_PENCIL, _( "Add tracks" ) );
        else
            SetToolID( id, wxCURSOR_QUESTION_ARROW, _( "Add tracks" ) );

        Compile_Ratsnest( &dc, true );
        break;

    case ID_PCB_MODULE_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add footprint" ) );
        break;

    case ID_PCB_ZONES_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add zones" ) );

        if( displ_opts->m_DisplayZonesMode != 0 )
            DisplayInfoMessage( this, _( "Warning: zone display is OFF!!!" ) );

        if( !GetBoard()->IsHighLightNetON() && (GetBoard()->GetHighLightNetCode() > 0 ) )
            HighLight( &dc );

        break;

    case ID_PCB_KEEPOUT_BUTT:
        SetToolID( id, wxCURSOR_PENCIL, _( "Add keepout" ) );
        break;

    case ID_PCB_TARGET_BUTT:
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
        SetToolID( id, wxCURSOR_HAND, _( "Add footprint" ) );
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

    case ID_LOCAL_RATSNEST_BUTT:
        SetToolID( id, wxCURSOR_HAND, _( "Select rats nest" ) );

        Compile_Ratsnest( &dc, true );

        break;

    // collect GAL-only tools here
    case ID_PCB_DRAW_VIA_BUTT:
    case ID_PCB_MEASUREMENT_TOOL:
        SetToolID( id, wxCURSOR_DEFAULT, _( "Unsupported tool in this canvas" ) );
        break;
    }
}


void PCB_EDIT_FRAME::moveExact()
{
    wxPoint         translation;
    double          rotation;
    ROTATION_ANCHOR rotationAnchor = ROTATE_AROUND_ITEM_ANCHOR;

    DIALOG_MOVE_EXACT dialog( this, translation, rotation, rotationAnchor );
    int ret = dialog.ShowModal();

    if( ret == wxID_OK )
    {
        if( BOARD_ITEM* item = GetScreen()->GetCurItem() )
        {
            // When a pad is modified, the full footprint is saved
            BOARD_ITEM* itemToSave = item;

            if( item->Type() == PCB_PAD_T )
                itemToSave = item->GetParent();

            // Could be moved or rotated
            SaveCopyInUndoList( itemToSave, UR_CHANGED );

            item->Move( translation );

            switch( rotationAnchor )
            {
            case ROTATE_AROUND_ITEM_ANCHOR:
                item->Rotate( item->GetPosition(), rotation );
                break;
            case ROTATE_AROUND_USER_ORIGIN:
                item->Rotate( GetScreen()->m_O_Curseur, rotation );
                break;
            case ROTATE_AROUND_AUX_ORIGIN:
                item->Rotate( GetAuxOrigin(), rotation );
                break;
            default:
                wxFAIL_MSG( "Rotation choice shouldn't have been available in this context." );
            }

            m_canvas->Refresh();
        }
    }

    m_canvas->MoveCursorToCrossHair();
}


class LEGACY_ARRAY_CREATOR: public ARRAY_CREATOR
{
public:

    LEGACY_ARRAY_CREATOR( PCB_BASE_EDIT_FRAME& editFrame ):
        ARRAY_CREATOR( editFrame ),
        m_item( m_parent.GetScreen()->GetCurItem() )
    {}

private:

    int getNumberOfItemsToArray() const override
    {
        // only handle single items
        return (m_item != NULL) ? 1 : 0;
    }

    BOARD_ITEM* getNthItemToArray( int n ) const override
    {
        wxASSERT_MSG( n == 0, "Legacy array tool can only handle a single item" );
        return m_item;
    }

    BOARD* getBoard() const override
    {
        return m_parent.GetBoard();
    }

    MODULE* getModule() const override
    {
        return dynamic_cast<MODULE*>( m_item->GetParent() );
    }

    wxPoint getRotationCentre() const override
    {
        return m_item->GetCenter();
    }

    void finalise() override
    {
        m_parent.GetCanvas()->Refresh();
    }

    BOARD_ITEM* m_item; // only have the one
};


void PCB_BASE_EDIT_FRAME::createArray()
{
    LEGACY_ARRAY_CREATOR array_creator( *this );

    array_creator.Invoke();
}
