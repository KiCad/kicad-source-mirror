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

#include <refdes_tracker.h>
#include <sch_reference_list.h>

struct REFDES_UNITS_TEST_CASE
{
    std::string m_caseName;
    std::string m_testRefPrefix;
    std::string m_testRefValue;
    std::map<int, std::vector<std::tuple<std::string, int>>> m_refNumberMap; // Map of ref number to vector of (value, unit) tuples
    std::vector<int> m_requiredUnits;
    int m_minValue;
    int m_expectedResult;
    std::vector<std::string> m_trackerPreloads; // References to preload in tracker
};

class TEST_REFDES_TRACKER_UNITS : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    void runTestCase( const REFDES_UNITS_TEST_CASE& testCase );

    // Helper method to create test references
    SCH_REFERENCE createTestReference( const std::string& aRefPrefix, const std::string& aValue, int aUnit )
    {
        SCH_SYMBOL dummySymbol;
        SCH_SHEET_PATH dummyPath;

        SCH_REFERENCE ref( &dummySymbol, dummyPath );
        ref.SetRef( aRefPrefix );
        ref.SetValue( aValue );
        ref.SetUnit( aUnit );

        return ref;
    }

    // Helper method to setup units checker
    void setupRefDesTracker( REFDES_TRACKER& tracker )
    {
        tracker.SetReuseRefDes( false ); // Disable reuse for these tests
        tracker.SetUnitsChecker( []( const SCH_REFERENCE& aTestRef,
                                     const std::vector<SCH_REFERENCE>& aExistingRefs,
                                     const std::vector<int>& aRequiredUnits )
        {
            // Check if all required units are available
            for( int unit : aRequiredUnits )
            {
                for( const auto& ref : aExistingRefs )
                {
                    if( ref.GetUnit() == unit
                        || ref.CompareValue( aTestRef ) != 0 )
                    {
                        return false; // Conflict found
                    }
                }
            }
            return true; // All required units are available
        } );
    }
};

void TEST_REFDES_TRACKER_UNITS::runTestCase( const REFDES_UNITS_TEST_CASE& testCase )
{
    REFDES_TRACKER tracker;

    // Preload tracker with existing references
    for( const std::string& ref : testCase.m_trackerPreloads )
    {
        tracker.Insert( ref );
    }

    // Create test reference
    SCH_REFERENCE testRef = createTestReference( testCase.m_testRefPrefix, testCase.m_testRefValue, 1 );

    // Convert test case data to actual SCH_REFERENCE map
    std::map<int, std::vector<SCH_REFERENCE>> refNumberMap;
    for( const auto& [refNum, tupleVec] : testCase.m_refNumberMap )
    {
        std::vector<SCH_REFERENCE> refs;
        for( const auto& [value, unit] : tupleVec )
        {
            refs.push_back( createTestReference( testCase.m_testRefPrefix, value, unit ) );
        }
        refNumberMap[refNum] = refs;
    }

    BOOST_TEST_INFO( "Testing case: " + testCase.m_caseName );

    setupRefDesTracker( tracker );

    // Test GetNextRefDesForUnits logic using the 4-parameter method
    int result = tracker.GetNextRefDesForUnits( testRef,
                                               refNumberMap,
                                               testCase.m_requiredUnits,
                                               testCase.m_minValue );

    BOOST_CHECK_EQUAL( result, testCase.m_expectedResult );

    // Additional verification: check that the result reference is in the tracker
    // (unless it was a case where units were available in existing reference)
    std::string resultRefDes = testCase.m_testRefPrefix + std::to_string( result );

    // Check if this reference number was already in use
    bool wasInUse = testCase.m_refNumberMap.find( result ) != testCase.m_refNumberMap.end();

    if( !wasInUse && !testCase.m_requiredUnits.empty() )
    {
        // For completely new references, it should be added to tracker
        BOOST_CHECK( tracker.Contains( resultRefDes ) );
    }
}

