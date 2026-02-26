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
#include <board_commit.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint.h>
#include <zone.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>
#include <geometry/shape_poly_set.h>
#include <advanced_config.h>
#include <connectivity/connectivity_data.h>
#include <teardrop/teardrop.h>


struct ZONE_FILL_TEST_FIXTURE
{
    ZONE_FILL_TEST_FIXTURE()
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


int delta = KiROUND( 0.006 * pcbIUScale.IU_PER_MM );


BOOST_FIXTURE_TEST_CASE( BasicZoneFills, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "zone_filler", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    KI_TEST::FillZones( m_board.get() );

    // Now that the zones are filled we're going to increase the size of -some- pads and
    // tracks so that they generate DRC errors.  The test then makes sure that those errors
    // are generated, and that the other pads and tracks do -not- generate errors.

    for( PAD* pad : m_board->Footprints()[0]->Pads() )
    {
        if( pad->GetNumber() == "2" || pad->GetNumber() == "4" || pad->GetNumber() == "6" )
        {
            pad->SetSize( PADSTACK::ALL_LAYERS,
                          pad->GetSize( PADSTACK::ALL_LAYERS ) + VECTOR2I( delta, delta ) );
        }
    }

    int  ii = 0;
    KIID arc8;
    KIID arc12;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_ARC_T )
        {
            ii++;

            if( ii == 8 )
            {
                arc8 = track->m_Uuid;
                track->SetWidth( track->GetWidth() + delta + delta );
            }
            else if( ii == 12 )
            {
                arc12 = track->m_Uuid;
                track->Move( VECTOR2I( -delta, -delta ) );
            }
        }
    }

    bool foundPad2Error = false;
    bool foundPad4Error = false;
    bool foundPad6Error = false;
    bool foundArc8Error = false;
    bool foundArc12Error = false;
    bool foundOtherError = false;

    bds.m_DRCEngine->InitEngine( wxFileName() );     // Just to be sure to be sure

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_CLEARANCE )
                {
                    BOARD_ITEM* item_a = m_board->ResolveItem( aItem->GetMainItemID() );
                    PAD*        pad_a = dynamic_cast<PAD*>( item_a );
                    PCB_TRACK*  trk_a = dynamic_cast<PCB_TRACK*>( item_a );

                    BOARD_ITEM* item_b = m_board->ResolveItem( aItem->GetAuxItemID() );
                    PAD*        pad_b = dynamic_cast<PAD*>( item_b );
                    PCB_TRACK*  trk_b = dynamic_cast<PCB_TRACK*>( item_b );

                    if(      pad_a && pad_a->GetNumber() == "2" ) foundPad2Error = true;
                    else if( pad_a && pad_a->GetNumber() == "4" ) foundPad4Error = true;
                    else if( pad_a && pad_a->GetNumber() == "6" ) foundPad6Error = true;
                    else if( pad_b && pad_b->GetNumber() == "2" ) foundPad2Error = true;
                    else if( pad_b && pad_b->GetNumber() == "4" ) foundPad4Error = true;
                    else if( pad_b && pad_b->GetNumber() == "6" ) foundPad6Error = true;
                    else if( trk_a && trk_a->m_Uuid == arc8 )     foundArc8Error = true;
                    else if( trk_a && trk_a->m_Uuid == arc12 )    foundArc12Error = true;
                    else if( trk_b && trk_b->m_Uuid == arc8 )     foundArc8Error = true;
                    else if( trk_b && trk_b->m_Uuid == arc12 )    foundArc12Error = true;
                    else                                          foundOtherError = true;

                }
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_CHECK_EQUAL( foundPad2Error, true );
    BOOST_CHECK_EQUAL( foundPad4Error, true );
    BOOST_CHECK_EQUAL( foundPad6Error, true );
    BOOST_CHECK_EQUAL( foundArc8Error, true );
    BOOST_CHECK_EQUAL( foundArc12Error, true );
    BOOST_CHECK_EQUAL( foundOtherError, false );
}


BOOST_FIXTURE_TEST_CASE( NotchedZones, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "notched_zones", m_board );

    // Older algorithms had trouble where the filleted zones intersected and left notches.
    // See:
    //   https://gitlab.com/kicad/code/kicad/-/issues/2737
    //   https://gitlab.com/kicad/code/kicad/-/issues/2752
    SHAPE_POLY_SET frontCopper;

    KI_TEST::FillZones( m_board.get() );

    frontCopper = SHAPE_POLY_SET();

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->GetLayerSet().Contains( F_Cu ) )
        {
            frontCopper.BooleanAdd( *zone->GetFilledPolysList( F_Cu ) );
        }
    }

    BOOST_CHECK_EQUAL( frontCopper.OutlineCount(), 2 );
}


static const std::vector<wxString> RegressionZoneFillTests_tests = {
    "issue18",
    "issue2568",
    "issue3812",
    "issue5102",
    "issue5313",
    "issue5320",
    "issue5567",
    "issue5830",
    "issue6039",
    "issue6260",
    "issue6284",
    "issue7086",
    "issue14294",   // Bad Clipper2 fill
    "fill_bad"      // Missing zone clearance expansion
};


