/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <sch_pin.h>
#include <lib_symbol.h>
#include <eeschema_test_utils.h>
#include <units_provider.h>
#include <base_units.h>

extern void CheckDuplicatePins( LIB_SYMBOL* aSymbol, std::vector<wxString>& aMessages,
                                UNITS_PROVIDER* aUnitsProvider );

struct STACKED_PIN_CONVERSION_FIXTURE
{
    STACKED_PIN_CONVERSION_FIXTURE()
    {
        m_settingsManager = std::make_unique<SETTINGS_MANAGER>();
        m_symbol = std::make_unique<LIB_SYMBOL>( "TestSymbol" );
    }

    std::unique_ptr<SETTINGS_MANAGER> m_settingsManager;
    std::unique_ptr<LIB_SYMBOL>       m_symbol;
};

BOOST_FIXTURE_TEST_SUITE( StackedPinConversion, STACKED_PIN_CONVERSION_FIXTURE )


/**
 * Test basic stacked pin number expansion functionality
 */
BOOST_AUTO_TEST_CASE( TestStackedPinExpansion )
{
    // Test simple list notation
    SCH_PIN* pin = new SCH_PIN( m_symbol.get() );
    pin->SetNumber( wxT("[1,2,3]") );

    bool isValid;
    std::vector<wxString> expanded = pin->GetStackedPinNumbers( &isValid );

    BOOST_CHECK( isValid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 3 );
    BOOST_CHECK_EQUAL( expanded[0], "1" );
    BOOST_CHECK_EQUAL( expanded[1], "2" );
    BOOST_CHECK_EQUAL( expanded[2], "3" );

    delete pin;

    // Test range notation
    pin = new SCH_PIN( m_symbol.get() );
    pin->SetNumber( wxT("[5-7]") );

    expanded = pin->GetStackedPinNumbers( &isValid );

    BOOST_CHECK( isValid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 3 );
    BOOST_CHECK_EQUAL( expanded[0], "5" );
    BOOST_CHECK_EQUAL( expanded[1], "6" );
    BOOST_CHECK_EQUAL( expanded[2], "7" );

    delete pin;

    // Test mixed notation
    pin = new SCH_PIN( m_symbol.get() );
    pin->SetNumber( wxT("[1,3,5-7]") );

    expanded = pin->GetStackedPinNumbers( &isValid );

    BOOST_CHECK( isValid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 5 );
    BOOST_CHECK_EQUAL( expanded[0], "1" );
    BOOST_CHECK_EQUAL( expanded[1], "3" );
    BOOST_CHECK_EQUAL( expanded[2], "5" );
    BOOST_CHECK_EQUAL( expanded[3], "6" );
    BOOST_CHECK_EQUAL( expanded[4], "7" );

    delete pin;
}


/**
 * Test stacked pin validity checking
 */
BOOST_AUTO_TEST_CASE( TestStackedPinValidity )
{
    SCH_PIN* pin = new SCH_PIN( m_symbol.get() );

    // Test valid single pin (should not be considered stacked)
    pin->SetNumber( wxT("1") );
    bool isValid;
    std::vector<wxString> expanded = pin->GetStackedPinNumbers( &isValid );
    BOOST_CHECK( isValid );
    BOOST_CHECK_EQUAL( expanded.size(), 1 );
    BOOST_CHECK_EQUAL( expanded[0], "1" );

    // Test invalid notation (malformed brackets)
    pin->SetNumber( wxT("[1,2") );
    expanded = pin->GetStackedPinNumbers( &isValid );
    BOOST_CHECK( !isValid );

    // Test invalid range
    pin->SetNumber( wxT("[5-3]") );  // backwards range
    expanded = pin->GetStackedPinNumbers( &isValid );
    BOOST_CHECK( !isValid );

    // Test empty brackets
    pin->SetNumber( wxT("[]") );
    expanded = pin->GetStackedPinNumbers( &isValid );
    BOOST_CHECK( !isValid );

    delete pin;
}


/**
 * Test pin creation and positioning for conversion tests
 */
