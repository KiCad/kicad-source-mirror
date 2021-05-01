/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <vector>
#include <bezier_curves.h>
#include <trigo.h>
#include <board.h>
#include <pad.h>
#include <dimension.h>
#include <track.h>
#include <kicad_string.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <zone.h>
#include <footprint.h>
#include <fp_shape.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_line_chain.h>


// A helper struct for the callback function
// These variables are parameters used in addTextSegmToPoly.
// But addTextSegmToPoly is a call-back function,
// so we cannot send them as arguments.
struct TSEGM_2_POLY_PRMS
{
    int m_textWidth;
    int m_error;
    SHAPE_POLY_SET* m_cornerBuffer;
};

TSEGM_2_POLY_PRMS prms;


// This is a call back function, used by GRText to draw the 3D text shape:
static void addTextSegmToPoly( int x0, int y0, int xf, int yf, void* aData )
{
    TSEGM_2_POLY_PRMS* prm = static_cast<TSEGM_2_POLY_PRMS*>( aData );
    TransformOvalToPolygon( *prm->m_cornerBuffer, wxPoint( x0, y0 ), wxPoint( xf, yf ),
                            prm->m_textWidth, prm->m_error, ERROR_INSIDE );
}


void BOARD::ConvertBrdLayerToPolygonalContours( PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aOutlines ) const
{
    int maxError = GetDesignSettings().m_MaxError;

    // convert tracks and vias:
    for( const TRACK* track : m_tracks )
    {
        if( !track->IsOnLayer( aLayer ) )
            continue;

        track->TransformShapeWithClearanceToPolygon( aOutlines, aLayer, 0, maxError,
                                                     ERROR_INSIDE );
    }

    // convert pads and other copper items in footprints
    for( const FOOTPRINT* footprint : m_footprints )
    {
        footprint->TransformPadsWithClearanceToPolygon( aOutlines, aLayer, 0, maxError,
                                                        ERROR_INSIDE );

        // Micro-wave footprints may have items on copper layers
        footprint->TransformFPShapesWithClearanceToPolygon( aOutlines, aLayer, 0, maxError,
                                                            ERROR_INSIDE,
                                                            true, /* include text */
                                                            true  /* include shapes */ );

        for( const ZONE* zone : footprint->Zones() )
        {
            if( zone->GetLayerSet().test( aLayer ) )
                zone->TransformSolidAreasShapesToPolygon( aLayer, aOutlines );
        }
    }

    // convert copper zones
    for( const ZONE* zone : Zones() )
    {
        if( zone->GetLayerSet().test( aLayer ) )
            zone->TransformSolidAreasShapesToPolygon( aLayer, aOutlines );
    }

    // convert graphic items on copper layers (texts)
    for( const BOARD_ITEM* item : m_drawings )
    {
        if( !item->IsOnLayer( aLayer ) )
            continue;

        switch( item->Type() )
        {
        case PCB_SHAPE_T:
        {
            const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );
            shape->TransformShapeWithClearanceToPolygon( aOutlines, aLayer, 0, maxError,
                                                         ERROR_INSIDE );
        }
            break;

        case PCB_TEXT_T:
        {
            const PCB_TEXT* text = static_cast<const PCB_TEXT*>( item );
            text->TransformTextShapeWithClearanceToPolygon( aOutlines, aLayer, 0, maxError,
                                                            ERROR_INSIDE );
        }
            break;

        default:
            break;
        }
    }
}


