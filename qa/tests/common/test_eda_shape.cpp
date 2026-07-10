/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 */


#include <qa_utils/wx_utils/unit_test_utils.h>

#include <cmath>

#include <base_units.h>
#include <eda_shape.h>
#include <math/util.h>
#include <tool/point_editor_behavior.h>
#include <qa_utils/geometry/geometry.h> // For KI_TEST::IsVecWithinTol
#include <geometry/shape_arc.h> // For SHAPE_ARC::DefaultAccuracyForPCB()


BOOST_AUTO_TEST_SUITE( EdaShape )

class EDA_SHAPE_MOCK : public EDA_SHAPE
{
public:
    EDA_SHAPE_MOCK( SHAPE_T aShapeType ) : EDA_SHAPE( aShapeType, 0, FILL_T::NO_FILL ){};
};


struct SET_ANGLE_END_CASE
{
    std::string m_CaseName;
    VECTOR2I    m_Start;
    VECTOR2I    m_Center;
    double      m_Angle;
    VECTOR2I    m_ExpectedEndBeforeSwap;
    bool        m_ExpectedStartEndSwapped;
};


static const std::vector<SET_ANGLE_END_CASE> set_angle_end_cases =
{
    {
        "Issue 13626: clockwise semicircle",
        {-428880000, 117229160 },
        {-430060565, 113472820 },
        180.0,
        {-431241130, 109716480 },
        false
    },
    {
        "Issue 13626: anticlockwise arc",
        { -431241130, 109716480 },
        { -434923630, 112954230 },
        -138.46654568595355,
        { -439827050, 112936200 },
        true
    }
};


BOOST_AUTO_TEST_CASE( SetAngleAndEnd )
{
    for( const auto& c : set_angle_end_cases )
    {
        BOOST_TEST_INFO_SCOPE( c.m_CaseName );

        EDA_SHAPE_MOCK shape( SHAPE_T::ARC );
        shape.SetStart( c.m_Start );
        shape.SetCenter( c.m_Center );

        shape.SetArcAngleAndEnd( EDA_ANGLE( c.m_Angle, DEGREES_T ), true );

        BOOST_CHECK_EQUAL( shape.EndsSwapped(), c.m_ExpectedStartEndSwapped );

        const VECTOR2I newEnd = shape.EndsSwapped() ? shape.GetStart() : shape.GetEnd();

        BOOST_CHECK_PREDICATE(
                KI_TEST::IsVecWithinTol<VECTOR2I>,
                (newEnd) ( c.m_ExpectedEndBeforeSwap ) ( SHAPE_ARC::DefaultAccuracyForPCB() ) );
    }
}


struct SET_ARC_GEOMETRY_CASE
{
    std::string m_CaseName;
    VECTOR2I    m_Start;
    VECTOR2I    m_Mid;
    VECTOR2I    m_End;
    VECTOR2I    m_ExpectedCenter;
    int         m_ExpectedRadius;
    bool        m_ExpectedStartEndSwapped;
    VECTOR2I    m_ExpectedEndAfterSwap;
    double      m_ExpectedAngleAfterSwapDeg;
};

static const std::vector<SET_ARC_GEOMETRY_CASE> set_arc_geometry_cases = {
    {
            // Test that when setting an arc by start/mid/end, the winding
            // direction is correctly determined (in 15694, this was in FP_SHAPE,
            // but the logic has since been merged with EDA_SHAPE).
            "Issue 15694: clockwise arc",
            { 10000000, 0 },
            { 0, 10000000 },
            { -10000000, 0 },
            { 0, 0 },
            10000000,
            false,
            { -10000000, 0 }, // unchanged
            180.0,
    },
    {
            "Issue 15694: anticlockwise arc",
            { -10000000, 0 },
            { 0, 10000000 },
            { 10000000, 0 },
            { 0, 0 },
            10000000,
            true,
            { 10000000, 0 }, // the start is the end after swapping
            180.0,           // angle is positive after swapping
    },
};

BOOST_AUTO_TEST_CASE( SetArcGeometry )
{
    const double angle_tol = 0.1;

    for( const auto& c : set_arc_geometry_cases )
    {
        BOOST_TEST_INFO_SCOPE( c.m_CaseName );

        EDA_SHAPE_MOCK shape( SHAPE_T::ARC );

        shape.SetArcGeometry( c.m_Start, c.m_Mid, c.m_End );

        const VECTOR2I center = shape.getCenter();

        BOOST_CHECK_PREDICATE(
                KI_TEST::IsVecWithinTol<VECTOR2I>,
                (center) ( c.m_ExpectedCenter ) ( SHAPE_ARC::DefaultAccuracyForPCB() ) );

        const int radius = shape.GetRadius();

        BOOST_CHECK_PREDICATE(
                KI_TEST::IsWithin<int>,
                (radius) ( c.m_ExpectedRadius ) ( SHAPE_ARC::DefaultAccuracyForPCB() ) );

        BOOST_CHECK_EQUAL( shape.EndsSwapped(), c.m_ExpectedStartEndSwapped );

        const VECTOR2I newEnd = shape.EndsSwapped() ? shape.GetStart() : shape.GetEnd();

        BOOST_CHECK_PREDICATE(
                KI_TEST::IsVecWithinTol<VECTOR2I>,
                (newEnd) ( c.m_ExpectedEndAfterSwap ) ( SHAPE_ARC::DefaultAccuracyForPCB() ) );

        const EDA_ANGLE angle = shape.GetArcAngle();

        BOOST_CHECK_PREDICATE(
                KI_TEST::IsWithinWrapped<double>,
                ( angle.AsDegrees() )( c.m_ExpectedAngleAfterSwapDeg )( 360.0 )( angle_tol ) );

        // Check that the centre is still correct
    }
}

