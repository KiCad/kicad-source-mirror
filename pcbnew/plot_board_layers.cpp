/**
 * @file plot_board_layers.cpp
 * @brief Functions to plot one board layer (silkscreen layers or other layers).
 * Silkscreen layers have specific requirement for pads (not filled) and texts
 * (with option to remove them from some copper areas (pads...)
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <plotter.h>
#include <base_struct.h>
#include <gr_text.h>
#include <geometry/geometry_utils.h>
#include <trigo.h>
#include <pcb_base_frame.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_pcb_target.h>
#include <class_dimension.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <gbr_metadata.h>

/*
 * Plot a solder mask layer.  Solder mask layers have a minimum thickness value and cannot be
 * drawn like standard layers, unless the minimum thickness is 0.
 */
static void PlotSolderMaskLayer( BOARD *aBoard, PLOTTER* aPlotter, LSET aLayerMask,
                                 const PCB_PLOT_PARAMS& aPlotOpt, int aMinThickness );

/*
 * Creates the plot for silkscreen layers.  Silkscreen layers have specific requirement for
 * pads (not filled) and texts (with option to remove them from some copper areas (pads...)
 */
void PlotSilkScreen( BOARD *aBoard, PLOTTER* aPlotter, LSET aLayerMask,
                     const PCB_PLOT_PARAMS& aPlotOpt )
{
    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerSet( aLayerMask );

    // Plot edge layer and graphic items
    itemplotter.PlotBoardGraphicItems();

    // Plot footprint outlines :
    itemplotter.Plot_Edges_Modules();

    // Plot pads (creates pads outlines, for pads on silkscreen layers)
    LSET layersmask_plotpads = aLayerMask;

    // Calculate the mask layers of allowed layers for pads

    if( !aPlotOpt.GetPlotPadsOnSilkLayer() )       // Do not plot pads on silk screen layers
        layersmask_plotpads.set( B_SilkS, false ).set( F_SilkS, false );

    if( layersmask_plotpads.any() )
    {
        for( auto Module : aBoard->Modules() )
        {
            aPlotter->StartBlock( NULL );

            for( auto pad : Module->Pads() )
            {
                // See if the pad is on this layer
                LSET masklayer = pad->GetLayerSet();
                if( !( masklayer & layersmask_plotpads ).any() )
                    continue;

                COLOR4D color = COLOR4D::BLACK;

                if( layersmask_plotpads[B_SilkS] )
                   color = aBoard->Colors().GetLayerColor( B_SilkS );

                if( layersmask_plotpads[F_SilkS] )
                    color = ( color == COLOR4D::BLACK) ? aBoard->Colors().GetLayerColor( F_SilkS ) : color;

                itemplotter.PlotPad( pad, color, SKETCH );
            }

            aPlotter->EndBlock( NULL );
        }
    }

    // Plot footprints fields (ref, value ...)
    for( auto module : aBoard->Modules() )
    {
        if( ! itemplotter.PlotAllTextsModule( module ) )
        {
             wxLogMessage( _( "Your BOARD has a bad layer number for footprint %s" ),
                           module->GetReference() );
        }
    }

    // Plot filled areas
    aPlotter->StartBlock( NULL );

    // Plot all zones together so we don't end up with divots where zones touch each other.
    ZONE_CONTAINER* zone = nullptr;
    SHAPE_POLY_SET aggregateArea;

    for( ZONE_CONTAINER* candidate : aBoard->Zones() )
    {
        if( !aLayerMask[ candidate->GetLayer() ] )
            continue;

        if( !zone )
            zone = candidate;

        aggregateArea.BooleanAdd( candidate->GetFilledPolysList(), SHAPE_POLY_SET::PM_FAST );
    }

    aggregateArea.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    itemplotter.PlotFilledAreas( zone, aggregateArea );

    aPlotter->EndBlock( NULL );
}

