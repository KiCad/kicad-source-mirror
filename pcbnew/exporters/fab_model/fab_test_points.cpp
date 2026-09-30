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

#include "fab_test_points.h"

#include <algorithm>
#include <memory>
#include <board.h>
#include <connectivity/connectivity_data.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>


std::vector<FAB_TEST_POINT> FabTestPoints( const BOARD& aBoard )
{
    std::vector<FAB_TEST_POINT>        points;
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = aBoard.GetConnectivity();

    for( PCB_TRACK* track : aBoard.Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA*       via = static_cast<PCB_VIA*>( track );
        FAB_TEST_POINT point;
        const LSET     layers = via->GetLayerSet();
        point.m_via = via;
        point.m_netCode = via->GetNetCode();
        point.m_position = via->GetPosition();
        point.m_front = layers.test( F_Cu );
        point.m_back = layers.test( B_Cu );
        point.m_frontMask = layers.test( F_Mask );
        point.m_backMask = layers.test( B_Mask );
        point.m_drill = via->GetDrillValue();

        // A via with more copper after it is a midpoint and not a net end
        point.m_netEnd =
                connectivity->GetConnectedItemsAtAnchor( via, point.m_position, { PCB_TRACE_T, PCB_ARC_T } ).size() < 2;
        points.push_back( point );
    }

    for( FOOTPRINT* footprint : aBoard.Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            FAB_TEST_POINT point;
            const LSET     layers = pad->GetLayerSet();
            point.m_pad = pad;
            point.m_netCode = pad->GetNetCode();
            point.m_position = pad->GetPosition();
            point.m_front = layers.test( F_Cu );
            point.m_back = layers.test( B_Cu );
            point.m_frontMask = layers.test( F_Mask );
            point.m_backMask = layers.test( B_Mask );
            point.m_drill = pad->HasHole() ? std::min( pad->GetDrillSize().x, pad->GetDrillSize().y ) : 0;
            points.push_back( point );
        }
    }

    return points;
}
