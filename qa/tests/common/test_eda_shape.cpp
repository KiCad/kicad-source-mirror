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
#include <geometry/shape_line_chain.h>
#include <geometry/shape_segment.h>
#include <render_settings.h>
#include <stroke_params.h>


BOOST_AUTO_TEST_SUITE( EdaShape )

class EDA_SHAPE_MOCK : public EDA_SHAPE
{
public:
    EDA_SHAPE_MOCK( SHAPE_T aShapeType ) : EDA_SHAPE( aShapeType, 0, FILL_T::NO_FILL ){};
};


static void checkVectorClose( const VECTOR2D& aActual, const VECTOR2D& aExpected, double aTolerance = 1e-6 )
{
    BOOST_CHECK_SMALL( aActual.x - aExpected.x, aTolerance );
    BOOST_CHECK_SMALL( aActual.y - aExpected.y, aTolerance );
}


static void checkVectorEqual( const VECTOR2I& aActual, const VECTOR2I& aExpected )
{
    BOOST_CHECK_EQUAL( aActual.x, aExpected.x );
    BOOST_CHECK_EQUAL( aActual.y, aExpected.y );
}


static void checkAngleClose( const EDA_ANGLE& aActual, double aExpectedDegrees, double aTolerance = 1e-6 )
{
    BOOST_CHECK_SMALL( aActual.AsDegrees() - aExpectedDegrees, aTolerance );
}


static void checkAngleInRange( const EDA_ANGLE& aActual, double aMinDegrees, double aMaxDegrees )
{
    BOOST_CHECK_MESSAGE( aActual.AsDegrees() > aMinDegrees && aActual.AsDegrees() < aMaxDegrees,
                         "Expected angle " << aActual.AsDegrees() << " to be between " << aMinDegrees << " and "
                                           << aMaxDegrees );
}


static void deleteShapes( std::vector<SHAPE*>& aShapes )
{
    for( SHAPE* shape : aShapes )
        delete shape;

    aShapes.clear();
}


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


BOOST_AUTO_TEST_CASE( PolygonBehaviorSignalsRebuildWhenShapeMorphs )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::POLY );

    SHAPE_POLY_SET& poly = shape.GetPolyShape();
    poly.NewOutline();
    poly.Append( { 0, 0 } );
    poly.Append( { 1000000, 0 } );
    poly.Append( { 1000000, 1000000 } );
    poly.Append( { 0, 1000000 } );

    EDA_POLYGON_POINT_EDIT_BEHAVIOR behavior( shape );

    EDIT_POINTS points( nullptr );
    behavior.MakePoints( points );
    BOOST_CHECK( behavior.UpdatePoints( points ) );

    shape.SetShape( SHAPE_T::RECTANGLE );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000000, 1000000 } );

    BOOST_CHECK( !behavior.UpdatePoints( points ) );
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


BOOST_AUTO_TEST_CASE( ShortenSegmentForEndingsTrimsStartAndEnd )
{
    VECTOR2I    start( 0, 0 );
    VECTOR2I    end( 1000, 0 );
    LINE_ENDING startEnding( LINE_ENDING_STYLE::ARROW, 100, 100 );
    LINE_ENDING endEnding( LINE_ENDING_STYLE::CIRCLE, 200, 200 );

    BOOST_CHECK( EDA_SHAPE::ShortenSegmentForEndings( start, end, startEnding, endEnding, 20 ) );
    BOOST_CHECK_EQUAL( start.x, 100 );
    BOOST_CHECK_EQUAL( start.y, 0 );
    BOOST_CHECK_EQUAL( end.x, 900 );
    BOOST_CHECK_EQUAL( end.y, 0 );
}


BOOST_AUTO_TEST_CASE( SegmentOpenArrowLeavesBody )
{
    VECTOR2I    start( 0, 0 );
    VECTOR2I    end( 1000, 0 );
    LINE_ENDING startEnding( LINE_ENDING_STYLE::ARROW_OPEN, 500, 500 );
    LINE_ENDING endEnding;

    BOOST_CHECK( EDA_SHAPE::ShortenSegmentForEndings( start, end, startEnding, endEnding, 20 ) );
    BOOST_CHECK_EQUAL( start.x, 0 );
    BOOST_CHECK_EQUAL( start.y, 0 );
    BOOST_CHECK_EQUAL( end.x, 1000 );
    BOOST_CHECK_EQUAL( end.y, 0 );
}


