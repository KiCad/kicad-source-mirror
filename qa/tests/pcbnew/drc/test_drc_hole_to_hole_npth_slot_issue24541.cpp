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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_drc_hole_to_hole_npth_slot_issue24541.cpp
 *
 * Regression test for issue #24541: DRC silently ignores NPTH (oval) slots in
 * the hole-to-hole clearance test, so two overlapping NPTH slots produce no
 * violation even though the project sets hole-to-hole = error.
 *
 * DRC_TEST_PROVIDER_HOLE_TO_HOLE only inserts a pad into its hole r-tree when
 * the drill is round (drill.x == drill.y) and only queries pads for which
 * PAD::HasDrilledHole() is true. Oval slots fail both gates, so a slot is
 * never tested against any other hole. The provider's stated rationale is
 * "slots are milled after drilling", but that does not exempt a slot from the
 * mutual hole-to-hole clearance against other slots / drilled holes.
 *
 * Fixture (qa/data/pcbnew/issue24541) is the reporter's project: two
 * footprints, each carrying a 10 mm x 1 mm NPTH oval slot, placed so the
 * slots overlap. min_hole_to_hole = 0.25 mm, hole_to_hole severity = error.
 *
 * Expected (correct) behavior: at least one DRCE_DRILLED_HOLES_TOO_CLOSE
 * violation between the two overlapping slots.
 *
 * Pre-fix: 0 violations (slots skipped entirely).
 *
 * This test documents the reproduction; it is expected to FAIL on the buggy
 * code and PASS once slots participate in the hole-to-hole check.
 */

#include <algorithm>
#include <set>
#include <utility>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_HOLE_TO_HOLE_NPTH_SLOT_FIXTURE
{
    DRC_HOLE_TO_HOLE_NPTH_SLOT_FIXTURE() = default;

    ~DRC_HOLE_TO_HOLE_NPTH_SLOT_FIXTURE()
    {
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();

        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( HoleToHoleNpthSlotIssue24541, DRC_HOLE_TO_HOLE_NPTH_SLOT_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24541/issue24541", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24541" );
    BOOST_REQUIRE_EQUAL( m_board->Footprints().size(), 2 );

    // Confirm the fixture still carries two NPTH oval slots whose drill is a
    // non-round slot (drill.x != drill.y); these are the holes the provider
    // currently ignores.
    int npthSlotCount = 0;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH
                    && pad->GetDrillSize().x != pad->GetDrillSize().y )
            {
                ++npthSlotCount;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( npthSlotCount == 2,
                           "Fixture changed: expected two NPTH oval slots." );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_DRILLED_HOLES_TOO_CLOSE] = SEVERITY::RPT_SEVERITY_ERROR;
    bds.m_DRCSeverities[DRCE_DRILLED_HOLES_COLOCATED] = SEVERITY::RPT_SEVERITY_ERROR;

    // Count only violations between the two NPTH oval slots, deduplicated by the pair of items
    // involved.  A bare >= 1 check could be satisfied by an unrelated hole pair in the fixture or
    // by the same pair being reported twice, so we assert exactly one distinct slot-to-slot pair.
    int                             holeToHoleCount = 0;
    std::set<std::pair<KIID, KIID>> npthSlotPairs;

    auto isNpthSlot =
            []( const BOARD_ITEM* aItem ) -> bool
            {
                const PAD* pad = dynamic_cast<const PAD*>( aItem );

                return pad && pad->GetAttribute() == PAD_ATTRIB::NPTH
                        && pad->GetDrillSize().x != pad->GetDrillSize().y;
            };

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& /*aPos*/, int /*aLayer*/,
                 const std::function<void( PCB_MARKER* )>& /*aPathGen*/ )
            {
                int code = aItem->GetErrorCode();

                if( code != DRCE_DRILLED_HOLES_TOO_CLOSE && code != DRCE_DRILLED_HOLES_COLOCATED )
                    return;

                ++holeToHoleCount;

                BOARD_ITEM* itemA = m_board->ResolveItem( aItem->GetMainItemID(), true );
                BOARD_ITEM* itemB = m_board->ResolveItem( aItem->GetAuxItemID(), true );

                if( isNpthSlot( itemA ) && isNpthSlot( itemB ) )
                    npthSlotPairs.emplace( std::minmax( itemA->m_Uuid, itemB->m_Uuid ) );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Hole-to-hole violations: %d; NPTH slot pairs: %d",
                                          holeToHoleCount, (int) npthSlotPairs.size() ) );

    // The two overlapping NPTH slots must produce exactly one distinct hole-to-hole violation.
    BOOST_CHECK_EQUAL( npthSlotPairs.size(), 1 );
}
