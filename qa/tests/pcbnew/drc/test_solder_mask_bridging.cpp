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
#include <pcb_marker.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>


struct DRC_SOLDER_MASK_BRIDGING_TEST_FIXTURE
{
    DRC_SOLDER_MASK_BRIDGING_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCSolderMaskBridgingTest, DRC_SOLDER_MASK_BRIDGING_TEST_FIXTURE )
{
    wxString brd_name( wxT( "solder_mask_bridge_test" ) );
    KI_TEST::LoadBoard( m_settingsManager, brd_name, m_board );
    KI_TEST::FillZones( m_board.get() );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Disable some DRC tests not useful in this testcase (and time consuming)
    bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_ISSUES ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_MISMATCH ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_COPPER_SLIVER ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_STARVED_THERMAL ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_SILK_CLEARANCE ] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, VECTOR2I aPos, int aLayer,
                 DRC_CUSTOM_MARKER_HANDLER* aCustomHandler )
            {
                PCB_MARKER temp( aItem, aPos );

                if( bds.m_DrcExclusions.find( temp.SerializeToString() ) == bds.m_DrcExclusions.end() )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    const int expected_err_cnt = 5;

    if( violations.size() == expected_err_cnt )
    {
        BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
        BOOST_TEST_MESSAGE( "DRC solder mask bridge test passed" );
    }
    else
    {
        BOOST_CHECK_EQUAL( violations.size(), expected_err_cnt );

        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::IN );

        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const DRC_ITEM& item : violations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );

        BOOST_ERROR( wxString::Format( "DRC solder mask bridge test failed board <%s>", brd_name ) );
    }
}
