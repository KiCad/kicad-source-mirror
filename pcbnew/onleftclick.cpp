/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
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
 * @file pcbnew/onleftclick.cpp
 * @brief Functions called when the left button is clicked or double clicked.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <msgpanel.h>

#include <class_board.h>
#include <class_drawsegment.h>
#include <class_dimension.h>
#include <class_zone.h>
#include <class_pcb_text.h>
#include <class_text_mod.h>
#include <class_module.h>
#include <class_mire.h>
#include <project.h>

#include <pcbnew.h>
#include <pcbnew_id.h>
#include <menus_helpers.h>


/* Handle the left button mouse click, when a tool is active
 */
void PCB_EDIT_FRAME::OnLeftClick( wxDC* aDC, const wxPoint& aPosition )
{
    BOARD_ITEM* DrawStruct = GetCurItem();
    bool        exit = false;
    bool no_tool = GetToolId() == ID_NO_TOOL_SELECTED;

    if( no_tool || ( DrawStruct && DrawStruct->GetFlags() ) )
    {
        m_canvas->SetAutoPanRequest( false );

        if( DrawStruct && DrawStruct->GetFlags() ) // Command in progress
        {
            m_canvas->SetIgnoreMouseEvents( true );
            m_canvas->CrossHairOff( aDC );

            switch( DrawStruct->Type() )
            {
            case PCB_ZONE_AREA_T:
                if( DrawStruct->IsNew() )
                {
                    m_canvas->SetAutoPanRequest( true );
                    Begin_Zone( aDC );
                }
                else
                {
                    End_Move_Zone_Corner_Or_Outlines( aDC, static_cast<ZONE_CONTAINER*>( DrawStruct ) );
                }

                exit = true;
                break;

            case PCB_TRACE_T:
            case PCB_VIA_T:
                if( DrawStruct->IsDragging() )
                {
                    PlaceDraggedOrMovedTrackSegment( static_cast<TRACK*>( DrawStruct ), aDC );
                    exit = true;
                }

                break;

            case PCB_TEXT_T:
                Place_Texte_Pcb( static_cast<TEXTE_PCB*>( DrawStruct ), aDC );
                exit = true;
                break;

            case PCB_MODULE_TEXT_T:
                PlaceTexteModule( static_cast<TEXTE_MODULE*>( DrawStruct ), aDC );
                exit = true;
                break;

            case PCB_PAD_T:
                PlacePad( static_cast<D_PAD*>( DrawStruct ), aDC );
                exit = true;
                break;

            case PCB_MODULE_T:
                PlaceModule( static_cast<MODULE*>( DrawStruct ), aDC );
                exit = true;
                break;

            case PCB_TARGET_T:
                PlaceTarget( static_cast<PCB_TARGET*>( DrawStruct ), aDC );
                exit = true;
                break;

            case PCB_LINE_T:
                if( no_tool )   // when no tools: existing item moving.
                {
                    Place_DrawItem( static_cast<DRAWSEGMENT*>( DrawStruct ), aDC );
                    exit = true;
                }

                break;

            case PCB_DIMENSION_T:
                if( ! DrawStruct->IsNew() )
                {   // We are moving the text of an existing dimension. Place it
                    PlaceDimensionText( static_cast<DIMENSION*>( DrawStruct ), aDC );
                    exit = true;
                }
                break;

            default:
                DisplayError( this,
                              wxT( "PCB_EDIT_FRAME::OnLeftClick() err: DrawType %d m_Flags != 0" ),
                              DrawStruct->Type() );
                exit = true;
                break;
            }

            m_canvas->SetIgnoreMouseEvents( false );
            m_canvas->CrossHairOn( aDC );

            if( exit )
                return;
        }
        else if( !wxGetKeyState( WXK_SHIFT ) && !wxGetKeyState( WXK_ALT )
                && !wxGetKeyState( WXK_CONTROL ) )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();

            if( DrawStruct )
                SendMessageToEESCHEMA( DrawStruct );
        }
    }

    if( DrawStruct ) // display netclass info for zones, tracks and pads
    {
        switch( DrawStruct->Type() )
        {
        case PCB_ZONE_AREA_T:
        case PCB_TRACE_T:
        case PCB_VIA_T:
        case PCB_PAD_T:
            GetDesignSettings().SetCurrentNetClass(
                ((BOARD_CONNECTED_ITEM*)DrawStruct)->GetNetClassName() );
            updateTraceWidthSelectBox();
            updateViaSizeSelectBox();
            break;

        default:
           break;
        }
    }

    switch( GetToolId() )
    {
    case ID_MAIN_MENUBAR:
    case ID_NO_TOOL_SELECTED:
        break;

    case ID_PCB_MUWAVE_TOOL_SELF_CMD:
    case ID_PCB_MUWAVE_TOOL_GAP_CMD:
    case ID_PCB_MUWAVE_TOOL_STUB_CMD:
    case ID_PCB_MUWAVE_TOOL_STUB_ARC_CMD:
    case ID_PCB_MUWAVE_TOOL_FUNCTION_SHAPE_CMD:
        MuWaveCommand( aDC, aPosition );
        break;

    case ID_PCB_HIGHLIGHT_BUTT:
    {
        int netcode = SelectHighLight( aDC );

        if( netcode < 0 )
            SetMsgPanel( GetBoard() );
        else
        {
            NETINFO_ITEM* net = GetBoard()->FindNet( netcode );

            if( net )
            {
                MSG_PANEL_ITEMS items;
                net->GetMsgPanelInfo( items );
                SetMsgPanel( items );
            }
        }
    }
    break;

    case ID_PCB_SHOW_1_RATSNEST_BUTT:
        DrawStruct = PcbGeneralLocateAndDisplay();
        Show_1_Ratsnest( DrawStruct, aDC );

        if( DrawStruct )
            SendMessageToEESCHEMA( DrawStruct );

        break;

    case ID_PCB_MIRE_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->GetFlags() == 0) )
        {
            SetCurItem( (BOARD_ITEM*) CreateTarget( aDC ) );
            m_canvas->MoveCursorToCrossHair();
        }
        else if( DrawStruct->Type() == PCB_TARGET_T )
        {
            PlaceTarget( (PCB_TARGET*) DrawStruct, aDC );
        }
        else
        {
            DisplayError( this, wxT( "OnLeftClick err: not a PCB_TARGET_T" ) );
        }

        break;

    case ID_PCB_CIRCLE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_ADD_LINE_BUTT:
        {
            STROKE_T shape = S_SEGMENT;

            if( GetToolId() == ID_PCB_CIRCLE_BUTT )
                shape = S_CIRCLE;

            if( GetToolId() == ID_PCB_ARC_BUTT )
                shape = S_ARC;

            if( IsCopperLayer( GetActiveLayer() ) )
            {
                DisplayError( this, _( "Graphic not allowed on Copper layers" ) );
                break;
            }

            if( (DrawStruct == NULL) || (DrawStruct->GetFlags() == 0) )
            {
                DrawStruct = (BOARD_ITEM*) Begin_DrawSegment( NULL, shape, aDC );
                SetCurItem( DrawStruct );
                m_canvas->SetAutoPanRequest( true );
            }
            else if( DrawStruct
                   && (DrawStruct->Type() == PCB_LINE_T)
                   && DrawStruct->IsNew() )
            {
                DrawStruct = (BOARD_ITEM*) Begin_DrawSegment( (DRAWSEGMENT*) DrawStruct, shape, aDC );
                SetCurItem( DrawStruct );
                m_canvas->SetAutoPanRequest( true );
            }
        }
        break;

    case ID_TRACK_BUTT:
        if( !IsCopperLayer( GetActiveLayer() ) )
        {
            DisplayError( this, _( "Tracks on Copper layers only " ) );
            break;
        }

        if( (DrawStruct == NULL) || (DrawStruct->GetFlags() == 0) )
        {
            DrawStruct = (BOARD_ITEM*) Begin_Route( NULL, aDC );
            SetCurItem( DrawStruct );

            if( DrawStruct )
                m_canvas->SetAutoPanRequest( true );
        }
        else if( DrawStruct && DrawStruct->IsNew() )
        {
            TRACK* track = Begin_Route( (TRACK*) DrawStruct, aDC );

            // SetCurItem() must not write to the msg panel
            // because a track info is displayed while moving the mouse cursor
            if( track )  // A new segment was created
                SetCurItem( DrawStruct = (BOARD_ITEM*) track, false );

            m_canvas->SetAutoPanRequest( true );
        }

        break;

    case ID_PCB_ZONES_BUTT:
    case ID_PCB_KEEPOUT_AREA_BUTT:
        /* ZONE or KEEPOUT Tool is selected. Determine action for a left click:
         *  this can be start a new zone or select and move an existing zone outline corner
         *  if found near the mouse cursor
         */
        if( (DrawStruct == NULL) || (DrawStruct->GetFlags() == 0) )
        {
            if( Begin_Zone( aDC ) )
            {
                m_canvas->SetAutoPanRequest( true );
                DrawStruct = GetBoard()->m_CurrentZoneContour;
                GetScreen()->SetCurItem( DrawStruct );
            }
        }
        else if( DrawStruct && (DrawStruct->Type() == PCB_ZONE_AREA_T) && DrawStruct->IsNew() )
        {   // Add a new corner to the current outline being created:
            m_canvas->SetAutoPanRequest( true );
            Begin_Zone( aDC );
            DrawStruct = GetBoard()->m_CurrentZoneContour;
            GetScreen()->SetCurItem( DrawStruct );
        }
        else
        {
            DisplayError( this, wxT( "PCB_EDIT_FRAME::OnLeftClick() zone internal error" ) );
        }

        break;

    case ID_PCB_ADD_TEXT_BUTT:
        if( Edge_Cuts == GetActiveLayer() )
        {
            DisplayError( this,
                          _( "Texts not allowed on Edge Cut layer" ) );
            break;
        }

        if( (DrawStruct == NULL) || (DrawStruct->GetFlags() == 0) )
        {
            SetCurItem( CreateTextePcb( aDC ) );
            m_canvas->MoveCursorToCrossHair();
            m_canvas->SetAutoPanRequest( true );
        }
        else if( DrawStruct->Type() == PCB_TEXT_T )
        {
            Place_Texte_Pcb( (TEXTE_PCB*) DrawStruct, aDC );
            m_canvas->SetAutoPanRequest( false );
        }
        else
        {
            DisplayError( this, wxT( "OnLeftClick err: not a PCB_TEXT_T" ) );
        }

        break;

    case ID_PCB_MODULE_BUTT:
        if( (DrawStruct == NULL) || (DrawStruct->GetFlags() == 0) )
        {
            m_canvas->MoveCursorToCrossHair();
            DrawStruct = (BOARD_ITEM*) LoadModuleFromLibrary(
                    wxEmptyString, Prj().PcbFootprintLibs(), true, aDC );

            SetCurItem( DrawStruct );

            if( DrawStruct )
                StartMoveModule( (MODULE*) DrawStruct, aDC, false );
        }
        else if( DrawStruct->Type() == PCB_MODULE_T )
        {
            PlaceModule( (MODULE*) DrawStruct, aDC );
            m_canvas->SetAutoPanRequest( false );
        }
        else
        {
            DisplayError( this, wxT( "Internal err: Struct not PCB_MODULE_T" ) );
        }

        break;

    case ID_PCB_DIMENSION_BUTT:
        if( IsCopperLayer( GetActiveLayer() ) || GetActiveLayer() == Edge_Cuts )
        {
            DisplayError( this, _( "Dimension not allowed on Copper or Edge Cut layers" ) );
            break;
        }

        if( !DrawStruct || !DrawStruct->GetFlags() )
        {
            DrawStruct = (BOARD_ITEM*) EditDimension( NULL, aDC );
            SetCurItem( DrawStruct );
            m_canvas->SetAutoPanRequest( true );
        }
        else if( DrawStruct && (DrawStruct->Type() == PCB_DIMENSION_T) && DrawStruct->IsNew() )
        {
            DrawStruct = (BOARD_ITEM*) EditDimension( (DIMENSION*) DrawStruct, aDC );
            SetCurItem( DrawStruct );
            m_canvas->SetAutoPanRequest( true );
        }
        else
        {
            DisplayError( this,
                          wxT( "PCB_EDIT_FRAME::OnLeftClick() error item is not a DIMENSION" ) );
        }

        break;

    case ID_PCB_DELETE_ITEM_BUTT:
        if( !DrawStruct || !DrawStruct->GetFlags() )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();

            if( DrawStruct && (DrawStruct->GetFlags() == 0) )
            {
                RemoveStruct( DrawStruct, aDC );
                SetCurItem( DrawStruct = NULL );
            }
        }

        break;

    case ID_PCB_PLACE_OFFSET_COORD_BUTT:
        m_canvas->DrawAuxiliaryAxis( aDC, GR_XOR );
        SetAuxOrigin( GetCrossHairPosition() );
        m_canvas->DrawAuxiliaryAxis( aDC, GR_COPY );
        OnModify();
        break;

    case ID_PCB_PLACE_GRID_COORD_BUTT:
        m_canvas->DrawGridAxis( aDC, GR_XOR, GetBoard()->GetGridOrigin() );
        SetGridOrigin( GetCrossHairPosition() );
        m_canvas->DrawGridAxis( aDC, GR_COPY, GetBoard()->GetGridOrigin() );
        break;

    default:
        DisplayError( this, wxT( "PCB_EDIT_FRAME::OnLeftClick() id error" ) );
        SetToolID( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor(), wxEmptyString );
        break;
    }
}


