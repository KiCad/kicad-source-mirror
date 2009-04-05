/****************************************************/
/*              Track editing						*/
/* routines to move and drag track segments or node */
/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"

#include "pcbnew.h"
#include "autorout.h"
#include "trigo.h"

#include "drag.h"
#include "id.h"

#include "protos.h"


/* local functions */
static void Show_MoveNode( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void Show_Drag_Track_Segment_With_Cte_Slope( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void Abort_MoveTrack( WinEDA_DrawPanel* Panel, wxDC* DC );
static bool InitialiseDragParameters();

/* variables locales */
static wxPoint PosInit, s_LastPos;
static TRACK*  NewTrack;    /* Nouvelle piste creee ou piste deplacee */
static int     NbPtNewTrack;
static int     Old_HightLigth_NetCode;
static bool    Old_HightLigt_Status;
static double  s_StartSegmentSlope, s_EndSegmentSlope, s_MovingSegmentSlope,
               s_StartSegment_Yorg, s_EndSegment_Yorg,
               s_MovingSegment_Yorg; //slope and intercept parameters of lines
bool           s_StartPointVertical, s_EndPointVertical,
               s_MovingSegmentVertical, s_MovingSegmentHorizontal,
               s_StartPointHorizontal, s_EndPointHorizontal; // vertical or horizontal line indicators
bool           s_StartSegmentPresent, s_EndSegmentPresent;

/**************************************************************/
static void Abort_MoveTrack( WinEDA_DrawPanel* Panel, wxDC* DC )
/***************************************************************/

/* routine d'annulation de la commande drag, copy ou move track  si une piste est en cours
 *  de tracage, ou de sortie de l'application EDITRACK.
 */
{
    TRACK* NextS;
    int ii;

    /* Erase the current drawings */
    wxPoint             oldpos = Panel->GetScreen()->m_Curseur;

    Panel->GetScreen()->m_Curseur = PosInit;

    if( Panel->ManageCurseur )
        Panel->ManageCurseur( Panel, DC, TRUE );

    Panel->GetScreen()->m_Curseur = oldpos;
    g_HightLigt_Status = FALSE;
    ( (WinEDA_PcbFrame*) Panel->m_Parent )->GetBoard()->DrawHighLight( Panel,
                                                                       DC,
                                                                       g_HightLigth_NetCode );

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
        else    /* Move : remise en ancienne position */
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
    ( (WinEDA_PcbFrame*) Panel->m_Parent )->SetCurItem( NULL );

    /* Annulation deplacement et Redessin des segments dragges */
    DRAG_SEGM* pt_drag = g_DragSegmentList;
    for( ; pt_drag != NULL; pt_drag = pt_drag->Pnext )
    {
        TRACK* Track = pt_drag->m_Segm;
        pt_drag->SetInitialValues();
        Track->SetState( EDIT, OFF );
        Track->m_Flags = 0;
        Track->Draw( Panel, DC, GR_OR );
    }

    g_HightLigth_NetCode = Old_HightLigth_NetCode;
    g_HightLigt_Status   = Old_HightLigt_Status;
    if( g_HightLigt_Status )
        ( (WinEDA_PcbFrame*) Panel->m_Parent )->GetBoard()->DrawHighLight( Panel,
                                                                           DC,
                                                                           g_HightLigth_NetCode );

    EraseDragListe();
}


/*************************************************************************/
static void Show_MoveNode( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/*************************************************************************/
/* Redraw the moved node according to the mouse cursor position */
{
    int          ii, dx, dy;
    TRACK*       Track;
    BASE_SCREEN* screen = panel->GetScreen();
    int          track_fill_copy = DisplayOpt.DisplayPcbTrackFill;
    int          draw_mode = GR_XOR | GR_SURBRILL;

    DisplayOpt.DisplayPcbTrackFill = FALSE;

    erase = TRUE;

    /* erase the current moved track segments from screen */
    if( erase )
    {
        if( NewTrack )
            Trace_Une_Piste( panel, DC, NewTrack, NbPtNewTrack, draw_mode );
    }

    /* set the new track coordinates */
    wxPoint Pos = screen->m_Curseur;

    dx = Pos.x - s_LastPos.x;
    dy = Pos.y - s_LastPos.y;

    s_LastPos = Pos;

    ii    = NbPtNewTrack;
    Track = NewTrack;
    for( ; (ii > 0) && (Track != NULL); ii--, Track = Track->Next() )
    {
        if( Track->m_Flags & STARTPOINT )
        {
            Track->m_Start.x += dx;
            Track->m_Start.y += dy;
        }
        if( Track->m_Flags & ENDPOINT )
        {
            Track->m_End.x += dx;
            Track->m_End.y += dy;
        }
    }

    /* Redraw the current moved track segments */
    Trace_Une_Piste( panel, DC, NewTrack, NbPtNewTrack, GR_XOR );

    DRAG_SEGM* pt_drag = g_DragSegmentList;
    for( ; pt_drag != NULL; pt_drag = pt_drag->Pnext )
    {
        Track = pt_drag->m_Segm;
        if( erase )
            Track->Draw( panel, DC, draw_mode );

        if( Track->m_Flags & STARTPOINT )
        {
            Track->m_Start.x += dx;
            Track->m_Start.y += dy;
        }

        if( Track->m_Flags & ENDPOINT )
        {
            Track->m_End.x += dx;
            Track->m_End.y += dy;
        }

        Track->Draw( panel, DC, draw_mode );
    }

    DisplayOpt.DisplayPcbTrackFill = track_fill_copy;
}


/*************************************************************************/
static void Show_Drag_Track_Segment_With_Cte_Slope( WinEDA_DrawPanel* panel,
                                                    wxDC* DC, bool erase )
/*************************************************************************/

/* drawing the track segment movement
 *  > s_MovingSegmentSlope slope = moving track segment slope
 *  > s_StartSegmentSlope slope = slope of the segment connected to the start point of the moving segment
 *  > s_EndSegmentSlope slope = slope of the segment connected to the end point of the moving segment
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
 *      xi1=(s_MovingSegment_Yorg-s_StartSegment_Yorg)/(s_StartSegmentSlope-s_MovingSegmentSlope)
 *      yi1=s_MovingSegmentSlope*xi1+s_MovingSegment_Yorg
 *      or yi1=s_StartSegmentSlope*xi1+s_MovingSegment_Yorg
 *
 *  second intersection point
 *      y2=yt ->
 *      xi2=(s_MovingSegment_Yorg-s_StartSegment_Yorg)/(s_MovingSegmentSlope-s_MovingSegmentSlope)
 *      yi2=s_MovingSegmentSlope*xi2+s_MovingSegment_Yorg
 *      or yi1=s_EndSegmentSlope*xi2+s_MovingSegment_Yorg
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *  !!!!!    special attention to vertical segments because
 *  !!!!!    their slope=infinite
 *  !!!!!    intersection point will be calculated using the
 *  !!!!!    segment intersecting it
 *  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *  Slope parametres are computed once, because they can become undetermined when moving segments
 *  (i.e. when a segment lenght is 0) and we want keep them constant
 */

{
    double       xi1 = 0, yi1 = 0, xi2 = 0, yi2 = 0;    // calculated intersection points
    double       tx1, tx2, ty1, ty2;                    // temporary storage of points
    int          dx, dy;
    BASE_SCREEN* screen = panel->GetScreen();
    bool         update = true;
    TRACK*       Track;
    DRAG_SEGM*   TrackSegWrapper = g_DragSegmentList;
    TRACK*       tSegmentToStart = NULL, * tSegmentToEnd = NULL;

    if( TrackSegWrapper == NULL )
        return;

    Track = TrackSegWrapper->m_Segm;
    if( Track == NULL )
        return;

    TrackSegWrapper = TrackSegWrapper->Pnext;
    if( TrackSegWrapper )
    {
        if( s_EndSegmentPresent )
        {
            tSegmentToEnd   = TrackSegWrapper->m_Segm;  // Get the segment connected to the end point
            TrackSegWrapper = TrackSegWrapper->Pnext;
        }
        if( s_StartSegmentPresent )
        {
            if( TrackSegWrapper )
                tSegmentToStart = TrackSegWrapper->m_Segm; // Get the segment connected to the start point
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
        s_MovingSegment_Yorg = ty1 - (s_MovingSegmentSlope * tx1);
    }

    if( (!s_EndPointVertical) && (!s_MovingSegmentVertical) )
    {
        xi2 = (s_MovingSegment_Yorg -
               s_EndSegment_Yorg) / (s_EndSegmentSlope - s_MovingSegmentSlope);
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
        yi2 = s_MovingSegmentSlope * (xi2) + s_MovingSegment_Yorg;
    }
    else
    {
        if( !s_EndPointVertical )
        {
            yi2 = s_EndSegmentSlope * (xi2) + s_EndSegment_Yorg;
        }
        else
        {
            if( !s_EndPointHorizontal )
            {
                update = false;
            }
            else
            {
                yi2 = s_MovingSegmentSlope * (xi2) + s_MovingSegment_Yorg;
            }
        }
    }

    if( (!s_StartPointVertical) && (!s_MovingSegmentVertical) )
    {
        xi1 = (s_MovingSegment_Yorg -
               s_StartSegment_Yorg) / (s_StartSegmentSlope - s_MovingSegmentSlope);
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
        yi1 = s_MovingSegmentSlope * (xi1) + s_MovingSegment_Yorg;
    }
    else
    {
        if( !s_StartPointVertical )
        {
            yi1 = s_StartSegmentSlope * (xi1) + s_StartSegment_Yorg;
        }
        else
        {
            if( !s_StartPointHorizontal )
            {
                update = false;
            }
            else
            {
                yi2 = s_MovingSegmentSlope * (xi1) + s_MovingSegment_Yorg;
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
}


/**********************************/
bool InitialiseDragParameters()
/**********************************/

/* Init variables (slope, Y intersect point, flags) for Show_Drag_Track_Segment_With_Cte_Slope()
 *  return TRUE if Ok, FALSE if dragging is not possible
 *  (2 colinear segments)
 */
{
    double     tx1, tx2, ty1, ty2; // temporary storage of points
    TRACK*     Track;
    DRAG_SEGM* TrackSegWrapper = g_DragSegmentList;
    TRACK*     tSegmentToStart = NULL, * tSegmentToEnd = NULL;

    if( TrackSegWrapper == NULL )
        return FALSE;
    Track = TrackSegWrapper->m_Segm;
    if( Track == NULL )
        return FALSE;

    TrackSegWrapper = TrackSegWrapper->Pnext;
    if( TrackSegWrapper )
    {
        if( s_EndSegmentPresent )
        {
            tSegmentToEnd   = TrackSegWrapper->m_Segm;  // Get the segment connected to the end point
            TrackSegWrapper = TrackSegWrapper->Pnext;
        }
        if( s_StartSegmentPresent )
        {
            if( TrackSegWrapper )
                tSegmentToStart = TrackSegWrapper->m_Segm; // Get the segment connected to the start point
        }
    }

    //would be nice to eliminate collinear segments here, so we don't
    //have to deal with that annoying "Unable to drag this segment: two collinear segments"

    s_StartPointVertical = false;
    s_EndPointVertical = false;
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
    if( tx1!=tx2 )
    {
        s_StartSegmentSlope = (ty2 - ty1) / (tx2 - tx1);
        s_StartSegment_Yorg = ty1 - (ty2 - ty1) * tx1 / (tx2 - tx1);
    }
    else
    {
        s_StartPointVertical = true;            //signal first segment vertical
    }
    if( ty1==ty2 )
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

    if( tx2!=tx1 )
    {
        s_EndSegmentSlope = (ty2 - ty1) / (tx2 - tx1);
        s_EndSegment_Yorg = ty1 - (ty2 - ty1) * tx1 / (tx2 - tx1);
    }
    else
    {
        s_EndPointVertical = true;      //signal second segment vertical
    }
    if( ty1==ty2 )
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
        s_MovingSegmentSlope = (ty2 - ty1) / (tx2 - tx1);
    }
    else
    {
        s_MovingSegmentVertical = true;      //signal vertical line
    }
    if( ty1==ty2 )
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
        if( !s_EndPointVertical && (s_MovingSegmentSlope == s_EndSegmentSlope) )
            return false;
        if( !s_StartPointVertical && (s_MovingSegmentSlope == s_StartSegmentSlope) )
            return false;
    }

    return TRUE;
}


/*************************************************************************************/
void WinEDA_PcbFrame::Start_MoveOneNodeOrSegment( TRACK* track, wxDC* DC, int command )
/*************************************************************************************/

/* Init parametres to move one node:
 *  a via or/and a terminal point of a track segment
 *  The terminal point of other connected segments (if any) are moved too.
 */
{
    if( !track )
        return;

    NewTrack     = NULL;
    NbPtNewTrack = 0;
    EraseDragListe();

    /* Change highlighted net: the new one will be hightlighted */
    Old_HightLigt_Status   = g_HightLigt_Status;
    Old_HightLigth_NetCode = g_HightLigth_NetCode;
    if( g_HightLigt_Status )
        Hight_Light( DC );
    PosInit = GetScreen()->m_Curseur;

    if( track->Type() == TYPE_VIA )
    {
        track->m_Flags = IS_DRAGGED | STARTPOINT | ENDPOINT;
        if( command != ID_POPUP_PCB_MOVE_TRACK_SEGMENT )
        {
            Collect_TrackSegmentsToDrag( DrawPanel, DC, track->m_Start,
                                        track->ReturnMaskLayer(), track->GetNet() );
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
        case ID_POPUP_PCB_MOVE_TRACK_SEGMENT:
            track->m_Flags |= IS_DRAGGED | ENDPOINT | STARTPOINT;
            AddSegmentToDragList( DrawPanel, DC, track->m_Flags, track );
            break;

        case ID_POPUP_PCB_DRAG_TRACK_SEGMENT:
            pos = track->m_Start;
            Collect_TrackSegmentsToDrag( DrawPanel, DC, pos,
                                        track->ReturnMaskLayer(), track->GetNet() );
            pos = track->m_End;
            track->m_Flags |= IS_DRAGGED | ENDPOINT | STARTPOINT;
            Collect_TrackSegmentsToDrag( DrawPanel, DC, pos,
                                        track->ReturnMaskLayer(), track->GetNet() );
            break;

        case ID_POPUP_PCB_MOVE_TRACK_NODE:
            pos = (diag & STARTPOINT) ? track->m_Start : track->m_End;
            Collect_TrackSegmentsToDrag( DrawPanel, DC, pos,
                                        track->ReturnMaskLayer(), track->GetNet() );
            PosInit = pos;
            break;
        }

        track->m_Flags |= IS_DRAGGED;
    }
    s_LastPos = PosInit;
    DrawPanel->ManageCurseur = Show_MoveNode;
    DrawPanel->ForceCloseManageCurseur = Abort_MoveTrack;

    g_HightLigth_NetCode = track->GetNet();
    g_HightLigt_Status   = TRUE;

    GetBoard()->DrawHighLight( DrawPanel, DC, g_HightLigth_NetCode );
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
}


#if 0
// @todo: This function is broken: does not handle pointers to pads for start and end and flags relative to these pointers
void SortTrackEndPoints( TRACK* track )
{
    //sort the track endpoints -- should not matter in terms of drawing
    //or producing the pcb -- but makes doing comparisons easier.
    int     dx = track->m_End.x - track->m_Start.x;

    if( dx )
    {
        if( track->m_Start.x > track->m_End.x )
        {
            EXCHG(track->m_Start, track->m_End);
        }
    }
    else
    {
        if( track->m_Start.y > track->m_End.y )
        {
            EXCHG(track->m_Start, track->m_End);
        }
    }
}


/***********************************************************************************/
bool WinEDA_PcbFrame::MergeCollinearTracks( TRACK* track, wxDC* DC, int end )
/***********************************************************************************/
/**
 * @todo: this function is broken, because it merge segments having different width or without any connectivity test.
 * 2 collinear segments can be merged only in no other segment or vais is connected to the common point
 * and if they have the same width. See cleanup.cpp for merge functions,
 * and consider Marque_Une_Piste() to locate segments that can be merged
 */

    testtrack = (TRACK*) Locate_Piste_Connectee( track, GetBoard()->m_Track, NULL, end );
    if( testtrack )
    {
        SortTrackEndPoints( track );
        SortTrackEndPoints( testtrack );
        int dx  = track->m_End.x - track->m_Start.x;
        int dy  = track->m_End.y - track->m_Start.y;
        int tdx = testtrack->m_End.x - testtrack->m_Start.x;
        int tdy = testtrack->m_End.y - testtrack->m_Start.y;

        if( (dy * tdx == dx * tdy && dy != 0 && dx != 0 && tdy != 0 && tdx != 0) /*angle, same slope*/
           || (dy == 0 && tdy == 0 && dx * tdx )  /*horizontal*/
           || (dx == 0 && tdx == 0 && dy * tdy ) /*vertical*/ )
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

/***********************************************************************************/
void WinEDA_PcbFrame::Start_DragTrackSegmentAndKeepSlope( TRACK* track, wxDC* DC )
/***********************************************************************************/
{
    TRACK* TrackToStartPoint = NULL;
    TRACK* TrackToEndPoint   = NULL;
    bool   error = FALSE;

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

    s_StartSegmentPresent = s_EndSegmentPresent = TRUE;

    if( (track->start == NULL) || (track->start->Type() == TYPE_TRACK) )
        TrackToStartPoint = (TRACK*) Locate_Piste_Connectee( track,
                                                             GetBoard()->m_Track, NULL, START );

    //  Test if more than one segment is connected to this point
    if( TrackToStartPoint )
    {
        TrackToStartPoint->SetState( BUSY, ON );
        if( Locate_Piste_Connectee( track, GetBoard()->m_Track, NULL, START ) )
            error = TRUE;
        TrackToStartPoint->SetState( BUSY, OFF );
    }

    if( (track->end == NULL) || (track->end->Type() == TYPE_TRACK) )
        TrackToEndPoint = (TRACK*) Locate_Piste_Connectee( track, GetBoard()->m_Track, NULL, END );

    //  Test if more than one segment is connected to this point
    if( TrackToEndPoint )
    {
        TrackToEndPoint->SetState( BUSY, ON );
        if( Locate_Piste_Connectee( track, GetBoard()->m_Track, NULL, END ) )
            error = TRUE;
        TrackToEndPoint->SetState( BUSY, OFF );
    }

    if( error )
    {
        DisplayError( this, _( "Unable to drag this segment: too many segments connected" ) );
        return;
    }

    if( !TrackToStartPoint || (TrackToStartPoint->Type() != TYPE_TRACK) )
        s_StartSegmentPresent = FALSE;

    if( !TrackToEndPoint || (TrackToEndPoint->Type() != TYPE_TRACK) )
        s_EndSegmentPresent = FALSE;

    /* Change hight light net: the new one will be hightlighted */
    Old_HightLigt_Status   = g_HightLigt_Status;
    Old_HightLigth_NetCode = g_HightLigth_NetCode;
    if( g_HightLigt_Status )
        Hight_Light( DC );

    EraseDragListe();

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

    g_HightLigth_NetCode = track->GetNet();
    g_HightLigt_Status   = TRUE;
    GetBoard()->DrawHighLight( DrawPanel, DC, g_HightLigth_NetCode );

    if( !InitialiseDragParameters() )
    {
        DisplayError( this, _( "Unable to drag this segment: two collinear segments" ) );
        DrawPanel->ManageCurseur = NULL;
        Abort_MoveTrack( DrawPanel, DC );
        return;
    }
}


/**********************************************************************/
bool WinEDA_PcbFrame::PlaceDraggedTrackSegment( TRACK* Track, wxDC* DC )
/**********************************************************************/
/* Place a dragged (or moved) track segment or via */
{
    int        errdrc;
    DRAG_SEGM* pt_drag;

    if( Track == NULL )
        return FALSE;

    int current_net_code = Track->GetNet();

    // DRC control:
    if( Drc_On )
    {
        errdrc = m_drc->Drc( Track, GetBoard()->m_Track );
        if( errdrc == BAD_DRC )
            return FALSE;
        /* Redraw the dragged segments */
        pt_drag = g_DragSegmentList;
        for( ; pt_drag != NULL; pt_drag = pt_drag->Pnext )
        {
            errdrc = m_drc->Drc( pt_drag->m_Segm, GetBoard()->m_Track );
            if( errdrc == BAD_DRC )
                return FALSE;
        }
    }

    int draw_mode = GR_OR | GR_SURBRILL;

    // DRC Ok: place track segments
    Track->m_Flags = 0;
    Track->SetState( EDIT, OFF );
    Track->Draw( DrawPanel, DC, draw_mode );

    /* Tracage des segments dragges */
    pt_drag = g_DragSegmentList;
    for( ; pt_drag; pt_drag = pt_drag->Pnext )
    {
        Track = pt_drag->m_Segm;
        Track->SetState( EDIT, OFF );
        Track->m_Flags = 0;
        Track->Draw( DrawPanel, DC, draw_mode );

        /* Test the connections modified by the move
         *  (only pad connection must be tested, track connection will be tested by test_1_net_connexion() ) */
        int masque_layer = g_TabOneLayerMask[Track->GetLayer()];
        Track->start = Fast_Locate_Pad_Connecte( GetBoard(), Track->m_Start, masque_layer );
        Track->end   = Fast_Locate_Pad_Connecte( GetBoard(), Track->m_End, masque_layer );
    }

    EraseDragListe();

    GetScreen()->SetModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;

    if( current_net_code > 0 )
        test_1_net_connexion( DC, current_net_code );

    return TRUE;
}


/************************************************************************/
EDA_BaseStruct* LocateLockPoint( BOARD* Pcb, wxPoint pos, int LayerMask )
/************************************************************************/

/* Routine trouvant le point " d'accrochage " d'une extremite de piste.
 *  Ce point peut etre un PAD ou un autre segment de piste
 *  Retourne:
 *      - pointeur sur ce PAD ou:
 *      - pointeur sur le segment ou:
 *      - NULL
 *  Parametres d'appel:
 *   coord pX, pY du point tst
 *   masque des couches a tester
 */
{
    for( MODULE* module = Pcb->m_Modules;  module;  module = module->Next() )
    {
        D_PAD* pad = Locate_Pads( module, pos, LayerMask );
        if( pad )
            return pad;
    }

    /* ici aucun pad n'a ete localise: detection d'un segment de piste */

    TRACK* ptsegm = Fast_Locate_Piste( Pcb->m_Track, NULL, pos, LayerMask );
    if( ptsegm == NULL )
        ptsegm = Locate_Pistes( Pcb->m_Track, pos, LayerMask );

    return ptsegm;
}


/******************************************************************************/
TRACK* CreateLockPoint( int* pX, int* pY, TRACK* ptsegm, TRACK* refsegm )
/******************************************************************************/

/* Routine de creation d'un point intermediaire sur un segment
 *  le segment ptsegm est casse en 2 segments se raccordant au point pX, pY
 *  retourne:
 *      NULL si pas de nouveau point ( c.a.d si pX, pY correspondait deja
 *      a une extremite ou:
 *      pointeur sur le segment cree
 *  si refsegm != NULL refsegm est pointeur sur le segment incident,
 *  et le point cree est l'intersection des 2 axes des segments ptsegm et
 *  refsegm
 *  retourne la valeur exacte de pX et pY
 *  Si ptsegm pointe sur une via:
 *      retourne la valeur exacte de pX et pY et ptsegm,
 *      mais ne cree pas de point supplementaire
 *
 */
{
    int cX, cY;
    int dx, dy;             /* Coord de l'extremite du segm ptsegm / origine */
    int ox, oy, fx, fy;     /* coord de refsegm / origine de prsegm */

    if( (ptsegm->m_Start.x == *pX) && (ptsegm->m_Start.y == *pY) )
        return NULL;

    if( (ptsegm->m_End.x == *pX) && (ptsegm->m_End.y == *pY) )
        return NULL;

    /* le point n'est pas sur une extremite de piste */
    if( ptsegm->Type() == TYPE_VIA )
    {
        *pX = ptsegm->m_Start.x;
        *pY = ptsegm->m_Start.y;
        return ptsegm;
    }

    /* calcul des coord vraies du point intermediaire dans le repere d'origine
     *  = origine de ptsegm
     */
    cX = *pX - ptsegm->m_Start.x;
    cY = *pY - ptsegm->m_Start.y;

    dx = ptsegm->m_End.x - ptsegm->m_Start.x;
    dy = ptsegm->m_End.y - ptsegm->m_Start.y;

    // ***** A COMPLETER : non utilise
    if( refsegm )
    {
        ox = refsegm->m_Start.x - ptsegm->m_Start.x;
        oy = refsegm->m_Start.y - ptsegm->m_Start.y;
        fx = refsegm->m_End.x - ptsegm->m_Start.x;
        fy = refsegm->m_End.y - ptsegm->m_Start.y;
    }

    /* pour que le point soit sur le segment ptsegm: cY/cX = dy/dx */
    if( dx == 0 )
        cX = 0;         /* segm horizontal */
    else
        cY = (cX * dy) / dx;

    /* creation du point intermediaire ( c'est a dire creation d'un nouveau
     * segment, debutant au point intermediaire
     */

    cX += ptsegm->m_Start.x;
    cY += ptsegm->m_Start.y;

    TRACK*        newTrack = ptsegm->Copy();

    DLIST<TRACK>* list = (DLIST<TRACK>*)ptsegm->GetList();
    wxASSERT( list );
    list->Insert( newTrack, ptsegm->Next() );

    /* correction du pointeur de fin du nouveau segment */
    newTrack->end = ptsegm->end;

    /* le segment primitif finit au nouveau point : */
    ptsegm->m_End.x = cX;
    ptsegm->m_End.y = cY;

    ptsegm->SetState( END_ONPAD, OFF );

    /* le nouveau segment debute au nouveau point : */
    ptsegm = newTrack;;
    ptsegm->m_Start.x = cX;
    ptsegm->m_Start.y = cY;
    ptsegm->SetState( BEGIN_ONPAD, OFF );

    *pX = cX;
    *pY = cY;

    return ptsegm;
}
