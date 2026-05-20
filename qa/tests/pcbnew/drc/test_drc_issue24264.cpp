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
#include <drc/drc_report.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>

#include <fstream>
#include <sstream>

#include <json_common.h>


struct ISSUE24264_FIXTURE
{
    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


// https://gitlab.com/kicad/code/kicad/-/issues/24264
//
// A custom rule that pins a constraint to "(severity exclusion)" should produce a marker that
// appears in the JSON DRC report as excluded.  Before the fix, the JSON output reported these
// markers as plain errors with "excluded": false, while the interactive DRC dialog correctly
// treated them as exclusions.
BOOST_FIXTURE_TEST_CASE( DRCRuleSeverityExclusionMarkedExcluded, ISSUE24264_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24264/issue24264", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Silence unrelated checks that can fire on the imported reproduction board
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES]   = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_FOOTPRINT_TYPE_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS]      = SEVERITY::RPT_SEVERITY_IGNORE;

    auto& drcEngine = bds.m_DRCEngine;

    BOOST_REQUIRE( drcEngine );

    drcEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                PCB_MARKER* marker = new PCB_MARKER( aItem, aPos, aLayer );

                if( aPathGenerator )
                    aPathGenerator( marker );

                m_board->Add( marker );
            } );

    drcEngine->RunTests( EDA_UNITS::MM, true, false );
    drcEngine->ClearViolationHandler();

    // Find the rule-driven courtyard exclusion marker
    int courtyardCount     = 0;
    int courtyardExcluded  = 0;

    for( PCB_MARKER* marker : m_board->Markers() )
    {
        if( marker->GetRCItem()->GetErrorCode() == DRCE_OVERLAPPING_FOOTPRINTS )
        {
            ++courtyardCount;

            if( marker->GetSeverity() == RPT_SEVERITY_EXCLUSION )
                ++courtyardExcluded;
        }
    }

    BOOST_TEST_MESSAGE( "Courtyard markers found: " << courtyardCount
                        << ", excluded: " << courtyardExcluded );

    BOOST_REQUIRE_GE( courtyardCount, 1 );
    BOOST_CHECK_EQUAL( courtyardExcluded, courtyardCount );

    // Now exercise the report writer path used by `kicad-cli pcb drc` and check the JSON output
    auto markersProvider = std::make_shared<DRC_ITEMS_PROVIDER>(
            m_board.get(), MARKER_BASE::MARKER_DRC, MARKER_BASE::MARKER_DRAWING_SHEET );
    auto ratsnestProvider = std::make_shared<DRC_ITEMS_PROVIDER>(
            m_board.get(), MARKER_BASE::MARKER_RATSNEST );
    auto fpWarningsProvider = std::make_shared<DRC_ITEMS_PROVIDER>(
            m_board.get(), MARKER_BASE::MARKER_PARITY );

    int allSev = RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING | RPT_SEVERITY_EXCLUSION;
    markersProvider->SetSeverities( allSev );
    ratsnestProvider->SetSeverities( allSev );
    fpWarningsProvider->SetSeverities( allSev );

    DRC_REPORT report( m_board.get(), EDA_UNITS::MM, markersProvider, ratsnestProvider,
                       fpWarningsProvider );

    // wxFileName::CreateTempFileName reserves a unique path; we write directly to it instead
    // of swapping the extension so we do not leave the reserved file behind.
    wxString jsonPath = wxFileName::CreateTempFileName( wxT( "kicad-drc-24264-json-" ) );
    wxString textPath = wxFileName::CreateTempFileName( wxT( "kicad-drc-24264-text-" ) );

    BOOST_REQUIRE( report.WriteJsonReport( jsonPath ) );
    BOOST_REQUIRE( report.WriteTextReport( textPath ) );

    std::ifstream reportStream( jsonPath.fn_str() );
    BOOST_REQUIRE( reportStream.is_open() );

    nlohmann::json reportJson;
    reportStream >> reportJson;
    reportStream.close();
    wxRemoveFile( jsonPath );

    BOOST_REQUIRE( reportJson.contains( "violations" ) );

    int sawCourtyardExcluded = 0;

    for( const auto& violation : reportJson["violations"] )
    {
        if( violation.value( "type", std::string() ) == "courtyards_overlap" )
        {
            BOOST_CHECK( violation.value( "excluded", false ) );

            if( violation.value( "excluded", false ) )
                ++sawCourtyardExcluded;
        }
    }

    BOOST_CHECK_EQUAL( sawCourtyardExcluded, courtyardCount );

    // Text report should also surface the exclusion suffix from RC_ITEM::ShowReport()
    std::ifstream textStream( textPath.fn_str() );
    BOOST_REQUIRE( textStream.is_open() );

    std::stringstream textBuf;
    textBuf << textStream.rdbuf();
    textStream.close();
    wxRemoveFile( textPath );

    const std::string textReport = textBuf.str();

    BOOST_TEST_MESSAGE( "Text report excerpt:\n" << textReport );
    BOOST_CHECK( textReport.find( "courtyards_overlap" ) != std::string::npos );
    BOOST_CHECK( textReport.find( "(excluded)" ) != std::string::npos );
}
