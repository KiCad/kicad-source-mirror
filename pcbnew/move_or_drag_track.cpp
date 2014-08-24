/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file move_or_drag_track.cpp
 * @brief Track editing routines to move and drag track segments or node.
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <trigo.h>
#include <macros.h>
#include <gr_basic.h>

#include <class_board.h>

#include <pcbnew.h>
#include <drc_stuff.h>
#include <drag.h>
#include <pcbnew_id.h>


static void Show_MoveNode( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                           bool aErase );
static void Show_Drag_Track_Segment_With_Cte_Slope( EDA_DRAW_PANEL* aPanel,
                                                    wxDC*           aDC,
                                                    const wxPoint&  aPosition,
                                                    bool            aErase );
static void Abort_MoveTrack( EDA_DRAW_PANEL* Panel, wxDC* DC );
static bool InitialiseDragParameters();


static wxPoint PosInit, s_LastPos;
static double  s_StartSegmentSlope, s_EndSegmentSlope,
               s_MovingSegmentSlope,
               s_StartSegment_Yorg, s_EndSegment_Yorg,
               s_MovingSegment_Yorg; //slope and intercept parameters of lines
bool s_StartPointVertical, s_EndPointVertical,
     s_MovingSegmentVertical, s_MovingSegmentHorizontal,
     s_StartPointHorizontal, s_EndPointHorizontal;           // vertical or
                                                             // horizontal line
                                                             // indicators
bool s_StartSegmentPresent, s_EndSegmentPresent;

static PICKED_ITEMS_LIST s_ItemsListPicker;


/** Abort function for drag or move track
 */
static void Abort_MoveTrack( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) aPanel->GetParent();
    BOARD * pcb = frame->GetBoard();

    pcb->HighLightOFF();
    pcb->PopHighLight();

    frame->SetCurItem( NULL );
    aPanel->SetMouseCapture( NULL, NULL );

    // Undo move and redraw trace segments.
    for( unsigned jj=0 ; jj < g_DragSegmentList.size(); jj++ )
    {
        TRACK* track = g_DragSegmentList[jj].m_Track;
        g_DragSegmentList[jj].RestoreInitialValues();
        track->SetState( IN_EDIT, false );
        track->ClearFlags();
    }

    // Clear the undo picker list:
    s_ItemsListPicker.ClearListAndDeleteItems();
    EraseDragList();
    aPanel->Refresh();
}


// Redraw the moved node according to the mouse cursor position
static void Show_MoveNode( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                           bool aErase )
{
    wxPoint      moveVector;
    int          tmp = DisplayOpt.DisplayPcbTrackFill;
    GR_DRAWMODE  draw_mode = GR_XOR | GR_HIGHLIGHT;

    DisplayOpt.DisplayPcbTrackFill = false;

#ifndef USE_WX_OVERLAY
    aErase = true;
#else
    aErase = false;
#endif

    // set the new track coordinates
    wxPoint Pos = aPanel->GetParent()->GetCrossHairPosition();

    moveVector = Pos - s_LastPos;
    s_LastPos  = Pos;

    TRACK *track = NULL;

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        track = g_DragSegmentList[ii].m_Track;

        if( aErase )
            track->Draw( aPanel, aDC, draw_mode );

        if( track->GetFlags() & STARTPOINT )
            track->SetStart( track->GetStart() + moveVector );

        if( track->GetFlags() & ENDPOINT )
            track->SetEnd( track->GetEnd() + moveVector );

        if( track->Type() == PCB_VIA_T )
            track->SetEnd( track->GetStart() );

        track->Draw( aPanel, aDC, draw_mode );
    }

    DisplayOpt.DisplayPcbTrackFill = tmp;

    // Display track length
    if( track )
    {
        PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) aPanel->GetParent();
        frame->SetMsgPanel( track );
    }
}


