/**
 * @file print_board_functions.cpp
 * @brief Functions to print boards.
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
  * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <printout_controler.h>
#include <colors_selection.h>
#include <pcbcommon.h>

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_track.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <module_editor_frame.h>


static void Print_Module( EDA_DRAW_PANEL* aPanel, wxDC* aDC, MODULE* aModule,
                          GR_DRAWMODE aDraw_mode, LSET aMasklayer,
                          PRINT_PARAMETERS::DrillShapeOptT aDrillShapeOpt );

void FOOTPRINT_EDIT_FRAME::PrintPage( wxDC* aDC,
                                      LSET aPrintMaskLayer,
                                      bool  aPrintMirrorMode,
                                      void * aData)
{
    GR_DRAWMODE drawmode = GR_COPY;
    int     defaultPenSize = Millimeter2iu( 0.2 );

    DISPLAY_OPTIONS save_opt;

    PRINT_PARAMETERS * printParameters = (PRINT_PARAMETERS*) aData; // can be null
    PRINT_PARAMETERS::DrillShapeOptT drillShapeOpt = PRINT_PARAMETERS::FULL_DRILL_SHAPE;

    if( printParameters )
         defaultPenSize = printParameters->m_PenDefaultSize;

    save_opt = DisplayOpt;

    DisplayOpt.ContrastModeDisplay = false;
    DisplayOpt.DisplayPadFill = true;
    DisplayOpt.DisplayViaFill = true;

    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum = DisplayOpt.DisplayPadNum = false;
    bool nctmp = GetBoard()->IsElementVisible(NO_CONNECTS_VISIBLE);
    GetBoard()->SetElementVisibility(NO_CONNECTS_VISIBLE, false);
    DisplayOpt.DisplayPadIsol    = false;
    DisplayOpt.DisplayModEdge    = FILLED;
    DisplayOpt.DisplayModText    = FILLED;
    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill = true;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
    DisplayOpt.DisplayDrawItems    = FILLED;
    DisplayOpt.DisplayZonesMode    = 0;
    DisplayOpt.DisplayNetNamesMode = 0;

    m_canvas->SetPrintMirrored( aPrintMirrorMode );

    // The OR mode is used in color mode, but be aware the background *must be
    // BLACK.  In the print page dialog, we first print in BLACK, and after
    // reprint in color, on the black "local" background, in OR mode the black
    // print is not made before, only a white page is printed
    if( GetGRForceBlackPenState() == false )
        drawmode = GR_OR;

    // Draw footprints, this is done at last in order to print the pad holes in
    // white (or g_DrawBgColor) after the tracks and zones
    int tmp = D_PAD::m_PadSketchModePenSize;
    D_PAD::m_PadSketchModePenSize = defaultPenSize;

    wxSize  pageSizeIU = GetPageSizeIU() / 2;
    wxPoint offset( pageSizeIU.x, pageSizeIU.y );

    for( MODULE* module = GetBoard()->m_Modules;  module;  module = module->Next() )
    {
        module->Move( offset );
        Print_Module( m_canvas, aDC, module, drawmode, aPrintMaskLayer, drillShapeOpt );
        module->Move( -offset );
    }

    D_PAD::m_PadSketchModePenSize = tmp;

    m_canvas->SetPrintMirrored( false );

    DisplayOpt = save_opt;
    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum  = DisplayOpt.DisplayPadNum;
    GetBoard()->SetElementVisibility( NO_CONNECTS_VISIBLE, nctmp );
}


void PCB_EDIT_FRAME::PrintPage( wxDC* aDC,
                                LSET  aPrintMask,
                                bool  aPrintMirrorMode,
                                void* aData)
{
    GR_DRAWMODE     drawmode = GR_COPY;
    DISPLAY_OPTIONS save_opt;
    BOARD*          Pcb   = GetBoard();
    int             defaultPenSize = Millimeter2iu( 0.2 );
    bool            onePagePerLayer = false;

    PRINT_PARAMETERS* printParameters = (PRINT_PARAMETERS*) aData; // can be null

    if( printParameters && printParameters->m_OptionPrintPage == 0 )
        onePagePerLayer = true;

    PRINT_PARAMETERS::DrillShapeOptT drillShapeOpt = PRINT_PARAMETERS::FULL_DRILL_SHAPE;

    if( printParameters )
    {
        drillShapeOpt = printParameters->m_DrillShapeOpt;
        defaultPenSize = printParameters->m_PenDefaultSize;
    }

    save_opt = DisplayOpt;

    LAYER_ID activeLayer = GetScreen()->m_Active_Layer;

    DisplayOpt.ContrastModeDisplay = false;
    DisplayOpt.DisplayPadFill = true;
    DisplayOpt.DisplayViaFill = true;

    if( !( aPrintMask & LSET::AllCuMask() ).any() )
    {
        if( onePagePerLayer )
        {
            // We can print mask layers (solder mask and solder paste) with the actual
            // pad sizes.  To do that, we must set ContrastModeDisplay to true and set
            // the GetScreen()->m_Active_Layer to the current printed layer
            DisplayOpt.ContrastModeDisplay = true;
            DisplayOpt.DisplayPadFill = true;

            // Calculate the active layer number to print from its mask layer:
            GetScreen()->m_Active_Layer = B_Cu;

            for( LAYER_NUM id = LAYER_ID_COUNT-1; id >= 0; --id )
            {
                if( aPrintMask[id] )
                {
                    GetScreen()->m_Active_Layer = LAYER_ID( id );
                    break;
                }
            }

            // pads on Silkscreen layer are usually plot in sketch mode:
            if( GetScreen()->m_Active_Layer == B_SilkS ||
                GetScreen()->m_Active_Layer == F_SilkS )
            {
                DisplayOpt.DisplayPadFill = false;
            }
        }
        else
        {
            DisplayOpt.DisplayPadFill = false;
        }
    }

    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum = DisplayOpt.DisplayPadNum = false;

    bool nctmp = GetBoard()->IsElementVisible( NO_CONNECTS_VISIBLE );

    GetBoard()->SetElementVisibility( NO_CONNECTS_VISIBLE, false );

    bool anchorsTmp = GetBoard()->IsElementVisible( ANCHOR_VISIBLE );

    GetBoard()->SetElementVisibility( ANCHOR_VISIBLE, false );

    DisplayOpt.DisplayPadIsol = false;
    m_DisplayModEdge = DisplayOpt.DisplayModEdge    = FILLED;
    m_DisplayModText = DisplayOpt.DisplayModText    = FILLED;
    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill = true;
    DisplayOpt.ShowTrackClearanceMode = DO_NOT_SHOW_CLEARANCE;
    DisplayOpt.DisplayDrawItems    = FILLED;
    DisplayOpt.DisplayZonesMode    = 0;
    DisplayOpt.DisplayNetNamesMode = 0;

    m_canvas->SetPrintMirrored( aPrintMirrorMode );

    // The OR mode is used in color mode, but be aware the background *must be
    // BLACK.  In the print page dialog, we first print in BLACK, and after
    // reprint in color, on the black "local" background, in OR mode the black
    // print is not made before, only a white page is printed
    if( GetGRForceBlackPenState() == false )
        drawmode = GR_OR;

    // Print the pcb graphic items (texts, ...)
    GRSetDrawMode( aDC, drawmode );

    for( BOARD_ITEM* item = Pcb->m_Drawings; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_LINE_T:
        case PCB_DIMENSION_T:
        case PCB_TEXT_T:
        case PCB_TARGET_T:
            if( aPrintMask[item->GetLayer()] )
                item->Draw( m_canvas, aDC, drawmode );
            break;

        case PCB_MARKER_T:
        default:
            break;
        }
    }

    // Print tracks
    for( TRACK* track = Pcb->m_Track; track; track = track->Next() )
    {
        if( !( aPrintMask & track->GetLayerSet() ).any() )
            continue;

        if( track->Type() == PCB_VIA_T ) // VIA encountered.
        {
            int         radius = track->GetWidth() / 2;
            const VIA*  via = static_cast<const VIA*>( track );

            EDA_COLOR_T color = g_ColorsSettings.GetItemColor( VIAS_VISIBLE + via->GetViaType() );

            GRSetDrawMode( aDC, drawmode );
            GRFilledCircle( m_canvas->GetClipBox(), aDC,
                            via->GetStart().x,
                            via->GetStart().y,
                            radius,
                            0, color, color );
        }
        else
        {
            track->Draw( m_canvas, aDC, drawmode );
        }
    }

    // Outdated: only for compatibility to old boards
    for( TRACK* track = Pcb->m_Zone; track; track = track->Next() )
    {
        if( !( aPrintMask & track->GetLayerSet() ).any() )
            continue;

        track->Draw( m_canvas, aDC, drawmode );
    }

    // Draw filled areas (i.e. zones)
    for( int ii = 0; ii < Pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = Pcb->GetArea( ii );

        if( aPrintMask[zone->GetLayer()] )
            zone->DrawFilledArea( m_canvas, aDC, drawmode );
    }

    // Draw footprints, this is done at last in order to print the pad holes in
    // white (or g_DrawBgColor) after the tracks and zones
    int tmp = D_PAD::m_PadSketchModePenSize;
    D_PAD::m_PadSketchModePenSize = defaultPenSize;

    for( MODULE* module = (MODULE*) Pcb->m_Modules; module;  module = module->Next() )
    {
        Print_Module( m_canvas, aDC, module, drawmode, aPrintMask, drillShapeOpt );
    }

    D_PAD::m_PadSketchModePenSize = tmp;

    /* Print via holes in bg color: Not sure it is good for buried or blind
     * vias */
    if( drillShapeOpt != PRINT_PARAMETERS::NO_DRILL_SHAPE )
    {
        TRACK*      track = Pcb->m_Track;
        EDA_COLOR_T color = g_DrawBgColor;

        bool blackpenstate = GetGRForceBlackPenState();

        GRForceBlackPen( false );
        GRSetDrawMode( aDC, GR_COPY );

        for( ; track; track = track->Next() )
        {
            if( !( aPrintMask & track->GetLayerSet() ).any() )
                continue;

            if( track->Type() == PCB_VIA_T ) // VIA encountered.
            {
                int diameter;
                const VIA *via = static_cast<const VIA*>( track );

                if( drillShapeOpt == PRINT_PARAMETERS::SMALL_DRILL_SHAPE )
                    diameter = std::min( SMALL_DRILL, via->GetDrillValue() );
                else
                    diameter = via->GetDrillValue();

                GRFilledCircle( m_canvas->GetClipBox(), aDC,
                                track->GetStart().x, track->GetStart().y,
                                diameter/2,
                                0, color, color );
            }
        }

        GRForceBlackPen( blackpenstate );
    }

    m_canvas->SetPrintMirrored( false );

    DisplayOpt = save_opt;
    GetScreen()->m_Active_Layer = activeLayer;
    m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    m_DisplayPadFill = DisplayOpt.DisplayPadFill;
    m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    m_DisplayPadNum  = DisplayOpt.DisplayPadNum;
    m_DisplayModEdge = DisplayOpt.DisplayModEdge;
    m_DisplayModText = DisplayOpt.DisplayModText;

    GetBoard()->SetElementVisibility( NO_CONNECTS_VISIBLE, nctmp );
    GetBoard()->SetElementVisibility( ANCHOR_VISIBLE, anchorsTmp );
}


