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

#include <sch_reference_list.h>
#include <lib_id.h>
#include <refdes_tracker.h>
#include <sch_symbol.h>

#include <memory>

class TEST_ANNOTATION_UNITS_CONFLICTS : public KI_TEST::SCHEMATIC_TEST_FIXTURE
{
protected:
    /// SCH_REFERENCE holds a raw pointer to its symbol and CompareLibName dereferences it, so the
    /// fixture has to own every symbol it hands out.
    SCH_REFERENCE createTestReference( const wxString& aRef, const wxString& aValue, int aUnit,
                                       const wxString& aLibName = wxS( "TestPart" ) )
    {
        SCH_SYMBOL* symbol = m_symbols.emplace_back( std::make_unique<SCH_SYMBOL>() ).get();
        symbol->SetLibId( LIB_ID( wxEmptyString, aLibName ) );

        SCH_SHEET_PATH path;
        SCH_REFERENCE  ref( symbol, path );
        ref.SetRef( aRef );
        ref.SetValue( aValue );
        ref.SetUnit( aUnit );

        return ref;
    }

private:
    std::vector<std::unique_ptr<SCH_SYMBOL>> m_symbols;
};


BOOST_FIXTURE_TEST_SUITE( UnitConflicts, TEST_ANNOTATION_UNITS_CONFLICTS )



BOOST_AUTO_TEST_CASE( GetNextRefDesForUnits_Integration )
{
    REFDES_TRACKER tracker;
    tracker.SetReuseRefDes( false );

    // Test the overall GetNextRefDesForUnits logic using the tracker

    // Preload some references to simulate previously used ones
    tracker.Insert( "U1" );
    tracker.Insert( "U5" );

    // Test case 1: Completely unused reference with empty units
    // Should get U2 (first unused after U1)
    SCH_REFERENCE testRef = createTestReference( "U", "LM358", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> emptyUnits;

    int nextRef = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( nextRef, 2 ); // Should skip U1, get U2

    // Test case 2: Min value higher than next available
    nextRef = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 10 );
    BOOST_CHECK_EQUAL( nextRef, 10 ); // Should start from min value

    // Verify references were inserted
    BOOST_CHECK( tracker.Contains( "U2" ) );
    BOOST_CHECK( tracker.Contains( "U10" ) );

    // Test case 3: New prefix
    SCH_REFERENCE icRef = createTestReference( "IC", "74HC00", 1 );
    nextRef = tracker.GetNextRefDesForUnits( icRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( nextRef, 1 ); // New prefix should start at 1
    BOOST_CHECK( tracker.Contains( "IC1" ) );
}

BOOST_AUTO_TEST_CASE( RefDesTracker_StateConsistency )
{
    REFDES_TRACKER tracker;
    tracker.SetReuseRefDes( false );

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

    // Test GetNextRefDesForUnits with empty units - should fill gap at R2
    SCH_REFERENCE testRef = createTestReference( "R", "1k", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> emptyUnits;

    int next = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( next, 2 );
    BOOST_CHECK( tracker.Contains( "R2" ) );

    // Get next reference - should fill gap at R4
    next = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( next, 4 );
    BOOST_CHECK( tracker.Contains( "R4" ) );

    // Get next reference - should go to R6
    next = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( next, 6 );
    BOOST_CHECK( tracker.Contains( "R6" ) );

    // Verify total count
    BOOST_CHECK_EQUAL( tracker.Size(), 6 );
}

BOOST_AUTO_TEST_CASE( CacheConsistency_AfterInserts )
{
    REFDES_TRACKER tracker;
    tracker.SetReuseRefDes( false );

    // Test that cache remains consistent after mixed Insert/GetNextRefDesForUnits operations

    // Start with some manual inserts
    tracker.Insert( "C1" );
    tracker.Insert( "C5" );
    tracker.Insert( "C10" );

    SCH_REFERENCE testRef = createTestReference( "C", "100nF", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> emptyUnits;

    // Get next ref - should use cached logic to find C2
    int next = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( next, 2 );

    // Insert C3 manually
    tracker.Insert( "C3" );

    // Get next ref - cache should be updated, should get C4
    next = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( next, 4 );

    // Test with minimum value - should respect cache
    next = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 7 );
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
    tracker.SetReuseRefDes( false );

    // Basic validation that thread-safe operations work
    BOOST_CHECK( tracker.Insert( "U1" ) );
    BOOST_CHECK( tracker.Contains( "U1" ) );

    // Test GetNextRefDesForUnits with thread safety
    SCH_REFERENCE testRef = createTestReference( "U", "LM358", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> emptyUnits;

    int next = tracker.GetNextRefDesForUnits( testRef, emptyMap, emptyUnits, 1 );
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