/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#include <snap/snap_inference.h>

#include "snap_manifold.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>
#include <string>
#include <tuple>


namespace
{
double squaredDistanceTo( const INTERSECTABLE_GEOM& aGeometry, const VECTOR2I& aPoint )
{
    return std::visit(
            [&]( const auto& aShape ) -> double
            {
                using SHAPE_TYPE = std::decay_t<decltype( aShape )>;

                if constexpr( std::is_same_v<SHAPE_TYPE, HALF_LINE> || std::is_same_v<SHAPE_TYPE, SHAPE_ARC> )
                {
                    return aShape.NearestPoint( aPoint ).SquaredDistance( aPoint );
                }
                else if constexpr( std::is_same_v<SHAPE_TYPE, SEG> )
                {
                    return aShape.SquaredDistance( aPoint );
                }
                else
                {
                    double distance = snapManifoldDistance( aShape, aPoint );
                    return distance * distance;
                }
            },
            aGeometry );
}


INTERSECTABLE_GEOM extendedGeometry( const SNAP_OBJECT_PATH& aPath, bool aExtensionActive )
{
    if( aExtensionActive )
    {
        if( const SEG* segment = std::get_if<SEG>( &aPath.geometry ) )
            return LINE( *segment );

        if( const HALF_LINE* ray = std::get_if<HALF_LINE>( &aPath.geometry ) )
            return LINE( ray->GetContainedSeg() );
    }

    return aPath.geometry;
}


std::optional<VECTOR2I> linearDirection( const INTERSECTABLE_GEOM& aGeometry )
{
    if( const SEG* segment = std::get_if<SEG>( &aGeometry ) )
        return segment->B - segment->A;

    if( const LINE* line = std::get_if<LINE>( &aGeometry ) )
    {
        const SEG& segment = line->GetContainedSeg();
        return segment.B - segment.A;
    }

    if( const HALF_LINE* ray = std::get_if<HALF_LINE>( &aGeometry ) )
    {
        const SEG& segment = ray->GetContainedSeg();
        return segment.B - segment.A;
    }

    return std::nullopt;
}


bool parallelLinearGeometry( const INTERSECTABLE_GEOM& aFirst, const INTERSECTABLE_GEOM& aSecond )
{
    std::optional<VECTOR2I> first = linearDirection( aFirst );
    std::optional<VECTOR2I> second = linearDirection( aSecond );

    if( !first || !second )
        return false;

    return first->Cross( *second ) == 0;
}


SNAP_CANDIDATE pathCandidate( const SNAP_OBJECT_PATH& aPath, const VECTOR2I& aSource, int aRadius,
                              bool aExtensionActive )
{
    SNAP_CANDIDATE candidate;

    std::visit(
            [&]( const auto& aGeometry )
            {
                using SHAPE_TYPE = std::decay_t<decltype( aGeometry )>;

                if constexpr( std::is_same_v<SHAPE_TYPE, SEG> )
                {
                    candidate =
                            SNAP_CANDIDATE::Line( aPath.id, SNAP_PRIORITY_TIER::OBJECT,
                                                  aExtensionActive ? SNAP_CANDIDATE_SUBTYPE::ACTIVE_EXTENSION
                                                                   : SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD,
                                                  aGeometry.A, VECTOR2D( aGeometry.B - aGeometry.A ),
                                                  snapManifoldDistance( aGeometry, aSource ) / std::max( 1, aRadius ) );
                    candidate.relation =
                            aExtensionActive ? SNAP_RELATION::POINT_ON_LINE : SNAP_RELATION::POINT_ON_SEGMENT;
                    candidate.finite = !aExtensionActive;
                    candidate.domainStart = 0.0;
                    candidate.domainEnd = 1.0;
                    candidate.manifold = aExtensionActive ? INTERSECTABLE_GEOM( LINE( aGeometry ) )
                                                          : INTERSECTABLE_GEOM( aGeometry );
                }
                else if constexpr( std::is_same_v<SHAPE_TYPE, LINE> )
                {
                    const SEG& segment = aGeometry.GetContainedSeg();
                    candidate = SNAP_CANDIDATE::Line(
                            aPath.id, SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::ACTIVE_EXTENSION, segment.A,
                            VECTOR2D( segment.B - segment.A ),
                            snapManifoldDistance( aGeometry, aSource ) / std::max( 1, aRadius ) );
                    candidate.manifold = aGeometry;
                }
                else
                {
                    VECTOR2I point;

                    if constexpr( std::is_same_v<SHAPE_TYPE, CIRCLE> )
                        point = aGeometry.NearestPoint( aSource );
                    else if constexpr( std::is_same_v<SHAPE_TYPE, SHAPE_ARC> )
                        point = aGeometry.NearestPoint( aSource );
                    else if constexpr( std::is_same_v<SHAPE_TYPE, HALF_LINE> )
                    {
                        point = aGeometry.NearestPoint( aSource );
                        candidate = SNAP_CANDIDATE::Line(
                                aPath.id, SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::ACTIVE_EXTENSION,
                                aGeometry.GetStart(), VECTOR2D( aGeometry.GetContainedPoint() - aGeometry.GetStart() ),
                                point.Distance( aSource ) / static_cast<double>( std::max( 1, aRadius ) ) );
                        candidate.relation = SNAP_RELATION::POINT_ON_RAY;
                        candidate.manifold = aGeometry;
                        return;
                    }
                    else
                        point = aGeometry.Centre();

                    candidate = SNAP_CANDIDATE::Point(
                            aPath.id, SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD, point,
                            point.Distance( aSource ) / static_cast<double>( std::max( 1, aRadius ) ) );
                    candidate.consumedDof = 1;
                    candidate.manifold = aGeometry;

                    if constexpr( std::is_same_v<SHAPE_TYPE, CIRCLE> )
                        candidate.relation = SNAP_RELATION::POINT_ON_CIRCLE;
                    else if constexpr( std::is_same_v<SHAPE_TYPE, SHAPE_ARC> )
                        candidate.relation = SNAP_RELATION::POINT_ON_ARC;
                }
            },
            aPath.geometry );

    if( aPath.intrinsic )
        candidate.subtype = SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR;
    else if( aExtensionActive )
        candidate.subtype = SNAP_CANDIDATE_SUBTYPE::ACTIVE_EXTENSION;

    return candidate;
}


// Unoptimized builds otherwise retain descriptor calls in hot layout loops, causing a measurable
// stress-path regression.
#if defined( __GNUC__ )
#define SNAP_ALWAYS_INLINE inline __attribute__( ( always_inline ) )
#elif defined( _MSC_VER )
#define SNAP_ALWAYS_INLINE __forceinline
#else
#define SNAP_ALWAYS_INLINE inline
#endif

template <bool IsX>
struct AXIS_DESCRIPTOR
{
    static constexpr size_t       index = IsX ? 0 : 1;
    static constexpr SNAP_ID_KIND boundsKind = IsX ? SNAP_ID_KIND::BOUNDS_X : SNAP_ID_KIND::BOUNDS_Y;
    static constexpr SNAP_ID_KIND anchorPointKind =
            IsX ? SNAP_ID_KIND::ANCHOR_POINT_X : SNAP_ID_KIND::ANCHOR_POINT_Y;
    static constexpr SNAP_ID_KIND equalGapKind = IsX ? SNAP_ID_KIND::EQUAL_GAP_X : SNAP_ID_KIND::EQUAL_GAP_Y;
    static constexpr SNAP_ID_KIND copyGapKind = IsX ? SNAP_ID_KIND::COPY_GAP_X : SNAP_ID_KIND::COPY_GAP_Y;