/* drawing the track segment movement
 *  > s_MovingSegmentSlope slope = moving track segment slope
 *  > s_StartSegmentSlope slope = slope of the segment connected to the start
 * point of the moving segment
 *  > s_EndSegmentSlope slope = slope of the segment connected to the end point
 * of the moving segment
 *
 *  moved segment function :
 *      yt=s_MovingSegmentSlope * x + s_MovingSegment_Yorg
 *
 *  segment connected to moved segment's start:
 *      y1 = s_StartSegmentSlope * x + s_StartSegment_Yorg
 *
 *  segment connected to moved segment's end:
 *      y2=s_EndSegmentSlope * x + s_EndSegment_Yorg
 *
 *  first intersection point will be located at
 *      y1=yt ->
 *
 * xi1=(s_MovingSegment_Yorg-s_StartSegment_Yorg)/(s_StartSegmentSlope-s_MovingSegmentSlope)
 *      yi1=s_MovingSegmentSlope*xi1+s_MovingSegment_Yorg
 *      or yi1=s_StartSegmentSlope*xi1+s_MovingSegment_Yorg
 *
 *  second intersection point
 *      y2=yt ->
 *
 * xi2=(s_MovingSegment_Yorg-s_StartSegment_Yorg)/(s_MovingSegmentSlope-s_MovingSegmentSlope)
 *      yi2=s_MovingSegmentSlope*xi2+s_MovingSegment_Yorg
 *      or yi1=s_EndSegmentSlope*xi2+s_MovingSegment_Yorg
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *  !!!!!    special attention to vertical segments because
 *  !!!!!    their slope=infinite
 *  !!!!!    intersection point will be calculated using the
 *  !!!!!    segment intersecting it
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *  Slope parameters are computed once, because they can become undetermined
 * when moving segments
 *  (i.e. when a segment length is 0) and we want keep them constant
 */
