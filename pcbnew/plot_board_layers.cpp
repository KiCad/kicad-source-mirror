/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/log.h>
#include <eda_item.h>
#include <layer_ids.h>
#include <lset.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_segment.h>
#include <pcb_base_frame.h>
#include <math/util.h>      // for KiROUND
#include <board.h>
#include <footprint.h>
#include <pcb_track.h>
#include <pad.h>
#include <zone.h>
#include <pcb_shape.h>
#include <pcb_target.h>
#include <pcb_dimension.h>
#include <pcbplot.h>
#include <plotters/plotter.h>
#include <plotters/plotter_dxf.h>
#include <plotters/plotter_gerber.h>
#include <plotters/plotters_pslike.h>
#include <pcb_painter.h>
#include <gbr_metadata.h>
#include <advanced_config.h>

void GenerateLayerPoly( SHAPE_POLY_SET* aResult, BOARD *aBoard, PLOTTER* aPlotter, PCB_LAYER_ID aLayer,
                        bool aPlotFPText, bool aPlotReferences, bool aPlotValues );


void PlotLayer( BOARD* aBoard, PLOTTER* aPlotter, const LSET& layerMask,
                const PCB_PLOT_PARAMS& plotOpts )
{
    // PlotLayerOutlines() is designed only for DXF plotters.
    if( plotOpts.GetFormat() == PLOT_FORMAT::DXF && plotOpts.GetDXFPlotPolygonMode() )
        PlotLayerOutlines( aBoard, aPlotter, layerMask, plotOpts );
    else
        PlotStandardLayer( aBoard, aPlotter, layerMask, plotOpts );
};


void PlotPolySet( BOARD* aBoard, PLOTTER* aPlotter, const PCB_PLOT_PARAMS& aPlotOpt,
                  SHAPE_POLY_SET* aPolySet, PCB_LAYER_ID aLayer )
{
    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    LSET             layers = { aLayer };

    itemplotter.SetLayerSet( layers );

    // To avoid a lot of code, use a ZONE to handle and plot polygons, because our polygons look
    // exactly like filled areas in zones.
    // Note, also this code is not optimized: it creates a lot of copy/duplicate data.
    // However it is not complex, and fast enough for plot purposes (copy/convert data is only a
    // very small calculation time for these calculations).
    ZONE zone( aBoard );
    zone.SetMinThickness( 0 );
    zone.SetLayer( aLayer );

    aPolySet->Fracture();
    itemplotter.PlotZone( &zone, aLayer, *aPolySet );
}


/**
 * Plot a solder mask layer.
 *
 * Solder mask layers have a minimum thickness value and cannot be drawn like standard layers,
 * unless the minimum thickness is 0.
 */
void PlotSolderMaskLayer( BOARD* aBoard, PLOTTER* aPlotter, const LSET& aLayerMask,
                          const PCB_PLOT_PARAMS& aPlotOpt )
{
    if( aBoard->GetDesignSettings().m_SolderMaskMinWidth == 0 )
    {
        PlotLayer( aBoard, aPlotter, aLayerMask, aPlotOpt );
        return;
    }

    SHAPE_POLY_SET solderMask;
    PCB_LAYER_ID   layer = aLayerMask[B_Mask] ? B_Mask : F_Mask;

    GenerateLayerPoly( &solderMask, aBoard, aPlotter, layer, aPlotOpt.GetPlotFPText(),
                       aPlotOpt.GetPlotReference(), aPlotOpt.GetPlotValue() );

    PlotPolySet( aBoard, aPlotter, aPlotOpt, &solderMask, layer );
}


void PlotClippedSilkLayer( BOARD* aBoard, PLOTTER* aPlotter, const LSET& aLayerMask,
                           const PCB_PLOT_PARAMS& aPlotOpt )
{
    SHAPE_POLY_SET silkscreen, solderMask;
    PCB_LAYER_ID   silkLayer = aLayerMask[F_SilkS] ? F_SilkS : B_SilkS;
    PCB_LAYER_ID   maskLayer = aLayerMask[F_SilkS] ? F_Mask : B_Mask;

    GenerateLayerPoly( &silkscreen, aBoard, aPlotter, silkLayer, aPlotOpt.GetPlotFPText(),
                       aPlotOpt.GetPlotReference(), aPlotOpt.GetPlotValue() );
    GenerateLayerPoly( &solderMask, aBoard, aPlotter, maskLayer, aPlotOpt.GetPlotFPText(),
                       aPlotOpt.GetPlotReference(), aPlotOpt.GetPlotValue() );

    silkscreen.BooleanSubtract( solderMask );
    PlotPolySet( aBoard, aPlotter, aPlotOpt, &silkscreen, silkLayer );
}


void PlotBoardLayers( BOARD* aBoard, PLOTTER* aPlotter, const LSEQ& aLayers,
                      const PCB_PLOT_PARAMS& aPlotOptions )
{
    if( !aBoard || !aPlotter || aLayers.empty() )
        return;

    for( PCB_LAYER_ID layer : aLayers )
        PlotOneBoardLayer( aBoard, aPlotter, layer, aPlotOptions, layer == aLayers[0] );

    // Drill marks are plotted in white to knockout the pad if any layers of the pad are
    // being plotted, and in black if the pad is not being plotted. For the former, this
    // must happen after all other layers are plotted.
    if( aPlotOptions.GetDrillMarksType() != DRILL_MARKS::NO_DRILL_SHAPE )
    {
        BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOptions );
        itemplotter.SetLayerSet( aLayers );
        itemplotter.PlotDrillMarks();
    }
}


void PlotInteractiveLayer( BOARD* aBoard, PLOTTER* aPlotter, const PCB_PLOT_PARAMS& aPlotOpt )
{
    for( const FOOTPRINT* fp : aBoard->Footprints() )
    {
        if( fp->GetLayer() == F_Cu && !aPlotOpt.m_PDFFrontFPPropertyPopups )
            continue;

        if( fp->GetLayer() == B_Cu && !aPlotOpt.m_PDFBackFPPropertyPopups )
            continue;

        std::vector<wxString> properties;

        properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                   _( "Reference designator" ),
                                                   fp->Reference().GetShownText( false ) ) );

        properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                   _( "Value" ),
                                                   fp->Value().GetShownText( false ) ) );

        properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                   _( "Footprint" ),
                                                   fp->GetFPID().GetUniStringLibItemName() ) );

        for( const PCB_FIELD* field : fp->GetFields() )
        {
            wxCHECK2( field, continue );

            if( field->IsReference() || field->IsValue() )
                continue;

            if( field->GetText().IsEmpty() )
                continue;

            properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                       field->GetName(),
                                                       field->GetText() ) );
        }

        // These 2 properties are not very useful in a plot file (like a PDF)