BOOST_DATA_TEST_CASE_F( ZONE_FILL_TEST_FIXTURE, RegressionZoneFillTests,
                        boost::unit_test::data::make( RegressionZoneFillTests_tests ), relPath )
{
    KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    KI_TEST::FillZones( m_board.get() );

    std::vector<DRC_ITEM> violations;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_CLEARANCE )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    if( violations.empty() )
    {
        BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
        BOOST_TEST_MESSAGE( wxString::Format( "Zone fill regression: %s passed", relPath ) );
    }
    else
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const DRC_ITEM& item : violations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );

        BOOST_ERROR( wxString::Format( "Zone fill regression: %s failed", relPath ) );
    }
}


/**
 * Test for issue 23053: Zone clearance violations between zones with iterative refill.
 *
 * When iterative refill is enabled, zone-to-zone clearance knockouts were applied after
 * the min-width deflate/inflate cycle. The reinflation could push copper into the zone
 * clearance area, causing DRC clearance violations between different-net zones.
 *
 * Also verifies the non-iterative path produces no violations on the same board.
 */
BOOST_FIXTURE_TEST_CASE( RegressionZoneClearanceWithIterativeRefill, ZONE_FILL_TEST_FIXTURE )
{
    ADVANCED_CFG& cfg = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    bool originalIterativeRefill = cfg.m_ZoneFillIterativeRefill;

    struct ScopeGuard { bool& ref; bool orig; ~ScopeGuard() { ref = orig; } }
        guard{ cfg.m_ZoneFillIterativeRefill, originalIterativeRefill };

    auto runDrcClearanceCheck =
            [this]( bool aIterative ) -> int
            {
                ADVANCED_CFG& innerCfg = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
                innerCfg.m_ZoneFillIterativeRefill = aIterative;

                KI_TEST::LoadBoard( m_settingsManager, "issue23053/issue23053", m_board );

                BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

                KI_TEST::FillZones( m_board.get() );

                std::vector<DRC_ITEM> violations;

                std::map<KIID, EDA_ITEM*> itemMap;
                m_board->FillItemMap( itemMap );
                UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MM );

                bds.m_DRCEngine->SetViolationHandler(
                        [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos,
                             int aLayer,
                             const std::function<void( PCB_MARKER* )>& aPathGenerator )
                        {
                            if( aItem->GetErrorCode() == DRCE_CLEARANCE )
                            {
                                BOARD_ITEM* itemA = m_board->ResolveItem( aItem->GetMainItemID() );
                                BOARD_ITEM* itemB = m_board->ResolveItem( aItem->GetAuxItemID() );

                                if( dynamic_cast<ZONE*>( itemA ) && dynamic_cast<ZONE*>( itemB ) )
                                {
                                    violations.push_back( *aItem );

                                    BOOST_TEST_MESSAGE(
                                            aItem->ShowReport( &unitsProvider,
                                                               RPT_SEVERITY_ERROR, itemMap ) );
                                }
                            }
                        } );

                bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

                return static_cast<int>( violations.size() );
            };

    int iterativeViolations = runDrcClearanceCheck( true );

    BOOST_CHECK_MESSAGE( iterativeViolations == 0,
                         wxString::Format( "Iterative refill produced %d zone-to-zone clearance "
                                           "violations (expected 0)", iterativeViolations ) );

    int nonIterativeViolations = runDrcClearanceCheck( false );

    BOOST_CHECK_MESSAGE( nonIterativeViolations == 0,
                         wxString::Format( "Non-iterative refill produced %d zone-to-zone clearance "
                                           "violations (expected 0)", nonIterativeViolations ) );
}


static const std::vector<wxString> RegressionSliverZoneFillTests_tests = {
    "issue16182"    // Slivers
};


BOOST_DATA_TEST_CASE_F( ZONE_FILL_TEST_FIXTURE, RegressionSliverZoneFillTests,
                        boost::unit_test::data::make( RegressionSliverZoneFillTests_tests ),
                        relPath )
{
    KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    KI_TEST::FillZones( m_board.get() );

    std::vector<DRC_ITEM> violations;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_COPPER_SLIVER )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    if( violations.empty() )
    {
        BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
        BOOST_TEST_MESSAGE( wxString::Format( "Zone fill copper sliver regression: %s passed", relPath ) );
    }
    else
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const DRC_ITEM& item : violations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );

        BOOST_ERROR( wxString::Format( "Zone fill copper sliver regression: %s failed", relPath ) );
    }
}


static const std::vector<std::pair<wxString,int>> RegressionTeardropFill_tests = {
        { "teardrop_issue_JPC2", 5 },    // Arcs with teardrops connecting to pads
};


BOOST_DATA_TEST_CASE_F( ZONE_FILL_TEST_FIXTURE, RegressionTeardropFill,
                        boost::unit_test::data::make( RegressionTeardropFill_tests ), test )
{
    const wxString& relPath = test.first;
    const int count = test.second;

    KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    KI_TEST::FillZones( m_board.get() );

    int zoneCount = 0;

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->IsTeardropArea() )
            zoneCount++;
    }

    BOOST_CHECK_MESSAGE( zoneCount == count, "Expected " << count << " teardrop zones in "
                                                            << relPath << ", found "
                                                            << zoneCount );
}


