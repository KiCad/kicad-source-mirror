/**
 * @file drc_marker_functions.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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


/* Methods of class DRC to initialize drc markers with messages
 * according to items and error code
*/

#include <fctsys.h>
#include <common.h>
#include <pcbnew.h>
#include <board_design_settings.h>

#include <drc.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_marker_pcb.h>
#include <class_pcb_text.h>
#include <class_board_item.h>


MARKER_PCB* DRC::fillMarker( TRACK* aTrack, BOARD_ITEM* bItem, int aErrorCode, MARKER_PCB* fillMe )
{
    wxPoint  posA;
    wxPoint  posB = wxPoint();

    if( bItem )     // aItem might be NULL
    {
        if( bItem->Type() == PCB_PAD_T )
        {
            posB = posA = ((D_PAD*)bItem)->GetPosition();
        }
        else if( bItem->Type() == PCB_VIA_T )
        {
            posB = posA = ((VIA*)bItem)->GetPosition();
        }
        else if( bItem->Type() == PCB_TRACE_T )
        {
            TRACK*  track  = (TRACK*) bItem;

            posB = track->GetPosition();

            wxPoint endPos = track->GetEnd();

            // either of aItem's start or end will be used for the marker position
            // first assume start, then switch at end if needed.  decision made on
            // distance from end of aTrack.
            posA = track->GetStart();

            double dToEnd = GetLineLength( endPos, aTrack->GetEnd() );
            double dToStart = GetLineLength( posA, aTrack->GetEnd() );

            if( dToEnd < dToStart )
                posA = endPos;
        }
        else if( bItem->Type() == PCB_TEXT_T )
        {
            posA = aTrack->GetPosition();
            posB = ((TEXTE_PCB*) bItem)->GetPosition();
        }
    }
    else
        posA = aTrack->GetPosition();

    if( fillMe )
        fillMe->SetData( aErrorCode, posA, aTrack, aTrack->GetPosition(), bItem, posB );
    else
        fillMe = new MARKER_PCB( aErrorCode, posA, aTrack, aTrack->GetPosition(), bItem, posB );

    return fillMe;
}


MARKER_PCB* DRC::fillMarker( D_PAD* aPad, BOARD_ITEM* aItem, int aErrorCode, MARKER_PCB* fillMe )
{
    wxPoint  posA = aPad->GetPosition();
    wxPoint  posB;

    if( aItem )
    {
        switch( aItem->Type() )
        {
        case PCB_PAD_T:
            posB = ((D_PAD*)aItem)->GetPosition();
            break;

        case PCB_TEXT_T:
            posB = ((TEXTE_PCB*)aItem)->GetPosition();
            break;

        default:
            wxLogDebug( wxT("fillMarker: unsupported item") );
            break;
        }
    }

    if( fillMe )
        fillMe->SetData( aErrorCode, posA, aPad, posA, aItem, posB );
    else
        fillMe = new MARKER_PCB( aErrorCode, posA, aPad, posA, aItem, posB );

    return fillMe;
}


MARKER_PCB* DRC::fillMarker(BOARD_ITEM *aItem, const wxPoint &aPos, int aErrorCode,
                            MARKER_PCB *fillMe)
{
    return fillMarker(aPos, aItem, nullptr, aErrorCode, fillMe );
}


MARKER_PCB* DRC::fillMarker( const wxPoint& aPos, BOARD_ITEM* aItem, BOARD_ITEM* bItem,
                             int aErrorCode, MARKER_PCB* fillMe )
{
    if( fillMe )
        fillMe->SetData( aErrorCode, aPos, aItem, aPos, bItem, aPos );
    else
        fillMe = new MARKER_PCB( aErrorCode, aPos, aItem, aPos, bItem, aPos );

    return fillMe;
}


MARKER_PCB* DRC::fillMarker( int aErrorCode, const wxString& aMessage, MARKER_PCB* fillMe )
{
    wxPoint posA;   // not displayed

    if( fillMe )
        fillMe->SetData( aErrorCode, posA, aMessage, posA );
    else
        fillMe = new MARKER_PCB( aErrorCode, posA, aMessage, posA );

    fillMe->SetShowNoCoordinate();

    return fillMe;
}


