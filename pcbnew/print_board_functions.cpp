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
#include <pcb_edit_frame.h>
#include <printout_controler.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_track.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <footprint_edit_frame.h>


static void Print_Module( EDA_DRAW_PANEL* aPanel, wxDC* aDC, MODULE* aModule,
                          GR_DRAWMODE aDraw_mode, LSET aMasklayer,
                          PRINT_PARAMETERS::DrillShapeOptT aDrillShapeOpt );

void FOOTPRINT_EDIT_FRAME::PrintPage( wxDC* aDC,
                                      LSET aPrintMaskLayer,
                                      bool  aPrintMirrorMode,
                                      void * aData)
{
    const GR_DRAWMODE drawmode = (GR_DRAWMODE) 0;
    int     defaultPenSize = Millimeter2iu( 0.2 );
    auto displ_opts = (PCB_DISPLAY_OPTIONS*) GetDisplayOptions();
    PCB_DISPLAY_OPTIONS save_opt;

    PRINT_PARAMETERS * printParameters = (PRINT_PARAMETERS*) aData; // can be null
    PRINT_PARAMETERS::DrillShapeOptT drillShapeOpt = PRINT_PARAMETERS::FULL_DRILL_SHAPE;

    if( printParameters )
         defaultPenSize = printParameters->m_PenDefaultSize;

    save_opt = *displ_opts;

    displ_opts->m_ContrastModeDisplay = false;
    displ_opts->m_DisplayPadFill = true;
    displ_opts->m_DisplayViaFill = true;
    displ_opts->m_DisplayPadNum = false;
    bool nctmp = GetBoard()->IsElementVisible( LAYER_NO_CONNECTS );
    GetBoard()->SetElementVisibility( LAYER_NO_CONNECTS, false );
    displ_opts->m_DisplayPadIsol    = false;
    displ_opts->m_DisplayModEdgeFill    = FILLED;
    displ_opts->m_DisplayModTextFill    = FILLED;
    displ_opts->m_DisplayPcbTrackFill = true;
    displ_opts->m_ShowTrackClearanceMode = PCB_DISPLAY_OPTIONS::DO_NOT_SHOW_CLEARANCE;
    displ_opts->m_DisplayDrawItemsFill    = FILLED;
    displ_opts->m_DisplayZonesMode    = 0;
    displ_opts->m_DisplayNetNamesMode = 0;

    m_canvas->SetPrintMirrored( aPrintMirrorMode );

    // Draw footprints, this is done at last in order to print the pad holes in
    // white after the tracks and zones
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

    *displ_opts = save_opt;
    GetBoard()->SetElementVisibility( LAYER_NO_CONNECTS, nctmp );
}


