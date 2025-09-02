/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>

using namespace KIGEOM;


SHAPE_LINE_CHAIN KIGEOM::ConvertToChain( const SHAPE_SEGMENT& aOval )
{
    const SEG&     seg = aOval.GetSeg();
    const VECTOR2I perp = GetRotated( seg.B - seg.A, ANGLE_90 ).Resize( aOval.GetWidth() / 2 );

    SHAPE_LINE_CHAIN chain;
    chain.Append( seg.A - perp );
    chain.Append( SHAPE_ARC( seg.A, seg.A - perp, ANGLE_180 ) );
    chain.Append( seg.B + perp );
    chain.Append( SHAPE_ARC( seg.B, seg.B + perp, ANGLE_180 ) );
    chain.SetClosed( true );
    return chain;
}


std::vector<TYPED_POINT2I> KIGEOM::GetOvalKeyPoints( const SHAPE_SEGMENT& aOval, OVAL_KEY_POINT_FLAGS aFlags )
{
    const int       half_width = aOval.GetWidth() / 2;
    const int       half_len = aOval.GetTotalLength() / 2;
    const EDA_ANGLE rotation = aOval.GetAngle() - ANGLE_90;

    // Points on a non-rotated pad at the origin, long-axis is y
    // (so for now, width is left/right, len is up/down)
    std::vector<TYPED_POINT2I> pts;

    if( aFlags & OVAL_CENTER )
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
        const POINT_TYPE pt_type = rotation.IsCardinal() ? PT_QUADRANT : PT_END;

        // Cap ends
        pts.emplace_back( VECTOR2I{ 0, half_len }, pt_type );
        pts.emplace_back( VECTOR2I{ 0, -half_len }, pt_type );
    }

    // Distance from centre to cap centres
    const int d_centre_to_cap_centre = half_len - half_width;

    if( aFlags & OVAL_CAP_CENTERS )
    {
        // Cap centres
        pts.emplace_back( VECTOR2I{ 0, d_centre_to_cap_centre }, POINT_TYPE::PT_CENTER );
        pts.emplace_back( VECTOR2I{ 0, -d_centre_to_cap_centre }, POINT_TYPE::PT_CENTER );
    }

    if( aFlags & OVAL_SIDE_ENDS )
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
        EDA_ANGLE radial_line_rotation = rotation;

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
        RotatePoint( pt.m_point, -rotation );

        // Translate to the actual position
        pt.m_point += aOval.GetCenter();
    }

    return pts;
}
