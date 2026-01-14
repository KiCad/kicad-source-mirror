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

BOOST_AUTO_TEST_CASE( AlternatePinRenameUpdates )
{
    SCH_PIN::ALT alt;
    alt.m_Name = wxS( "ALT1" );
    alt.m_Shape = GRAPHIC_PINSHAPE::INVERTED;
    alt.m_Type = ELECTRICAL_PINTYPE::PT_INPUT;
    m_lib_pin->GetAlternates()[ wxS( "ALT1" ) ] = alt;

    m_parent_symbol->SetLibSymbol( m_parent_part->Flatten().release() );
    m_sch_pin = m_parent_symbol->GetPins()[0];
    m_sch_pin->SetAlt( wxS( "ALT1" ) );

    SCH_PIN::ALT altNew = alt;
    m_lib_pin->GetAlternates().erase( wxS( "ALT1" ) );
    altNew.m_Name = wxS( "ALT1_NEW" );
    m_lib_pin->GetAlternates()[ wxS( "ALT1_NEW" ) ] = altNew;

    m_parent_symbol->SetLibSymbol( m_parent_part->Flatten().release() );

    SCH_PIN* updatedPin = m_parent_symbol->GetPins()[0];

    BOOST_CHECK_EQUAL( updatedPin->GetAlt(), "ALT1_NEW" );
    BOOST_CHECK( updatedPin->GetAlternates().count( wxS( "ALT1" ) ) == 0 );
}


/**
 * Test for issue #22286 - GetType() should return the alternate's type when alternate is selected
 */
BOOST_AUTO_TEST_CASE( AlternatePinTypeReturnsCorrectType )
{
    // Set the library pin's default type to no_connect
    m_lib_pin->SetType( ELECTRICAL_PINTYPE::PT_NC );

    // Add an alternate with a different type (power_in)
    SCH_PIN::ALT powerAlt;
    powerAlt.m_Name = wxS( "8.pow" );
    powerAlt.m_Shape = GRAPHIC_PINSHAPE::LINE;
    powerAlt.m_Type = ELECTRICAL_PINTYPE::PT_POWER_IN;
    m_lib_pin->GetAlternates()[ wxS( "8.pow" ) ] = powerAlt;

    // Flatten and update the symbol to get a fresh schematic pin
    m_parent_symbol->SetLibSymbol( m_parent_part->Flatten().release() );
    m_sch_pin = m_parent_symbol->GetPins()[0];

    // Before selecting alternate, type should be the default (NC)
    BOOST_CHECK( m_sch_pin->GetType() == ELECTRICAL_PINTYPE::PT_NC );

    // Select the alternate
    m_sch_pin->SetAlt( wxS( "8.pow" ) );

    // After selecting alternate, type should be the alternate's type (POWER_IN)
    BOOST_CHECK_EQUAL( m_sch_pin->GetAlt(), "8.pow" );
    BOOST_CHECK( m_sch_pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN );

    // Also verify shape changed
    BOOST_CHECK( m_sch_pin->GetShape() == GRAPHIC_PINSHAPE::LINE );
}


/**
 * Test for issue #22286 - simulate schematic loading scenario
 * When a schematic is loaded, the pin's alternate is set BEFORE SetLibSymbol is called.
 * The alternate must persist through UpdatePins().
 */