BOOST_AUTO_TEST_CASE( SegmentShorteningRejectsOverrun )
{
    VECTOR2I    start( 0, 0 );
    VECTOR2I    end( 1000, 0 );
    LINE_ENDING startEnding( LINE_ENDING_STYLE::ARROW, 600, 100 );
    LINE_ENDING endEnding( LINE_ENDING_STYLE::ARROW, 500, 100 );

    BOOST_CHECK( !EDA_SHAPE::ShortenSegmentForEndings( start, end, startEnding, endEnding, 20 ) );
    BOOST_CHECK_EQUAL( start.x, 0 );
    BOOST_CHECK_EQUAL( start.y, 0 );
    BOOST_CHECK_EQUAL( end.x, 0 );
    BOOST_CHECK_EQUAL( end.y, 0 );
}


BOOST_AUTO_TEST_CASE( SegmentEndingTangentsPointOutward )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );

    EDA_ANGLE startTangent;
    EDA_ANGLE endTangent;
    shape.GetEndingTangents( startTangent, endTangent, 20 );

    checkAngleClose( startTangent, 180.0 );
    checkAngleClose( endTangent, 0.0 );
}


BOOST_AUTO_TEST_CASE( OpenArrowBezierTangentsUseDepth )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::BEZIER );
    shape.SetStart( { 0, 0 } );
    shape.SetBezierC1( { 1000, 0 } );
    shape.SetBezierC2( { 0, 1000 } );
    shape.SetEnd( { 1000, 1000 } );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW_OPEN );
    shape.SetStartEndingLength( 300 );
    shape.SetStartEndingWidth( 300 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW_OPEN );
    shape.SetEndEndingLength( 300 );
    shape.SetEndEndingWidth( 300 );

    EDA_ANGLE startTangent;
    EDA_ANGLE endTangent;
    shape.GetEndingTangents( startTangent, endTangent, 20 );

    checkAngleInRange( startTangent, -180.0, -90.0 );
    checkAngleInRange( endTangent, 0.0, 90.0 );
}


BOOST_AUTO_TEST_CASE( LineEndingEndpointsUseShapeOwnedSources )
{
    VECTOR2I start;
    VECTOR2I end;

    EDA_SHAPE_MOCK segment( SHAPE_T::SEGMENT );
    segment.SetStart( { 0, 10 } );
    segment.SetEnd( { 100, 20 } );

    BOOST_CHECK( segment.GetLineEndingEndpoints( start, end ) );
    checkVectorEqual( start, { 0, 10 } );
    checkVectorEqual( end, { 100, 20 } );

    EDA_SHAPE_MOCK arc( SHAPE_T::ARC );
    arc.SetStart( { 10, 0 } );
    arc.SetEnd( { 20, 30 } );

    BOOST_CHECK( arc.GetLineEndingEndpoints( start, end ) );
    checkVectorEqual( start, { 10, 0 } );
    checkVectorEqual( end, { 20, 30 } );

    EDA_SHAPE_MOCK bezier( SHAPE_T::BEZIER );
    bezier.SetStart( { 0, 0 } );
    bezier.SetBezierC1( { 20, 0 } );
    bezier.SetBezierC2( { 80, 100 } );
    bezier.SetEnd( { 100, 100 } );
    bezier.RebuildBezierToSegmentsPointsList( 1 );

    BOOST_CHECK( bezier.GetLineEndingEndpoints( start, end ) );
    checkVectorEqual( start, { 0, 0 } );
    checkVectorEqual( end, { 100, 100 } );

    EDA_SHAPE_MOCK polyShape( SHAPE_T::POLY );
    SHAPE_POLY_SET poly;
    poly.NewOutline();
    poly.Outline( 0 ).SetClosed( false );
    poly.Append( { 5, 5 } );
    poly.Append( { 50, 10 } );
    poly.Append( { 100, 20 } );
    polyShape.SetPolyShape( poly );

    BOOST_CHECK( polyShape.GetLineEndingEndpoints( start, end ) );
    checkVectorEqual( start, { 5, 5 } );
    checkVectorEqual( end, { 100, 20 } );
}


