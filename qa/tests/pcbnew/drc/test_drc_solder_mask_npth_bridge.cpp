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

/**
 * F.Mask polygon over a GND zone, GND vias, and an NPTH with an annular ring
 * must produce zero DRCE_SOLDERMASK_BRIDGE.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_NPTH_MASK_BRIDGE_FIXTURE
{
    DRC_NPTH_MASK_BRIDGE_FIXTURE() = default;

    ~DRC_NPTH_MASK_BRIDGE_FIXTURE()
    {
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();

        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCSolderMaskNPTHBridge, DRC_NPTH_MASK_BRIDGE_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "npth_mask_bridge_test", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load fixture" );

    KI_TEST::FillZones( m_board.get() );

    std::vector<std::shared_ptr<DRC_ITEM>> bridgeViolations;
    BOARD_DESIGN_SETTINGS&                 bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_SOLDERMASK_BRIDGE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& /*aPos*/, int /*aLayer*/,
                 const std::function<void( PCB_MARKER* )>& /*aPathGen*/ )
            {
                if( aItem->GetErrorCode() == DRCE_SOLDERMASK_BRIDGE )
                    bridgeViolations.push_back( aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_TEST_MESSAGE( wxString::Format( "Soldermask bridge violations: %d", (int) bridgeViolations.size() ) );

    if( !bridgeViolations.empty() )
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MM );

        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const std::shared_ptr<DRC_ITEM>& item : bridgeViolations )
            BOOST_TEST_MESSAGE( item->ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );
    }

    // Everything exposed by the F.Mask polygon is either GND (zone, vias) or
    // an NPTH (no net). No real bridge exists.
    BOOST_CHECK_EQUAL( bridgeViolations.size(), 0 );
}
