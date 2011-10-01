/**
 * @file move_or_drag_track.cpp
 * @brief Track editing routines to move and drag track segments or node.
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "wxPcbStruct.h"
#include "trigo.h"
#include "macros.h"
#include "gr_basic.h"
#include "pcbcommon.h"

#include "class_board.h"

#include "pcbnew.h"
#include "drc_stuff.h"
#include "drag.h"
#include "pcbnew_id.h"
#include "protos.h"


static void Show_MoveNode( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                           bool aErase );
static void Show_Drag_Track_Segment_With_Cte_Slope( EDA_DRAW_PANEL* aPanel,
                                                    wxDC*           aDC,
                                                    const wxPoint&  aPosition,
                                                    bool            aErase );
static void Abort_MoveTrack( EDA_DRAW_PANEL* Panel, wxDC* DC );
static bool InitialiseDragParameters();


static wxPoint PosInit, s_LastPos;
static TRACK*  NewTrack;    /* New track or track being moved. */
static int     NbPtNewTrack;
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


/** Abort function for commands drag, copy or move track
 */
static void Abort_MoveTrack( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    TRACK* NextS;
    int    ii;
    BOARD * pcb = ( (PCB_EDIT_FRAME*) Panel->GetParent() )->GetBoard();

    /* Erase the current drawings */
    wxPoint             oldpos = Panel->GetScreen()->GetCrossHairPosition();

    Panel->GetScreen()->SetCrossHairPosition( PosInit );

    if( Panel->IsMouseCaptured() )
        Panel->m_mouseCaptureCallback( Panel, DC, wxDefaultPosition, true );

    Panel->GetScreen()->SetCrossHairPosition( oldpos );
    pcb->HighLightOFF();
    pcb->DrawHighLight( Panel, DC, pcb->GetHighLightNetCode() );

    if( NewTrack )
    {
        if( NewTrack->IsNew() )
        {
            for( ii = 0; ii < NbPtNewTrack; ii++, NewTrack = NextS )
            {
                if( NewTrack == NULL )
                    break;

                NextS = NewTrack->Next();
                delete NewTrack;
            }
        }
        else    /* Move existing trace.  */
        {
            TRACK* Track = NewTrack;
            int    dx    = s_LastPos.x - PosInit.x;
            int    dy    = s_LastPos.y - PosInit.y;

            for( ii = 0; ii < NbPtNewTrack; ii++, Track = Track->Next() )
            {
                if( Track == NULL )
                    break;

                Track->m_Start.x -= dx;
                Track->m_Start.y -= dy;

                Track->m_End.x -= dx;
                Track->m_End.y -= dy;

                Track->m_Flags = 0;
            }

            DrawTraces( Panel, DC, NewTrack, NbPtNewTrack, GR_OR );
        }

        NewTrack = NULL;
    }

    ( (PCB_EDIT_FRAME*) Panel->GetParent() )->SetCurItem( NULL );

    /* Undo move and redraw trace segments. */
    for( unsigned jj=0 ; jj < g_DragSegmentList.size(); jj++ )
    {
        TRACK* Track = g_DragSegmentList[jj].m_Segm;
        g_DragSegmentList[jj].SetInitialValues();
        Track->SetState( IN_EDIT, OFF );
        Track->m_Flags = 0;
        Track->Draw( Panel, DC, GR_OR );
    }

    // Clear the undo picker list:
    s_ItemsListPicker.ClearListAndDeleteItems();

    pcb->PopHighLight();

    if( pcb->IsHighLightNetON() )
        pcb->DrawHighLight( Panel, DC, pcb->GetHighLightNetCode() );

    EraseDragList();
    Panel->SetMouseCapture( NULL, NULL );

    Panel->Refresh();
}