    static SNAP_ALWAYS_INLINE int coordinate( const VECTOR2I& aPoint ) { return IsX ? aPoint.x : aPoint.y; }
    static SNAP_ALWAYS_INLINE int perpendicularCoordinate( const VECTOR2I& aPoint )
    {
        return IsX ? aPoint.y : aPoint.x;
    }
    static SNAP_ALWAYS_INLINE int low( const BOX2I& aBox ) { return IsX ? aBox.GetLeft() : aBox.GetTop(); }
    static SNAP_ALWAYS_INLINE int high( const BOX2I& aBox ) { return IsX ? aBox.GetRight() : aBox.GetBottom(); }
    static SNAP_ALWAYS_INLINE int size( const BOX2I& aBox ) { return IsX ? aBox.GetWidth() : aBox.GetHeight(); }
    static SNAP_ALWAYS_INLINE int perpendicularLow( const BOX2I& aBox ) { return IsX ? aBox.GetTop() : aBox.GetLeft(); }
    static SNAP_ALWAYS_INLINE int perpendicularHigh( const BOX2I& aBox )
    {
        return IsX ? aBox.GetBottom() : aBox.GetRight();
    }
    static SNAP_ALWAYS_INLINE int preferredFeature( const SNAP_REFERENCE_PREFERENCE& aPreference )
    {
        return IsX ? aPreference.horizontalFeature : aPreference.verticalFeature;
    }

    static SNAP_ALWAYS_INLINE std::array<int, 3> features( const BOX2I& aBox )
    {
        return { low( aBox ), coordinate( aBox.Centre() ), high( aBox ) };
    }

