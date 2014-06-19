/**
 * @file plot_board_layers.cpp
 * @brief Functions to plot one board layer (silkscreen layers or other layers).
 * Silkscreen layers have specific requirement for pads (not filled) and texts
 * (with option to remove them from some copper areas (pads...)
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <common.h>
#include <plot_common.h>
#include <base_struct.h>
#include <drawtxt.h>
#include <trigo.h>
#include <wxBasePcbFrame.h>
#include <pcbcommon.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_mire.h>
#include <class_dimension.h>

#include <pcbnew.h>
#include <pcbplot.h>

// Local
/* Plot a solder mask layer.
 * Solder mask layers have a minimum thickness value and cannot be drawn like standard layers,
 * unless the minimum thickness is 0.
 */
static void PlotSolderMaskLayer( BOARD *aBoard, PLOTTER* aPlotter,
                                 LAYER_MSK aLayerMask, const PCB_PLOT_PARAMS& aPlotOpt,
                                 int aMinThickness );

/* Creates the plot for silkscreen layers
 * Silkscreen layers have specific requirement for pads (not filled) and texts
 * (with option to remove them from some copper areas (pads...)
 */
void PlotSilkScreen( BOARD *aBoard, PLOTTER* aPlotter, LAYER_MSK aLayerMask,
                     const PCB_PLOT_PARAMS& aPlotOpt )
{
    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerMask( aLayerMask );

    // Plot edge layer and graphic items
    itemplotter.PlotBoardGraphicItems();

    // Plot footprint outlines :
    itemplotter.Plot_Edges_Modules();

    // Plot pads (creates pads outlines, for pads on silkscreen layers)
    int layersmask_plotpads = aLayerMask;
    // Calculate the mask layers of allowed layers for pads

    if( !aPlotOpt.GetPlotPadsOnSilkLayer() )       // Do not plot pads on silk screen layers
        layersmask_plotpads &= ~(SILKSCREEN_LAYER_BACK | SILKSCREEN_LAYER_FRONT );

    if( layersmask_plotpads )
    {
        for( MODULE* Module = aBoard->m_Modules; Module; Module = Module->Next() )
        {
            for( D_PAD * pad = Module->Pads(); pad != NULL; pad = pad->Next() )
            {
                // See if the pad is on this layer
                LAYER_MSK masklayer = pad->GetLayerMask();
                if( (masklayer & layersmask_plotpads) == 0 )
                    continue;

                EDA_COLOR_T color = ColorFromInt(0);
                if( (layersmask_plotpads & SILKSCREEN_LAYER_BACK) )
                   color = aBoard->GetLayerColor( SILKSCREEN_N_BACK );

                if((layersmask_plotpads & SILKSCREEN_LAYER_FRONT ) )
                    color = ColorFromInt( color | aBoard->GetLayerColor( SILKSCREEN_N_FRONT ) );

                itemplotter.PlotPad( pad, color, LINE );
            }
        }
    }

    // Plot footprints fields (ref, value ...)
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        if( ! itemplotter.PlotAllTextsModule( module ) )
        {
             wxLogMessage( _( "Your BOARD has a bad layer number for module %s" ),
                           GetChars( module->GetReference() ) );
        }
    }

    // Plot filled areas
    for( int ii = 0; ii < aBoard->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = aBoard->GetArea( ii );

        if( ( GetLayerMask( edge_zone->GetLayer() ) & aLayerMask ) == 0 )
            continue;

        itemplotter.PlotFilledAreas( edge_zone );
    }

    // Plot segments used to fill zone areas (outdated, but here for old boards
    // compatibility):
    for( SEGZONE* seg = aBoard->m_Zone; seg != NULL; seg = seg->Next() )
    {
        if( ( GetLayerMask( seg->GetLayer() ) & aLayerMask ) == 0 )
            continue;

        aPlotter->ThickSegment( seg->GetStart(), seg->GetEnd(), seg->GetWidth(),
                                itemplotter.GetMode() );
    }
}

