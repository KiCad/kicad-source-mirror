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
#include <snap/snap_inference.h>
#include <settings/snap_settings.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <map>


namespace
{
SNAP_TARGET_ID targetId( const char* aName )
{
    return KIID::FromName( aName ).AsBytes();
}


SNAP_STABLE_ID featureId( const char* aItem, const char* aFeature )
{
    return { SNAP_ID_KIND::ITEM_GEOMETRY, targetId( aItem ), static_cast<int>( KIID::FromName( aFeature ).Hash() ), 0 };
}


template <typename T>
VECTOR2<T> transpose( const VECTOR2<T>& aPoint )
{
    return { aPoint.y, aPoint.x };
}


BOX2I transpose( const BOX2I& aBox )
{
    return { transpose( aBox.GetOrigin() ), transpose( aBox.GetSize() ) };
}


SNAP_ID_KIND transpose( SNAP_ID_KIND aKind )
{
    switch( aKind )
    {
    case SNAP_ID_KIND::BOUNDS_X: return SNAP_ID_KIND::BOUNDS_Y;
    case SNAP_ID_KIND::BOUNDS_Y: return SNAP_ID_KIND::BOUNDS_X;
    case SNAP_ID_KIND::ANCHOR_POINT_X: return SNAP_ID_KIND::ANCHOR_POINT_Y;
    case SNAP_ID_KIND::ANCHOR_POINT_Y: return SNAP_ID_KIND::ANCHOR_POINT_X;
    case SNAP_ID_KIND::EQUAL_GAP_X: return SNAP_ID_KIND::EQUAL_GAP_Y;
    case SNAP_ID_KIND::EQUAL_GAP_Y: return SNAP_ID_KIND::EQUAL_GAP_X;
    case SNAP_ID_KIND::COPY_GAP_X: return SNAP_ID_KIND::COPY_GAP_Y;
    case SNAP_ID_KIND::COPY_GAP_Y: return SNAP_ID_KIND::COPY_GAP_X;
    case SNAP_ID_KIND::GRID_X: return SNAP_ID_KIND::GRID_Y;
    case SNAP_ID_KIND::GRID_Y: return SNAP_ID_KIND::GRID_X;
    default: return aKind;
    }
}


SNAP_CANDIDATE transpose( SNAP_CANDIDATE aCandidate )
{
    aCandidate.id.kind = transpose( aCandidate.id.kind );
    aCandidate.origin = transpose( aCandidate.origin );
    aCandidate.direction = transpose( aCandidate.direction );

    for( SNAP_GUIDE& guide : aCandidate.guides )
    {
        guide.start = transpose( guide.start );
        guide.end = transpose( guide.end );
    }

    return aCandidate;
}


void checkTransposeEquivalent( std::vector<SNAP_CANDIDATE> aExpected, std::vector<SNAP_CANDIDATE> aTransposed )
{
    for( SNAP_CANDIDATE& candidate : aTransposed )
        candidate = transpose( std::move( candidate ) );

    const auto byId = []( const SNAP_CANDIDATE& aLeft, const SNAP_CANDIDATE& aRight )
    {
        return aLeft.id < aRight.id;
    };
    std::sort( aExpected.begin(), aExpected.end(), byId );
    std::sort( aTransposed.begin(), aTransposed.end(), byId );
    BOOST_REQUIRE_EQUAL( aExpected.size(), aTransposed.size() );

    for( size_t i = 0; i < aExpected.size(); ++i )
    {
        const SNAP_CANDIDATE& expected = aExpected[i];
        const SNAP_CANDIDATE& actual = aTransposed[i];
        BOOST_CHECK( expected.id == actual.id );
        BOOST_CHECK( expected.priority == actual.priority );
        BOOST_CHECK( expected.subtype == actual.subtype );
        BOOST_CHECK( expected.relation == actual.relation );
        BOOST_CHECK_EQUAL( expected.origin, actual.origin );
        BOOST_CHECK_EQUAL( expected.direction, actual.direction );
        BOOST_CHECK_EQUAL( expected.normalizedScreenResidual, actual.normalizedScreenResidual );
        BOOST_CHECK_EQUAL( expected.referenceAffinity, actual.referenceAffinity );
        BOOST_CHECK_EQUAL( expected.consumedDof, actual.consumedDof );
        BOOST_CHECK_EQUAL( expected.finite, actual.finite );
        BOOST_CHECK_EQUAL( expected.domainStart, actual.domainStart );
        BOOST_CHECK_EQUAL( expected.domainEnd, actual.domainEnd );
        BOOST_REQUIRE_EQUAL( expected.guides.size(), actual.guides.size() );

        for( size_t guide = 0; guide < expected.guides.size(); ++guide )
        {
            BOOST_CHECK_EQUAL( expected.guides[guide].start, actual.guides[guide].start );
            BOOST_CHECK_EQUAL( expected.guides[guide].end, actual.guides[guide].end );
            BOOST_CHECK( expected.guides[guide].style == actual.guides[guide].style );
        }

        BOOST_CHECK( expected.manifold == actual.manifold );
    }
}
} // namespace


