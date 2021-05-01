/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  create_graphic_brd_items.cpp
 * @brief This file implements the creation of 2D graphic primitives of pcb items:
 *  pads, tracks, drawsegments, texts....
 * It is based on the function found in the files:
 *  board_items_to_polygon_shape_transform.cpp
 */

#include "../3d_rendering/3d_render_raytracing/shapes2D/ring_2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/filled_circle_2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/round_segment_2d.h"
#include "../3d_rendering/3d_render_raytracing/shapes2D/triangle_2d.h"
#include <board_adapter.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_text.h>
#include <fp_shape.h>
#include <zone.h>
#include <kicad_string.h>
#include <fp_text.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <geometry/shape_segment.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_simple.h>
#include <gr_text.h>
#include <utility>
#include <vector>


// These variables are parameters used in addTextSegmToContainer.
// But addTextSegmToContainer is a call-back function,
// so we cannot send them as arguments.
static int s_textWidth;
static CONTAINER_2D_BASE* s_dstcontainer = nullptr;
static float s_biuTo3Dunits;
static const BOARD_ITEM* s_boardItem = nullptr;


// This is a call back function, used by GRText to draw the 3D text shape:
void addTextSegmToContainer( int x0, int y0, int xf, int yf, void* aData )
{
    const SFVEC2F start3DU( x0 * s_biuTo3Dunits, -y0 * s_biuTo3Dunits );
    const SFVEC2F end3DU  ( xf * s_biuTo3Dunits, -yf * s_biuTo3Dunits );

    if( Is_segment_a_circle( start3DU, end3DU ) )
        s_dstcontainer->Add( new FILLED_CIRCLE_2D( start3DU, ( s_textWidth / 2 ) * s_biuTo3Dunits,
                                                   *s_boardItem) );
    else
        s_dstcontainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, s_textWidth * s_biuTo3Dunits,
                                                   *s_boardItem ) );
}


// Based on
// void PCB_TEXT::TransformTextShapeWithClearanceToPolygon
// board_items_to_polygon_shape_transform.cpp
void BOARD_ADAPTER::addShapeWithClearance( const PCB_TEXT* aText, CONTAINER_2D_BASE* aDstContainer,
                                           PCB_LAYER_ID aLayerId, int aClearanceValue )
{
    wxSize size = aText->GetTextSize();

    if( aText->IsMirrored() )
        size.x = -size.x;

    s_boardItem    = (const BOARD_ITEM *) &aText;
    s_dstcontainer = aDstContainer;
    s_textWidth    = aText->GetEffectiveTextPenWidth() + ( 2 * aClearanceValue );
    s_biuTo3Dunits = m_biuTo3Dunits;

    // not actually used, but needed by GRText
    const COLOR4D dummy_color = COLOR4D::BLACK;
    bool          forceBold = true;
    int           penWidth = 0;         // force max width for bold

    if( aText->IsMultilineAllowed() )
    {
        wxArrayString strings_list;
        wxStringSplit( aText->GetShownText(), strings_list, '\n' );
        std::vector<wxPoint> positions;
        positions.reserve( strings_list.Count() );
        aText->GetLinePositions( positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ++ii )
        {
            wxString txt = strings_list.Item( ii );

            GRText( nullptr, positions[ii], dummy_color, txt, aText->GetTextAngle(), size,
                    aText->GetHorizJustify(), aText->GetVertJustify(), penWidth,
                    aText->IsItalic(), forceBold, addTextSegmToContainer );
        }
    }
    else
    {
        GRText( nullptr, aText->GetTextPos(), dummy_color, aText->GetShownText(),
                aText->GetTextAngle(), size, aText->GetHorizJustify(), aText->GetVertJustify(),
                penWidth, aText->IsItalic(), forceBold, addTextSegmToContainer );
    }
}