void PlotOneBoardLayer( BOARD *aBoard, PLOTTER* aPlotter, LAYER_NUM aLayer,
                     const PCB_PLOT_PARAMS& aPlotOpt )
{
    PCB_PLOT_PARAMS plotOpt = aPlotOpt;
    int soldermask_min_thickness = aBoard->GetDesignSettings().m_SolderMaskMinWidth;

    // Set a default color and the text mode for this layer
    aPlotter->SetColor( aPlotOpt.GetColor() );
    aPlotter->SetTextMode( aPlotOpt.GetTextMode() );

    // Specify that the contents of the "Edges Pcb" layer are to be plotted
    // in addition to the contents of the currently specified layer.
    LAYER_MSK layer_mask = GetLayerMask( aLayer );

    if( !aPlotOpt.GetExcludeEdgeLayer() )
        layer_mask |= EDGE_LAYER;

    switch( aLayer )
    {
    case FIRST_COPPER_LAYER:
    case LAYER_N_2:
    case LAYER_N_3:
    case LAYER_N_4:
    case LAYER_N_5:
    case LAYER_N_6:
    case LAYER_N_7:
    case LAYER_N_8:
    case LAYER_N_9:
    case LAYER_N_10:
    case LAYER_N_11:
    case LAYER_N_12:
    case LAYER_N_13:
    case LAYER_N_14:
    case LAYER_N_15:
    case LAST_COPPER_LAYER:
        // Skip NPTH pads on copper layers ( only if hole size == pad size ):
        // Drill mark will be plotted,
        // if drill mark is SMALL_DRILL_SHAPE  or FULL_DRILL_SHAPE
        if( plotOpt.GetFormat() == PLOT_FORMAT_DXF )
        {
            plotOpt.SetSkipPlotNPTH_Pads( false );
            PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
        }
        else
        {
            plotOpt.SetSkipPlotNPTH_Pads( true );
            PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
        }
        break;

    case SOLDERMASK_N_BACK:
    case SOLDERMASK_N_FRONT:
        plotOpt.SetSkipPlotNPTH_Pads( false );
        // Disable plot pad holes
        plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

        // Plot solder mask:
        if( soldermask_min_thickness == 0 )
        {
            if( plotOpt.GetFormat() == PLOT_FORMAT_DXF )
                PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
            else
                PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
        }
        else
            PlotSolderMaskLayer( aBoard, aPlotter, layer_mask, plotOpt,
                                 soldermask_min_thickness );

        break;

    case SOLDERPASTE_N_BACK:
    case SOLDERPASTE_N_FRONT:
        plotOpt.SetSkipPlotNPTH_Pads( false );
        // Disable plot pad holes
        plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

        if( plotOpt.GetFormat() == PLOT_FORMAT_DXF )
            PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
        else
            PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
        break;

    case SILKSCREEN_N_FRONT:
    case SILKSCREEN_N_BACK:
        if( plotOpt.GetFormat() == PLOT_FORMAT_DXF )
            PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
        else
            PlotSilkScreen( aBoard, aPlotter, layer_mask, plotOpt );

        // Gerber: Subtract soldermask from silkscreen if enabled
        if( aPlotter->GetPlotterType() == PLOT_FORMAT_GERBER
            && plotOpt.GetSubtractMaskFromSilk() )
        {
            if( aLayer == SILKSCREEN_N_FRONT )
                layer_mask = GetLayerMask( SOLDERMASK_N_FRONT );
            else
                layer_mask = GetLayerMask( SOLDERMASK_N_BACK );

            // Create the mask to subtract by creating a negative layer polarity
            aPlotter->SetLayerPolarity( false );
            // Disable plot pad holes
            plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );
            // Plot the mask
            PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
        }
        break;

    default:
        PlotSilkScreen( aBoard, aPlotter, layer_mask, plotOpt );
        break;
    }
}


/* Plot a copper layer or mask.
 * Silk screen layers are not plotted here.
 */