static void Show_Drag_Track_Segment_With_Cte_Slope( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                                    const wxPoint& aPosition, bool aErase )
{
    double       xi1 = 0, yi1 = 0, xi2 = 0, yi2 = 0;    // calculated intersection points
    double       tx1, tx2, ty1, ty2;                    // temporary storage of points
    int          dx, dy;
    bool         update = true;
    TRACK*       Track;
    TRACK*       tSegmentToStart = NULL, * tSegmentToEnd = NULL;

    if( g_DragSegmentList.size() == 0 )
        return;

    /* get the segments :
     * from last to first in list are:
     * the segment to move
     * the segment connected to its end point (if exists)
     * the segment connected to its start point (if exists)
     */
    int ii = g_DragSegmentList.size() - 1;
    Track = g_DragSegmentList[ii].m_Track;

    if( Track == NULL )
        return;

    ii--;

    if( ii >= 0)
    {
        if( s_EndSegmentPresent )
        {
            // Get the segment connected to the end point
            tSegmentToEnd   = g_DragSegmentList[ii].m_Track;
            ii--;
        }

        if( s_StartSegmentPresent )
        {
            // Get the segment connected to the start point
            if( ii >= 0 )
                tSegmentToStart = g_DragSegmentList[ii].m_Track;
        }
    }

    GR_DRAWMODE draw_mode = GR_XOR | GR_HIGHLIGHT;

    // Undraw the current moved track segments before modification

#ifndef USE_WX_OVERLAY
//  if( erase )
    {
        Track->Draw( aPanel, aDC, draw_mode );

        if( tSegmentToStart )
            tSegmentToStart->Draw( aPanel, aDC, draw_mode );

        if( tSegmentToEnd )
            tSegmentToEnd->Draw( aPanel, aDC, draw_mode );
    }
#endif

    // Compute the new track segment position
    wxPoint Pos = aPanel->GetParent()->GetCrossHairPosition();

    dx = Pos.x - s_LastPos.x;
    dy = Pos.y - s_LastPos.y;

    // move the line by dx and dy
    tx1 = (double) ( Track->GetStart().x + dx );
    ty1 = (double) ( Track->GetStart().y + dy );
    tx2 = (double) ( Track->GetEnd().x + dx );
    ty2 = (double) ( Track->GetEnd().y + dy );

    // recalculate the segments new parameters and intersection points
    // only the intercept will change, segment slopes does not change
    // because we are moving parallel with is initial state
    if( !s_MovingSegmentVertical )
    {
        s_MovingSegment_Yorg = ty1 - ( s_MovingSegmentSlope * tx1 );
    }

    if( ( !s_EndPointVertical ) && ( !s_MovingSegmentVertical ) )
    {
        xi2 = ( s_MovingSegment_Yorg - s_EndSegment_Yorg )
            / ( s_EndSegmentSlope - s_MovingSegmentSlope );
    }
    else
    {
        if( !s_EndPointVertical )
        {
            xi2 = tx2;
        }
        else
        {
            //P1=P2
            if( !s_EndPointHorizontal )
            {
                xi2 = tx2 - dx;
            }
            else
            {
                update = false;
            }
        }
    }

    if( !s_MovingSegmentVertical )
    {
        yi2 = s_MovingSegmentSlope * ( xi2 ) + s_MovingSegment_Yorg;
    }
    else
    {
        if( !s_EndPointVertical )
        {
            yi2 = s_EndSegmentSlope * ( xi2 ) + s_EndSegment_Yorg;
        }
        else
        {
            if( !s_EndPointHorizontal )
            {
                update = false;
            }
            else
            {
                yi2 = s_MovingSegmentSlope * ( xi2 ) + s_MovingSegment_Yorg;
            }
        }
    }

    if( ( !s_StartPointVertical ) && ( !s_MovingSegmentVertical ) )
    {
        xi1 = ( s_MovingSegment_Yorg - s_StartSegment_Yorg )
            / ( s_StartSegmentSlope - s_MovingSegmentSlope );
    }
    else
    {
        if( !s_StartPointVertical )
        {
            xi1 = tx1;
        }
        else
        {
            //P1=P2
            if( !s_StartPointHorizontal )
            {
                xi1 = tx1 - dx;
            }
            else
            {
                if( !s_StartPointHorizontal )
                {
                    update = false;
                }
            }
        }
    }

    if( !s_MovingSegmentVertical )
    {
        yi1 = s_MovingSegmentSlope * ( xi1 ) + s_MovingSegment_Yorg;
    }
    else
    {
        if( !s_StartPointVertical )
        {
            yi1 = s_StartSegmentSlope * ( xi1 ) + s_StartSegment_Yorg;
        }
        else
        {
            if( !s_StartPointHorizontal )
            {
                update = false;
            }
            else
            {
                yi2 = s_MovingSegmentSlope * ( xi1 ) + s_MovingSegment_Yorg;
            }
        }
    }

    // update the segment coordinates (if possible)
    if( tSegmentToStart == NULL )
    {
        xi1 = tx1;
        yi1 = ty1;
    }

    if( tSegmentToEnd == NULL )
    {
        xi2 = tx2;
        yi2 = ty2;
    }

    if( update )
    {
        s_LastPos = Pos;
        Track->SetStart( wxPoint( KiROUND( xi1 ), KiROUND( yi1 ) ) );
        Track->SetEnd( wxPoint( KiROUND( xi2 ), KiROUND( yi2 ) ) );

        if( tSegmentToEnd )
        {
            if( tSegmentToEnd->GetFlags() & STARTPOINT )
                tSegmentToEnd->SetStart( Track->GetEnd() );
            else
                tSegmentToEnd->SetEnd( Track->GetEnd() );
        }

        if( tSegmentToStart )
        {
            if( tSegmentToStart->GetFlags() & STARTPOINT )
                tSegmentToStart->SetStart( Track->GetStart() );
            else
                tSegmentToStart->SetEnd( Track->GetStart() );
        }
    }

    Track->Draw( aPanel, aDC, draw_mode );

    if( tSegmentToStart )
        tSegmentToStart->Draw( aPanel, aDC, draw_mode );

    if( tSegmentToEnd )
        tSegmentToEnd->Draw( aPanel, aDC, draw_mode );

    // Display track length
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) aPanel->GetParent();
    frame->SetMsgPanel( Track );
}


