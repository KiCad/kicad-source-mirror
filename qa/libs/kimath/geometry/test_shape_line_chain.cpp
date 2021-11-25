/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * NOTE: Collision of SHAPE_LINE_CHAIN with arcs is tested in test_shape_arc.cpp
 */

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


// Test special case where the last arc in the chain has a shared point with the first arc
BOOST_AUTO_TEST_CASE( ArcWrappingToStart )
{
    // represent a circle with two semicircular arcs
    SHAPE_ARC arc1( VECTOR2I( 100000, 0 ), VECTOR2I( 0, 100000 ), VECTOR2I( -100000, 0 ), 0 );
    SHAPE_ARC arc2( VECTOR2I( -100000, 0 ), VECTOR2I( 0, -100000 ), VECTOR2I( 100000, 0 ), 0 );

    // Start a chain with the two arcs
    SHAPE_LINE_CHAIN chain;
    chain.Append( arc1 );
    chain.Append( arc2 );
    BOOST_CHECK_EQUAL( chain.PointCount(), 13 );
    //BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );

    // OPEN CHAIN
    // Start of the chain is not yet a shared point, so can't be an arc end either
    BOOST_CHECK_EQUAL( chain.IsSharedPt( 0 ), false );
    BOOST_CHECK_EQUAL( chain.IsArcEnd( 0 ), false );
    BOOST_CHECK_EQUAL( chain.IsArcStart( 0 ), true );

    // Index 6 is the shared point between the two arcs in the middle of the chain
    BOOST_CHECK_EQUAL( chain.IsSharedPt( 6 ), true );
    BOOST_CHECK_EQUAL( chain.IsArcEnd( 6 ), true );
    BOOST_CHECK_EQUAL( chain.IsArcStart( 6 ), true );

    // End index is not yet a shared point
    int endIndex = chain.PointCount() - 1;
    BOOST_CHECK_EQUAL( chain.IsSharedPt( endIndex ), false );
    BOOST_CHECK_EQUAL( chain.IsArcEnd( endIndex ), true );
    BOOST_CHECK_EQUAL( chain.IsArcStart( endIndex ), false );

    for( int i = 0; i < chain.PointCount(); i++ )
    {
        BOOST_CHECK_EQUAL( chain.IsPtOnArc( i ), true ); // all points in the chain are arcs
    }

    // CLOSED CHAIN
    chain.SetClosed( true );
    BOOST_CHECK_EQUAL( chain.PointCount(), 12 ); // (-1) should have removed coincident points
    //BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );

    // Start of the chain should be a shared point now, so can't be an arc end either
    BOOST_CHECK_EQUAL( chain.IsSharedPt( 0 ), true );
    BOOST_CHECK_EQUAL( chain.IsArcEnd( 0 ), true );
    BOOST_CHECK_EQUAL( chain.IsArcStart( 0 ), true );

    // Index 6 is the shared point between the two arcs in the middle of the chain
    BOOST_CHECK_EQUAL( chain.IsSharedPt( 6 ), true );
    BOOST_CHECK_EQUAL( chain.IsArcEnd( 6 ), true );
    BOOST_CHECK_EQUAL( chain.IsArcStart( 6 ), true );

    // End index is in the middle of an arc, so not an end point or shared point
    endIndex = chain.PointCount() - 1;
    BOOST_CHECK_EQUAL( chain.IsSharedPt( endIndex ), false );
    BOOST_CHECK_EQUAL( chain.IsArcEnd( endIndex ), false );
    BOOST_CHECK_EQUAL( chain.IsArcStart( endIndex ), false );
}

