/****************************************************/
/*              Track editing						*/
/* routines to move and drag track segments or node */
/****************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "trigo.h"

#include "drag.h"
#include "pcbnew_id.h"

#include "protos.h"


static void Show_MoveNode( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void Show_Drag_Track_Segment_With_Cte_Slope( WinEDA_DrawPanel* panel,
                                                    wxDC*             DC,
                                                    bool              erase );
static void Abort_MoveTrack( WinEDA_DrawPanel* Panel, wxDC* DC );
static bool InitialiseDragParameters();


static wxPoint PosInit, s_LastPos;
static TRACK*  NewTrack;    /* New track or track being moved. */
static int     NbPtNewTrack;
static int     Old_HighLigth_NetCode;
static bool    Old_HighLigt_Status;
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
static void Abort_MoveTrack( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    TRACK* NextS;
    int    ii;

    /* Erase the current drawings */
    wxPoint             oldpos = Panel->GetScreen()->m_Curseur;

    Panel->GetScreen()->m_Curseur = PosInit;

    if( Panel->ManageCurseur )
        Panel->ManageCurseur( Panel, DC, true );

    Panel->GetScreen()->m_Curseur = oldpos;
    g_HighLight_Status = false;
    ( (WinEDA_PcbFrame*) Panel->GetParent() )->GetBoard()->DrawHighLight(
        Panel,
        DC,
        g_HighLight_NetCode );

    if( NewTrack )
    {
        if( NewTrack->m_Flags & IS_NEW )
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

            Trace_Une_Piste( Panel, DC, NewTrack, NbPtNewTrack, GR_OR );
        }

        NewTrack = NULL;
    }

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    ( (WinEDA_PcbFrame*) Panel->GetParent() )->SetCurItem( NULL );

    /* Undo move and redraw trace segments. */
    for( unsigned jj=0 ; jj < g_DragSegmentList.size(); jj++ )
    {
        TRACK* Track = g_DragSegmentList[jj].m_Segm;
        g_DragSegmentList[jj].SetInitialValues();
        Track->SetState( EDIT, OFF );
        Track->m_Flags = 0;
        Track->Draw( Panel, DC, GR_OR );
    }

    // Clear the undo picker list:
    s_ItemsListPicker.ClearListAndDeleteItems();

    g_HighLight_NetCode = Old_HighLigth_NetCode;
    g_HighLight_Status   = Old_HighLigt_Status;
    if( g_HighLight_Status )
        ( (WinEDA_PcbFrame*) Panel->GetParent() )->GetBoard()->DrawHighLight(
            Panel, DC, g_HighLight_NetCode );

    EraseDragList();
}