/**
 * Editing a small eeschema arc must not snap its radius up to the PCB-scale
 * 1 mil minimum.  The edit helpers used to hard-code pcbIUScale, which in
 * schematic IUs (1 IU = 100 nm) is a 25400 IU == 100 mil floor.
 * See https://gitlab.com/kicad/code/kicad/-/issues/24396.
 */
BOOST_AUTO_TEST_CASE( ArcEditKeepsSmallSchematicRadius )
{
    // 50 mil radius arc in schematic IUs, well under the buggy 100 mil floor.
    const int      radius = schIUScale.MilsToIU( 50 );
    const VECTOR2I center( 0, 0 );
    const VECTOR2I start( radius, 0 );
    const VECTOR2I end( 0, radius );
    const VECTOR2I mid( KiROUND( radius / std::sqrt( 2.0 ) ),
                        KiROUND( radius / std::sqrt( 2.0 ) ) );

    EDA_SHAPE_MOCK arc( SHAPE_T::ARC );
    arc.SetArcGeometry( start, mid, end );

    BOOST_REQUIRE_LT( arc.GetRadius(), schIUScale.MilsToIU( 100 ) );

    // Drag the endpoint a few IU; with the bug the radius snaps up to 100 mil.
    const VECTOR2I newEnd( 5, radius );

    KI_ARC_EDIT::EditArcEndpointKeepCenter( arc, center, start, mid, newEnd, newEnd, schIUScale );
    BOOST_CHECK_LT( arc.GetRadius(), schIUScale.MilsToIU( 100 ) );

    // Same for the mid-point helper, which has its own minimum-radius clamp.
    const VECTOR2I smallerMid( KiROUND( ( radius - 100 ) / std::sqrt( 2.0 ) ),
                               KiROUND( ( radius - 100 ) / std::sqrt( 2.0 ) ) );

    EDA_SHAPE_MOCK arc2( SHAPE_T::ARC );
    arc2.SetArcGeometry( start, mid, end );

    KI_ARC_EDIT::EditArcMidKeepCenter( arc2, center, start, mid, end, smallerMid, schIUScale );
    BOOST_CHECK_LT( arc2.GetRadius(), schIUScale.MilsToIU( 100 ) );
}


/**
 * Verify that EDA_POLYGON_POINT_EDIT_BEHAVIOR survives EDA_SHAPE assignment.
 *
 * EDA_SHAPE::operator= replaces m_poly with a new unique_ptr. The behavior must
 * resolve GetPolyShape() on each call rather than caching a reference that goes stale.
 * See https://gitlab.com/kicad/code/kicad/-/issues/23648
 */
BOOST_AUTO_TEST_CASE( PolygonBehaviorSurvivesAssignment )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::POLY );

    SHAPE_POLY_SET& poly = shape.GetPolyShape();
    poly.NewOutline();
    poly.Append( { 0, 0 } );
    poly.Append( { 1000000, 0 } );
    poly.Append( { 1000000, 1000000 } );

    EDA_POLYGON_POINT_EDIT_BEHAVIOR behavior( shape );

    EDIT_POINTS points( nullptr );
    behavior.MakePoints( points );
    BOOST_CHECK_EQUAL( points.PointsSize(), 3u );

    EDA_SHAPE_MOCK copy( shape );
    shape = copy;

    // After assignment, shape.m_poly is a fresh allocation.
    // The behavior must still work (not use-after-free).
    EDIT_POINTS points2( nullptr );
    behavior.MakePoints( points2 );
    BOOST_CHECK_EQUAL( points2.PointsSize(), 3u );

    BOOST_CHECK( behavior.UpdatePoints( points ) );
}


