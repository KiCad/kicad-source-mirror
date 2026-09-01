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
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>
#include <zone.h>
#include <pcbnew_utils/board_file_utils.h>


struct DRC_COPPER_THIEVING_FIXTURE
{
    DRC_COPPER_THIEVING_FIXTURE() {}

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


/**
 * A copper-thieving zone produces a grid of intentionally-isolated stamps.
 * Each stamp would trigger DRCE_ISOLATED_COPPER without the explicit exclusion
 * in drc_cache_generator and drc_test_provider_connectivity.  Verify that a
 * thieving zone produces zero isolated-copper violations.
 */
BOOST_FIXTURE_TEST_CASE( ThievingZoneProducesNoIsolatedCopperViolations,
                         DRC_COPPER_THIEVING_FIXTURE )
{
    m_board = KI_TEST::ReadBoardFromFileOrStream(
            KI_TEST::GetPcbnewTestDataDir() + "drc_copper_thieving/copper_thieving.kicad_pcb" );

    BOOST_REQUIRE( m_board );

    // A board with no thieving fill also reports no isolated copper, so confirm
    // the stamps the exclusion has to cover are actually present.
    BOOST_REQUIRE_EQUAL( m_board->Zones().size(), 1u );

    const ZONE* zone = m_board->Zones().front();

    BOOST_REQUIRE( zone->GetFillMode() == ZONE_FILL_MODE::COPPER_THIEVING );
    BOOST_REQUIRE_GT( zone->GetFilledPolysList( F_Cu )->OutlineCount(), 1 );

    int isolatedCount = 0;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // The QA loader does not wire up the DRC engine the way board_loader.cpp does.
    if( !bds.m_DRCEngine )
        bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( m_board.get(), &bds );

    bds.m_DRCEngine->InitEngine( wxFileName() );

    // Disable all DRC checks except the one we care about so the test focuses on
    // the isolated-copper path.
    for( int code = 0; code < DRCE_LAST; ++code )
    {
        if( code != DRCE_ISOLATED_COPPER )
            bds.m_DRCSeverities[ code ] = SEVERITY::RPT_SEVERITY_IGNORE;
    }

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& /*aPos*/, int /*aLayer*/,
                 const std::function<void( PCB_MARKER* )>& /*aPathGenerator*/ )
            {
                if( aItem->GetErrorCode() == DRCE_ISOLATED_COPPER )
                    ++isolatedCount;
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_CHECK_EQUAL( isolatedCount, 0 );
}
