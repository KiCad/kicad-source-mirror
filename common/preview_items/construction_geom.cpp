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

#include "preview_items/construction_geom.h"

#include <layer_ids.h>
#include <utility>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/line.h>
#include <geometry/shape_utils.h>
#include <preview_items/item_drawing_utils.h>
#include <view/view.h>

using namespace KIGFX;


CONSTRUCTION_GEOM::CONSTRUCTION_GEOM() :
        EDA_ITEM( nullptr, NOT_USED ), // Never added to a BOARD/SCHEMATIC so it needs no type
        m_color( COLOR4D::WHITE ), m_persistentColor( COLOR4D::WHITE )
{
}


void CONSTRUCTION_GEOM::AddDrawable( const DRAWABLE& aItem, bool aPersistent, int aLineWidth )
{
    m_drawables.push_back( { aItem, aPersistent, aLineWidth } );
}


void CONSTRUCTION_GEOM::SetSnapGuides( std::vector<SNAP_GUIDE> aGuides )
{
    m_snapGuides = std::move( aGuides );
}


void CONSTRUCTION_GEOM::ClearDrawables()
{
    m_drawables.clear();
}


const BOX2I CONSTRUCTION_GEOM::ViewBBox() const
{
    // We could be a bit more circumspect here, but much of the time the
    // extended lines go across the whole screen anyway
    BOX2I bbox;
    bbox.SetMaximum();
    return bbox;
}


void CONSTRUCTION_GEOM::ViewDraw( int aLayer, VIEW* aView ) const
{
    GAL& gal = *aView->GetGAL();

    gal.SetIsFill( false );
    gal.SetIsStroke( true );
    gal.SetLineWidth( 1 );

    const BOX2I viewport = BOX2ISafe( aView->GetViewport() );

    // Prevents extremely short snap lines from inhibiting drawing
    // These can happen due to rounding in intersections, etc.
    // (usually it is length 1 IU)
    const int  minSnapLineLength = 10;
    const bool haveSnapLine = m_snapLine && m_snapLine->Length() >= minSnapLineLength;

    // Avoid fighting with the snap line
    const auto drawLineIfNotAlsoSnapLine =
            [&]( const SEG& aLine )
            {
                if( !haveSnapLine || !aLine.ApproxCollinear( *m_snapLine, 1 ) )
                {
                    gal.DrawLine( aLine.A, aLine.B );
                }
            };

    // Draw all the items
    for( const DRAWABLE_INFO& drawable : m_drawables )
    {
        gal.SetStrokeColor( drawable.IsPersistent ? m_persistentColor : m_color );
        gal.SetLineWidth( drawable.LineWidth / gal.GetWorldScale() );

        std::visit(
                [&]( const auto& visited )
                {
                    using ItemType = std::decay_t<decltype( visited )>;

                    if constexpr( std::is_same_v<ItemType, LINE> )
                    {
                        // Extend the segment to the viewport boundary
                        std::optional<SEG> segToBoundary =
                                KIGEOM::ClipLineToBox( visited, viewport );

                        if( segToBoundary )
                        {
                            drawLineIfNotAlsoSnapLine( *segToBoundary );
                        }
                    }
                    else if constexpr( std::is_same_v<ItemType, HALF_LINE> )
                    {
                        // Extend the ray to the viewport boundary
                        std::optional<SEG> segToBoundary =
                                KIGEOM::ClipHalfLineToBox( visited, viewport );

                        if( segToBoundary )
                        {
                            drawLineIfNotAlsoSnapLine( *segToBoundary );
                        }
                    }
                    else if constexpr( std::is_same_v<ItemType, SEG> )
                    {
                        drawLineIfNotAlsoSnapLine( visited );
                    }
                    else if constexpr( std::is_same_v<ItemType, CIRCLE> )
                    {
                        gal.DrawCircle( visited.Center, visited.Radius );
                    }
                    else if constexpr( std::is_same_v<ItemType, SHAPE_ARC> )
                    {
                        gal.DrawArc( visited.GetCenter(), visited.GetRadius(),
                                     visited.GetStartAngle(), visited.GetCentralAngle() );
                    }
                    else if constexpr( std::is_same_v<ItemType, VECTOR2I> )
                    {
                        KIGFX::DrawCross( gal, visited, aView->ToWorld( 16 ) );
                    }
                },
                drawable.Item );
    }

    for( const SNAP_GUIDE& guide : m_snapGuides )
    {
        const SEG& segment = guide.Segment;

        if( segment.A == segment.B )
            continue;

        std::optional<SEG> clipped = KIGEOM::ClipLineToBox( LINE( segment ), viewport );

        if( !clipped )
            continue;

        gal.SetStrokeColor( guide.Color );
        gal.SetLineWidth( guide.LineWidth );
        gal.DrawLine( clipped->A, clipped->B );
    }

    if( haveSnapLine )
    {
        gal.SetStrokeColor( m_persistentColor );
        gal.SetLineWidth( 2 );

        const int dashSizeBasis = aView->ToWorld( 12 );
        const int snapOriginMarkerSize = aView->ToWorld( 16 );

        // Avoid clash with the snap marker if very close
        const int omitStartMarkerIfWithinLength = aView->ToWorld( 8 );

        // The line itself
        KIGFX::DrawDashedLine( gal, *m_snapLine, dashSizeBasis );

        // The line origin marker if the line is long enough
        if( m_snapLine->A.Distance( m_snapLine->B ) > omitStartMarkerIfWithinLength )
        {
            KIGFX::DrawCross( gal, m_snapLine->A, snapOriginMarkerSize );
            gal.DrawCircle( m_snapLine->A, snapOriginMarkerSize / 2 );
        }
    }
}


std::vector<int> CONSTRUCTION_GEOM::ViewGetLayers() const
{
    // Don't use LAYER_GP_OVERLAY, we need the construction geometry to be visible
    // on top of the axis cross
    std::vector<int> layers{ LAYER_UI_START };
    return layers;
}
