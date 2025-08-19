/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/shape_arc.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>

#include <qa_utils/geometry/geometry.h>
#include <qa_utils/numeric.h>

#include "geom_test_utils.h"

BOOST_AUTO_TEST_SUITE( ShapeArc )

/**
 * All properties of an arc (depending on how it's constructed, some of these
 * might be the same as the constructor params)
 */
struct ARC_PROPERTIES
{
    VECTOR2I m_center_point;
    VECTOR2I m_start_point;
    VECTOR2I m_end_point;
    double   m_center_angle;
    double   m_start_angle;
    double   m_end_angle;
    int      m_radius;
    BOX2I    m_bbox;
};

/**
 * Check a #SHAPE_ARC against a given set of geometric properties
 * @param aArc Arc to test
 * @param aProps Properties to test against
 * @param aSynErrIU Permitted error for synthetic points and dimensions (currently radius and center)
 */
static void CheckArcGeom( const SHAPE_ARC& aArc, const ARC_PROPERTIES& aProps, const int aSynErrIU = 1 )
{
    // Angular error - note this can get quite large for very small arcs,
    // as the integral position rounding has a relatively greater effect
    const double angle_tol_deg = 2.0;

    // Position error - rounding to nearest integer
    const int pos_tol = 1;

    BOOST_CHECK_PREDICATE( KI_TEST::IsVecWithinTol<VECTOR2I>,
            ( aProps.m_start_point )( aProps.m_start_point )( pos_tol ) );

    BOOST_CHECK_PREDICATE( KI_TEST::IsVecWithinTol<VECTOR2I>,
            ( aArc.GetP1() )( aProps.m_end_point )( pos_tol ) );

    BOOST_CHECK_PREDICATE( KI_TEST::IsVecWithinTol<VECTOR2I>,
            ( aArc.GetCenter() )( aProps.m_center_point )( aSynErrIU ) );

    BOOST_CHECK_PREDICATE( KI_TEST::IsWithinWrapped<double>,
            ( aArc.GetCentralAngle().AsDegrees() )( aProps.m_center_angle )( 360.0 )( angle_tol_deg ) );

    BOOST_CHECK_PREDICATE( KI_TEST::IsWithinWrapped<double>,
            ( aArc.GetStartAngle().AsDegrees() )( aProps.m_start_angle )( 360.0 )( angle_tol_deg ) );

    BOOST_CHECK_PREDICATE( KI_TEST::IsWithinWrapped<double>,
            ( aArc.GetEndAngle().AsDegrees() )( aProps.m_end_angle )( 360.0 )( angle_tol_deg ) );

    BOOST_CHECK_PREDICATE( KI_TEST::IsWithin<double>,
            ( aArc.GetRadius() )( aProps.m_radius )( aSynErrIU ) );

    // Angle normalization contracts
    BOOST_TEST( aArc.GetStartAngle().AsDegrees() >= 0.0 );
    BOOST_TEST( aArc.GetStartAngle().AsDegrees() <= 360.0 );

    BOOST_TEST( aArc.GetEndAngle().AsDegrees() >= 0.0 );
    BOOST_TEST( aArc.GetEndAngle().AsDegrees() <= 360.0 );

    BOOST_TEST( aArc.GetCentralAngle().AsDegrees() >= -360.0 );
    BOOST_TEST( aArc.GetCentralAngle().AsDegrees() <= 360.0 );

    /// Check the chord agrees
    const SEG chord = aArc.GetChord();

    BOOST_CHECK_PREDICATE( KI_TEST::IsVecWithinTol<VECTOR2I>,
            ( chord.A )( aProps.m_start_point )( pos_tol ) );

    BOOST_CHECK_PREDICATE( KI_TEST::IsVecWithinTol<VECTOR2I>,
            ( chord.B )( aProps.m_end_point )( pos_tol ) );

    /// All arcs are solid
    BOOST_CHECK_EQUAL( aArc.IsSolid(), true );

    BOOST_CHECK_PREDICATE( KI_TEST::IsBoxWithinTol<BOX2I>,
            ( aArc.BBox() )( aProps.m_bbox )( pos_tol ) );

    /// Collisions will be checked elsewhere.
}


/**
 * Check an arcs geometry and other class functions
 * @param aArc Arc to test
 * @param aProps Properties to test against
 * @param aSynErrIU Permitted error for synthetic points and dimensions (currently radius and center)
 */
static void CheckArc( const SHAPE_ARC& aArc, const ARC_PROPERTIES& aProps, const int aSynErrIU = 1 )
{
    // Check the original arc
    CheckArcGeom( aArc, aProps, aSynErrIU );

    // Test the Clone function (also tests copy-ctor)
    std::unique_ptr<SHAPE> new_shape{ aArc.Clone() };

    BOOST_REQUIRE_EQUAL( new_shape->Type(), SH_ARC );

    SHAPE_ARC* new_arc = dynamic_cast<SHAPE_ARC*>( new_shape.get() );

    BOOST_REQUIRE( new_arc != nullptr );

    /// Should have identical geom props
    CheckArcGeom( *new_arc, aProps, aSynErrIU );
}

/**
 * Check correct handling of filter strings (as used by WX)
 */
BOOST_AUTO_TEST_CASE( NullCtor )
{
    auto arc = SHAPE_ARC();

    BOOST_CHECK_EQUAL( arc.GetWidth(), 0 );

    static ARC_PROPERTIES null_props{
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        0,
        0,
        0,
        0,
    };

    CheckArc( arc, null_props );
}


/**
 * Info to set up an arc by start, mid and end points
 */
struct ARC_START_MID_END
{
    VECTOR2I m_start_point;
    VECTOR2I m_mid_point;
    VECTOR2I m_end_point;
};

struct ARC_SME_CASE : public KI_TEST::NAMED_CASE
{
    /// Geom of the arc
    ARC_START_MID_END m_geom;

