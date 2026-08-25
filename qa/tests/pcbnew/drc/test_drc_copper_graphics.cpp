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
#include <drc/drc_rtree.h>
#include <footprint.h>
#include <netclass.h>
#include <pad.h>
#include <pcb_marker.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <project/net_settings.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>


struct DRC_COPPER_GRAPHICS_TEST_FIXTURE
{
    DRC_COPPER_GRAPHICS_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


static void runLineEndingMatrixDrcTest( SETTINGS_MANAGER& aSettingsManager, std::unique_ptr<BOARD>& aBoard,
                                        const wxString& aBoardName, int aExpectedViolationCount )
{
    KI_TEST::LoadBoard( aSettingsManager, aBoardName, aBoard );

    BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

    for( int code = 0; code < DRCE_LAST; ++code )
        bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CLEARANCE] = SEVERITY::RPT_SEVERITY_ERROR;
    bds.m_DRCSeverities[DRCE_SHORTING_ITEMS] = SEVERITY::RPT_SEVERITY_ERROR;

    std::vector<DRC_ITEM> violations;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_CLEARANCE || aItem->GetErrorCode() == DRCE_SHORTING_ITEMS )
                {
                    violations.push_back( *aItem );
                }
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );
    bds.m_DRCEngine->ClearViolationHandler();

    if( (int) violations.size() == aExpectedViolationCount )
    {
        BOOST_CHECK_EQUAL( violations.size(), (size_t) aExpectedViolationCount );
        return;
    }

    UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

    wxString                  report;
    std::map<KIID, EDA_ITEM*> itemMap;
    aBoard->FillItemMap( itemMap );

    for( const DRC_ITEM& item : violations )
        report += item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap );

    BOOST_ERROR( wxString::Format( "DRC line ending matrix: %s\n"
                                   "%d violations found (expected %d)\n"
                                   "%s",
                                   aBoardName, (int) violations.size(), aExpectedViolationCount, report ) );
}


BOOST_FIXTURE_TEST_CASE( DRCCopperGraphicsTest, DRC_COPPER_GRAPHICS_TEST_FIXTURE )
{
    wxString brd_name( wxT( "test_copper_graphics" ) );
    KI_TEST::LoadBoard( m_settingsManager, brd_name, m_board );

    // Do NOT refill zones; this will prevent some of the items from being tested.
    // KI_TEST::FillZones( m_board.get() );

    std::vector<DRC_ITEM>  violations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Disable some DRC tests not useful in this testcase (and time consuming)
    bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_ISSUES ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_LIB_FOOTPRINT_MISMATCH ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_COPPER_SLIVER ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_STARVED_THERMAL ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_SILK_MASK_CLEARANCE ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_ISOLATED_COPPER ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_MIRRORED_TEXT_ON_FRONT_LAYER ] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[ DRCE_UNMIRRORED_TEXT_ON_BACK_LAYER ] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                PCB_MARKER temp( aItem, aPos );

                if( bds.m_DrcExclusions.find( DRC_EXCLUSION::FromMarker( temp ) ) == bds.m_DrcExclusions.end() )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    const int expected_err_cnt = 4;  // "What" copper text shorting zone
                                     // Copper knockout text shorting two zones twice (but not
                                     // three times as the two vertical zones are the same net)
                                     // Net-tie rectangle shorting zone

    if( violations.size() == expected_err_cnt )
    {
        BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
        BOOST_TEST_MESSAGE( "DRC copper graphics test passed" );
    }
    else
    {
        BOOST_CHECK_EQUAL( violations.size(), expected_err_cnt );

        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

        wxString report;
        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const DRC_ITEM& item : violations )
            report += item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap );

        BOOST_ERROR( wxString::Format( "DRC copper graphics: %s\n"
                                       "%d violations found (expected %d)\n"
                                       "%s",
                                       brd_name,
                                       (int) violations.size(),
                                       expected_err_cnt,
                                       report ) );
    }
}


BOOST_FIXTURE_TEST_CASE( DRCCopperGraphicLineEndingMatrix, DRC_COPPER_GRAPHICS_TEST_FIXTURE )
{
    struct MATRIX_CASE
    {
        wxString boardName;
        int      expectedViolations;
    };

    const MATRIX_CASE cases[] = {
        { wxT( "line_ending_drc/line_ending_drc_fail" ), 123 },
        { wxT( "line_ending_drc/line_ending_drc_pass" ), 0 },
    };

    for( const MATRIX_CASE& testCase : cases )
    {
        runLineEndingMatrixDrcTest( m_settingsManager, m_board, testCase.boardName, testCase.expectedViolations );
    }
}


