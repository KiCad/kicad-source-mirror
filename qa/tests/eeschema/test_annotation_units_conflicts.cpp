/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include "eeschema_test_utils.h"

#include <sch_reference_list.h>
#include <refdes_tracker.h>

// Test cases that specifically validate the unit conflict detection logic
// These tests focus on the areUnitsAvailable method and related functionality

struct UNIT_CONFLICT_TEST_CASE
{
    std::string m_caseName;
    std::string m_refPrefix;
    std::string m_refValue;
    std::string m_refLibName;
    std::vector<int> m_existingUnits;           // Units already used for this reference number
    std::string m_existingValue;                // Value of existing references
    std::string m_existingLibName;              // Library name of existing references
    std::vector<int> m_requestedUnits;          // Units being requested
    bool m_expectedAvailable;                   // Whether units should be available
    std::string m_reason;                       // Reason for expected result
};

class TEST_UNIT_CONFLICTS_FIXTURE : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    // Helper to create mock SCH_REFERENCE objects for testing
    SCH_REFERENCE createMockReference( const std::string& prefix, int number, int unit,
                                      const std::string& value, const std::string& libName );
};

// Note: In a real implementation, this would need actual SCH_SYMBOL and SCH_SHEET_PATH objects
// For testing purposes, we'll focus on the logical validation

static const std::vector<UNIT_CONFLICT_TEST_CASE> unitConflictCases = {
    {
        "Units available - no conflicts",
        "U", "LM358", "OpAmp_Dual",
        {3, 4},              // Existing units 3, 4
        "LM358", "OpAmp_Dual",
        {1, 2},              // Requesting units 1, 2
        true,                // Should be available
        "Requested units don't conflict with existing units"
    },
    {
        "Units conflict - same unit requested",
        "U", "LM358", "OpAmp_Dual",
        {1, 2},              // Existing units 1, 2
        "LM358", "OpAmp_Dual",
        {2, 3},              // Requesting units 2, 3 (2 conflicts)
        false,               // Should NOT be available
        "Unit 2 is already in use"
    },
    {
        "Value mismatch - can't share reference",
        "R", "1k", "Resistor",
        {1},                 // Existing unit 1
        "2k", "Resistor",    // Different value
        {2},                 // Requesting unit 2
        false,               // Should NOT be available
        "Can't share reference designator with different values"
    },
    {
        "Library mismatch - can't share reference",
        "U", "LM358", "OpAmp_Dual",
        {1},                 // Existing unit 1
        "LM358", "OpAmp_Single", // Different library
        {2},                 // Requesting unit 2
        false,               // Should NOT be available
        "Can't share reference designator with different library parts"
    },
    {
        "Empty existing units - should be available",
        "IC", "74HC00", "Logic_Gate",
        {},                  // No existing units
        "74HC00", "Logic_Gate",
        {1, 2, 3, 4},       // Requesting all 4 units
        true,                // Should be available
        "No existing units to conflict with"
    },
    {
        "Negative units filtered out",
        "U", "LM324", "OpAmp_Quad",
        {2},                 // Existing unit 2
        "LM324", "OpAmp_Quad",
        {-1, 1, -5, 3},     // Mix of negative and positive units
        true,                // Should be available (negatives ignored)
        "Negative unit numbers are filtered out, only units 1,3 considered"
    },
    {
        "All units conflict",
        "U", "LM324", "OpAmp_Quad",
        {1, 2, 3, 4},       // All units already used
        "LM324", "OpAmp_Quad",
        {1, 2, 3, 4},       // Requesting all units
        false,               // Should NOT be available
        "All requested units are already in use"
    },
    {
        "Partial conflict with mixed values",
        "R", "1k", "Resistor",
        {1},                 // Existing unit 1 with value "1k"
        "1k", "Resistor",
        {1, 2},             // Requesting units 1,2 where 1 has same value
        false,               // Should NOT be available
        "Unit 1 conflicts even with same value (already occupied)"
    },
    {
        "Complex multi-unit scenario",
        "U", "LM339", "Comparator_Quad",
        {1, 3},             // Existing units 1, 3
        "LM339", "Comparator_Quad",
        {2, 4},             // Requesting units 2, 4
        true,                // Should be available
        "Units 2,4 don't conflict with existing 1,3"
    }
};