void FOOTPRINT::TransformPadsWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                     PCB_LAYER_ID aLayer, int aClearance,
                                                     int aMaxError, ERROR_LOC aErrorLoc,
                                                     bool aSkipNPTHPadsWihNoCopper,
                                                     bool aSkipPlatedPads,
                                                     bool aSkipNonPlatedPads ) const
{
    for( const PAD* pad : m_pads )
    {
        if( aLayer != UNDEFINED_LAYER && !pad->IsOnLayer(aLayer) )
            continue;

        if( !pad->FlashLayer( aLayer ) && IsCopperLayer( aLayer ) )
            continue;

        // NPTH pads are not drawn on layers if the shape size and pos is the same
        // as their hole:
        if( aSkipNPTHPadsWihNoCopper && pad->GetAttribute() == PAD_ATTRIB_NPTH )
        {
            if( pad->GetDrillSize() == pad->GetSize() && pad->GetOffset() == wxPoint( 0, 0 ) )
            {
                switch( pad->GetShape() )
                {
                case PAD_SHAPE::CIRCLE:
                    if( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                        continue;
                    break;

                case PAD_SHAPE::OVAL:
                    if( pad->GetDrillShape() != PAD_DRILL_SHAPE_CIRCLE )
                        continue;
                    break;

                default:
                    break;
                }
            }
        }

        const bool isPlated = ( ( aLayer == F_Cu ) && pad->FlashLayer( F_Mask ) ) ||
                              ( ( aLayer == B_Cu ) && pad->FlashLayer( B_Mask ) );

        if( aSkipPlatedPads && isPlated )
            continue;

        if( aSkipNonPlatedPads && !isPlated )
            continue;

        wxSize clearance( aClearance, aClearance );

        switch( aLayer )
        {
        case F_Mask:
        case B_Mask:
            clearance.x += pad->GetSolderMaskMargin();
            clearance.y += pad->GetSolderMaskMargin();
            break;

        case F_Paste:
        case B_Paste:
            clearance += pad->GetSolderPasteMargin();
            break;

        default:
            break;
        }

        // Our standard TransformShapeWithClearanceToPolygon() routines can't handle differing
        // x:y clearance values (which get generated when a relative paste margin is used with
        // an oblong pad).  So we apply this huge hack and fake a larger pad to run the transform
        // on.
        // Of course being a hack it falls down when dealing with custom shape pads (where the
        // size is only the size of the anchor), so for those we punt and just use clearance.x.

        if( ( clearance.x < 0 || clearance.x != clearance.y )
                && pad->GetShape() != PAD_SHAPE::CUSTOM )
        {
            PAD dummy( *pad );
            dummy.SetSize( pad->GetSize() + clearance + clearance );
            dummy.TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, 0,
                                                        aMaxError, aErrorLoc );
        }
        else
        {
            pad->TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, clearance.x,
                                                       aMaxError, aErrorLoc );
        }
    }
}

/**
 * Generate shapes of graphic items (outlines) as polygons added to a buffer.
 * @aCornerBuffer = the buffer to store polygons
 * @aInflateValue = a value to inflate shapes
 * @aError = the maximum error to allow when approximating curves with segments
 * @aIncludeText = indicates footprint text items (reference, value, etc.) should be included
 *                 in the outline
 */
void FOOTPRINT::TransformFPShapesWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                         PCB_LAYER_ID aLayer, int aClearance,
                                                         int aError, ERROR_LOC aErrorLoc,
                                                         bool aIncludeText,
                                                         bool aIncludeShapes ) const
{
    std::vector<FP_TEXT*> texts;  // List of FP_TEXT to convert

    for( BOARD_ITEM* item : GraphicalItems() )
    {
        if( item->Type() == PCB_FP_TEXT_T && aIncludeText )
        {
            FP_TEXT* text = static_cast<FP_TEXT*>( item );

            if( aLayer != UNDEFINED_LAYER && text->GetLayer() == aLayer && text->IsVisible() )
                texts.push_back( text );
        }

        if( item->Type() == PCB_FP_SHAPE_T && aIncludeShapes )
        {
            const FP_SHAPE* outline = static_cast<FP_SHAPE*>( item );

            if( aLayer != UNDEFINED_LAYER && outline->GetLayer() == aLayer )
            {
                outline->TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, 0,
                                                               aError, aErrorLoc );
            }
        }
    }

    if( aIncludeText )
    {
        if( Reference().GetLayer() == aLayer && Reference().IsVisible() )
            texts.push_back( &Reference() );

        if( Value().GetLayer() == aLayer && Value().IsVisible() )
            texts.push_back( &Value() );
    }

    for( const FP_TEXT* text : texts )
    {
        text->TransformTextShapeWithClearanceToPolygon( aCornerBuffer, aLayer, aClearance,
                                                        aError, aErrorLoc );
    }
}


