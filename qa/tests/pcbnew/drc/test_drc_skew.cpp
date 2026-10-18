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
#include <qa_utils/file_utils.h>
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
#include <settings/settings_manager.h>

#include <filesystem>
#include <fstream>


namespace
{
struct DRC_REGRESSION_TEST_FIXTURE
{
    DRC_REGRESSION_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};
} // namespace


BOOST_FIXTURE_TEST_CASE( DRCSkew, DRC_REGRESSION_TEST_FIXTURE )
{
    // Check for minimum copper connection errors

    std::vector<std::pair<wxString, int>> tests =
    {
        { "skew_within_diff_pairs_drc", 2 },
        { "skew_group_matched_drc", 5 }
    };

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, test.first, m_board );
        KI_TEST::FillZones( m_board.get() );

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        // Disable DRC tests not useful or not handled in this testcase
        bds.m_DRCSeverities[ DRCE_INVALID_OUTLINE ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_UNCONNECTED_ITEMS ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_COPPER_SLIVER ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_STARVED_THERMAL ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_DRILL_OUT_OF_RANGE ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_VIA_DIAMETER ] = SEVERITY::RPT_SEVERITY_IGNORE;
        // These DRC tests are not useful and do not work because they need a footprint library
        // associated to the board
        bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_ISSUES ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_MISMATCH ] = SEVERITY::RPT_SEVERITY_IGNORE;

        // Ensure that our desired error is fired
        bds.m_DRCSeverities[ DRCE_SKEW_OUT_OF_RANGE ] = SEVERITY::RPT_SEVERITY_ERROR;

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
            BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
            BOOST_TEST_MESSAGE( wxString::Format( "DRC skew: %s, passed", test.first ) );
        }
        else
        {
            UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

            BOARD_RC_ITEM_RESOLVER resolver( *m_board );

            for( const DRC_ITEM& item : violations )
            {
                BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR,
                                                     resolver ) );
            }

            BOOST_ERROR( wxString::Format( "DRC skew: %s, failed (violations found %d expected %d)",
                                            test.first, (int)violations.size(), test.second ) );
        }
    }
}


static size_t runInlineSkewRule( SETTINGS_MANAGER& aSettingsManager, std::unique_ptr<BOARD>& aBoard,
                                 const wxString& aMin, const wxString& aMax, const std::string& aTmpSubdir )
{
    namespace fs = std::filesystem;

    KI_TEST::SCOPED_TEMP_DIR tmpDir( wxString::FromUTF8( aTmpSubdir ) );

    fs::path druPath = tmpDir.Path() / "skew.kicad_dru";

    {
        std::ofstream druOut( druPath );
        druOut << wxString::Format( "(version 1)\n"
                                    "(rule \"skew\"\n"
                                    "    (constraint skew (min %smm) (max %smm) (within_diff_pairs))\n"
                                    "    (condition \"A.inDiffPair('*')\")\n"
                                    ")\n",
                                    aMin, aMax )
                          .ToStdString();
    }

    KI_TEST::LoadBoard( aSettingsManager, "diff_pair_uncoupled_tuning_drc", aBoard );

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    auto       drcEngine = std::make_shared<DRC_ENGINE>( aBoard.get(), &bds );
    wxFileName ruleFile( druPath.string() );
    drcEngine->InitEngine( ruleFile );
    bds.m_DRCEngine = drcEngine;

    bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_DANGLING_VIA] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_DRILL_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_SKEW_OUT_OF_RANGE] = SEVERITY::RPT_SEVERITY_ERROR;

    size_t violations = 0;

    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_SKEW_OUT_OF_RANGE )
                    ++violations;
            } );

    drcEngine->RunTests( EDA_UNITS::MM, true, false );

    std::error_code ec;
    fs::remove( druPath, ec );

    return violations;
}


// Skew is measured against the longest net of the pair, so that net is measured against itself
// and reads exactly zero. The pair is skewed 0.0961 mm, which clears a 0.05 mm minimum.
BOOST_FIXTURE_TEST_CASE( SkewMinimumNotTestedAgainstReferenceNet, DRC_REGRESSION_TEST_FIXTURE )
{
    size_t violations =
            runInlineSkewRule( m_settingsManager, m_board, wxS( "0.05" ), wxS( "0.55" ), "kicad_drc_skew_min_ref" );

    BOOST_CHECK_MESSAGE( violations == 0, "Expected no violation, got " << violations );
}


// The minimum must still be enforced on every other net, otherwise a deliberate non-zero skew
// cannot be given a lower bound at all.
BOOST_FIXTURE_TEST_CASE( SkewMinimumStillCatchesUnderSkewedPair, DRC_REGRESSION_TEST_FIXTURE )
{
    size_t violations =
            runInlineSkewRule( m_settingsManager, m_board, wxS( "0.45" ), wxS( "0.55" ), "kicad_drc_skew_min_under" );

    BOOST_CHECK_MESSAGE( violations == 1, "Expected one violation, got " << violations );
}