BOOST_AUTO_TEST_CASE( AlternatePinTypePersistsThroughSymbolUpdate )
{
    // Set the library pin's default type to no_connect
    m_lib_pin->SetType( ELECTRICAL_PINTYPE::PT_NC );

    // Add an alternate with a different type (power_in)
    SCH_PIN::ALT powerAlt;
    powerAlt.m_Name = wxS( "8.pow" );
    powerAlt.m_Shape = GRAPHIC_PINSHAPE::LINE;
    powerAlt.m_Type = ELECTRICAL_PINTYPE::PT_POWER_IN;
    m_lib_pin->GetAlternates()[ wxS( "8.pow" ) ] = powerAlt;

    // First flatten to set up the symbol
    m_parent_symbol->SetLibSymbol( m_parent_part->Flatten().release() );
    m_sch_pin = m_parent_symbol->GetPins()[0];

    // Set the alternate on the schematic pin (like the parser would)
    m_sch_pin->SetAlt( wxS( "8.pow" ) );
    BOOST_CHECK_EQUAL( m_sch_pin->GetAlt(), "8.pow" );
    BOOST_CHECK( m_sch_pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN );

    // Now simulate what happens when the library symbol is re-resolved
    // This is similar to what happens after loading a schematic from file
    m_parent_symbol->SetLibSymbol( m_parent_part->Flatten().release() );
    m_sch_pin = m_parent_symbol->GetPins()[0];

    // After the symbol update, the alternate should still be set
    BOOST_CHECK_EQUAL( m_sch_pin->GetAlt(), "8.pow" );

    // And GetType() should return the alternate's type
    BOOST_CHECK( m_sch_pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN );
}


/**
 * Test for issue #22286 - simulate parser-created pins with alternates
 * The parser creates pins with m_alt set but m_libPin=nullptr.
 * Verify that after UpdatePins() the alternate type is correctly returned.
 */
BOOST_AUTO_TEST_CASE( ParserCreatedPinWithAlternate )
{
    // Create a library symbol with a pin that has alternates
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( "test_symbol", nullptr );

    SCH_PIN* libPin = new SCH_PIN( libSymbol );
    libPin->SetNumber( "8" );
    libPin->SetName( "PIN8" );
    libPin->SetType( ELECTRICAL_PINTYPE::PT_NC );  // Default type is NC
    libPin->SetPosition( VECTOR2I( 0, 0 ) );

    // Add an alternate with power_in type
    SCH_PIN::ALT powerAlt;
    powerAlt.m_Name = wxS( "8.pow" );
    powerAlt.m_Shape = GRAPHIC_PINSHAPE::LINE;
    powerAlt.m_Type = ELECTRICAL_PINTYPE::PT_POWER_IN;
    libPin->GetAlternates()[ wxS( "8.pow" ) ] = powerAlt;

    libSymbol->AddDrawItem( libPin );

    // Create a schematic symbol
    SCH_SHEET_PATH path;
    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libSymbol->GetLibId(), &path, 0, 0,
                                          VECTOR2I( 0, 0 ) );
    symbol->SetRef( &path, "J1" );

    // Simulate what the parser does: create a raw pin with alternate set
    // This bypasses SetAlt() validation by using the constructor directly
    symbol->GetRawPins().clear();  // Remove auto-created pins
    symbol->GetRawPins().emplace_back(
        std::make_unique<SCH_PIN>( symbol, wxS( "8" ), wxS( "8.pow" ), KIID() ) );

    SCH_PIN* parserPin = symbol->GetRawPins()[0].get();

    // At this point, m_alt is set but m_libPin is nullptr
    BOOST_CHECK_EQUAL( parserPin->GetAlt(), "8.pow" );
    BOOST_CHECK( parserPin->GetLibPin() == nullptr );

    // GetType() should return PT_UNSPECIFIED when m_libPin is null
    BOOST_CHECK( parserPin->GetType() == ELECTRICAL_PINTYPE::PT_UNSPECIFIED );

    // Now simulate SetLibSymbol() which calls UpdatePins()
    symbol->SetLibSymbol( libSymbol->Flatten().release() );

    // Verify the flattened library symbol has pins with alternates
    BOOST_CHECK( symbol->GetLibSymbolRef() != nullptr );
    std::vector<SCH_PIN*> libPins = symbol->GetLibSymbolRef()->GetPins();
    BOOST_CHECK_EQUAL( libPins.size(), 1 );

    if( !libPins.empty() )
    {
        SCH_PIN* flattenedLibPin = libPins[0];
        BOOST_CHECK( !flattenedLibPin->GetAlternates().empty() );
        BOOST_CHECK( flattenedLibPin->GetAlternates().count( wxS( "8.pow" ) ) > 0 );
    }

    // Get the pin after the update
    SCH_PIN* updatedPin = symbol->GetPins( &path )[0];

    // The alternate should still be set
    BOOST_CHECK_EQUAL( updatedPin->GetAlt(), "8.pow" );

    // m_libPin should now be set
    BOOST_CHECK( updatedPin->GetLibPin() != nullptr );

    // GetType() should return the alternate's type (PT_POWER_IN)
    BOOST_CHECK( updatedPin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN );

    delete symbol;
    delete libSymbol;
}


