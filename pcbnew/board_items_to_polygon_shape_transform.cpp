/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <vector>
#include <bezier_curves.h>
#include <base_units.h>     // for IU_PER_MM
#include <gr_text.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <trigo.h>
#include <class_board.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_zone.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/geometry_utils.h>

// A helper struct for the callback function
// These variables are parameters used in addTextSegmToPoly.
// But addTextSegmToPoly is a call-back function,
// so we cannot send them as arguments.
struct TSEGM_2_POLY_PRMS {
    int m_textWidth;
    int m_error;
    SHAPE_POLY_SET* m_cornerBuffer;
};
TSEGM_2_POLY_PRMS prms;

// This is a call back function, used by GRText to draw the 3D text shape:
static void addTextSegmToPoly( int x0, int y0, int xf, int yf, void* aData )
{
    TSEGM_2_POLY_PRMS* prm = static_cast<TSEGM_2_POLY_PRMS*>( aData );
    TransformSegmentToPolygon( *prm->m_cornerBuffer,
                               wxPoint( x0, y0 ), wxPoint( xf, yf ),
                               prm->m_error, prm->m_textWidth );
}


void BOARD::ConvertBrdLayerToPolygonalContours( PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aOutlines )
{
    // convert tracks and vias:
    for( auto track : m_tracks )
    {
        if( !track->IsOnLayer( aLayer ) )
            continue;

        track->TransformShapeWithClearanceToPolygon( aOutlines, 0 );
    }

    // convert pads
    for( auto module : m_modules )
    {
        module->TransformPadsShapesWithClearanceToPolygon( aLayer, aOutlines, 0 );

        // Micro-wave modules may have items on copper layers
        module->TransformGraphicShapesWithClearanceToPolygonSet( aLayer, aOutlines, 0 );
    }

    // convert copper zones
    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = GetArea( ii );
        PCB_LAYER_ID    zonelayer = zone->GetLayer();

        if( zonelayer == aLayer )
            zone->TransformSolidAreasShapesToPolygonSet( aOutlines );
    }

    // convert graphic items on copper layers (texts)
    for( auto item : m_drawings )
    {
        if( !item->IsOnLayer( aLayer ) )
            continue;

        switch( item->Type() )
        {
        case PCB_LINE_T:
            ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon( aOutlines, 0 );
            break;

        case PCB_TEXT_T:
            ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygonSet( aOutlines, 0 );
            break;

        default:
            break;
        }
    }
}


void MODULE::TransformPadsShapesWithClearanceToPolygon( PCB_LAYER_ID aLayer,
        SHAPE_POLY_SET& aCornerBuffer, int aInflateValue, int aMaxError,
        bool aSkipNPTHPadsWihNoCopper ) const
{
    for( auto pad : m_pads )
    {
        if( aLayer != UNDEFINED_LAYER && !pad->IsOnLayer(aLayer) )
            continue;

        // NPTH pads are not drawn on layers if the shape size and pos is the same
        // as their hole:
        if( aSkipNPTHPadsWihNoCopper && pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
        {
            if( pad->GetDrillSize() == pad->GetSize() && pad->GetOffset() == wxPoint( 0, 0 ) )
            {
                switch( pad->GetShape() )
                {
                case PAD_SHAPE_CIRCLE:
                    if( pad->GetDrillShape() == PAD_DRILL_SHAPE_CIRCLE )
                        continue;
                    break;

                case PAD_SHAPE_OVAL:
                    if( pad->GetDrillShape() != PAD_DRILL_SHAPE_CIRCLE )
                        continue;
                    break;

                default:
                    break;
                }
            }
        }

        wxSize margin;
        int clearance = aInflateValue;

        switch( aLayer )
        {
        case F_Mask:
        case B_Mask:
            clearance += pad->GetSolderMaskMargin();
            break;

        case F_Paste:
        case B_Paste:
            margin = pad->GetSolderPasteMargin();
            clearance += ( margin.x + margin.y ) / 2;
            break;

        default:
            break;
        }

        pad->TransformShapeWithClearanceToPolygon( aCornerBuffer, clearance );
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
void MODULE::TransformGraphicShapesWithClearanceToPolygonSet( PCB_LAYER_ID aLayer,
                                                              SHAPE_POLY_SET& aCornerBuffer,
                                                              int aInflateValue,
                                                              int aError, bool aIncludeText ) const
{
    std::vector<TEXTE_MODULE *> texts;  // List of TEXTE_MODULE to convert

    for( auto item : GraphicalItems() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );

            if( ( aLayer != UNDEFINED_LAYER && text->GetLayer() == aLayer ) && text->IsVisible() )
                texts.push_back( text );
        }
            break;

        case PCB_MODULE_EDGE_T:
        {
            EDGE_MODULE* outline = (EDGE_MODULE*) item;

            if( aLayer != UNDEFINED_LAYER && outline->GetLayer() != aLayer )
                break;

            outline->TransformShapeWithClearanceToPolygon( aCornerBuffer, 0, aError );
        }
            break;

        default:
            break;
        }
    }

    if( !aIncludeText )
        return;

    // Convert texts sur modules
    if( Reference().GetLayer() == aLayer && Reference().IsVisible() )
        texts.push_back( &Reference() );

    if( Value().GetLayer() == aLayer && Value().IsVisible() )
        texts.push_back( &Value() );

    prms.m_cornerBuffer = &aCornerBuffer;

    for( TEXTE_MODULE* textmod : texts )
    {
        prms.m_textWidth  = textmod->GetThickness() + ( 2 * aInflateValue );
        prms.m_error = aError;
        wxSize size = textmod->GetTextSize();

        if( textmod->IsMirrored() )
            size.x = -size.x;

        GRText( NULL, textmod->GetTextPos(), BLACK, textmod->GetShownText(),
                textmod->GetDrawRotation(), size, textmod->GetHorizJustify(),
                textmod->GetVertJustify(), textmod->GetThickness(), textmod->IsItalic(),
                true, addTextSegmToPoly, &prms );
    }

}


