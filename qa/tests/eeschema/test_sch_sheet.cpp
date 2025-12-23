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

/**
 * @file
 * Test suite for SCH_SHEET
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_screen.h>
#include <schematic.h>

#include <qa_utils/uuid_test_utils.h>
#include <qa_utils/wx_utils/wx_assert.h>

class TEST_SCH_SHEET_FIXTURE
{
public:
    TEST_SCH_SHEET_FIXTURE() :
            m_schematic( nullptr ),
            m_sheet(),
            m_csheet( m_sheet )
    {
    }

    ///< Dummy schematic to attach the test sheet to
    SCHEMATIC m_schematic;

    SCH_SHEET m_sheet;

    ///< Can use when you need a const ref (lots of places need fixing here)
    const SCH_SHEET& m_csheet;
};


/**
 * Print helper.
 * Not a print_log_value because old Boosts don't like that in BOOST_CHECK_EQUAL_COLLECTIONS
 */
std::ostream& operator<<( std::ostream& os, DANGLING_END_ITEM const& d )
{
    os << "DANGLING_END_ITEM[ type " << d.GetType() << " @(" << d.GetPosition().x << ", "
       << d.GetPosition().y << "), item " << d.GetItem() << ", parent " << d.GetParent() << " ]";
    return os;
}

/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( SchSheet, TEST_SCH_SHEET_FIXTURE )


/**
 * Check default properties
 */
BOOST_AUTO_TEST_CASE( Default )
{
    BOOST_CHECK_EQUAL( m_csheet.GetPosition(), VECTOR2I( 0, 0 ) );

    BOOST_CHECK_EQUAL( m_sheet.GetParent(), nullptr );
    BOOST_CHECK_EQUAL( m_sheet.CountSheets(), 1 );

    BOOST_CHECK_EQUAL( m_csheet.GetScreenCount(), 0 );

    BOOST_CHECK_EQUAL( m_sheet.SymbolCount(), 0 );
}

/**
 * Test setting parent schematic
 */
BOOST_AUTO_TEST_CASE( SchematicParent )
{
    m_sheet.SetParent( &m_schematic );

    BOOST_CHECK_EQUAL( m_sheet.IsVirtualRootSheet(), false );
    BOOST_CHECK_EQUAL( m_sheet.IsTopLevelSheet(), false );  // Not yet a top-level sheet

    SCH_SCREEN* screen = new SCH_SCREEN( &m_schematic );
    m_sheet.SetScreen( screen );
    m_schematic.AddTopLevelSheet( &m_sheet );

    // After AddTopLevelSheet, the sheet becomes a top-level sheet under the virtual root
    BOOST_CHECK_EQUAL( m_sheet.IsVirtualRootSheet(), false );  // Sheet is not the virtual root
    BOOST_CHECK_EQUAL( m_sheet.IsTopLevelSheet(), true );      // Sheet is now a top-level sheet
    BOOST_CHECK_EQUAL( m_schematic.Root().IsVirtualRootSheet(), true );  // The root is virtual

    m_schematic.RemoveTopLevelSheet( &m_sheet );
}

/**
 * Test adding pins to a sheet
 */