/* Redraw the moved node according to the mouse cursor position */
static void Show_MoveNode( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                           bool aErase )
{
    int          ii;
    wxPoint      moveVector;
    TRACK*       Track;
    BASE_SCREEN* screen = aPanel->GetScreen();
    int          track_fill_copy = DisplayOpt.DisplayPcbTrackFill;
    int          draw_mode = GR_XOR | GR_SURBRILL;

    DisplayOpt.DisplayPcbTrackFill = false;

    aErase = true;

    /* erase the current moved track segments from screen */
    if( aErase )
    {
        if( NewTrack )
            DrawTraces( aPanel, aDC, NewTrack, NbPtNewTrack, draw_mode );
    }

    /* set the new track coordinates */
    wxPoint Pos = screen->GetCrossHairPosition();

    moveVector = Pos - s_LastPos;
    s_LastPos  = Pos;

    ii    = NbPtNewTrack;
    Track = NewTrack;

    for( ; (ii > 0) && (Track != NULL); ii--, Track = Track->Next() )
    {
        if( Track->m_Flags & STARTPOINT )
            Track->m_Start += moveVector;

        if( Track->m_Flags & ENDPOINT )
            Track->m_End += moveVector;
    }

    /* Redraw the current moved track segments */
    DrawTraces( aPanel, aDC, NewTrack, NbPtNewTrack, draw_mode );

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;

        if( aErase )
            Track->Draw( aPanel, aDC, draw_mode );

        if( Track->m_Flags & STARTPOINT )
            Track->m_Start += moveVector;

        if( Track->m_Flags & ENDPOINT )
            Track->m_End += moveVector;

        Track->Draw( aPanel, aDC, draw_mode );
    }

    DisplayOpt.DisplayPcbTrackFill = track_fill_copy;

    // Display track length
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) aPanel->GetParent();
    Track->DisplayInfo( frame );
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
    BASE_SCREEN* screen = aPanel->GetScreen();
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
    Track = g_DragSegmentList[ii].m_Segm;

    if( Track == NULL )
        return;

    ii--;

    if( ii >= 0)
    {
        if( s_EndSegmentPresent )
        {
            // Get the segment connected to the end point
            tSegmentToEnd   = g_DragSegmentList[ii].m_Segm;
            ii--;
        }

        if( s_StartSegmentPresent )
        {
            // Get the segment connected to the start point
            if( ii >= 0 )
                tSegmentToStart = g_DragSegmentList[ii].m_Segm;
        }
    }

    int draw_mode = GR_XOR | GR_SURBRILL;

    /* Undraw the current moved track segments before modification*/