void PlotOneBoardLayer( BOARD *aBoard, PLOTTER* aPlotter, PCB_LAYER_ID aLayer,
                        const PCB_PLOT_PARAMS& aPlotOpt )
{
    PCB_PLOT_PARAMS plotOpt = aPlotOpt;
    int soldermask_min_thickness = aBoard->GetDesignSettings().m_SolderMaskMinWidth;

    // Set a default color and the text mode for this layer
    aPlotter->SetColor( aPlotOpt.GetColor() );
    aPlotter->SetTextMode( aPlotOpt.GetTextMode() );

    // Specify that the contents of the "Edges Pcb" layer are to be plotted in addition to the
    // contents of the currently specified layer.
    LSET    layer_mask( aLayer );

    if( !aPlotOpt.GetExcludeEdgeLayer() )
        layer_mask.set( Edge_Cuts );

    if( IsCopperLayer( aLayer ) )
    {
        // Skip NPTH pads on copper layers ( only if hole size == pad size ):
        // Drill mark will be plotted if drill mark is SMALL_DRILL_SHAPE  or FULL_DRILL_SHAPE
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
    }
    else
    {
        switch( aLayer )
        {
        case B_Mask:
        case F_Mask:
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

        case B_Adhes:
        case F_Adhes:
        case B_Paste:
        case F_Paste:
            plotOpt.SetSkipPlotNPTH_Pads( false );
            // Disable plot pad holes
            plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

            if( plotOpt.GetFormat() == PLOT_FORMAT_DXF )
                PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
            else
                PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
            break;

        case F_SilkS:
        case B_SilkS:
            if( plotOpt.GetFormat() == PLOT_FORMAT_DXF && plotOpt.GetDXFPlotPolygonMode() )
                // PlotLayerOutlines() is designed only for DXF plotters.
                // and must not be used for other plot formats
                PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
            else
                PlotSilkScreen( aBoard, aPlotter, layer_mask, plotOpt );

            // Gerber: Subtract soldermask from silkscreen if enabled
            if( aPlotter->GetPlotterType() == PLOT_FORMAT_GERBER
                && plotOpt.GetSubtractMaskFromSilk() )
            {
                if( aLayer == F_SilkS )
                    layer_mask = LSET( F_Mask );
                else
                    layer_mask = LSET( B_Mask );

                // Create the mask to subtract by creating a negative layer polarity
                aPlotter->SetLayerPolarity( false );

                // Disable plot pad holes
                plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

                // Plot the mask
                PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
            }
            break;

        // These layers are plotted like silk screen layers.
        // Mainly, pads on these layers are not filled.
        // This is not necessary the best choice.
        case Dwgs_User:
        case Cmts_User:
        case Eco1_User:
        case Eco2_User:
        case Edge_Cuts:
        case Margin:
        case F_CrtYd:
        case B_CrtYd:
        case F_Fab:
        case B_Fab:
            plotOpt.SetSkipPlotNPTH_Pads( false );
            plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

            if( plotOpt.GetFormat() == PLOT_FORMAT_DXF && plotOpt.GetDXFPlotPolygonMode() )
                // PlotLayerOutlines() is designed only for DXF plotters.
                // and must not be used for other plot formats
                PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
            else
                PlotSilkScreen( aBoard, aPlotter, layer_mask, plotOpt );
            break;

        default:
            plotOpt.SetSkipPlotNPTH_Pads( false );
            plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

            if( plotOpt.GetFormat() == PLOT_FORMAT_DXF && plotOpt.GetDXFPlotPolygonMode() )
                // PlotLayerOutlines() is designed only for DXF plotters.
                // and must not be used for other plot formats
                PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
            else
                PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
            break;
        }
    }
}


/* Plot a copper layer or mask.
 * Silk screen layers are not plotted here.
 */