    /// Arc line width
    int m_width;

    /// Expected properties
    ARC_PROPERTIES m_properties;
};


static const std::vector<ARC_SME_CASE> arc_sme_cases = {
    {
            "S(-100,0), M(0,100), E(100,0)",
            {
                    { -100, 0 },
                    { 0, 100 },
                    { 100, 0 },
            },
            0,
            {
                    { 0, 0 },
                    { -100, 0 },
                    { 100, 0 },
                    180,
                    180,
                    0,
                    100,
                    { { -100, 0 }, { 200, 100 } },
            },
    },
    {
            "S(100,0), M(0,100), E(-100,0) (reversed)",
            {
                    { 100, 0 },
                    { 0, 100 },
                    { -100, 0 },
            },
            0,
            {
                    { 0, 0 },
                    { 100, 0 },
                    { -100, 0 },
                    -180,
                    0,
                    180,
                    100,
                    { { -100, 0 }, { 200, 100 } },
            },
    },
    {
            // This data has a midpoint not exactly at the midway point of the arc.
            // This should be corrected by the constructor.
            // The mid point should be at about (-71, -71) for a 270 degree arc, with the
            // bottom right quadrant open.
            "S(100,0), M(-100,0), E(0,100) (bad midpoint)",
            {
                    { 100, 0 },
                    { -100, 0 },
                    { 0, 100 },
            },
            0,
            {
                    { 0, 0 },
                    { 100, 0 },
                    { 0, 100 },
                    -270,
                    0,
                    90,
                    100,
                    { { -100, -100 }, { 200, 200 } },
            },
    }
};


BOOST_DATA_TEST_CASE( BasicSMEGeom, boost::unit_test::data::make( arc_sme_cases ), c )
{
    const SHAPE_ARC this_arc{
        c.m_geom.m_start_point,
        c.m_geom.m_mid_point,
        c.m_geom.m_end_point,
        c.m_width,
    };

    CheckArc( this_arc, c.m_properties );
}


/**
 * Info to set up an arc by centre, start point and angle
 */
struct ARC_CENTRE_PT_ANGLE
{
    VECTOR2I m_center_point;
    VECTOR2I m_start_point;
    double   m_center_angle;
};


struct ARC_CPA_CASE : public KI_TEST::NAMED_CASE
{
    /// Geom of the arc
    ARC_CENTRE_PT_ANGLE m_geom;

    /// Arc line width
    int m_width;

    /// Expected properties
    ARC_PROPERTIES m_properties;
};


static const std::vector<ARC_CPA_CASE> arc_cases = {
    {
            "C(0,0) 114 + 360 degree",
            {
                    { 0, 0 },
                    { -306451, 687368 },
                    360,
            },
            0,
            {
                    { 0, 0 },
                    { -306451, 687368 },
                    { -306451, 687368 },
                    360,
                    113.95929,
                    113.95929,
                    752587,
                    { { -752587, -752587 }, { 1505174, 1505174 } },
            },
    },
    {
            "C(0,0) 180 + 360 degree",
            {
                    { 0, 0 },
                    { -100, 0 },
                    360,
            },
            0,
            {
                    { 0, 0 },
                    { -100, 0 },
                    { -100, 0 },
                    360,
                    180,
                    180,
                    100,
                    { { -100, -100 }, { 200, 200 } },
            },
    },
    {
            "C(0,0) 180 + 90 degree",
            {
                    { 0, 0 },
                    { -100, 0 },
                    90,
            },
            0,
            {
                    { 0, 0 },
                    { -100, 0 },
                    { 0, -100 },
                    90,
                    180,
                    270,
                    100,
                    { { -100, -100 }, { 100, 100 } },
            },
    },
    {
            "C(100,200)  0 - 30 degree",
            {
                    { 100, 200 },
                    { 300, 200 },
                    -30,
            },
            0,
            {
                    { 100, 200 },
                    { 300, 200 },
                    { 273, 100 }, // 200 * sin(30) = 100, 200* cos(30) = 173
                    -30,
                    0,
                    330,
                    200,
                    { { 273, 100 }, { 27, 100 } },
            },
    },
    {
            // This is a "fan shape" which includes the top quadrant point,
            // so it exercises the bounding box code (centre and end points
            // do not contain the top quadrant)
            "C(0,0) 30 + 120 degree",
            {
                    { 0, 0 },
                    { 17320, 10000 },
                    120,
            },
            0,
            {
                    { 0, 0 },
                    { 17320, 10000 },
                    { -17320, 10000 }, // 200 * sin(30) = 100, 200* cos(30) = 173
                    120,
                    30,
                    150,
                    20000,
                    // bbox defined by: centre, top quadrant point, two endpoints
                    { { -17320, 10000 }, { 17320 * 2, 10000 } },
            },
    },
    {
            // An arc that covers three quadrant points (L/R, bottom)
            "C(0,0) 150 + 240 degree",
            {
                    { 0, 0 },
                    { -17320, 10000 },
                    240,
            },
            0,
            {
                    { 0, 0 },
                    { -17320, 10000 },
                    { 17320, 10000 },
                    240,
                    150,
                    30,
                    20000,
                    // bbox defined by: L/R quads, bottom quad and start/end
                    { { -20000, -20000 }, { 40000, 30000 } },
            },
    },
    {
            // Same as above but reverse direction
            "C(0,0) 30 - 300 degree",
            {
                    { 0, 0 },
                    { 17320, 10000 },
                    -240,
            },
            0,
            {
                    { 0, 0 },
                    { 17320, 10000 },
                    { -17320, 10000 },
                    -240,
                    30,
                    150,
                    20000,
                    // bbox defined by: L/R quads, bottom quad and start/end
                    { { -20000, -20000 }, { 40000, 30000 } },
            },
    },
};