/* Init variables (slope, Y intersect point, flags) for
 * Show_Drag_Track_Segment_With_Cte_Slope()
 *  return true if Ok, false if dragging is not possible
 *  (2 colinear segments)
 */
bool InitialiseDragParameters()
{
    double     tx1, tx2, ty1, ty2; // temporary storage of points
    TRACK*     Track;
    TRACK*     tSegmentToStart = NULL, * tSegmentToEnd = NULL;

    if( g_DragSegmentList.size() == 0 )
        return false;

    /* get the segments :
     * from last to first in list are:
     * the segment to move
     * the segment connected to its end point (if exists)
     * the segment connected to its start point (if exists)
     */
    int ii = g_DragSegmentList.size() - 1;
    Track = g_DragSegmentList[ii].m_Track;
    if( Track == NULL )
        return false;

    ii--;

    if( ii >= 0)
    {
        if( s_EndSegmentPresent )
        {
            tSegmentToEnd = g_DragSegmentList[ii].m_Track;  // Get the segment connected to
                                                           // the end point
            ii--;
        }

        if( s_StartSegmentPresent )
        {
            if( ii  >= 0 )
                tSegmentToStart = g_DragSegmentList[ii].m_Track;  // Get the segment connected to
                                                                 // the start point
        }
    }

    // would be nice to eliminate collinear segments here, so we don't
    // have to deal with that annoying "Unable to drag this segment: two
    // collinear segments"

    s_StartPointVertical = false;
    s_EndPointVertical   = false;
    s_MovingSegmentVertical   = false;
    s_StartPointHorizontal    = false;
    s_EndPointHorizontal      = false;
    s_MovingSegmentHorizontal = false;

    // Init parameters for the starting point of the moved segment
    if( tSegmentToStart )
    {
        if( tSegmentToStart->GetFlags() & ENDPOINT )
        {
            tx1 = (double) tSegmentToStart->GetStart().x;
            ty1 = (double) tSegmentToStart->GetStart().y;
            tx2 = (double) tSegmentToStart->GetEnd().x;
            ty2 = (double) tSegmentToStart->GetEnd().y;
        }
        else
        {
            tx1 = (double) tSegmentToStart->GetEnd().x;
            ty1 = (double) tSegmentToStart->GetEnd().y;
            tx2 = (double) tSegmentToStart->GetStart().x;
            ty2 = (double) tSegmentToStart->GetStart().y;
        }
    }
    else // move the start point on a line starting at Track->GetStart(), and perpendicular to Track
    {
        tx1 = (double) Track->GetStart().x;
        ty1 = (double) Track->GetStart().y;
        tx2 = (double) Track->GetEnd().x;
        ty2 = (double) Track->GetEnd().y;
        RotatePoint( &tx2, &ty2, tx1, ty1, 900 );
    }

    if( tx1 != tx2 )
    {
        s_StartSegmentSlope = ( ty2 - ty1 ) / ( tx2 - tx1 );
        s_StartSegment_Yorg = ty1 - ( ty2 - ty1 ) * tx1 / ( tx2 - tx1 );
    }
    else
    {
        s_StartPointVertical = true;            //signal first segment vertical
    }

    if( ty1 == ty2 )
    {
        s_StartPointHorizontal = true;
    }

    // Init parameters for the ending point of the moved segment
    if( tSegmentToEnd )
    {
        //check if second line is vertical
        if( tSegmentToEnd->GetFlags() & STARTPOINT )
        {
            tx1 = (double) tSegmentToEnd->GetStart().x;
            ty1 = (double) tSegmentToEnd->GetStart().y;
            tx2 = (double) tSegmentToEnd->GetEnd().x;
            ty2 = (double) tSegmentToEnd->GetEnd().y;
        }
        else
        {
            tx1 = (double) tSegmentToEnd->GetEnd().x;
            ty1 = (double) tSegmentToEnd->GetEnd().y;
            tx2 = (double) tSegmentToEnd->GetStart().x;
            ty2 = (double) tSegmentToEnd->GetStart().y;
        }
    }
    else // move the start point on a line starting at Track->GetEnd(), and perpendicular to Track
    {
        tx1 = (double) Track->GetEnd().x;
        ty1 = (double) Track->GetEnd().y;
        tx2 = (double) Track->GetStart().x;
        ty2 = (double) Track->GetStart().y;
        RotatePoint( &tx2, &ty2, tx1, ty1, -900 );
    }

    if( tx2 != tx1 )
    {
        s_EndSegmentSlope = ( ty2 - ty1 ) / ( tx2 - tx1 );
        s_EndSegment_Yorg = ty1 - ( ty2 - ty1 ) * tx1 / ( tx2 - tx1 );
    }
    else
    {
        s_EndPointVertical = true;      //signal second segment vertical
    }

    if( ty1 == ty2 )
    {
        s_EndPointHorizontal = true;
    }

    // Init parameters for the moved segment
    tx1 = (double) Track->GetStart().x;
    ty1 = (double) Track->GetStart().y;
    tx2 = (double) Track->GetEnd().x;
    ty2 = (double) Track->GetEnd().y;

    if( tx2 != tx1 )
    {
        s_MovingSegmentSlope = ( ty2 - ty1 ) / ( tx2 - tx1 );
    }
    else
    {
        s_MovingSegmentVertical = true;      // signal vertical line
    }

    if( ty1 == ty2 )
    {
        s_MovingSegmentHorizontal = true;
    }

    // Test if drag is possible:
    if( s_MovingSegmentVertical )
    {
        if( s_EndPointVertical || s_StartPointVertical )
            return false;
    }
    else
    {
        if( !s_EndPointVertical && ( s_MovingSegmentSlope == s_EndSegmentSlope ) )
            return false;

        if( !s_StartPointVertical && ( s_MovingSegmentSlope == s_StartSegmentSlope ) )
            return false;
    }

    return true;
}


