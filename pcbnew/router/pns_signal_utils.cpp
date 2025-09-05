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

#include "pns_signal_utils.h"

#include <board.h>
#include <netinfo.h>
#include <pcb_track.h>

namespace PNS {

static void accumulateNet( BOARD* aBoard, int aNetCode, long long& aLen, long long& aDelay )
{
    if( !aBoard )
        return;

    PCB_TRACK* rep = nullptr;
    for( BOARD_ITEM* bi : aBoard->Tracks() )
    {
        if( auto tr = dynamic_cast<PCB_TRACK*>( bi ) )
        {
            if( tr->GetNetCode() == aNetCode ) { rep = tr; break; }
        }
    }

    if( rep )
    {
        int count = 0; double track = 0, pad = 0, tDelay = 0, padDelay = 0;
        std::tie( count, track, pad, tDelay, padDelay ) = aBoard->GetTrackLength( *rep );
        aLen += static_cast<long long>( track + pad );
        if( tDelay > 0.0 || padDelay > 0.0 )
            aDelay += static_cast<long long>( tDelay + padDelay );
    }
}

long long ComputeExtraSignalLength( BOARD* aBoard, const wxString& aSignal, const std::set<int>& aExclude )
{
    long long total = 0;
    if( !aBoard || aSignal.IsEmpty() )
        return total;

    for( NETINFO_ITEM* net : aBoard->GetNetInfo() )
    {
        if( net->GetSignal() != aSignal )
            continue;
        if( aExclude.count( net->GetNetCode() ) )
            continue;
        long long dummyDelay = 0;
        accumulateNet( aBoard, net->GetNetCode(), total, dummyDelay );
    }
    return total;
}

long long ComputeExtraSignalDelay( BOARD* aBoard, const wxString& aSignal, const std::set<int>& aExclude )
{
    long long totalDelay = 0;
    if( !aBoard || aSignal.IsEmpty() )
        return totalDelay;

    for( NETINFO_ITEM* net : aBoard->GetNetInfo() )
    {
        if( net->GetSignal() != aSignal )
            continue;
        if( aExclude.count( net->GetNetCode() ) )
            continue;
        long long len = 0;
        accumulateNet( aBoard, net->GetNetCode(), len, totalDelay );
    }
    return totalDelay;
}

}