void PlotStandardLayer( BOARD *aBoard, PLOTTER* aPlotter,
                        LAYER_MSK aLayerMask, const PCB_PLOT_PARAMS& aPlotOpt )
{

    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerMask( aLayerMask );

    EDA_DRAW_MODE_T plotMode = aPlotOpt.GetMode();

     // Plot edge layer and graphic items
    itemplotter.PlotBoardGraphicItems();

    // Draw footprint shapes without pads (pads will plotted later)
    // We plot here module texts, but they are usually on silkscreen layer,
    // so they are not plot here but plot by PlotSilkScreen()
    // Plot footprints fields (ref, value ...)
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        if( ! itemplotter.PlotAllTextsModule( module ) )
        {
            wxLogMessage( _( "Your BOARD has a bad layer number for module %s" ),
                           GetChars( module->GetReference() ) );
        }
    }

    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItems(); item; item = item->Next() )
        {
            if( ! (aLayerMask & GetLayerMask( item->GetLayer() ) ) )
                continue;

            switch( item->Type() )
            {
            case PCB_MODULE_EDGE_T:
                itemplotter.Plot_1_EdgeModule( (EDGE_MODULE*) item );
                break;

            default:
                break;
            }
        }
    }

    // Plot footprint pads
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->Pads();  pad;  pad = pad->Next() )
        {
            if( (pad->GetLayerMask() & aLayerMask) == 0 )
                continue;

            wxSize margin;
            double width_adj = 0;

            if( aLayerMask & ALL_CU_LAYERS )
                width_adj =  itemplotter.getFineWidthAdj();

            switch( aLayerMask &
                   ( SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT |
                     SOLDERPASTE_LAYER_BACK | SOLDERPASTE_LAYER_FRONT ) )
            {
            case SOLDERMASK_LAYER_FRONT:
            case SOLDERMASK_LAYER_BACK:
                margin.x = margin.y = pad->GetSolderMaskMargin();
                break;

            case SOLDERPASTE_LAYER_FRONT:
            case SOLDERPASTE_LAYER_BACK:
                margin = pad->GetSolderPasteMargin();
                break;

            default:
                break;
            }

            wxSize padPlotsSize;
            padPlotsSize.x = pad->GetSize().x + ( 2 * margin.x ) + width_adj;
            padPlotsSize.y = pad->GetSize().y + ( 2 * margin.y ) + width_adj;

            // Don't draw a null size item :
            if( padPlotsSize.x <= 0 || padPlotsSize.y <= 0 )
                continue;

            EDA_COLOR_T color = BLACK;

            if( (pad->GetLayerMask() & LAYER_BACK) )
               color = aBoard->GetVisibleElementColor( PAD_BK_VISIBLE );

            if((pad->GetLayerMask() & LAYER_FRONT ) )
                color = ColorFromInt( color | aBoard->GetVisibleElementColor( PAD_FR_VISIBLE ) );

            // Temporary set the pad size to the required plot size:
            wxSize tmppadsize = pad->GetSize();
            pad->SetSize( padPlotsSize );
            switch( pad->GetShape() )
            {
            case PAD_CIRCLE:
            case PAD_OVAL:
                if( aPlotOpt.GetSkipPlotNPTH_Pads() &&
                    (pad->GetSize() == pad->GetDrillSize()) &&
                    (pad->GetAttribute() == PAD_HOLE_NOT_PLATED) )
                    break;

                // Fall through:
            case PAD_TRAPEZOID:
            case PAD_RECT:
            default:
                itemplotter.PlotPad( pad, color, plotMode );
                break;
            }

            pad->SetSize( tmppadsize );     // Restore the pad size
        }
    }

    // Plot vias on copper layers, and if aPlotOpt.GetPlotViaOnMaskLayer() is true,
    // plot them on solder mask
    for( TRACK* track = aBoard->m_Track; track; track = track->Next() )
    {
        const VIA* Via = dyn_cast<const VIA*>( track );

        if( !Via )
            continue;

        // vias are not plotted if not on selected layer, but if layer
        // is SOLDERMASK_LAYER_BACK or SOLDERMASK_LAYER_FRONT,vias are drawn,
        // only if they are on the corresponding external copper layer
        int via_mask_layer = Via->GetLayerMask();

        if( aPlotOpt.GetPlotViaOnMaskLayer() )
        {
            if( via_mask_layer & LAYER_BACK )
                via_mask_layer |= SOLDERMASK_LAYER_BACK;

            if( via_mask_layer & LAYER_FRONT )
                via_mask_layer |= SOLDERMASK_LAYER_FRONT;
        }

        if( ( via_mask_layer & aLayerMask ) == 0 )
            continue;

        int via_margin = 0;
        double width_adj = 0;

        // If the current layer is a solder mask, use the global mask
        // clearance for vias
        if( ( aLayerMask & ( SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT ) ) )
            via_margin = aBoard->GetDesignSettings().m_SolderMaskMargin;

        if( aLayerMask & ALL_CU_LAYERS )
            width_adj = itemplotter.getFineWidthAdj();

        int diameter = Via->GetWidth() + 2 * via_margin + width_adj;

        // Don't draw a null size item :
        if( diameter <= 0 )
            continue;

        EDA_COLOR_T color = aBoard->GetVisibleElementColor(VIAS_VISIBLE + Via->GetViaType());
        // Set plot color (change WHITE to LIGHTGRAY because
        // the white items are not seen on a white paper or screen
        aPlotter->SetColor( color != WHITE ? color : LIGHTGRAY);
        aPlotter->FlashPadCircle( Via->GetStart(), diameter, plotMode );
    }

    // Plot tracks (not vias) :
    for( TRACK* track = aBoard->m_Track; track; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
            continue;

        if( (GetLayerMask( track->GetLayer() ) & aLayerMask) == 0 )
            continue;

        int width = track->GetWidth() + itemplotter.getFineWidthAdj();
        aPlotter->SetColor( itemplotter.getColor( track->GetLayer() ) );
        aPlotter->ThickSegment( track->GetStart(), track->GetEnd(), width, plotMode );
    }

    // Plot zones (outdated, for old boards compatibility):
    for( TRACK* track = aBoard->m_Zone; track; track = track->Next() )
    {
        if( (GetLayerMask( track->GetLayer() ) & aLayerMask) == 0 )
            continue;

        int width = track->GetWidth() + itemplotter.getFineWidthAdj();
        aPlotter->SetColor( itemplotter.getColor( track->GetLayer() ) );
        aPlotter->ThickSegment( track->GetStart(), track->GetEnd(), width, plotMode );
    }

    // Plot filled ares
    for( int ii = 0; ii < aBoard->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = aBoard->GetArea( ii );

        if( ( GetLayerMask(zone->GetLayer() )  & aLayerMask ) == 0 )
            continue;

        itemplotter.PlotFilledAreas( zone );
    }

    // Adding drill marks, if required and if the plotter is able to plot them:
    if( aPlotOpt.GetDrillMarksType() != PCB_PLOT_PARAMS::NO_DRILL_SHAPE )
        itemplotter.PlotDrillMarks();
}

