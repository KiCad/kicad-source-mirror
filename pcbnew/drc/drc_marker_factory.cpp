/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "drc_marker_factory.h"

#include <board_design_settings.h>
#include <class_board_item.h>
#include <class_edge_mod.h>
#include <class_marker_pcb.h>
#include <class_pad.h>
#include <class_pcb_text.h>
#include <class_text_mod.h>
#include <class_track.h>
#include <class_zone.h>
#include <common.h>
#include <tools/drc.h>
#include <fctsys.h>
#include <geometry/geometry_utils.h>
#include <pcb_edit_frame.h>
#include <pcbnew.h>


const int EPSILON = Mils2iu( 5 );


DRC_MARKER_FACTORY::DRC_MARKER_FACTORY()
{
    SetUnits( EDA_UNITS_T::MILLIMETRES );
}


void DRC_MARKER_FACTORY::SetUnitsProvider( UNITS_PROVIDER aUnitsProvider )
{
    m_units_provider = aUnitsProvider;
}


void DRC_MARKER_FACTORY::SetUnits( EDA_UNITS_T aUnits )
{
    m_units_provider = [=]() { return aUnits; };
}


MARKER_PCB* DRC_MARKER_FACTORY::NewMarker(
        TRACK* aTrack, ZONE_CONTAINER* aConflictZone, int aErrorCode ) const
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

    return new MARKER_PCB( getCurrentUnits(), aErrorCode, markerPos, aTrack, aTrack->GetPosition(),
            aConflictZone, aConflictZone->GetPosition() );
}


MARKER_PCB* DRC_MARKER_FACTORY::NewMarker(
        TRACK* aTrack, BOARD_ITEM* aConflitItem, const SEG& aConflictSeg, int aErrorCode ) const
{
    wxPoint markerPos;
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

    return new MARKER_PCB( getCurrentUnits(), aErrorCode, markerPos, aTrack, aTrack->GetPosition(),
            aConflitItem, aConflitItem->GetPosition() );
}


MARKER_PCB* DRC_MARKER_FACTORY::NewMarker(
        D_PAD* aPad, BOARD_ITEM* aConflictItem, int aErrorCode ) const
{
    return new MARKER_PCB( getCurrentUnits(), aErrorCode, aPad->GetPosition(), aPad,
            aPad->GetPosition(), aConflictItem, aConflictItem->GetPosition() );
}


MARKER_PCB* DRC_MARKER_FACTORY::NewMarker(
        const wxPoint& aPos, BOARD_ITEM* aItem, int aErrorCode ) const
{
    return new MARKER_PCB(
            getCurrentUnits(), aErrorCode, aPos, aItem, aPos, nullptr, wxPoint() );
}


MARKER_PCB* DRC_MARKER_FACTORY::NewMarker(
        const wxPoint& aPos, BOARD_ITEM* aItem, BOARD_ITEM* bItem, int aErrorCode ) const
{
    return new MARKER_PCB( getCurrentUnits(), aErrorCode, aPos, aItem, aItem->GetPosition(), bItem,
            bItem->GetPosition() );
}


MARKER_PCB* DRC_MARKER_FACTORY::NewMarker( int aErrorCode, const wxString& aMessage ) const
{
    MARKER_PCB* marker = new MARKER_PCB( aErrorCode, wxPoint(), aMessage, wxPoint() );

    marker->SetShowNoCoordinate();

    return marker;
}
