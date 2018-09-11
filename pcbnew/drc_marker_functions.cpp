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
#include <geometry/geometry_utils.h>
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


const int EPSILON = Mils2iu( 5 );


MARKER_PCB* DRC::newMarker( TRACK* aTrack, ZONE_CONTAINER* aConflictZone, int aErrorCode )
{
    SHAPE_POLY_SET* conflictOutline;

    if( aConflictZone->IsFilled() )
        conflictOutline = const_cast<SHAPE_POLY_SET*>( &aConflictZone->GetFilledPolysList() );
    else
        conflictOutline = aConflictZone->Outline();

    wxPoint markerPos;
    wxPoint pt1 = aTrack->GetPosition();
    wxPoint pt2 = aTrack->GetEnd();

    // If the mid-point is in the zone, then that's a fine place for the marker
    if( conflictOutline->Distance( ( pt1 + pt2 ) / 2 ) == 0 )
        markerPos = ( pt1 + pt2 ) / 2;

    // Otherwise do a binary search for a "good enough" marker location
    else
    {
        while( GetLineLength( pt1, pt2 ) > EPSILON )
        {
            if( conflictOutline->Distance( pt1 ) < conflictOutline->Distance( pt2 ) )
                pt2 = ( pt1 + pt2 ) / 2;
            else
                pt1 = ( pt1 + pt2 ) / 2;
        }

        // Once we're within EPSILON pt1 and pt2 are "equivalent"
        markerPos = pt1;
    }

    return new MARKER_PCB( m_pcbEditorFrame->GetUserUnits(), aErrorCode, markerPos,
                           aTrack, aTrack->GetPosition(),
                           aConflictZone, aConflictZone->GetPosition() );
}


MARKER_PCB* DRC::newMarker( TRACK* aTrack, BOARD_ITEM* aConflitItem, const SEG& aConflictSeg,
                            int aErrorCode )
{
    wxPoint  markerPos;
    wxPoint pt1 = aTrack->GetPosition();
    wxPoint pt2 = aTrack->GetEnd();

    // Do a binary search along the track for a "good enough" marker location
    while( GetLineLength( pt1, pt2 ) > EPSILON )
    {
        if( aConflictSeg.Distance( pt1 ) < aConflictSeg.Distance( pt2 ) )
            pt2 = ( pt1 + pt2 ) / 2;
        else
            pt1 = ( pt1 + pt2 ) / 2;
    }

    // Once we're within EPSILON pt1 and pt2 are "equivalent"
    markerPos = pt1;

    return new MARKER_PCB( m_pcbEditorFrame->GetUserUnits(), aErrorCode, markerPos,
                           aTrack, aTrack->GetPosition(),
                           aConflitItem, aConflitItem->GetPosition() );
}


MARKER_PCB* DRC::newMarker( D_PAD* aPad, BOARD_ITEM* aConflictItem, int aErrorCode )
{
    return new MARKER_PCB( m_pcbEditorFrame->GetUserUnits(), aErrorCode, aPad->GetPosition(),
                           aPad, aPad->GetPosition(),
                           aConflictItem, aConflictItem->GetPosition() );
}


MARKER_PCB* DRC::newMarker(const wxPoint &aPos, BOARD_ITEM *aItem, int aErrorCode )
{
    return new MARKER_PCB( m_pcbEditorFrame->GetUserUnits(), aErrorCode, aPos,
                           aItem, aItem->GetPosition(), nullptr, wxPoint() );
}


MARKER_PCB* DRC::newMarker( const wxPoint &aPos, BOARD_ITEM* aItem, BOARD_ITEM* bItem,
                            int aErrorCode )
{
    return new MARKER_PCB( m_pcbEditorFrame->GetUserUnits(), aErrorCode, aPos,
                           aItem, aItem->GetPosition(), bItem, bItem->GetPosition() );
}


MARKER_PCB* DRC::newMarker( int aErrorCode, const wxString& aMessage )
{
    MARKER_PCB* marker = new MARKER_PCB( aErrorCode, wxPoint(), aMessage, wxPoint() );

    marker->SetShowNoCoordinate();

    return marker;
}


