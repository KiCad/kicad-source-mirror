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

#include <boost/test/unit_test.hpp>

#include <kiid.h>
#include <snap/snap_resolver.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>


namespace
{
SNAP_TARGET_ID targetId( const char* aName )
{
    return KIID::FromName( aName ).AsBytes();
}


SNAP_STABLE_ID id( const char* aFeature )
{
    return { SNAP_ID_KIND::ITEM_GEOMETRY, targetId( aFeature ), 0, 0 };
}


std::string targetBytes( const SNAP_STABLE_ID& aId )
{
    std::ostringstream stream;

    for( uint8_t byte : aId.target )
        stream << std::hex << std::setfill( '0' ) << std::setw( 2 ) << static_cast<int>( byte );

    return stream.str();
}


std::string resultSignature( const SNAP_RESULT& aResult )
{
    std::ostringstream stream;
    stream << static_cast<int>( aResult.status ) << ':' << aResult.position.x << ',' << aResult.position.y << ':'
           << aResult.remainingDof;

    for( const SNAP_STABLE_ID& accepted : aResult.accepted )
    {
        stream << ':' << static_cast<int>( accepted.kind ) << ',' << targetBytes( accepted ) << ','
               << accepted.featureIndex << ',' << accepted.solutionBranch;
    }

    stream << std::hexfloat;

    for( double residual : aResult.quantizedResiduals )
        stream << ':' << residual;

    for( const SNAP_GUIDE& guide : aResult.guides )
    {
        stream << ':' << guide.start.x << ',' << guide.start.y << ',' << guide.end.x << ',' << guide.end.y << ','
               << static_cast<int>( guide.style );
    }

    return stream.str();
}
} // namespace


BOOST_AUTO_TEST_SUITE( SnapResolver )


BOOST_AUTO_TEST_CASE( StableIdentityUsesTypedKindAndTarget )
{
    const SNAP_TARGET_ID target = targetId( "target" );
    const SNAP_STABLE_ID first{ SNAP_ID_KIND::ITEM_GEOMETRY, target, 2, 1 };
    const SNAP_STABLE_ID same{ SNAP_ID_KIND::ITEM_GEOMETRY, target, 2, 1 };
    const SNAP_STABLE_ID differentKind{ SNAP_ID_KIND::INTRINSIC_ANCHOR, target, 2, 1 };

    BOOST_CHECK( first == same );
    BOOST_CHECK( first != differentKind );
    BOOST_CHECK( SNAP_STABLE_ID{}.target == SNAP_TARGET_ID{} );
    BOOST_CHECK( KIID::FromBytes( target ) == KIID::FromName( "target" ) );
}


BOOST_AUTO_TEST_CASE( PointIdentityPreservesCoordinateOrder )
{
    BOOST_CHECK( MakePointSnapId( SNAP_ID_KIND::CONSTRUCTION, { 1, 2 } )
                 != MakePointSnapId( SNAP_ID_KIND::CONSTRUCTION, { 2, 1 } ) );
}


BOOST_AUTO_TEST_CASE( IntersectionIdentityIsSourceOrderIndependent )
{
    const SNAP_STABLE_ID first{ SNAP_ID_KIND::ITEM_GEOMETRY, KIID( "00000000-0000-0000-0000-000000000001" ).AsBytes(),
                                1, 2 };
    const SNAP_STABLE_ID second{ SNAP_ID_KIND::ITEM_GEOMETRY, KIID( "00000000-0000-0000-0000-000000000002" ).AsBytes(),
                                 3, 4 };

    BOOST_CHECK( MakeIntersectionSnapId( first, second, 5 ) == MakeIntersectionSnapId( second, first, 5 ) );
}


BOOST_AUTO_TEST_CASE( CompositeIdentityIsOrderIndependentWithoutXorCancellation )
{
    const SNAP_TARGET_ID first = KIID( "00000000-0000-0000-0000-000000000001" ).AsBytes();
    const SNAP_TARGET_ID second = KIID( "00000000-0000-0000-0000-000000000002" ).AsBytes();

    BOOST_CHECK( MakeCompositeSnapId( SNAP_ID_KIND::INTRINSIC_ANCHOR, { first, second }, 3 )
                 == MakeCompositeSnapId( SNAP_ID_KIND::INTRINSIC_ANCHOR, { second, first }, 3 ) );
    BOOST_CHECK( MakeCompositeSnapId( SNAP_ID_KIND::INTRINSIC_ANCHOR, { first, first }, 3 ).target
                 != SNAP_TARGET_ID{} );
}


