/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include "../../../pcbnew/diff_merge/applier_helpers.h"

#include <board.h>
#include <board_item.h>
#include <footprint.h>
#include <pad.h>
#include <settings/settings_manager.h>


using namespace KICAD_DIFF;


struct APPLIER_HELPERS_FIXTURE
{
    APPLIER_HELPERS_FIXTURE()
    {
        KI_TEST::LoadBoard( m_settings, "complex_hierarchy", m_board );
        BOOST_REQUIRE( m_board );
    }

    SETTINGS_MANAGER       m_settings;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_SUITE( ApplierHelpers, APPLIER_HELPERS_FIXTURE )


// Null board -> no inserts, no crash.
BOOST_AUTO_TEST_CASE( CollectTopLevelIdsNullBoardIsNoop )
{
    std::set<KIID> out;
    CollectTopLevelIds( nullptr, out );
    BOOST_CHECK( out.empty() );
}


// Every top-level item's UUID must end up in the output set, while footprint
// children (pads etc.) must NOT -- that "top-level only" boundary is the
// helper's whole reason to exist. The const pointer also pins that the helper
// is callable on a const BOARD without a const_cast in the caller.
BOOST_AUTO_TEST_CASE( CollectTopLevelIdsIncludesItemsButNotChildren )
{
    const BOARD*   constBoard = m_board.get();
    std::set<KIID> out;
    CollectTopLevelIds( constBoard, out );

    BOOST_REQUIRE( !out.empty() );

    for( const BOARD_ITEM* item : m_board->GetItemSet() )
    {
        if( item )
            BOOST_CHECK( out.count( item->m_Uuid ) == 1 );
    }

    // A footprint's pads are children, not top-level items, so their UUIDs
    // must be absent from the collected set.
    bool checkedAChild = false;

    for( const FOOTPRINT* fp : m_board->Footprints() )
    {
        for( const PAD* pad : fp->Pads() )
        {
            BOOST_CHECK( out.count( pad->m_Uuid ) == 0 );
            checkedAChild = true;
        }
    }

    BOOST_REQUIRE( checkedAChild );   // fixture must actually contain a footprint pad
}


// Output is additive: calling twice doesn't drop entries from the first call.
BOOST_AUTO_TEST_CASE( CollectTopLevelIdsIsAdditive )
{
    std::set<KIID> out;
    out.insert( KIID() );

    const size_t initial = out.size();
    CollectTopLevelIds( m_board.get(), out );

    BOOST_CHECK_GT( out.size(), initial );
}


BOOST_AUTO_TEST_SUITE_END()