//  if( erase )
    {
        Track->Draw( aPanel, aDC, draw_mode );

        if( tSegmentToStart )
            tSegmentToStart->Draw( aPanel, aDC, draw_mode );

        if( tSegmentToEnd )
            tSegmentToEnd->Draw( aPanel, aDC, draw_mode );
    }

    /* Compute the new track segment position */
    wxPoint Pos = screen->GetCrossHairPosition();

    dx = Pos.x - s_LastPos.x;
    dy = Pos.y - s_LastPos.y;

    //move the line by dx and dy
    tx1 = (double) ( Track->m_Start.x + dx );
    ty1 = (double) ( Track->m_Start.y + dy );
    tx2 = (double) ( Track->m_End.x + dx );
    ty2 = (double) ( Track->m_End.y + dy );

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
        Track->m_Start.x = wxRound( xi1 );
        Track->m_Start.y = wxRound( yi1 );
        Track->m_End.x   = wxRound( xi2 );
        Track->m_End.y   = wxRound( yi2 );

        if( tSegmentToEnd )
        {
            if( tSegmentToEnd->m_Flags & STARTPOINT )
                tSegmentToEnd->m_Start = Track->m_End;
            else
                tSegmentToEnd->m_End = Track->m_End;
        }

        if( tSegmentToStart )
        {
            if( tSegmentToStart->m_Flags & STARTPOINT )
                tSegmentToStart->m_Start = Track->m_Start;
            else
                tSegmentToStart->m_End = Track->m_Start;
        }
    }

    Track->Draw( aPanel, aDC, draw_mode );

    if( tSegmentToStart )
        tSegmentToStart->Draw( aPanel, aDC, draw_mode );

    if( tSegmentToEnd )
        tSegmentToEnd->Draw( aPanel, aDC, draw_mode );

    // Display track length
    PCB_BASE_FRAME* frame = (PCB_BASE_FRAME*) aPanel->GetParent();
    Track->DisplayInfo( frame );
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
    Track = g_DragSegmentList[ii].m_Segm;
    if( Track == NULL )
        return false;

    ii--;

    if( ii >= 0)
    {
        if( s_EndSegmentPresent )
        {
            tSegmentToEnd = g_DragSegmentList[ii].m_Segm;  // Get the segment connected to
                                                           // the end point
            ii--;
        }

        if( s_StartSegmentPresent )
        {
            if( ii  >= 0 )
                tSegmentToStart = g_DragSegmentList[ii].m_Segm;  // Get the segment connected to
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
        if( tSegmentToStart->m_Flags & ENDPOINT )
        {
            tx1 = (double) tSegmentToStart->m_Start.x;
            ty1 = (double) tSegmentToStart->m_Start.y;
            tx2 = (double) tSegmentToStart->m_End.x;
            ty2 = (double) tSegmentToStart->m_End.y;
        }
        else
        {
            tx1 = (double) tSegmentToStart->m_End.x;
            ty1 = (double) tSegmentToStart->m_End.y;
            tx2 = (double) tSegmentToStart->m_Start.x;
            ty2 = (double) tSegmentToStart->m_Start.y;
        }
    }
    else // move the start point on a line starting at Track->m_Start, and perpendicular to Track
    {
        tx1 = (double) Track->m_Start.x;
        ty1 = (double) Track->m_Start.y;
        tx2 = (double) Track->m_End.x;
        ty2 = (double) Track->m_End.y;
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
        if( tSegmentToEnd->m_Flags & STARTPOINT )
        {
            tx1 = (double) tSegmentToEnd->m_Start.x;
            ty1 = (double) tSegmentToEnd->m_Start.y;
            tx2 = (double) tSegmentToEnd->m_End.x;
            ty2 = (double) tSegmentToEnd->m_End.y;
        }
        else
        {
            tx1 = (double) tSegmentToEnd->m_End.x;
            ty1 = (double) tSegmentToEnd->m_End.y;
            tx2 = (double) tSegmentToEnd->m_Start.x;
            ty2 = (double) tSegmentToEnd->m_Start.y;
        }
    }
    else // move the start point on a line starting at Track->m_End, and perpendicular to Track
    {
        tx1 = (double) Track->m_End.x;
        ty1 = (double) Track->m_End.y;
        tx2 = (double) Track->m_Start.x;
        ty2 = (double) Track->m_Start.y;
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
    tx1 = (double) Track->m_Start.x;
    ty1 = (double) Track->m_Start.y;
    tx2 = (double) Track->m_End.x;
    ty2 = (double) Track->m_End.y;

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

    NewTrack     = NULL;
    NbPtNewTrack = 0;
    EraseDragList();

    /* Change highlighted net: the new one will be highlighted */
    GetBoard()->PushHighLight();

    if( GetBoard()->IsHighLightNetON() )
        High_Light( aDC );

    PosInit = GetScreen()->GetCrossHairPosition();

    if( aTrack->Type() == PCB_VIA_T )     // For a via: always drag it
    {
        aTrack->m_Flags = IS_DRAGGED | STARTPOINT | ENDPOINT;

        if( aCommand != ID_POPUP_PCB_MOVE_TRACK_SEGMENT )
        {
            Collect_TrackSegmentsToDrag( DrawPanel, aDC, aTrack->m_Start,
                                         aTrack->ReturnMaskLayer(),
                                         aTrack->GetNet() );
        }

        NewTrack     = aTrack;
        NbPtNewTrack = 1;
        PosInit = aTrack->m_Start;
    }
    else
    {
        int     diag = aTrack->IsPointOnEnds( GetScreen()->GetCrossHairPosition(), -1 );
        wxPoint pos;

        switch( aCommand )
        {
        case ID_POPUP_PCB_MOVE_TRACK_SEGMENT:   // Move segment
            aTrack->m_Flags |= IS_DRAGGED | ENDPOINT | STARTPOINT;
            AddSegmentToDragList( DrawPanel, aDC, aTrack->m_Flags, aTrack );
            break;

        case ID_POPUP_PCB_DRAG_TRACK_SEGMENT:   // drag a segment
            pos = aTrack->m_Start;
            Collect_TrackSegmentsToDrag( DrawPanel, aDC, pos,
                                         aTrack->ReturnMaskLayer(),
                                         aTrack->GetNet() );
            pos = aTrack->m_End;
            aTrack->m_Flags |= IS_DRAGGED | ENDPOINT | STARTPOINT;
            Collect_TrackSegmentsToDrag( DrawPanel, aDC, pos,
                                         aTrack->ReturnMaskLayer(),
                                         aTrack->GetNet() );
            break;

        case ID_POPUP_PCB_MOVE_TRACK_NODE:  // Drag via or move node
            pos = (diag & STARTPOINT) ? aTrack->m_Start : aTrack->m_End;
            Collect_TrackSegmentsToDrag( DrawPanel, aDC, pos,
                                         aTrack->ReturnMaskLayer(),
                                         aTrack->GetNet() );
            PosInit = pos;
            break;
        }

        aTrack->m_Flags |= IS_DRAGGED;
    }

    // Prepare the Undo command
    ITEM_PICKER picker( aTrack, UR_CHANGED );
    picker.m_Link = aTrack->Copy();
    s_ItemsListPicker.PushItem( picker );

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        TRACK* draggedtrack = g_DragSegmentList[ii].m_Segm;
        picker.m_PickedItem = draggedtrack;
        picker.m_Link = draggedtrack->Copy();
        s_ItemsListPicker.PushItem( picker );
        draggedtrack = (TRACK*) picker.m_Link;
        draggedtrack->SetStatus( 0 );
        draggedtrack->m_Flags = 0;
    }

    s_LastPos = PosInit;
    DrawPanel->SetMouseCapture( Show_MoveNode, Abort_MoveTrack );

    GetBoard()->SetHighLightNet( aTrack->GetNet() );
    GetBoard()->HighLightON();

    GetBoard()->DrawHighLight( DrawPanel, aDC, GetBoard()->GetHighLightNetCode() );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, wxDefaultPosition, true );
}


