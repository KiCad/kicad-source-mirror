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

#include <boost/test/unit_test.hpp>

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <board.h>
#include <downgrade/board_downgrade.h>
#include <settings/settings_manager.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/downgrade_oracle_utils.h>
#include <qa_utils/downgrade_golden_utils.h>


BOOST_AUTO_TEST_SUITE( DowngradeOracle )

static const DOWNGRADE_TARGET& kicad9 = KI_TEST::RequireDowngradeTarget( wxT( "9.0" ) );
static const DOWNGRADE_TARGET& kicad10 = KI_TEST::RequireDowngradeTarget( wxT( "10.0" ) );


static void checkNativeBoardGolden( const DOWNGRADE_TARGET& aTarget, const std::string& aRelease,
                                    const std::string& aBoard, const std::string& aReference, size_t aExpectedDrops,
                                    size_t aExpectedLowerings, bool aDropInsteadOfApproximate = false )
{
    SETTINGS_MANAGER             settingsManager;
    const std::string            fixture = KI_TEST::GetPcbnewTestDataDir() + "../downgrade/golden/";
    KI_TEST::TEMPORARY_DIRECTORY tmp( "kicad_qa_native_golden", "" );
    const std::string            dest = ( tmp.GetPath() / ( aBoard + ".kicad_pcb" ) ).string();
    COMPATIBILITY_REPORT         report;

    // Expected files were saved by native KiCad, never by the exporter under test.
    BOOST_REQUIRE( ExportBoardToOlderVersion( fixture + "current/" + aBoard + ".kicad_pcb", dest, aTarget, report,
                                              nullptr, wxEmptyString, aDropInsteadOfApproximate ) );
    BOOST_CHECK( !report.IsBlocked() );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::DROP ), aExpectedDrops );
    BOOST_CHECK_EQUAL( report.Count( DOWNGRADE_BUCKET::LOWER ), aExpectedLowerings );
    const std::string difference =
            KI_TEST::GoldenFileDifference( fixture + aRelease + "/" + aReference + ".kicad_pcb", dest );
    BOOST_CHECK_MESSAGE( difference.empty(), difference );
}


BOOST_AUTO_TEST_CASE( BoardMatchesNativeKicad9Golden )
{
    // KiCad 9 drops the explicit disabled via-protection overrides without changing the via.
    checkNativeBoardGolden( kicad9, "v9", "geometry", "geometry", 1, 0 );
}


BOOST_AUTO_TEST_CASE( BoardMatchesNativeKicad10Golden )
{
    checkNativeBoardGolden( kicad10, "v10", "geometry", "geometry", 0, 0 );
}


BOOST_AUTO_TEST_CASE( LoweredGraphicsMatchNativeKicad9Golden )
{
    checkNativeBoardGolden( kicad9, "v9", "graphics", "graphics", 0, 2 );
}


BOOST_AUTO_TEST_CASE( GraphicsMatchNativeKicad10Golden )
{
    checkNativeBoardGolden( kicad10, "v10", "graphics", "graphics", 0, 0 );
}


BOOST_AUTO_TEST_CASE( DroppedGraphicsMatchNativeKicad9Golden )
{
    checkNativeBoardGolden( kicad9, "v9", "graphics", "graphics-drop", 2, 0, true );
}


BOOST_AUTO_TEST_CASE( DropApproximationsKeepsNativeKicad10GraphicsGolden )
{
    checkNativeBoardGolden( kicad10, "v10", "graphics", "graphics", 0, 0, true );
}


BOOST_AUTO_TEST_CASE( RequiredReleaseLookupFailsForMissingId )
{
    BOOST_CHECK( KI_TEST::RequireDowngradeTarget( wxT( "9.0" ) ).m_id == wxT( "9.0" ) );
    BOOST_CHECK( KI_TEST::RequireDowngradeTarget( wxT( "10.0" ) ).m_id == wxT( "10.0" ) );
    BOOST_CHECK_THROW( KI_TEST::RequireDowngradeTarget( wxT( "missing-release" ) ), std::runtime_error );
}


// Guard the oracle itself: normalization must not hide meaningful design changes.
BOOST_AUTO_TEST_CASE( GoldenComparisonDetectsDesignAndSettingsChanges )
{
    SEXPR::PARSER     parser;
    const std::string golden = "(kicad_pcb (version 20241229) (generator_version \"9.0\") "
                               "(setup (pad_to_mask_clearance 0.05)) "
                               "(pad \"1\" (at 100 100 30) (size 2 1) (net 1 \"SIGNAL\")))";
    auto              expected = parser.Parse( golden );

    for( const auto& [from, to] : std::vector<std::pair<std::string, std::string>>{ { "100 100", "0 0" },
                                                                                    { "(size 2 1)", "(size 1 2)" },
                                                                                    { "SIGNAL", "GND" },
                                                                                    { "0.05", "0.1" },
                                                                                    { "100 100 30", "100 100 31" },
                                                                                    { "20241229", "20260206" } } )
    {
        std::string changed = golden;
        changed.replace( changed.find( from ), from.size(), to );
        auto actual = parser.Parse( changed );
        BOOST_CHECK( !KI_TEST::GoldenSexprDifference( *expected, *actual ).empty() );
    }

    auto metadataOnly = parser.Parse( "(kicad_pcb (version 20241229) (generator_version \"10.99\") "
                                      "(setup (pad_to_mask_clearance 0.05)) "
                                      "(pad \"1\" (at 100.000001 99.999999 30) (size 2 1) (net 1 \"SIGNAL\")))" );
    BOOST_CHECK( KI_TEST::GoldenSexprDifference( *expected, *metadataOnly ).empty() );

    const std::string plotGolden = "(kicad_pcb (setup (pcbplotparams (hpglpennumber 1) (hpglpenspeed 20) "
                                   "(hpglpendiameter 15) (plotinvisibletext no))))";
    auto              plotExpected = parser.Parse( plotGolden );
    auto modernPlot = parser.Parse( "(kicad_pcb (setup (pcbplotparams (pngdpi 300) (pngantialias yes))))" );
    BOOST_CHECK( KI_TEST::GoldenSexprDifference( *plotExpected, *modernPlot ).empty() );

    for( const auto& [from, to] :
         std::vector<std::pair<std::string, std::string>>{ { "hpglpennumber 1", "hpglpennumber 2" },
                                                           { "hpglpenspeed 20", "hpglpenspeed 30" },
                                                           { "hpglpendiameter 15", "hpglpendiameter 16" },
                                                           { "plotinvisibletext no", "plotinvisibletext yes" } } )
    {
        std::string changed = plotGolden;
        changed.replace( changed.find( from ), from.size(), to );
        auto nonDefault = parser.Parse( changed );
        BOOST_CHECK( !KI_TEST::GoldenSexprDifference( *nonDefault, *modernPlot ).empty() );
    }
}


BOOST_AUTO_TEST_SUITE_END()