/* Redraw the moved node according to the mouse cursor position */
static void Show_MoveNode( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    int          ii;
    wxPoint      moveVector;
    TRACK*       Track;
    BASE_SCREEN* screen = panel->GetScreen();
    int          track_fill_copy = DisplayOpt.DisplayPcbTrackFill;
    int          draw_mode = GR_XOR | GR_SURBRILL;

    DisplayOpt.DisplayPcbTrackFill = false;

    erase = true;

    /* erase the current moved track segments from screen */
    if( erase )
    {
        if( NewTrack )
            Trace_Une_Piste( panel, DC, NewTrack, NbPtNewTrack, draw_mode );
    }

    /* set the new track coordinates */
    wxPoint Pos = screen->m_Curseur;

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
    Trace_Une_Piste( panel, DC, NewTrack, NbPtNewTrack, draw_mode );

    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;
        if( erase )
            Track->Draw( panel, DC, draw_mode );

        if( Track->m_Flags & STARTPOINT )
            Track->m_Start += moveVector;

        if( Track->m_Flags & ENDPOINT )
            Track->m_End += moveVector;

        Track->Draw( panel, DC, draw_mode );
    }

    DisplayOpt.DisplayPcbTrackFill = track_fill_copy;

    // Display track length
    WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) panel->GetParent();
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
static void Show_Drag_Track_Segment_With_Cte_Slope( WinEDA_DrawPanel* panel,
                                                    wxDC* DC, bool erase )
{
    double       xi1 = 0, yi1 = 0, xi2 = 0, yi2 = 0;    // calculated
                                                        // intersection points
    double       tx1, tx2, ty1, ty2;                    // temporary storage of
                                                        // points
    int          dx, dy;
    BASE_SCREEN* screen = panel->GetScreen();
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

//	if( erase )
    {
        Track->Draw( panel, DC, draw_mode );
        if( tSegmentToStart )
            tSegmentToStart->Draw( panel, DC, draw_mode );

        if( tSegmentToEnd )
            tSegmentToEnd->Draw( panel, DC, draw_mode );
    }

    /* Compute the new track segment position */
    wxPoint Pos = screen->m_Curseur;

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

    Track->Draw( panel, DC, draw_mode );
    if( tSegmentToStart )
        tSegmentToStart->Draw( panel, DC, draw_mode );
    if( tSegmentToEnd )
        tSegmentToEnd->Draw( panel, DC, draw_mode );

    // Display track length
    WinEDA_BasePcbFrame* frame = (WinEDA_BasePcbFrame*) panel->GetParent();
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
            tSegmentToEnd   = g_DragSegmentList[ii].m_Segm;  // Get the segment
                                                        // connected to the end
                                                        // point
            ii--;
        }
        if( s_StartSegmentPresent )
        {
            if( ii  >= 0 )
                tSegmentToStart = g_DragSegmentList[ii].m_Segm;  // Get the segment
                                                            // connected to the
                                                            // start point
        }
    }

    //would be nice to eliminate collinear segments here, so we don't
    //have to deal with that annoying "Unable to drag this segment: two
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
    else // move the start point on a line starting at Track->m_Start, and
         // perpendicular to Track
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
    else // move the start point on a line starting at Track->m_End, and
         // perpendicular to Track
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
        s_MovingSegmentVertical = true;      //signal vertical line
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
        if( !s_EndPointVertical
           && ( s_MovingSegmentSlope == s_EndSegmentSlope ) )
            return false;
        if( !s_StartPointVertical
           && ( s_MovingSegmentSlope == s_StartSegmentSlope ) )
            return false;
    }

    return true;
}


/* Init parameters to move one node:
 *  a via or/and a terminal point of a track segment
 *  The terminal point of other connected segments (if any) are moved too.
 */
void WinEDA_PcbFrame::Start_MoveOneNodeOrSegment( TRACK* track,
                                                  wxDC*  DC,
                                                  int    command )
{
    if( !track )
        return;

    NewTrack     = NULL;
    NbPtNewTrack = 0;
    EraseDragList();

    /* Change highlighted net: the new one will be highlighted */
    Old_HighLigt_Status   = g_HighLight_Status;
    Old_HighLigth_NetCode = g_HighLight_NetCode;
    if( g_HighLight_Status )
        High_Light( DC );
    PosInit = GetScreen()->m_Curseur;

    if( track->Type() == TYPE_VIA )     // For a via: always drag it
    {
        track->m_Flags = IS_DRAGGED | STARTPOINT | ENDPOINT;
        if( command != ID_POPUP_PCB_MOVE_TRACK_SEGMENT )
        {
            Collect_TrackSegmentsToDrag( DrawPanel, DC, track->m_Start,
                                         track->ReturnMaskLayer(),
                                         track->GetNet() );
        }
        NewTrack     = track;
        NbPtNewTrack = 1;
        PosInit = track->m_Start;
    }
    else
    {
        int     diag = track->IsPointOnEnds( GetScreen()->m_Curseur, -1 );
        wxPoint pos;

        switch( command )
        {
        case ID_POPUP_PCB_MOVE_TRACK_SEGMENT:   // Move segment
            track->m_Flags |= IS_DRAGGED | ENDPOINT | STARTPOINT;
            AddSegmentToDragList( DrawPanel, DC, track->m_Flags, track );
            break;

        case ID_POPUP_PCB_DRAG_TRACK_SEGMENT:   // drag a segment
            pos = track->m_Start;
            Collect_TrackSegmentsToDrag( DrawPanel, DC, pos,
                                         track->ReturnMaskLayer(),
                                         track->GetNet() );
            pos = track->m_End;
            track->m_Flags |= IS_DRAGGED | ENDPOINT | STARTPOINT;
            Collect_TrackSegmentsToDrag( DrawPanel, DC, pos,
                                         track->ReturnMaskLayer(),
                                         track->GetNet() );
            break;

        case ID_POPUP_PCB_MOVE_TRACK_NODE:  // Drag via or move node
            pos = (diag & STARTPOINT) ? track->m_Start : track->m_End;
            Collect_TrackSegmentsToDrag( DrawPanel, DC, pos,
                                         track->ReturnMaskLayer(),
                                         track->GetNet() );
            PosInit = pos;
            break;
        }

        track->m_Flags |= IS_DRAGGED;
    }

    // Prepare the Undo command
    ITEM_PICKER picker( track, UR_CHANGED );
    picker.m_Link = track->Copy();
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
    DrawPanel->ManageCurseur = Show_MoveNode;
    DrawPanel->ForceCloseManageCurseur = Abort_MoveTrack;

    g_HighLight_NetCode = track->GetNet();
    g_HighLight_Status   = true;

    GetBoard()->DrawHighLight( DrawPanel, DC, g_HighLight_NetCode );
    DrawPanel->ManageCurseur( DrawPanel, DC, true );
}


