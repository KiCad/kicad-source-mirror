/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <boost/test/unit_test.hpp>
#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcb_io/pads/pads_parser.h>
#include <pcb_io/pads/pcb_io_pads.h>
#include <board.h>
#include <zone.h>

using namespace PADS_IO;


BOOST_AUTO_TEST_SUITE( PadsKeepoutParsing )


BOOST_AUTO_TEST_CASE( KeepoutStruct_DefaultConstruction )
{
    KEEPOUT keepout;
    BOOST_CHECK( keepout.outline.empty() );
    BOOST_CHECK( keepout.layers.empty() );
    BOOST_CHECK_EQUAL( static_cast<int>( keepout.type ), static_cast<int>( KEEPOUT_TYPE::ALL ) );
    BOOST_CHECK( keepout.no_traces );
    BOOST_CHECK( keepout.no_vias );
    BOOST_CHECK( keepout.no_copper );
    BOOST_CHECK( !keepout.no_components );
}


BOOST_AUTO_TEST_CASE( KeepoutType_AllKeepout )
{
    KEEPOUT keepout;
    keepout.type = KEEPOUT_TYPE::ALL;
    keepout.no_traces = true;
    keepout.no_vias = true;
    keepout.no_copper = true;

    BOOST_CHECK_EQUAL( static_cast<int>( keepout.type ), static_cast<int>( KEEPOUT_TYPE::ALL ) );
    BOOST_CHECK( keepout.no_traces );
    BOOST_CHECK( keepout.no_vias );
    BOOST_CHECK( keepout.no_copper );
}


BOOST_AUTO_TEST_CASE( KeepoutType_ViaOnly )
{
    KEEPOUT keepout;
    keepout.type = KEEPOUT_TYPE::VIA;
    keepout.no_traces = false;
    keepout.no_vias = true;
    keepout.no_copper = false;

    BOOST_CHECK_EQUAL( static_cast<int>( keepout.type ), static_cast<int>( KEEPOUT_TYPE::VIA ) );
    BOOST_CHECK( !keepout.no_traces );
    BOOST_CHECK( keepout.no_vias );
    BOOST_CHECK( !keepout.no_copper );
}


BOOST_AUTO_TEST_CASE( KeepoutType_RouteOnly )
{
    KEEPOUT keepout;
    keepout.type = KEEPOUT_TYPE::ROUTE;
    keepout.no_traces = true;
    keepout.no_vias = false;
    keepout.no_copper = false;

    BOOST_CHECK_EQUAL( static_cast<int>( keepout.type ), static_cast<int>( KEEPOUT_TYPE::ROUTE ) );
    BOOST_CHECK( keepout.no_traces );
    BOOST_CHECK( !keepout.no_vias );
    BOOST_CHECK( !keepout.no_copper );
}


BOOST_AUTO_TEST_CASE( KeepoutType_PlacementOnly )
{
    KEEPOUT keepout;
    keepout.type = KEEPOUT_TYPE::PLACEMENT;
    keepout.no_traces = false;
    keepout.no_vias = false;
    keepout.no_copper = false;
    keepout.no_components = true;

    BOOST_CHECK_EQUAL( static_cast<int>( keepout.type ), static_cast<int>( KEEPOUT_TYPE::PLACEMENT ) );
    BOOST_CHECK( !keepout.no_traces );
    BOOST_CHECK( !keepout.no_vias );
    BOOST_CHECK( !keepout.no_copper );
    BOOST_CHECK( keepout.no_components );
}


BOOST_AUTO_TEST_CASE( KeepoutOutline_RectanglePoints )
{
    KEEPOUT keepout;
    keepout.outline.emplace_back( 0, 0 );
    keepout.outline.emplace_back( 100, 0 );
    keepout.outline.emplace_back( 100, 100 );
    keepout.outline.emplace_back( 0, 100 );

    BOOST_REQUIRE_EQUAL( keepout.outline.size(), 4 );
    BOOST_CHECK_EQUAL( keepout.outline[0].x, 0 );
    BOOST_CHECK_EQUAL( keepout.outline[0].y, 0 );
    BOOST_CHECK_EQUAL( keepout.outline[2].x, 100 );
    BOOST_CHECK_EQUAL( keepout.outline[2].y, 100 );
}


BOOST_AUTO_TEST_CASE( KeepoutLayers_SingleLayer )
{
    KEEPOUT keepout;
    keepout.layers.push_back( 1 );

    BOOST_REQUIRE_EQUAL( keepout.layers.size(), 1 );
    BOOST_CHECK_EQUAL( keepout.layers[0], 1 );
}


