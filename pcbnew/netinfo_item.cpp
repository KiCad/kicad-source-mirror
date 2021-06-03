/**
 * @brief NETINFO_ITEM class, to handle info on nets: netnames, net constraints
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kicad_string.h>
#include <widgets/msgpanel.h>
#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <track.h>
#include <pad.h>


/*********************************************************/
/* class NETINFO_ITEM: handle data relative to a given net */
/*********************************************************/

NETINFO_ITEM::NETINFO_ITEM( BOARD* aParent, const wxString& aNetName, int aNetCode ) :
        BOARD_ITEM( aParent, PCB_NETINFO_T ),
        m_netCode( aNetCode ),
        m_netname( aNetName ),
        m_shortNetname( m_netname.AfterLast( '/' ) ),
        m_isCurrent( true )
{
    m_parent = aParent;

    if( aParent )
        m_netClass = aParent->GetDesignSettings().GetNetClasses().GetDefault();
    else
        m_netClass = std::make_shared<NETCLASS>( "<invalid>" );
}


NETINFO_ITEM::~NETINFO_ITEM()
{
    // m_NetClass is not owned by me.
}


void NETINFO_ITEM::SetNetClass( const NETCLASSPTR& aNetClass )
{
    wxCHECK( m_parent, /* void */ );
    m_netClass = aNetClass ? aNetClass : m_parent->GetDesignSettings().GetNetClasses().GetDefault();
}


void NETINFO_ITEM::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    aList.emplace_back( _( "Net Name" ), UnescapeString( GetNetname() ) );

    aList.emplace_back( _( "Net Code" ), wxString::Format( "%d", GetNetCode() ) );

    // Warning: for netcode == NETINFO_LIST::ORPHANED, the parent or the board can be NULL
    BOARD * board = m_parent ? m_parent->GetBoard() : NULL;

    if( board )
    {
        int    count      = 0;
        TRACK* startTrack = nullptr;

        for( FOOTPRINT* footprint : board->Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
            {
                if( pad->GetNetCode() == GetNetCode() )
                    count++;
            }
        }

        aList.emplace_back( _( "Pads" ), wxString::Format( "%d", count ) );

        count = 0;

        for( TRACK* track : board->Tracks() )
        {
            if( track->GetNetCode() == GetNetCode() )
            {
                if( track->Type() == PCB_VIA_T )
                    count++;
                else if( !startTrack )
                    startTrack = track;
            }
        }

        aList.emplace_back( _( "Vias" ), wxString::Format( "%d", count ) );

        if( startTrack )
        {
            double lengthNet      = 0.0; // This  is the length of tracks on pcb
            double lengthPadToDie = 0.0; // this is the length of internal ICs connections

            std::tie( count, lengthNet, lengthPadToDie ) = board->GetTrackLength( *startTrack );

            // Displays the full net length (tracks on pcb + internal ICs connections ):
            msg = MessageTextFromValue( aFrame->GetUserUnits(), lengthNet + lengthPadToDie );
            aList.emplace_back( _( "Net Length" ), msg );

            // Displays the net length of tracks only:
            msg = MessageTextFromValue( aFrame->GetUserUnits(), lengthNet );
            aList.emplace_back( _( "On Board" ), msg );

            // Displays the net length of internal ICs connections (wires inside ICs):
            msg = MessageTextFromValue( aFrame->GetUserUnits(), lengthPadToDie );
            aList.emplace_back( _( "In Package" ), msg );
        }
    }
}
