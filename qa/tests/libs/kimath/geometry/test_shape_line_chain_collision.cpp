/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <geometry/shape.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <geometry/segment_index.h>

#include "fixtures_geometry.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>


BOOST_AUTO_TEST_SUITE( SHAPE_LINE_CHAIN_COLLIDE_TEST )


// Sentinels far outside the fixtures' range, so an untouched output cannot look like a real one
static constexpr int  NO_ACTUAL = 123456789;
static const VECTOR2I NO_LOCATION( 123456789, -123456789 );


enum COLLISION_OUTPUTS
{
    WANT_NOTHING = 0,
    WANT_ACTUAL = 1,
    WANT_LOCATION = 2,
    WANT_BOTH = 3
};


static SHAPE_POLY_SET loadZone( const std::string& aFilename, int aZone )
{
    const std::string path = KI_TEST::GetTestDataRootDir() + "triangulation/" + aFilename;
    std::ifstream     stream( path );
    BOOST_REQUIRE_MESSAGE( stream, "Unable to open " << path );

    const std::string data( ( std::istreambuf_iterator<char>( stream ) ), {} );
    size_t            pos = 0;

    for( int zone = 0; zone <= aZone; ++zone )
    {
        pos = data.find( "(zone (layer \"", pos );
        BOOST_REQUIRE_MESSAGE( pos != std::string::npos, "Missing zone " << aZone << " in " << path );
        ++pos;
    }

    pos = data.find( "polyset ", pos );
    BOOST_REQUIRE_MESSAGE( pos != std::string::npos, "Missing polyset for zone " << aZone << " in " << path );

    // Parse consumes only the polyset expression, so handing it the rest of the file is harmless
    std::stringstream serialized( data.substr( pos ) );
    SHAPE_POLY_SET    result;
    BOOST_REQUIRE_MESSAGE( result.Parse( serialized ), "Unable to parse zone " << aZone << " in " << path );

    return result;
}


static SHAPE_LINE_CHAIN extractSubchain( const SHAPE_LINE_CHAIN& aSource, int aStart, int aSegmentCount )
{
    SHAPE_LINE_CHAIN result;
    result.Append( aSource.CSegment( aStart ).A );

    for( int i = 0; i < aSegmentCount; ++i )
        result.Append( aSource.CSegment( aStart + i ).B, true );

    return result;
}


struct COLLISION_OUTPUT
{
    bool     result;
    int      actual;
    VECTOR2I location;
};


// The pre-index algorithm, which walked every segment pair in sorted order
// Valid only for arc-free chains, since it skips the arc refinement pass
static COLLISION_OUTPUT referenceCollision( const SHAPE_LINE_CHAIN& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                                            int aOutputs )
{
    int      closest = std::numeric_limits<int>::max();
    VECTOR2I nearest;

    if( aB.IsClosed() && aA.GetPointCount() > 0 && aB.PointInside( aA.GetPoint( 0 ) ) )
    {
        closest = 0;
        nearest = aA.GetPoint( 0 );
    }
    else if( aA.IsClosed() && aB.GetPointCount() > 0 && aA.PointInside( aB.GetPoint( 0 ) ) )
    {
        closest = 0;
        nearest = aB.GetPoint( 0 );
    }
    else if( aClearance >= 0 )
    {
        std::vector<SEG> aSegments;
        std::vector<SEG> bSegments;

        for( int i = 0; i < aA.SegmentCount(); ++i )
            aSegments.push_back( aA.CSegment( i ) );

        for( int i = 0; i < aB.SegmentCount(); ++i )
            bSegments.push_back( aB.CSegment( i ) );

        auto segmentSort = []( const SEG& aFirst, const SEG& aSecond )
        {
            return aFirst.A.x < aSecond.A.x || ( aFirst.A.x == aSecond.A.x && aFirst.A.y < aSecond.A.y );
        };

        std::sort( aSegments.begin(), aSegments.end(), segmentSort );
        std::sort( bSegments.begin(), bSegments.end(), segmentSort );

        for( const SEG& a : aSegments )
        {
            for( const SEG& b : bSegments )
            {
                int distance = 0;

                if( a.Collide( b, aClearance, &distance ) )
                {
                    if( distance < closest )
                    {
                        nearest = a.NearestPoint( b );
                        closest = distance;
                    }

                    if( closest == 0 || !( aOutputs & WANT_ACTUAL ) )
                        break;
                }
            }
        }
    }

    COLLISION_OUTPUT result = { false, NO_ACTUAL, NO_LOCATION };

    if( closest == 0 || closest < aClearance )
    {
        result.result = true;

        if( aOutputs & WANT_ACTUAL )
            result.actual = closest;

        if( aOutputs & WANT_LOCATION )
            result.location = nearest;
    }

    return result;
}


