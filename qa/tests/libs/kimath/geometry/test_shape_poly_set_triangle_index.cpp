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

/**
 * Tests for the spatial index over SHAPE_POLY_SET's cached triangulation.
 *
 * Every probe below is a fixed literal and every assertion compares against an independent full
 * scan written in this file, or against a literal.  There is no randomisation and no threading,
 * so a regression fails immediately rather than hanging CI.
 */

#include <boost/test/unit_test.hpp>

#include <limits>
#include <vector>

#include <geometry/shape_poly_set.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_rect.h>

namespace
{

/// A regular polygon; callers choose the vertex count to sit above or below the index threshold.
SHAPE_LINE_CHAIN MakeNgon( const VECTOR2I& aCentre, int aRadius, int aVertices )
{
    SHAPE_LINE_CHAIN chain;

    for( int ii = 0; ii < aVertices; ++ii )
    {
        double angle = 2.0 * M_PI * ii / aVertices;
        chain.Append( aCentre.x + KiROUND( aRadius * cos( angle ) ), aCentre.y + KiROUND( aRadius * sin( angle ) ) );
    }

    chain.SetClosed( true );
    return chain;
}


/// The blob every case queries: one big outline, two holes, comfortably over 64 triangles.
SHAPE_POLY_SET MakeBlob()
{
    SHAPE_POLY_SET poly;
    poly.AddOutline( MakeNgon( VECTOR2I( 0, 0 ), 1000000, 180 ) );
    poly.AddHole( MakeNgon( VECTOR2I( -400000, 0 ), 150000, 60 ) );
    poly.AddHole( MakeNgon( VECTOR2I( 400000, 250000 ), 120000, 60 ) );
    poly.CacheTriangulation();
    return poly;
}


/**
 * The collision flag and distance are the contract; the reported location deliberately is not.
 * On an exact-distance tie the indexed walk may reach a different equidistant triangle than the
 * full scan, and that is accepted.
 */
struct PROBE_RESULT
{
    bool collides;
    int  actual;

    bool operator==( const PROBE_RESULT& aOther ) const
    {
        return collides == aOther.collides && actual == aOther.actual;
    }
};


std::ostream& operator<<( std::ostream& aStream, const PROBE_RESULT& aResult )
{
    return aStream << "{ collides=" << aResult.collides << " actual=" << aResult.actual << " }";
}


PROBE_RESULT Probe( const SHAPE_POLY_SET& aPoly, const SHAPE& aShape, int aClearance )
{
    PROBE_RESULT r;
    r.actual = -1;
    r.collides = aPoly.Collide( &aShape, aClearance, &r.actual, nullptr );
    return r;
}


/**
 * The oracle: an independent full scan over every cached triangle, written here rather than
 * reached through the class under test.
 */
PROBE_RESULT ReferenceScan( const SHAPE_POLY_SET& aPoly, const SHAPE& aShape, int aClearance )
{
    PROBE_RESULT r;
    r.collides = false;
    r.actual = -1;

    int best = std::numeric_limits<int>::max();

    for( unsigned ii = 0; ii < aPoly.TriangulatedPolyCount(); ++ii )
    {
        const SHAPE_POLY_SET::TRIANGULATED_POLYGON* tpoly = aPoly.TriangulatedPolygon( ii );

        for( const SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI& tri : tpoly->Triangles() )
        {
            int triActual;

            if( aShape.Collide( &tri, aClearance, &triActual, nullptr ) )
                best = std::min( best, triActual );
        }
    }

    if( best < std::numeric_limits<int>::max() )
    {
        r.collides = true;
        r.actual = std::max( 0, best );
    }

    return r;
}


/**
 * Boolean-mode probe: Collide() with no out-parameters.
 *
 * This is a different branch of Collide() from Probe() above, and the one DRC takes most often.
 * It stops at the first hit rather than seeking a minimum, so it needs its own coverage.
 */
bool ProbeBool( const SHAPE_POLY_SET& aPoly, const SHAPE& aShape, int aClearance )
{
    return aPoly.Collide( &aShape, aClearance );
}


/// Boolean oracle: an independent full scan, first hit wins.
bool ReferenceScanBool( const SHAPE_POLY_SET& aPoly, const SHAPE& aShape, int aClearance )
{
    for( unsigned ii = 0; ii < aPoly.TriangulatedPolyCount(); ++ii )
    {
        const SHAPE_POLY_SET::TRIANGULATED_POLYGON* tpoly = aPoly.TriangulatedPolygon( ii );

        for( const SHAPE_POLY_SET::TRIANGULATED_POLYGON::TRI& tri : tpoly->Triangles() )
        {
            if( aShape.Collide( &tri, aClearance ) )
                return true;
        }
    }

    return false;
}


/**
 * Build the index by running one query, and assert it exists afterwards.
 *
 * Without that assertion every comparison in this file degrades to fallback-vs-oracle, which
 * agree by construction -- deleting the index from Collide() entirely would leave the suite
 * green.  This is what makes the rest of the cases falsifiers rather than tautologies.
 */
void WarmAndRequireIndexed( SHAPE_POLY_SET& aPoly )
{
    SHAPE_RECT warmup( VECTOR2I( 0, 0 ), 1000, 1000 );
    aPoly.Collide( &warmup, 0 );

    BOOST_REQUIRE( aPoly.TriangulatedPolyCount() > 0 );
    BOOST_REQUIRE( aPoly.TriangulationIndex() != nullptr );
}


/// Fixed probe positions, chosen to straddle the outline, both holes and empty space.
const std::vector<VECTOR2I> g_probes = {
    VECTOR2I( 0, 0 ),            // deep inside
    VECTOR2I( 999000, 0 ),       // just inside the outline
    VECTOR2I( 1001000, 0 ),      // just outside the outline
    VECTOR2I( -400000, 0 ),      // centre of hole 1
    VECTOR2I( -250000, 0 ),      // just outside hole 1
    VECTOR2I( 400000, 250000 ),  // centre of hole 2
    VECTOR2I( 3000000, 3000000 ) // far away
};

} // namespace


