/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  create_3Dgraphic_brd_items.cpp
 * @brief This file implements the creation of 2D graphic primitives of pcb items:
 *  pads, tracks, drawsegments, texts....
 * It is based on the function found in the files:
 *  board_items_to_polygon_shape_transform.cpp
 */

#include "../3d_rendering/raytracing/shapes2D/ring_2d.h"
#include "../3d_rendering/raytracing/shapes2D/filled_circle_2d.h"
#include "../3d_rendering/raytracing/shapes2D/round_segment_2d.h"
#include "../3d_rendering/raytracing/shapes2D/triangle_2d.h"
#include <board_adapter.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <board_design_settings.h>
#include <pcb_painter.h>        // for PCB_RENDER_SETTINGS
#include <zone.h>
#include <convert_basic_shapes_to_polygon.h>
#include <trigo.h>
#include <geometry/shape_segment.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_simple.h>
#include <utility>
#include <vector>
#include <wx/log.h>
#include <macros.h>
#include <callback_gal.h>


#define TO_3DU( x ) ( ( x ) * m_biuTo3Dunits )

#define TO_SFVEC2F( vec ) SFVEC2F( TO_3DU( vec.x ), TO_3DU( -vec.y ) )


void BOARD_ADAPTER::addText( const EDA_TEXT* aText, CONTAINER_2D_BASE* aContainer,
                             const BOARD_ITEM* aOwner )
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    TEXT_ATTRIBUTES            attrs = aText->GetAttributes();
    float                      penWidth_3DU = TO_3DU( aText->GetEffectiveTextPenWidth() );
    KIFONT::FONT*              font = aText->GetFont();

    if( !font )
        font = KIFONT::FONT::GetFont( wxEmptyString, aText->IsBold(), aText->IsItalic() );

    if( aOwner && aOwner->IsKnockout() )
    {
        SHAPE_POLY_SET  finalPoly;
        const PCB_TEXT* pcbText = static_cast<const PCB_TEXT*>( aOwner );

        pcbText->TransformTextToPolySet( finalPoly, 0, m_board->GetDesignSettings().m_MaxError,
                                         ERROR_INSIDE );

        // Do not call finalPoly.Fracture() here: ConvertPolygonToTriangles() call it
        // if needed, and Fracture() called twice can create bad results and is useless
        ConvertPolygonToTriangles( finalPoly, *aContainer, m_biuTo3Dunits, *aOwner );
    }
    else
    {
        CALLBACK_GAL callback_gal( empty_opts,
                // Stroke callback
                [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
                {
                    const SFVEC2F pt1_3DU = TO_SFVEC2F( aPt1 );
                    const SFVEC2F pt2_3DU = TO_SFVEC2F( aPt2 );

                    if( penWidth_3DU == 0.0 )
                    {
                        // Don't attempt to render degenerate shapes
                    }
                    else if( Is_segment_a_circle( pt1_3DU, pt2_3DU ) )
                    {
                        // Cannot add segments that have the same start and end point
                        aContainer->Add( new FILLED_CIRCLE_2D( pt1_3DU, penWidth_3DU / 2,
                                                               *aOwner ) );
                    }
                    else
                    {
                        aContainer->Add( new ROUND_SEGMENT_2D( pt1_3DU, pt2_3DU, penWidth_3DU,
                                                               *aOwner ) );
                    }
                },
                // Triangulation callback
                [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
                {
                    aContainer->Add( new TRIANGLE_2D( TO_SFVEC2F( aPt1 ), TO_SFVEC2F( aPt2 ),
                                                      TO_SFVEC2F( aPt3 ), *aOwner ) );
                } );

        attrs.m_Angle = aText->GetDrawRotation();

        font->Draw( &callback_gal, aText->GetShownText( true ), aText->GetDrawPos(), attrs );
    }
}


void BOARD_ADAPTER::addShape( const PCB_DIMENSION_BASE* aDimension, CONTAINER_2D_BASE* aContainer,
                              const BOARD_ITEM* aOwner )
{
    addText( aDimension, aContainer, aDimension );

    const int linewidth = aDimension->GetLineThickness();

    for( const std::shared_ptr<SHAPE>& shape : aDimension->GetShapes() )
    {
        switch( shape->Type() )
        {
        case SH_SEGMENT:
        {
            const SEG& seg = static_cast<const SHAPE_SEGMENT*>( shape.get() )->GetSeg();

            aContainer->Add( new ROUND_SEGMENT_2D( TO_SFVEC2F( seg.A ), TO_SFVEC2F( seg.B ),
                                                   TO_3DU( linewidth ), *aOwner ) );
            break;
        }

        case SH_CIRCLE:
        {
            int radius = static_cast<const SHAPE_CIRCLE*>( shape.get() )->GetRadius();
            int delta = aDimension->GetLineThickness() / 2;

            aContainer->Add( new RING_2D( TO_SFVEC2F( shape->Centre() ), TO_3DU( radius - delta ),
                                          TO_3DU( radius + delta ), *aOwner ) );
            break;
        }

        default:
            break;
        }
    }
}


void BOARD_ADAPTER::addFootprintShapes( const FOOTPRINT* aFootprint, CONTAINER_2D_BASE* aContainer,
                                        PCB_LAYER_ID aLayerId )
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;

    for( PCB_FIELD* field : aFootprint->GetFields() )
    {
        if( field->GetLayer() == aLayerId && field->IsVisible() )
            addText( field, aContainer, field );
    }

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        switch( item->Type() )
        {
        case PCB_TEXT_T:
        {
            PCB_TEXT* text = static_cast<PCB_TEXT*>( item );

            if( text->GetLayer() == aLayerId && text->IsVisible() )
                addText( text, aContainer, text );

            break;
        }

        case PCB_TEXTBOX_T:
        {
            PCB_TEXTBOX* textbox = static_cast<PCB_TEXTBOX*>( item );

            if( textbox->GetLayer() == aLayerId )
            {
                addShape( textbox, aContainer, aFootprint );
                addText( textbox, aContainer, aFootprint );
            }

            break;
        }

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_LEADER_T:
        {
            PCB_DIMENSION_BASE* dimension = static_cast<PCB_DIMENSION_BASE*>( item );

            if( dimension->GetLayer() == aLayerId )
                addShape( dimension, aContainer, aFootprint );

            break;
        }

        case PCB_SHAPE_T:
        {
            PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

            if( shape->GetLayer() == aLayerId )
                addShape( shape, aContainer, aFootprint );

            break;
        }

        default:
            break;
        }
    }
}


