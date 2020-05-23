/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pcbnew.h>

#include <class_board.h>
#include <class_board_item.h>

#include <connectivity/connectivity_data.h>


BOARD_CONNECTED_ITEM::BOARD_CONNECTED_ITEM( BOARD_ITEM* aParent, KICAD_T idtype ) :
    BOARD_ITEM( aParent, idtype ), m_netinfo( NETINFO_LIST::OrphanedItem() )
{
    m_localRatsnestVisible = true;
}


bool BOARD_CONNECTED_ITEM::SetNetCode( int aNetCode, bool aNoAssert )
{
    if( !IsOnCopperLayer() )
        aNetCode = 0;

    // if aNetCode < 0 ( typically NETINFO_LIST::FORCE_ORPHANED )
    // or no parent board,
    // set the m_netinfo to the dummy NETINFO_LIST::ORPHANED

    BOARD* board = GetBoard();
    //auto connectivity = board ? board->GetConnectivity() : nullptr;
    //bool addRatsnest = false;

    //if( connectivity )
        //addRatsnest = connectivity->Remove( this );

    if( ( aNetCode >= 0 ) && board )
        m_netinfo = board->FindNet( aNetCode );
    else
        m_netinfo = NETINFO_LIST::OrphanedItem();

    if( !aNoAssert )
        wxASSERT( m_netinfo );

    // Add only if it was previously added to the ratsnest
    //if( addRatsnest )
    //    connectivity->Add( this );

    return ( m_netinfo != NULL );
}


// This method returns the Default netclass for nets which don't have their own.
NETCLASS* BOARD_CONNECTED_ITEM::GetEffectiveNetclass() const
{
    // NB: we must check the net first, as when it is 0 GetNetClass() will return the
    // orphaned net netclass, not the default netclass.
    if( m_netinfo->GetNet() == 0 )
        return GetBoard()->GetDesignSettings().GetDefault();
    else
        return GetNetClass();
}


/*
 * Clearances exist in a hiearchy:
 * 1) accumulated board & netclass constraints
 * 2) last rule whose condition evaluates to true
 * 4) footprint override
 * 5) pad override
 *
 * The base class handles (1) and (2).
 */
int BOARD_CONNECTED_ITEM::GetClearance( BOARD_ITEM* aItem, wxString* aSource ) const
{
    BOARD* board = GetBoard();

    // No clearance if "this" is not (yet) linked to a board therefore no available netclass
    if( !board )
        return 0;

    DRC_RULE* rule = GetRule( this, aItem, CLEARANCE_CONSTRAINT );

    if( rule )
    {
        if( aSource )
            *aSource = wxString::Format( _( "'%s' rule clearance" ), rule->m_Name );

        return rule->m_Clearance.Min;
    }

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();
    int                    clearance = bds.m_MinClearance;

    if( aSource )
        *aSource = _( "board minimum" );

    NETCLASS* netclass = GetEffectiveNetclass();

    if( netclass && netclass->GetClearance() > clearance )
    {
        clearance = netclass->GetClearance();

        if( aSource )
            *aSource = wxString::Format( _( "'%s' netclass" ), netclass->GetName() );
    }

    if( aItem && aItem->IsConnected() )
    {
        netclass = static_cast<BOARD_CONNECTED_ITEM*>( aItem )->GetEffectiveNetclass();

        if( netclass && netclass->GetClearance() > clearance )
        {
            clearance = netclass->GetClearance();

            if( aSource )
                *aSource = wxString::Format( _( "'%s' netclass" ), netclass->GetName() );
        }
    }

    if( aItem && aItem->GetLayer() == Edge_Cuts && bds.m_CopperEdgeClearance > clearance )
    {
        clearance = bds.m_CopperEdgeClearance;

        if( aSource )
            *aSource = _( "board edge" );
    }

    return clearance;
}


// Note: do NOT return a std::shared_ptr from this.  It is used heavily in DRC, and the
// std::shared_ptr stuff shows up large in performance profiling.
NETCLASS* BOARD_CONNECTED_ITEM::GetNetClass() const
{
    NETCLASS* netclass = m_netinfo->GetNetClass();

    if( netclass )
        return netclass;
    else
        return GetBoard()->GetDesignSettings().GetDefault();
}


wxString BOARD_CONNECTED_ITEM::GetNetClassName() const
{
    return m_netinfo->GetClassName();
}