static const std::vector<REFDES_UNITS_TEST_CASE> refdesUnitsTestCases = {
    {
        "Case 1: Completely unused reference - empty units",
        "U", "LM358",
        {}, // No currently used references
        {}, // Empty required units (need completely unused)
        1,  // Min value
        1,  // Expected: U1
        {}  // No preloaded references
    },
    {
        "Case 2: Completely unused reference - with units",
        "U", "LM358",
        {}, // No currently used references
        {1, 2}, // Need units 1 and 2
        1,  // Min value
        1,  // Expected: U1
        {}  // No preloaded references
    },
    {
        "Case 3: Skip currently in use reference",
        "U", "LM358",
        {
            { 1, { std::make_tuple("LM358", 1) } } // U1 unit 1 in use
        },
        {1, 2}, // Need units 1 and 2
        1,  // Min value
        2,  // Expected: U2 (U1 conflicts with unit 1)
        {}
    },
    {
        "Case 4: Units available in currently used reference",
        "U", "LM358",
        {
            { 1, { std::make_tuple("LM358", 3),
                   std::make_tuple("LM358", 4) } } // U1 units 3,4 in use
        },
        {1, 2}, // Need units 1 and 2 (available)
        1,  // Min value
        1,  // Expected: U1 (units 1,2 are free)
        {}
    },
    {
        "Case 5: Different value conflict",
        "U", "LM358",
        {
            { 1, { std::make_tuple("LM741", 1) } } // U1 different value
        },
        {1}, // Need unit 1
        1,  // Min value
        2,  // Expected: U2 (can't share with different value)
        {}
    },
    {
        "Case 6: Previously used reference in tracker",
        "U", "LM358",
        {}, // No currently used references
        {1}, // Need unit 1
        1,   // Min value
        2,   // Expected: U2 (U1 was previously used)
        {"U1"} // U1 preloaded in tracker
    },
    {
        "Case 7: Min value higher than available",
        "U", "LM358",
        {
            { 5, { std::make_tuple("LM358", 1) } } // U5 unit 1 in use
        },
        {2}, // Need unit 2
        10,  // Min value = 10
        10,  // Expected: U10 (U5 has unit 2 available, but min value is 10)
        {}
    },
    {
        "Case 8: Negative units filtered out",
        "U", "LM358",
        {},
        {-1, 1, -5, 2}, // Mix of negative and positive units
        1,  // Min value
        1,  // Expected: U1 (only units 1,2 considered)
        {}
    },
    {
        "Case 9: Complex scenario with gaps",
        "IC", "74HC00",
        {
            { 2, { std::make_tuple("74HC00", 1) } }, // IC2 unit 1 used
            { 4, { std::make_tuple("74HC00", 2) } }  // IC4 unit 2 used
        },
        {1, 3}, // Need units 1 and 3
        1,      // Min value
        3,      // Expected: IC3 (IC1 unused, IC2 conflicts unit 1, IC3 available)
        {"IC1"} // IC1 previously used
    }
};

BOOST_FIXTURE_TEST_SUITE( RefDesTrackerUnits, TEST_REFDES_TRACKER_UNITS )

BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_BasicCases )
{
    for( const REFDES_UNITS_TEST_CASE& testCase : refdesUnitsTestCases )
    {
        runTestCase( testCase );
    }
}

BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_EdgeCases )
{
    REFDES_TRACKER tracker;

    // Test empty required units vector - should find completely unused reference
    SCH_REFERENCE testRef = createTestReference( "R", "1k", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> emptyUnits;

    setupRefDesTracker( tracker );
    int result = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( result, 1 );
    BOOST_CHECK( tracker.Contains( "R1" ) );

    // Test with some references already in tracker
    tracker.Insert( "R3" );
    result = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( result, 2 ); // Should skip R1 (already inserted above) and get R2

    // Test with negative units (should be filtered out)
    std::vector<int> mixedUnits = {-1, 1, -5, 2};
    SCH_REFERENCE testRef2 = createTestReference( "C", "100nF", 1 );
    result = tracker.GetNextRefDesForUnits( testRef2, emptyMap, mixedUnits, 1 );
    BOOST_CHECK_EQUAL( result, 1 );
}

BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_UsagePattern )
{
    REFDES_TRACKER tracker;

    // Demonstrate actual usage pattern for GetNextRefDesForUnits with our test helper
    SCH_REFERENCE testRef = createTestReference( "U", "LM358", 1 );

    // Create map of currently used references
    std::map<int, std::vector<SCH_REFERENCE>> refNumberMap;
    refNumberMap[1] = { createTestReference("U", "LM358", 1),
                       createTestReference("U", "LM358", 2) }; // U1 has units 1,2 used
    refNumberMap[3] = { createTestReference("U", "LM358", 1) }; // U3 has unit 1 used

    // Specify required units for new symbol
    std::vector<int> requiredUnits = {1, 2};

    setupRefDesTracker( tracker );

    // Get next available reference number
    int nextRefNum = tracker.GetNextRefDesForUnits( testRef, refNumberMap, requiredUnits, 1 );

    // Should return 2 (U2) since U1 conflicts (units 1,2 already used) and U2 is available
    BOOST_CHECK_EQUAL( nextRefNum, 2 );
    BOOST_CHECK( tracker.Contains( "U2" ) );

    // Test case where units are available in existing reference
    std::vector<int> requiredUnits2 = {3, 4}; // These should be available in U1
    refNumberMap[3] = { createTestReference("U", "LM358", 1) }; // U1 only has unit 1 and 2 used

    nextRefNum = tracker.GetNextRefDesForUnits( testRef, refNumberMap, requiredUnits2, 1 );
    BOOST_CHECK_EQUAL( nextRefNum, 1 ); // U1 should work since units 3,4 are available

    // Test different value conflict
    SCH_REFERENCE differentValueRef = createTestReference( "U", "LM741", 1 );
    refNumberMap[4] = { createTestReference("U", "LM358", 1) }; // U4 has different value

    nextRefNum = tracker.GetNextRefDesForUnits( differentValueRef, refNumberMap, {1}, 4 );
    BOOST_CHECK_EQUAL( nextRefNum, 5 ); // Should skip U4 due to value conflict
}

BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_ThreadSafety )
{
    REFDES_TRACKER tracker( true ); // Enable thread safety

    // Test that GetNextRefDesForUnits works with thread safety enabled
    SCH_REFERENCE testRef = createTestReference( "U", "LM358", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> requiredUnits = {1, 2};

    setupRefDesTracker( tracker );
    int result = tracker.GetNextRefDesForUnits( testRef, emptyMap, requiredUnits, 1 );
    BOOST_CHECK_EQUAL( result, 1 );
    BOOST_CHECK( tracker.Contains( "U1" ) );

    // Test multiple calls work correctly with thread safety
    result = tracker.GetNextRefDesForUnits( testRef, emptyMap, requiredUnits, 1 );
    BOOST_CHECK_EQUAL( result, 2 );
    BOOST_CHECK( tracker.Contains( "U2" ) );

    // Test with conflicts and thread safety
    std::map<int, std::vector<SCH_REFERENCE>> conflictMap;
    conflictMap[3] = { createTestReference("U", "LM358", 1) }; // U3 unit 1 in use

    result = tracker.GetNextRefDesForUnits( testRef, conflictMap, requiredUnits, 1 );
    BOOST_CHECK_EQUAL( result, 4 ); // Should skip U1,U2 (in tracker) and U3 (conflicted)
}

BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_Integration )
{
    REFDES_TRACKER tracker;

    // Test that GetNextRefDesForUnits properly integrates with existing Insert/Contains
    tracker.Insert( "U1" );
    tracker.Insert( "U3" );

    BOOST_CHECK( tracker.Contains( "U1" ) );
    BOOST_CHECK( tracker.Contains( "U3" ) );
    BOOST_CHECK( !tracker.Contains( "U2" ) );

    // Test GetNextRefDesForUnits with preloaded tracker
    SCH_REFERENCE testRef = createTestReference( "U", "LM358", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> requiredUnits = {1, 2};

    setupRefDesTracker( tracker );

    // Should get U2 since U1 is already in tracker (preloaded) and U3 is also preloaded
    int next = tracker.GetNextRefDesForUnits( testRef, emptyMap, requiredUnits, 1 );
    BOOST_CHECK_EQUAL( next, 2 );
    BOOST_CHECK( tracker.Contains( "U2" ) );

    // Test with higher minimum value
    next = tracker.GetNextRefDesForUnits( testRef, emptyMap, requiredUnits, 5 );
    BOOST_CHECK_EQUAL( next, 5 );
    BOOST_CHECK( tracker.Contains( "U5" ) );

    // Test integration with serialization
    std::string serialized = tracker.Serialize();
    REFDES_TRACKER tracker2;
    BOOST_CHECK( tracker2.Deserialize( serialized ) );

    // Verify deserialized tracker has the same state
    BOOST_CHECK( tracker2.Contains( "U1" ) );
    BOOST_CHECK( tracker2.Contains( "U2" ) );
    BOOST_CHECK( tracker2.Contains( "U3" ) );
    BOOST_CHECK( tracker2.Contains( "U5" ) );

    setupRefDesTracker( tracker2 );

    // GetNextRefDesForUnits should work with deserialized tracker
    next = tracker2.GetNextRefDesForUnits( testRef, emptyMap, requiredUnits, 1 );
    BOOST_CHECK_EQUAL( next, 4 ); // Should get U4 (first available after U1,U2,U3,U5)
}

BOOST_AUTO_TEST_CASE( Serialization_WithTrackedReferences )
{
    REFDES_TRACKER tracker;

    // Add some references using both Insert and GetNextRefDesForUnits
    tracker.Insert( "R1" );
    tracker.Insert( "R3" );

    setupRefDesTracker( tracker );

    // Use GetNextRefDesForUnits to get next reference
    SCH_REFERENCE testRef = createTestReference( "R", "1k", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> requiredUnits = {1};

    int next = tracker.GetNextRefDesForUnits( testRef, emptyMap, requiredUnits, 1 );
    BOOST_CHECK_EQUAL( next, 2 ); // Should get R2

    // Test with different prefix
    SCH_REFERENCE capacitorRef = createTestReference( "C", "100nF", 1 );
    next = tracker.GetNextRefDesForUnits( capacitorRef, emptyMap, requiredUnits, 5 );
    BOOST_CHECK_EQUAL( next, 5 ); // Should get C5

    // Test serialization
    std::string serialized = tracker.Serialize();
    BOOST_CHECK( !serialized.empty() );

    // Test deserialization
    REFDES_TRACKER tracker2;
    BOOST_CHECK( tracker2.Deserialize( serialized ) );

    BOOST_CHECK( tracker2.Contains( "R1" ) );
    BOOST_CHECK( tracker2.Contains( "R2" ) );
    BOOST_CHECK( tracker2.Contains( "R3" ) );
    BOOST_CHECK( tracker2.Contains( "C5" ) );

    setupRefDesTracker( tracker2 );

    // Test GetNextRefDesForUnits with deserialized tracker
    next = tracker2.GetNextRefDesForUnits( testRef, emptyMap, requiredUnits, 1 );
    BOOST_CHECK_EQUAL( next, 4 ); // Next reference should be R4

    // Test with unit conflicts after deserialization
    std::map<int, std::vector<SCH_REFERENCE>> conflictMap;
    conflictMap[4] = { createTestReference("R", "2k", 1) }; // R4 different value

    next = tracker2.GetNextRefDesForUnits( testRef, conflictMap, requiredUnits, 1 );
    BOOST_CHECK_EQUAL( next, 5 ); // Should skip R4 due to value conflict
}

BOOST_AUTO_TEST_SUITE_END()