/**
 * Test for issue #22286 - detailed trace of SetLibSymbol flow
 * Verifies that library pin alternates are preserved through the copy
 * and that SetAlt succeeds in UpdatePins().
 */
BOOST_AUTO_TEST_CASE( LibraryPinAlternatesPreservedThroughCopy )
{
    // Create original library symbol with pin that has alternates
    LIB_SYMBOL* origLibSymbol = new LIB_SYMBOL( "test_symbol", nullptr );

    SCH_PIN* origLibPin = new SCH_PIN( origLibSymbol );
    origLibPin->SetNumber( "8" );
    origLibPin->SetName( "8" );  // Default name matches number
    origLibPin->SetType( ELECTRICAL_PINTYPE::PT_NC );  // Default type is NC
    origLibPin->SetPosition( VECTOR2I( 0, 0 ) );

    // Add alternate with power_in type
    SCH_PIN::ALT powerAlt;
    powerAlt.m_Name = wxS( "8.pow" );
    powerAlt.m_Shape = GRAPHIC_PINSHAPE::LINE;
    powerAlt.m_Type = ELECTRICAL_PINTYPE::PT_POWER_IN;
    origLibPin->GetAlternates()[ wxS( "8.pow" ) ] = powerAlt;

    origLibSymbol->AddDrawItem( origLibPin );

    // Verify original has alternates
    BOOST_CHECK( !origLibPin->GetAlternates().empty() );
    BOOST_CHECK( origLibPin->GetAlternates().count( wxS( "8.pow" ) ) > 0 );

    // Simulate what happens during loading:
    // 1. Library symbol is stored in m_libSymbols (just keep the original)
    // 2. Create a COPY of the library symbol (like UpdateLocalLibSymbolLinks does)
    LIB_SYMBOL* copiedLibSymbol = new LIB_SYMBOL( *origLibSymbol );

    // Verify the copied library symbol has pins with alternates
    std::vector<SCH_PIN*> copiedLibPins = copiedLibSymbol->GetPins();
    BOOST_CHECK_EQUAL( copiedLibPins.size(), 1 );

    if( !copiedLibPins.empty() )
    {
        SCH_PIN* copiedLibPin = copiedLibPins[0];

        // This is the critical check - does the copied lib pin have alternates?
        BOOST_CHECK_MESSAGE( !copiedLibPin->GetAlternates().empty(),
                             "Copied library pin should have alternates" );
        BOOST_CHECK_MESSAGE( copiedLibPin->GetAlternates().count( wxS( "8.pow" ) ) > 0,
                             "Copied library pin should have '8.pow' alternate" );

        // Check if the alternate has the correct type
        if( copiedLibPin->GetAlternates().count( wxS( "8.pow" ) ) > 0 )
        {
            BOOST_CHECK( copiedLibPin->GetAlternates().at( wxS( "8.pow" ) ).m_Type
                         == ELECTRICAL_PINTYPE::PT_POWER_IN );
        }
    }

    // Now create a schematic symbol with parser-created raw pins
    SCH_SHEET_PATH path;
    SCH_SYMBOL* symbol = new SCH_SYMBOL( *origLibSymbol, origLibSymbol->GetLibId(), &path, 0, 0,
                                          VECTOR2I( 0, 0 ) );
    symbol->SetRef( &path, "J1" );

    // Clear auto-created pins and add parser-style raw pin
    symbol->GetRawPins().clear();
    symbol->GetRawPins().emplace_back(
        std::make_unique<SCH_PIN>( symbol, wxS( "8" ), wxS( "8.pow" ), KIID() ) );

    // Verify raw pin state before SetLibSymbol
    SCH_PIN* rawPin = symbol->GetRawPins()[0].get();
    BOOST_CHECK_EQUAL( rawPin->GetAlt(), "8.pow" );
    BOOST_CHECK( rawPin->GetLibPin() == nullptr );

    // Call SetLibSymbol with the COPIED library symbol (like UpdateLocalLibSymbolLinks does)
    symbol->SetLibSymbol( copiedLibSymbol );  // Takes ownership

    // Get the pin after UpdatePins() was called
    std::vector<SCH_PIN*> schPins = symbol->GetPins( &path );
    BOOST_CHECK_EQUAL( schPins.size(), 1 );

    if( !schPins.empty() )
    {
        SCH_PIN* schPin = schPins[0];

        // Check that m_libPin is set
        BOOST_CHECK_MESSAGE( schPin->GetLibPin() != nullptr,
                             "Schematic pin should have m_libPin set after UpdatePins" );

        // Check that the alternate is still set
        BOOST_CHECK_MESSAGE( schPin->GetAlt() == "8.pow",
                             "Alternate should be preserved as '8.pow'" );

        // Check that GetType() returns the alternate's type
        BOOST_CHECK_MESSAGE( schPin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN,
                             "GetType() should return alternate's type (PT_POWER_IN)" );

        // Additional diagnostic: check the library pin's alternates
        if( schPin->GetLibPin() )
        {
            BOOST_CHECK_MESSAGE( !schPin->GetLibPin()->GetAlternates().empty(),
                                 "Library pin pointed to by schematic pin should have alternates" );
            BOOST_CHECK_MESSAGE( schPin->GetLibPin()->GetAlternates().count( wxS( "8.pow" ) ) > 0,
                                 "Library pin should have '8.pow' in its alternates map" );
        }
    }

    delete symbol;
    delete origLibSymbol;
}