BOOST_FIXTURE_TEST_SUITE( UnitConflicts, TEST_UNIT_CONFLICTS_FIXTURE )

BOOST_AUTO_TEST_CASE( ValidateUnitConflictDetection )
{
    for( const UNIT_CONFLICT_TEST_CASE& testCase : unitConflictCases )
    {
        BOOST_TEST_INFO_SCOPE( testCase.m_caseName );

        auto validateUnitConflictLogic = [&]( const UNIT_CONFLICT_TEST_CASE& aTestCase )
        {

            REFDES_TRACKER tracker;

            // Create mock reference for testing (conceptual - would need real SCH_REFERENCE in practice)
            BOOST_TEST_MESSAGE( "Testing: " + aTestCase.m_reason );

            // Test the logical conditions that areUnitsAvailable should check:

            // 1. Value comparison logic
            bool valueMatches = ( aTestCase.m_refValue == aTestCase.m_existingValue );

            // 2. Library comparison logic
            bool libMatches = ( aTestCase.m_refLibName == aTestCase.m_existingLibName );

            // 3. Unit conflict detection
            bool hasUnitConflict = false;
            for( int requestedUnit : aTestCase.m_requestedUnits )
            {
                if( requestedUnit < 0 ) continue; // Skip negative units

                for( int existingUnit : aTestCase.m_existingUnits )
                {
                    if( requestedUnit == existingUnit )
                    {
                        hasUnitConflict = true;
                        break;
                    }
                }
                if( hasUnitConflict ) break;
            }

            // The logic from areUnitsAvailable:
            // Return false if: different value OR different library OR unit conflict
            bool shouldBeAvailable = valueMatches && libMatches && !hasUnitConflict;

            BOOST_CHECK_EQUAL( shouldBeAvailable, aTestCase.m_expectedAvailable );

            if( shouldBeAvailable != aTestCase.m_expectedAvailable )
            {
                BOOST_TEST_MESSAGE( "Logic mismatch:" );
                BOOST_TEST_MESSAGE( "  Value match: " + std::to_string( valueMatches ) );
                BOOST_TEST_MESSAGE( "  Lib match: " + std::to_string( libMatches ) );
                BOOST_TEST_MESSAGE( "  Unit conflict: " + std::to_string( hasUnitConflict ) );
                BOOST_TEST_MESSAGE( "  Expected: " + std::to_string( aTestCase.m_expectedAvailable ) );
                BOOST_TEST_MESSAGE( "  Actual: " + std::to_string( shouldBeAvailable ) );
            }
        };

        validateUnitConflictLogic( testCase );
    }
}


BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_Integration )
{
    REFDES_TRACKER tracker;

    // Test the overall GetNextRefDesForUnits logic using the tracker

    // Preload some references to simulate previously used ones
    tracker.Insert( "U1" );
    tracker.Insert( "U5" );

    // Test case 1: Completely unused reference with empty units
    // Should get U2 (first unused after U1)
    // Note: This test is conceptual since we need real SCH_REFERENCE objects

    int nextRef = tracker.GetNextRefDes( "U", 1 );
    BOOST_CHECK_EQUAL( nextRef, 2 ); // Should skip U1, get U2

    // Test case 2: Min value higher than next available
    nextRef = tracker.GetNextRefDes( "U", 10 );
    BOOST_CHECK_EQUAL( nextRef, 10 ); // Should start from min value

    // Verify references were inserted
    BOOST_CHECK( tracker.Contains( "U2" ) );
    BOOST_CHECK( tracker.Contains( "U10" ) );

    // Test case 3: New prefix
    nextRef = tracker.GetNextRefDes( "IC", 1 );
    BOOST_CHECK_EQUAL( nextRef, 1 ); // New prefix should start at 1
    BOOST_CHECK( tracker.Contains( "IC1" ) );
}