BOOST_AUTO_TEST_CASE( IdentityConstructionHasStableGoldenBytes )
{
    const SNAP_STABLE_ID first{ SNAP_ID_KIND::ITEM_GEOMETRY, KIID( "00000000-0000-0000-0000-000000000001" ).AsBytes(),
                                1, 2 };
    const SNAP_STABLE_ID second{ SNAP_ID_KIND::INTRINSIC_ANCHOR,
                                 KIID( "00000000-0000-0000-0000-000000000002" ).AsBytes(), 3, 4 };

    BOOST_CHECK_EQUAL( targetBytes( MakePointSnapId( SNAP_ID_KIND::CONSTRUCTION, { 123, -456 }, 7 ) ),
                       "b1fa72f98d0eaaf396ce1f1030e5f274" );
    BOOST_CHECK_EQUAL( targetBytes( MakeDerivedSnapId( SNAP_ID_KIND::BOUNDS_X, first, 8, 9 ) ),
                       "38ec5cc638db882f25564f708cd908d4" );
    BOOST_CHECK_EQUAL( targetBytes( MakeIntersectionSnapId( first, second, 5 ) ), "b260a931293228a311feb6c704737605" );
    BOOST_CHECK_EQUAL( targetBytes( MakeCompositeSnapId( SNAP_ID_KIND::INTRINSIC_ANCHOR,
                                                         { KIID( "00000000-0000-0000-0000-000000000002" ).AsBytes(),
                                                           KIID( "00000000-0000-0000-0000-000000000001" ).AsBytes() },
                                                         3 ) ),
                       "9084556ddbcdd0543f07abd73d7fb746" );
}


BOOST_AUTO_TEST_CASE( AuthoredRelationWinsEveryTransientTier )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 18, 19 };

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "cursor" ), SNAP_PRIORITY_TIER::CURSOR,
                                                  SNAP_CANDIDATE_SUBTYPE::CURSOR, { 18, 19 }, 0.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "object" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD, { 20, 20 }, 1.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "authored" ), SNAP_PRIORITY_TIER::AUTHORED_INTRINSIC,
                                                  SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR, { 10, 10 }, 20.0 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_REQUIRE_EQUAL( result.status, SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 10, 10 ) );
    BOOST_REQUIRE_EQUAL( result.accepted.size(), 1 );
    BOOST_CHECK( result.accepted.front() == id( "authored" ) );
}


BOOST_AUTO_TEST_CASE( AngleRejectsIncompatibleObjectWithoutRelaxing )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 12, 4 };

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( SNAP_CANDIDATE::Line( id( "angle" ), SNAP_PRIORITY_TIER::ANGLE,
                                                 SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH, { 0, 0 }, { 1.0, 0.0 }, 4.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "object" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT, { 10, 5 }, 1.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "grid-x" ), SNAP_PRIORITY_TIER::GRID,
                                                  SNAP_CANDIDATE_SUBTYPE::GRID_AXIS, 10, 2.0 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_REQUIRE_EQUAL( result.status, SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 10, 0 ) );
    BOOST_CHECK( result.Accepted( id( "angle" ) ) );
    BOOST_CHECK( !result.Accepted( id( "object" ) ) );
    BOOST_CHECK( result.Accepted( id( "grid-x" ) ) );
}


BOOST_AUTO_TEST_CASE( CompatibleObjectAxesCompose )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 14, 17 };

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "align-x" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 10, 4.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisY( id( "align-y" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 20, 3.0 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_REQUIRE_EQUAL( result.status, SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 10, 20 ) );
    BOOST_CHECK( result.Accepted( id( "align-x" ) ) );
    BOOST_CHECK( result.Accepted( id( "align-y" ) ) );
    BOOST_CHECK_EQUAL( result.remainingDof, 0 );
}