BOOST_AUTO_TEST_SUITE( SPSTriangleIndex )


/**
 * A cold Collide() must come back indexed.  MakeBlob() pre-triangulates, so every other case
 * warms the already-valid early-out of cacheTriangulation(); this one starts with no cached
 * triangulation at all, so the lazy build has to run on the fresh-triangulation exit too.
 */
BOOST_AUTO_TEST_CASE( FirstColdCollideBuildsTheIndex )
{
    SHAPE_POLY_SET poly;
    poly.AddOutline( MakeNgon( VECTOR2I( 0, 0 ), 1000000, 180 ) );
    poly.AddHole( MakeNgon( VECTOR2I( -400000, 0 ), 150000, 60 ) );
    poly.AddHole( MakeNgon( VECTOR2I( 400000, 250000 ), 120000, 60 ) );

    SHAPE_RECT probe( VECTOR2I( 0, 0 ), 1000, 1000 );
    poly.Collide( &probe, 0 );

    BOOST_REQUIRE( poly.TriangulatedPolyCount() > 0 );
    BOOST_REQUIRE( poly.TriangulationIndex() != nullptr );
}


/**
 * The contract the index must not break: DRC_RTREE stores raw TRI pointers, so a geometry Move()
 * must translate the triangles in place, never replace them.
 */
BOOST_AUTO_TEST_CASE( TriPointersSurviveMove )
{
    SHAPE_POLY_SET poly = MakeBlob();

    WarmAndRequireIndexed( poly );

    std::vector<const SHAPE*> before;
    poly.GetIndexableSubshapes( before );
    BOOST_REQUIRE( before.size() > 64 );

    std::vector<VECTOR2I> firstVertexBefore;

    for( const SHAPE* s : before )
        firstVertexBefore.push_back( static_cast<const SHAPE_LINE_CHAIN_BASE*>( s )->GetPoint( 0 ) );

    const VECTOR2I delta( 123456, -654321 );
    poly.Move( delta );

    std::vector<const SHAPE*> after;
    poly.GetIndexableSubshapes( after );

    BOOST_REQUIRE_EQUAL( before.size(), after.size() );

    for( size_t ii = 0; ii < before.size(); ++ii )
    {
        // Same objects, not clones: a DRC_RTREE built before the Move must stay valid.
        BOOST_CHECK_EQUAL( (const void*) before[ii], (const void*) after[ii] );

        VECTOR2I moved = static_cast<const SHAPE_LINE_CHAIN_BASE*>( after[ii] )->GetPoint( 0 );
        BOOST_CHECK_EQUAL( moved, firstVertexBefore[ii] + delta );
    }
}