void BOARD_ADAPTER::createViaWithMargin( const PCB_TRACK* aTrack, CONTAINER_2D_BASE* aDstContainer,
                                         int aMargin )
{
    SFVEC2F start3DU = TO_SFVEC2F( aTrack->GetStart() );
    SFVEC2F end3DU = TO_SFVEC2F( aTrack->GetEnd() );

    const float radius3DU = TO_3DU( ( aTrack->GetWidth() / 2 ) + aMargin );

    if( radius3DU > 0.0 )
        aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, radius3DU, *aTrack ) );
}


void BOARD_ADAPTER::createTrack( const PCB_TRACK* aTrack, CONTAINER_2D_BASE* aDstContainer )
{
    SFVEC2F start3DU = TO_SFVEC2F( aTrack->GetStart() );
    SFVEC2F end3DU = TO_SFVEC2F( aTrack->GetEnd() );

    switch( aTrack->Type() )
    {
    case PCB_VIA_T:
    {
        const float radius3DU = TO_3DU( aTrack->GetWidth() / 2 );

        if( radius3DU > 0.0 )
            aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, radius3DU, *aTrack ) );

        break;
    }

    case PCB_ARC_T:
    {
        const PCB_ARC* arc = static_cast<const PCB_ARC*>( aTrack );

        VECTOR2D  center( arc->GetCenter() );
        EDA_ANGLE arc_angle = arc->GetAngle();
        double    radius = arc->GetRadius();
        int       arcsegcount = GetArcToSegmentCount( radius, ARC_HIGH_DEF, arc_angle );
        int       circlesegcount;

        // Avoid arcs that cannot be drawn
        if( radius < std::numeric_limits<double>::min() || arc_angle.IsZero() )
            break;

        // We need a circle to segment count. However, the arc angle can be small, and the
        // radius very big. so we calculate a reasonable value for circlesegcount.
        if( arcsegcount <= 1 )  // The arc will be approximated by a segment
        {
            circlesegcount = 1;
        }
        else
        {
            circlesegcount = KiROUND( arcsegcount * 360.0 / std::abs( arc_angle.AsDegrees() ) );
            circlesegcount = alg::clamp( 1, circlesegcount, 128 );
        }

        transformArcToSegments( VECTOR2I( center.x, center.y ), arc->GetStart(), arc_angle,
                                circlesegcount, arc->GetWidth(), aDstContainer, *arc );
        break;
    }

    case PCB_TRACE_T:    // Track is a usual straight segment
    {
        if( aTrack->GetWidth() == 0 )
        {
            // Don't attempt to render degenerate shapes
        }
        else if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            // Cannot add segments that have the same start and end point
            aDstContainer->Add( new FILLED_CIRCLE_2D( start3DU, TO_3DU( aTrack->GetWidth() / 2 ),
                                                      *aTrack ) );
        }
        else
        {
            aDstContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, TO_3DU( aTrack->GetWidth() ),
                                                      *aTrack ) );
        }

        break;
    }

    default:
        break;
    }
}