/* Plot outlines of copper, for copper layer
 */
#include "clipper.hpp"
void PlotLayerOutlines( BOARD *aBoard, PLOTTER* aPlotter,
                        LAYER_MSK aLayerMask, const PCB_PLOT_PARAMS& aPlotOpt )
{

    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerMask( aLayerMask );

    CPOLYGONS_LIST outlines;

    for( LAYER_NUM layer = FIRST_LAYER; layer < NB_PCB_LAYERS; layer++ )
    {
        LAYER_MSK layer_mask = GetLayerMask( layer );

        if( (aLayerMask & layer_mask ) == 0 )
            continue;

        outlines.RemoveAllContours();
        aBoard->ConvertBrdLayerToPolygonalContours( layer, outlines );

        // Merge all overlapping polygons.
        KI_POLYGON_SET kpolygons;
        KI_POLYGON_SET ktmp;
        outlines.ExportTo( ktmp );

        kpolygons += ktmp;

        // Plot outlines
        std::vector< wxPoint > cornerList;

        for( unsigned ii = 0; ii < kpolygons.size(); ii++ )
        {
            KI_POLYGON polygon = kpolygons[ii];

            // polygon contains only one polygon, but it can have holes linked by
            // overlapping segments.
            // To plot clean outlines, we have to break this polygon into more polygons with
            // no overlapping segments, using Clipper, because boost::polygon
            // does not allow that
            ClipperLib::Path raw_polygon;
            ClipperLib::Paths normalized_polygons;

            for( unsigned ic = 0; ic < polygon.size(); ic++ )
            {
                KI_POLY_POINT corner = *(polygon.begin() + ic);
                raw_polygon.push_back( ClipperLib::IntPoint( corner.x(), corner.y() ) );
            }

            ClipperLib::SimplifyPolygon( raw_polygon, normalized_polygons );

            // Now we have one or more basic polygons: plot each polygon
            for( unsigned ii = 0; ii < normalized_polygons.size(); ii++ )
            {
                ClipperLib::Path& polygon = normalized_polygons[ii];
                cornerList.clear();

                for( unsigned jj = 0; jj < polygon.size(); jj++ )
                    cornerList.push_back( wxPoint( polygon[jj].X , polygon[jj].Y ) );

                // Ensure the polygon is closed
                if( cornerList[0] != cornerList[cornerList.size()-1] )
                    cornerList.push_back( cornerList[0] );

                aPlotter->PlotPoly( cornerList, NO_FILL );
            }
        }

        // Plot pad holes
        if( aPlotOpt.GetDrillMarksType() != PCB_PLOT_PARAMS::NO_DRILL_SHAPE )
        {
            for( MODULE* module = aBoard->m_Modules; module; module = module->Next() )
            {
                for( D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
                {
                    wxSize hole = pad->GetDrillSize();

                    if( hole.x == 0 || hole.y == 0 )
                        continue;

                    if( hole.x == hole.y )
                        aPlotter->Circle( pad->GetPosition(), hole.x, NO_FILL );
                    else
                    {
                        wxPoint drl_start, drl_end;
                        int width;
                        pad->GetOblongDrillGeometry( drl_start, drl_end, width );
                        aPlotter->ThickSegment( pad->GetPosition() + drl_start,
                                pad->GetPosition() + drl_end, width, SKETCH );
                    }
                }
            }
        }

        // Plot vias holes
        for( TRACK* track = aBoard->m_Track; track; track = track->Next() )
        {
            const VIA* via = dyn_cast<const VIA*>( track );

            if( via && via->IsOnLayer( layer ) )    // via holes can be not through holes
            {
                aPlotter->Circle( via->GetPosition(), via->GetDrillValue(), NO_FILL );
            }
        }
    }
}


/* Plot a solder mask layer.
 * Solder mask layers have a minimum thickness value and cannot be drawn like standard layers,
 * unless the minimum thickness is 0.
 * Currently the algo is:
 * 1 - build all pad shapes as polygons with a size inflated by
 *      mask clearance + (min width solder mask /2)
 * 2 - Merge shapes
 * 3 - deflate result by (min width solder mask /2)
 * 4 - oring result by all pad shapes as polygons with a size inflated by
 *      mask clearance only (because deflate sometimes creates shape artifacts)
 * 5 - draw result as polygons
 *
 * TODO:
 * make this calculation only for shapes with clearance near than (min width solder mask)
 * (using DRC algo)
 * plot all other shapes by flashing the basing shape
 * (shapes will be better, and calculations faster)
 */
void PlotSolderMaskLayer( BOARD *aBoard, PLOTTER* aPlotter,
                          LAYER_MSK aLayerMask, const PCB_PLOT_PARAMS& aPlotOpt,
                          int aMinThickness )
{
    LAYER_NUM layer = ( aLayerMask & SOLDERMASK_LAYER_BACK ) ?
                 SOLDERMASK_N_BACK : SOLDERMASK_N_FRONT;
    int inflate = aMinThickness/2;

    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerMask( aLayerMask );

     // Plot edge layer and graphic items
    itemplotter.PlotBoardGraphicItems();

    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        for( BOARD_ITEM* item = module->GraphicalItems(); item; item = item->Next() )
        {
            if( layer != item->GetLayer() )
                continue;

            switch( item->Type() )
            {
            case PCB_MODULE_EDGE_T:
                itemplotter.Plot_1_EdgeModule( (EDGE_MODULE*) item );
                break;

            default:
                break;
            }
        }
    }

    // Build polygons for each pad shape.
    // the size of the shape on solder mask should be:
    // size of pad + clearance around the pad.
    // clearance = solder mask clearance + extra margin
    // extra margin is half the min width for solder mask
    // This extra margin is used to merge too close shapes
    // (distance < aMinThickness), and will be removed when creating
    // the actual shapes
    CPOLYGONS_LIST bufferPolys;   // Contains shapes to plot
    CPOLYGONS_LIST initialPolys;  // Contains exact shapes to plot

    /* calculates the coeff to compensate radius reduction of holes clearance
     * due to the segment approx ( 1 /cos( PI/circleToSegmentsCount )
     */
    int circleToSegmentsCount = 32;
    double correction = 1.0 / cos( M_PI / circleToSegmentsCount );

    // Plot pads
    for( MODULE* module = aBoard->m_Modules; module; module = module->Next() )
    {
        // add shapes with exact size
        module->TransformPadsShapesWithClearanceToPolygon( layer,
                        initialPolys, 0,
                        circleToSegmentsCount, correction );
        // add shapes inflated by aMinThickness/2
        module->TransformPadsShapesWithClearanceToPolygon( layer,
                        bufferPolys, inflate,
                        circleToSegmentsCount, correction );
    }

    // Plot vias on solder masks, if aPlotOpt.GetPlotViaOnMaskLayer() is true,
    if( aPlotOpt.GetPlotViaOnMaskLayer() )
    {
        // The current layer is a solder mask,
        // use the global mask clearance for vias
        int via_clearance = aBoard->GetDesignSettings().m_SolderMaskMargin;
        int via_margin = via_clearance + inflate;
        for( TRACK* track = aBoard->m_Track; track; track = track->Next() )
        {
            const VIA* via = dyn_cast<const VIA*>( track );

            if( !via )
                continue;

            // vias are plotted only if they are on the corresponding
            // external copper layer
            LAYER_MSK via_mask_layer = via->GetLayerMask();

            if( via_mask_layer & LAYER_BACK )
                via_mask_layer |= SOLDERMASK_LAYER_BACK;

            if( via_mask_layer & LAYER_FRONT )
                via_mask_layer |= SOLDERMASK_LAYER_FRONT;

            if( ( via_mask_layer & aLayerMask ) == 0 )
                continue;

            via->TransformShapeWithClearanceToPolygon( bufferPolys, via_margin,
                    circleToSegmentsCount,
                    correction );
            via->TransformShapeWithClearanceToPolygon( initialPolys, via_clearance,
                    circleToSegmentsCount,
                    correction );
        }
    }

    // Add filled zone areas
    for( int ii = 0; ii < aBoard->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = aBoard->GetArea( ii );

        if( zone->GetLayer() != layer )
            continue;

        zone->TransformOutlinesShapeWithClearanceToPolygon( bufferPolys,
                    inflate, true );
    }

    // Now:
    // 1 - merge areas which are intersecting, i.e. remove gaps
    //     having a thickness < aMinThickness
    // 2 - deflate resulting areas by aMinThickness/2
    KI_POLYGON_SET areasToMerge;
    bufferPolys.ExportTo( areasToMerge );
    KI_POLYGON_SET initialAreas;
    initialPolys.ExportTo( initialAreas );

    // Merge polygons: because each shape was created with an extra margin
    // = aMinThickness/2, shapes too close ( dist < aMinThickness )
    // will be merged, because they are overlapping
    KI_POLYGON_SET areas;
    areas |= areasToMerge;

    // Deflate: remove the extra margin, to create the actual shapes
    // Here I am using polygon:resize, because this function creates better shapes
    // than deflate algo.
    // Use here deflate with arc creation and 18 segments per circle to create arcs
    // In boost polygon (at least v 1.54 and previous) in very rare cases resize crashes
    // with 16 segments (perhaps related to 45 degrees pads). So using 18 segments
    // is a workaround to try to avoid these crashes
    areas = resize( areas, -inflate , true, 18 );

    // Resize slightly changes shapes. So *ensure* initial shapes are kept
    areas |= initialAreas;

    // To avoid a lot of code, use a ZONE_CONTAINER
    // to plot polygons, because they are exactly like
    // filled areas in zones
    ZONE_CONTAINER zone( aBoard );
    zone.SetArcSegmentCount( 32 );
    zone.SetMinThickness( 0 );      // trace polygons only
    zone.SetLayer ( layer );

    zone.CopyPolygonsFromKiPolygonListToFilledPolysList( areas );
    itemplotter.PlotFilledAreas( &zone );
}



