/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcbnew_utils/board_construction_utils.h>

#include <fp_shape.h>
#include <footprint.h>
#include <geometry/seg.h>
#include <math/vector2d.h>


namespace KI_TEST
{

void DrawSegment( FOOTPRINT& aFootprint, const SEG& aSeg, int aWidth, PCB_LAYER_ID aLayer )
{
    auto seg = std::make_unique<FP_SHAPE>( &aFootprint, SHAPE_T::SEGMENT );

    seg->SetStart0( (wxPoint) aSeg.A );
    seg->SetEnd0( (wxPoint) aSeg.B );

    seg->SetWidth( aWidth );
    seg->SetLayer( aLayer );

    aFootprint.Add( seg.release() );
}


void DrawPolyline( FOOTPRINT& aFootprint, const std::vector<VECTOR2I>& aPts, int aWidth,
                   PCB_LAYER_ID aLayer )
{
    for( unsigned i = 0; i < aPts.size() - 1; ++i )
    {
        DrawSegment( aFootprint, { aPts[i], aPts[i + 1] }, aWidth, aLayer );
    }
}


void DrawArc( FOOTPRINT& aFootprint, const VECTOR2I& aCentre, const VECTOR2I& aStart,
              double aAngle, int aWidth, PCB_LAYER_ID aLayer )
{
    auto seg = std::make_unique<FP_SHAPE>( &aFootprint, SHAPE_T::ARC );

    seg->SetCenter0( (wxPoint) aCentre );
    seg->SetStart0( (wxPoint) aStart );
    seg->SetArcAngleAndEnd0( aAngle * 10 );

    seg->SetWidth( aWidth );
    seg->SetLayer( aLayer );

    aFootprint.Add( seg.release() );
}


void DrawRect( FOOTPRINT& aFootprint, const VECTOR2I& aPos, const VECTOR2I& aSize, int aRadius,
               int aWidth, PCB_LAYER_ID aLayer )
{
    const auto x_r = aPos.x + aSize.x / 2;
    const auto x_l = aPos.x - aSize.x / 2;
    const auto y_t = aPos.y + aSize.y / 2;
    const auto y_b = aPos.y - aSize.y / 2;

    const auto xin_r = x_r - aRadius;
    const auto xin_l = x_l + aRadius;
    const auto yin_t = y_t - aRadius;
    const auto yin_b = y_b + aRadius;

    // If non-zero (could be zero if it's a stadium shape)
    if( xin_l != xin_r )
    {
        DrawSegment( aFootprint, { { xin_l, y_t }, { xin_r, y_t } }, aWidth, aLayer );
        DrawSegment( aFootprint, { { xin_l, y_b }, { xin_r, y_b } }, aWidth, aLayer );
    }

    if( yin_b != yin_t )
    {
        DrawSegment( aFootprint, { { x_l, yin_b }, { x_l, yin_t } }, aWidth, aLayer );
        DrawSegment( aFootprint, { { x_r, yin_b }, { x_r, yin_t } }, aWidth, aLayer );
    }

    if( aRadius > 0 )
    {
        DrawArc( aFootprint, { xin_r, yin_t }, { x_r, yin_t }, 90, aWidth, aLayer );
        DrawArc( aFootprint, { xin_l, yin_t }, { xin_l, y_t }, 90, aWidth, aLayer );
        DrawArc( aFootprint, { xin_l, yin_b }, { x_l, yin_b }, 90, aWidth, aLayer );
        DrawArc( aFootprint, { xin_r, yin_b }, { xin_r, y_b }, 90, aWidth, aLayer );
    }
}

} // namespace KI_TEST