void BOARD_ADAPTER::createPadWithMargin( const PAD* aPad, CONTAINER_2D_BASE* aContainer,
                                         PCB_LAYER_ID aLayer, const VECTOR2I& aMargin ) const
{
    SHAPE_POLY_SET poly;
    int            maxError = GetBoard()->GetDesignSettings().m_MaxError;
    VECTOR2I       clearance = aMargin;

    // Our shape-based builder can't handle negative or differing x:y clearance values (the
    // former are common for solder paste while the later get generated when a relative paste
    // margin is used with an oblong pad).  So we apply this huge hack and fake a larger pad to
    // run the general-purpose polygon builder on.
    // Of course being a hack it falls down when dealing with custom shape pads (where the size
    // is only the size of the anchor), so for those we punt and just use aMargin.x.

    if( ( clearance.x < 0 || clearance.x != clearance.y )
            && aPad->GetShape() != PAD_SHAPE::CUSTOM )
    {
        VECTOR2I dummySize = VECTOR2I( aPad->GetSize() ) + clearance + clearance;

        if( dummySize.x <= 0 || dummySize.y <= 0 )
            return;

        PAD dummy( *aPad );
        dummy.SetSize( VECTOR2I( dummySize.x, dummySize.y ) );
        dummy.TransformShapeToPolygon( poly, aLayer, 0, maxError, ERROR_INSIDE );
        clearance = { 0, 0 };
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
                const SHAPE_SEGMENT* seg = static_cast<const SHAPE_SEGMENT*>( shape );

                const SFVEC2F a3DU = TO_SFVEC2F( seg->GetSeg().A );
                const SFVEC2F b3DU = TO_SFVEC2F( seg->GetSeg().B );
                const double  width3DU = TO_3DU(  seg->GetWidth() + clearance.x * 2 );

                if( width3DU == 0.0 )
                {
                    // Don't attempt to render degenerate shapes
                }
                else if( Is_segment_a_circle( a3DU, b3DU ) )
                {
                    // Cannot add segments that have the same start and end point
                    aContainer->Add( new FILLED_CIRCLE_2D( a3DU, width3DU / 2, *aPad ) );
                }
                else
                {
                    aContainer->Add( new ROUND_SEGMENT_2D( a3DU, b3DU, width3DU, *aPad ) );
                }

                break;
            }

            case SH_CIRCLE:
            {
                const SHAPE_CIRCLE* circle = static_cast<const SHAPE_CIRCLE*>( shape );

                const double  radius3DU = TO_3DU( circle->GetRadius() + clearance.x );
                const SFVEC2F center3DU = TO_SFVEC2F( circle->GetCenter() );

                // Don't render zero radius circles
                if( radius3DU > 0.0 )
                    aContainer->Add( new FILLED_CIRCLE_2D( center3DU, radius3DU, *aPad ) );

                break;
            }

            case SH_RECT:
            {
                const SHAPE_RECT* rect = static_cast<const SHAPE_RECT*>( shape );

                poly.NewOutline();
                poly.Append( rect->GetPosition() );
                poly.Append( rect->GetPosition().x + rect->GetSize().x, rect->GetPosition().y );
                poly.Append( rect->GetPosition() + rect->GetSize() );
                poly.Append( rect->GetPosition().x, rect->GetPosition().y + rect->GetSize().y );
                break;
            }

            case SH_SIMPLE:
                poly.AddOutline( static_cast<const SHAPE_SIMPLE*>( shape )->Vertices() );
                break;

            case SH_POLY_SET:
                poly = *(SHAPE_POLY_SET*) shape;
                break;

            case SH_ARC:
            {
                const SHAPE_ARC* arc = static_cast<const SHAPE_ARC*>( shape );
                SHAPE_LINE_CHAIN l = arc->ConvertToPolyline( maxError );

                for( int i = 0; i < l.SegmentCount(); i++ )
                {
                    SHAPE_SEGMENT seg( l.Segment( i ).A, l.Segment( i ).B, arc->GetWidth() );
                    const SFVEC2F a3DU = TO_SFVEC2F( seg.GetSeg().A );
                    const SFVEC2F b3DU = TO_SFVEC2F( seg.GetSeg().B );
                    const double  width3DU = TO_3DU( arc->GetWidth() + clearance.x * 2 );

                    if( width3DU == 0.0 )
                    {
                        // Don't attempt to render degenerate shapes
                    }
                    else if( Is_segment_a_circle( a3DU, b3DU ) )
                    {
                        // Cannot add segments that have the same start and end point
                        aContainer->Add( new FILLED_CIRCLE_2D( a3DU, width3DU / 2, *aPad ) );
                    }
                    else
                    {
                        aContainer->Add( new ROUND_SEGMENT_2D( a3DU, b3DU, width3DU, *aPad ) );
                    }
                }

                break;
            }

            default:
                UNIMPLEMENTED_FOR( SHAPE_TYPE_asString( shape->Type() ) );
                break;
            }
        }
    }

    if( !poly.IsEmpty() )
    {
        if( clearance.x )
            poly.Inflate( clearance.x, SHAPE_POLY_SET::ROUND_ALL_CORNERS, maxError );

        // Add the PAD polygon
        ConvertPolygonToTriangles( poly, *aContainer, m_biuTo3Dunits, *aPad );
    }
}


