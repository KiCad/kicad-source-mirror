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
#include <boost/test/data/test_case.hpp>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_report.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <footprint.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>

#include <thread>
#include <chrono>
#include <exception>
#include <fstream>

#include <json_common.h>
#include <wx/filefn.h>
#include <wx/filename.h>


namespace
{
struct DRC_REGRESSION_TEST_FIXTURE
{
    DRC_REGRESSION_TEST_FIXTURE()
    { }

    ~DRC_REGRESSION_TEST_FIXTURE()
    {
        // Ensure proper cleanup
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
        {
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();
        }

        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


nlohmann::json writeAndReadJsonReport( BOARD* aBoard )
{
    auto markersProvider =
            std::make_shared<DRC_ITEMS_PROVIDER>( aBoard, MARKER_BASE::MARKER_DRC, MARKER_BASE::MARKER_DRAWING_SHEET );
    auto ratsnestProvider = std::make_shared<DRC_ITEMS_PROVIDER>( aBoard, MARKER_BASE::MARKER_RATSNEST );
    auto fpWarningsProvider = std::make_shared<DRC_ITEMS_PROVIDER>( aBoard, MARKER_BASE::MARKER_PARITY );

    int allSeverities = RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING | RPT_SEVERITY_EXCLUSION;
    markersProvider->SetSeverities( allSeverities );
    ratsnestProvider->SetSeverities( allSeverities );
    fpWarningsProvider->SetSeverities( allSeverities );

    DRC_REPORT report( aBoard, EDA_UNITS::MM, markersProvider, ratsnestProvider, fpWarningsProvider );

    wxString jsonPath = wxFileName::CreateTempFileName( wxT( "kicad-drc-json-" ) );
    bool     written = report.WriteJsonReport( jsonPath );

    nlohmann::json reportJson;

    if( written )
    {
        std::ifstream reportStream( jsonPath.fn_str() );

        if( reportStream.is_open() )
            reportStream >> reportJson;
    }

    wxRemoveFile( jsonPath );
    BOOST_REQUIRE( written );

    return reportJson;
}
} // namespace


// clang-format off
static const std::vector<std::pair<wxString, int>> DRCCopperSliver_cases =
{
    { "sliver",       1  },
    { "issue14449",   0  },
    { "issue14549",   0  },
    { "issue14549_2", 0  },
    { "issue14559",   0  }
};
// clang-format on


BOOST_DATA_TEST_CASE_F( DRC_REGRESSION_TEST_FIXTURE, DRCCopperSliver,
                        boost::unit_test::data::make( DRCCopperSliver_cases ), test )
{
    // Check for minimum copper connection errors

    KI_TEST::LoadBoard( m_settingsManager, test.first, m_board );

    // Validate board was loaded successfully
    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board for test: " + test.first.ToStdString() );

    KI_TEST::FillZones( m_board.get() );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Validate DRC engine is properly initialized
    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized for test: " + test.first.ToStdString() );

    // Disable DRC tests not useful or not handled in this testcase
    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    // Ensure that our desired error is fired
    bds.m_DRCSeverities[DRCE_COPPER_SLIVER] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                    violations.push_back( *aItem );
            } );

    BOOST_TEST_CHECKPOINT( "Running copper sliver drc" );

    // Add exception handling around DRC engine run to catch potential crashes
    try
    {
        bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

        // Add a small delay to allow any background threads to complete
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        BOOST_TEST_CHECKPOINT( "DRC engine run completed successfully" );
    }
    catch( const std::exception& e )
    {
        BOOST_ERROR( wxString::Format( "DRC copper sliver: %s, exception during RunTests: %s",
                                       test.first, e.what() ) );
        return;
    }
    catch( ... )
    {
        BOOST_ERROR( wxString::Format( "DRC copper sliver: %s, unknown exception during RunTests",
                                       test.first ) );
        return;
    }

    // Clear the violation handler to prevent any potential issues with cleanup
    BOOST_TEST_CHECKPOINT( "Clearing violation handler" );
    bds.m_DRCEngine->ClearViolationHandler();

    if( violations.size() == test.second )
    {
        BOOST_CHECK_EQUAL( 1, 1 ); // quiet "did not check any assertions" warning
        BOOST_TEST_MESSAGE( wxString::Format( "DRC copper sliver: %s, passed", test.first ) );
    }
    else
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

