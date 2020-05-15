/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file board_connected_item.cpp
 * @brief BOARD_CONNECTED_ITEM class functions.
 */

#include <fctsys.h>
#include <pcbnew.h>

#include <class_board.h>
#include <class_board_item.h>

#include <connectivity/connectivity_data.h>


const wxChar* const traceMask = wxT( "BOARD_CONNECTED_ITEM" );


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


int BOARD_CONNECTED_ITEM::GetClearance( BOARD_ITEM* aItem, wxString* aSource ) const
{
    NETCLASS*              myNetclass = nullptr;
    NETCLASS*              itemNetclass = nullptr;
    BOARD_DESIGN_SETTINGS* bds = nullptr;

    // NB: we must check the net first, as when it is 0 GetNetClass() will return the
    // orphaned net netclass, not the default netclass.
    if( GetBoard() )
    {
        if( m_netinfo->GetNet() == 0 )
            myNetclass = GetBoard()->GetDesignSettings().GetDefault().get();
        else
            myNetclass = GetNetClass().get();

        bds = &GetBoard()->GetDesignSettings();
    }
    // No clearance if "this" is not (yet) linked to a board therefore no available netclass

    if( aItem && aItem->GetBoard() && dynamic_cast<BOARD_CONNECTED_ITEM*>( aItem ) )
    {
        if( dynamic_cast<BOARD_CONNECTED_ITEM*>( aItem )->GetNet()->GetNet() == 0 )
            itemNetclass = GetBoard()->GetDesignSettings().GetDefault().get();
        else
            itemNetclass = dynamic_cast<BOARD_CONNECTED_ITEM*>( aItem )->GetNetClass().get();

        bds = &aItem->GetBoard()->GetDesignSettings();
    }

    int      myClearance = myNetclass ? myNetclass->GetClearance() : 0;
    int      itemClearance = itemNetclass ? itemNetclass->GetClearance() : 0;
    wxString ruleSource;
    int      ruleClearance = bds ? bds->GetRuleClearance( this, myNetclass,
                                                          aItem, itemNetclass, &ruleSource ) : 0;
    int      clearance = std::max( std::max( myClearance, itemClearance ), ruleClearance );

    if( aSource )
    {
        if( clearance == myClearance && myNetclass )
        {
            *aSource = wxString::Format( _( "'%s' netclass clearance" ), myNetclass->GetName() );
        }
        else if( clearance == itemClearance && itemNetclass )
        {
            *aSource = wxString::Format( _( "'%s' netclass clearance" ), itemNetclass->GetName() );
        }
        else if( clearance == ruleClearance && !ruleSource.IsEmpty() )
        {
            *aSource = wxString::Format( _( "'%s' rule clearance" ), ruleSource );
        }
        else
        {
            *aSource = _( "No netclass" );
        }
    }

    return clearance;
}


NETCLASSPTR BOARD_CONNECTED_ITEM::GetNetClass() const
{
    NETCLASSPTR netclass = m_netinfo->GetNetClass();

    if( netclass )
        return netclass;
    else
        return GetBoard()->GetDesignSettings().GetDefault();
}


wxString BOARD_CONNECTED_ITEM::GetNetClassName() const
{
    return m_netinfo->GetClassName();
}