/** Set up most plot options for plotting a board (especially the viewport)
 * Important thing:
 *      page size is the 'drawing' page size,
 *      paper size is the physical page size
 */
static void initializePlotter( PLOTTER *aPlotter, BOARD * aBoard,
                               PCB_PLOT_PARAMS *aPlotOpts )
{
    PAGE_INFO pageA4( wxT( "A4" ) );
    const PAGE_INFO& pageInfo = aBoard->GetPageSettings();
    const PAGE_INFO* sheet_info;
    double paperscale; // Page-to-paper ratio
    wxSize paperSizeIU;
    wxSize pageSizeIU( pageInfo.GetSizeIU() );
    bool autocenter = false;

    /* Special options: to fit the sheet to an A4 sheet replace
       the paper size. However there is a difference between
       the autoscale and the a4paper option:
       - Autoscale fits the board to the paper size
       - A4paper fits the original paper size to an A4 sheet
       - Both of them fit the board to an A4 sheet
     */
    if( aPlotOpts->GetA4Output() )      // Fit paper to A4
    {
        sheet_info  = &pageA4;
        paperSizeIU = pageA4.GetSizeIU();
        paperscale  = (double) paperSizeIU.x / pageSizeIU.x;
        autocenter  = true;
    }
    else
    {
        sheet_info  = &pageInfo;
        paperSizeIU = pageSizeIU;
        paperscale  = 1;

        // Need autocentering only if scale is not 1:1
        autocenter  = (aPlotOpts->GetScale() != 1.0);
    }

    EDA_RECT bbox = aBoard->ComputeBoundingBox();
    wxPoint boardCenter = bbox.Centre();
    wxSize boardSize = bbox.GetSize();

    double compound_scale;

    /* Fit to 80% of the page if asked; it could be that the board is empty,
     * in this case regress to 1:1 scale */
    if( aPlotOpts->GetAutoScale() && boardSize.x > 0 && boardSize.y > 0 )
    {
        double xscale = (paperSizeIU.x * 0.8) / boardSize.x;
        double yscale = (paperSizeIU.y * 0.8) / boardSize.y;

        compound_scale = std::min( xscale, yscale ) * paperscale;
    }
    else
        compound_scale = aPlotOpts->GetScale() * paperscale;


    /* For the plot offset we have to keep in mind the auxiliary origin
       too: if autoscaling is off we check that plot option (i.e. autoscaling
       overrides auxiliary origin) */
    wxPoint offset( 0, 0);

    if( autocenter )
    {
        offset.x = KiROUND( boardCenter.x - ( paperSizeIU.x / 2.0 ) / compound_scale );
        offset.y = KiROUND( boardCenter.y - ( paperSizeIU.y / 2.0 ) / compound_scale );
    }
    else
    {
        if( aPlotOpts->GetUseAuxOrigin() )
            offset = aBoard->GetAuxOrigin();
    }

    /* Configure the plotter object with all the stuff computed and
       most of that taken from the options */
    aPlotter->SetPageSettings( *sheet_info );

    aPlotter->SetViewport( offset, IU_PER_DECIMILS, compound_scale,
                           aPlotOpts->GetMirror() );
    aPlotter->SetDefaultLineWidth( aPlotOpts->GetLineWidth() );
    aPlotter->SetCreator( wxT( "PCBNEW" ) );
    aPlotter->SetColorMode( false );        // default is plot in Black and White.
    aPlotter->SetTextMode( aPlotOpts->GetTextMode() );
}

