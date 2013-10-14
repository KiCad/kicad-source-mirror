/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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


#include <geometry/seg.h>

template<typename T> int sgn( T val ) {
    return ( T( 0 ) < val ) - ( val < T( 0 ) );
}


bool SEG::PointCloserThan( const VECTOR2I& aP, int dist ) const
{
    VECTOR2I         d = b - a;
    ecoord dist_sq = (ecoord) dist * dist;

    SEG::ecoord l_squared = d.Dot( d );
    SEG::ecoord t = d.Dot( aP - a );

    if( t <= 0 || !l_squared )
        return ( aP - a ).SquaredEuclideanNorm() < dist_sq;
    else if( t >= l_squared )
        return ( aP - b ).SquaredEuclideanNorm() < dist_sq;

    int dxdy = abs( d.x ) - abs( d.y );

    if( ( dxdy >= -1 && dxdy <= 1 ) || abs( d.x ) <= 1 || abs( d.y ) <= 1 )
    {
        int ca = -sgn( d.y );
        int cb = sgn( d.x );
        int cc = -ca * a.x - cb * a.y;

        ecoord num = ca * aP.x + cb * aP.y + cc;
        num *= num;

        if( ca && cb )
            num >>= 1;

        if( num > ( dist_sq + 100 ) )
            return false;
        else if( num < ( dist_sq - 100 ) )
            return true;
    }

    VECTOR2I nearest;
    nearest.x = a.x + rescale( t, (ecoord)d.x, l_squared );
    nearest.y = a.y + rescale( t, (ecoord)d.y, l_squared );

    return ( nearest - aP ).SquaredEuclideanNorm() <= dist_sq;
}


SEG::ecoord SEG::SquaredDistance( const SEG& aSeg ) const
{
    // fixme: rather inefficient....
    if( Intersect( aSeg ) )
        return 0;

    const VECTOR2I pts[4] =
    {
        aSeg.NearestPoint( a ) - a,
        aSeg.NearestPoint( b ) - b,
        NearestPoint( aSeg.a ) - aSeg.a,
        NearestPoint( aSeg.b ) - aSeg.b
    };

    ecoord m = VECTOR2I::ECOORD_MAX;
    for( int i = 0; i < 4; i++ )
        m = std::min( m, pts[i].SquaredEuclideanNorm() );

    return m;
}


OPT_VECTOR2I SEG::Intersect( const SEG& aSeg, bool aIgnoreEndpoints, bool aLines ) const
{
    const VECTOR2I e ( b - a );
    const VECTOR2I f ( aSeg.b - aSeg.a );
    const VECTOR2I ac ( aSeg.a - a );

    ecoord d = f.Cross( e );
    ecoord p = f.Cross( ac );
    ecoord q = e.Cross( ac );

    if( d == 0 )
        return OPT_VECTOR2I();
    if ( !aLines && d > 0 && ( q < 0 || q > d || p < 0 || p > d ) )
        return OPT_VECTOR2I();
    if ( !aLines && d < 0 && ( q < d || p < d || p > 0 || q > 0 ) )
        return OPT_VECTOR2I();
    if ( !aLines && aIgnoreEndpoints && ( q == 0 || q == d ) && ( p == 0 || p == d ) )
        return OPT_VECTOR2I();

     VECTOR2I ip( aSeg.a.x + rescale( q, (ecoord)f.x, d ),
                  aSeg.a.y + rescale( q, (ecoord)f.y, d ) );

     return ip;
}


bool SEG::ccw( const VECTOR2I& a, const VECTOR2I& b, const VECTOR2I& c ) const
{
    return (ecoord)( c.y - a.y ) * ( b.x - a.x ) > (ecoord)( b.y - a.y ) * ( c.x - a.x );
}


bool SEG::Collide( const SEG& aSeg, int aClearance ) const
{
    // check for intersection
    // fixme: move to a method
    if( ccw( a, aSeg.a, aSeg.b ) != ccw( b, aSeg.a, aSeg.b ) &&
            ccw( a, b, aSeg.a ) != ccw( a, b, aSeg.b ) )
        return true;

#define CHK(_seg, _pt) \
    if( (_seg).PointCloserThan (_pt, aClearance ) ) return true;

    CHK( *this, aSeg.a );
    CHK( *this, aSeg.b );
    CHK( aSeg, a );
    CHK( aSeg, b );
#undef CHK

    return false;
}


bool SEG::Contains( const VECTOR2I& aP ) const
{
    return PointCloserThan( aP, 1 );
}
