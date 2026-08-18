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

/**
 * @file test_drc_creepage.cpp
 *
 * Creepage DRC test with HV/GND netclass constraints. The board has an HV trace
 * running near a GND trace. The custom rule requires 8mm creepage between HV and
 * GND netclasses, but the actual surface distance is ~3.25mm.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>
#include <base_units.h>
#include <netinfo.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <zone.h>

#include <filesystem>
#include <fstream>


struct DRC_CREEPAGE_TEST_FIXTURE
{
    DRC_CREEPAGE_TEST_FIXTURE() = default;

    ~DRC_CREEPAGE_TEST_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( CreepageHVvsGND, DRC_CREEPAGE_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "creepage/creepage", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board creepage" );

    std::vector<std::shared_ptr<DRC_ITEM>> violations;
    BOARD_DESIGN_SETTINGS&                 bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                    violations.push_back( aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d creepage violations", (int) violations.size() ) );

    for( const auto& v : violations )
        BOOST_TEST_MESSAGE( wxString::Format( "  Violation: %s", v->GetErrorMessage( false ) ) );

    // The board has HV and GND netclass traces ~3.25mm apart. The custom rule requires
    // 8mm creepage between HV and GND netclasses, so at least one violation must be detected.
    BOOST_CHECK_GE( violations.size(), 1 );
}


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23653
 *
 * A board with an extra line on Edge.Cuts that prevents GetBoardPolygonOutlines
 * from forming a valid polygon should still produce creepage violations. Previously
 * the creepage test bailed out entirely when the outline was malformed.
 */
BOOST_FIXTURE_TEST_CASE( CreepageMalformedEdge, DRC_CREEPAGE_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "creepage/creepage_malformed_edge", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board creepage_malformed_edge" );

    std::vector<std::shared_ptr<DRC_ITEM>> violations;
    BOARD_DESIGN_SETTINGS&                 bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                    violations.push_back( aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d creepage violations (malformed edge)",
                                          (int) violations.size() ) );

    for( const auto& v : violations )
        BOOST_TEST_MESSAGE( wxString::Format( "  Violation: %s", v->GetErrorMessage( false ) ) );

    // Same board geometry as CreepageHVvsGND but with an extra malformed Edge.Cuts line.
    // The creepage DRC must still detect violations even when the board outline is invalid.
    BOOST_CHECK_GE( violations.size(), 1 );
}


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/25165
 *
 * A rule area has no net, so it lands on netcode 0 and used to be collected as copper.
 * The netless corner tracks give netcode 0 a bounding box wide enough to get past the
 * early reject in testCreepage. They sit too far from the HV track to violate anything.
 */
BOOST_FIXTURE_TEST_CASE( CreepageIgnoresRuleAreas, DRC_CREEPAGE_TEST_FIXTURE )
{
    KI_TEST::TEMPORARY_DIRECTORY tmpDir( "kicad_creepage_rule_area", "" );

    const std::filesystem::path rulePath = tmpDir.GetPath() / "creepage_rule_area.kicad_dru";

    {
        std::ofstream ruleFile( rulePath );
        ruleFile << "(version 1)\n"
                 << "(rule OVC_3_Reinforced (constraint creepage (min 5.5mm)))\n";
    }

    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 2 );

    PCB_SHAPE* edge = new PCB_SHAPE( m_board.get(), SHAPE_T::RECTANGLE );
    edge->SetLayer( Edge_Cuts );
    edge->SetStart( VECTOR2I( pcbIUScale.mmToIU( 100 ), pcbIUScale.mmToIU( 80 ) ) );
    edge->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 170 ), pcbIUScale.mmToIU( 130 ) ) );
    m_board->Add( edge );

    // Restricts nothing and spans every copper layer, like the areas the Multi-Channel tool
    // generates.
    ZONE* ruleArea = new ZONE( m_board.get() );
    ruleArea->SetIsRuleArea( true );
    ruleArea->SetLayerSet( LSET( { F_Cu, B_Cu } ) );
    ruleArea->SetDoNotAllowZoneFills( false );
    ruleArea->SetDoNotAllowVias( false );
    ruleArea->SetDoNotAllowTracks( false );
    ruleArea->SetDoNotAllowPads( false );
    ruleArea->SetDoNotAllowFootprints( false );
    ruleArea->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 117 ), pcbIUScale.mmToIU( 97 ) ), -1 );
    ruleArea->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 153 ), pcbIUScale.mmToIU( 97 ) ), -1 );
    ruleArea->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 153 ), pcbIUScale.mmToIU( 103 ) ), -1 );
    ruleArea->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 117 ), pcbIUScale.mmToIU( 103 ) ), -1 );
    m_board->Add( ruleArea );

    NETINFO_ITEM* hvNet = new NETINFO_ITEM( m_board.get(), wxT( "HV" ), 1 );
    m_board->Add( hvNet );

    auto addTrack = [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd, NETINFO_ITEM* aNet )
    {
        PCB_TRACK* track = new PCB_TRACK( m_board.get() );
        track->SetLayer( F_Cu );
        track->SetWidth( pcbIUScale.mmToIU( 0.2 ) );
        track->SetStart( aStart );
        track->SetEnd( aEnd );

        if( aNet )
            track->SetNet( aNet );

        m_board->Add( track );
    };

    addTrack( VECTOR2I( pcbIUScale.mmToIU( 120 ), pcbIUScale.mmToIU( 100 ) ),
              VECTOR2I( pcbIUScale.mmToIU( 150 ), pcbIUScale.mmToIU( 100 ) ), hvNet );

    // Netless, and at right angles so netcode 0's bounding box spans the HV track. Two parallel
    // segments would give a flat box that the reject still throws out.
    addTrack( VECTOR2I( pcbIUScale.mmToIU( 103 ), pcbIUScale.mmToIU( 127 ) ),
              VECTOR2I( pcbIUScale.mmToIU( 167 ), pcbIUScale.mmToIU( 127 ) ), nullptr );
    addTrack( VECTOR2I( pcbIUScale.mmToIU( 167 ), pcbIUScale.mmToIU( 127 ) ),
              VECTOR2I( pcbIUScale.mmToIU( 167 ), pcbIUScale.mmToIU( 83 ) ), nullptr );

    m_board->BuildConnectivity();

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( m_board.get(), &bds );
    bds.m_DRCEngine->InitEngine( wxFileName( rulePath.string() ) );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    std::vector<std::shared_ptr<DRC_ITEM>> violations;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_CREEPAGE )
                    violations.push_back( aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    int ruleAreaHits = 0;

    for( const std::shared_ptr<DRC_ITEM>& item : violations )
    {
        BOOST_TEST_MESSAGE( wxString::Format( "  Violation: %s", item->GetErrorMessage( false ) ) );

        for( const KIID& id : { item->GetMainItemID(), item->GetAuxItemID() } )
        {
            BOARD_ITEM* boardItem = m_board->ResolveItem( id, true );

            if( boardItem && boardItem->Type() == PCB_ZONE_T && static_cast<ZONE*>( boardItem )->GetIsRuleArea() )
            {
                ++ruleAreaHits;
            }
        }
    }

    BOOST_CHECK_MESSAGE( ruleAreaHits == 0,
                         wxString::Format( "%d creepage violations name the rule area as a violating item. A "
                                           "rule area is a boundary, not copper.",
                                           ruleAreaHits ) );

    BOOST_CHECK_MESSAGE( violations.empty(),
                         wxString::Format( "Expected no creepage violations, got %d.", (int) violations.size() ) );
}