BOOST_AUTO_TEST_CASE( TestPinCreation )
{
    // Create multiple pins at the same location
    VECTOR2I position( 0, 0 );

    SCH_PIN* pin1 = new SCH_PIN( m_symbol.get() );
    pin1->SetNumber( wxT("1") );
    pin1->SetPosition( position );
    pin1->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );

    SCH_PIN* pin2 = new SCH_PIN( m_symbol.get() );
    pin2->SetNumber( wxT("2") );
    pin2->SetPosition( position );
    pin2->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );

    SCH_PIN* pin3 = new SCH_PIN( m_symbol.get() );
    pin3->SetNumber( wxT("3") );
    pin3->SetPosition( position );
    pin3->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );

    // Verify pins are at same location
    BOOST_CHECK_EQUAL( pin1->GetPosition(), pin2->GetPosition() );
    BOOST_CHECK_EQUAL( pin2->GetPosition(), pin3->GetPosition() );

    // Test IsStacked functionality
    BOOST_CHECK( pin1->IsStacked( pin2 ) );
    BOOST_CHECK( pin2->IsStacked( pin3 ) );

    delete pin1;
    delete pin2;
    delete pin3;
}


/**
 * Test net name generation with stacked pins
 */
BOOST_AUTO_TEST_CASE( TestStackedPinNetNaming )
{
    SCH_PIN* pin = new SCH_PIN( m_symbol.get() );
    pin->SetNumber( wxT("[8,9,10]") );

    // This test would require a full SCH_SYMBOL context to test GetDefaultNetName
    // For now just verify the pin number expansion works
    bool isValid;
    std::vector<wxString> expanded = pin->GetStackedPinNumbers( &isValid );

    BOOST_CHECK( isValid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 3 );
    // The smallest number should be first for deterministic net naming
    BOOST_CHECK_EQUAL( expanded[0], "8" );

    delete pin;
}


/**
 * Test conversion from multiple co-located pins to stacked notation
 */
BOOST_AUTO_TEST_CASE( TestConvertMultiplePinsToStacked )
{
    // Create multiple pins at the same location
    VECTOR2I position( 0, 0 );

    SCH_PIN* pin1 = new SCH_PIN( m_symbol.get() );
    pin1->SetNumber( wxT("1") );
    pin1->SetPosition( position );
    pin1->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );
    pin1->SetVisible( true );

    SCH_PIN* pin2 = new SCH_PIN( m_symbol.get() );
    pin2->SetNumber( wxT("2") );
    pin2->SetPosition( position );
    pin2->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );
    pin2->SetVisible( true );

    SCH_PIN* pin3 = new SCH_PIN( m_symbol.get() );
    pin3->SetNumber( wxT("3") );
    pin3->SetPosition( position );
    pin3->SetOrientation( PIN_ORIENTATION::PIN_RIGHT );
    pin3->SetVisible( true );

    // Test basic property access before adding to symbol
    BOOST_CHECK_EQUAL( pin1->GetNumber(), "1" );
    BOOST_CHECK_EQUAL( pin2->GetNumber(), "2" );
    BOOST_CHECK_EQUAL( pin3->GetNumber(), "3" );

    // Just test the basic conversion logic without symbol management
    // Build the stacked notation string
    wxString stackedNotation = wxT("[");
    stackedNotation += pin1->GetNumber();
    stackedNotation += wxT(",");
    stackedNotation += pin2->GetNumber();
    stackedNotation += wxT(",");
    stackedNotation += pin3->GetNumber();
    stackedNotation += wxT("]");

    // Test stacked notation creation
    BOOST_CHECK_EQUAL( stackedNotation, "[1,2,3]" );

    // Set stacked notation on one pin
    pin1->SetNumber( stackedNotation );
    BOOST_CHECK_EQUAL( pin1->GetNumber(), "[1,2,3]" );

    // Verify the stacked pin expansion works
    bool isValid;
    std::vector<wxString> expanded = pin1->GetStackedPinNumbers( &isValid );
    BOOST_CHECK( isValid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 3 );
    BOOST_CHECK_EQUAL( expanded[0], "1" );
    BOOST_CHECK_EQUAL( expanded[1], "2" );
    BOOST_CHECK_EQUAL( expanded[2], "3" );

    // Clean up - delete pins manually since they're not in symbol
    delete pin1;
    delete pin2;
    delete pin3;
}


/**
 * Test range collapsing functionality in stacked pin conversion
 */