#if 0
        properties.emplace_back( wxString::Format( wxT( "!%s = %s" ), _( "Library Description" ),
                                                   fp->GetLibDescription() ) );

        properties.emplace_back( wxString::Format( wxT( "!%s = %s" ), _( "Keywords" ),
                                                   fp->GetKeywords() ) );
#endif
        // Draw items are plotted with a position offset. So we need to move
        // our boxes (which are not plotted) by the same offset.
        VECTOR2I offset = -aPlotter->GetPlotOffsetUserUnits();

        // Use a footprint bbox without texts to create the hyperlink area
        BOX2I bbox = fp->GetBoundingBox( false );
        bbox.Move( offset );
        aPlotter->HyperlinkMenu( bbox, properties );

        // Use a footprint bbox with visible texts only to create the bookmark area
        // which is the area to zoom on ft selection
        // However the bbox need to be inflated for a better look.
        bbox = fp->GetBoundingBox( true );
        bbox.Move( offset );
        bbox.Inflate( bbox.GetWidth() /2, bbox.GetHeight() /2 );
        aPlotter->Bookmark( bbox, fp->GetReference(), _( "Footprints" ) );
    }
}


void PlotOneBoardLayer( BOARD *aBoard, PLOTTER* aPlotter, PCB_LAYER_ID aLayer,
                        const PCB_PLOT_PARAMS& aPlotOpt, bool isPrimaryLayer )
{
    PCB_PLOT_PARAMS plotOpt = aPlotOpt;

    // Set a default color and the text mode for this layer
    aPlotter->SetColor( BLACK );
    aPlotter->SetTextMode( aPlotOpt.GetTextMode() );

    // Specify that the contents of the "Edges Pcb" layer are to be plotted in addition to the
    // contents of the currently specified layer.
    LSET    layer_mask( { aLayer } );

    if( IsCopperLayer( aLayer ) )
    {
        // Skip NPTH pads on copper layers ( only if hole size == pad size ):
        // Drill mark will be plotted if drill mark is SMALL_DRILL_SHAPE or FULL_DRILL_SHAPE
        if( plotOpt.GetFormat() == PLOT_FORMAT::DXF )
            plotOpt.SetDXFPlotPolygonMode( true );
        else
            plotOpt.SetSkipPlotNPTH_Pads( true );

        PlotLayer( aBoard, aPlotter, layer_mask, plotOpt );
    }
    else
    {
        switch( aLayer )
        {
        case B_Mask:
        case F_Mask:
            // Use outline mode for DXF
            plotOpt.SetDXFPlotPolygonMode( true );

            // Plot solder mask:
            PlotSolderMaskLayer( aBoard, aPlotter, layer_mask, plotOpt );

            break;

        case B_Adhes:
        case F_Adhes:
        case B_Paste:
        case F_Paste:
            // Disable plot pad holes
            plotOpt.SetDrillMarksType( DRILL_MARKS::NO_DRILL_SHAPE );

            // Use outline mode for DXF
            plotOpt.SetDXFPlotPolygonMode( true );

            PlotLayer( aBoard, aPlotter, layer_mask, plotOpt );

            break;

        case F_SilkS:
        case B_SilkS:
            if( plotOpt.GetSubtractMaskFromSilk() )
            {
                if( aPlotter->GetPlotterType() == PLOT_FORMAT::GERBER && isPrimaryLayer )
                {
                    // Use old-school, positive/negative mask plotting which preserves utilization
                    // of Gerber aperture masks.  This method can only be used when the given silk
                    // layer is the primary layer as the negative mask will also knockout any other
                    // (non-silk) layers that were plotted before the silk layer.

                    PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );

                    // Create the mask to subtract by creating a negative layer polarity
                    aPlotter->SetLayerPolarity( false );

                    // Disable plot pad holes
                    plotOpt.SetDrillMarksType( DRILL_MARKS::NO_DRILL_SHAPE );

                    // Plot the mask
                    layer_mask = ( aLayer == F_SilkS ) ? LSET( { F_Mask } ) : LSET( { B_Mask } );
                    PlotSolderMaskLayer( aBoard, aPlotter, layer_mask, plotOpt );

                    // Disable the negative polarity
                    aPlotter->SetLayerPolarity( true );
                }
                else
                {
                    PlotClippedSilkLayer( aBoard, aPlotter, layer_mask, plotOpt );
                }

                break;
            }

            PlotLayer( aBoard, aPlotter, layer_mask, plotOpt );
            break;

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
        default:
            PlotLayer( aBoard, aPlotter, layer_mask, plotOpt );
            break;
        }
    }
}


/**
 * Plot any layer EXCEPT a solder-mask with an enforced minimum width.
 */
