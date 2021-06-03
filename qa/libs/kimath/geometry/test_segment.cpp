/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <geometry/seg.h>

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

struct SEG_SEG_DISTANCE_CASE
{
    std::string m_case_name;
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
};
// clang-format on


BOOST_AUTO_TEST_CASE( SegSegDistance )
{
    for( const auto& c : seg_seg_dist_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            BOOST_CHECK_PREDICATE( SegDistanceCorrect, ( c.m_seg_a )( c.m_seg_b )( c.m_exp_dist ) );
        }
    }
}


struct SEG_VECTOR_DISTANCE_CASE
{
    std::string m_case_name;
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
        283, // sqrt(200^2 + 200^2) = 282.8, rounded to nearest
    },
};
// clang-format on


BOOST_AUTO_TEST_CASE( SegVecDistance )
{
    for( const auto& c : seg_vec_dist_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            BOOST_CHECK_PREDICATE( SegVecDistanceCorrect, ( c.m_seg )( c.m_vec )( c.m_exp_dist ) );
        }
    }
}


/**
 * Test cases for collisions (with clearance, for no clearance,
 * it's just a SEG_SEG_DISTANCE_CASE of 0)
 */
struct SEG_SEG_COLLIDE_CASE
{
    std::string m_case_name;
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
};
// clang-format on


BOOST_AUTO_TEST_CASE( SegSegCollision )
{
    for( const auto& c : seg_seg_coll_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            BOOST_CHECK_PREDICATE( SegCollideCorrect,
                    ( c.m_seg_a )( c.m_seg_b )( c.m_clearance )( c.m_exp_coll ) );
        }
    }
}


/**
 * Struct to hold general cases for collinearity, paralellism and perpendicularity
 */
struct SEG_SEG_BOOLEAN_CASE
{
    std::string m_case_name;
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


BOOST_AUTO_TEST_CASE( SegSegCollinear )
{
    for( const auto& c : seg_vec_collinear_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            BOOST_CHECK_PREDICATE( SegCollinearCorrect,
                                   ( c.m_seg_a )( c.m_seg_b )( c.m_exp_result ) );
        }
    }
}


// clang-format off
/**
 * Test cases for paralellism
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


BOOST_AUTO_TEST_CASE( SegSegParallel )
{
    for( const auto& c : seg_vec_parallel_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            BOOST_CHECK_PREDICATE( SegParallelCorrect,
                                   ( c.m_seg_a )( c.m_seg_b )( c.m_exp_result ) );
        }
    }
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


BOOST_AUTO_TEST_CASE( SegSegPerpendicular )
{
    for( const auto& c : seg_vec_perpendicular_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            BOOST_CHECK_PREDICATE( SegPerpendicularCorrect,
                                   ( c.m_seg_a )( c.m_seg_b )( c.m_exp_result ) );
        }
    }
}


/**
 * Struct to hold cases for operations with a #SEG, and a #VECTOR2I
 */
struct SEG_VEC_CASE
{
    std::string m_case_name;
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


BOOST_AUTO_TEST_CASE( SegCreateParallel )
{
    for( const auto& c : segment_and_point_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            SEG perpendicular = c.m_seg.ParallelSeg( c.m_vec );

            BOOST_CHECK_PREDICATE( SegParallelCorrect, ( perpendicular )( c.m_seg )( true ) );
            BOOST_CHECK_PREDICATE( SegVecDistanceCorrect, ( perpendicular )( c.m_vec )( 0 ) );
        }
    }
}

BOOST_AUTO_TEST_CASE( SegCreatePerpendicular )
{
    for( const auto& c : segment_and_point_cases )
    {
        BOOST_TEST_CONTEXT( c.m_case_name )
        {
            SEG perpendicular = c.m_seg.PerpendicularSeg( c.m_vec );

            BOOST_CHECK_PREDICATE( SegPerpendicularCorrect, ( perpendicular )( c.m_seg )( true ) );
            BOOST_CHECK_PREDICATE( SegVecDistanceCorrect, ( perpendicular )( c.m_vec )( 0 ) );
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
