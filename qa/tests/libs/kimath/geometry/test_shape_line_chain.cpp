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

struct SLC_CASES
{
    SHAPE_LINE_CHAIN Circle1Arc;
    SHAPE_LINE_CHAIN Circle2Arcs;
    SHAPE_LINE_CHAIN ArcsCoincident;
    SHAPE_LINE_CHAIN ArcsCoincidentClosed;
    SHAPE_LINE_CHAIN ArcsIndependent;
    SHAPE_LINE_CHAIN DuplicateArcs;
    SHAPE_LINE_CHAIN ArcsAndSegMixed;
    SHAPE_LINE_CHAIN ArcAndPoint;
    SHAPE_LINE_CHAIN SegAndArcCoincident;
    SHAPE_LINE_CHAIN EmptyChain;
    SHAPE_LINE_CHAIN OnePoint;
    SHAPE_LINE_CHAIN TwoPoints;
    SHAPE_LINE_CHAIN ThreePoints;

    SHAPE_ARC ArcCircle; ///< Full Circle arc
    SHAPE_ARC Arc0a; ///< First half of a circle
    SHAPE_ARC Arc0b; ///< Second half of a circle
    SHAPE_ARC Arc1; ///< start coincident with Arc0a end
    SHAPE_ARC Arc2; ///< Independent arc
    SHAPE_ARC Arc3; ///< Arc with angle >180

    SLC_CASES()
    {
        ArcCircle = SHAPE_ARC( VECTOR2I( 183450000, 128360000 ),
                               VECTOR2I( 183850000, 128360000 ),
                               VECTOR2I( 183450000, 128360000 ), 0 );

        Arc0a =     SHAPE_ARC( VECTOR2I( 183450000, 128360000 ),
                               VECTOR2I( 183650000, 128560000 ),
                               VECTOR2I( 183850000, 128360000 ), 0 );

        Arc0b =     SHAPE_ARC( VECTOR2I( 183850000, 128360000 ),
                               VECTOR2I( 183650000, 128160000 ),
                               VECTOR2I( 183450000, 128360000 ), 0 );

        Arc1 =      SHAPE_ARC( VECTOR2I( 183850000, 128360000 ),
                               VECTOR2I( 183638550, 128640305 ),
                               VECTOR2I( 183500000, 129204974 ), 0 );

        Arc2 =      SHAPE_ARC( VECTOR2I( 283450000, 228360000 ),
                               VECTOR2I( 283650000, 228560000 ),
                               VECTOR2I( 283850000, 228360000 ), 0 );

        Arc3 =      SHAPE_ARC( VECTOR2I( 0,         0 ),
                               VECTOR2I( 24142136,  10000000 ),
                               VECTOR2I( 0,         20000000 ), 0 );

        Circle1Arc.Append( ArcCircle );
        Circle1Arc.SetClosed( true );

        Circle2Arcs.Append( Arc0a );
        Circle2Arcs.Append( Arc0b );
        Circle2Arcs.SetClosed( true );

        ArcsCoincident.Append( Arc0a );
        ArcsCoincident.Append( Arc1 );

        ArcsCoincidentClosed=ArcsCoincident;
        ArcsCoincidentClosed.SetClosed( true );

        ArcsIndependent.Append( Arc0a );
        ArcsIndependent.Append( Arc2 );

        DuplicateArcs=ArcsCoincident;
        DuplicateArcs.Append( Arc1 ); //should add a segment between end of the chain and new copy of the arc

        ArcAndPoint.Append( Arc0a );
        ArcAndPoint.Append( VECTOR2I( 233450000, 228360000 ) );

        ArcsAndSegMixed = ArcAndPoint;
        ArcsAndSegMixed.Append( Arc2 );

        OnePoint.Append( VECTOR2I( 233450000, 228360000 ) );

        TwoPoints.Append( VECTOR2I( 233450000, 228360000 ) );
        TwoPoints.Append( VECTOR2I( 263450000, 258360000 ) );

        ThreePoints = TwoPoints;
        ThreePoints.Append( VECTOR2I( 263450000, 308360000 ) );

        SegAndArcCoincident.Append( VECTOR2I( 0, 20000000 ) );
        SegAndArcCoincident.Append( Arc3 );
    }
};


BOOST_FIXTURE_TEST_SUITE( TestShapeLineChain, SLC_CASES )


