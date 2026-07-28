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
#include <sch_sheet_path.h>
#include <lib_id.h>
#include <refdes_tracker.h>
#include <sch_symbol.h>

#include <memory>

class TEST_ANNOTATION_UNITS_INTEGRATION : public KI_TEST::SCHEMATIC_TEST_FIXTURE
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


BOOST_FIXTURE_TEST_SUITE( SchReferenceListUnits, TEST_ANNOTATION_UNITS_INTEGRATION )



BOOST_AUTO_TEST_CASE( RefDesTrackerIntegration )
{
    // Test the integration between SCH_REFERENCE_LIST and REFDES_TRACKER
    auto tracker = std::make_shared<REFDES_TRACKER>( false );

    // Preload some references
    tracker->Insert( "U1" );
    tracker->Insert( "U3" );
    tracker->Insert( "R1" );
    tracker->Insert( "R2" );
    tracker->Insert( "R5" );

    // Test GetNextRefDesForUnits respects previously inserted references
    SCH_REFERENCE uRef = createTestReference( "U", "LM358", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> emptyUnits;

    tracker->SetReuseRefDes( false );

    int nextU = tracker->GetNextRefDesForUnits( uRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( nextU, 2 ); // Should skip U1, get U2

    SCH_REFERENCE rRef = createTestReference( "R", "1k", 1 );
    int nextR = tracker->GetNextRefDesForUnits( rRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( nextR, 3 ); // Should skip R1,R2, get R3

    SCH_REFERENCE icRef = createTestReference( "IC", "74HC00", 1 );
    int nextIC = tracker->GetNextRefDesForUnits( icRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( nextIC, 1 ); // New prefix, should get IC1

    // Test with minimum values
    int nextU_min5 = tracker->GetNextRefDesForUnits( uRef, emptyMap, emptyUnits, 5 );
    BOOST_CHECK_EQUAL( nextU_min5, 5 ); // Should get U5 (min value 5)

    // Verify all references were inserted
    BOOST_CHECK( tracker->Contains( "U2" ) );
    BOOST_CHECK( tracker->Contains( "R3" ) );
    BOOST_CHECK( tracker->Contains( "IC1" ) );
    BOOST_CHECK( tracker->Contains( "U5" ) );
}


BOOST_AUTO_TEST_CASE( SerializationWithComplexRefs )
{
    auto tracker = std::make_shared<REFDES_TRACKER>( false );

    // Add references through various methods
    tracker->Insert( "U1" );
    tracker->Insert( "U3" );

    // Use GetNextRefDesForUnits to get U2
    SCH_REFERENCE uRef = createTestReference( "U", "LM358", 1 );
    std::map<int, std::vector<SCH_REFERENCE>> emptyMap;
    std::vector<int> emptyUnits;

    tracker->SetReuseRefDes( false );

    int next = tracker->GetNextRefDesForUnits( uRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( next, 2 ); // Gets U2

    tracker->Insert( "IC1" );
    tracker->Insert( "IC5" );

    SCH_REFERENCE icRef = createTestReference( "IC", "74HC00", 1 );
    next = tracker->GetNextRefDesForUnits( icRef, emptyMap, emptyUnits, 3 );
    BOOST_CHECK_EQUAL( next, 3 ); // Gets IC3

    // Test serialization captures all state
    std::string serialized = tracker->Serialize();

    auto tracker2 = std::make_shared<REFDES_TRACKER>( false );
    BOOST_CHECK( tracker2->Deserialize( serialized ) );

    // Verify all references are preserved
    BOOST_CHECK( tracker2->Contains( "U1" ) );
    BOOST_CHECK( tracker2->Contains( "U2" ) );
    BOOST_CHECK( tracker2->Contains( "U3" ) );
    BOOST_CHECK( tracker2->Contains( "IC1" ) );
    BOOST_CHECK( tracker2->Contains( "IC3" ) );
    BOOST_CHECK( tracker2->Contains( "IC5" ) );

    // Verify next references continue correctly using GetNextRefDesForUnits
    tracker2->SetReuseRefDes( false );
    next = tracker2->GetNextRefDesForUnits( uRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( next, 4 );

    next = tracker2->GetNextRefDesForUnits( icRef, emptyMap, emptyUnits, 1 );
    BOOST_CHECK_EQUAL( next, 2 );
}


BOOST_AUTO_TEST_SUITE_END()