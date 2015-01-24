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
#include <class_drawpanel.h>
#include <wxBasePcbFrame.h>
#include <common.h>
#include <kicad_string.h>
#include <pcbnew.h>
#include <colors_selection.h>
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

NETINFO_ITEM::NETINFO_ITEM( BOARD_ITEM* aParent, const wxString& aNetName, int aNetCode ) :
    m_NetCode( aNetCode ), m_Netname( aNetName ), m_ShortNetname( m_Netname.AfterLast( '/' ) )
{
    m_parent   = aParent;
    m_RatsnestStartIdx = 0;     // Starting point of ratsnests of this net in a
                                // general buffer of ratsnest
    m_RatsnestEndIdx   = 0;     // Ending point of ratsnests of this net

    m_NetClassName = NETCLASS::Default;
}


NETINFO_ITEM::~NETINFO_ITEM()
{
    // m_NetClass is not owned by me.
}


/**
 * Function Draw (TODO)
 */
void NETINFO_ITEM::Draw( EDA_DRAW_PANEL* panel,
                         wxDC*           DC,
                         GR_DRAWMODE     aDrawMode,
                         const wxPoint&  aOffset )
{
}


void NETINFO_ITEM::GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList )
{
    wxString  txt;
    double    lengthnet = 0.0;      // This  is the lenght of tracks on pcb
    double    lengthPadToDie = 0.0; // this is the lenght of internal ICs connections

    aList.push_back( MSG_PANEL_ITEM( _( "Net Name" ), GetNetname(), RED ) );

    txt.Printf( wxT( "%d" ), GetNet() );
    aList.push_back( MSG_PANEL_ITEM( _( "Net Code" ), txt, RED ) );

    // Warning: for netcode == NETINFO_LIST::ORPHANED, the parent or the board
    // can be NULL
    BOARD * board = m_parent ? m_parent->GetBoard() : NULL;

    if( board == NULL )
        return;

    int count = 0;
    for( MODULE* module = board->m_Modules; module != NULL; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad != 0; pad = pad->Next() )
        {
            if( pad->GetNetCode() == GetNet() )
            {
                count++;
                lengthPadToDie += pad->GetPadToDieLength();
            }
        }
    }

    txt.Printf( wxT( "%d" ), count );
    aList.push_back( MSG_PANEL_ITEM( _( "Pads" ), txt, DARKGREEN ) );

    count  = 0;

    for( const TRACK *track = board->m_Track; track != NULL; track = track->Next() )
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
    aList.push_back( MSG_PANEL_ITEM( _( "Vias" ), txt, BLUE ) );

    // Displays the full net length (tracks on pcb + internal ICs connections ):
    txt = ::LengthDoubleToString( lengthnet + lengthPadToDie );
    aList.push_back( MSG_PANEL_ITEM( _( "Net Length" ), txt, RED ) );

    // Displays the net length of tracks only:
    txt = ::LengthDoubleToString( lengthnet );
    aList.push_back( MSG_PANEL_ITEM( _( "On Board" ), txt, RED ) );

    // Displays the net length of internal ICs connections (wires inside ICs):
    txt = ::LengthDoubleToString( lengthPadToDie );
    aList.push_back( MSG_PANEL_ITEM( _( "In Package" ), txt, RED ) );
}


/***********************/
/* class RATSNEST_ITEM */
/***********************/

RATSNEST_ITEM::RATSNEST_ITEM()
{
    m_NetCode  = 0;         // netcode ( = 1.. n ,  0 is the value used for not
                            // connected items)
    m_Status   = 0;         // state
    m_PadStart = NULL;      // pointer to the starting pad
    m_PadEnd   = NULL;      // pointer to ending pad
    m_Lenght   = 0;         // length of the line (temporary used in some
                            // calculations)
}


/**
 * Function Draw
 * Draws a line (a ratsnest) from the starting pad to the ending pad
 */
void RATSNEST_ITEM::Draw( EDA_DRAW_PANEL* panel,
                          wxDC*           DC,
                          GR_DRAWMODE     aDrawMode,
                          const wxPoint&  aOffset )
{
    GRSetDrawMode( DC, aDrawMode );

    EDA_COLOR_T color = g_ColorsSettings.GetItemColor(RATSNEST_VISIBLE);

    GRLine( panel->GetClipBox(), DC,
            m_PadStart->GetPosition() - aOffset,
            m_PadEnd->GetPosition() - aOffset, 0, color );
}
