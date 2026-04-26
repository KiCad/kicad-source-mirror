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
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <geometry/grid_geometry.h>

#include <algorithm>
#include <cmath>

#include <trigo.h>
#include <wx/log.h>


VECTOR2D GRID_GEOMETRY::Snap( const VECTOR2D& aPoint ) const
{
    const VECTOR2D local = GetRotated( aPoint - origin, EDA_ANGLE( -orientation, RADIANS_T ) );
    VECTOR2D       candidate;

    switch( kind )
    {
    case KIND::POLAR:
    {
        const double dr = pitch.x;
        const double dPhi = pitch.y;
        const double rMax = extent.x;
        const double phiMax = extent.y;

        if( dr <= 0.0 || dPhi <= 0.0 )
            return aPoint;

        const double r = std::hypot( local.x, local.y );
        const int    maxRIdx = static_cast<int>( rMax / dr );
        const int    rIdx = std::clamp( static_cast<int>( std::round( r / dr ) ), 0, maxRIdx );

        // Normalise phi to [0, 2*pi) and clamp to the last drawn spoke; otherwise
        // a cursor just inside the wedge rounds up past the last spoke.
        double phi = std::atan2( local.y, local.x );

        if( phi < 0 )
            phi += 2 * M_PI;

        const int maxPIdx = static_cast<int>( phiMax / dPhi );
        const int pIdx = std::clamp( static_cast<int>( std::round( phi / dPhi ) ), 0, maxPIdx );

        const double phiQ = pIdx * dPhi;
        const double rSnapped = rIdx * dr;

        candidate = VECTOR2D( rSnapped * std::cos( phiQ ), rSnapped * std::sin( phiQ ) );
        break;
    }

    case KIND::CARTESIAN:
    {
        const double dx = pitch.x;
        const double dy = pitch.y;

        if( dx <= 0.0 || dy <= 0.0 )
            return aPoint;

        // Centred on origin; extent is size/2.  Indices go -N..+N.
        const int nX = static_cast<int>( extent.x / dx );
        const int nY = static_cast<int>( extent.y / dy );

        const int xIdx = std::clamp( static_cast<int>( std::round( local.x / dx ) ), -nX, nX );
        const int yIdx = std::clamp( static_cast<int>( std::round( local.y / dy ) ), -nY, nY );

        candidate = VECTOR2D( xIdx * dx, yIdx * dy );
        break;
    }

    default: wxFAIL_MSG( wxT( "GRID_GEOMETRY::Snap: unhandled KIND" ) ); return aPoint;
    }

    return GetRotated( candidate, EDA_ANGLE( orientation, RADIANS_T ) ) + origin;
}


bool GRID_GEOMETRY::Contains( const VECTOR2D& aPoint, double aTolerance ) const
{
    const VECTOR2D local = GetRotated( aPoint - origin, EDA_ANGLE( -orientation, RADIANS_T ) );

    switch( kind )
    {
    case KIND::POLAR:
    {
        const double r = std::hypot( local.x, local.y );

        if( r > extent.x + aTolerance )
            return false;

        const double phiMax = extent.y;
        double       phi = std::atan2( local.y, local.x );

        while( phi < 0 )
            phi += 2 * M_PI;

        return phi <= std::abs( phiMax );
    }

    case KIND::CARTESIAN:
        return local.x >= -extent.x - aTolerance && local.x <= extent.x + aTolerance
               && local.y >= -extent.y - aTolerance && local.y <= extent.y + aTolerance;

    default: wxFAIL_MSG( wxT( "GRID_GEOMETRY::Contains: unhandled KIND" ) ); return false;
    }
}


double GRID_GEOMETRY::Area() const
{
    switch( kind )
    {
    case KIND::POLAR:
        // Polar wedge: 0.5 * r^2 * phi.
        return 0.5 * extent.x * extent.x * std::abs( extent.y );

    case KIND::CARTESIAN:
        // Centred rectangle: 4 * (size/2).x * (size/2).y.
        return std::abs( 4.0 * extent.x * extent.y );

    default: wxFAIL_MSG( wxT( "GRID_GEOMETRY::Area: unhandled KIND" ) ); return 0.0;
    }
}
