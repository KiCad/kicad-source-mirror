/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>        // for min
#include <geometry/seg.h>
#include <math/util.h>      // for rescale
#include <math/vector2d.h>  // for VECTOR2I, VECTOR2
#include <trigo.h>          // for RAD2DEG

template <typename T>
int sgn( T aVal )
{
    return ( T( 0 ) < aVal ) - ( aVal < T( 0 ) );
}


SEG::ecoord SEG::SquaredDistance( const SEG& aSeg ) const
{
    if( Intersects( aSeg ) )
        return 0;

    const VECTOR2I pts[4] =
    {
        aSeg.NearestPoint( A ) - A,
        aSeg.NearestPoint( B ) - B,
        NearestPoint( aSeg.A ) - aSeg.A,
        NearestPoint( aSeg.B ) - aSeg.B
    };

    ecoord m = VECTOR2I::ECOORD_MAX;

    for( int i = 0; i < 4; i++ )
        m = std::min( m, pts[i].SquaredEuclideanNorm() );

    return m;
}


double SEG::AngleDegrees( const SEG& aOther ) const
{
    VECTOR2I thisVec = A - B;
    VECTOR2I otherVec = aOther.A - aOther.B;

    double thisVecAngle = NormalizeAngle180( RAD2DECIDEG( thisVec.Angle() ) );
    double otherVecAngle = NormalizeAngle180( RAD2DECIDEG( otherVec.Angle() ) );
    double angleDegrees = std::abs( NormalizeAngle180( thisVecAngle - otherVecAngle ) ) / 10.0;

    return std::min( 180.0 - angleDegrees, angleDegrees );
}


const VECTOR2I SEG::NearestPoint( const SEG& aSeg ) const
{
    if( OPT_VECTOR2I p = Intersect( aSeg ) )
        return *p;

    const VECTOR2I pts_origin[4] =
    {
        aSeg.NearestPoint( A ),
        aSeg.NearestPoint( B ),
        NearestPoint( aSeg.A ),
        NearestPoint( aSeg.B )
    };


    const VECTOR2I* pts_out[4] =
    {
        &A,
        &B,
        &pts_origin[2],
        &pts_origin[3]
    };

    const ecoord pts_dist[4] =
    {
        ( pts_origin[0] - A ).SquaredEuclideanNorm(),
        ( pts_origin[1] - B ).SquaredEuclideanNorm(),
        ( pts_origin[2] - aSeg.A ).SquaredEuclideanNorm(),
        ( pts_origin[3] - aSeg.B ).SquaredEuclideanNorm()
    };

    int min_i = 0;

    for( int i = 0; i < 4; i++ )
    {
        if( pts_dist[i] < pts_dist[min_i] )
            min_i = i;
    }

    return *pts_out[min_i];
}


bool SEG::intersects( const SEG& aSeg, bool aIgnoreEndpoints, bool aLines, VECTOR2I* aPt ) const
{
    const VECTOR2I  e( B - A );
    const VECTOR2I  f( aSeg.B - aSeg.A );
    const VECTOR2I  ac( aSeg.A - A );

    ecoord d = f.Cross( e );
    ecoord p = f.Cross( ac );
    ecoord q = e.Cross( ac );

    if( d == 0 )
        return false;

    if( !aLines && d > 0 && ( q < 0 || q > d || p < 0 || p > d ) )
        return false;

    if( !aLines && d < 0 && ( q < d || p < d || p > 0 || q > 0 ) )
        return false;

    if( !aLines && aIgnoreEndpoints && ( q == 0 || q == d ) && ( p == 0 || p == d ) )
        return false;

    if( aPt )
    {
        *aPt = VECTOR2I( aSeg.A.x + rescale( q, (ecoord) f.x, d ),
                         aSeg.A.y + rescale( q, (ecoord) f.y, d ) );
    }

    return true;
}


bool SEG::Intersects( const SEG& aSeg ) const
{
    return intersects( aSeg );
}