// Test SHAPE_LINE_CHAIN::Split()
BOOST_AUTO_TEST_CASE( Split )
{
    SEG       seg1( VECTOR2I( 0, 100000 ), VECTOR2I( 50000, 0 ) );
    SEG       seg2( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ) );
    SHAPE_ARC arc( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ), 180.0 );

    // Start a chain with 2 points (seg1)
    SHAPE_LINE_CHAIN chain( { seg1.A, seg1.B } );
    BOOST_CHECK_EQUAL( chain.PointCount(), 2 );
    // Add first arc
    chain.Append( arc );
    BOOST_CHECK_EQUAL( chain.PointCount(), 9 );
    // Add two points (seg2)
    chain.Append( seg2.A );
    chain.Append( seg2.B );
    BOOST_CHECK_EQUAL( chain.PointCount(), 11 );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );

    BOOST_TEST_CONTEXT( "Case 1: Point not in the chain" )
    {
        SHAPE_LINE_CHAIN chainCopy = chain;
        BOOST_CHECK_EQUAL( chainCopy.Split( VECTOR2I( 400000, 0 ) ), -1 );
        BOOST_CHECK_EQUAL( chainCopy.PointCount(), chain.PointCount() );
        BOOST_CHECK_EQUAL( chainCopy.ArcCount(), chain.ArcCount() );
    }

    BOOST_TEST_CONTEXT( "Case 2: Point close to start of a segment" )
    {
        SHAPE_LINE_CHAIN chainCopy = chain;
        VECTOR2I         splitPoint = seg1.A + VECTOR2I( 5, -10 );
        BOOST_CHECK_EQUAL( chainCopy.Split( splitPoint ), 1 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chainCopy ) );
        BOOST_CHECK_EQUAL( chainCopy.GetPoint( 1 ), splitPoint );
        BOOST_CHECK_EQUAL( chainCopy.PointCount(), chain.PointCount() + 1 ); // new point added
        BOOST_CHECK_EQUAL( chainCopy.ArcCount(), chain.ArcCount() );
    }

    BOOST_TEST_CONTEXT( "Case 3: Point exactly on the segment" )
    {
        SHAPE_LINE_CHAIN chainCopy = chain;
        VECTOR2I         splitPoint = seg1.B;
        BOOST_CHECK_EQUAL( chainCopy.Split( splitPoint ), 1 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chainCopy ) );
        BOOST_CHECK_EQUAL( chainCopy.GetPoint( 1 ), splitPoint );
        BOOST_CHECK_EQUAL( chainCopy.PointCount(), chain.PointCount() );
        BOOST_CHECK_EQUAL( chainCopy.ArcCount(), chain.ArcCount() );
    }

    BOOST_TEST_CONTEXT( "Case 4: Point at start of arc" )
    {
        SHAPE_LINE_CHAIN chainCopy = chain;
        VECTOR2I         splitPoint = arc.GetP0();
        BOOST_CHECK_EQUAL( chainCopy.Split( splitPoint ), 2 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chainCopy ) );
        BOOST_CHECK_EQUAL( chainCopy.GetPoint( 2 ), splitPoint );
        BOOST_CHECK_EQUAL( chainCopy.PointCount(), chain.PointCount() );
        BOOST_CHECK_EQUAL( chainCopy.ArcCount(), chain.ArcCount() );
    }

    BOOST_TEST_CONTEXT( "Case 5: Point close to start of arc" )
    {
        SHAPE_LINE_CHAIN chainCopy = chain;
        VECTOR2I         splitPoint = arc.GetP0() + VECTOR2I( -10, 130 );
        BOOST_CHECK_EQUAL( chainCopy.Split( splitPoint ), 3 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chainCopy ) );
        BOOST_CHECK_EQUAL( chainCopy.GetPoint( 3 ), splitPoint );
        BOOST_CHECK_EQUAL( chainCopy.IsSharedPt( 3 ), true ); // must be a shared point
        BOOST_CHECK_EQUAL( chainCopy.PointCount(), chain.PointCount() + 1 ); // new point added
        BOOST_CHECK_EQUAL( chainCopy.ArcCount(), chain.ArcCount() + 1 ); // new arc should have been created
    }
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

    BOOST_TEST_CONTEXT( "Case 5: Chain ends in arc and point" )
    {
        SHAPE_LINE_CHAIN chainCopy = chain;
        chainCopy.Append( VECTOR2I( 400000, 400000 ) );

        SHAPE_LINE_CHAIN sliceResult = chainCopy.Slice( 11, -1 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( -1 ), VECTOR2I( 400000, 400000 ) );
    }
}


