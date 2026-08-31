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

#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <zone.h>
#include <settings/settings_manager.h>
#include <geometry/shape_poly_set.h>
#include <teardrop/teardrop.h>
#include <tool/tool_manager.h>


/**
 * The U2 corner of demos/royalblue54L_feather.  The QFN's 3.6 mm exposed pad is held by the GND
 * pour through thermal spokes so it wants no teardrop, but its centre falls in the thermal gap
 * punched for the via underneath it, where the fill reads as absent.
 */
BOOST_AUTO_TEST_CASE( NoTeardropOnZoneConnectedThermalPad )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board;

    KI_TEST::LoadBoard( settingsManager, "teardrop_qfn_thermal_pad_zone", board );
    KI_TEST::FillZones( board.get() );
    board->BuildConnectivity();

    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( board.get(), nullptr, nullptr, nullptr, nullptr );

    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    toolMgr.RegisterTool( dummyTool );

    BOARD_COMMIT     commit( dummyTool );
    TEARDROP_MANAGER teardropMgr( board.get(), &toolMgr );

    teardropMgr.UpdateTeardrops( commit, nullptr, nullptr, true );

    if( !commit.Empty() )
        commit.Push( wxT( "Add teardrops" ), SKIP_UNDO | SKIP_SET_DIRTY );

    PAD* exposedPad = nullptr;

    for( FOOTPRINT* footprint : board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            // The footprint splits the exposed pad into eleven sub-pads all numbered 33.
            if( footprint->GetReference() == wxT( "U2" ) && pad->GetNumber() == wxT( "33" )
                && pad->GetSize( F_Cu ).x > pcbIUScale.mmToIU( 3.0 ) )
            {
                exposedPad = pad;
            }
        }
    }

    BOOST_REQUIRE( exposedPad );

    SHAPE_POLY_SET padPoly;
    int            teardropsOnPad = 0;

    exposedPad->TransformShapeToPolygon( padPoly, F_Cu, 0,
                                         board->GetDesignSettings().m_MaxError, ERROR_INSIDE );

    for( ZONE* zone : board->Zones() )
    {
        if( !zone->IsTeardropArea() || zone->GetFirstLayer() != F_Cu )
            continue;

        SHAPE_POLY_SET overlap = *zone->Outline();
        overlap.BooleanIntersection( padPoly );

        if( !overlap.IsEmpty() )
            teardropsOnPad++;
    }

    BOOST_CHECK_EQUAL( teardropsOnPad, 0 );
}