OBJECT_2D* BOARD_ADAPTER::createPadWithDrill( const PAD* aPad, int aInflateValue )
{
    if( !aPad->HasHole() )
    {
        wxLogTrace( m_logTrace, wxT( "BOARD_ADAPTER::createPadWithDrill - found an invalid pad" ) );
        return nullptr;
    }

    std::shared_ptr<SHAPE_SEGMENT> slot = aPad->GetEffectiveHoleShape();

    if( slot->GetSeg().A == slot->GetSeg().B )
    {
        return new FILLED_CIRCLE_2D( TO_SFVEC2F( slot->GetSeg().A ),
                                     TO_3DU( slot->GetWidth() / 2 + aInflateValue ),
                                     *aPad );
    }
    else
    {
        return new ROUND_SEGMENT_2D( TO_SFVEC2F( slot->GetSeg().A ),
                                     TO_SFVEC2F( slot->GetSeg().B ),
                                     TO_3DU( slot->GetWidth() + aInflateValue * 2 ),
                                     *aPad );
    }
}


void BOARD_ADAPTER::addPads( const FOOTPRINT* aFootprint, CONTAINER_2D_BASE* aContainer,
                             PCB_LAYER_ID aLayerId, bool aSkipPlatedPads, bool aSkipNonPlatedPads )
{
    for( PAD* pad : aFootprint->Pads() )
    {
        if( !pad->IsOnLayer( aLayerId ) )
            continue;

        if( IsCopperLayer( aLayerId ) )
        {
            // Skip pad annulus when there isn't one (note: this is more discerning than
            // pad->IsOnLayer(), which doesn't check for NPTH pads with holes that consume
            // the entire pad).
            if( !pad->IsOnCopperLayer() )
                continue;

            // Skip pad annulus when not connected on this layer (if removing is enabled)
            if( !pad->FlashLayer( aLayerId ) )
                continue;
        }

        VECTOR2I margin( 0, 0 );

        switch( aLayerId )
        {
        case F_Cu:
            if( aSkipPlatedPads && pad->FlashLayer( F_Mask ) )
                continue;

            if( aSkipNonPlatedPads && !pad->FlashLayer( F_Mask ) )
                continue;

            break;

        case B_Cu:
            if( aSkipPlatedPads && pad->FlashLayer( B_Mask ) )
                continue;

            if( aSkipNonPlatedPads && !pad->FlashLayer( B_Mask ) )
                continue;

            break;

        case F_Mask:
        case B_Mask:
            margin.x += pad->GetSolderMaskExpansion();
            margin.y += pad->GetSolderMaskExpansion();
            break;

        case F_Paste:
        case B_Paste:
            margin += pad->GetSolderPasteMargin();
            break;

        default:
            break;
        }

        createPadWithMargin( pad, aContainer, aLayerId, margin );
    }
}


