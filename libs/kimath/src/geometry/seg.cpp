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

template <typename T>
constexpr T sqrt_helper(T x, T lo, T hi)
{
  if (lo == hi)
    return lo;

  const T mid = (lo + hi + 1) / 2;
  if (x / mid < mid)
    return sqrt_helper<T>(x, lo, mid - 1);
  else
    return sqrt_helper(x, mid, hi);
}

template <typename T>
constexpr T ct_sqrt(T x)
{
  return sqrt_helper<T>(x, 0, x / 2 + 1);
}

template <typename T>
static constexpr T sqrt_max_typed = ct_sqrt( std::numeric_limits<T>::max() );

template <typename T>
T isqrt(T x)
{
  T r = (T) std::sqrt((double) x);
  T sqrt_max = sqrt_max_typed<T>;

  while (r < sqrt_max && r * r < x)
    r++;
  while (r > sqrt_max || r * r > x)
    r--;

  return r;
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


EDA_ANGLE SEG::Angle( const SEG& aOther ) const
{
    EDA_ANGLE thisAngle = EDA_ANGLE( A - B ).Normalize180();
    EDA_ANGLE otherAngle = EDA_ANGLE( aOther.A - aOther.B ).Normalize180();

    EDA_ANGLE angle = std::abs( ( thisAngle - otherAngle ).Normalize180() );

    return std::min( ANGLE_180 - angle, angle );
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
    const VECTOR2<ecoord> e = VECTOR2<ecoord>( B ) - A;
    const VECTOR2<ecoord> f = VECTOR2<ecoord>( aSeg.B ) - aSeg.A;
    const VECTOR2<ecoord> ac = VECTOR2<ecoord>( aSeg.A ) - A;

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
        VECTOR2<ecoord> result( aSeg.A.x + rescale( q, (ecoord) f.x, d ),
                                aSeg.A.y + rescale( q, (ecoord) f.y, d ) );

        if( abs( result.x ) > std::numeric_limits<VECTOR2I::coord_type>::max()
            || abs( result.y ) > std::numeric_limits<VECTOR2I::coord_type>::max() )
        {
            return false;
        }

        *aPt = VECTOR2I( (int) result.x, (int) result.y );
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
            *aActual = int( isqrt( dist_sq ) );

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

    ecoord xp = rescale( t, (ecoord) d.x, l_squared );
    ecoord yp = rescale( t, (ecoord) d.y, l_squared );

    return VECTOR2<ecoord>( A.x + xp, A.y + yp );
}


const VECTOR2I SEG::ReflectPoint( const VECTOR2I& aP ) const
{
    VECTOR2I                d = B - A;
    VECTOR2I::extended_type l_squared = d.Dot( d );
    VECTOR2I::extended_type t = d.Dot( aP - A );
    VECTOR2<ecoord>         c;

    if( !l_squared )
    {
        c = aP;
    }
    else
    {
        c.x = A.x + rescale( t, static_cast<VECTOR2I::extended_type>( d.x ), l_squared );
        c.y = A.y + rescale( t, static_cast<VECTOR2I::extended_type>( d.y ), l_squared );
    }

    return VECTOR2<ecoord>( 2 * c.x - aP.x, 2 * c.y - aP.y );
}


VECTOR2I SEG::LineProject( const VECTOR2I& aP ) const
{
    VECTOR2I d = B - A;
    ecoord   l_squared = d.Dot( d );

    if( l_squared == 0 )
        return A;

    ecoord t = d.Dot( aP - A );

    ecoord xp = rescale( t, ecoord{ d.x }, l_squared );
    ecoord yp = rescale( t, ecoord{ d.y }, l_squared );

    return VECTOR2<ecoord>( A.x + xp, A.y + yp );
}


int SEG::Distance( const SEG& aSeg ) const
{
    return int( isqrt( SquaredDistance( aSeg ) ) );
}


int SEG::Distance( const VECTOR2I& aP ) const
{
    return int( isqrt( SquaredDistance( aP ) ) );
}


int SEG::LineDistance( const VECTOR2I& aP, bool aDetermineSide ) const
{
    ecoord p = ecoord{ A.y } - B.y;
    ecoord q = ecoord{ B.x } - A.x;
    ecoord r = -p * A.x - q * A.y;
    ecoord l = p * p + q * q;
    ecoord det = p * aP.x + q * aP.y + r;
    ecoord dist_sq = 0;

    if( l > 0 )
    {
        dist_sq  = rescale( det, det, l );
    }

    ecoord dist = isqrt( dist_sq );

    return static_cast<int>( aDetermineSide ? dist : std::abs( dist ) );
}


bool SEG::mutualDistance( const SEG& aSeg, ecoord& aD1, ecoord& aD2 ) const
{
    SEG a( *this );
    SEG b( aSeg );

    if( a.SquaredLength() < b.SquaredLength() )
    {
        std::swap(a, b);
    }

    ecoord p = ecoord{ a.A.y } - a.B.y;
    ecoord q = ecoord{ a.B.x } - a.A.x;
    ecoord r = -p * a.A.x - q * a.A.y;

    ecoord l = p * p + q * q;

    if( l == 0 )
        return false;

    ecoord det1 = p * b.A.x + q * b.A.y + r;
    ecoord det2 = p * b.B.x + q * b.B.y + r;

    ecoord dsq1 = rescale( det1, det1, l );
    ecoord dsq2 = rescale( det2, det2, l );

    aD1 = sgn( det1 ) * isqrt( dsq1 );
    aD2 = sgn( det2 ) * isqrt( dsq2 );

    return true;
}

bool SEG::ApproxCollinear( const SEG& aSeg, int aDistanceThreshold ) const
{
    ecoord d1, d2;

    if( ! mutualDistance( aSeg, d1, d2 ) )
        return false;

    return std::abs( d1 ) <= aDistanceThreshold && std::abs( d2 ) <= aDistanceThreshold;
}


bool SEG::ApproxParallel( const SEG& aSeg, int aDistanceThreshold ) const
{
    ecoord d1, d2;

    if( ! mutualDistance( aSeg, d1, d2 ) )
        return false;

    return std::abs( d1 - d2 ) <= (ecoord) aDistanceThreshold;
}


bool SEG::ApproxPerpendicular( const SEG& aSeg ) const
{
    SEG perp = PerpendicularSeg( A );

    return aSeg.ApproxParallel( perp );
}