/**
 * Function TransformTextShapeWithClearanceToPolygon
 * Convert the text to a polygonSet describing the actual character strokes (one per segment).
 * @aCornerBuffer = SHAPE_POLY_SET to store the polygon corners
 * @aClearanceValue = the clearance around the text
 * @aError = the maximum error to allow when approximating curves
 */
void FP_TEXT::TransformTextShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                        PCB_LAYER_ID aLayer, int aClearance,
                                                        int aError, ERROR_LOC aErrorLoc ) const
{
    bool forceBold = true;
    int  penWidth = 0;      // force max width for bold text

    prms.m_cornerBuffer = &aCornerBuffer;
    prms.m_textWidth  = GetEffectiveTextPenWidth() + ( 2 * aClearance );
    prms.m_error = aError;
    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    GRText( NULL, GetTextPos(), BLACK, GetShownText(), GetDrawRotation(), size, GetHorizJustify(),
            GetVertJustify(), penWidth, IsItalic(), forceBold, addTextSegmToPoly, &prms );
}


void FP_TEXT::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                    PCB_LAYER_ID aLayer, int aClearance,
                                                    int aError, ERROR_LOC aErrorLoc,
                                                    bool aIgnoreLineWidth ) const
{
    EDA_TEXT::TransformBoundingBoxWithClearanceToPolygon( &aCornerBuffer, aClearance );
}


void ZONE::TransformSolidAreasShapesToPolygon( PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aCornerBuffer,
                                               int aError ) const
{
    if( !m_FilledPolysList.count( aLayer ) || m_FilledPolysList.at( aLayer ).IsEmpty() )
        return;

    // Just add filled areas if filled polygons outlines have no thickness
    if( !GetFilledPolysUseThickness() || GetMinThickness() == 0 )
    {
        const SHAPE_POLY_SET& polys = m_FilledPolysList.at( aLayer );
        aCornerBuffer.Append( polys );
        return;
    }

    // Filled areas have polygons with outline thickness.
    // we must create the polygons and add inflated polys
    SHAPE_POLY_SET polys = m_FilledPolysList.at( aLayer );

    auto board = GetBoard();
    int maxError = ARC_HIGH_DEF;

    if( board )
        maxError = board->GetDesignSettings().m_MaxError;

    int numSegs = GetArcToSegmentCount( GetMinThickness(), maxError, 360.0 );

    polys.InflateWithLinkedHoles( GetMinThickness()/2, numSegs, SHAPE_POLY_SET::PM_FAST );

    aCornerBuffer.Append( polys );
}


void EDA_TEXT::TransformBoundingBoxWithClearanceToPolygon( SHAPE_POLY_SET* aCornerBuffer,
                                                           int aClearanceValue ) const
{
    if( GetText().Length() == 0 )
        return;

    wxPoint  corners[4];    // Buffer of polygon corners

    EDA_RECT rect = GetTextBox();
    rect.Inflate( aClearanceValue + Millimeter2iu( DEFAULT_TEXT_WIDTH ) );
    corners[0].x = rect.GetOrigin().x;
    corners[0].y = rect.GetOrigin().y;
    corners[1].y = corners[0].y;
    corners[1].x = rect.GetRight();
    corners[2].x = corners[1].x;
    corners[2].y = rect.GetBottom();
    corners[3].y = corners[2].y;
    corners[3].x = corners[0].x;

    aCornerBuffer->NewOutline();

    for( wxPoint& corner : corners )
    {
        // Rotate polygon
        RotatePoint( &corner.x, &corner.y, GetTextPos().x, GetTextPos().y, GetTextAngle() );
        aCornerBuffer->Append( corner.x, corner.y );
    }
}


