/****************/
/* Edit traces.	*/
/****************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"

#include "drc_stuff.h"
#include "trigo.h"

#include "protos.h"


static void Exit_Editrack( WinEDA_DrawPanel* panel, wxDC* DC );
void        ShowNewTrackWhenMovingCursor( WinEDA_DrawPanel* panel,
                                          wxDC* DC, bool erase );
static void ComputeBreakPoint( TRACK* track, int n, wxPoint end );
static void DeleteNullTrackSegments( BOARD* pcb, DLIST<TRACK>& aTrackList );

static void EnsureEndTrackOnPad( D_PAD* Pad );


static int OldNetCodeSurbrillance;
static int OldEtatSurbrillance;
static PICKED_ITEMS_LIST s_ItemsListPicker;


/* Routine to cancel the route if a track is being drawn, or exit the
 * application EDITRACK.
 */
static void Exit_Editrack( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    WinEDA_PcbFrame* frame = (WinEDA_PcbFrame*) Panel->m_Parent;
    TRACK*           track = (TRACK*) frame->GetCurItem();

    if( track && ( track->Type()==TYPE_VIA || track->Type()==TYPE_TRACK ) )
    {
        /* Erase the current drawing */
        ShowNewTrackWhenMovingCursor( Panel, DC, false );
        if( g_HightLigt_Status )
            frame->Hight_Light( DC );

        g_HightLigth_NetCode = OldNetCodeSurbrillance;
        if( OldEtatSurbrillance )
            frame->Hight_Light( DC );

        frame->MsgPanel->EraseMsgBox();

        // Undo pending changes (mainly a lock point cretion) and clear the
        // undo picker list:
        frame->PutDataInPreviousState( &s_ItemsListPicker, false, false );
        s_ItemsListPicker.ClearListAndDeleteItems();

        // Delete current (new) track
        g_CurrentTrackList.DeleteAll();
    }

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    frame->SetCurItem( NULL );
}


/*
 * Begin drawing a new track and/or establish of a new track point.
 *
 * If no current track record of:
 * - Search netname of the new track (pad out departure netname
 *   if the departure runway on an old track
 * - Highlight all the net
 * - Initialize the various trace pointers.
 * If current track:
 * - Control DRC
 * - OK if DRC: adding a new track.
 */