void PCB_EDIT_FRAME::StartMoveOneNodeOrSegment( TRACK* aTrack, wxDC* aDC, int aCommand )
{
    if( !aTrack )
        return;

    EraseDragList();

    // Change highlighted net: the new one will be highlighted
    GetBoard()->PushHighLight();

    if( GetBoard()->IsHighLightNetON() )
        HighLight( aDC );

    PosInit = GetCrossHairPosition();

    if( aTrack->Type() == PCB_VIA_T )
    {
        aTrack->SetFlags( IS_DRAGGED | STARTPOINT | ENDPOINT );
        AddSegmentToDragList( aTrack->GetFlags(), aTrack );

        if( aCommand != ID_POPUP_PCB_MOVE_TRACK_SEGMENT )
        {
            Collect_TrackSegmentsToDrag( GetBoard(), aTrack->GetStart(),
                                         aTrack->GetLayerSet(),
                                         aTrack->GetNetCode(), aTrack->GetWidth() / 2 );
        }

        PosInit = aTrack->GetStart();
    }
    else
    {
        STATUS_FLAGS diag = aTrack->IsPointOnEnds( GetCrossHairPosition(), -1 );
        wxPoint pos;

        switch( aCommand )
        {
        case ID_POPUP_PCB_MOVE_TRACK_SEGMENT:   // Move segment
            aTrack->SetFlags( IS_DRAGGED | ENDPOINT | STARTPOINT );
            AddSegmentToDragList( aTrack->GetFlags(), aTrack );
            break;

        case ID_POPUP_PCB_DRAG_TRACK_SEGMENT:   // drag a segment
            pos = aTrack->GetStart();
            Collect_TrackSegmentsToDrag( GetBoard(), pos, aTrack->GetLayerSet(),
                                         aTrack->GetNetCode(), aTrack->GetWidth() / 2 );
            pos = aTrack->GetEnd();
            aTrack->SetFlags( IS_DRAGGED | ENDPOINT | STARTPOINT );
            Collect_TrackSegmentsToDrag( GetBoard(), pos, aTrack->GetLayerSet(),
                                         aTrack->GetNetCode(), aTrack->GetWidth() / 2 );
            break;

        case ID_POPUP_PCB_MOVE_TRACK_NODE:  // Drag via or move node
            pos = (diag & STARTPOINT) ? aTrack->GetStart() : aTrack->GetEnd();
            Collect_TrackSegmentsToDrag( GetBoard(), pos, aTrack->GetLayerSet(),
                                         aTrack->GetNetCode(), aTrack->GetWidth() / 2 );
            PosInit = pos;
            break;
        }

        aTrack->SetFlags( IS_DRAGGED );
    }

    // Prepare the Undo command
    ITEM_PICKER picker( aTrack, UR_CHANGED );
    picker.SetLink( aTrack->Clone() );
    s_ItemsListPicker.PushItem( picker );

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        TRACK* draggedtrack = g_DragSegmentList[ii].m_Track;
        picker.SetItem( draggedtrack );
        picker.SetLink( draggedtrack->Clone() );
        s_ItemsListPicker.PushItem( picker );
        draggedtrack = (TRACK*) picker.GetLink();
        draggedtrack->SetStatus( 0 );
        draggedtrack->ClearFlags();
    }

    s_LastPos = PosInit;
    m_canvas->SetMouseCapture( Show_MoveNode, Abort_MoveTrack );

    GetBoard()->SetHighLightNet( aTrack->GetNetCode() );
    GetBoard()->HighLightON();

    GetBoard()->DrawHighLight( m_canvas, aDC, GetBoard()->GetHighLightNetCode() );
    m_canvas->CallMouseCapture( aDC, wxDefaultPosition, true );

    UndrawAndMarkSegmentsToDrag( m_canvas, aDC );
}


