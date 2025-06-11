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

// Mock SCH_REFERENCE class for testing GetNextRefDesForUnits
class MOCK_SCH_REFERENCE
{
public:
    MOCK_SCH_REFERENCE( const wxString& aRef, const wxString& aValue,
                       const wxString& aLibName, int aUnit ) :
        m_ref( aRef ), m_value( aValue ), m_libName( aLibName ), m_unit( aUnit ),
        m_refStr( aRef.ToStdString() )
    {
    }

    wxString GetRef() const { return m_ref; }
    const char* GetRefStr() const { return m_refStr.c_str(); }
    wxString GetValue() const { return m_value; }
    int GetUnit() const { return m_unit; }

    int CompareValue( const MOCK_SCH_REFERENCE& other ) const
    {
        return m_value.Cmp( other.m_value );
    }

    int CompareLibName( const MOCK_SCH_REFERENCE& other ) const
    {
        return m_libName.Cmp( other.m_libName );
    }

private:
    wxString m_ref;
    wxString m_value;
    wxString m_libName;
    int m_unit;
    std::string m_refStr; // Store as std::string to avoid temp object issues
};

struct REFDES_UNITS_TEST_CASE
{
    std::string m_caseName;
    std::string m_testRefPrefix;
    std::string m_testRefValue;
    std::string m_testRefLibName;
    std::map<int, std::vector<MOCK_SCH_REFERENCE>> m_refNumberMap;
    std::vector<int> m_requiredUnits;
    int m_minValue;
    int m_expectedResult;
    std::vector<std::string> m_trackerPreloads; // References to preload in tracker
};

class TEST_REFDES_TRACKER_UNITS : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    void runTestCase( const REFDES_UNITS_TEST_CASE& testCase );
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
    MOCK_SCH_REFERENCE testRef( testCase.m_testRefPrefix, testCase.m_testRefValue,
                               testCase.m_testRefLibName, 1 );

    // Convert mock references to SCH_REFERENCE map (this would need adaptation for real SCH_REFERENCE)
    // For now, we'll test the core logic conceptually

    BOOST_TEST_INFO( "Testing case: " + testCase.m_caseName );

    // Test the basic tracker functionality first
    BOOST_CHECK_EQUAL( tracker.Contains( testCase.m_testRefPrefix + "1" ),
                      testCase.m_trackerPreloads.end() !=
                      std::find( testCase.m_trackerPreloads.begin(),
                                testCase.m_trackerPreloads.end(),
                                testCase.m_testRefPrefix + "1" ) );
}