TRACK* WinEDA_PcbFrame::Begin_Route( TRACK* aTrack, wxDC* DC )
{
    D_PAD*      pt_pad = NULL;
    TRACK*      TrackOnStartPoint = NULL;
    int         masquelayer =
        g_TabOneLayerMask[( (PCB_SCREEN*) GetScreen() )->m_Active_Layer];
    BOARD_ITEM* LockPoint;
    wxPoint     pos = GetScreen()->m_Curseur;

    DrawPanel->ManageCurseur = ShowNewTrackWhenMovingCursor;
    DrawPanel->ForceCloseManageCurseur = Exit_Editrack;

    if( aTrack == NULL )  /* Starting a new track  */
    {
        // Prepare the undo command info
        s_ItemsListPicker.ClearListAndDeleteItems();        // Should not be
                                                            // necessary,
                                                            // but...

        /* erase old highlight */
        OldNetCodeSurbrillance = g_HightLigth_NetCode;
        OldEtatSurbrillance    = g_HightLigt_Status;

        if( g_HightLigt_Status )
            Hight_Light( DC );

        g_CurrentTrackList.PushBack( new TRACK( GetBoard() ) );
        g_CurrentTrackSegment->m_Flags = IS_NEW;

        g_HightLigth_NetCode = 0;

        // Search for a starting point of the new track, a track or pad
        LockPoint = LocateLockPoint( GetBoard(), pos, masquelayer );

        if( LockPoint ) // An item (pad or track) is found
        {
            if( LockPoint->Type() == TYPE_PAD )
            {
                pt_pad = (D_PAD*) LockPoint;

                /* A pad is found: put the starting point on pad centre */
                pos = pt_pad->m_Pos;
                g_HightLigth_NetCode = pt_pad->GetNet();
            }
            else /* A track segment is found */
            {
                TrackOnStartPoint    = (TRACK*) LockPoint;
                g_HightLigth_NetCode = TrackOnStartPoint->GetNet();
                CreateLockPoint( pos,
                                 TrackOnStartPoint,
                                 NULL,
                                 &s_ItemsListPicker );
            }
        }
        else    // no starting point, but a filled zone area can exist. This is
                // also a good starting point.
        {
            ZONE_CONTAINER* zone =
                GetBoard()->HitTestForAnyFilledArea( pos,
                                                     GetScreen()->
                                                     m_Active_Layer );
            if( zone )
                g_HightLigth_NetCode = zone->GetNet();
        }

        D( g_CurrentTrackList.VerifyListIntegrity(); );

        build_ratsnest_pad( LockPoint, wxPoint( 0, 0 ), true );

        D( g_CurrentTrackList.VerifyListIntegrity(); );

        Hight_Light( DC );

        // Display info about track Net class, and init track and vias sizes:
        g_CurrentTrackSegment->SetNet( g_HightLigth_NetCode );
        GetBoard()->SetCurrentNetClass( g_CurrentTrackSegment->GetNetClassName() );
        m_TrackAndViasSizesList_Changed = true;
        AuxiliaryToolBar_Update_UI();

        g_CurrentTrackSegment->SetLayer( GetScreen()->m_Active_Layer );
        g_CurrentTrackSegment->m_Width = GetBoard()->GetCurrentTrackWidth();

        if( g_DesignSettings.m_UseConnectedTrackWidth )
        {
            if( TrackOnStartPoint && TrackOnStartPoint->Type() == TYPE_TRACK )
                g_CurrentTrackSegment->m_Width = TrackOnStartPoint->m_Width;
        }
        g_CurrentTrackSegment->m_Start = pos;
        g_CurrentTrackSegment->m_End   = pos;

        if( pt_pad )
        {
            g_CurrentTrackSegment->start = pt_pad;
            g_CurrentTrackSegment->SetState( BEGIN_ONPAD, ON );
        }
        else
            g_CurrentTrackSegment->start = TrackOnStartPoint;

        if( g_TwoSegmentTrackBuild )
        {
            // Create 2nd segment
            g_CurrentTrackList.PushBack( g_CurrentTrackSegment->Copy() );

            D( g_CurrentTrackList.VerifyListIntegrity(); );

            g_CurrentTrackSegment->start = g_FirstTrackSegment;
            g_FirstTrackSegment->end     = g_CurrentTrackSegment;

            g_FirstTrackSegment->SetState( BEGIN_ONPAD | END_ONPAD, OFF );
        }

        D( g_CurrentTrackList.VerifyListIntegrity(); );

        g_CurrentTrackSegment->DisplayInfoBase( this );
        SetCurItem( g_CurrentTrackSegment, false );
        DrawPanel->ManageCurseur( DrawPanel, DC, false );

        if( Drc_On )
        {
            if( BAD_DRC ==
               m_drc->Drc( g_CurrentTrackSegment, GetBoard()->m_Track ) )
            {
                return g_CurrentTrackSegment;
            }
        }
    }
    else    /* Track in progress : segment coordinates are updated by
             * ShowNewTrackWhenMovingCursor*/
    {
        /* Tst for a D.R.C. error: */
        if( Drc_On )
        {
            if( BAD_DRC ==
               m_drc->Drc( g_CurrentTrackSegment, GetBoard()->m_Track ) )
                return NULL;

            // We must handle 2 segments
            if( g_TwoSegmentTrackBuild && g_CurrentTrackSegment->Back() )
            {
                if( BAD_DRC ==
                   m_drc->Drc( g_CurrentTrackSegment->Back(),
                               GetBoard()->m_Track ) )
                    return NULL;
            }
        }

        /* Current track is Ok: current segment is kept, and a new one is
         * created unless the current segment is null, or 2 last are null
         * if a 2 segments track build.
         */
        bool CanCreateNewSegment = true;
        if( !g_TwoSegmentTrackBuild && g_CurrentTrackSegment->IsNull() )
            CanCreateNewSegment = false;

        if( g_TwoSegmentTrackBuild && g_CurrentTrackSegment->IsNull()
           && g_CurrentTrackSegment->Back()
           && g_CurrentTrackSegment->Back()->IsNull() )
            CanCreateNewSegment = false;

        if( CanCreateNewSegment )
        {
            /* Erase old track on screen */
            D( g_CurrentTrackList.VerifyListIntegrity(); );

            ShowNewTrackWhenMovingCursor( DrawPanel, DC, false );

            D( g_CurrentTrackList.VerifyListIntegrity(); );

            if( g_Raccord_45_Auto )
            {
                Add_45_degrees_Segment( DC );
            }

            TRACK* oneBeforeLatest = g_CurrentTrackSegment;

            TRACK* newTrack = g_CurrentTrackSegment->Copy();
            g_CurrentTrackList.PushBack( newTrack );
            newTrack->m_Flags = IS_NEW;

            newTrack->SetState( BEGIN_ONPAD | END_ONPAD, OFF );

            oneBeforeLatest->end = Locate_Pad_Connecte(
                GetBoard(), oneBeforeLatest, END );
            if( oneBeforeLatest->end )
            {
                oneBeforeLatest->SetState( END_ONPAD, ON );
                newTrack->SetState( BEGIN_ONPAD, ON );
            }
            newTrack->start = oneBeforeLatest->end;

            D( g_CurrentTrackList.VerifyListIntegrity(); );

            newTrack->m_Start = newTrack->m_End;

            newTrack->SetLayer( ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer );
            if( !g_DesignSettings.m_UseConnectedTrackWidth )
            {
                newTrack->m_Width = GetBoard()->GetCurrentTrackWidth();
            }

            D( g_CurrentTrackList.VerifyListIntegrity(); );

            /* Show the new position */
            ShowNewTrackWhenMovingCursor( DrawPanel, DC, false );
        }
    }

    SetCurItem( g_CurrentTrackSegment, false );
    return g_CurrentTrackSegment;
}