    static SNAP_ALWAYS_INLINE VECTOR2I point( int aCoordinate, int aPerpendicular )
    {
        return IsX ? VECTOR2I( aCoordinate, aPerpendicular ) : VECTOR2I( aPerpendicular, aCoordinate );
    }

    static SNAP_ALWAYS_INLINE VECTOR2I offset( int aDistance )
    {
        return IsX ? VECTOR2I( aDistance, 0 ) : VECTOR2I( 0, aDistance );
    }

    static SNAP_ALWAYS_INLINE SNAP_CANDIDATE candidate( SNAP_STABLE_ID aId, int aCoordinate, double aResidual )
    {
        if constexpr( IsX )
        {
            return SNAP_CANDIDATE::AxisX( std::move( aId ), SNAP_PRIORITY_TIER::OBJECT,
                                          SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, aCoordinate, aResidual );
        }
        else
        {
            return SNAP_CANDIDATE::AxisY( std::move( aId ), SNAP_PRIORITY_TIER::OBJECT,
                                          SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, aCoordinate, aResidual );
        }
    }
};

template <typename Callback>
SNAP_ALWAYS_INLINE void forEachAxis( Callback&& aCallback )
{
    aCallback.template operator()<AXIS_DESCRIPTOR<true>>();
    aCallback.template operator()<AXIS_DESCRIPTOR<false>>();
}

#undef SNAP_ALWAYS_INLINE
} // namespace


void SNAP_INFERENCE_PROVIDER::AddPath( SNAP_OBJECT_PATH aPath )
{
    m_paths.emplace_back( std::move( aPath ) );
}


void SNAP_INFERENCE_PROVIDER::AddBounds( SNAP_OBJECT_BOUNDS aBounds )
{
    m_bounds.emplace_back( std::move( aBounds ) );
}


void SNAP_INFERENCE_PROVIDER::AddAlignmentPoint( SNAP_ALIGNMENT_POINT aPoint )
{
    m_alignmentPoints.emplace_back( std::move( aPoint ) );
}


void SNAP_INFERENCE_PROVIDER::Clear()
{
    m_paths.clear();
    m_bounds.clear();
    m_alignmentPoints.clear();
    m_activeExtensions.clear();
}


void SNAP_INFERENCE_PROVIDER::ActivateExtension( const SNAP_STABLE_ID& aId )
{
    if( std::find( m_activeExtensions.begin(), m_activeExtensions.end(), aId ) == m_activeExtensions.end() )
    {
        m_activeExtensions.push_back( aId );
    }
}


void SNAP_INFERENCE_PROVIDER::ClearExtensions()
{
    m_activeExtensions.clear();
}


bool SNAP_INFERENCE_PROVIDER::eligible( const SNAP_SOURCE_CONTEXT& aContext, const SNAP_STABLE_ID& aId ) const
{
    return std::find( aContext.movingFeatures.begin(), aContext.movingFeatures.end(), aId )
           == aContext.movingFeatures.end();
}


bool SNAP_INFERENCE_PROVIDER::eligible( const SNAP_SOURCE_CONTEXT& aContext, const SNAP_OBJECT_BOUNDS& aBounds ) const
{
    if( !eligible( aContext, aBounds.id ) )
        return false;

    return !aContext.movingItem || !aBounds.parent || *aBounds.parent != aContext.movingItem->target;
}


bool SNAP_INFERENCE_PROVIDER::eligible( const SNAP_SOURCE_CONTEXT& aContext, const SNAP_ALIGNMENT_POINT& aPoint ) const
{
    if( !eligible( aContext, aPoint.id ) )
        return false;

    return !aContext.movingItem || !aPoint.parent || *aPoint.parent != aContext.movingItem->target;
}