BOOST_FIXTURE_TEST_CASE( DRCCopperGraphicLineEndingAgainstTrack, DRC_COPPER_GRAPHICS_TEST_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 2 );

    NETINFO_ITEM* netA = new NETINFO_ITEM( m_board.get(), wxS( "A" ), 1 );
    NETINFO_ITEM* netB = new NETINFO_ITEM( m_board.get(), wxS( "B" ), 2 );
    m_board->Add( netA );
    m_board->Add( netB );

    auto addPad = [&]( const VECTOR2I& aPos, NETINFO_ITEM* aNet )
    {
        FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
        fp->SetPosition( aPos );
        m_board->Add( fp );

        PAD* pad = new PAD( fp );
        pad->SetPadstackMode( PADSTACK::MODE::NORMAL );
        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( pcbIUScale.mmToIU( 0.4 ), pcbIUScale.mmToIU( 0.4 ) ) );
        pad->SetLayerSet( LSET( { F_Cu } ) );
        pad->SetPosition( aPos );
        pad->SetNet( aNet );
        fp->Add( pad );

        return pad;
    };

    PCB_SHAPE* graphic = new PCB_SHAPE( m_board.get(), SHAPE_T::SEGMENT );
    graphic->SetLayer( F_Cu );
    graphic->SetStart( VECTOR2I( 0, 0 ) );
    graphic->SetEnd( VECTOR2I( 0, pcbIUScale.mmToIU( 4.0 ) ) );
    graphic->SetWidth( pcbIUScale.mmToIU( 0.1 ) );
    graphic->SetStartEndingStyle( LINE_ENDING_STYLE::SQUARE );
    graphic->SetStartEndingLength( pcbIUScale.mmToIU( 1.0 ) );
    graphic->SetStartEndingWidth( pcbIUScale.mmToIU( 1.0 ) );
    graphic->SetNet( netA );
    m_board->Add( graphic );
    PAD* graphicPad = addPad( graphic->GetEnd(), netA );

    PCB_TRACK* track = new PCB_TRACK( m_board.get() );
    track->SetLayer( F_Cu );
    track->SetStart( VECTOR2I( -pcbIUScale.mmToIU( 4.0 ), 0 ) );
    track->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 0.4 ), 0 ) );
    track->SetWidth( pcbIUScale.mmToIU( 0.05 ) );
    track->SetNet( netB );
    m_board->Add( track );
    PAD* trackPad = addPad( track->GetStart(), netB );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    bds.m_MinClearance = pcbIUScale.mmToIU( 0.5 );
    bds.m_NetSettings->GetDefaultNetclass()->SetClearance( pcbIUScale.mmToIU( 0.5 ) );

    if( !bds.m_DRCEngine )
        bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( m_board.get(), &bds );

    bds.m_DRCEngine->InitEngine( wxFileName() );

    for( int code = 0; code < DRCE_LAST; ++code )
        bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CLEARANCE] = SEVERITY::RPT_SEVERITY_ERROR;
    bds.m_DRCSeverities[DRCE_SHORTING_ITEMS] = SEVERITY::RPT_SEVERITY_ERROR;

    BOOST_REQUIRE_EQUAL( m_board->Drawings().size(), 1 );
    BOOST_REQUIRE_EQUAL( m_board->Tracks().size(), 1 );
    BOOST_REQUIRE_EQUAL( graphicPad->GetNetCode(), netA->GetNetCode() );
    BOOST_REQUIRE_EQUAL( trackPad->GetNetCode(), netB->GetNetCode() );
    BOOST_REQUIRE_NE( graphic->GetNetCode(), track->GetNetCode() );

    DRC_CONSTRAINT directConstraint = bds.m_DRCEngine->EvalRules( CLEARANCE_CONSTRAINT, graphic, track, F_Cu );
    BOOST_REQUIRE( directConstraint.GetSeverity() != RPT_SEVERITY_IGNORE );
    BOOST_REQUIRE_GT( directConstraint.GetValue().Min(), 0 );

    DRC_CONSTRAINT reverseConstraint = bds.m_DRCEngine->EvalRules( CLEARANCE_CONSTRAINT, track, graphic, F_Cu );
    BOOST_REQUIRE( reverseConstraint.GetSeverity() != RPT_SEVERITY_IGNORE );
    BOOST_REQUIRE_GT( reverseConstraint.GetValue().Min(), 0 );

    std::shared_ptr<SHAPE> trackShape = track->GetEffectiveShape( F_Cu );
    std::shared_ptr<SHAPE> graphicShape = graphic->GetEffectiveShape( F_Cu );
    int                    actual = 0;
    VECTOR2I               pos;

    BOOST_REQUIRE( trackShape );
    BOOST_REQUIRE( graphicShape );
    BOOST_REQUIRE( trackShape->Collide( graphicShape.get(), directConstraint.GetValue().Min(), &actual, &pos ) );

    int              violationCount = 0;
    std::vector<int> violationCodes;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& /*aPos*/, int /*aLayer*/,
                 const std::function<void( PCB_MARKER* )>& /*aPathGenerator*/ )
            {
                violationCodes.push_back( aItem->GetErrorCode() );

                if( aItem->GetErrorCode() == DRCE_CLEARANCE || aItem->GetErrorCode() == DRCE_SHORTING_ITEMS )
                {
                    ++violationCount;
                }
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );
    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_REQUIRE( m_board->m_CopperItemRTreeCache );
    BOOST_CHECK_GE( m_board->m_CopperItemRTreeCache->QueryColliding( track, F_Cu, F_Cu,
                                                                     [&]( BOARD_ITEM* other ) -> bool
                                                                     {
                                                                         return other == graphic;
                                                                     } ),
                    1 );
    BOOST_CHECK_GE( m_board->m_CopperItemRTreeCache->QueryColliding(
                            track, F_Cu, F_Cu,
                            [&]( BOARD_ITEM* other ) -> bool
                            {
                                return other == graphic;
                            },
                            nullptr, m_board->m_DRCMaxClearance ),
                    1 );

    BOOST_CHECK_NE( graphic->GetNetCode(), track->GetNetCode() );

    if( violationCount < 1 )
    {
        wxString codes;

        for( int code : violationCodes )
            codes += wxString::Format( wxT( " %d" ), code );

        BOOST_TEST_MESSAGE( wxString::Format( wxT( "Reported violation codes:%s" ), codes ) );
    }

    BOOST_CHECK_GE( violationCount, 1 );
}