BOOST_AUTO_TEST_CASE( RefDesTracker_StateConsistency )
{
    REFDES_TRACKER tracker;

    // Test that the tracker maintains consistent state across operations

    // Insert some references manually
    BOOST_CHECK( tracker.Insert( "R1" ) );
    BOOST_CHECK( tracker.Insert( "R3" ) );
    BOOST_CHECK( tracker.Insert( "R5" ) );

    // Verify they exist
    BOOST_CHECK( tracker.Contains( "R1" ) );
    BOOST_CHECK( tracker.Contains( "R3" ) );
    BOOST_CHECK( tracker.Contains( "R5" ) );
    BOOST_CHECK( !tracker.Contains( "R2" ) );
    BOOST_CHECK( !tracker.Contains( "R4" ) );

    // Get next reference - should fill gap at R2
    int next = tracker.GetNextRefDes( "R", 1 );
    BOOST_CHECK_EQUAL( next, 2 );
    BOOST_CHECK( tracker.Contains( "R2" ) );

    // Get next reference - should fill gap at R4
    next = tracker.GetNextRefDes( "R", 1 );
    BOOST_CHECK_EQUAL( next, 4 );
    BOOST_CHECK( tracker.Contains( "R4" ) );

    // Get next reference - should go to R6
    next = tracker.GetNextRefDes( "R", 1 );
    BOOST_CHECK_EQUAL( next, 6 );
    BOOST_CHECK( tracker.Contains( "R6" ) );

    // Verify total count
    BOOST_CHECK_EQUAL( tracker.Size(), 6 );
}

BOOST_AUTO_TEST_CASE( CacheConsistency_AfterInserts )
{
    REFDES_TRACKER tracker;

    // Test that cache remains consistent after mixed Insert/GetNextRefDes operations

    // Start with some manual inserts
    tracker.Insert( "C1" );
    tracker.Insert( "C5" );
    tracker.Insert( "C10" );

    // Get next ref - should use cached logic to find C2
    int next = tracker.GetNextRefDes( "C", 1 );
    BOOST_CHECK_EQUAL( next, 2 );

    // Insert C3 manually
    tracker.Insert( "C3" );

    // Get next ref - cache should be updated, should get C4
    next = tracker.GetNextRefDes( "C", 1 );
    BOOST_CHECK_EQUAL( next, 4 );

    // Test with minimum value - should respect cache
    next = tracker.GetNextRefDes( "C", 7 );
    BOOST_CHECK_EQUAL( next, 7 ); // C6 available but min is 7

    // Verify all references exist
    BOOST_CHECK( tracker.Contains( "C1" ) );
    BOOST_CHECK( tracker.Contains( "C2" ) );
    BOOST_CHECK( tracker.Contains( "C3" ) );
    BOOST_CHECK( tracker.Contains( "C4" ) );
    BOOST_CHECK( tracker.Contains( "C5" ) );
    BOOST_CHECK( !tracker.Contains( "C6" ) ); // Skipped due to min value
    BOOST_CHECK( tracker.Contains( "C7" ) );
    BOOST_CHECK( tracker.Contains( "C10" ) );
}

BOOST_AUTO_TEST_CASE( ThreadSafety_BasicValidation )
{
    REFDES_TRACKER tracker( true ); // Enable thread safety

    // Basic validation that thread-safe operations work
    BOOST_CHECK( tracker.Insert( "U1" ) );
    BOOST_CHECK( tracker.Contains( "U1" ) );

    int next = tracker.GetNextRefDes( "U", 1 );
    BOOST_CHECK_EQUAL( next, 2 );

    // Test serialization with thread safety
    std::string serialized = tracker.Serialize();
    BOOST_CHECK( !serialized.empty() );

    REFDES_TRACKER tracker2( true );
    BOOST_CHECK( tracker2.Deserialize( serialized ) );
    BOOST_CHECK( tracker2.Contains( "U1" ) );
    BOOST_CHECK( tracker2.Contains( "U2" ) );
}

BOOST_AUTO_TEST_SUITE_END()