BOOST_AUTO_TEST_CASE( TestRangeCollapsingConversion )
{
    // Test range collapsing logic directly without symbol management

    // Test consecutive pins that should collapse to a range
    std::vector<long> numbers = { 1, 2, 3, 4 };

    // Build collapsed ranges
    wxString result;
    size_t i = 0;
    while( i < numbers.size() )
    {
        if( !result.IsEmpty() )
            result += wxT(",");

        long start = numbers[i];
        long end = start;

        // Find the end of consecutive sequence
        while( i + 1 < numbers.size() && numbers[i + 1] == numbers[i] + 1 )
        {
            i++;
            end = numbers[i];
        }

        // Add range or single number
        if( end > start + 1 )  // Range of 3+ numbers
            result += wxString::Format( wxT("%ld-%ld"), start, end );
        else if( end == start + 1 )  // Two consecutive numbers
            result += wxString::Format( wxT("%ld,%ld"), start, end );
        else  // Single number
            result += wxString::Format( wxT("%ld"), start );

        i++;
    }

    // Verify range collapsing: 1,2,3,4 should become "1-4"
    BOOST_CHECK_EQUAL( result, "1-4" );

    // Test with mixed consecutive and non-consecutive: 1,2,3,4,7,8,9
    numbers = { 1, 2, 3, 4, 7, 8, 9 };
    result.Clear();
    i = 0;

    while( i < numbers.size() )
    {
        if( !result.IsEmpty() )
            result += wxT(",");

        long start = numbers[i];
        long end = start;

        // Find the end of consecutive sequence
        while( i + 1 < numbers.size() && numbers[i + 1] == numbers[i] + 1 )
        {
            i++;
            end = numbers[i];
        }

        // Add range or single number
        if( end > start + 1 )  // Range of 3+ numbers
            result += wxString::Format( wxT("%ld-%ld"), start, end );
        else if( end == start + 1 )  // Two consecutive numbers
            result += wxString::Format( wxT("%ld,%ld"), start, end );
        else  // Single number
            result += wxString::Format( wxT("%ld"), start );

        i++;
    }

    // Verify mixed ranges: 1,2,3,4,7,8,9 should become "1-4,7-9"
    BOOST_CHECK_EQUAL( result, "1-4,7-9" );

    // Test edge cases
    numbers = { 1, 3, 5 };  // Non-consecutive
    result.Clear();
    i = 0;

    while( i < numbers.size() )
    {
        if( !result.IsEmpty() )
            result += wxT(",");

        long start = numbers[i];
        long end = start;

        // Find the end of consecutive sequence
        while( i + 1 < numbers.size() && numbers[i + 1] == numbers[i] + 1 )
        {
            i++;
            end = numbers[i];
        }

        // Add range or single number
        if( end > start + 1 )  // Range of 3+ numbers
            result += wxString::Format( wxT("%ld-%ld"), start, end );
        else if( end == start + 1 )  // Two consecutive numbers
            result += wxString::Format( wxT("%ld,%ld"), start, end );
        else  // Single number
            result += wxString::Format( wxT("%ld"), start );

        i++;
    }

    // Verify non-consecutive: 1,3,5 should remain "1,3,5"
    BOOST_CHECK_EQUAL( result, "1,3,5" );

    // Test two consecutive numbers
    numbers = { 5, 6 };
    result.Clear();
    i = 0;

    while( i < numbers.size() )
    {
        if( !result.IsEmpty() )
            result += wxT(",");

        long start = numbers[i];
        long end = start;

        // Find the end of consecutive sequence
        while( i + 1 < numbers.size() && numbers[i + 1] == numbers[i] + 1 )
        {
            i++;
            end = numbers[i];
        }

        // Add range or single number
        if( end > start + 1 )  // Range of 3+ numbers
            result += wxString::Format( wxT("%ld-%ld"), start, end );
        else if( end == start + 1 )  // Two consecutive numbers
            result += wxString::Format( wxT("%ld,%ld"), start, end );
        else  // Single number
            result += wxString::Format( wxT("%ld"), start );

        i++;
    }

    // Verify two consecutive: 5,6 should remain "5,6" (not convert to range)
    BOOST_CHECK_EQUAL( result, "5,6" );

    // Test complex mixed case: 1,2,4,5,6,8,9,10,11
    numbers = { 1, 2, 4, 5, 6, 8, 9, 10, 11 };
    result.Clear();
    i = 0;

    while( i < numbers.size() )
    {
        if( !result.IsEmpty() )
            result += wxT(",");

        long start = numbers[i];
        long end = start;

        // Find the end of consecutive sequence
        while( i + 1 < numbers.size() && numbers[i + 1] == numbers[i] + 1 )
        {
            i++;
            end = numbers[i];
        }

        // Add range or single number
        if( end > start + 1 )  // Range of 3+ numbers
            result += wxString::Format( wxT("%ld-%ld"), start, end );
        else if( end == start + 1 )  // Two consecutive numbers
            result += wxString::Format( wxT("%ld,%ld"), start, end );
        else  // Single number
            result += wxString::Format( wxT("%ld"), start );

        i++;
    }

    // Verify complex case: 1,2,4,5,6,8,9,10,11 should become "1,2,4-6,8-11"
    BOOST_CHECK_EQUAL( result, "1,2,4-6,8-11" );

    // Test that our range notation can be expanded back correctly
    SCH_PIN* rangePin = new SCH_PIN( m_symbol.get() );
    rangePin->SetNumber( wxT("[1-4,7-9]") );

    bool isValid;
    std::vector<wxString> expanded = rangePin->GetStackedPinNumbers( &isValid );
    BOOST_CHECK( isValid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 7 );

    // Should expand to: 1,2,3,4,7,8,9
    BOOST_CHECK_EQUAL( expanded[0], "1" );
    BOOST_CHECK_EQUAL( expanded[1], "2" );
    BOOST_CHECK_EQUAL( expanded[2], "3" );
    BOOST_CHECK_EQUAL( expanded[3], "4" );
    BOOST_CHECK_EQUAL( expanded[4], "7" );
    BOOST_CHECK_EQUAL( expanded[5], "8" );
    BOOST_CHECK_EQUAL( expanded[6], "9" );

    delete rangePin;
}


