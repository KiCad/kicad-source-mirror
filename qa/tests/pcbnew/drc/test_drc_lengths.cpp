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
#include <pad.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <footprint.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <length_delay_calculation/length_delay_calculation_item.h>
#include <settings/settings_manager.h>


struct DRC_REGRESSION_TEST_FIXTURE
{
    DRC_REGRESSION_TEST_FIXTURE() {}

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCLengths, DRC_REGRESSION_TEST_FIXTURE )
{
    // Check for minimum copper connection errors

    std::vector<std::pair<wxString, int>> tests = {
        { "length_calculations", 0 },
        { "time_calculations", 1 }, // Expect one skew DRC error from NET_P
        { "issue22536", 0 }         // Via electrical span calculation (GitLab #22536)
    };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, test.first, m_board );
        KI_TEST::FillZones( m_board.get() );

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        // Disable DRC tests not useful or not handled in this testcase
        bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[DRCE_COPPER_SLIVER] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[DRCE_STARVED_THERMAL] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[DRCE_DRILL_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[DRCE_VIA_DIAMETER] = SEVERITY::RPT_SEVERITY_IGNORE;
        // These DRC tests are not useful and do not work because they need a footprint library
        // associated to the board
        bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[DRCE_DANGLING_VIA] = SEVERITY::RPT_SEVERITY_IGNORE;

        // Ensure that our desired error is fired
        bds.m_DRCSeverities[DRCE_LENGTH_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_ERROR;

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                     const std::function<void( PCB_MARKER* )>& aPathGenerator )
                {
                    if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                        violations.push_back( *aItem );
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

        if( violations.size() == test.second )
        {
            BOOST_CHECK_EQUAL( 1, 1 ); // quiet "did not check any assertions" warning
            BOOST_TEST_MESSAGE( wxString::Format( "DRC lengths: %s, passed", test.first ) );
        }
        else
        {
            UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

            wxString report;
            std::map<KIID, EDA_ITEM*> itemMap;
            m_board->FillItemMap( itemMap );

            for( const DRC_ITEM& item : violations )
                report += item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap );

            BOOST_ERROR( wxString::Format( "DRC lengths: %s\n"
                                           "%d violations found (expected %d)\n"
                                           "%s",
                                           test.first,
                                           (int) violations.size(),
                                           test.second,
                                           report ) );
        }
    }
}


// Test that via electrical span is calculated from trace connections, not physical via span.
// GitLab issue #22536: TH vias near TH connector pads should use the electrical span
// (layers where traces connect) rather than spanning to layers where TH pads exist.
BOOST_FIXTURE_TEST_CASE( ViaElectricalSpan, DRC_REGRESSION_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue22536", m_board );

    LENGTH_DELAY_CALCULATION lengthCalc( m_board.get() );

    // Find vias on Net 3 (Net-(J1-Pin_3)) which has:
    // - TH vias with physical span F.Cu to B.Cu
    // - Traces on F.Cu and In1.Cu only
    // The electrical span should be F.Cu to In1.Cu, not F.Cu to B.Cu
    NETINFO_ITEM* net3 = m_board->FindNet( "Net-(J1-Pin_3)" );
    BOOST_REQUIRE( net3 != nullptr );

    int viaCount = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        if( track->GetNetCode() != net3->GetNetCode() )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );

        // Verify the via is physically a TH via spanning F.Cu to B.Cu
        BOOST_CHECK_EQUAL( via->TopLayer(), F_Cu );
        BOOST_CHECK_EQUAL( via->BottomLayer(), B_Cu );

        // Calculate the electrical span using CalculateViaLayers
        LENGTH_DELAY_CALCULATION_ITEM item = lengthCalc.GetLengthCalculationItem( via );
        auto [startLayer, endLayer] = item.GetLayers();

        // The electrical span should be F.Cu to In1.Cu (where traces connect),
        // not F.Cu to B.Cu (the physical via span)
        BOOST_CHECK_EQUAL( startLayer, F_Cu );
        BOOST_CHECK_EQUAL( endLayer, In1_Cu );

        viaCount++;
    }

    BOOST_CHECK_EQUAL( viaCount, 2 ); // Net 3 has 2 vias
}
