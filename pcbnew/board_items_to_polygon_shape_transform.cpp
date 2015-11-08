/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/***
 * @file board_items_to_polygon_shape_transform.cpp
 * @brief function to convert shapes of items ( pads, tracks... ) to polygons
 */

/* Function to convert pad and track shapes to polygons
 * Used to fill zones areas and in 3D viewer
 */
#include <vector>

#include <fctsys.h>
#include <drawtxt.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
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

// These variables are parameters used in addTextSegmToPoly.
// But addTextSegmToPoly is a call-back function,
// so we cannot send them as arguments.
static int s_textWidth;
static int s_textCircle2SegmentCount;
static SHAPE_POLY_SET* s_cornerBuffer;

// This is a call back function, used by DrawGraphicText to draw the 3D text shape:
static void addTextSegmToPoly( int x0, int y0, int xf, int yf )
{
    TransformRoundedEndsSegmentToPolygon( *s_cornerBuffer,
                                           wxPoint( x0, y0), wxPoint( xf, yf ),
                                           s_textCircle2SegmentCount, s_textWidth );
}


void BOARD::ConvertBrdLayerToPolygonalContours( LAYER_ID aLayer, SHAPE_POLY_SET& aOutlines )
{
    // Number of segments to convert a circle to a polygon
    const int       segcountforcircle   = 18;
    double          correctionFactor    = 1.0 / cos( M_PI / (segcountforcircle * 2) );

    // convert tracks and vias:
    for( TRACK* track = m_Track; track != NULL; track = track->Next() )
    {
        if( !track->IsOnLayer( aLayer ) )
            continue;

        track->TransformShapeWithClearanceToPolygon( aOutlines,
                0, segcountforcircle, correctionFactor );
    }

    // convert pads
    for( MODULE* module = m_Modules; module != NULL; module = module->Next() )
    {
        module->TransformPadsShapesWithClearanceToPolygon( aLayer,
                aOutlines, 0, segcountforcircle, correctionFactor );

        // Micro-wave modules may have items on copper layers
        module->TransformGraphicShapesWithClearanceToPolygonSet( aLayer,
                aOutlines, 0, segcountforcircle, correctionFactor );
    }

    // convert copper zones
    for( int ii = 0; ii < GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = GetArea( ii );
        LAYER_ID        zonelayer = zone->GetLayer();

        if( zonelayer == aLayer )
            zone->TransformSolidAreasShapesToPolygonSet(
                aOutlines, segcountforcircle, correctionFactor );
    }

    // convert graphic items on copper layers (texts)
    for( BOARD_ITEM* item = m_Drawings; item; item = item->Next() )
    {
        if( !item->IsOnLayer( aLayer ) )
            continue;

        switch( item->Type() )
        {
        case PCB_LINE_T:    // should not exist on copper layers
            ( (DRAWSEGMENT*) item )->TransformShapeWithClearanceToPolygon(
                aOutlines, 0, segcountforcircle, correctionFactor );
            break;

        case PCB_TEXT_T:
            ( (TEXTE_PCB*) item )->TransformShapeWithClearanceToPolygonSet(
                aOutlines, 0, segcountforcircle, correctionFactor );
            break;

        default:
            break;
        }
    }
}


void MODULE::TransformPadsShapesWithClearanceToPolygon( LAYER_ID aLayer,
                        SHAPE_POLY_SET& aCornerBuffer,
                        int                    aInflateValue,
                        int                    aCircleToSegmentsCount,
                        double                 aCorrectionFactor,
                        bool                   aSkipNPTHPadsWihNoCopper )
{
    D_PAD* pad = Pads();

    wxSize margin;
    for( ; pad != NULL; pad = pad->Next() )
    {
        if( !pad->IsOnLayer(aLayer) )
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

        switch( aLayer )
        {
        case F_Mask:
        case B_Mask:
            margin.x = margin.y = pad->GetSolderMaskMargin() + aInflateValue;
            break;

        case F_Paste:
        case B_Paste:
            margin = pad->GetSolderPasteMargin();
            margin.x += aInflateValue;
            margin.y += aInflateValue;
            break;

        default:
            margin.x = margin.y = aInflateValue;
            break;
        }

        pad->BuildPadShapePolygon( aCornerBuffer, margin,
                                   aCircleToSegmentsCount, aCorrectionFactor );
    }
}

/* generate shapes of graphic items (outlines) on layer aLayer as polygons,
 * and adds these polygons to aCornerBuffer
 * aCornerBuffer = the buffer to store polygons
 * aInflateValue = a value to inflate shapes
 * aCircleToSegmentsCount = number of segments to approximate a circle
 * aCorrectionFactor = the correction to apply to the circle radius
 *  to generate the polygon.
 *  if aCorrectionFactor = 1.0, the polygon is inside the circle
 *  the radius of circle approximated by segments is
 *  initial radius * aCorrectionFactor
 */