BOOST_AUTO_TEST_CASE( ClipperConstructorCase1 )
{
    // Case of an arc followed by a segment
    // The clipper path is not in order (on purpose), to simulate the typical return from clipper

    ClipperLib::Path pathClipper1 = {
        { { 125663951, 120099260, 24 }, { 125388111, 120170850, 25 }, { 125124975, 120280270, 26 },
          { 124879705, 120425376, 27 }, { 124657110, 120603322, 28 }, { 124461556, 120810617, 29 },
          { 124296876, 121043198, 30 }, { 124166301, 121296503, 31 }, { 124072391, 121565564, 32 },
          { 124016988, 121845106, 33 }, { 124001177, 122129646, 34 }, { 124025270, 122413605, 35 },
          { 124088794, 122691414, 36 }, { 124190502, 122957625, 37 }, { 124328401, 123207018, 38 },
          { 124499787, 123434703, 39 }, { 124598846, 123537154, 40 }, { 127171000, 123786000, 4 },
          { 127287862, 123704439, 5 },  { 127499716, 123513831, 6 },  { 127682866, 123295498, 7 },
          { 127833720, 123053722, 8 },  { 127949321, 122793242, 9 },  { 128027402, 122519168, 10 },
          { 128066430, 122236874, 11 }, { 128065642, 121951896, 12 }, { 128025053, 121669823, 13 },
          { 127945457, 121396185, 14 }, { 127828417, 121136349, 15 }, { 127676227, 120895410, 16 },
          { 127491873, 120678094, 17 }, { 127278968, 120488661, 18 }, { 127041689, 120330827, 19 },
          { 126784688, 120207687, 20 }, { 126513005, 120121655, 21 }, { 126231968, 120074419, 22 },
          { 125947087, 120066905, 23 } }
    };
    Clipper2Lib::Path64 pathClipper2 = {
        { { 125663951, 120099260, 24 }, { 125388111, 120170850, 25 }, { 125124975, 120280270, 26 },
          { 124879705, 120425376, 27 }, { 124657110, 120603322, 28 }, { 124461556, 120810617, 29 },
          { 124296876, 121043198, 30 }, { 124166301, 121296503, 31 }, { 124072391, 121565564, 32 },
          { 124016988, 121845106, 33 }, { 124001177, 122129646, 34 }, { 124025270, 122413605, 35 },
          { 124088794, 122691414, 36 }, { 124190502, 122957625, 37 }, { 124328401, 123207018, 38 },
          { 124499787, 123434703, 39 }, { 124598846, 123537154, 40 }, { 127171000, 123786000, 4 },
          { 127287862, 123704439, 5 },  { 127499716, 123513831, 6 },  { 127682866, 123295498, 7 },
          { 127833720, 123053722, 8 },  { 127949321, 122793242, 9 },  { 128027402, 122519168, 10 },
          { 128066430, 122236874, 11 }, { 128065642, 121951896, 12 }, { 128025053, 121669823, 13 },
          { 127945457, 121396185, 14 }, { 127828417, 121136349, 15 }, { 127676227, 120895410, 16 },
          { 127491873, 120678094, 17 }, { 127278968, 120488661, 18 }, { 127041689, 120330827, 19 },
          { 126784688, 120207687, 20 }, { 126513005, 120121655, 21 }, { 126231968, 120074419, 22 },
          { 125947087, 120066905, 23 } }
    };

    std::vector<CLIPPER_Z_VALUE> z_values = {
        { { -1, -1 }, 0 }, { { -1, -1 }, 0 }, { { -1, -1 }, 0 }, { { -1, -1 }, 0 },
        { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },
        { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },
        { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },
        { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },
        { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },
        { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },
        { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },
        { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },
        { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },  { { 0, -1 }, 0 },
        { { 0, -1 }, 0 }
    };

    std::vector<SHAPE_ARC> arcs = {
        SHAPE_ARC( { 127171000, 123786000 }, { 126231718, 120077003 }, { 124598846, 123537154 }, 0 )
    };

    SHAPE_LINE_CHAIN clipper1chain( pathClipper1, z_values, arcs );
    SHAPE_LINE_CHAIN clipper2chain( pathClipper2, z_values, arcs );

    BOOST_CHECK( GEOM_TEST::IsOutlineValid( clipper1chain ) );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( clipper2chain ) );

    BOOST_CHECK_EQUAL( clipper1chain.PointCount(), 37 );
    BOOST_CHECK_EQUAL( clipper2chain.PointCount(), 37 );

    BOOST_CHECK_EQUAL( clipper1chain.ArcCount(), 1 );
    BOOST_CHECK_EQUAL( clipper2chain.ArcCount(), 1 );

    BOOST_CHECK_EQUAL( clipper1chain.ShapeCount(), 2 );
    BOOST_CHECK_EQUAL( clipper2chain.ShapeCount(), 2 );

    BOOST_CHECK_EQUAL( clipper1chain.IsClosed(), true );
    BOOST_CHECK_EQUAL( clipper2chain.IsClosed(), true );
}


BOOST_AUTO_TEST_CASE( ArcToPolyline )
{
    SHAPE_LINE_CHAIN base_chain( { VECTOR2I( 0, 0 ), VECTOR2I( 0, 1000 ), VECTOR2I( 1000, 0 ) } );

    SHAPE_LINE_CHAIN chain_insert( {
            VECTOR2I( 0, 1500 ),
            VECTOR2I( 1500, 1500 ),
            VECTOR2I( 1500, 0 ),
    } );

    SHAPE_LINE_CHAIN arc_insert1( SHAPE_ARC( VECTOR2I( 0, -100 ), VECTOR2I( 0, -200 ), ANGLE_180 ) );

    SHAPE_LINE_CHAIN arc_insert2( SHAPE_ARC( VECTOR2I( 0, 500 ), VECTOR2I( 0, 400 ), ANGLE_180 ) );

    BOOST_CHECK_EQUAL( base_chain.CShapes().size(), base_chain.CPoints().size() );
    BOOST_CHECK_EQUAL( arc_insert1.CShapes().size(), arc_insert1.CPoints().size() );
    BOOST_CHECK_EQUAL( arc_insert2.CShapes().size(), arc_insert2.CPoints().size() );

    BOOST_CHECK( GEOM_TEST::IsOutlineValid( base_chain ) );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( arc_insert1 ) );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( arc_insert2 ) );

    base_chain.Insert( 0, SHAPE_ARC( VECTOR2I( 0, -100 ), VECTOR2I( 0, -200 ), ANGLE_180 ) );
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

    base_chain.Append( SHAPE_ARC( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 100000 ), ANGLE_180 ) );

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
    BOOST_CHECK_EQUAL( base_chain.CPoints().size(), base_chain.CShapes().size() );
    BOOST_CHECK_EQUAL( base_chain.PointCount(), 11 ); // We should have the same number of points
    BOOST_CHECK_EQUAL( base_chain.ArcCount(), 0 ); // All arcs should have been removed
    BOOST_CHECK_EQUAL( base_chain.Area(), areaPriorToArcRemoval ); // Area should not have changed
}

// Test that duplicate point gets removed when line is set to be closed and added where required
BOOST_AUTO_TEST_CASE( SetClosedDuplicatePoint )
{
    // Test from issue #9843
    SHAPE_LINE_CHAIN chain;

    chain.Append(
            SHAPE_ARC( { -859598, 2559876 }, { -1632771, 1022403 }, { -3170244, 249230 }, 0 ) );

    chain.Append(
            SHAPE_ARC( { -3170244, -1657832 }, { -292804, -317564 }, { 1047464, 2559876 }, 0 ) );

    chain.Append( VECTOR2I( -859598, 2559876 ) ); // add point that is equal to first arc start

    BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
    BOOST_CHECK_EQUAL( chain.PointCount(), 31 );

    // CLOSED CHAIN
    chain.SetClosed( true );
    BOOST_CHECK_EQUAL( chain.CPoints().size(), chain.CShapes().size() );
    BOOST_CHECK_EQUAL( chain.PointCount(), 30 ); // (-1) should have removed coincident points
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );

    // Special case: arc wrapping around to start (e.g. circle)
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( Circle2Arcs ) );
    BOOST_CHECK_EQUAL( Circle2Arcs.IsClosed(), true );
    BOOST_CHECK_EQUAL( Circle2Arcs.PointCount(), 16 );
    BOOST_CHECK_EQUAL( Circle2Arcs.IsArcSegment( 15 ), true );
    BOOST_CHECK_EQUAL( Circle2Arcs.ShapeCount(), 2 );
    Circle2Arcs.SetClosed( false );
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( Circle2Arcs ) );
    BOOST_CHECK_EQUAL( Circle2Arcs.IsClosed(), false );
    BOOST_CHECK_EQUAL( Circle2Arcs.PointCount(), 17 );
    BOOST_CHECK_EQUAL( Circle2Arcs.IsArcSegment( 15 ), true );
    BOOST_CHECK_EQUAL( Circle2Arcs.IsArcSegment( 16 ), false ); // last point doesn't join up
}