// Same as function TransformGraphicShapesWithClearanceToPolygonSet but this only for text
void MODULE::TransformGraphicTextWithClearanceToPolygonSet( PCB_LAYER_ID aLayer,
                                                            SHAPE_POLY_SET& aCornerBuffer,
                                                            int aInflateValue, int aError ) const
{
    std::vector<TEXTE_MODULE *> texts;  // List of TEXTE_MODULE to convert

    for( auto item : GraphicalItems() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
        {
            TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );

            if( text->GetLayer() == aLayer && text->IsVisible() )
                texts.push_back( text );
        }
            break;

        case PCB_MODULE_EDGE_T:
            // This function does not render this
            break;

        default:
            break;
        }
    }

    // Convert texts sur modules
    if( Reference().GetLayer() == aLayer && Reference().IsVisible() )
        texts.push_back( &Reference() );

    if( Value().GetLayer() == aLayer && Value().IsVisible() )
        texts.push_back( &Value() );

    prms.m_cornerBuffer = &aCornerBuffer;

    for( TEXTE_MODULE* textmod : texts )
    {
        prms.m_textWidth = textmod->GetThickness() + ( 2 * aInflateValue );
        prms.m_error = aError;
        wxSize size = textmod->GetTextSize();

        if( textmod->IsMirrored() )
            size.x = -size.x;

        GRText( NULL, textmod->GetTextPos(), BLACK, textmod->GetShownText(),
                textmod->GetDrawRotation(), size, textmod->GetHorizJustify(),
                textmod->GetVertJustify(), textmod->GetThickness(), textmod->IsItalic(),
                true, addTextSegmToPoly, &prms );
    }

}


void ZONE_CONTAINER::TransformSolidAreasShapesToPolygonSet( SHAPE_POLY_SET& aCornerBuffer,
                                                            int aError ) const
{
    if( GetFilledPolysList().IsEmpty() )
        return;

    // add filled areas polygons
    aCornerBuffer.Append( m_FilledPolysList );
    auto board = GetBoard();
    int maxError = ARC_HIGH_DEF;

    if( board )
        maxError = board->GetDesignSettings().m_MaxError;

    // add filled areas outlines, which are drawn with thick lines
    for( int i = 0; i < m_FilledPolysList.OutlineCount(); i++ )
    {
        const SHAPE_LINE_CHAIN& path = m_FilledPolysList.COutline( i );

        for( int j = 0; j < path.PointCount(); j++ )
        {
            const VECTOR2I& a = path.CPoint( j );
            const VECTOR2I& b = path.CPoint( j + 1 );
            int             width = GetMinThickness();

            TransformSegmentToPolygon( aCornerBuffer, (wxPoint) a, (wxPoint) b, maxError, width );
        }
    }
}