BOOST_FIXTURE_TEST_CASE( RegressionNetTie, ZONE_FILL_TEST_FIXTURE )
{

    std::vector<wxString> tests = { { "issue19956/issue19956" }    // Arcs with teardrops connecting to pads
                                };

    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
        KI_TEST::FillZones( m_board.get() );

        for( ZONE* zone : m_board->Zones() )
        {
            for( PCB_LAYER_ID layer : zone->GetLayerSet() )
            {
                std::shared_ptr<SHAPE> a_shape( zone->GetEffectiveShape( layer ) );

                for( PAD* pad : m_board->GetPads() )
                {
                    std::shared_ptr<SHAPE> pad_shape( pad->GetEffectiveShape( layer ) );
                    int                    clearance = pad_shape->GetClearance( a_shape.get() );
                    BOOST_CHECK_MESSAGE( pad->GetNetCode() == zone->GetNetCode() || clearance != 0,
                                         wxString::Format( "Pad %s from Footprint %s has net code %s and "
                                                           "is connected to zone with net code %s",
                                                           pad->GetNumber(),
                                                           pad->GetParentFootprint()->GetReferenceAsString(),
                                                           pad->GetNetname(),
                                                           zone->GetNetname() ) );
                }
            }
        }
    }
}


/**
 * Test for issue 21746: Lower priority zones should fill areas where higher priority
 * zones have isolated islands that get removed.
 *
 * The test board has:
 * - A VDD zone with priority 1 (higher priority)
 * - A GND zone with priority 0 (lower priority, default)
 *
 * The VDD zone only connects to a small area, creating an isolated island that should
 * be removed. The GND zone should then fill that area.
 *
 * With the bug, GND is knocked out by VDD before VDD's isolated island is removed,
 * leaving GND mostly empty.
 */
BOOST_FIXTURE_TEST_CASE( RegressionZonePriorityIsolatedIslands, ZONE_FILL_TEST_FIXTURE )
{
    // Enable iterative refill to fix issue 21746
    ADVANCED_CFG& cfg = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    bool originalIterativeRefill = cfg.m_ZoneFillIterativeRefill;
    cfg.m_ZoneFillIterativeRefill = true;

    // Restore config at end of scope to avoid polluting other tests
    struct ScopeGuard { bool& ref; bool orig; ~ScopeGuard() { ref = orig; } } guard{ cfg.m_ZoneFillIterativeRefill, originalIterativeRefill };

    KI_TEST::LoadBoard( m_settingsManager, "issue21746/issue21746", m_board );

    KI_TEST::FillZones( m_board.get() );

    // Find the GND zone
    ZONE* gndZone = nullptr;

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->GetNetname() == "GND" )
        {
            gndZone = zone;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( gndZone != nullptr, "GND zone not found in test board" );

    // Calculate board outline area
    SHAPE_POLY_SET boardOutline;
    bool hasOutline = m_board->GetBoardPolygonOutlines( boardOutline, true );
    BOOST_REQUIRE_MESSAGE( hasOutline, "Board outline not found" );

    double boardArea = 0.0;

    for( int i = 0; i < boardOutline.OutlineCount(); i++ )
        boardArea += boardOutline.Outline( i ).Area();

    // Get GND zone filled area
    gndZone->CalculateFilledArea();
    double gndFilledArea = gndZone->GetFilledArea();

    // The GND zone should fill at least 25% of the board area
    // With the bug, it fills almost nothing because VDD knocks it out
    double fillRatio = gndFilledArea / boardArea;

    BOOST_TEST_MESSAGE( wxString::Format( "Board area: %.2f sq mm, GND filled area: %.2f sq mm, "
                                          "Fill ratio: %.1f%%",
                                          boardArea / 1e6, gndFilledArea / 1e6,
                                          fillRatio * 100.0 ) );

    BOOST_CHECK_MESSAGE( fillRatio >= 0.25,
                         wxString::Format( "GND zone fill ratio %.1f%% is less than expected 25%%. "
                                           "This indicates issue 21746 - lower priority zones not "
                                           "filling areas where higher priority isolated islands "
                                           "were removed.",
                                           fillRatio * 100.0 ) );
}


/**
 * Test for issue 22010: Via annular rings should not appear on zone layers when the
 * zone fill doesn't actually reach the via.
 *
 * The test board has:
 * - A GND zone on In1.Cu and In2.Cu with large clearance
 * - GND vias with "remove unused layers" and "keep start/end layers" enabled
 * - Tracks that block the zone fill from reaching some vias
 *
 * Before the fix, vias within the zone outline would flash even if the zone fill
 * didn't reach them due to obstacles (tracks). This caused false DRC violations
 * for clearances and blocked dense routing near the vias.
 */
