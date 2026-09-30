/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "fab_pad_geometry.h"

#include <geometry/shape_poly_set.h>


PAD_LAYER_GEOMETRY::PAD_LAYER_GEOMETRY( const PAD& aPad, PCB_LAYER_ID aLayer ) :
        m_pad( std::make_unique<PAD>( aPad ) ),
        m_layer( aLayer )
{
}


SHAPE_POLY_SET PAD_LAYER_GEOMETRY::Polygon() const
{
    SHAPE_POLY_SET polygon;

    if( !m_empty )
        m_pad->TransformShapeToPolygon( polygon, m_layer, 0, m_maxError, ERROR_INSIDE );

    return polygon;
}


VECTOR2I ResolvePadMargin( const PAD& aPad, PCB_LAYER_ID aLayer )
{
    if( aLayer == F_Mask || aLayer == B_Mask )
    {
        int expansion = aPad.GetSolderMaskExpansion( aLayer );
        return VECTOR2I( expansion, expansion );
    }

    if( aLayer == F_Paste || aLayer == B_Paste )
        return aPad.GetSolderPasteMargin( aLayer );

    return VECTOR2I( 0, 0 );
}


PAD_LAYER_GEOMETRY ResolvePadLayer( const PAD& aPad, PCB_LAYER_ID aLayer, int aMaxError )
{
    PAD_LAYER_GEOMETRY result( aPad, aLayer );
    result.m_maxError = aMaxError;
    result.m_margin = ResolvePadMargin( aPad, aLayer );

    const VECTOR2I  adjustedSize = aPad.GetSize( aLayer ) + 2 * result.m_margin;
    const int       clearance = result.m_margin.x;
    const bool      sameMargin = result.m_margin.x == result.m_margin.y;
    const PAD_SHAPE shape = aPad.GetShape( aLayer );
    PAD&            resolved = *result.m_pad;
    auto            makePolygonPad = [&]( const SHAPE_POLY_SET& aOutline )
    {
        resolved.SetAnchorPadShape( aLayer, PAD_SHAPE::CIRCLE );
        resolved.SetShape( aLayer, PAD_SHAPE::CUSTOM );
        resolved.DeletePrimitivesList();
        resolved.AddPrimitivePoly( aLayer, aOutline, 0, true );
        resolved.SetSize( aLayer, VECTOR2I( 0, 0 ) );
    };

    if( shape != PAD_SHAPE::CUSTOM && ( adjustedSize.x <= 0 || adjustedSize.y <= 0 ) )
        result.m_empty = true;

    switch( shape )
    {
    case PAD_SHAPE::CIRCLE:
    case PAD_SHAPE::OVAL: resolved.SetSize( aLayer, adjustedSize ); break;

    case PAD_SHAPE::RECTANGLE:
        resolved.SetSize( aLayer, adjustedSize );

        if( clearance > 0 )
        {
            result.m_cornerRadius = clearance;
            resolved.SetShape( aLayer, PAD_SHAPE::ROUNDRECT );
            resolved.SetRoundRectCornerRadius( aLayer, clearance );
        }

        break;

    case PAD_SHAPE::ROUNDRECT:
    {
        const double ratio = aPad.GetRoundRectRadiusRatio( aLayer );
        resolved.SetSize( aLayer, adjustedSize );

        // Equal margins offset the corner radius; unequal margins keep the ratio as the plotter does
        if( sameMargin )
        {
            result.m_cornerRadius = std::max( 0, aPad.GetRoundRectCornerRadius( aLayer ) + clearance );
            resolved.SetRoundRectCornerRadius( aLayer, result.m_cornerRadius );
        }
        else
        {
            result.m_cornerRadius =
                    KiROUND( ratio * std::min( std::abs( adjustedSize.x ), std::abs( adjustedSize.y ) ) );
        }

        break;
    }

    case PAD_SHAPE::CHAMFERED_RECT:
    {
        if( clearance <= 0 )
        {
            resolved.SetSize( aLayer, adjustedSize );
            break;
        }

        PAD dummy( aPad );
        dummy.SetPosition( VECTOR2I( 0, 0 ) );
        dummy.SetOffset( aLayer, VECTOR2I( 0, 0 ) );
        dummy.SetOrientation( ANGLE_0 );

        if( !sameMargin )
            dummy.SetSize( aLayer, adjustedSize );

        SHAPE_POLY_SET outline;
        dummy.TransformShapeToPolygon( outline, aLayer, 0, aMaxError, ERROR_INSIDE );

        if( sameMargin )
            outline.InflateWithLinkedHoles( clearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, aMaxError );

        makePolygonPad( outline );
        result.m_contour = std::move( outline );
        break;
    }

    case PAD_SHAPE::TRAPEZOID:
    {
        const VECTOR2I halfSize = aPad.GetSize( aLayer ) / 2;
        const VECTOR2I halfDelta = aPad.GetDelta( aLayer ) / 2;
        SHAPE_POLY_SET outline;
        outline.NewOutline();
        outline.Append( -halfSize.x - halfDelta.y, halfSize.y + halfDelta.x );
        outline.Append( halfSize.x + halfDelta.y, halfSize.y - halfDelta.x );
        outline.Append( halfSize.x - halfDelta.y, -halfSize.y + halfDelta.x );
        outline.Append( -halfSize.x + halfDelta.y, -halfSize.y - halfDelta.x );

        // Linked-hole inflation preserves deflated contours that contain holes
        if( clearance != 0 )
            outline.InflateWithLinkedHoles( clearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, aMaxError );

        result.m_contour = std::move( outline );

        if( clearance == 0 )
            break;

        makePolygonPad( result.m_contour );
        break;
    }

    case PAD_SHAPE::CUSTOM:
    {
        SHAPE_POLY_SET outline;
        aPad.MergePrimitivesAsPolygon( aLayer, &outline );

        if( clearance != 0 )
            outline.InflateWithLinkedHoles( clearance, CORNER_STRATEGY::ROUND_ALL_CORNERS, aMaxError );

        resolved.DeletePrimitivesList();
        resolved.AddPrimitivePoly( aLayer, outline, 0, true );
        result.m_contour = std::move( outline );

        // The anchor is merged into the shape, so it must fit inside a deflated outline
        if( clearance < 0 )
            resolved.SetSize( aLayer, VECTOR2I( std::max( 0, adjustedSize.x ), std::max( 0, adjustedSize.y ) ) );

        break;
    }

    default: result.m_empty = true; break;
    }

    return result;
}