void EDA_TEXT::TransformBoundingBoxWithClearanceToPolygon( SHAPE_POLY_SET* aCornerBuffer,
                                                           int aClearanceValue ) const
{
    if( GetText().Length() == 0 )
        return;

    wxPoint  corners[4];    // Buffer of polygon corners

    EDA_RECT rect = GetTextBox( -1 );
    rect.Inflate( aClearanceValue );
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
 * Function TransformShapeWithClearanceToPolygonSet
 * Convert the text shape to a set of polygons (one per segment).
 * @aCornerBuffer = SHAPE_POLY_SET to store the polygon corners
 * @aClearanceValue = the clearance around the text
 * @aError = the maximum error to allow when approximating curves
 */
void TEXTE_PCB::TransformShapeWithClearanceToPolygonSet( SHAPE_POLY_SET& aCornerBuffer,
                                                         int aClearanceValue, int aError ) const
{
    wxSize size = GetTextSize();

    if( IsMirrored() )
        size.x = -size.x;

    prms.m_cornerBuffer = &aCornerBuffer;
    prms.m_textWidth = GetThickness() + ( 2 * aClearanceValue );
    prms.m_error = aError;
    COLOR4D color = COLOR4D::BLACK;  // not actually used, but needed by GRText

    if( IsMultilineAllowed() )
    {
        wxArrayString strings_list;
        wxStringSplit( GetShownText(), strings_list, '\n' );
        std::vector<wxPoint> positions;
        positions.reserve( strings_list.Count() );
        GetPositionsOfLinesOfMultilineText( positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxString txt = strings_list.Item( ii );
            GRText( NULL, positions[ii], color, txt, GetTextAngle(), size, GetHorizJustify(),
                    GetVertJustify(), GetThickness(), IsItalic(), true, addTextSegmToPoly, &prms );
        }
    }
    else
    {
        GRText( NULL, GetTextPos(), color, GetShownText(), GetTextAngle(), size, GetHorizJustify(),
                GetVertJustify(), GetThickness(), IsItalic(), true, addTextSegmToPoly, &prms );
    }
}


void DRAWSEGMENT::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                        int aClearanceValue, int aError,
                                                        bool ignoreLineWidth ) const
{
    int width = ignoreLineWidth ? 0 : m_Width;

    width += 2 * aClearanceValue;

    // Creating a reliable clearance shape for circles and arcs is not so easy, due to
    // the error created by segment approximation.
    // for a circle this is not so hard: create a polygon from a circle slightly bigger:
    // thickness = width + s_error_max, and radius = initial radius + s_error_max/2
    // giving a shape with a suitable internal radius and external radius
    // For an arc this is more tricky: TODO

    switch( m_Shape )
    {
    case S_CIRCLE:
        TransformRingToPolygon( aCornerBuffer, GetCenter(), GetRadius(), aError, width );
        break;

    case S_ARC:
        TransformArcToPolygon( aCornerBuffer, GetCenter(), GetArcStart(), m_Angle, aError, width );
        break;

    case S_SEGMENT:
        TransformOvalToPolygon( aCornerBuffer, m_Start, m_End, width, aError );
        break;

    case S_POLYGON:
        if( IsPolyShapeValid() )
        {
            // The polygon is expected to be a simple polygon
            // not self intersecting, no hole.
            MODULE* module = GetParentModule();     // NULL for items not in footprints
            double orientation = module ? module->GetOrientation() : 0.0;
            wxPoint offset;

            if( module )
                offset = module->GetPosition();

            // Build the polygon with the actual position and orientation:
            std::vector< wxPoint> poly;
            poly = BuildPolyPointsList();

            for( wxPoint& point : poly )
            {
                RotatePoint( &point, orientation );
                point += offset;
            }

            // If the polygon is not filled, treat it as a closed set of lines
            if( !IsPolygonFilled() )
            {
                for( size_t ii = 1; ii < poly.size(); ii++ )
                    TransformOvalToPolygon( aCornerBuffer, poly[ii-1], poly[ii], width, aError );

                TransformOvalToPolygon( aCornerBuffer, poly.back(), poly.front(), width, aError );
                break;
            }

            // Generate polygons for the outline + clearance
            // This code is compatible with a polygon with holes linked to external outline
            // by overlapping segments.

            // Insert the initial polygon:
            aCornerBuffer.NewOutline();

            for( wxPoint& point : poly )
                aCornerBuffer.Append( point.x, point.y );

            if( width != 0 )     // Add thick outlines
            {
                wxPoint pt1( poly[ poly.size() - 1] );

                for( wxPoint pt2 : poly )
                {
                    if( pt2 != pt1 )
                        TransformSegmentToPolygon( aCornerBuffer, pt1, pt2, aError, width );

                    pt1 = pt2;
                }
            }
        }
        break;

    case S_CURVE:       // Bezier curve
        {
            std::vector<wxPoint> ctrlPoints = { m_Start, m_BezierC1, m_BezierC2, m_End };
            BEZIER_POLY converter( ctrlPoints );
            std::vector< wxPoint> poly;
            converter.GetPoly( poly, m_Width );

            if( width != 0 )
            {
                for( unsigned ii = 1; ii < poly.size(); ii++ )
                    TransformSegmentToPolygon( aCornerBuffer, poly[ii-1], poly[ii], aError, width );
            }
        }
        break;

    default:
        break;
    }
}