void PlotStandardLayer( BOARD* aBoard, PLOTTER* aPlotter, const LSET& aLayerMask,
                        const PCB_PLOT_PARAMS& aPlotOpt )
{
    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    int              maxError = aBoard->GetDesignSettings().m_MaxError;

    itemplotter.SetLayerSet( aLayerMask );

    bool onCopperLayer = ( LSET::AllCuMask() & aLayerMask ).any();
    bool onSolderMaskLayer = ( LSET( { F_Mask, B_Mask } ) & aLayerMask ).any();
    bool onSolderPasteLayer = ( LSET( { F_Paste, B_Paste } ) & aLayerMask ).any();
    bool onFrontFab = ( LSET( { F_Fab } ) & aLayerMask ).any();
    bool onBackFab  = ( LSET( { B_Fab } ) & aLayerMask ).any();
    bool sketchPads = ( onFrontFab || onBackFab ) && aPlotOpt.GetSketchPadsOnFabLayers();
    const wxString variantName = aBoard->GetCurrentVariant();

    // Plot edge layer and graphic items
    for( const BOARD_ITEM* item : aBoard->Drawings() )
        itemplotter.PlotBoardGraphicItem( item );

    // Draw footprint texts:
    for( const FOOTPRINT* footprint : aBoard->Footprints() )
        itemplotter.PlotFootprintTextItems( footprint );

    // Draw footprint other graphic items:
    for( const FOOTPRINT* footprint : aBoard->Footprints() )
        itemplotter.PlotFootprintGraphicItems( footprint );

    // Plot footprint pads
    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        const bool dnp = footprint->GetDNPForVariant( variantName );

        aPlotter->StartBlock( nullptr );

        for( PAD* pad : footprint->Pads() )
        {
            bool doSketchPads = false;

            if( !( pad->GetLayerSet() & aLayerMask ).any() )
            {
                if( sketchPads && (   ( onFrontFab && pad->GetLayerSet().Contains( F_Cu ) )
                                    || ( onBackFab && pad->GetLayerSet().Contains( B_Cu ) ) ) )
                {
                    doSketchPads = true;
                }
                else
                {
                    continue;
                }
            }

            if( onCopperLayer && !pad->IsOnCopperLayer() )
                continue;

            /// pads not connected to copper are optionally not drawn
            if( onCopperLayer && !pad->FlashLayer( aLayerMask ) )
                continue;

            // TODO(JE) padstacks - different behavior for single layer or multilayer

            COLOR4D color = COLOR4D::BLACK;

            // If we're plotting a single layer, the color for that layer can be used directly.
            if( aLayerMask.count() == 1 )
            {
                color = aPlotOpt.ColorSettings()->GetColor( aLayerMask.Seq()[0] );
            }
            else
            {
                if( ( pad->GetLayerSet() & aLayerMask )[B_Cu] )
                    color = aPlotOpt.ColorSettings()->GetColor( B_Cu );

                if( ( pad->GetLayerSet() & aLayerMask )[F_Cu] )
                    color = color.LegacyMix( aPlotOpt.ColorSettings()->GetColor( F_Cu ) );

                if( sketchPads && aLayerMask[F_Fab] )
                    color = aPlotOpt.ColorSettings()->GetColor( F_Fab );
                else if( sketchPads && aLayerMask[B_Fab] )
                    color = aPlotOpt.ColorSettings()->GetColor( B_Fab );
            }

            if( sketchPads && (   ( onFrontFab && pad->GetLayerSet().Contains( F_Cu ) )
                                || ( onBackFab && pad->GetLayerSet().Contains( B_Cu ) ) ) )
            {
                if( aPlotOpt.GetPlotPadNumbers() )
                    itemplotter.PlotPadNumber( pad, color );
            }

            auto plotPadLayer =
                [&]( PCB_LAYER_ID aLayer )
                {
                    VECTOR2I margin;
                    int width_adj = 0;

                    if( onCopperLayer )
                        width_adj = itemplotter.getFineWidthAdj();

                    if( onSolderMaskLayer )
                        margin.x = margin.y = pad->GetSolderMaskExpansion( aLayer );

                    if( onSolderPasteLayer )
                        margin = pad->GetSolderPasteMargin( aLayer );

                    // not all shapes can have a different margin for x and y axis
                    // in fact only oval and rect shapes can have different values.
                    // Round shape have always the same x,y margin
                    // so define a unique value for other shapes that do not support different values
                    int mask_clearance = margin.x;
                    // When clearance is same for x and y pad axis, calculations are more easy
                    bool sameXYClearance = margin.x == margin.y;

                    // Now offset the pad size by margin + width_adj
                    VECTOR2I padPlotsSize = pad->GetSize( aLayer ) + margin * 2 + VECTOR2I( width_adj, width_adj );

                    // Store these parameters that can be modified to plot inflated/deflated pads shape
                    PAD_SHAPE padShape = pad->GetShape( aLayer );
                    VECTOR2I  padSize = pad->GetSize( aLayer );
                    VECTOR2I  padDelta = pad->GetDelta( aLayer ); // has meaning only for trapezoidal pads
                    // CornerRadius and CornerRadiusRatio can be modified
                    // the radius is built from the ratio, so saving/restoring the ratio is enough
                    double    padCornerRadiusRatio = pad->GetRoundRectRadiusRatio( aLayer );

                    // Don't draw a 0 sized pad.
                    // Note: a custom pad can have its pad anchor with size = 0
                    if( padShape != PAD_SHAPE::CUSTOM
                        && ( padPlotsSize.x <= 0 || padPlotsSize.y <= 0 ) )
                    {
                        return;
                    }

                    switch( padShape )
                    {
                    case PAD_SHAPE::CIRCLE:
                    case PAD_SHAPE::OVAL:
                        pad->SetSize( aLayer, padPlotsSize );

                        if( aPlotOpt.GetSkipPlotNPTH_Pads() &&
                            ( aPlotOpt.GetDrillMarksType() == DRILL_MARKS::NO_DRILL_SHAPE ) &&
                            ( pad->GetSize(aLayer ) == pad->GetDrillSize() ) &&
                            ( pad->GetAttribute() == PAD_ATTRIB::NPTH ) )
                        {
                            break;
                        }

                        itemplotter.PlotPad( pad, aLayer, color, doSketchPads );
                        break;

                    case PAD_SHAPE::RECTANGLE:
                        pad->SetSize( aLayer, padPlotsSize );

                        if( mask_clearance > 0 )
                        {
                            pad->SetShape( aLayer, PAD_SHAPE::ROUNDRECT );
                            pad->SetRoundRectCornerRadius( aLayer, mask_clearance );
                        }

                        itemplotter.PlotPad( pad, aLayer, color, doSketchPads );
                        break;

                    case PAD_SHAPE::TRAPEZOID:
                        // inflate/deflate a trapezoid is a bit complex.
                        // so if the margin is not null, build a similar polygonal pad shape,
                        // and inflate/deflate the polygonal shape
                        // because inflating/deflating using different values for y and y
                        // we are using only margin.x as inflate/deflate value
                        if( mask_clearance == 0 )
                        {
                            itemplotter.PlotPad( pad, aLayer, color, doSketchPads );
                        }
                        else
                        {
                            PAD dummy( *pad );
                            dummy.SetAnchorPadShape( aLayer, PAD_SHAPE::CIRCLE );
                            dummy.SetShape( aLayer, PAD_SHAPE::CUSTOM );
                            SHAPE_POLY_SET outline;
                            outline.NewOutline();
                            int dx = padSize.x / 2;
                            int dy = padSize.y / 2;
                            int ddx = padDelta.x / 2;
                            int ddy = padDelta.y / 2;

                            outline.Append( -dx - ddy,  dy + ddx );
                            outline.Append(  dx + ddy,  dy - ddx );
                            outline.Append(  dx - ddy, -dy + ddx );
                            outline.Append( -dx + ddy, -dy - ddx );

                            // Shape polygon can have holes so use InflateWithLinkedHoles(), not Inflate()
                            // which can create bad shapes if margin.x is < 0
                            outline.InflateWithLinkedHoles( mask_clearance, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                                            maxError );
                            dummy.DeletePrimitivesList();
                            dummy.AddPrimitivePoly( aLayer, outline, 0, true );

                            // Be sure the anchor pad is not bigger than the deflated shape because this
                            // anchor will be added to the pad shape when plotting the pad. So now the
                            // polygonal shape is built, we can clamp the anchor size
                            dummy.SetSize( aLayer, VECTOR2I( 0, 0 ) );

                            itemplotter.PlotPad( &dummy, aLayer, color, doSketchPads );
                        }

                        break;

                    case PAD_SHAPE::ROUNDRECT:
                    {
                        // rounding is stored as a percent, but we have to update this ratio
                        // to force recalculation of other values after size changing (we do not
                        // really change the rounding percent value)
                        double radius_ratio = pad->GetRoundRectRadiusRatio( aLayer );
                        pad->SetSize( aLayer, padPlotsSize );
                        pad->SetRoundRectRadiusRatio( aLayer, radius_ratio );

                        itemplotter.PlotPad( pad, aLayer, color, doSketchPads );
                        break;
                    }

                    case PAD_SHAPE::CHAMFERED_RECT:
                        // for smaller/same rect size than initial shape (i.e. mask_clearance <= 0)
                        // use the rect with size set to padPlotsSize. It gives a good shape
                        if( mask_clearance <= 0 )
                        {
                            // the size can be slightly inflated by width_adj (PS/PDF only)
                            pad->SetSize( aLayer, padPlotsSize );
                            itemplotter.PlotPad( pad, aLayer, color, doSketchPads );
                        }
                        else
                        {
                            // Due to the polygonal shape of a CHAMFERED_RECT pad, the best way is to
                            // convert the pad shape to a full polygon and inflate it
                            // and use a dummy  CUSTOM pad to plot the final shape.
                            // However one can inflate polygon only if X,Y has same inflate value
                            // if not the case, just use a rectangle having the padPlotsSize new size
                            PAD dummy( *pad );
                            // Build the dummy pad outline with coordinates relative to the pad position
                            // pad offset and orientation 0. The actual pos, offset and rotation will be
                            // taken in account later by the plot function
                            dummy.SetPosition( VECTOR2I( 0, 0 ) );
                            dummy.SetOffset( aLayer, VECTOR2I( 0, 0 ) );

                            if( !sameXYClearance )
                                dummy.SetSize( aLayer, padPlotsSize );

                            dummy.SetOrientation( ANGLE_0 );
                            SHAPE_POLY_SET outline;
                            dummy.TransformShapeToPolygon( outline, aLayer, 0, maxError, ERROR_INSIDE );

                            if( sameXYClearance )
                                outline.InflateWithLinkedHoles( mask_clearance, CORNER_STRATEGY::ROUND_ALL_CORNERS,
                                                                maxError );

                            // Initialize the dummy pad shape:
                            dummy.SetAnchorPadShape( aLayer, PAD_SHAPE::CIRCLE );
                            dummy.SetShape( aLayer, PAD_SHAPE::CUSTOM );
                            dummy.DeletePrimitivesList();
                            dummy.AddPrimitivePoly( aLayer, outline, 0, true );

                            // Be sure the anchor pad is not bigger than the deflated shape because this
                            // anchor will be added to the pad shape when plotting the pad.
                            // So we set the anchor size to 0
                            dummy.SetSize( aLayer, VECTOR2I( 0, 0 ) );
                            // Restore pad position and offset
                            dummy.SetPosition( pad->GetPosition() );
                            dummy.SetOffset( aLayer, pad->GetOffset( aLayer ) );
                            dummy.SetOrientation( pad->GetOrientation() );

                            itemplotter.PlotPad( &dummy, aLayer, color, doSketchPads );
                        }

                        break;

                    case PAD_SHAPE::CUSTOM:
                    {
                        // inflate/deflate a custom shape is a bit complex.
                        // so build a similar pad shape, and inflate/deflate the polygonal shape
                        PAD dummy( *pad );
                        dummy.SetParentGroup( nullptr );

                        SHAPE_POLY_SET shape;
                        pad->MergePrimitivesAsPolygon( aLayer, &shape );

                        // Shape polygon can have holes so use InflateWithLinkedHoles(), not Inflate()
                        // which can create bad shapes if margin.x is < 0
                        shape.InflateWithLinkedHoles( mask_clearance,
                                                      CORNER_STRATEGY::ROUND_ALL_CORNERS, maxError );
                        dummy.DeletePrimitivesList();
                        dummy.AddPrimitivePoly( aLayer, shape, 0, true );

                        // Be sure the anchor pad is not bigger than the deflated shape because this
                        // anchor will be added to the pad shape when plotting the pad. So now the
                        // polygonal shape is built, we can clamp the anchor size
                        if( mask_clearance < 0 )  // we expect margin.x = margin.y for custom pads
                        {
                            dummy.SetSize( aLayer, VECTOR2I( std::max( 0, padPlotsSize.x ),
                                                             std::max( 0, padPlotsSize.y ) ) );
                        }

                        itemplotter.PlotPad( &dummy, aLayer, color, doSketchPads );
                        break;
                    }
                    }

                    // Restore the pad parameters modified by the plot code
                    pad->SetSize( aLayer, padSize );
                    pad->SetDelta( aLayer, padDelta );
                    pad->SetShape( aLayer, padShape );
                    pad->SetRoundRectRadiusRatio( aLayer, padCornerRadiusRatio );
                };

            for( PCB_LAYER_ID layer : aLayerMask.SeqStackupForPlotting() )
                plotPadLayer( layer );
        }

        if( dnp
                && !itemplotter.GetHideDNPFPsOnFabLayers()
                && itemplotter.GetCrossoutDNPFPsOnFabLayers()
                && (   ( onFrontFab && footprint->GetLayer() == F_Cu )
                    || ( onBackFab && footprint->GetLayer() == B_Cu ) ) )
        {
            BOX2I                 rect;
            const SHAPE_POLY_SET& courtyard = footprint->GetCourtyard( footprint->GetLayer() );

            if( courtyard.IsEmpty() )
                rect = footprint->GetEffectiveShape()->BBox();
            else
                rect = courtyard.BBox();

            int   width = aBoard->GetDesignSettings().m_LineThickness[ LAYER_CLASS_FAB ];

            // Use DNP cross color from color scheme
            COLOR4D dnpMarkerColor = aPlotOpt.ColorSettings()->GetColor( LAYER_DNP_MARKER );
            if( dnpMarkerColor != COLOR4D::UNSPECIFIED )
                aPlotter->SetColor( dnpMarkerColor );
            else
                aPlotter->SetColor( aPlotOpt.ColorSettings()->GetColor( onFrontFab ? F_Fab : B_Fab ) );

            aPlotter->ThickSegment( rect.GetOrigin(), rect.GetEnd(), width, nullptr );
            aPlotter->ThickSegment( VECTOR2I( rect.GetLeft(), rect.GetBottom() ),
                                    VECTOR2I( rect.GetRight(), rect.GetTop() ),
                                    width, nullptr );
        }

        aPlotter->EndBlock( nullptr );
    }

    // Plot vias on copper layers, and if aPlotOpt.GetPlotViaOnMaskLayer() is true,

    GBR_METADATA gbr_metadata;

    if( onCopperLayer )
    {
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_VIAPAD );
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET );
    }

    auto getMetadata =
            [&]()
            {
                if( aPlotter->GetPlotterType() == PLOT_FORMAT::GERBER )
                    return (void*) &gbr_metadata;
                else if( aPlotter->GetPlotterType() == PLOT_FORMAT::DXF )
                    return (void*) &aPlotOpt;
                else
                    return (void*) nullptr;
            };

    aPlotter->StartBlock( nullptr );

    for( const PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

        // vias are not plotted if not on selected layer
        LSET via_mask_layer = via->GetLayerSet();

        if( !( via_mask_layer & aLayerMask ).any() )
            continue;

        int via_margin = 0;
        double width_adj = 0;

        // TODO(JE) padstacks - separate top/bottom margin
        if( onSolderMaskLayer )
            via_margin = via->GetSolderMaskExpansion();

        if( ( aLayerMask & LSET::AllCuMask() ).any() )
            width_adj = itemplotter.getFineWidthAdj();

        /// Vias not connected to copper are optionally not drawn
        if( onCopperLayer && !via->FlashLayer( aLayerMask ) )
            continue;

        int diameter = 0;

        for( PCB_LAYER_ID layer : aLayerMask )
            diameter = std::max( diameter, via->GetWidth( layer ) );

        diameter += 2 * via_margin + width_adj;

        // Don't draw a null size item :
        if( diameter <= 0 )
            continue;

        // Some vias can be not connected (no net).
        // Set the m_NotInNet for these vias to force a empty net name in gerber file
        gbr_metadata.m_NetlistMetadata.m_NotInNet = via->GetNetname().IsEmpty();

        gbr_metadata.SetNetName( via->GetNetname() );

        COLOR4D color;

        // If we're plotting a single layer, the color for that layer can be used directly.
        if( aLayerMask.count() == 1 )
            color = aPlotOpt.ColorSettings()->GetColor( aLayerMask.Seq()[0] );
        else
            color = aPlotOpt.ColorSettings()->GetColor( LAYER_VIAS + static_cast<int>( via->GetViaType() ) );

        // Change UNSPECIFIED or WHITE to LIGHTGRAY because the white items are not seen on a
        // white paper or screen
        if( color == COLOR4D::UNSPECIFIED || color == WHITE )
            color = LIGHTGRAY;

        aPlotter->SetColor( color );
        aPlotter->FlashPadCircle( via->GetStart(), diameter, getMetadata() );
    }

    aPlotter->EndBlock( nullptr );
    aPlotter->StartBlock( nullptr );

    if( onCopperLayer )
    {
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONDUCTOR );
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET );
    }
    else
    {
        // Reset attributes if non-copper (soldermask) layer
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONE );
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_UNSPECIFIED );
    }

    // Plot tracks (not vias) :
    for( const PCB_TRACK* track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            continue;

        if( !( aLayerMask & track->GetLayerSet() ).any() )
            continue;

        // Some track segments can be not connected (no net).
        // Set the m_NotInNet for these segments to force a empty net name in gerber file
        gbr_metadata.m_NetlistMetadata.m_NotInNet = track->GetNetname().IsEmpty();

        gbr_metadata.SetNetName( track->GetNetname() );

        int margin = 0;

        if( onSolderMaskLayer )
            margin = track->GetSolderMaskExpansion();

        int width = track->GetWidth() + 2 * margin + itemplotter.getFineWidthAdj();

        aPlotter->SetColor( itemplotter.getColor( track->GetLayer() ) );

        if( track->Type() == PCB_ARC_T )
        {
            const PCB_ARC* arc = static_cast<const PCB_ARC*>( track );

            // Too small arcs cannot be really handled: arc center (and arc radius)
            // cannot be safely computed
            if( !arc->IsDegenerated( 10 /* in IU */ ) )
            {
                aPlotter->ThickArc( arc->GetCenter(), arc->GetArcAngleStart(), arc->GetAngle(),
                                    arc->GetRadius(), width, getMetadata() );
            }
            else
            {
                // Approximate this very small arc by a segment.
                aPlotter->ThickSegment( track->GetStart(), track->GetEnd(), width, getMetadata() );
            }
        }
        else
        {
            aPlotter->ThickSegment( track->GetStart(), track->GetEnd(), width, getMetadata() );
        }
    }

    aPlotter->EndBlock( nullptr );

    // Plot filled ares
    aPlotter->StartBlock( nullptr );

    NETINFO_ITEM nonet( aBoard );

    for( const ZONE* zone : aBoard->Zones() )
    {
        if( zone->GetIsRuleArea() )
            continue;

        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            if( !aLayerMask[layer] )
                continue;

            SHAPE_POLY_SET mainArea = zone->GetFilledPolysList( layer )->CloneDropTriangulation();
            SHAPE_POLY_SET islands;

            for( int i = mainArea.OutlineCount() - 1; i >= 0; i-- )
            {
                if( zone->IsIsland( layer, i ) )
                {
                    islands.AddOutline( mainArea.CPolygon( i )[0] );
                    mainArea.DeletePolygon( i );
                }
            }

            itemplotter.PlotZone( zone, layer, mainArea );

            if( !islands.IsEmpty() )
            {
                ZONE dummy( *zone );
                dummy.SetNet( &nonet );
                itemplotter.PlotZone( &dummy, layer, islands );
            }
        }
    }

    aPlotter->EndBlock( nullptr );
}


