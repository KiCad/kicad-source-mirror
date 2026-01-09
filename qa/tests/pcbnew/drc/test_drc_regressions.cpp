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
#include <pad.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <footprint.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_REGRESSION_TEST_FIXTURE
{
    DRC_REGRESSION_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCFalsePositiveRegressions, DRC_REGRESSION_TEST_FIXTURE )
{
    // These documents at one time flagged DRC errors that they shouldn't have.

    std::vector<wxString> tests =
    {
        "issue4139",    // DRC fails wrongly with minimally-spaced pads at 45 degree
        "issue4774",    // Shape collisions missing SH_POLY_SET
        "issue5978",    // Hole clearance violation with non-copper pad
        "issue5990",    // DRC flags a board edge clearance violation although the clearance is respected
        "issue6443",    // Wrong DRC and rendering of THT pads with selective inner copper layers
        "issue7567",    // DRC constraint to disallow holes gets SMD pads also
        "issue7975",    // Differential pair gap out of range fault by DRC
        "issue8407",    // PCBNEW: Arc for diff pair has clearance DRC error
        "issue10906",   // Soldermask bridge for only one object
        "issue11814",   // Bad cache hit in isInsideArea
        "issue12609",   // Arc collison edge case
        "issue14412",   // Solder mask bridge between pads in a net-tie pad group
        "issue15280",   // Very wide spokes mis-counted as being single spoke
        "issue14008",   // Net-tie clearance error
        "issue17967/issue17967",   // Arc dp coupling
        "issue18203",   // DRC error due to colliding arc and circle
        "unconnected-netnames/unconnected-netnames", // Raised false schematic partity error
        "net_tie_drc"   // Net tie bridging soldermask DRC test
    };

    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );
        // Do not refill zones here because this is testing the DRC engine, not the zone filler

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        // Disable DRC tests not useful or not handled in this testcase
        bds.m_DRCSeverities[ DRCE_INVALID_OUTLINE ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_UNCONNECTED_ITEMS ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_COPPER_SLIVER ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_STARVED_THERMAL ] = SEVERITY::RPT_SEVERITY_IGNORE;
        // These DRC tests are not useful and do not work because they need a footprint library
        // associated to the board
        bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_ISSUES ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_MISMATCH ] = SEVERITY::RPT_SEVERITY_IGNORE;

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                     const std::function<void( PCB_MARKER* )>& aPathGenerator )
                {
                    if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                        violations.push_back( *aItem );
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

        if( violations.empty() )
        {
            BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
            BOOST_TEST_MESSAGE( wxString::Format( "DRC regression: %s, passed", relPath ) );
        }
        else
        {
            UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

            wxString report;
            std::map<KIID, EDA_ITEM*> itemMap;
            m_board->FillItemMap( itemMap );

            for( const DRC_ITEM& item : violations )
                report += item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap );

            BOOST_ERROR( wxString::Format( "DRC regression: %s\n"
                                           "%d violations found (expected 0)\n"
                                           "%s",
                                           relPath,
                                           (int) violations.size(),
                                           report ) );
        }
    }
}