void MODULE::TransformGraphicShapesWithClearanceToPolygonSet(
                        LAYER_ID        aLayer,
                        SHAPE_POLY_SET& aCornerBuffer,
                        int             aInflateValue,
                        int             aCircleToSegmentsCount,
                        double          aCorrectionFactor,
                        int             aCircleToSegmentsCountForTexts )
{
    std::vector<TEXTE_MODULE *> texts;  // List of TEXTE_MODULE to convert
    EDGE_MODULE* outline;

    for( EDA_ITEM* item = GraphicalItems(); item != NULL; item = item->Next() )
    {
        switch( item->Type() )
        {
        case PCB_MODULE_TEXT_T:
            {
                TEXTE_MODULE* text = static_cast<TEXTE_MODULE*>( item );

                if( text->GetLayer() == aLayer && text->IsVisible() )
                    texts.push_back( text );

                break;
            }

        case PCB_MODULE_EDGE_T:
            outline = (EDGE_MODULE*) item;

            if( outline->GetLayer() != aLayer )
                break;
                outline->TransformShapeWithClearanceToPolygon( aCornerBuffer,
                                                        0,
                                                        aCircleToSegmentsCount,
                                                        aCorrectionFactor );
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

    s_cornerBuffer = &aCornerBuffer;

    // To allow optimization of circles approximated by segments,
    // aCircleToSegmentsCountForTexts, when not 0, is used.
    // if 0 (default value) the aCircleToSegmentsCount is used
    s_textCircle2SegmentCount = aCircleToSegmentsCountForTexts ?
                                aCircleToSegmentsCountForTexts : aCircleToSegmentsCount;

    for( unsigned ii = 0; ii < texts.size(); ii++ )
    {
        TEXTE_MODULE *textmod = texts[ii];
        s_textWidth  = textmod->GetThickness() + ( 2 * aInflateValue );
        wxSize size = textmod->GetSize();

        if( textmod->IsMirrored() )
            size.x = -size.x;

        DrawGraphicText( NULL, NULL, textmod->GetTextPosition(), BLACK,
                         textmod->GetShownText(), textmod->GetDrawRotation(), size,
                         textmod->GetHorizJustify(), textmod->GetVertJustify(),
                         textmod->GetThickness(), textmod->IsItalic(),
                         true, addTextSegmToPoly );
    }

}

 /* Function TransformSolidAreasShapesToPolygonSet
 * Convert solid areas full shapes to polygon set
 * (the full shape is the polygon area with a thick outline)
 * Used in 3D view
 * Arcs (ends of segments) are approximated by segments
 * aCornerBuffer = a buffer to store the polygons
 * aCircleToSegmentsCount = the number of segments to approximate a circle
 * aCorrectionFactor = the correction to apply to arcs radius to roughly
 * keep arc radius when approximated by segments
 */
void ZONE_CONTAINER::TransformSolidAreasShapesToPolygonSet(
        SHAPE_POLY_SET& aCornerBuffer,
        int                    aCircleToSegmentsCount,
        double                 aCorrectionFactor )
{
    if( GetFilledPolysList().IsEmpty() )
        return;

    // add filled areas polygons
    aCornerBuffer.Append( m_FilledPolysList );

    // add filled areas outlines, which are drawn with thick lines
    for( int i = 0; i < m_FilledPolysList.OutlineCount(); i++ )
    {
        const SHAPE_LINE_CHAIN& path = m_FilledPolysList.COutline( i );

        for( int j = 0; j < path.PointCount(); j++ )
        {
            const VECTOR2I& a = path.CPoint( j );
            const VECTOR2I& b = path.CPoint( j + 1 );

            TransformRoundedEndsSegmentToPolygon( aCornerBuffer, wxPoint( a.x, a.y ), wxPoint( b.x, b.y ),
                                                    aCircleToSegmentsCount,
                                                    GetMinThickness() );
        }
    }
}

/**
 * Function TransformBoundingBoxWithClearanceToPolygon
 * Convert the text bounding box to a rectangular polygon
 * Used in filling zones calculations
 * Circles and arcs are approximated by segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aClearanceValue = the clearance around the text bounding box
 */
void TEXTE_PCB::TransformBoundingBoxWithClearanceToPolygon(
                    SHAPE_POLY_SET& aCornerBuffer,
                    int             aClearanceValue ) const
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

    aCornerBuffer.NewOutline();

    for( int ii = 0; ii < 4; ii++ )
    {
        // Rotate polygon
        RotatePoint( &corners[ii].x, &corners[ii].y, m_Pos.x, m_Pos.y, m_Orient );
        aCornerBuffer.Append( corners[ii].x, corners[ii].y );
    }
}


/* Function TransformShapeWithClearanceToPolygonSet
 * Convert the text shape to a set of polygons (one by segment)
 * Used in filling zones calculations and 3D view
 * Circles and arcs are approximated by segments
 * aCornerBuffer = SHAPE_POLY_SET to store the polygon corners
 * aClearanceValue = the clearance around the text
 * aCircleToSegmentsCount = the number of segments to approximate a circle
 * aCorrectionFactor = the correction to apply to circles radius to keep
 * clearance when the circle is approximated by segment bigger or equal
 * to the real clearance value (usually near from 1.0)
 */

void TEXTE_PCB::TransformShapeWithClearanceToPolygonSet(
                            SHAPE_POLY_SET& aCornerBuffer,
                            int                    aClearanceValue,
                            int                    aCircleToSegmentsCount,
                            double                 aCorrectionFactor ) const
{
    wxSize size = GetSize();

    if( IsMirrored() )
        size.x = -size.x;

    s_cornerBuffer = &aCornerBuffer;
    s_textWidth  = GetThickness() + ( 2 * aClearanceValue );
    s_textCircle2SegmentCount = aCircleToSegmentsCount;
    EDA_COLOR_T color = BLACK;  // not actually used, but needed by DrawGraphicText

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
            DrawGraphicText( NULL, NULL, positions[ii], color,
                             txt, GetOrientation(), size,
                             GetHorizJustify(), GetVertJustify(),
                             GetThickness(), IsItalic(),
                             true, addTextSegmToPoly );
        }
    }
    else
    {
        DrawGraphicText( NULL, NULL, GetTextPosition(), color,
                         GetShownText(), GetOrientation(), size,
                         GetHorizJustify(), GetVertJustify(),
                         GetThickness(), IsItalic(),
                         true, addTextSegmToPoly );
    }
}


