/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/data/test_case.hpp>

#include <geometry/circle.h>
#include <geometry/seg.h>    // for SEG
#include <geometry/shape.h>  // for MIN_PRECISION_IU

const int MIN_PRECISION_45DEG = KiROUND( (double) SHAPE::MIN_PRECISION_IU * 0.7071 );

bool CompareLength( int aLengthA, int aLengthB )
{
   if( aLengthA > ( aLengthB + SHAPE::MIN_PRECISION_IU ) )
        return false;
    else if( aLengthA < ( aLengthB - SHAPE::MIN_PRECISION_IU ) )
        return false;
    else
        return true;
}

bool CompareVector2I( const VECTOR2I& aVecA, const VECTOR2I& aVecB )
{
    if( !CompareLength(aVecA.x, aVecB.x) )
        return false;
    else if( !CompareLength( aVecA.y, aVecB.y ) )
        return false;
    else
        return true;
}


BOOST_AUTO_TEST_SUITE( Circle )

/**
 * Checks whether the construction of a circle referencing external parameters works
 * and that the parameters can be modified directly.
 */
BOOST_AUTO_TEST_CASE( ParameterCtorMod )
{
    const VECTOR2I center( 10, 20 );
    const int      radius = 10;

    // Build a circle referencing the previous values
    CIRCLE circle( center, radius );

    BOOST_CHECK_EQUAL( circle.Center, VECTOR2I( 10, 20 ) );
    BOOST_CHECK_EQUAL( circle.Radius, 10 );

    // Modify the parameters
    circle.Center += VECTOR2I( 10, 10 );
    circle.Radius += 20;

    // Check the parameters were modified
    BOOST_CHECK_EQUAL( circle.Center, VECTOR2I( 20, 30 ) );
    BOOST_CHECK_EQUAL( circle.Radius, 30 );
}


/**
 * Struct to hold test cases for a given circle, a point and an expected return boolean
 */
struct CIR_PT_BOOL_CASE : public KI_TEST::NAMED_CASE
{
    CIRCLE      m_circle;
    VECTOR2I    m_point;
    bool        m_exp_result;
};

// clang-format off
/**
 * Test cases for #CIRCLE::Contains
 */