/**
 * Test for issue #22286 - verify behavior when GetType() is called before
 * m_libPin is set. This simulates what happens if GetType() is called
 * during loading before UpdateLocalLibSymbolLinks() runs.
 */
BOOST_AUTO_TEST_CASE( GetTypeBeforeLibPinSet )
{
    // Create a schematic symbol with parser-created raw pin
    SCH_SHEET_PATH path;
    LIB_SYMBOL* libSymbol = new LIB_SYMBOL( "test_symbol", nullptr );

    SCH_PIN* libPin = new SCH_PIN( libSymbol );
    libPin->SetNumber( "8" );
    libPin->SetName( "8" );
    libPin->SetType( ELECTRICAL_PINTYPE::PT_NC );
    libPin->SetPosition( VECTOR2I( 0, 0 ) );

    SCH_PIN::ALT powerAlt;
    powerAlt.m_Name = wxS( "8.pow" );
    powerAlt.m_Shape = GRAPHIC_PINSHAPE::LINE;
    powerAlt.m_Type = ELECTRICAL_PINTYPE::PT_POWER_IN;
    libPin->GetAlternates()[ wxS( "8.pow" ) ] = powerAlt;
    libSymbol->AddDrawItem( libPin );

    // Create symbol using default constructor (like parser does)
    SCH_SYMBOL* symbol = new SCH_SYMBOL();
    symbol->SetLibId( libSymbol->GetLibId() );

    // Add parser-style raw pin with alternate set but no m_libPin
    symbol->GetRawPins().emplace_back(
        std::make_unique<SCH_PIN>( symbol, wxS( "8" ), wxS( "8.pow" ), KIID() ) );

    SCH_PIN* rawPin = symbol->GetRawPins()[0].get();

    // At this point: m_alt is set, m_libPin is nullptr, m_type is PT_INHERIT
    BOOST_CHECK_EQUAL( rawPin->GetAlt(), "8.pow" );
    BOOST_CHECK( rawPin->GetLibPin() == nullptr );

    // GetType() with m_alt set but m_libPin=nullptr should return PT_UNSPECIFIED
    // per the code in GetType()
    BOOST_CHECK_MESSAGE( rawPin->GetType() == ELECTRICAL_PINTYPE::PT_UNSPECIFIED,
                         "GetType() with m_alt set but m_libPin=nullptr should return PT_UNSPECIFIED" );

    // GetShownName() should still return the alternate name
    BOOST_CHECK_EQUAL( rawPin->GetShownName(), "8.pow" );

    // Now set the library symbol which triggers UpdatePins()
    symbol->SetLibSymbol( new LIB_SYMBOL( *libSymbol ) );

    // After UpdatePins(), pin should have correct type
    std::vector<SCH_PIN*> schPins = symbol->GetPins( &path );
    BOOST_CHECK_EQUAL( schPins.size(), 1 );

    if( !schPins.empty() )
    {
        SCH_PIN* schPin = schPins[0];
        BOOST_CHECK_EQUAL( schPin->GetAlt(), "8.pow" );
        BOOST_CHECK( schPin->GetLibPin() != nullptr );
        BOOST_CHECK_MESSAGE( schPin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN,
                             "After SetLibSymbol, GetType() should return PT_POWER_IN" );
    }

    delete symbol;
    delete libSymbol;
}