struct CLOSE_TOGGLE_SHAPE_CASE
{
    std::string m_ctx_name;
    SHAPE_LINE_CHAIN m_chain;
    bool m_closed;
    int m_shape_count;
    int m_point_count;
    int m_expected_shape_count;
    int m_expected_point_count;
};

static const std::vector<CLOSE_TOGGLE_SHAPE_CASE> close_toggle_shape_cases =
        {
            { "Circle1Arc",           SLC_CASES().Circle1Arc,           true,  1, 15, 1, 16 },
            { "Circle2Arcs",          SLC_CASES().Circle2Arcs,          true,  2, 16, 2, 17 },
            { "ArcsCoincident",       SLC_CASES().ArcsCoincident,       false, 2, 14, 3, 14 },
            { "ArcsCoincidentClosed", SLC_CASES().ArcsCoincidentClosed, true,  3, 14, 2, 14 },
            { "ArcsIndependent",      SLC_CASES().ArcsIndependent,      false, 3, 18, 4, 18 },
            // SegAndArcCoincident will remove the segment after SetClosed(true) and SetClosed(false)
            // disable test for now
            //{ "SegAndArcCoincident",  SLC_CASES().SegAndArcCoincident,  false, 2, 92, 2, 91 },
            { "DuplicateArcs",        SLC_CASES().DuplicateArcs,        false, 4, 20, 5, 20 },
            { "ArcAndPoint",          SLC_CASES().ArcAndPoint,          false, 2, 10, 3, 10 },
            { "ArcsAndSegMixed",      SLC_CASES().ArcsAndSegMixed,      false, 4, 19, 5, 19 },
            { "OnePoint",             SLC_CASES().OnePoint,             false, 0,  1, 0,  1 }, // no shapes
            { "TwoPoints",            SLC_CASES().TwoPoints,            false, 1,  2, 2,  2 }, // there and back
            { "ThreePoints",          SLC_CASES().ThreePoints,          false, 2,  3, 3,  3 },
        };

BOOST_AUTO_TEST_CASE( ToggleClosed )
{
    for( const CLOSE_TOGGLE_SHAPE_CASE& c : close_toggle_shape_cases )
    {
        BOOST_TEST_CONTEXT( c.m_ctx_name )
        {
            SHAPE_LINE_CHAIN slc_case = c.m_chain; // make a copy to edit
            BOOST_CHECK( GEOM_TEST::IsOutlineValid( slc_case ) );
            BOOST_CHECK_EQUAL( slc_case.IsClosed(), c.m_closed );
            BOOST_CHECK_EQUAL( slc_case.ShapeCount(), c.m_shape_count );
            BOOST_CHECK_EQUAL( slc_case.PointCount(), c.m_point_count );
            slc_case.SetClosed( !c.m_closed );
            BOOST_CHECK( GEOM_TEST::IsOutlineValid( slc_case ) );
            BOOST_CHECK_EQUAL( slc_case.IsClosed(), !c.m_closed );
            BOOST_CHECK_EQUAL( slc_case.ShapeCount(), c.m_expected_shape_count );
            BOOST_CHECK_EQUAL( slc_case.PointCount(), c.m_expected_point_count );
            slc_case.SetClosed( c.m_closed ); // toggle back to normal
            BOOST_CHECK( GEOM_TEST::IsOutlineValid( slc_case ) );
            BOOST_CHECK_EQUAL( slc_case.IsClosed(), c.m_closed );
            BOOST_CHECK_EQUAL( slc_case.ShapeCount(), c.m_shape_count );
            BOOST_CHECK_EQUAL( slc_case.PointCount(), c.m_point_count );
        }
    }
}


// Test that duplicate point gets removed when we call simplify
BOOST_AUTO_TEST_CASE( SimplifyDuplicatePoint )
{
    SHAPE_LINE_CHAIN chain;

    chain.Append( { 100, 100 } );
    chain.Append( { 100, 100 }, true ); //duplicate point to simplify
    chain.Append( { 200, 100 } );

    BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
    BOOST_CHECK_EQUAL( chain.PointCount(), 3 );

    chain.Simplify();

    BOOST_CHECK_EQUAL( chain.CPoints().size(), chain.CShapes().size() );
    BOOST_CHECK_EQUAL( chain.PointCount(), 2 ); // (-1) should have removed coincident points
    BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
}


BOOST_AUTO_TEST_CASE( SimplifyComplexChain )
{
    SHAPE_LINE_CHAIN chain;

    // Append points
    chain.Append( { 130000, 147320 } );
    chain.Append( { 125730, 147320 } );
    chain.Append( { 125730, 150630 } );
    chain.Append( { 128800, 153700 } );
    chain.Append( { 150300, 153700 } );
    chain.Append( { 151500, 152500 } );
    chain.Append( { 151500, 148900 } );
    chain.Append( { 149920, 147320 } );
    chain.Append( { 140000, 147320 } );

    BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
    BOOST_CHECK_EQUAL( chain.PointCount(), 9 );

    // The chain should be open, so the points should not be simplified
    // between the begining and the end.
    chain.Simplify();

    BOOST_CHECK_EQUAL( chain.PointCount(), 9 );

    chain.SetClosed( true );
    chain.Simplify();

    BOOST_CHECK_EQUAL( chain.PointCount(), 8 );
}

struct REMOVE_SHAPE_CASE
{
    std::string m_ctx_name;
    SHAPE_LINE_CHAIN m_chain;
    int m_shape_count;
    int m_arc_count;
    int m_remove_index;
    int m_expected_shape_count;
    int m_expected_arc_count;
};