/**
 * Plot outlines.
 */
void PlotLayerOutlines( BOARD* aBoard, PLOTTER* aPlotter, const LSET& aLayerMask,
                        const PCB_PLOT_PARAMS& aPlotOpt )
{
    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerSet( aLayerMask );

    int smallDrill = pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_SmallDrillMarkSize );

    SHAPE_POLY_SET outlines;

    for( PCB_LAYER_ID layer : aLayerMask.Seq( aLayerMask.SeqStackupForPlotting() ) )
    {
        outlines.RemoveAllContours();
        aBoard->ConvertBrdLayerToPolygonalContours( layer, outlines, aPlotter->RenderSettings() );

        outlines.Simplify();

        // Plot outlines
        std::vector<VECTOR2I> cornerList;

        // Now we have one or more basic polygons: plot each polygon
        for( int ii = 0; ii < outlines.OutlineCount(); ii++ )
        {
            for( int kk = 0; kk <= outlines.HoleCount(ii); kk++ )
            {
                cornerList.clear();
                const SHAPE_LINE_CHAIN& path = ( kk == 0 ) ? outlines.COutline( ii )
                                                           : outlines.CHole( ii, kk - 1 );

                aPlotter->PlotPoly( path, FILL_T::NO_FILL, PLOTTER::USE_DEFAULT_LINE_WIDTH, nullptr );
            }
        }

        // Plot pad holes
        if( aPlotOpt.GetDrillMarksType() != DRILL_MARKS::NO_DRILL_SHAPE )
        {
            for( FOOTPRINT* footprint : aBoard->Footprints() )
            {
                for( PAD* pad : footprint->Pads() )
                {
                    if( pad->HasHole() )
                    {
                        if( pad->GetDrillSizeX() == pad->GetDrillSizeY() )
                        {
                            int drill = pad->GetDrillSizeX();

                            if( aPlotOpt.GetDrillMarksType() == DRILL_MARKS::SMALL_DRILL_SHAPE )
                                drill = std::min( smallDrill, drill );

                            aPlotter->ThickCircle( pad->ShapePos( layer ), drill,
                                                   PLOTTER::USE_DEFAULT_LINE_WIDTH, nullptr );
                        }
                        else
                        {
                            // Note: small drill marks have no significance when applied to slots

                            aPlotter->ThickOval( pad->ShapePos( layer ), pad->GetSize( layer ),
                                                 pad->GetOrientation(), PLOTTER::USE_DEFAULT_LINE_WIDTH,
                                                 nullptr );
                        }
                    }
                }
            }
        }

        // Plot vias holes
        for( PCB_TRACK* track : aBoard->Tracks() )
        {
            if( track->Type() != PCB_VIA_T )
                continue;

            const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

            if( via->GetLayerSet().Contains( layer ) )   // via holes can be not through holes
            {
                aPlotter->Circle( via->GetPosition(), via->GetDrillValue(), FILL_T::NO_FILL,
                                  PLOTTER::USE_DEFAULT_LINE_WIDTH );
            }
        }
    }
}


