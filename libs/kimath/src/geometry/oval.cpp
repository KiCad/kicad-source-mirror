/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include "geometry/oval.h"

#include <trigo.h> // for RotatePoint


std::vector<TYPED_POINT2I> GetOvalKeyPoints( const VECTOR2I& aOvalSize, const EDA_ANGLE& aRotation,
                                             OVAL_KEY_POINT_FLAGS aFlags )
{
    const VECTOR2I half_size = aOvalSize / 2;
    const int      half_width = std::min( half_size.x, half_size.y );
    const int      half_len = std::max( half_size.x, half_size.y );

    // Points on a non-rotated pad at the origin, long-axis is y
    // (so for now, width is left/right, len is up/down)
    std::vector<TYPED_POINT2I> pts;

    if ( aFlags & OVAL_CENTER )
    {
        // Centre is easy
        pts.emplace_back( VECTOR2I{ 0, 0 }, POINT_TYPE::PT_CENTER );
    };

    if( aFlags & OVAL_SIDE_MIDPOINTS )
    {
        // Side midpoints
        pts.emplace_back( VECTOR2I{ half_width, 0 }, POINT_TYPE::PT_MID );
        pts.emplace_back( VECTOR2I{ -half_width, 0 }, POINT_TYPE::PT_MID );
    }

    if( aFlags & OVAL_CAP_TIPS )
    {
        // If the oval is square-on, the tips are quadrants
        const POINT_TYPE pt_type = aRotation.IsCardinal() ? PT_QUADRANT : PT_END;

        // Cap ends
        pts.emplace_back( VECTOR2I{ 0, half_len }, pt_type );
        pts.emplace_back( VECTOR2I{ 0, -half_len }, pt_type );
    }

    // Distance from centre to cap centres
    const int d_centre_to_cap_centre = half_len - half_width;

    if ( aFlags & OVAL_CAP_CENTERS )
    {
        // Cap centres
        pts.emplace_back( VECTOR2I{ 0, d_centre_to_cap_centre }, POINT_TYPE::PT_CENTER );
        pts.emplace_back( VECTOR2I{ 0, -d_centre_to_cap_centre }, POINT_TYPE::PT_CENTER );
    }

    if ( aFlags & OVAL_SIDE_ENDS )
    {
        const auto add_end = [&]( const VECTOR2I& aPt )
        {
            pts.emplace_back( aPt, POINT_TYPE::PT_END );
        };

        // End points of flat sides (always vertical)
        add_end( { half_width, d_centre_to_cap_centre } );
        add_end( { half_width, -d_centre_to_cap_centre } );
        add_end( { -half_width, d_centre_to_cap_centre } );
        add_end( { -half_width, -d_centre_to_cap_centre } );
    }

    // If the pad is horizontal (i.e. x > y), we'll rotate the whole thing
    // 90 degrees and work with it as if it was vertical
    const bool      swap_xy = half_size.x > half_size.y;
    const EDA_ANGLE rotation = aRotation + ( swap_xy ? -ANGLE_90 : ANGLE_0 );

    // Add the quadrant points to the caps only if rotated
    // (otherwise they're just the tips)
    if( ( aFlags & OVAL_CARDINAL_EXTREMES ) && !rotation.IsCardinal() )
    {
        // We need to find two perpendicular lines from the centres
        // of each cap to the cap edge, which will hit the points
        // where the cap is tangent to H/V lines when rotated into place.
        //
        // Because we know the oval is always vertical, this means the
        // two lines are formed between _|, through \/ to |_
        // where the apex is the cap centre.

        // The vector from a cap centre to the tip (i.e. vertical)
        const VECTOR2I cap_radial = { 0, half_width };

        // Rotate in the opposite direction to the oval's rotation
        // (that will be unwound later)
        EDA_ANGLE radial_line_rotation = -rotation;

        radial_line_rotation.Normalize90();

        VECTOR2I cap_radial_to_x_axis = cap_radial;
        RotatePoint( cap_radial_to_x_axis, radial_line_rotation );

        // Find the other line - it's 90 degrees away, but re-normalise
        // as it could be to the left or right
        radial_line_rotation -= ANGLE_90;
        radial_line_rotation.Normalize90();

        VECTOR2I cap_radial_to_y_axis = cap_radial;
        RotatePoint( cap_radial_to_y_axis, radial_line_rotation );

        const auto add_quadrant = [&]( const VECTOR2I& aPt )
        {
            pts.emplace_back( aPt, POINT_TYPE::PT_QUADRANT );
        };

        // The quadrant points are then the relevant offsets from each cap centre
        add_quadrant( VECTOR2I{ 0, d_centre_to_cap_centre } + cap_radial_to_y_axis );
        add_quadrant( VECTOR2I{ 0, d_centre_to_cap_centre } + cap_radial_to_x_axis );
        // The opposite cap offsets go from the other cap centre, the other way
        add_quadrant( VECTOR2I{ 0, -d_centre_to_cap_centre } - cap_radial_to_y_axis );
        add_quadrant( VECTOR2I{ 0, -d_centre_to_cap_centre } - cap_radial_to_x_axis );
    }

    for( TYPED_POINT2I& pt : pts )
    {
        // Transform to the actual orientation
        // Already includes the extra 90 to swap x/y if needed
        RotatePoint( pt.m_point, rotation );
    }

    return pts;
}