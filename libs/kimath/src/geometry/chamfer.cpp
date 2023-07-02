/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <geometry/chamfer.h>

std::optional<CHAMFER_RESULT> ComputeChamferPoints( const SEG aSegA, const SEG& aSegB,
                                                    const CHAMFER_PARAMS& aChamferParams )
{
    const int line_a_setback = aChamferParams.m_chamfer_setback_a_IU;
    const int line_b_setback = aChamferParams.m_chamfer_setback_b_IU;

    if( line_a_setback == 0 && line_b_setback == 0 )
    {
        // No chamfer to do
        // In theory a chamfer of 0 on one side is kind-of valid (adds a collinear point)
        // so allow it (using an and above, not an or)
        return std::nullopt;
    }

    if( aSegA.Length() < line_a_setback || aSegB.Length() < line_b_setback )
    {
        // Chamfer is too big for the line segments
        return std::nullopt;
    }

    // We only support the case where the lines intersect at the end points
    // otherwise we would need to decide which inside corner to chamfer

    // Figure out which end points are the ones at the intersection
    const VECTOR2I* a_pt = nullptr;
    const VECTOR2I* b_pt = nullptr;

    if( aSegA.A == aSegB.A )
    {
        a_pt = &aSegA.A;
        b_pt = &aSegB.A;
    }
    else if( aSegA.A == aSegB.B )
    {
        a_pt = &aSegA.A;
        b_pt = &aSegB.B;
    }
    else if( aSegA.B == aSegB.A )
    {
        a_pt = &aSegA.B;
        b_pt = &aSegB.A;
    }
    else if( aSegA.B == aSegB.B )
    {
        a_pt = &aSegA.B;
        b_pt = &aSegB.B;
    }

    if( !a_pt || !b_pt )
    {
        // No intersection found, so no chamfer to do
        return std::nullopt;
    }

    // These are the other existing line points (the ones that are not the intersection)
    const VECTOR2I& a_end_pt = ( &aSegA.A == a_pt ) ? aSegA.B : aSegA.A;
    const VECTOR2I& b_end_pt = ( &aSegB.A == b_pt ) ? aSegB.B : aSegB.A;

    // Now, construct segment of the set-back lengths, that begins
    // at the intersection point and is parallel to each line segments
    SEG setback_a( *a_pt, *b_pt + VECTOR2I( a_end_pt - *a_pt ).Resize( line_a_setback ) );
    SEG setback_b( *b_pt, *b_pt + VECTOR2I( b_end_pt - *b_pt ).Resize( line_b_setback ) );

    // The chamfer segment then goes between the end points of the set-back segments
    SEG chamfer( setback_a.B, setback_b.B );

    // The adjusted segments go from the old end points to the chamfer ends

    std::optional<SEG> new_a;
    if( a_end_pt != chamfer.A )
    {
        new_a = SEG{ a_end_pt, chamfer.A };
    }

    std::optional<SEG> new_b;
    if( b_end_pt != chamfer.B )
    {
        new_b = SEG{ b_end_pt, chamfer.B };
    }

    return CHAMFER_RESULT{ chamfer, new_a, new_b };
}