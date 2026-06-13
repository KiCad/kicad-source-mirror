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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <boost/test/unit_test.hpp>

#include <diff_merge/identity_reconciler.h>

#include <chrono>


using namespace KICAD_DIFF;


namespace
{

ITEM_DESCRIPTOR make( const wxString& aType, const KIID& aUuid,
                     int aX = 0, int aY = 0,
                     std::vector<std::pair<wxString, std::string>> aProps = {} )
{
    ITEM_DESCRIPTOR d;
    d.id.push_back( aUuid );
    d.type     = aType;
    d.position = VECTOR2I( aX, aY );
    d.bbox     = BOX2I( VECTOR2I( aX, aY ), VECTOR2I( 10, 10 ) );
    d.keyProps = std::move( aProps );
    return d;
}

} // namespace


BOOST_AUTO_TEST_SUITE( IdentityReconciler )


BOOST_AUTO_TEST_CASE( DirectUuidMatch )
{
    KIID::SeedGenerator( 1 );
    KIID id1, id2;

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id1 ),
                                       make( wxS( "PCB_VIA" ),   id2 ) };

    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "PCB_TRACK" ), id1 ),
                                       make( wxS( "PCB_VIA" ),   id2 ) };

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, b );

    BOOST_CHECK_EQUAL( result.aToB.size(), 2u );
    BOOST_CHECK_EQUAL( result.aOnly.size(), 0u );
    BOOST_CHECK_EQUAL( result.bOnly.size(), 0u );
    BOOST_CHECK_EQUAL( result.similarityMatches, 0u );
}


BOOST_AUTO_TEST_CASE( UnmatchedItems )
{
    KIID::SeedGenerator( 2 );
    KIID id1, id2, id3;

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id1 ),
                                       make( wxS( "PCB_VIA" ),   id2 ) };

    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "PCB_TRACK" ), id1 ),
                                       make( wxS( "ZONE" ),      id3 ) };

    IDENTITY_RECONCILER r;
    IDENTITY_RECONCILER::CONFIG cfg;
    cfg.enableSimilarity = false;   // pure UUID match for this test
    r.SetConfig( cfg );

    RECONCILIATION result = r.Reconcile( a, b );

    BOOST_CHECK_EQUAL( result.aToB.size(), 1u );
    BOOST_CHECK_EQUAL( result.aOnly.size(), 1u );
    BOOST_CHECK_EQUAL( result.bOnly.size(), 1u );
}


BOOST_AUTO_TEST_CASE( DuplicateUuidDetection )
{
    KIID::SeedGenerator( 3 );
    KIID id;

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id ),
                                       make( wxS( "PCB_TRACK" ), id ) };
    std::vector<ITEM_DESCRIPTOR> b;

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, b );

    BOOST_CHECK_EQUAL( result.duplicatesA.size(), 1u );
}


BOOST_AUTO_TEST_CASE( SimilarityFallback )
{
    KIID::SeedGenerator( 4 );
    KIID idOld, idNew;

    // Same item, but UUID has churned (e.g., from import). Both have identical
    // position, bbox, and a matching key property.
    std::vector<std::pair<wxString, std::string>> props =
        { { wxS( "lib_id" ), "Device:R" }, { wxS( "reference" ), "R1" } };

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "FOOTPRINT" ), idOld, 100, 200, props ) };
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "FOOTPRINT" ), idNew, 100, 200, props ) };

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, b );

    BOOST_CHECK_EQUAL( result.aToB.size(), 1u );
    BOOST_CHECK_EQUAL( result.similarityMatches, 1u );
    BOOST_CHECK_EQUAL( result.aOnly.size(), 0u );
    BOOST_CHECK_EQUAL( result.bOnly.size(), 0u );
}


BOOST_AUTO_TEST_CASE( SimilarityRespectsTypeMismatch )
{
    KIID::SeedGenerator( 5 );
    KIID id1, id2;

    std::vector<std::pair<wxString, std::string>> p1 = { { wxS( "k" ), "v" } };
    std::vector<std::pair<wxString, std::string>> p2 = p1;

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id1, 0, 0, p1 ) };
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "PCB_VIA" ),   id2, 0, 0, p2 ) };

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, b );

    // Different type -> never match
    BOOST_CHECK_EQUAL( result.aToB.size(), 0u );
    BOOST_CHECK_EQUAL( result.aOnly.size(), 1u );
    BOOST_CHECK_EQUAL( result.bOnly.size(), 1u );
}