BOOST_AUTO_TEST_CASE( LineEndingEndpointsRejectInvalidPolys )
{
    VECTOR2I start;
    VECTOR2I end;

    EDA_SHAPE_MOCK emptyPoly( SHAPE_T::POLY );
    BOOST_CHECK( !emptyPoly.GetLineEndingEndpoints( start, end ) );

    EDA_SHAPE_MOCK onePointPoly( SHAPE_T::POLY );
    SHAPE_POLY_SET poly;
    poly.NewOutline();
    poly.Append( { 5, 5 } );
    onePointPoly.SetPolyShape( poly );

    BOOST_CHECK( !onePointPoly.GetLineEndingEndpoints( start, end ) );
}


BOOST_AUTO_TEST_CASE( EffectiveShapesIncludeClosedEnding )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::CIRCLE );
    shape.SetEndEndingLength( 400 );
    shape.SetEndEndingWidth( 400 );

    std::vector<SHAPE*> shapes = shape.MakeLineEndingEffectiveShapes( 20 );

    BOOST_REQUIRE_EQUAL( shapes.size(), 1 );

    BOX2I bbox = shapes[0]->BBox();
    BOOST_CHECK( bbox.GetLeft() <= 800 );
    BOOST_CHECK( bbox.GetRight() >= 1200 );
    BOOST_CHECK( bbox.GetTop() <= -200 );
    BOOST_CHECK( bbox.GetBottom() >= 200 );

    deleteShapes( shapes );
}


BOOST_AUTO_TEST_CASE( EffectiveShapesIncludeOpenArrowLegs )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW_OPEN );
    shape.SetEndEndingLength( 300 );
    shape.SetEndEndingWidth( 200 );
    shape.SetEndEndingStrokeWidth( 70 );

    std::vector<SHAPE*> shapes = shape.MakeLineEndingEffectiveShapes( 20 );

    BOOST_REQUIRE_EQUAL( shapes.size(), 2 );

    for( SHAPE* shapePtr : shapes )
    {
        const SHAPE_SEGMENT* leg = dynamic_cast<const SHAPE_SEGMENT*>( shapePtr );
        BOOST_REQUIRE( leg );
        BOOST_CHECK_EQUAL( leg->GetWidth(), 70 );
    }

    deleteShapes( shapes );
}


BOOST_AUTO_TEST_CASE( BBoxIncludesCenteredEnding )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::SQUARE );
    shape.SetEndEndingLength( 400 );
    shape.SetEndEndingWidth( 400 );

    BOX2I bbox;

    BOOST_CHECK( shape.GetLineEndingsBoundingBox( bbox, 20 ) );
    BOOST_CHECK( bbox.GetLeft() <= 800 );
    BOOST_CHECK( bbox.GetRight() >= 1200 );
    BOOST_CHECK( bbox.GetTop() <= -200 );
    BOOST_CHECK( bbox.GetBottom() >= 200 );
}


BOOST_AUTO_TEST_CASE( BBoxUsesRotatedEndingGeometry )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 1000 } );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::SQUARE );
    shape.SetEndEndingLength( 400 );
    shape.SetEndEndingWidth( 400 );

    BOX2I bbox;

    BOOST_CHECK( shape.GetLineEndingsBoundingBox( bbox, 20 ) );
    BOOST_CHECK( bbox.GetLeft() <= 718 );
    BOOST_CHECK( bbox.GetRight() >= 1282 );
    BOOST_CHECK( bbox.GetTop() <= 718 );
    BOOST_CHECK( bbox.GetBottom() >= 1282 );
}


BOOST_AUTO_TEST_CASE( PolygonConversionAppendsEndings )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetWidth( 20 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::SQUARE );
    shape.SetEndEndingLength( 400 );
    shape.SetEndEndingWidth( 400 );

    SHAPE_POLY_SET body;
    SHAPE_POLY_SET withEnding;

    shape.TransformShapeToPolygon( body, 0, 1, ERROR_OUTSIDE );
    shape.TransformShapeToPolygon( withEnding, 0, 1, ERROR_OUTSIDE );
    shape.TransformLineEndingsToPolygon( withEnding, 0, 1, ERROR_OUTSIDE, shape.GetWidth() );

    BOX2I bodyBBox = body.BBox();
    BOX2I endingBBox = withEnding.BBox();

    BOOST_CHECK( endingBBox.GetRight() > bodyBBox.GetRight() );
    BOOST_CHECK( endingBBox.GetTop() < bodyBBox.GetTop() );
    BOOST_CHECK( endingBBox.GetBottom() > bodyBBox.GetBottom() );
}