/* handle the double click on the mouse left button
 */
void PCB_EDIT_FRAME::OnLeftDClick( wxDC* aDC, const wxPoint& aPosition )
{
    BOARD_ITEM* DrawStruct = GetCurItem();

    switch( GetToolId() )
    {
    case ID_NO_TOOL_SELECTED:
        if( (DrawStruct == NULL) || (DrawStruct->GetFlags() == 0) )
        {
            DrawStruct = PcbGeneralLocateAndDisplay();
        }

        if( (DrawStruct == NULL) || (DrawStruct->GetFlags() != 0) )
            break;

        SendMessageToEESCHEMA( DrawStruct );

        // An item is found
        SetCurItem( DrawStruct );

        switch( DrawStruct->Type() )
        {
        case PCB_TRACE_T:
        case PCB_VIA_T:
            if( DrawStruct->IsNew() )
            {
                if( End_Route( (TRACK*) DrawStruct, aDC ) )
                    m_canvas->SetAutoPanRequest( false );
            }
            else if( DrawStruct->GetFlags() == 0 )
            {
                Edit_TrackSegm_Width( aDC, (TRACK*) DrawStruct );
            }

            break;

        case PCB_TEXT_T:
        case PCB_PAD_T:
        case PCB_MODULE_T:
        case PCB_TARGET_T:
        case PCB_DIMENSION_T:
        case PCB_MODULE_TEXT_T:
            OnEditItemRequest( aDC, DrawStruct );
            m_canvas->MoveCursorToCrossHair();
            break;

        case PCB_LINE_T:
            OnEditItemRequest( aDC, DrawStruct );
            break;

        case PCB_ZONE_AREA_T:
            if( DrawStruct->GetFlags() )
                break;

            OnEditItemRequest( aDC, DrawStruct );
            break;

        default:
            break;
        }

        break;      // end case 0

    case ID_TRACK_BUTT:
        if( DrawStruct && DrawStruct->IsNew() )
        {
            if( End_Route( (TRACK*) DrawStruct, aDC ) )
                m_canvas->SetAutoPanRequest( false );
        }

        break;

    case ID_PCB_ZONES_BUTT:
    case ID_PCB_KEEPOUT_AREA_BUTT:
        if( End_Zone( aDC ) )
        {
            m_canvas->SetAutoPanRequest( false );
            SetCurItem( NULL );
        }

        break;

    case ID_PCB_ADD_LINE_BUTT:
    case ID_PCB_ARC_BUTT:
    case ID_PCB_CIRCLE_BUTT:
        if( DrawStruct == NULL )
            break;

        if( DrawStruct->Type() != PCB_LINE_T )
        {
            DisplayError( this, wxT( "DrawStruct Type error" ) );
            m_canvas->SetAutoPanRequest( false );
            break;
        }

        if( DrawStruct->IsNew() )
        {
            End_Edge( (DRAWSEGMENT*) DrawStruct, aDC );
            m_canvas->SetAutoPanRequest( false );
            SetCurItem( NULL );
        }

        break;
    }
}