/**
 * Generates a SHAPE_POLY_SET representing the plotted items on a layer.
 */
void GenerateLayerPoly( SHAPE_POLY_SET* aResult, BOARD *aBoard, PLOTTER* aPlotter, PCB_LAYER_ID aLayer,
                         bool aPlotFPText, bool aPlotReferences, bool aPlotValues )
{
    int             maxError = aBoard->GetDesignSettings().m_MaxError;
    SHAPE_POLY_SET  buffer;
    int             inflate = 0;

    if( aLayer == F_Mask || aLayer == B_Mask )
    {
        // We remove 1nm as we expand both sides of the shapes, so allowing for a strictly greater
        // than or equal comparison in the shape separation (boolean add)
        inflate = aBoard->GetDesignSettings().m_SolderMaskMinWidth / 2 - 1;
    }

    // Build polygons for each pad shape.  The size of the shape on solder mask should be size
    // of pad + clearance around the pad, where clearance = solder mask clearance + extra margin.
    // Extra margin is half the min width for solder mask, which is used to merge too-close shapes
    // (distance < SolderMaskMinWidth).

    // Will contain exact shapes of all items on solder mask.  We add this back in at the end just
    // to make sure that any artefacts introduced by the inflate/deflate don't remove parts of the
    // individual shapes.
    SHAPE_POLY_SET exactPolys;

    auto handleFPTextItem =
            [&]( const PCB_TEXT& aText )
            {
                if( !aPlotFPText )
                    return;

                if( aText.GetText() == wxT( "${REFERENCE}" ) && !aPlotReferences )
                    return;

                if( aText.GetText() == wxT( "${VALUE}" ) && !aPlotValues )
                    return;

                if( inflate != 0 )
                    aText.TransformTextToPolySet( exactPolys, 0, maxError, ERROR_OUTSIDE );

                aText.TransformTextToPolySet( *aResult, inflate, maxError, ERROR_OUTSIDE );
            };

    // Generate polygons with arcs inside the shape or exact shape to minimize shape changes
    // created by arc to segment size correction.
    DISABLE_ARC_RADIUS_CORRECTION disabler;
    {
        // Plot footprint pads and graphics
        for( const FOOTPRINT* footprint : aBoard->Footprints() )
        {
            if( inflate != 0 )
                footprint->TransformPadsToPolySet( exactPolys, aLayer, 0, maxError, ERROR_OUTSIDE );

            footprint->TransformPadsToPolySet( *aResult, aLayer, inflate, maxError, ERROR_OUTSIDE );

            for( const PCB_FIELD* field : footprint->GetFields() )
            {
                wxCHECK2( field, continue );

                if( field->IsReference() && !aPlotReferences )
                    continue;

                if( field->IsValue() && !aPlotValues )
                    continue;

                if( field->IsVisible() && field->IsOnLayer( aLayer ) )
                    handleFPTextItem( static_cast<const PCB_TEXT&>( *field ) );
            }

            for( const BOARD_ITEM* item : footprint->GraphicalItems() )
            {
                if( item->IsOnLayer( aLayer ) )
                {
                    if( item->Type() == PCB_TEXT_T )
                    {
                        handleFPTextItem( static_cast<const PCB_TEXT&>( *item ) );
                    }
                    else
                    {
                        if( inflate != 0 )
                            item->TransformShapeToPolySet( exactPolys, aLayer, 0, maxError, ERROR_OUTSIDE );

                        item->TransformShapeToPolySet( *aResult, aLayer, inflate, maxError, ERROR_OUTSIDE );
                    }
                }
            }
        }

        // Plot untented vias and tracks
        for( const PCB_TRACK* track : aBoard->Tracks() )
        {
            // Note: IsOnLayer() checks relevant mask layers of untented vias and tracks
            if( !track->IsOnLayer( aLayer ) )
                continue;

            int clearance = track->GetSolderMaskExpansion();

            if( inflate != 0 )
                track->TransformShapeToPolygon( exactPolys, aLayer, clearance, maxError, ERROR_OUTSIDE );

            track->TransformShapeToPolygon( *aResult, aLayer, clearance + inflate, maxError, ERROR_OUTSIDE );
        }

        for( const BOARD_ITEM* item : aBoard->Drawings() )
        {
            if( item->IsOnLayer( aLayer ) )
            {
                if( item->Type() == PCB_TEXT_T )
                {
                    const PCB_TEXT* text = static_cast<const PCB_TEXT*>( item );

                    if( inflate != 0 )
                        text->TransformTextToPolySet( exactPolys, 0, maxError, ERROR_OUTSIDE );

                    text->TransformTextToPolySet( *aResult, inflate, maxError, ERROR_OUTSIDE );
                }
                else
                {
                    if( inflate != 0 )
                        item->TransformShapeToPolySet( exactPolys, aLayer, 0, maxError, ERROR_OUTSIDE,
                                                       aPlotter->RenderSettings() );

                    item->TransformShapeToPolySet( *aResult, aLayer, inflate, maxError,
                                                    ERROR_OUTSIDE, aPlotter->RenderSettings() );
                }
            }
        }

        // Add filled zone areas.
        for( ZONE* zone : aBoard->Zones() )
        {
            if( zone->GetIsRuleArea() )
                continue;

            if( !zone->IsOnLayer( aLayer ) )
                continue;

            SHAPE_POLY_SET area = *zone->GetFill( aLayer );

            if( inflate != 0 )
                exactPolys.Append( area );

            area.Inflate( inflate, CORNER_STRATEGY::CHAMFER_ALL_CORNERS, maxError );
            aResult->Append( area );
        }
    }

    // Merge all polygons
    aResult->Simplify();

    if( inflate != 0 )
    {
        aResult->Deflate( inflate, CORNER_STRATEGY::CHAMFER_ALL_CORNERS, maxError );
        // Add back in the exact polys. This is mandatory because inflate/deflate transform is
        // not perfect, and we want the initial areas perfectly kept.
        aResult->BooleanAdd( exactPolys );
    }
#undef ERROR
}


