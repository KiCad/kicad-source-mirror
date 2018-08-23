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
#include <pcb_edit_frame.h>
#include <drc.h>
#include <class_pad.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_marker_pcb.h>
#include <class_pcb_text.h>
#include <class_text_mod.h>
#include <class_edge_mod.h>
#include <class_board_item.h>


MARKER_PCB* DRC::fillMarker( TRACK* aTrack, BOARD_ITEM* bItem, int aErrorCode, MARKER_PCB* fillMe )
{
    EDA_UNITS_T units = m_pcbEditorFrame->GetUserUnits();
    const int EPSILON = Mils2iu( 5 );

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
            TRACK*  bTrack  = (TRACK*) bItem;
            SEG     bTrackSeg( bTrack->GetPosition(), bTrack->GetEnd() );
            wxPoint pt1 = aTrack->GetPosition();
            wxPoint pt2 = aTrack->GetEnd();

            // Do a binary search for a "good enough" marker location
            while( GetLineLength( pt1, pt2 ) > EPSILON )
            {
                if( bTrackSeg.Distance( pt1 ) < bTrackSeg.Distance( pt2 ) )
                    pt2 = ( pt1 + pt2 ) / 2;
                else
                    pt1 = ( pt1 + pt2 ) / 2;
            }

            // Once we're within EPSILON pt1 and pt2 are "equivalent"
            posA = pt1;
            posB = bTrack->GetPosition();
        }
        else if( bItem->Type() == PCB_ZONE_T || bItem->Type() == PCB_ZONE_AREA_T )
        {
            ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( bItem );
            SHAPE_POLY_SET* outline;

            if( zone->IsFilled() )
                outline = const_cast<SHAPE_POLY_SET*>( &zone->GetFilledPolysList() );
            else
                outline = zone->Outline();

            wxPoint pt1 = aTrack->GetPosition();
            wxPoint pt2 = aTrack->GetEnd();

            // If the mid-point is in the zone, then that's a fine place for the marker
            if( outline->Distance( ( pt1 + pt2 ) / 2 ) == 0 )
                posA = ( pt1 + pt2 ) / 2;

            // Otherwise do a binary search for a "good enough" marker location
            else
            {
                while( GetLineLength( pt1, pt2 ) > EPSILON )
                {
                    if( outline->Distance( pt1 ) < outline->Distance( pt2 ) )
                        pt2 = ( pt1 + pt2 ) / 2;
                    else
                        pt1 = ( pt1 + pt2 ) / 2;
                }
                // Once we're within EPSILON pt1 and pt2 are "equivalent"
                posA = pt1;
            }

            posB = ((ZONE_CONTAINER*)bItem)->GetPosition();
        }
    }
    else
        posA = aTrack->GetPosition();

    if( fillMe )
        fillMe->SetData( units, aErrorCode, posA, aTrack, aTrack->GetPosition(), bItem, posB );
    else
        fillMe = new MARKER_PCB( units, aErrorCode, posA, aTrack, aTrack->GetPosition(), bItem, posB );

    return fillMe;
}


MARKER_PCB* DRC::fillMarker( D_PAD* aPad, BOARD_ITEM* aItem, int aErrorCode, MARKER_PCB* fillMe )
{
    EDA_UNITS_T units = m_pcbEditorFrame->GetUserUnits();

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

        case PCB_MODULE_TEXT_T:
            posB = ((TEXTE_MODULE*)aItem)->GetPosition();
            break;

        default:
            wxLogDebug( wxT("fillMarker: unsupported item") );
            break;
        }
    }

    if( fillMe )
        fillMe->SetData( units, aErrorCode, posA, aPad, posA, aItem, posB );
    else
        fillMe = new MARKER_PCB( units, aErrorCode, posA, aPad, posA, aItem, posB );

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
    EDA_UNITS_T units = m_pcbEditorFrame->GetUserUnits();

    if( fillMe )
        fillMe->SetData( units, aErrorCode, aPos, aItem, aPos, bItem, aPos );
    else
        fillMe = new MARKER_PCB( units, aErrorCode, aPos, aItem, aPos, bItem, aPos );

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