void PCB_EDIT_FRAME::OnEditItemRequest( wxDC* aDC, BOARD_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_TRACE_T:
    case PCB_VIA_T:
        Edit_TrackSegm_Width( aDC, static_cast<TRACK*>( aItem ) );
        break;

    case PCB_TEXT_T:
        InstallTextPCBOptionsFrame( static_cast<TEXTE_PCB*>( aItem ), aDC );
        break;

    case PCB_PAD_T:
        InstallPadOptionsFrame( static_cast<D_PAD*>( aItem ) );
        break;

    case PCB_MODULE_T:
        InstallModuleOptionsFrame( static_cast<MODULE*>( aItem ), aDC );
        break;

    case PCB_TARGET_T:
        ShowTargetOptionsDialog( static_cast<PCB_TARGET*>( aItem ), aDC );
        break;

    case PCB_DIMENSION_T:
        ShowDimensionPropertyDialog( static_cast<DIMENSION*>( aItem ), aDC );
        break;

    case PCB_MODULE_TEXT_T:
        InstallTextModOptionsFrame( static_cast<TEXTE_MODULE*>( aItem ), aDC );
        break;

    case PCB_LINE_T:
        InstallGraphicItemPropertiesDialog( static_cast<DRAWSEGMENT*>( aItem ), aDC );
        break;

    case PCB_ZONE_AREA_T:
        Edit_Zone_Params( aDC, static_cast<ZONE_CONTAINER*>( aItem ) );
        break;

    default:
        break;
    }
}