OPT_VECTOR2I SEG::Intersect( const SEG& aSeg, bool aIgnoreEndpoints, bool aLines ) const
{
    VECTOR2I ip;

    if( intersects( aSeg, aIgnoreEndpoints, aLines, &ip ) )
        return ip;
    else
        return OPT_VECTOR2I();
}


SEG SEG::PerpendicularSeg( const VECTOR2I& aP ) const
{
    VECTOR2I slope( B - A );
    VECTOR2I endPoint = slope.Perpendicular() + aP;

    return SEG( aP, endPoint );
}


SEG SEG::ParallelSeg( const VECTOR2I& aP ) const
{
    VECTOR2I slope( B - A );
    VECTOR2I endPoint = slope + aP;

    return SEG( aP, endPoint );
}


bool SEG::ccw( const VECTOR2I& aA, const VECTOR2I& aB, const VECTOR2I& aC ) const
{
    return (ecoord) ( aC.y - aA.y ) * ( aB.x - aA.x ) > (ecoord) ( aB.y - aA.y ) * ( aC.x - aA.x );
}


bool SEG::Collide( const SEG& aSeg, int aClearance, int* aActual ) const
{
    // check for intersection
    // fixme: move to a method
    if( ccw( A, aSeg.A, aSeg.B ) != ccw( B, aSeg.A, aSeg.B ) &&
            ccw( A, B, aSeg.A ) != ccw( A, B, aSeg.B ) )
    {
        if( aActual )
            *aActual = 0;

        return true;
    }

    ecoord dist_sq = VECTOR2I::ECOORD_MAX;

    dist_sq = std::min( dist_sq, SquaredDistance( aSeg.A ) );
    dist_sq = std::min( dist_sq, SquaredDistance( aSeg.B ) );
    dist_sq = std::min( dist_sq, aSeg.SquaredDistance( A ) );
    dist_sq = std::min( dist_sq, aSeg.SquaredDistance( B ) );

    if( dist_sq == 0 || dist_sq < (ecoord) aClearance * aClearance )
    {
        if( aActual )
            *aActual = sqrt( dist_sq );

        return true;
    }

    return false;
}


bool SEG::Contains( const VECTOR2I& aP ) const
{
    return Distance( aP ) <= 1;
}


const VECTOR2I SEG::NearestPoint( const VECTOR2I& aP ) const
{
    VECTOR2I d = B - A;
    ecoord   l_squared = d.Dot( d );

    if( l_squared == 0 )
        return A;

    ecoord t = d.Dot( aP - A );

    if( t < 0 )
        return A;
    else if( t > l_squared )
        return B;

    int xp = rescale( t, (ecoord) d.x, l_squared );
    int yp = rescale( t, (ecoord) d.y, l_squared );

    return A + VECTOR2I( xp, yp );
}


const VECTOR2I SEG::ReflectPoint( const VECTOR2I& aP ) const
{
    VECTOR2I                d = B - A;
    VECTOR2I::extended_type l_squared = d.Dot( d );
    VECTOR2I::extended_type t = d.Dot( aP - A );
    VECTOR2I                c;

    if( !l_squared )
        c = aP;
    else
    {
        c.x = A.x + rescale( t, static_cast<VECTOR2I::extended_type>( d.x ), l_squared );
        c.y = A.y + rescale( t, static_cast<VECTOR2I::extended_type>( d.y ), l_squared );
    }

    return 2 * c - aP;
}


VECTOR2I SEG::LineProject( const VECTOR2I& aP ) const
{
    VECTOR2I d = B - A;
    ecoord   l_squared = d.Dot( d );

    if( l_squared == 0 )
        return A;

    ecoord t = d.Dot( aP - A );

    int xp = rescale( t, ecoord{ d.x }, l_squared );
    int yp = rescale( t, ecoord{ d.y }, l_squared );

    return A + VECTOR2I( xp, yp );
}