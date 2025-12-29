/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <geometry/seg.h>

namespace
{

/**
 * Predicate to check expected collision between two segments
 * @param  aSegA the first #SEG
 * @param  aSegB      the second #SEG
 * @param  aClearance the collision clearance
 * @param  aExp  expected collision
 * @return       does the distance calculated agree?
 */
bool SegCollideCorrect( const SEG& aSegA, const SEG& aSegB, int aClearance, bool aExp )
{
    const bool AtoB = aSegA.Collide( aSegB, aClearance );
    const bool BtoA = aSegB.Collide( aSegA, aClearance );

    const bool ok = ( AtoB == aExp ) && ( BtoA == aExp );

    if( AtoB != BtoA )
    {
        std::stringstream ss;
        ss << "Segment collision is not the same in both directions: expected " << aExp << ", got "
           << AtoB << " & " << BtoA;
        BOOST_TEST_INFO( ss.str() );
    }
    else if( !ok )
    {
        std::stringstream ss;
        ss << "Collision incorrect: expected " << aExp << ", got " << AtoB;
        BOOST_TEST_INFO( ss.str() );
    }

    return ok;
}


/**
 * Predicate to check expected distance between two segments
 * @param  aSegA the first #SEG
 * @param  aSegB the second #SEG
 * @param  aExp  expected distance
 * @return       does the distance calculated agree?
 */
bool SegDistanceCorrect( const SEG& aSegA, const SEG& aSegB, int aExp )
{
    const int AtoB = aSegA.Distance( aSegB );
    const int BtoA = aSegB.Distance( aSegA );

    bool ok = ( AtoB == aExp ) && ( BtoA == aExp );

    if( AtoB != BtoA )
    {
        std::stringstream ss;
        ss << "Segment distance is not the same in both directions: expected " << aExp << ", got "
           << AtoB << " & " << BtoA;
        BOOST_TEST_INFO( ss.str() );
    }
    else if( !ok )
    {
        std::stringstream ss;
        ss << "Distance incorrect: expected " << aExp << ", got " << AtoB;
        BOOST_TEST_INFO( ss.str() );
    }

    // Sanity check: the collision should be consistent with the distance
    ok = ok && SegCollideCorrect( aSegA, aSegB, 0, aExp == 0 );

    return ok;
}

/**
 * Predicate to check expected distance between a segment and a point
 * @param  aSegA the segment
 * @param  aVec  the vector (point)
 * @param  aExp  expected distance
 * @return       does the distance calculated agree?
 */
bool SegVecDistanceCorrect( const SEG& aSeg, const VECTOR2I& aVec, int aExp )
{
    const SEG::ecoord squaredDistance = aSeg.SquaredDistance( aVec );
    BOOST_REQUIRE( squaredDistance >= 0 );

    const int dist = aSeg.Distance( aVec );

    bool ok = ( dist == aExp );

    if( !ok )
    {
        std::stringstream ss;
        ss << "Distance incorrect: expected " << aExp << ", got " << dist;
        BOOST_TEST_INFO( ss.str() );
    }

    return ok;
}

/**
 * Predicate to check expected collision between two segments
 * @param  aSegA the first #SEG
 * @param  sSegB the second #SEG
 * @param  aExp  expected collinearity
 * @return       does the collinearity calculated agree?
 */
bool SegCollinearCorrect( const SEG& aSegA, const SEG& aSegB, bool aExp )
{
    const bool AtoB = aSegA.Collinear( aSegB );
    const bool BtoA = aSegB.Collinear( aSegA );

    const bool ok = ( AtoB == aExp ) && ( BtoA == aExp );

    if( AtoB != BtoA )
    {
        std::stringstream ss;
        ss << "Segment collinearity is not the same in both directions: expected " << aExp
           << ", got " << AtoB << " & " << BtoA;
        BOOST_TEST_INFO( ss.str() );
    }
    else if( !ok )
    {
        std::stringstream ss;
        ss << "Collinearity incorrect: expected " << aExp << ", got " << AtoB;
        BOOST_TEST_INFO( ss.str() );
    }

    return ok;
}

/**
 * Predicate to check expected parallelism between two segments
 * @param  aSegA the first #SEG
 * @param  sSegB the second #SEG
 * @param  aExp  expected parallelism: true = segments are parallel
 *                                     false = segments are not parallel
 * @return       does the parallelism calculated agree?
 */
bool SegParallelCorrect( const SEG& aSegA, const SEG& aSegB, bool aExp )
{
    const bool AtoB = aSegA.ApproxParallel( aSegB );
    const bool BtoA = aSegB.ApproxParallel( aSegA );

    const bool ok = ( AtoB == aExp ) && ( BtoA == aExp );

    if( AtoB != BtoA )
    {
        std::stringstream ss;
        ss << "Segment parallelism is not the same in both directions: expected " << aExp
           << ", got AtoB: " << AtoB << " BtoA:" << BtoA;
        BOOST_TEST_INFO( ss.str() );
    }
    else if( !ok )
    {
        std::stringstream ss;
        ss << "Parallelism incorrect: expected " << aExp << ", got " << AtoB;
        BOOST_TEST_INFO( ss.str() );
    }

    return ok;
}

/**
 * Predicate to check expected perpendicularity between two segments
 * @param  aSegA the first #SEG
 * @param  sSegB the second #SEG
 * @param  aExp  expected perpendicularity: true = segments are perpendicular
 *                                          false = segments are not perpendicular
 * @return       does the perpendicularity calculated agree?
 */
bool SegPerpendicularCorrect( const SEG& aSegA, const SEG& aSegB, bool aExp )
{
    const bool AtoB = aSegA.ApproxPerpendicular( aSegB );
    const bool BtoA = aSegB.ApproxPerpendicular( aSegA );

    const bool ok = ( AtoB == aExp ) && ( BtoA == aExp );

    if( AtoB != BtoA )
    {
        std::stringstream ss;
        ss << "Segment perpendicularity is not the same in both directions: expected " << aExp
           << ", got AtoB: " << AtoB << " BtoA:" << BtoA;
        BOOST_TEST_INFO( ss.str() );
    }
    else if( !ok )
    {
        std::stringstream ss;
        ss << "Perpendicularity incorrect: expected " << aExp << ", got " << AtoB;
        BOOST_TEST_INFO( ss.str() );
    }

    return ok;
}

} // namespace

BOOST_AUTO_TEST_SUITE( Segment )

/**
 * Checks whether the construction of a segment referencing external points works
 * and that the endpoints can be modified as normal points.
 */
