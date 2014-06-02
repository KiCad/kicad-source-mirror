/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file class_netinfolist.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <macros.h>
#include <pcbnew.h>

#include <class_board.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_netinfo.h>


// Constructor and destructor
NETINFO_LIST::NETINFO_LIST( BOARD* aParent ) : m_Parent( aParent )
{
    // Make sure that the unconnected net has number 0
    AppendNet( new NETINFO_ITEM( aParent, wxEmptyString, 0 ) );

    m_newNetCode = 0;
}


NETINFO_LIST::~NETINFO_LIST()
{
    clear();
}


void NETINFO_LIST::clear()
{
    NETNAMES_MAP::iterator it, itEnd;
    for( it = m_netNames.begin(), itEnd = m_netNames.end(); it != itEnd; ++it )
        delete it->second;

    m_PadsFullList.clear();
    m_netNames.clear();
    m_netCodes.clear();
    m_newNetCode = 0;
}


void NETINFO_LIST::AppendNet( NETINFO_ITEM* aNewElement )
{
    // if there is a net with such name then just assign the correct number
    NETINFO_ITEM* sameName = GetNetItem( aNewElement->GetNetname() );

    if( sameName != NULL )
    {
        aNewElement->m_NetCode = sameName->GetNet();

        return;
    }
    // be sure that net codes are consecutive
    // negative net code means that it has to be auto assigned
    else if( ( aNewElement->m_NetCode != (int) m_netCodes.size() ) || ( aNewElement->m_NetCode < 0 ) )
    {
        aNewElement->m_NetCode = getFreeNetCode();
    }

    // net names & codes are supposed to be unique
    assert( GetNetItem( aNewElement->GetNetname() ) == NULL );
    assert( GetNetItem( aNewElement->GetNet() ) == NULL );

    // add an entry for fast look up by a net name using a map
    m_netNames.insert( std::make_pair( aNewElement->GetNetname(), aNewElement ) );
    m_netCodes.insert( std::make_pair( aNewElement->GetNet(), aNewElement ) );
}


/* sort function, to sort pad list by netnames
 * this is a case sensitive sort.
 * DO NOT change it because NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname )
 * when search a net by its net name does a binary search
 * and expects to have a nets list sorted by an alphabetic case sensitive sort
 */

static bool padlistSortByNetnames( const D_PAD* a, const D_PAD* b )
{
    return ( a->GetNetname().Cmp( b->GetNetname() ) ) < 0;
}


/**
 *  Compute and update the net_codes for PADS et and equipots (.m_NetCode member)
 *  net_codes are >= 1 (net_code = 0 means not connected)
 *  Update the net buffer
 *  Must be called after editing pads (netname, or deleting) or after read a netlist
 *  set to 1 flag NET_CODE_OK  of m_Pcb->m_Status_Pcb;
 *  m_Pcb->m_NbNodes and m_Pcb->m_NbNets are updated
 * Be aware NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname )
 * when search a net by its net name does a binary search
 * and expects to have a nets list sorted by an alphabetic case sensitive sort
 * So do not change Build_Pads_Full_List() which build a sorted list of pads
 */
void NETINFO_LIST::buildListOfNets()
{
    D_PAD*          pad;
    int             nodes_count = 0;

    // Build the PAD list, sorted by net
    buildPadsFullList();

    // Restore the initial state of NETINFO_ITEMs
    for( NETINFO_LIST::iterator net( begin() ), netEnd( end() ); net != netEnd; ++net )
        net->Clear();

    // Assign pads to appropriate NETINFO_ITEMs
    for( unsigned ii = 0; ii < m_PadsFullList.size(); ii++ )
    {
        pad = m_PadsFullList[ii];

        if( pad->GetNetCode() == NETINFO_LIST::UNCONNECTED ) // pad not connected
            continue;

        // Add pad to the appropriate list of pads
        NETINFO_ITEM* net = pad->GetNet();
        // it should not be possible for BOARD_CONNECTED_ITEM to return NULL as a result of GetNet()
        wxASSERT( net );

        if( net )
            net->m_PadInNetList.push_back( pad );

        ++nodes_count;
    }

    m_Parent->SetNodeCount( nodes_count );

    m_Parent->SynchronizeNetsAndNetClasses( );

    m_Parent->m_Status_Pcb |= NET_CODES_OK;

    m_Parent->SetAreasNetCodesFromNetNames();
}