BOOST_FIXTURE_TEST_CASE( RegressionViaFlashingUnreachableZone, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue22010/issue22010", m_board );

    KI_TEST::FillZones( m_board.get() );

    // Find vias with zone_layer_connections set for In1.Cu or In2.Cu
    // After filling, vias that the zone doesn't actually reach should NOT be flashed
    int viasWithUnreachableFlashing = 0;
    int totalConditionalVias = 0;

    PCB_LAYER_ID in1Cu = m_board->GetLayerID( wxT( "In1.Cu" ) );
    PCB_LAYER_ID in2Cu = m_board->GetLayerID( wxT( "In2.Cu" ) );

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );

        if( !via->GetRemoveUnconnected() )
            continue;

        totalConditionalVias++;

        // Check if via is flashed on In1.Cu or In2.Cu
        bool flashedOnIn1 = via->FlashLayer( in1Cu );
        bool flashedOnIn2 = via->FlashLayer( in2Cu );

        if( !flashedOnIn1 && !flashedOnIn2 )
            continue;

        VECTOR2I viaCenter = via->GetPosition();
        int      holeRadius = via->GetDrillValue() / 2;

        // Check if any zone fill actually reaches this via
        bool zoneReachesVia = false;

        for( ZONE* zone : m_board->Zones() )
        {
            if( zone->GetIsRuleArea() )
                continue;

            if( zone->GetNetCode() != via->GetNetCode() )
                continue;

            for( PCB_LAYER_ID layer : { in1Cu, in2Cu } )
            {
                if( !zone->IsOnLayer( layer ) )
                    continue;

                if( !zone->HasFilledPolysForLayer( layer ) )
                    continue;

                const std::shared_ptr<SHAPE_POLY_SET>& fill = zone->GetFilledPolysList( layer );

                if( fill->Contains( viaCenter, -1, holeRadius ) )
                {
                    zoneReachesVia = true;
                    break;
                }
            }

            if( zoneReachesVia )
                break;
        }

        // If via is flashed but zone doesn't reach it, that's the bug
        if( !zoneReachesVia && ( flashedOnIn1 || flashedOnIn2 ) )
            viasWithUnreachableFlashing++;
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Total conditional vias: %d, Vias with unreachable "
                                          "flashing: %d", totalConditionalVias,
                                          viasWithUnreachableFlashing ) );

    BOOST_CHECK_MESSAGE( viasWithUnreachableFlashing == 0,
                         wxString::Format( "Found %d vias flashed on zone layers where the zone "
                                           "fill doesn't actually reach them. This indicates "
                                           "issue 22010 is not fixed.",
                                           viasWithUnreachableFlashing ) );
}


/**
 * Test for issue 12964: Vias with remove_unused_layers should not flash on layers where
 * they would short to a zone with a different net.
 *
 * The test board has:
 * - GND vias (net 1) with remove_unused_layers enabled
 * - A +3.3V zone (net 2) on F.Cu that the vias pass through
 * - A GND zone (net 1) on F.Cu
 *
 * After zone fill, the vias should NOT flash on F.Cu in areas covered by the +3.3V zone,
 * as that would short GND to +3.3V.
 */
BOOST_FIXTURE_TEST_CASE( RegressionViaZoneNetShort, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue12964/issue12964", m_board );

    KI_TEST::FillZones( m_board.get() );

    int viasShortingZones = 0;
    int totalConditionalVias = 0;

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );

        if( !via->GetRemoveUnconnected() )
            continue;

        totalConditionalVias++;

        VECTOR2I viaCenter = via->GetPosition();

        for( ZONE* zone : m_board->Zones() )
        {
            if( zone->GetIsRuleArea() )
                continue;

            if( zone->GetNetCode() == via->GetNetCode() )
                continue;

            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            {
                if( !via->FlashLayer( layer ) )
                    continue;

                if( !zone->HasFilledPolysForLayer( layer ) )
                    continue;

                const std::shared_ptr<SHAPE_POLY_SET>& fill = zone->GetFilledPolysList( layer );
                int viaRadius = via->GetWidth( layer ) / 2;

                if( fill->Contains( viaCenter, -1, viaRadius ) )
                {
                    BOOST_TEST_MESSAGE( wxString::Format(
                            "Via at (%d, %d) on net %s is flashing on layer %s where zone "
                            "net %s is filled - this creates a short!",
                            viaCenter.x, viaCenter.y, via->GetNetname(),
                            m_board->GetLayerName( layer ), zone->GetNetname() ) );
                    viasShortingZones++;
                }
            }
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Total conditional vias: %d, Vias shorting zones: %d",
                                          totalConditionalVias, viasShortingZones ) );

    BOOST_CHECK_MESSAGE( viasShortingZones == 0,
                         wxString::Format( "Found %d vias flashed on layers where they short to "
                                           "zones with different nets. This indicates issue 12964 "
                                           "is not fixed.",
                                           viasShortingZones ) );
}


/**
 * Test that hatch zone thermal reliefs maintain connectivity even when the hatch gap
 * is larger than the thermal ring diameter.
 *
 * The test board has:
 * - A hatch zone with large gap (6mm) and thermal relief settings
 * - A pad with thermal relief connection to the zone
 * - A track that should be connected to the pad via the zone
 *
 * Without the fix, the thermal ring around the pad could be entirely inside a hatch hole,
 * leaving it electrically isolated from the zone webbing.
 */
BOOST_FIXTURE_TEST_CASE( HatchZoneThermalConnectivity, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "hatch_thermal_connectivity/hatch_thermal_connectivity",
                        m_board );

    KI_TEST::FillZones( m_board.get() );

    m_board->BuildConnectivity();

    int unconnectedCount = m_board->GetConnectivity()->GetUnconnectedCount( false );

    BOOST_CHECK_MESSAGE( unconnectedCount == 0,
                         wxString::Format( "Found %d unconnected items after zone fill. "
                                           "Hatch zone thermal reliefs should maintain connectivity "
                                           "even with large hatch gaps.",
                                           unconnectedCount ) );
}