// based on TransformArcToPolygon function from
// common/convert_basic_shapes_to_polygon.cpp
void BOARD_ADAPTER::transformArcToSegments( const VECTOR2I& aCentre, const VECTOR2I& aStart,
                                            const EDA_ANGLE& aArcAngle, int aCircleToSegmentsCount,
                                            int aWidth, CONTAINER_2D_BASE* aContainer,
                                            const BOARD_ITEM& aOwner )
{
    // Don't attempt to render degenerate shapes
    if( aWidth == 0 )
        return;

    VECTOR2I  arc_start, arc_end;
    EDA_ANGLE arcAngle( aArcAngle );
    EDA_ANGLE delta = ANGLE_360 / aCircleToSegmentsCount;   // rotate angle

    arc_end = arc_start = aStart;

    if( arcAngle != ANGLE_360 )
        RotatePoint( arc_end, aCentre, -arcAngle );

    if( arcAngle < ANGLE_0 )
    {
        std::swap( arc_start, arc_end );
        arcAngle = -arcAngle;
    }

    // Compute the ends of segments and creates poly
    VECTOR2I curr_end = arc_start;
    VECTOR2I curr_start = arc_start;

    for( EDA_ANGLE ii = delta; ii < arcAngle; ii += delta )
    {
        curr_end = arc_start;
        RotatePoint( curr_end, aCentre, -ii );

        const SFVEC2F start3DU = TO_SFVEC2F( curr_start );
        const SFVEC2F end3DU = TO_SFVEC2F( curr_end );

        if( Is_segment_a_circle( start3DU, end3DU ) )
            aContainer->Add( new FILLED_CIRCLE_2D( start3DU, TO_3DU( aWidth / 2 ), aOwner ) );
        else
            aContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, TO_3DU( aWidth ), aOwner ) );

        curr_start = curr_end;
    }

    if( curr_end != arc_end )
    {
        const SFVEC2F start3DU = TO_SFVEC2F( curr_end );
        const SFVEC2F end3DU = TO_SFVEC2F( arc_end );

        if( Is_segment_a_circle( start3DU, end3DU ) )
            aContainer->Add( new FILLED_CIRCLE_2D( start3DU, TO_3DU( aWidth / 2 ), aOwner ) );
        else
            aContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, TO_3DU( aWidth ), aOwner ) );
    }
}