void TRACK::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                  int aClearanceValue, int aError,
                                                  bool ignoreLineWidth ) const
{
    wxASSERT_MSG( !ignoreLineWidth, "IgnoreLineWidth has no meaning for tracks." );

    switch( Type() )
    {
    case PCB_VIA_T:
    {
        int radius = ( m_Width / 2 ) + aClearanceValue;
        TransformCircleToPolygon( aCornerBuffer, m_Start, radius, aError );
    }
        break;

    default:
    {
        int width = m_Width + ( 2 * aClearanceValue );
        TransformOvalToPolygon( aCornerBuffer, m_Start, m_End, width, aError );
    }
        break;
    }
}


void D_PAD::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                  int aClearanceValue, int aError,
                                                  bool ignoreLineWidth ) const
{
    wxASSERT_MSG( !ignoreLineWidth, "IgnoreLineWidth has no meaning for pads." );

    // minimal segment count to approximate a circle to create the polygonal pad shape
    // This minimal value is mainly for very small pads, like SM0402.
    // Most of time pads are using the segment count given by aError value.
    const int pad_min_seg_per_circle_count = 16;
    double  angle = m_Orient;
    int     dx = (m_Size.x / 2) + aClearanceValue;
    int     dy = (m_Size.y / 2) + aClearanceValue;

    wxPoint padShapePos = ShapePos();         // Note: for pad having a shape offset,
                                              // the pad position is NOT the shape position

    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
        TransformCircleToPolygon( aCornerBuffer, padShapePos, dx, aError );
        break;

    case PAD_SHAPE_OVAL:

        // If the oval is actually a circle (same x/y size), treat it the same
        if( dx == dy )
        {
            TransformCircleToPolygon( aCornerBuffer, padShapePos, dx, aError );
        }
        else
        {
            int width;
            wxPoint shape_offset;
            if( dy > dx )   // Oval pad X/Y ratio for choosing translation axis
            {
                shape_offset.y = dy - dx;
                width = dx * 2;
            }
            else    //if( dy <= dx )
            {
                shape_offset.x = dy - dx;
                width = dy * 2;
            }

            RotatePoint( &shape_offset, angle );
            wxPoint start = padShapePos - shape_offset;
            wxPoint end = padShapePos + shape_offset;
            TransformOvalToPolygon( aCornerBuffer, start, end, width, aError );
        }

        break;

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_RECT:
    {
        wxPoint corners[4];
        BuildPadPolygon( corners, wxSize( 0, 0 ), angle );

        SHAPE_POLY_SET outline;
        outline.NewOutline();

        for( wxPoint& corner : corners )
        {
            corner += padShapePos;
            outline.Append( corner.x, corner.y );
        }

        if( aClearanceValue )
        {
            int    numSegs = std::max( GetArcToSegmentCount( aClearanceValue, aError, 360.0 ),
                                       pad_min_seg_per_circle_count );
            double correction = GetCircletoPolyCorrectionFactor( numSegs );
            int    clearance = KiROUND( aClearanceValue * correction );
            outline.Inflate( clearance, numSegs );
        }

        aCornerBuffer.Append( outline );
    }
        break;

    case PAD_SHAPE_CHAMFERED_RECT:
    case PAD_SHAPE_ROUNDRECT:
    {
        int    radius = GetRoundRectCornerRadius() + aClearanceValue;
        int    numSegs = std::max( GetArcToSegmentCount( radius, aError, 360.0 ),
                                   pad_min_seg_per_circle_count );
        double correction = GetCircletoPolyCorrectionFactor( numSegs );
        int    clearance = KiROUND( aClearanceValue * correction );
        wxSize shapesize( m_Size );

        radius = KiROUND( radius * correction );
        shapesize.x += clearance * 2;
        shapesize.y += clearance * 2;
        bool doChamfer = GetShape() == PAD_SHAPE_CHAMFERED_RECT;

        SHAPE_POLY_SET outline;
        TransformRoundChamferedRectToPolygon( outline, padShapePos, shapesize, angle, radius,
                                              doChamfer ? GetChamferRectRatio() : 0.0,
                                              doChamfer ? GetChamferPositions() : 0, aError );

        aCornerBuffer.Append( outline );
    }
        break;

    case PAD_SHAPE_CUSTOM:
    {
        int    numSegs = std::max( GetArcToSegmentCount( aClearanceValue, aError, 360.0 ),
                                                         pad_min_seg_per_circle_count );
        double correction = GetCircletoPolyCorrectionFactor( numSegs );
        int    clearance = KiROUND( aClearanceValue * correction );
        SHAPE_POLY_SET outline;     // Will contain the corners in board coordinates
        outline.Append( m_customShapeAsPolygon );
        CustomShapeAsPolygonToBoardPosition( &outline, GetPosition(), GetOrientation() );
        outline.Simplify( SHAPE_POLY_SET::PM_FAST );
        outline.Inflate( clearance, numSegs );
        outline.Fracture( SHAPE_POLY_SET::PM_FAST );
        aCornerBuffer.Append( outline );
    }
        break;
    }
}