static const std::vector<REMOVE_SHAPE_CASE> remove_shape_cases =
        {
            { "Circle1Arc - 1st arc - index on start", SLC_CASES().Circle1Arc, 1, 1,  0, 0, 0 },
            { "Circle1Arc - 1st arc - index on mid",   SLC_CASES().Circle1Arc, 1, 1,  8, 0, 0 },
            { "Circle1Arc - 1st arc - index on end",   SLC_CASES().Circle1Arc, 1, 1, 14, 0, 0 },
            { "Circle1Arc - 1st arc - index on  -1",   SLC_CASES().Circle1Arc, 1, 1, -1, 0, 0 },
            { "Circle1Arc - invalid index",            SLC_CASES().Circle1Arc, 1, 1, 15, 1, 1 },

            { "Circle2Arcs - 1st arc - index on start", SLC_CASES().Circle2Arcs, 2, 2,  0, 2, 1 },
            { "Circle2Arcs - 1st arc - index on mid",   SLC_CASES().Circle2Arcs, 2, 2,  3, 2, 1 },
            { "Circle2Arcs - 1st arc - index on end",   SLC_CASES().Circle2Arcs, 2, 2,  7, 2, 1 },
            { "Circle2Arcs - 2nd arc - index on start", SLC_CASES().Circle2Arcs, 2, 2,  8, 2, 1 },
            { "Circle2Arcs - 2nd arc - index on mid",   SLC_CASES().Circle2Arcs, 2, 2, 11, 2, 1 },
            { "Circle2Arcs - 2nd arc - index on end",   SLC_CASES().Circle2Arcs, 2, 2, 15, 2, 1 },
            { "Circle2Arcs - 2nd arc - index on  -1",   SLC_CASES().Circle2Arcs, 2, 2, -1, 2, 1 },
            { "Circle2Arcs - invalid index",            SLC_CASES().Circle2Arcs, 2, 2, 16, 2, 2 },

            { "ArcsCoinc. - 1st arc - idx on start", SLC_CASES().ArcsCoincident, 2, 2,  0, 1, 1 },
            { "ArcsCoinc. - 1st arc - idx on mid",   SLC_CASES().ArcsCoincident, 2, 2,  3, 1, 1 },
            { "ArcsCoinc. - 1st arc - idx on end",   SLC_CASES().ArcsCoincident, 2, 2,  7, 1, 1 },
            { "ArcsCoinc. - 2nd arc - idx on start", SLC_CASES().ArcsCoincident, 2, 2,  8, 1, 1 },
            { "ArcsCoinc. - 2nd arc - idx on mid",   SLC_CASES().ArcsCoincident, 2, 2, 10, 1, 1 },
            { "ArcsCoinc. - 2nd arc - idx on end",   SLC_CASES().ArcsCoincident, 2, 2, 13, 1, 1 },
            { "ArcsCoinc. - 2nd arc - idx on  -1",   SLC_CASES().ArcsCoincident, 2, 2, -1, 1, 1 },
            { "ArcsCoinc. - invalid idx",            SLC_CASES().ArcsCoincident, 2, 2, 14, 2, 2 },
            { "ArcsCoinc. - 1st arc - idx on start", SLC_CASES().ArcsCoincident, 2, 2,  0, 1, 1 },

            { "A.Co.Closed - 1st arc - idx on start", SLC_CASES().ArcsCoincidentClosed, 3, 2,  1, 2, 1 },
            { "A.Co.Closed - 1st arc - idx on mid",   SLC_CASES().ArcsCoincidentClosed, 3, 2,  3, 2, 1 },
            { "A.Co.Closed - 1st arc - idx on end",   SLC_CASES().ArcsCoincidentClosed, 3, 2,  7, 2, 1 },
            { "A.Co.Closed - 2nd arc - idx on start", SLC_CASES().ArcsCoincidentClosed, 3, 2,  8, 2, 1 },
            { "A.Co.Closed - 2nd arc - idx on mid",   SLC_CASES().ArcsCoincidentClosed, 3, 2, 10, 2, 1 },
            { "A.Co.Closed - 2nd arc - idx on end",   SLC_CASES().ArcsCoincidentClosed, 3, 2, 13, 2, 1 },
            { "A.Co.Closed - 2nd arc - idx on  -1",   SLC_CASES().ArcsCoincidentClosed, 3, 2, -1, 2, 1 },
            { "A.Co.Closed - invalid idx",            SLC_CASES().ArcsCoincidentClosed, 3, 2, 14, 3, 2 },

            { "ArcsIndep. - 1st arc - idx on start", SLC_CASES().ArcsIndependent, 3, 2,  0, 1, 1 },
            { "ArcsIndep. - 1st arc - idx on mid",   SLC_CASES().ArcsIndependent, 3, 2,  3, 1, 1 },
            { "ArcsIndep. - 1st arc - idx on end",   SLC_CASES().ArcsIndependent, 3, 2,  8, 1, 1 },
            { "ArcsIndep. - 2nd arc - idx on start", SLC_CASES().ArcsIndependent, 3, 2,  9, 1, 1 },
            { "ArcsIndep. - 2nd arc - idx on mid",   SLC_CASES().ArcsIndependent, 3, 2, 12, 1, 1 },
            { "ArcsIndep. - 2nd arc - idx on end",   SLC_CASES().ArcsIndependent, 3, 2, 17, 1, 1 },
            { "ArcsIndep. - 2nd arc - idx on  -1",   SLC_CASES().ArcsIndependent, 3, 2, -1, 1, 1 },
            { "ArcsIndep. - invalid idx",            SLC_CASES().ArcsIndependent, 3, 2, 18, 3, 2 },

            { "Dup.Arcs - 1st arc - idx on start", SLC_CASES().DuplicateArcs, 4, 3,  0, 3, 2 },
            { "Dup.Arcs - 1st arc - idx on mid",   SLC_CASES().DuplicateArcs, 4, 3,  3, 3, 2 },
            { "Dup.Arcs - 1st arc - idx on end",   SLC_CASES().DuplicateArcs, 4, 3,  7, 3, 2 },
            { "Dup.Arcs - 2nd arc - idx on start", SLC_CASES().DuplicateArcs, 4, 3,  8, 3, 2 },
            { "Dup.Arcs - 2nd arc - idx on mid",   SLC_CASES().DuplicateArcs, 4, 3, 10, 3, 2 },
            { "Dup.Arcs - 2nd arc - idx on end",   SLC_CASES().DuplicateArcs, 4, 3, 13, 3, 2 },
            { "Dup.Arcs - 3rd arc - idx on start", SLC_CASES().DuplicateArcs, 4, 3, 14, 2, 2 },
            { "Dup.Arcs - 3rd arc - idx on mid",   SLC_CASES().DuplicateArcs, 4, 3, 17, 2, 2 },
            { "Dup.Arcs - 3rd arc - idx on end",   SLC_CASES().DuplicateArcs, 4, 3, 19, 2, 2 },
            { "Dup.Arcs - 3rd arc - idx on  -1",   SLC_CASES().DuplicateArcs, 4, 3, -1, 2, 2 },
            { "Dup.Arcs - invalid idx",            SLC_CASES().DuplicateArcs, 4, 3, 20, 4, 3 },

            { "Arcs Mixed - 1st arc - idx on start", SLC_CASES().ArcsAndSegMixed, 4, 2,  0, 2, 1 },
            { "Arcs Mixed - 1st arc - idx on mid",   SLC_CASES().ArcsAndSegMixed, 4, 2,  3, 2, 1 },
            { "Arcs Mixed - 1st arc - idx on end",   SLC_CASES().ArcsAndSegMixed, 4, 2,  8, 2, 1 },
            { "Arcs Mixed - Straight segment",       SLC_CASES().ArcsAndSegMixed, 4, 2,  9, 3, 2 },
            { "Arcs Mixed - 2nd arc - idx on start", SLC_CASES().ArcsAndSegMixed, 4, 2, 10, 2, 1 },
            { "Arcs Mixed - 2nd arc - idx on mid",   SLC_CASES().ArcsAndSegMixed, 4, 2, 14, 2, 1 },
            { "Arcs Mixed - 2nd arc - idx on end",   SLC_CASES().ArcsAndSegMixed, 4, 2, 18, 2, 1 },
            { "Arcs Mixed - 2nd arc - idx on  -1",   SLC_CASES().ArcsAndSegMixed, 4, 2, -1, 2, 1 },
            { "Arcs Mixed - invalid idx",            SLC_CASES().ArcsAndSegMixed, 4, 2, 19, 4, 2 }
        };


