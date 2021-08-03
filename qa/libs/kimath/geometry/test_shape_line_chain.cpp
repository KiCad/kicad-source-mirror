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

    // Test ClearArcs
    base_chain.SetClosed( true );
    double areaPriorToArcRemoval = base_chain.Area();
    base_chain.ClearArcs();

    BOOST_CHECK( GEOM_TEST::IsOutlineValid( base_chain ) );
    BOOST_CHECK_EQUAL( base_chain.PointCount(), 11 ); // We should have the same number of points
    BOOST_CHECK_EQUAL( base_chain.ArcCount(), 0 ); // All arcs should have been removed
    BOOST_CHECK_EQUAL( base_chain.Area(), areaPriorToArcRemoval ); // Area should not have changed
}


// Test SHAPE_LINE_CHAIN::Slice()
BOOST_AUTO_TEST_CASE( Slice )
{
    SEG       targetSegment( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ) );
    SHAPE_ARC firstArc( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ), 180.0 );
    SHAPE_ARC secondArc( VECTOR2I( -200000, -200000 ), VECTOR2I( -300000, -100000 ), -180.0 );
    int       tol = SHAPE_ARC::DefaultAccuracyForPCB(); // Tolerance for arc collisions

    // Start a chain with 3 points
    SHAPE_LINE_CHAIN chain( { VECTOR2I( 0, 0 ), VECTOR2I( 0, 100000 ), VECTOR2I( 100000, 0 ) } );
    BOOST_CHECK_EQUAL( chain.PointCount(), 3 );
    // Add first arc
    chain.Append( firstArc );
    BOOST_CHECK_EQUAL( chain.PointCount(), 10 );
    // Add two points (target segment)
    chain.Append( targetSegment.A );
    chain.Append( targetSegment.B );
    BOOST_CHECK_EQUAL( chain.PointCount(), 12 );
    // Add a second arc
    chain.Append( secondArc );
    BOOST_CHECK_EQUAL( chain.PointCount(), 20 );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );

    //////////////////////////////////////////////////////////
    /// CASE 1: Start at arc endpoint, finish middle of arc  /
    //////////////////////////////////////////////////////////
    BOOST_TEST_CONTEXT( "Case 1: Start at arc endpoint, finish middle of arc" )
    {
        SHAPE_LINE_CHAIN sliceResult = chain.Slice( 9, 18 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( sliceResult ) );

        BOOST_CHECK_EQUAL( sliceResult.ArcCount(), 1 );
        SHAPE_ARC sliceArc0 = sliceResult.Arc( 0 );
        BOOST_CHECK_EQUAL( secondArc.GetP0(), sliceArc0.GetP0() ); // equal arc start points
        BOOST_CHECK( secondArc.Collide( sliceArc0.GetArcMid(), tol ) );
        BOOST_CHECK( secondArc.Collide( sliceArc0.GetP1(), tol ) );

        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 10 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ), firstArc.GetP1() ); // equal to arc end
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 1 ), targetSegment.A );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 2 ), targetSegment.B );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 3 ), sliceArc0.GetP0() ); // equal to arc start
        BOOST_CHECK_EQUAL( sliceResult.IsArcStart( 3 ), true );

        for( int i = 4; i <= 8; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcStart( i ), false );

        for( int i = 3; i <= 7; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( i ), false );

        BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( 9 ), true );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 9 ), sliceArc0.GetP1() ); // equal to arc end
    }

    //////////////////////////////////////////////////////////////////
    /// CASE 2: Start at middle of an arc, finish at arc startpoint  /
    //////////////////////////////////////////////////////////////////
    BOOST_TEST_CONTEXT( "Case 2: Start at middle of an arc, finish at arc startpoint" )
    {
        SHAPE_LINE_CHAIN sliceResult = chain.Slice( 5, 12 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( sliceResult ) );

        BOOST_CHECK_EQUAL( sliceResult.ArcCount(), 1 );
        SHAPE_ARC sliceArc0 = sliceResult.Arc( 0 );
        BOOST_CHECK_EQUAL( firstArc.GetP1(), sliceArc0.GetP1() ); // equal arc end points
        BOOST_CHECK( firstArc.Collide( sliceArc0.GetArcMid(), tol ) );
        BOOST_CHECK( firstArc.Collide( sliceArc0.GetP0(), tol ) );

        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 8 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ), sliceArc0.GetP0() );// equal to arc start
        BOOST_CHECK_EQUAL( sliceResult.IsArcStart( 0 ), true );

        for( int i = 1; i <= 4; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcStart( i ), false );

        for( int i = 0; i <= 3; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( i ), false );

        BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( 4 ), true );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 4 ), sliceArc0.GetP1() ); // equal to arc end

        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 5 ), targetSegment.A );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 6 ), targetSegment.B );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 7 ), secondArc.GetP0() );
    }

    //////////////////////////////////////////////////////////////////
    /// CASE 3: Full arc, nothing else                               /
    //////////////////////////////////////////////////////////////////
    BOOST_TEST_CONTEXT( "Case 3: Full arc, nothing else" )
    {
        SHAPE_LINE_CHAIN sliceResult = chain.Slice( 3, 9 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( sliceResult ) );

        BOOST_CHECK_EQUAL( sliceResult.ArcCount(), 1 );
        SHAPE_ARC sliceArc0 = sliceResult.Arc( 0 );

        // Equal arc to original inserted arc
        BOOST_CHECK_EQUAL( firstArc.GetP1(), sliceArc0.GetP1() );
        BOOST_CHECK_EQUAL( firstArc.GetArcMid(), sliceArc0.GetArcMid() );
        BOOST_CHECK_EQUAL( firstArc.GetP1(), sliceArc0.GetP1() );

        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 7 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ), sliceArc0.GetP0() ); // equal to arc start
        BOOST_CHECK_EQUAL( sliceResult.IsArcStart( 0 ), true );

        for( int i = 1; i <= 6; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcStart( i ), false );

        for( int i = 0; i <= 5; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( i ), false );

        BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( 6 ), true );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 6 ), sliceArc0.GetP1() ); // equal to arc end
    }

    //////////////////////////////////////////////////////////////////
    /// CASE 4: Full arc, and straight segments to next arc start    /
    //////////////////////////////////////////////////////////////////
    BOOST_TEST_CONTEXT( "Case 4:  Full arc, and straight segments to next arc start" )
    {
        SHAPE_LINE_CHAIN sliceResult = chain.Slice( 3, 12 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( sliceResult ) );

        BOOST_CHECK_EQUAL( sliceResult.ArcCount(), 1 );
        SHAPE_ARC sliceArc0 = sliceResult.Arc( 0 );

        // Equal arc to original inserted arc
        BOOST_CHECK_EQUAL( firstArc.GetP1(), sliceArc0.GetP1() );
        BOOST_CHECK_EQUAL( firstArc.GetArcMid(), sliceArc0.GetArcMid() );
        BOOST_CHECK_EQUAL( firstArc.GetP1(), sliceArc0.GetP1() );

        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 10 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ), sliceArc0.GetP0() ); // equal to arc start
        BOOST_CHECK_EQUAL( sliceResult.IsArcStart( 0 ), true );

        for( int i = 1; i <= 6; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcStart( i ), false );

        for( int i = 0; i <= 5; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( i ), false );

        BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( 6 ), true );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 6 ), sliceArc0.GetP1() ); // equal to arc end

        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 7 ), targetSegment.A );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 8 ), targetSegment.B );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 9 ), secondArc.GetP0() );
    }
}


BOOST_AUTO_TEST_SUITE_END()
