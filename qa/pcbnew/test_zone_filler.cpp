/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <qa/pcbnew/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint.h>
#include <zone.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>


struct ZONE_FILL_TEST_FIXTURE
{
    ZONE_FILL_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


constexpr int delta = KiROUND( 0.006 * IU_PER_MM );


BOOST_FIXTURE_TEST_CASE( BasicZoneFills, ZONE_FILL_TEST_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "zone_filler", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    KI_TEST::FillZones( m_board.get(), 6 );

    // Now that the zones are filled we're going to increase the size of -some- pads and
    // tracks so that they generate DRC errors.  The test then makes sure that those errors
    // are generated, and that the other pads and tracks do -not- generate errors.

    for( PAD* pad : m_board->Footprints()[0]->Pads() )
    {
        if( pad->GetName() == "2" || pad->GetName() == "4" || pad->GetName() == "6" )
            pad->SetSize( pad->GetSize() + wxSize( delta, delta ) );
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
                track->Move( wxPoint( -delta, -delta ) );
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
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
            {
                if( aItem->GetErrorCode() == DRCE_CLEARANCE )
                {
                    BOARD_ITEM* item_a = m_board->GetItem( aItem->GetMainItemID() );
                    PAD*        pad_a = dynamic_cast<PAD*>( item_a );
                    PCB_TRACK*  trk_a = dynamic_cast<PCB_TRACK*>( item_a );

                    BOARD_ITEM* item_b = m_board->GetItem( aItem->GetAuxItemID() );
                    PAD*        pad_b = dynamic_cast<PAD*>( item_b );
                    PCB_TRACK*  trk_b = dynamic_cast<PCB_TRACK*>( item_b );

                    if(      pad_a && pad_a->GetName() == "2" ) foundPad2Error = true;
                    else if( pad_a && pad_a->GetName() == "4" ) foundPad4Error = true;
                    else if( pad_a && pad_a->GetName() == "6" ) foundPad6Error = true;
                    else if( pad_b && pad_b->GetName() == "2" ) foundPad2Error = true;
                    else if( pad_b && pad_b->GetName() == "4" ) foundPad4Error = true;
                    else if( pad_b && pad_b->GetName() == "6" ) foundPad6Error = true;
                    else if( trk_a && trk_a->m_Uuid == arc8 )   foundArc8Error = true;
                    else if( trk_a && trk_a->m_Uuid == arc12 )  foundArc12Error = true;
                    else if( trk_b && trk_b->m_Uuid == arc8 )   foundArc8Error = true;
                    else if( trk_b && trk_b->m_Uuid == arc12 )  foundArc12Error = true;
                    else                                        foundOtherError = true;
                }
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MILLIMETRES, true, false );

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

    // First, run a sanity check to validate that the saved board still has holes.
    SHAPE_POLY_SET frontCopper;

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->GetLayerSet().Contains( F_Cu ) )
        {
            frontCopper.BooleanAdd( zone->GetFilledPolysList( F_Cu ),
                                    SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
        }
    }

    BOOST_CHECK_GT( frontCopper.OutlineCount(), 2 );

    // Now re-fill and make sure the holes are gone.
    KI_TEST::FillZones( m_board.get(), 6 );

    frontCopper = SHAPE_POLY_SET();

    for( ZONE* zone : m_board->Zones() )
    {
        if( zone->GetLayerSet().Contains( F_Cu ) )
        {
            frontCopper.BooleanAdd( zone->GetFilledPolysList( F_Cu ),
                                    SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
        }
    }

    BOOST_CHECK_EQUAL( frontCopper.OutlineCount(), 2 );
}


BOOST_FIXTURE_TEST_CASE( RegressionZoneFillTests, ZONE_FILL_TEST_FIXTURE )
{
    std::vector<wxString> tests = { "issue18",
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
                                    "issue7086" };

    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );

        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        for( int fillVersion : { 5, 6 } )
        {
            KI_TEST::FillZones( m_board.get(), fillVersion );

            std::vector<DRC_ITEM> violations;

            bds.m_DRCEngine->SetViolationHandler(
                    [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
                    {
                        if( aItem->GetErrorCode() == DRCE_CLEARANCE )
                            violations.push_back( *aItem );
                    } );

            bds.m_DRCEngine->RunTests( EDA_UNITS::MILLIMETRES, true, false );

            if( violations.empty() )
            {
                BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
                BOOST_TEST_MESSAGE( wxString::Format( "Zone fill regression: %s, V%d algo passed",
                                                      relPath,
                                                      fillVersion ) );
            }
            else
            {
                std::map<KIID, EDA_ITEM*> itemMap;
                m_board->FillItemMap( itemMap );

                for( const DRC_ITEM& item : violations )
                {
                    BOOST_TEST_MESSAGE( item.ShowReport( EDA_UNITS::INCHES, RPT_SEVERITY_ERROR,
                                                         itemMap ) );
                }

                BOOST_ERROR( wxString::Format( "Zone fill regression: %s, V%d algo failed",
                                               relPath,
                                               fillVersion ) );
            }
        }
    }
}