BOOST_FIXTURE_TEST_CASE( DRCFalseNegativeRegressions, DRC_REGRESSION_TEST_FIXTURE )
{
    // These documents at one time failed to catch DRC errors that they should have

    std::map<int, SEVERITY> issue19325_ignore, issue22102_ignore;
    issue19325_ignore[DRCE_DRILLED_HOLES_TOO_CLOSE] = SEVERITY::RPT_SEVERITY_IGNORE;
    issue22102_ignore[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
    issue22102_ignore[DRCE_DANGLING_TRACK] = SEVERITY::RPT_SEVERITY_IGNORE;

    std::vector<std::tuple<wxString, int, decltype(BOARD_DESIGN_SETTINGS::m_DRCSeverities)>> tests =
    {
        { "issue1358",  2, {} },
        { "issue2512",  5, {} },
        { "issue2528",  1, {} },
        { "issue5750",  4, {} },   // Shorting zone fills pass DRC in some cases
        { "issue5854",  3, {} },
        { "issue6879",  6, {} },
        { "issue6945",  2, {} },
        { "issue7241",  1, {} },
        { "issue7267",  5, {} },
        { "issue7325",  2, {} },
        { "issue8003",  2, {} },
        { "issue9081",  2, {} },
        { "issue12109", 8, {} },        // Pads fail annular width test
        { "issue14334", 2, {} },        // Thermal spoke to otherwise unconnected island
        { "issue16566", 6, {} },        // Pad_Shape vs Shape property
        { "issue18142", 1, {} },        // blind/buried via to micro-via hole-to-hole
        { "reverse_via", 3, {} },       // Via/track ordering
        { "intersectingzones", 1, {} }, // zones are too close to each other
        { "fill_bad",   1, {} },        // zone max BBox was too small
        { "issue18878", 9, {} },
        { "issue19325/issue19325", 4, issue19325_ignore }, // Overlapping pad annular ring calculation
        { "issue22102", 2, issue22102_ignore },        // arc-to-rect collision; colocated arcs collision
    };

    for( const auto& [testName, expectedErrors, customSeverities] : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, testName, m_board );
        // Do not refill zones here because this is testing the DRC engine, not the zone filler

        std::vector<PCB_MARKER> markers;
        std::vector<DRC_ITEM>   violations;
        BOARD_DESIGN_SETTINGS&  bds = m_board->GetDesignSettings();

        // Disable DRC tests not useful in this testcase
        bds.m_DRCSeverities[DRCE_COPPER_SLIVER] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;

        for(const auto [test, severity] : customSeverities)
            bds.m_DRCSeverities[test] = severity;

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                     const std::function<void( PCB_MARKER* )>& aPathGenerator )
                {
                    markers.emplace_back( PCB_MARKER( aItem, aPos ) );

                    if( bds.m_DrcExclusions.find( markers.back().SerializeToString() )
                        == bds.m_DrcExclusions.end() )
                    {
                        violations.push_back( *aItem );
                    }
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

        if( violations.size() == expectedErrors )
        {
            BOOST_CHECK_EQUAL( 1, 1 ); // quiet "did not check any assertions" warning
            BOOST_TEST_MESSAGE( wxString::Format( "DRC regression: %s, passed", testName ) );
        }
        else
        {
            UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

            wxString report;
            std::map<KIID, EDA_ITEM*> itemMap;
            m_board->FillItemMap( itemMap );

            for( const DRC_ITEM& item : violations )
                report += item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap );

            BOOST_ERROR( wxString::Format( "DRC regression: %s\n"
                                           "%d violations found (expected %d)\n"
                                           "%s",
                                           testName,
                                           (int) violations.size(),
                                           expectedErrors,
                                           report ) );
        }
    }
}


BOOST_FIXTURE_TEST_CASE( DRCZoneFalsePositiveRegressions, DRC_REGRESSION_TEST_FIXTURE )
{
    // These documents at one time flagged DRC errors that they shouldn't have.
    // These tests require zone filling to properly test the DRC checks.

    std::vector<wxString> tests =
    {
        "issue19090/issue19090",  // Copper graphic shapes count as thermal spoke connections
    };

    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );
        KI_TEST::FillZones( m_board.get() );

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        // Disable DRC tests not useful or not handled in this testcase
        bds.m_DRCSeverities[ DRCE_INVALID_OUTLINE ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_UNCONNECTED_ITEMS ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_COPPER_SLIVER ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_ISSUES ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_MISMATCH ] = SEVERITY::RPT_SEVERITY_IGNORE;

        // Ensure starved thermal is enabled for this test
        bds.m_DRCSeverities[ DRCE_STARVED_THERMAL ] = SEVERITY::RPT_SEVERITY_ERROR;

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                     const std::function<void( PCB_MARKER* )>& aPathGenerator )
                {
                    if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                        violations.push_back( *aItem );
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

        if( violations.empty() )
        {
            BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
            BOOST_TEST_MESSAGE( wxString::Format( "DRC zone regression: %s, passed", relPath ) );
        }
        else
        {
            UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

            wxString report;
            std::map<KIID, EDA_ITEM*> itemMap;
            m_board->FillItemMap( itemMap );

            for( const DRC_ITEM& item : violations )
                report += item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap );

            BOOST_ERROR( wxString::Format( "DRC zone regression: %s\n"
                                           "%d violations found (expected 0)\n"
                                           "%s",
                                           relPath,
                                           (int) violations.size(),
                                           report ) );
        }
    }
}