#if 0

// @todo: This function is broken: does not handle pointers to pads for start
// and end and flags relative to these pointers
void SortTrackEndPoints( TRACK* track )
{
    // sort the track endpoints -- should not matter in terms of drawing
    // or producing the pcb -- but makes doing comparisons easier.
    int dx = track->m_End.x - track->m_Start.x;

    if( dx )
    {
        if( track->m_Start.x > track->m_End.x )
        {
            EXCHG( track->m_Start, track->m_End );
        }
    }
    else
    {
        if( track->m_Start.y > track->m_End.y )
        {
            EXCHG( track->m_Start, track->m_End );
        }
    }
}


bool PCB_EDIT_FRAME::MergeCollinearTracks( TRACK* track, wxDC* DC, int end )
{
    testtrack = track->GetTrace( GetBoard()->m_Track, NULL, end );

    if( testtrack )
    {
        SortTrackEndPoints( track );
        SortTrackEndPoints( testtrack );
        int dx  = track->m_End.x - track->m_Start.x;
        int dy  = track->m_End.y - track->m_Start.y;
        int tdx = testtrack->m_End.x - testtrack->m_Start.x;
        int tdy = testtrack->m_End.y - testtrack->m_Start.y;

        if( ( dy * tdx == dx * tdy && dy != 0 && dx != 0 && tdy != 0 && tdx != 0 )  /* angle, same slope */
           || ( dy == 0 && tdy == 0 && dx * tdx ) /*horizontal */
           || ( dx == 0 && tdx == 0 && dy * tdy ) /*vertical */
            )
        {
            if( track->m_Start == testtrack->m_Start || track->m_End == testtrack->m_Start )
            {
                if( ( dx * tdx && testtrack->m_End.x > track->m_End.x )
                   ||( dy * tdy && testtrack->m_End.y > track->m_End.y ) )
                {
                    track->m_End = testtrack->m_End;

                    Delete_Segment( DC, testtrack );
                    return true;
                }
            }

            if( track->m_Start == testtrack->m_End || track->m_End == testtrack->m_End )
            {
                if( ( dx * tdx && testtrack->m_Start.x < track->m_Start.x )
                   || ( dy * tdy && testtrack->m_Start.y < track->m_Start.y ) )
                {
                    track->m_Start = testtrack->m_Start;

                    Delete_Segment( DC, testtrack );
                    return true;
                }
            }
        }
    }

    return false;
}