        BOARD_RC_ITEM_RESOLVER resolver( *m_board );

        // Report violations with bounds checking
        for( size_t i = 0; i < violations.size() && i < 100; ++i ) // Limit output to prevent excessive logging
        {
            try
            {
                const DRC_ITEM& item = violations[i];
                BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, resolver ) );
            }
            catch( const std::exception& e )
            {
                BOOST_TEST_MESSAGE( wxString::Format( "Error reporting violation %zu: %s", i, e.what() ) );
            }
            catch( ... )
            {
                BOOST_TEST_MESSAGE( wxString::Format( "Unknown error reporting violation %zu", i ) );
            }
        }

        BOOST_ERROR( wxString::Format( "DRC copper sliver: %s, failed (violations found %d "
                                       "expected %d)",
                                       test.first,
                                       (int) violations.size(),
                                       test.second ) );
    }
}


BOOST_FIXTURE_TEST_CASE( DRCCopperSliverJsonReportsMarkerLocation, DRC_REGRESSION_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "sliver", m_board );
    BOOST_REQUIRE( m_board );

    KI_TEST::FillZones( m_board.get() );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    BOOST_REQUIRE( bds.m_DRCEngine );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_COPPER_SLIVER] = SEVERITY::RPT_SEVERITY_ERROR;

    VECTOR2I expectedPos;
    int      expectedLayer = UNDEFINED_LAYER;
    int      sliverCount = 0;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() != DRCE_COPPER_SLIVER )
                    return;

                expectedPos = aPos;
                expectedLayer = aLayer;
                ++sliverCount;

                PCB_MARKER* marker = new PCB_MARKER( aItem, aPos, aLayer );

                if( aPathGenerator )
                    aPathGenerator( marker );

                m_board->Add( marker );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );
    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_REQUIRE_EQUAL( sliverCount, 1 );
    BOOST_REQUIRE( IsValidLayer( expectedLayer ) );

    nlohmann::json reportJson = writeAndReadJsonReport( m_board.get() );

    bool foundSliver = false;

    for( const auto& violation : reportJson["violations"] )
    {
        if( violation.value( "type", std::string() ) != "copper_sliver" )
            continue;

        foundSliver = true;
        BOOST_REQUIRE( violation.contains( "pos" ) );
        BOOST_REQUIRE( violation.contains( "layer" ) );
        BOOST_CHECK( violation["items"].empty() );

        const double expectedX = pcbIUScale.IUTomm( expectedPos.x );
        const double expectedY = pcbIUScale.IUTomm( expectedPos.y );
        const double actualX = violation["pos"]["x"].get<double>();
        const double actualY = violation["pos"]["y"].get<double>();

        BOOST_CHECK_SMALL( actualX - expectedX, 1e-9 );
        BOOST_CHECK_SMALL( actualY - expectedY, 1e-9 );
        BOOST_CHECK_EQUAL( violation["layer"].get<std::string>(),
                           m_board->GetLayerName( ToLAYER_ID( expectedLayer ) ).ToStdString() );
    }

    BOOST_CHECK( foundSliver );
}


BOOST_FIXTURE_TEST_CASE( DRCJsonReportOmitsUnknownMarkerLocation, DRC_REGRESSION_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "sliver", m_board );
    BOOST_REQUIRE( m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_MISSING_FOOTPRINT] = SEVERITY::RPT_SEVERITY_ERROR;

    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MISSING_FOOTPRINT );
    drcItem->SetErrorMessage( wxT( "Missing footprint" ) );
    m_board->Add( new PCB_MARKER( drcItem, VECTOR2I(), UNDEFINED_LAYER ) );

    nlohmann::json reportJson = writeAndReadJsonReport( m_board.get() );

    bool foundMissingFootprint = false;

    for( const auto& violation : reportJson["schematic_parity"] )
    {
        if( violation.value( "type", std::string() ) != "missing_footprint" )
            continue;

        foundMissingFootprint = true;
        BOOST_CHECK( !violation.contains( "pos" ) );
        BOOST_CHECK( !violation.contains( "layer" ) );
    }

    BOOST_CHECK( foundMissingFootprint );
}