BOOST_AUTO_TEST_CASE( KeepoutLayers_MultipleLayers )
{
    KEEPOUT keepout;
    keepout.layers.push_back( 1 );
    keepout.layers.push_back( 2 );
    keepout.layers.push_back( 4 );

    BOOST_REQUIRE_EQUAL( keepout.layers.size(), 3 );
    BOOST_CHECK_EQUAL( keepout.layers[0], 1 );
    BOOST_CHECK_EQUAL( keepout.layers[1], 2 );
    BOOST_CHECK_EQUAL( keepout.layers[2], 4 );
}


BOOST_AUTO_TEST_CASE( KeepoutLayers_AllLayers )
{
    // Empty layers vector means all layers
    KEEPOUT keepout;

    BOOST_CHECK( keepout.layers.empty() );
}


BOOST_AUTO_TEST_CASE( ParseKeepoutFile )
{
    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/keepout_test.asc";

    PARSER parser;
    parser.Parse( filename );

    const auto& keepouts = parser.GetKeepouts();

    BOOST_REQUIRE_EQUAL( keepouts.size(), 5 );

    // Verify KEEPOUT (all restrictions)
    bool found_keepout = false;

    for( const auto& ko : keepouts )
    {
        if( ko.type == KEEPOUT_TYPE::ALL && ko.outline.size() == 4 )
        {
            found_keepout = true;
            BOOST_CHECK( ko.no_traces );
            BOOST_CHECK( ko.no_vias );
            BOOST_CHECK( ko.no_copper );
            break;
        }
    }

    BOOST_CHECK( found_keepout );

    // Verify RESTRICTVIA (via only)
    bool found_via_restrict = false;

    for( const auto& ko : keepouts )
    {
        if( ko.type == KEEPOUT_TYPE::VIA )
        {
            found_via_restrict = true;
            BOOST_CHECK( !ko.no_traces );
            BOOST_CHECK( ko.no_vias );
            BOOST_CHECK( !ko.no_copper );
            break;
        }
    }

    BOOST_CHECK( found_via_restrict );

    // Verify RESTRICTROUTE (routing only)
    bool found_route_restrict = false;

    for( const auto& ko : keepouts )
    {
        if( ko.type == KEEPOUT_TYPE::ROUTE )
        {
            found_route_restrict = true;
            BOOST_CHECK( ko.no_traces );
            BOOST_CHECK( !ko.no_vias );
            BOOST_CHECK( !ko.no_copper );
            break;
        }
    }

    BOOST_CHECK( found_route_restrict );

    // Verify RESTRICTAREA (all copper)
    bool found_area_restrict = false;

    for( const auto& ko : keepouts )
    {
        if( ko.type == KEEPOUT_TYPE::ALL && ko.outline.size() > 0 )
        {
            // This could be the general KEEPOUT or the RESTRICTAREA
            // We need to verify we have at least one
            found_area_restrict = true;
        }
    }

    BOOST_CHECK( found_area_restrict );

    // Verify PLACEMENT_KEEPOUT
    bool found_placement = false;

    for( const auto& ko : keepouts )
    {
        if( ko.type == KEEPOUT_TYPE::PLACEMENT )
        {
            found_placement = true;
            BOOST_CHECK( !ko.no_traces );
            BOOST_CHECK( !ko.no_vias );
            BOOST_CHECK( !ko.no_copper );
            BOOST_CHECK( ko.no_components );
            break;
        }
    }

    BOOST_CHECK( found_placement );
}


BOOST_AUTO_TEST_CASE( ParseKeepoutOutlineGeometry )
{
    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/keepout_test.asc";

    PARSER parser;
    parser.Parse( filename );

    const auto& keepouts = parser.GetKeepouts();

    // Find the first keepout with 4 points (the rectangular KEEPOUT)
    const KEEPOUT* rect_keepout = nullptr;

    for( const auto& ko : keepouts )
    {
        if( ko.type == KEEPOUT_TYPE::ALL && ko.outline.size() == 4 )
        {
            rect_keepout = &ko;
            break;
        }
    }

    BOOST_REQUIRE( rect_keepout != nullptr );

    // File defines KEEPOUT_RECT at 100,100 with 4 corners relative to that:
    // 0,0 / 200,0 / 200,200 / 0,200
    // So absolute coordinates should be: 100,100 / 300,100 / 300,300 / 100,300
    BOOST_REQUIRE_EQUAL( rect_keepout->outline.size(), 4 );

    // The first point should be at (100, 100) in PADS coordinates
    BOOST_CHECK_CLOSE( rect_keepout->outline[0].x, 100.0, 0.1 );
    BOOST_CHECK_CLOSE( rect_keepout->outline[0].y, 100.0, 0.1 );

    // The third point should be at (300, 300)
    BOOST_CHECK_CLOSE( rect_keepout->outline[2].x, 300.0, 0.1 );
    BOOST_CHECK_CLOSE( rect_keepout->outline[2].y, 300.0, 0.1 );
}