/**
 * The prune box must be BBox() inflated by the clearance, not BBox( aClearance ):
 * SHAPE_COMPOUND::BBox() ignores its argument, so the latter silently drops near-miss triangles.
 * This is the exact falsifier for that bug.
 */
BOOST_AUTO_TEST_CASE( CompoundNearMissNotPruned )
{
    SHAPE_POLY_SET poly = MakeBlob();

    // Two segments forming a compound, sitting a known distance outside the outline.
    const int      gap = 20000;
    const VECTOR2I a( 1000000 + gap, -100000 );
    const VECTOR2I b( 1000000 + gap, 100000 );

    SHAPE_COMPOUND compound;
    compound.AddShape( new SHAPE_SEGMENT( a, b, 0 ) );
    compound.AddShape( new SHAPE_SEGMENT( b, b + VECTOR2I( 50000, 0 ), 0 ) );

    // Comfortably clear.
    BOOST_CHECK_EQUAL( Probe( poly, compound, gap / 2 ), ReferenceScan( poly, compound, gap / 2 ) );

    // Clearance large enough that the near-miss triangles matter: a pruned candidate set
    // would report no collision here while the full scan reports one.
    BOOST_CHECK_EQUAL( Probe( poly, compound, gap * 4 ), ReferenceScan( poly, compound, gap * 4 ) );

    BOOST_CHECK( Probe( poly, compound, gap * 4 ).collides );
}


/**
 * Minimum distances must match the full scan even at awkward positions: probes equidistant to
 * adjacent triangles, plus one at exactly the clearance distance to pin the
 * inclusive/exclusive box-overlap boundary.  (Which equidistant triangle reports the collision
 * is not pinned -- see PROBE_RESULT.)
 */
BOOST_AUTO_TEST_CASE( MinDistanceMatchesFullScan )
{
    SHAPE_POLY_SET poly = MakeBlob();

    const std::vector<std::pair<VECTOR2I, int>> cases = {
        { VECTOR2I( 1000000, 0 ), 0 },      // on the outline, between two triangles
        { VECTOR2I( 1010000, 0 ), 10000 },  // exactly at clearance distance
        { VECTOR2I( 1010000, 0 ), 9999 },   // one IU inside
        { VECTOR2I( 1010000, 0 ), 10001 },  // one IU outside
        { VECTOR2I( -400000, 0 ), 100000 }, // inside a hole, reaching its rim
        { VECTOR2I( 0, 999000 ), 5000 }     // near the outline on another axis
    };

    for( const auto& [pt, clearance] : cases )
    {
        SHAPE_RECT probe( pt, 1, 1 );
        BOOST_CHECK_EQUAL( Probe( poly, probe, clearance ), ReferenceScan( poly, probe, clearance ) );
    }
}


/**
 * Every other case here uses a blob big enough to index, so the un-indexed branch of the walk
 * never runs.  A polygon below the threshold exercises it: results must be identical either way,
 * because whether an index exists is a performance decision, not a semantic one.
 */
BOOST_AUTO_TEST_CASE( SubThresholdPolygonMatchesFullScan )
{
    SHAPE_POLY_SET poly;
    poly.AddOutline( MakeNgon( VECTOR2I( 0, 0 ), 1000000, 8 ) ); // 8 vertices -> ~6 triangles
    poly.CacheTriangulation();

    size_t triangles = 0;

    for( unsigned ii = 0; ii < poly.TriangulatedPolyCount(); ++ii )
        triangles += poly.TriangulatedPolygon( ii )->GetTriangleCount();

    // If this ever exceeds the threshold (RTREE_THRESHOLD, 32) the case stops testing what it
    // claims to.
    BOOST_REQUIRE( triangles < 32 );

    const std::vector<VECTOR2I> probes = { VECTOR2I( 0, 0 ), VECTOR2I( 900000, 0 ), VECTOR2I( 1100000, 0 ),
                                           VECTOR2I( 0, 900000 ) };

    for( const VECTOR2I& pt : probes )
    {
        SHAPE_RECT probe( pt, 1000, 1000 );
        BOOST_CHECK_EQUAL( Probe( poly, probe, 0 ), ReferenceScan( poly, probe, 0 ) );
        BOOST_CHECK_EQUAL( ProbeBool( poly, probe, 0 ), ReferenceScanBool( poly, probe, 0 ) );
    }
}


