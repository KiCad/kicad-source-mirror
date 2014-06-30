/**
 * @file drag.h
 * @brief Useful classes and functions used to collect tracks to drag
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

#ifndef _DRAG_H_
#define _DRAG_H_


#include <class_track.h>
#include <vector>


class wxDC;
class wxPoint;
class EDA_DRAW_PANEL;
class MODULE;
class D_PAD;
class CONNECTIONS;


/** Helper classes to handle a list of track segments to drag
 * and has info to undo/abort the move command
 */

 /*
  * a DRAG_LIST manages the list of track segments to modify
  * when the pad or the module is moving
  */

/*
  * a DRAG_SEGM_PICKER manage one track segment or a via
  */
class DRAG_SEGM_PICKER
{
public:
    TRACK*  m_Track;            // pointer to the parent track segment
    D_PAD*  m_Pad_Start;        // pointer to the moving pad
                                // if the start point should follow this pad
                                // or NULL
    D_PAD*  m_Pad_End;          // pointer to the moving pad
                                // if the end point should follow this pad
                                // or NULL
    bool    m_Flag;             // flag used in drag vias and drag track segment functions

private:
    double  m_RotationOffset;   // initial orientation of the parent module
                                // Used to recalculate m_PadStartOffset and m_PadEndOffset
                                // after a module rotation when dragging
    bool    m_Flipped;          // initial side of the parent module
                                // Used to recalculate m_PadStartOffset and m_PadEndOffset
                                // if the module is flipped when dragging
    wxPoint m_PadStartOffset;   // offset between the pad and the starting point of the track
                                // usually 0,0, but not always
    wxPoint m_PadEndOffset;     // offset between the pad and the ending point of the track
                                // usually 0,0, but not always
    wxPoint m_startInitialValue;
    wxPoint m_endInitialValue;  // For abort command:
                                // initial m_Start and m_End values for m_Track

public:

    DRAG_SEGM_PICKER( TRACK* aTrack );
    ~DRAG_SEGM_PICKER() {};

    /**
     * Set auxiliary parameters relative to calculations needed
     * to find track ends positions while dragging pads
     * and when modules are rotated, flipped
     */
    void SetAuxParameters();

    /**
     * Calculate track ends position while dragging pads
     * and when modules are rotated, flipped
     * @param aOffset = offset of module or pad position (when moving)
     */
    void SetTrackEndsCoordinates( wxPoint aOffset );

    void RestoreInitialValues()
    {
        m_Track->SetStart( m_startInitialValue );
        m_Track->SetEnd( m_endInitialValue );
    }
};


class DRAG_LIST
{
public:
    BOARD*   m_Brd;         // the main board
    MODULE*  m_Module;      // The link to the module to move, or NULL
    D_PAD*   m_Pad;         // The link to the pad to move, or NULL

    std::vector<DRAG_SEGM_PICKER> m_DragList; // The list of DRAG_SEGM_PICKER items

public:
    DRAG_LIST( BOARD* aPcb )
    {
        m_Brd = aPcb;
    }

    /**
     * Function ClearList
     * clear the .m_Flags of all track segments in m_DragList
     * and clear the list.
     */
    void ClearList();

    /** Build the list of track segments connected to pads of aModule
     *  in m_DragList
     *  For each selected track segment the EDIT flag is set
     */
    void BuildDragListe( MODULE* aModule );

    /** Build the list of track segments connected to aPad
     *  in m_DragList
     *  For each selected track segment the EDIT flag is set
     */
    void BuildDragListe( D_PAD* aPad );

private:

    /** Fills m_DragList with of track segments connected to pads in aConnections
     *  For each selected track segment the EDIT flag is set
     */
    void fillList( CONNECTIONS& aConnections );
};


// Global variables:

// a list of DRAG_SEGM_PICKER items used to move or drag tracks.
// Each DRAG_SEGM_PICKER item points a segment to move.
extern std::vector<DRAG_SEGM_PICKER> g_DragSegmentList;

// Functions:
void DrawSegmentWhileMovingFootprint( EDA_DRAW_PANEL* panel, wxDC* DC );

/**
 * Function EraseDragList
 * clear the .m_Flags of all track segments stored in g_DragSegmentList
 * and clear the list.
 * In order to avoid useless memory reallocation, the memory is not freed
 * and will be reused when creating a new list
 */
void EraseDragList();

/**
 * Function Collect_TrackSegmentsToDrag.
 * used to collect track segments in drag track segment
 * Build the list of tracks connected to the ref point by calling
 * AddSegmentToDragList for each selected track
 * Net codes must be up to date, because only tracks having the right net code are tested.
 *
 * @param aPcb A point the the #BOARD object to collect track segment to drag.
 * @param aRefPos = reference point of connection
 * @param aLayerMask = layers mask to collect tracks
 * @param aNetCode = the net code to consider
 * @param aMaxDist = max distance from aRefPos to a track end candidate to collect the track
 */
void Collect_TrackSegmentsToDrag( BOARD* aPcb, const wxPoint& aRefPos, LSET aLayerMask,
                                  int aNetCode, int aMaxDist );

/* Add aTrack to the drag list
 * flag = STARTPOINT (if the point to drag is the start point of Track)
 * or ENDPOINT (if the point to drag is the end point of Track)
 */
void AddSegmentToDragList( int flag, TRACK* aTrack );

/*
 * Undraw the track segments in list, and set the EDIT flag
 * Usually called after the track list is built, to prepare
 * the redraw of the list when the mouse is moved
 */
void UndrawAndMarkSegmentsToDrag( EDA_DRAW_PANEL* aCanvas, wxDC* aDC );


#endif    // _DRAG_H_