BOOST_AUTO_TEST_CASE( RemoveShape )
{
    for( const REMOVE_SHAPE_CASE& c : remove_shape_cases )
    {
        BOOST_TEST_CONTEXT( c.m_ctx_name )
        {
            SHAPE_LINE_CHAIN slc_case = c.m_chain; // make a copy to edit
            BOOST_CHECK_EQUAL( slc_case.ShapeCount(), c.m_shape_count );
            BOOST_CHECK_EQUAL( slc_case.ArcCount(), c.m_arc_count );
            BOOST_CHECK_EQUAL( GEOM_TEST::IsOutlineValid( slc_case ), true );
            slc_case.RemoveShape( c.m_remove_index );
            BOOST_CHECK_EQUAL( slc_case.ShapeCount(), c.m_expected_shape_count );
            BOOST_CHECK_EQUAL( slc_case.ArcCount(), c.m_expected_arc_count );
            BOOST_CHECK_EQUAL( GEOM_TEST::IsOutlineValid( slc_case ), true );
        }
    }
}


BOOST_AUTO_TEST_CASE( RemoveShapeAfterSimplify )
{
    for( const REMOVE_SHAPE_CASE& c : remove_shape_cases )
    {
        BOOST_TEST_CONTEXT( c.m_ctx_name )
        {
            SHAPE_LINE_CHAIN slc_case = c.m_chain; // make a copy to edit
            BOOST_CHECK_EQUAL( GEOM_TEST::IsOutlineValid( slc_case ), true );
            BOOST_CHECK_EQUAL( slc_case.ShapeCount(), c.m_shape_count );
            BOOST_CHECK_EQUAL( slc_case.ArcCount(), c.m_arc_count );
            slc_case.Simplify();
            BOOST_CHECK_EQUAL( GEOM_TEST::IsOutlineValid( slc_case ), true );
            BOOST_CHECK_EQUAL( slc_case.ShapeCount(), c.m_shape_count );
            BOOST_CHECK_EQUAL( slc_case.ArcCount(), c.m_arc_count );
            slc_case.RemoveShape( c.m_remove_index );
            BOOST_CHECK_EQUAL( GEOM_TEST::IsOutlineValid( slc_case ), true );
            BOOST_CHECK_EQUAL( slc_case.ShapeCount(), c.m_expected_shape_count );
            BOOST_CHECK_EQUAL( slc_case.ArcCount(), c.m_expected_arc_count );
        }
    }
}


BOOST_AUTO_TEST_CASE( ShapeCount )
{
    BOOST_CHECK_EQUAL( Circle1Arc.ShapeCount(), 1 );
    BOOST_CHECK_EQUAL( Circle2Arcs.ShapeCount(), 2 );
    BOOST_CHECK_EQUAL( ArcsCoincident.ShapeCount(), 2 );
    BOOST_CHECK_EQUAL( ArcsCoincidentClosed.ShapeCount(), 3 );
    BOOST_CHECK_EQUAL( DuplicateArcs.ShapeCount(), 4 );
    BOOST_CHECK_EQUAL( ArcAndPoint.ShapeCount(), 2 );
    BOOST_CHECK_EQUAL( ArcsAndSegMixed.ShapeCount(), 4 );
    BOOST_CHECK_EQUAL( SegAndArcCoincident.ShapeCount(), 2 );
    BOOST_CHECK_EQUAL( EmptyChain.ShapeCount(), 0 );
    BOOST_CHECK_EQUAL( OnePoint.ShapeCount(), 0 );
    BOOST_CHECK_EQUAL( TwoPoints.ShapeCount(), 1 );
    BOOST_CHECK_EQUAL( ThreePoints.ShapeCount(), 2 );
}