BOOST_DATA_TEST_CASE( BasicCPAGeom, boost::unit_test::data::make( arc_cases ), c )
{
    const SHAPE_ARC this_arc{
        c.m_geom.m_center_point,
        c.m_geom.m_start_point,
        EDA_ANGLE( c.m_geom.m_center_angle, DEGREES_T ),
        c.m_width,
    };

    CheckArc( this_arc, c.m_properties );
}



/**
 * Info to set up an arc by tangent to two segments and a radius
 */
struct ARC_TAN_TAN_RADIUS
{
    SEG m_segment_1;
    SEG m_segment_2;
    int m_radius;
};


struct ARC_TTR_CASE : public KI_TEST::NAMED_CASE
{
    /// Geom of the arc
    ARC_TAN_TAN_RADIUS m_geom;

    /// Arc line width
    int m_width;

    /// Expected properties
    ARC_PROPERTIES m_properties;
};


static const std::vector<ARC_TTR_CASE> arc_ttr_cases = {
    {
            "90 degree segments intersecting",
            {
                    { 0, 0, 0, 1000 },
                    { 0, 0, 1000, 0 },
                    1000,
            },
            0,
            {
                    { 1000, 1000 },
                    { 0, 1000 },    //start on first segment
                    { 1000, 0 },    //end on second segment
                    90,             //positive angle due to start/end
                    180,
                    270,
                    1000,
                    { { 0, 0 }, { 1000, 1000 } },
            }
    },
    {
            "45 degree segments intersecting",
            {
                    { 0, 0, 0, 1000 },
                    { 0, 0, 1000, 1000 },
                    1000,
            },
            0,
            {
                    { 1000, 2414 },
                    { 0, 2414 },       //start on first segment
                    { 1707, 1707 },    //end on second segment
                    135,               //positive angle due to start/end
                    180,
                    315,
                    1000,
                    { { 0, 1414 }, { 1707, 1000 } },
            }
    },
    {
            "135 degree segments intersecting",
            {
                    { 0, 0, 0, 1000 },
                    { 0, 0, 1000, -1000 },
                    1000,
            },
            0,
            {
                    { 1000, 414 },
                    { 0, 414 },       //start on first segment ( radius * tan(45 /2) )
                    { 293, -293 },    //end on second segment (radius * 1-cos(45)) )
                    45,               //positive angle due to start/end
                    180,
                    225,
                    1000,
                    { { 0, -293 }, { 293, 707 } },
            }
    }


};


BOOST_DATA_TEST_CASE( BasicTTRGeom, boost::unit_test::data::make( arc_ttr_cases ), c )
{
    for( int testCase = 0; testCase < 8; ++testCase )
    {
        SEG            seg1  = c.m_geom.m_segment_1;
        SEG            seg2  = c.m_geom.m_segment_2;
        ARC_PROPERTIES props = c.m_properties;

        if( testCase > 3 )
        {
            //Swap input segments.
            seg1 = c.m_geom.m_segment_2;
            seg2 = c.m_geom.m_segment_1;

            //The result should swap start and end points and invert the angles:
            props.m_end_point    = c.m_properties.m_start_point;
            props.m_start_point  = c.m_properties.m_end_point;
            props.m_start_angle  = c.m_properties.m_end_angle;
            props.m_end_angle    = c.m_properties.m_start_angle;
            props.m_center_angle = -c.m_properties.m_center_angle;
        }

        //Test all combinations of start and end points for the segments
        if( ( testCase % 4 ) == 1 || ( testCase % 4 ) == 3 )
        {
            //Swap start and end points for seg1
            VECTOR2I temp = seg1.A;
            seg1.A = seg1.B;
            seg1.B = temp;
        }

        if( ( testCase % 4 ) == 2 || ( testCase % 4 ) == 3 )
        {
            //Swap start and end points for seg2
            VECTOR2I temp = seg2.A;
            seg2.A        = seg2.B;
            seg2.B        = temp;
        }

        const auto this_arc = SHAPE_ARC{ seg1, seg2,
            c.m_geom.m_radius, c.m_width };

        // Error of 4 IU permitted for the center and radius calculation
        CheckArc( this_arc, props, SHAPE_ARC::MIN_PRECISION_IU );
    }
}


/**
 * Info to set up an arc start, end and center
 */
struct ARC_START_END_CENTER
{
    VECTOR2I m_start;
    VECTOR2I m_end;
    VECTOR2I m_center;
};


struct ARC_SEC_CASE : public KI_TEST::NAMED_CASE
{
    /// Geom of the arc
    ARC_START_END_CENTER m_geom;

    /// clockwise or anti-clockwise?
    bool m_clockwise;

    /// Expected mid-point of the arc
    VECTOR2I m_expected_mid;
};



static const std::vector<ARC_SEC_CASE> arc_sec_cases = {
    { "180 deg, clockwise", { { 100, 0 }, { 0, 0 }, { 50, 0 } }, true, { 50, -50 } },
    { "180 deg, anticlockwise", { { 100, 0 }, { 0, 0 }, { 50, 0 } }, false, { 50, 50 } },
    { "180 deg flipped, clockwise", { { 0, 0 }, { 100, 0 }, { 50, 0 } }, true, { 50, 50 } },
    { "180 deg flipped, anticlockwise", { { 0, 0 }, { 100, 0 }, { 50, 0 } }, false, { 50, -50 } },
    { "90 deg, clockwise", { { -100, 0 }, { 0, 100 }, { 0, 0 } }, true, { -71, 71 } },
    { "90 deg, anticlockwise", { { -100, 0 }, { 0, 100 }, { 0, 0 } }, false, { 71, -71 } },
};