void BOARD_ADAPTER::addShapeWithClearance( const DIMENSION_BASE* aDimension,
                                           CONTAINER_2D_BASE* aDstContainer,
                                           PCB_LAYER_ID aLayerId, int aClearanceValue )
{
    addShapeWithClearance( &aDimension->Text(), aDstContainer, aLayerId, aClearanceValue );

    const int linewidth = aDimension->GetLineThickness() + ( 2 * aClearanceValue );

    for( const std::shared_ptr<SHAPE>& shape : aDimension->GetShapes() )
    {
        switch( shape->Type() )
        {
        case SH_SEGMENT:
        {
            const SEG& seg = static_cast<const SHAPE_SEGMENT*>( shape.get() )->GetSeg();

            const SFVEC2F start3DU( seg.A.x * m_biuTo3Dunits, -seg.A.y * m_biuTo3Dunits );

            const SFVEC2F end3DU( seg.B.x * m_biuTo3Dunits, -seg.B.y * m_biuTo3Dunits );

            aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, linewidth * m_biuTo3Dunits,
                                                      *aDimension ) );
            break;
        }

        case SH_CIRCLE:
        {
            int radius = static_cast<const SHAPE_CIRCLE*>( shape.get() )->GetRadius();
            int deltar = aDimension->GetLineThickness();

            SFVEC2F center( shape->Centre().x * m_biuTo3Dunits,
                            shape->Centre().y * m_biuTo3Dunits );

            aDstContainer->Add( new RING_2D( center, ( radius - deltar ) * m_biuTo3Dunits,
                                             ( radius + deltar ) * m_biuTo3Dunits, *aDimension ) );

            break;
        }

        default:
            break;
        }

    }

}


// Based on
// void FOOTPRINT::TransformFPShapesWithClearanceToPolygonSet
// board_items_to_polygon_shape_transform.cpp#L204
void BOARD_ADAPTER::addFootprintShapesWithClearance( const FOOTPRINT* aFootprint,
                                                     CONTAINER_2D_BASE* aDstContainer,
                                                     PCB_LAYER_ID aLayerId, int aInflateValue )
{
    std::vector<FP_TEXT*> texts;  // List of FP_TEXT to convert
    FP_SHAPE* outline;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        switch( item->Type() )
        {
        case PCB_FP_TEXT_T:
        {
            FP_TEXT* text = static_cast<FP_TEXT*>( item );

            if( text->GetLayer() == aLayerId && text->IsVisible() )
                texts.push_back( text );
        }
        break;


        case PCB_FP_SHAPE_T:
        {
            outline = (FP_SHAPE*) item;

            if( outline->GetLayer() != aLayerId )
                break;

            addShapeWithClearance( (const PCB_SHAPE*) outline, aDstContainer, aLayerId, 0 );
        }
        break;

        default:
            break;
        }
    }

    // Convert texts for footprints
    if( aFootprint->Reference().GetLayer() == aLayerId && aFootprint->Reference().IsVisible() )
        texts.push_back( &aFootprint->Reference() );

    if( aFootprint->Value().GetLayer() == aLayerId && aFootprint->Value().IsVisible() )
        texts.push_back( &aFootprint->Value() );

    s_boardItem    = (const BOARD_ITEM *)&aFootprint->Value();
    s_dstcontainer = aDstContainer;
    s_biuTo3Dunits = m_biuTo3Dunits;

    for( FP_TEXT* text : texts )
    {
        s_textWidth = text->GetEffectiveTextPenWidth() + ( 2 * aInflateValue );
        wxSize size = text->GetTextSize();
        bool   forceBold = true;
        int    penWidth = 0;        // force max width for bold

        if( text->IsMirrored() )
            size.x = -size.x;

        GRText( nullptr, text->GetTextPos(), BLACK, text->GetShownText(), text->GetDrawRotation(),
                size, text->GetHorizJustify(), text->GetVertJustify(), penWidth, text->IsItalic(),
                forceBold, addTextSegmToContainer );
    }
}