static void Print_Module( EDA_DRAW_PANEL* aPanel, wxDC* aDC, MODULE* aModule,
                          GR_DRAWMODE aDraw_mode, LSET aMask,
                          PRINT_PARAMETERS::DrillShapeOptT aDrillShapeOpt )
{
    // Print pads
    for( D_PAD* pad = aModule->Pads();  pad;  pad = pad->Next() )
    {
        if( !( pad->GetLayerSet() & aMask ).any() )
            continue;

        // Manage hole according to the print drill option
        wxSize drill_tmp = pad->GetDrillSize();

        switch( aDrillShapeOpt )
        {
        case PRINT_PARAMETERS::NO_DRILL_SHAPE:
            pad->SetDrillSize( wxSize(0,0) );
            break;

        case PRINT_PARAMETERS::SMALL_DRILL_SHAPE:
            {
                wxSize sz(  std::min( SMALL_DRILL, pad->GetDrillSize().x ),
                            std::min( SMALL_DRILL, pad->GetDrillSize().y ) );

                pad->SetDrillSize( sz );
            }
            break;

        case PRINT_PARAMETERS::FULL_DRILL_SHAPE:
            // Do nothing
            break;
        }

        pad->Draw( aPanel, aDC, aDraw_mode );
        pad->SetDrillSize( drill_tmp );
    }

    // Print footprint graphic shapes
    LSET mlayer( aModule->GetLayer() );

    if( aModule->GetLayer() == B_Cu )
        mlayer = LSET( B_SilkS );
    else if( aModule->GetLayer() == F_Cu )
        mlayer = LSET( F_SilkS );

    if( ( mlayer & aMask ).any() )
    {
        if( aModule->Reference().IsVisible() )
            aModule->Reference().Draw( aPanel, aDC, aDraw_mode );

        if( aModule->Value().IsVisible() )
            aModule->Value().Draw( aPanel, aDC, aDraw_mode );
    }

    for( EDA_ITEM* item = aModule->GraphicalItems();  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            if( !( mlayer & aMask ).any() )
                break;

            TEXTE_MODULE* textMod;
            textMod = (TEXTE_MODULE*) item;
            textMod->Draw( aPanel, aDC, aDraw_mode );
            break;

        case PCB_MODULE_EDGE_T:
            {
                EDGE_MODULE* edge = (EDGE_MODULE*) item;

                if( !aMask[edge->GetLayer()] )
                    break;

                edge->Draw( aPanel, aDC, aDraw_mode );
            }
            break;

        default:
            break;
        }
    }
}