/* Correct a bend is 90 and changes by 2 elbows at 45
 * This only works on horizontal or vertical segments.
 *
 * Input: pointer to the segment that we have drawn
 * Assume that the preceding segment is one that has been
 * previously drawn
 * Returns:
 *   1 if ok
 *   0 if not
 */
bool WinEDA_PcbFrame::Add_45_degrees_Segment( wxDC* DC )
{
    int pas_45;
    int dx0, dy0, dx1, dy1;

    if( g_CurrentTrackList.GetCount() < 2 )
        return false;                         /* There must be 2 segments. */

    TRACK* curTrack  = g_CurrentTrackSegment;
    TRACK* prevTrack = curTrack->Back();

    // Test whether there has 2 consecutive segments to be connected.
    if( curTrack->Type() != TYPE_TRACK || prevTrack->Type() != TYPE_TRACK )
    {
        return false;
    }

    pas_45 = (int) GetScreen()->GetGridSize().x / 2;
    if( pas_45 < curTrack->m_Width )
        pas_45 = (int) GetScreen()->GetGridSize().x;

    while( pas_45 < curTrack->m_Width )
        pas_45 *= 2;

    // Test if the segments are horizontal or vertical.
    dx0 = prevTrack->m_End.x - prevTrack->m_Start.x;
    dy0 = prevTrack->m_End.y - prevTrack->m_Start.y;

    dx1 = curTrack->m_End.x - curTrack->m_Start.x;
    dy1 = curTrack->m_End.y - curTrack->m_Start.y;

    // Segments must be of sufficient length.
    if( MAX( abs( dx0 ), abs( dy0 ) ) < ( pas_45 * 2 ) )
        return false;

    if( MAX( abs( dx1 ), abs( dy1 ) ) < ( pas_45 * 2 ) )
        return false;

    /* Create a new segment and connect it with the previous 2 segments. */
    TRACK* newTrack = curTrack->Copy();

    newTrack->m_Start = prevTrack->m_End;
    newTrack->m_End   = curTrack->m_Start;

    if( dx0 == 0 )          // Segment precedent Vertical
    {
        if( dy1 != 0 )      // 2 segments are not 90 degrees.
        {
            delete newTrack;
            return false;
        }

        /* Calculate coordinates the connection point.
         * The new segment connects the 1st segment Vertical and the 2nd
         * horizontal segment.
         */
        if( dy0 > 0 )
            newTrack->m_Start.y -= pas_45;
        else
            newTrack->m_Start.y += pas_45;

        if( dx1 > 0 )
            newTrack->m_End.x += pas_45;
        else
            newTrack->m_End.x -= pas_45;

        if( Drc_On && BAD_DRC == m_drc->Drc( curTrack, GetBoard()->m_Track ) )
        {
            delete newTrack;
            return false;
        }

        prevTrack->m_End  = newTrack->m_Start;
        curTrack->m_Start = newTrack->m_End;

        g_CurrentTrackList.Insert( newTrack, curTrack );
        return true;
    }

    if( dy0 == 0 )      // Segment precedent horizontal
    {
        if( dx1 != 0 )  // 2 segments are not 90 degrees
        {
            delete newTrack;
            return false;
        }

        /*  Calculate the coordinates of the point of connection:
         * A new segment has been established, connecting segment 1
         * (horizontal) and segment 2 (vertical)
         */
        if( dx0 > 0 )
            newTrack->m_Start.x -= pas_45;
        else
            newTrack->m_Start.x += pas_45;

        if( dy1 > 0 )
            newTrack->m_End.y += pas_45;
        else
            newTrack->m_End.y -= pas_45;

        if( Drc_On && BAD_DRC==m_drc->Drc( newTrack, GetBoard()->m_Track ) )
        {
            delete newTrack;
            return false;
        }

        prevTrack->m_End  = newTrack->m_Start;
        curTrack->m_Start = newTrack->m_End;

        g_CurrentTrackList.Insert( newTrack, curTrack );
        return true;
    }

    return false;
}