/**
 * Test for issue #22566 - changing to a symbol with fewer pins should not crash
 */
BOOST_AUTO_TEST_CASE( ChangeSymbolFewerPinsNoCrash )
{
    // Create a symbol with multiple pins (pins 1, 2, 3)
    LIB_SYMBOL* multiPinPart = new LIB_SYMBOL( "multi_pin_part", nullptr );

    SCH_PIN* pin1 = new SCH_PIN( multiPinPart );
    pin1->SetNumber( "1" );
    pin1->SetName( "PIN1" );
    pin1->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
    pin1->SetPosition( VECTOR2I( 0, 0 ) );
    multiPinPart->AddDrawItem( pin1 );

    SCH_PIN* pin2 = new SCH_PIN( multiPinPart );
    pin2->SetNumber( "2" );
    pin2->SetName( "PIN2" );
    pin2->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
    pin2->SetPosition( VECTOR2I( 100, 0 ) );
    multiPinPart->AddDrawItem( pin2 );

    SCH_PIN* pin3 = new SCH_PIN( multiPinPart );
    pin3->SetNumber( "3" );
    pin3->SetName( "PIN3" );
    pin3->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
    pin3->SetPosition( VECTOR2I( 200, 0 ) );
    multiPinPart->AddDrawItem( pin3 );

    SCH_SHEET_PATH path;
    SCH_SYMBOL* symbol = new SCH_SYMBOL( *multiPinPart, multiPinPart->GetLibId(), &path, 0, 0,
                                          VECTOR2I( 0, 0 ) );
    symbol->SetRef( &path, "U1" );
    symbol->UpdatePins();

    BOOST_CHECK_EQUAL( symbol->GetPins( &path ).size(), 3 );

    // Create a symbol with only one pin (pin 1)
    LIB_SYMBOL* singlePinPart = new LIB_SYMBOL( "single_pin_part", nullptr );

    SCH_PIN* newPin1 = new SCH_PIN( singlePinPart );
    newPin1->SetNumber( "1" );
    newPin1->SetName( "NEW_PIN1" );
    newPin1->SetType( ELECTRICAL_PINTYPE::PT_OUTPUT );
    newPin1->SetPosition( VECTOR2I( 0, 0 ) );
    singlePinPart->AddDrawItem( newPin1 );

    // Change to the single-pin symbol - this should not crash
    symbol->SetLibSymbol( singlePinPart->Flatten().release() );

    // Verify the symbol now has only one pin
    BOOST_CHECK_EQUAL( symbol->GetPins( &path ).size(), 1 );

    // Verify GetPin returns the correct pin for the new lib pin
    std::vector<SCH_PIN*> libPins = symbol->GetLibSymbolRef()->GetPins();
    BOOST_CHECK_EQUAL( libPins.size(), 1 );

    SCH_PIN* schPin = symbol->GetPin( libPins[0] );
    BOOST_CHECK( schPin != nullptr );

    if( schPin )
    {
        BOOST_CHECK_EQUAL( schPin->GetNumber(), "1" );
    }

    delete symbol;
    delete multiPinPart;
    delete singlePinPart;
}

BOOST_AUTO_TEST_SUITE_END()
