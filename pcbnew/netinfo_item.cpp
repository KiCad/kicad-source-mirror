/**
 * @brief NETINFO_ITEM class, to handle info on nets: netnames, net constraints
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_base_frame.h>
#include <string_utils.h>
#include <widgets/msgpanel.h>
#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <connectivity/connectivity_data.h>
#include <footprint.h>
#include <pcb_track.h>
#include <pad.h>


NETINFO_ITEM::NETINFO_ITEM( BOARD* aParent, const wxString& aNetName, int aNetCode ) :
        BOARD_ITEM( aParent, PCB_NETINFO_T ),
        m_netCode( aNetCode ),
        m_netname( aNetName ),
        m_shortNetname( m_netname.AfterLast( '/' ) ),
        m_displayNetname( UnescapeString( m_shortNetname ) ),
        m_isCurrent( true )
{
    m_parent = aParent;

    if( aParent )
        m_netClass = aParent->GetDesignSettings().m_NetSettings->m_DefaultNetClass;
    else
        m_netClass = std::make_shared<NETCLASS>( wxT( "Default" ) );
}


NETINFO_ITEM::~NETINFO_ITEM()
{
    // m_NetClass is not owned by me.
}


void NETINFO_ITEM::Clear()
{
    wxCHECK( m_parent, /* void */ );
    m_netClass = m_parent->GetDesignSettings().m_NetSettings->m_DefaultNetClass;
}


void NETINFO_ITEM::SetNetClass( const std::shared_ptr<NETCLASS>& aNetClass )
{
    wxCHECK( m_parent, /* void */ );

    if( aNetClass )
        m_netClass = aNetClass;
    else
        m_netClass = m_parent->GetDesignSettings().m_NetSettings->m_DefaultNetClass;
}


void NETINFO_ITEM::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    aList.emplace_back( _( "Net Name" ), UnescapeString( GetNetname() ) );

    aList.emplace_back( _( "Net Code" ), wxString::Format( wxT( "%d" ), GetNetCode() ) );

    // Warning: for netcode == NETINFO_LIST::ORPHANED, the parent or the board can be NULL
    BOARD * board = m_parent ? m_parent->GetBoard() : nullptr;

    if( board )
    {
        int        count      = 0;
        PCB_TRACK* startTrack = nullptr;

        for( FOOTPRINT* footprint : board->Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
            {
                if( pad->GetNetCode() == GetNetCode() )
                    count++;
            }
        }

        aList.emplace_back( _( "Pads" ), wxString::Format( wxT( "%d" ), count ) );

        count = 0;

        for( PCB_TRACK* track : board->Tracks() )
        {
            if( track->GetNetCode() == GetNetCode() )
            {
                if( track->Type() == PCB_VIA_T )
                    count++;
                else if( !startTrack )
                    startTrack = track;
            }
        }

        aList.emplace_back( _( "Vias" ), wxString::Format( wxT( "%d" ), count ) );

        if( startTrack )
        {
            double lengthNet      = 0.0; // This  is the length of tracks on pcb
            double lengthPadToDie = 0.0; // this is the length of internal ICs connections

            std::tie( count, lengthNet, lengthPadToDie ) = board->GetTrackLength( *startTrack );

            // Displays the full net length (tracks on pcb + internal ICs connections ):
            aList.emplace_back( _( "Net Length" ),
                                aFrame->MessageTextFromValue( lengthNet + lengthPadToDie ) );

            // Displays the net length of tracks only:
            aList.emplace_back( _( "On Board" ), aFrame->MessageTextFromValue( lengthNet ) );

            // Displays the net length of internal ICs connections (wires inside ICs):
            aList.emplace_back( _( "In Package" ), aFrame->MessageTextFromValue( lengthPadToDie ) );
        }
    }
}


bool NETINFO_ITEM::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return BOARD_ITEM::Matches( GetNetname(), aSearchData );
}


const BOX2I NETINFO_ITEM::GetBoundingBox() const
{
    std::shared_ptr<CONNECTIVITY_DATA> conn = GetBoard()->GetConnectivity();
    BOX2I                              bbox;

    for( BOARD_ITEM* item : conn->GetNetItems( m_netCode, { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T,
                                                            PCB_ZONE_T, PCB_PAD_T } ) )
    {
        bbox.Merge( item->GetBoundingBox() );
    }

    return bbox;
}
