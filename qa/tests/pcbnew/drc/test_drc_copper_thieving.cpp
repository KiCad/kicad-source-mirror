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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 2 );

    // Each thieving stamp is an isolated island; without the DRC exclusion every
    // one would trigger DRCE_ISOLATED_COPPER.
    ZONE* zone = new ZONE( m_board.get() );
    zone->SetLayer( F_Cu );
    zone->AppendCorner( VECTOR2I( 0, 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ), -1 );
    zone->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ), -1 );
    zone->AppendCorner( VECTOR2I( 0, pcbIUScale.mmToIU( 20 ) ), -1 );
    zone->SetFillMode( ZONE_FILL_MODE::COPPER_THIEVING );

    THIEVING_SETTINGS thieving;
    thieving.pattern      = THIEVING_PATTERN::DOTS;
    thieving.element_size = pcbIUScale.mmToIU( 0.5 );
    thieving.gap        = pcbIUScale.mmToIU( 2.0 );
    zone->SetThievingSettings( thieving );
    zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
    m_board->Add( zone );

    KI_TEST::FillZones( m_board.get() );

    int isolatedCount = 0;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Synthetic boards skip the loader, so the DRC engine is not yet wired up.
    // The loader does this at board_loader.cpp:132; replicate it here.
    if( !bds.m_DRCEngine )
        bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( m_board.get(), &bds );

    bds.m_DRCEngine->InitEngine( wxFileName() );

    // Disable all DRC checks except the one we care about so the test focuses on
    // the isolated-copper path and runs fast on a synthetic board.
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