BOOST_AUTO_TEST_CASE( ScoreSimilarityBreakdown )
{
    KIID::SeedGenerator( 6 );
    KIID id1, id2;

    auto p = std::vector<std::pair<wxString, std::string>>{ { wxS( "k" ), "v" } };
    ITEM_DESCRIPTOR a = make( wxS( "PCB_TRACK" ), id1, 100, 200, p );
    ITEM_DESCRIPTOR b = make( wxS( "PCB_TRACK" ), id2, 100, 200, p );

    IDENTITY_RECONCILER r;
    double score = r.ScoreSimilarity( a, b );
    // Position (0.4) + bbox (0.2) + full key prop overlap (0.4) = 1.0
    BOOST_CHECK_CLOSE( score, 1.0, 0.001 );

    // Move position; bbox originates from position above so this perturbs both.
    b.position = VECTOR2I( 999, 999 );
    b.bbox     = BOX2I( VECTOR2I( 999, 999 ), VECTOR2I( 10, 10 ) );
    score      = r.ScoreSimilarity( a, b );
    // Only keyProps match now: 0.4
    BOOST_CHECK_CLOSE( score, 0.4, 0.001 );
}


BOOST_AUTO_TEST_CASE( GreedyAssignment )
{
    KIID::SeedGenerator( 7 );
    KIID idA1, idA2, idB1, idB2;

    auto p1 = std::vector<std::pair<wxString, std::string>>{ { wxS( "k" ), "X" } };
    auto p2 = std::vector<std::pair<wxString, std::string>>{ { wxS( "k" ), "Y" } };

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "FOOTPRINT" ), idA1, 0, 0, p1 ),
                                       make( wxS( "FOOTPRINT" ), idA2, 100, 100, p2 ) };

    // B has them in different order with new UUIDs but matching props.
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "FOOTPRINT" ), idB1, 100, 100, p2 ),
                                       make( wxS( "FOOTPRINT" ), idB2, 0, 0, p1 ) };

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, b );

    BOOST_CHECK_EQUAL( result.aToB.size(), 2u );
    BOOST_CHECK_EQUAL( result.aOnly.size(), 0u );
    BOOST_CHECK_EQUAL( result.bOnly.size(), 0u );
    BOOST_CHECK_EQUAL( result.similarityMatches, 2u );
}


BOOST_AUTO_TEST_CASE( DeterministicOutput )
{
    KIID::SeedGenerator( 8 );
    KIID id1, id2, id3, id4;

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id1 ),
                                       make( wxS( "PCB_TRACK" ), id2 ),
                                       make( wxS( "PCB_TRACK" ), id3 ) };
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "PCB_TRACK" ), id1 ),
                                       make( wxS( "PCB_TRACK" ), id4 ) };

    IDENTITY_RECONCILER r;
    RECONCILIATION      one = r.Reconcile( a, b );
    RECONCILIATION      two = r.Reconcile( a, b );

    BOOST_CHECK( one.aToB == two.aToB );
    BOOST_CHECK( one.aOnly == two.aOnly );
    BOOST_CHECK( one.bOnly == two.bOnly );
}


BOOST_AUTO_TEST_CASE( EmptyKeyPropsScoreZero )
{
    KIID::SeedGenerator( 100 );
    KIID id1, id2;

    // Same position, same bbox, no keyProps — should NOT score 1.0.
    // Otherwise generic items at the same coordinates false-match perfectly.
    ITEM_DESCRIPTOR a = make( wxS( "PCB_TRACK" ), id1, 100, 200 );
    ITEM_DESCRIPTOR b = make( wxS( "PCB_TRACK" ), id2, 100, 200 );
    a.keyProps.clear();
    b.keyProps.clear();

    IDENTITY_RECONCILER r;
    double score = r.ScoreSimilarity( a, b );

    // Position (0.4) + bbox (0.2) = 0.6; keyProps contribute 0.
    BOOST_CHECK_CLOSE( score, 0.6, 0.001 );
    BOOST_CHECK_LT( score, r.GetConfig().similarityThreshold );
}


