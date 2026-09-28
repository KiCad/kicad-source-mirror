/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <geometry/arc_construction.h>

#include <cmath>
#include <limits>

#include <math/util.h>


namespace KIGEOM
{

namespace
{

/// Cross product z with the rounding error compensated, for near-collinear tests on large coordinates
double crossZ( double aX, double aY, double bX, double bY )
{
    const double p = aY * bX;
    const double e = std::fma( aY, bX, -p );
    return std::fma( aX, bY, -p ) - e;
}


/// Round to internal units, failing when the value does not fit
bool toIU( const VECTOR2D& aPt, VECTOR2I& aOut )
{
    constexpr double limit = std::numeric_limits<int>::max() - 1;

    if( !std::isfinite( aPt.x ) || !std::isfinite( aPt.y ) || std::abs( aPt.x ) > limit
        || std::abs( aPt.y ) > limit )
    {
        return false;
    }

    aOut = VECTOR2I( KiROUND( aPt.x ), KiROUND( aPt.y ) );
    return true;
}


// aSide is +1 for an arc on the (-dy, dx) side of start to end, and aCenterT is the center offset along that normal
ARC_SOLUTION buildOnChord( const VECTOR2I& aStart, const VECTOR2I& aEnd, double aSide, double aCenterT,
                           double aRadius )
{
    ARC_SOLUTION sol;
    sol.start = aStart;
    sol.end = aEnd;

    const VECTOR2D chord( static_cast<double>( aEnd.x ) - aStart.x, static_cast<double>( aEnd.y ) - aStart.y );
    const double   len = chord.EuclideanNorm();

    if( len <= 0.0 || !std::isfinite( aRadius ) || aRadius <= 0.0 )
        return sol;

    const VECTOR2D normal( -chord.y / len, chord.x / len );
    const VECTOR2D chordMid( ( static_cast<double>( aStart.x ) + aEnd.x ) * 0.5,
                             ( static_cast<double>( aStart.y ) + aEnd.y ) * 0.5 );
    const double   half = len * 0.5;
    const double   offset = std::abs( aCenterT );

    // radius - offset cancels for shallow arcs, so use the equivalent half^2 / ( radius + offset )
    const double height = aSide * aCenterT <= 0.0 ? half * half / ( aRadius + offset ) : aRadius + offset;

    if( !toIU( chordMid + normal * ( aSide * height ), sol.mid ) )
        return sol;

    // An apex within rounding of the chord cannot be stored as an arc
    if( crossZ( chord.x, chord.y, static_cast<double>( sol.mid.x ) - aStart.x,
                static_cast<double>( sol.mid.y ) - aStart.y ) == 0.0 )
    {
        return sol;
    }

    sol.center = chordMid + normal * aCenterT;
    sol.radius = aRadius;
    sol.valid = true;
    return sol;
}

} // namespace


VECTOR2I ProjectToChordBisector( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCursor )
{
    const VECTOR2D chord( static_cast<double>( aEnd.x ) - aStart.x, static_cast<double>( aEnd.y ) - aStart.y );
    const double   len = chord.EuclideanNorm();

    if( len <= 0.0 )
        return aCursor;

    const VECTOR2D normal( -chord.y / len, chord.x / len );
    const VECTOR2D chordMid( ( static_cast<double>( aStart.x ) + aEnd.x ) * 0.5,
                             ( static_cast<double>( aStart.y ) + aEnd.y ) * 0.5 );
    const VECTOR2D rel( aCursor.x - chordMid.x, aCursor.y - chordMid.y );

    VECTOR2I projected;

    if( !toIU( chordMid + normal * rel.Dot( normal ), projected ) )
        return aCursor;

    return projected;
}


ARC_SOLUTION ArcThroughPoints( const VECTOR2I& aStart, const VECTOR2I& aOnArc, const VECTOR2I& aEnd )
{
    const VECTOR2D b( static_cast<double>( aEnd.x ) - aStart.x, static_cast<double>( aEnd.y ) - aStart.y );
    const VECTOR2D c( static_cast<double>( aOnArc.x ) - aStart.x, static_cast<double>( aOnArc.y ) - aStart.y );
    const double   d = crossZ( b.x, b.y, c.x, c.y );

    if( d == 0.0 )
        return ARC_SOLUTION();

    const double bb = b.SquaredEuclideanNorm();
    const double cc = c.SquaredEuclideanNorm();

    // Circumcenter relative to the start point
    const VECTOR2D rel( ( c.y * bb - b.y * cc ) / ( 2.0 * d ), ( b.x * cc - c.x * bb ) / ( 2.0 * d ) );
    const double   len = b.EuclideanNorm();

    // The signed distance of the center along the chord normal, measured from the chord midpoint
    const double centerT = ( -rel.x * b.y + rel.y * b.x ) / len;

    return buildOnChord( aStart, aEnd, d > 0.0 ? 1.0 : -1.0, centerT, rel.EuclideanNorm() );
}


ARC_SOLUTION ArcFromStartEndMidDrag( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCursor )
{
    const VECTOR2I projected = ProjectToChordBisector( aStart, aEnd, aCursor );
    ARC_SOLUTION   sol = ArcThroughPoints( aStart, projected, aEnd );

    // The circle passes through the projected point exactly, so keep it as the midpoint
    if( sol.valid )
        sol.mid = projected;

    return sol;
}


ARC_SOLUTION ArcFromStartEndCenterDrag( const VECTOR2I& aStart, const VECTOR2I& aEnd, const VECTOR2I& aCursor,
                                        bool aMajor )
{
    const VECTOR2D chord( static_cast<double>( aEnd.x ) - aStart.x, static_cast<double>( aEnd.y ) - aStart.y );
    const double   len = chord.EuclideanNorm();

    if( len <= 0.0 )
        return ARC_SOLUTION();

    const VECTOR2D normal( -chord.y / len, chord.x / len );
    const VECTOR2D chordMid( ( static_cast<double>( aStart.x ) + aEnd.x ) * 0.5,
                             ( static_cast<double>( aStart.y ) + aEnd.y ) * 0.5 );
    const double   centerT = VECTOR2D( aCursor.x - chordMid.x, aCursor.y - chordMid.y ).Dot( normal );

    // The minor arc bulges away from the center, the major arc toward it
    double side = centerT < 0.0 ? -1.0 : 1.0;

    if( !aMajor )
        side = -side;

    return buildOnChord( aStart, aEnd, side, centerT, std::hypot( len * 0.5, centerT ) );
}


ARC_SOLUTION ArcFromStartTangentEnd( const VECTOR2I& aStart, const VECTOR2D& aTangent, const VECTOR2I& aEnd )
{
    const double tangentLen = aTangent.EuclideanNorm();

    if( tangentLen <= 0.0 || !std::isfinite( tangentLen ) )
        return ARC_SOLUTION();

    const VECTOR2D t( aTangent.x / tangentLen, aTangent.y / tangentLen );
    const VECTOR2D d( static_cast<double>( aEnd.x ) - aStart.x, static_cast<double>( aEnd.y ) - aStart.y );
    const double   dd = d.SquaredEuclideanNorm();
    const double   cross = crossZ( d.x, d.y, t.x, t.y );

    if( dd <= 0.0 || cross == 0.0 )
        return ARC_SOLUTION();

    // The center sits on the normal to the tangent at the start, equidistant from start and end
    const double   k = -dd / ( 2.0 * cross );
    const VECTOR2D n( -t.y, t.x );
    const double   len = std::sqrt( dd );
    const VECTOR2D chordNormal( -d.y / len, d.x / len );

    return buildOnChord( aStart, aEnd, cross > 0.0 ? 1.0 : -1.0, k * n.Dot( chordNormal ), std::abs( k ) );
}

} // namespace KIGEOM
