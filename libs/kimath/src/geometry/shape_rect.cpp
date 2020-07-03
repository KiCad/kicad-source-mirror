/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
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


bool SHAPE_RECT::Collide( const SEG& aSeg, int aClearance, int* aActual ) const
{
    if( BBox( 0 ).Contains( aSeg.A ) || BBox( 0 ).Contains( aSeg.B ) )
    {
        if( aActual )
            *aActual = 0;

        return true;
    }

    VECTOR2I corners[] = { VECTOR2I( m_p0.x, m_p0.y ),
                           VECTOR2I( m_p0.x, m_p0.y + m_h ),
                           VECTOR2I( m_p0.x + m_w, m_p0.y + m_h ),
                           VECTOR2I( m_p0.x + m_w, m_p0.y ),
                           VECTOR2I( m_p0.x, m_p0.y ) };

    SEG s( corners[0], corners[1] );
    SEG::ecoord dist_squared = s.SquaredDistance( aSeg );

    for( int i = 1; i < 4; i++ )
    {
        s = SEG( corners[i], corners[ i + 1] );
        dist_squared = std::min( dist_squared, s.SquaredDistance( aSeg ) );
    }

    if( dist_squared < (ecoord) aClearance * aClearance )
    {
        if( aActual )
            *aActual = sqrt( dist_squared );

        return true;
    }

    return false;
}