BOOST_AUTO_TEST_CASE( DuplicatesExcludedFromMatching )
{
    KIID::SeedGenerator( 101 );
    KIID id;

    // Two items in A with the same UUID, one item in B with the same UUID.
    // Without the duplicate-exclude logic, the first-seen A entry would match
    // B normally — masking the duplicate from the report.
    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id ),
                                       make( wxS( "PCB_TRACK" ), id ) };
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "PCB_TRACK" ), id ) };

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, b );

    BOOST_CHECK_EQUAL( result.duplicatesA.size(), 1u );
    BOOST_CHECK_EQUAL( result.aToB.size(), 0u );   // duplicates not matched
    BOOST_CHECK_EQUAL( result.bOnly.size(), 1u );
}


BOOST_AUTO_TEST_CASE( DuplicatesOnBSide )
{
    // Mirror of DuplicatesExcludedFromMatching — duplicate in B, not A. The
    // duplicatesB vector must populate symmetrically.
    KIID::SeedGenerator( 102 );
    KIID id;

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id ) };
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "PCB_TRACK" ), id ),
                                       make( wxS( "PCB_TRACK" ), id ) };

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, b );

    BOOST_CHECK_EQUAL( result.duplicatesB.size(), 1u );
    BOOST_CHECK_EQUAL( result.aToB.size(), 0u );
    BOOST_CHECK_EQUAL( result.aOnly.size(), 1u );
}


BOOST_AUTO_TEST_CASE( DetectDuplicatesDisabledSkipsReporting )
{
    // detectDuplicates=false means colliding KIIDs aren't surfaced; the engine
    // still has to choose some mapping but the user opted out of the warning.
    // B has one matching item so the test also exercises the "still choose a
    // mapping" path rather than just "duplicate vector is empty".
    KIID::SeedGenerator( 103 );
    KIID id;

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id ),
                                       make( wxS( "PCB_TRACK" ), id ) };
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "PCB_TRACK" ), id ) };

    IDENTITY_RECONCILER::CONFIG cfg;
    cfg.detectDuplicates = false;
    IDENTITY_RECONCILER r( cfg );

    RECONCILIATION result = r.Reconcile( a, b );
    BOOST_CHECK( result.duplicatesA.empty() );
    // With the toggle off, the first-seen A entry maps to B.
    BOOST_CHECK_EQUAL( result.aToB.size(), 1u );
    BOOST_CHECK( result.bOnly.empty() );
}


BOOST_AUTO_TEST_CASE( EnableSimilarityFalseSkipsFallback )
{
    // With UUIDs churned and similarity off, items should land in aOnly/bOnly
    // even when they're an obvious positional + key-prop match.
    KIID::SeedGenerator( 104 );
    KIID idA, idB;

    auto props = std::vector<std::pair<wxString, std::string>>{ { wxS( "k" ), "v" } };
    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "FOOTPRINT" ), idA, 0, 0, props ) };
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "FOOTPRINT" ), idB, 0, 0, props ) };

    IDENTITY_RECONCILER::CONFIG cfg;
    cfg.enableSimilarity = false;
    IDENTITY_RECONCILER r( cfg );

    RECONCILIATION result = r.Reconcile( a, b );
    BOOST_CHECK_EQUAL( result.similarityMatches, 0u );
    BOOST_CHECK_EQUAL( result.aOnly.size(), 1u );
    BOOST_CHECK_EQUAL( result.bOnly.size(), 1u );
}


BOOST_AUTO_TEST_CASE( SimilarityThresholdGatesMatch )
{
    // Score 0.6 (position + bbox, no keyProps) sits below the default 0.85
    // threshold so it's NOT matched. Raise the threshold higher → still no
    // match. Lower below 0.6 → match. `make()` defaults keyProps to empty,
    // so no clear() needed.
    KIID::SeedGenerator( 105 );
    KIID idA, idB;

    std::vector<ITEM_DESCRIPTOR> aVec = { make( wxS( "FOOTPRINT" ), idA, 100, 200 ) };
    std::vector<ITEM_DESCRIPTOR> bVec = { make( wxS( "FOOTPRINT" ), idB, 100, 200 ) };

    {
        IDENTITY_RECONCILER r;   // default threshold 0.85
        BOOST_CHECK_EQUAL( r.Reconcile( aVec, bVec ).similarityMatches, 0u );
    }

    {
        IDENTITY_RECONCILER::CONFIG cfg;
        cfg.similarityThreshold = 0.5;
        IDENTITY_RECONCILER r( cfg );
        BOOST_CHECK_EQUAL( r.Reconcile( aVec, bVec ).similarityMatches, 1u );
    }
}