BOOST_AUTO_TEST_SUITE( SnapInference )


BOOST_AUTO_TEST_CASE( GroupDefaultsPreserveConservativeInference )
{
    SNAP_INFERENCE_SETTINGS settings;

    BOOST_CHECK( settings.objectGeometry );
    BOOST_CHECK( settings.constructionExtensions );
    BOOST_CHECK( !settings.tangentNormal );
    BOOST_CHECK( !settings.alignmentDistribution );
}


BOOST_AUTO_TEST_CASE( FiniteSegmentsRejectExtensionOnlyIntersection )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddPath( { featureId( "horizontal", "edge" ), SEG( { 0, 0 }, { 10, 0 } ) } );
    provider.AddPath( { featureId( "vertical", "edge" ), SEG( { 20, -10 }, { 20, 10 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 20, 0 };

    std::vector<SNAP_CANDIDATE> finite = provider.CollectObjectGeometry( context, 25 );
    BOOST_CHECK( std::none_of( finite.begin(), finite.end(),
                               []( const SNAP_CANDIDATE& aCandidate )
                               {
                                   return aCandidate.subtype == SNAP_CANDIDATE_SUBTYPE::INTERSECTION;
                               } ) );

    provider.ActivateExtension( featureId( "horizontal", "edge" ) );
    std::vector<SNAP_CANDIDATE> extended = provider.CollectObjectGeometry( context, 25 );
    BOOST_CHECK( std::any_of( extended.begin(), extended.end(),
                              []( const SNAP_CANDIDATE& aCandidate )
                              {
                                  return aCandidate.subtype == SNAP_CANDIDATE_SUBTYPE::INTERSECTION;
                              } ) );
}


BOOST_AUTO_TEST_CASE( IntersectionBranchIdentityDoesNotFollowCursorDistance )
{
    SNAP_INFERENCE_PROVIDER provider;
    SNAP_STABLE_ID          first = featureId( "first", "circle" );
    first.featureIndex = 2;
    SNAP_STABLE_ID second = featureId( "second", "circle" );
    second.solutionBranch = 3;
    provider.AddPath( { first, CIRCLE( { -3, 0 }, 5 ) } );
    provider.AddPath( { second, CIRCLE( { 3, 0 }, 5 ) } );

    const auto intersectionIds = [&]( const VECTOR2I& aCursor )
    {
        SNAP_SOURCE_CONTEXT context;
        context.sourcePoint = aCursor;
        std::map<VECTOR2I, SNAP_STABLE_ID> ids;

        for( const SNAP_CANDIDATE& candidate : provider.CollectObjectGeometry( context, 20 ) )
        {
            if( candidate.subtype == SNAP_CANDIDATE_SUBTYPE::INTERSECTION )
            {
                ids.emplace( VECTOR2I( KiROUND( candidate.origin.x ), KiROUND( candidate.origin.y ) ), candidate.id );
            }
        }

        return ids;
    };

    auto upperCursor = intersectionIds( { 0, 10 } );
    auto lowerCursor = intersectionIds( { 0, -10 } );
    BOOST_REQUIRE_EQUAL( upperCursor.size(), 2 );
    BOOST_CHECK( upperCursor == lowerCursor );
    BOOST_CHECK( upperCursor.begin()->second.kind == SNAP_ID_KIND::INTERSECTION );
    BOOST_CHECK( upperCursor.begin()->second.target
                 == MakeIntersectionSnapId( first, second, upperCursor.begin()->second.solutionBranch ).target );
}


BOOST_AUTO_TEST_CASE( IntersectionBranchIdentitySurvivesSiblingRadiusFiltering )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddPath( { featureId( "first", "circle" ), CIRCLE( { -3, 0 }, 5 ) } );
    provider.AddPath( { featureId( "second", "circle" ), CIRCLE( { 3, 0 }, 5 ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 0, 4 };

    const auto upperIntersectionId = [&]( int aRadius )
    {
        for( const SNAP_CANDIDATE& candidate : provider.CollectObjectGeometry( context, aRadius ) )
        {
            if( candidate.subtype == SNAP_CANDIDATE_SUBTYPE::INTERSECTION && candidate.origin == VECTOR2D( 0.0, 4.0 ) )
            {
                return std::optional<SNAP_STABLE_ID>( candidate.id );
            }
        }

        return std::optional<SNAP_STABLE_ID>();
    };

    std::optional<SNAP_STABLE_ID> isolated = upperIntersectionId( 1 );
    std::optional<SNAP_STABLE_ID> withSibling = upperIntersectionId( 10 );
    BOOST_REQUIRE( isolated );
    BOOST_REQUIRE( withSibling );
    BOOST_CHECK( *isolated == *withSibling );
}


BOOST_AUTO_TEST_CASE( ActiveExtensionRayRetainsItsFiniteDirection )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddPath(
            { featureId( "line", "extension" ), HALF_LINE( VECTOR2I( 0, 0 ), VECTOR2I( 10, 0 ) ), false, true } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { -20, 0 };

    BOOST_CHECK( provider.CollectObjectGeometry( context, 10 ).empty() );

    context.sourcePoint = { 5, 2 };
    std::vector<SNAP_CANDIDATE> candidates = provider.CollectObjectGeometry( context, 10 );

    BOOST_REQUIRE_EQUAL( candidates.size(), 1 );
    BOOST_CHECK( candidates.front().subtype == SNAP_CANDIDATE_SUBTYPE::ACTIVE_EXTENSION );
    BOOST_CHECK( candidates.front().relation == SNAP_RELATION::POINT_ON_RAY );

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( candidates.front() );
    SNAP_RESULT result = resolver.Resolve( context );
    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 5, 0 ) );
}


BOOST_AUTO_TEST_CASE( MovingFeaturesExcludedButStationarySelfFeaturesRemain )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddPath( { featureId( "polygon", "moving-edge" ), SEG( { 0, 0 }, { 10, 0 } ) } );
    provider.AddPath( { featureId( "polygon", "fixed-edge" ), SEG( { 0, 10 }, { 10, 10 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 5, 5 };
    context.movingFeatures.push_back( featureId( "polygon", "moving-edge" ) );
    context.stationarySelfFeatures.push_back( featureId( "polygon", "fixed-edge" ) );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectObjectGeometry( context, 20 );

    BOOST_CHECK( std::none_of( candidates.begin(), candidates.end(),
                               []( const SNAP_CANDIDATE& aCandidate )
                               {
                                   return aCandidate.id == featureId( "polygon", "moving-edge" );
                               } ) );
    BOOST_CHECK( std::any_of( candidates.begin(), candidates.end(),
                              []( const SNAP_CANDIDATE& aCandidate )
                              {
                                  return aCandidate.id == featureId( "polygon", "fixed-edge" );
                              } ) );
}


BOOST_AUTO_TEST_CASE( CircleNormalEnumeratesBothFiniteBranches )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddPath( { featureId( "circle", "outline" ), CIRCLE( { 0, 0 }, 5 ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 8, 1 };
    context.stationarySourceLeg = VECTOR2I( 10, 0 );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectTangentNormal( context, 20, false, true );

    BOOST_REQUIRE_EQUAL( candidates.size(), 2 );
    BOOST_CHECK( std::any_of( candidates.begin(), candidates.end(),
                              []( const SNAP_CANDIDATE& aCandidate )
                              {
                                  return aCandidate.origin == VECTOR2D( 5.0, 0.0 );
                              } ) );
    BOOST_CHECK( std::any_of( candidates.begin(), candidates.end(),
                              []( const SNAP_CANDIDATE& aCandidate )
                              {
                                  return aCandidate.origin == VECTOR2D( -5.0, 0.0 );
                              } ) );
}


BOOST_AUTO_TEST_CASE( NormalBranchIdentityDoesNotDependOnTangentSetting )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddPath( { featureId( "circle", "outline" ), CIRCLE( { 0, 0 }, 5 ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 5, 0 };
    context.stationarySourceLeg = VECTOR2I( 10, 0 );

    const auto positiveNormalId = [&]( bool aTangentEnabled )
    {
        for( const SNAP_CANDIDATE& candidate : provider.CollectTangentNormal( context, 20, aTangentEnabled, true ) )
        {
            if( candidate.relation == SNAP_RELATION::NORMAL && candidate.origin == VECTOR2D( 5.0, 0.0 ) )
            {
                return std::optional<SNAP_STABLE_ID>( candidate.id );
            }
        }

        return std::optional<SNAP_STABLE_ID>();
    };

    std::optional<SNAP_STABLE_ID> withoutTangents = positiveNormalId( false );
    std::optional<SNAP_STABLE_ID> withTangents = positiveNormalId( true );
    BOOST_REQUIRE( withoutTangents );
    BOOST_REQUIRE( withTangents );
    BOOST_CHECK( *withoutTangents == *withTangents );
}


BOOST_AUTO_TEST_CASE( ArcNormalRejectsContactOutsideFiniteDomain )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddPath( { featureId( "arc", "outline" ), SHAPE_ARC( VECTOR2I( 0, 0 ), VECTOR2I( 5, 0 ), ANGLE_90 ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 5, 1 };
    context.stationarySourceLeg = VECTOR2I( 10, 0 );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectTangentNormal( context, 20, false, true );

    BOOST_REQUIRE_EQUAL( candidates.size(), 1 );
    BOOST_CHECK_GT( candidates.front().origin.x, 0.0 );
    BOOST_CHECK( candidates.front().relation == SNAP_RELATION::NORMAL );

    SNAP_RESOLVER resolver;
    resolver.AddCandidate( candidates.front() );
    SNAP_RESULT result = resolver.Resolve( context );
    BOOST_CHECK_EQUAL( result.position,
                       VECTOR2I( KiROUND( candidates.front().origin.x ), KiROUND( candidates.front().origin.y ) ) );
}


BOOST_AUTO_TEST_CASE( CircleRemainsComposableManifold )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddPath( { featureId( "circle", "outline" ), CIRCLE( { 0, 0 }, 5 ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 4, 4 };

    SNAP_RESOLVER resolver;

    for( SNAP_CANDIDATE& candidate : provider.CollectObjectGeometry( context, 20 ) )
        resolver.AddCandidate( std::move( candidate ) );

    resolver.AddCandidate( SNAP_CANDIDATE::AxisY( featureId( "grid", "y" ), SNAP_PRIORITY_TIER::GRID,
                                                  SNAP_CANDIDATE_SUBTYPE::GRID_AXIS, 3, 1.0 ) );

    SNAP_RESULT result = resolver.Resolve( context );

    BOOST_CHECK_EQUAL( result.position, VECTOR2I( 4, 3 ) );
    BOOST_CHECK( result.Accepted( featureId( "circle", "outline" ) ) );
    BOOST_CHECK( result.Accepted( featureId( "grid", "y" ) ) );
    BOOST_CHECK_EQUAL( result.remainingDof, 0 );
}


BOOST_AUTO_TEST_CASE( BoundingBoxFeaturesTranslateSourcePointExactly )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "target", "bounds" ), BOX2I( { 0, 0 }, { 20, 20 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 45, 45 };
    context.movingBounds = BOX2I( { 40, 40 }, { 10, 10 } );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectAlignment( context, 50 );

    BOOST_CHECK( std::any_of( candidates.begin(), candidates.end(),
                              []( const SNAP_CANDIDATE& aCandidate )
                              {
                                  return aCandidate.relation == SNAP_RELATION::BBOX_ALIGNMENT
                                         && aCandidate.origin.x == 5.0;
                              } ) );
    BOOST_CHECK( std::any_of( candidates.begin(), candidates.end(),
                              []( const SNAP_CANDIDATE& aCandidate )
                              {
                                  return aCandidate.relation == SNAP_RELATION::BBOX_ALIGNMENT
                                         && aCandidate.origin.y == 25.0;
                              } ) );

    auto xCandidate =
            std::find_if( candidates.begin(), candidates.end(),
                          []( const SNAP_CANDIDATE& aCandidate )
                          {
                              return aCandidate.relation == SNAP_RELATION::BBOX_ALIGNMENT && aCandidate.origin.x == 5.0;
                          } );
    BOOST_REQUIRE( xCandidate != candidates.end() );
    SNAP_RESOLVER resolver;
    resolver.AddCandidate( *xCandidate );
    BOOST_CHECK_EQUAL( resolver.Resolve( context ).position, VECTOR2I( 5, 45 ) );
}


BOOST_AUTO_TEST_CASE( ProjectedMovingBoundsDoNotOscillateAfterSnap )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "target", "bounds" ), BOX2I( { 0, 0 }, { 20, 20 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 45, 48 };
    context.movingReferencePoint = VECTOR2I( 45, 45 );
    context.movingBounds = BOX2I( { 5, 5 }, { 10, 10 } );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectAlignment( context, 50 );
    auto                        centerToCenter =
            std::find_if( candidates.begin(), candidates.end(),
                          []( const SNAP_CANDIDATE& aCandidate )
                          {
                              return aCandidate.id.kind == SNAP_ID_KIND::BOUNDS_Y && aCandidate.id.featureIndex == 4;
                          } );

    BOOST_REQUIRE( centerToCenter != candidates.end() );
    BOOST_CHECK_EQUAL( centerToCenter->origin.y, 45.0 );

    SNAP_RESOLVER resolver;
    resolver.SetStickyCandidates( { centerToCenter->id } );
    resolver.AddCandidate( *centerToCenter );

    BOOST_CHECK_EQUAL( resolver.Resolve( context ).position.y, 45 );
}


BOOST_AUTO_TEST_CASE( AlignmentGuideIdentifiesMatchedTargetFeature )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "target", "bounds" ), BOX2I( { 0, 0 }, { 20, 20 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 45, 48 };
    context.movingReferencePoint = VECTOR2I( 45, 45 );
    context.movingBounds = BOX2I( { 5, 5 }, { 10, 10 } );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectAlignment( context, 50 );
    auto                        centerToCenter =
            std::find_if( candidates.begin(), candidates.end(),
                          []( const SNAP_CANDIDATE& aCandidate )
                          {
                              return aCandidate.id.kind == SNAP_ID_KIND::BOUNDS_Y && aCandidate.id.featureIndex == 4;
                          } );

    BOOST_REQUIRE( centerToCenter != candidates.end() );
    BOOST_REQUIRE_EQUAL( centerToCenter->guides.size(), 1 );
    BOOST_CHECK_EQUAL( centerToCenter->guides.front().start, VECTOR2I( 0, 10 ) );
    BOOST_CHECK_EQUAL( centerToCenter->guides.front().end, VECTOR2I( 20, 10 ) );
}


BOOST_AUTO_TEST_CASE( BoundsReferencePrefersMatchingFeatures )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "target", "bounds" ), BOX2I( { 0, 0 }, { 20, 20 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 45, 45 };
    context.movingBounds = BOX2I( { 40, 40 }, { 10, 10 } );
    context.referencePreference = SNAP_REFERENCE_PREFERENCE{ SNAP_REFERENCE_KIND::BOUNDS_FEATURE, 1, 1 };

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectAlignment( context, 50 );
    auto                        centerToCenter =
            std::find_if( candidates.begin(), candidates.end(),
                          []( const SNAP_CANDIDATE& aCandidate )
                          {
                              return aCandidate.id.kind == SNAP_ID_KIND::BOUNDS_X && aCandidate.id.featureIndex == 4;
                          } );
    auto leftToLeft =
            std::find_if( candidates.begin(), candidates.end(),
                          []( const SNAP_CANDIDATE& aCandidate )
                          {
                              return aCandidate.id.kind == SNAP_ID_KIND::BOUNDS_X && aCandidate.id.featureIndex == 0;
                          } );

    BOOST_REQUIRE( centerToCenter != candidates.end() );
    BOOST_REQUIRE( leftToLeft != candidates.end() );
    BOOST_CHECK_EQUAL( centerToCenter->referenceAffinity, 0 );
    BOOST_CHECK_GT( leftToLeft->referenceAffinity, centerToCenter->referenceAffinity );
}


BOOST_AUTO_TEST_CASE( PadCenterReferencePrefersPadCenterTargets )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "target", "bounds" ), BOX2I( { 0, 0 }, { 20, 20 } ) } );
    provider.AddAlignmentPoint(
            { featureId( "target-pad", "center" ), { 0, 10 } } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 2, 40 };
    context.movingBounds = BOX2I( { 2, 35 }, { 10, 10 } );
    context.referencePreference = SNAP_REFERENCE_PREFERENCE{ SNAP_REFERENCE_KIND::ANCHOR_POINT, -1, -1 };

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectAlignment( context, 50 );
    auto                        padCenter = std::find_if( candidates.begin(), candidates.end(),
                                                          []( const SNAP_CANDIDATE& aCandidate )
                                                          {
                                       return aCandidate.id.kind == SNAP_ID_KIND::ANCHOR_POINT_X;
                                   } );
    auto                        boundsFallback = std::find_if( candidates.begin(), candidates.end(),
                                                               []( const SNAP_CANDIDATE& aCandidate )
                                                               {
                                            return aCandidate.id.kind == SNAP_ID_KIND::BOUNDS_X;
                                        } );

    BOOST_REQUIRE( padCenter != candidates.end() );
    BOOST_REQUIRE( boundsFallback != candidates.end() );
    BOOST_CHECK_EQUAL( padCenter->origin.x, 0.0 );
    BOOST_CHECK_EQUAL( padCenter->referenceAffinity, 0 );
    BOOST_CHECK_GT( boundsFallback->referenceAffinity, padCenter->referenceAffinity );
}