BOOST_AUTO_TEST_CASE( ImportKeepoutAsRuleArea )
{
    PCB_IO_PADS plugin;
    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/keepout_test.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );
    BOOST_REQUIRE( board != nullptr );

    // Count rule areas (keepout zones)
    int ruleAreaCount = 0;
    int allKeepoutCount = 0;
    int viaKeepoutCount = 0;
    int routeKeepoutCount = 0;
    int placementKeepoutCount = 0;

    for( ZONE* zone : board->Zones() )
    {
        if( zone->GetIsRuleArea() )
        {
            ruleAreaCount++;

            if( zone->GetZoneName().StartsWith( wxT( "Keepout_" ) ) )
                allKeepoutCount++;
            else if( zone->GetZoneName().StartsWith( wxT( "ViaKeepout_" ) ) )
                viaKeepoutCount++;
            else if( zone->GetZoneName().StartsWith( wxT( "RouteKeepout_" ) ) )
                routeKeepoutCount++;
            else if( zone->GetZoneName().StartsWith( wxT( "PlacementKeepout_" ) ) )
                placementKeepoutCount++;
        }
    }

    // The test file should have 5 keepouts (KEEPOUT, RESTRICTVIA, RESTRICTROUTE, RESTRICTAREA, PLACEMENT_KEEPOUT)
    BOOST_CHECK_EQUAL( ruleAreaCount, 5 );
    BOOST_CHECK_EQUAL( allKeepoutCount, 2 );      // KEEPOUT and RESTRICTAREA both become Keepout_*
    BOOST_CHECK_EQUAL( viaKeepoutCount, 1 );
    BOOST_CHECK_EQUAL( routeKeepoutCount, 1 );
    BOOST_CHECK_EQUAL( placementKeepoutCount, 1 );
}


BOOST_AUTO_TEST_CASE( KeepoutConstraintFlags )
{
    PCB_IO_PADS plugin;
    wxString filename = KI_TEST::GetPcbnewTestDataDir() + "plugins/pads/keepout_test.asc";

    std::unique_ptr<BOARD> board( plugin.LoadBoard( filename, nullptr, nullptr, nullptr ) );
    BOOST_REQUIRE( board != nullptr );

    // Find ViaKeepout and verify only vias are prohibited
    ZONE* viaKeepout = nullptr;

    for( ZONE* zone : board->Zones() )
    {
        if( zone->GetZoneName().StartsWith( wxT( "ViaKeepout_" ) ) )
        {
            viaKeepout = zone;
            break;
        }
    }

    BOOST_REQUIRE( viaKeepout != nullptr );
    BOOST_CHECK( viaKeepout->GetDoNotAllowVias() );
    BOOST_CHECK( !viaKeepout->GetDoNotAllowTracks() );
    BOOST_CHECK( !viaKeepout->GetDoNotAllowZoneFills() );
    BOOST_CHECK( !viaKeepout->GetDoNotAllowFootprints() );

    // Find RouteKeepout and verify only tracks are prohibited
    ZONE* routeKeepout = nullptr;

    for( ZONE* zone : board->Zones() )
    {
        if( zone->GetZoneName().StartsWith( wxT( "RouteKeepout_" ) ) )
        {
            routeKeepout = zone;
            break;
        }
    }

    BOOST_REQUIRE( routeKeepout != nullptr );
    BOOST_CHECK( routeKeepout->GetDoNotAllowTracks() );
    BOOST_CHECK( !routeKeepout->GetDoNotAllowVias() );
    BOOST_CHECK( !routeKeepout->GetDoNotAllowZoneFills() );
    BOOST_CHECK( !routeKeepout->GetDoNotAllowFootprints() );

    // Find PlacementKeepout and verify only footprints are prohibited
    ZONE* placementKeepout = nullptr;

    for( ZONE* zone : board->Zones() )
    {
        if( zone->GetZoneName().StartsWith( wxT( "PlacementKeepout_" ) ) )
        {
            placementKeepout = zone;
            break;
        }
    }

    BOOST_REQUIRE( placementKeepout != nullptr );
    BOOST_CHECK( placementKeepout->GetDoNotAllowFootprints() );
    BOOST_CHECK( !placementKeepout->GetDoNotAllowTracks() );
    BOOST_CHECK( !placementKeepout->GetDoNotAllowVias() );
    BOOST_CHECK( !placementKeepout->GetDoNotAllowZoneFills() );
}


BOOST_AUTO_TEST_SUITE_END()
