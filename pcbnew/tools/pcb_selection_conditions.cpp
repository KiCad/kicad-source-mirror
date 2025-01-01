/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "pcb_selection_conditions.h"
#include "pcb_selection_tool.h"
#include <board_connected_item.h>

#include <functional>
using namespace std::placeholders;


SELECTION_CONDITION PCB_SELECTION_CONDITIONS::SameNet( bool aAllowUnconnected )
{
    return std::bind( &PCB_SELECTION_CONDITIONS::sameNetFunc, _1, aAllowUnconnected );
}


SELECTION_CONDITION PCB_SELECTION_CONDITIONS::SameLayer()
{
    return std::bind( &PCB_SELECTION_CONDITIONS::sameLayerFunc, _1 );
}


bool PCB_SELECTION_CONDITIONS::HasLockedItems( const SELECTION& aSelection )
{
    for( EDA_ITEM* item : aSelection.Items() )
    {
        if( item->IsBOARD_ITEM() && static_cast<BOARD_ITEM*>( item )->IsLocked() )
            return true;
    }

    return false;
}


bool PCB_SELECTION_CONDITIONS::HasUnlockedItems( const SELECTION& aSelection )
{
    for( EDA_ITEM* item : aSelection.Items() )
    {
        if( item->IsBOARD_ITEM() && !static_cast<BOARD_ITEM*>( item )->IsLocked() )
            return true;
    }

    return false;
}


bool PCB_SELECTION_CONDITIONS::sameNetFunc( const SELECTION& aSelection, bool aAllowUnconnected )
{
    if( aSelection.Empty() )
        return false;

    int netcode = -1;   // -1 stands for 'net code is not yet determined'

    for( const EDA_ITEM* aitem : aSelection )
    {
        int current_netcode = -1;

        const BOARD_CONNECTED_ITEM* item = dynamic_cast<const BOARD_CONNECTED_ITEM*>( aitem );

        if( item )
        {
            current_netcode = item->GetNetCode();
        }
        else
        {
            if( !aAllowUnconnected )
                return false;
            else
                // if it is not a BOARD_CONNECTED_ITEM, treat it as if there was no net assigned
                current_netcode = 0;
        }

        assert( current_netcode >= 0 );

        if( netcode < 0 )
        {
            netcode = current_netcode;

            if( netcode == NETINFO_LIST::UNCONNECTED && !aAllowUnconnected )
                return false;
        }
        else if( netcode != current_netcode )
        {
            return false;
        }
    }

    return true;
}


bool PCB_SELECTION_CONDITIONS::sameLayerFunc( const SELECTION& aSelection )
{
    if( aSelection.Empty() )
        return false;

    LSET layerSet;
    layerSet.set();

    for( const EDA_ITEM* i : aSelection )
    {
        const BOARD_ITEM* item = static_cast<const BOARD_ITEM*>( i );
        layerSet &= item->GetLayerSet();

        if( !layerSet.any() )       // there are no common layers left
            return false;
    }

    return true;
}
