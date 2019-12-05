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

#include <fctsys.h>
#include <gr_basic.h>
#include <pcb_base_frame.h>
#include <common.h>
#include <kicad_string.h>
#include <pcbnew.h>
#include <richio.h>
#include <macros.h>
#include <msgpanel.h>
#include <base_units.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>


/*********************************************************/
/* class NETINFO_ITEM: handle data relative to a given net */
/*********************************************************/

NETINFO_ITEM::NETINFO_ITEM( BOARD* aParent, const wxString& aNetName, int aNetCode ) :
    BOARD_ITEM( aParent, PCB_NETINFO_T ),
    m_NetCode( aNetCode ),
    m_isCurrent( true ),
    m_Netname( aNetName ),
    m_ShortNetname( m_Netname.AfterLast( '/' ) )
{
    m_parent = aParent;

    if( aParent )
        m_NetClass = aParent->GetDesignSettings().m_NetClasses.GetDefault();
    else
        m_NetClass = std::make_shared<NETCLASS>( "<invalid>" );
}


NETINFO_ITEM::~NETINFO_ITEM()
{
    // m_NetClass is not owned by me.
}


/**
 * Function Print (TODO)
 */
void NETINFO_ITEM::Print( PCB_BASE_FRAME* frame, wxDC* DC, const wxPoint& aOffset )
{
}


void NETINFO_ITEM::SetClass( const NETCLASSPTR& aNetClass )
{
    wxCHECK( m_parent, /* void */ );
    m_NetClass = aNetClass ? aNetClass : m_parent->GetDesignSettings().m_NetClasses.GetDefault();
}


void NETINFO_ITEM::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString  txt;
    double    lengthnet = 0.0;      // This  is the length of tracks on pcb
    double    lengthPadToDie = 0.0; // this is the length of internal ICs connections

    aList.emplace_back( _( "Net Name" ), GetNetname(), RED );

    txt.Printf( wxT( "%d" ), GetNet() );
    aList.emplace_back( _( "Net Code" ), txt, RED );

    // Warning: for netcode == NETINFO_LIST::ORPHANED, the parent or the board
    // can be NULL
    BOARD * board = m_parent ? m_parent->GetBoard() : NULL;

    if( board == NULL )
        return;

    int count = 0;
    for( auto mod : board->Modules() )
    {
        for( auto pad : mod->Pads() )
        {
            if( pad->GetNetCode() == GetNet() )
            {
                count++;
                lengthPadToDie += pad->GetPadToDieLength();
            }
        }
    }

    txt.Printf( wxT( "%d" ), count );
    aList.emplace_back( _( "Pads" ), txt, DARKGREEN );

    count  = 0;

    for( auto track : board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            if( track->GetNetCode() == GetNet() )
                count++;
        }

        if( track->Type() == PCB_TRACE_T )
        {
            if( track->GetNetCode() == GetNet() )
                lengthnet += track->GetLength();
        }
    }

    txt.Printf( wxT( "%d" ), count );
    aList.emplace_back( _( "Vias" ), txt, BLUE );

    // Displays the full net length (tracks on pcb + internal ICs connections ):
    txt = MessageTextFromValue( aUnits, lengthnet + lengthPadToDie );
    aList.emplace_back( _( "Net Length" ), txt, RED );

    // Displays the net length of tracks only:
    txt = MessageTextFromValue( aUnits, lengthnet );
    aList.emplace_back( _( "On Board" ), txt, RED );

    // Displays the net length of internal ICs connections (wires inside ICs):
    txt = MessageTextFromValue( aUnits, lengthPadToDie, true );
    aList.emplace_back( _( "In Package" ), txt, RED );
}