BOOST_AUTO_TEST_CASE( EffectiveShapesShortenArrowBody )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetWidth( 20 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 200 );
    shape.SetEndEndingWidth( 200 );

    std::vector<SHAPE*>  shapes = shape.MakeEffectiveShapesWithLineEndings( shape.GetWidth() );
    const SHAPE_SEGMENT* body = nullptr;

    for( SHAPE* shapePtr : shapes )
    {
        if( const SHAPE_SEGMENT* segment = dynamic_cast<const SHAPE_SEGMENT*>( shapePtr ) )
        {
            body = segment;
            break;
        }
    }

    BOOST_REQUIRE( body );
    checkVectorEqual( body->GetStart(), { 0, 0 } );
    checkVectorEqual( body->GetEnd(), { 800, 0 } );

    deleteShapes( shapes );
}


BOOST_AUTO_TEST_CASE( StrokingShapesShortenArrowBodyOnly )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetWidth( 20 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 200 );
    shape.SetEndEndingWidth( 200 );

    std::vector<SHAPE*> shapes = shape.MakeEffectiveShapesForStroking( shape.GetWidth() );

    BOOST_REQUIRE_EQUAL( shapes.size(), 1 );

    const SHAPE_SEGMENT* body = dynamic_cast<const SHAPE_SEGMENT*>( shapes.front() );
    BOOST_REQUIRE( body );
    checkVectorEqual( body->GetStart(), { 0, 0 } );
    checkVectorEqual( body->GetEnd(), { 800, 0 } );

    deleteShapes( shapes );
}


BOOST_AUTO_TEST_CASE( StrokingShapesRejectConsumedSegmentBody )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetWidth( 20 );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 600 );
    shape.SetStartEndingWidth( 200 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 500 );
    shape.SetEndEndingWidth( 200 );

    std::vector<SHAPE*> shapes = shape.MakeEffectiveShapesForStroking( shape.GetWidth() );

    BOOST_CHECK( shapes.empty() );

    deleteShapes( shapes );
}


BOOST_AUTO_TEST_CASE( EffectiveShapesHitEndpointSquare )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetWidth( 100 );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::SQUARE );
    shape.SetStartEndingLength( 400 );
    shape.SetStartEndingWidth( 400 );

    std::vector<SHAPE*> shapes = shape.MakeEffectiveShapesWithLineEndings( shape.GetWidth() );
    SHAPE_SEGMENT       probe( VECTOR2I( 0, -150 ), VECTOR2I( 0, 150 ), 20 );
    bool                collides = false;

    for( SHAPE* shapePtr : shapes )
    {
        if( shapePtr->Collide( probe.GetSeg(), 0 ) )
        {
            collides = true;
            break;
        }
    }

    BOOST_CHECK( collides );

    deleteShapes( shapes );
}


BOOST_AUTO_TEST_CASE( PolygonConversionShortensArrowBody )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::SEGMENT );
    shape.SetStart( { 0, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetWidth( 100 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 200 );
    shape.SetEndEndingWidth( 200 );

    SHAPE_POLY_SET unshortenedBodyPlusEnding;
    SHAPE_POLY_SET shortenedBodyPlusEnding;

    shape.TransformShapeToPolygon( unshortenedBodyPlusEnding, 0, 1, ERROR_OUTSIDE );
    shape.TransformLineEndingsToPolygon( unshortenedBodyPlusEnding, 0, 1, ERROR_OUTSIDE, shape.GetWidth() );
    shape.TransformWithLineEndingsToPolygon( shortenedBodyPlusEnding, 0, 1, ERROR_OUTSIDE );

    BOOST_CHECK_GT( unshortenedBodyPlusEnding.BBox().GetRight(), 1000 );
    BOOST_CHECK_LE( shortenedBodyPlusEnding.BBox().GetRight(), 1001 );
}


BOOST_AUTO_TEST_CASE( ShortenArcForEndingsKeepsPositiveSweep )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::ARC );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 100 );
    shape.SetStartEndingWidth( 100 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 200 );
    shape.SetEndEndingWidth( 100 );

    EDA_ANGLE        startAngle( 10.0, DEGREES_T );
    EDA_ANGLE        arcAngle( 90.0, DEGREES_T );
    constexpr double radius = 1000.0;
    constexpr double radToDeg = 180.0 / M_PI;

    BOOST_CHECK( shape.ShortenArcForEndings( startAngle, arcAngle, radius, 20 ) );
    checkAngleClose( startAngle, 10.0 + 100.0 / radius * radToDeg );
    checkAngleClose( arcAngle, 90.0 - 300.0 / radius * radToDeg );
    BOOST_CHECK( arcAngle > ANGLE_0 );
}