void BOARD_ADAPTER::createTrack( const TRACK* aTrack, CONTAINER_2D_BASE* aDstContainer,
                                 int aClearanceValue )
{
    SFVEC2F start3DU( aTrack->GetStart().x * m_biuTo3Dunits,
                      -aTrack->GetStart().y * m_biuTo3Dunits ); // y coord is inverted

    switch( aTrack->Type() )
    {
    case PCB_VIA_T:
    {
        const float radius = ( ( aTrack->GetWidth() / 2 ) + aClearanceValue ) * m_biuTo3Dunits;
        aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, radius, *aTrack ) );
        break;
    }

    case PCB_ARC_T:
    {
        const ARC* arc = static_cast<const ARC*>( aTrack );
        VECTOR2D center( arc->GetCenter() );
        double arc_angle = arc->GetAngle();
        double radius = arc->GetRadius();
        int arcsegcount = GetArcToSegmentCount( radius, Millimeter2iu( 0.005), arc_angle/10 );
        int circlesegcount;

        // We need a circle to segment count. However, the arc angle can be small, and the
        // radius very big. so we calculate a reasonable value for circlesegcount.
        if( arcsegcount <= 1 )  // The arc will be approximated by a segment
            circlesegcount = 1;
        else
        {
            double cnt = arcsegcount * 3600/std::abs( arc_angle );

#define SEG_CNT_MAX 128
            if( cnt < SEG_CNT_MAX )
            {
                circlesegcount = (int)cnt;

                if( circlesegcount == 0 )
                    circlesegcount = 1;
            }
            else
            {
                circlesegcount = SEG_CNT_MAX;
            }
        }

        transformArcToSegments( wxPoint( center.x, center.y ), arc->GetStart(),
                                arc_angle, circlesegcount,
                                arc->GetWidth() + 2 * aClearanceValue, aDstContainer, *arc );
        break;
    }

    case PCB_TRACE_T:    // Track is a usual straight segment
    {
        SFVEC2F end3DU( aTrack->GetEnd().x * m_biuTo3Dunits, -aTrack->GetEnd().y * m_biuTo3Dunits );

        // Cannot add segments that have the same start and end point
        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            const float radius = ((aTrack->GetWidth() / 2) + aClearanceValue) * m_biuTo3Dunits;

            aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, radius, *aTrack ) );
        }
        else
        {
            const float width = (aTrack->GetWidth() + 2 * aClearanceValue ) * m_biuTo3Dunits;

            aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, width, *aTrack ) );
        }

        break;
    }

    default:
        break;
    }
}