std::vector<SNAP_CANDIDATE> SNAP_INFERENCE_PROVIDER::CollectObjectGeometry( const SNAP_SOURCE_CONTEXT& aContext,
                                                                            int                        aRadius ) const
{
    struct ELIGIBLE_PATH
    {
        const SNAP_OBJECT_PATH* path;
        bool                    extension;
        bool                    expand;
        double                  distanceSquared;
    };

    constexpr size_t            maxCandidatePaths = 64;
    constexpr size_t            maxIntersectionPaths = 12;
    std::vector<ELIGIBLE_PATH>  paths;
    std::vector<SNAP_CANDIDATE> result;
    paths.reserve( maxCandidatePaths );

    auto betterPath = []( const ELIGIBLE_PATH& aLeft, const ELIGIBLE_PATH& aRight )
    {
        return std::forward_as_tuple( !aLeft.path->intrinsic, aLeft.distanceSquared, aLeft.path->id )
               < std::forward_as_tuple( !aRight.path->intrinsic, aRight.distanceSquared, aRight.path->id );
    };
    const double radiusSquared = static_cast<double>( aRadius ) * aRadius;

    for( const SNAP_OBJECT_PATH& path : m_paths )
    {
        if( !eligible( aContext, path.id ) )
            continue;

        bool expand =
                std::find( m_activeExtensions.begin(), m_activeExtensions.end(), path.id ) != m_activeExtensions.end();
        bool                              extension = path.activeExtension || expand;
        std::optional<INTERSECTABLE_GEOM> extended;
        const INTERSECTABLE_GEOM*         geometry = &path.geometry;

        if( expand )
        {
            extended = extendedGeometry( path, true );
            geometry = &*extended;
        }

        double distanceSquared = squaredDistanceTo( *geometry, aContext.sourcePoint );

        if( distanceSquared > radiusSquared )
            continue;

        ELIGIBLE_PATH candidate{ &path, extension, expand, distanceSquared };

        if( paths.size() < maxCandidatePaths )
        {
            paths.push_back( candidate );
            std::push_heap( paths.begin(), paths.end(), betterPath );
        }
        else if( betterPath( candidate, paths.front() ) )
        {
            std::pop_heap( paths.begin(), paths.end(), betterPath );
            paths.back() = candidate;
            std::push_heap( paths.begin(), paths.end(), betterPath );
        }
    }

    std::sort_heap( paths.begin(), paths.end(), betterPath );
    size_t candidatePathCount = paths.size();

    for( size_t i = 0; i < candidatePathCount; ++i )
    {
        result.push_back( pathCandidate( *paths[i].path, aContext.sourcePoint, aRadius, paths[i].extension ) );
    }

    size_t                          intersectionPathCount = std::min( candidatePathCount, maxIntersectionPaths );
    std::vector<INTERSECTABLE_GEOM> intersectionGeometry;
    intersectionGeometry.reserve( intersectionPathCount );

    for( size_t i = 0; i < intersectionPathCount; ++i )
    {
        intersectionGeometry.push_back( extendedGeometry( *paths[i].path, paths[i].expand ) );
    }

    std::vector<VECTOR2I> intersections;

    for( size_t first = 0; first < intersectionPathCount; ++first )
    {
        for( size_t second = first + 1; second < intersectionPathCount; ++second )
        {
            if( parallelLinearGeometry( intersectionGeometry[first], intersectionGeometry[second] ) )
            {
                continue;
            }

            intersections.clear();
            std::visit( INTERSECTION_VISITOR( intersectionGeometry[second], intersections ),
                        intersectionGeometry[first] );

            std::sort( intersections.begin(), intersections.end(),
                       []( const VECTOR2I& aLeft, const VECTOR2I& aRight )
                       {
                           return std::tie( aLeft.x, aLeft.y ) < std::tie( aRight.x, aRight.y );
                       } );
            intersections.erase( std::unique( intersections.begin(), intersections.end() ), intersections.end() );

            for( size_t branch = 0; branch < intersections.size(); ++branch )
            {
                const VECTOR2I& point = intersections[branch];

                if( point.Distance( aContext.sourcePoint ) > aRadius )
                    continue;

                SNAP_CANDIDATE candidate = SNAP_CANDIDATE::Point(
                        MakeIntersectionSnapId( paths[first].path->id, paths[second].path->id,
                                                static_cast<int>( branch ) ),
                        SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::INTERSECTION, point,
                        point.Distance( aContext.sourcePoint ) / static_cast<double>( std::max( 1, aRadius ) ) );
                result.push_back( std::move( candidate ) );
            }
        }
    }

    return result;
}