void PCB_EDIT_FRAME::Start_DragTrackSegmentAndKeepSlope( TRACK* track, wxDC*  DC )
{
    TRACK* TrackToStartPoint = NULL;
    TRACK* TrackToEndPoint   = NULL;
    bool   error = false;

    if( !track )
        return;

    // TODO: Use clenup functions to merge collinear segments if track
    // is connected to a collinear segment.

    s_StartSegmentPresent = s_EndSegmentPresent = true;

    if( ( track->start == NULL ) || ( track->start->Type() == PCB_TRACE_T ) )
        TrackToStartPoint = track->GetTrack( GetBoard()->m_Track, NULL, ENDPOINT_START, true, false );

    //  Test if more than one segment is connected to this point
    if( TrackToStartPoint )
    {
        TrackToStartPoint->SetState( BUSY, true );

        if( ( TrackToStartPoint->Type() == PCB_VIA_T )
           || track->GetTrack( GetBoard()->m_Track, NULL, ENDPOINT_START, true, false ) )
            error = true;

        TrackToStartPoint->SetState( BUSY, false );
    }

    if( ( track->end == NULL ) || ( track->end->Type() == PCB_TRACE_T ) )
        TrackToEndPoint = track->GetTrack( GetBoard()->m_Track, NULL, ENDPOINT_END, true, false );

    //  Test if more than one segment is connected to this point
    if( TrackToEndPoint )
    {
        TrackToEndPoint->SetState( BUSY, true );

        if( (TrackToEndPoint->Type() == PCB_VIA_T)
           || track->GetTrack( GetBoard()->m_Track, NULL, ENDPOINT_END, true, false ) )
            error = true;

        TrackToEndPoint->SetState( BUSY, false );
    }

    if( error )
    {
        DisplayError( this,
                      _( "Unable to drag this segment: too many segments connected" ) );
        return;
    }

    if( !TrackToStartPoint || ( TrackToStartPoint->Type() != PCB_TRACE_T ) )
        s_StartSegmentPresent = false;

    if( !TrackToEndPoint || ( TrackToEndPoint->Type() != PCB_TRACE_T ) )
        s_EndSegmentPresent = false;

    // Change high light net: the new one will be highlighted
    GetBoard()->PushHighLight();

    if( GetBoard()->IsHighLightNetON() )
        HighLight( DC );

    EraseDragList();

    track->SetFlags( IS_DRAGGED );

    if( TrackToStartPoint )
    {
        STATUS_FLAGS flag = STARTPOINT;

        if( track->GetStart() != TrackToStartPoint->GetStart() )
            flag = ENDPOINT;

        AddSegmentToDragList( flag, TrackToStartPoint );
        track->SetFlags( STARTPOINT );
    }

    if( TrackToEndPoint )
    {
        STATUS_FLAGS flag = STARTPOINT;

        if( track->GetEnd() != TrackToEndPoint->GetStart() )
            flag = ENDPOINT;

        AddSegmentToDragList( flag, TrackToEndPoint );
        track->SetFlags( ENDPOINT );
    }

    AddSegmentToDragList( track->GetFlags(), track );

    UndrawAndMarkSegmentsToDrag( m_canvas, DC );


    PosInit   = GetCrossHairPosition();
    s_LastPos = GetCrossHairPosition();
    m_canvas->SetMouseCapture( Show_Drag_Track_Segment_With_Cte_Slope, Abort_MoveTrack );

    GetBoard()->SetHighLightNet( track->GetNetCode() );
    GetBoard()->HighLightON();
    GetBoard()->DrawHighLight( m_canvas, DC, GetBoard()->GetHighLightNetCode() );

    // Prepare the Undo command
    ITEM_PICKER picker( NULL, UR_CHANGED );

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        TRACK* draggedtrack = g_DragSegmentList[ii].m_Track;
        picker.SetItem( draggedtrack);
        picker.SetLink ( draggedtrack->Clone() );
        s_ItemsListPicker.PushItem( picker );
        draggedtrack = (TRACK*) picker.GetLink();
        draggedtrack->SetStatus( 0 );
        draggedtrack->ClearFlags();
    }

    if( !InitialiseDragParameters() )
    {
        DisplayError( this, _( "Unable to drag this segment: two collinear segments" ) );
        m_canvas->SetMouseCaptureCallback( NULL );
        Abort_MoveTrack( m_canvas, DC );
        return;
    }
}