/**
 * Test for issue 22475: Zone fill should not produce artifacts when tracks contain
 * shallow-radius arcs (where the mid-point is nearly collinear with start and end).
 *
 * The test board has:
 * - A GND zone on In1.Cu
 * - PCB_ARC tracks with very shallow radii causing mid-points to be within 250µm of
 *   the start-end line
 *
 * Before the fix, these shallow arcs caused TransformArcToPolygon to compute extremely
 * large radii, leading to integer overflow and invalid clearance hole geometry. This
 * resulted in phantom voids and disconnected fill areas in the zone.
 *
 * The fix treats arcs with mid-point distance <= 250µm from the chord as straight
 * line segments, avoiding numerical instability.
 */
BOOST_FIXTURE_TEST_CASE( RegressionShallowArcZoneFill, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue22475/issue22475", m_board );

    PCB_LAYER_ID in1Cu = m_board->GetLayerID( wxT( "In1.Cu" ) );

    ZONE* gndZone = nullptr;

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->GetNetname() == "GND" && zone->IsOnLayer( in1Cu ) )
        {
            gndZone = zone;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( gndZone != nullptr, "GND zone on In1.Cu not found in test board" );

    if( !gndZone )
        return;

    KI_TEST::FillZones( m_board.get() );

    BOOST_REQUIRE_MESSAGE( gndZone->HasFilledPolysForLayer( in1Cu ),
                           "GND zone has no fill on In1.Cu" );

    const std::shared_ptr<SHAPE_POLY_SET>& fill = gndZone->GetFilledPolysList( in1Cu );

    // The zone fill should produce a single contiguous outline. Multiple outlines
    // indicate disconnected fill areas caused by malformed clearance holes.
    BOOST_CHECK_EQUAL( fill->OutlineCount(), 1 );

    double zoneOutlineArea = gndZone->Outline()->Area();

    BOOST_REQUIRE_MESSAGE( zoneOutlineArea > 0.0, "Zone outline area must be positive" );

    double fillArea = 0.0;

    for( int i = 0; i < fill->OutlineCount(); i++ )
        fillArea += std::abs( fill->Outline( i ).Area() );

    double fillRatio = fillArea / zoneOutlineArea;

    // The zone should be mostly filled. A low fill ratio indicates excessive voids
    // from malformed clearance holes around shallow arcs.
    BOOST_CHECK_GE( fillRatio, 0.90 );
}


/**
 * Test for issue 22809: Zone keepouts should be respected by iterative refiller.
 *
 * The test board has:
 * - A net zone that covers most of the board
 * - Several zone keepouts (rule areas with copperpour not_allowed)
 *
 * Before the fix, when iterative refill was enabled (ADVANCED_CFG::m_ZoneFillIterativeRefill),
 * zone keepouts were being completely ignored because the code path that handled keepouts
 * (buildCopperItemClearances with aIncludeZoneClearances=true) was skipped when iterative
 * refill was enabled. Additionally, refillZoneFromCache did not subtract keepouts.
 */
BOOST_FIXTURE_TEST_CASE( RegressionIterativeRefillRespectsKeepouts, ZONE_FILL_TEST_FIXTURE )
{
    // Enable iterative refill
    ADVANCED_CFG& cfg = const_cast<ADVANCED_CFG&>( ADVANCED_CFG::GetCfg() );
    bool originalIterativeRefill = cfg.m_ZoneFillIterativeRefill;
    cfg.m_ZoneFillIterativeRefill = true;

    struct ScopeGuard { bool& ref; bool orig; ~ScopeGuard() { ref = orig; } }
        guard{ cfg.m_ZoneFillIterativeRefill, originalIterativeRefill };

    KI_TEST::LoadBoard( m_settingsManager, "issue22809/issue22809", m_board );

    KI_TEST::FillZones( m_board.get() );

    // Find all zone keepouts
    std::vector<ZONE*> keepouts;

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->GetIsRuleArea() && zone->GetDoNotAllowZoneFills() )
            keepouts.push_back( zone );
    }

    BOOST_REQUIRE_MESSAGE( !keepouts.empty(), "No zone keepouts found in test board" );

    // For each keepout, check that no zone fill exists inside it
    int violationCount = 0;

    for( ZONE* keepout : keepouts )
    {
        for( PCB_LAYER_ID layer : keepout->GetLayerSet().Seq() )
        {
            SHAPE_POLY_SET keepoutOutline( *keepout->Outline() );
            keepoutOutline.ClearArcs();

            for( ZONE* zone : m_board->Zones() )
            {
                if( zone->GetIsRuleArea() )
                    continue;

                if( !zone->IsOnLayer( layer ) )
                    continue;

                if( !zone->HasFilledPolysForLayer( layer ) )
                    continue;

                const std::shared_ptr<SHAPE_POLY_SET>& fill = zone->GetFilledPolysList( layer );

                // Check if any fill intersects the keepout
                SHAPE_POLY_SET intersection = *fill;
                intersection.BooleanIntersection( keepoutOutline );

                if( intersection.OutlineCount() > 0 )
                {
                    double intersectionArea = 0;

                    for( int i = 0; i < intersection.OutlineCount(); i++ )
                        intersectionArea += std::abs( intersection.Outline( i ).Area() );

                    // Allow for small numerical errors (less than 1 square mm)
                    if( intersectionArea > 1e6 )
                    {
                        BOOST_TEST_MESSAGE( wxString::Format(
                                "Zone %s fill on layer %s overlaps keepout by %.2f sq mm",
                                zone->GetNetname(),
                                m_board->GetLayerName( layer ),
                                intersectionArea / 1e6 ) );
                        violationCount++;
                    }
                }
            }
        }
    }

    BOOST_CHECK_MESSAGE( violationCount == 0,
                         wxString::Format( "Found %d zone fills overlapping keepout areas. "
                                           "This indicates issue 22809 - iterative refiller "
                                           "ignores zone keepouts.", violationCount ) );
}