std::vector<SNAP_CANDIDATE> SNAP_INFERENCE_PROVIDER::CollectTangentNormal( const SNAP_SOURCE_CONTEXT& aContext,
                                                                           int aRadius, bool aTangentEnabled,
                                                                           bool aNormalEnabled ) const
{
    std::vector<SNAP_CANDIDATE> result;

    if( !aContext.stationarySourceLeg )
        return result;

    for( const SNAP_OBJECT_PATH& path : m_paths )
    {
        if( !eligible( aContext, path.id ) )
            continue;

        const CIRCLE*    circle = std::get_if<CIRCLE>( &path.geometry );
        const SHAPE_ARC* arc = std::get_if<SHAPE_ARC>( &path.geometry );

        if( !circle && !arc )
            continue;

        VECTOR2D source( *aContext.stationarySourceLeg );
        VECTOR2D center( circle ? circle->Center : arc->GetCenter() );
        double   radius = circle ? circle->Radius : arc->GetRadius();
        VECTOR2D delta = source - center;
        double   distanceSquared = delta.SquaredEuclideanNorm();

        if( distanceSquared == 0.0 )
            continue;

        const auto addContact = [&]( const VECTOR2D& aContact, SNAP_RELATION aRelation, int aBranch )
        {
            VECTOR2I point( KiROUND( aContact.x ), KiROUND( aContact.y ) );

            if( point.Distance( aContext.sourcePoint ) > aRadius || ( arc && !arc->Collide( point, 2 ) ) )
                return;

            SNAP_STABLE_ID contactId = MakeDerivedSnapId( aRelation == SNAP_RELATION::TANGENT ? SNAP_ID_KIND::TANGENT
                                                                                              : SNAP_ID_KIND::NORMAL,
                                                          path.id, path.id.featureIndex, aBranch );
            SNAP_CANDIDATE candidate = SNAP_CANDIDATE::Point(
                    std::move( contactId ), SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::TANGENT_NORMAL, point,
                    point.Distance( aContext.sourcePoint ) / static_cast<double>( std::max( 1, aRadius ) ) );
            candidate.relation = aRelation;
            candidate.guides.push_back( SNAP_GUIDE{ *aContext.stationarySourceLeg, point } );
            result.push_back( std::move( candidate ) );
        };

        if( aTangentEnabled && distanceSquared >= radius * radius )
        {
            double radiusSquared = radius * radius;
            double base = radiusSquared / distanceSquared;
            double perpendicular =
                    radius * std::sqrt( std::max( 0.0, distanceSquared - radiusSquared ) ) / distanceSquared;
            VECTOR2D normal( -delta.y, delta.x );
            addContact( center + delta * base + normal * perpendicular, SNAP_RELATION::TANGENT, 0 );
            addContact( center + delta * base - normal * perpendicular, SNAP_RELATION::TANGENT, 1 );
        }

        if( aNormalEnabled )
        {
            double   length = std::sqrt( distanceSquared );
            VECTOR2D radial = delta * ( radius / length );
            addContact( center + radial, SNAP_RELATION::NORMAL, 0 );
            addContact( center - radial, SNAP_RELATION::NORMAL, 1 );
        }
    }

    return result;
}