BOOST_AUTO_TEST_CASE( EmptyInputsProduceEmptyReconciliation )
{
    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( {}, {} );

    BOOST_CHECK( result.aToB.empty() );
    BOOST_CHECK( result.bToA.empty() );
    BOOST_CHECK( result.aOnly.empty() );
    BOOST_CHECK( result.bOnly.empty() );
    BOOST_CHECK( result.duplicatesA.empty() );
    BOOST_CHECK( result.duplicatesB.empty() );
    BOOST_CHECK_EQUAL( result.similarityMatches, 0u );
}


BOOST_AUTO_TEST_CASE( OneSideEmptyOtherFullyUnmatched )
{
    KIID::SeedGenerator( 106 );
    KIID id1, id2;

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id1 ),
                                       make( wxS( "PCB_TRACK" ), id2 ) };

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, {} );

    BOOST_CHECK_EQUAL( result.aOnly.size(), 2u );
    BOOST_CHECK( result.bOnly.empty() );
    BOOST_CHECK_EQUAL( result.aToB.size(), 0u );
}


BOOST_AUTO_TEST_CASE( ReverseLookupBToAMirrorsAToB_DirectMatch )
{
    // Pass-1 (direct UUID match) symmetry: aToB[x] == y implies bToA[y] == x.
    KIID::SeedGenerator( 107 );
    KIID id1, id2;

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "PCB_TRACK" ), id1 ),
                                       make( wxS( "PCB_VIA" ),   id2 ) };
    std::vector<ITEM_DESCRIPTOR> b = a;

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, b );

    BOOST_REQUIRE_EQUAL( result.aToB.size(), 2u );
    for( const auto& [aId, bId] : result.aToB )
    {
        BOOST_REQUIRE( result.bToA.count( bId ) == 1 );
        BOOST_CHECK( result.bToA.at( bId ) == aId );
    }
}


BOOST_AUTO_TEST_CASE( ReverseLookupBToAMirrorsAToB_SimilarityPath )
{
    // Mirror coverage for the Pass-2 similarity path. When `b = a` was used
    // above, every match came from Pass-1 (identical UUIDs) so the bToA
    // assignment inside the similarity loop was never exercised. Force the
    // similarity path by churning B's UUIDs while keeping position + props
    // identical, then assert the same a<->b symmetry.
    KIID::SeedGenerator( 108 );
    KIID idA1, idA2, idB1, idB2;

    auto props = std::vector<std::pair<wxString, std::string>>{
        { wxS( "lib_id" ), "Device:R" },
        { wxS( "reference" ), "R1" } };

    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "FOOTPRINT" ), idA1, 0,   0,   props ),
                                       make( wxS( "FOOTPRINT" ), idA2, 100, 100, props ) };
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "FOOTPRINT" ), idB1, 0,   0,   props ),
                                       make( wxS( "FOOTPRINT" ), idB2, 100, 100, props ) };

    IDENTITY_RECONCILER r;
    RECONCILIATION      result = r.Reconcile( a, b );

    BOOST_REQUIRE_EQUAL( result.aToB.size(),              2u );
    BOOST_REQUIRE_EQUAL( result.similarityMatches,        2u );
    for( const auto& [aId, bId] : result.aToB )
    {
        BOOST_REQUIRE( result.bToA.count( bId ) == 1 );
        BOOST_CHECK( result.bToA.at( bId ) == aId );
    }
}