BOOST_AUTO_TEST_CASE( ShortenArcForEndingsKeepsNegativeSweep )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::ARC );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 100 );
    shape.SetStartEndingWidth( 100 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 200 );
    shape.SetEndEndingWidth( 100 );

    EDA_ANGLE        startAngle( 170.0, DEGREES_T );
    EDA_ANGLE        arcAngle( -90.0, DEGREES_T );
    constexpr double radius = 1000.0;
    constexpr double radToDeg = 180.0 / M_PI;

    BOOST_CHECK( shape.ShortenArcForEndings( startAngle, arcAngle, radius, 20 ) );
    checkAngleClose( startAngle, 170.0 - 100.0 / radius * radToDeg );
    checkAngleClose( arcAngle, -90.0 + 300.0 / radius * radToDeg );
    BOOST_CHECK( arcAngle < ANGLE_0 );
}


BOOST_AUTO_TEST_CASE( ArcShorteningRejectsOverrun )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::ARC );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 100 );
    shape.SetStartEndingWidth( 100 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 100 );
    shape.SetEndEndingWidth( 100 );

    EDA_ANGLE startAngle( 30.0, DEGREES_T );
    EDA_ANGLE arcAngle( 10.0, DEGREES_T );

    BOOST_CHECK( !shape.ShortenArcForEndings( startAngle, arcAngle, 1000.0, 20 ) );
    checkAngleClose( startAngle, 30.0 );
    checkAngleClose( arcAngle, 0.0 );
}


BOOST_AUTO_TEST_CASE( BezierNoEndingsReturnsSource )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::BEZIER );
    shape.SetStart( { 0, 0 } );
    shape.SetBezierC1( { 300, 0 } );
    shape.SetBezierC2( { 700, 0 } );
    shape.SetEnd( { 1000, 0 } );

    std::optional<BEZIER<double>> curve = shape.ShortenedBezierCurve( 20 );

    BOOST_REQUIRE( curve );
    checkVectorClose( curve->Start, { 0, 0 } );
    checkVectorClose( curve->C1, { 300, 0 } );
    checkVectorClose( curve->C2, { 700, 0 } );
    checkVectorClose( curve->End, { 1000, 0 } );
}


BOOST_AUTO_TEST_CASE( BezierCurveTrimsBothEnds )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::BEZIER );
    shape.SetStart( { 0, 0 } );
    shape.SetBezierC1( { 300, 0 } );
    shape.SetBezierC2( { 700, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 100 );
    shape.SetStartEndingWidth( 100 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 200 );
    shape.SetEndEndingWidth( 100 );

    std::optional<BEZIER<double>> curve = shape.ShortenedBezierCurve( 20 );

    BOOST_REQUIRE( curve );
    checkVectorClose( curve->Start, { 100, 0 }, 1e-3 );
    checkVectorClose( curve->End, { 800, 0 }, 1e-3 );
}


BOOST_AUTO_TEST_CASE( BezierCurveRejectsOverrun )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::BEZIER );
    shape.SetStart( { 0, 0 } );
    shape.SetBezierC1( { 300, 0 } );
    shape.SetBezierC2( { 700, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 600 );
    shape.SetStartEndingWidth( 100 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 500 );
    shape.SetEndEndingWidth( 100 );

    BOOST_CHECK( !shape.ShortenedBezierCurve( 20 ) );
}


BOOST_AUTO_TEST_CASE( ShortenedBezierPolylineUsesShortenedCurve )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::BEZIER );
    shape.SetStart( { 0, 0 } );
    shape.SetBezierC1( { 300, 0 } );
    shape.SetBezierC2( { 700, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 100 );
    shape.SetStartEndingWidth( 100 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 200 );
    shape.SetEndEndingWidth( 100 );

    std::vector<VECTOR2D> points = shape.ShortenedBezierPolyline( 20 );

    BOOST_REQUIRE_GE( points.size(), 2 );
    checkVectorClose( points.front(), { 100, 0 }, 1e-3 );
    checkVectorClose( points.back(), { 800, 0 }, 1e-3 );
}


