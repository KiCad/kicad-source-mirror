/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <qa_utils/geometry/geometry.h>
#include <qa_utils/numeric.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

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
    const double angle_tol_deg = 1.0;

    // Position error - rounding to nearest integer
    const int pos_tol = 1;

    BOOST_CHECK_PREDICATE( KI_TEST::IsVecWithinTol<VECTOR2I>,
            ( aProps.m_start_point )( aProps.m_start_point )( pos_tol ) );
    BOOST_CHECK_PREDICATE(
            KI_TEST::IsVecWithinTol<VECTOR2I>, ( aArc.GetP1() )( aProps.m_end_point )( pos_tol ) );
    BOOST_CHECK_PREDICATE( KI_TEST::IsVecWithinTol<VECTOR2I>,
            ( aArc.GetCenter() )( aProps.m_center_point )( aSynErrIU ) );
    BOOST_CHECK_PREDICATE( KI_TEST::IsWithinWrapped<double>,
            ( aArc.GetCentralAngle() )( aProps.m_center_angle )( 360.0 )( angle_tol_deg ) );
    BOOST_CHECK_PREDICATE( KI_TEST::IsWithinWrapped<double>,
            ( aArc.GetStartAngle() )( aProps.m_start_angle )( 360.0 )( angle_tol_deg ) );
    BOOST_CHECK_PREDICATE( KI_TEST::IsWithinWrapped<double>,
            ( aArc.GetEndAngle() )( aProps.m_end_angle )( 360.0 )( angle_tol_deg ) );
    BOOST_CHECK_PREDICATE(
            KI_TEST::IsWithin<double>, ( aArc.GetRadius() )( aProps.m_radius )( aSynErrIU ) );

    /// Check the chord agrees
    const auto chord = aArc.GetChord();

    BOOST_CHECK_PREDICATE(
            KI_TEST::IsVecWithinTol<VECTOR2I>, ( chord.A )( aProps.m_start_point )( pos_tol ) );
    BOOST_CHECK_PREDICATE(
            KI_TEST::IsVecWithinTol<VECTOR2I>, ( chord.B )( aProps.m_end_point )( pos_tol ) );

    /// All arcs are solid
    BOOST_CHECK_EQUAL( aArc.IsSolid(), true );

    BOOST_CHECK_PREDICATE(
            KI_TEST::IsBoxWithinTol<BOX2I>, ( aArc.BBox() )( aProps.m_bbox )( pos_tol ) );

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

    BOOST_CHECK_EQUAL( new_shape->Type(), SH_ARC );

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
 * Info to set up an arc by centre, start point and angle
 *
 * In future there may be more ways to set this up, so keep it separate
 */
struct ARC_CENTRE_PT_ANGLE
{
    VECTOR2I m_center_point;
    VECTOR2I m_start_point;
    double   m_center_angle;
};