/*
 * End trace route in progress.
 */
void WinEDA_PcbFrame::End_Route( TRACK* aTrack, wxDC* DC )
{
    int masquelayer =
        g_TabOneLayerMask[( (PCB_SCREEN*) GetScreen() )->m_Active_Layer];

    if( aTrack == NULL )
        return;

    if( Drc_On && BAD_DRC==
       m_drc->Drc( g_CurrentTrackSegment, GetBoard()->m_Track ) )
        return;

    /* Sauvegarde des coord du point terminal de la piste */
    wxPoint pos = g_CurrentTrackSegment->m_End;

    D( g_CurrentTrackList.VerifyListIntegrity(); );

    if( Begin_Route( aTrack, DC ) == NULL )
        return;

    ShowNewTrackWhenMovingCursor( DrawPanel, DC, true );
    ShowNewTrackWhenMovingCursor( DrawPanel, DC, false );
    trace_ratsnest_pad( DC );

    /* cleanup
     *  if( g_CurrentTrackSegment->Next() != NULL )
     *  {
     *   delete g_CurrentTrackSegment->Next();
     *   g_CurrentTrackSegment->SetNext( NULL );
     *  }
     */

    D( g_CurrentTrackList.VerifyListIntegrity(); );


    /* The track here is non chained to the list of track segments.
     * It must be seen in the area of net
     * As close as possible to the segment base (or end), because
     * This helps to reduce the computing time */

    /* Attaching the end of the track. */
    BOARD_ITEM* LockPoint = LocateLockPoint( GetBoard(), pos, masquelayer );

    if( LockPoint ) /* End of trace is on a pad. */
    {
        if( LockPoint->Type() ==  TYPE_PAD )
        {
            EnsureEndTrackOnPad( (D_PAD*) LockPoint );
        }
        else    /* End of is on a different track, it will
                 * possibly create an anchor. */
        {
            TRACK* adr_buf = (TRACK*) LockPoint;
            g_HightLigth_NetCode = adr_buf->GetNet();

            /* Possible establishment of a hanging point. */
            LockPoint = CreateLockPoint( g_CurrentTrackSegment->m_End,
                                         adr_buf,
                                         g_CurrentTrackSegment,
                                         &s_ItemsListPicker );
        }
    }

    // Delete Null segments:
    DeleteNullTrackSegments( GetBoard(), g_CurrentTrackList );

    // Insert new segments if they exist.  This can be NULL on a double click
    // on the start point
    if( g_FirstTrackSegment != NULL )
    {
        int    netcode    = g_FirstTrackSegment->GetNet();
        TRACK* firstTrack = g_FirstTrackSegment;
        int    newCount   = g_CurrentTrackList.GetCount();

        // Put entire new current segment list in BOARD, ans prepare undo
        // command
        TRACK* track;
        TRACK* insertBeforeMe = g_CurrentTrackSegment->GetBestInsertPoint(
             GetBoard() );
        while( ( track = g_CurrentTrackList.PopFront() ) != NULL )
        {
            ITEM_PICKER picker( track, UR_NEW );
            s_ItemsListPicker.PushItem( picker );
            GetBoard()->m_Track.Insert( track, insertBeforeMe );
        }

        trace_ratsnest_pad( DC );

        Trace_Une_Piste( DrawPanel, DC, firstTrack, newCount, GR_OR );

        int i = 0;
        for( track = firstTrack; track && i<newCount; ++i, track = track->Next() )
        {
            track->m_Flags = 0;
            track->SetState( BUSY, OFF );
        }

        // erase the old track, if exists
        if( g_AutoDeleteOldTrack )
        {
            EraseRedundantTrack( DC, firstTrack, newCount, &s_ItemsListPicker );
        }
        SaveCopyInUndoList( s_ItemsListPicker, UR_UNSPECIFIED );
        s_ItemsListPicker.ClearItemsList(); // s_ItemsListPicker is no more
                                            // owner of picked items

        /* compute the new rastnest */
        test_1_net_connexion( DC, netcode );

        GetScreen()->SetModify();
        GetBoard()->DisplayInfo( this );
    }

    wxASSERT( g_FirstTrackSegment==NULL );
    wxASSERT( g_CurrentTrackSegment==NULL );
    wxASSERT( g_CurrentTrackList.GetCount()==0 );

    if( g_HightLigt_Status )
        Hight_Light( DC );

    g_HightLigth_NetCode = OldNetCodeSurbrillance;
    if( OldEtatSurbrillance )
        Hight_Light( DC );

    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
}