static COLLISION_OUTPUT indexedCollision( const SHAPE_LINE_CHAIN& aA, const SHAPE_LINE_CHAIN& aB, int aClearance,
                                          int aOutputs )
{
    COLLISION_OUTPUT result = { false, NO_ACTUAL, NO_LOCATION };
    int*             actual = aOutputs & WANT_ACTUAL ? &result.actual : nullptr;
    VECTOR2I*        location = aOutputs & WANT_LOCATION ? &result.location : nullptr;

    result.result = static_cast<const SHAPE&>( aA ).Collide( &aB, aClearance, actual, location );

    return result;
}


static void checkCollisionParity( const SHAPE_LINE_CHAIN& aA, const SHAPE_LINE_CHAIN& aB, int aClearance )
{
    for( int outputs = WANT_NOTHING; outputs <= WANT_BOTH; ++outputs )
    {
        const COLLISION_OUTPUT expected = referenceCollision( aA, aB, aClearance, outputs );
        const COLLISION_OUTPUT actual = indexedCollision( aA, aB, aClearance, outputs );

        BOOST_CHECK_EQUAL( actual.result, expected.result );
        BOOST_CHECK_EQUAL( actual.actual, expected.actual );
        BOOST_CHECK( actual.location == expected.location );
    }
}


struct CORPUS_CASE
{
    const char* file;
    int         zone;
    int         outlineA;
    int         outlineB;
};


BOOST_AUTO_TEST_CASE( Collide_RealContourParity )
{
    const std::vector<CORPUS_CASE> cases = { { "One-Air-Max.kicad_polys", 18, 0, 3 },
                                             { "issue5093.kicad_polys", 3, 0, 1 },
                                             { "bad_triangulation_case.kicad_polys", 47, 0, 3 } };

    for( const CORPUS_CASE& test : cases )
    {
        const SHAPE_POLY_SET poly = loadZone( test.file, test.zone );
        BOOST_REQUIRE_GT( poly.OutlineCount(), std::max( test.outlineA, test.outlineB ) );

        const SHAPE_LINE_CHAIN& a = poly.COutline( test.outlineA );
        const SHAPE_LINE_CHAIN& b = poly.COutline( test.outlineB );
        BOOST_REQUIRE_EQUAL( a.ArcCount(), 0 );
        BOOST_REQUIRE_EQUAL( b.ArcCount(), 0 );

        for( int clearance : { -1, 0, 200000, 2000000 } )
        {
            checkCollisionParity( a, b, clearance );
            checkCollisionParity( b, a, clearance );
        }
    }
}