BOOST_AUTO_TEST_CASE( BezierPolylineRejectsOverrun )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::BEZIER );
    shape.SetStart( { 0, 0 } );
    shape.SetBezierC1( { 300, 0 } );
    shape.SetBezierC2( { 700, 0 } );
    shape.SetEnd( { 1000, 0 } );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 600 );
    shape.SetStartEndingWidth( 100 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 500 );
    shape.SetEndEndingWidth( 100 );

    BOOST_CHECK( shape.ShortenedBezierPolyline( 20 ).empty() );
}


// Minimal concrete RENDER_SETTINGS; STROKE_PARAMS::Stroke only reads the dash/gap ratios.
class STROKE_TEST_RENDER_SETTINGS : public KIGFX::RENDER_SETTINGS
{
public:
    KIGFX::COLOR4D GetColor( const KIGFX::VIEW_ITEM* aItem, int aLayer ) const override
    {
        return KIGFX::COLOR4D::BLACK;
    }

    const KIGFX::COLOR4D& GetBackgroundColor() const override { return m_color; }
    void                  SetBackgroundColor( const KIGFX::COLOR4D& aColor ) override { m_color = aColor; }
    const KIGFX::COLOR4D& GetGridColor() override { return m_color; }
    const KIGFX::COLOR4D& GetCursorColor() override { return m_color; }

private:
    KIGFX::COLOR4D m_color = KIGFX::COLOR4D::BLACK;
};


// A dashed Bezier with endings must still be stroked as ONE chain: loose segments would
// restart the dash pattern at every tessellation vertex (issue #25110) and draw near-solid.
BOOST_AUTO_TEST_CASE( StrokingShortenedBezierDashes )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::BEZIER );
    shape.SetStart( { 0, 0 } );
    shape.SetBezierC1( { 30000, 20000 } );
    shape.SetBezierC2( { 70000, -20000 } );
    shape.SetEnd( { 100000, 0 } );
    shape.SetWidth( 100 );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 2000 );
    shape.SetStartEndingWidth( 1000 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetEndEndingLength( 3000 );
    shape.SetEndEndingWidth( 1000 );

    std::vector<SHAPE*> shapes = shape.MakeEffectiveShapesForStroking( shape.GetWidth() );

    // The shortened body must come back as a single chain.
    BOOST_REQUIRE_EQUAL( shapes.size(), 1 );

    const SHAPE_LINE_CHAIN* chain = dynamic_cast<const SHAPE_LINE_CHAIN*>( shapes.front() );
    BOOST_REQUIRE( chain );
    BOOST_REQUIRE_GE( chain->SegmentCount(), 2 );

    double chainLength = 0.0;

    for( int ii = 0; ii < chain->SegmentCount(); ++ii )
        chainLength += VECTOR2D( chain->CSegment( ii ).B - chain->CSegment( ii ).A ).EuclideanNorm();

    // Stroke it dashed and glue touching pieces back into runs (a dash spanning a vertex
    // arrives as several touching pieces).
    STROKE_TEST_RENDER_SETTINGS settings;
    std::vector<SEG>            pieces;

    STROKE_PARAMS::Stroke( chain, LINE_STYLE::DASH, shape.GetWidth(), &settings,
                           [&pieces]( const VECTOR2I& a, const VECTOR2I& b )
                           {
                               pieces.emplace_back( a, b );
                           } );

    std::vector<double> runs;
    double              drawn = 0.0;

    for( size_t ii = 0; ii < pieces.size(); )
    {
        double   length = VECTOR2D( pieces[ii].B - pieces[ii].A ).EuclideanNorm();
        VECTOR2I end = pieces[ii].B;
        size_t   jj = ii + 1;

        while( jj < pieces.size() && pieces[jj].A == end )
        {
            length += VECTOR2D( pieces[jj].B - pieces[jj].A ).EuclideanNorm();
            end = pieces[jj].B;
            jj++;
        }

        runs.push_back( length );
        drawn += length;
        ii = jj;
    }

    BOOST_REQUIRE_MESSAGE( runs.size() > 5, "expected many dashes, got " << runs.size() << " run(s)" );

    // If the pattern restarted at every tessellation vertex the curve would come out nearly
    // solid; the drawn length must instead match the dash/gap duty cycle.
    double dash = settings.GetDashLength( shape.GetWidth() );
    double gap = settings.GetGapLength( shape.GetWidth() );
    double expected = chainLength * dash / ( dash + gap );

    BOOST_CHECK_MESSAGE( std::abs( drawn - expected ) < 0.05 * chainLength,
                         "dashes should cover " << expected << " IU of the " << chainLength
                                                << " IU shortened body, they cover " << drawn );

    deleteShapes( shapes );
}


