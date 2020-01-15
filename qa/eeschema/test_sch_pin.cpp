/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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
 * Test suite for SCH_PIM
 */

#include <unit_test_utils/geometry.h>
#include <unit_test_utils/unit_test_utils.h>
#include <unit_test_utils/wx_assert.h>

// Code under test
#include <sch_pin.h>

#include <sch_component.h>

#include <eda_rect.h>


class TEST_SCH_PIN_FIXTURE
{
public:
    TEST_SCH_PIN_FIXTURE()
            : m_parent_part( "parent_part", nullptr ),
              m_lib_pin( &m_parent_part ),
              m_parent_comp( wxPoint( 0, 0 ), nullptr ),
              m_sch_pin( &m_lib_pin, &m_parent_comp )
    {
        // give the pin some kind of data we can use to test
        m_lib_pin.SetNumber( "42" );
        m_lib_pin.SetName( "pinname" );
        m_lib_pin.SetType( ELECTRICAL_PINTYPE::PT_INPUT );

        SCH_SHEET_PATH path;
        m_parent_comp.SetRef( &path, "U2" );
    }

    LIB_PART m_parent_part;
    LIB_PIN  m_lib_pin;

    SCH_COMPONENT m_parent_comp;
    SCH_PIN       m_sch_pin;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchPin, TEST_SCH_PIN_FIXTURE )

/**
 * Check basic properties of an un-modified SCH_PIN object
 */
BOOST_AUTO_TEST_CASE( DefaultProperties )
{
    BOOST_CHECK_EQUAL( m_sch_pin.GetParentComponent(), &m_parent_comp );
    BOOST_CHECK_EQUAL( m_sch_pin.GetLibPin(), &m_lib_pin );

    BOOST_CHECK_EQUAL( m_sch_pin.GetPosition(), wxPoint( 0, 0 ) );

    // These just forward to LIB_PIN for now, so this isn't very interesting
    // but later we will want to test these functions for SCH_PIN's own functionality
    BOOST_CHECK_EQUAL( m_sch_pin.IsVisible(), m_lib_pin.IsVisible() );
    BOOST_CHECK_EQUAL( m_sch_pin.GetName(), m_lib_pin.GetName() );
    BOOST_CHECK_EQUAL( m_sch_pin.GetNumber(), m_lib_pin.GetNumber() );

    BOOST_CHECK( ( m_sch_pin.GetType() == m_lib_pin.GetType() ) );

    BOOST_CHECK_EQUAL( m_sch_pin.IsPowerConnection(), m_lib_pin.IsPowerConnection() );
}

/**
 * Check the assignment operator
 */
BOOST_AUTO_TEST_CASE( Assign )
{
    SCH_PIN assigned = m_sch_pin;

    BOOST_CHECK_EQUAL( assigned.GetParentComponent(), &m_parent_comp );
}

/**
 * Check the copy ctor
 */
BOOST_AUTO_TEST_CASE( Copy )
{
    SCH_PIN copied( m_sch_pin );

    BOOST_CHECK_EQUAL( copied.GetParentComponent(), &m_parent_comp );
}

/**
 * Check the pin dangling flag
 */
BOOST_AUTO_TEST_CASE( PinDangling )
{
    // dangles by default
    BOOST_CHECK_EQUAL( m_sch_pin.IsDangling(), true );

    // all you have to do to un-dangle is say so
    m_sch_pin.SetIsDangling( false );
    BOOST_CHECK_EQUAL( m_sch_pin.IsDangling(), false );

    // and the same to re-dangle
    m_sch_pin.SetIsDangling( true );
    BOOST_CHECK_EQUAL( m_sch_pin.IsDangling(), true );
}

/**
 * Check the pin labelling
 */
BOOST_AUTO_TEST_CASE( PinNumbering )
{
    SCH_SHEET_PATH path;

    const wxString name = m_sch_pin.GetDefaultNetName( path );
    BOOST_CHECK_EQUAL( name, "Net-(U2-Pad42)" );

    // do it again: this should now (transparently) go though the net name map
    // can't really check directly, but coverage tools should see this
    const wxString map_name = m_sch_pin.GetDefaultNetName( path );
    BOOST_CHECK_EQUAL( map_name, name );
}

/**
 * Check the pin labelling when it's a power pin
 */
BOOST_AUTO_TEST_CASE( PinNumberingPower )
{
    // but if we set is power...
    m_lib_pin.SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
    m_parent_part.SetPower();

    // the name is just the pin name
    SCH_SHEET_PATH path;
    const wxString pwr_name = m_sch_pin.GetDefaultNetName( path );
    BOOST_CHECK_EQUAL( pwr_name, "pinname" );
}

BOOST_AUTO_TEST_SUITE_END()