#endif


void PCB_EDIT_FRAME::Start_DragTrackSegmentAndKeepSlope( TRACK* track, wxDC*  DC )
{
    TRACK* TrackToStartPoint = NULL;
    TRACK* TrackToEndPoint   = NULL;
    bool   error = false;

    if( !track )
        return;


#if 0

    // Broken functions: see comments
    while( MergeCollinearTracks( track, DC, START ) )
    {
    };

    while( MergeCollinearTracks( track, DC, END ) )
    {
    };
#endif

    s_StartSegmentPresent = s_EndSegmentPresent = true;

    if( ( track->start == NULL ) || ( track->start->Type() == PCB_TRACE_T ) )
        TrackToStartPoint = track->GetTrace( GetBoard()->m_Track, NULL, START );

    //  Test if more than one segment is connected to this point
    if( TrackToStartPoint )
    {
        TrackToStartPoint->SetState( BUSY, ON );

        if( ( TrackToStartPoint->Type() == PCB_VIA_T )
           || track->GetTrace( GetBoard()->m_Track, NULL, START ) )
            error = true;

        TrackToStartPoint->SetState( BUSY, OFF );
    }

    if( ( track->end == NULL ) || ( track->end->Type() == PCB_TRACE_T ) )
        TrackToEndPoint = track->GetTrace( GetBoard()->m_Track, NULL, END );

    //  Test if more than one segment is connected to this point
    if( TrackToEndPoint )
    {
        TrackToEndPoint->SetState( BUSY, ON );

        if( (TrackToEndPoint->Type() == PCB_VIA_T)
           || track->GetTrace( GetBoard()->m_Track, NULL, END ) )
            error = true;

        TrackToEndPoint->SetState( BUSY, OFF );
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

    /* Change high light net: the new one will be highlighted */
    GetBoard()->PushHighLight();

    if( GetBoard()->IsHighLightNetON() )
        High_Light( DC );

    EraseDragList();

    NewTrack = NULL;
    NbPtNewTrack   = 0;
    track->m_Flags = IS_DRAGGED;

    if( TrackToStartPoint )
    {
        int flag = STARTPOINT;

        if( track->m_Start != TrackToStartPoint->m_Start )
            flag = ENDPOINT;

        AddSegmentToDragList( DrawPanel, DC, flag, TrackToStartPoint );
        track->m_Flags |= STARTPOINT;
    }

    if( TrackToEndPoint )
    {
        int flag = STARTPOINT;

        if( track->m_End != TrackToEndPoint->m_Start )
            flag = ENDPOINT;

        AddSegmentToDragList( DrawPanel, DC, flag, TrackToEndPoint );
        track->m_Flags |= ENDPOINT;
    }

    AddSegmentToDragList( DrawPanel, DC, track->m_Flags, track );


    PosInit   = GetScreen()->GetCrossHairPosition();
    s_LastPos = GetScreen()->GetCrossHairPosition();
    DrawPanel->SetMouseCapture( Show_Drag_Track_Segment_With_Cte_Slope, Abort_MoveTrack );

    GetBoard()->SetHighLightNet( track->GetNet() );
    GetBoard()->HighLightON();
    GetBoard()->DrawHighLight( DrawPanel, DC, GetBoard()->GetHighLightNetCode() );

    // Prepare the Undo command
    ITEM_PICKER picker( NULL, UR_CHANGED );

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        TRACK* draggedtrack = g_DragSegmentList[ii].m_Segm;
        picker.m_PickedItem = draggedtrack;
        picker.m_Link = draggedtrack->Copy();
        s_ItemsListPicker.PushItem( picker );
        draggedtrack = (TRACK*) picker.m_Link;
        draggedtrack->SetStatus( 0 );
        draggedtrack->m_Flags = 0;
    }

    if( !InitialiseDragParameters() )
    {
        DisplayError( this, _( "Unable to drag this segment: two collinear segments" ) );
        DrawPanel->m_mouseCaptureCallback = NULL;
        Abort_MoveTrack( DrawPanel, DC );
        return;
    }
}