TRACK* LocateIntrusion( TRACK* listStart, TRACK* aTrack )
{
    int     net   = aTrack->GetNet();
    int     width = aTrack->m_Width;
    int     layer = ( (PCB_SCREEN*) ActiveScreen )->m_Active_Layer;

    wxPoint ref = ActiveScreen->RefPos( true );

    TRACK*  found = NULL;

    for( TRACK* track = listStart; track; track = track->Next() )
    {
        if( track->Type() == TYPE_TRACK )    // skip vias
        {
            if( track->GetState( BUSY | DELETED ) )
                continue;

            if( layer != track->GetLayer() )
                continue;

            if( track->GetNet() == net )
                continue;

            /* TRACK::HitTest */
            int     dist = (width + track->m_Width) / 2 + aTrack->GetClearance(
                track );

            wxPoint pos = ref - track->m_Start;
            wxPoint vec = track->m_End - track->m_Start;

            if( !DistanceTest( dist, vec.x, vec.y, pos.x, pos.y ) )
                continue;

            found = track;

            /* prefer intrusions from the side, not the end */
            double tmp = (double) pos.x * vec.x + (double) pos.y * vec.y;
            if( tmp >= 0 && tmp <= (double) vec.x * vec.x + (double) vec.y *
                vec.y )
                break;
        }
    }

    return found;
}


/**
 * Function PushTrack
 * detects if the mouse is pointing into a conflicting track.
 * In this case, it tries to push the new track out of the conflicting track's
 * clearance zone. This gives us a cheap mechanism for drawing tracks that
 * tightly follow others, independent of grid settings.
 *
 * KNOWN BUGS:
 * - we do the same sort of search and calculation up to three times:
 *   1) we search for magnetic hits (in controle.cpp)
 *   2) we check if there's a DRC violation in the making (also controle.cpp)
 *   3) we try to fix the DRC violation (here)
 * - if we have a magnetic hit and a DRC violation at the same time, we choose
 *   the magnetic hit instead of solving the violation
 * - should locate conflicting tracks also when we're crossing over them
 */
static void PushTrack( WinEDA_DrawPanel* panel )
{
    BOARD*  pcb    = ( (WinEDA_BasePcbFrame*) (panel->m_Parent) )->GetBoard();
    wxPoint cursor = ActiveScreen->m_Curseur;
    wxPoint cv, vec, n;
    TRACK*  track = g_CurrentTrackSegment;
    TRACK*  other;
    double  det;
    int     dist;
    double  f;

    other = LocateIntrusion( pcb->m_Track, track );

    /* are we currently pointing into a conflicting trace ? */
    if( !other )
        return;

    if( other->GetNet() == track->GetNet() )
        return;

    cv  = cursor - other->m_Start;
    vec = other->m_End - other->m_Start;

    det = (double) cv.x * vec.y - (double) cv.y * vec.x;

    /* cursor is right at the center of the old track */
    if( !det )
        return;

    dist =
        (track->m_Width +
         1) / 2 + (other->m_Width + 1) / 2 + track->GetClearance(
            other ) + 2;

    /*
     * DRC wants >, so +1.
     * We may have a quantization error of 1/sqrt(2), so +1 again.
     */

    /* Vector "n" is perpendicular to "other", pointing towards the cursor. */
    if( det > 0 )
    {
        n.x = vec.y;
        n.y = -vec.x;
    }
    else
    {
        n.x = -vec.y;
        n.y = vec.x;
    }
    f   = dist / hypot( double(n.x), double(n.y) );
    n.x = wxRound( f * n.x );
    n.y = wxRound( f * n.y );

    Project( &track->m_End, cursor, other );
    track->m_End += n;
}