/**
 * Test for issue 22826: TH pads with remove_unused_layers should properly flash on inner
 * layers when inside a zone of the same net.
 *
 * The test board has:
 * - TH pads with remove_unused_layers enabled, on the VBUS_DUT net
 * - A VBUS_DUT zone on In2.Cu
 * - Pads that are within the zone boundary
 *
 * Before the fix, the zone filler checked if fill->Contains(pad_center) without tolerance,
 * which would fail because the fill has a thermal relief cutout around the pad. The fix
 * uses the pad's drill radius as tolerance, similar to how vias are handled.
 */
BOOST_FIXTURE_TEST_CASE( RegressionTHPadInnerLayerFlashing, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue22826/issue22826", m_board );

    KI_TEST::FillZones( m_board.get() );

    PCB_LAYER_ID in2Cu = m_board->GetLayerID( wxT( "In2.Cu" ) );
    int padsWithMissingFlashing = 0;
    int totalConditionalPads = 0;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( !pad->GetRemoveUnconnected() )
                continue;

            if( !pad->HasHole() )
                continue;

            if( pad->GetNetname() != "VBUS_DUT" && pad->GetNetname() != "VBUS_DBG" )
                continue;

            totalConditionalPads++;

            // Check if the pad should flash on In2.Cu
            bool shouldFlash = false;

            for( ZONE* zone : m_board->Zones() )
            {
                if( zone->GetIsRuleArea() )
                    continue;

                if( zone->GetNetCode() != pad->GetNetCode() )
                    continue;

                if( !zone->IsOnLayer( in2Cu ) )
                    continue;

                if( zone->Outline()->Contains( pad->GetPosition() ) )
                {
                    shouldFlash = true;
                    break;
                }
            }

            if( shouldFlash && !pad->FlashLayer( in2Cu ) )
            {
                BOOST_TEST_MESSAGE( wxString::Format(
                        "Pad %s at (%d, %d) on net %s is inside zone but not flashing on In2.Cu",
                        pad->GetNumber(), pad->GetPosition().x, pad->GetPosition().y,
                        pad->GetNetname() ) );
                padsWithMissingFlashing++;
            }
        }
    }

    BOOST_TEST_MESSAGE( wxString::Format( "Total conditional pads: %d, Pads with missing "
                                          "flashing: %d", totalConditionalPads,
                                          padsWithMissingFlashing ) );

    BOOST_CHECK_MESSAGE( padsWithMissingFlashing == 0,
                         wxString::Format( "Found %d TH pads that should flash on inner layers "
                                           "but don't. This indicates issue 22826 is not fixed.",
                                           padsWithMissingFlashing ) );
}


/**
 * Test for issue 19405: Rounded teardrop geometry should not create concave shapes
 * when connecting to rounded rectangle pads at corners.
 *
 * The test board has a rounded rectangle pad with a track connecting to its short side
 * (hitting the corner radius). With curved teardrops enabled, the teardrop should not
 * intersect the pad's corner radius, which would create sharp inside corners.
 */
BOOST_FIXTURE_TEST_CASE( RegressionRoundRectTeardropGeometry, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue19405_roundrect_teardrop", m_board );

    // Set up tool manager for teardrop generation
    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, nullptr );

    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    toolMgr.RegisterTool( dummyTool );

    // Generate teardrops
    BOARD_COMMIT commit( dummyTool );
    TEARDROP_MANAGER teardropMgr( m_board.get(), &toolMgr );
    teardropMgr.UpdateTeardrops( commit, nullptr, nullptr, true );

    if( !commit.Empty() )
        commit.Push( _( "Add teardrops" ), SKIP_UNDO | SKIP_SET_DIRTY );

    // Find teardrop zones
    int teardropCount = 0;
    bool foundBadTeardrop = false;

    for( ZONE* zone : m_board->Zones() )
    {
        if( !zone->IsTeardropArea() )
            continue;

        teardropCount++;

        // Get the teardrop outline
        const SHAPE_POLY_SET* outline = zone->Outline();

        if( !outline || outline->OutlineCount() == 0 )
            continue;

        const SHAPE_LINE_CHAIN& chain = outline->Outline( 0 );

        // Check that the teardrop polygon is convex or at least doesn't have
        // any sharp concave angles that would indicate intersection with the pad corner.
        // A well-formed teardrop should have all turns in the same direction
        // (or very close to it) except at the pad anchor points.
        int concaveCount = 0;

        for( int i = 0; i < chain.PointCount(); i++ )
        {
            int prev = ( i == 0 ) ? chain.PointCount() - 1 : i - 1;
            int next = ( i + 1 ) % chain.PointCount();

            VECTOR2I v1 = chain.CPoint( i ) - chain.CPoint( prev );
            VECTOR2I v2 = chain.CPoint( next ) - chain.CPoint( i );

            // Cross product gives handedness of turn
            int64_t cross = (int64_t) v1.x * v2.y - (int64_t) v1.y * v2.x;

            // Count significant concave turns (negative cross product for CCW polygons)
            // Small values are numerical noise
            if( cross < -1000 )
                concaveCount++;
        }

        // A teardrop should have at most 2-3 concave points (at the pad anchor points)
        // Many concave points indicate the curve is intersecting the pad corner
        if( concaveCount > 5 )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "Teardrop has %d concave vertices, "
                                                   "indicating possible corner intersection",
                                                   concaveCount ) );
            foundBadTeardrop = true;
        }
    }

    BOOST_CHECK_MESSAGE( teardropCount > 0, "Expected at least one teardrop zone" );

    BOOST_CHECK_MESSAGE( !foundBadTeardrop,
                         "Found teardrop with excessive concave vertices, indicating "
                         "issue 19405 - teardrop curve intersecting rounded rectangle corner" );
}