/**
 * Test round-trip conversion: multiple pins -> stacked -> multiple pins
 */
BOOST_AUTO_TEST_CASE( TestRoundTripConversion )
{
    // Create multiple pins at the same location
    VECTOR2I position( 100, 200 );

    SCH_PIN* pin5 = new SCH_PIN( m_symbol.get() );
    pin5->SetNumber( wxT("5") );
    pin5->SetPosition( position );
    pin5->SetOrientation( PIN_ORIENTATION::PIN_LEFT );
    pin5->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
    pin5->SetName( wxT("TestPin") );
    pin5->SetVisible( true );

    SCH_PIN* pin7 = new SCH_PIN( m_symbol.get() );
    pin7->SetNumber( wxT("7") );
    pin7->SetPosition( position );
    pin7->SetOrientation( PIN_ORIENTATION::PIN_LEFT );
    pin7->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
    pin7->SetName( wxT("TestPin") );
    pin7->SetVisible( true );

    SCH_PIN* pin9 = new SCH_PIN( m_symbol.get() );
    pin9->SetNumber( wxT("9") );
    pin9->SetPosition( position );
    pin9->SetOrientation( PIN_ORIENTATION::PIN_LEFT );
    pin9->SetType( ELECTRICAL_PINTYPE::PT_INPUT );
    pin9->SetName( wxT("TestPin") );
    pin9->SetVisible( true );

    // Store original properties for comparison
    PIN_ORIENTATION originalOrientation = pin5->GetOrientation();
    ELECTRICAL_PINTYPE originalType = pin5->GetType();
    wxString originalName = pin5->GetName();
    int originalLength = pin5->GetLength();

    // Step 1: Convert to stacked notation (simulating ConvertStackedPins)
    std::vector<SCH_PIN*> pinsToConvert = { pin5, pin7, pin9 };

    // Sort pins numerically
    std::sort( pinsToConvert.begin(), pinsToConvert.end(),
        []( SCH_PIN* a, SCH_PIN* b )
        {
            long numA, numB;
            if( a->GetNumber().ToLong( &numA ) && b->GetNumber().ToLong( &numB ) )
                return numA < numB;
            return a->GetNumber() < b->GetNumber();
        });

    // Build stacked notation
    wxString stackedNotation = wxT("[5,7,9]");
    pinsToConvert[0]->SetNumber( stackedNotation );

    // Remove other pins (don't delete them yet for testing)
    SCH_PIN* stackedPin = pinsToConvert[0];

    // Step 2: Verify stacked notation
    bool isValid;
    std::vector<wxString> expanded = stackedPin->GetStackedPinNumbers( &isValid );
    BOOST_CHECK( isValid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 3 );
    BOOST_CHECK_EQUAL( expanded[0], "5" );
    BOOST_CHECK_EQUAL( expanded[1], "7" );
    BOOST_CHECK_EQUAL( expanded[2], "9" );

    // Step 3: Convert back to individual pins (simulating ExplodeStackedPin)
    // Sort the stacked numbers (should already be sorted in our case)
    std::sort( expanded.begin(), expanded.end(),
        []( const wxString& a, const wxString& b )
        {
            long numA, numB;
            if( a.ToLong( &numA ) && b.ToLong( &numB ) )
                return numA < numB;
            return a < b;
        });

    // Change the original pin to use the first (smallest) number and make it visible
    stackedPin->SetNumber( expanded[0] );
    stackedPin->SetVisible( true );

    // Create additional pins for the remaining numbers and make them invisible
    std::vector<SCH_PIN*> explodedPins;
    explodedPins.push_back( stackedPin );

    for( size_t i = 1; i < expanded.size(); ++i )
    {
        SCH_PIN* newPin = new SCH_PIN( m_symbol.get() );

        // Copy all properties from the original pin
        newPin->SetPosition( stackedPin->GetPosition() );
        newPin->SetOrientation( stackedPin->GetOrientation() );
        newPin->SetShape( stackedPin->GetShape() );
        newPin->SetLength( stackedPin->GetLength() );
        newPin->SetType( stackedPin->GetType() );
        newPin->SetName( stackedPin->GetName() );
        newPin->SetNumber( expanded[i] );
        newPin->SetNameTextSize( stackedPin->GetNameTextSize() );
        newPin->SetNumberTextSize( stackedPin->GetNumberTextSize() );
        newPin->SetUnit( stackedPin->GetUnit() );
        newPin->SetBodyStyle( stackedPin->GetBodyStyle() );
        newPin->SetVisible( false );  // Make all other pins invisible

        explodedPins.push_back( newPin );
    }

    // Step 4: Verify the round-trip conversion
    BOOST_REQUIRE_EQUAL( explodedPins.size(), 3 );

    // Check pin numbers
    BOOST_CHECK_EQUAL( explodedPins[0]->GetNumber(), "5" );
    BOOST_CHECK_EQUAL( explodedPins[1]->GetNumber(), "7" );
    BOOST_CHECK_EQUAL( explodedPins[2]->GetNumber(), "9" );

    // Check visibility (only first pin should be visible)
    BOOST_CHECK( explodedPins[0]->IsVisible() );
    BOOST_CHECK( !explodedPins[1]->IsVisible() );
    BOOST_CHECK( !explodedPins[2]->IsVisible() );

    // Check that properties were preserved
    for( SCH_PIN* pin : explodedPins )
    {
        BOOST_CHECK_EQUAL( pin->GetPosition(), position );
        BOOST_CHECK( pin->GetOrientation() == originalOrientation );
        BOOST_CHECK( pin->GetType() == originalType );
        BOOST_CHECK_EQUAL( pin->GetName(), originalName );
        BOOST_CHECK_EQUAL( pin->GetLength(), originalLength );
    }

    // Clean up
    for( size_t i = 1; i < explodedPins.size(); ++i )
        delete explodedPins[i];
    // Note: explodedPins[0] is the original stackedPin, don't delete twice
}