/**
 * Set up most plot options for plotting a board (especially the viewport)
 * Important thing:
 *      page size is the 'drawing' page size,
 *      paper size is the physical page size
 */
static void initializePlotter( PLOTTER* aPlotter, const BOARD* aBoard, const PCB_PLOT_PARAMS* aPlotOpts )
{
    PAGE_INFO pageA4( PAGE_SIZE_TYPE::A4 );
    const PAGE_INFO& pageInfo = aBoard->GetPageSettings();
    const PAGE_INFO* sheet_info;
    double paperscale; // Page-to-paper ratio
    VECTOR2I paperSizeIU;
    VECTOR2I pageSizeIU( pageInfo.GetSizeIU( pcbIUScale.IU_PER_MILS ) );
    bool autocenter = false;

    // Special options: to fit the sheet to an A4 sheet replace the paper size. However there
    // is a difference between the autoscale and the a4paper option:
    //  - Autoscale fits the board to the paper size
    //  - A4paper fits the original paper size to an A4 sheet
    //  - Both of them fit the board to an A4 sheet
    if( aPlotOpts->GetA4Output() )
    {
        sheet_info  = &pageA4;
        paperSizeIU = pageA4.GetSizeIU( pcbIUScale.IU_PER_MILS );
        paperscale  = (double) paperSizeIU.x / pageSizeIU.x;
        autocenter  = true;
    }
    else
    {
        sheet_info  = &pageInfo;
        paperSizeIU = pageSizeIU;
        paperscale  = 1;

        // Need autocentering only if scale is not 1:1
        autocenter  = (aPlotOpts->GetScale() != 1.0) || aPlotOpts->GetAutoScale();
    }

    BOX2I    bbox = aBoard->ComputeBoundingBox( false );
    VECTOR2I boardCenter = bbox.Centre();
    VECTOR2I boardSize = bbox.GetSize();

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
    {
        compound_scale = aPlotOpts->GetScale() * paperscale;
    }

    // For the plot offset we have to keep in mind the auxiliary origin too: if autoscaling is
    // off we check that plot option (i.e. autoscaling overrides auxiliary origin)
    VECTOR2I offset( 0, 0);

    if( autocenter )
    {
        offset.x = KiROUND( boardCenter.x - ( paperSizeIU.x / 2.0 ) / compound_scale );
        offset.y = KiROUND( boardCenter.y - ( paperSizeIU.y / 2.0 ) / compound_scale );
    }
    else
    {
        if( aPlotOpts->GetUseAuxOrigin() )
            offset = aBoard->GetDesignSettings().GetAuxOrigin();
    }

    aPlotter->SetPageSettings( *sheet_info );

    aPlotter->SetViewport( offset, pcbIUScale.IU_PER_MILS/10, compound_scale, aPlotOpts->GetMirror() );

    // Has meaning only for gerber plotter. Must be called only after SetViewport
    aPlotter->SetGerberCoordinatesFormat( aPlotOpts->GetGerberPrecision() );

    // Has meaning only for SVG plotter. Must be called only after SetViewport
    aPlotter->SetSvgCoordinatesFormat( aPlotOpts->GetSvgPrecision() );

    aPlotter->SetCreator( wxT( "PCBNEW" ) );
    aPlotter->SetColorMode( !aPlotOpts->GetBlackAndWhite() );        // default is plot in Black and White.
    aPlotter->SetTextMode( aPlotOpts->GetTextMode() );
}


