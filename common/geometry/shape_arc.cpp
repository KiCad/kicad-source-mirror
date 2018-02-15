/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
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

#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>

bool SHAPE_ARC::Collide( const SEG& aSeg, int aClearance ) const
{
    int minDist = aClearance + m_width / 2;
    auto centerDist = aSeg.Distance( m_pc );

    if( centerDist < minDist )
        return true;

    auto ab = (aSeg.B - aSeg.A );
    auto ac = ( m_pc - aSeg.A );

    auto lenAbSq = ab.SquaredEuclideanNorm();

    auto lambda = (double) ac.Dot( ab ) / (double) lenAbSq;


    if( lambda >= 0.0 && lambda <= 1.0 )
    {
        VECTOR2I p;

        p.x = (double) aSeg.A.x * lambda + (double) aSeg.B.x * (1.0 - lambda);
        p.y = (double) aSeg.A.y * lambda + (double) aSeg.B.y * (1.0 - lambda);

        auto p0pdist = ( m_p0 - p ).EuclideanNorm();

        if( p0pdist < minDist )
            return true;

        auto p1pdist = ( m_p1 - p ).EuclideanNorm();

        if( p1pdist < minDist )
            return true;
    }

    auto p0dist = aSeg.Distance( m_p0 );

    if( p0dist > minDist )
        return true;

    auto p1dist = aSeg.Distance( m_p1 );

    if( p1dist > minDist )
        return false;


    return true;
}


bool SHAPE_ARC::ConstructFromCorners( const VECTOR2I& aP0, const VECTOR2I& aP1, double aCenterAngle )
{
    VECTOR2D mid = ( VECTOR2D( aP0 ) + VECTOR2D( aP1 ) ) * 0.5;
    VECTOR2D chord = VECTOR2D( aP1 ) - VECTOR2D( aP0 );
    double c = (aP1 - aP0).EuclideanNorm() / 2.0;
    VECTOR2D d = chord.Rotate( M_PI / 2.0 ).Resize( c );

    m_pc = mid + d * ( 1.0 / tan( aCenterAngle / 2.0 * M_PI / 180.0 ) );
    m_p0 = aP0;
    m_p1 = aP1;

    return true;
}


bool SHAPE_ARC::ConstructFromCornerAndAngles( const VECTOR2I& aP0,
        double aStartAngle,
        double aCenterAngle,
        double aRadius )
{
    m_p0 = aP0;
    auto d1 = VECTOR2D( 1.0, 0.0 ).Rotate( aStartAngle * M_PI / 180.0 ) * aRadius;
    auto d2 =
        VECTOR2D( 1.0, 0.0 ).Rotate( (aStartAngle + aCenterAngle) * M_PI / 180.0 ) * aRadius;

    m_pc = m_p0 - (VECTOR2I) d1;
    m_p1 = m_pc + (VECTOR2I) d2;

    if( aCenterAngle < 0 )
        std::swap( m_p0, m_p1 );

    return true;
}


bool SHAPE_ARC::Collide( const VECTOR2I& aP, int aClearance ) const
{
    assert( false );
    return false;
}


double SHAPE_ARC::GetStartAngle() const
{
    VECTOR2D d( m_p0 - m_pc );

    return 180.0 / M_PI * atan2( d.y, d.x );
}


double SHAPE_ARC::GetEndAngle() const
{
    VECTOR2D d( m_p1 - m_pc );

    return 180.0 / M_PI * atan2( d.y, d.x );
}


double SHAPE_ARC::GetCentralAngle() const
{
    auto ea = GetEndAngle();
    auto sa = GetStartAngle();

    if( ea < sa )
        ea += 360.0;

    while( sa < 0.0 )
    {
        sa  += 360.0;
        ea  += 360.0;
    }

    return ea - sa;
}


SHAPE_LINE_CHAIN SHAPE_ARC::ConvertToPolyline( double aAccuracy ) const
{
    SHAPE_LINE_CHAIN rv;
    double ca = GetCentralAngle();
    double r = GetRadius();
    double step;
    auto c = GetCenter();
    int n;

    if( r == 0.0 )
        return rv;

    step = 180.0 / M_PI * acos( r * ( 1.0 - aAccuracy ) / r );
    n = (int) ceil( ca / step );

    for( int i = 0; i <= n ; i++ )
    {
        double a = GetStartAngle() + ca * (double) i / (double) n;
        double x = c.x + r * cos( a * M_PI / 180.0 );
        double y = c.y + r * sin( a * M_PI / 180.0 );

        rv.Append( (int) x, (int) y );
    }

    return rv;
}
