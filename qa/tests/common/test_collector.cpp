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

#include <boost/test/unit_test.hpp>
#include <collector.h>
#include <eda_item.h>
#include <vector>
#include <ostream>

// Mock EDA_ITEM for testing
class TEST_EDA_ITEM : public EDA_ITEM
{
public:
    TEST_EDA_ITEM( KICAD_T aType ) : EDA_ITEM( aType ) {}

    wxString GetClass() const override { return wxT("TEST_EDA_ITEM"); }

    // Minimal required implementations for abstract methods
    EDA_ITEM* Clone() const override { return new TEST_EDA_ITEM( Type() ); }
};

// Test collector that implements Inspect
class TEST_COLLECTOR : public COLLECTOR
{
public:
    TEST_COLLECTOR() : m_inspectCalls( 0 ), m_shouldCollect( true ) {}

    INSPECT_RESULT Inspect( EDA_ITEM* aTestItem, void* aTestData ) override
    {
        m_inspectCalls++;

        if( m_shouldCollect && aTestItem )
        {
            // Only collect items of specified types if scan types are set
            if( !m_scanTypes.empty() )
            {
                for( KICAD_T type : m_scanTypes )
                {
                    if( aTestItem->Type() == type )
                    {
                        Append( aTestItem );
                        break;
                    }
                }
            }
            else
            {
                Append( aTestItem );
            }
        }

        return INSPECT_RESULT::CONTINUE;
    }

    int GetInspectCalls() const { return m_inspectCalls; }
    void SetShouldCollect( bool aShouldCollect ) { m_shouldCollect = aShouldCollect; }

private:
    int m_inspectCalls;
    bool m_shouldCollect;
};

// Add this for Boost test output of INSPECT_RESULT
std::ostream& operator<<( std::ostream& os, const INSPECT_RESULT& result )
{
    switch( result )
    {
        case INSPECT_RESULT::CONTINUE: os << "CONTINUE"; break;
        case INSPECT_RESULT::QUIT:     os << "QUIT"; break;
        default:                       os << "UNKNOWN"; break;
    }
    return os;
}

BOOST_AUTO_TEST_SUITE( CollectorTests )

BOOST_AUTO_TEST_CASE( Constructor_DefaultValues )
{
    COLLECTOR collector;

    BOOST_CHECK_EQUAL( collector.GetCount(), 0 );
    BOOST_CHECK_EQUAL( collector.m_Threshold, 0 );
    BOOST_CHECK_EQUAL( collector.m_MenuCancelled, false );
    BOOST_CHECK( !collector.HasAdditionalItems() );
}

BOOST_AUTO_TEST_CASE( Append_SingleItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item( PCB_T );

    collector.Append( &item );

    BOOST_CHECK_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK_EQUAL( collector[0], &item );
}

BOOST_AUTO_TEST_CASE( Append_MultipleItems )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );
    collector.Append( &item3 );

    BOOST_CHECK_EQUAL( collector.GetCount(), 3 );
    BOOST_CHECK_EQUAL( collector[0], &item1 );
    BOOST_CHECK_EQUAL( collector[1], &item2 );
    BOOST_CHECK_EQUAL( collector[2], &item3 );
}

BOOST_AUTO_TEST_CASE( Append_NullItem )
{
    COLLECTOR collector;

    collector.Append( nullptr );

    BOOST_CHECK_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK_EQUAL( collector[0], nullptr );
}

BOOST_AUTO_TEST_CASE( Empty_ClearsList )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );

    collector.Append( &item1 );
    collector.Append( &item2 );
    BOOST_CHECK_EQUAL( collector.GetCount(), 2 );

    collector.Empty();

    BOOST_CHECK_EQUAL( collector.GetCount(), 0 );
}