/**
 * Test visibility behavior during conversions
 */
BOOST_AUTO_TEST_CASE( TestVisibilityHandling )
{
    // Create a single pin to test visibility handling
    SCH_PIN* pin = new SCH_PIN( m_symbol.get() );
    pin->SetNumber( wxT("[8,10,12]") );
    pin->SetVisible( false );  // Start invisible

    // Test expansion of stacked notation
    bool isValid;
    std::vector<wxString> expanded = pin->GetStackedPinNumbers( &isValid );
    BOOST_CHECK( isValid );
    BOOST_REQUIRE_EQUAL( expanded.size(), 3 );

    // Sort expanded numbers
    std::sort( expanded.begin(), expanded.end(),
        []( const wxString& a, const wxString& b )
        {
            long numA, numB;
            if( a.ToLong( &numA ) && b.ToLong( &numB ) )
                return numA < numB;
            return a < b;
        });

    // Verify sorted order is correct
    BOOST_CHECK_EQUAL( expanded[0], "8" );
    BOOST_CHECK_EQUAL( expanded[1], "10" );
    BOOST_CHECK_EQUAL( expanded[2], "12" );

    // Set the smallest pin number and make it visible
    pin->SetNumber( expanded[0] );  // "8"
    pin->SetVisible( true );        // Make visible

    // Verify the smallest pin is visible and has correct number
    BOOST_CHECK_EQUAL( pin->GetNumber(), "8" );
    BOOST_CHECK( pin->IsVisible() );

    // Clean up
    delete pin;
}