BOOST_AUTO_TEST_CASE( AlignmentAcceptsOneFeatureIdentityPerAxis )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "target", "bounds" ), BOX2I( { 0, 0 }, { 20, 20 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 45, 45 };
    context.movingReferencePoint = VECTOR2I( 45, 45 );
    context.movingBounds = BOX2I( { 5, 5 }, { 20, 20 } );

    SNAP_RESOLVER resolver;

    for( SNAP_CANDIDATE candidate : provider.CollectAlignment( context, 50 ) )
        resolver.AddCandidate( std::move( candidate ) );

    SNAP_RESULT result = resolver.Resolve( context );
    size_t      xCount = 0;
    size_t      yCount = 0;

    for( const SNAP_STABLE_ID& accepted : result.accepted )
    {
        xCount += accepted.kind == SNAP_ID_KIND::BOUNDS_X;
        yCount += accepted.kind == SNAP_ID_KIND::BOUNDS_Y;
    }

    BOOST_CHECK_EQUAL( xCount, 1 );
    BOOST_CHECK_EQUAL( yCount, 1 );
}


BOOST_AUTO_TEST_CASE( DenseAlignmentBoundsCandidateCountBeforeResolution )
{
    SNAP_INFERENCE_PROVIDER provider;

    for( int index = 0; index < 1000; ++index )
    {
        provider.AddBounds( { featureId( std::to_string( index ).c_str(), "bounds" ),
                              BOX2I( { index % 5, index % 7 }, { 20, 20 } ) } );
    }

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 10, 10 };
    context.movingBounds = BOX2I( { 0, 0 }, { 20, 20 } );

    BOOST_CHECK_EQUAL( provider.CollectAlignment( context, 50 ).size(), 128 );
}