/**
 * Prefill in black an area a little bigger than the board to prepare for the negative plot
 */
static void FillNegativeKnockout( PLOTTER *aPlotter, const BOX2I &aBbbox )
{
    const int margin = 5 * pcbIUScale.IU_PER_MM; // Add a 5 mm margin around the board
    aPlotter->SetNegative( true );
    aPlotter->SetColor( WHITE );        // Which will be plotted as black

    BOX2I area = aBbbox;
    area.Inflate( margin );
    aPlotter->Rect( area.GetOrigin(), area.GetEnd(), FILL_T::FILLED_SHAPE, 0, 0 );
    aPlotter->SetColor( BLACK );
}


static void plotPdfBackground( BOARD* aBoard, const PCB_PLOT_PARAMS* aPlotOpts, PLOTTER* aPlotter )
{
    if( aPlotter->GetColorMode()
        && aPlotOpts->GetPDFBackgroundColor() != COLOR4D::UNSPECIFIED )
    {
        aPlotter->SetColor( aPlotOpts->GetPDFBackgroundColor() );

        // Use page size selected in pcb to know the schematic bg area
        const PAGE_INFO& actualPage = aBoard->GetPageSettings();

        VECTOR2I end( actualPage.GetWidthIU( pcbIUScale.IU_PER_MILS ),
                      actualPage.GetHeightIU( pcbIUScale.IU_PER_MILS ) );

        aPlotter->Rect( VECTOR2I( 0, 0 ), end, FILL_T::FILLED_SHAPE, 1.0 );
    }
}


/**
 * Open a new plotfile using the options (and especially the format) specified in the options
 * and prepare the page for plotting.
 *
 * @return the plotter object if OK, NULL if the file is not created (or has a problem).
 */
