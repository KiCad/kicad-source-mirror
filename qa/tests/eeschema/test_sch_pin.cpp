/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <qa_utils/wx_utils/wx_assert.h>

// Code under test
#include <lib_symbol.h>
#include <pin_type.h>
#include <sch_pin.h>
#include <sch_symbol.h>


class TEST_SCH_PIN_FIXTURE
{
public:
    TEST_SCH_PIN_FIXTURE()
    {
        m_parent_part = new LIB_SYMBOL( "parent_part", nullptr );

        m_lib_pin = new SCH_PIN( m_parent_part );
        m_parent_part->AddDrawItem( m_lib_pin );

        // give the pin some kind of data we can use to test
        m_lib_pin->SetNumber( "42" );
        m_lib_pin->SetName( "pinname" );
        m_lib_pin->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
        m_lib_pin->SetPosition( VECTOR2I( 1, 2 ) );

        SCH_SHEET_PATH path;
        m_parent_symbol = new SCH_SYMBOL( *m_parent_part, m_parent_part->GetLibId(), &path, 0, 0,
                                          VECTOR2I( 1, 2 ) );
        m_parent_symbol->SetRef( &path, "U2" );
        m_parent_symbol->UpdatePins();

        m_sch_pin = m_parent_symbol->GetPins( &path )[0];
    }

    ~TEST_SCH_PIN_FIXTURE()
    {
        delete m_parent_symbol;
        delete m_parent_part;
    }

    LIB_SYMBOL* m_parent_part;
    SCH_PIN*    m_lib_pin;

    SCH_SYMBOL* m_parent_symbol;
    SCH_PIN*    m_sch_pin;       // owned by m_parent_symbol, not us
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
    BOOST_CHECK_EQUAL( m_sch_pin->GetParentSymbol(), m_parent_symbol );

    BOOST_CHECK_EQUAL( m_sch_pin->GetLocalPosition(), VECTOR2I( 1, 2 ) );
    BOOST_CHECK_EQUAL( m_sch_pin->GetPosition(), VECTOR2I( 2, 4 ) );

    BOOST_CHECK_EQUAL( m_sch_pin->IsVisible(), m_lib_pin->IsVisible() );
    BOOST_CHECK_EQUAL( m_sch_pin->GetName(), m_lib_pin->GetName() );
    BOOST_CHECK_EQUAL( m_sch_pin->GetNumber(), m_lib_pin->GetNumber() );

    BOOST_CHECK( ( m_sch_pin->GetType() == m_lib_pin->GetType() ) );

    BOOST_CHECK_EQUAL( m_sch_pin->IsGlobalPower(), m_lib_pin->IsGlobalPower() );
    BOOST_CHECK_EQUAL( m_sch_pin->IsLocalPower(), m_lib_pin->IsLocalPower() );
}

/**
 * Check the assignment operator
 */
BOOST_AUTO_TEST_CASE( Assign )
{
    SCH_PIN assigned = *m_sch_pin;

    BOOST_CHECK_EQUAL( assigned.GetParentSymbol(), m_parent_symbol );
    BOOST_CHECK_EQUAL( assigned.GetNumber(), m_lib_pin->GetNumber() );
}

/**
 * Check the copy ctor
 */
BOOST_AUTO_TEST_CASE( Copy )
{
    SCH_PIN copied( *m_sch_pin );

    BOOST_CHECK_EQUAL( copied.GetParentSymbol(), m_parent_symbol );
    BOOST_CHECK_EQUAL( copied.GetNumber(), m_lib_pin->GetNumber() );
    BOOST_CHECK_EQUAL( copied.GetAlt(), wxEmptyString );

    SCH_PIN::ALT alt;
    alt.m_Name = wxS( "alt" );
    alt.m_Shape = GRAPHIC_PINSHAPE::INVERTED;
    alt.m_Type = ELECTRICAL_PINTYPE::PT_OUTPUT;
    copied.GetAlternates()[ wxS( "alt" ) ] = alt;

    // Set some non-default values
    copied.SetAlt( "alt" );

    SCH_PIN copied2( copied );
    BOOST_CHECK_EQUAL( copied2.GetAlt(), "alt" );
}

/**
 * Check the pin dangling flag
 */
BOOST_AUTO_TEST_CASE( PinDangling )
{
    // dangles by default
    BOOST_CHECK_EQUAL( m_sch_pin->IsDangling(), true );

    // all you have to do to un-dangle is say so
    m_sch_pin->SetIsDangling( false );
    BOOST_CHECK_EQUAL( m_sch_pin->IsDangling(), false );

    // and the same to re-dangle
    m_sch_pin->SetIsDangling( true );
    BOOST_CHECK_EQUAL( m_sch_pin->IsDangling(), true );
}

/**
 * Check the pin labelling
 */
BOOST_AUTO_TEST_CASE( PinNumbering )
{
    SCH_SHEET_PATH path;

    const wxString name = m_sch_pin->GetDefaultNetName( path );
    BOOST_CHECK_EQUAL( name, "Net-(U2-pinname)" );

    // do it again: this should now (transparently) go though the net name map
    // can't really check directly, but coverage tools should see this
    const wxString map_name = m_sch_pin->GetDefaultNetName( path );
    BOOST_CHECK_EQUAL( map_name, name );
}

/**
 * Check the pin labelling when it's a power pin
 */
BOOST_AUTO_TEST_CASE( PinNumberingPower )
{
    // but if we set isPower...
    m_lib_pin->SetType( ELECTRICAL_PINTYPE::PT_POWER_IN );
    m_parent_part->SetGlobalPower();
    BOOST_CHECK_EQUAL( m_lib_pin->IsGlobalPower(), true );

    // and update symbol from library...
    SCH_SHEET_PATH path;
    delete m_parent_symbol;
    m_parent_symbol = new SCH_SYMBOL( *m_parent_part, m_parent_part->GetLibId(), &path, 0, 0,
                                      VECTOR2I( 1, 2 ) );
    m_parent_symbol->SetRef( &path, "U2" );
    m_parent_symbol->SetValueFieldText( "voltage_value" );
    m_parent_symbol->UpdatePins();

    m_sch_pin = m_parent_symbol->GetPins( &path )[0];

    // ... then the name is just the pin name
    const wxString pwr_name = m_sch_pin->GetDefaultNetName( path );
    BOOST_CHECK_EQUAL( pwr_name, "voltage_value" );
}

BOOST_AUTO_TEST_SUITE_END()
