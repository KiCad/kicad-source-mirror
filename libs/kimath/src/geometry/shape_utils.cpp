/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "geometry/shape_utils.h"

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>


bool KIGEOM::AddHoleIfValid( SHAPE_POLY_SET& aOutline, SHAPE_LINE_CHAIN&& aHole )
{
    if( aHole.PointCount() < 3 || aHole.Area() == 0 )
    {
        return false;
    }

    aOutline.AddHole( std::move( aHole ) );
    return true;
}


std::vector<VECTOR2I> KIGEOM::MakeRegularPolygonPoints( const VECTOR2I& aCenter, size_t aN,
                                                        const VECTOR2I& aPt0 )
{
    VECTOR2D              pt0FromC = aPt0 - aCenter;
    std::vector<VECTOR2I> pts;

    for( size_t i = 0; i < aN; i++ )
    {
        VECTOR2D pt = pt0FromC;
        RotatePoint( pt, ( FULL_CIRCLE / double( aN ) ) * i );
        pts.push_back( KiROUND( pt + aCenter ) );
    }

    return pts;
}


std::vector<VECTOR2I> KIGEOM::MakeRegularPolygonPoints( const VECTOR2I& aCenter, size_t aN,
                                                        int aRadius, bool aAcrossCorners,
                                                        EDA_ANGLE aAngle )
{
    if( !aAcrossCorners )
    {
        // if across flats, increase the radius
        aRadius = aRadius / ( FULL_CIRCLE / ( aN * 2 ) ).Cos();
    }

    VECTOR2I pt = VECTOR2I{ aRadius, 0 };
    RotatePoint( pt, aAngle );

    const VECTOR2I pt0 = aCenter + pt;
    return KIGEOM::MakeRegularPolygonPoints( aCenter, aN, pt0 );
}


std::vector<SEG> KIGEOM::MakeCrossSegments( const VECTOR2I& aCenter, const VECTOR2I& aSize,
                                            EDA_ANGLE aAngle )
{
    std::vector<SEG> segs;

    VECTOR2I pt = VECTOR2I{ aSize.x / 2, 0 };
    RotatePoint( pt, aAngle );

    VECTOR2I pt0 = aCenter - pt;
    segs.emplace_back( pt0, aCenter - ( pt0 - aCenter ) );

    pt = VECTOR2I{ 0, aSize.y / 2 };
    RotatePoint( pt, aAngle );

    pt0 = aCenter - pt;
    segs.emplace_back( pt0, aCenter - ( pt0 - aCenter ) );

    return segs;
}