// Place a dragged (or moved) track segment or via
bool PCB_EDIT_FRAME::PlaceDraggedOrMovedTrackSegment( TRACK* Track, wxDC* DC )
{
    int        errdrc;

    if( Track == NULL )
        return false;

    int current_net_code = Track->GetNetCode();

    // DRC control:
    if( g_Drc_On )
    {
        errdrc = m_drc->Drc( Track, GetBoard()->m_Track );

        if( errdrc == BAD_DRC )
            return false;

        // Redraw the dragged segments
        for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
        {
            errdrc = m_drc->Drc( g_DragSegmentList[ii].m_Track, GetBoard()->m_Track );

            if( errdrc == BAD_DRC )
                return false;
        }
    }

    // DRC Ok: place track segments
    Track->ClearFlags();
    Track->SetState( IN_EDIT, false );

    // Draw dragged tracks
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Track;
        Track->SetState( IN_EDIT, false );
        Track->ClearFlags();

        /* Test the connections modified by the move
         *  (only pad connection must be tested, track connection will be
         * tested by TestNetConnection() ) */
        LSET layerMask( Track->GetLayer() );

        Track->start = GetBoard()->GetPadFast( Track->GetStart(), layerMask );

        if( Track->start )
            Track->SetState( BEGIN_ONPAD, true );
        else
            Track->SetState( BEGIN_ONPAD, false );

        Track->end = GetBoard()->GetPadFast( Track->GetEnd(), layerMask );

        if( Track->end )
            Track->SetState( END_ONPAD, true );
        else
            Track->SetState( END_ONPAD, false );
    }

    EraseDragList();

    SaveCopyInUndoList( s_ItemsListPicker, UR_UNSPECIFIED );
    s_ItemsListPicker.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items

    GetBoard()->PopHighLight();

    OnModify();
    m_canvas->SetMouseCapture( NULL, NULL );

    if( current_net_code > 0 )
        TestNetConnection( DC, current_net_code );

    m_canvas->Refresh();

    return true;
}
