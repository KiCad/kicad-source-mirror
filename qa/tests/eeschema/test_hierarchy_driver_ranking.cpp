/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
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

#include <connection_graph.h>
#include <locale_io.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <settings/settings_manager.h>

#include <map>


struct HIERARCHY_DRIVER_RANKING_FIXTURE
{
    SETTINGS_MANAGER           m_settingsManager;
    std::unique_ptr<SCHEMATIC> m_schematic;
};


// The hierarchy_driver_ranking schematic runs four independent nets through a four-sheet
// hierarchy, each one setting up a different comparison between driver candidates:
//
//   EXT_RST      two global labels of equal rank and equal depth, EXT_RST in LeafB against
//                EXT_RSTIN in LeafA
//   +5V          a global power pin in LeafA against the local label SUPPLY_LOCAL at the root
//   VBUS         a global label in LeafB against the +3V3 power pin in LeafA
//   ZZZ_SHALLOW  two global labels of equal rank at different depths, ZZZ_SHALLOW in Mid
//                against AAA_DEEP one sheet further down
//
// The subgraphs are collected in an unordered set, so a ranking rule that accepts a candidate
// without comparing it against the incumbent resolves these by heap address instead of by rank.
BOOST_FIXTURE_TEST_CASE( HierarchyDriverRanking, HIERARCHY_DRIVER_RANKING_FIXTURE )
{
    LOCALE_IO dummy;

    KI_TEST::LoadSchematic( m_settingsManager, "hierarchy_driver_ranking/hierarchy_driver_ranking",
                            m_schematic );

    std::map<wxString, wxString> pinNets;

    for( const auto& [key, subgraphs] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( !symbol )
                    continue;

                wxString ref = symbol->GetRef( &subgraph->GetSheet() );
                pinNets[ref + wxT( "-" ) + pin->GetNumber()] = key.Name;
            }
        }
    }

    const std::vector<std::pair<wxString, wxString>> expected = {
        { wxT( "R1-1" ),  wxT( "EXT_RST" ) },
        { wxT( "R11-1" ), wxT( "EXT_RST" ) },
        { wxT( "R21-1" ), wxT( "EXT_RST" ) },

        { wxT( "R2-1" ),  wxT( "+5V" ) },
        { wxT( "R12-1" ), wxT( "+5V" ) },

        { wxT( "R3-1" ),  wxT( "VBUS" ) },
        { wxT( "R13-1" ), wxT( "VBUS" ) },
        { wxT( "R22-1" ), wxT( "VBUS" ) },

        { wxT( "R4-1" ),  wxT( "ZZZ_SHALLOW" ) },
        { wxT( "R31-1" ), wxT( "ZZZ_SHALLOW" ) },
        { wxT( "R41-1" ), wxT( "ZZZ_SHALLOW" ) },
    };

    for( const auto& [pin, netName] : expected )
    {
        auto it = pinNets.find( pin );

        BOOST_REQUIRE_MESSAGE( it != pinNets.end(), pin + wxT( " is missing from the netlist" ) );
        BOOST_CHECK_MESSAGE( it->second == netName,
                             pin + wxT( " resolved to " ) + it->second + wxT( " instead of " )
                                     + netName );
    }
}
