/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <geometry/shape_arc.h>
#include <geometry/shape_line_chain.h>
#include <trigo.h>

#include <qa_utils/geometry/geometry.h>
#include <qa_utils/numeric.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include "geom_test_utils.h"

BOOST_AUTO_TEST_SUITE( ShapeLineChain )

BOOST_AUTO_TEST_CASE( ArcToPolyline )
{
    SHAPE_LINE_CHAIN base_chain( { VECTOR2I( 0, 0 ), VECTOR2I( 0, 1000 ), VECTOR2I( 1000, 0 ) } );

    SHAPE_LINE_CHAIN chain_insert( {
            VECTOR2I( 0, 1500 ),
            VECTOR2I( 1500, 1500 ),
            VECTOR2I( 1500, 0 ),
    } );

    SHAPE_LINE_CHAIN arc_insert1( SHAPE_ARC( VECTOR2I( 0, -100 ), VECTOR2I( 0, -200 ), 180.0 ) );

    SHAPE_LINE_CHAIN arc_insert2( SHAPE_ARC( VECTOR2I( 0, 500 ), VECTOR2I( 0, 400 ), 180.0 ) );

    BOOST_CHECK_EQUAL( base_chain.CShapes().size(), base_chain.CPoints().size() );
    BOOST_CHECK_EQUAL( arc_insert1.CShapes().size(), arc_insert1.CPoints().size() );
    BOOST_CHECK_EQUAL( arc_insert2.CShapes().size(), arc_insert2.CPoints().size() );

    BOOST_CHECK( GEOM_TEST::IsOutlineValid( base_chain ) );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( arc_insert1 ) );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( arc_insert2 ) );

    base_chain.Insert( 0, SHAPE_ARC( VECTOR2I( 0, -100 ), VECTOR2I( 0, -200 ), 1800 ) );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( base_chain ) );
    BOOST_CHECK_EQUAL( base_chain.CShapes().size(), base_chain.CPoints().size() );

    base_chain.Replace( 0, 2, chain_insert );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( base_chain ) );
    BOOST_CHECK_EQUAL( base_chain.CShapes().size(), base_chain.CPoints().size() );
}


// Similar test to above but with larger coordinates, so we have more than one point per arc
BOOST_AUTO_TEST_CASE( ArcToPolylineLargeCoords )
{
    SHAPE_LINE_CHAIN base_chain( { VECTOR2I( 0, 0 ), VECTOR2I( 0, 100000 ), VECTOR2I( 100000, 0 ) } );

    SHAPE_LINE_CHAIN chain_insert( {
            VECTOR2I( 0, 1500000 ),
            VECTOR2I( 1500000, 1500000 ),
            VECTOR2I( 1500000, 0 ),
    } );

    base_chain.Append( SHAPE_ARC( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 100000 ), 180.0 ) );

    BOOST_CHECK( GEOM_TEST::IsOutlineValid( base_chain ) );
    BOOST_CHECK_EQUAL( base_chain.PointCount(), 11 );

    base_chain.Insert( 9, VECTOR2I( 250000, 0 ) );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( base_chain ) );
    BOOST_CHECK_EQUAL( base_chain.PointCount(), 12 );
    BOOST_CHECK_EQUAL( base_chain.ArcCount(), 2 ); // Should have two arcs after the split

    base_chain.Replace( 5, 6, chain_insert );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( base_chain ) );
    BOOST_CHECK_EQUAL( base_chain.PointCount(), 13 ); // Adding 3 points, removing 2
    BOOST_CHECK_EQUAL( base_chain.ArcCount(), 3 ); // Should have three arcs after the split

    base_chain.Replace( 4, 6, VECTOR2I( 550000, 0 ) );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( base_chain ) );
    BOOST_CHECK_EQUAL( base_chain.PointCount(), 11 ); // Adding 1 point, removing 3
    BOOST_CHECK_EQUAL( base_chain.ArcCount(), 3 );    // Should still have three arcs
}


struct ARC_FOUND_PROPERTIES
{
    VECTOR2I m_Start;
    VECTOR2I m_End;
    VECTOR2I m_Center;
    double   m_Radius;

    ARC_FOUND_PROPERTIES()
    {
        m_Start = { 0, 0 };
        m_End = { 0, 0 };
        m_Center = { 0, 0 };
        m_Radius = 0.0;
    }

    ARC_FOUND_PROPERTIES( SHAPE_ARC aArc )
    {
        m_Start = aArc.GetP0();
        m_End = aArc.GetP1();
        m_Center = aArc.GetCenter();
        m_Radius = aArc.GetRadius();
    };
};


struct DETECT_ARCS_CASE
{
    /// The text context name
    std::string m_ctx_name;