/**
 * Test that teardrops connecting to oval pads at their curved ends have proper tangent curves.
 *
 * Oval pads have semicircular ends. When a track connects to the curved end, the teardrop
 * curve should be tangent to the semicircle, similar to how rounded rectangle corners are
 * handled.
 */
BOOST_FIXTURE_TEST_CASE( OvalPadTeardropGeometry, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "oval_teardrop", m_board );

    // Set up tool manager for teardrop generation
    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, nullptr );

    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    toolMgr.RegisterTool( dummyTool );

    // Generate teardrops
    BOARD_COMMIT commit( dummyTool );
    TEARDROP_MANAGER teardropMgr( m_board.get(), &toolMgr );
    teardropMgr.UpdateTeardrops( commit, nullptr, nullptr, true );

    if( !commit.Empty() )
        commit.Push( _( "Add teardrops" ), SKIP_UNDO | SKIP_SET_DIRTY );

    // Find teardrop zones
    int teardropCount = 0;
    bool foundBadTeardrop = false;

    for( ZONE* zone : m_board->Zones() )
    {
        if( !zone->IsTeardropArea() )
            continue;

        teardropCount++;

        const SHAPE_POLY_SET* outline = zone->Outline();

        if( !outline || outline->OutlineCount() == 0 )
            continue;

        const SHAPE_LINE_CHAIN& chain = outline->Outline( 0 );

        // Check for excessive concave vertices that would indicate the teardrop curve
        // is not tangent to the oval's semicircular end
        int concaveCount = 0;

        for( int i = 0; i < chain.PointCount(); i++ )
        {
            int prev = ( i == 0 ) ? chain.PointCount() - 1 : i - 1;
            int next = ( i + 1 ) % chain.PointCount();

            VECTOR2I v1 = chain.CPoint( i ) - chain.CPoint( prev );
            VECTOR2I v2 = chain.CPoint( next ) - chain.CPoint( i );

            int64_t cross = (int64_t) v1.x * v2.y - (int64_t) v1.y * v2.x;

            if( cross < -1000 )
                concaveCount++;
        }

        if( concaveCount > 5 )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "Oval teardrop has %d concave vertices",
                                                   concaveCount ) );
            foundBadTeardrop = true;
        }
    }

    BOOST_CHECK_MESSAGE( teardropCount > 0, "Expected at least one teardrop zone" );

    BOOST_CHECK_MESSAGE( !foundBadTeardrop,
                         "Found teardrop with excessive concave vertices on oval pad, "
                         "indicating curve is not tangent to semicircular end" );
}


/**
 * Test that teardrops connecting to large circular pads maintain proper tangent contact.
 *
 * When a circle is larger than the configured teardrop max width, the anchor points should
 * still be on the actual circle edge (not on a clipped boundary) to ensure the teardrop
 * curve is tangent to the circle.
 */