BOOST_AUTO_TEST_CASE( ConflictingGridAxisLeavesCompatibleAxisAccepted )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 9, 13 };

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( SNAP_CANDIDATE::Line( id( "angle" ), SNAP_PRIORITY_TIER::ANGLE,
                                                 SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH, { 0, 0 }, { 1.0, 1.0 }, 3.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "grid-x" ), SNAP_PRIORITY_TIER::GRID,
                                                  SNAP_CANDIDATE_SUBTYPE::GRID_AXIS, 10, 1.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisY( id( "grid-y" ), SNAP_PRIORITY_TIER::GRID,
                                                  SNAP_CANDIDATE_SUBTYPE::GRID_AXIS, 15, 2.0 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_REQUIRE_EQUAL( result.status, SNAP_RESULT_STATUS::SUCCESS );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 10, 10 ) );
    BOOST_CHECK( result.Accepted( id( "grid-x" ) ) );
    BOOST_CHECK( !result.Accepted( id( "grid-y" ) ) );
}


BOOST_AUTO_TEST_CASE( StableIdentityBreaksExactRankTie )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 0, 0 };

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "z" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT, { 5, 5 }, 1.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "a" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT, { 6, 6 }, 1.0 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_REQUIRE_EQUAL( result.accepted.size(), 1 );
    BOOST_CHECK( result.accepted.front() == std::min( id( "a" ), id( "z" ) ) );
}


BOOST_AUTO_TEST_CASE( FixedCursorPathIsIndependentOfCandidateStorageOrder )
{
    const std::vector<VECTOR2I> cursorPath = { { 0, 0 }, { 3, 5 }, { 7, 9 }, { 11, 13 } };

    const auto resolvePath = [&]( bool aReverse )
    {
        std::vector<std::string> signatures;

        for( const VECTOR2I& cursor : cursorPath )
        {
            std::vector<SNAP_CANDIDATE> candidates = {
                SNAP_CANDIDATE::AxisX( id( "x-z" ), SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT,
                                       cursor.x + 2, 0.2 ),
                SNAP_CANDIDATE::AxisX( id( "x-a" ), SNAP_PRIORITY_TIER::OBJECT, SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT,
                                       cursor.x + 1, 0.2 ),
                SNAP_CANDIDATE::AxisY( id( "y" ), SNAP_PRIORITY_TIER::GRID, SNAP_CANDIDATE_SUBTYPE::GRID_AXIS,
                                       cursor.y - 1, 0.1 )
            };

            if( aReverse )
                std::reverse( candidates.begin(), candidates.end() );

            SNAP_SOURCE_CONTEXT context;
            context.sourcePoint = cursor;
            SNAP_RESOLVER resolver;

            for( SNAP_CANDIDATE& candidate : candidates )
                resolver.AddCandidate( std::move( candidate ) );

            signatures.push_back( resultSignature( resolver.Resolve( context ) ) );
        }

        return signatures;
    };

    BOOST_CHECK( resolvePath( false ) == resolvePath( true ) );
}


BOOST_AUTO_TEST_CASE( ReferenceAffinityOutranksDistance )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 0, 0 };

    SNAP_CANDIDATE preferred = SNAP_CANDIDATE::AxisX( id( "preferred" ), SNAP_PRIORITY_TIER::OBJECT,
                                                      SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 8, 0.32 );
    preferred.referenceAffinity = 0;

    SNAP_CANDIDATE fallback = SNAP_CANDIDATE::AxisX( id( "fallback" ), SNAP_PRIORITY_TIER::OBJECT,
                                                     SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 1, 0.04 );
    fallback.referenceAffinity = 1;

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( std::move( fallback ) );
    resolver.AddCandidate( std::move( preferred ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK( result.Accepted( id( "preferred" ) ) );
    BOOST_CHECK( !result.Accepted( id( "fallback" ) ) );
}


BOOST_AUTO_TEST_CASE( InfeasiblePreferredReferenceFallsBack )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 0, 0 };

    SNAP_CANDIDATE preferred = SNAP_CANDIDATE::AxisX( id( "preferred" ), SNAP_PRIORITY_TIER::OBJECT,
                                                      SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 8, 0.32 );
    preferred.referenceAffinity = 0;

    SNAP_CANDIDATE fallback = SNAP_CANDIDATE::AxisX( id( "fallback" ), SNAP_PRIORITY_TIER::OBJECT,
                                                     SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 1, 0.04 );
    fallback.referenceAffinity = 1;

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( std::move( fallback ) );
    resolver.AddCandidate( std::move( preferred ) );
    resolver.SetFeasibilityCallback(
            [&]( const SNAP_SOURCE_CONTEXT&, const std::vector<SNAP_CANDIDATE>& aTrial )
            {
                SNAP_RESULT result;
                result.position = context.sourcePoint;

                if( aTrial.empty() )
                    return result;

                if( aTrial.back().id == id( "preferred" ) )
                {
                    result.status = SNAP_RESULT_STATUS::INCOMPATIBLE;
                    return result;
                }

                result.position.x = KiROUND( aTrial.back().origin.x );
                result.remainingDof = 1;
                return result;
            } );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK( !result.Accepted( id( "preferred" ) ) );
    BOOST_CHECK( result.Accepted( id( "fallback" ) ) );
    BOOST_CHECK_EQUAL( result.position.x, 1 );
}