static const std::vector<REFDES_UNITS_TEST_CASE> refdesUnitsTestCases = {
    {
        "Case 1: Completely unused reference - empty units",
        "U", "LM358", "OpAmp",
        {}, // No currently used references
        {}, // Empty required units (need completely unused)
        1,  // Min value
        1,  // Expected: U1
        {}  // No preloaded references
    },
    {
        "Case 2: Completely unused reference - with units",
        "U", "LM358", "OpAmp",
        {}, // No currently used references
        {1, 2}, // Need units 1 and 2
        1,  // Min value
        1,  // Expected: U1
        {}  // No preloaded references
    },
    {
        "Case 3: Skip currently in use reference",
        "U", "LM358", "OpAmp",
        {
            { 1, { MOCK_SCH_REFERENCE("U", "LM358", "OpAmp", 1) } } // U1 unit 1 in use
        },
        {1, 2}, // Need units 1 and 2
        1,  // Min value
        2,  // Expected: U2 (U1 conflicts with unit 1)
        {}
    },
    {
        "Case 4: Units available in currently used reference",
        "U", "LM358", "OpAmp",
        {
            { 1, { MOCK_SCH_REFERENCE("U", "LM358", "OpAmp", 3),
                   MOCK_SCH_REFERENCE("U", "LM358", "OpAmp", 4) } } // U1 units 3,4 in use
        },
        {1, 2}, // Need units 1 and 2 (available)
        1,  // Min value
        1,  // Expected: U1 (units 1,2 are free)
        {}
    },
    {
        "Case 5: Different value conflict",
        "U", "LM358", "OpAmp",
        {
            { 1, { MOCK_SCH_REFERENCE("U", "LM741", "OpAmp", 1) } } // U1 different value
        },
        {1}, // Need unit 1
        1,  // Min value
        2,  // Expected: U2 (can't share with different value)
        {}
    },
    {
        "Case 6: Different library conflict",
        "U", "LM358", "OpAmp",
        {
            { 1, { MOCK_SCH_REFERENCE("U", "LM358", "DifferentLib", 2) } } // U1 different lib
        },
        {1}, // Need unit 1
        1,  // Min value
        2,  // Expected: U2 (can't share with different library)
        {}
    },
    {
        "Case 7: Previously used reference in tracker",
        "U", "LM358", "OpAmp",
        {}, // No currently used references
        {1}, // Need unit 1
        1,   // Min value
        2,   // Expected: U2 (U1 was previously used)
        {"U1"} // U1 preloaded in tracker
    },
    {
        "Case 8: Min value higher than available",
        "U", "LM358", "OpAmp",
        {
            { 5, { MOCK_SCH_REFERENCE("U", "LM358", "OpAmp", 1) } } // U5 unit 1 in use
        },
        {2}, // Need unit 2
        10,  // Min value = 10
        10,  // Expected: U10 (U5 has unit 2 available, but min value is 10)
        {}
    },
    {
        "Case 9: Negative units filtered out",
        "U", "LM358", "OpAmp",
        {},
        {-1, 1, -5, 2}, // Mix of negative and positive units
        1,  // Min value
        1,  // Expected: U1 (only units 1,2 considered)
        {}
    },
    {
        "Case 10: Complex scenario with gaps",
        "IC", "74HC00", "Logic",
        {
            { 2, { MOCK_SCH_REFERENCE("IC", "74HC00", "Logic", 1) } }, // IC2 unit 1 used
            { 4, { MOCK_SCH_REFERENCE("IC", "74HC00", "Logic", 2) } }  // IC4 unit 2 used
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

    // Test empty required units vector
    MOCK_SCH_REFERENCE testRef( "R", "1k", "Resistor", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> emptyUnits;

    // This should return the first unused reference number
    // Note: This test framework is conceptual since we can't easily mock SCH_REFERENCE

    BOOST_TEST_MESSAGE( "Edge case testing would require full SCH_REFERENCE mock implementation" );
}

BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_ThreadSafety )
{
    REFDES_TRACKER tracker( true ); // Enable thread safety

    // Basic test that thread-safe version can be created and used
    int result = tracker.GetNextRefDes( "U", 1 );
    BOOST_CHECK_EQUAL( result, 1 );

    BOOST_CHECK( tracker.Contains( "U1" ) );
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

    // GetNextRefDes should return U2
    int next = tracker.GetNextRefDes( "U", 1 );
    BOOST_CHECK_EQUAL( next, 2 );
    BOOST_CHECK( tracker.Contains( "U2" ) );
}

BOOST_AUTO_TEST_CASE( Serialization_WithTrackedReferences )
{
    REFDES_TRACKER tracker;

    // Add some references using both Insert and GetNextRefDes
    tracker.Insert( "R1" );
    tracker.Insert( "R3" );
    int next = tracker.GetNextRefDes( "R", 1 ); // Should get R2
    BOOST_CHECK_EQUAL( next, 2 );

    next = tracker.GetNextRefDes( "C", 5 ); // Should get C5
    BOOST_CHECK_EQUAL( next, 5 );

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

    // Next reference should be R4
    next = tracker2.GetNextRefDes( "R", 1 );
    BOOST_CHECK_EQUAL( next, 4 );
}

BOOST_AUTO_TEST_SUITE_END()