/**
 * Function TransformShapeWithClearanceToPolygon
 * Convert the track shape to a closed polygon
 * Used in filling zones calculations
 * Circles and arcs are approximated by segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aClearanceValue = the clearance around the pad
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aCorrectionFactor = the correction to apply to circles radius to keep
 * clearance when the circle is approxiamted by segment bigger or equal
 * to the real clearance value (usually near from 1.0)
 */
void DRAWSEGMENT::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                        int             aClearanceValue,
                                                        int             aCircleToSegmentsCount,
                                                        double          aCorrectionFactor ) const
{
    // The full width of the lines to create:
    int linewidth = m_Width + (2 * aClearanceValue);

    switch( m_Shape )
    {
    case S_CIRCLE:
        TransformRingToPolygon( aCornerBuffer, GetCenter(), GetRadius(),
                                aCircleToSegmentsCount, linewidth ) ;
        break;

    case S_ARC:
        TransformArcToPolygon( aCornerBuffer, GetCenter(),
                               GetArcStart(), m_Angle,
                               aCircleToSegmentsCount, linewidth );
        break;

    case S_SEGMENT:
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer, m_Start, m_End,
                                              aCircleToSegmentsCount, linewidth );
        break;

    case S_POLYGON:
        if ( GetPolyPoints().size() < 2 )
            break;      // Malformed polygon.
        {
        // The polygon is expected to be a simple polygon
        // not self intersecting, no hole.
        MODULE* module = GetParentModule();     // NULL for items not in footprints
        double orientation = module ? module->GetOrientation() : 0.0;

        // Build the polygon with the actual position and orientation:
        std::vector< wxPoint> poly;
        poly = GetPolyPoints();

        for( unsigned ii = 0; ii < poly.size(); ii++ )
        {
            RotatePoint( &poly[ii], orientation );
            poly[ii] += GetPosition();
        }

        // Generate polygons for the outline + clearance
        // This code is compatible with a polygon with holes linked to external outline
        // by overlapping segments.

        // Insert the initial polygon:
        aCornerBuffer.NewOutline();

        for( unsigned ii = 0; ii < poly.size(); ii++ )
            aCornerBuffer.Append( poly[ii].x, poly[ii].y );

        if( linewidth )     // Add thick outlines
        {
            CPolyPt corner1( poly[poly.size()-1] );

            for( unsigned ii = 0; ii < poly.size(); ii++ )
            {
                CPolyPt corner2( poly[ii] );

                if( corner2 != corner1 )
                {
                    TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                            corner1, corner2, aCircleToSegmentsCount, linewidth );
                }

                corner1 = corner2;
            }
        }
        }
        break;

    case S_CURVE:       // Bezier curve (TODO: not yet in use)
        break;

    default:
        break;
    }
}