struct ARC_CPA_CASE
{
    /// The text context name
    std::string m_ctx_name;

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


BOOST_AUTO_TEST_CASE( BasicCPAGeom )
{
    for( const auto& c : arc_cases )
    {
        BOOST_TEST_CONTEXT( c.m_ctx_name )
        {

            const auto this_arc = SHAPE_ARC{ c.m_geom.m_center_point, c.m_geom.m_start_point,
                c.m_geom.m_center_angle, c.m_width };

            CheckArc( this_arc, c.m_properties );
        }
    }
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


struct ARC_TTR_CASE
{
    /// The text context name
    std::string m_ctx_name;

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
                    225,
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


BOOST_AUTO_TEST_CASE( BasicTTRGeom )
{
    for( const auto& c : arc_ttr_cases )
    {
        BOOST_TEST_CONTEXT( c.m_ctx_name )
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


struct ARC_SEC_CASE
{
    /// The text context name
    std::string m_ctx_name;

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


BOOST_AUTO_TEST_CASE( BasicSECGeom )
{
    for( const auto& c : arc_sec_cases )
    {
        BOOST_TEST_CONTEXT( c.m_ctx_name )
        {
            VECTOR2I start = c.m_geom.m_start;
            VECTOR2I end = c.m_geom.m_end;
            VECTOR2I center = c.m_geom.m_center;
            bool     cw = c.m_clockwise;

            SHAPE_ARC this_arc;
            this_arc.ConstructFromStartEndCenter( start, end, center, cw );

            BOOST_CHECK_EQUAL( this_arc.GetArcMid(), c.m_expected_mid );
        }
    }
}


struct ARC_PT_COLLIDE_CASE
{
    std::string         m_ctx_name;
    ARC_CENTRE_PT_ANGLE m_geom;
    int                 m_arc_clearance;
    VECTOR2I            m_point;
    bool                m_exp_result;
    int                 m_exp_distance;
};


static const std::vector<ARC_PT_COLLIDE_CASE> arc_pt_collide_cases = {
    { " 270deg, 0 cl, 0   deg    ", { { 0, 0 }, { 100, 0 },  270.0 }, 0, { 100, 0 }, true, 0 },
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
    { " 270deg, 5 cl, 45  deg, 5 pos", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 75, 74 }, true, 5 },
    { " 270deg, 5 cl, -45 deg, 5 pos", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 75, -74 }, false, -1 },
    { " 270deg, 5 cl, 45  deg, 5 neg", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 67, 68 }, true, 5 },
    { " 270deg, 5 cl, -45 deg, 5 neg", { { 0, 0 }, { 100, 0 },  270.0 }, 5, { 67, -68 }, false, -1 },
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
};


BOOST_AUTO_TEST_CASE( CollidePt )
{
    for( const auto& c : arc_pt_collide_cases )
    {
        BOOST_TEST_CONTEXT( c.m_ctx_name )
        {
            SHAPE_ARC arc( c.m_geom.m_center_point, c.m_geom.m_start_point,
                           c.m_geom.m_center_angle );

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
    }
}


struct ARC_TO_POLYLINE_CASE
{
    std::string         m_ctx_name;
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


BOOST_AUTO_TEST_CASE( ArcToPolyline )
{
    const std::vector<ARC_TO_POLYLINE_CASE> cases = {
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

    const int width = 0;

    // Note: do not expect accuracies around 1 to work.  We use integers internally so we're
    // liable to rounding errors.  In PCBNew accuracy defaults to 5000 and we don't recommend
    // anything lower than 1000 (for performance reasons).
    const int accuracy = 100;
    const int epsilon  = 1;

    for( const auto& c : cases )
    {
        BOOST_TEST_CONTEXT( c.m_ctx_name )
        {
            const SHAPE_ARC this_arc{ c.m_geom.m_center_point, c.m_geom.m_start_point,
                                      c.m_geom.m_center_angle, width };

            const SHAPE_LINE_CHAIN chain = this_arc.ConvertToPolyline( accuracy );

            BOOST_TEST_MESSAGE( "Polyline has " << chain.PointCount() << " points" );

            // Start point (exactly) where expected
            BOOST_CHECK_EQUAL( chain.CPoint( 0 ), c.m_geom.m_start_point );

            // End point (exactly) where expected
            BOOST_CHECK_EQUAL( chain.CPoint( -1 ), this_arc.GetP1() );

            int radius = ( c.m_geom.m_center_point - c.m_geom.m_start_point ).EuclideanNorm();

            // Other points within accuracy + epsilon (for rounding) of where they should be
            BOOST_CHECK_PREDICATE( ArePolylineEndPointsNearCircle,
                    ( chain )( c.m_geom.m_center_point )( radius )( accuracy + epsilon ) );

            BOOST_CHECK_PREDICATE( ArePolylineMidPointsNearCircle,
                    ( chain )( c.m_geom.m_center_point )( radius )( accuracy + epsilon ) );
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