void PlotStandardLayer( BOARD *aBoard, PLOTTER* aPlotter,
                        LSET aLayerMask, const PCB_PLOT_PARAMS& aPlotOpt )
{
    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );

    itemplotter.SetLayerSet( aLayerMask );

    EDA_DRAW_MODE_T plotMode = aPlotOpt.GetPlotMode();

     // Plot edge layer and graphic items
    itemplotter.PlotBoardGraphicItems();

    // Draw footprint texts:
    for( auto module : aBoard->Modules() )
    {
        if( ! itemplotter.PlotAllTextsModule( module ) )
        {
            wxLogMessage( _( "Your BOARD has a bad layer number for footprint %s" ),
                          module->GetReference() );
        }
    }

    // Draw footprint other graphic items:
    for( auto module : aBoard->Modules() )
    {
        for( auto item : module->GraphicalItems() )
        {
            if( item->Type() == PCB_MODULE_EDGE_T && aLayerMask[ item->GetLayer() ] )
                itemplotter.Plot_1_EdgeModule( (EDGE_MODULE*) item );
        }
    }

    // Plot footprint pads
    for( auto module : aBoard->Modules() )
    {
        aPlotter->StartBlock( NULL );

        for( auto pad : module->Pads() )
        {
            if( (pad->GetLayerSet() & aLayerMask) == 0 )
                continue;

            wxSize margin;
            double width_adj = 0;

            if( ( aLayerMask & LSET::AllCuMask() ).any() )
                width_adj =  itemplotter.getFineWidthAdj();

            static const LSET speed( 4, B_Mask, F_Mask, B_Paste, F_Paste );

            LSET anded = ( speed & aLayerMask );

            if( anded == LSET( F_Mask ) || anded == LSET( B_Mask ) )
            {
                margin.x = margin.y = pad->GetSolderMaskMargin();
            }
            else if( anded == LSET( F_Paste ) || anded == LSET( B_Paste ) )
            {
                margin = pad->GetSolderPasteMargin();
            }

            // Now offset the pad size by margin + width_adj
            // this is easy for most shapes, but not for a trapezoid or a custom shape
            wxSize padPlotsSize;
            wxSize extraSize = margin * 2;
            extraSize.x += width_adj;
            extraSize.y += width_adj;
            wxSize deltaSize = pad->GetDelta(); // has meaning only for trapezoidal pads

            if( pad->GetShape() == PAD_SHAPE_TRAPEZOID )
            {   // The easy way is to use BuildPadPolygon to calculate
                // size and delta of the trapezoidal pad after offseting:
                wxPoint coord[4];
                pad->BuildPadPolygon( coord, extraSize/2, 0.0 );
                // Calculate the size and delta from polygon corners coordinates:
                // coord[0] is the lower left
                // coord[1] is the upper left
                // coord[2] is the upper right
                // coord[3] is the lower right

                // the size is the distance between middle of segments
                // (left/right or top/bottom)
                // size X is the dist between left and right middle points:
                padPlotsSize.x = ( ( -coord[0].x + coord[3].x )     // the lower segment X length
                                 + ( -coord[1].x + coord[2].x ) )   // the upper segment X length
                                 / 2;           // the Y size is the half sum
                // size Y is the dist between top and bottom middle points:
                padPlotsSize.y = ( ( coord[0].y - coord[1].y )      // the left segment Y lenght
                                 + ( coord[3].y - coord[2].y ) )    // the right segment Y lenght
                                 / 2;           // the Y size is the half sum

                // calculate the delta ( difference of lenght between 2 opposite edges )
                // The delta.x is the delta along the X axis, therefore the delta of Y lenghts
                wxSize delta;

                if( coord[0].y != coord[3].y )
                    delta.x = coord[0].y - coord[3].y;
                else
                    delta.y = coord[1].x - coord[0].x;

                pad->SetDelta( delta );
            }
            else
                padPlotsSize = pad->GetSize() + extraSize;

            // Don't draw a null size item :
            if( padPlotsSize.x <= 0 || padPlotsSize.y <= 0 )
                continue;

            COLOR4D color = COLOR4D::BLACK;

            if( pad->GetLayerSet()[B_Cu] )
               color = aBoard->Colors().GetItemColor( LAYER_PAD_BK );

            if( pad->GetLayerSet()[F_Cu] )
                color = color.LegacyMix( aBoard->Colors().GetItemColor( LAYER_PAD_FR ) );

            // Temporary set the pad size to the required plot size:
            wxSize tmppadsize = pad->GetSize();

            switch( pad->GetShape() )
            {
            case PAD_SHAPE_CIRCLE:
            case PAD_SHAPE_OVAL:
                pad->SetSize( padPlotsSize );

                if( aPlotOpt.GetSkipPlotNPTH_Pads() &&
                    ( aPlotOpt.GetDrillMarksType() == PCB_PLOT_PARAMS::NO_DRILL_SHAPE ) &&
                    ( pad->GetSize() == pad->GetDrillSize() ) &&
                    ( pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED ) )
                    break;

                itemplotter.PlotPad( pad, color, plotMode );
                break;

            case PAD_SHAPE_TRAPEZOID:
            case PAD_SHAPE_RECT:
            case PAD_SHAPE_ROUNDRECT:
            case PAD_SHAPE_CHAMFERED_RECT:
                pad->SetSize( padPlotsSize );
                itemplotter.PlotPad( pad, color, plotMode );
                break;

            case PAD_SHAPE_CUSTOM:
            {
                // inflate/deflate a custom shape is a bit complex.
                // so build a similar pad shape, and inflate/deflate the polygonal shape
                D_PAD dummy( *pad );
                SHAPE_POLY_SET shape;
                pad->MergePrimitivesAsPolygon( &shape );
                // Shape polygon can have holes so use InflateWithLinkedHoles(), not Inflate()
                // which can create bad shapes if margin.x is < 0
                int maxError = aBoard->GetDesignSettings().m_MaxError;
                int numSegs = std::max( GetArcToSegmentCount( margin.x, maxError, 360.0 ), 6 );
                shape.InflateWithLinkedHoles( margin.x, numSegs, SHAPE_POLY_SET::PM_FAST );
                dummy.DeletePrimitivesList();
                dummy.AddPrimitive( shape, 0 );
                dummy.MergePrimitivesAsPolygon();

                // Be sure the anchor pad is not bigger than the deflated shape because this
                // anchor will be added to the pad shape when plotting the pad. So now the
                // polygonal shape is built, we can clamp the anchor size
                if( margin.x < 0 )  // we expect margin.x = margin.y for custom pads
                    dummy.SetSize( padPlotsSize );

                itemplotter.PlotPad( &dummy, color, plotMode );
            }
                break;
            }

            pad->SetSize( tmppadsize );     // Restore the pad size
            pad->SetDelta( deltaSize );
        }

        aPlotter->EndBlock( NULL );
    }

    // Plot vias on copper layers, and if aPlotOpt.GetPlotViaOnMaskLayer() is true,
    // plot them on solder mask

    GBR_METADATA gbr_metadata;

    bool isOnCopperLayer = ( aLayerMask & LSET::AllCuMask() ).any();

    if( isOnCopperLayer )
    {
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_VIAPAD );
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET );
    }

    aPlotter->StartBlock( NULL );

    for( auto track : aBoard->Tracks() )
    {
        const VIA* Via = dyn_cast<const VIA*>( track );

        if( !Via )
            continue;

        // vias are not plotted if not on selected layer, but if layer is SOLDERMASK_LAYER_BACK
        // or SOLDERMASK_LAYER_FRONT, vias are drawn only if they are on the corresponding
        // external copper layer
        LSET via_mask_layer = Via->GetLayerSet();

        if( aPlotOpt.GetPlotViaOnMaskLayer() )
        {
            if( via_mask_layer[B_Cu] )
                via_mask_layer.set( B_Mask );

            if( via_mask_layer[F_Cu] )
                via_mask_layer.set( F_Mask );
        }

        if( !( via_mask_layer & aLayerMask ).any() )
            continue;

        int via_margin = 0;
        double width_adj = 0;

        // If the current layer is a solder mask, use the global mask clearance for vias
        if( aLayerMask[B_Mask] || aLayerMask[F_Mask] )
            via_margin = aBoard->GetDesignSettings().m_SolderMaskMargin;

        if( ( aLayerMask & LSET::AllCuMask() ).any() )
            width_adj = itemplotter.getFineWidthAdj();

        int diameter = Via->GetWidth() + 2 * via_margin + width_adj;

        // Don't draw a null size item :
        if( diameter <= 0 )
            continue;

        // Some vias can be not connected (no net).
        // Set the m_NotInNet for these vias to force a empty net name in gerber file
        gbr_metadata.m_NetlistMetadata.m_NotInNet = Via->GetNetname().IsEmpty();

        gbr_metadata.SetNetName( Via->GetNetname() );

        COLOR4D color = aBoard->Colors().GetItemColor( LAYER_VIAS + Via->GetViaType() );
        // Set plot color (change WHITE to LIGHTGRAY because the white items are not seen on a
        // white paper or screen
        aPlotter->SetColor( color != WHITE ? color : LIGHTGRAY);
        aPlotter->FlashPadCircle( Via->GetStart(), diameter, plotMode, &gbr_metadata );
    }

    aPlotter->EndBlock( NULL );
    aPlotter->StartBlock( NULL );
    gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONDUCTOR );

    // Plot tracks (not vias) :
    for( auto track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            continue;

        if( !aLayerMask[track->GetLayer()] )
            continue;

        // Some track segments can be not connected (no net).
        // Set the m_NotInNet for these segments to force a empty net name in gerber file
        gbr_metadata.m_NetlistMetadata.m_NotInNet = track->GetNetname().IsEmpty();

        gbr_metadata.SetNetName( track->GetNetname() );
        int width = track->GetWidth() + itemplotter.getFineWidthAdj();
        aPlotter->SetColor( itemplotter.getColor( track->GetLayer() ) );
        aPlotter->ThickSegment( track->GetStart(), track->GetEnd(), width, plotMode, &gbr_metadata );
    }

    aPlotter->EndBlock( NULL );

    // Plot filled ares
    aPlotter->StartBlock( NULL );

    // Plot all zones of the same layer & net together so we don't end up with divots where
    // zones touch each other.
    std::set<ZONE_CONTAINER*> plotted;

    for( ZONE_CONTAINER* zone : aBoard->Zones() )
    {
        if( !aLayerMask[ zone->GetLayer() ] || plotted.count( zone ) )
            continue;

        plotted.insert( zone );

        SHAPE_POLY_SET aggregateArea = zone->GetFilledPolysList();

        for( ZONE_CONTAINER* candidate : aBoard->Zones() )
        {
            if( !aLayerMask[ candidate->GetLayer() ] || plotted.count( candidate ) )
                continue;

            if( candidate->GetNetCode() != zone->GetNetCode() )
                continue;

            plotted.insert( candidate );
            aggregateArea.BooleanAdd( candidate->GetFilledPolysList(), SHAPE_POLY_SET::PM_FAST );
        }

        aggregateArea.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
        itemplotter.PlotFilledAreas( zone, aggregateArea );
    }
    aPlotter->EndBlock( NULL );

    // Adding drill marks, if required and if the plotter is able to plot them:
    if( aPlotOpt.GetDrillMarksType() != PCB_PLOT_PARAMS::NO_DRILL_SHAPE )
        itemplotter.PlotDrillMarks();
}


