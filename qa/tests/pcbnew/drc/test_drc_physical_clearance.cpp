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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_PHYSICAL_CLEARANCE_TEST_FIXTURE
{
    DRC_PHYSICAL_CLEARANCE_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCPhysicalClearanceNotInflatedByConditionalRules,
                          DRC_PHYSICAL_CLEARANCE_TEST_FIXTURE )
{
    // Regression test for issue 22996.
    //
    // The barcode implicit rule adds a PHYSICAL_CLEARANCE_CONSTRAINT with a condition
    // (A.Type == 'Barcode'). QueryWorstConstraint was not filtering conditional constraints,
    // causing m_DRCMaxPhysicalClearance to be set to 1mm on ALL boards, which forced the
    // physical clearance test to run unnecessarily with O(n^2) memory growth.
    //
    // Verify that boards without physical clearance rules and without barcodes have
    // m_DRCMaxPhysicalClearance == 0 after DRC cache generation.

    KI_TEST::LoadBoard( m_settingsManager, "issue4139", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_COPPER_SLIVER] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_STARVED_THERMAL] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_CHECK_EQUAL( m_board->m_DRCMaxPhysicalClearance, 0 );
}