#if 0

// @todo: This function is broken: does not handle pointers to pads for start
// and end and flags relative to these pointers
void SortTrackEndPoints( TRACK* track )
{
    //sort the track endpoints -- should not matter in terms of drawing
    //or producing the pcb -- but makes doing comparisons easier.
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


/**
 * @todo: this function is broken, because it merge segments having different
 * width or without any connectivity test.
 * 2 collinear segments can be merged only in no other segment or via is
 * connected to the common point
 * and if they have the same width. See cleanup.cpp for merge functions,
 * and consider Marque_Une_Piste() to locate segments that can be merged
 */
bool WinEDA_PcbFrame::MergeCollinearTracks( TRACK* track, wxDC* DC, int end )
{
    testtrack = (TRACK*) Locate_Piste_Connectee( track,
                                                 GetBoard()->m_Track, NULL,
                                                 end );
    if( testtrack )
    {
        SortTrackEndPoints( track );
        SortTrackEndPoints( testtrack );
        int dx  = track->m_End.x - track->m_Start.x;
        int dy  = track->m_End.y - track->m_Start.y;
        int tdx = testtrack->m_End.x - testtrack->m_Start.x;
        int tdy = testtrack->m_End.y - testtrack->m_Start.y;

        if( ( dy * tdx == dx * tdy && dy != 0 && dx != 0 && tdy != 0 && tdx !=
             0 )                                  /* angle, same slope */
           || ( dy == 0 && tdy == 0 && dx * tdx ) /*horizontal */
           || ( dx == 0 && tdx == 0 && dy * tdy ) /*vertical */
            )
        {
            if( track->m_Start == testtrack->m_Start || track->m_End ==
                testtrack->m_Start )
            {
                if( ( dx * tdx && testtrack->m_End.x > track->m_End.x )
                   ||( dy * tdy && testtrack->m_End.y > track->m_End.y ) )
                {
                    track->m_End = testtrack->m_End;

                    Delete_Segment( DC, testtrack );
                    return true;
                }
            }
            if( track->m_Start == testtrack->m_End || track->m_End ==
                testtrack->m_End )
            {
                if( ( dx * tdx && testtrack->m_Start.x < track->m_Start.x )
                   ||( dy * tdy && testtrack->m_Start.y < track->m_Start.y ) )
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


void WinEDA_PcbFrame::Start_DragTrackSegmentAndKeepSlope( TRACK* track,
                                                          wxDC*  DC )
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

    if( ( track->start == NULL ) || ( track->start->Type() == TYPE_TRACK ) )
        TrackToStartPoint = Locate_Piste_Connectee( track,
                                                    GetBoard()->m_Track, NULL,
                                                    START );

    //  Test if more than one segment is connected to this point
    if( TrackToStartPoint )
    {
        TrackToStartPoint->SetState( BUSY, ON );
        if( ( TrackToStartPoint->Type() == TYPE_VIA )
           || Locate_Piste_Connectee( track, GetBoard()->m_Track, NULL, START ) )
            error = true;
        TrackToStartPoint->SetState( BUSY, OFF );
    }

    if( ( track->end == NULL ) || ( track->end->Type() == TYPE_TRACK ) )
        TrackToEndPoint = Locate_Piste_Connectee( track,
                                                  GetBoard()->m_Track, NULL,
                                                  END );

    //  Test if more than one segment is connected to this point
    if( TrackToEndPoint )
    {
        TrackToEndPoint->SetState( BUSY, ON );
        if( (TrackToEndPoint->Type() == TYPE_VIA)
           || Locate_Piste_Connectee( track, GetBoard()->m_Track, NULL, END ) )
            error = true;
        TrackToEndPoint->SetState( BUSY, OFF );
    }

    if( error )
    {
        DisplayError( this,
                      _( "Unable to drag this segment: too many segments connected" ) );
        return;
    }

    if( !TrackToStartPoint || ( TrackToStartPoint->Type() != TYPE_TRACK ) )
        s_StartSegmentPresent = false;

    if( !TrackToEndPoint || ( TrackToEndPoint->Type() != TYPE_TRACK ) )
        s_EndSegmentPresent = false;

    /* Change high light net: the new one will be highlighted */
    Old_HighLigt_Status   = g_HighLight_Status;
    Old_HighLigth_NetCode = g_HighLight_NetCode;
    if( g_HighLight_Status )
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


    PosInit   = GetScreen()->m_Curseur;
    s_LastPos = GetScreen()->m_Curseur;
    DrawPanel->ManageCurseur = Show_Drag_Track_Segment_With_Cte_Slope;
    DrawPanel->ForceCloseManageCurseur = Abort_MoveTrack;

    g_HighLight_NetCode = track->GetNet();
    g_HighLight_Status   = true;
    GetBoard()->DrawHighLight( DrawPanel, DC, g_HighLight_NetCode );

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
        DisplayError( this,
                      _( "Unable to drag this segment: two collinear segments" ) );
        DrawPanel->ManageCurseur = NULL;
        Abort_MoveTrack( DrawPanel, DC );
        return;
    }
}


/* Place a dragged (or moved) track segment or via */
bool WinEDA_PcbFrame::PlaceDraggedOrMovedTrackSegment( TRACK* Track, wxDC* DC )
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
    Track->SetState( EDIT, OFF );
    Track->Draw( DrawPanel, DC, draw_mode );

    /* Draw dragged tracks */
    for( unsigned ii = 0; ii < g_DragSegmentList.size(); ii++ )
    {
        Track = g_DragSegmentList[ii].m_Segm;
        Track->SetState( EDIT, OFF );
        Track->m_Flags = 0;
        Track->Draw( DrawPanel, DC, draw_mode );

        /* Test the connections modified by the move
         *  (only pad connection must be tested, track connection will be
         * tested by test_1_net_connexion() ) */
        int masque_layer = g_TabOneLayerMask[Track->GetLayer()];
        Track->start = Fast_Locate_Pad_Connecte(
            GetBoard(), Track->m_Start, masque_layer );
        Track->end   = Fast_Locate_Pad_Connecte(
            GetBoard(), Track->m_End, masque_layer );
    }

    EraseDragList();

    SaveCopyInUndoList( s_ItemsListPicker, UR_UNSPECIFIED );
    s_ItemsListPicker.ClearItemsList(); // s_ItemsListPicker is no more owner
                                        // of picked items

    OnModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;

    if( current_net_code > 0 )
        test_1_net_connexion( DC, current_net_code );

    return true;
}


/* Find the point "attachment" of the end of a trace.
 * This may be a TBP or another segment of the trace
 * Returns:
 * - Pointer to the PAD or:
 * - Pointer to the segment or:
 * - NULL
 * Parameters:
 * - position to test
 * - mask layers to be tested
 */
BOARD_ITEM* LocateLockPoint( BOARD* Pcb, wxPoint pos, int LayerMask )
{
    for( MODULE* module = Pcb->m_Modules; module; module = module->Next() )
    {
        D_PAD* pad = Locate_Pads( module, pos, LayerMask );
        if( pad )
            return pad;
    }

    /* No pad has been located so check for a segment of the trace. */
    TRACK* ptsegm = Fast_Locate_Piste( Pcb->m_Track, NULL, pos, LayerMask );
    if( ptsegm == NULL )
        ptsegm = Locate_Pistes( Pcb, Pcb->m_Track, pos, LayerMask );

    return ptsegm;
}


/* Create an intermediate point on a segment
 * aSegm segment is broken into 2 segments connecting point pX, pY
 * After insertion:
 *   The new segment starts from  to new point, and ends to initial aSegm ending point
 *   the old segment aSegm ends to new point
 * Returns:
 *   NULL if no new point (ie if aRefPoint already corresponded at one end of aSegm
 * or
 *   Pointer to the segment created
 *   Returns the exact value of aRefPoint
 * If aSegm points to a via:
 *   Returns the exact value of aRefPoint and a pointer to the via,
 *   But does not create extra point
 */
TRACK* CreateLockPoint( BOARD* aPcb,
                        wxPoint&           aRefPoint,
                        TRACK*             aSegm,
                        PICKED_ITEMS_LIST* aItemsListPicker )
{
    if( aSegm->m_Start == aRefPoint || aSegm->m_End == aRefPoint )
        return NULL;

    /* A via is a good lock point */
    if( aSegm->Type() == TYPE_VIA )
    {
        aRefPoint = aSegm->m_Start;
        return aSegm;
    }

    /* Calculation coordinate of intermediate point relative to
     * the start point of aSegm
     */
     wxPoint delta = aSegm->m_End - aSegm->m_Start;

    // Not yet in use:
#if 0
    int ox, oy, fx, fy;

    if( aRefSegm )
    {
        ox = aRefSegm->m_Start.x - aSegm->m_Start.x;
        oy = aRefSegm->m_Start.y - aSegm->m_Start.y;
        fx = aRefSegm->m_End.x - aSegm->m_Start.x;
        fy = aRefSegm->m_End.y - aSegm->m_Start.y;
    }
#endif

    // calculate coordinates of aRefPoint relative to aSegm->m_Start
    wxPoint newPoint = aRefPoint - aSegm->m_Start;
    // newPoint must be on aSegm:
    // Ensure newPoint.y/newPoint.y = delta.y/delta.x
    if( delta.x == 0 )
        newPoint.x = 0;         /* horizontal segment*/
    else
        newPoint.y = wxRound(( (double)newPoint.x * delta.y ) / delta.x);

    /* Create the intermediate point (that is to say creation of a new
     * segment, beginning at the intermediate point.
     */
    newPoint.x += aSegm->m_Start.x;
    newPoint.y += aSegm->m_Start.y;

    TRACK* newTrack = aSegm->Copy();
    if( aItemsListPicker )
    {
        ITEM_PICKER picker( newTrack, UR_NEW );
        aItemsListPicker->PushItem( picker );
    }


    DLIST<TRACK>* list = (DLIST<TRACK>*)aSegm->GetList();
    wxASSERT( list );
    list->Insert( newTrack, aSegm->Next() );

    if( aItemsListPicker )
    {
        ITEM_PICKER picker( aSegm, UR_CHANGED );
        picker.m_Link = aSegm->Copy();
        aItemsListPicker->PushItem( picker );
    }

    /* Correct pointer at the end of the new segment. */
    newTrack->end = aSegm->end;
    newTrack->SetState( END_ONPAD, aSegm->GetState( END_ONPAD ) );

    /* Set connections info relative to the new point
    */

    /* Old segment now ends at new point. */
    aSegm->m_End = newPoint;
    aSegm->end = newTrack;
    aSegm->SetState( END_ONPAD, OFF );

    /* The new segment begins at the new point. */
    newTrack->m_Start = newPoint;
    newTrack->start = aSegm;
    newTrack->SetState( BEGIN_ONPAD, OFF );

    D_PAD * pad = Locate_Pad_Connecte( aPcb, newTrack, START );
    if ( pad )
    {
        newTrack->start = pad;
        newTrack->SetState( BEGIN_ONPAD, ON );
        aSegm->end = pad;
        aSegm->SetState( END_ONPAD, ON );
    }

    aRefPoint = newPoint;
    return newTrack;
}