/* Place a dragged (or moved) track segment or via */
bool PCB_EDIT_FRAME::PlaceDraggedOrMovedTrackSegment( TRACK* Track, wxDC* DC )
{
    int        errdrc;

    if( Track == NULL )
        return false;

    int current_net_code = Track->GetNet();

    // DRC control:
    if( Drc_On )
    {
        errdrc = m_drc->Drc( Track, GetBoard()->m_Track );

        if( errdrc == BAD_DRC )
            return false;

        /* Redraw the dragged segments */
        for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
        {
            errdrc = m_drc->Drc( g_DragSegmentList[ii].m_Segm, GetBoard()->m_Track );

            if( errdrc == BAD_DRC )
                return false;
        }
    }

    int draw_mode = GR_OR | GR_SURBRILL;

    // DRC Ok: place track segments
    Track->m_Flags = 0;
    Track->SetState( IN_EDIT, OFF );
    Track->Draw( DrawPanel, DC, draw_mode );

    /* Draw dragged tracks */
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;
        Track->SetState( IN_EDIT, OFF );
        Track->m_Flags = 0;
        Track->Draw( DrawPanel, DC, draw_mode );

        /* Test the connections modified by the move
         *  (only pad connection must be tested, track connection will be
         * tested by TestNetConnection() ) */
        int layerMask = g_TabOneLayerMask[Track->GetLayer()];
        Track->start = GetBoard()->GetPadFast( Track->m_Start, layerMask );

        if( Track->start )
            Track->SetState( BEGIN_ONPAD, ON );
        else
            Track->SetState( BEGIN_ONPAD, OFF );

        Track->end = GetBoard()->GetPadFast( Track->m_End, layerMask );

        if( Track->end )
            Track->SetState( END_ONPAD, ON );
        else
            Track->SetState( END_ONPAD, OFF );
    }

    EraseDragList();

    SaveCopyInUndoList( s_ItemsListPicker, UR_UNSPECIFIED );
    s_ItemsListPicker.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items

    if( GetBoard()->IsHighLightNetON() )
        High_Light( DC );

    GetBoard()->PopHighLight();

    if( GetBoard()->IsHighLightNetON() )
        GetBoard()->DrawHighLight( DrawPanel, DC, GetBoard()->GetHighLightNetCode() );

    OnModify();
    DrawPanel->SetMouseCapture( NULL, NULL );

    DrawPanel->Refresh();

    if( current_net_code > 0 )
        TestNetConnection( DC, current_net_code );

    return true;
}