BOOST_AUTO_TEST_CASE( PolyShorteningTrimsEnds )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::POLY );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 100 );
    shape.SetStartEndingWidth( 100 );
    shape.SetEndEndingStyle( LINE_ENDING_STYLE::CIRCLE );
    shape.SetEndEndingLength( 200 );
    shape.SetEndEndingWidth( 200 );

    VECTOR2D first( 0, 0 );
    VECTOR2D second( 1000, 0 );
    VECTOR2D penultimate( 1000, 1000 );
    VECTOR2D last( 1000, 2000 );

    BOOST_CHECK( shape.ShortenPolyForEndings( first, last, second, penultimate, 20 ) );

    checkVectorClose( first, { 100, 0 } );
    checkVectorClose( last, { 1000, 1900 } );
}


BOOST_AUTO_TEST_CASE( ShortenPolyForEndingsLeavesOpenArrowBody )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::POLY );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW_OPEN );
    shape.SetStartEndingLength( 500 );
    shape.SetStartEndingWidth( 500 );

    VECTOR2D first( 0, 0 );
    VECTOR2D second( 1000, 0 );
    VECTOR2D penultimate( 1000, 1000 );
    VECTOR2D last( 1000, 2000 );

    BOOST_CHECK( shape.ShortenPolyForEndings( first, last, second, penultimate, 20 ) );

    checkVectorClose( first, { 0, 0 } );
    checkVectorClose( last, { 1000, 2000 } );
}


BOOST_AUTO_TEST_CASE( PolyShorteningRejectsOverrun )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::POLY );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 200 );
    shape.SetStartEndingWidth( 100 );

    VECTOR2D first( 0, 0 );
    VECTOR2D second( 100, 0 );
    VECTOR2D penultimate( 1000, 0 );
    VECTOR2D last( 1100, 0 );

    BOOST_CHECK( !shape.ShortenPolyForEndings( first, last, second, penultimate, 20 ) );
    checkVectorClose( first, { 0, 0 } );
    checkVectorClose( last, { 0, 0 } );
}


BOOST_AUTO_TEST_CASE( BodyPolyPointsShortenFirstOpenOutline )
{
    EDA_SHAPE_MOCK shape( SHAPE_T::POLY );
    shape.SetStartEndingStyle( LINE_ENDING_STYLE::ARROW );
    shape.SetStartEndingLength( 100 );
    shape.SetStartEndingWidth( 100 );

    SHAPE_LINE_CHAIN outline;
    outline.Append( VECTOR2I( 0, 0 ) );
    outline.Append( VECTOR2I( 1000, 0 ) );
    outline.Append( VECTOR2I( 1000, 1000 ) );
    outline.SetClosed( false );

    std::vector<VECTOR2I> pts;

    BOOST_CHECK( shape.GetShortenedBodyPolyPoints( outline, 0, pts, 20 ) );
    BOOST_REQUIRE_EQUAL( pts.size(), 3 );
    checkVectorEqual( pts.front(), { 100, 0 } );
    checkVectorEqual( pts.back(), { 1000, 1000 } );

    BOOST_CHECK( shape.GetShortenedBodyPolyPoints( outline, 1, pts, 20 ) );
    BOOST_REQUIRE_EQUAL( pts.size(), 3 );
    checkVectorEqual( pts.front(), { 0, 0 } );
    checkVectorEqual( pts.back(), { 1000, 1000 } );

    outline.SetClosed( true );

    BOOST_CHECK( shape.GetShortenedBodyPolyPoints( outline, 0, pts, 20 ) );
    BOOST_REQUIRE_EQUAL( pts.size(), 3 );
    checkVectorEqual( pts.front(), { 0, 0 } );
    checkVectorEqual( pts.back(), { 1000, 1000 } );
}

BOOST_AUTO_TEST_SUITE_END()