// Seems like we want to plot from back to front?
static const PCB_LAYER_ID plot_seq[] = {

    B_Adhes,        // 32
    F_Adhes,
    B_Paste,
    F_Paste,
    B_SilkS,
    B_Mask,
    F_Mask,
    Dwgs_User,
    Cmts_User,
    Eco1_User,
    Eco2_User,
    Edge_Cuts,
    Margin,

    F_CrtYd,        // CrtYd & Body are footprint only
    B_CrtYd,
    F_Fab,
    B_Fab,

    B_Cu,
    In30_Cu,
    In29_Cu,
    In28_Cu,
    In27_Cu,
    In26_Cu,
    In25_Cu,
    In24_Cu,
    In23_Cu,
    In22_Cu,
    In21_Cu,
    In20_Cu,
    In19_Cu,
    In18_Cu,
    In17_Cu,
    In16_Cu,
    In15_Cu,
    In14_Cu,
    In13_Cu,
    In12_Cu,
    In11_Cu,
    In10_Cu,
    In9_Cu,
    In8_Cu,
    In7_Cu,
    In6_Cu,
    In5_Cu,
    In4_Cu,
    In3_Cu,
    In2_Cu,
    In1_Cu,
    F_Cu,

    F_SilkS,
};


/*
 * Plot outlines of copper, for copper layer
 */
