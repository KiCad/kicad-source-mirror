/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
  T sqrt_max = sqrt_max_typed<T>;

  if( x < 0 )
    return sqrt_max;

  T r = (T) std::sqrt( (double) x );

  while( r < sqrt_max && r * r < x )
    r++;

  while( r > sqrt_max || r * r > x )
    r--;

  return r;
}


SEG::ecoord SEG::SquaredDistance( const SEG& aSeg ) const
{
    // Handle zero-length segments (points) specially.
    // The Intersects() check below doesn't handle this case correctly because
    // the cross product with a zero vector is always zero, causing false positives.
    if( A == B )
        return aSeg.SquaredDistance( A );

    if( aSeg.A == aSeg.B )
        return SquaredDistance( aSeg.A );

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

    return std::abs( ( thisAngle - otherAngle ).Normalize180() );
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


bool SEG::NearestPoints( const SEG& aSeg, VECTOR2I& aPtA, VECTOR2I& aPtB, int64_t& aDistSq ) const
{
    if( OPT_VECTOR2I p = Intersect( aSeg ) )
    {
        aPtA = aPtB = *p;
        aDistSq = 0;

        return true;
    }

    const VECTOR2I pts_origin[4] =
    {
        aSeg.NearestPoint( A ),
        aSeg.NearestPoint( B ),
        NearestPoint( aSeg.A ),
        NearestPoint( aSeg.B )
    };

    const VECTOR2I* pts_a_out[4] =
    {
        &A,
        &B,
        &pts_origin[2],
        &pts_origin[3]
    };

    const VECTOR2I* pts_b_out[4] =
    {
        &pts_origin[0],
        &pts_origin[1],
        &aSeg.A,
        &aSeg.B
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

    aPtA = *pts_a_out[min_i];
    aPtB = *pts_b_out[min_i];
    aDistSq = pts_dist[min_i];

    return true;
}


bool SEG::checkCollinearOverlap( const SEG& aSeg, bool useXAxis, bool aIgnoreEndpoints, VECTOR2I* aPt ) const
{
    // Extract coordinates based on the chosen axis
    int seg1_start, seg1_end, seg2_start, seg2_end;
    int coord1_start, coord1_end; // For calculating other axis coordinate

    if( useXAxis )
    {
        seg1_start = A.x;       seg1_end = B.x;
        seg2_start = aSeg.A.x;  seg2_end = aSeg.B.x;
        coord1_start = A.y;     coord1_end = B.y;
    }
    else
    {
        seg1_start = A.y;       seg1_end = B.y;
        seg2_start = aSeg.A.y;  seg2_end = aSeg.B.y;
        coord1_start = A.x;     coord1_end = B.x;
    }

    // Find segment ranges on the projection axis
    const int seg1_min = std::min( seg1_start, seg1_end );
    const int seg1_max = std::max( seg1_start, seg1_end );
    const int seg2_min = std::min( seg2_start, seg2_end );
    const int seg2_max = std::max( seg2_start, seg2_end );

    // Check for overlap
    const bool overlaps = seg1_max >= seg2_min && seg2_max >= seg1_min;
    if( !overlaps )
        return false;

    // Check if intersection is only at endpoints when aIgnoreEndpoints is true
    if( aIgnoreEndpoints )
    {
        // Calculate overlap region
        const int overlap_start = std::max( seg1_min, seg2_min );
        const int overlap_end = std::min( seg1_max, seg2_max );

        // If overlap region has zero length, segments only touch at endpoint
        if( overlap_start == overlap_end )
        {
            // Check if this endpoint touching involves actual segment endpoints
            // (not just projected endpoints due to min/max calculation)
            bool isEndpointTouch = false;

            // Check if the touch point corresponds to actual segment endpoints
            if( overlap_start == seg1_min || overlap_start == seg1_max )
            {
                // Touch point is at seg1's endpoint, check if it's also at seg2's endpoint
                if( overlap_start == seg2_min || overlap_start == seg2_max )
                {
                    isEndpointTouch = true;
                }
            }

            if( isEndpointTouch )
                return false;  // Ignore endpoint-only intersection
        }
    }

    // Calculate intersection point if requested
    if( aPt )
    {
        // Find midpoint of overlap region
        const int overlap_start = std::max( seg1_min, seg2_min );
        const int overlap_end = std::min( seg1_max, seg2_max );
        const int intersection_proj = ( overlap_start + overlap_end ) / 2;

        // Calculate corresponding coordinate on the other axis
        int intersection_other;
        if( seg1_end != seg1_start )
        {
            // Use this segment's line equation to find other coordinate
            intersection_other = coord1_start + static_cast<int>(
                rescale( intersection_proj - seg1_start, coord1_end - coord1_start, seg1_end - seg1_start ) );
        }
        else
        {
            // Degenerate segment (point) or perpendicular to projection axis
            intersection_other = coord1_start;
        }

        // Set result based on projection axis
        if( useXAxis )
            *aPt = VECTOR2I( intersection_proj, intersection_other );
        else
            *aPt = VECTOR2I( intersection_other, intersection_proj );
    }

    return true;
}


bool SEG::intersects( const SEG& aSeg, bool aIgnoreEndpoints, bool aLines, VECTOR2I* aPt ) const
{
    // Quick rejection: check if segment bounding boxes overlap
    // (Skip for line mode since infinite lines can intersect anywhere)
    if( !aLines )
    {
        const int this_min_x = std::min( A.x, B.x );
        const int this_max_x = std::max( A.x, B.x );
        const int this_min_y = std::min( A.y, B.y );
        const int this_max_y = std::max( A.y, B.y );

        const int other_min_x = std::min( aSeg.A.x, aSeg.B.x );
        const int other_max_x = std::max( aSeg.A.x, aSeg.B.x );
        const int other_min_y = std::min( aSeg.A.y, aSeg.B.y );
        const int other_max_y = std::max( aSeg.A.y, aSeg.B.y );

        if( this_max_x < other_min_x || other_max_x < this_min_x ||
            this_max_y < other_min_y || other_max_y < this_min_y )
        {
            return false;
        }
    }

    // Calculate direction vectors and offset vector using VECTOR2 operations
    // Using parametric form: P₁ = A + t*dir1, P₂ = aSeg.A + s*dir2
    const VECTOR2L dir1 = VECTOR2L( B ) - A;           // direction vector e
    const VECTOR2L dir2 = VECTOR2L( aSeg.B ) - aSeg.A; // direction vector f
    const VECTOR2L offset = VECTOR2L( aSeg.A ) - A;    // offset vector ac
    const ecoord determinant = dir2.Cross( dir1 );

    // Handle parallel/collinear case
    if( determinant == 0 )
    {
        // Check if lines are collinear (not just parallel) using cross product
        // Lines are collinear if offset vector is also parallel to direction vector
        const ecoord collinear_test = dir1.Cross( offset );

        if( collinear_test != 0 )
            return false;  // Parallel but not collinear

        // Lines are collinear - for infinite lines, they always intersect
        if( aLines )
        {
            // For infinite collinear lines, intersection point is ambiguous
            // Use the midpoint between the two segment start points as a reasonable choice
            if( aPt )
            {
                // If aSeg is degenerate (point), use its start point
                if( aSeg.A == aSeg.B )
                {
                    *aPt = aSeg.A;
                }
                else if( A == B )
                { // If this segment is degenerate (point), use its start point
                    *aPt = A;
                }
                else
                {
                    const VECTOR2I midpoint = ( A + aSeg.A ) / 2;
                    *aPt = midpoint;
                }
            }
            return true;
        }

        // For segments, check overlap using the axis with larger coordinate range
        const bool use_x_axis = std::abs( dir1.x ) >= std::abs( dir1.y );
        return checkCollinearOverlap( aSeg, use_x_axis, aIgnoreEndpoints, aPt );
    }

    // param2_num = f × ac (parameter for second segment: s = p/d)
    // param1_num = e × ac (parameter for first segment: t = q/d)
    const ecoord param2_num = dir2.Cross( offset );
    const ecoord param1_num = dir1.Cross( offset );

    // For segments (not infinite lines), check if intersection is within both segments
    if( !aLines )
    {
        // Parameters must be in [0,1] for intersection within segments
        // Since we're comparing t = q/d and s = p/d to [0,1], we need to handle sign of d
        if( determinant > 0 )
        {
            // d > 0: check 0 ≤ q ≤ d and 0 ≤ p ≤ d
            if( param1_num < 0 || param1_num > determinant ||
                param2_num < 0 || param2_num > determinant )
                return false;
        }
        else
        {
            // d < 0: check d ≤ q ≤ 0 and d ≤ p ≤ 0
            if( param1_num > 0 || param1_num < determinant ||
                param2_num > 0 || param2_num < determinant )
                return false;
        }

        // Optionally exclude endpoint intersections (when segments share vertices)
        if( aIgnoreEndpoints &&
            ( param1_num == 0 || param1_num == determinant ) &&
            ( param2_num == 0 || param2_num == determinant ) )
        {
            return false;
        }
    }

    if( aPt )
    {
        // Use parametric equation: intersection = aSeg.A + (q/d) * f
        const VECTOR2L scaled_dir2( rescale( param1_num, dir2.x, determinant ),
                                   rescale( param1_num, dir2.y, determinant ) );
        const VECTOR2L result = VECTOR2L( aSeg.A ) + scaled_dir2;

        // Verify result fits in coordinate type range
        constexpr ecoord max_coord = std::numeric_limits<VECTOR2I::coord_type>::max();
        constexpr ecoord min_coord = std::numeric_limits<VECTOR2I::coord_type>::min();

        if( result.x > max_coord || result.x < min_coord ||
            result.y > max_coord || result.y < min_coord )
        {
            return false;  // Intersection exists but coordinates overflow
        }

        *aPt = VECTOR2I( static_cast<int>( result.x ), static_cast<int>( result.y ) );
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


bool SEG::IntersectsLine( double aSlope, double aOffset, VECTOR2I& aIntersection ) const
{
    const VECTOR2L segA( A );
    const VECTOR2L segB( B );
    const VECTOR2L segDir = segB - segA;

    // Handle vertical segment case
    if( segDir.x == 0 )
    {
        // Vertical segment: x = A.x, find y on the line
        const double intersect_y = aSlope * A.x + aOffset;
        const int intersect_y_int = KiROUND( intersect_y );

        // Check if intersection is within segment's y-range
        const int seg_min_y = std::min( A.y, B.y );
        const int seg_max_y = std::max( A.y, B.y );

        if( intersect_y_int >= seg_min_y && intersect_y_int <= seg_max_y )
        {
            aIntersection = VECTOR2I( A.x, intersect_y_int );
            return true;
        }
        return false;
    }

    const VECTOR2L lineDir( 1000, static_cast<ecoord>( aSlope * 1000 ) );
    const ecoord cross_product = segDir.Cross( lineDir );

    if( cross_product == 0 )
    {
        // Parallel lines - check if segment lies on the line
        const double expected_y = aSlope * A.x + aOffset;
        const double diff = std::abs( A.y - expected_y );

        if( diff < 0.5 )
        {
            // Collinear: segment lies on the line, return midpoint
            aIntersection = ( A + B ) / 2;
            return true;
        }

        return false; // Parallel but not collinear
    }

    // Find intersection using parametric equations
    // Segment: P = segA + t * segDir
    // Line: y = aSlope * x + aOffset
    //
    // At intersection: segA.y + t * segDir.y = aSlope * (segA.x + t * segDir.x) + aOffset
    // Solving for t: t = (aSlope * segA.x + aOffset - segA.y) / (segDir.y - aSlope * segDir.x)

    const double numerator = aSlope * segA.x + aOffset - segA.y;
    const double denominator = segDir.y - aSlope * segDir.x;

    const double t = numerator / denominator;

    // Check if intersection is within segment bounds
    if( t >= 0.0 && t <= 1.0 )
    {
        aIntersection = KiROUND( segA.x + t * segDir.x, segA.y + t * segDir.y );
        return true;
    }

    return false;
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


bool SEG::Collide( const SEG& aSeg, int aClearance, int* aActual ) const
{
    // Handle negative clearance
    if( aClearance < 0 )
    {
        if( aActual )
            *aActual = 0;

        return false;
    }

    // Handle zero-length segments (points) specially.
    // The intersects() check below doesn't handle this case correctly because
    // the cross product with a zero vector is always zero, causing false positives.
    if( A == B )
    {
        int dist = aSeg.Distance( A );

        if( aActual )
            *aActual = dist;

        return dist == 0 || dist < aClearance;
    }

    if( aSeg.A == aSeg.B )
    {
        int dist = Distance( aSeg.A );

        if( aActual )
            *aActual = dist;

        return dist == 0 || dist < aClearance;
    }

    // Check for exact intersection first
    if( intersects( aSeg, false, false ) )
    {
        if( aActual )
            *aActual = 0;

        return true;
    }

    const ecoord clearance_sq = static_cast<ecoord>( aClearance ) * aClearance;
    ecoord min_dist_sq = VECTOR2I::ECOORD_MAX;

    auto checkDistance = [&]( ecoord dist, ecoord& min_dist ) -> bool
    {
        if( dist == 0 )
        {
            if( aActual )
                *aActual = 0;

            return true;
        }

        min_dist = std::min( min_dist, dist );
        return false; // Continue checking
    };

    // There are 4 points to check: start and end of this segment, and
    // start and end of the other segment.
    if( checkDistance( SquaredDistance( aSeg.A ), min_dist_sq ) ||
        checkDistance( SquaredDistance( aSeg.B ), min_dist_sq ) ||
        checkDistance( aSeg.SquaredDistance( A ), min_dist_sq ) ||
        checkDistance( aSeg.SquaredDistance( B ), min_dist_sq ) )
    {
        return true;
    }

    if( min_dist_sq < clearance_sq )
    {
        if( aActual )
            *aActual = static_cast<int>( isqrt( min_dist_sq ) );

        return true;
    }

    if( aActual )
        *aActual = static_cast<int>( isqrt( min_dist_sq ) );

    return false;
}


bool SEG::Contains( const VECTOR2I& aP ) const
{
    return SquaredDistance( aP ) <= 3;
}


const VECTOR2I SEG::NearestPoint( const VECTOR2I& aP ) const
{
    // Inlined for performance reasons
    VECTOR2L d;
    d.x = static_cast<int64_t>( B.x ) - A.x;
    d.y = static_cast<int64_t>( B.y ) - A.y;

    ecoord   l_squared( d.x * d.x + d.y * d.y );

    if( l_squared == 0 )
        return A;

    // Inlined for performance reasons
    VECTOR2L pa;
    pa.x = static_cast<int64_t>( aP.x ) - A.x;
    pa.y = static_cast<int64_t>( aP.y ) - A.y;

    ecoord t = d.Dot( pa );

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


SEG::ecoord SEG::SquaredDistance( const VECTOR2I& aP ) const
{
    VECTOR2<ecoord> ab( ecoord( B.x ) - A.x, ecoord( B.y ) - A.y );
    VECTOR2<ecoord> ap( ecoord( aP.x ) - A.x, ecoord( aP.y ) - A.y );

    ecoord e = ap.Dot( ab );

    if( e <= 0 )
        return ap.SquaredEuclideanNorm();

    ecoord f = ab.SquaredEuclideanNorm();

    if( e >= f )
    {
        VECTOR2<ecoord> bp( ecoord( aP.x ) - B.x, ecoord( aP.y ) - B.y );

        return bp.Dot( bp );
    }

    const double g = ap.SquaredEuclideanNorm() - ( double( e ) * e ) / f;

    // The only way g can be negative is if there was a rounding error since
    // e is the projection of aP onto ab and therefore cannot be greater than
    // the length of ap and f is guaranteed to be greater than e, meaning
    // e * e / f cannot be greater than ap.SquaredEuclideanNorm()
    if( g < 0 || g > static_cast<double>( std::numeric_limits<ecoord>::max() ) )
        return 0;

    return KiROUND<double, ecoord>( g );
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

    return static_cast<int>( aDetermineSide ? sgn( det ) * dist : std::abs( dist ) );
}


bool SEG::mutualDistanceSquared( const SEG& aSeg, ecoord& aD1, ecoord& aD2 ) const
{
    SEG a( *this );
    SEG b( aSeg );

    if( a.SquaredLength() < b.SquaredLength() )
        std::swap(a, b);

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

    aD1 = sgn( det1 ) * dsq1;
    aD2 = sgn( det2 ) * dsq2;

    return true;
}

bool SEG::ApproxCollinear( const SEG& aSeg, int aDistanceThreshold ) const
{
    ecoord thresholdSquared = Square( aDistanceThreshold );
    ecoord d1_sq, d2_sq;

    if( !mutualDistanceSquared( aSeg, d1_sq, d2_sq ) )
        return false;

    return std::abs( d1_sq ) <= thresholdSquared && std::abs( d2_sq ) <= thresholdSquared;
}


bool SEG::ApproxParallel( const SEG& aSeg, int aDistanceThreshold ) const
{
    ecoord thresholdSquared = Square( aDistanceThreshold );
    ecoord d1_sq, d2_sq;

    if( ! mutualDistanceSquared( aSeg, d1_sq, d2_sq ) )
        return false;

    return std::abs( d1_sq - d2_sq ) <= thresholdSquared;
}


bool SEG::ApproxPerpendicular( const SEG& aSeg ) const
{
    SEG perp = PerpendicularSeg( A );

    return aSeg.ApproxParallel( perp );
}