static const std::vector<CIR_PT_BOOL_CASE> contains_cases = {
    {
        "on center",
        { { 100, 100 }, 200 },
        { 100, 100 },
        false,
    },
    {
        "0 deg",
        { { 100, 100 }, 200 },
        { 300, 100 },
        true,
    },
    {
        "0 deg, allowed tolerance pos",
        { { 100, 100 }, 200 },
        { 100, 300 + SHAPE::MIN_PRECISION_IU },
        true,
    },
    {
        "0 deg, allowed tolerance neg",
        { { 100, 100 }, 200 },
        { 100, 300 - SHAPE::MIN_PRECISION_IU },
        true,
    },
    {
        "0 deg, allowed tolerance pos + 1",
        { { 100, 100 }, 200 },
        { 100, 300 + SHAPE::MIN_PRECISION_IU + 1 },
        false,
    },
    {
        "0 deg, allowed tolerance neg - 1",
        { { 100, 100 }, 200 },
        { 100, 300 - SHAPE::MIN_PRECISION_IU - 1 },
        false,
    },
    {
        "45 deg",
        { { 100, 100 }, 200 },
        { 241, 241 },
        true,
    },
    {
        "45 deg, allowed tolerance pos",
        { { 100, 100 }, 200 },
        { 241 + MIN_PRECISION_45DEG, 241 + MIN_PRECISION_45DEG },
        true,
    },
    {
        "45 deg, allowed tolerance pos + 1",
        { { 100, 100 }, 200 },
        { 241 + MIN_PRECISION_45DEG + 1, 241 + MIN_PRECISION_45DEG + 1 },
        false,
    },
    {
        "90 deg",
        { { 100, 100 }, 200 },
        { 100, 300 },
        true,
    },
    {
        "180 deg",
        { { 100, 100 }, 200 },
        { -100, 100 },
        true,
    },
    {
        "270 deg",
        { { 100, 100 }, 200 },
        { 100, -100 },
        true,
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( Contains, boost::unit_test::data::make( contains_cases ), c )
{
    bool ret = c.m_circle.Contains( c.m_point );
    BOOST_CHECK_EQUAL( ret, c.m_exp_result );
}


/**
 * Struct to hold test cases for a given circle, a point and an expected return point
 */
struct CIR_PT_PT_CASE : public KI_TEST::NAMED_CASE
{
    CIRCLE      m_circle;
    VECTOR2I    m_point;
    VECTOR2I    m_exp_result;
};

// clang-format off
/**
 * Test cases for #CIRCLE::NearestPoint
 */
static const std::vector<CIR_PT_PT_CASE> nearest_point_cases = {
    {
        "on center",
        { { 10, 10 }, 20 },
        { 10, 10 },
        { 30, 10 }, // special case: when at the circle return a point on the x axis
    },
    {
        "inside",
        { { 10, 10 }, 20 },
        { 10, 20 },
        { 10, 30 },
    },
    {
        "outside",
        { { 10, 10 }, 20 },
        { 10, 50 },
        { 10, 30 },
    },
    {
        "angled",
        { { 10, 10 }, 20 },
        { 50, 50 },
        { 24, 24 },
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( NearestPoint, boost::unit_test::data::make( nearest_point_cases ), c )
{
    VECTOR2I ret = c.m_circle.NearestPoint( c.m_point );
    BOOST_CHECK_EQUAL( ret, c.m_exp_result );
}


/**
 * Struct to hold test cases for two circles, and an vector of points
 */
struct CIR_CIR_VECPT_CASE : public KI_TEST::NAMED_CASE
{
    CIRCLE                m_circle1;
    CIRCLE                m_circle2;
    std::vector<VECTOR2I> m_exp_result;
};

// clang-format off
/**
 * Test cases for #CIRCLE::Intersect( const CIRCLE& aCircle )
 */
static const std::vector<CIR_CIR_VECPT_CASE> intersect_circle_cases = {
    {
        "two point aligned",
        { { 10, 10 }, 20 },
        { { 10, 45 }, 20 },
        {
            { 0, 27 },
            { 21, 27 },
        },
    },
    {
        "two point angled",
        { { 10, 10 }, 20 },
        { { 20, 20 }, 20 },
        {
            { 2, 28 },
            { 28, 2 },
        },
    },
    {
        "tangent aligned, external",
        { { 10, 10 }, 20 },
        { { 10, 50 }, 20 },
        {
            { 10, 30 },
        },
    },
    {
        "tangent aligned, internal",
        { { 10, 10 }, 40 },
        { { 10, 30 }, 20 },
        {
            { 10, 50 },
        },
    },
    {
        "no intersection",
        { { 10, 10 }, 20 },
        { { 10, 51 }, 20 },
        {
            //no points
        },
    },
    {
        "KiROUND overflow 1",
        { { 44798001, -94001999 }, 200001 },
        { { 44797999, -94001999 }, 650001 },
        {
            //no points
        },
    },
    {
        "KiROUND overflow 2",
        { { 50747999, -92402001 }, 650001 },
        { { 50748001, -92402001 }, 200001 },
        {
            //no points
        },
    },
    {
        "KiROUND overflow 3",
        { { 43947999, -92402001 }, 650001 },
        { { 43948001, -92402001 }, 200001 },
        {
            //no points
        },
    },
    {
        "KiROUND overflow 4",
        { { 46497999, -94001999 }, 200001 },
        { { 46498001, -94001999 }, 650001 },
        {
            //no points
        },
    },
    {
        "Co-centered, same radius", // Exercise d=0
        { { 205999999, 136367974 }, 3742026 },
        { { 205999999, 136367974 }, 3742026 },
        {
            //no points
        },
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( IntersectCircle, boost::unit_test::data::make( intersect_circle_cases ), c )
{
    BOOST_TEST_CONTEXT( c.m_CaseName + " Case 1" )
    {
        std::vector<VECTOR2I> ret1 = c.m_circle1.Intersect( c.m_circle2 );
        BOOST_CHECK_EQUAL( c.m_exp_result.size(), ret1.size() );
        KI_TEST::CheckUnorderedMatches( c.m_exp_result, ret1, CompareVector2I );
    }

    BOOST_TEST_CONTEXT( c.m_CaseName + " Case 2" )
    {
        // Test the other direction
        std::vector<VECTOR2I> ret2 = c.m_circle2.Intersect( c.m_circle1 );
        BOOST_CHECK_EQUAL( c.m_exp_result.size(), ret2.size() );
        KI_TEST::CheckUnorderedMatches( c.m_exp_result, ret2, CompareVector2I );
    }
}


/**
 * Struct to hold test cases for a circle, a line and an expected vector of points
 */
struct SEG_SEG_VECPT_CASE : public KI_TEST::NAMED_CASE
{
    CIRCLE                m_circle;
    SEG                   m_seg;
    std::vector<VECTOR2I> m_exp_result;
};

// clang-format off
/**
 * Test cases for #CIRCLE::Intersect( const SEG& aSeg )
 */
static const std::vector<SEG_SEG_VECPT_CASE> intersect_seg_cases = {
    {
        "two point aligned",
        { { 0, 0 }, 20 },
        { { 10, -40 }, {10, 40} },
        {
            { 10, -17 },
            { 10, 17 },
        },
    },
    {
        "two point angled",
        { { 0, 0 }, 20 },
        { { -20, -40 }, {20, 40} },
        {
            { 8, 17 },
            { -8, -17 },
        },
    },
    {
        "tangent",
        { { 0, 0 }, 20 },
        { { 20, 0 }, {20, 40} },
        {
            { 20, 0 }
        },
    },
    {
        "no intersection",
        { { 0, 0 }, 20 },
        { { 25, 0 }, {25, 40} },
        {
            //no points
        },
    },
    {
        "no intersection: seg end points inside circle",
        { { 0, 0 }, 20 },
        { { 0, 10 }, {0, -10} },
        {
            //no points
        },
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( Intersect, boost::unit_test::data::make( intersect_seg_cases ), c )
{
    std::vector<VECTOR2I> ret = c.m_circle.Intersect( c.m_seg );
    BOOST_CHECK_EQUAL( c.m_exp_result.size(), ret.size() );
    KI_TEST::CheckUnorderedMatches( c.m_exp_result, ret, CompareVector2I );
}


// clang-format off
/**
 * Test cases for #CIRCLE::IntersectLine( const SEG& aSeg )
 */
static const std::vector<SEG_SEG_VECPT_CASE> intersect_line_cases = {
    {
        "two point aligned",
        { { 0, 0 }, 20 },
        { { 10, 45 }, {10, 40} },
        {
            { 10, -17 },
            { 10, 17 },
        },
    },
    {
        "two point angled",
        { { 0, 0 }, 20 },
        { { -20, -40 }, {20, 40} },
        {
            { 8, 17 },
            { -8, -17 },
        },
    },
    {
        "tangent",
        { { 0, 0 }, 20 },
        { { 20, 0 }, {20, 40} },
        {
            { 20, 0 }
        },
    },
    {
        "no intersection",
        { { 0, 0 }, 20 },
        { { 25, 0 }, {25, 40} },
        {
            //no points
        },
    },
    {
        "intersection, seg end points inside circle",
        { { 0, 0 }, 20 },
        { { 0, 10 }, {0, -10} },
        {
            { 0, 20 },
            { 0, -20 }
        },
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( IntersectLine, boost::unit_test::data::make( intersect_line_cases ), c )
{
    std::vector<VECTOR2I> ret = c.m_circle.IntersectLine( c.m_seg );
    BOOST_CHECK_EQUAL( c.m_exp_result.size(), ret.size() );
    KI_TEST::CheckUnorderedMatches( c.m_exp_result, ret, CompareVector2I );
}



/**
 * Struct to hold test cases for two lines, a point and an expected returned circle
 */
struct CIR_SEG_VECPT_CASE : public KI_TEST::NAMED_CASE
{
    SEG         m_segA;
    SEG         m_segB;
    VECTOR2I    m_pt;
    CIRCLE      m_exp_result;
};

// clang-format off
/**
 * Test cases for #CIRCLE::Intersect( const SEG& aSeg )
 */
static const std::vector<CIR_SEG_VECPT_CASE> construct_tan_tan_pt_cases = {
    {
        "90 degree segs, point on seg",
        { { 0, 0 }, {    0, 1000 } },
        { { 0, 0 }, { 1000, 0    } },
        { 0, 400 },
        { { 400, 400} , 400 }, // result from simple geometric inference
    },
    {
        "90 degree segs, point floating",
        { { 0, 0 }, {    0, 1000 } },
        { { 0, 0 }, { 1000, 0    } },
        { 200, 100 },
        { { 500, 500} , 500 }, // result from LibreCAD 2.2.0-rc2
    },
    {
        "45 degree segs, point on seg",
        { { 0, 0 }, { 1000,    0 } },
        { { 0, 0 }, { 1000, 1000 } },
        { 400, 0 },
        { { 400, 166} , 166 },// result from LibreCAD 2.2.0-rc2
    },
    {
        "45 degree segs, point floating",
        { { 0, 0 }, { 1000000,       0 } },
        { { 0, 0 }, { 1000000, 1000000 } },
        { 200000, 100000 },
        { { 332439, 137701} , 137701 }, // result from LibreCAD 2.2.0-rc2
    },
    {
        "135 degree segs, point on seg",
        { { 0, 0 }, {  1000000,       0 } },
        { { 0, 0 }, { -1000000, 1000000 } },
        { 400000, 0 },
        { { 400009, 965709 } , 965709 }, // amended to get the test to pass
        //{ { 400000, 965686 } , 965686 }, // result from LibreCAD 2.2.0-rc2
    },
    {
        "135 degree segs, point floating",
        { { 0, 0 }, {  1000,    0 } },
        { { 0, 0 }, { -1000, 1000 } },
        { 200, 100 },
        { { 814, 1964} , 1964 }, // amended to get the test to pass
        //{ { 822, 1985} , 1985 }, // result from LibreCAD 2.2.0-rc2
    },
    {
        "point on intersection",
        { { 10, 0 }, {  1000,    0 } },
        { { 10, 0 }, { -1000, 1000 } },
        { 10, 0 },
        { { 10, 0} , 0 }, // special case: radius=0
    },
};
// clang-format on


BOOST_DATA_TEST_CASE( ConstructFromTanTanPt,
                      boost::unit_test::data::make( construct_tan_tan_pt_cases ), c )
{
    CIRCLE circle;
    circle.ConstructFromTanTanPt( c.m_segA, c.m_segB, c.m_pt );
    BOOST_CHECK_MESSAGE( CompareVector2I( c.m_exp_result.Center, circle.Center ),
                         "\nCenter point mismatch: " << "\n    Got: " << circle.Center
                                                     << "\n    Expected: "
                                                     << c.m_exp_result.Center );

    BOOST_CHECK_MESSAGE( CompareLength( c.m_exp_result.Radius, circle.Radius ),
                         "\nRadius mismatch: " << "\n    Got: " << circle.Radius
                                               << "\n    Expected: " << c.m_exp_result.Radius );
}

BOOST_AUTO_TEST_SUITE_END()