BOOST_AUTO_TEST_CASE( LayoutInferenceIsInvariantUnderAxisTranspose )
{
    const std::array<SNAP_OBJECT_BOUNDS, 2> bounds = {
        SNAP_OBJECT_BOUNDS{ featureId( "first", "bounds" ), BOX2I( { 0, 0 }, { 10, 12 } ) },
        SNAP_OBJECT_BOUNDS{ featureId( "second", "bounds" ), BOX2I( { 30, 1 }, { 10, 9 } ) }
    };
    const SNAP_ALIGNMENT_POINT padCenter{ featureId( "pad", "center" ),
                                          { 33, 6 } };

    const auto makeProvider = [&]( bool aTranspose )
    {
        SNAP_INFERENCE_PROVIDER provider;

        for( SNAP_OBJECT_BOUNDS bound : bounds )
        {
            if( aTranspose )
                bound.bounds = transpose( bound.bounds );

            provider.AddBounds( std::move( bound ) );
        }

        SNAP_ALIGNMENT_POINT point = padCenter;

        if( aTranspose )
            point.position = transpose( point.position );

        provider.AddAlignmentPoint( std::move( point ) );
        return provider;
    };

    const auto makeContext = []( SNAP_REFERENCE_KIND aReferenceKind, bool aTranspose )
    {
        SNAP_SOURCE_CONTEXT context;
        context.sourcePoint = { 53, 7 };
        context.movingBounds = BOX2I( { 50, 2 }, { 10, 8 } );
        context.movingReferencePoint = VECTOR2I( 55, 6 );
        context.referencePreference = { aReferenceKind, 2, 0 };

        if( aTranspose )
        {
            context.sourcePoint = transpose( context.sourcePoint );
            context.movingBounds = transpose( *context.movingBounds );
            context.movingReferencePoint = transpose( *context.movingReferencePoint );
            std::swap( context.referencePreference.horizontalFeature, context.referencePreference.verticalFeature );
        }

        return context;
    };

    for( SNAP_REFERENCE_KIND reference : { SNAP_REFERENCE_KIND::BOUNDS_FEATURE, SNAP_REFERENCE_KIND::ANCHOR_POINT } )
    {
        SNAP_INFERENCE_PROVIDER original = makeProvider( false );
        SNAP_INFERENCE_PROVIDER transposed = makeProvider( true );
        checkTransposeEquivalent( original.CollectAlignment( makeContext( reference, false ), 100 ),
                                  transposed.CollectAlignment( makeContext( reference, true ), 100 ) );
    }

    SNAP_INFERENCE_PROVIDER     original = makeProvider( false );
    SNAP_INFERENCE_PROVIDER     transposed = makeProvider( true );
    std::vector<SNAP_CANDIDATE> originalSpacing =
            original.CollectEqualSpacing( makeContext( SNAP_REFERENCE_KIND::NONE, false ), 100 );
    std::vector<SNAP_CANDIDATE> transposedSpacing =
            transposed.CollectEqualSpacing( makeContext( SNAP_REFERENCE_KIND::NONE, true ), 100 );

    for( int branch : { -1, 0, 1 } )
    {
        BOOST_CHECK( std::any_of( originalSpacing.begin(), originalSpacing.end(),
                                  [&]( const SNAP_CANDIDATE& aCandidate )
                                  {
                                      return aCandidate.id.solutionBranch == branch
                                             && ( aCandidate.id.kind == SNAP_ID_KIND::EQUAL_GAP_X
                                                  || aCandidate.id.kind == SNAP_ID_KIND::COPY_GAP_X );
                                  } ) );
    }

    checkTransposeEquivalent( std::move( originalSpacing ), std::move( transposedSpacing ) );
}


