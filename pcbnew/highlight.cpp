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

/**
 * @file highlight.cpp
 * @brief Highlight nets.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <kicad_string.h>
#include <wxPcbStruct.h>
#include <kicad_device_context.h>
#include <macros.h>

#include <class_board.h>
#include <class_track.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <collectors.h>


#define Pad_fill ( Pad_Fill_Item.State == RUN )


void PCB_EDIT_FRAME::ListNetsAndSelect( wxCommandEvent& event )
{
    NETINFO_ITEM* net;
    wxString      netFilter;
    wxArrayString list;

    netFilter = wxT( "*" );
    wxTextEntryDialog dlg( this, _( "Filter Net Names" ), _( "Net Filter" ), netFilter );

    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    netFilter = dlg.GetValue( );

    if( netFilter.IsEmpty() )
        return;

    wxString Line;
    for( unsigned ii = 0; ii < GetBoard()->GetNetCount(); ii++ )
    {
        net = GetBoard()->m_NetInfo.GetNetItem( ii );

        if( !WildCompareString( netFilter, net->GetNetname(), false ) )
            continue;

        Line.Printf( wxT( "net %3.3d:  %s" ), net->GetNet(),
                     GetChars( net->GetNetname() ) );
        list.Add( Line );
    }

    wxSingleChoiceDialog choiceDlg( this, wxEmptyString, _( "Select Net" ), list );

    if( (choiceDlg.ShowModal() == wxID_CANCEL) || (choiceDlg.GetSelection() == wxNOT_FOUND) )
        return;

    bool     found   = false;
    unsigned netcode = (unsigned) choiceDlg.GetSelection();

    // Search for the net selected.
    for( unsigned ii = 0; ii < GetBoard()->GetNetCount(); ii++ )
    {
        net = GetBoard()->FindNet( ii );

        if( !WildCompareString( netFilter, net->GetNetname(), false ) )
            continue;

        if( ii == netcode )
        {
            netcode = net->GetNet();
            found   = true;
            break;
        }
    }

    if( found )
    {
        INSTALL_UNBUFFERED_DC( dc, m_canvas );

        if( GetBoard()->IsHighLightNetON() )
            HighLight( &dc );

        GetBoard()->SetHighLightNet( netcode );
        HighLight( &dc );
    }
}


int PCB_EDIT_FRAME::SelectHighLight( wxDC* DC )
{
    int netcode = -1;

    if( GetBoard()->IsHighLightNetON() )
        HighLight( DC );

    // use this scheme because a pad is a higher priority than a track in the
    // search, and finding a pad, instead of a track on a pad,
    // allows us to fire a message to Eeschema.

    GENERAL_COLLECTORS_GUIDE guide = GetCollectorsGuide();

    // optionally, modify the "guide" here as needed using its member functions

    m_Collector->Collect( GetBoard(), GENERAL_COLLECTOR::PadsTracksOrZones,
                          RefPos( true ), guide );

    BOARD_ITEM* item = (*m_Collector)[0];

    if( item )
    {
        switch( item->Type() )
        {
        case PCB_PAD_T:
            netcode = ( (D_PAD*) item )->GetNetCode();
            SendMessageToEESCHEMA( item );
            break;

        case PCB_TRACE_T:
        case PCB_VIA_T:
        case PCB_ZONE_T:
            // since these classes are all derived from TRACK, use a common
            // GetNet() function:
            netcode = ( (TRACK*) item )->GetNetCode();
            break;

        case PCB_ZONE_AREA_T:
            netcode = ( (ZONE_CONTAINER*) item )->GetNetCode();
            break;

        default:
            ;   // until somebody changes GENERAL_COLLECTOR::PadsOrTracks,
                // this should not happen.
        }
    }

    if( netcode >= 0 )
    {
        GetBoard()->SetHighLightNet( netcode );
        HighLight( DC );
    }

    return netcode;      // HitTest() failed.
}


void PCB_EDIT_FRAME::HighLight( wxDC* DC )
{
    if( GetBoard()->IsHighLightNetON() )
        GetBoard()->HighLightOFF();
    else
        GetBoard()->HighLightON();

    GetBoard()->DrawHighLight( m_canvas, DC, GetBoard()->GetHighLightNetCode() );
}

void PCB_EDIT_FRAME::HighlightUnconnectedPads( wxDC* DC )
{
    for( unsigned ii = 0; ii < GetBoard()->GetRatsnestsCount(); ii++ )
    {
        RATSNEST_ITEM* net = &GetBoard()->m_FullRatsnest[ii];

        if( (net->m_Status & CH_ACTIF) == 0 )
            continue;

        net->m_PadStart->Draw( m_canvas, DC, GR_OR | GR_HIGHLIGHT );
        net->m_PadEnd->Draw( m_canvas, DC, GR_OR | GR_HIGHLIGHT );
    }
}