void BOARD_ADAPTER::createPadWithClearance( const PAD* aPad, CONTAINER_2D_BASE* aDstContainer,
                                               PCB_LAYER_ID aLayer, wxSize aClearanceValue ) const
{
    SHAPE_POLY_SET poly;

    // Our shape-based builder can't handle negative or differing x:y clearance values (the
    // former are common for solder paste while the later get generated when a relative paste
    // margin is used with an oblong pad).  So we apply this huge hack and fake a larger pad to
    // run the general-purpose polygon builder on.
    // Of course being a hack it falls down when dealing with custom shape pads (where the size
    // is only the size of the anchor), so for those we punt and just use aClearanceValue.x.

    if( ( aClearanceValue.x < 0 || aClearanceValue.x != aClearanceValue.y )
            && aPad->GetShape() != PAD_SHAPE::CUSTOM )
    {
        PAD dummy( *aPad );
        dummy.SetSize( aPad->GetSize() + aClearanceValue + aClearanceValue );
        dummy.TransformShapeWithClearanceToPolygon( poly, aLayer, 0, ARC_HIGH_DEF, ERROR_INSIDE );
        aClearanceValue = { 0, 0 };
    }
    else
    {
        auto padShapes = std::static_pointer_cast<SHAPE_COMPOUND>( aPad->GetEffectiveShape() );

        for( const SHAPE* shape : padShapes->Shapes() )
        {
            switch( shape->Type() )
            {
            case SH_SEGMENT:
            {
                const SHAPE_SEGMENT* seg = (SHAPE_SEGMENT*) shape;
                const SFVEC2F        start3DU(  seg->GetSeg().A.x * m_biuTo3Dunits,
                                               -seg->GetSeg().A.y * m_biuTo3Dunits );
                const SFVEC2F        end3DU  (  seg->GetSeg().B.x * m_biuTo3Dunits,
                                               -seg->GetSeg().B.y * m_biuTo3Dunits );
                const int            width = seg->GetWidth() + aClearanceValue.x * 2;

                 // Cannot add segments that have the same start and end point
                if( Is_segment_a_circle( start3DU, end3DU ) )
                {
                    aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU,
                                                             ( width / 2) * m_biuTo3Dunits,
                                                             *aPad ) );
                }
                else
                {
                    aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU,
                                                              width * m_biuTo3Dunits,
                                                              *aPad ) );
                }
            }
                break;

            case SH_CIRCLE:
            {
                const SHAPE_CIRCLE* circle = (SHAPE_CIRCLE*) shape;
                const int           radius = circle->GetRadius() + aClearanceValue.x;
                const SFVEC2F       center(  circle->GetCenter().x * m_biuTo3Dunits,
                                             -circle->GetCenter().y * m_biuTo3Dunits );

                aDstContainer->Add( new FILLED_CIRCLE_2D( center, radius * m_biuTo3Dunits,
                                                          *aPad ) );
            }
                break;

            case SH_RECT:
            {
                SHAPE_RECT* rect = (SHAPE_RECT*) shape;

                poly.NewOutline();
                poly.Append( rect->GetPosition() );
                poly.Append( rect->GetPosition().x + rect->GetSize().x, rect->GetPosition().y );
                poly.Append( rect->GetPosition() + rect->GetSize() );
                poly.Append( rect->GetPosition().x, rect->GetPosition().y + rect->GetSize().y );
            }
                break;

            case SH_SIMPLE:
                poly.AddOutline( static_cast<const SHAPE_SIMPLE*>( shape )->Vertices() );
                break;

            case SH_POLY_SET:
                poly = *(SHAPE_POLY_SET*) shape;
                break;

            case SH_ARC:
            {
                SHAPE_ARC* arc = (SHAPE_ARC*) shape;
                SHAPE_LINE_CHAIN l = arc->ConvertToPolyline();

                for( int i = 0; i < l.SegmentCount(); i++ )
                {
                    SHAPE_SEGMENT seg( l.Segment( i ).A, l.Segment( i ).B, arc->GetWidth() );
                    const SFVEC2F start3DU(  seg.GetSeg().A.x * m_biuTo3Dunits,
                                             -seg.GetSeg().A.y * m_biuTo3Dunits );
                    const SFVEC2F end3DU( seg.GetSeg().B.x * m_biuTo3Dunits,
                                          -seg.GetSeg().B.y * m_biuTo3Dunits );
                    const int width = arc->GetWidth() + aClearanceValue.x * 2;

                     // Cannot add segments that have the same start and end point
                    if( Is_segment_a_circle( start3DU, end3DU ) )
                    {
                        aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU,
                                                                 ( width / 2) * m_biuTo3Dunits,
                                                                 *aPad ) );
                    }
                    else
                    {
                        aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU,
                                                                  width * m_biuTo3Dunits,
                                                                  *aPad ) );
                    }
                }
            }
                break;

            default:
                wxFAIL_MSG( "BOARD_ADAPTER::createPadWithClearance no implementation for "
                            + SHAPE_TYPE_asString( shape->Type() ) );
                break;
            }
        }
    }

    if( !poly.IsEmpty() )
    {
        if( aClearanceValue.x )
            poly.Inflate( aClearanceValue.x, 32 );

        // Add the PAD polygon
        ConvertPolygonToTriangles( poly, *aDstContainer, m_biuTo3Dunits, *aPad );
    }
}