// 100 equal-scoring candidates: greedy-best with deterministic tie-break must
// produce the same assignment every run.  Pin determinism by running the
// reconciler twice on the same inputs and asserting identical aToB.
BOOST_AUTO_TEST_CASE( TieBreakDeterminismWithManyEqualScoreCandidates )
{
    constexpr int N = 100;

    KIID::SeedGenerator( 200 );
    std::vector<KIID> aIds( N );
    std::vector<KIID> bIds( N );

    auto props = std::vector<std::pair<wxString, std::string>>{
            { wxS( "lib_id" ), "Device:R" },
            { wxS( "reference" ), "R1" } };

    std::vector<ITEM_DESCRIPTOR> a;
    std::vector<ITEM_DESCRIPTOR> b;
    a.reserve( N );
    b.reserve( N );

    // All items share the same position and props so similarity scores
    // collide.  Different UUIDs on both sides force the similarity fallback
    // path.
    for( int i = 0; i < N; ++i )
    {
        a.push_back( make( wxS( "FOOTPRINT" ), aIds[i], 0, 0, props ) );
        b.push_back( make( wxS( "FOOTPRINT" ), bIds[i], 0, 0, props ) );
    }

    IDENTITY_RECONCILER r;
    RECONCILIATION      first  = r.Reconcile( a, b );
    RECONCILIATION      second = r.Reconcile( a, b );

    BOOST_REQUIRE_EQUAL( first.aToB.size(), second.aToB.size() );

    for( const auto& [aId, bId] : first.aToB )
    {
        BOOST_REQUIRE( second.aToB.count( aId ) );
        BOOST_CHECK( second.aToB.at( aId ) == bId );
    }
}


// 1000-item input: the reconciler must complete in a reasonable time budget.
// This is a smoke test for asymptotic explosion -- pin a generous 5-second
// upper bound (typical run is under 1s on modern hardware) so a future
// O(n^3) regression in similarity matching shows up as a CI timeout.
BOOST_AUTO_TEST_CASE( LargeInputPerformanceSmoke )
{
    constexpr int N = 1000;

    KIID::SeedGenerator( 300 );
    std::vector<ITEM_DESCRIPTOR> a;
    std::vector<ITEM_DESCRIPTOR> b;
    a.reserve( N );
    b.reserve( N );

    for( int i = 0; i < N; ++i )
    {
        KIID idA, idB;
        // Mix of direct-UUID matches and similarity-only matches so both
        // paths exercise.  Even i: same UUID on both sides; odd i: only
        // similarity path.
        if( ( i % 2 ) == 0 )
            idB = idA;

        auto props = std::vector<std::pair<wxString, std::string>>{
                { wxS( "lib_id" ), wxString::Format( "Device:R%d", i ).ToStdString() },
                { wxS( "reference" ), wxString::Format( "R%d", i ).ToStdString() } };

        a.push_back( make( wxS( "FOOTPRINT" ), idA, i, i, props ) );
        b.push_back( make( wxS( "FOOTPRINT" ), idB, i, i, props ) );
    }

    IDENTITY_RECONCILER r;
    const auto start = std::chrono::steady_clock::now();
    RECONCILIATION result = r.Reconcile( a, b );
    const auto elapsed = std::chrono::steady_clock::now() - start;

    const double seconds = std::chrono::duration<double>( elapsed ).count();

    BOOST_TEST_MESSAGE( "Reconcile(1k items) elapsed: " << seconds << "s" );
    BOOST_CHECK( seconds < 5.0 );

    // Every A must have found a match.
    BOOST_CHECK_EQUAL( result.aToB.size(), static_cast<size_t>( N ) );
}


// Non-zero positionTolerance: items at slightly different positions should
// score as identical-position when within tolerance.  Use a position delta
// that would fail a strict-equality (tolerance=0) check but pass at
// tolerance=1000nm.
BOOST_AUTO_TEST_CASE( PositionToleranceAllowsNearMatches )
{
    KIID::SeedGenerator( 400 );
    KIID idA1, idB1;

    auto props = std::vector<std::pair<wxString, std::string>>{
            { wxS( "lib_id" ), "Device:R" },
            { wxS( "reference" ), "R1" } };

    // A at (0,0), B at (500, 500) -- 500nm apart, within 1000nm tolerance.
    std::vector<ITEM_DESCRIPTOR> a = { make( wxS( "FOOTPRINT" ), idA1, 0,   0,   props ) };
    std::vector<ITEM_DESCRIPTOR> b = { make( wxS( "FOOTPRINT" ), idB1, 500, 500, props ) };

    IDENTITY_RECONCILER::CONFIG config;
    config.positionTolerance = 1000;
    IDENTITY_RECONCILER r( config );
    RECONCILIATION      result = r.Reconcile( a, b );

    BOOST_CHECK_EQUAL( result.aToB.size(), 1u );
    BOOST_CHECK_EQUAL( result.similarityMatches, 1u );
}


BOOST_AUTO_TEST_SUITE_END()