/** Prefill in black an area a little bigger than the board to prepare for the
 *  negative plot */
static void FillNegativeKnockout( PLOTTER *aPlotter, const EDA_RECT &aBbbox )
{
    const int margin = 5 * IU_PER_MM;   // Add a 5 mm margin around the board
    aPlotter->SetNegative( true );
    aPlotter->SetColor( WHITE );       // Which will be plotted as black
    EDA_RECT area = aBbbox;
    area.Inflate( margin );
    aPlotter->Rect( area.GetOrigin(), area.GetEnd(), FILLED_SHAPE );
    aPlotter->SetColor( BLACK );
}

/** Calculate the effective size of HPGL pens and set them in the
 * plotter object */
static void ConfigureHPGLPenSizes( HPGL_PLOTTER *aPlotter,
                                   PCB_PLOT_PARAMS *aPlotOpts )
{
    /* Compute pen_dim (the value is given in mils) in pcb units,
       with plot scale (if Scale is 2, pen diameter value is always m_HPGLPenDiam
       so apparent pen diam is actually pen diam / Scale */
    int pen_diam = KiROUND( aPlotOpts->GetHPGLPenDiameter() * IU_PER_MILS /
                            aPlotOpts->GetScale() );

    // compute pen_overlay (value comes in mils) in pcb units with plot scale
    if( aPlotOpts->GetHPGLPenOverlay() < 0 )
        aPlotOpts->SetHPGLPenOverlay( 0 );

    if( aPlotOpts->GetHPGLPenOverlay() >= aPlotOpts->GetHPGLPenDiameter() )
        aPlotOpts->SetHPGLPenOverlay( aPlotOpts->GetHPGLPenDiameter() - 1 );

    int pen_overlay = KiROUND( aPlotOpts->GetHPGLPenOverlay() * IU_PER_MILS /
                               aPlotOpts->GetScale() );

    // Set HPGL-specific options and start
    aPlotter->SetPenSpeed( aPlotOpts->GetHPGLPenSpeed() );
    aPlotter->SetPenNumber( aPlotOpts->GetHPGLPenNum() );
    aPlotter->SetPenOverlap( pen_overlay );
    aPlotter->SetPenDiameter( pen_diam );
}

