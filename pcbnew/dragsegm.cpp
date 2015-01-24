/**
 * @file dragsegm.cpp
 * @brief Classes to find track segments connected to a pad or a module
 * for drag commands
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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
#include <common.h>
#include <trigo.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <wxBasePcbFrame.h>
#include <macros.h>

#include <drag.h>
#include <pcbnew.h>

#include <class_module.h>
#include <class_board.h>
#include <connect.h>


/* a list of DRAG_SEGM_PICKER items used to move or drag tracks */
std::vector<DRAG_SEGM_PICKER> g_DragSegmentList;

/* helper class to handle a list of track segments to drag or move
 */
DRAG_SEGM_PICKER::DRAG_SEGM_PICKER( TRACK* aTrack )
{
    m_Track = aTrack;
    m_startInitialValue = m_Track->GetStart();
    m_endInitialValue   = m_Track->GetEnd();
    m_Pad_Start = m_Track->GetState( START_ON_PAD ) ? (D_PAD*)m_Track->start : NULL;
    m_Pad_End = m_Track->GetState( END_ON_PAD ) ? (D_PAD*)m_Track->end : NULL;
    m_TempFlags = 0;
    m_RotationOffset = 0.0;
    m_Flipped = false;
}


void DRAG_SEGM_PICKER::SetAuxParameters()
{
    MODULE* module = NULL;

    if( m_Pad_Start )
    {
        module = (MODULE *) m_Pad_Start->GetParent();
        m_PadStartOffset = m_Track->GetStart() - m_Pad_Start->GetPosition();
    }

    if( m_Pad_End  )
    {
        if( module == NULL )
            module = (MODULE *) m_Pad_End->GetParent();

        m_PadEndOffset = m_Track->GetEnd() - m_Pad_End->GetPosition();
    }

    if( module )
    {
        m_Flipped = module->IsFlipped();
        m_RotationOffset = module->GetOrientation();
    }
}


void DRAG_SEGM_PICKER::SetTrackEndsCoordinates( wxPoint aOffset )
{
    // the track start position is the pad position + m_PadStartOffset
    // however m_PadStartOffset is known for the initial rotation/flip
    // this is also true for track end position and m_PadEndOffset
    // Therefore, because rotation/flipping is allowed during a drag
    // and move module, we should recalculate the pad offset,
    // depending on the current orientation/flip state of the module
    // relative to its initial orientation.
    // (although most of time, offset is 0,0)

    double curr_rot_offset = m_RotationOffset;
    MODULE* module = NULL;
    bool flip = false;

    if( m_Pad_Start )
        module = (MODULE *) m_Pad_Start->GetParent();

    if( module == NULL && m_Pad_End )
        module = (MODULE *) m_Pad_End->GetParent();

    if( module )
    {
        flip = m_Flipped != module->IsFlipped();
        curr_rot_offset = module->GetOrientation() - m_RotationOffset;

        if( flip )  // when flipping, module orientation is negated
            curr_rot_offset = - module->GetOrientation() - m_RotationOffset;
    }

    if( m_Pad_Start )
    {
        wxPoint padoffset = m_PadStartOffset;

        if( curr_rot_offset != 0.0 )
            RotatePoint(&padoffset, curr_rot_offset);

        if( flip )
            NEGATE( padoffset.y );

        m_Track->SetStart( m_Pad_Start->GetPosition() - aOffset + padoffset );
    }

    if( m_Pad_End )
    {
        wxPoint padoffset = m_PadEndOffset;

        if( curr_rot_offset != 0.0 )
            RotatePoint( &padoffset, curr_rot_offset );

        if( flip )
            NEGATE( padoffset.y );

        m_Track->SetEnd( m_Pad_End->GetPosition() - aOffset + padoffset );
    }
}


// A sort function needed to build ordered pads lists
extern bool sortPadsByXthenYCoord( D_PAD* const & ref, D_PAD* const & comp );


