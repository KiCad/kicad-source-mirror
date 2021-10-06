/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.TXT for contributors.
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

/**
 * @file
 * Test suite for LIB_ARC
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <trigo.h>
#include <convert_to_biu.h>

// Code under test
#include <lib_arc.h>

class TEST_LIB_ARC_FIXTURE
{
public:
    TEST_LIB_ARC_FIXTURE() :
        m_arc( nullptr )
    {
    }

    ///> Part with no extra data set
    LIB_ARC m_arc;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( LibArc, TEST_LIB_ARC_FIXTURE )


/**
 * Check that we can get the default properties out as expected
 */
BOOST_AUTO_TEST_CASE( DefaultProperties )
{
    BOOST_CHECK_EQUAL( m_arc.Type(), LIB_ARC_T );
    BOOST_CHECK_EQUAL( m_arc.GetClass(), "LIB_ARC" );
    BOOST_CHECK_EQUAL( m_arc.GetPosition(), wxPoint( 0, 0 ) );
}


/**
 * Test the function that calculates the radius angles based on the center, start, and end points.
 */
BOOST_AUTO_TEST_CASE( TestCalcRadiusAngles )
{
    double radius = 5.0;   // Use millimeters and convert to internal units.
    int startX = Millimeter2iu( radius * cos( DEG2RAD( 10.0 ) ) );
    int startY = Millimeter2iu( radius * sin( DEG2RAD( 10.0 ) ) );
    int endX = Millimeter2iu( radius * cos( DEG2RAD( 45.0 ) ) );
    int endY = Millimeter2iu( radius * sin( DEG2RAD( 45.0 ) ) );

    m_arc.SetStart( wxPoint( startX, startY ) );
    m_arc.SetEnd( wxPoint( endX, endY ) );

    m_arc.CalcRadiusAngles();
    BOOST_CHECK_EQUAL( m_arc.GetFirstRadiusAngle(), 100 );
    BOOST_CHECK_EQUAL( m_arc.GetSecondRadiusAngle(), 450 );

    // Set arc end point in the second quadrant.
    endX = Millimeter2iu( radius * cos( DEG2RAD( 145.0 ) ) );
    endY = Millimeter2iu( radius * sin( DEG2RAD( 145.0 ) ) );
    m_arc.SetEnd( wxPoint( endX, endY ) );
    m_arc.CalcRadiusAngles();
    BOOST_CHECK_EQUAL( m_arc.GetFirstRadiusAngle(), 100 );
    BOOST_CHECK_EQUAL( m_arc.GetSecondRadiusAngle(), 1450 );
}


/**
 * Test the function that calculates the mid point based on the start and end angles and
 * radius length.
 */
BOOST_AUTO_TEST_CASE( TestCalcMidPoint )
{
    // Midpoint angle is 77.5 degrees.
    m_arc.SetRadius( Millimeter2iu( 5.0 ) );
    m_arc.SetFirstRadiusAngle( 100 );
    m_arc.SetSecondRadiusAngle( 1450 );
    BOOST_CHECK_EQUAL( m_arc.CalcMidPoint(), VECTOR2I( 10822, 48815 ) );
    m_arc.SetFirstRadiusAngle( 850 );
    m_arc.SetSecondRadiusAngle( 950 );
    BOOST_CHECK_EQUAL( m_arc.CalcMidPoint(), VECTOR2I( 0, 50000 ) );
    m_arc.SetFirstRadiusAngle( 1700 );
    m_arc.SetSecondRadiusAngle( 1900 );
    BOOST_CHECK_EQUAL( m_arc.CalcMidPoint(), VECTOR2I( -50000, 0 ) );
    m_arc.SetFirstRadiusAngle( 2500 );
    m_arc.SetSecondRadiusAngle( 2900 );
    BOOST_CHECK_EQUAL( m_arc.CalcMidPoint(), VECTOR2I( 0, -50000 ) );
    m_arc.SetFirstRadiusAngle( 3500 );
    m_arc.SetSecondRadiusAngle( 100 );
    BOOST_CHECK_EQUAL( m_arc.CalcMidPoint(), VECTOR2I( 50000, 0 ) );
}


BOOST_AUTO_TEST_SUITE_END()