/* Redraw the current track beiing created when the mouse cursor is moved
 */
void ShowNewTrackWhenMovingCursor( WinEDA_DrawPanel* panel,
                                   wxDC*             DC,
                                   bool              erase )
{
    D( g_CurrentTrackList.VerifyListIntegrity(); );

    PCB_SCREEN*          screen = (PCB_SCREEN*) panel->GetScreen();
    WinEDA_BasePcbFrame* frame  = (WinEDA_BasePcbFrame*) panel->m_Parent;

    bool      Track_fill_copy = DisplayOpt.DisplayPcbTrackFill;
    DisplayOpt.DisplayPcbTrackFill = true;
    int       showTrackClearanceMode = DisplayOpt.ShowTrackClearanceMode;

    NETCLASS* netclass = g_FirstTrackSegment->GetNetClass();

    if( showTrackClearanceMode != DO_NOT_SHOW_CLEARANCE )
        DisplayOpt.ShowTrackClearanceMode = SHOW_CLEARANCE_ALWAYS;

    /* Erase old track */
    if( erase )
    {
        Trace_Une_Piste( panel,
                         DC,
                         g_FirstTrackSegment,
                         g_CurrentTrackList.GetCount(),
                         GR_XOR );

        frame->trace_ratsnest_pad( DC );

        if( showTrackClearanceMode >= SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS )
        {
            int color =
                g_DesignSettings.m_LayerColor[g_CurrentTrackSegment->GetLayer()];

            GRCircle( &panel->m_ClipBox, DC, g_CurrentTrackSegment->m_End.x,
                      g_CurrentTrackSegment->m_End.y,
                      ( netclass->GetViaDiameter() / 2 ) + netclass->GetClearance(),
                      color );
        }
    }

    // MacOSX seems to need this.
    if( g_CurrentTrackList.GetCount() == 0 )
        return;

    // Set track parameters, that can be modified while creating the track
    g_CurrentTrackSegment->SetLayer( screen->m_Active_Layer );
    g_CurrentTrackSegment->m_Width = frame->GetBoard()->GetCurrentTrackWidth();

    if( g_TwoSegmentTrackBuild )
    {
        TRACK* previous_track = g_CurrentTrackSegment->Back();
        if( previous_track  &&  previous_track->Type()==TYPE_TRACK )
        {
            previous_track->SetLayer( screen->m_Active_Layer );

            if( !g_DesignSettings.m_UseConnectedTrackWidth )
                previous_track->m_Width =
                    frame->GetBoard()->GetCurrentTrackWidth();
        }
    }

    if( Track_45_Only )
    {
        if( g_TwoSegmentTrackBuild )
        {
            g_CurrentTrackSegment->m_End = ActiveScreen->m_Curseur;

            if( Drc_On )
                PushTrack( panel );

            ComputeBreakPoint( g_CurrentTrackSegment,
                               g_CurrentTrackList.GetCount(),
                               g_CurrentTrackSegment->m_End );
        }
        else
        {
            /* Calculate of the end of the path for the permitted directions:
             * horizontal, vertical or 45 degrees.
             */
            Calcule_Coord_Extremite_45( g_CurrentTrackSegment->m_Start.x,
                                        g_CurrentTrackSegment->m_Start.y,
                                        &g_CurrentTrackSegment->m_End.x,
                                        &g_CurrentTrackSegment->m_End.y );
        }
    }
    else    /* Here the angle is arbitrary */
    {
        g_CurrentTrackSegment->m_End = screen->m_Curseur;
    }

    /* Redraw the new track */
    D( g_CurrentTrackList.VerifyListIntegrity(); );
    Trace_Une_Piste( panel,
                     DC,
                     g_FirstTrackSegment,
                     g_CurrentTrackList.GetCount(),
                     GR_XOR );

    if( showTrackClearanceMode >= SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS )
    {
        int color =
            g_DesignSettings.m_LayerColor[g_CurrentTrackSegment->GetLayer()];

        GRCircle( &panel->m_ClipBox, DC, g_CurrentTrackSegment->m_End.x,
                  g_CurrentTrackSegment->m_End.y,
                  ( netclass->GetViaDiameter() / 2 ) + netclass->GetClearance(),
                  color );
    }

    /* Display info about currrent segment and the full new track:
     *  Choose the interesting segment: because we are using a 2 segments step,
     *  the last segment can be null, and the previous segment can be the
     * interesting segment.
     */
    TRACK* isegm = g_CurrentTrackSegment;
    if( isegm->GetLength() == 0 && g_CurrentTrackSegment->Back() )
        isegm = g_CurrentTrackSegment->Back();

    // display interesting segment info only:
    isegm->DisplayInfoBase( frame );

    // Add current track length
    int      trackLen = 0;
    wxString msg;
    for( TRACK* track = g_FirstTrackSegment; track; track = track->Next() )
        trackLen += track->GetLength();

    valeur_param( trackLen, msg );
    frame->AppendMsgPanel( _( "Track Len" ), msg, DARKCYAN );

    // Add current segments count (number of segments in this new track):
    msg.Printf( wxT( "%d" ), g_CurrentTrackList.GetCount() );
    frame->AppendMsgPanel( _( "Segs Count" ), msg, DARKCYAN );

    DisplayOpt.ShowTrackClearanceMode = showTrackClearanceMode;
    DisplayOpt.DisplayPcbTrackFill    = Track_fill_copy;

    frame->build_ratsnest_pad( NULL, g_CurrentTrackSegment->m_End, false );
    frame->trace_ratsnest_pad( DC );
}