BOOST_DATA_TEST_CASE( BasicSECGeom, boost::unit_test::data::make( arc_sec_cases ), c )
{
    VECTOR2I start = c.m_geom.m_start;
    VECTOR2I end = c.m_geom.m_end;
    VECTOR2I center = c.m_geom.m_center;
    bool     cw = c.m_clockwise;

    SHAPE_ARC this_arc;
    this_arc.ConstructFromStartEndCenter( start, end, center, cw );

    BOOST_CHECK_EQUAL( this_arc.GetArcMid(), c.m_expected_mid );
}


struct ARC_CICLE_COLLIDE_CASE : public KI_TEST::NAMED_CASE
{
    ARC_START_MID_END   m_geom;
    int                 m_arc_clearance;
    VECTOR2I            m_circle_center;
    int                 m_circle_radius;
    bool                m_exp_result;
    int                 m_exp_distance;
};

static const std::vector<ARC_CICLE_COLLIDE_CASE> arc_circle_collide_cases = {
    { " Issue 20336, large arc", { { 183000000, 65710001}, {150496913, 147587363},{116291153, 66406583}}, 2000000 / 2, {116300000, 133100000}, 300000, true, 53319 }
};


BOOST_DATA_TEST_CASE( CollideCircle, boost::unit_test::data::make( arc_circle_collide_cases ), c )
{
    SHAPE_ARC arc( c.m_geom.m_start_point, c.m_geom.m_mid_point, c.m_geom.m_end_point, 0 );
    SHAPE_CIRCLE circle( c.m_circle_center, c.m_circle_radius );

    // Test a zero width arc (distance should equal the clearance)
    BOOST_TEST_CONTEXT( "Test Clearance" )
    {
        int dist = -1;
        BOOST_CHECK_EQUAL( arc.Collide( &circle, c.m_arc_clearance, &dist ), c.m_exp_result );
        BOOST_CHECK_EQUAL( dist, c.m_exp_distance );
    }

    // Test by changing the width of the arc (distance should equal zero)
    BOOST_TEST_CONTEXT( "Test Width" )
    {
        int dist = -1;
        arc.SetWidth( c.m_arc_clearance * 2 );
        BOOST_CHECK_EQUAL( arc.Collide( &circle, 0, &dist ), c.m_exp_result );

        if( c.m_exp_result )
            BOOST_CHECK_EQUAL( dist, 0 );
        else
            BOOST_CHECK_EQUAL( dist, -1 );
    }
}


struct ARC_PT_COLLIDE_CASE : public KI_TEST::NAMED_CASE
{
    ARC_CENTRE_PT_ANGLE m_geom;
    int                 m_arc_clearance;
    VECTOR2I            m_point;
    bool                m_exp_result;
    int                 m_exp_distance;
};


static const std::vector<ARC_PT_COLLIDE_CASE> arc_pt_collide_cases = {
    { " 270deg, 0 cl, 0   deg    ", { { 0, 0 }, { 100, 0 }, 270.0 }, 0, { 100, 0 }, true, 0 },
    { " 270deg, 0 cl, 90  deg    ", { { 0, 0 }, { 100, 0 },  270.0 }, 0, { 0, 100 }, true, 0 },
    { " 270deg, 0 cl, 180 deg    ", { { 0, 0 }, { 100, 0 },  270.0 }, 0, { -100, 0 }, true, 0 },
    { " 270deg, 0 cl, 270 deg    ", { { 0, 0 }, { 100, 0 },  270.0 }, 0, { 0, -100 }, true, 0 },
    { " 270deg, 0 cl, 45  deg    ", { { 0, 0 }, { 100, 0 },  270.0 }, 0, { 71, 71 }, true, 0 },
    { " 270deg, 0 cl, -45 deg    ", { { 0, 0 }, { 100, 0 },  270.0 }, 0, { 71, -71 }, false, -1 },
    { "-270deg, 0 cl, 0   deg    ", { { 0, 0 }, { 100, 0 }, -270.0 }, 0, { 100, 0 }, true, 0 },
    { "-270deg, 0 cl, 90  deg    ", { { 0, 0 }, { 100, 0 }, -270.0 }, 0, { 0, 100 }, true, 0 },
    { "-270deg, 0 cl, 180 deg    ", { { 0, 0 }, { 100, 0 }, -270.0 }, 0, { -100, 0 }, true, 0 },
    { "-270deg, 0 cl, 270 deg    ", { { 0, 0 }, { 100, 0 }, -270.0 }, 0, { 0, -100 }, true, 0 },
    { "-270deg, 0 cl, 45  deg    ", { { 0, 0 }, { 100, 0 }, -270.0 }, 0, { 71, 71 }, false, -1 },
    { "-270deg, 0 cl, -45 deg    ", { { 0, 0 }, { 100, 0 }, -270.0 }, 0, { 71, -71 }, true, 0 },
    { " 270deg, 5 cl, 0   deg, 5 pos X", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 105, 0 }, true, 5 },
    { " 270deg, 5 cl, 0  deg, 5 pos Y", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 100, -5 }, true, 5 },
    { " 270deg, 5 cl, 90  deg, 5 pos", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 0, 105 }, true, 5 },
    { " 270deg, 5 cl, 180 deg, 5 pos", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { -105, 0 }, true, 5 },
    { " 270deg, 5 cl, 270 deg, 5 pos", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 0, -105 }, true, 5 },
    { " 270deg, 5 cl, 0   deg, 5 neg", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 105, 0 }, true, 5 },
    { " 270deg, 5 cl, 90  deg, 5 neg", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 0, 105 }, true, 5 },
    { " 270deg, 5 cl, 180 deg, 5 neg", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { -105, 0 }, true, 5 },
    { " 270deg, 5 cl, 270 deg, 5 neg", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 0, -105 }, true, 5 },
    { " 270deg, 5 cl, 45  deg, 5 pos", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 74, 75 }, true, 5 }, // 74.246, -74.246
    { " 270deg, 5 cl, -45 deg, 5 pos", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 74, -75 }, false, -1 }, //74.246, -74.246
    { " 270deg, 5 cl, 45  deg, 5 neg", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 67, 67 }, true, 5 }, // 67.17, 67.17
    { " 270deg, 5 cl, -45 deg, 5 neg", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 67, -67 }, false, -1 }, // 67.17, -67.17
    { " 270deg, 4 cl, 0   deg pos", { { 0, 0 }, { 100, 0 },  270.0 }, 4, { 105, 0 }, false, -1 },
    { " 270deg, 4 cl, 90  deg pos", { { 0, 0 }, { 100, 0 },  270.0 }, 4, { 0, 105 }, false, -1 },
    { " 270deg, 4 cl, 180 deg pos", { { 0, 0 }, { 100, 0 },  270.0 }, 4, { -105, 0 }, false, -1 },
    { " 270deg, 4 cl, 270 deg pos", { { 0, 0 }, { 100, 0 },  270.0 }, 4, { 0, -105 }, false, -1 },
    { "  90deg, 0 cl,   0 deg    ", { { 0, 0 }, { 71, -71 },  90.0 }, 0, { 71, -71 }, true, 0 },
    { "  90deg, 0 cl,  45 deg    ", { { 0, 0 }, { 71, -71 },  90.0 }, 0, { 100, 0 }, true, 0 },
    { "  90deg, 0 cl,  90 deg    ", { { 0, 0 }, { 71, -71 },  90.0 }, 0, { 71, 71 }, true, 0 },
    { "  90deg, 0 cl, 135 deg    ", { { 0, 0 }, { 71, -71 },  90.0 }, 0, { 0, -100 }, false, -1 },
    { "  90deg, 0 cl, -45 deg    ", { { 0, 0 }, { 71, -71 },  90.0 }, 0, { 0,  100 }, false, -1 },
    { " -90deg, 0 cl,   0 deg    ", { { 0, 0 }, { 71,  71 }, -90.0 }, 0, { 71, -71 }, true, 0 },
    { " -90deg, 0 cl,  45 deg    ", { { 0, 0 }, { 71,  71 }, -90.0 }, 0, { 100, 0 }, true, 0 },
    { " -90deg, 0 cl,  90 deg    ", { { 0, 0 }, { 71,  71 }, -90.0 }, 0, { 71, 71 }, true, 0 },
    { " -90deg, 0 cl, 135 deg    ", { { 0, 0 }, { 71,  71 }, -90.0 }, 0, { 0, -100 }, false, -1 },
    { " -90deg, 0 cl, -45 deg    ", { { 0, 0 }, { 71,  71 }, -90.0 }, 0, { 0,  100 }, false, -1 },
    { "issue 11358 collide",
      { { 119888000, 60452000 }, { 120904000, 60452000 }, 360.0 },
      0,
      { 120395500, 59571830 },
      true,
      0 },
    { "issue 11358 dist",
      { { 119888000, 60452000 }, { 120904000, 60452000 }, 360.0 },
      100,
      { 118872050, 60452000 },
      true,
      50 },
};