BOOST_AUTO_TEST_CASE( FullyConstrainedAnchorSuppressesRedundantLayoutFeedback )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 9, 9 };

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "anchor" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR, { 10, 10 }, 1.0 ) );

    SNAP_CANDIDATE alignment = SNAP_CANDIDATE::AxisX( id( "alignment" ), SNAP_PRIORITY_TIER::OBJECT,
                                                      SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 10, 1.0 );
    resolver.AddCandidate( std::move( alignment ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_REQUIRE_EQUAL( result.accepted.size(), 1 );
    BOOST_CHECK( result.Accepted( id( "anchor" ) ) );
    BOOST_CHECK( !result.Accepted( id( "alignment" ) ) );
}


BOOST_AUTO_TEST_CASE( AdjacentAngleBranchesRemainAlternatives )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 10, 4 };

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( SNAP_CANDIDATE::Line( id( "horizontal" ), SNAP_PRIORITY_TIER::ANGLE,
                                                 SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH, { 0, 0 }, { 1.0, 0.0 }, 4.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Line( id( "vertical" ), SNAP_PRIORITY_TIER::ANGLE,
                                                 SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH, { 0, 0 }, { 0.0, 1.0 }, 10.0 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 10, 0 ) );
    BOOST_CHECK( result.Accepted( id( "horizontal" ) ) );
    BOOST_CHECK( !result.Accepted( id( "vertical" ) ) );
}


BOOST_AUTO_TEST_CASE( JointFeasibilityRunsFromCommonBaseForEveryTrial )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 12, 4 };
    int calls = 0;

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( SNAP_CANDIDATE::Line( id( "angle" ), SNAP_PRIORITY_TIER::ANGLE,
                                                 SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH, { 0, 0 }, { 1.0, 0.0 }, 4.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "object" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::CONSTRUCTED_POINT, { 10, 5 }, 1.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "grid-x" ), SNAP_PRIORITY_TIER::GRID,
                                                  SNAP_CANDIDATE_SUBTYPE::GRID_AXIS, 10, 2.0 ) );
    resolver.SetFeasibilityCallback(
            [&]( const SNAP_SOURCE_CONTEXT&, const std::vector<SNAP_CANDIDATE>& aTrial )
            {
                ++calls;
                SNAP_RESULT result;

                if( aTrial.empty() )
                {
                    result.position = context.sourcePoint;
                    return result;
                }

                if( aTrial.back().id == id( "object" ) )
                {
                    result.status = SNAP_RESULT_STATUS::INCOMPATIBLE;
                    return result;
                }

                result.position = aTrial.back().id == id( "grid-x" ) ? VECTOR2I( 10, 0 ) : VECTOR2I( 12, 0 );
                result.remainingDof = aTrial.size() == 1 ? 1 : 0;
                return result;
            } );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK_EQUAL( calls, 4 );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 10, 0 ) );
    BOOST_CHECK( result.Accepted( id( "angle" ) ) );
    BOOST_CHECK( !result.Accepted( id( "object" ) ) );
    BOOST_CHECK( result.Accepted( id( "grid-x" ) ) );
}