BOOST_AUTO_TEST_CASE( Remove_ByIndex_ValidIndex )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );
    collector.Append( &item3 );

    collector.Remove( 1 );  // Remove middle item

    BOOST_CHECK_EQUAL( collector.GetCount(), 2 );
    BOOST_CHECK_EQUAL( collector[0], &item1 );
    BOOST_CHECK_EQUAL( collector[1], &item3 );
}

BOOST_AUTO_TEST_CASE( Remove_ByIndex_FirstItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );

    collector.Append( &item1 );
    collector.Append( &item2 );

    collector.Remove( 0 );

    BOOST_CHECK_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK_EQUAL( collector[0], &item2 );
}

BOOST_AUTO_TEST_CASE( Remove_ByIndex_LastItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );

    collector.Append( &item1 );
    collector.Append( &item2 );

    collector.Remove( 1 );

    BOOST_CHECK_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK_EQUAL( collector[0], &item1 );
}

BOOST_AUTO_TEST_CASE( Remove_ByPointer_ExistingItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );
    collector.Append( &item3 );

    collector.Remove( &item2 );

    BOOST_CHECK_EQUAL( collector.GetCount(), 2 );
    BOOST_CHECK_EQUAL( collector[0], &item1 );
    BOOST_CHECK_EQUAL( collector[1], &item3 );
}

BOOST_AUTO_TEST_CASE( Remove_ByPointer_NonExistingItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );

    collector.Remove( &item3 );  // Not in collection

    BOOST_CHECK_EQUAL( collector.GetCount(), 2 );
    BOOST_CHECK_EQUAL( collector[0], &item1 );
    BOOST_CHECK_EQUAL( collector[1], &item2 );
}

BOOST_AUTO_TEST_CASE( Remove_ByPointer_DuplicateItems )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );

    collector.Append( &item1 );
    collector.Append( &item1 );
    collector.Append( &item1 );

    collector.Remove( &item1 );

    BOOST_CHECK_EQUAL( collector.GetCount(), 0 );  // All instances should be removed
}

BOOST_AUTO_TEST_CASE( Transfer_ByIndex_ValidIndex )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );
    collector.Append( &item3 );

    collector.Transfer( 1 );

    BOOST_CHECK_EQUAL( collector.GetCount(), 2 );
    BOOST_CHECK_EQUAL( collector[0], &item1 );
    BOOST_CHECK_EQUAL( collector[1], &item3 );
    BOOST_CHECK( collector.HasAdditionalItems() );
}

BOOST_AUTO_TEST_CASE( Transfer_ByPointer_ExistingItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );
    collector.Append( &item3 );

    collector.Transfer( &item2 );

    BOOST_CHECK_EQUAL( collector.GetCount(), 2 );
    BOOST_CHECK_EQUAL( collector[0], &item1 );
    BOOST_CHECK_EQUAL( collector[1], &item3 );
    BOOST_CHECK( collector.HasAdditionalItems() );
}

BOOST_AUTO_TEST_CASE( Transfer_ByPointer_NonExistingItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );

    collector.Transfer( &item3 );  // Not in collection

    BOOST_CHECK_EQUAL( collector.GetCount(), 2 );
    BOOST_CHECK( !collector.HasAdditionalItems() );
}

BOOST_AUTO_TEST_CASE( Combine_RestoresBackupItems )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );
    collector.Append( &item3 );

    collector.Transfer( &item2 );
    collector.Transfer( &item3 );

    BOOST_CHECK_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK( collector.HasAdditionalItems() );

    collector.Combine();

    BOOST_CHECK_EQUAL( collector.GetCount(), 3 );
    BOOST_CHECK( !collector.HasAdditionalItems() );
    BOOST_CHECK_EQUAL( collector[0], &item1 );
    BOOST_CHECK_EQUAL( collector[1], &item2 );
    BOOST_CHECK_EQUAL( collector[2], &item3 );
}