PLOTTER* StartPlotBoard( BOARD *aBoard, const PCB_PLOT_PARAMS *aPlotOpts, int aLayer,
                         const wxString& aLayerName, const wxString& aFullFileName,
                         const wxString& aSheetName, const wxString& aSheetPath,
                         const wxString& aPageName, const wxString& aPageNumber,
                         const int aPageCount )
{
    wxCHECK( aBoard && aPlotOpts, nullptr );

    // Create the plotter driver and set the few plotter specific options
    PLOTTER*    plotter = nullptr;

    switch( aPlotOpts->GetFormat() )
    {
    case PLOT_FORMAT::DXF:
        DXF_PLOTTER* DXF_plotter;
        DXF_plotter = new DXF_PLOTTER();
        DXF_plotter->SetUnits( aPlotOpts->GetDXFPlotUnits() );

        plotter = DXF_plotter;

        if( !aPlotOpts->GetLayersToExport().empty() )
            plotter->SetLayersToExport( aPlotOpts->GetLayersToExport() );
        break;

    case PLOT_FORMAT::POST:
        PS_PLOTTER* PS_plotter;
        PS_plotter = new PS_PLOTTER();
        PS_plotter->SetScaleAdjust( aPlotOpts->GetFineScaleAdjustX(),
                                    aPlotOpts->GetFineScaleAdjustY() );
        plotter = PS_plotter;
        break;

    case PLOT_FORMAT::PDF:
        plotter = new PDF_PLOTTER( aBoard->GetProject() );
        break;

    case PLOT_FORMAT::HPGL:
        wxLogError( _( "HPGL plotting is no longer supported as of KiCad 10.0" ) );
        return nullptr;

    case PLOT_FORMAT::GERBER:
        // For Gerber plotter, a valid board layer must be set, in order to create a valid
        // Gerber header, especially the TF.FileFunction and .FilePolarity data
        if( aLayer < PCBNEW_LAYER_ID_START || aLayer >= PCB_LAYER_ID_COUNT )
        {
            wxLogError( wxString::Format( "Invalid board layer %d, cannot build a valid Gerber file header",
                                          aLayer ) );
        }

        plotter = new GERBER_PLOTTER();
        break;

    case PLOT_FORMAT::SVG:
        plotter = new SVG_PLOTTER();
        break;

    default:
        wxASSERT( false );
        return nullptr;
    }

    KIGFX::PCB_RENDER_SETTINGS* renderSettings = new KIGFX::PCB_RENDER_SETTINGS();
    renderSettings->LoadColors( aPlotOpts->ColorSettings() );
    renderSettings->SetDefaultPenWidth( pcbIUScale.mmToIU( 0.0212 ) );  // Hairline at 1200dpi
    renderSettings->SetLayerName( aLayerName );

    plotter->SetRenderSettings( renderSettings );

    // Compute the viewport and set the other options

    // page layout is not mirrored, so temporarily change mirror option for the page layout
    PCB_PLOT_PARAMS plotOpts = *aPlotOpts;

    if( plotOpts.GetPlotFrameRef() )
    {
        if( plotOpts.GetMirror() )
            plotOpts.SetMirror( false );
        if( plotOpts.GetScale() != 1.0 )
            plotOpts.SetScale( 1.0 );
        if( plotOpts.GetAutoScale() )
            plotOpts.SetAutoScale( false );
    }

    initializePlotter( plotter, aBoard, &plotOpts );

    if( plotter->OpenFile( aFullFileName ) )
    {
        plotter->ClearHeaderLinesList();

        // For the Gerber "file function" attribute, set the layer number
        if( plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
        {
            bool useX2mode = plotOpts.GetUseGerberX2format();

            GERBER_PLOTTER* gbrplotter = static_cast <GERBER_PLOTTER*> ( plotter );
            gbrplotter->DisableApertMacros( plotOpts.GetDisableGerberMacros() );
            gbrplotter->UseX2format( useX2mode );
            gbrplotter->UseX2NetAttributes( plotOpts.GetIncludeGerberNetlistInfo() );

            // Attributes can be added using X2 format or as comment (X1 format)
            AddGerberX2Attribute( plotter, aBoard, aLayer, not useX2mode );
        }

        bool startPlotSuccess = false;
        try
        {
            if( plotter->GetPlotterType() == PLOT_FORMAT::PDF )
                startPlotSuccess = static_cast<PDF_PLOTTER*>( plotter )->StartPlot( aPageNumber, aPageName );
            else
                startPlotSuccess = plotter->StartPlot( aPageName );
        }
        catch( ... )
        {
            startPlotSuccess = false;
        }


        if( startPlotSuccess )
        {
            if( aPlotOpts->GetFormat() == PLOT_FORMAT::PDF )
                plotPdfBackground( aBoard, aPlotOpts, plotter );

            // Plot the frame reference if requested
            if( aPlotOpts->GetPlotFrameRef() )
            {
                PlotDrawingSheet( plotter, aBoard->GetProject(), aBoard->GetTitleBlock(),
                                  aBoard->GetPageSettings(), &aBoard->GetProperties(), aPageNumber,
                                  aPageCount, aSheetName, aSheetPath, aBoard->GetFileName(),
                                  renderSettings->GetLayerColor( LAYER_DRAWINGSHEET ) );

                if( aPlotOpts->GetMirror() || aPlotOpts->GetScale() != 1.0 || aPlotOpts->GetAutoScale() )
                    initializePlotter( plotter, aBoard, aPlotOpts );
            }

            // When plotting a negative board: draw a black rectangle (background for plot board
            // in white) and switch the current color to WHITE; note the color inversion is actually
            // done in the driver (if supported)
            if( aPlotOpts->GetNegative() )
            {
                BOX2I bbox = aBoard->ComputeBoundingBox( false );
                FillNegativeKnockout( plotter, bbox );
            }

            return plotter;
        }
    }

    delete plotter->RenderSettings();
    delete plotter;
    return nullptr;
}


void setupPlotterNewPDFPage( PLOTTER* aPlotter, BOARD* aBoard, PCB_PLOT_PARAMS* aPlotOpts,
                             const wxString& aLayerName, const wxString& aSheetName,
                             const wxString& aSheetPath, const wxString& aPageNumber,
                             int aPageCount )
{
    plotPdfBackground( aBoard, aPlotOpts, aPlotter );

    aPlotter->RenderSettings()->SetLayerName( aLayerName );

    // Plot the frame reference if requested
    if( aPlotOpts->GetPlotFrameRef() )
    {
        // Mirror and scale shouldn't be applied to the drawing sheet
        bool   revertOps = false;
        bool   oldMirror = aPlotOpts->GetMirror();
        bool   oldAutoScale = aPlotOpts->GetAutoScale();
        double oldScale = aPlotOpts->GetScale();

        if( oldMirror || oldAutoScale || oldScale != 1.0 )
        {
            aPlotOpts->SetMirror( false );
            aPlotOpts->SetScale( 1.0 );
            aPlotOpts->SetAutoScale( false );
            initializePlotter( aPlotter, aBoard, aPlotOpts );
            revertOps = true;
        }

        PlotDrawingSheet( aPlotter, aBoard->GetProject(), aBoard->GetTitleBlock(),
                          aBoard->GetPageSettings(), &aBoard->GetProperties(), aPageNumber,
                          aPageCount,
                          aSheetName, aSheetPath, aBoard->GetFileName(),
                          aPlotter->RenderSettings()->GetLayerColor( LAYER_DRAWINGSHEET ) );

        if( revertOps )
        {
            aPlotOpts->SetMirror( oldMirror );
            aPlotOpts->SetScale( oldScale );
            aPlotOpts->SetAutoScale( oldAutoScale );
            initializePlotter( aPlotter, aBoard, aPlotOpts );
        }
    }
}
