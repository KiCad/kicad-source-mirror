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

#include <tool/arc_tangent_seed.h>

#include <array>
#include <span>

#include <bezier_curves.h>
#include <eda_shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>


static std::optional<VECTOR2D> normalized( const VECTOR2D& aVec )
{
    double length = aVec.EuclideanNorm();

    if( length == 0.0 )
        return std::nullopt;

    return aVec / length;
}


// Whichever of the two ends lies nearer aPoint, or null when both are farther than aTolerance
static const VECTOR2I* nearestEnd( const VECTOR2I& aFirst, const VECTOR2I& aLast, const VECTOR2I& aPoint,
                                   std::optional<int> aTolerance = std::nullopt )
{
    const double firstDist = ( aFirst - aPoint ).EuclideanNorm();
    const double lastDist = ( aLast - aPoint ).EuclideanNorm();
    const bool   useFirst = firstDist <= lastDist;
    const double dist = useFirst ? firstDist : lastDist;

    if( aTolerance && dist > *aTolerance )
        return nullptr;

    return useFirst ? &aFirst : &aLast;
}


static std::optional<VECTOR2D> tangentAtEndpoint( const EDA_SHAPE& aShape, const VECTOR2I& aPoint )
{
    const VECTOR2I& start = aShape.GetStart();
    const VECTOR2I& end = aShape.GetEnd();
    const bool      atStart = aPoint == start;

    if( !atStart && aPoint != end )
        return std::nullopt;

    if( aShape.GetShape() == SHAPE_T::SEGMENT )
        return normalized( VECTOR2D( atStart ? start - end : end - start ) );

    if( aShape.GetShape() != SHAPE_T::ARC )
        return std::nullopt;

    const VECTOR2D mid( aShape.GetArcMid() );

    // The turn of start->mid->end gives the sweep direction whatever the stored start/end order
    const double turn = ( mid - VECTOR2D( start ) ).Cross( VECTOR2D( end ) - mid );

    if( turn == 0.0 )
        return std::nullopt;

    // Tangent along increasing polar angle is the radius rotated by a quarter turn
    VECTOR2D tangent = ( VECTOR2D( aPoint ) - VECTOR2D( aShape.getCenter() ) ).Perpendicular();

    // Outward is the direction of travel at the end and against it at the start
    if( ( turn > 0.0 ) == atStart )
        tangent = -tangent;

    return normalized( tangent );
}


std::optional<ARC_TANGENT_SEED> ArcTangentSeedNear( const EDA_SHAPE& aShape, const VECTOR2I& aNear )
{
    const VECTOR2I endpoint = *nearestEnd( aShape.GetStart(), aShape.GetEnd(), aNear );

    std::optional<VECTOR2D> direction = tangentAtEndpoint( aShape, endpoint );

    if( !direction )
        return std::nullopt;

    return ARC_TANGENT_SEED{ endpoint, *direction };
}


// Seed for a polyline, which also stands in for a Bezier curve
static std::optional<ARC_TANGENT_SEED> polylineSeedAt( std::span<const VECTOR2I> aVertices, bool aClosed,
                                                       const VECTOR2I& aPoint, int aTolerance )
{
    const size_t count = aVertices.size();

    if( count < 2 )
        return std::nullopt;

    // A closed outline has no free ends
    if( !aClosed )
    {
        if( const VECTOR2I* end = nearestEnd( aVertices.front(), aVertices.back(), aPoint, aTolerance ) )
        {
            const VECTOR2I& inner = end == &aVertices.front() ? aVertices[1] : aVertices[count - 2];

            if( std::optional<VECTOR2D> dir = normalized( VECTOR2D( *end - inner ) ) )
                return ARC_TANGENT_SEED{ *end, *dir, false };

            return std::nullopt;
        }
    }

    const size_t edges = aClosed ? count : count - 1;
    std::optional<SEG> best;
    double             bestDist = aTolerance;

    for( size_t i = 0; i < edges; ++i )
    {
        const SEG edge( aVertices[i], aVertices[( i + 1 ) % count] );
        const double dist = edge.Distance( aPoint );

        if( dist <= bestDist )
        {
            best = edge;
            bestDist = dist;
        }
    }

    if( !best )
        return std::nullopt;

    if( std::optional<VECTOR2D> axis = normalized( VECTOR2D( best->B - best->A ) ) )
        return ARC_TANGENT_SEED{ best->NearestPoint( aPoint ), *axis, true };

    return std::nullopt;
}


