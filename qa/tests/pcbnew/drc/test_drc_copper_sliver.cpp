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


struct DRC_REGRESSION_TEST_FIXTURE
{
    DRC_REGRESSION_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

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
    KI_TEST::FillZones( m_board.get() );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Disable DRC tests not useful or not handled in this testcase
    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    // Ensure that our desired error is fired
    bds.m_DRCSeverities[DRCE_COPPER_SLIVER] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, VECTOR2I aPos, int aLayer,
                 DRC_CUSTOM_MARKER_HANDLER* aCustomHandler )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    if( violations.size() == test.second )
    {
        BOOST_CHECK_EQUAL( 1, 1 ); // quiet "did not check any assertions" warning
        BOOST_TEST_MESSAGE( wxString::Format( "DRC copper sliver: %s, passed", test.first ) );
    }
    else
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::IN );

        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const DRC_ITEM& item : violations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );

        BOOST_ERROR( wxString::Format( "DRC copper sliver: %s, failed (violations found %d "
                                       "expected %d)",
                                       test.first,
                                       (int) violations.size(),
                                       test.second ) );
    }
}
