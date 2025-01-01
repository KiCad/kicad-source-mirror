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

#include <eda_shape.h>
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

BOOST_AUTO_TEST_SUITE_END()