OBJECT_2D* BOARD_ADAPTER::createPadWithDrill( const PAD* aPad, int aInflateValue )
{
    wxSize drillSize = aPad->GetDrillSize();

    if( !drillSize.x || !drillSize.y )
    {
        wxLogTrace( m_logTrace, wxT( "BOARD_ADAPTER::createPadWithDrill - found an invalid pad" ) );
        return nullptr;
    }

    if( drillSize.x == drillSize.y )    // usual round hole
    {
        const int radius = ( drillSize.x / 2 ) + aInflateValue;

        const SFVEC2F center(  aPad->GetPosition().x * m_biuTo3Dunits,
                              -aPad->GetPosition().y * m_biuTo3Dunits );

        return new FILLED_CIRCLE_2D( center, radius * m_biuTo3Dunits, *aPad );

    }
    else                                // Oblong hole
    {
        const SHAPE_SEGMENT* seg = aPad->GetEffectiveHoleShape();
        float width = seg->GetWidth() + aInflateValue * 2;

        SFVEC2F start3DU(  seg->GetSeg().A.x * m_biuTo3Dunits,
                          -seg->GetSeg().A.y * m_biuTo3Dunits );

        SFVEC2F end3DU (  seg->GetSeg().B.x * m_biuTo3Dunits,
                         -seg->GetSeg().B.y * m_biuTo3Dunits );

        return new ROUND_SEGMENT_2D( start3DU, end3DU, width * m_biuTo3Dunits, *aPad );
    }

    return nullptr;
}


void BOARD_ADAPTER::addPadsWithClearance( const FOOTPRINT* aFootprint,
                                          CONTAINER_2D_BASE* aDstContainer,
                                          PCB_LAYER_ID aLayerId, int aInflateValue,
                                          bool aSkipNPTHPadsWihNoCopper, bool aSkipPlatedPads,
                                          bool aSkipNonPlatedPads )
{
    for( PAD* pad : aFootprint->Pads() )
    {
        if( !pad->IsOnLayer( aLayerId ) )
            continue;

        // Skip pad annulus when not connected on this layer (if removing is enabled)
        if( !pad->FlashLayer( aLayerId ) && IsCopperLayer( aLayerId ) )
            continue;

        // NPTH pads are not drawn on layers if the
        // shape size and pos is the same as their hole:
        if( aSkipNPTHPadsWihNoCopper && ( pad->GetAttribute() == PAD_ATTRIB::NPTH ) )
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

        const bool isPlated = ( ( aLayerId == F_Cu ) && pad->FlashLayer( F_Mask ) ) ||
                              ( ( aLayerId == B_Cu ) && pad->FlashLayer( B_Mask ) );

        if( aSkipPlatedPads && isPlated )
            continue;

        if( aSkipNonPlatedPads && !isPlated )
            continue;

        wxSize margin( aInflateValue, aInflateValue );

        switch( aLayerId )
        {
        case F_Mask:
        case B_Mask:
            margin.x += pad->GetSolderMaskMargin();
            margin.y += pad->GetSolderMaskMargin();
            break;

        case F_Paste:
        case B_Paste:
            margin += pad->GetSolderPasteMargin();
            break;

        default:
            break;
        }

        createPadWithClearance( pad, aDstContainer, aLayerId, margin );
    }
}