BOOST_AUTO_TEST_CASE( MandatoryPrefixCompletesAfterDeadline )
{
    using CLOCK = std::chrono::steady_clock;

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 8, 3 };
    CLOCK::time_point now;
    int               optionalCalls = 0;

    SNAP_RESOLVER resolver;
    resolver.SetClock(
            [&]
            {
                return now;
            } );
    resolver.SetDeadline( std::chrono::milliseconds( 8 ) );
    resolver.SetRetainedCandidate( id( "retained" ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Line( id( "angle-a" ), SNAP_PRIORITY_TIER::ANGLE,
                                                 SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH, { 0, 0 }, { 1.0, 0.0 }, 3.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Line( id( "angle-b" ), SNAP_PRIORITY_TIER::ANGLE,
                                                 SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH, { 0, 0 }, { 1.0, 1.0 }, 4.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "retained" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD, { 8, 0 }, 3.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "intrinsic" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR, 8, 0.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisY( id( "optional" ), SNAP_PRIORITY_TIER::GRID,
                                                  SNAP_CANDIDATE_SUBTYPE::GRID_AXIS, 0, 3.0 ) );
    resolver.SetFeasibilityCallback(
            [&]( const SNAP_SOURCE_CONTEXT&, const std::vector<SNAP_CANDIDATE>& aTrial )
            {
                SNAP_RESULT result;
                result.position = context.sourcePoint;

                if( !aTrial.empty() )
                {
                    now += std::chrono::milliseconds( 3 );

                    if( aTrial.back().id == id( "optional" ) )
                        ++optionalCalls;
                }

                return result;
            } );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK_EQUAL( optionalCalls, 0 );
    BOOST_CHECK_EQUAL( result.status, SNAP_RESULT_STATUS::BUDGET_EXHAUSTED );
    BOOST_CHECK( result.Accepted( id( "angle-a" ) ) );
    BOOST_CHECK( result.Accepted( id( "retained" ) ) );
    BOOST_CHECK( result.Accepted( id( "intrinsic" ) ) );
}


BOOST_AUTO_TEST_CASE( RetainedAngleBranchUsesFivePixelHysteresis )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 10, 4 };

    SNAP_RESOLVER resolver;
    resolver.SetRetainedCandidate( id( "horizontal" ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Line( id( "horizontal" ), SNAP_PRIORITY_TIER::ANGLE,
                                                 SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH, { 0, 0 }, { 1.0, 0.0 }, 0.16 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Line( id( "diagonal" ), SNAP_PRIORITY_TIER::ANGLE,
                                                 SNAP_CANDIDATE_SUBTYPE::ANGLE_BRANCH, { 0, 0 }, { 1.0, 1.0 }, 0.02 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK( result.Accepted( id( "horizontal" ) ) );
    BOOST_CHECK( !result.Accepted( id( "diagonal" ) ) );
}


BOOST_AUTO_TEST_CASE( StickyCandidateHoldsWithinHysteresis )
{
    // A snap accepted on the previous resolve keeps winning even when a competitor for the same
    // axis is nearer, as long as the gap is inside the ranking hysteresis. Without this the
    // equal-spacing solutions jitter frame to frame.
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 4, 0 };

    SNAP_RESOLVER resolver;
    resolver.SetStickyCandidates( { id( "held" ) } );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "held" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 0, 0.16 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "nearer" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 5, 0.02 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK( result.Accepted( id( "held" ) ) );
    BOOST_CHECK( !result.Accepted( id( "nearer" ) ) );
}


BOOST_AUTO_TEST_CASE( StickyCandidateReleasesBeyondHysteresis )
{
    // Once a competitor is clearly nearer (beyond the hysteresis margin) the sticky snap yields, so
    // the hysteresis stabilizes selection without trapping the cursor on a stale solution.
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 4, 0 };

    SNAP_RESOLVER resolver;
    resolver.SetStickyCandidates( { id( "held" ) } );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "held" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 0, 0.5 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "nearer" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 5, 0.02 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK( result.Accepted( id( "nearer" ) ) );
    BOOST_CHECK( !result.Accepted( id( "held" ) ) );
}


BOOST_AUTO_TEST_CASE( TraceCallbackReportsRankingTrialsAndResult )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 4, 0 };
    std::vector<std::string> trace;

    SNAP_RESOLVER resolver;
    resolver.SetTraceCallback(
            [&]( const std::string& aMessage )
            {
                trace.push_back( aMessage );
            } );
    resolver.SetStickyCandidates( { id( "held" ) } );
    SNAP_CANDIDATE held = SNAP_CANDIDATE::AxisX( id( "held" ), SNAP_PRIORITY_TIER::OBJECT,
                                                 SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 0, 0.16 );
    resolver.AddCandidate( std::move( held ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "nearer" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 5, 0.02 ) );

    resolver.Resolve( context );

    const auto contains = [&]( const char* aText )
    {
        return std::any_of( trace.begin(), trace.end(),
                            [&]( const std::string& aMessage )
                            {
                                return aMessage.find( aText ) != std::string::npos;
                            } );
    };

    BOOST_CHECK( contains( "rank index=0" ) );
    BOOST_CHECK( contains( "sticky=1" ) );
    BOOST_CHECK( contains( "trial id=" ) );
    BOOST_CHECK( contains( "accepted=1" ) );
    BOOST_CHECK( contains( "result status=SUCCESS position=(0,0) accepted=[" ) );
}


