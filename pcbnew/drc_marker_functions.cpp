/**
 * @file drc_marker_functions.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2010 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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
 * according to items and error ode
*/

#include <fctsys.h>
#include <common.h>
#include <pcbnew.h>
#include <class_board_design_settings.h>

#include <drc_stuff.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_marker_pcb.h>


MARKER_PCB* DRC::fillMarker( const TRACK* aTrack, BOARD_ITEM* aItem, int aErrorCode,
                             MARKER_PCB* fillMe )
{
    wxString textA = aTrack->GetSelectMenuText();
    wxString textB;

    wxPoint  position;
    wxPoint  posB;

    if( aItem )     // aItem might be NULL
    {
        textB = aItem->GetSelectMenuText();

        if( aItem->Type() == PCB_PAD_T )
        {
            posB = position = ((D_PAD*)aItem)->GetPosition();
        }
        else if( aItem->Type() == PCB_VIA_T )
        {
            posB = position = ((VIA*)aItem)->GetPosition();
        }
        else if( aItem->Type() == PCB_TRACE_T )
        {
            TRACK*  track  = (TRACK*) aItem;

            posB = track->GetPosition();

            wxPoint endPos = track->GetEnd();

            // either of aItem's start or end will be used for the marker position
            // first assume start, then switch at end if needed.  decision made on
            // distance from end of aTrack.
            position = track->GetStart();

            double dToEnd = GetLineLength( endPos, aTrack->GetEnd() );
            double dToStart = GetLineLength( position, aTrack->GetEnd() );

            if( dToEnd < dToStart )
                position = endPos;
        }
    }
    else
        position = aTrack->GetPosition();

    if( fillMe )
    {
        if( aItem )
            fillMe->SetData( aErrorCode, position,
                             textA, aTrack->GetPosition(),
                             textB, posB );
        else
            fillMe->SetData( aErrorCode, position,
                             textA, aTrack->GetPosition() );
    }
    else
    {
        if( aItem )
        {
            fillMe = new MARKER_PCB( aErrorCode, position,
                                     textA, aTrack->GetPosition(),
                                     textB, posB );
            fillMe->SetItem( aItem );
        }
        else
        {
            fillMe = new MARKER_PCB( aErrorCode, position,
                                     textA, aTrack->GetPosition() );
        }
    }

    return fillMe;
}


MARKER_PCB* DRC::fillMarker( D_PAD* aPad, D_PAD* bPad, int aErrorCode, MARKER_PCB* fillMe )
{
    wxString textA = aPad->GetSelectMenuText();
    wxString textB = bPad->GetSelectMenuText();

    wxPoint  posA = aPad->GetPosition();
    wxPoint  posB = bPad->GetPosition();

    if( fillMe )
    {
        fillMe->SetData( aErrorCode, posA, textA, posA, textB, posB );
    }
    else
    {
        fillMe = new MARKER_PCB( aErrorCode, posA, textA, posA, textB, posB );
        fillMe->SetItem( aPad );    // TODO it has to be checked
    }

    return fillMe;
}


MARKER_PCB* DRC::fillMarker( ZONE_CONTAINER* aArea, int aErrorCode, MARKER_PCB* fillMe )
{
    wxString textA = aArea->GetSelectMenuText();

    wxPoint  posA = aArea->GetPosition();

    if( fillMe )
    {
        fillMe->SetData( aErrorCode, posA, textA, posA );
    }
    else
    {
        fillMe = new MARKER_PCB( aErrorCode, posA, textA, posA );
        fillMe->SetItem( aArea );
    }

    return fillMe;
}


MARKER_PCB* DRC::fillMarker( const ZONE_CONTAINER* aArea,
                             const wxPoint&        aPos,
                             int                   aErrorCode,
                             MARKER_PCB*           fillMe )
{
    wxString textA = aArea->GetSelectMenuText();

    wxPoint  posA = aPos;

    if( fillMe )
    {
        fillMe->SetData( aErrorCode, posA, textA, posA );
    }
    else
    {
        fillMe = new MARKER_PCB( aErrorCode, posA, textA, posA );
        fillMe->SetItem( aArea );
    }

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

