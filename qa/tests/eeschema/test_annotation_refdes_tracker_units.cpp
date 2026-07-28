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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include "eeschema_test_utils.h"

#include <lib_id.h>
#include <refdes_tracker.h>
#include <sch_reference_list.h>
#include <sch_symbol.h>

#include <memory>

/// One existing reference occupying a reference number.
struct EXISTING_REF
{
    std::string m_value;
    std::string m_libName;
    int         m_unit;
};

struct REFDES_UNITS_TEST_CASE
{
    std::string                                m_caseName;
    std::string                                m_testRefPrefix;
    std::string                                m_testRefValue;
    std::string                                m_testRefLibName;
    std::map<int, std::vector<EXISTING_REF>>   m_refNumberMap;
    std::vector<int>                           m_requiredUnits;
    int                                        m_minValue;
    int                                        m_expectedResult;
    std::vector<std::string>                   m_trackerPreloads;
};

class TEST_REFDES_TRACKER_UNITS : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    void runTestCase( const REFDES_UNITS_TEST_CASE& testCase );

    /// SCH_REFERENCE holds a raw pointer to its symbol and CompareLibName dereferences it, so the
    /// fixture has to own every symbol it hands out.
    SCH_REFERENCE createTestReference( const std::string& aRefPrefix, const std::string& aValue, int aUnit,
                                       const std::string& aLibName = "TestPart" )
    {
        SCH_SYMBOL* symbol = m_symbols.emplace_back( std::make_unique<SCH_SYMBOL>() ).get();
        symbol->SetLibId( LIB_ID( wxEmptyString, aLibName ) );

        SCH_SHEET_PATH path;
        SCH_REFERENCE  ref( symbol, path );
        ref.SetRef( aRefPrefix );
        ref.SetValue( aValue );
        ref.SetUnit( aUnit );

        return ref;
    }

private:
    std::vector<std::unique_ptr<SCH_SYMBOL>> m_symbols;
};

void TEST_REFDES_TRACKER_UNITS::runTestCase( const REFDES_UNITS_TEST_CASE& testCase )
{
    BOOST_TEST_INFO_SCOPE( testCase.m_caseName );

    REFDES_TRACKER tracker;
    tracker.SetReuseRefDes( false );

    for( const std::string& ref : testCase.m_trackerPreloads )
        tracker.Insert( ref );

    SCH_REFERENCE testRef = createTestReference( testCase.m_testRefPrefix, testCase.m_testRefValue, 1,
                                                 testCase.m_testRefLibName );

    std::map<int, std::vector<SCH_REFERENCE>> refNumberMap;

    for( const auto& [refNum, existing] : testCase.m_refNumberMap )
    {
        std::vector<SCH_REFERENCE> refs;

        for( const EXISTING_REF& e : existing )
            refs.push_back( createTestReference( testCase.m_testRefPrefix, e.m_value, e.m_unit, e.m_libName ) );

        refNumberMap[refNum] = refs;
    }

    int result = tracker.GetNextRefDesForUnits( testRef, refNumberMap, testCase.m_requiredUnits,
                                                testCase.m_minValue );

    BOOST_CHECK_EQUAL( result, testCase.m_expectedResult );
    BOOST_CHECK_GE( result, testCase.m_minValue );

    const std::string resultRefDes = testCase.m_testRefPrefix + std::to_string( result );
    auto              selected = testCase.m_refNumberMap.find( result );

    if( selected == testCase.m_refNumberMap.end() )
    {
        // A freshly allocated number is reserved, and must not have been preloaded as used
        BOOST_CHECK( tracker.Contains( resultRefDes ) );
        BOOST_CHECK( std::find( testCase.m_trackerPreloads.begin(), testCase.m_trackerPreloads.end(),
                                resultRefDes ) == testCase.m_trackerPreloads.end() );
    }
    else
    {
        // Sharing an occupied number is only legal when every requested unit is genuinely free on
        // a part with the same library item and value
        for( int unit : testCase.m_requiredUnits )
        {
            if( unit < 0 )
                continue;

            for( const EXISTING_REF& e : selected->second )
            {
                BOOST_CHECK( e.m_unit != unit );
                BOOST_CHECK_EQUAL( e.m_value, testCase.m_testRefValue );
                BOOST_CHECK_EQUAL( e.m_libName, testCase.m_testRefLibName );
            }
        }
    }
}

