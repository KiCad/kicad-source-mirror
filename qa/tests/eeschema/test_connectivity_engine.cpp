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
#include <connectivity/conn_engine.h>
#include <connectivity/conn_facade.h>
#include <connectivity/conn_dump.h>
#include <schematic_utils/schematic_file_util.h>
#include <schematic.h>
#include <sch_label.h>
#include <sch_screen.h>
#include <sch_connection.h>
#include <bus_alias.h>
#include <connection_graph.h>
#include <sch_no_connect.h>
#include <sch_symbol.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <settings/settings_manager.h>
#include <thread_pool.h>
#include <algorithm>

using namespace SCH_CONNECTIVITY;

BOOST_AUTO_TEST_SUITE( ConnectivityEngine )

BOOST_AUTO_TEST_CASE( NativePublicationIsStableAcrossWorkerCounts )
{
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "issue9673/issue9673", schematic );
    auto& pool = GetKiCadThreadPool();
    struct RESTORE_POOL
    {
        thread_pool& pool;
        size_t       count;
        ~RESTORE_POOL() { pool.reset( count ); }
    } restore{ pool, pool.get_thread_count() };
    FACADE facade;
    std::string expected;
    std::string expectedChanged;
    std::shared_ptr<BUS_ALIAS> alias;

    for( const auto& candidate : schematic->GetAllBusAliases() )
    {
        if( candidate && candidate->GetName() == wxS( "MIXED_BUS" ) )
            alias = candidate;
    }

    BOOST_REQUIRE( alias );
    const auto members = alias->Members();

    for( size_t count : { 1, 2, 8 } )
    {
        pool.reset( count );
        facade.Update( *schematic, true );
        const auto actual = Dump( *schematic, facade );

        if( expected.empty() )
            expected = actual;

        BOOST_CHECK_EQUAL( actual, expected );
        facade.Update( *schematic );
        BOOST_CHECK( facade.Published().Changes().Empty() );
        BOOST_CHECK_EQUAL( Dump( *schematic, facade ), expected );

        alias->AddMember( wxS( "HAM" ) );
        facade.Update( *schematic );
        const auto changed = Dump( *schematic, facade );
        BOOST_CHECK_NE( changed, expected );

        if( expectedChanged.empty() )
            expectedChanged = changed;

        BOOST_CHECK_EQUAL( changed, expectedChanged );
        facade.Update( *schematic, true );
        BOOST_CHECK_EQUAL( Dump( *schematic, facade ), changed );
        alias->SetMembers( members );
    }
}

BOOST_AUTO_TEST_CASE( NativeNoConnectNamingRetainsExcludedPinWitnesses )
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "NoConnectPinsConnectedByLine", schematic );
    const auto      path = schematic->Hierarchy().front();
    SCH_NO_CONNECT* marker = nullptr;
    SCH_SYMBOL*     excluded = nullptr;
    SCH_PIN*        pin = nullptr;
    SCH_LINE*       bridge = nullptr;

    for( SCH_ITEM* item : path.LastScreen()->Items() )
    {
        if( auto* candidate = dynamic_cast<SCH_NO_CONNECT*>( item ) )
            marker = candidate;

        if( auto* line = dynamic_cast<SCH_LINE*>( item ); line && line->GetStartPoint().y == line->GetEndPoint().y )
            bridge = line;

        if( auto* symbol = dynamic_cast<SCH_SYMBOL*>( item ) )
        {
            if( symbol->GetRef( &path ) == "TP1" )
                pin = symbol->GetPins( &path ).front();
            else
                excluded = symbol;
        }
    }

    BOOST_REQUIRE( marker );
    BOOST_REQUIRE( excluded );
    BOOST_REQUIRE( pin );
    BOOST_REQUIRE( bridge );
    ENGINE     engine;
    const auto check = [&]( bool aUnconnected )
    {
        schematic->ConnectionGraph()->Recalculate( schematic->Hierarchy(), true );
        BOOST_REQUIRE( pin->Connection( &path ) );
        const wxString expected = pin->Connection( &path )->Name();
        BOOST_CHECK_EQUAL( expected.StartsWith( "unconnected-(" ), aUnconnected );
        engine.Update( schematic->Hierarchy(), 1, {} );
        size_t checked = 0;

        for( const auto& entry : engine.Signals().Entries() )
        {
            const auto& signal = entry->value;

            if( std::none_of( signal.items.begin(), signal.items.end(),
                              [&]( const ITEM_KEY& item )
                              {
                                  return item.item == pin->m_Uuid;
                              } ) )
                continue;

            BOOST_REQUIRE_NE( signal.baseName, INVALID_ID );
            BOOST_CHECK_EQUAL( engine.Keys().Name( signal.baseName ), expected );
            ++checked;
        }

        BOOST_CHECK_EQUAL( checked, 1 );
    };
    check( true );
    const VECTOR2I original = marker->GetPosition();
    marker->SetPosition( original + VECTOR2I( 10000000, 10000000 ) );
    marker->SetConnectivityDirty( true );
    check( false );
    excluded->SetExcludedFromBoard( true );
    excluded->SetConnectivityDirty( true );
    check( false );
    excluded->SetExcludedFromBoard( false );
    excluded->SetConnectivityDirty( true );
    check( false );
    marker->SetPosition( original );
    marker->SetConnectivityDirty( true );
    check( true );
    marker->SetPosition( original + VECTOR2I( 10000000, 10000000 ) );
    marker->SetConnectivityDirty( true );
    check( false );
}

BOOST_AUTO_TEST_CASE( JoinedLabelsUseTheirIslandLocalName )
{
    SETTINGS_MANAGER           settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "netlists/multinetclasses/multinetclasses", schematic );
    const auto      path = schematic->Hierarchy().front();
    SCH_LABEL_BASE* first = nullptr;
    SCH_LABEL_BASE* second = nullptr;

    for( SCH_ITEM* item : path.LastScreen()->Items() )
    {
        auto* label = dynamic_cast<SCH_LABEL_BASE*>( item );

        if( label && label->GetText() == "NET_1" )
            first = label;
        else if( label && label->GetText() == "NET_2" )
            second = label;
    }

    BOOST_REQUIRE( first );
    BOOST_REQUIRE( second );
    second->SetPosition( first->GetPosition() );
    second->SetConnectivityDirty( true );
    schematic->ConnectionGraph()->Recalculate( schematic->Hierarchy(), true );
    ENGINE engine;
    engine.Update( schematic->Hierarchy(), 1, {} );
    size_t checked = 0;

    for( const auto& [key, row] : engine.Published().Rows() )
    {
        SCH_LABEL_BASE* label = key.item == first->m_Uuid ? first : key.item == second->m_Uuid ? second : nullptr;

        if( !label )
            continue;

        const auto* connection = label->Connection( &path );
        BOOST_REQUIRE( connection );
        BOOST_CHECK_EQUAL( engine.Keys().Name( row.localName ), first->GetText() );
        BOOST_CHECK_EQUAL( engine.Keys().Name( row.localName ), connection->LocalName() );
        BOOST_CHECK_EQUAL( engine.Keys().Name( row.fullLocalName ), connection->FullLocalName() );
        BOOST_CHECK_EQUAL( engine.Keys().Name( row.name ), connection->Name() );
        BOOST_CHECK( row.itemType == connection->Type() );
        ++checked;
    }

    BOOST_CHECK_EQUAL( checked, 2 );
}

BOOST_AUTO_TEST_SUITE_END()