// based on TransformArcToPolygon function from
// common/convert_basic_shapes_to_polygon.cpp
void BOARD_ADAPTER::transformArcToSegments( const wxPoint& aCentre, const wxPoint& aStart,
                                            double aArcAngle, int aCircleToSegmentsCount,
                                            int aWidth, CONTAINER_2D_BASE* aDstContainer,
                                            const BOARD_ITEM& aBoardItem )
{
    wxPoint arc_start, arc_end;
    int     delta = 3600 / aCircleToSegmentsCount;   // rotate angle in 0.1 degree

    arc_end = arc_start = aStart;

    if( aArcAngle != 3600 )
    {
        RotatePoint( &arc_end, aCentre, -aArcAngle );
    }

    if( aArcAngle < 0 )
    {
        std::swap( arc_start, arc_end );
        aArcAngle = -aArcAngle;
    }

    // Compute the ends of segments and creates poly
    wxPoint curr_end    = arc_start;
    wxPoint curr_start  = arc_start;

    for( int ii = delta; ii < aArcAngle; ii += delta )
    {
        curr_end = arc_start;
        RotatePoint( &curr_end, aCentre, -ii );

        const SFVEC2F start3DU( curr_start.x * m_biuTo3Dunits, -curr_start.y * m_biuTo3Dunits );
        const SFVEC2F end3DU  ( curr_end.x   * m_biuTo3Dunits, -curr_end.y   * m_biuTo3Dunits );

        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, ( aWidth / 2 ) * m_biuTo3Dunits,
                                                      aBoardItem ) );
        }
        else
        {
            aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, aWidth * m_biuTo3Dunits,
                                                      aBoardItem ) );
        }

        curr_start = curr_end;
    }

    if( curr_end != arc_end )
    {
        const SFVEC2F start3DU( curr_end.x * m_biuTo3Dunits, -curr_end.y * m_biuTo3Dunits );
        const SFVEC2F end3DU  ( arc_end.x  * m_biuTo3Dunits, -arc_end.y  * m_biuTo3Dunits );

        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, ( aWidth / 2 ) * m_biuTo3Dunits,
                                                      aBoardItem ) );
        }
        else
        {
            aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, aWidth * m_biuTo3Dunits,
                                                      aBoardItem ) );
        }
    }
}