/**
 * The set-level invariant this design rests on: every SHAPE_POLY_SET operation that rebuilds,
 * discards or moves the triangles drops the set's index, so a live index can never describe
 * stale geometry.  (With one tree per set, TRIANGULATED_POLYGON's own mutators can no longer
 * self-guard -- this is the trade-off of the combined-tree design.)
 *
 * Each mutation with reachable geometry is checked twice over: the mechanism (the index is
 * gone immediately afterwards) and the behavior (post-mutation results match the oracle, so a
 * stale index could not hide).  The assigned geometry in the copy-assign leg is far from the
 * old, so a stale index could not accidentally agree.
 */
BOOST_AUTO_TEST_CASE( CheckIndexDropped )
{
    // Move: the index is dropped, and the rebuilt one must describe the new positions.
    SHAPE_POLY_SET poly = MakeBlob();
    WarmAndRequireIndexed( poly );

    const VECTOR2I delta( 5000000, 0 );
    poly.Move( delta );
    BOOST_CHECK( poly.TriangulationIndex() == nullptr );

    for( const VECTOR2I& pt : g_probes )
    {
        SHAPE_RECT shifted( pt + delta, 1000, 1000 );
        BOOST_CHECK_EQUAL( Probe( poly, shifted, 0 ), ReferenceScan( poly, shifted, 0 ) );
        BOOST_CHECK_EQUAL( ProbeBool( poly, shifted, 0 ), ReferenceScanBool( poly, shifted, 0 ) );

        // The old location must now be empty.
        SHAPE_RECT stale( pt, 1000, 1000 );
        BOOST_CHECK_EQUAL( Probe( poly, stale, 0 ), ReferenceScan( poly, stale, 0 ) );
    }

    // RemoveAllContours discards the triangles outright.
    poly = MakeBlob();
    WarmAndRequireIndexed( poly );
    poly.RemoveAllContours();
    BOOST_CHECK( poly.TriangulationIndex() == nullptr );

    // Re-triangulating replaces every polygon, so the index must not survive it either.
    poly = MakeBlob();
    WarmAndRequireIndexed( poly );
    poly.AddHole( MakeNgon( VECTOR2I( 0, -500000 ), 100000, 60 ) );
    poly.CacheTriangulation();
    BOOST_CHECK( poly.TriangulationIndex() == nullptr );

    for( const VECTOR2I& pt : g_probes )
    {
        SHAPE_RECT probe( pt, 1000, 1000 );
        BOOST_CHECK_EQUAL( Probe( poly, probe, 0 ), ReferenceScan( poly, probe, 0 ) );
        BOOST_CHECK_EQUAL( ProbeBool( poly, probe, 0 ), ReferenceScanBool( poly, probe, 0 ) );
    }

    // The new hole must read as empty.
    SHAPE_RECT inNewHole( VECTOR2I( 0, -500000 ), 1000, 1000 );
    BOOST_CHECK_EQUAL( Probe( poly, inNewHole, 0 ), ReferenceScan( poly, inNewHole, 0 ) );

    // Copy assignment, against a blob far from the first.
    SHAPE_POLY_SET target = MakeBlob();
    WarmAndRequireIndexed( target );

    SHAPE_POLY_SET other;
    other.AddOutline( MakeNgon( VECTOR2I( 8000000, 8000000 ), 900000, 180 ) );
    other.CacheTriangulation();

    target = other;

    BOOST_REQUIRE( target.TriangulatedPolyCount() > 0 );
    BOOST_CHECK( target.TriangulationIndex() == nullptr );

    const std::vector<VECTOR2I> probes = {
        VECTOR2I( 8000000, 8000000 ), // inside the new geometry
        VECTOR2I( 8890000, 8000000 ), // just inside its edge
        VECTOR2I( 8910000, 8000000 ), // just outside its edge
        VECTOR2I( 0, 0 )              // where the OLD geometry used to be
    };

    for( const VECTOR2I& pt : probes )
    {
        SHAPE_RECT probe( pt, 1000, 1000 );
        BOOST_CHECK_EQUAL( Probe( target, probe, 0 ), ReferenceScan( target, probe, 0 ) );
        BOOST_CHECK_EQUAL( ProbeBool( target, probe, 0 ), ReferenceScanBool( target, probe, 0 ) );
    }
}


BOOST_AUTO_TEST_SUITE_END()