static const std::vector<REFDES_UNITS_TEST_CASE> refdesUnitsTestCases = {
    {
        "Completely unused reference - empty units",
        "U", "LM358", "OpAmp_Dual",
        {}, // no existing references for any refdes
        {}, // no required units, so any brand new refdes is acceptable
        1,  // min value
        1,  // expected U1
        {}  // no preloaded refdes
    },
    {
        "Completely unused reference - with units",
        "U", "LM358", "OpAmp_Dual",
        {}, // no existing references for any refdes
        { 1, 2 }, // need units 1 and 2
        1,  // min value
        1,  // expected U1
        {}  // no preloaded refdes
    },
    {
        "Skip currently in use reference",
        "U", "LM358", "OpAmp_Dual",
        { { 1, { { "LM358", "OpAmp_Dual", 1 } } } }, // U1 unit 1 in use
        { 1, 2 }, // need units 1 and 2
        1,  // min value
        2,  // expected U2, U1 conflicts on unit 1
        {}
    },
    {
        "Units available in currently used reference",
        "U", "LM358", "OpAmp_Dual",
        { { 1, { { "LM358", "OpAmp_Dual", 3 }, { "LM358", "OpAmp_Dual", 4 } } } }, // U1 units 3,4 in use
        { 1, 2 }, // need units 1 and 2, both free on U1
        1,  // min value
        1,  // expected U1, units 1,2 are free
        {}
    },
    {
        "Different value conflict",
        "U", "LM358", "OpAmp_Dual",
        { { 1, { { "LM741", "OpAmp_Dual", 1 } } } }, // U1 has a different value
        { 1 }, // need unit 1
        1,  // min value
        2,  // expected U2, can't share U1 with a different value
        {}
    },
    {
        "Previously used reference in tracker",
        "U", "LM358", "OpAmp_Dual",
        {}, // no currently used references
        { 1 }, // need unit 1
        1,  // min value
        2,  // expected U2, U1 was previously used
        { "U1" } // U1 preloaded in tracker
    },
    {
        "Min value higher than available",
        "U", "LM358", "OpAmp_Dual",
        { { 5, { { "LM358", "OpAmp_Dual", 1 } } } }, // U5 unit 1 in use
        { 2 },  // need unit 2
        10, // min value
        10, // expected U10, U5 has unit 2 free but min value forces 10
        {}
    },
    {
        "Negative units filtered out",
        "U", "LM358", "OpAmp_Dual",
        {}, // no existing references
        { -1, 1, -5, 2 }, // mix of negative and positive units
        1,  // min value
        1,  // expected U1, only units 1,2 are considered
        {}
    },
    {
        "Complex scenario with gaps",
        "IC", "74HC00", "Logic_Gate",
        { { 2, { { "74HC00", "Logic_Gate", 1 } } },  // IC2 unit 1 used
          { 4, { { "74HC00", "Logic_Gate", 2 } } } }, // IC4 unit 2 used
        { 1, 3 }, // need units 1 and 3
        1,  // min value
        3,  // expected IC3, IC1 was previously used, IC2 conflicts on unit 1
        { "IC1" } // IC1 preloaded in tracker
    },

    // Conflict vectors preserved from the removed ValidateUnitConflictDetection table, now decided
    // by REFDES_TRACKER::areUnitsAvailable instead of a copy of it
    {
        "Units available - no conflicts",
        "U", "LM358", "OpAmp_Dual",
        { { 1, { { "LM358", "OpAmp_Dual", 3 }, { "LM358", "OpAmp_Dual", 4 } } } }, // U1 units 3,4 in use
        { 1, 2 }, // need units 1 and 2
        1,  // min value
        1,  // expected U1, requested units don't conflict with existing units
        {}
    },
    {
        "Units conflict - same unit requested",
        "U", "LM358", "OpAmp_Dual",
        { { 1, { { "LM358", "OpAmp_Dual", 1 }, { "LM358", "OpAmp_Dual", 2 } } } }, // U1 units 1,2 in use
        { 2, 3 }, // need units 2 and 3
        1,  // min value
        2,  // expected U2, unit 2 is already in use on U1
        {}
    },
    {
        "Value mismatch - can't share reference",
        "R", "1k", "Resistor",
        { { 1, { { "2k", "Resistor", 1 } } } }, // R1 has a different value
        { 2 }, // need unit 2
        1,  // min value
        2,  // expected R2, can't share a refdes with a different value
        {}
    },
    {
        "Library mismatch - can't share reference",
        "U", "LM358", "OpAmp_Dual",
        { { 1, { { "LM358", "OpAmp_Single", 1 } } } }, // U1 has a different library part
        { 2 }, // need unit 2
        1,  // min value
        2,  // expected U2, can't share a refdes with a different library part
        {}
    },
    {
        "Empty existing units - should be available",
        "IC", "74HC00", "Logic_Gate",
        {}, // no existing units to conflict with
        { 1, 2, 3, 4 }, // requesting all 4 units
        1,  // min value
        1,  // expected IC1
        {}
    },
    {
        "Negative units filtered out with occupied neighbour",
        "U", "LM324", "OpAmp_Quad",
        { { 1, { { "LM324", "OpAmp_Quad", 2 } } } }, // U1 unit 2 in use
        { -1, 1, -5, 3 }, // only units 1,3 are considered, neither conflicts
        1,  // min value
        1,  // expected U1
        {}
    },
    {
        "All units conflict",
        "U", "LM324", "OpAmp_Quad",
        { { 1, { { "LM324", "OpAmp_Quad", 1 }, { "LM324", "OpAmp_Quad", 2 },
                 { "LM324", "OpAmp_Quad", 3 }, { "LM324", "OpAmp_Quad", 4 } } } }, // U1 all units in use
        { 1, 2, 3, 4 }, // requesting all units
        1,  // min value
        2,  // expected U2, all requested units are already in use on U1
        {}
    },
    {
        "Partial conflict with mixed values",
        "R", "1k", "Resistor",
        { { 1, { { "1k", "Resistor", 1 } } } }, // R1 unit 1 in use, same value
        { 1, 2 }, // need units 1 and 2
        1,  // min value
        2,  // expected R2, unit 1 conflicts even with a matching value
        {}
    },
    {
        "Complex multi-unit scenario",
        "U", "LM339", "Comparator_Quad",
        { { 1, { { "LM339", "Comparator_Quad", 1 }, { "LM339", "Comparator_Quad", 3 } } } }, // U1 units 1,3 in use
        { 2, 4 }, // need units 2 and 4, neither conflicts with existing 1,3
        1,  // min value
        1,  // expected U1
        {}
    }
};

BOOST_FIXTURE_TEST_SUITE( RefDesTrackerUnits, TEST_REFDES_TRACKER_UNITS )

BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_BasicCases )
{
    for( const REFDES_UNITS_TEST_CASE& testCase : refdesUnitsTestCases )
        runTestCase( testCase );
}

BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_EdgeCases )
{
    REFDES_TRACKER tracker;

    // Test empty required units vector - should find completely unused reference
    SCH_REFERENCE testRef = createTestReference( "R", "1k", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> emptyUnits;

    tracker.SetReuseRefDes( false );
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

    tracker.SetReuseRefDes( false );

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

    tracker.SetReuseRefDes( false );
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

    tracker.SetReuseRefDes( false );

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

    tracker2.SetReuseRefDes( false );

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

    tracker.SetReuseRefDes( false );

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

    tracker2.SetReuseRefDes( false );

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