/* Determine the coordinate to advanced the the current segment
 * in 0, 90, or 45 degrees, depending on position of  origin and
 * cursor position.
 */
void Calcule_Coord_Extremite_45( int ox, int oy, int* fx, int* fy )
{
    int deltax, deltay, angle;

    deltax = ActiveScreen->m_Curseur.x - ox;
    deltay = ActiveScreen->m_Curseur.y - oy;

    deltax = abs( deltax );
    deltay = abs( deltay );
    angle  = 45;

    if( deltax >= deltay )
    {
        if( deltax == 0 )
            angle = 0;
        else if( ( (deltay << 6 ) / deltax ) < 26 )
            angle = 0;
    }
    else
    {
        angle = 45;

        if( deltay == 0 )
            angle = 90;
        else if( ( (deltax << 6 ) / deltay ) < 26 )
            angle = 90;
    }

    switch( angle )
    {
    case 0:
        *fx = ActiveScreen->m_Curseur.x;
        *fy = oy;
        break;

    case 45:
        deltax = MIN( deltax, deltay );
        deltay = deltax;

        /* Recalculate the signs fo deltax and deltaY. */
        if( ( ActiveScreen->m_Curseur.x - ox ) < 0 )
            deltax = -deltax;
        if( ( ActiveScreen->m_Curseur.y - oy ) < 0 )
            deltay = -deltay;

        *fx = ox + deltax;
        *fy = oy + deltay;
        break;

    case 90:
        *fx = ox;
        *fy = ActiveScreen->m_Curseur.y;
        break;
    }
}


/**
 * Compute new track angle based on previous track.
 */
