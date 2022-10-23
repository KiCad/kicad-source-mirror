/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2015-2022 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <geometry/shape_rect.h>

bool SHAPE_RECT::Collide( const SEG& aSeg, int aClearance, int* aActual, VECTOR2I* aLocation ) const
{
    BOX2I bbox( BBox() );

    if( bbox.Contains( aSeg.A ) )
    {
        if( aLocation )
            *aLocation = aSeg.A;

        if( aActual )
            *aActual = 0;

        return true;
    }

    if( bbox.Contains( aSeg.B ) )
    {
        if( aLocation )
            *aLocation = aSeg.B;

        if( aActual )
            *aActual = 0;

        return true;
    }

    VECTOR2I corners[] = { VECTOR2I( m_p0.x, m_p0.y ),
                           VECTOR2I( m_p0.x, m_p0.y + m_h ),
                           VECTOR2I( m_p0.x + m_w, m_p0.y + m_h ),
                           VECTOR2I( m_p0.x + m_w, m_p0.y ),
                           VECTOR2I( m_p0.x, m_p0.y ) };

    SEG::ecoord closest_dist_sq = VECTOR2I::ECOORD_MAX;
    VECTOR2I nearest;

    for( int i = 0; i < 4; i++ )
    {
        SEG side( corners[i], corners[ i + 1] );
        SEG::ecoord dist_sq = side.SquaredDistance( aSeg );

        if( dist_sq < closest_dist_sq )
        {
            if ( aLocation )
            {
                nearest = side.NearestPoint( aSeg );
            }

            closest_dist_sq = dist_sq;
        }
    }

    if( closest_dist_sq == 0 || closest_dist_sq < SEG::Square( aClearance ) )
    {
        if( aActual )
            *aActual = sqrt( closest_dist_sq );

        if( aLocation )
            *aLocation = nearest;

        return true;
    }

    return false;
}

const std::string SHAPE_RECT::Format( bool aCplusPlus ) const
{
    std::stringstream ss;

    ss << "SHAPE_RECT( ";
    ss << m_p0.x;
    ss << ", ";
    ss << m_p0.y;
    ss << ", ";
    ss << m_w;
    ss << ", ";
    ss << m_h;
    ss << ");";

    return ss.str();
}
