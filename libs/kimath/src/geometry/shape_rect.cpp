/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
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


#include <geometry/shape_rect.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/roundrect.h>

bool SHAPE_RECT::Collide( const SEG& aSeg, int aClearance, int* aActual, VECTOR2I* aLocation ) const
{
    if( m_radius > 0 )
    {
        SHAPE_LINE_CHAIN lineChain( Outline() );
        return lineChain.Collide( aSeg, aClearance, aActual, aLocation );
    }

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
        else if( aLocation && dist_sq == closest_dist_sq )
        {
            VECTOR2I near = side.NearestPoint( aSeg );

            if( ( near - aSeg.A ).SquaredEuclideanNorm()
                < ( nearest - aSeg.A ).SquaredEuclideanNorm() )
            {
                nearest = near;
            }
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
    ss << ", ";
    ss << m_radius;
    ss << ");";

    return ss.str();
}


void SHAPE_RECT::TransformToPolygon( SHAPE_POLY_SET& aBuffer, int aError, ERROR_LOC aErrorLoc ) const
{
    if( m_radius > 0 )
    {
        ROUNDRECT rr( *this, m_radius );
        rr.TransformToPolygon( aBuffer, aError );
        return;
    }

    int idx = aBuffer.NewOutline();
    SHAPE_LINE_CHAIN& outline = aBuffer.Outline( idx );

    outline.Append( m_p0 );
    outline.Append( { m_p0.x + m_w, m_p0.y } );
    outline.Append( { m_p0.x + m_w, m_p0.y + m_h } );
    outline.Append( { m_p0.x, m_p0.y + m_h } );
    outline.SetClosed( true );
}


const SHAPE_LINE_CHAIN SHAPE_RECT::Outline() const
{
    // TODO: we're DEPENDING on clients of this routine to use the actual arcs (if any)
    // inserted into the SHAPE_LINE_CHAIN.  They must NOT use the approximated segments
    // because we don't know what IUScale to generate them in.

    SHAPE_POLY_SET buffer;
    TransformToPolygon( buffer, SHAPE_ARC::DefaultAccuracyForPCB(), ERROR_INSIDE );
    return std::move( buffer.Outline( 0 ) );
}


void SHAPE_RECT::Normalize()
{
    // Ensure that the height and width are positive.
    if( m_w < 0 )
    {
        m_w = -m_w;
        m_p0.x -= m_w;
    }

    if( m_h < 0 )
    {
        m_h = -m_h;
        m_p0.y -= m_h;
    }
}