void DRAG_LIST::BuildDragListe( MODULE* aModule )
{
    m_Pad = NULL;
    m_Module = aModule;

    // Build connections info
    CONNECTIONS connections( m_Brd );
    std::vector<D_PAD*>&padList = connections.GetPadsList();

    for( D_PAD* pad = aModule->Pads();  pad;  pad = pad->Next() )
        padList.push_back( pad );

    sort( padList.begin(), padList.end(), sortPadsByXthenYCoord );

    fillList( connections );
}


void DRAG_LIST::BuildDragListe( D_PAD* aPad )
{
    m_Pad = aPad;
    m_Module = NULL;

    // Build connections info
    CONNECTIONS connections( m_Brd );
    std::vector<D_PAD*>&padList = connections.GetPadsList();
    padList.push_back( aPad );

    fillList( connections );
}


// A helper function to sort track list per tracks
bool sort_tracklist( const DRAG_SEGM_PICKER& ref, const DRAG_SEGM_PICKER& tst )
{
    return ref.m_Track < tst.m_Track;
}


void DRAG_LIST::fillList( CONNECTIONS& aConnections )
{
    aConnections.BuildTracksCandidatesList( m_Brd->m_Track, NULL);

    // Build connections info tracks to pads
    // Rebuild pads to track info only)
    aConnections.SearchTracksConnectedToPads( false, true );

    std::vector<D_PAD*>padList = aConnections.GetPadsList();

    // clear flags and variables of selected tracks
    for( unsigned ii = 0;  ii < padList.size(); ii++ )
    {
        D_PAD * pad = padList[ii];

        // store track connected to the pad
        for( unsigned jj = 0; jj < pad->m_TracksConnected.size(); jj++ )
        {
            TRACK * track = pad->m_TracksConnected[jj];
            track->start = NULL;
            track->end = NULL;
            track->SetState( START_ON_PAD|END_ON_PAD|BUSY, false );
        }
    }

    // store tracks connected to pads
    for(  unsigned ii = 0;  ii < padList.size(); ii++ )
    {
        D_PAD * pad = padList[ii];

        // store track connected to the pad
        for( unsigned jj = 0; jj < pad->m_TracksConnected.size(); jj++ )
        {
            TRACK * track = pad->m_TracksConnected[jj];

            if( pad->HitTest( track->GetStart() ) )
            {
                track->start = pad;
                track->SetState( START_ON_PAD, true );
            }

            if( pad->HitTest( track->GetEnd() ) )
            {
                track->end = pad;
                track->SetState( END_ON_PAD, true );
            }

            DRAG_SEGM_PICKER wrapper( track );
            m_DragList.push_back( wrapper );
        }
    }

    // remove duplicate in m_DragList:
    // a track can be stored more than once if connected to 2 overlapping pads, or
    // each track end connected to 2 moving pads
    // to avoid artifact in draw function, items should be not duplicated
    // However, there is not a lot of items to be removed, so there ir no optimization.

    // sort the drag list by track pointers
    sort( m_DragList.begin(), m_DragList.end(), sort_tracklist );

    // Explore the list, merge duplicates
    for( int ii = 0; ii < (int)m_DragList.size()-1; ii++ )
    {
        int jj = ii+1;

        if( m_DragList[ii].m_Track != m_DragList[jj].m_Track )
            continue;

        // duplicate found: merge info and remove duplicate
        if( m_DragList[ii].m_Pad_Start == NULL )
            m_DragList[ii].m_Pad_Start = m_DragList[jj].m_Pad_Start;

        if( m_DragList[ii].m_Pad_End == NULL )
            m_DragList[ii].m_Pad_End = m_DragList[jj].m_Pad_End;

        m_DragList.erase( m_DragList.begin() + jj );
        ii--;
    }

    // Initialize pad offsets and other params
    for( unsigned ii = 0; ii < m_DragList.size(); ii++ )
        m_DragList[ii].SetAuxParameters();

    // Copy the list in global list
    g_DragSegmentList = m_DragList;
}


void DRAG_LIST::ClearList()
{
    for( unsigned ii = 0; ii < m_DragList.size(); ii++ )
        m_DragList[ii].m_Track->ClearFlags();

    m_DragList.clear();

    m_Module = NULL;
    m_Pad = NULL;
}