// Based on
// TransformShapeWithClearanceToPolygon
// board_items_to_polygon_shape_transform.cpp#L431
void BOARD_ADAPTER::addShapeWithClearance( const PCB_SHAPE* aShape,
                                           CONTAINER_2D_BASE* aDstContainer, PCB_LAYER_ID aLayerId,
                                           int aClearanceValue )
{
    // The full width of the lines to create
    // The extra 1 protects the inner/outer radius values from degeneracy
    const int linewidth = aShape->GetWidth() + ( 2 * aClearanceValue ) + 1;

    switch( aShape->GetShape() )
    {
    case PCB_SHAPE_TYPE::CIRCLE:
    {
        const SFVEC2F center3DU( aShape->GetCenter().x * m_biuTo3Dunits,
                                 -aShape->GetCenter().y * m_biuTo3Dunits );

        float inner_radius = ( aShape->GetRadius() - linewidth / 2 ) * m_biuTo3Dunits;
        float outer_radius = ( aShape->GetRadius() + linewidth / 2 ) * m_biuTo3Dunits;

        if( inner_radius < 0 )
            inner_radius = 0;

        if( aShape->IsFilled() )
            aDstContainer->Add( new FILLED_CIRCLE_2D( center3DU, outer_radius, *aShape ) );
        else
            aDstContainer->Add( new RING_2D( center3DU, inner_radius, outer_radius, *aShape ) );
    }
        break;

    case PCB_SHAPE_TYPE::RECT:
        if( aShape->IsFilled() )
        {
            SHAPE_POLY_SET polyList;

            aShape->TransformShapeWithClearanceToPolygon( polyList, aLayerId, 0,
                                                          ARC_HIGH_DEF, ERROR_INSIDE );

            polyList.Simplify( SHAPE_POLY_SET::PM_FAST );

            ConvertPolygonToTriangles( polyList, *aDstContainer, m_biuTo3Dunits, *aShape );
        }
        else
        {
            std::vector<wxPoint> pts = aShape->GetRectCorners();

            const SFVEC2F topLeft3DU(  pts[0].x * m_biuTo3Dunits, -pts[0].y * m_biuTo3Dunits );
            const SFVEC2F topRight3DU( pts[1].x * m_biuTo3Dunits, -pts[1].y * m_biuTo3Dunits );
            const SFVEC2F botRight3DU( pts[2].x * m_biuTo3Dunits, -pts[2].y * m_biuTo3Dunits );
            const SFVEC2F botLeft3DU(  pts[3].x * m_biuTo3Dunits, -pts[3].y * m_biuTo3Dunits );

            aDstContainer->Add( new ROUND_SEGMENT_2D( topLeft3DU, topRight3DU,
                                                      linewidth * m_biuTo3Dunits, *aShape ) );
            aDstContainer->Add( new ROUND_SEGMENT_2D( topRight3DU, botRight3DU,
                                                      linewidth * m_biuTo3Dunits, *aShape ) );
            aDstContainer->Add( new ROUND_SEGMENT_2D( botRight3DU, botLeft3DU,
                                                      linewidth * m_biuTo3Dunits, *aShape ) );
            aDstContainer->Add( new ROUND_SEGMENT_2D( botLeft3DU, topLeft3DU,
                                                      linewidth * m_biuTo3Dunits, *aShape ) );
        }
        break;

    case PCB_SHAPE_TYPE::ARC:
    {
        unsigned int segCount = GetCircleSegmentCount( aShape->GetBoundingBox().GetSizeMax() );

        transformArcToSegments( aShape->GetCenter(), aShape->GetArcStart(), aShape->GetAngle(),
                                segCount, linewidth, aDstContainer, *aShape );
    }
    break;

    case PCB_SHAPE_TYPE::SEGMENT:
    {
        const SFVEC2F start3DU( aShape->GetStart().x * m_biuTo3Dunits,
                                -aShape->GetStart().y * m_biuTo3Dunits );

        const SFVEC2F end3DU  ( aShape->GetEnd().x * m_biuTo3Dunits,
                                -aShape->GetEnd().y * m_biuTo3Dunits );

        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, ( linewidth / 2 ) * m_biuTo3Dunits,
                                                     *aShape ) );
        }
        else
        {
            aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, linewidth * m_biuTo3Dunits,
                                                      *aShape ) );
        }
    }
    break;

    case PCB_SHAPE_TYPE::CURVE:
    case PCB_SHAPE_TYPE::POLYGON:
    {
        SHAPE_POLY_SET polyList;

        aShape->TransformShapeWithClearanceToPolygon( polyList, aLayerId, 0,
                                                      ARC_HIGH_DEF, ERROR_INSIDE );

        polyList.Simplify( SHAPE_POLY_SET::PM_FAST );

        if( polyList.IsEmpty() ) // Just for caution
            break;

        ConvertPolygonToTriangles( polyList, *aDstContainer, m_biuTo3Dunits, *aShape );
    }
    break;

    default:
        wxFAIL_MSG( "BOARD_ADAPTER::addShapeWithClearance no implementation for "
                    + PCB_SHAPE_TYPE_T_asString( aShape->GetShape() ) );
        break;
    }
}