    /// Chain with arcs
    SHAPE_LINE_CHAIN m_chain;

    /// Ars that are used as hints in DetectArcs function
    std::vector<SHAPE_ARC> m_arcs_to_find;

    /// Expected properties of the arcs found in m_chain
    std::vector<ARC_FOUND_PROPERTIES> m_expected_arc_props;
};


static std::vector<DETECT_ARCS_CASE> DetectArcsTestCases( int aTolerance, SHAPE_ARC aArc )
{
    std::vector<DETECT_ARCS_CASE> retval;

    SHAPE_LINE_CHAIN base_chain(
            { VECTOR2I( 0, 0 ), VECTOR2I( 0, 1000000 ), VECTOR2I( 1000000, 0 ) } );
    int       num_pts = base_chain.GetPointCount();
    SHAPE_ARC appended_arc1 = SHAPE_ARC( VECTOR2I( 0, 0 ), VECTOR2I( 0, -200000 ), 45 );
    SHAPE_ARC appended_arc2 = aArc;

    // Add the arcs to the chain as individual line segments, ensuring the chain has no knowledge
    // of the arcs that were added to it.
    SHAPE_LINE_CHAIN poly_arc1 = appended_arc1.ConvertToPolyline( aTolerance );
    SHAPE_LINE_CHAIN poly_arc2 = appended_arc2.ConvertToPolyline( aTolerance );
    base_chain.Append( poly_arc1 );
    base_chain.Append( poly_arc2 );

    wxASSERT_MSG( poly_arc2.PointCount() > 3, "Test arc is too small" );

    // Simple case: Arcs in the chain are as created by ConvertToPolyline
    SHAPE_LINE_CHAIN simple_chain( base_chain );
    retval.push_back( { "Simple case",
                        simple_chain,
                        { appended_arc1, appended_arc2 },
                        { appended_arc1, appended_arc2 } } );

    // Reversed case: Points are reversed
    SHAPE_LINE_CHAIN reversed_chain( base_chain.Reverse() );
    retval.push_back( { "Reversed case",
                        reversed_chain,
                        { appended_arc1, appended_arc2 },
                        { appended_arc2.Reversed(), appended_arc1.Reversed() } } );

    // Complex Case 1: Half of the end points of arc 2 removed
    {
        SHAPE_LINE_CHAIN truncated_chain( base_chain );

        int start_index = truncated_chain.GetPointCount()
                          - std::max( poly_arc2.GetPointCount() / 2, (size_t) 1 );
        int end_index = truncated_chain.GetPointCount() - 1;

        truncated_chain.Remove( start_index, end_index );

        ARC_FOUND_PROPERTIES expected_arc2( appended_arc2 );
        expected_arc2.m_End = truncated_chain.GetPoint( truncated_chain.GetPointCount() - 1 );

        retval.push_back( { "Complex Case 1: End Point Changed",
                            truncated_chain,
                            { appended_arc1, appended_arc2 },
                            { appended_arc1, expected_arc2 } } );
    }

    // Complex Case 2: Half of the START points of arc 2 removed
    {
        SHAPE_LINE_CHAIN truncated_chain( base_chain );

        int start_index = truncated_chain.GetPointCount() - poly_arc2.GetPointCount();
        int end_index = truncated_chain.GetPointCount()
                         - std::max( poly_arc2.GetPointCount() / 2, (size_t) 1 );

        ARC_FOUND_PROPERTIES expected_arc2( appended_arc2 );
        expected_arc2.m_Start = truncated_chain.GetPoint( end_index );

        truncated_chain.Remove( start_index, end_index - 1 );

        retval.push_back( { "Complex Case 2: Start Point Changed",
                            truncated_chain,
                            { appended_arc1, appended_arc2 },
                            { appended_arc1, expected_arc2 } } );
    }

    // Complex Case 3: Start and end points removed
    {
        SHAPE_LINE_CHAIN     truncated_chain( base_chain );

        // Remove some points at the start:
        int start_index = truncated_chain.GetPointCount() - poly_arc2.GetPointCount();
        int end_index = truncated_chain.GetPointCount()
                        - std::max( ( 3 * poly_arc2.GetPointCount() ) / 4, (size_t) 1 );

        ARC_FOUND_PROPERTIES expected_arc2( appended_arc2 );
        expected_arc2.m_Start = truncated_chain.GetPoint( end_index );

        truncated_chain.Remove( start_index, end_index - 1 );

        // Remove some points at the end:
        start_index = truncated_chain.GetPointCount()
                          - std::max( poly_arc2.GetPointCount() / 4, (size_t) 1 );
        end_index = truncated_chain.GetPointCount() - 1;

        truncated_chain.Remove( start_index, end_index );

        expected_arc2.m_End = truncated_chain.GetPoint( truncated_chain.GetPointCount() - 1 );

        retval.push_back( { "Complex Case 3: Start and End Points Changed",
                            truncated_chain,
                            { appended_arc1, appended_arc2 },
                            { appended_arc1, expected_arc2 } } );
    }

    return retval;
}