BOOST_AUTO_TEST_CASE( EndpointCtorMod )
{
    const VECTOR2I pointA{ 10, 20 };
    const VECTOR2I pointB{ 100, 200 };

    // Build a segment referencing the previous points
    SEG segment( pointA, pointB );

    BOOST_CHECK_EQUAL( pointA, VECTOR2I( 10, 20 ) );
    BOOST_CHECK_EQUAL( pointB, VECTOR2I( 100, 200 ) );

    // Modify the ends of the segments
    segment.A += VECTOR2I( 10, 10 );
    segment.B += VECTOR2I( 100, 100 );

    // Check that the ends in segment are modified
    BOOST_CHECK_EQUAL( segment.A, VECTOR2I( 20, 30 ) );
    BOOST_CHECK_EQUAL( segment.B, VECTOR2I( 200, 300 ) );
}

struct SEG_SEG_DISTANCE_CASE : public KI_TEST::NAMED_CASE
{
    SEG         m_seg_a;
    SEG         m_seg_b;
    int         m_exp_dist;
};


// clang-format off
static const std::vector<SEG_SEG_DISTANCE_CASE> seg_seg_dist_cases = {
    {
        "Parallel, 10 apart",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 10 }, { 10, 10 } },
        10,
    },
    {
        "Non-parallel, 10 apart",
        { { 0, -5 }, { 10, 0 } },
        { { 0, 10 }, { 10, 10 } },
        10,
    },
    {
        "Co-incident",
        { { 0, 0 }, { 30, 0 } },
        { { 10, 0 }, { 20, 0 } },
        0,
    },
    {
        "Crossing",
        { { 0, -10 }, { 0, 10 } },
        { { -20, 0 }, { 20, 0 } },
        0,
    },
    {
        "T-junction",
        { { 0, -10 }, { 0, 10 } },
        { { -20, 0 }, { 0, 0 } },
        0,
    },
    {
        "T-junction (no touch)",
        { { 0, -10 }, { 0, 10 } },
        { { -20, 0 }, { -2, 0 } },
        2,
    },
    {
        "Zero-length segment A",
        { { 0, 0 }, { 0, 0 } },
        { { 10, 0 }, { 20, 0 } },
        10,
    },
    {
        "Zero-length segment B",
        { { 10, 0 }, { 20, 0 } },
        { { 0, 0 }, { 0, 0 } },
        10,
    },
    {
        "Both zero-length",
        { { 0, 0 }, { 0, 0 } },
        { { 10, 0 }, { 10, 0 } },
        10,
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( SegSegDistance, boost::unit_test::data::make( seg_seg_dist_cases ), c )
{
    BOOST_CHECK_PREDICATE( SegDistanceCorrect, ( c.m_seg_a )( c.m_seg_b )( c.m_exp_dist ) );
}


struct SEG_VECTOR_DISTANCE_CASE : public KI_TEST::NAMED_CASE
{
    SEG         m_seg;
    VECTOR2I    m_vec;
    int         m_exp_dist;
};


// clang-format off
static const std::vector<SEG_VECTOR_DISTANCE_CASE> seg_vec_dist_cases = {
    {
        "On endpoint",
        { { 0, 0 }, { 10, 0 } },
        { 0, 0 },
        0,
    },
    {
        "On segment",
        { { 0, 0 }, { 10, 0 } },
        { 3, 0 },
        0,
    },
    {
        "At side",
        { { 0, 0 }, { 10, 0 } },
        { 3, 2 },
        2,
    },
    {
        "At end (collinear)",
        { { 0, 0 }, { 10, 0 } },
        { 12, 0 },
        2,
    },
    {
        "At end (not collinear)",
        { { 0, 0 }, { 1000, 0 } },
        { 1000 + 200, 200 },
        282, // sqrt(200^2 + 200^2) = 282.8, rounded to nearest
    },
    {
        "Issue 18473 (inside hit with rounding error)",
        { { 187360000, 42510000 }, { 105796472, 42510000 } },
        { 106645000, 42510000 },
          0,
    },
    {
        "Straight line x distance",
        { { 187360000, 42510000 }, { 105796472, 42510000 } },
          { 197360000, 42510000 },
             10000000,
    },
    {
        "Straight line -x distance",
        { { 187360000, 42510000 }, { 105796472, 42510000 } },
          { 104796472, 42510000 },
              1000000,
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( SegVecDistance, boost::unit_test::data::make( seg_vec_dist_cases ), c )
{
    BOOST_CHECK_PREDICATE( SegVecDistanceCorrect, ( c.m_seg )( c.m_vec )( c.m_exp_dist ) );
}


/**
 * Test cases for collisions (with clearance, for no clearance,
 * it's just a SEG_SEG_DISTANCE_CASE of 0)
 */
struct SEG_SEG_COLLIDE_CASE : public KI_TEST::NAMED_CASE
{
    SEG         m_seg_a;
    SEG         m_seg_b;
    int         m_clearance;
    bool        m_exp_coll;
};


// clang-format off
static const std::vector<SEG_SEG_COLLIDE_CASE> seg_seg_coll_cases = {
    {
        "Parallel, 10 apart, 5 clear",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 10 }, { 10, 10 } },
        5,
        false,
    },
    {
        "Parallel, 10 apart, 10 clear",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 10 }, { 10, 10 } },
        10,
        false,
    },
    {
        "Parallel, 10 apart, 11 clear",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 10 }, { 10, 10 } },
        11,
        true,
    },
    {
        "T-junction, 2 apart, 2 clear",
        { { 0, -10 }, { 0, 0 } },
        { { -20, 0 }, { -2, 0 } },
        2,
        false,
    },
    {
        "T-junction, 2 apart, 3 clear",
        { { 0, -10 }, { 0, 0 } },
        { { -20, 0 }, { -2, 0 } },
        3,
        true,
    },
    {
        "Zero-length segment A, 10 apart",
        { { 0, 0 }, { 0, 0 } },
        { { 10, 0 }, { 20, 0 } },
        0,
        false,
    },
    {
        "Zero-length segment A, 10 apart, 9 clear",
        { { 0, 0 }, { 0, 0 } },
        { { 10, 0 }, { 20, 0 } },
        9,
        false,
    },
    {
        "Zero-length segment A, 10 apart, 10 clear",
        { { 0, 0 }, { 0, 0 } },
        { { 10, 0 }, { 20, 0 } },
        10,
        false,
    },
    {
        "Zero-length segment A, 10 apart, 11 clear",
        { { 0, 0 }, { 0, 0 } },
        { { 10, 0 }, { 20, 0 } },
        11,
        true,
    },
    {
        "Zero-length segment B, 10 apart",
        { { 10, 0 }, { 20, 0 } },
        { { 0, 0 }, { 0, 0 } },
        0,
        false,
    },
    {
        "Both zero-length, same point",
        { { 5, 5 }, { 5, 5 } },
        { { 5, 5 }, { 5, 5 } },
        0,
        true,
    },
    {
        "Both zero-length, 10 apart",
        { { 0, 0 }, { 0, 0 } },
        { { 10, 0 }, { 10, 0 } },
        0,
        false,
    },
    {
        "Zero-length on segment",
        { { 5, 0 }, { 5, 0 } },
        { { 0, 0 }, { 10, 0 } },
        0,
        true,
    },
    {
        "Zero-length near segment, x overlaps but y differs",
        { { 5, 5 }, { 5, 5 } },
        { { 0, 0 }, { 10, 0 } },
        0,
        false,
    },
    {
        "Zero-length near segment, x overlaps but y differs, 4 clear",
        { { 5, 5 }, { 5, 5 } },
        { { 0, 0 }, { 10, 0 } },
        4,
        false,
    },
    {
        "Zero-length near segment, x overlaps but y differs, 5 clear",
        { { 5, 5 }, { 5, 5 } },
        { { 0, 0 }, { 10, 0 } },
        5,
        false,
    },
    {
        "Zero-length near segment, x overlaps but y differs, 6 clear",
        { { 5, 5 }, { 5, 5 } },
        { { 0, 0 }, { 10, 0 } },
        6,
        true,
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( SegSegCollision, boost::unit_test::data::make( seg_seg_coll_cases ), c )
{
    BOOST_CHECK_PREDICATE( SegCollideCorrect,
                           ( c.m_seg_a )( c.m_seg_b )( c.m_clearance )( c.m_exp_coll ) );
}


/**
 * Struct to hold general cases for collinearity, parallelism and perpendicularity
 */
struct SEG_SEG_BOOLEAN_CASE : public KI_TEST::NAMED_CASE
{
    SEG         m_seg_a;
    SEG         m_seg_b;
    bool        m_exp_result;
};

// clang-format off
/**
 * Test cases for collinearity
 */
static const std::vector<SEG_SEG_BOOLEAN_CASE> seg_vec_collinear_cases = {
    {
        "coincident",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 0 }, { 10, 0 } },
        true,
    },
    {
        "end-to-end",
        { { 0, 0 }, { 10, 0 } },
        { { 10, 0 }, { 20, 0 } },
        true,
    },
    {
        "In segment",
        { { 0, 0 }, { 10, 0 } },
        { { 4, 0 }, { 7, 0 } },
        true,
    },
    {
        "At side, parallel",
        { { 0, 0 }, { 10, 0 } },
        { { 4, 1 }, { 7, 1 } },
        false,
    },
    {
        "crossing",
        { { 0, 0 }, { 10, 0 } },
        { { 5, -5 }, { 5, 5 } },
        false,
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( SegSegCollinear, boost::unit_test::data::make( seg_vec_collinear_cases ), c )
{
    BOOST_CHECK_PREDICATE( SegCollinearCorrect, ( c.m_seg_a )( c.m_seg_b )( c.m_exp_result ) );
}


// clang-format off
/**
 * Test cases for parallelism
 */
static const std::vector<SEG_SEG_BOOLEAN_CASE> seg_vec_parallel_cases = {
    {
        "coincident",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 0 }, { 10, 0 } },
        true,
    },
    {
        "end-to-end",
        { { 0, 0 }, { 10, 0 } },
        { { 10, 0 }, { 20, 0 } },
        true,
    },
    {
        "In segment",
        { { 0, 0 }, { 10, 0 } },
        { { 4, 0 }, { 7, 0 } },
        true,
    },
    {
        "At side, parallel",
        { { 0, 0 }, { 10, 0 } },
        { { 4, 1 }, { 7, 1 } },
        true,
    },
    {
        "crossing",
        { { 0, 0 }, { 10, 0 } },
        { { 5, -5 }, { 5, 5 } },
        false,
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( SegSegParallel, boost::unit_test::data::make( seg_vec_parallel_cases ), c )
{
    BOOST_CHECK_PREDICATE( SegParallelCorrect, ( c.m_seg_a )( c.m_seg_b )( c.m_exp_result ) );
}


// clang-format off
/**
 * Test cases for perpendicularity
 */
static const std::vector<SEG_SEG_BOOLEAN_CASE> seg_vec_perpendicular_cases = {
    {
        "coincident",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 0 }, { 10, 0 } },
        false,
    },
    {
        "end-to-end",
        { { 0, 0 }, { 10, 0 } },
        { { 10, 0 }, { 20, 0 } },
        false,
    },
    {
        "In segment",
        { { 0, 0 }, { 10, 0 } },
        { { 4, 0 }, { 7, 0 } },
        false,
    },
    {
        "At side, parallel",
        { { 0, 0 }, { 10, 0 } },
        { { 4, 1 }, { 7, 1 } },
        false,
    },
    {
        "crossing 45 deg",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 0 }, { 5, 5 } },
        false,
    },
    {
        "very nearly perpendicular",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 0 }, { 1, 10 } },
        true, //allow error margin of 1 IU
    },
    {
        "not really perpendicular",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 0 }, { 3, 10 } },
        false,
    },
    {
        "perpendicular",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 0 }, { 0, 10 } },
        true,
    },
    {
        "perpendicular not intersecting",
        { { 0, 0 }, { 10, 0 } },
        { { 15, 5 }, { 15, 10 } },
        true,
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( SegSegPerpendicular,
                      boost::unit_test::data::make( seg_vec_perpendicular_cases ), c )
{
    BOOST_CHECK_PREDICATE( SegPerpendicularCorrect, ( c.m_seg_a )( c.m_seg_b )( c.m_exp_result ) );
}


/**
 * Struct to hold cases for operations with a #SEG, and a #VECTOR2I
 */
struct SEG_VEC_CASE : public KI_TEST::NAMED_CASE
{
    SEG         m_seg;
    VECTOR2I    m_vec;
};


// clang-format off
/**
 * Test cases to create segments passing through a point
 */
static const std::vector<SEG_VEC_CASE> segment_and_point_cases = {
    {
        "Horizontal: point on edge of seg",
        { { 0, 0 }, { 10, 0 } },
        { 0, 0 },
    },
    {
        "Horizontal: point in middle of seg",
        { { 0, 0 }, { 10, 0 } },
        { 5, 0 },
    },
    {
        "Horizontal: point outside seg",
        { { 0, 0 }, { 10, 0 } },
        { 20, 20 },
    },
    {
        "Vertical: point on edge of seg",
        { { 0, 0 }, { 0, 10 } },
        { 0, 0 },
    },
    {
        "Vertical: point in middle of seg",
        { { 0, 0 }, { 0, 10 } },
        { 0, 5 },
    },
    {
        "Vertical: point outside seg",
        { { 0, 0 }, { 0, 10 } },
        { 20, 20 },
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( SegCreateParallel, boost::unit_test::data::make( segment_and_point_cases ),
                      c )
{
    const SEG perpendicular = c.m_seg.ParallelSeg( c.m_vec );

    BOOST_CHECK_PREDICATE( SegParallelCorrect, (perpendicular) ( c.m_seg )( true ) );
    BOOST_CHECK_PREDICATE( SegVecDistanceCorrect, (perpendicular) ( c.m_vec )( 0 ) );
}

BOOST_DATA_TEST_CASE( SegCreatePerpendicular,
                      boost::unit_test::data::make( segment_and_point_cases ), c )
{
    const SEG perpendicular = c.m_seg.PerpendicularSeg( c.m_vec );

    BOOST_CHECK_PREDICATE( SegPerpendicularCorrect, (perpendicular) ( c.m_seg )( true ) );
    BOOST_CHECK_PREDICATE( SegVecDistanceCorrect, (perpendicular) ( c.m_vec )( 0 ) );
}

BOOST_AUTO_TEST_CASE( LineDistance )
{
    SEG seg( { 0, 0 }, { 10, 0 } );

    BOOST_TEST( seg.LineDistance( { 5, 0 } ) == 0 );
    BOOST_TEST( seg.LineDistance( { 5, 8 } ) == 8 );
}

BOOST_AUTO_TEST_CASE( LineDistanceSided )
{
    SEG seg( { 0, 0 }, { 10, 0 } );

    BOOST_TEST( seg.LineDistance( { 5, 8 }, true ) == 8 );
    BOOST_TEST( seg.LineDistance( { 5, -8 }, true ) == -8 );
}

/**
 * Test cases for segment intersection
 */
struct SEG_SEG_INTERSECT_CASE : public KI_TEST::NAMED_CASE
{
    SEG         m_seg_a;
    SEG         m_seg_b;
    bool        m_ignore_endpoints;
    bool        m_lines;
    bool        m_exp_intersect;
    VECTOR2I    m_exp_point;
};

// clang-format off
static const std::vector<SEG_SEG_INTERSECT_CASE> seg_intersect_cases = {
    // Basic crossing cases
    {
        "Crossing at origin",
        { { -10, 0 }, { 10, 0 } },
        { { 0, -10 }, { 0, 10 } },
        false, false, true,
        { 0, 0 }
    },
    {
        "Crossing at (5,5)",
        { { 0, 5 }, { 10, 5 } },
        { { 5, 0 }, { 5, 10 } },
        false, false, true,
        { 5, 5 }
    },
    {
        "T-junction intersection",
        { { 0, 0 }, { 10, 0 } },
        { { 5, -5 }, { 5, 0 } },
        false, false, true,
        { 5, 0 }
    },

    // Non-intersecting cases
    {
        "Parallel segments",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 5 }, { 10, 5 } },
        false, false, false,
        { 0, 0 }
    },
    {
        "Separated segments",
        { { 0, 0 }, { 5, 0 } },
        { { 10, 0 }, { 15, 0 } },
        false, false, false,
        { 0, 0 }
    },
    {
        "Lines would intersect, but segments don't",
        { { 0, 0 }, { 2, 0 } },
        { { 5, -5 }, { 5, 5 } },
        false, false, false,
        { 0, 0 }
    },

    // Endpoint intersection cases
    {
        "Endpoint touching - should intersect",
        { { 0, 0 }, { 10, 0 } },
        { { 10, 0 }, { 20, 0 } },
        false, false, true,
        { 10, 0 }
    },
    {
        "Endpoint touching - ignore endpoints",
        { { 0, 0 }, { 10, 0 } },
        { { 10, 0 }, { 20, 0 } },
        true, false, false,
        { 0, 0 }
    },
    {
        "Endpoint touching at angle",
        { { 0, 0 }, { 10, 0 } },
        { { 10, 0 }, { 15, 5 } },
        false, false, true,
        { 10, 0 }
    },

    // Collinear cases
    {
        "Collinear overlapping segments",
        { { 0, 0 }, { 10, 0 } },
        { { 5, 0 }, { 15, 0 } },
        false, false, true,
        { 7, 0 }  // Midpoint of overlap [5,10]
    },
    {
        "Collinear non-overlapping segments",
        { { 0, 0 }, { 5, 0 } },
        { { 10, 0 }, { 15, 0 } },
        false, false, false,
        { 0, 0 }
    },
    {
        "Collinear touching at endpoint",
        { { 0, 0 }, { 10, 0 } },
        { { 10, 0 }, { 20, 0 } },
        false, false, true,
        { 10, 0 }
    },
    {
        "Collinear contained segment",
        { { 0, 0 }, { 20, 0 } },
        { { 5, 0 }, { 15, 0 } },
        false, false, true,
        { 10, 0 }  // Midpoint of contained segment
    },
    {
        "Collinear vertical overlapping",
        { { 5, 0 }, { 5, 10 } },
        { { 5, 5 }, { 5, 15 } },
        false, false, true,
        { 5, 7 }  // Midpoint of overlap [5,10]
    },

    // Line mode cases (infinite lines)
    {
        "Lines intersect, segments don't",
        { { 0, 0 }, { 2, 0 } },
        { { 5, -5 }, { 5, 5 } },
        false, true, true,
        { 5, 0 }
    },
    {
        "Parallel lines (infinite)",
        { { 0, 0 }, { 10, 0 } },
        { { 0, 5 }, { 10, 5 } },
        false, true, false,
        { 0, 0 }
    },
    {
        "Collinear lines (infinite)",
        { { 0, 0 }, { 10, 0 } },
        { { 20, 0 }, { 30, 0 } },
        false, true, true,
        { 10, 0 }  // Midpoint between segment starts
    },

    // Edge cases
    {
        "Zero-length segment intersection",
        { { 5, 5 }, { 5, 5 } },
        { { 0, 5 }, { 10, 5 } },
        false, false, true,
        { 5, 5 }
    },
    {
        "Both zero-length, same point",
        { { 5, 5 }, { 5, 5 } },
        { { 5, 5 }, { 5, 5 } },
        false, false, true,
        { 5, 5 }
    },
    {
        "Both zero-length, different points",
        { { 5, 5 }, { 5, 5 } },
        { { 10, 10 }, { 10, 10 } },
        false, false, false,
        { 0, 0 }
    },

    // Diagonal intersection cases
    {
        "45-degree crossing",
        { { 0, 0 }, { 10, 10 } },
        { { 0, 10 }, { 10, 0 } },
        false, false, true,
        { 5, 5 }
    },
    {
        "Arbitrary angle crossing",
        { { 0, 0 }, { 6, 8 } },
        { { 0, 8 }, { 6, 0 } },
        false, false, true,
        { 3, 4 }
    },

    // Bounding box optimization test cases
    {
        "Far apart horizontal segments",
        { { 0, 0 }, { 10, 0 } },
        { { 100, 0 }, { 110, 0 } },
        false, false, false,
        { 0, 0 }
    },
    {
        "Far apart vertical segments",
        { { 0, 0 }, { 0, 10 } },
        { { 0, 100 }, { 0, 110 } },
        false, false, false,
        { 0, 0 }
    },
    {
        "Far apart diagonal segments",
        { { 0, 0 }, { 10, 10 } },
        { { 100, 100 }, { 110, 110 } },
        false, false, false,
        { 0, 0 }
    },
};
// clang-format on


/**
 * Predicate to check expected intersection between two segments
 * @param  aCase the test case containing all parameters
 * @return does the intersection calculated agree?
 */
bool SegIntersectCorrect( const SEG_SEG_INTERSECT_CASE& aCase )
{
    const auto resultA = aCase.m_seg_a.Intersect( aCase.m_seg_b, aCase.m_ignore_endpoints, aCase.m_lines );
    const auto resultB = aCase.m_seg_b.Intersect( aCase.m_seg_a, aCase.m_ignore_endpoints, aCase.m_lines );

    const bool intersectsA = resultA.has_value();
    const bool intersectsB = resultB.has_value();

    bool ok = ( intersectsA == aCase.m_exp_intersect ) && ( intersectsB == aCase.m_exp_intersect );

    if( intersectsA != intersectsB )
    {
        std::stringstream ss;
        ss << "Segment intersection is not the same in both directions: expected " << aCase.m_exp_intersect
           << ", got " << intersectsA << " & " << intersectsB;
        BOOST_TEST_INFO( ss.str() );
        ok = false;
    }
    else if( !ok )
    {
        std::stringstream ss;
        ss << "Intersection incorrect: expected " << aCase.m_exp_intersect << ", got " << intersectsA;
        BOOST_TEST_INFO( ss.str() );
    }

    // Check intersection point if intersection was expected
    if( ok && aCase.m_exp_intersect && aCase.m_exp_point != VECTOR2I() )
    {
        // Allow some tolerance for intersection point calculation
        const int tolerance = 1;

        if( !resultA || !resultB )
        {
            std::stringstream ss;
            ss << "Expected intersection but got nullopt";
            BOOST_TEST_INFO( ss.str() );
            ok = false;
        }
        else
        {
            const VECTOR2I pointA = *resultA;
            const VECTOR2I pointB = *resultB;

            bool pointOk = ( std::abs( pointA.x - aCase.m_exp_point.x ) <= tolerance &&
                            std::abs( pointA.y - aCase.m_exp_point.y ) <= tolerance &&
                            std::abs( pointB.x - aCase.m_exp_point.x ) <= tolerance &&
                            std::abs( pointB.y - aCase.m_exp_point.y ) <= tolerance );

            if( !pointOk )
            {
                std::stringstream ss;
                ss << "Intersection point incorrect: expected " << aCase.m_exp_point.Format()
                   << ", got " << pointA.Format() << " & " << pointB.Format();
                BOOST_TEST_INFO( ss.str() );
                ok = false;
            }
        }
    }

    return ok;
}

BOOST_DATA_TEST_CASE( SegSegIntersection, boost::unit_test::data::make( seg_intersect_cases ), c )
{
    BOOST_CHECK_PREDICATE( SegIntersectCorrect, ( c ) );
}


// Additional focused test cases for specific scenarios
BOOST_AUTO_TEST_CASE( IntersectLargeCoordinates )
{
    // Test with large coordinates to verify overflow protection
    SEG segA( { 1000000000, 0 }, { -1000000000, 0 } );
    SEG segB( { 0, 1000000000 }, { 0, -1000000000 } );

    auto intersection = segA.Intersect( segB, false, false );

    BOOST_CHECK( intersection.has_value() );
    BOOST_CHECK_EQUAL( intersection->x, 0 );
    BOOST_CHECK_EQUAL( intersection->y, 0 );
}

BOOST_AUTO_TEST_CASE( IntersectOverflowDetection )
{
    // Test intersection that would overflow coordinate range
    constexpr int max_coord = std::numeric_limits<int>::max();

    SEG segA( { 0, 0 }, { max_coord, max_coord } );
    SEG segB( { max_coord, 0 }, { 0, max_coord } );

    // This should either work or return nullopt due to overflow protection
    auto intersection = segA.Intersect( segB, false, false );

    // The test passes if it doesn't crash - the exact result depends on overflow handling
    BOOST_TEST_MESSAGE( "Overflow test completed without crash. Has intersection: " << intersection.has_value() );
    if( intersection.has_value() )
    {
        BOOST_TEST_MESSAGE( "Intersection point: " << intersection->Format() );
    }
}

BOOST_AUTO_TEST_CASE( IntersectPrecisionEdgeCases )
{
    // Test cases that might have precision issues
    SEG segA( { 0, 0 }, { 1000000, 1 } );
    SEG segB( { 500000, -1 }, { 500000, 2 } );

    auto intersection = segA.Intersect( segB, false, false );

    BOOST_CHECK( intersection.has_value() );
    // The intersection should be very close to (500000, 0.5), rounded to (500000, 1) or (500000, 0)
    BOOST_CHECK_EQUAL( intersection->x, 500000 );
    BOOST_CHECK( intersection->y >= 0 && intersection->y <= 1 );
}

BOOST_AUTO_TEST_CASE( IntersectIgnoreEndpointsEdgeCases )
{
    // Test edge cases with ignore endpoints
    SEG segA( { 0, 0 }, { 10, 0 } );
    SEG segB( { 5, -5 }, { 5, 5 } );

    // Normal intersection should work
    auto intersection1 = segA.Intersect( segB, false, false );
    BOOST_CHECK( intersection1.has_value() );
    BOOST_CHECK_EQUAL( *intersection1, VECTOR2I( 5, 0 ) );

    // Should still work when ignoring endpoints (this is a middle intersection)
    auto intersection2 = segA.Intersect( segB, true, false );
    BOOST_CHECK( intersection2.has_value() );
    BOOST_CHECK_EQUAL( *intersection2, VECTOR2I( 5, 0 ) );

    // Test actual endpoint intersection
    SEG segC( { 10, 0 }, { 20, 0 } );
    auto intersection3 = segA.Intersect( segC, false, false );
    BOOST_CHECK( intersection3.has_value() );
    BOOST_CHECK_EQUAL( *intersection3, VECTOR2I( 10, 0 ) );

    // Should not intersect when ignoring endpoints
    auto intersection4 = segA.Intersect( segC, true, false );
    BOOST_CHECK( !intersection4.has_value() );
}

BOOST_AUTO_TEST_CASE( IntersectCollinearRegressionTests )
{
    // Regression tests for collinear segment handling

    // Test case: horizontal segments with partial overlap
    SEG seg1( { 0, 5 }, { 10, 5 } );
    SEG seg2( { 5, 5 }, { 15, 5 } );

    auto intersection = seg1.Intersect( seg2, false, false );

    BOOST_CHECK( intersection.has_value() );
    BOOST_CHECK_EQUAL( intersection->y, 5 );
    BOOST_CHECK( intersection->x >= 5 && intersection->x <= 10 ); // Should be in overlap region

    // Test case: vertical segments with complete overlap (one contained in other)
    SEG seg3( { 3, 0 }, { 3, 20 } );
    SEG seg4( { 3, 5 }, { 3, 15 } );

    auto intersection2 = seg3.Intersect( seg4, false, false );

    BOOST_CHECK( intersection2.has_value() );
    BOOST_CHECK_EQUAL( intersection2->x, 3 );
    BOOST_CHECK( intersection2->y >= 5 && intersection2->y <= 15 ); // Should be in contained segment

    // Test case: diagonal collinear segments
    SEG seg5( { 0, 0 }, { 10, 10 } );
    SEG seg6( { 5, 5 }, { 15, 15 } );

    auto intersection3 = seg5.Intersect( seg6, false, false );

    BOOST_CHECK( intersection3.has_value() );
    BOOST_CHECK( intersection3->x >= 5 && intersection3->x <= 10 );
    BOOST_CHECK( intersection3->y >= 5 && intersection3->y <= 10 );
    BOOST_CHECK_EQUAL( intersection3->x, intersection3->y ); // Should maintain diagonal relationship

    // Test case: collinear segments that touch only at endpoints
    SEG seg7( { 0, 0 }, { 5, 0 } );
    SEG seg8( { 5, 0 }, { 10, 0 } );

    auto intersection4 = seg7.Intersect( seg8, false, false );
    BOOST_CHECK( intersection4.has_value() );
    BOOST_CHECK_EQUAL( *intersection4, VECTOR2I( 5, 0 ) );

    // Same test but ignoring endpoints
    auto intersection5 = seg7.Intersect( seg8, true, false );
    BOOST_CHECK( !intersection5.has_value() );

    // Test case: collinear segments that don't overlap
    SEG seg9( { 0, 0 }, { 5, 0 } );
    SEG seg10( { 10, 0 }, { 15, 0 } );

    auto intersection6 = seg9.Intersect( seg10, false, false );
    BOOST_CHECK( !intersection6.has_value() );
}

BOOST_AUTO_TEST_CASE( IntersectBoundingBoxOptimization )
{
    // Test that bounding box optimization works correctly

    // Segments that are clearly separated - should be rejected quickly
    SEG seg1( { 0, 0 }, { 10, 10 } );
    SEG seg2( { 100, 100 }, { 110, 110 } );

    auto intersection = seg1.Intersect( seg2, false, false );
    BOOST_CHECK( !intersection.has_value() );

    // Segments with overlapping bounding boxes but no intersection
    SEG seg3( { 0, 0 }, { 10, 0 } );
    SEG seg4( { 5, 5 }, { 15, 5 } );

    auto intersection2 = seg3.Intersect( seg4, false, false );
    BOOST_CHECK( !intersection2.has_value() );

    // Segments with touching bounding boxes and actual intersection
    SEG seg5( { 0, 0 }, { 10, 10 } );
    SEG seg6( { 10, 0 }, { 0, 10 } );

    auto intersection3 = seg5.Intersect( seg6, false, false );
    BOOST_CHECK( intersection3.has_value() );
    BOOST_CHECK_EQUAL( *intersection3, VECTOR2I( 5, 5 ) );
}

BOOST_AUTO_TEST_CASE( IntersectLineVsSegmentMode )
{
    // Test the difference between line mode and segment mode

    SEG seg1( { 0, 0 }, { 5, 0 } );
    SEG seg2( { 10, -5 }, { 10, 5 } );

    // In segment mode, these don't intersect
    auto segmentIntersect = seg1.Intersect( seg2, false, false );
    BOOST_CHECK( !segmentIntersect.has_value() );

    // In line mode, they should intersect
    auto lineIntersect = seg1.Intersect( seg2, false, true );
    BOOST_CHECK( lineIntersect.has_value() );
    BOOST_CHECK_EQUAL( *lineIntersect, VECTOR2I( 10, 0 ) );

    // Test collinear case in line mode
    SEG seg3( { 0, 0 }, { 10, 0 } );
    SEG seg4( { 20, 0 }, { 30, 0 } );

    // Segments don't intersect
    auto segmentIntersect2 = seg3.Intersect( seg4, false, false );
    BOOST_CHECK( !segmentIntersect2.has_value() );

    // Lines (infinite) do intersect (collinear)
    auto lineIntersect2 = seg3.Intersect( seg4, false, true );
    BOOST_CHECK( lineIntersect2.has_value() );
}

BOOST_AUTO_TEST_CASE( IntersectNumericalStability )
{
    // Test cases designed to stress numerical precision

    // Very small segments
    SEG seg1( { 0, 0 }, { 1, 1 } );
    SEG seg2( { 0, 1 }, { 1, 0 } );

    auto intersection = seg1.Intersect( seg2, false, false );

    BOOST_CHECK( intersection.has_value() );
    // Intersection should be very close to (0.5, 0.5), rounded to (0,0), (0,1), (1,0), or (1,1)
    BOOST_CHECK( intersection->x >= 0 && intersection->x <= 1 );
    BOOST_CHECK( intersection->y >= 0 && intersection->y <= 1 );

    // Nearly parallel segments
    SEG seg3( { 0, 0 }, { 1000, 1 } );
    SEG seg4( { 0, 1 }, { 1000, 2 } );

    auto intersection2 = seg3.Intersect( seg4, false, false );
    BOOST_CHECK( !intersection2.has_value() ); // Should be detected as parallel/non-intersecting

    // Segments that intersect at a very acute angle
    SEG seg5( { 0, 0 }, { 1000000, 1 } );
    SEG seg6( { 500000, -1 }, { 500000, 2 } );

    auto intersection3 = seg5.Intersect( seg6, false, false );
    BOOST_CHECK( intersection3.has_value() );
    BOOST_CHECK_EQUAL( intersection3->x, 500000 );
}

BOOST_AUTO_TEST_CASE( IntersectZeroLengthSegments )
{
    // Comprehensive tests for zero-length segments (points)

    VECTOR2I point1( 5, 5 );
    VECTOR2I point2( 10, 10 );

    SEG pointSeg1( point1, point1 );  // Zero-length segment (point)
    SEG pointSeg2( point2, point2 );  // Another zero-length segment
    SEG normalSeg( { 0, 5 }, { 10, 5 } );  // Normal segment

    // Point intersecting with normal segment
    auto intersection1 = pointSeg1.Intersect( normalSeg, false, false );
    BOOST_CHECK( intersection1.has_value() );
    BOOST_CHECK_EQUAL( *intersection1, point1 );

    // Point not intersecting with normal segment
    auto intersection2 = pointSeg2.Intersect( normalSeg, false, false );
    BOOST_CHECK( !intersection2.has_value() );

    // Two points at same location
    SEG pointSeg3( point1, point1 );
    auto intersection3 = pointSeg1.Intersect( pointSeg3, false, false );
    BOOST_CHECK( intersection3.has_value() );
    BOOST_CHECK_EQUAL( *intersection3, point1 );

    // Two points at different locations
    auto intersection4 = pointSeg1.Intersect( pointSeg2, false, false );
    BOOST_CHECK( !intersection4.has_value() );

    // Point on line (infinite mode)
    SEG lineSeg( { 0, 0 }, { 1, 1 } );  // Diagonal line segment
    SEG pointOnLine( { 100, 100 }, { 100, 100 } );  // Point on extended line

    auto intersection5 = pointOnLine.Intersect( lineSeg, false, false );
    BOOST_CHECK( !intersection5.has_value() );  // Point not on segment

    auto intersection6 = pointOnLine.Intersect( lineSeg, false, true );
    BOOST_CHECK( intersection6.has_value() );  // Point on infinite line
    BOOST_CHECK_EQUAL( *intersection6, VECTOR2I( 100, 100 ) );
}


/**
 * Test cases for segment-line intersection
 */
struct SEG_LINE_INTERSECT_CASE : public KI_TEST::NAMED_CASE
{
    SEG         m_seg;
    double      m_slope;
    double      m_offset;
    bool        m_exp_intersect;
    VECTOR2I    m_exp_point;
};

/**
 * Predicate to check expected intersection between a segment and an infinite line
 * @param  aSeg the segment
 * @param  aSlope the line slope
 * @param  aOffset the line y-intercept
 * @param  aExpIntersect expected intersection result
 * @param  aExpPoint expected intersection point (if intersection occurs)
 * @return does the intersection calculated agree?
 */
bool SegLineIntersectCorrect( const SEG& aSeg, double aSlope, double aOffset,
                             bool aExpIntersect, const VECTOR2I& aExpPoint = VECTOR2I() )
{
    VECTOR2I intersection;
    const bool intersects = aSeg.IntersectsLine( aSlope, aOffset, intersection );

    bool ok = ( intersects == aExpIntersect );

    if( !ok )
    {
        std::stringstream ss;
        ss << "Line intersection incorrect: expected " << aExpIntersect << ", got " << intersects;
        BOOST_TEST_INFO( ss.str() );
    }

    // Check intersection point if intersection was expected
    if( ok && aExpIntersect && aExpPoint != VECTOR2I() )
    {
        // Allow some tolerance for intersection point calculation
        const int tolerance = 1;

        bool pointOk = ( std::abs( intersection.x - aExpPoint.x ) <= tolerance &&
                        std::abs( intersection.y - aExpPoint.y ) <= tolerance );

        if( !pointOk )
        {
            std::stringstream ss;
            ss << "Intersection point incorrect: expected " << aExpPoint.Format()
               << ", got " << intersection.Format();
            BOOST_TEST_INFO( ss.str() );
            ok = false;
        }
    }

    return ok;
}

// clang-format off
static const std::vector<SEG_LINE_INTERSECT_CASE> seg_line_intersect_cases = {
    // Basic intersection cases
    {
        "Horizontal segment, diagonal line",
        { { 0, 5 }, { 10, 5 } },
        1.0, 0.0,  // y = x
        true,
        { 5, 5 }
    },
    {
        "Vertical segment, horizontal line",
        { { 5, 0 }, { 5, 10 } },
        0.0, 3.0,  // y = 3
        true,
        { 5, 3 }
    },
    {
        "Diagonal segment, horizontal line crossing",
        { { 0, 0 }, { 10, 10 } },
        0.0, 5.0,  // y = 5
        true,
        { 5, 5 }
    },
    {
        "Diagonal segment, vertical line (steep slope)",
        { { 0, 0 }, { 10, 10 } },
        1000.0, -5000.0,  // Very steep line: y = 1000x - 5000, crosses at x=5
        true,
        { 5, 5 }
    },

    // Non-intersecting cases
    {
        "Horizontal segment, parallel horizontal line",
        { { 0, 5 }, { 10, 5 } },
        0.0, 10.0,  // y = 10 (parallel to y = 5)
        false,
        { 0, 0 }
    },
    {
        "Diagonal segment, parallel line",
        { { 0, 0 }, { 10, 10 } },
        1.0, 5.0,  // y = x + 5 (parallel to y = x)
        false,
        { 0, 0 }
    },
    {
        "Segment above line",
        { { 0, 10 }, { 10, 10 } },
        0.0, 5.0,  // y = 5
        false,
        { 0, 0 }
    },
    {
        "Segment to left of steep line",
        { { 0, 0 }, { 2, 2 } },
        1.0, 10.0,  // y = x + 10
        false,
        { 0, 0 }
    },

    // Collinear cases (segment lies on line)
    {
        "Horizontal segment on horizontal line",
        { { 0, 5 }, { 10, 5 } },
        0.0, 5.0,  // y = 5
        true,
        { 5, 5 }  // Midpoint
    },
    {
        "Diagonal segment on diagonal line",
        { { 0, 0 }, { 10, 10 } },
        1.0, 0.0,  // y = x
        true,
        { 5, 5 }  // Midpoint
    },
    {
        "Vertical segment, any line slope (collinear impossible)",
        { { 5, 0 }, { 5, 10 } },
        2.0, -5.0,  // y = 2x - 5, passes through (5, 5)
        true,
        { 5, 5 }
    },

    // Edge cases
    {
        "Zero-length segment (point) on line",
        { { 3, 7 }, { 3, 7 } },
        2.0, 1.0,  // y = 2x + 1, point (3,7) should be on this line
        true,
        { 3, 7 }
    },
    {
        "Zero-length segment (point) not on line",
        { { 3, 5 }, { 3, 5 } },
        2.0, 1.0,  // y = 2x + 1, point (3,5) not on line (should be y=7)
        false,
        { 0, 0 }
    },
    {
        "Line with zero slope (horizontal)",
        { { 0, 0 }, { 10, 5 } },
        0.0, 2.5,  // y = 2.5
        true,
        { 5, 2 }  // Intersection at x=5, y=2.5 rounded to y=2 or 3
    },
    {
        "Very steep positive slope",
        { { 0, 0 }, { 10, 1 } },
        100.0, -250.0,  // y = 100x - 250, intersects at x=2.5
        true,
        { 2, 0 }  // Approximately (2.5, 0)
    },
    {
        "Very steep negative slope",
        { { 0, 0 }, { 10, 10 } },
        -100.0, 505.0,  // y = -100x + 505, intersects at x=5.05, y≈0
        true,
        { 5, 5 }  // Approximately (5.05, 0) but segment has y=5 at x=5
    },
    {
        "Fractional slope",
        { { 0, 0 }, { 12, 8 } },
        0.5, 1.0,  // y = 0.5x + 1
        true,
        { 6, 4 }  // Intersection where segment y = 2x/3 meets line y = 0.5x + 1
    },

    // Endpoint intersections
    {
        "Line passes through segment start point",
        { { 2, 3 }, { 80, 90 } },
        1.0, 1.0,  // y = x + 1, passes through (2,3)
        true,
        { 2, 3 }
    },
    {
        "Line passes through segment end point",
        { { 20, 30 }, { 8, 9 } },
        1.0, 1.0,  // y = x + 1, passes through (8,9)
        true,
        { 8, 9 }
    },
    {
        "Line intersects near endpoint",
        { { 0, 0 }, { 10, 0 } },
        0.0, 0.0,  // y = 0, same as segment
        true,
        { 5, 0 }  // Collinear, returns midpoint
    },

    // Precision edge cases
    {
        "Nearly parallel lines",
        { { 0, 0 }, { 1000, 1 } },
        0.0011, -0.05,  // Very slightly different slope
        true,
        { 500, 1 }  // At 500, y will round up to 1 in both cases
    },
    {
        "Line intersection outside segment bounds",
        { { 5, 5 }, { 10, 10 } },
        1.0, -10.0,  // y = x - 10, would intersect extended line at (15, 5)
        false,
        { 0, 0 }
    },
};
// clang-format on

BOOST_DATA_TEST_CASE( SegLineIntersection, boost::unit_test::data::make( seg_line_intersect_cases ), c )
{
    BOOST_CHECK_PREDICATE( SegLineIntersectCorrect, ( c.m_seg )( c.m_slope )( c.m_offset )( c.m_exp_intersect )( c.m_exp_point ) );
}

// Additional focused test cases for specific scenarios
BOOST_AUTO_TEST_CASE( IntersectLineVerticalSegments )
{
    // Test vertical segments with various line slopes
    SEG verticalSeg( { 5, 0 }, { 5, 10 } );
    VECTOR2I intersection;

    // Horizontal line intersecting vertical segment
    bool intersects1 = verticalSeg.IntersectsLine( 0.0, 7.0, intersection );
    BOOST_CHECK( intersects1 );
    BOOST_CHECK_EQUAL( intersection, VECTOR2I( 5, 7 ) );

    // Diagonal line intersecting vertical segment
    bool intersects2 = verticalSeg.IntersectsLine( 2.0, -5.0, intersection );  // y = 2x - 5
    BOOST_CHECK( intersects2 );
    BOOST_CHECK_EQUAL( intersection, VECTOR2I( 5, 5 ) );  // At x=5: y = 2*5 - 5 = 5

    // Line that misses vertical segment
    bool intersects3 = verticalSeg.IntersectsLine( 1.0, 20.0, intersection );  // y = x + 20
    BOOST_CHECK( !intersects3 );
}

BOOST_AUTO_TEST_CASE( IntersectLineVerticalSegmentsCorrection )
{
    // Corrected test for vertical segments
    SEG verticalSeg( { 5, 0 }, { 5, 10 } );
    VECTOR2I intersection;

    // Line that misses vertical segment (intersection outside y-range)
    bool intersects1 = verticalSeg.IntersectsLine( 1.0, 20.0, intersection );  // y = x + 20
    BOOST_CHECK( !intersects1 );  // At x=5: y = 25, which is outside [0,10]

    // Line that intersects within segment bounds
    bool intersects2 = verticalSeg.IntersectsLine( 0.5, 2.0, intersection );  // y = 0.5x + 2
    BOOST_CHECK( intersects2 );
    BOOST_CHECK_EQUAL( intersection, VECTOR2I( 5, 5 ) );  // At x=5: y = 0.5*5 + 2 = 4.5 ≈ 5 (round up)
}

BOOST_AUTO_TEST_CASE( IntersectLineParallelDetection )
{
    // Test parallel line detection using cross products

    // Horizontal segment with horizontal line
    SEG horizontalSeg( { 0, 5 }, { 10, 5 } );
    VECTOR2I intersection;

    // Parallel but not collinear
    bool intersects1 = horizontalSeg.IntersectsLine( 0.0, 8.0, intersection );  // y = 8
    BOOST_CHECK( !intersects1 );

    // Collinear (segment lies on line)
    bool intersects2 = horizontalSeg.IntersectsLine( 0.0, 5.0, intersection );  // y = 5
    BOOST_CHECK( intersects2 );
    BOOST_CHECK_EQUAL( intersection, VECTOR2I( 5, 5 ) );  // Midpoint

    // Diagonal segment with parallel line
    SEG diagonalSeg( { 0, 0 }, { 10, 10 } );

    // Parallel but offset
    bool intersects3 = diagonalSeg.IntersectsLine( 1.0, 3.0, intersection );  // y = x + 3
    BOOST_CHECK( !intersects3 );

    // Collinear
    bool intersects4 = diagonalSeg.IntersectsLine( 1.0, 0.0, intersection );  // y = x
    BOOST_CHECK( intersects4 );
    BOOST_CHECK_EQUAL( intersection, VECTOR2I( 5, 5 ) );  // Midpoint
}

BOOST_AUTO_TEST_CASE( IntersectLinePrecisionEdgeCases )
{
    // Test precision-sensitive cases

    // Very shallow segment with steep line
    SEG shallowSeg( { 0, 100 }, { 1000000, 101 } );  // Almost horizontal
    VECTOR2I intersection;

    bool intersects = shallowSeg.IntersectsLine( 1000.0, -499900.0, intersection );
    // Line: y = 1000x - 499900
    // This should intersect around x = 500, y ≈ 100.001

    if( intersects )
    {
        BOOST_CHECK( intersection.x >= 0 && intersection.x <= 1000000 );
        BOOST_CHECK( intersection.y >= 100 && intersection.y <= 101 );
    }

    // Test with very large coordinates
    SEG largeSeg( { 1000000, 1000000 }, { 2000000, 2000000 } );
    bool intersects2 = largeSeg.IntersectsLine( 1.0, 0.0, intersection );  // y = x
    BOOST_CHECK( intersects2 );
    BOOST_CHECK_EQUAL( intersection, VECTOR2I( 1500000, 1500000 ) );  // Midpoint
}

BOOST_AUTO_TEST_CASE( IntersectLineZeroLengthSegments )
{
    // Test with zero-length segments (points)

    VECTOR2I point( 10, 20 );
    SEG pointSeg( point, point );
    VECTOR2I intersection;

    // Point lies on line
    bool intersects1 = pointSeg.IntersectsLine( 2.0, 0.0, intersection );  // y = 2x
    BOOST_CHECK( intersects1 );  // Point (10, 20) is on line y = 2x
    BOOST_CHECK_EQUAL( intersection, point );

    // Point does not lie on line
    bool intersects2 = pointSeg.IntersectsLine( 3.0, 0.0, intersection );  // y = 3x
    BOOST_CHECK( !intersects2 );  // Point (10, 20) not on line y = 3x (would be y = 30)

    // Point on horizontal line
    bool intersects3 = pointSeg.IntersectsLine( 0.0, 20.0, intersection );  // y = 20
    BOOST_CHECK( intersects3 );
    BOOST_CHECK_EQUAL( intersection, point );
}

BOOST_AUTO_TEST_SUITE_END()
