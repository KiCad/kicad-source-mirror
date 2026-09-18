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
#include <pcbnew_utils/board_test_utils.h>

#include <map>
#include <board.h>
#include <settings/settings_manager.h>
#include <zone.h>
#include <zone_settings_bag.h>

// Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/25521
//
// The Zone Manager edits clones of the board zones. Resolving a unique name against the board
// originals made every clone collide with itself, so each pass through the dialog appended another
// suffix and "bigger" became "bigger_1", then "bigger_2".

BOOST_AUTO_TEST_SUITE( Issue25521 )


BOOST_AUTO_TEST_CASE( ZoneManagerKeepsZoneNames )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board;

    KI_TEST::LoadBoard( settingsManager, "issue25521_zone_names", board );

    ZONE_SETTINGS_BAG bag( board.get() );

    BOOST_REQUIRE_EQUAL( bag.GetClonedZoneList().size(), 3u );

    std::map<ZONE*, wxString> originalNames;

    for( const auto& [zone, clone] : bag.GetZonesCloneMap() )
        originalNames[clone.get()] = zone->GetZoneName();

    // Every selection change in the manager reads the panel back, resolving the name once per zone
    for( ZONE* clone : bag.GetClonedZoneList() )
    {
        std::shared_ptr<ZONE_SETTINGS> settings = bag.GetZoneSettings( clone );

        settings->m_Name = bag.GetUniqueZoneName( *board, settings->m_Name, clone );
    }

    bag.UpdateClonedZones();

    for( ZONE* clone : bag.GetClonedZoneList() )
        BOOST_CHECK_EQUAL( clone->GetZoneName(), originalNames[clone] );
}


BOOST_AUTO_TEST_SUITE_END()