// Based on
// TransformSolidAreasShapesToPolygonSet
// board_items_to_polygon_shape_transform.cpp
void BOARD_ADAPTER::addSolidAreasShapes( const ZONE* aZoneContainer,
                                         CONTAINER_2D_BASE* aDstContainer, PCB_LAYER_ID aLayerId )
{
    // Copy the polys list because we have to simplify it
    SHAPE_POLY_SET polyList = SHAPE_POLY_SET( aZoneContainer->GetFilledPolysList( aLayerId ) );

    // This convert the poly in outline and holes
    ConvertPolygonToTriangles( polyList, *aDstContainer, m_biuTo3Dunits, *aZoneContainer );

    // add filled areas outlines, which are drawn with thick lines segments
    // but only if filled polygons outlines have thickness
    if( !aZoneContainer->GetFilledPolysUseThickness() )
        return;

    float line_thickness = aZoneContainer->GetMinThickness() * m_biuTo3Dunits;

    for( int i = 0; i < polyList.OutlineCount(); ++i )
    {
        // Add outline
        const SHAPE_LINE_CHAIN& pathOutline = polyList.COutline( i );

        for( int j = 0; j < pathOutline.PointCount(); ++j )
        {
            const VECTOR2I& a = pathOutline.CPoint( j );
            const VECTOR2I& b = pathOutline.CPoint( j + 1 );

            SFVEC2F start3DU( a.x * m_biuTo3Dunits, -a.y * m_biuTo3Dunits );
            SFVEC2F end3DU  ( b.x * m_biuTo3Dunits, -b.y * m_biuTo3Dunits );

            if( Is_segment_a_circle( start3DU, end3DU ) )
            {
                float radius = line_thickness/2;

                if( radius > 0.0 )  // degenerated circles crash 3D viewer
                    aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, radius, *aZoneContainer ) );
            }
            else
            {
                aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, line_thickness,
                                                          *aZoneContainer ) );
            }
        }

        // Add holes (of the poly, ie: the open parts) for this outline
        for( int h = 0; h < polyList.HoleCount( i ); ++h )
        {
            const SHAPE_LINE_CHAIN& pathHole = polyList.CHole( i, h );

            for( int j = 0; j < pathHole.PointCount(); j++ )
            {
                const VECTOR2I& a = pathHole.CPoint( j );
                const VECTOR2I& b = pathHole.CPoint( j + 1 );

                SFVEC2F start3DU( a.x * m_biuTo3Dunits, -a.y * m_biuTo3Dunits );
                SFVEC2F end3DU  ( b.x * m_biuTo3Dunits, -b.y * m_biuTo3Dunits );

                if( Is_segment_a_circle( start3DU, end3DU ) )
                {
                    float radius = line_thickness/2;

                    if( radius > 0.0 )  // degenerated circles crash 3D viewer
                        aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, radius,
                                                                  *aZoneContainer ) );
                }
                else
                {
                    aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, line_thickness,
                                                              *aZoneContainer ) );
                }
            }
        }
    }
}


void BOARD_ADAPTER::buildPadOutlineAsSegments( const PAD* aPad, CONTAINER_2D_BASE* aDstContainer,
                                               int aWidth )
{
    if( aPad->GetShape() == PAD_SHAPE::CIRCLE )    // Draw a ring
    {
        const SFVEC2F center3DU( aPad->ShapePos().x * m_biuTo3Dunits,
                                 -aPad->ShapePos().y * m_biuTo3Dunits );

        const int radius = aPad->GetSize().x / 2;
        const float inner_radius = ( radius - aWidth / 2 ) * m_biuTo3Dunits;
        const float outer_radius = ( radius + aWidth / 2 ) * m_biuTo3Dunits;

        aDstContainer->Add( new RING_2D( center3DU, inner_radius, outer_radius, *aPad ) );

        return;
    }

    // For other shapes, add outlines as thick segments in polygon buffer
    const std::shared_ptr<SHAPE_POLY_SET>& corners = aPad->GetEffectivePolygon();
    const SHAPE_LINE_CHAIN&                path = corners->COutline( 0 );

    for( int j = 0; j < path.PointCount(); j++ )
    {
        const VECTOR2I& a = path.CPoint( j );
        const VECTOR2I& b = path.CPoint( j + 1 );

        SFVEC2F start3DU( a.x * m_biuTo3Dunits, -a.y * m_biuTo3Dunits );
        SFVEC2F end3DU  ( b.x * m_biuTo3Dunits, -b.y * m_biuTo3Dunits );

        if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, ( aWidth / 2 ) * m_biuTo3Dunits,
                                                      *aPad ) );
        }
        else
        {
            aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, aWidth * m_biuTo3Dunits,
                                                      *aPad ) );
        }
    }
}