BOOST_AUTO_TEST_CASE( EqualGapCentersBetweenImmediateNeighbors )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "left", "bounds" ), BOX2I( { 0, 0 }, { 10, 10 } ) } );
    provider.AddBounds( { featureId( "right", "bounds" ), BOX2I( { 30, 0 }, { 10, 10 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 55, 5 };
    context.movingBounds = BOX2I( { 50, 0 }, { 10, 10 } );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectEqualSpacing( context, 100 );

    BOOST_CHECK( std::any_of( candidates.begin(), candidates.end(),
                              []( const SNAP_CANDIDATE& aCandidate )
                              {
                                  return aCandidate.id.kind == SNAP_ID_KIND::EQUAL_GAP_X
                                         && aCandidate.relation == SNAP_RELATION::BBOX_EQUAL_GAP
                                         && aCandidate.origin.x == 20.0;
                              } ) );

    auto equalGap = std::find_if( candidates.begin(), candidates.end(),
                                  []( const SNAP_CANDIDATE& aCandidate )
                                  {
                                      return aCandidate.id.kind == SNAP_ID_KIND::EQUAL_GAP_X;
                                  } );
    BOOST_REQUIRE( equalGap != candidates.end() );
    SNAP_RESOLVER resolver;
    resolver.AddCandidate( *equalGap );
    BOOST_CHECK_EQUAL( resolver.Resolve( context ).position, VECTOR2I( 20, 5 ) );
}


BOOST_AUTO_TEST_CASE( EqualHorizontalGapCarriesBothBracketSpans )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "left", "bounds" ), BOX2I( { 0, 0 }, { 10, 10 } ) } );
    provider.AddBounds( { featureId( "right", "bounds" ), BOX2I( { 30, 0 }, { 10, 10 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 55, 5 };
    context.movingBounds = BOX2I( { 50, 0 }, { 10, 10 } );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectEqualSpacing( context, 100 );
    auto                        equalGap = std::find_if( candidates.begin(), candidates.end(),
                                                         []( const SNAP_CANDIDATE& aCandidate )
                                                         {
                                      return aCandidate.id.kind == SNAP_ID_KIND::EQUAL_GAP_X;
                                  } );

    BOOST_REQUIRE( equalGap != candidates.end() );
    BOOST_REQUIRE_EQUAL( equalGap->guides.size(), 2 );
    BOOST_CHECK( equalGap->guides[0].style == SNAP_GUIDE_STYLE::DIMENSION_BRACKET );
    BOOST_CHECK_EQUAL( equalGap->guides[0].start, VECTOR2I( 10, 10 ) );
    BOOST_CHECK_EQUAL( equalGap->guides[0].end, VECTOR2I( 15, 10 ) );
    BOOST_CHECK( equalGap->guides[1].style == SNAP_GUIDE_STYLE::DIMENSION_BRACKET );
    BOOST_CHECK_EQUAL( equalGap->guides[1].start, VECTOR2I( 25, 10 ) );
    BOOST_CHECK_EQUAL( equalGap->guides[1].end, VECTOR2I( 30, 10 ) );
}


BOOST_AUTO_TEST_CASE( EqualVerticalGapCarriesBothBracketSpans )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "top", "bounds" ), BOX2I( { 0, 0 }, { 10, 10 } ) } );
    provider.AddBounds( { featureId( "bottom", "bounds" ), BOX2I( { 0, 30 }, { 10, 10 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 5, 55 };
    context.movingBounds = BOX2I( { 0, 50 }, { 10, 10 } );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectEqualSpacing( context, 100 );
    auto                        equalGap = std::find_if( candidates.begin(), candidates.end(),
                                                         []( const SNAP_CANDIDATE& aCandidate )
                                                         {
                                      return aCandidate.id.kind == SNAP_ID_KIND::EQUAL_GAP_Y;
                                  } );

    BOOST_REQUIRE( equalGap != candidates.end() );
    BOOST_REQUIRE_EQUAL( equalGap->guides.size(), 2 );
    BOOST_CHECK( equalGap->guides[0].style == SNAP_GUIDE_STYLE::DIMENSION_BRACKET );
    BOOST_CHECK_EQUAL( equalGap->guides[0].start, VECTOR2I( 10, 10 ) );
    BOOST_CHECK_EQUAL( equalGap->guides[0].end, VECTOR2I( 10, 15 ) );
    BOOST_CHECK( equalGap->guides[1].style == SNAP_GUIDE_STYLE::DIMENSION_BRACKET );
    BOOST_CHECK_EQUAL( equalGap->guides[1].start, VECTOR2I( 10, 25 ) );
    BOOST_CHECK_EQUAL( equalGap->guides[1].end, VECTOR2I( 10, 30 ) );
}


BOOST_AUTO_TEST_CASE( EqualGapPreservesSourceOffsetFromMovingBounds )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "left", "bounds" ), BOX2I( { 0, 0 }, { 10, 10 } ) } );
    provider.AddBounds( { featureId( "right", "bounds" ), BOX2I( { 30, 0 }, { 10, 10 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 52, 2 };
    context.movingBounds = BOX2I( { 50, 0 }, { 10, 10 } );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectEqualSpacing( context, 100 );

    BOOST_CHECK( std::any_of( candidates.begin(), candidates.end(),
                              []( const SNAP_CANDIDATE& aCandidate )
                              {
                                  return aCandidate.id.kind == SNAP_ID_KIND::EQUAL_GAP_X && aCandidate.origin.x == 17.0;
                              } ) );
}


BOOST_AUTO_TEST_CASE( EqualGapIgnoresReferencePreference )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "left", "bounds" ), BOX2I( { 0, 0 }, { 10, 10 } ) } );
    provider.AddBounds( { featureId( "right", "bounds" ), BOX2I( { 30, 0 }, { 10, 10 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 52, 2 };
    context.movingBounds = BOX2I( { 50, 0 }, { 10, 10 } );
    context.referencePreference = SNAP_REFERENCE_PREFERENCE{ SNAP_REFERENCE_KIND::ANCHOR_POINT, -1, -1 };

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectEqualSpacing( context, 100 );
    auto                        equalGap = std::find_if( candidates.begin(), candidates.end(),
                                                         []( const SNAP_CANDIDATE& aCandidate )
                                                         {
                                      return aCandidate.id.kind == SNAP_ID_KIND::EQUAL_GAP_X;
                                  } );

    BOOST_REQUIRE( equalGap != candidates.end() );
    BOOST_CHECK_EQUAL( equalGap->origin.x, 17.0 );
    BOOST_CHECK_EQUAL( equalGap->referenceAffinity, 0 );
}


BOOST_AUTO_TEST_CASE( EqualGapCopiesAdjacentStationaryGap )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "first", "bounds" ), BOX2I( { 0, 0 }, { 10, 10 } ) } );
    provider.AddBounds( { featureId( "second", "bounds" ), BOX2I( { 20, 0 }, { 10, 10 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 44, 5 };
    context.movingBounds = BOX2I( { 39, 0 }, { 10, 10 } );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectEqualSpacing( context, 20 );

    auto copiedGap = std::find_if( candidates.begin(), candidates.end(),
                                   []( const SNAP_CANDIDATE& aCandidate )
                                   {
                                       return aCandidate.id.kind == SNAP_ID_KIND::COPY_GAP_X
                                              && aCandidate.id.solutionBranch > 0 && aCandidate.origin.x == 45.0;
                                   } );

    BOOST_REQUIRE( copiedGap != candidates.end() );
    BOOST_REQUIRE_EQUAL( copiedGap->guides.size(), 2 );
    BOOST_CHECK_EQUAL( copiedGap->guides[0].start, VECTOR2I( 10, 10 ) );
    BOOST_CHECK_EQUAL( copiedGap->guides[0].end, VECTOR2I( 20, 10 ) );
    BOOST_CHECK_EQUAL( copiedGap->guides[1].start, VECTOR2I( 30, 10 ) );
    BOOST_CHECK_EQUAL( copiedGap->guides[1].end, VECTOR2I( 40, 10 ) );
}


BOOST_AUTO_TEST_CASE( EqualGapCopiesAdjacentStationaryGapBeforePair )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "first", "bounds" ), BOX2I( { 0, 0 }, { 10, 10 } ) } );
    provider.AddBounds( { featureId( "second", "bounds" ), BOX2I( { 20, 0 }, { 10, 10 } ) } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { -14, 5 };
    context.movingBounds = BOX2I( { -19, 0 }, { 10, 10 } );

    std::vector<SNAP_CANDIDATE> candidates = provider.CollectEqualSpacing( context, 20 );
    auto                        copiedGap = std::find_if( candidates.begin(), candidates.end(),
                                                          []( const SNAP_CANDIDATE& aCandidate )
                                                          {
                                       return aCandidate.id.kind == SNAP_ID_KIND::COPY_GAP_X
                                              && aCandidate.id.solutionBranch < 0 && aCandidate.origin.x == -15.0;
                                   } );

    BOOST_REQUIRE( copiedGap != candidates.end() );
    BOOST_REQUIRE_EQUAL( copiedGap->guides.size(), 2 );
    BOOST_CHECK_EQUAL( copiedGap->guides[0].start, VECTOR2I( -10, 10 ) );
    BOOST_CHECK_EQUAL( copiedGap->guides[0].end, VECTOR2I( 0, 10 ) );
    BOOST_CHECK_EQUAL( copiedGap->guides[1].start, VECTOR2I( 10, 10 ) );
    BOOST_CHECK_EQUAL( copiedGap->guides[1].end, VECTOR2I( 20, 10 ) );
}