BOOST_AUTO_TEST_CASE( NextShape )
{
    BOOST_CHECK_EQUAL( Circle1Arc.NextShape( 0 ), -1 ); //only one arc

    BOOST_CHECK_EQUAL( Circle2Arcs.NextShape( 0 ), 8 ); // next shape "Arc0b"
    BOOST_CHECK_EQUAL( Circle2Arcs.NextShape( 8 ), -1 ); //no more shapes (last point joins with first, part of arc)

    BOOST_CHECK_EQUAL( ArcsCoincident.NextShape( 0 ), 8 ); // next shape "Arc1"
    BOOST_CHECK_EQUAL( ArcsCoincident.NextShape( 8 ), -1 ); //no more shapes

    BOOST_CHECK_EQUAL( ArcsCoincidentClosed.NextShape( 0 ), 8 ); // next shape "Arc1"
    BOOST_CHECK_EQUAL( ArcsCoincidentClosed.NextShape( 8 ), 13 ); //next shape is hidden segment joining last/first
    BOOST_CHECK_EQUAL( ArcsCoincidentClosed.NextShape( 13 ), -1 ); //no more shapes

    BOOST_CHECK_EQUAL( ArcsIndependent.NextShape( 0 ), 8 ); // next shape straight seg
    BOOST_CHECK_EQUAL( ArcsIndependent.NextShape( 8 ), 9 ); //next shape second arc
    BOOST_CHECK_EQUAL( ArcsIndependent.NextShape( 9 ), -1 ); //no more shapes

    BOOST_CHECK_EQUAL( DuplicateArcs.NextShape( 0 ), 8 ); // next shape "Arc1"
    BOOST_CHECK_EQUAL( DuplicateArcs.NextShape( 8 ), 13 ); // next shape hidden segment joining the 2 duplicate arcs
    BOOST_CHECK_EQUAL( DuplicateArcs.NextShape( 13 ), 14 ); // next shape "Arc1" (duplicate)
    BOOST_CHECK_EQUAL( DuplicateArcs.NextShape( 14 ), -1 ); //no more shapes

    BOOST_CHECK_EQUAL( ArcAndPoint.NextShape( 0 ), 8 ); // next shape straight segment (end of arc->point)
    BOOST_CHECK_EQUAL( ArcAndPoint.NextShape( 8 ), -1 ); //no more shapes

    BOOST_CHECK_EQUAL( ArcsAndSegMixed.NextShape( 0 ), 8 ); // next shape straight segment (end of arc->point)
    BOOST_CHECK_EQUAL( ArcsAndSegMixed.NextShape( 8 ), 9 ); // next shape straight segment (point->begining of arc)
    BOOST_CHECK_EQUAL( ArcsAndSegMixed.NextShape( 9 ), 10 ); //next shape second arc
    BOOST_CHECK_EQUAL( ArcsAndSegMixed.NextShape( 10 ), -1 ); //no more shapes
    BOOST_CHECK_EQUAL( ArcsAndSegMixed.NextShape( 20 ), -1 ); //invalid indices should still work
    BOOST_CHECK_EQUAL( ArcsAndSegMixed.NextShape( -50 ), -1 ); //invalid indices should still work

    BOOST_CHECK_EQUAL( SegAndArcCoincident.NextShape( 0 ), 1 ); // next shape Arc3
    BOOST_CHECK_EQUAL( SegAndArcCoincident.NextShape( 1 ), -1 ); //no more shapes

    BOOST_CHECK_EQUAL( EmptyChain.NextShape( 0 ), -1 );
    BOOST_CHECK_EQUAL( EmptyChain.NextShape( 1 ), -1 ); //invalid indices should still work
    BOOST_CHECK_EQUAL( EmptyChain.NextShape( 2 ), -1 ); //invalid indices should still work
    BOOST_CHECK_EQUAL( EmptyChain.NextShape( -2 ), -1 ); //invalid indices should still work

    BOOST_CHECK_EQUAL( OnePoint.NextShape( 0 ), -1 );
    BOOST_CHECK_EQUAL( OnePoint.NextShape( -1 ), -1 );
    BOOST_CHECK_EQUAL( OnePoint.NextShape( 1 ), -1 );  //invalid indices should still work
    BOOST_CHECK_EQUAL( OnePoint.NextShape( 2 ), -1 ); //invalid indices should still work
    BOOST_CHECK_EQUAL( OnePoint.NextShape( -2 ), -1 ); //invalid indices should still work

    BOOST_CHECK_EQUAL( TwoPoints.NextShape( 0 ), -1 );
    BOOST_CHECK_EQUAL( TwoPoints.NextShape( 1 ), -1 );
    BOOST_CHECK_EQUAL( TwoPoints.NextShape( -1 ), -1 );

    BOOST_CHECK_EQUAL( ThreePoints.NextShape( 0 ), 1 );
    BOOST_CHECK_EQUAL( ThreePoints.NextShape( 1 ), -1 );
    BOOST_CHECK_EQUAL( ThreePoints.NextShape( 2 ), -1 );
    BOOST_CHECK_EQUAL( ThreePoints.NextShape( -1 ), -1 );
}