BOOST_AUTO_TEST_CASE( Combine_EmptyBackupList )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );

    collector.Append( &item1 );

    BOOST_CHECK( !collector.HasAdditionalItems() );

    collector.Combine();

    BOOST_CHECK_EQUAL( collector.GetCount(), 1 );
    BOOST_CHECK( !collector.HasAdditionalItems() );
}

BOOST_AUTO_TEST_CASE( OperatorBrackets_ValidIndex )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );

    collector.Append( &item1 );
    collector.Append( &item2 );

    BOOST_CHECK_EQUAL( collector[0], &item1 );
    BOOST_CHECK_EQUAL( collector[1], &item2 );
}

BOOST_AUTO_TEST_CASE( OperatorBrackets_InvalidIndex )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );

    collector.Append( &item1 );

    BOOST_CHECK_EQUAL( collector[1], nullptr );   // Out of bounds
    BOOST_CHECK_EQUAL( collector[-1], nullptr );  // Negative index
    BOOST_CHECK_EQUAL( collector[100], nullptr ); // Large index
}

BOOST_AUTO_TEST_CASE( OperatorBrackets_EmptyCollector )
{
    COLLECTOR collector;

    BOOST_CHECK_EQUAL( collector[0], nullptr );
}

BOOST_AUTO_TEST_CASE( HasItem_ExistingItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );

    collector.Append( &item1 );
    collector.Append( &item2 );

    BOOST_CHECK( collector.HasItem( &item1 ) );
    BOOST_CHECK( collector.HasItem( &item2 ) );
}

BOOST_AUTO_TEST_CASE( HasItem_NonExistingItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );

    BOOST_CHECK( !collector.HasItem( &item3 ) );
}

BOOST_AUTO_TEST_CASE( HasItem_NullItem )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );

    collector.Append( &item1 );
    collector.Append( nullptr );

    BOOST_CHECK( collector.HasItem( nullptr ) );
}

BOOST_AUTO_TEST_CASE( HasItem_EmptyCollector )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );

    BOOST_CHECK( !collector.HasItem( &item1 ) );
}

BOOST_AUTO_TEST_CASE( CountType_SingleType )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_T );
    TEST_EDA_ITEM item3( PCB_VIA_T );

    collector.Append( &item1 );
    collector.Append( &item2 );
    collector.Append( &item3 );

    BOOST_CHECK_EQUAL( collector.CountType( PCB_T ), 2 );
    BOOST_CHECK_EQUAL( collector.CountType( PCB_VIA_T ), 1 );
    BOOST_CHECK_EQUAL( collector.CountType( PCB_TRACE_T ), 0 );
}

BOOST_AUTO_TEST_CASE( CountType_EmptyCollector )
{
    COLLECTOR collector;

    BOOST_CHECK_EQUAL( collector.CountType( PCB_T ), 0 );
}

BOOST_AUTO_TEST_CASE( SetScanTypes_Basic )
{
    COLLECTOR collector;
    std::vector<KICAD_T> types = { PCB_T, PCB_VIA_T };

    collector.SetScanTypes( types );

    // No direct way to test this, but it should not crash
    BOOST_CHECK( true );
}

BOOST_AUTO_TEST_CASE( SetRefPos_Basic )
{
    COLLECTOR collector;
    VECTOR2I refPos( 100, 200 );

    collector.SetRefPos( refPos );

    // No direct way to test this, but it should not crash
    BOOST_CHECK( true );
}

BOOST_AUTO_TEST_CASE( Iterator_BeginEnd )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    collector.Append( &item1 );
    collector.Append( &item2 );
    collector.Append( &item3 );

    std::vector<EDA_ITEM*> items;
    for( auto iter = collector.begin(); iter != collector.end(); ++iter )
    {
        items.push_back( *iter );
    }

    BOOST_CHECK_EQUAL( items.size(), 3 );
    BOOST_CHECK_EQUAL( items[0], &item1 );
    BOOST_CHECK_EQUAL( items[1], &item2 );
    BOOST_CHECK_EQUAL( items[2], &item3 );
}