#if defined(DEBUG)
void NETINFO_LIST::Show() const
{
    int i = 0;
    NETNAMES_MAP::const_iterator it, itEnd;
    for( it = m_netNames.begin(), itEnd = m_netNames.end(); it != itEnd; ++it )
    {
        printf( "[%d]: netcode:%d  netname:<%s>\n",
            i++, it->second->GetNet(),
            TO_UTF8( it->second->GetNetname() ) );
    }
}
#endif

void NETINFO_LIST::buildPadsFullList()
{
    /*
     * initialize:
     *   m_Pads (list of pads)
     * set m_Status_Pcb = LISTE_PAD_OK;
     * also clear m_Pcb->m_FullRatsnest that could have bad data
     *   (m_Pcb->m_FullRatsnest uses pointer to pads)
     * Be aware NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname )
     * when search a net by its net name does a binary search
     * and expects to have a nets list sorted by an alphabetic case sensitive sort
     * So do not change the sort function used here
     */

    if( m_Parent->m_Status_Pcb & LISTE_PAD_OK )
        return;

    // empty the old list
    m_PadsFullList.clear();
    m_Parent->m_FullRatsnest.clear();

    // Clear variables used in ratsnest computation
    for( MODULE* module = m_Parent->m_Modules;  module;  module = module->Next() )
    {
        for( D_PAD* pad = module->Pads();  pad;  pad = pad->Next() )
        {
            m_PadsFullList.push_back( pad );

            pad->SetSubRatsnest( 0 );
            pad->SetParent( module );
        }
    }

    // Sort pad list per net
    sort( m_PadsFullList.begin(), m_PadsFullList.end(), padlistSortByNetnames );

    m_Parent->m_Status_Pcb = LISTE_PAD_OK;
}


int NETINFO_LIST::getFreeNetCode()
{
    do {
        if( m_newNetCode < 0 )
            m_newNetCode = 0;
    } while( m_netCodes.count( ++m_newNetCode ) != 0 );

    return m_newNetCode;
}


int NETINFO_MAPPING::Translate( int aNetCode ) const
{
    std::map<int, int>::const_iterator value = m_netMapping.find( aNetCode );

    if( value != m_netMapping.end() )
        return value->second;

    // There was no entry for the given net code
    return aNetCode;
}


void NETINFO_MAPPING::Update()
{
    // Collect all the used nets
    std::set<int> nets;

    // Be sure that the unconnected gets 0 and is mapped as 0
    nets.insert( 0 );

    // Zones
    for( int i = 0; i < m_board->GetAreaCount(); ++i )
        nets.insert( m_board->GetArea( i )->GetNetCode() );

    // Tracks
    for( TRACK* track = m_board->m_Track; track; track = track->Next() )
        nets.insert( track->GetNetCode() );

    // Modules/pads
    for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads().GetFirst(); pad; pad = pad->Next() )
        {
            nets.insert( pad->GetNetCode() );
        }
    }

    // Segzones
    for( SEGZONE* zone = m_board->m_Zone; zone; zone = zone->Next() )
        nets.insert( zone->GetNetCode() );

    // Prepare the new mapping
    m_netMapping.clear();

    // Now the nets variable stores all the used net codes (not only for pads) and we are ready to
    // assign new consecutive net numbers
    int newNetCode = 0;
    for( std::set<int>::const_iterator it = nets.begin(), itEnd = nets.end(); it != itEnd; ++it )
        m_netMapping[*it] = newNetCode++;
}


NETINFO_ITEM* NETINFO_MAPPING::iterator::operator*() const
{
    return m_mapping->m_board->FindNet( m_iterator->first );
}


NETINFO_ITEM* NETINFO_MAPPING::iterator::operator->() const
{
    return m_mapping->m_board->FindNet( m_iterator->first );
}


const int NETINFO_LIST::UNCONNECTED = 0;
NETINFO_ITEM NETINFO_LIST::ORPHANED = NETINFO_ITEM( NULL, wxEmptyString, NETINFO_LIST::UNCONNECTED );