BOOST_AUTO_TEST_CASE( DenseEqualSpacingBoundsCandidateCountBeforeResolution )
{
    SNAP_INFERENCE_PROVIDER provider;

    for( int index = 0; index < 1000; ++index )
    {
        provider.AddBounds(
                { featureId( std::to_string( index ).c_str(), "bounds" ), BOX2I( { index * 30, 0 }, { 10, 10 } ) } );
    }

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 0, 5 };
    context.movingBounds = BOX2I( { -10, 0 }, { 10, 10 } );

    BOOST_CHECK_EQUAL( provider.CollectEqualSpacing( context, 100000 ).size(), 128 );
}


BOOST_AUTO_TEST_CASE( LayoutRejectsMovingSelectionDescendants )
{
    SNAP_INFERENCE_PROVIDER provider;
    provider.AddBounds( { featureId( "child", "bounds" ), BOX2I( { 0, 0 }, { 10, 10 } ),
                          featureId( "selection", "bounds" ).target } );

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 12, 5 };
    context.movingBounds = BOX2I( { 10, 0 }, { 10, 10 } );
    context.movingItem = featureId( "selection", "bounds" );

    BOOST_CHECK( provider.CollectAlignment( context, 50 ).empty() );
    BOOST_CHECK( provider.CollectEqualSpacing( context, 50 ).empty() );
}