/**
 * Function TransformTextShapeWithClearanceToPolygon
 * Convert the text to a polygonSet describing the actual character strokes (one per segment).
 * @aCornerBuffer = SHAPE_POLY_SET to store the polygon corners
 * @aClearanceValue = the clearance around the text
 * @aError = the maximum error to allow when approximating curves
 */
void PCB_TEXT::TransformTextShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                     PCB_LAYER_ID aLayer, int aClearanceValue,
                                                     int aError, ERROR_LOC aErrorLoc ) const
{
    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    bool forceBold = true;
    int  penWidth = GetEffectiveTextPenWidth();

    prms.m_cornerBuffer = &aCornerBuffer;
    prms.m_textWidth = GetEffectiveTextPenWidth() + ( 2 * aClearanceValue );
    prms.m_error = aError;
    COLOR4D color = COLOR4D::BLACK;  // not actually used, but needed by GRText

    if( IsMultilineAllowed() )
    {
        wxArrayString strings_list;
        wxStringSplit( GetShownText(), strings_list, '\n' );
        std::vector<wxPoint> positions;
        positions.reserve( strings_list.Count() );
        GetLinePositions( positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxString txt = strings_list.Item( ii );
            GRText( NULL, positions[ii], color, txt, GetTextAngle(), size, GetHorizJustify(),
                    GetVertJustify(), penWidth, IsItalic(), forceBold, addTextSegmToPoly, &prms );
        }
    }
    else
    {
        GRText( NULL, GetTextPos(), color, GetShownText(), GetTextAngle(), size, GetHorizJustify(),
                GetVertJustify(), penWidth, IsItalic(), forceBold, addTextSegmToPoly, &prms );
    }
}


void PCB_TEXT::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                    PCB_LAYER_ID aLayer, int aClearance,
                                                    int aError, ERROR_LOC aErrorLoc,
                                                    bool aIgnoreLineWidth ) const
{
    EDA_TEXT::TransformBoundingBoxWithClearanceToPolygon( &aCornerBuffer, aClearance );
}


void PCB_SHAPE::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                      PCB_LAYER_ID aLayer, int aClearanceValue,
                                                      int aError, ERROR_LOC aErrorLoc,
                                                      bool ignoreLineWidth ) const
{
    int width = ignoreLineWidth ? 0 : m_width;

    width += 2 * aClearanceValue;

    switch( m_shape )
    {
    case PCB_SHAPE_TYPE::CIRCLE:
        if( IsFilled() )
        {
            TransformCircleToPolygon( aCornerBuffer, GetCenter(), GetRadius() + width / 2, aError,
                                      aErrorLoc );
        }
        else
        {
            TransformRingToPolygon( aCornerBuffer, GetCenter(), GetRadius(), width, aError,
                                    aErrorLoc );
        }
        break;

    case PCB_SHAPE_TYPE::RECT:
    {
        std::vector<wxPoint> pts = GetRectCorners();

        if( IsFilled() )
        {
            aCornerBuffer.NewOutline();

            for( const wxPoint& pt : pts )
                aCornerBuffer.Append( pt );
        }

        if( width > 0 || !IsFilled() )
        {
            // Add in segments
            TransformOvalToPolygon( aCornerBuffer, pts[0], pts[1], width, aError, aErrorLoc );
            TransformOvalToPolygon( aCornerBuffer, pts[1], pts[2], width, aError, aErrorLoc );
            TransformOvalToPolygon( aCornerBuffer, pts[2], pts[3], width, aError, aErrorLoc );
            TransformOvalToPolygon( aCornerBuffer, pts[3], pts[0], width, aError, aErrorLoc );
        }
    }
        break;

    case PCB_SHAPE_TYPE::ARC:
        TransformArcToPolygon( aCornerBuffer, GetArcStart(), GetArcMid(), GetArcEnd(), width,
                               aError, aErrorLoc );
        break;

    case PCB_SHAPE_TYPE::SEGMENT:
        TransformOvalToPolygon( aCornerBuffer, m_start, m_end, width, aError, aErrorLoc );
        break;

    case PCB_SHAPE_TYPE::POLYGON:
    {
        if( !IsPolyShapeValid() )
            break;

        // The polygon is expected to be a simple polygon; not self intersecting, no hole.
        FOOTPRINT* footprint = GetParentFootprint();
        double     orientation = footprint ? footprint->GetOrientation() : 0.0;
        wxPoint    offset;

        if( footprint )
            offset = footprint->GetPosition();

        // Build the polygon with the actual position and orientation:
        std::vector<wxPoint> poly;
        poly = BuildPolyPointsList();

        for( wxPoint& point : poly )
        {
            RotatePoint( &point, orientation );
            point += offset;
        }

        if( IsFilled() )
        {
            aCornerBuffer.NewOutline();

            for( const wxPoint& point : poly )
                aCornerBuffer.Append( point.x, point.y );
        }

        if( width > 0 || !IsFilled() )
        {
            wxPoint pt1( poly[ poly.size() - 1] );

            for( const wxPoint& pt2 : poly )
            {
                if( pt2 != pt1 )
                    TransformOvalToPolygon( aCornerBuffer, pt1, pt2, width, aError, aErrorLoc );

                pt1 = pt2;
            }
        }
    }
        break;

    case PCB_SHAPE_TYPE::CURVE: // Bezier curve
    {
        std::vector<wxPoint> ctrlPoints = { m_start, m_bezierC1, m_bezierC2, m_end };
        BEZIER_POLY converter( ctrlPoints );
        std::vector< wxPoint> poly;
        converter.GetPoly( poly, m_width );

        for( unsigned ii = 1; ii < poly.size(); ii++ )
        {
            TransformOvalToPolygon( aCornerBuffer, poly[ii - 1], poly[ii], width, aError, aErrorLoc );
        }
    }
        break;

    default:
        wxFAIL_MSG( "PCB_SHAPE::TransformShapeWithClearanceToPolygon no implementation for "
                    + PCB_SHAPE_TYPE_T_asString( m_shape ) );
        break;
    }
}