BOOST_AUTO_TEST_CASE( IntrinsicObjectOutranksIncompatibleRetainedTarget )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 3, 4 };

    SNAP_RESOLVER resolver;
    resolver.SetRetainedCandidate( id( "retained" ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "retained" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD, { 10, 10 }, 0.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::Point( id( "intrinsic" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::INTRINSIC_ANCHOR, { 2, 2 }, 1.0 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK( result.Accepted( id( "intrinsic" ) ) );
    BOOST_CHECK( !result.Accepted( id( "retained" ) ) );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 2, 2 ) );
}


BOOST_AUTO_TEST_CASE( RunningTrialFinishesButNoFurtherTrialStartsAfterOverrun )
{
    using CLOCK = std::chrono::steady_clock;

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 4, 6 };
    CLOCK::time_point now;
    int               trials = 0;

    SNAP_RESOLVER resolver;
    resolver.SetClock(
            [&]
            {
                return now;
            } );
    resolver.SetDeadline( std::chrono::milliseconds( 8 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisX( id( "first" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD, 4, 0.0 ) );
    resolver.AddCandidate( SNAP_CANDIDATE::AxisY( id( "second" ), SNAP_PRIORITY_TIER::OBJECT,
                                                  SNAP_CANDIDATE_SUBTYPE::FINITE_MANIFOLD, 6, 0.0 ) );
    resolver.SetFeasibilityCallback(
            [&]( const SNAP_SOURCE_CONTEXT&, const std::vector<SNAP_CANDIDATE>& )
            {
                SNAP_RESULT result;
                result.position = context.sourcePoint;
                ++trials;

                if( trials == 2 )
                    now += std::chrono::milliseconds( 9 );

                return result;
            } );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK_EQUAL( trials, 2 );
    BOOST_CHECK_EQUAL( result.status, SNAP_RESULT_STATUS::BUDGET_EXHAUSTED );
    BOOST_CHECK( result.Accepted( std::min( id( "first" ), id( "second" ) ) ) );
    BOOST_CHECK( !result.Accepted( std::max( id( "first" ), id( "second" ) ) ) );
}


BOOST_AUTO_TEST_CASE( AcceptedCandidateForwardsAllGuides )
{
    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 10, 10 };

    SNAP_CANDIDATE accepted = SNAP_CANDIDATE::AxisX( id( "accepted" ), SNAP_PRIORITY_TIER::OBJECT,
                                                     SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 12, 0.1 );
    accepted.guides = { { { 0, 0 }, { 5, 0 }, SNAP_GUIDE_STYLE::DIMENSION_BRACKET },
                        { { 10, 0 }, { 15, 0 }, SNAP_GUIDE_STYLE::DIMENSION_BRACKET } };

    SNAP_CANDIDATE rejected = SNAP_CANDIDATE::AxisX( id( "rejected" ), SNAP_PRIORITY_TIER::OBJECT,
                                                     SNAP_CANDIDATE_SUBTYPE::BBOX_LAYOUT, 20, 0.5 );
    rejected.guides = { { { 20, 0 }, { 25, 0 }, SNAP_GUIDE_STYLE::DIMENSION_BRACKET } };

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( std::move( rejected ) );
    resolver.AddCandidate( std::move( accepted ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_REQUIRE_EQUAL( result.guides.size(), 2 );
    BOOST_CHECK_EQUAL( result.guides[0].start, VECTOR2I( 0, 0 ) );
    BOOST_CHECK_EQUAL( result.guides[0].end, VECTOR2I( 5, 0 ) );
    BOOST_CHECK_EQUAL( result.guides[1].start, VECTOR2I( 10, 0 ) );
    BOOST_CHECK_EQUAL( result.guides[1].end, VECTOR2I( 15, 0 ) );
}


BOOST_AUTO_TEST_SUITE_END()
