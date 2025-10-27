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


struct DRC_REGRESSION_TEST_FIXTURE
{
    // clang-format off : suggestions look worse.
    DRC_REGRESSION_TEST_FIXTURE() {}
    // clang-format on

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCStarvedThermal, DRC_REGRESSION_TEST_FIXTURE )
{
    // Check for starved thermal connection

    // clang-format off : suggestions look worse.
    std::vector<std::pair<wxString, int>> tests = {
        { "test_starved_thermal", 2 },
    };
    // clang-format on

    for( const std::pair<wxString, int>& test : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, test.first, m_board );
        KI_TEST::FillZones( m_board.get() );

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        // Ensure that our desired error is fired
        bds.m_DRCSeverities[DRCE_STARVED_THERMAL] = SEVERITY::RPT_SEVERITY_ERROR;

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
            BOOST_TEST_MESSAGE( wxString::Format( "DRC starved thermal: %s, passed", test.first ) );
        }
        else
        {
            UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

            wxString report;
            std::map<KIID, EDA_ITEM*> itemMap;
            m_board->FillItemMap( itemMap );

            for( const DRC_ITEM& item : violations )
                report += item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap );

            BOOST_ERROR( wxString::Format( "DRC starved thermal: %s\n"
                                           "%d violations found (expected %d)\n"
                                           "%s",
                                           test.first,
                                           (int) violations.size(),
                                           test.second,
                                           report ) );
        }
    }
}