void TRACK::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                  PCB_LAYER_ID aLayer, int aClearanceValue,
                                                  int aError, ERROR_LOC aErrorLoc,
                                                  bool ignoreLineWidth ) const
{
    wxASSERT_MSG( !ignoreLineWidth, "IgnoreLineWidth has no meaning for tracks." );


    switch( Type() )
    {
    case PCB_VIA_T:
    {
        int radius = ( m_Width / 2 ) + aClearanceValue;
        TransformCircleToPolygon( aCornerBuffer, m_Start, radius, aError, aErrorLoc );
    }
        break;

    case PCB_ARC_T:
    {
        const ARC* arc = static_cast<const ARC*>( this );
        int        width = m_Width + ( 2 * aClearanceValue );

        TransformArcToPolygon( aCornerBuffer, arc->GetStart(), arc->GetMid(),
                               arc->GetEnd(), width, aError, aErrorLoc );
    }
        break;

    default:
    {
        int width = m_Width + ( 2 * aClearanceValue );

        TransformOvalToPolygon( aCornerBuffer, m_Start, m_End, width, aError, aErrorLoc );
    }
        break;
    }
}


void PAD::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                PCB_LAYER_ID aLayer, int aClearanceValue,
                                                int aError, ERROR_LOC aErrorLoc,
                                                bool ignoreLineWidth ) const
{
    wxASSERT_MSG( !ignoreLineWidth, "IgnoreLineWidth has no meaning for pads." );

    // minimal segment count to approximate a circle to create the polygonal pad shape
    // This minimal value is mainly for very small pads, like SM0402.
    // Most of time pads are using the segment count given by aError value.
    const int pad_min_seg_per_circle_count = 16;
    double  angle = m_orient;
    int     dx = m_size.x / 2;
    int     dy = m_size.y / 2;

    wxPoint padShapePos = ShapePos();         // Note: for pad having a shape offset,
                                              // the pad position is NOT the shape position

    switch( GetShape() )
    {
    case PAD_SHAPE::CIRCLE:
    case PAD_SHAPE::OVAL:
        if( dx == dy )
        {
            TransformCircleToPolygon( aCornerBuffer, padShapePos, dx + aClearanceValue, aError,
                                      aErrorLoc );
        }
        else
        {
            int     half_width = std::min( dx, dy );
            wxPoint delta( dx - half_width, dy - half_width );

            RotatePoint( &delta, angle );

            TransformOvalToPolygon( aCornerBuffer, padShapePos - delta, padShapePos + delta,
                                    ( half_width + aClearanceValue ) * 2, aError, aErrorLoc );
        }

        break;

    case PAD_SHAPE::TRAPEZOID:
    case PAD_SHAPE::RECT:
    {
        int  ddx = GetShape() == PAD_SHAPE::TRAPEZOID ? m_deltaSize.x / 2 : 0;
        int  ddy = GetShape() == PAD_SHAPE::TRAPEZOID ? m_deltaSize.y / 2 : 0;

        wxPoint corners[4];
        corners[0] = wxPoint( -dx - ddy,  dy + ddx );
        corners[1] = wxPoint(  dx + ddy,  dy - ddx );
        corners[2] = wxPoint(  dx - ddy, -dy + ddx );
        corners[3] = wxPoint( -dx + ddy, -dy - ddx );

        SHAPE_POLY_SET outline;
        outline.NewOutline();

        for( wxPoint& corner : corners )
        {
            RotatePoint( &corner, angle );
            corner += padShapePos;
            outline.Append( corner.x, corner.y );
        }

        if( aClearanceValue )
        {
            int numSegs = std::max( GetArcToSegmentCount( aClearanceValue, aError, 360.0 ),
                                    pad_min_seg_per_circle_count );
            int clearance = aClearanceValue;

            if( aErrorLoc == ERROR_OUTSIDE )
                clearance += GetCircleToPolyCorrection( aError );

            outline.Inflate( clearance, numSegs );
        }

        aCornerBuffer.Append( outline );
    }
        break;

    case PAD_SHAPE::CHAMFERED_RECT:
    case PAD_SHAPE::ROUNDRECT:
    {
        int    radius = GetRoundRectCornerRadius();
        wxSize shapesize( m_size );
        bool   doChamfer = GetShape() == PAD_SHAPE::CHAMFERED_RECT;

        double chamferRatio = doChamfer ? GetChamferRectRatio() : 0.0;

        if( aClearanceValue )
        {
            radius += aClearanceValue;
            shapesize.x += aClearanceValue * 2;
            shapesize.y += aClearanceValue * 2;

            // The chamfer position (the 45 deg line on corner) must be
            // offsetted by aClearanceValue from the base shape chamfer pos
            // So we recalculate the chamferRatio to do that
            //
            // the chamfered shape is square with widet = w, and a corner dist from center
            // is w*1.414 / 2 = w*0.707
            // the distance from corner to chamfer line is ch = chamfer_size/707
            // the distance from center to chamfer line is
            // d = w*707 - ch/707
            // so we have:
            // base shape: d1 = w1*707 - ch1/707 = 0.707 * ( w1 - w1*chamferRatio)
            // shape with clearance: d2 = w2*707 - ch2/707 = d1 + aClearanceValue
            const double rootsq_2 = 1.41421356237/2;
            int d1 = rootsq_2 * std::min( m_size.x, m_size.y ) * ( 1 - GetChamferRectRatio() );
            int d2 = d1 + aClearanceValue;
            // d2 = 0.707 * w2 * ( 1 - chamferRatio2 )
            // 1 - d2 / ( 0.707 * w2 ) = chamferRatio2
            chamferRatio = 1.0 - d2 / ( rootsq_2 * std::min( shapesize.x, shapesize.y ) );

            // Ensure chamferRatio = 0.0 ... 0.5
            if( chamferRatio < 0.0 )
                chamferRatio = 0.0;

            if( chamferRatio > 0.5 )
                chamferRatio = 0.5;
        }

        SHAPE_POLY_SET outline;
        TransformRoundChamferedRectToPolygon( outline, padShapePos, shapesize, angle, radius,
                                              chamferRatio,
                                              doChamfer ? GetChamferPositions() : 0,
                                              aError, aErrorLoc );

        aCornerBuffer.Append( outline );
    }
        break;

    case PAD_SHAPE::CUSTOM:
    {
        SHAPE_POLY_SET outline;
        MergePrimitivesAsPolygon( &outline, aLayer, aErrorLoc );
        outline.Rotate( -DECIDEG2RAD( m_orient ) );
        outline.Move( VECTOR2I( m_pos ) );

        if( aClearanceValue )
        {
            int numSegs = std::max( GetArcToSegmentCount( aClearanceValue, aError, 360.0 ),
                                                          pad_min_seg_per_circle_count );
            int clearance = aClearanceValue;

            if( aErrorLoc == ERROR_OUTSIDE )
                clearance += GetCircleToPolyCorrection( aError );

            outline.Inflate( clearance, numSegs );
            outline.Simplify( SHAPE_POLY_SET::PM_FAST );
            outline.Fracture( SHAPE_POLY_SET::PM_FAST );
        }

        aCornerBuffer.Append( outline );
    }
        break;

    default:
        wxFAIL_MSG( "PAD::TransformShapeWithClearanceToPolygon no implementation for "
                    + PAD_SHAPE_T_asString( GetShape() ) );
        break;
    }
}