/** Open a new plotfile using the options (and especially the format)
 * specified in the options and prepare the page for plotting.
 * Return the plotter object if OK, NULL if the file is not created
 * (or has a problem)
 */
PLOTTER* StartPlotBoard( BOARD *aBoard, PCB_PLOT_PARAMS *aPlotOpts,
                         int aLayer,
                         const wxString& aFullFileName,
                         const wxString& aSheetDesc )
{
    // Create the plotter driver and set the few plotter specific
    // options
    PLOTTER*    plotter = NULL;

    switch( aPlotOpts->GetFormat() )
    {
    case PLOT_FORMAT_DXF:
        plotter = new DXF_PLOTTER();
        break;

    case PLOT_FORMAT_POST:
        PS_PLOTTER* PS_plotter;
        PS_plotter = new PS_PLOTTER();
        PS_plotter->SetScaleAdjust( aPlotOpts->GetFineScaleAdjustX(),
                                    aPlotOpts->GetFineScaleAdjustY() );
        plotter = PS_plotter;
        break;

    case PLOT_FORMAT_PDF:
        plotter = new PDF_PLOTTER();
        break;

    case PLOT_FORMAT_HPGL:
        HPGL_PLOTTER* HPGL_plotter;
        HPGL_plotter = new HPGL_PLOTTER();

        /* HPGL options are a little more convoluted to compute, so
           they're split in an other function */
        ConfigureHPGLPenSizes( HPGL_plotter, aPlotOpts );
        plotter = HPGL_plotter;
        break;

    case PLOT_FORMAT_GERBER:
        plotter = new GERBER_PLOTTER();
        break;

    case PLOT_FORMAT_SVG:
        plotter = new SVG_PLOTTER();
        break;

    default:
        wxASSERT( false );
        return NULL;
    }

    // Compute the viewport and set the other options

    // page layout is not mirrored, so temporary change mirror option
    // just to plot the page layout
    PCB_PLOT_PARAMS plotOpts = *aPlotOpts;

    if( plotOpts.GetPlotFrameRef() && plotOpts.GetMirror() )
        plotOpts.SetMirror( false );

    initializePlotter( plotter, aBoard, &plotOpts );

    if( plotter->OpenFile( aFullFileName ) )
    {
        // For the Gerber "file function" attribute, set the layer number
        if( plotter->GetPlotterType() == PLOT_FORMAT_GERBER && plotOpts.GetUseGerberAttributes() )
            plotter->SetLayerAttribFunction( GetGerberFileFunction( aBoard, aLayer ) );

        plotter->StartPlot();

        // Plot the frame reference if requested
        if( aPlotOpts->GetPlotFrameRef() )
        {
            PlotWorkSheet( plotter, aBoard->GetTitleBlock(),
                           aBoard->GetPageSettings(),
                           1, 1, // Only one page
                           aSheetDesc, aBoard->GetFileName() );

            if( aPlotOpts->GetMirror() )
                initializePlotter( plotter, aBoard, aPlotOpts );
        }

        /* When plotting a negative board: draw a black rectangle
         * (background for plot board in white) and switch the current
         * color to WHITE; note the color inversion is actually done
         * in the driver (if supported) */
        if( aPlotOpts->GetNegative() )
        {
            EDA_RECT bbox = aBoard->ComputeBoundingBox();
            FillNegativeKnockout( plotter, bbox );
        }

        return plotter;
    }

    delete plotter;
    return NULL;
}