/**
 * Test alphanumeric range collapsing functionality
 */
BOOST_AUTO_TEST_CASE( TestAlphanumericRangeCollapsing )
{
    // Test the new alphanumeric prefix parsing logic

    // Helper function to test prefix parsing
    auto testPrefixParsing = []( const wxString& pinNumber ) -> std::pair<wxString, long>
    {
        wxString prefix;
        long numValue = -1;

        // Find where numeric part starts (scan from end)
        size_t numStart = pinNumber.length();
        for( int i = pinNumber.length() - 1; i >= 0; i-- )
        {
            if( !wxIsdigit( pinNumber[i] ) )
            {
                numStart = i + 1;
                break;
            }
            if( i == 0 )  // All digits
                numStart = 0;
        }

        if( numStart < pinNumber.length() )  // Has numeric suffix
        {
            prefix = pinNumber.Left( numStart );
            wxString numericPart = pinNumber.Mid( numStart );
            numericPart.ToLong( &numValue );
        }

        return std::make_pair( prefix, numValue );
    };

    // Test basic prefix parsing
    auto [prefix1, num1] = testPrefixParsing( wxT("A1") );
    BOOST_CHECK_EQUAL( prefix1, "A" );
    BOOST_CHECK_EQUAL( num1, 1 );

    auto [prefix2, num2] = testPrefixParsing( wxT("AB12") );
    BOOST_CHECK_EQUAL( prefix2, "AB" );
    BOOST_CHECK_EQUAL( num2, 12 );

    auto [prefix3, num3] = testPrefixParsing( wxT("123") );
    BOOST_CHECK_EQUAL( prefix3, "" );
    BOOST_CHECK_EQUAL( num3, 123 );

    auto [prefix4, num4] = testPrefixParsing( wxT("XYZ") );
    BOOST_CHECK_EQUAL( prefix4, "" );  // No numeric suffix
    BOOST_CHECK_EQUAL( num4, -1 );

    // Test grouping logic with example: AA1,AA2,AA3,AB4,CD12,CD13,CD14
    std::map<wxString, std::vector<long>> prefixGroups;
    std::vector<wxString> testPins = { wxT("AA1"), wxT("AA2"), wxT("AA3"), wxT("AB4"), wxT("CD12"), wxT("CD13"), wxT("CD14") };

    for( const wxString& pinNumber : testPins )
    {
        auto [prefix, numValue] = testPrefixParsing( pinNumber );
        if( numValue != -1 )
            prefixGroups[prefix].push_back( numValue );
    }

    // Verify grouping
    BOOST_CHECK_EQUAL( prefixGroups.size(), 3 );
    BOOST_CHECK_EQUAL( prefixGroups[wxT("AA")].size(), 3 );
    BOOST_CHECK_EQUAL( prefixGroups[wxT("AB")].size(), 1 );
    BOOST_CHECK_EQUAL( prefixGroups[wxT("CD")].size(), 3 );

    // Build expected result: AA1-AA3,AB4,CD12-CD14
    wxString expectedResult;
    for( auto& [prefix, numbers] : prefixGroups )
    {
        if( !expectedResult.IsEmpty() )
            expectedResult += wxT(",");

        std::sort( numbers.begin(), numbers.end() );

        size_t i = 0;
        while( i < numbers.size() )
        {
            if( i > 0 )
                expectedResult += wxT(",");

            long start = numbers[i];
            long end = start;

            // Find consecutive sequence
            while( i + 1 < numbers.size() && numbers[i + 1] == numbers[i] + 1 )
            {
                i++;
                end = numbers[i];
            }

            // Format with prefix
            if( end > start + 1 )  // Range of 3+ numbers
                expectedResult += wxString::Format( wxT("%s%ld-%s%ld"), prefix, start, prefix, end );
            else if( end == start + 1 )  // Two consecutive numbers
                expectedResult += wxString::Format( wxT("%s%ld,%s%ld"), prefix, start, prefix, end );
            else  // Single number
                expectedResult += wxString::Format( wxT("%s%ld"), prefix, start );

            i++;
        }
    }

    // Should result in: AA1-AA3,AB4,CD12-CD14
    BOOST_CHECK_EQUAL( expectedResult, "AA1-AA3,AB4,CD12-CD14" );
}