BOOST_DATA_TEST_CASE( CollidePt, boost::unit_test::data::make( arc_pt_collide_cases ), c )
{
    SHAPE_ARC arc( c.m_geom.m_center_point, c.m_geom.m_start_point,
                    EDA_ANGLE( c.m_geom.m_center_angle, DEGREES_T ) );

    // Test a zero width arc (distance should equal the clearance)
    BOOST_TEST_CONTEXT( "Test Clearance" )
    {
        int dist = -1;
        BOOST_CHECK_EQUAL( arc.Collide( c.m_point, c.m_arc_clearance, &dist ),
                            c.m_exp_result );
        BOOST_CHECK_EQUAL( dist, c.m_exp_distance );
    }

    // Test by changing the width of the arc (distance should equal zero)
    BOOST_TEST_CONTEXT( "Test Width" )
    {
        int dist = -1;
        arc.SetWidth( c.m_arc_clearance * 2 );
        BOOST_CHECK_EQUAL( arc.Collide( c.m_point, 0, &dist ), c.m_exp_result );

        if( c.m_exp_result )
            BOOST_CHECK_EQUAL( dist, 0 );
        else
            BOOST_CHECK_EQUAL( dist, -1 );
    }
}

struct ARC_SEG_COLLIDE_CASE : public KI_TEST::NAMED_CASE
{
    ARC_CENTRE_PT_ANGLE m_geom;
    int                 m_arc_clearance;
    SEG                 m_seg;
    bool                m_exp_result;
    int                 m_exp_distance;
    VECTOR2I            m_collide_point;
};


static const std::vector<ARC_SEG_COLLIDE_CASE> arc_seg_collide_cases = {
    { "0   deg    ", { { 0, 0 }, { 100, 0 }, 270.0 }, 0, { { 100, 0 }, { 50, 0 } }, true, 0, { 100, 0 } },
    { "90  deg    ", { { 0, 0 }, { 100, 0 }, 270.0 }, 0, { { 0, 100 }, { 0, 50 } }, true, 0, { 0, 100 } },
    { "180 deg    ", { { 0, 0 }, { 100, 0 }, 270.0 }, 0, { { -100, 0 }, { -50, 0 } }, true, 0, { -100, 0 } },
    { "270 deg    ", { { 0, 0 }, { 100, 0 }, 270.0 }, 0, { { 0, -100 }, { 0, -50 } }, true, 0, { 0, -100 } },
    { "45  deg    ", { { 0, 0 }, { 100, 0 }, 270.0 }, 0, { { 71, 71 }, { 35, 35 } }, true, 0, { 70, 70 } },
    { "-45 deg    ", { { 0, 0 }, { 100, 0 }, 270.0 }, 0, { { 71, -71 }, { 35, -35 } }, false, -1, { 0, 0 } },
    { "seg inside arc start", { { 0, 0 }, { 71, -71 }, 90.0 },
                        10, { { 90, 0 }, { -35, 0 } }, true, 10, { 100, 0 } },
    { "seg inside arc end", { { 0, 0 }, { 71, -71  }, 90.0 },
                        10, { { -35, 0 }, { 90, 0 } }, true, 10, { 100, 0 } },
    { "large diameter arc", { { 172367922, 82282076 }, { 162530000, 92120000 }, -45.0 },
                        433300, { { 162096732, 92331236 }, { 162096732, 78253268 } }, true, 433268, { 162530000, 92120000 } },
    { "upside down collide", { { 26250000, 16520000 }, { 28360000, 16520000 }, 90.0 },
                        0, { { 27545249, 18303444 }, { 27545249, 18114500 } }, true, 0, { 27545249, 18185662 } }
};