void BOARD_ADAPTER::addShape( const PCB_SHAPE* aShape, CONTAINER_2D_BASE* aContainer,
                              const BOARD_ITEM* aOwner )
{
    // The full width of the lines to create
    // The extra 1 protects the inner/outer radius values from degeneracy
    const int      linewidth = aShape->GetWidth() + 1;
    PLOT_DASH_TYPE lineStyle = aShape->GetStroke().GetPlotStyle();

    if( lineStyle <= PLOT_DASH_TYPE::FIRST_TYPE )
    {
        switch( aShape->GetShape() )
        {
        case SHAPE_T::CIRCLE:
        {
            const SFVEC2F center3DU = TO_SFVEC2F( aShape->GetCenter() );
            float         inner_radius3DU = TO_3DU( aShape->GetRadius() - linewidth / 2 );
            float         outer_radius3DU = TO_3DU( aShape->GetRadius() + linewidth / 2 );

            if( inner_radius3DU < 0 )
                inner_radius3DU = 0.0;

            if( outer_radius3DU == 0.0 )
            {
                // Don't attempt to render degenerate shapes
            }
            else if( aShape->IsFilled() )
            {
                aContainer->Add( new FILLED_CIRCLE_2D( center3DU, outer_radius3DU,
                                                       *aOwner ) );
            }
            else
            {
                aContainer->Add( new RING_2D( center3DU, inner_radius3DU, outer_radius3DU,
                                              *aOwner ) );
            }

            break;
        }

        case SHAPE_T::RECTANGLE:
            if( aShape->IsFilled() )
            {
                SHAPE_POLY_SET polyList;

                aShape->TransformShapeToPolygon( polyList, UNDEFINED_LAYER, 0, ARC_HIGH_DEF,
                                                 ERROR_INSIDE );

                polyList.Simplify( SHAPE_POLY_SET::PM_FAST );

                ConvertPolygonToTriangles( polyList, *aContainer, m_biuTo3Dunits, *aOwner );
            }
            else
            {
                std::vector<VECTOR2I> pts = aShape->GetRectCorners();

                aContainer->Add( new ROUND_SEGMENT_2D( TO_SFVEC2F( pts[0] ), TO_SFVEC2F( pts[1] ),
                                                       TO_3DU( linewidth ), *aOwner ) );
                aContainer->Add( new ROUND_SEGMENT_2D( TO_SFVEC2F( pts[1] ), TO_SFVEC2F( pts[2] ),
                                                       TO_3DU( linewidth ), *aOwner ) );
                aContainer->Add( new ROUND_SEGMENT_2D( TO_SFVEC2F( pts[2] ), TO_SFVEC2F( pts[3] ),
                                                       TO_3DU( linewidth ), *aOwner ) );
                aContainer->Add( new ROUND_SEGMENT_2D( TO_SFVEC2F( pts[3] ), TO_SFVEC2F( pts[0] ),
                                                       TO_3DU( linewidth ), *aOwner ) );
            }
            break;

        case SHAPE_T::ARC:
        {
            unsigned int segCount = GetCircleSegmentCount( aShape->GetBoundingBox().GetSizeMax() );

            transformArcToSegments( aShape->GetCenter(), aShape->GetStart(), aShape->GetArcAngle(),
                                    segCount, linewidth, aContainer, *aOwner );
            break;
        }

        case SHAPE_T::SEGMENT:
        {
            const SFVEC2F start3DU = TO_SFVEC2F( aShape->GetStart() );
            const SFVEC2F end3DU = TO_SFVEC2F( aShape->GetEnd() );
            const double  linewidth3DU = TO_3DU( linewidth );

            if( linewidth3DU == 0.0 )
            {
                // Don't attempt to render degenerate shapes
            }
            else if( Is_segment_a_circle( start3DU, end3DU ) )
            {
                // Cannot add segments that have the same start and end point
                aContainer->Add( new FILLED_CIRCLE_2D( start3DU, linewidth3DU / 2, *aOwner ) );
            }
            else
            {
                aContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, linewidth3DU, *aOwner ) );
            }

            break;
        }

        case SHAPE_T::BEZIER:
        case SHAPE_T::POLY:
        {
            SHAPE_POLY_SET polyList;

            aShape->TransformShapeToPolygon( polyList, UNDEFINED_LAYER, 0, ARC_HIGH_DEF,
                                             ERROR_INSIDE );

            if( polyList.IsEmpty() ) // Just for caution
                break;

            ConvertPolygonToTriangles( polyList, *aContainer, m_biuTo3Dunits, *aOwner );
            break;
        }

        default:
            wxFAIL_MSG( wxT( "BOARD_ADAPTER::addShape no implementation for " )
                        + aShape->SHAPE_T_asString() );
            break;
        }
    }
    else if( linewidth > 0 )
    {
        std::vector<SHAPE*> shapes = aShape->MakeEffectiveShapes( true );
        SFVEC2F             a3DU;
        SFVEC2F             b3DU;
        double              width3DU = TO_3DU( linewidth );

        const PCB_PLOT_PARAMS&     plotParams = aShape->GetBoard()->GetPlotOptions();
        KIGFX::PCB_RENDER_SETTINGS renderSettings;

        renderSettings.SetDashLengthRatio( plotParams.GetDashedLineDashRatio() );
        renderSettings.SetGapLengthRatio( plotParams.GetDashedLineGapRatio() );

        for( SHAPE* shape : shapes )
        {
            STROKE_PARAMS::Stroke( shape, lineStyle, linewidth, &renderSettings,
                    [&]( const VECTOR2I& a, const VECTOR2I& b )
                    {
                        a3DU = TO_SFVEC2F( a );
                        b3DU = TO_SFVEC2F( b );

                        if( Is_segment_a_circle( a3DU, b3DU ) )
                            aContainer->Add( new FILLED_CIRCLE_2D( a3DU, width3DU / 2, *aOwner ) );
                        else
                            aContainer->Add( new ROUND_SEGMENT_2D( a3DU, b3DU, width3DU, *aOwner ) );
                    } );
        }

        for( SHAPE* shape : shapes )
            delete shape;
    }
}