/**
 * Function TransformShapeWithClearanceToPolygon
 * Convert the track shape to a closed polygon
 * Used in filling zones calculations
 * Circles (vias) and arcs (ends of tracks) are approximated by segments
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aClearanceValue = the clearance around the pad
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aCorrectionFactor = the correction to apply to circles radius to keep
 * clearance when the circle is approximated by segment bigger or equal
 * to the real clearance value (usually near from 1.0)
 */
void TRACK::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                   int                      aClearanceValue,
                                                   int                      aCircleToSegmentsCount,
                                                   double                   aCorrectionFactor ) const
{
    switch( Type() )
    {
    case PCB_VIA_T:
    {
        int radius = (m_Width / 2) + aClearanceValue;
        radius = KiROUND( radius * aCorrectionFactor );
        TransformCircleToPolygon( aCornerBuffer, m_Start, radius, aCircleToSegmentsCount );
    }
        break;

    default:
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                                              m_Start, m_End,
                                              aCircleToSegmentsCount,
                                              m_Width + ( 2 * aClearanceValue) );
        break;
    }
}


/* Function TransformShapeWithClearanceToPolygon
 * Convert the pad shape to a closed polygon
 * Used in filling zones calculations and 3D view generation
 * Circles and arcs are approximated by segments
 * aCornerBuffer = a SHAPE_POLY_SET to store the polygon corners
 * aClearanceValue = the clearance around the pad
 * aCircleToSegmentsCount = the number of segments to approximate a circle
 * aCorrectionFactor = the correction to apply to circles radius to keep
 * clearance when the circle is approximated by segment bigger or equal
 * to the real clearance value (usually near from 1.0)
 */
void D_PAD:: TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                   int             aClearanceValue,
                                                   int             aCircleToSegmentsCount,
                                                   double          aCorrectionFactor ) const
{
    double  angle = m_Orient;
    int     dx = (m_Size.x / 2) + aClearanceValue;
    int     dy = (m_Size.y / 2) + aClearanceValue;

    wxPoint PadShapePos = ShapePos();               /* Note: for pad having a shape offset,
                                                     * the pad position is NOT the shape position */

    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
        dx = KiROUND( dx * aCorrectionFactor );
        TransformCircleToPolygon( aCornerBuffer, PadShapePos, dx,
                                  aCircleToSegmentsCount );
        break;

    case PAD_SHAPE_OVAL:
        // An oval pad has the same shape as a segment with rounded ends
        {
        int width;
        wxPoint shape_offset;
        if( dy > dx )   // Oval pad X/Y ratio for choosing translation axis
        {
            dy = KiROUND( dy * aCorrectionFactor );
            shape_offset.y = dy - dx;
            width = dx * 2;
        }
        else    //if( dy <= dx )
        {
            dx = KiROUND( dx * aCorrectionFactor );
            shape_offset.x = dy - dx;
            width = dy * 2;
        }

        RotatePoint( &shape_offset, angle );
        wxPoint start = PadShapePos - shape_offset;
        wxPoint end = PadShapePos + shape_offset;
        TransformRoundedEndsSegmentToPolygon( aCornerBuffer, start, end,
                                              aCircleToSegmentsCount, width );
        }
        break;

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_RECT:
    {
        wxPoint corners[4];
        BuildPadPolygon( corners, wxSize( 0, 0 ), angle );

        SHAPE_POLY_SET outline;

        outline.NewOutline();

        for( int ii = 0; ii < 4; ii++ )
        {
            corners[ii] += PadShapePos;
            outline.Append( corners[ii].x, corners[ii].y );
        }

        double rounding_radius = aClearanceValue * aCorrectionFactor;

        outline.Inflate( (int) rounding_radius, aCircleToSegmentsCount );

        aCornerBuffer.Append( outline );
    }
        break;
    }
}

/*
 * Function BuildPadShapePolygon
 * Build the Corner list of the polygonal shape,
 * depending on shape, extra size (clearance ...) pad and orientation
 * Note: for Round and oval pads this function is equivalent to
 * TransformShapeWithClearanceToPolygon, but not for other shapes
 */
void D_PAD::BuildPadShapePolygon( SHAPE_POLY_SET& aCornerBuffer,
                                  wxSize aInflateValue, int aSegmentsPerCircle,
                                  double aCorrectionFactor ) const
{
    wxPoint corners[4];
    wxPoint PadShapePos = ShapePos();         /* Note: for pad having a shape offset,
                                                     * the pad position is NOT the shape position */
    switch( GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
    case PAD_SHAPE_OVAL:
        TransformShapeWithClearanceToPolygon( aCornerBuffer, aInflateValue.x,
                                              aSegmentsPerCircle, aCorrectionFactor );
        break;

    case PAD_SHAPE_TRAPEZOID:
    case PAD_SHAPE_RECT:
        aCornerBuffer.NewOutline();

        BuildPadPolygon( corners, aInflateValue, m_Orient );
        for( int ii = 0; ii < 4; ii++ )
        {
            corners[ii] += PadShapePos;          // Shift origin to position
            aCornerBuffer.Append( corners[ii].x, corners[ii].y );
        }

        break;
    }
}

