/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <schematic_utils/schematic_file_util.h>

#include <advanced_config.h>
#include <erc/erc.h>
#include <erc/erc_exclusion.h>
#include <locale_io.h>
#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <scoped_set_reset.h>
#include <set>


BOOST_AUTO_TEST_CASE( ERCOffGridLinesAndEntriesKeepCapturedEndpoints )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, enabled );

    for( bool backend : { false, true } )
    {
        BOOST_TEST_CONTEXT( "new engine=" << backend )
        {
            enabled = backend;
            SETTINGS_MANAGER settings;
            std::unique_ptr<SCHEMATIC> schematic;
            KI_TEST::LoadSchematic( settings, "unconnected_bus_entry_qa", schematic );
            SCH_SCREEN* screen = schematic->Hierarchy().front().LastScreen();
            const int grid = schematic->Settings().m_ConnectionGridSize;
            BOOST_REQUIRE_GT( grid, 1 );
            const auto onGrid = [grid]( const VECTOR2I& point )
            {
                return point.x % grid == 0 && point.y % grid == 0;
            };
            SCH_LINE* wire = nullptr;
            SCH_BUS_WIRE_ENTRY* entry = nullptr;

            for( SCH_ITEM* item : screen->Items() )
            {
                if( item->Type() == SCH_LINE_T && item->GetLayer() == LAYER_WIRE )
                {
                    auto* candidate = static_cast<SCH_LINE*>( item );

                    if( onGrid( candidate->GetStartPoint() ) && onGrid( candidate->GetEndPoint() ) )
                        wire = candidate;
                }
                else if( item->Type() == SCH_BUS_WIRE_ENTRY_T )
                {
                    auto* candidate = static_cast<SCH_BUS_WIRE_ENTRY*>( item );

                    if( onGrid( candidate->GetPosition() ) && onGrid( candidate->GetEnd() ) )
                        entry = candidate;
                }
            }

            BOOST_REQUIRE( wire );
            BOOST_REQUIRE( entry );
            const VECTOR2I start = wire->GetStartPoint();
            const VECTOR2I end = wire->GetEndPoint();
            const VECTOR2I entryStart = entry->GetPosition();
            const VECTOR2I entryEnd = entry->GetEnd();
            const VECTOR2I offset( 1, 1 );
            using POSITIONS = std::multiset<std::pair<int, int>>;
            const auto position = []( const VECTOR2I& point ) { return std::pair{ point.x, point.y }; };
            ERC_TESTER tester( schematic.get() );
            const auto check = [&]( const POSITIONS& wirePositions, const POSITIONS& entryPositions )
            {
                tester.TestOffGridEndpoints();
                POSITIONS actualWire;
                POSITIONS actualEntry;
                std::vector<SCH_ITEM*> markers;

                for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
                {
                    auto* marker = static_cast<SCH_MARKER*>( item );
                    const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

                    if( error->GetErrorCode() != ERCE_ENDPOINT_OFF_GRID )
                        continue;

                    const KIID& id = error->GetMainItemID();

                    if( id == wire->m_Uuid || id == entry->m_Uuid )
                    {
                        ( id == wire->m_Uuid ? actualWire : actualEntry ).insert( position( marker->GetPosition() ) );
                    }

                    markers.push_back( marker );
                }

                BOOST_CHECK( actualWire == wirePositions );
                BOOST_CHECK( actualEntry == entryPositions );

                for( SCH_ITEM* marker : markers )
                    screen->DeleteItem( marker );
            };

            for( bool startOffGrid : { true, false } )
            {
                wire->SetStartPoint( start + ( startOffGrid ? offset : VECTOR2I() ) );
                wire->SetEndPoint( end + offset );
                entry->SetPosition( entryStart + offset );
                screen->Update( wire, false );
                screen->Update( entry, false );
                schematic->RebuildConnectivity();
                const POSITIONS wirePositions{ position( ( startOffGrid ? start : end ) + offset ) };
                const POSITIONS entryPositions{ position( entryStart + offset ), position( entryEnd + offset ) };
                check( wirePositions, entryPositions );
                wire->SetStartPoint( start );
                wire->SetEndPoint( end );
                entry->SetPosition( entryStart );
                screen->Update( wire, false );
                screen->Update( entry, false );
                schematic->RebuildConnectivity();
                check( {}, {} );
            }
        }
    }
}


BOOST_AUTO_TEST_CASE( ERCOffGridExclusionsSurviveEngineChange )
{
    LOCALE_IO locale;
    auto& enabled = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_ConnectivityEngine;
    SCOPED_SET_RESET restore( enabled, false );
    SETTINGS_MANAGER settings;
    std::unique_ptr<SCHEMATIC> schematic;
    KI_TEST::LoadSchematic( settings, "legacy_hierarchy/legacy_hierarchy", schematic );
    std::vector<SCH_SHEET_PATH> paths;

    for( const SCH_SHEET_PATH& path : schematic->Hierarchy() )
    {
        if( path.LastScreen()->GetFileName().EndsWith( "ampli_ht.kicad_sch" ) )
            paths.push_back( path );
    }

    BOOST_REQUIRE_EQUAL( paths.size(), 2 );
    SCH_SCREEN* screen = paths[0].LastScreen();
    BOOST_REQUIRE( screen == paths[1].LastScreen() );
    SCH_LINE* wire = nullptr;

    for( SCH_ITEM* item : screen->Items().OfType( SCH_LINE_T ) )
    {
        if( item->IsConnectable() )
        {
            wire = static_cast<SCH_LINE*>( item );
            break;
        }
    }

    BOOST_REQUIRE( wire );
    const int grid = schematic->Settings().m_ConnectionGridSize;
    BOOST_REQUIRE_GT( grid, 1 );
    VECTOR2I start = wire->GetStartPoint();
    start.x += start.x % grid == 0 ? 1 : 0;
    wire->SetStartPoint( start );
    screen->Update( wire, false );
    schematic->RebuildConnectivity();
    ERC_TESTER tester( schematic.get() );
    const auto markers = [&]()
    {
        std::vector<SCH_MARKER*> result;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            auto* marker = static_cast<SCH_MARKER*>( item );
            const auto error = std::static_pointer_cast<ERC_ITEM>( marker->GetRCItem() );

            if( error->GetErrorCode() == ERCE_ENDPOINT_OFF_GRID && error->GetMainItemID() == wire->m_Uuid )
                result.push_back( marker );
        }

        return result;
    };
    tester.TestOffGridEndpoints();
    BOOST_REQUIRE_EQUAL( markers().size(), 1 );
    auto exclusion = ERC_EXCLUSION::FromMarker( *markers().front() );
    exclusion.SetComment( "Saved by legacy" );
    schematic->ErcSettings().m_ErcExclusions.insert( exclusion );

    for( SCH_MARKER* marker : markers() )
        screen->DeleteItem( marker );

    enabled = true;
    schematic->RebuildConnectivity();
    tester.TestOffGridEndpoints();
    schematic->ResolveERCExclusionsPostUpdate();
    BOOST_CHECK_EQUAL( markers().size(), 1 );

    for( SCH_MARKER* marker : markers() )
    {
        BOOST_CHECK( marker->IsExcluded() );
        BOOST_CHECK_EQUAL( marker->GetComment(), wxString( "Saved by legacy" ) );
    }
}