// Redraw the list of segments stored in g_DragSegmentList, while moving a footprint
void DrawSegmentWhileMovingFootprint( EDA_DRAW_PANEL* panel, wxDC* DC )
{
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        TRACK* track = g_DragSegmentList[ii].m_Track;

#ifndef USE_WX_OVERLAY
        track->Draw( panel, DC, GR_XOR );   // erase from screen at old position
#endif
        g_DragSegmentList[ii].SetTrackEndsCoordinates( g_Offset_Module );
        track->Draw( panel, DC, GR_XOR );
    }
}


void EraseDragList()
{
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
        g_DragSegmentList[ii].m_Track->ClearFlags();

    g_DragSegmentList.clear();
}


void AddSegmentToDragList( int flag, TRACK* aTrack )
{
    DRAG_SEGM_PICKER wrapper( aTrack );

    if( flag & STARTPOINT )
    {
        wrapper.m_TempFlags |= STARTPOINT;
        aTrack->SetFlags( STARTPOINT );
    }

    if( flag & ENDPOINT )
    {
        wrapper.m_TempFlags |= ENDPOINT;
        aTrack->SetFlags( ENDPOINT );
    }

    g_DragSegmentList.push_back( wrapper );
}


void Collect_TrackSegmentsToDrag( BOARD* aPcb, const wxPoint& aRefPos, LSET aLayerMask,
                                  int aNetCode, int aMaxDist )
{
    TRACK* track = aPcb->m_Track->GetStartNetCode( aNetCode );

    for( ; track; track = track->Next() )
    {
        if( track->GetNetCode() != aNetCode )   // not the same netcode: all candidates tested
            break;

        if( !( aLayerMask & track->GetLayerSet() ).any() )
            continue;                       // Cannot be connected, not on the same layer

        if( track->IsDragging() )
            continue;                       // already put in list

        STATUS_FLAGS flag = 0;
        int maxdist = std::max( aMaxDist, track->GetWidth() / 2 );

        if( (track->GetFlags() & STARTPOINT) == 0 )
        {
            wxPoint delta = track->GetStart() - aRefPos;

            if( std::abs( delta.x ) <= maxdist && std::abs( delta.y ) <= maxdist )
            {
                int dist = KiROUND( EuclideanNorm( delta ) );

                if( dist <= maxdist )
                {
                    flag |= STARTPOINT;

                    if( track->Type() == PCB_VIA_T )
                        flag |= ENDPOINT;
                }
            }
        }

        if( (track->GetFlags() & ENDPOINT) == 0 )
        {
            wxPoint delta = track->GetEnd() - aRefPos;

            if( std::abs( delta.x ) <= maxdist && std::abs( delta.y ) <= maxdist )
            {
                int dist = KiROUND( EuclideanNorm( delta ) );

                if( dist <= maxdist )
                    flag |= ENDPOINT;
            }
        }

        // Note: vias will be flagged with both STARTPOINT and ENDPOINT
        // and must not be entered twice.
        if( flag )
        {
            AddSegmentToDragList( flag, track );

            // If a connected via is found at location aRefPos,
            // collect also tracks connected by this via.
            if( track->Type() == PCB_VIA_T )
                Collect_TrackSegmentsToDrag( aPcb, aRefPos, track->GetLayerSet(),
                                             aNetCode, track->GetWidth() / 2 );
        }
    }
}


void UndrawAndMarkSegmentsToDrag( EDA_DRAW_PANEL* aCanvas, wxDC* aDC )
{
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        TRACK* track = g_DragSegmentList[ii].m_Track;

        track->Draw( aCanvas, aDC, GR_XOR );
        track->SetState( IN_EDIT, false );
        track->SetFlags( IS_DRAGGED );

        if( g_DragSegmentList[ii].m_TempFlags & STARTPOINT )
            track->SetFlags( STARTPOINT );

        if( g_DragSegmentList[ii].m_TempFlags & ENDPOINT )
            track->SetFlags( ENDPOINT );

        track->Draw( aCanvas, aDC, GR_XOR );
    }
}