BOOST_AUTO_TEST_CASE( AddPins )
{
    const VECTOR2I pinPos{ 42, 13 };

    // we should catch null insertions
    CHECK_WX_ASSERT( m_sheet.AddPin( nullptr ) );

    auto newPin = std::make_unique<SCH_SHEET_PIN>( &m_sheet, pinPos, "pinname" );

    // can't be const because of RemovePin (?!)
    SCH_SHEET_PIN& pinRef = *newPin;

    m_sheet.AddPin( newPin.release() );

    // now we can find it in the list
    BOOST_CHECK_EQUAL( m_sheet.HasPins(), true );
    BOOST_CHECK_EQUAL( m_sheet.HasPin( "pinname" ), true );
    BOOST_CHECK_EQUAL( m_sheet.HasPin( "PINname" ), false );

    BOOST_CHECK_EQUAL( m_sheet.GetPin( pinPos ), &pinRef );

    // check the actual list can be retrieved
    std::vector<SCH_SHEET_PIN*>& pins = m_sheet.GetPins();
    BOOST_CHECK_EQUAL( pins[0], &pinRef );

    // catch the bad call
    CHECK_WX_ASSERT( m_sheet.RemovePin( nullptr ) );

    m_sheet.RemovePin( &pinRef );

    // and it's gone
    BOOST_CHECK_EQUAL( m_sheet.HasPins(), false );
    BOOST_CHECK_EQUAL( m_sheet.HasPin( "pinname" ), false );
    BOOST_CHECK_EQUAL( m_sheet.GetPin( pinPos ), nullptr );

    delete &pinRef;
}

/**
 * Check that pins are added and renumbered to be unique
 */
BOOST_AUTO_TEST_CASE( PinRenumbering )
{
    for( int i = 0; i < 5; ++i )
    {
        SCH_SHEET_PIN* pin = new SCH_SHEET_PIN( &m_sheet, VECTOR2I{ i, i }, "name" );

        // set the pins to have the same number going in
        pin->SetNumber( 2 );

        m_sheet.AddPin( pin );
    }

    std::vector<SCH_SHEET_PIN*>& pins = m_sheet.GetPins();

    std::vector<int> numbers;

    for( SCH_SHEET_PIN* pin : pins )
        numbers.push_back( pin->GetNumber() );

    // and now...they are all unique
    BOOST_CHECK_PREDICATE( KI_TEST::CollectionHasNoDuplicates<decltype( numbers )>, ( numbers ) );
}


struct TEST_END_CONN_PIN
{
    std::string m_pin_name;
    VECTOR2I    m_pos;
};


/**
 * Test the endpoint and connection point collections: we should be able to add pins, then
 * have them appear as endpoints.
 */
BOOST_AUTO_TEST_CASE( EndconnectionPoints )
{
    // x = zero because the pin is clamped to the left side by default
    const std::vector<TEST_END_CONN_PIN> pin_defs = {
        {
            "1name",
            { 0, 13 },
        },
        {
            "2name",
            { 0, 130 },
        },
    };

    // Insert the pins into the sheet
    for( const auto& pin : pin_defs )
        m_sheet.AddPin( new SCH_SHEET_PIN( &m_sheet, pin.m_pos, pin.m_pin_name ) );

    std::vector<SCH_SHEET_PIN*>& pins = m_sheet.GetPins();

    // make sure the pins made it in
    BOOST_CHECK_EQUAL( pins.size(), pin_defs.size() );

    // Check that the End getter gets the right things
    {
        std::vector<DANGLING_END_ITEM> expectedDangling;

        // Construct expected from the pin, not defs, as we need the pin address
        for( SCH_SHEET_PIN* pin : pins )
        {
            expectedDangling.emplace_back( DANGLING_END_T::SHEET_LABEL_END, pin,
                                           pin->GetPosition(), pin );
        }

        std::vector<DANGLING_END_ITEM> dangling;
        m_sheet.GetEndPoints( dangling );

        BOOST_CHECK_EQUAL_COLLECTIONS( dangling.begin(), dangling.end(),
                                       expectedDangling.begin(), expectedDangling.end() );
    }

    // And check the connection getter
    {
        std::vector<VECTOR2I> expectedConnections;

        // we want to see every pin that we just added
        for( const auto& pin : pin_defs )
        {
            expectedConnections.push_back( pin.m_pos );
        }

        std::vector<VECTOR2I> connections = m_sheet.GetConnectionPoints();

        BOOST_CHECK_EQUAL_COLLECTIONS( connections.begin(), connections.end(),
                                       expectedConnections.begin(), expectedConnections.end() );
    }
}


BOOST_AUTO_TEST_SUITE_END()