BOOST_AUTO_TEST_CASE( DetectArcs )
{
    std::vector<int> tolerance_cases = { 10, 50, 100, 500, 1000, 5000 };

    std::vector<std::pair<VECTOR2I, VECTOR2I>> center_start_pts_cases =
    {
        { VECTOR2I( 0, 500000 ), VECTOR2I( 0, 400000 ) },
        { VECTOR2I( 0, 40505 ), VECTOR2I( 45205, 4245 ) },
        { VECTOR2I( 47244, 4005 ), VECTOR2I( 94504, 5550 ) },
        { VECTOR2I( 5000, 0 ), VECTOR2I( 0, 4000 ) }
    };

    std::vector<double> arc_angle_cases = { 15, 45, 90, 135, 180, 225, 270, 305, 360 };

    std::vector<SHAPE_ARC> arc_cases;

    for( std::pair<VECTOR2I, VECTOR2I> center_start : center_start_pts_cases )
    {
        for( double angle : arc_angle_cases )
            arc_cases.emplace_back( center_start.first, center_start.second, angle );
    }

    // SHAPE_ARC does not define arc centre and radius. These are calculated parameters based on the
    // three points of the arc. We want to ensure the centre and radius are somewhat close to the
    // original arc, but accept that there will be rounding errors when calculating.
    const int maxTolDerived = 450;

    for( int& tol : tolerance_cases )
    {
        for( SHAPE_ARC& arc_case : arc_cases )
        {
            std::vector<DETECT_ARCS_CASE> test_cases = DetectArcsTestCases( tol, arc_case );

            for( DETECT_ARCS_CASE& tc : test_cases )
            {
                BOOST_TEST_CONTEXT( tc.m_ctx_name << " -> Tolerance IU: " << tol
                                                  << " -> Arc Tested: { center=" << arc_case.GetCenter()
                                                  << " start=" << arc_case.GetP0()
                                                  << " angle=" << arc_case.GetCentralAngle() << "}" )
                {
                    BOOST_REQUIRE_MESSAGE( tc.m_arcs_to_find.size()
                                                   == tc.m_expected_arc_props.size(),
                                           "\nMalformed test case. m_arcs_to_find.size()"
                                           " should equal m_expected_arc_props.size()." );

                    BOOST_REQUIRE_MESSAGE( tc.m_chain.ArcCount() == 0,
                                           "\nMalformed test case. The SHAPE_LINE_CHAIN to test "
                                           "(m_chain) should have no arcs to start with. I.e. "
                                           "m_chain.ArcCount() should equal 0." );

                    tc.m_chain.DetectArcs( tc.m_arcs_to_find, tol );

                    BOOST_CHECK_EQUAL( tc.m_chain.ArcCount(), tc.m_expected_arc_props.size() );

                    BOOST_REQUIRE_MESSAGE( tc.m_chain.ArcCount() == tc.m_expected_arc_props.size(),
                                           "\nConvertToArcs failed: "
                                           "\n   Expected arcs to be found: "
                                                   << tc.m_expected_arc_props.size()
                                                   << "\n   Actual number found: "
                                                   << tc.m_chain.ArcCount() );
                    int i = 0;
                    int td = maxTolDerived;

                    for( ARC_FOUND_PROPERTIES& exp : tc.m_expected_arc_props )
                    {
                        BOOST_TEST_CONTEXT( "Arc Index: " << i )
                        {
                            BOOST_CHECK_PREDICATE(
                                    KI_TEST::IsVecWithinTol<VECTOR2I>,
                                    ( tc.m_chain.Arc( i ).GetP0() )( exp.m_Start )( tol / 2 ) );
                            BOOST_CHECK_PREDICATE(
                                    KI_TEST::IsVecWithinTol<VECTOR2I>,
                                    ( tc.m_chain.Arc( i ).GetP1() )( exp.m_End )( tol / 2 ) );
                            BOOST_CHECK_PREDICATE(
                                    KI_TEST::IsVecWithinTol<VECTOR2I>,
                                    ( tc.m_chain.Arc( i ).GetCenter() )( exp.m_Center )( td ) );
                            BOOST_CHECK_PREDICATE(
                                    KI_TEST::IsWithin<double>,
                                    ( tc.m_chain.Arc( i ).GetRadius() )( exp.m_Radius )( td ) );

                            ++i;
                        }
                    }
                }
            }
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