void BOARD_ADAPTER::addSolidAreasShapes( const ZONE* aZone, CONTAINER_2D_BASE* aContainer,
                                         PCB_LAYER_ID aLayerId )
{
    // This convert the poly in outline and holes
    ConvertPolygonToTriangles( *aZone->GetFilledPolysList( aLayerId ), *aContainer,
                               m_biuTo3Dunits, *aZone );
}


void BOARD_ADAPTER::buildPadOutlineAsSegments( const PAD* aPad, CONTAINER_2D_BASE* aContainer,
                                               int aWidth )
{
    if( aPad->GetShape() == PAD_SHAPE::CIRCLE )    // Draw a ring
    {
        const SFVEC2F center3DU = TO_SFVEC2F( aPad->ShapePos() );
        const int     radius = aPad->GetSize().x / 2;
        const float   inner_radius3DU = TO_3DU( radius - aWidth / 2 );
        const float   outer_radius3DU = TO_3DU( radius + aWidth / 2 );

        aContainer->Add( new RING_2D( center3DU, inner_radius3DU, outer_radius3DU, *aPad ) );

        return;
    }

    // For other shapes, add outlines as thick segments in polygon buffer
    const std::shared_ptr<SHAPE_POLY_SET>& corners = aPad->GetEffectivePolygon();
    const SHAPE_LINE_CHAIN&                path = corners->COutline( 0 );

    for( int j = 0; j < path.PointCount(); j++ )
    {
        SFVEC2F start3DU = TO_SFVEC2F( path.CPoint( j ) );
        SFVEC2F end3DU = TO_SFVEC2F( path.CPoint( j + 1 ) );

        if( aWidth == 0 )
        {
            // Don't attempt to render degenerate shapes
        }
        else if( Is_segment_a_circle( start3DU, end3DU ) )
        {
            // Cannot add segments that have the same start and end point
            aContainer->Add( new FILLED_CIRCLE_2D( start3DU, TO_3DU( aWidth / 2 ), *aPad ) );
        }
        else
        {
            aContainer->Add( new ROUND_SEGMENT_2D( start3DU, end3DU, TO_3DU( aWidth ), *aPad ) );
        }
    }
}
