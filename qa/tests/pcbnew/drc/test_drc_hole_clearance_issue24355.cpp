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
 * @file test_drc_hole_clearance_issue24355.cpp
 *
 * Regression test for issue #24355: DRC silently misses hole-clearance
 * violations when the pointer-order dedup in testPadClearances picks the
 * direction whose iterator pad does not flash the layer being tested.
 *
 * testPadAgainstItem tested only "pad copper vs other hole" and gated it on
 * pad->FlashLayer( aLayer ). The dedup at the caller picks exactly one
 * direction per pad pair. When that direction puts a non-flashing pad
 * (typical NPTH mounting tab where drill == pad size, or any PTH whose
 * unconnected-layer-mode removes the inner annulus) as the iterator, the
 * flash check zeroed the clearance and the test silently passed.
 *
 * Fixture (qa/data/pcbnew/issue24355/) -- single footprint with two pads:
 *
 *   Pad 1: NPTH, round, drill 2.0 mm, size 2.0 mm, at (0, 0). FlashLayer
 *          returns false on both F.Cu and B.Cu because drill_size.x >=
 *          size.x (pad.cpp PAD::FlashLayer).
 *   Pad 2: PTH, round, drill 0.6 mm, size 1.0 mm, at (1.6, 0). FlashLayer
 *          returns true on F.Cu and B.Cu.
 *
 * Edge of NPTH drill to edge of PTH copper = 1.6 - 1.0 - 0.5 = 0.1 mm.
 * Project hole clearance constraint = 0.2 mm -- violated on both copper
 * layers.
 *
 * Pad-allocation order is asserted at fixture creation time so the bug
 * deterministically manifests on common allocators (glibc/msvcrt): the
 * NPTH pad is listed first in the .kicad_pcb so the parser allocates it
 * at a lower address than the PTH, which is the direction the dedup
 * picks and the buggy code short-circuits.
 *
 * Expected: 2 DRCE_HOLE_CLEARANCE violations (one per shared copper layer;
 * per-layer pad violation behavior is intentional in DRC_TEST_PROVIDER_
 * COPPER_CLEARANCE::testPadClearances).
 *
 * Pre-fix: 0 violations (the only direction tested gets short-circuited).
 * Post-fix: 2 violations (the swapped direction runs and finds them).
 */

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


struct DRC_HOLE_CLEARANCE_FIXTURE
{
    DRC_HOLE_CLEARANCE_FIXTURE() = default;

    ~DRC_HOLE_CLEARANCE_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( HoleClearanceIssue24355, DRC_HOLE_CLEARANCE_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24355/issue24355", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24355" );
    BOOST_REQUIRE_EQUAL( m_board->Footprints().size(), 1 );

    FOOTPRINT* fp = m_board->Footprints().front();
    BOOST_REQUIRE_EQUAL( fp->Pads().size(), 2 );

    // The bug is direction-sensitive; the fixture deliberately lists the
    // NPTH pad first so the parser allocates it at the lower heap address.
    // If a future allocator change flips this, the test may pass on buggy
    // code by accident -- flag it so the failure mode is obvious.
    PAD* first = fp->Pads()[0];
    PAD* second = fp->Pads()[1];
    BOOST_REQUIRE_MESSAGE( first->GetAttribute() == PAD_ATTRIB::NPTH,
                           "Fixture pad order changed: expected NPTH first." );
    BOOST_REQUIRE_MESSAGE( second->GetAttribute() == PAD_ATTRIB::PTH,
                           "Fixture pad order changed: expected PTH second." );
    BOOST_WARN_MESSAGE( static_cast<void*>( first ) < static_cast<void*>( second ),
                        "Heap put PTH below NPTH; the bug may not trigger in this run." );

    int                    holeClearanceCount = 0;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_HOLE_CLEARANCE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& /*aPos*/, int /*aLayer*/,
                 const std::function<void( PCB_MARKER* )>& /*aPathGen*/ )
            {
                if( aItem->GetErrorCode() == DRCE_HOLE_CLEARANCE )
                    ++holeClearanceCount;
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Hole clearance violations: %d", holeClearanceCount ) );

    // One violation per shared copper layer (F.Cu, B.Cu) for a 2-layer board.
    BOOST_CHECK_EQUAL( holeClearanceCount, 2 );
}
