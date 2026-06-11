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
 * @file test_creepage_engine_parity.cpp
 *
 * Runs the batch creepage DRC twice on each corpus board, once with the legacy solver and once
 * with the V2 engine, and asserts both report the same set of violations.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <advanced_config.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>

#include <set>


struct CREEPAGE_PARITY_FIXTURE
{
    CREEPAGE_PARITY_FIXTURE() = default;

    ~CREEPAGE_PARITY_FIXTURE()
    {
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();

        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    /// Run the creepage DRC and return the multiset of violation messages. Messages carry the
    /// constraint name and distances but not the violating items; Dijkstra breaks ties by node
    /// pointer, so equal-length paths can resolve to different equidistant items between runs.
    std::multiset<wxString> runCreepage()
    {
        std::multiset<wxString> result;
        BOARD_DESIGN_SETTINGS&  bds = m_board->GetDesignSettings();

        for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
            bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

        bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                     const std::function<void( PCB_MARKER* )>& )
                {
                    if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                        result.emplace( aItem->GetErrorMessage( false ) );
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );
        bds.m_DRCEngine->ClearViolationHandler();

        return result;
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


struct ADVANCED_CFG_FLAG_GUARD
{
    explicit ADVANCED_CFG_FLAG_GUARD( bool aValue )
    {
        bool& flag = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_RealtimeCreepage;
        m_saved = flag;
        flag = aValue;
    }

    ~ADVANCED_CFG_FLAG_GUARD()
    {
        const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() ).m_RealtimeCreepage = m_saved;
    }

    bool m_saved;
};


static void checkParity( CREEPAGE_PARITY_FIXTURE& aFixture, const std::string& aBoard )
{
    KI_TEST::LoadBoard( aFixture.m_settingsManager, aBoard, aFixture.m_board );
    BOOST_REQUIRE_MESSAGE( aFixture.m_board, "Failed to load board " << aBoard );
    BOOST_REQUIRE_MESSAGE( aFixture.m_board->GetDesignSettings().m_DRCEngine,
                           "DRC engine not initialized" );

    std::multiset<wxString> v1;
    std::multiset<wxString> v2;

    {
        ADVANCED_CFG_FLAG_GUARD guard( false );
        v1 = aFixture.runCreepage();
    }

    {
        ADVANCED_CFG_FLAG_GUARD guard( true );
        v2 = aFixture.runCreepage();
    }

    BOOST_TEST_MESSAGE( wxString::Format( "%s: V1=%d V2=%d violations", aBoard.c_str(),
                                          (int) v1.size(), (int) v2.size() ) );

    BOOST_CHECK_EQUAL( v1.size(), v2.size() );
    BOOST_CHECK_MESSAGE( v1 == v2, "V2 engine violation set differs from V1 for " << aBoard );
}


BOOST_FIXTURE_TEST_SUITE( CreepageEngineParity, CREEPAGE_PARITY_FIXTURE )


BOOST_AUTO_TEST_CASE( CreepageBoard )
{
    checkParity( *this, "creepage/creepage" );
}


BOOST_AUTO_TEST_CASE( CreepageMalformedEdge )
{
    checkParity( *this, "creepage/creepage_malformed_edge" );
}


BOOST_AUTO_TEST_CASE( CreepageSlots )
{
    checkParity( *this, "creepage_slots/creepage_slots" );
}


BOOST_AUTO_TEST_SUITE_END()
