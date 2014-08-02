/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file attribut.cpp
 * @brief Track attribute flags editing.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <gr_basic.h>
#include <wxPcbStruct.h>
#include <msgpanel.h>

#include <pcbnew.h>
#include <protos.h>

#include <class_track.h>
#include <class_board.h>


/* Attribute change for 1 track segment.
 *  Attributes are
 *  TRACK_LOCKED       protection against global delete
 *  TRACK_AR           AutoRouted segment
 */
void PCB_EDIT_FRAME::Attribut_Segment( TRACK* track, wxDC* DC, bool Flag_On )
{
    if( track == NULL )
        return;

    OnModify();
    m_canvas->CrossHairOff( DC );   // Erase cursor shape
    track->SetState( TRACK_LOCKED, Flag_On );
    track->Draw( m_canvas, DC, GR_OR | GR_HIGHLIGHT );
    m_canvas->CrossHairOn( DC );    // Display cursor shape

    MSG_PANEL_ITEMS items;
    track->GetMsgPanelInfo( items );
    SetMsgPanel( items );
}


/* Attribute change for an entire track */
void PCB_EDIT_FRAME::Attribut_Track( TRACK* track, wxDC* DC, bool Flag_On )
{
    TRACK* Track;
    int    nb_segm;

    if( (track == NULL ) || (track->Type() == PCB_ZONE_T) )
        return;

    m_canvas->CrossHairOff( DC );   // Erase cursor shape
    Track = GetBoard()->MarkTrace( track, &nb_segm, NULL, NULL, true );
    DrawTraces( m_canvas, DC, Track, nb_segm, GR_OR | GR_HIGHLIGHT );

    for( ; (Track != NULL) && (nb_segm > 0); nb_segm-- )
    {
        Track->SetState( TRACK_LOCKED, Flag_On );
        Track->SetState( BUSY, false );
        Track = Track->Next();
    }

    m_canvas->CrossHairOn( DC );    // Display cursor shape

    OnModify();
}


/* Modify the flag TRACK_LOCKED according to Flag_On value,
 *  for all the segments related to net_code.
 *  if net_code < 0 all the segments are modified.
 */
void PCB_EDIT_FRAME::Attribut_net( wxDC* DC, int net_code, bool Flag_On )
{
    TRACK* Track = GetBoard()->m_Track;

    /* search the first segment for the selected net_code */
    if( net_code >= 0 )
    {
        for( ; Track != NULL; Track = Track->Next() )
        {
            if( net_code == Track->GetNetCode() )
                break;
        }
    }

    m_canvas->CrossHairOff( DC );     // Erase cursor shape

    while( Track )                  /* Flag change */
    {
        if( ( net_code >= 0 ) && ( net_code != Track->GetNetCode() ) )
            break;

        OnModify();
        Track->SetState( TRACK_LOCKED, Flag_On );
        Track->Draw( m_canvas, DC, GR_OR | GR_HIGHLIGHT );
        Track = Track->Next();
    }

    m_canvas->CrossHairOn( DC );    // Display cursor shape
    OnModify();
}