BOOST_AUTO_TEST_CASE( GetPolyPointsPreservesOrderAcrossOutlines )
{
    // GetPolyPoints flattens every outline of the poly shape into a single
    // ordered vector. Verify the count and order are preserved across multiple
    // outlines so the single up-front reserve does not alter behavior.
    EDA_SHAPE_MOCK shape( SHAPE_T::POLY );

    SHAPE_POLY_SET& poly = shape.GetPolyShape();

    poly.NewOutline();
    poly.Append( { 0, 0 } );
    poly.Append( { 1000, 0 } );
    poly.Append( { 1000, 1000 } );

    poly.NewOutline();
    poly.Append( { 5000, 5000 } );
    poly.Append( { 6000, 5000 } );

    const std::vector<VECTOR2I> expected = {
        { 0, 0 }, { 1000, 0 }, { 1000, 1000 }, { 5000, 5000 }, { 6000, 5000 }
    };

    const std::vector<VECTOR2I> points = shape.GetPolyPoints();

    BOOST_REQUIRE_EQUAL( points.size(), expected.size() );

    for( size_t ii = 0; ii < expected.size(); ++ii )
    {
        BOOST_CHECK_EQUAL( points[ii].x, expected[ii].x );
        BOOST_CHECK_EQUAL( points[ii].y, expected[ii].y );
    }
}


BOOST_AUTO_TEST_CASE( EllipseBasicAccessors )
{
    // Construct a closed ellipse EDA_SHAPE and round-trip every accessor.
    EDA_SHAPE_MOCK e( SHAPE_T::ELLIPSE );
    e.SetEllipseCenter( VECTOR2I( 100, 200 ) );
    e.SetEllipseMajorRadius( 500 );
    e.SetEllipseMinorRadius( 300 );
    e.SetEllipseRotation( EDA_ANGLE( 30.0, DEGREES_T ) );

    BOOST_CHECK( e.GetShape() == SHAPE_T::ELLIPSE );
    BOOST_CHECK_EQUAL( e.GetEllipseCenter().x, 100 );
    BOOST_CHECK_EQUAL( e.GetEllipseCenter().y, 200 );
    BOOST_CHECK_EQUAL( e.GetEllipseMajorRadius(), 500 );
    BOOST_CHECK_EQUAL( e.GetEllipseMinorRadius(), 300 );
    BOOST_CHECK_CLOSE( e.GetEllipseRotation().AsDegrees(), 30.0, 1e-6 );

    // Closed ellipse reports itself as a closed shape.
    BOOST_CHECK( e.IsClosed() );
}


BOOST_AUTO_TEST_CASE( EllipseArcIsOpenCurve )
{
    // Elliptical arcs are open
    // IsClosed() must return false.
    EDA_SHAPE_MOCK arc( SHAPE_T::ELLIPSE_ARC );
    arc.SetEllipseCenter( VECTOR2I( 0, 0 ) );
    arc.SetEllipseMajorRadius( 500 );
    arc.SetEllipseMinorRadius( 300 );
    arc.SetEllipseRotation( EDA_ANGLE( 0.0, DEGREES_T ) );
    arc.SetEllipseStartAngle( EDA_ANGLE( 0.0, DEGREES_T ) );
    arc.SetEllipseEndAngle( EDA_ANGLE( 180.0, DEGREES_T ) );

    BOOST_CHECK( arc.GetShape() == SHAPE_T::ELLIPSE_ARC );
    BOOST_CHECK( !arc.IsClosed() );

    // Start/end angles round trip through the accessors.
    BOOST_CHECK_CLOSE( arc.GetEllipseStartAngle().AsDegrees(), 0.0, 1e-6 );
    BOOST_CHECK_CLOSE( arc.GetEllipseEndAngle().AsDegrees(), 180.0, 1e-6 );
}


BOOST_AUTO_TEST_CASE( EllipsePerimeterForCircleCase )
{
    // An ellipse with MajorRadius == MinorRadius is a circle.
    // Ramanujan's approximation returns 2πr for this case.
    EDA_SHAPE_MOCK e( SHAPE_T::ELLIPSE );
    e.SetEllipseCenter( VECTOR2I( 0, 0 ) );
    e.SetEllipseMajorRadius( 1000 );
    e.SetEllipseMinorRadius( 1000 );
    e.SetEllipseRotation( EDA_ANGLE( 0.0, DEGREES_T ) );

    const double expected = 2.0 * M_PI * 1000.0;
    BOOST_CHECK_CLOSE( e.GetLength(), expected, 1e-6 );
}


BOOST_AUTO_TEST_CASE( EllipseMakeEffectiveShapesNonEmpty )
{
    // MakeEffectiveShapes converts the ellipse into primitive shapes that DRC
    // the router, and exporters consume. Verify it returns at least one shape

    EDA_SHAPE_MOCK e( SHAPE_T::ELLIPSE );
    e.SetEllipseCenter( VECTOR2I( 0, 0 ) );
    e.SetEllipseMajorRadius( 500 );
    e.SetEllipseMinorRadius( 300 );
    e.SetEllipseRotation( EDA_ANGLE( 0.0, DEGREES_T ) );

    std::vector<SHAPE*> shapes = e.MakeEffectiveShapes();
    BOOST_CHECK( !shapes.empty() );

    for( SHAPE* s : shapes )
        delete s;
}

BOOST_AUTO_TEST_SUITE_END()
