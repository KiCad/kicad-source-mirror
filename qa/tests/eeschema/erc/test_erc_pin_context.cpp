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
#include <schematic_utils/schematic_file_util.h>
#include <erc/erc_sch_pin_context.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_symbol.h>
#include <settings/settings_manager.h>
#include <locale_io.h>
#include <algorithm>
#include <map>
#include <set>

BOOST_AUTO_TEST_CASE( ERCPinContextOrderUsesInstanceAndPinIdentity )
{
    LOCALE_IO locale;
    SETTINGS_MANAGER firstSettings;
    SETTINGS_MANAGER secondSettings;
    std::unique_ptr<SCHEMATIC> first;
    std::unique_ptr<SCHEMATIC> second;
    KI_TEST::LoadSchematic( firstSettings, "issue23840/BusAndVectors", first );
    KI_TEST::LoadSchematic( secondSettings, "issue23840/BusAndVectors", second );
    using KEY = std::pair<KIID_PATH, KIID>;
    const auto contexts = []( SCHEMATIC& schematic )
    {
        std::map<KEY, ERC_SCH_PIN_CONTEXT> result;

        for( const SCH_SHEET_PATH& path : schematic.Hierarchy() )
        {
            for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
            {
                for( SCH_PIN* pin : static_cast<SCH_SYMBOL*>( item )->GetPins( &path ) )
                {
                    const bool inserted = result.emplace( KEY{ path.Path(), pin->m_Uuid },
                                                          ERC_SCH_PIN_CONTEXT( pin, path ) ).second;
                    BOOST_REQUIRE( inserted );
                }
            }
        }

        return result;
    };
    const auto originals = contexts( *first );
    const auto reloaded = contexts( *second );
    BOOST_REQUIRE_GT( originals.size(), 1 );
    BOOST_REQUIRE_EQUAL( originals.size(), reloaded.size() );
    const ERC_SCH_PIN_CONTEXT empty;
    const ERC_SCH_PIN_CONTEXT missing( nullptr, originals.begin()->second.Sheet() );
    BOOST_CHECK( empty == ERC_SCH_PIN_CONTEXT() );
    BOOST_CHECK( !( empty < empty ) );
    BOOST_CHECK( empty < missing );
    BOOST_CHECK( !( missing < empty ) );
    BOOST_CHECK( !( empty == missing ) );
    BOOST_CHECK( missing < originals.begin()->second );
    BOOST_CHECK( !( originals.begin()->second < missing ) );
    BOOST_CHECK( !( missing == originals.begin()->second ) );
    std::map<SCH_PIN*, std::set<KIID_PATH>> instances;
    std::vector<ERC_SCH_PIN_CONTEXT> shuffled;

    for( const auto& [key, context] : originals )
    {
        const auto other = reloaded.find( key );
        BOOST_REQUIRE( other != reloaded.end() );
        BOOST_CHECK( context.Pin() != other->second.Pin() );
        BOOST_CHECK( context == other->second );
        BOOST_CHECK( !( context < other->second ) );
        BOOST_CHECK( !( other->second < context ) );
        instances[context.Pin()].insert( key.first );
        shuffled.push_back( context );
    }

    BOOST_REQUIRE( std::any_of( instances.begin(), instances.end(),
                               []( const auto& entry ) { return entry.second.size() > 1; } ) );
    std::reverse( shuffled.begin(), shuffled.end() );
    std::sort( shuffled.begin(), shuffled.end() );
    auto expected = originals.begin();

    for( const ERC_SCH_PIN_CONTEXT& context : shuffled )
    {
        BOOST_CHECK( context.Sheet().PathRef() == expected->first.first );
        BOOST_CHECK( context.Pin()->m_Uuid == expected->first.second );
        ++expected;
    }
}