BOOST_AUTO_TEST_CASE( AppendArc )
{
    BOOST_TEST_CONTEXT( "Case 1: Arc mid point nearly collinear" )
    {
        SHAPE_ARC arc( VECTOR2I( 100000, 0 ), VECTOR2I( 0, 2499 ), VECTOR2I( -100000, 0 ), 0 );
        SHAPE_LINE_CHAIN chain;
        chain.Append( arc, 5000 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
        BOOST_CHECK_EQUAL( chain.ArcCount(), 0 );
        BOOST_CHECK_EQUAL( chain.PointCount(), 2 );
        BOOST_CHECK_EQUAL( chain.GetPoint( 0 ), VECTOR2I( 100000, 0 ) ); //arc start
        BOOST_CHECK_EQUAL( chain.GetPoint( 1 ), VECTOR2I( -100000, 0 ) ); //arc end
        BOOST_CHECK_EQUAL( chain.GetPoint( -1 ), VECTOR2I( -100000, 0 ) ); //arc end
    }

    BOOST_TEST_CONTEXT( "Case 2: Arc = Large Circle" )
    {
        SHAPE_ARC        arc( VECTOR2I( 100000, 0 ), VECTOR2I( 0, 0 ), VECTOR2I( 100000, 0 ), 0 );
        SHAPE_LINE_CHAIN chain;
        chain.Append( arc, 5000 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
        BOOST_CHECK_EQUAL( chain.ArcCount(), 1 );
        BOOST_CHECK_EQUAL( chain.PointCount(), 10 );
        BOOST_CHECK_EQUAL( chain.GetPoint( 0 ), VECTOR2I( 100000, 0 ) );   //arc start
        BOOST_CHECK_EQUAL( chain.GetPoint( 9 ), VECTOR2I( 100000, 0 ) );  //arc end
        BOOST_CHECK_EQUAL( chain.GetPoint( -1 ), VECTOR2I( 100000, 0 ) ); //arc end
    }

    BOOST_TEST_CONTEXT( "Case 3: Arc = Small Circle (approximate to point)" )
    {
        SHAPE_ARC        arc( VECTOR2I( 2499, 0 ), VECTOR2I( 0, 0 ), VECTOR2I( 2499, 0 ), 0 );
        SHAPE_LINE_CHAIN chain;
        chain.Append( arc, 5000 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
        BOOST_CHECK_EQUAL( chain.ArcCount(), 0 );
        BOOST_CHECK_EQUAL( chain.PointCount(), 1 );
        BOOST_CHECK_EQUAL( chain.GetPoint( 0 ), VECTOR2I( 2499, 0 ) );  //arc start
    }

    BOOST_TEST_CONTEXT( "Case 3: Small Arc (approximate to segment)" )
    {
        SHAPE_ARC        arc( VECTOR2I( 1767, 0 ), VECTOR2I( 2499, 2499 ), VECTOR2I( 0, 1767 ), 0 );
        SHAPE_LINE_CHAIN chain;
        chain.Append( arc, 5000 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
        BOOST_CHECK_EQUAL( chain.ArcCount(), 0 );
        BOOST_CHECK_EQUAL( chain.PointCount(), 2 );
        BOOST_CHECK_EQUAL( chain.GetPoint( 0 ), VECTOR2I( 1767, 0 ) ); //arc start
        BOOST_CHECK_EQUAL( chain.GetPoint( 1 ), VECTOR2I( 0, 1767 ) ); //arc end
    }

    BOOST_TEST_CONTEXT( "Case 4: Arc = null arc (all points coincident)" )
    {
        SHAPE_ARC        arc( VECTOR2I( 2499, 0 ), VECTOR2I( 2499, 0 ), VECTOR2I( 2499, 0 ), 0 );
        SHAPE_LINE_CHAIN chain;
        chain.Append( arc, 5000 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
        BOOST_CHECK_EQUAL( chain.ArcCount(), 0 );
        BOOST_CHECK_EQUAL( chain.PointCount(), 1 );
        BOOST_CHECK_EQUAL( chain.GetPoint( 0 ), VECTOR2I( 2499, 0 ) );    //arc start
    }

    BOOST_TEST_CONTEXT( "Case 5: Arc = infinite radius (all points very close)" )
    {
        SHAPE_ARC        arc( VECTOR2I( 2499, 0 ), VECTOR2I( 2500, 0 ), VECTOR2I( 2501, 0 ), 0 );
        SHAPE_LINE_CHAIN chain;
        chain.Append( arc, 5000 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
        BOOST_CHECK_EQUAL( chain.ArcCount(), 0 );
        BOOST_CHECK_EQUAL( chain.PointCount(), 2 );
        BOOST_CHECK_EQUAL( chain.GetPoint( 0 ), VECTOR2I( 2499, 0 ) ); //arc start
        BOOST_CHECK_EQUAL( chain.GetPoint( 1 ), VECTOR2I( 2501, 0 ) ); //arc end
    }

    BOOST_TEST_CONTEXT( "Case 6: Arc = large radius (all points very close)" )
    {
        SHAPE_ARC arc( VECTOR2I( -100000, 0 ), VECTOR2I( 0, 1 ), VECTOR2I( 100000, 0 ), 0 );
        SHAPE_LINE_CHAIN chain;
        chain.Append( arc, 5000 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( chain ) );
        BOOST_CHECK_EQUAL( chain.ArcCount(), 0 );
        BOOST_CHECK_EQUAL( chain.PointCount(), 2 );
        BOOST_CHECK_EQUAL( chain.GetPoint( 0 ), VECTOR2I( -100000, 0 ) ); //arc start
        BOOST_CHECK_EQUAL( chain.GetPoint( 1 ), VECTOR2I( 100000, 0 ) );  //arc end
    }
}


// Test special case where the last arc in the chain has a shared point with the first arc
BOOST_AUTO_TEST_CASE( ArcWrappingToStartSharedPoints )
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
    SHAPE_ARC arc( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ), ANGLE_180 );

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
    SHAPE_ARC firstArc( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ), ANGLE_180 );
    SHAPE_ARC secondArc( VECTOR2I( -200000, -200000 ), VECTOR2I( -300000, -100000 ), -ANGLE_180 );
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
        SHAPE_ARC expectedSliceArc0;
        expectedSliceArc0.ConstructFromStartEndCenter( secondArc.GetP0(), chain.GetPoint( 18 ),
                                                       secondArc.GetCenter(),
                                                       secondArc.IsClockwise() );

        BOOST_CHECK_EQUAL( sliceResult.Arc( 0 ).GetP0(), expectedSliceArc0.GetP0() ); // equal arc start points
        BOOST_CHECK( sliceResult.Arc( 0 ).Collide( expectedSliceArc0.GetArcMid(), tol ) );
        BOOST_CHECK( sliceResult.Arc( 0 ).Collide( expectedSliceArc0.GetP1(), tol ) );

        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 10 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ), firstArc.GetP1() ); // equal to arc end
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 1 ), targetSegment.A );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 2 ), targetSegment.B );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 3 ), expectedSliceArc0.GetP0() ); // equal to arc start
        BOOST_CHECK_EQUAL( sliceResult.IsArcStart( 3 ), true );

        for( int i = 4; i <= 8; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcStart( i ), false );

        for( int i = 3; i <= 7; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( i ), false );

        BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( 9 ), true );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 9 ), expectedSliceArc0.GetP1() ); // equal to arc end
    }

    //////////////////////////////////////////////////////////////////
    /// CASE 2: Start at middle of an arc, finish at arc startpoint  /
    //////////////////////////////////////////////////////////////////
    BOOST_TEST_CONTEXT( "Case 2: Start at middle of an arc, finish at arc startpoint" )
    {
        SHAPE_LINE_CHAIN sliceResult = chain.Slice( 5, 12 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( sliceResult ) );

        BOOST_CHECK_EQUAL( sliceResult.ArcCount(), 1 );
        SHAPE_ARC expectedSliceArc0;
        expectedSliceArc0.ConstructFromStartEndCenter( chain.GetPoint( 5 ), firstArc.GetP1(),
                                                       firstArc.GetCenter(),
                                                       firstArc.IsClockwise() );

        BOOST_CHECK_EQUAL( sliceResult.Arc( 0 ).GetP1(),
                           expectedSliceArc0.GetP1() ); // equal arc end points
        BOOST_CHECK( sliceResult.Arc( 0 ).Collide( expectedSliceArc0.GetArcMid(), tol ) );
        BOOST_CHECK( sliceResult.Arc( 0 ).Collide( expectedSliceArc0.GetP0(), tol ) );

        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 8 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ),
                           expectedSliceArc0.GetP0() ); // equal to arc start
        BOOST_CHECK_EQUAL( sliceResult.IsArcStart( 0 ), true );

        for( int i = 1; i <= 4; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcStart( i ), false );

        for( int i = 0; i <= 3; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( i ), false );

        BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( 4 ), true );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 4 ),
                           expectedSliceArc0.GetP1() ); // equal to arc end

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
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( sliceResult ) );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( -1 ), VECTOR2I( 400000, 400000 ) );
    }

    BOOST_TEST_CONTEXT( "Case 6: Start to end, chain with one point" )
    {
        SHAPE_LINE_CHAIN chainCopy = SLC_CASES().OnePoint;

        SHAPE_LINE_CHAIN sliceResult = chainCopy.Slice( 0, -1 );
        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 1 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ), VECTOR2I( 233450000, 228360000 ) );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( -1 ), VECTOR2I( 233450000, 228360000 ) ); // Same as index 0
    }

    BOOST_TEST_CONTEXT( "Case 7: Start to end, chain with two points" )
    {
        SHAPE_LINE_CHAIN chainCopy = SLC_CASES().TwoPoints;

        SHAPE_LINE_CHAIN sliceResult = chainCopy.Slice( 0, -1 );
        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 2 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ), VECTOR2I( 233450000, 228360000 ) );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 1 ), VECTOR2I( 263450000, 258360000 ) );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( -1 ), VECTOR2I( 263450000, 258360000 ) ); // Same as index 1
    }

    BOOST_TEST_CONTEXT( "Case 8: Full 2nd arc, nothing else" )
    {
        SHAPE_LINE_CHAIN sliceResult = chain.Slice( 12, 19 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( sliceResult ) );

        BOOST_CHECK_EQUAL( sliceResult.ArcCount(), 1 );
        SHAPE_ARC sliceArc0 = sliceResult.Arc( 0 );

        // Equal arc to original inserted arc
        BOOST_CHECK_EQUAL( secondArc.GetP1(), sliceArc0.GetP1() );
        BOOST_CHECK_EQUAL( secondArc.GetArcMid(), sliceArc0.GetArcMid() );
        BOOST_CHECK_EQUAL( secondArc.GetP1(), sliceArc0.GetP1() );

        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 8 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ), sliceArc0.GetP0() ); // equal to arc start
        BOOST_CHECK_EQUAL( sliceResult.IsArcStart( 0 ), true );

        for( int i = 1; i <= 7; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcStart( i ), false );

        for( int i = 0; i <= 6; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( i ), false );

        BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( 7 ), true );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 7 ), sliceArc0.GetP1() ); // equal to arc end
    }

    BOOST_TEST_CONTEXT( "Case 9: Start at middle of a 2nd arc, finish at end" )
    {
        SHAPE_LINE_CHAIN sliceResult = chain.Slice( 16, 19 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( sliceResult ) );

        BOOST_CHECK_EQUAL( sliceResult.ArcCount(), 1 );

        SHAPE_ARC expectedSliceArc0;
        expectedSliceArc0.ConstructFromStartEndCenter( chain.GetPoint( 16 ), secondArc.GetP1(),
                                                       secondArc.GetCenter(),
                                                       secondArc.IsClockwise() );

        BOOST_CHECK_EQUAL( sliceResult.Arc( 0 ).GetP1(),
                           expectedSliceArc0.GetP1() ); // equal arc end points
        BOOST_CHECK( sliceResult.Arc( 0 ).Collide( expectedSliceArc0.GetArcMid(), tol ) );
        BOOST_CHECK( sliceResult.Arc( 0 ).Collide( expectedSliceArc0.GetP0(), tol ) );

        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 4 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ),
                           expectedSliceArc0.GetP0() ); // equal to arc start
        BOOST_CHECK_EQUAL( sliceResult.IsArcStart( 0 ), true );

        for( int i = 1; i <= 3; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcStart( i ), false );

        for( int i = 0; i <= 2; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( i ), false );

        BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( 3 ), true );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 3 ),
                           expectedSliceArc0.GetP1() ); // equal to arc end
    }

    BOOST_TEST_CONTEXT( "Case 10: New chain, start at arc middle, finish at end" )
    {
        SHAPE_LINE_CHAIN chain10;
        chain10.Append( firstArc );

        SHAPE_LINE_CHAIN sliceResult = chain10.Slice( 3, 6 );
        BOOST_CHECK( GEOM_TEST::IsOutlineValid( sliceResult ) );

        BOOST_CHECK_EQUAL( sliceResult.ArcCount(), 1 );

        SHAPE_ARC expectedSliceArc0;
        expectedSliceArc0.ConstructFromStartEndCenter( chain10.GetPoint( 3 ), firstArc.GetP1(),
                                                       firstArc.GetCenter(),
                                                       firstArc.IsClockwise() );

        BOOST_CHECK_EQUAL( sliceResult.Arc( 0 ).GetP1(),
                           expectedSliceArc0.GetP1() ); // equal arc end points
        BOOST_CHECK( sliceResult.Arc( 0 ).Collide( expectedSliceArc0.GetArcMid(), tol ) );
        BOOST_CHECK( sliceResult.Arc( 0 ).Collide( expectedSliceArc0.GetP0(), tol ) );

        BOOST_CHECK_EQUAL( sliceResult.PointCount(), 4 );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 0 ),
                           expectedSliceArc0.GetP0() ); // equal to arc start
        BOOST_CHECK_EQUAL( sliceResult.IsArcStart( 0 ), true );

        for( int i = 1; i <= 3; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcStart( i ), false );

        for( int i = 0; i <= 2; i++ )
            BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( i ), false );

        BOOST_CHECK_EQUAL( sliceResult.IsArcEnd( 3 ), true );
        BOOST_CHECK_EQUAL( sliceResult.GetPoint( 3 ),
                           expectedSliceArc0.GetP1() ); // equal to arc end
    }
}


// Test SHAPE_LINE_CHAIN::NearestPoint( VECTOR2I )
BOOST_AUTO_TEST_CASE( NearestPointPt )
{
    SEG       seg1( VECTOR2I( 0, 100000 ), VECTOR2I( 50000, 0 ) );
    SEG       seg2( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ) );
    SHAPE_ARC arc( VECTOR2I( 200000, 0 ), VECTOR2I( 300000, 0 ), ANGLE_180 );

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
