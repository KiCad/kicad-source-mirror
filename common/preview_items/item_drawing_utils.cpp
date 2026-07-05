/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "preview_items/item_drawing_utils.h"

#include <array>

#include <geometry/geometry_utils.h>
#include <geometry/seg.h>

using namespace KIGFX;

void KIGFX::DrawCross( GAL& aGal, const VECTOR2I& aPosition, int aSize )
{
    const int size = aSize / 2;
    aGal.DrawLine( aPosition - VECTOR2I( size, 0 ), aPosition + VECTOR2I( size, 0 ) );
    aGal.DrawLine( aPosition - VECTOR2I( 0, size ), aPosition + VECTOR2I( 0, size ) );
}


void KIGFX::DrawDashedLine( GAL& aGal, const SEG& aSeg, double aDashSize )
{
    const std::array<double, 2> strokes = { aDashSize, aDashSize / 2 };
    const double                dashCycleLen = strokes[0] + strokes[1];

    // Endpoints may overflow int32 when an item is dragged past the coordinate limit, wrapping
    // an int length negative and spinning the dash loop forever. Measure in double precision.
    const VECTOR2D segVec( double( aSeg.B.x ) - aSeg.A.x, double( aSeg.B.y ) - aSeg.A.y );
    const double   segLen = segVec.EuclideanNorm();

    // Draw solid when the cycle is degenerate, sub-pixel, or would emit too many dashes; the
    // latter two arise for off-screen or overflowed geometry and would otherwise hang.
    constexpr double maxDashes = 100000.0;

    if( dashCycleLen <= 0.0 || dashCycleLen * aGal.GetWorldScale() <= 1.0
        || segLen / dashCycleLen > maxDashes )
    {
        aGal.DrawLine( aSeg.A, aSeg.B );
        return;
    }

    const BOX2I clip = BOX2ISafe( VECTOR2D( aSeg.A.x, aSeg.A.y ), segVec );

    const double theta = atan2( aSeg.B.y - aSeg.A.y, aSeg.B.x - aSeg.A.x );

    const VECTOR2D cycleVec{
        dashCycleLen * cos( theta ),
        dashCycleLen * sin( theta ),
    };

    const VECTOR2D dashVec{
        strokes[0] * cos( theta ),
        strokes[0] * sin( theta ),
    };

    const unsigned cycleCount = static_cast<unsigned>( segLen / dashCycleLen ) + 1;

    for( unsigned cyclei = 0; cyclei < cycleCount; ++cyclei )
    {
        const VECTOR2D dashStart = aSeg.A + cycleVec * cyclei;
        const VECTOR2D dashEnd = dashStart + dashVec;

        // Drawing each segment can be done rounded to ints.
        SEG dashSeg{ KiROUND( dashStart ), KiROUND( dashEnd ) };

        if( ClipLine( &clip, dashSeg.A.x, dashSeg.A.y, dashSeg.B.x, dashSeg.B.y ) )
            break;

        aGal.DrawLine( dashSeg.A, dashSeg.B );
    }
}
