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
#include <boost/test/data/test_case.hpp>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_marker.h>
#include <footprint.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>

#include <thread>
#include <chrono>
#include <exception>


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

        std::map<KIID, EDA_ITEM*> itemMap;

        // Safely build the item map with error handling
        try
        {
            m_board->FillItemMap( itemMap );
        }
        catch( const std::exception& e )
        {
            BOOST_ERROR( wxString::Format( "DRC copper sliver: %s, exception in FillItemMap: %s",
                                           test.first, e.what() ) );
            return;
        }
        catch( ... )
        {
            BOOST_ERROR( wxString::Format( "DRC copper sliver: %s, unknown exception in FillItemMap",
                                           test.first ) );
            return;
        }

        // Safely report violations with bounds checking
        for( size_t i = 0; i < violations.size() && i < 100; ++i ) // Limit output to prevent excessive logging
        {
            try
            {
                const DRC_ITEM& item = violations[i];
                BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );
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