std::optional<ARC_TANGENT_SEED> ArcTangentSeedAt( const SEG& aSegment, const VECTOR2I& aPoint, int aTolerance )
{
    const std::array<VECTOR2I, 2> ends{ aSegment.A, aSegment.B };

    return polylineSeedAt( ends, false, aPoint, aTolerance );
}


std::optional<ARC_TANGENT_SEED> ArcTangentSeedAt( const EDA_SHAPE& aShape, const VECTOR2I& aPoint, int aTolerance )
{
    switch( aShape.GetShape() )
    {
    case SHAPE_T::SEGMENT:
        return ArcTangentSeedAt( SEG( aShape.GetStart(), aShape.GetEnd() ), aPoint, aTolerance );

    case SHAPE_T::BEZIER:
    {
        // The curve lies inside its control polygon, so a far point skips the costly flattening
        BOX2I hull( aShape.GetStart(), VECTOR2I( 0, 0 ) );

        for( const VECTOR2I& pt : { aShape.GetBezierC1(), aShape.GetBezierC2(), aShape.GetEnd() } )
            hull.Merge( pt );

        if( !hull.Inflate( aTolerance ).Contains( aPoint ) )
            return std::nullopt;

        std::vector<VECTOR2I> poly;
        BEZIER_POLY           converter( aShape.GetStart(), aShape.GetBezierC1(), aShape.GetBezierC2(),
                                         aShape.GetEnd() );

        // Keep the polyline much finer than the pick tolerance so the tangent follows the curve
        converter.GetPoly( poly, std::max( 1, aTolerance / 10 ) );

        std::optional<ARC_TANGENT_SEED> seed = polylineSeedAt( poly, false, aPoint, aTolerance );

        // At an endpoint the control polygon gives the exact tangent
        if( seed && !seed->m_directionIsAxis )
        {
            const bool      atStart = seed->m_start == aShape.GetStart();
            const VECTOR2I& end = atStart ? aShape.GetStart() : aShape.GetEnd();
            const VECTOR2I& firstCtrl = atStart ? aShape.GetBezierC1() : aShape.GetBezierC2();
            const VECTOR2I& secondCtrl = atStart ? aShape.GetBezierC2() : aShape.GetBezierC1();
            const VECTOR2I& opposite = atStart ? aShape.GetEnd() : aShape.GetStart();

            for( const VECTOR2I* control : { &firstCtrl, &secondCtrl, &opposite } )
            {
                if( std::optional<VECTOR2D> dir = normalized( VECTOR2D( end - *control ) ) )
                {
                    seed->m_direction = *dir;
                    break;
                }
            }
        }

        return seed;
    }

    case SHAPE_T::ARC:
    {
        if( const VECTOR2I* end = nearestEnd( aShape.GetStart(), aShape.GetEnd(), aPoint, aTolerance ) )
        {
            if( std::optional<VECTOR2D> dir = tangentAtEndpoint( aShape, *end ) )
                return ARC_TANGENT_SEED{ *end, *dir, false };

            return std::nullopt;
        }

        const SHAPE_ARC arc( aShape.GetStart(), aShape.GetArcMid(), aShape.GetEnd(), 0 );
        const VECTOR2I  nearest = arc.NearestPoint( aPoint );

        if( ( nearest - aPoint ).EuclideanNorm() > aTolerance )
            return std::nullopt;

        const VECTOR2D radius( VECTOR2D( nearest ) - VECTOR2D( arc.GetCenter() ) );

        if( std::optional<VECTOR2D> axis = normalized( radius.Perpendicular() ) )
            return ARC_TANGENT_SEED{ nearest, *axis, true };

        return std::nullopt;
    }

    case SHAPE_T::POLY:
    {
        std::optional<ARC_TANGENT_SEED> best;
        double                          bestDist = 0.0;
        const SHAPE_POLY_SET&           poly = aShape.GetPolyShape();

        auto considerContour =
                [&]( const SHAPE_LINE_CHAIN& aChain )
                {
                    std::optional<ARC_TANGENT_SEED> seed =
                            polylineSeedAt( aChain.CPoints(), aChain.IsClosed(), aPoint, aTolerance );

                    if( !seed )
                        return;

                    double dist = ( seed->m_start - aPoint ).EuclideanNorm();

                    if( !best || dist < bestDist )
                    {
                        best = seed;
                        bestDist = dist;
                    }
                };

        for( int outline = 0; outline < poly.OutlineCount(); ++outline )
        {
            considerContour( poly.COutline( outline ) );

            for( int hole = 0; hole < poly.HoleCount( outline ); ++hole )
                considerContour( poly.CHole( outline, hole ) );
        }

        return best;
    }

    default: return std::nullopt;
    }
}