std::vector<SNAP_CANDIDATE> SNAP_INFERENCE_PROVIDER::CollectAlignment( const SNAP_SOURCE_CONTEXT& aContext,
                                                                       int                        aRadius ) const
{
    struct PROPOSAL
    {
        const SNAP_OBJECT_BOUNDS* target;
        int                       sourceFeature;
        int                       targetFeature;
        int                       affinity;
        int                       coordinate;
        int                       targetCoordinate;
        int                       displacement;
    };

    constexpr size_t MAX_CANDIDATES_PER_AXIS = 64;
    const auto       proposalKey = []( const PROPOSAL& aProposal )
    {
        return std::forward_as_tuple( aProposal.affinity, aProposal.displacement, aProposal.target->id,
                                      aProposal.sourceFeature, aProposal.targetFeature );
    };

    const auto compareProposal = [proposalKey]( const PROPOSAL& aLeft, const PROPOSAL& aRight )
    {
        return proposalKey( aLeft ) < proposalKey( aRight );
    };

    using PROPOSAL_QUEUE = std::priority_queue<PROPOSAL, std::vector<PROPOSAL>, decltype( compareProposal )>;

    std::array<PROPOSAL_QUEUE, 2> proposalQueues{ PROPOSAL_QUEUE( compareProposal ),
                                                  PROPOSAL_QUEUE( compareProposal ) };
    std::vector<SNAP_CANDIDATE>   result;

    if( !aContext.movingBounds )
        return result;

    BOX2I movingBounds = *aContext.movingBounds;

    if( aContext.movingReferencePoint )
        movingBounds.Offset( aContext.sourcePoint - *aContext.movingReferencePoint );

    const auto retain = [&]( PROPOSAL_QUEUE& aQueue, PROPOSAL aProposal )
    {
        if( aQueue.size() < MAX_CANDIDATES_PER_AXIS )
        {
            aQueue.push( std::move( aProposal ) );
        }
        else if( proposalKey( aProposal ) < proposalKey( aQueue.top() ) )
        {
            aQueue.pop();
            aQueue.push( std::move( aProposal ) );
        }
    };

    const auto boundsAffinity = [&]<typename Axis>( int aSourceFeature, int aTargetFeature )
    {
        if( aContext.referencePreference.kind == SNAP_REFERENCE_KIND::NONE )
            return 0;

        if( aContext.referencePreference.kind != SNAP_REFERENCE_KIND::BOUNDS_FEATURE )
            return 1;

        int preferred = Axis::preferredFeature( aContext.referencePreference );
        return aSourceFeature == preferred && aTargetFeature == preferred ? 0 : 1;
    };
    std::array<std::array<int, 3>, 2> movingFeatures;
    forEachAxis(
            [&]<typename Axis>()
            {
                movingFeatures[Axis::index] = Axis::features( movingBounds );
            } );

    for( const SNAP_OBJECT_BOUNDS& target : m_bounds )
    {
        if( !eligible( aContext, target ) )
            continue;

        forEachAxis(
                [&]<typename Axis>()
                {
                    std::array<int, 3> targetFeatures = Axis::features( target.bounds );

                    for( int sourceFeature = 0; sourceFeature < 3; ++sourceFeature )
                    {
                        for( int targetFeature = 0; targetFeature < 3; ++targetFeature )
                        {
                            int resolved = Axis::coordinate( aContext.sourcePoint ) + targetFeatures[targetFeature]
                                           - movingFeatures[Axis::index][sourceFeature];
                            int displacement = std::abs( resolved - Axis::coordinate( aContext.sourcePoint ) );

                            if( displacement <= aRadius )
                            {
                                retain( proposalQueues[Axis::index],
                                        { &target, sourceFeature, targetFeature,
                                          boundsAffinity.template operator()<Axis>( sourceFeature, targetFeature ),
                                          resolved, targetFeatures[targetFeature], displacement } );
                            }
                        }
                    }
                } );
    }

    const auto ordered = [&]( PROPOSAL_QUEUE& aQueue )
    {
        std::vector<PROPOSAL> proposals;
        proposals.reserve( aQueue.size() );

        while( !aQueue.empty() )
        {
            proposals.push_back( aQueue.top() );
            aQueue.pop();
        }

        std::sort( proposals.begin(), proposals.end(),
                   [&]( const PROPOSAL& aLeft, const PROPOSAL& aRight )
                   {
                       return proposalKey( aLeft ) < proposalKey( aRight );
                   } );
        return proposals;
    };

    std::array<std::vector<SNAP_CANDIDATE>, 2> candidates;
    forEachAxis(
            [&]<typename Axis>()
            {
                std::vector<PROPOSAL>        retained = ordered( proposalQueues[Axis::index] );
                std::vector<SNAP_CANDIDATE>& axisCandidates = candidates[Axis::index];
                axisCandidates.reserve( retained.size() );

                for( const PROPOSAL& proposal : retained )
                {
                    SNAP_STABLE_ID id = MakeDerivedSnapId( Axis::boundsKind, proposal.target->id,
                                                           proposal.sourceFeature * 3 + proposal.targetFeature );
                    SNAP_CANDIDATE candidate =
                            Axis::candidate( std::move( id ), proposal.coordinate,
                                             proposal.displacement / static_cast<double>( std::max( 1, aRadius ) ) );
                    candidate.referenceAffinity = proposal.affinity;
                    candidate.relation = SNAP_RELATION::BBOX_ALIGNMENT;
                    candidate.guides.push_back(
                            { Axis::point( proposal.targetCoordinate,
                                           std::min( Axis::perpendicularLow( movingBounds ),
                                                     Axis::perpendicularLow( proposal.target->bounds ) ) ),
                              Axis::point( proposal.targetCoordinate,
                                           std::max( Axis::perpendicularHigh( movingBounds ),
                                                     Axis::perpendicularHigh( proposal.target->bounds ) ) ) } );
                    axisCandidates.push_back( std::move( candidate ) );
                }
            } );

    if( aContext.referencePreference.kind == SNAP_REFERENCE_KIND::ANCHOR_POINT )
    {
        for( const SNAP_ALIGNMENT_POINT& point : m_alignmentPoints )
        {
            if( !eligible( aContext, point ) )
                continue;

            forEachAxis(
                    [&]<typename Axis>()
                    {
                        int coordinate = Axis::coordinate( point.position );
                        int displacement = std::abs( coordinate - Axis::coordinate( aContext.sourcePoint ) );

                        if( displacement > aRadius )
                            return;

                        SNAP_STABLE_ID id = MakeDerivedSnapId( Axis::anchorPointKind, point.id );
                        SNAP_CANDIDATE candidate =
                                Axis::candidate( std::move( id ), coordinate,
                                                 displacement / static_cast<double>( std::max( 1, aRadius ) ) );
                        candidate.relation = SNAP_RELATION::BBOX_ALIGNMENT;
                        candidate.guides.push_back(
                                { Axis::point( coordinate,
                                               std::min( Axis::perpendicularLow( movingBounds ),
                                                         Axis::perpendicularCoordinate( point.position ) ) ),
                                  Axis::point( coordinate,
                                               std::max( Axis::perpendicularHigh( movingBounds ),
                                                         Axis::perpendicularCoordinate( point.position ) ) ) } );
                        candidates[Axis::index].push_back( std::move( candidate ) );
                    } );
        }
    }

    const auto candidateKey = []( const SNAP_CANDIDATE& aCandidate )
    {
        return std::forward_as_tuple( aCandidate.referenceAffinity, aCandidate.normalizedScreenResidual,
                                      aCandidate.id );
    };
    const auto retainBest = [&]( std::vector<SNAP_CANDIDATE>& aCandidates )
    {
        std::sort( aCandidates.begin(), aCandidates.end(),
                   [&]( const SNAP_CANDIDATE& aLeft, const SNAP_CANDIDATE& aRight )
                   {
                       return candidateKey( aLeft ) < candidateKey( aRight );
                   } );

        if( aCandidates.size() > MAX_CANDIDATES_PER_AXIS )
            aCandidates.resize( MAX_CANDIDATES_PER_AXIS );
    };

    retainBest( candidates[0] );
    retainBest( candidates[1] );
    result.reserve( candidates[0].size() + candidates[1].size() );
    std::move( candidates[0].begin(), candidates[0].end(), std::back_inserter( result ) );
    std::move( candidates[1].begin(), candidates[1].end(), std::back_inserter( result ) );
    return result;
}