void PlotLayerOutlines( BOARD* aBoard, PLOTTER* aPlotter, LSET aLayerMask,
                        const PCB_PLOT_PARAMS& aPlotOpt )
{

    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerSet( aLayerMask );

    SHAPE_POLY_SET outlines;

    for( LSEQ seq = aLayerMask.Seq( plot_seq, arrayDim( plot_seq ) );  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;

        outlines.RemoveAllContours();
        aBoard->ConvertBrdLayerToPolygonalContours( layer, outlines );

        outlines.Simplify( SHAPE_POLY_SET::PM_FAST );

        // Plot outlines
        std::vector< wxPoint > cornerList;

        // Now we have one or more basic polygons: plot each polygon
        for( int ii = 0; ii < outlines.OutlineCount(); ii++ )
        {
            for(int kk = 0; kk <= outlines.HoleCount (ii); kk++ )
            {
                cornerList.clear();
                const SHAPE_LINE_CHAIN& path = (kk == 0) ? outlines.COutline( ii ) : outlines.CHole( ii, kk - 1 );

                for( int jj = 0; jj < path.PointCount(); jj++ )
                    cornerList.emplace_back( (wxPoint) path.CPoint( jj ) );

                // Ensure the polygon is closed
                if( cornerList[0] != cornerList[cornerList.size() - 1] )
                    cornerList.push_back( cornerList[0] );

                aPlotter->PlotPoly( cornerList, NO_FILL );
            }
        }

        // Plot pad holes
        if( aPlotOpt.GetDrillMarksType() != PCB_PLOT_PARAMS::NO_DRILL_SHAPE )
        {
            int smallDrill = (aPlotOpt.GetDrillMarksType() == PCB_PLOT_PARAMS::SMALL_DRILL_SHAPE)
                                  ? SMALL_DRILL : INT_MAX;

            for( auto module : aBoard->Modules() )
            {
                for( auto pad : module->Pads() )
                {
                    wxSize hole = pad->GetDrillSize();

                    if( hole.x == 0 || hole.y == 0 )
                        continue;

                    if( hole.x == hole.y )
                    {
                        hole.x = std::min( smallDrill, hole.x );
                        aPlotter->Circle( pad->GetPosition(), hole.x, NO_FILL );
                    }
                    else
                    {
                        // Note: small drill marks have no significance when applied to slots
                        wxPoint drl_start, drl_end;
                        int width;
                        pad->GetOblongDrillGeometry( drl_start, drl_end, width );
                        aPlotter->ThickSegment( pad->GetPosition() + drl_start,
                                                pad->GetPosition() + drl_end, width, SKETCH, NULL );
                    }
                }
            }
        }

        // Plot vias holes
        for( auto track : aBoard->Tracks() )
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
 * 4 - ORing result by all pad shapes as polygons with a size inflated by
 *      mask clearance only (because deflate sometimes creates shape artifacts)
 * 5 - draw result as polygons
 *
 * TODO:
 * make this calculation only for shapes with clearance near than (min width solder mask)
 * (using DRC algo)
 * plot all other shapes by flashing the basing shape
 * (shapes will be better, and calculations faster)
 */
void PlotSolderMaskLayer( BOARD *aBoard, PLOTTER* aPlotter, LSET aLayerMask,
                          const PCB_PLOT_PARAMS& aPlotOpt, int aMinThickness )
{
    PCB_LAYER_ID    layer = aLayerMask[B_Mask] ? B_Mask : F_Mask;

    // We remove 1nm as we expand both sides of the shapes, so allowing for
    // a strictly greater than or equal comparison in the shape separation (boolean add)
    // means that we will end up with separate shapes that then are shrunk
    int             inflate = aMinThickness/2 - 1;

    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerSet( aLayerMask );

    // Plot edge layer and graphic items.
    // They do not have a solder Mask margin, because they  graphic items
    // on this layer (like logos), not actually areas around pads.
    itemplotter.PlotBoardGraphicItems();

    for( auto module : aBoard->Modules() )
    {
        for( auto item : module->GraphicalItems() )
        {
            itemplotter.PlotAllTextsModule( module );

            if( item->Type() == PCB_MODULE_EDGE_T && item->GetLayer() == layer )
                itemplotter.Plot_1_EdgeModule( (EDGE_MODULE*) item );
        }
    }

    // Build polygons for each pad shape.  The size of the shape on solder mask should be size
    // of pad + clearance around the pad, where clearance = solder mask clearance + extra margin.
    // Extra margin is half the min width for solder mask, which is used to merge too-close shapes
    // (distance < aMinThickness), and will be removed when creating the actual shapes.
    SHAPE_POLY_SET areas;           // Contains shapes to plot
    SHAPE_POLY_SET initialPolys;    // Contains exact shapes to plot

    // Plot pads
    for( auto module : aBoard->Modules() )
    {
        // add shapes with exact size
        module->TransformPadsShapesWithClearanceToPolygon( layer, initialPolys, 0 );
        // add shapes inflated by aMinThickness/2
        module->TransformPadsShapesWithClearanceToPolygon( layer, areas, inflate );
    }

    // Plot vias on solder masks, if aPlotOpt.GetPlotViaOnMaskLayer() is true,
    if( aPlotOpt.GetPlotViaOnMaskLayer() )
    {
        // The current layer is a solder mask, use the global mask clearance for vias
        int via_clearance = aBoard->GetDesignSettings().m_SolderMaskMargin;
        int via_margin = via_clearance + inflate;

        for( auto track : aBoard->Tracks() )
        {
            const VIA* via = dyn_cast<const VIA*>( track );

            if( !via )
                continue;

            // vias are plotted only if they are on the corresponding external copper layer
            LSET via_set = via->GetLayerSet();

            if( via_set[B_Cu] )
                via_set.set( B_Mask );

            if( via_set[F_Cu] )
                via_set.set( F_Mask );

            if( !( via_set & aLayerMask ).any() )
                continue;

            via->TransformShapeWithClearanceToPolygon( areas, via_margin );
            via->TransformShapeWithClearanceToPolygon( initialPolys, via_clearance );
        }
    }

    // Add filled zone areas.
#if 0   // Set to 1 if a solder mask margin must be applied to zones on solder mask
    int zone_margin = aBoard->GetDesignSettings().m_SolderMaskMargin;
#else
    int zone_margin = 0;
#endif

    for( ZONE_CONTAINER* zone : aBoard->Zones() )
    {
        if( zone->GetLayer() != layer )
            continue;

        // Some intersecting zones, despite being on the same layer with the same net, cannot be
        // merged due to other parameters such as fillet radius.  The copper pour will end up
        // effectively merged though, so we want to keep the corners of such intersections sharp.
        std::set<VECTOR2I> colinearCorners;
        zone->GetColinearCorners( aBoard, colinearCorners );

        zone->TransformOutlinesShapeWithClearanceToPolygon( areas, inflate + zone_margin, false,
                                                            &colinearCorners );
        zone->TransformOutlinesShapeWithClearanceToPolygon( initialPolys, zone_margin, false,
                                                            &colinearCorners );
    }

    // To avoid a lot of code, use a ZONE_CONTAINER to handle and plot polygons, because our
    // polygons look exactly like filled areas in zones.
    // Note, also this code is not optimized: it creates a lot of copy/duplicate data.
    // However it is not complex, and fast enough for plot purposes (copy/convert data is only a
    // very small calculation time for these calculations).
    ZONE_CONTAINER zone( aBoard );
    zone.SetMinThickness( 0 );      // trace polygons only
    zone.SetLayer( layer );
    int maxError = aBoard->GetDesignSettings().m_MaxError;
    int numSegs = std::max( GetArcToSegmentCount( inflate, maxError, 360.0 ), 6 );

    areas.BooleanAdd( initialPolys, SHAPE_POLY_SET::PM_FAST );
    areas.Deflate( inflate, numSegs );

    // Combine the current areas to initial areas. This is mandatory because inflate/deflate
    // transform is not perfect, and we want the initial areas perfectly kept
    areas.BooleanAdd( initialPolys, SHAPE_POLY_SET::PM_FAST );
    areas.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    itemplotter.PlotFilledAreas( &zone, areas );
}


/**
 * Set up most plot options for plotting a board (especially the viewport)
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

    // Special options: to fit the sheet to an A4 sheet replace the paper size. However there
    // is a difference between the autoscale and the a4paper option:
    //  - Autoscale fits the board to the paper size
    //  - A4paper fits the original paper size to an A4 sheet
    //  - Both of them fit the board to an A4 sheet
    if( aPlotOpts->GetA4Output() )
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

    // Fit to 80% of the page if asked; it could be that the board is empty, in this case
    // regress to 1:1 scale
    if( aPlotOpts->GetAutoScale() && boardSize.x > 0 && boardSize.y > 0 )
    {
        double xscale = (paperSizeIU.x * 0.8) / boardSize.x;
        double yscale = (paperSizeIU.y * 0.8) / boardSize.y;

        compound_scale = std::min( xscale, yscale ) * paperscale;
    }
    else
        compound_scale = aPlotOpts->GetScale() * paperscale;


    // For the plot offset we have to keep in mind the auxiliary origin too: if autoscaling is
    // off we check that plot option (i.e. autoscaling overrides auxiliary origin)
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

    aPlotter->SetPageSettings( *sheet_info );

    aPlotter->SetViewport( offset, IU_PER_MILS/10, compound_scale, aPlotOpts->GetMirror() );
    // Has meaning only for gerber plotter. Must be called only after SetViewport
    aPlotter->SetGerberCoordinatesFormat( aPlotOpts->GetGerberPrecision() );

    aPlotter->SetDefaultLineWidth( aPlotOpts->GetLineWidth() );
    aPlotter->SetCreator( wxT( "PCBNEW" ) );
    aPlotter->SetColorMode( false );        // default is plot in Black and White.
    aPlotter->SetTextMode( aPlotOpts->GetTextMode() );
}


/**
 * Prefill in black an area a little bigger than the board to prepare for the negative plot
 */
static void FillNegativeKnockout( PLOTTER *aPlotter, const EDA_RECT &aBbbox )
{
    const int margin = 5 * IU_PER_MM;   // Add a 5 mm margin around the board
    aPlotter->SetNegative( true );
    aPlotter->SetColor( WHITE );        // Which will be plotted as black
    EDA_RECT area = aBbbox;
    area.Inflate( margin );
    aPlotter->Rect( area.GetOrigin(), area.GetEnd(), FILLED_SHAPE );
    aPlotter->SetColor( BLACK );
}


/**
 * Calculate the effective size of HPGL pens and set them in the plotter object
 */
static void ConfigureHPGLPenSizes( HPGL_PLOTTER *aPlotter, PCB_PLOT_PARAMS *aPlotOpts )
{
    // Compute penDiam (the value is given in mils) in pcb units, with plot scale (if Scale is 2,
    // penDiam value is always m_HPGLPenDiam so apparent penDiam is actually penDiam / Scale
    int penDiam = KiROUND( aPlotOpts->GetHPGLPenDiameter() * IU_PER_MILS / aPlotOpts->GetScale() );

    // Set HPGL-specific options and start
    aPlotter->SetPenSpeed( aPlotOpts->GetHPGLPenSpeed() );
    aPlotter->SetPenNumber( aPlotOpts->GetHPGLPenNum() );
    aPlotter->SetPenDiameter( penDiam );
}


/**
 * Open a new plotfile using the options (and especially the format) specified in the options
 * and prepare the page for plotting.
 * Return the plotter object if OK, NULL if the file is not created (or has a problem)
 */
PLOTTER* StartPlotBoard( BOARD *aBoard, PCB_PLOT_PARAMS *aPlotOpts, int aLayer,
                         const wxString& aFullFileName, const wxString& aSheetDesc )
{
    // Create the plotter driver and set the few plotter specific options
    PLOTTER*    plotter = NULL;

    switch( aPlotOpts->GetFormat() )
    {
    case PLOT_FORMAT_DXF:
        DXF_PLOTTER* DXF_plotter;
        DXF_plotter = new DXF_PLOTTER();
        DXF_plotter->SetUnits(
                static_cast<DXF_PLOTTER::DXF_UNITS>( aPlotOpts->GetDXFPlotUnits() ) );

        plotter = DXF_plotter;
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

        // HPGL options are a little more convoluted to compute, so they get their own function
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

    // page layout is not mirrored, so temporarily change mirror option for the page layout
    PCB_PLOT_PARAMS plotOpts = *aPlotOpts;

    if( plotOpts.GetPlotFrameRef() && plotOpts.GetMirror() )
        plotOpts.SetMirror( false );

    initializePlotter( plotter, aBoard, &plotOpts );

    if( plotter->OpenFile( aFullFileName ) )
    {
        plotter->ClearHeaderLinesList();

        // For the Gerber "file function" attribute, set the layer number
        if( plotter->GetPlotterType() == PLOT_FORMAT_GERBER )
        {
            bool useX2mode = plotOpts.GetUseGerberX2format();

            GERBER_PLOTTER* gbrplotter = static_cast <GERBER_PLOTTER*> ( plotter );
            gbrplotter->UseX2format( useX2mode );
            gbrplotter->UseX2NetAttributes( plotOpts.GetIncludeGerberNetlistInfo() );

            // Attributes can be added using X2 format or as comment (X1 format)
            AddGerberX2Attribute( plotter, aBoard, aLayer, not useX2mode );
        }

        plotter->StartPlot();

        // Plot the frame reference if requested
        if( aPlotOpts->GetPlotFrameRef() )
        {
            PlotWorkSheet( plotter, aBoard->GetTitleBlock(), aBoard->GetPageSettings(),
                           1, 1, aSheetDesc, aBoard->GetFileName() );

            if( aPlotOpts->GetMirror() )
                initializePlotter( plotter, aBoard, aPlotOpts );
        }

        // When plotting a negative board: draw a black rectangle (background for plot board
        // in white) and switch the current color to WHITE; note the color inversion is actually
        // done in the driver (if supported)
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