void ComputeBreakPoint( TRACK* track, int SegmentCount, wxPoint end )
{
    int iDx    = 0;
    int iDy    = 0;
    int iAngle = 0;

    if( SegmentCount <= 0 )
        return;
    if( track == NULL )
        return;

    TRACK* newTrack = track;
    track = track->Back();
    SegmentCount--;
    if( track )
    {
        iDx = end.x - track->m_Start.x;
        iDy = end.y - track->m_Start.y;

        iDx = abs( iDx );
        iDy = abs( iDy );
    }

    TRACK* lastTrack = track ? track->Back() : NULL;
    if( lastTrack )
    {
        if( (lastTrack->m_End.x == lastTrack->m_Start.x)
           || (lastTrack->m_End.y == lastTrack->m_Start.y) )
        {
            iAngle = 45;
        }
    }

    if( iAngle == 0 )
    {
        if( iDx >= iDy )
            iAngle = 0;
        else
            iAngle = 90;
    }

    if( track == NULL )
        iAngle = -1;

    switch( iAngle )
    {
    case -1:
        break;

    case 0:
        if( ( end.x - track->m_Start.x ) < 0 )
            track->m_End.x = end.x + iDy;
        else
            track->m_End.x = end.x - iDy;
        track->m_End.y = track->m_Start.y;
        break;

    case 45:
        iDx = MIN( iDx, iDy );
        iDy = iDx;

        /* Recalculate the signs fo deltax and deltaY. */
        if( ( end.x - track->m_Start.x ) < 0 )
            iDx = -iDx;
        if( ( end.y - track->m_Start.y ) < 0 )
            iDy = -iDy;

        track->m_End.x = track->m_Start.x + iDx;
        track->m_End.y = track->m_Start.y + iDy;
        break;

    case 90:
        if( ( end.y - track->m_Start.y ) < 0 )
            track->m_End.y = end.y + iDx;
        else
            track->m_End.y = end.y - iDx;

        track->m_End.x = track->m_Start.x;
        break;
    }

    if( track )
    {
        if( track->IsNull() )
            track->m_End = end;

        newTrack->m_Start = track->m_End;
    }
    newTrack->m_End = end;
}


/* Delete track segments which have len = 0; after creating a new track
 *  return a pointer on the first segment (start of track list)
 */
void DeleteNullTrackSegments( BOARD* pcb, DLIST<TRACK>& aTrackList )
{
    if( aTrackList.GetCount() == 0 )
        return;

    TRACK*      track = aTrackList.GetFirst();
    TRACK*      firsttrack = track;
    TRACK*      oldtrack;

    BOARD_ITEM* LockPoint = track->start;
    while( track != NULL )
    {
        oldtrack = track;
        track    = track->Next();
        if( !oldtrack->IsNull() )
        {
            continue;
        }

        // NULL segment, delete it
        if( firsttrack == oldtrack )
            firsttrack = track;

        delete aTrackList.Remove( oldtrack );
    }

    if( aTrackList.GetCount() == 0 )
        return;         // all the new track segments have been deleted

    // we must set the pointers on connected items and the connection status
    oldtrack = track = firsttrack;
    firsttrack->start = NULL;
    while( track != NULL )
    {
        oldtrack = track;
        track    = track->Next();
        oldtrack->end = track;

        if( track )
            track->start = oldtrack;

        oldtrack->SetStatus( 0 );
    }

    firsttrack->start = LockPoint;
    if( LockPoint &&  LockPoint->Type()==TYPE_PAD )
        firsttrack->SetState( BEGIN_ONPAD, ON );

    track = firsttrack;
    while( track != NULL )
    {
        TRACK* next_track = track->Next();
        LockPoint = Locate_Pad_Connecte( pcb, track, END );
        if( LockPoint )
        {
            track->end = LockPoint;
            track->SetState( END_ONPAD, ON );
            if( next_track )
            {
                next_track->start = LockPoint;
                next_track->SetState( BEGIN_ONPAD, ON );
            }
        }
        track = next_track;
    }
}


/* Ensure the end point of g_CurrentTrackSegment is on the pas "Pad"
 *  if no, create a new track segment if necessary
 *  and move current (or new) end segment on pad
 */
void EnsureEndTrackOnPad( D_PAD* Pad )
{
    if( g_CurrentTrackSegment->m_End == Pad->m_Pos ) // Ok !
    {
        g_CurrentTrackSegment->end = Pad;
        g_CurrentTrackSegment->SetState( END_ONPAD, ON );
        return;
    }

    TRACK* lasttrack = g_CurrentTrackSegment;
    if( !g_CurrentTrackSegment->IsNull() )
    {
        /* Must create a new segment, from track end to pad center */
        g_CurrentTrackList.PushBack( lasttrack->Copy() );

        lasttrack->end = g_CurrentTrackSegment;
    }

    g_CurrentTrackSegment->m_End = Pad->m_Pos;
    g_CurrentTrackSegment->SetState( END_ONPAD, OFF );

    g_CurrentTrackSegment->end = Pad;
    g_CurrentTrackSegment->SetState( END_ONPAD, ON );
}