BOOST_FIXTURE_TEST_CASE( LargeCircleTeardropGeometry, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "large_circle_teardrop", m_board );

    // Set up tool manager for teardrop generation
    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, nullptr );

    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    toolMgr.RegisterTool( dummyTool );

    // Generate teardrops
    BOARD_COMMIT commit( dummyTool );
    TEARDROP_MANAGER teardropMgr( m_board.get(), &toolMgr );
    teardropMgr.UpdateTeardrops( commit, nullptr, nullptr, true );

    if( !commit.Empty() )
        commit.Push( _( "Add teardrops" ), SKIP_UNDO | SKIP_SET_DIRTY );

    // Find the pad and its teardrop
    PAD* largePad = nullptr;

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            if( pad->GetShape( F_Cu ) == PAD_SHAPE::CIRCLE )
            {
                largePad = pad;
                break;
            }
        }
    }

    BOOST_REQUIRE_MESSAGE( largePad != nullptr, "Expected a circular pad in test board" );

    int padRadius = largePad->GetSize( F_Cu ).x / 2;
    VECTOR2I padCenter = largePad->GetPosition();

    // Find teardrop zones
    int teardropCount = 0;
    bool foundBadTeardrop = false;

    for( ZONE* zone : m_board->Zones() )
    {
        if( !zone->IsTeardropArea() )
            continue;

        teardropCount++;

        const SHAPE_POLY_SET* outline = zone->Outline();

        if( !outline || outline->OutlineCount() == 0 )
            continue;

        const SHAPE_LINE_CHAIN& chain = outline->Outline( 0 );

        // Check for excessive concave vertices
        int concaveCount = 0;

        for( int i = 0; i < chain.PointCount(); i++ )
        {
            int prev = ( i == 0 ) ? chain.PointCount() - 1 : i - 1;
            int next = ( i + 1 ) % chain.PointCount();

            VECTOR2I v1 = chain.CPoint( i ) - chain.CPoint( prev );
            VECTOR2I v2 = chain.CPoint( next ) - chain.CPoint( i );

            int64_t cross = (int64_t) v1.x * v2.y - (int64_t) v1.y * v2.x;

            if( cross < -1000 )
                concaveCount++;
        }

        if( concaveCount > 5 )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "Large circle teardrop has %d concave vertices",
                                                   concaveCount ) );
            foundBadTeardrop = true;
        }

        // Also verify that the teardrop anchor points near the pad are approximately
        // on the circle edge (within tolerance)
        int maxError = m_board->GetDesignSettings().m_MaxError;

        for( int i = 0; i < chain.PointCount(); i++ )
        {
            VECTOR2I pt = chain.CPoint( i );
            double dist = ( pt - padCenter ).EuclideanNorm();

            // Points that are close to the circle should be approximately on it
            if( dist > padRadius * 0.5 && dist < padRadius * 1.5 )
            {
                double deviation = std::abs( dist - padRadius );

                // Allow some tolerance for polygon approximation
                if( deviation > maxError * 5 && deviation < padRadius * 0.2 )
                {
                    BOOST_TEST_MESSAGE( wxString::Format(
                            "Teardrop point at distance %.2f from pad center (radius %.2f), "
                            "deviation %.2f exceeds tolerance",
                            dist / 1000.0, padRadius / 1000.0, deviation / 1000.0 ) );
                }
            }
        }
    }

    BOOST_CHECK_MESSAGE( teardropCount > 0, "Expected at least one teardrop zone" );

    BOOST_CHECK_MESSAGE( !foundBadTeardrop,
                         "Found teardrop with excessive concave vertices on large circle, "
                         "indicating anchor points may not be on circle edge" );
}


/**
 * Test for issue 23123: Coincident pads from different footprints with different nets
 * must each get proper zone treatment.
 *
 * When two pads occupy the same position with the same geometry but different nets, the
 * zone filler's deduplication must not skip the second pad. A pad whose net differs from
 * the zone needs clearance; a pad matching the zone net gets thermal relief. If the
 * deduplication key omits the net code, the second pad is silently dropped and no
 * clearance is created.
 */
BOOST_FIXTURE_TEST_CASE( RegressionCoincidentPadClearance, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue23123_minimal", m_board );

    KI_TEST::FillZones( m_board.get() );

    // After filling, every pad whose net differs from the zone must have clearance.
    // Check each zone/pad combination on each shared layer.
    int violations = 0;

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->GetIsRuleArea() )
            continue;

        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            if( !zone->HasFilledPolysForLayer( layer ) )
                continue;

            const std::shared_ptr<SHAPE_POLY_SET>& fill = zone->GetFilledPolysList( layer );

            for( PAD* pad : m_board->GetPads() )
            {
                if( !pad->IsOnLayer( layer ) )
                    continue;

                if( pad->GetNetCode() == zone->GetNetCode() )
                    continue;

                std::shared_ptr<SHAPE> padShape = pad->GetEffectiveShape( layer );
                int clearance = padShape->GetClearance( fill.get() );

                if( clearance < 1 )
                {
                    BOOST_TEST_MESSAGE( wxString::Format(
                            "Pad %s (net %s) at (%d, %d) has zero clearance to zone %s "
                            "on layer %s",
                            pad->GetNumber(), pad->GetNetname(),
                            pad->GetPosition().x, pad->GetPosition().y,
                            zone->GetNetname(), m_board->GetLayerName( layer ) ) );
                    violations++;
                }
            }
        }
    }

    BOOST_CHECK_MESSAGE( violations == 0,
                         wxString::Format( "Found %d pads with missing zone clearance. "
                                           "Coincident pads with different nets must not be "
                                           "deduplicated in zone fill knockout.",
                                           violations ) );
}


/**
 * Verify zone fills clear PTH pads on different nets.
 *
 * After fill, DRC must report zero zone-to-pad clearance violations.  Clipper2
 * rounds corridor-cut vertices to integer coordinates; they can land within 1nm
 * of a segment endpoint but are not true pinch points.  Exact collinearity
 * detection keeps them from triggering splits that extend triangles into knockout
 * areas.
 */
BOOST_FIXTURE_TEST_CASE( ZoneViaNetClearance, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "connect/connect", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    KI_TEST::FillZones( m_board.get() );

    std::vector<DRC_ITEM> violations;

    bds.m_DRCEngine->InitEngine( wxFileName() );

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( aItem->GetErrorCode() == DRCE_CLEARANCE )
                {
                    BOARD_ITEM* item_a = m_board->ResolveItem( aItem->GetMainItemID() );
                    BOARD_ITEM* item_b = m_board->ResolveItem( aItem->GetAuxItemID() );

                    ZONE* zone_a = dynamic_cast<ZONE*>( item_a );
                    ZONE* zone_b = dynamic_cast<ZONE*>( item_b );

                    if( zone_a || zone_b )
                        violations.push_back( *aItem );
                }
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_CHECK_EQUAL( violations.size(), 0 );
}
