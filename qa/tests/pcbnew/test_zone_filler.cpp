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
#include <footprint.h>
#include <zone.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>


struct ZONE_FILL_TEST_FIXTURE
{
    ZONE_FILL_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


constexpr int delta = KiROUND( 0.006 * pcbIUScale.IU_PER_MM );


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
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, VECTOR2I aPos, int aLayer,
                 DRC_CUSTOM_MARKER_HANDLER* aCustomHandler )
            {
                if( aItem->GetErrorCode() == DRCE_CLEARANCE )
                {
                    BOARD_ITEM* item_a = m_board->GetItem( aItem->GetMainItemID() );
                    PAD*        pad_a = dynamic_cast<PAD*>( item_a );
                    PCB_TRACK*  trk_a = dynamic_cast<PCB_TRACK*>( item_a );

                    BOARD_ITEM* item_b = m_board->GetItem( aItem->GetAuxItemID() );
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
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, VECTOR2I aPos, int aLayer,
                 DRC_CUSTOM_MARKER_HANDLER* aCustomHandler )
            {
                if( aItem->GetErrorCode() == DRCE_CLEARANCE )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    if( violations.empty() )
    {
        BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
        BOOST_TEST_MESSAGE( (const char*)(wxString::Format( "Zone fill regression: %s passed", relPath ).utf8_str()) );
    }
    else
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const DRC_ITEM& item : violations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );

        BOOST_ERROR( (const char*)(wxString::Format( "Zone fill regression: %s failed", relPath ).utf8_str()) );
    }
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
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, VECTOR2I aPos, int aLayer,
                 DRC_CUSTOM_MARKER_HANDLER* aCustomHandler )
            {
                if( aItem->GetErrorCode() == DRCE_COPPER_SLIVER )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    if( violations.empty() )
    {
        BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
        BOOST_TEST_MESSAGE( (const char*)(wxString::Format( "Zone fill copper sliver regression: %s passed", relPath ).utf8_str()) );
    }
    else
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::INCH );

        std::map<KIID, EDA_ITEM*> itemMap;
        m_board->FillItemMap( itemMap );

        for( const DRC_ITEM& item : violations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );

        BOOST_ERROR( (const char*)(wxString::Format( "Zone fill copper sliver regression: %s failed", relPath ).utf8_str()) );
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
                                         "Pad " << pad->GetNumber() << " from Footprint "
                                                << pad->GetParentFootprint()
                                                           ->GetReferenceAsString()
                                                           .ToStdString()
                                                << " has net code "
                                                << pad->GetNetname().ToStdString()
                                                << " and is connected to zone with net code "
                                                << zone->GetNetname().ToStdString() );
                }
            }
        }
    }
}