// Test SHAPE_LINE_CHAIN::NearestPoint( VECTOR2I )
BOOST_AUTO_TEST_CASE( NearestPointPt )
{
    SEG       seg1( VECTOR2I( 0, 100000 ), VECTOR2I( 50000, 0 ) );
    SEG       seg2( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ) );
    SHAPE_ARC arc( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ), 180.0 );

    // Start a chain with 2 points (seg1)
    SHAPE_LINE_CHAIN chain( { seg1.A, seg1.B } );
    BOOST_CHECK_EQUAL( chain.PointCount(), 2 );
    // Add first arc
    chain.Append( arc );
    BOOST_CHECK_EQUAL( chain.PointCount(), 9 );
    // Add two points (seg2)
    chain.Append( seg2.A );
    chain.Append( seg2.B );
    BOOST_CHECK_EQUAL( chain.PointCount(), 11 );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );

    VECTOR2I ptOnArcCloseToStart( 297553, 31697 ); //should be index 3 in chain
    VECTOR2I ptOnArcCloseToEnd( 139709, 82983 ); //should be index 6 in chain

    BOOST_CHECK_EQUAL( chain.NearestPoint( ptOnArcCloseToStart, true ), ptOnArcCloseToStart );
    BOOST_CHECK_EQUAL( chain.NearestPoint( ptOnArcCloseToStart, false ), arc.GetP0() );

    BOOST_CHECK_EQUAL( chain.NearestPoint( ptOnArcCloseToEnd, true ), ptOnArcCloseToEnd );
    BOOST_CHECK_EQUAL( chain.NearestPoint( ptOnArcCloseToEnd, false ), arc.GetP1() );
}


// Test SHAPE_LINE_CHAIN::Replace( SHAPE_LINE_CHAIN )
BOOST_AUTO_TEST_CASE( ReplaceChain )
{
    BOOST_TEST_INFO( "8949 crash" );

    std::vector<VECTOR2I> linePts = {
        { 206000000, 140110000 }, { 192325020, 140110000 }, { 192325020, 113348216 },
        { 192251784, 113274980 }, { 175548216, 113274980 }, { 175474980, 113348216 },
        { 175474980, 136694980 }, { 160774511, 121994511 }, { 160774511, 121693501 },
        { 160086499, 121005489 }, { 159785489, 121005489 }, { 159594511, 120814511 },
        { 160086499, 120814511 }, { 160774511, 120126499 }, { 160774511, 119153501 },
        { 160086499, 118465489 }, { 159113501, 118465489 }, { 158425489, 119153501 },
        { 158425489, 119645489 }, { 157325020, 118545020 }, { 157325020, 101925020 },
        { 208674980, 101925020 }, { 208674980, 145474980 }, { 192325020, 145474980 },
        { 192325020, 140110000 }
    };

    SHAPE_LINE_CHAIN baseChain( linePts, false );
    baseChain.SetWidth( 250000 );
    BOOST_CHECK_EQUAL( baseChain.PointCount(), linePts.size() );

    SHAPE_LINE_CHAIN replaceChain( { VECTOR2I( 192325020, 140110000 ) }, false );
    BOOST_CHECK_EQUAL( replaceChain.PointCount(), 1 );

    baseChain.Replace( 1, 23, replaceChain );

    BOOST_CHECK_EQUAL( baseChain.PointCount(), linePts.size() - ( 23 - 1 ) );

    // Replacing the last point in a chain is special-cased
    baseChain.Replace( baseChain.PointCount() - 1, baseChain.PointCount() - 1, VECTOR2I( -1, -1 ) );

    BOOST_CHECK_EQUAL( baseChain.CLastPoint(), VECTOR2I( -1, -1 ) );
}


BOOST_AUTO_TEST_SUITE_END()