BOOST_AUTO_TEST_CASE( Collide_RealContourGateBoundaries )
{
    const SHAPE_POLY_SET    poly = loadZone( "issue5093.kicad_polys", 3 );
    const SHAPE_LINE_CHAIN& sourceA = poly.COutline( 0 );
    const SHAPE_LINE_CHAIN& sourceB = poly.COutline( 1 );
    // Sizes straddle the output gate of A>=32, B>=64 and 4096 pairs, so 31x133 stays direct while
    // 66x63 and 63x65 cross it in one argument order only
    const std::vector<std::pair<int, int>> sizes = { { 31, 133 }, { 32, 128 }, { 66, 63 }, { 63, 65 }, { 64, 64 } };
    BOOST_REQUIRE_EQUAL( sourceA.SegmentCount(), 265 );
    BOOST_REQUIRE_EQUAL( sourceB.SegmentCount(), 307 );

    for( const auto& [countA, countB] : sizes )
    {
        for( int startA : { 0, sourceA.SegmentCount() - countA } )
        {
            const SHAPE_LINE_CHAIN a = extractSubchain( sourceA, startA, countA );
            const SHAPE_LINE_CHAIN b = extractSubchain( sourceB, 0, countB );

            for( int clearance : { -1, 0, 200000, 2000000 } )
            {
                checkCollisionParity( a, b, clearance );
                checkCollisionParity( b, a, clearance );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( Collide_RealContourSharedSegments )
{
    const SHAPE_POLY_SET    poly = loadZone( "issue5093.kicad_polys", 3 );
    const SHAPE_LINE_CHAIN& source = poly.COutline( 0 );

    // Overlapping subchains share 32 segments, which drives the zero-distance exact-hit exit
    const SHAPE_LINE_CHAIN a = extractSubchain( source, 0, 64 );
    const SHAPE_LINE_CHAIN b = extractSubchain( source, 32, 128 );

    checkCollisionParity( a, b, 0 );
    checkCollisionParity( b, a, 0 );
}


BOOST_AUTO_TEST_CASE( SegmentIndex_RealContourCandidateSet )
{
    const SHAPE_POLY_SET    poly = loadZone( "One-Air-Max.kicad_polys", 18 );
    const SHAPE_LINE_CHAIN& contour = poly.COutline( 0 );
    std::vector<SEG>        segments;

    for( int i = 0; i < contour.SegmentCount(); ++i )
        segments.push_back( contour.CSegment( i ) );

    // Enough segments to force a multi-level tree rather than a single leaf
    SEGMENT_INDEX index( segments );
    BOOST_REQUIRE_GT( index.size(), 256 );

    for( size_t queryIndex = 0; queryIndex < segments.size(); queryIndex += 17 )
    {
        const SEG& query = segments[queryIndex];

        for( int padding : { 0, 200000, 2000000 } )
        {
            std::vector<int> expected;
            const int64_t    minX = static_cast<int64_t>( std::min( query.A.x, query.B.x ) ) - padding;
            const int64_t    minY = static_cast<int64_t>( std::min( query.A.y, query.B.y ) ) - padding;
            const int64_t    maxX = static_cast<int64_t>( std::max( query.A.x, query.B.x ) ) + padding;
            const int64_t    maxY = static_cast<int64_t>( std::max( query.A.y, query.B.y ) ) + padding;

            for( size_t i = 0; i < segments.size(); ++i )
            {
                const SEG& segment = segments[i];

                if( std::max( segment.A.x, segment.B.x ) >= minX && std::min( segment.A.x, segment.B.x ) <= maxX
                    && std::max( segment.A.y, segment.B.y ) >= minY && std::min( segment.A.y, segment.B.y ) <= maxY )
                {
                    expected.push_back( static_cast<int>( i ) );
                }
            }

            std::vector<int> actual;
            auto             visitor = [&]( int aItem )
            {
                actual.push_back( aItem );
                return true;
            };

            index.VisitCandidates( query, padding, visitor );
            std::sort( actual.begin(), actual.end() );
            BOOST_CHECK( actual == expected );
        }
    }

    int  calls = 0;
    auto stopVisitor = [&]( int )
    {
        ++calls;
        return false;
    };

    index.VisitCandidates( segments.front(), std::numeric_limits<int>::max(), stopVisitor );
    BOOST_CHECK_EQUAL( calls, 1 );
}

BOOST_AUTO_TEST_CASE( Collide_LineToLine )
{
    SHAPE_LINE_CHAIN lineA;
    lineA.Append( VECTOR2I( 0, 0 ) );
    lineA.Append( VECTOR2I( 10, 0 ) );

    SHAPE_LINE_CHAIN lineB;
    lineB.Append( VECTOR2I( 5, 5 ) );
    lineB.Append( VECTOR2I( 5, -5 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &lineA )->Collide( &lineB, 0, &actual, &location );

    BOOST_CHECK( collided );
    BOOST_TEST( actual == 0 );
    BOOST_CHECK_MESSAGE( location == VECTOR2I( 5, 0 ), "Expected: " << VECTOR2I( 5, 0 ) << " Actual: " << location );
}

BOOST_AUTO_TEST_CASE( Collide_LineToArc )
{
    SHAPE_LINE_CHAIN lineA;
    lineA.Append( VECTOR2I( 0, 0 ) );
    lineA.Append( VECTOR2I( 10, 0 ) );

    SHAPE_LINE_CHAIN arcB;
    arcB.Append( SHAPE_ARC( VECTOR2I( 5, 5 ), VECTOR2I( 6, 4 ), VECTOR2I( 7, 0 ), 0 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &lineA )->Collide( &arcB, 0, &actual, &location );

    BOOST_CHECK( collided );
    BOOST_TEST( actual == 0 );
    BOOST_CHECK_MESSAGE( location == VECTOR2I( 7, 0 ), "Expected: " << VECTOR2I( 7, 0 ) << " Actual: " << location );
}

BOOST_AUTO_TEST_CASE( Collide_ArcToArc )
{
    SHAPE_LINE_CHAIN arcA;
    arcA.Append( SHAPE_ARC( VECTOR2I( 0, 0 ), VECTOR2I( 10, 0 ), VECTOR2I( 5, 5 ), 0 ) );

    SHAPE_LINE_CHAIN arcB;
    arcB.Append( SHAPE_ARC( VECTOR2I( 5, 5 ), VECTOR2I( 5, -5 ), VECTOR2I( 10, 0 ), 0 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &arcA )->Collide( &arcB, 0, &actual, &location );

    BOOST_CHECK( collided );
    BOOST_TEST( actual == 0 );
    BOOST_CHECK_MESSAGE( location == VECTOR2I( 5, 5 ), "Expected: " << VECTOR2I( 5, 5 ) << " Actual: " << location );
}

BOOST_AUTO_TEST_CASE( Collide_WithClearance )
{
    SHAPE_LINE_CHAIN lineA;
    lineA.Append( VECTOR2I( 0, 0 ) );
    lineA.Append( VECTOR2I( 10, 0 ) );

    SHAPE_LINE_CHAIN lineB;
    lineB.Append( VECTOR2I( 5, 6 ) );
    lineB.Append( VECTOR2I( -5, 6 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &lineA )->Collide( &lineB, 7, &actual, &location );

    BOOST_CHECK( collided );
    BOOST_CHECK_MESSAGE( actual == 6, "Expected: " << 6 << " Actual: " << actual );
    BOOST_CHECK_MESSAGE( location == VECTOR2I( 0, 0 ), "Expected: " << VECTOR2I( 0, 0 ) << " Actual: " << location );
}

BOOST_AUTO_TEST_CASE( Collide_NoClearance )
{
    SHAPE_LINE_CHAIN lineA;
    lineA.Append( VECTOR2I( 0, 0 ) );
    lineA.Append( VECTOR2I( 10, 0 ) );

    SHAPE_LINE_CHAIN lineB;
    lineB.Append( VECTOR2I( 5, 6 ) );
    lineB.Append( VECTOR2I( -5, 6 ) );

    VECTOR2I location;
    int      actual = 0;
    bool     collided = static_cast<SHAPE*>( &lineA )->Collide( &lineB, 0, &actual, &location );

    BOOST_CHECK( !collided );
    BOOST_CHECK_MESSAGE( actual == 0, "Expected: " << 0 << " Actual: " << actual );
}

BOOST_AUTO_TEST_SUITE_END()