/*
 * Function BuildPadDrillShapePolygon
 * Build the Corner list of the polygonal drill shape,
 * depending on shape pad hole and orientation
 * return false if the pad has no hole, true otherwise
 */
bool D_PAD::BuildPadDrillShapePolygon( SHAPE_POLY_SET& aCornerBuffer,
                                       int aInflateValue, int aSegmentsPerCircle ) const
{
    wxSize drillsize = GetDrillSize();

    if( !drillsize.x || !drillsize.y )
        return false;

    if( drillsize.x == drillsize.y )    // usual round hole
    {
        TransformCircleToPolygon( aCornerBuffer, GetPosition(),
                (drillsize.x / 2) + aInflateValue, aSegmentsPerCircle );
    }
    else    // Oblong hole
    {
        wxPoint start, end;
        int width;

        GetOblongDrillGeometry( start, end, width );

        width += aInflateValue * 2;

        TransformRoundedEndsSegmentToPolygon( aCornerBuffer,
                GetPosition() + start, GetPosition() + end, aSegmentsPerCircle, width );
    }

    return true;
}

/**
 * Function CreateThermalReliefPadPolygon
 * Add holes around a pad to create a thermal relief
 * copper thickness is min (dx/2, aCopperWitdh) or min (dy/2, aCopperWitdh)
 * @param aCornerBuffer = a buffer to store the polygon
 * @param aPad     = the current pad used to create the thermal shape
 * @param aThermalGap = gap in thermal shape
 * @param aCopperThickness = stubs thickness in thermal shape
 * @param aMinThicknessValue = min copper thickness allowed
 * @param aCircleToSegmentsCount = the number of segments to approximate a circle
 * @param aCorrectionFactor = the correction to apply to circles radius to keep
 * @param aThermalRot = for rond pads the rotation of thermal stubs (450 usually for 45 deg.)
 */

/* thermal reliefs are created as 4 polygons.
 * each corner of a polygon if calculated for a pad at position 0, 0, orient 0,
 * and then moved and rotated acroding to the pad position and orientation
 */

/*
 * Note 1: polygons are drawm using outlines witk a thickness = aMinThicknessValue
 * so shapes must take in account this outline thickness
 *
 * Note 2:
 *      Trapezoidal pads are not considered here because they are very special case
 *      and are used in microwave applications and they *DO NOT* have a thermal relief that
 *      change the shape by creating stubs and destroy their properties.
 */