BOOST_DATA_TEST_CASE( CollideSeg, boost::unit_test::data::make( arc_seg_collide_cases ), c )
{
    SHAPE_ARC arc( c.m_geom.m_center_point, c.m_geom.m_start_point,
                    EDA_ANGLE( c.m_geom.m_center_angle, DEGREES_T ) );

    // Test a zero width arc (distance should equal the clearance)
    BOOST_TEST_CONTEXT( "Test Clearance" )
    {
        int dist = -1;
        BOOST_CHECK_EQUAL( arc.Collide( c.m_seg, c.m_arc_clearance, &dist ),
                            c.m_exp_result );
        BOOST_CHECK_EQUAL( dist, c.m_exp_distance );
    }

    // Test by changing the width of the arc (distance should equal zero)
    BOOST_TEST_CONTEXT( "Test Width" )
    {
        int dist = -1;
        arc.SetWidth( c.m_arc_clearance * 2 );
        BOOST_CHECK_EQUAL( arc.Collide( c.m_seg, 0, &dist ), c.m_exp_result );

        if( c.m_exp_result )
            BOOST_CHECK_EQUAL( dist, 0 );
        else
            BOOST_CHECK_EQUAL( dist, -1 );
    }

    BOOST_TEST_CONTEXT( "Test Collide Point" )
    {
        VECTOR2I collide_point;
        int dist = -1;

        if( c.m_exp_result )
        {
            arc.Collide( c.m_seg, c.m_arc_clearance, &dist, &collide_point );
            BOOST_CHECK_EQUAL( collide_point, c.m_collide_point );
        }
    }
}

struct ARC_DATA_MM
{
    // Coordinates and dimensions in millimeters
    double m_center_x;
    double m_center_y;
    double m_start_x;
    double m_start_y;
    double m_center_angle;
    double m_width;

    SHAPE_ARC GenerateArc() const
    {
        SHAPE_ARC arc( VECTOR2D( pcbIUScale.mmToIU( m_center_x ), pcbIUScale.mmToIU( m_center_y ) ),
                       VECTOR2D( pcbIUScale.mmToIU( m_start_x ), pcbIUScale.mmToIU( m_start_y ) ),
                       EDA_ANGLE( m_center_angle, DEGREES_T ), pcbIUScale.mmToIU( m_width ) );

        return arc;
    }
};


struct ARC_ARC_COLLIDE_CASE : public KI_TEST::NAMED_CASE
{
    ARC_DATA_MM         m_arc1;
    ARC_DATA_MM         m_arc2;
    double              m_clearance;
    bool                m_exp_result;
};


static const std::vector<ARC_ARC_COLLIDE_CASE> arc_arc_collide_cases = {
    { "case 1: No intersection",
      { 73.843527, 74.355869, 71.713528, 72.965869, -76.36664803, 0.2 },
      { 71.236473, 74.704131, 73.366472, 76.094131, -76.36664803, 0.2 },
      0,
      false },
    { "case 2: No intersection",
      { 82.542335, 74.825975, 80.413528, 73.435869, -76.4, 0.2 },
      { 76.491192, 73.839894, 78.619999, 75.23, -76.4, 0.2 },
      0,
      false },
    { "case 3: No intersection",
      { 89.318807, 74.810106, 87.19, 73.42, -76.4, 0.2 },
      { 87.045667, 74.632941, 88.826472, 75.794131, -267.9, 0.2 },
      0,
      false },
    { "case 4: Co-centered not intersecting",
      { 94.665667, 73.772941, 96.446472, 74.934131, -267.9, 0.2 },
      { 94.665667, 73.772941, 93.6551, 73.025482, -255.5, 0.2 },
      0,
      false },
    { "case 5: Not intersecting, but end points very close",
      { 72.915251, 80.493054, 73.570159, 81.257692, -260.5, 0.2 },
      { 73.063537, 82.295989, 71.968628, 81.581351, -255.5, 0.2 },
      0,
      false },
    { "case 6: Coincident centers, colliding due to arc thickness",
      { 79.279991, 80.67988, 80.3749, 81.394518, -255.5, 0.3 },
      { 79.279991, 80.67988, 80.3749, 81.694518, -255.5, 0.3 },
      0,
      true },
    { "case 7: Single intersection",
      { 88.495265, 81.766089, 90.090174, 82.867869, -255.5, 0.2 },
      { 86.995265, 81.387966, 89.090174, 82.876887, -255.5, 0.2 },
      0,
      true },
    { "case 8: Double intersection",
      { 96.149734, 81.792126, 94.99, 83.37, -347.2, 0.2 },
      { 94.857156, 81.240589, 95.91, 83.9, -288.5, 0.2 },
      0,
      true },
    { "case 9: Endpoints within arc width",
      { 72.915251, 86.493054, 73.970159, 87.257692, -260.5, 0.2 },
      { 73.063537, 88.295989, 71.968628, 87.581351, -255.5, 0.2 },
      0,
      true },
    { "case 10: Endpoints close, outside, no collision",
      { 78.915251, 86.393054, 79.970159, 87.157692, 99.5, 0.2 },
      { 79.063537, 88.295989, 77.968628, 87.581351, -255.5, 0.2 },
      0,
      false },
    { "case 11: Endpoints close, inside, collision due to arc width",
      { 85.915251, 86.993054, 86.970159, 87.757692, 99.5, 0.2 },
      { 86.063537, 88.295989, 84.968628, 87.581351, -255.5, 0.2 },
      0,
      true },
    { "case 12: Simulated differential pair length-tuning",
      { 94.6551, 88.296, 95.6551, 88.296, 90.0, 0.1 },
      { 94.6551, 88.296, 95.8551, 88.296, 90.0, 0.1 },
      0.1,
      false },
    { "case 13: One arc fully enclosed in other, non-concentric",
      { 73.77532, 93.413654, 75.70532, 93.883054, 60.0, 0.1 },
      { 73.86532, 93.393054, 75.86532, 93.393054, 90.0, 0.3 },
      0,
      true },
    { "case 14: One arc fully enclosed in other, concentric",
      { 79.87532, 93.413654, 81.64532, 94.113054, 60.0, 0.1 },
      { 79.87532, 93.413654, 81.86532, 93.393054, 90.0, 0.3 },
      0,
      true },
    { "case 15: Arcs separated by clearance",
        { 303.7615, 149.9252, 303.695968, 149.925237, 90.0262, 0.065 },
        { 303.6345, 149.2637, 303.634523, 148.85619, 89.9957, 0.065 },
      0.15,
      false },
};