void PCB_EDIT_FRAME::PrintPage( wxDC* aDC,
                                LSET  aPrintMask,
                                bool  aPrintMirrorMode,
                                void* aData)
{
    const GR_DRAWMODE drawmode = (GR_DRAWMODE) 0;
    PCB_DISPLAY_OPTIONS save_opt;
    BOARD*          Pcb   = GetBoard();
    int             defaultPenSize = Millimeter2iu( 0.2 );

    PRINT_PARAMETERS* printParameters = (PRINT_PARAMETERS*) aData; // can be null
    auto displ_opts = (PCB_DISPLAY_OPTIONS*) GetDisplayOptions();

    PRINT_PARAMETERS::DrillShapeOptT drillShapeOpt = PRINT_PARAMETERS::FULL_DRILL_SHAPE;

    if( printParameters )
    {
        drillShapeOpt = printParameters->m_DrillShapeOpt;
        defaultPenSize = printParameters->m_PenDefaultSize;
    }

    save_opt = *displ_opts;

    PCB_LAYER_ID activeLayer = GetScreen()->m_Active_Layer;

    displ_opts->m_ContrastModeDisplay = false;
    displ_opts->m_DisplayPadFill = true;
    displ_opts->m_DisplayViaFill = true;

    // Set all board layers as visible, because the print dialog has itself
    // a layer selection, that have priority over the layer manager setup
    LSET save_visible_brd_layers = Pcb->GetVisibleLayers();
    Pcb->SetVisibleLayers( aPrintMask );

    int save_visible_brd_elements = Pcb->GetVisibleElements();
    Pcb->SetElementVisibility( LAYER_PAD_FR, true );
    Pcb->SetElementVisibility( LAYER_PAD_BK, true );
    Pcb->SetElementVisibility( LAYER_MOD_TEXT_FR, true );
    Pcb->SetElementVisibility( LAYER_MOD_TEXT_BK, true );

    PCB_LAYER_ID layer = aPrintMask.ExtractLayer();

    // pads on Silkscreen layer are usually printed in sketch mode:
    if( layer == B_SilkS || layer == F_SilkS )
        displ_opts->m_DisplayPadFill = false;

    displ_opts->m_DisplayPadNum = false;

    bool nctmp = GetBoard()->IsElementVisible( LAYER_NO_CONNECTS );

    GetBoard()->SetElementVisibility( LAYER_NO_CONNECTS, false );

    bool anchorsTmp = GetBoard()->IsElementVisible( LAYER_ANCHOR );

    GetBoard()->SetElementVisibility( LAYER_ANCHOR, false );

    displ_opts->m_DisplayPadIsol = false;
    displ_opts->m_DisplayModEdgeFill = FILLED;
    displ_opts->m_DisplayModTextFill = FILLED;
    displ_opts->m_DisplayPcbTrackFill = true;
    displ_opts->m_ShowTrackClearanceMode = PCB_DISPLAY_OPTIONS::DO_NOT_SHOW_CLEARANCE;
    displ_opts->m_DisplayDrawItemsFill    = FILLED;
    displ_opts->m_DisplayZonesMode    = 0;
    displ_opts->m_DisplayNetNamesMode = 0;

    m_canvas->SetPrintMirrored( aPrintMirrorMode );

    for( auto item : Pcb->Drawings() )
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

            COLOR4D color = Settings().Colors().GetItemColor( LAYER_VIAS + via->GetViaType() );

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

    // Deprecated: only for compatibility with very old boards
    for( TRACK* track = Pcb->m_SegZoneDeprecated; track; track = track->Next() )
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
    // white after the tracks and zones
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
        COLOR4D     color = COLOR4D::WHITE;

        bool blackpenstate = GetGRForceBlackPenState();

        GRForceBlackPen( false );

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

    // Restore settings:
    *displ_opts = save_opt;
    Pcb->SetVisibleLayers( save_visible_brd_layers );
    Pcb->SetVisibleElements( save_visible_brd_elements );
    GetScreen()->m_Active_Layer = activeLayer;

    GetBoard()->SetElementVisibility( LAYER_NO_CONNECTS, nctmp );
    GetBoard()->SetElementVisibility( LAYER_ANCHOR, anchorsTmp );
}


static void Print_Module( EDA_DRAW_PANEL* aPanel, wxDC* aDC, MODULE* aModule,
                          GR_DRAWMODE aDraw_mode, LSET aMask,
                          PRINT_PARAMETERS::DrillShapeOptT aDrillShapeOpt )
{
    // Print pads
    for( D_PAD* pad = aModule->PadsList();  pad;  pad = pad->Next() )
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

    if( aModule->Reference().IsVisible() && aMask[aModule->Reference().GetLayer()] )
        aModule->Reference().Draw( aPanel, aDC, aDraw_mode );

    if( aModule->Value().IsVisible() && aMask[aModule->Value().GetLayer()] )
        aModule->Value().Draw( aPanel, aDC, aDraw_mode );

    for( EDA_ITEM* item = aModule->GraphicalItemsList();  item;  item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            {
                TEXTE_MODULE* textMod = static_cast<TEXTE_MODULE*>( item );

                if( !aMask[textMod->GetLayer()] )
                    break;

                textMod->Draw( aPanel, aDC, aDraw_mode );
                break;
            }

        case PCB_MODULE_EDGE_T:
            {
                EDGE_MODULE* edge = static_cast<EDGE_MODULE*>( item );

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