bool PAD::TransformHoleWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer, int aInflateValue,
                                               int aError, ERROR_LOC aErrorLoc ) const
{
    wxSize drillsize = GetDrillSize();

    if( !drillsize.x || !drillsize.y )
        return false;

    const SHAPE_SEGMENT* seg = GetEffectiveHoleShape();

    TransformOvalToPolygon( aCornerBuffer, (wxPoint) seg->GetSeg().A, (wxPoint) seg->GetSeg().B,
                            seg->GetWidth() + aInflateValue * 2, aError, aErrorLoc );

    return true;
}


void ZONE::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                 PCB_LAYER_ID aLayer, int aClearance, int aError,
                                                 ERROR_LOC aErrorLoc, bool aIgnoreLineWidth ) const
{
    wxASSERT_MSG( !aIgnoreLineWidth, "IgnoreLineWidth has no meaning for zones." );

    if( !m_FilledPolysList.count( aLayer ) )
        return;

    aCornerBuffer = m_FilledPolysList.at( aLayer );

    int numSegs = GetArcToSegmentCount( aClearance, aError, 360.0 );
    aCornerBuffer.Inflate( aClearance, numSegs );
    aCornerBuffer.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
}


void DIMENSION_BASE::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                           PCB_LAYER_ID aLayer, int aClearance,
                                                           int aError, ERROR_LOC aErrorLoc,
                                                           bool aIgnoreLineWidth ) const
{
    wxASSERT_MSG( !aIgnoreLineWidth, "IgnoreLineWidth has no meaning for dimensions." );

    for( const std::shared_ptr<SHAPE>& shape : m_shapes )
    {
        const SHAPE_CIRCLE*  circle = dynamic_cast<const SHAPE_CIRCLE*>( shape.get() );
        const SHAPE_SEGMENT* seg    = dynamic_cast<const SHAPE_SEGMENT*>( shape.get() );

        if( circle )
        {
            TransformCircleToPolygon( aCornerBuffer, (wxPoint) circle->GetCenter(),
                                      circle->GetRadius() + m_lineThickness / 2 + aClearance,
                                      aError, aErrorLoc );
        }
        else if( seg )
        {
            TransformOvalToPolygon( aCornerBuffer, (wxPoint) seg->GetSeg().A,
                                    (wxPoint) seg->GetSeg().B, m_lineThickness + 2 * aClearance,
                                    aError, aErrorLoc );
        }
        else
        {
            wxFAIL_MSG( "DIMENSION::TransformShapeWithClearanceToPolygon unexpected shape type." );
        }
    }
}