BOOST_DATA_TEST_CASE( CollideArc, boost::unit_test::data::make( arc_arc_collide_cases ), c )
{
    SHAPE_ARC arc1( c.m_arc1.GenerateArc() );
    SHAPE_ARC arc2( c.m_arc2.GenerateArc() );


    SHAPE_LINE_CHAIN arc1_slc( c.m_arc1.GenerateArc() );
    arc1_slc.SetWidth( 0 );

    SHAPE_LINE_CHAIN arc2_slc( c.m_arc2.GenerateArc() );
    arc2_slc.SetWidth( 0 );

    int      actual = 0;
    VECTOR2I location;

    SHAPE* arc1_sh = &arc1;
    SHAPE* arc2_sh = &arc2;
    SHAPE* arc1_slc_sh = &arc1_slc;
    SHAPE* arc2_slc_sh = &arc2_slc;

    bool result_arc_to_arc = arc1_sh->Collide( arc2_sh, pcbIUScale.mmToIU( c.m_clearance ),
                                                &actual, &location );

    // For arc to chain collisions, we need to re-calculate the clearances because the
    // SHAPE_LINE_CHAIN is zero width
    int clearance = pcbIUScale.mmToIU( c.m_clearance ) + ( arc2.GetWidth() / 2 );

    bool result_arc_to_chain =
            arc1_sh->Collide( arc2_slc_sh, clearance, &actual, &location );

    clearance = pcbIUScale.mmToIU( c.m_clearance ) + ( arc1.GetWidth() / 2 );
    bool result_chain_to_arc =
            arc1_slc_sh->Collide( arc2_sh, clearance, &actual, &location );

    clearance = ( arc1.GetWidth() / 2 ) + ( arc2.GetWidth() / 2 );
    bool result_chain_to_chain =
            arc1_slc_sh->Collide( arc2_slc_sh, clearance, &actual, &location );

    BOOST_CHECK_EQUAL( result_arc_to_arc, c.m_exp_result );
    BOOST_CHECK_EQUAL( result_arc_to_chain, c.m_exp_result );
    BOOST_CHECK_EQUAL( result_chain_to_arc, c.m_exp_result );
    BOOST_CHECK_EQUAL( result_chain_to_chain, c.m_exp_result );
}


BOOST_AUTO_TEST_CASE( CollideArcToShapeLineChain )
{
    SHAPE_ARC arc( VECTOR2I( 206000000, 140110000 ), VECTOR2I( 201574617, 139229737 ),
                   VECTOR2I( 197822958, 136722959 ), 250000 );

    SHAPE_LINE_CHAIN lc( { VECTOR2I( 159600000, 142500000 ), VECTOR2I( 159600000, 142600000 ),
                           VECTOR2I( 166400000, 135800000 ), VECTOR2I( 166400000, 111600000 ),
                           VECTOR2I( 190576804, 111600000 ), VECTOR2I( 192242284, 113265480 ),
                           VECTOR2I( 192255720, 113265480 ), VECTOR2I( 203682188, 124691948 ),
                           VECTOR2I( 203682188, 140332188 ), VECTOR2I( 206000000, 142650000 ) },
                         false );



    SHAPE* arc_sh = &arc;
    SHAPE* lc_sh = &lc;

    BOOST_CHECK_EQUAL( arc_sh->Collide( &lc, 100000 ), true );
    BOOST_CHECK_EQUAL( lc_sh->Collide( &arc, 100000 ), true );

    SEG seg( VECTOR2I( 203682188, 124691948 ), VECTOR2I( 203682188, 140332188 ) );
    BOOST_CHECK_EQUAL( arc.Collide( seg, 0 ), true );
}