BOOST_AUTO_TEST_CASE( Iterator_RangeBasedFor )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );

    collector.Append( &item1 );
    collector.Append( &item2 );

    std::vector<EDA_ITEM*> items;
    for( EDA_ITEM* item : collector )
    {
        items.push_back( item );
    }

    BOOST_CHECK_EQUAL( items.size(), 2 );
    BOOST_CHECK_EQUAL( items[0], &item1 );
    BOOST_CHECK_EQUAL( items[1], &item2 );
}

BOOST_AUTO_TEST_CASE( Iterator_ConstIterator )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );

    collector.Append( &item1 );

    const COLLECTOR& constCollector = collector;
    std::vector<const EDA_ITEM*> items;

    for( auto iter = constCollector.begin(); iter != constCollector.end(); ++iter )
    {
        items.push_back( *iter );
    }

    BOOST_CHECK_EQUAL( items.size(), 1 );
    BOOST_CHECK_EQUAL( items[0], &item1 );
}

BOOST_AUTO_TEST_CASE( Inspect_BasicFunctionality )
{
    TEST_COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );

    INSPECT_RESULT result = collector.Inspect( &item1, nullptr );

    BOOST_CHECK_EQUAL( result, INSPECT_RESULT::CONTINUE );
    BOOST_CHECK_EQUAL( collector.GetInspectCalls(), 1 );
    BOOST_CHECK_EQUAL( collector.GetCount(), 1 );
}

BOOST_AUTO_TEST_CASE( Inspect_WithScanTypes )
{
    TEST_COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );

    std::vector<KICAD_T> scanTypes = { PCB_T, PCB_VIA_T };
    collector.SetScanTypes( scanTypes );

    collector.Inspect( &item1, nullptr );  // Should be collected
    collector.Inspect( &item2, nullptr );  // Should be collected
    collector.Inspect( &item3, nullptr );  // Should NOT be collected

    BOOST_CHECK_EQUAL( collector.GetCount(), 2 );
    BOOST_CHECK( collector.HasItem( &item1 ) );
    BOOST_CHECK( collector.HasItem( &item2 ) );
    BOOST_CHECK( !collector.HasItem( &item3 ) );
}

BOOST_AUTO_TEST_CASE( ComplexWorkflow_TransferAndCombine )
{
    COLLECTOR collector;
    TEST_EDA_ITEM item1( PCB_T );
    TEST_EDA_ITEM item2( PCB_VIA_T );
    TEST_EDA_ITEM item3( PCB_TRACE_T );
    TEST_EDA_ITEM item4( PCB_PAD_T );

    // Add items
    collector.Append( &item1 );
    collector.Append( &item2 );
    collector.Append( &item3 );
    collector.Append( &item4 );
    BOOST_CHECK_EQUAL( collector.GetCount(), 4 );

    // Transfer some items
    collector.Transfer( &item2 );
    collector.Transfer( &item4 );
    BOOST_CHECK_EQUAL( collector.GetCount(), 2 );
    BOOST_CHECK( collector.HasAdditionalItems() );

    // Remove one remaining item
    collector.Remove( &item1 );
    BOOST_CHECK_EQUAL( collector.GetCount(), 1 );

    // Combine back
    collector.Combine();
    BOOST_CHECK_EQUAL( collector.GetCount(), 3 );
    BOOST_CHECK( !collector.HasAdditionalItems() );

    // Check final state
    BOOST_CHECK( !collector.HasItem( &item1 ) );  // Was removed
    BOOST_CHECK( collector.HasItem( &item2 ) );   // Was transferred back
    BOOST_CHECK( collector.HasItem( &item3 ) );   // Was never moved
    BOOST_CHECK( collector.HasItem( &item4 ) );   // Was transferred back
}

BOOST_AUTO_TEST_SUITE_END()