std::vector<SNAP_CANDIDATE> SNAP_INFERENCE_PROVIDER::CollectEqualSpacing( const SNAP_SOURCE_CONTEXT& aContext,
                                                                          int                        aRadius ) const
{
    constexpr size_t            MAX_CANDIDATES = 128;
    std::vector<SNAP_CANDIDATE> result;

    if( !aContext.movingBounds )
        return result;

    BOX2I movingBounds = *aContext.movingBounds;

    if( aContext.movingReferencePoint )
        movingBounds.Offset( aContext.sourcePoint - *aContext.movingReferencePoint );

    const auto betterCandidate = []( const SNAP_CANDIDATE& aLeft, const SNAP_CANDIDATE& aRight )
    {
        return std::forward_as_tuple( aLeft.normalizedScreenResidual, aLeft.id )
               < std::forward_as_tuple( aRight.normalizedScreenResidual, aRight.id );
    };

    const auto retainCandidate = [&]( SNAP_CANDIDATE aCandidate )
    {
        if( result.size() < MAX_CANDIDATES )
        {
            result.push_back( std::move( aCandidate ) );
            std::push_heap( result.begin(), result.end(), betterCandidate );
        }
        else if( betterCandidate( aCandidate, result.front() ) )
        {
            std::pop_heap( result.begin(), result.end(), betterCandidate );
            result.back() = std::move( aCandidate );
            std::push_heap( result.begin(), result.end(), betterCandidate );
        }
    };

    std::array<std::vector<const SNAP_OBJECT_BOUNDS*>, 2> aligned;

    const auto overlaps = []( int aFirstStart, int aFirstEnd, int aSecondStart, int aSecondEnd )
    {
        return aFirstStart < aSecondEnd && aSecondStart < aFirstEnd;
    };

    for( const SNAP_OBJECT_BOUNDS& bounds : m_bounds )
    {
        if( !eligible( aContext, bounds ) )
            continue;

        forEachAxis(
                [&]<typename Axis>()
                {
                    if( overlaps( Axis::perpendicularLow( bounds.bounds ), Axis::perpendicularHigh( bounds.bounds ),
                                  Axis::perpendicularLow( movingBounds ), Axis::perpendicularHigh( movingBounds ) ) )
                    {
                        aligned[Axis::index].push_back( &bounds );
                    }
                } );
    }

    forEachAxis(
            [&]<typename Axis>()
            {
                std::vector<const SNAP_OBJECT_BOUNDS*>& axisBounds = aligned[Axis::index];
                std::sort( axisBounds.begin(), axisBounds.end(),
                           []( const SNAP_OBJECT_BOUNDS* aFirst, const SNAP_OBJECT_BOUNDS* aSecond )
                           {
                               return std::forward_as_tuple( Axis::low( aFirst->bounds ), aFirst->id )
                                      < std::forward_as_tuple( Axis::low( aSecond->bounds ), aSecond->id );
                           } );
            } );

    const auto addCandidate = [&]<typename Axis>( SNAP_STABLE_ID aId, SNAP_ID_KIND aKind, int aResolvedSource,
                                                  const SNAP_OBJECT_BOUNDS& aFirst, const SNAP_OBJECT_BOUNDS& aSecond )
    {
        int residual = std::abs( aResolvedSource - Axis::coordinate( aContext.sourcePoint ) );

        if( residual > aRadius )
            return;

        BOX2I resolvedBounds = movingBounds;
        resolvedBounds.Offset( Axis::offset( aResolvedSource - Axis::coordinate( aContext.sourcePoint ) ) );

        aId.kind = aKind;
        SNAP_CANDIDATE candidate = Axis::candidate( std::move( aId ), aResolvedSource,
                                                    residual / static_cast<double>( std::max( 1, aRadius ) ) );
        candidate.relation = SNAP_RELATION::BBOX_EQUAL_GAP;
        const int perpendicular =
                std::max( Axis::perpendicularHigh( aFirst.bounds ), Axis::perpendicularHigh( aSecond.bounds ) );
        const auto addGuide = [&]( int aStart, int aEnd )
        {
            candidate.guides.push_back( { Axis::point( aStart, perpendicular ), Axis::point( aEnd, perpendicular ),
                                          SNAP_GUIDE_STYLE::DIMENSION_BRACKET } );
        };

        if( candidate.id.solutionBranch < 0 )
        {
            addGuide( Axis::high( resolvedBounds ), Axis::low( aFirst.bounds ) );
            addGuide( Axis::high( aFirst.bounds ), Axis::low( aSecond.bounds ) );
        }
        else if( candidate.id.solutionBranch > 0 )
        {
            addGuide( Axis::high( aFirst.bounds ), Axis::low( aSecond.bounds ) );
            addGuide( Axis::high( aSecond.bounds ), Axis::low( resolvedBounds ) );
        }
        else
        {
            addGuide( Axis::high( aFirst.bounds ), Axis::low( resolvedBounds ) );
            addGuide( Axis::high( resolvedBounds ), Axis::low( aSecond.bounds ) );
        }

        retainCandidate( std::move( candidate ) );
    };

    forEachAxis(
            [&]<typename Axis>()
            {
                const std::vector<const SNAP_OBJECT_BOUNDS*>& axisBounds = aligned[Axis::index];
                int sourceOffset = Axis::coordinate( aContext.sourcePoint ) - Axis::coordinate( movingBounds.Centre() );

                for( size_t i = 1; i < axisBounds.size(); ++i )
                {
                    if( axisBounds[i - 1]->parent != axisBounds[i]->parent )
                        continue;

                    int available = Axis::low( axisBounds[i]->bounds ) - Axis::high( axisBounds[i - 1]->bounds );

                    if( available < 0 )
                        continue;

                    SNAP_STABLE_ID id = MakeIntersectionSnapId( axisBounds[i - 1]->id, axisBounds[i]->id, 0 );

                    if( available >= Axis::size( movingBounds ) )
                    {
                        int center = KiROUND(
                                ( Axis::high( axisBounds[i - 1]->bounds ) + Axis::low( axisBounds[i]->bounds ) )
                                / 2.0 );
                        addCandidate.template operator()<Axis>( id, Axis::equalGapKind, center + sourceOffset,
                                                                *axisBounds[i - 1], *axisBounds[i] );
                    }

                    if( i + 1 == axisBounds.size()
                        || Axis::high( axisBounds[i]->bounds ) + available + Axis::size( movingBounds )
                                   <= Axis::low( axisBounds[i + 1]->bounds ) )
                    {
                        int            low = Axis::high( axisBounds[i]->bounds ) + available;
                        SNAP_STABLE_ID copiedId = id;
                        copiedId.solutionBranch = 1;
                        addCandidate.template operator()<Axis>( std::move( copiedId ), Axis::copyGapKind,
                                                                Axis::coordinate( aContext.sourcePoint ) + low
                                                                        - Axis::low( movingBounds ),
                                                                *axisBounds[i - 1], *axisBounds[i] );
                    }

                    if( i == 1
                        || Axis::high( axisBounds[i - 2]->bounds ) + available + Axis::size( movingBounds )
                                   <= Axis::low( axisBounds[i - 1]->bounds ) )
                    {
                        int            high = Axis::low( axisBounds[i - 1]->bounds ) - available;
                        SNAP_STABLE_ID copiedId = id;
                        copiedId.solutionBranch = -1;
                        addCandidate.template operator()<Axis>( std::move( copiedId ), Axis::copyGapKind,
                                                                Axis::coordinate( aContext.sourcePoint ) + high
                                                                        - Axis::high( movingBounds ),
                                                                *axisBounds[i - 1], *axisBounds[i] );
                    }
                }
            } );

    std::sort_heap( result.begin(), result.end(), betterCandidate );
    return result;
}
