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

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <macros.h>
#include <pcbnew.h>
#include <class_board.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_zone.h>
#include <netinfo.h>


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

    m_netNames.clear();
    m_netCodes.clear();
    m_newNetCode = 0;
}


NETINFO_ITEM* NETINFO_LIST::GetNetItem( int aNetCode ) const
{
    NETCODES_MAP::const_iterator result = m_netCodes.find( aNetCode );

    if( result != m_netCodes.end() )
        return (*result).second;

    return NULL;
}


NETINFO_ITEM* NETINFO_LIST::GetNetItem( const wxString& aNetName ) const
{
    NETNAMES_MAP::const_iterator result = m_netNames.find( aNetName );

    if( result != m_netNames.end() )
        return (*result).second;

    return NULL;
}


void NETINFO_LIST::RemoveNet( NETINFO_ITEM* aNet )
{
    for( NETCODES_MAP::iterator i = m_netCodes.begin(); i != m_netCodes.end(); ++i )
    {
        if ( i->second == aNet )
        {
            m_netCodes.erase(i);
            break;
        }
    }

    for( NETNAMES_MAP::iterator i = m_netNames.begin(); i != m_netNames.end(); ++i )
    {
        if ( i->second == aNet )
        {
            m_netNames.erase(i);
            break;
        }
    }

    m_newNetCode = std::min( m_newNetCode, aNet->m_NetCode - 1 );
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


/**
 *  Compute and update the net_codes for PADS et and equipots (.m_NetCode member)
 *  net_codes are >= 1 (net_code = 0 means not connected)
 *  Update the net buffer
 *  m_Pcb->m_NbNodes and m_Pcb->m_NbNets are updated
 * Be aware NETINFO_ITEM* BOARD::FindNet( const wxString& aNetname )
 * when search a net by its net name does a binary search
 * and expects to have a nets list sorted by an alphabetic case sensitive sort
 * So do not change Build_Pads_Full_List() which build a sorted list of pads
 */
void NETINFO_LIST::buildListOfNets()
{
    // Restore the initial state of NETINFO_ITEMs
    for( NETINFO_LIST::iterator net( begin() ), netEnd( end() ); net != netEnd; ++net )
        net->Clear();

    m_Parent->SynchronizeNetsAndNetClasses( );
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
                i++,
                it->second->GetNet(),
                TO_UTF8( it->second->GetNetname() ) );
    }
}
#endif


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
    for( auto track : m_board->Tracks() )
        nets.insert( track->GetNetCode() );

    // Modules/pads
    for( auto module : m_board->Modules() )
    {
        for( auto pad : module->Pads() )
        {
            nets.insert( pad->GetNetCode() );
        }
    }

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
const int NETINFO_LIST::ORPHANED = -1;

NETINFO_ITEM NETINFO_LIST::ORPHANED_ITEM = NETINFO_ITEM( NULL, wxEmptyString, NETINFO_LIST::UNCONNECTED );