BOOST_AUTO_TEST_CASE( CollideArcToPolygonApproximation )
{
    SHAPE_ARC arc( VECTOR2I( 73843527, 74355869 ), VECTOR2I( 71713528, 72965869 ),
                   EDA_ANGLE( -76.36664803, DEGREES_T ), 1000000 );

    // Create a polyset approximation from the arc - error outside (simulating the zone filler)
    SHAPE_POLY_SET arcBuffer;
    int            clearance = ( arc.GetWidth() * 3 ) / 2;
    int            polygonApproximationError = SHAPE_ARC::DefaultAccuracyForPCB();

    TransformArcToPolygon( arcBuffer, arc.GetP0(), arc.GetArcMid(), arc.GetP1(),
                           arc.GetWidth() + 2 * clearance,
                           polygonApproximationError, ERROR_OUTSIDE );

    BOOST_REQUIRE_EQUAL( arcBuffer.OutlineCount(), 1 );
    BOOST_CHECK_EQUAL( arcBuffer.HoleCount( 0 ), 0 );

    // Make a reasonably large rectangular outline around the arc shape
    BOX2I arcbbox = arc.BBox( clearance * 4 );

    SHAPE_LINE_CHAIN zoneOutline( { arcbbox.GetPosition(),
                                    arcbbox.GetPosition() + VECTOR2I( arcbbox.GetWidth(), 0 ),
                                    arcbbox.GetEnd(),
                                    arcbbox.GetEnd() - VECTOR2I( arcbbox.GetWidth(), 0 )
                                  },
                                  true );

    // Create a synthetic "zone fill" polygon
    SHAPE_POLY_SET zoneFill;
    zoneFill.AddOutline( zoneOutline );
    zoneFill.AddHole( arcBuffer.Outline( 0 ) );
    zoneFill.CacheTriangulation( false );

    int      actual = 0;
    VECTOR2I location;
    int      epsilon = polygonApproximationError / 10;

    BOOST_CHECK_EQUAL( zoneFill.Collide( &arc, clearance + epsilon, &actual, &location ), true );

    BOOST_CHECK_EQUAL( zoneFill.Collide( &arc, clearance - epsilon, &actual, &location ), false );
}


struct ARC_TO_POLYLINE_CASE : public KI_TEST::NAMED_CASE
{
    ARC_CENTRE_PT_ANGLE m_geom;
};


/**
 * Predicate for checking a polyline has all the points on (near) a circle of
 * given centre and radius
 * @param  aPolyline the polyline to check
 * @param  aCentre   the circle centre
 * @param  aRad      the circle radius
 * @param  aTolerance  the tolerance for the endpoint-centre distance
 * @return           true if predicate met
 */
bool ArePolylineEndPointsNearCircle( const SHAPE_LINE_CHAIN& aPolyline, const VECTOR2I& aCentre,
                                     int aRad, int aTolerance )
{
    std::vector<VECTOR2I> points;

    for( int i = 0; i < aPolyline.PointCount(); ++i )
    {
        points.push_back( aPolyline.CPoint( i ) );
    }

    return GEOM_TEST::ArePointsNearCircle( points, aCentre, aRad, aTolerance );
}


/**
 * Predicate for checking a polyline has all the segment mid points on
 * (near) a circle of given centre and radius
 * @param  aPolyline the polyline to check
 * @param  aCentre   the circle centre
 * @param  aRad      the circle radius
 * @param  aTolEnds  the tolerance for the midpoint-centre distance
 * @return           true if predicate met
 */
bool ArePolylineMidPointsNearCircle( const SHAPE_LINE_CHAIN& aPolyline, const VECTOR2I& aCentre,
                                     int aRad, int aTolerance )
{
    std::vector<VECTOR2I> points;

    for( int i = 0; i < aPolyline.PointCount() - 1; ++i )
    {
        const VECTOR2I mid_pt = ( aPolyline.CPoint( i ) + aPolyline.CPoint( i + 1 ) ) / 2;
        points.push_back( mid_pt );
    }

    return GEOM_TEST::ArePointsNearCircle( points, aCentre, aRad, aTolerance );
}


const std::vector<ARC_TO_POLYLINE_CASE> ArcToPolyline_cases{
    {
        "Zero rad",
        {
            { 0, 0 },
            { 0, 0 },
            180,
        },
    },
    {
        "Semicircle",
        {
            { 0, 0 },
            { -1000000, 0 },
            180,
        },
    },
    {
        // check that very small circles don't fall apart and that reverse angles
        // work too
        "Extremely small semicircle",
        {
            { 0, 0 },
            { -1000, 0 },
            -180,
        },
    },
    {
        // Make sure it doesn't only work for "easy" angles
        "Non-round geometry",
        {
            { 0, 0 },
            { 1234567, 0 },
            42.22,
        },
    },
};


BOOST_DATA_TEST_CASE( ArcToPolyline, boost::unit_test::data::make( ArcToPolyline_cases ), c )
{
    const int width = 0;

    // Note: do not expect accuracies around 1 to work.  We use integers internally so we're
    // liable to rounding errors.  In PCBNew accuracy defaults to 5000 and we don't recommend
    // anything lower than 1000 (for performance reasons).
    const int accuracy = 100;
    const int epsilon  = 1;

    const SHAPE_ARC this_arc{ c.m_geom.m_center_point, c.m_geom.m_start_point,
                                EDA_ANGLE( c.m_geom.m_center_angle, DEGREES_T ), width };

    const SHAPE_LINE_CHAIN chain = this_arc.ConvertToPolyline( accuracy );

    BOOST_TEST_MESSAGE( "Polyline has " << chain.PointCount() << " points" );

    // Start point (exactly) where expected
    BOOST_CHECK_EQUAL( chain.CPoint( 0 ), c.m_geom.m_start_point );

    // End point (exactly) where expected
    BOOST_CHECK_EQUAL( chain.CLastPoint(), this_arc.GetP1() );

    int radius = ( c.m_geom.m_center_point - c.m_geom.m_start_point ).EuclideanNorm();

    // Other points within accuracy + epsilon (for rounding) of where they should be
    BOOST_CHECK_PREDICATE( ArePolylineEndPointsNearCircle,
            ( chain )( c.m_geom.m_center_point )( radius )( accuracy + epsilon ) );

    BOOST_CHECK_PREDICATE( ArePolylineMidPointsNearCircle,
            ( chain )( c.m_geom.m_center_point )( radius )( accuracy + epsilon ) );
}


BOOST_AUTO_TEST_SUITE_END()