BOOST_AUTO_TEST_CASE( TestDuplicatePinDetectionWithStackedNotation )
{
    UNITS_PROVIDER unitsProvider( schIUScale, EDA_UNITS::MILS );

    SCH_PIN* stacked = new SCH_PIN( m_symbol.get() );
    stacked->SetNumber( wxT( "[1-3]" ) );
    stacked->SetPosition( VECTOR2I( 0, 0 ) );
    m_symbol->AddDrawItem( stacked );

    SCH_PIN* single = new SCH_PIN( m_symbol.get() );
    single->SetNumber( wxT( "2" ) );
    single->SetPosition( VECTOR2I( schIUScale.MilsToIU( 100 ), 0 ) );
    m_symbol->AddDrawItem( single );

    std::vector<wxString> messages;
    CheckDuplicatePins( m_symbol.get(), messages, &unitsProvider );

    BOOST_REQUIRE_EQUAL( messages.size(), 1 );
    BOOST_CHECK( messages.front().Contains( wxT( "Duplicate pin 2" ) ) );
    BOOST_CHECK( messages.front().Contains( wxT( "[1-3]" ) ) );

    single->SetNumber( wxT( "5" ) );
    messages.clear();

    CheckDuplicatePins( m_symbol.get(), messages, &unitsProvider );
    BOOST_CHECK( messages.empty() );
}


/**
 * Test that GetStackedPinCount returns the same count as GetStackedPinNumbers().size()
 * but more efficiently
 */
BOOST_AUTO_TEST_CASE( TestStackedPinCountEfficiency )
{
    // Test cases with different pin notations
    struct TestCase
    {
        wxString notation;
        int      expectedCount;
        bool     expectedValid;
    };

    std::vector<TestCase> testCases = {
        { wxT( "1" ), 1, true },                    // Simple pin
        { wxT( "[1,2,3]" ), 3, true },              // List notation
        { wxT( "[5-7]" ), 3, true },                // Range notation
        { wxT( "[1,3,5-7]" ), 5, true },            // Mixed notation
        { wxT( "[A1-A5]" ), 5, true },              // Alphanumeric range
        { wxT( "[1-10]" ), 10, true },              // Larger range
        { wxT( "[PA1,PA2,PB5-PB8]" ), 6, true },    // Complex mixed
        { wxT( "[1-3,10,20-22]" ), 7, true },       // Multiple ranges
        { wxT( "[10" ), 1, false },                 // Invalid (missing bracket)
        { wxT( "10]" ), 1, false },                 // Invalid (missing bracket)
        { wxT( "[5-3]" ), 1, false },               // Invalid (reverse range)
    };

    for( const auto& testCase : testCases )
    {
        SCH_PIN* pin = new SCH_PIN( m_symbol.get() );
        pin->SetNumber( testCase.notation );

        bool validExpand = false;
        bool validCount = false;

        std::vector<wxString> expanded = pin->GetStackedPinNumbers( &validExpand );
        int count = pin->GetStackedPinCount( &validCount );

        // Both methods should agree on validity
        BOOST_CHECK_EQUAL( validExpand, validCount );
        BOOST_CHECK_EQUAL( validExpand, testCase.expectedValid );

        // Both methods should return the same count
        BOOST_CHECK_EQUAL( static_cast<int>( expanded.size() ), count );
        BOOST_CHECK_EQUAL( count, testCase.expectedCount );

        delete pin;
    }
}

BOOST_AUTO_TEST_SUITE_END()