void    CreateThermalReliefPadPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                       D_PAD&          aPad,
                                       int             aThermalGap,
                                       int             aCopperThickness,
                                       int             aMinThicknessValue,
                                       int             aCircleToSegmentsCount,
                                       double          aCorrectionFactor,
                                       double          aThermalRot )
{
    wxPoint corner, corner_end;
    wxPoint PadShapePos = aPad.ShapePos();      // Note: for pad having a shape offset,
                                                // the pad position is NOT the shape position
    wxSize  copper_thickness;

    int     dx = aPad.GetSize().x / 2;
    int     dy = aPad.GetSize().y / 2;

    double  delta = 3600.0 / aCircleToSegmentsCount; // rot angle in 0.1 degree

    /* Keep in account the polygon outline thickness
     * aThermalGap must be increased by aMinThicknessValue/2 because drawing external outline
     * with a thickness of aMinThicknessValue will reduce gap by aMinThicknessValue/2
     */
    aThermalGap += aMinThicknessValue / 2;

    /* Keep in account the polygon outline thickness
     * copper_thickness must be decreased by aMinThicknessValue because drawing outlines
     * with a thickness of aMinThicknessValue will increase real thickness by aMinThicknessValue
     */
    aCopperThickness -= aMinThicknessValue;

    if( aCopperThickness < 0 )
        aCopperThickness = 0;

    copper_thickness.x = std::min( dx, aCopperThickness );
    copper_thickness.y = std::min( dy, aCopperThickness );

    switch( aPad.GetShape() )
    {
    case PAD_SHAPE_CIRCLE:    // Add 4 similar holes
        {
            /* we create 4 copper holes and put them in position 1, 2, 3 and 4
             * here is the area of the rectangular pad + its thermal gap
             * the 4 copper holes remove the copper in order to create the thermal gap
             * 4 ------ 1
             * |        |
             * |        |
             * |        |
             * |        |
             * 3 ------ 2
             * holes 2, 3, 4 are the same as hole 1, rotated 90, 180, 270 deg
             */

            // Build the hole pattern, for the hole in the X >0, Y > 0 plane:
            // The pattern roughtly is a 90 deg arc pie
            std::vector <wxPoint> corners_buffer;

            // Radius of outer arcs of the shape corrected for arc approximation by lines
            int outer_radius = KiROUND( (dx + aThermalGap) * aCorrectionFactor );

            // Crosspoint of thermal spoke sides, the first point of polygon buffer
            corners_buffer.push_back( wxPoint( copper_thickness.x / 2, copper_thickness.y / 2 ) );

            // Add an intermediate point on spoke sides, to allow a > 90 deg angle between side
            // and first seg of arc approx
            corner.x = copper_thickness.x / 2;
            int y = outer_radius - (aThermalGap / 4);
            corner.y = KiROUND( sqrt( ( (double) y * y  - (double) corner.x * corner.x ) ) );

            if( aThermalRot != 0 )
                corners_buffer.push_back( corner );

            // calculate the starting point of the outter arc
            corner.x = copper_thickness.x / 2;

            corner.y = KiROUND( sqrt( ( (double) outer_radius * outer_radius ) -
                                      ( (double) corner.x * corner.x ) ) );
            RotatePoint( &corner, 90 ); // 9 degrees is the spoke fillet size

            // calculate the ending point of the outter arc
            corner_end.x = corner.y;
            corner_end.y = corner.x;

            // calculate intermediate points (y coordinate from corner.y to corner_end.y
            while( (corner.y > corner_end.y)  && (corner.x < corner_end.x) )
            {
                corners_buffer.push_back( corner );
                RotatePoint( &corner, delta );
            }

            corners_buffer.push_back( corner_end );

            /* add an intermediate point, to avoid angles < 90 deg between last arc approx line
             * and radius line
             */
            corner.x = corners_buffer[1].y;
            corner.y = corners_buffer[1].x;
            corners_buffer.push_back( corner );

            // Now, add the 4 holes ( each is the pattern, rotated by 0, 90, 180 and 270  deg
            // aThermalRot = 450 (45.0 degrees orientation) work fine.
            double angle_pad = aPad.GetOrientation();              // Pad orientation
            double th_angle  = aThermalRot;

            for( unsigned ihole = 0; ihole < 4; ihole++ )
            {
                aCornerBuffer.NewOutline();

                for( unsigned ii = 0; ii < corners_buffer.size(); ii++ )
                {
                    corner = corners_buffer[ii];
                    RotatePoint( &corner, th_angle + angle_pad );          // Rotate by segment angle and pad orientation
                    corner += PadShapePos;
                    aCornerBuffer.Append( corner.x, corner.y );
                }

                th_angle += 900;       // Note: th_angle in in 0.1 deg.
            }
        }
        break;

    case PAD_SHAPE_OVAL:
        {
            // Oval pad support along the lines of round and rectangular pads
            std::vector <wxPoint> corners_buffer;               // Polygon buffer as vector

            int     dx = (aPad.GetSize().x / 2) + aThermalGap;     // Cutout radius x
            int     dy = (aPad.GetSize().y / 2) + aThermalGap;     // Cutout radius y

            wxPoint shape_offset;

            // We want to calculate an oval shape with dx > dy.
            // if this is not the case, exchange dx and dy, and rotate the shape 90 deg.
            int supp_angle = 0;

            if( dx < dy )
            {
                std::swap( dx, dy );
                supp_angle = 900;
                std::swap( copper_thickness.x, copper_thickness.y );
            }

            int deltasize = dx - dy;        // = distance between shape position and the 2 demi-circle ends centre
            // here we have dx > dy
            // Radius of outer arcs of the shape:
            int outer_radius = dy;     // The radius of the outer arc is radius end + aThermalGap

            // Some coordinate fiddling, depending on the shape offset direction
            shape_offset = wxPoint( deltasize, 0 );

            // Crosspoint of thermal spoke sides, the first point of polygon buffer
            corner.x = copper_thickness.x / 2;
            corner.y = copper_thickness.y / 2;
            corners_buffer.push_back( corner );

            // Arc start point calculation, the intersecting point of cutout arc and thermal spoke edge
            // If copper thickness is more than shape offset, we need to calculate arc intercept point.
            if( copper_thickness.x > deltasize )
            {
                corner.x = copper_thickness.x / 2;
                corner.y = KiROUND( sqrt( ( (double) outer_radius * outer_radius ) -
                                        ( (double) ( corner.x - delta ) * ( corner.x - deltasize ) ) ) );
                corner.x -= deltasize;

                /* creates an intermediate point, to have a > 90 deg angle
                 * between the side and the first segment of arc approximation
                 */
                wxPoint intpoint = corner;
                intpoint.y -= aThermalGap / 4;
                corners_buffer.push_back( intpoint + shape_offset );
                RotatePoint( &corner, 90 ); // 9 degrees of thermal fillet
            }
            else
            {
                corner.x = copper_thickness.x / 2;
                corner.y = outer_radius;
                corners_buffer.push_back( corner );
            }

            // Add an intermediate point on spoke sides, to allow a > 90 deg angle between side
            // and first seg of arc approx
            wxPoint last_corner;
            last_corner.y = copper_thickness.y / 2;
            int     px = outer_radius - (aThermalGap / 4);
            last_corner.x =
                KiROUND( sqrt( ( ( (double) px * px ) - (double) last_corner.y * last_corner.y ) ) );

            // Arc stop point calculation, the intersecting point of cutout arc and thermal spoke edge
            corner_end.y = copper_thickness.y / 2;
            corner_end.x =
                KiROUND( sqrt( ( (double) outer_radius *
                             outer_radius ) - ( (double) corner_end.y * corner_end.y ) ) );
            RotatePoint( &corner_end, -90 ); // 9 degrees of thermal fillet

            // calculate intermediate arc points till limit is reached
            while( (corner.y > corner_end.y)  && (corner.x < corner_end.x) )
            {
                corners_buffer.push_back( corner + shape_offset );
                RotatePoint( &corner, delta );
            }

            //corners_buffer.push_back(corner + shape_offset);      // TODO: about one mil geometry error forms somewhere.
            corners_buffer.push_back( corner_end + shape_offset );
            corners_buffer.push_back( last_corner + shape_offset );         // Enabling the line above shows intersection point.

            /* Create 2 holes, rotated by pad rotation.
             */
            double angle = aPad.GetOrientation() + supp_angle;

            for( int irect = 0; irect < 2; irect++ )
            {
                aCornerBuffer.NewOutline();
                for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
                {
                    wxPoint cpos = corners_buffer[ic];
                    RotatePoint( &cpos, angle );
                    cpos += PadShapePos;
                    aCornerBuffer.Append( cpos.x, cpos.y );
                }

                angle = AddAngles( angle, 1800 ); // this is calculate hole 3
            }

            // Create holes, that are the mirrored from the previous holes
            for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
            {
                wxPoint swap = corners_buffer[ic];
                swap.x = -swap.x;
                corners_buffer[ic] = swap;
            }

            // Now add corner 4 and 2 (2 is the corner 4 rotated by 180 deg
            angle = aPad.GetOrientation() + supp_angle;

            for( int irect = 0; irect < 2; irect++ )
            {
                aCornerBuffer.NewOutline();

                for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
                {
                    wxPoint cpos = corners_buffer[ic];
                    RotatePoint( &cpos, angle );
                    cpos += PadShapePos;
                    aCornerBuffer.Append( cpos.x, cpos.y );
                }

                angle = AddAngles( angle, 1800 );
            }
        }
        break;

    case PAD_SHAPE_RECT:       // draw 4 Holes
        {
            /* we create 4 copper holes and put them in position 1, 2, 3 and 4
             * here is the area of the rectangular pad + its thermal gap
             * the 4 copper holes remove the copper in order to create the thermal gap
             * 4 ------ 1
             * |        |
             * |        |
             * |        |
             * |        |
             * 3 ------ 2
             * hole 3 is the same as hole 1, rotated 180 deg
             * hole 4 is the same as hole 2, rotated 180 deg and is the same as hole 1, mirrored
             */

            // First, create a rectangular hole for position 1 :
            // 2 ------- 3
            //  |        |
            //  |        |
            //  |        |
            // 1  -------4

            // Modified rectangles with one corner rounded. TODO: merging with oval thermals
            // and possibly round too.

            std::vector <wxPoint> corners_buffer;               // Polygon buffer as vector

            int dx = (aPad.GetSize().x / 2) + aThermalGap;         // Cutout radius x
            int dy = (aPad.GetSize().y / 2) + aThermalGap;         // Cutout radius y

            // The first point of polygon buffer is left lower corner, second the crosspoint of
            // thermal spoke sides, the third is upper right corner and the rest are rounding
            // vertices going anticlockwise. Note the inveted Y-axis in CG.
            corners_buffer.push_back( wxPoint( -dx, -(aThermalGap / 4 + copper_thickness.y / 2) ) );    // Adds small miters to zone
            corners_buffer.push_back( wxPoint( -(dx - aThermalGap / 4), -copper_thickness.y / 2 ) );    // fill and spoke corner
            corners_buffer.push_back( wxPoint( -copper_thickness.x / 2, -copper_thickness.y / 2 ) );
            corners_buffer.push_back( wxPoint( -copper_thickness.x / 2, -(dy - aThermalGap / 4) ) );
            corners_buffer.push_back( wxPoint( -(aThermalGap / 4 + copper_thickness.x / 2), -dy ) );

            double angle = aPad.GetOrientation();
            int rounding_radius = KiROUND( aThermalGap * aCorrectionFactor );   // Corner rounding radius

            for( int i = 0; i < aCircleToSegmentsCount / 4 + 1; i++ )
            {
                wxPoint corner_position = wxPoint( 0, -rounding_radius );

                // Start at half increment offset
                RotatePoint( &corner_position, 1800.0 / aCircleToSegmentsCount );
                double angle_pg = i * delta;

                RotatePoint( &corner_position, angle_pg );          // Rounding vector rotation
                corner_position -= aPad.GetSize() / 2;              // Rounding vector + Pad corner offset

                corners_buffer.push_back( wxPoint( corner_position.x, corner_position.y ) );
            }

            for( int irect = 0; irect < 2; irect++ )
            {
                aCornerBuffer.NewOutline();

                for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
                {
                    wxPoint cpos = corners_buffer[ic];
                    RotatePoint( &cpos, angle );            // Rotate according to module orientation
                    cpos += PadShapePos;                    // Shift origin to position
                    aCornerBuffer.Append( cpos.x, cpos.y );
                }

                angle = AddAngles( angle, 1800 );       // this is calculate hole 3
            }

            // Create holes, that are the mirrored from the previous holes
            for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
            {
                wxPoint swap = corners_buffer[ic];
                swap.x = -swap.x;
                corners_buffer[ic] = swap;
            }

            // Now add corner 4 and 2 (2 is the corner 4 rotated by 180 deg
            for( int irect = 0; irect < 2; irect++ )
            {
                aCornerBuffer.NewOutline();

                for( unsigned ic = 0; ic < corners_buffer.size(); ic++ )
                {
                    wxPoint cpos = corners_buffer[ic];
                    RotatePoint( &cpos, angle );
                    cpos += PadShapePos;
                    aCornerBuffer.Append( cpos.x, cpos.y );
                }

                angle = AddAngles( angle, 1800 );
            }
        }
        break;

    case PAD_SHAPE_TRAPEZOID:
        {
        SHAPE_POLY_SET antipad;       // The full antipad area

        // We need a length to build the stubs of the thermal reliefs
        // the value is not very important. The pad bounding box gives a reasonable value
        EDA_RECT bbox = aPad.GetBoundingBox();
        int stub_len = std::max( bbox.GetWidth(), bbox.GetHeight() );

        aPad.TransformShapeWithClearanceToPolygon( antipad, aThermalGap,
                    aCircleToSegmentsCount, aCorrectionFactor );

        SHAPE_POLY_SET stub;          // A basic stub ( a rectangle)
        SHAPE_POLY_SET stubs;        // the full stubs shape


        // We now substract the stubs (connections to the copper zone)
        //ClipperLib::Clipper clip_engine;
        // Prepare a clipping transform
        //clip_engine.AddPath( antipad, ClipperLib::ptSubject, true );

        // Create stubs and add them to clipper engine
        wxPoint stubBuffer[4];
        stubBuffer[0].x = stub_len;
        stubBuffer[0].y = copper_thickness.y/2;
        stubBuffer[1] = stubBuffer[0];
        stubBuffer[1].y = -copper_thickness.y/2;
        stubBuffer[2] = stubBuffer[1];
        stubBuffer[2].x = -stub_len;
        stubBuffer[3] = stubBuffer[2];
        stubBuffer[3].y = copper_thickness.y/2;

        stub.NewOutline();

        for( unsigned ii = 0; ii < DIM( stubBuffer ); ii++ )
        {
            wxPoint cpos = stubBuffer[ii];
            RotatePoint( &cpos, aPad.GetOrientation() );
            cpos += PadShapePos;
            stub.Append( cpos.x, cpos.y );
        }

        stubs.Append( stub );

        stubBuffer[0].y = stub_len;
        stubBuffer[0].x = copper_thickness.x/2;
        stubBuffer[1] = stubBuffer[0];
        stubBuffer[1].x = -copper_thickness.x/2;
        stubBuffer[2] = stubBuffer[1];
        stubBuffer[2].y = -stub_len;
        stubBuffer[3] = stubBuffer[2];
        stubBuffer[3].x = copper_thickness.x/2;

        stub.RemoveAllContours();
        stub.NewOutline();

        for( unsigned ii = 0; ii < DIM( stubBuffer ); ii++ )
        {
            wxPoint cpos = stubBuffer[ii];
            RotatePoint( &cpos, aPad.GetOrientation() );
            cpos += PadShapePos;
            stub.Append( cpos.x, cpos.y );
        }

        stubs.Append( stub );
        stubs.Simplify();

        antipad.BooleanSubtract( stubs );
        aCornerBuffer.Append( antipad );

        break;
        }

    default:
        ;
    }
}