/*
 * Function BuildPadShapePolygon
 * Build the corner list of the polygonal shape, depending on shape, clearance and orientation
 * Note: for round & oval pads this function is equivalent to TransformShapeWithClearanceToPolygon,
 * but not for other shapes
 */
void D_PAD::BuildPadShapePolygon( SHAPE_POLY_SET& aCornerBuffer, wxSize aInflateValue,
                                  int aError ) const
{
    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
    case PAD_SHAPE_OVAL:
    case PAD_SHAPE_ROUNDRECT:
    case PAD_SHAPE_CHAMFERED_RECT:
    {
        // We are using TransformShapeWithClearanceToPolygon to build the shape.
        // Currently, this method uses only the same inflate value for X and Y dirs.
        // so because here this is not the case, we use a inflated dummy pad to build
        // the polygonal shape
        // TODO: remove this dummy pad when TransformShapeWithClearanceToPolygon will use
        // a wxSize to inflate the pad size
        D_PAD dummy( *this );
        dummy.SetSize( GetSize() + aInflateValue + aInflateValue );
        dummy.TransformShapeWithClearanceToPolygon( aCornerBuffer, 0 );
    }
        break;

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_RECT:
    {
        wxPoint corners[4];
        wxPoint padShapePos = ShapePos();    // Note: for pad having a shape offset,
                                             // the pad position is NOT the shape position

        aCornerBuffer.NewOutline();
        BuildPadPolygon( corners, aInflateValue, m_Orient );

        for( wxPoint& corner : corners )
        {
            corner += padShapePos;          // Shift origin to position
            aCornerBuffer.Append( corner.x, corner.y );
        }
    }
        break;

    case PAD_SHAPE_CUSTOM:
    {
        // For a custom shape, that is in fact a polygon (with holes), we use only a single
        // inflate value (different values for X and Y have no definition for a custom pad).
        int inflate = ( aInflateValue.x + aInflateValue.y ) / 2;

        TransformShapeWithClearanceToPolygon( aCornerBuffer, inflate );
    }
        break;
    }
}


bool D_PAD::BuildPadDrillShapePolygon(
        SHAPE_POLY_SET& aCornerBuffer, int aInflateValue, int aError ) const
{
    wxSize drillsize = GetDrillSize();

    if( !drillsize.x || !drillsize.y )
        return false;

    if( drillsize.x == drillsize.y )    // usual round hole
    {
        int radius = ( drillsize.x / 2 ) + aInflateValue;
        TransformCircleToPolygon( aCornerBuffer, GetPosition(), radius, aError );
    }
    else    // Oblong hole
    {
        wxPoint start, end;
        int width;

        GetOblongDrillGeometry( start, end, width );

        start += GetPosition();
        end += GetPosition();
        width += aInflateValue * 2;

        TransformSegmentToPolygon( aCornerBuffer, start, end, aError, width );
    }

    return true;
}


void ZONE_CONTAINER::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                           int aClearanceValue, int aError,
                                                           bool ignoreLineWidth ) const
{
    wxASSERT_MSG( !ignoreLineWidth, "IgnoreLineWidth has no meaning for zones." );

    aCornerBuffer = m_FilledPolysList;
    aCornerBuffer.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
}