BOOST_AUTO_TEST_CASE( CandidateDensityP95StaysWithinFourMilliseconds )
{
    SNAP_INFERENCE_PROVIDER provider;

    for( int i = 0; i < 10000; ++i )
    {
        provider.AddPath( { featureId( std::to_string( i ).c_str(), "edge" ),
                            SEG( VECTOR2I( 0, i % 20 ), VECTOR2I( 50, i % 20 ) ) } );
    }

    SNAP_SOURCE_CONTEXT context;
    context.sourcePoint = { 10, 0 };
    std::vector<std::chrono::microseconds> samples;

    for( int run = 0; run < 20; ++run )
    {
        std::clock_t                start = std::clock();
        std::vector<SNAP_CANDIDATE> candidates = provider.CollectObjectGeometry( context, 25 );
        samples.emplace_back( ( std::clock() - start ) * 1000000 / CLOCKS_PER_SEC );
        BOOST_CHECK_EQUAL( candidates.size(), 64 );
    }

    std::sort( samples.begin(), samples.end() );
    std::chrono::microseconds p95 = samples[18];
    BOOST_TEST_MESSAGE( "10k-object candidate generation p95: " << p95.count() << " us" );
    BOOST_CHECK_LE( p95.count(), 4000 );
}


BOOST_AUTO_TEST_SUITE_END()
