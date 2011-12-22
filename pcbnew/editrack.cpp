/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file editrack.cpp
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "pcbcommon.h"
#include "wxPcbStruct.h"
#include "colors_selection.h"

#include "pcbnew.h"
#include "drc_stuff.h"
#include "protos.h"

#include "class_board.h"
#include "class_track.h"
#include "class_zone.h"


static void Abort_Create_Track( EDA_DRAW_PANEL* panel, wxDC* DC );
void        ShowNewTrackWhenMovingCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                          const wxPoint& aPosition, bool aErase );
static void ComputeBreakPoint( TRACK* track, int n, wxPoint end );
static void DeleteNullTrackSegments( BOARD* pcb, DLIST<TRACK>& aTrackList );
static void EnsureEndTrackOnPad( D_PAD* Pad );

static PICKED_ITEMS_LIST s_ItemsListPicker;


/* Function called to abort a track creation
 */
static void Abort_Create_Track( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    PCB_EDIT_FRAME* frame = (PCB_EDIT_FRAME*) Panel->GetParent();
    BOARD* pcb = frame->GetBoard();
    TRACK* track = (TRACK*) frame->GetCurItem();

    if( track && ( track->Type()==PCB_VIA_T || track->Type()==PCB_TRACE_T ) )
    {
        /* Erase the current drawing */
        ShowNewTrackWhenMovingCursor( Panel, DC, wxDefaultPosition, false );

        if( pcb->IsHighLightNetON() )
            frame->HighLight( DC );

        pcb->PopHighLight();

        if( pcb->IsHighLightNetON() )
            pcb->DrawHighLight( Panel, DC, pcb->GetHighLightNetCode() );

        frame->ClearMsgPanel();

        // Undo pending changes (mainly a lock point creation) and clear the
        // undo picker list:
        frame->PutDataInPreviousState( &s_ItemsListPicker, false, false );
        s_ItemsListPicker.ClearListAndDeleteItems();

        // Delete current (new) track
        g_CurrentTrackList.DeleteAll();
    }

    frame->SetCurItem( NULL );
}

/*
 * This function starts a new track segment.
 * If a new track segment is in progress, ends this current new segment,
 * and created a new one.
 */
TRACK* PCB_EDIT_FRAME::Begin_Route( TRACK* aTrack, wxDC* aDC )
{
    TRACK*      TrackOnStartPoint = NULL;
    int         layerMask = GetLayerMask( GetScreen()->m_Active_Layer );
    BOARD_CONNECTED_ITEM* LockPoint;
    wxPoint     pos = GetScreen()->GetCrossHairPosition();

    if( aTrack == NULL )  /* Starting a new track segment */
    {
        m_canvas->SetMouseCapture( ShowNewTrackWhenMovingCursor, Abort_Create_Track );

        // Prepare the undo command info
        s_ItemsListPicker.ClearListAndDeleteItems();  // Should not be necessary, but...

        GetBoard()->PushHighLight();

        // erase old highlight
        if( GetBoard()->IsHighLightNetON() )
            HighLight( aDC );

        g_CurrentTrackList.PushBack( new TRACK( GetBoard() ) );
        g_CurrentTrackSegment->SetFlags( IS_NEW );

        GetBoard()->SetHighLightNet( 0 );

        // Search for a starting point of the new track, a track or pad
        LockPoint = GetBoard()->GetLockPoint( pos, layerMask );

        D_PAD* pad = NULL;
        if( LockPoint ) // An item (pad or track) is found
        {
            if( LockPoint->Type() == PCB_PAD_T )
            {
                pad = (D_PAD*) LockPoint;

                /* A pad is found: put the starting point on pad center */
                pos = pad->m_Pos;
                GetBoard()->SetHighLightNet( pad->GetNet() );
            }
            else /* A track segment is found */
            {
                TrackOnStartPoint    = (TRACK*) LockPoint;
                GetBoard()->SetHighLightNet( TrackOnStartPoint->GetNet() );
                GetBoard()->CreateLockPoint( pos, TrackOnStartPoint, &s_ItemsListPicker );
            }
        }
        else
        {
            // Not a starting point, but a filled zone area can exist. This is also a
            // good starting point.
            ZONE_CONTAINER* zone;
            zone = GetBoard()->HitTestForAnyFilledArea( pos, GetScreen()-> m_Active_Layer );

            if( zone )
                GetBoard()->SetHighLightNet( zone->GetNet() );
        }

        D( g_CurrentTrackList.VerifyListIntegrity(); );

        BuildAirWiresTargetsList( LockPoint, wxPoint( 0, 0 ), true );

        D( g_CurrentTrackList.VerifyListIntegrity(); );

        GetBoard()->HighLightON();
        GetBoard()->DrawHighLight( m_canvas, aDC, GetBoard()->GetHighLightNetCode() );

        // Display info about track Net class, and init track and vias sizes:
        g_CurrentTrackSegment->SetNet( GetBoard()->GetHighLightNetCode() );
        GetBoard()->SetCurrentNetClass( g_CurrentTrackSegment->GetNetClassName() );

        g_CurrentTrackSegment->SetLayer( GetScreen()->m_Active_Layer );
        g_CurrentTrackSegment->m_Width = GetBoard()->GetCurrentTrackWidth();

        if( GetBoard()->GetDesignSettings().m_UseConnectedTrackWidth )
        {
            if( TrackOnStartPoint && TrackOnStartPoint->Type() == PCB_TRACE_T )
                g_CurrentTrackSegment->m_Width = TrackOnStartPoint->m_Width;
        }

        g_CurrentTrackSegment->m_Start = pos;
        g_CurrentTrackSegment->m_End   = pos;

        if( pad )
        {
            g_CurrentTrackSegment->m_PadsConnected.push_back( pad );
            // Useful to display track length, if the pad has a die length:
            g_CurrentTrackSegment->SetState( BEGIN_ONPAD, ON );
            g_CurrentTrackSegment->start = pad;
        }

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
        m_canvas->m_mouseCaptureCallback( m_canvas, aDC, wxDefaultPosition, false );

        if( Drc_On )
        {
            if( BAD_DRC == m_drc->Drc( g_CurrentTrackSegment, GetBoard()->m_Track ) )
            {
                return g_CurrentTrackSegment;
            }
        }
    }
    else   // Track in progress : segment coordinates are updated by ShowNewTrackWhenMovingCursor.
    {
        /* Test for a D.R.C. error: */
        if( Drc_On )
        {
            if( BAD_DRC == m_drc->Drc( g_CurrentTrackSegment, GetBoard()->m_Track ) )
                return NULL;

            // We must handle 2 segments
            if( g_TwoSegmentTrackBuild && g_CurrentTrackSegment->Back() )
            {
                if( BAD_DRC == m_drc->Drc( g_CurrentTrackSegment->Back(), GetBoard()->m_Track ) )
                    return NULL;
            }
        }

        /* Current track is Ok: current segment is kept, and a new one is
         * created unless the current segment is null, or 2 last are null
         * if this is a 2 segments track build.
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

            ShowNewTrackWhenMovingCursor( m_canvas, aDC, wxDefaultPosition, false );

            D( g_CurrentTrackList.VerifyListIntegrity(); );

            if( g_Raccord_45_Auto )
                Add45DegreeSegment( aDC );

            TRACK* previousTrack = g_CurrentTrackSegment;

            TRACK* newTrack = g_CurrentTrackSegment->Copy();
            g_CurrentTrackList.PushBack( newTrack );
            newTrack->SetFlags( IS_NEW );

            newTrack->SetState( BEGIN_ONPAD | END_ONPAD, OFF );

            D_PAD* pad = GetBoard()->GetPad( previousTrack, END );

            if( pad )
            {
                newTrack->m_PadsConnected.push_back( pad );
                previousTrack->m_PadsConnected.push_back( pad );
            }

            newTrack->start = previousTrack->end;

            D( g_CurrentTrackList.VerifyListIntegrity(); );

            newTrack->m_Start = newTrack->m_End;

            newTrack->SetLayer( GetScreen()->m_Active_Layer );

            if( !GetBoard()->GetDesignSettings().m_UseConnectedTrackWidth )
                newTrack->m_Width = GetBoard()->GetCurrentTrackWidth();

            D( g_CurrentTrackList.VerifyListIntegrity(); );

            /* Show the new position */
            ShowNewTrackWhenMovingCursor( m_canvas, aDC, wxDefaultPosition, false );
        }
    }

    SetCurItem( g_CurrentTrackSegment, false );
    return g_CurrentTrackSegment;
}


bool PCB_EDIT_FRAME::Add45DegreeSegment( wxDC* aDC )
{
    int dx0, dy0, dx1, dy1;

    if( g_CurrentTrackList.GetCount() < 2 )
        return false;                         /* There must be 2 segments. */

    TRACK* curTrack  = g_CurrentTrackSegment;
    TRACK* prevTrack = curTrack->Back();

    // Test if we have 2 consecutive track segments ( not via ) to connect.
    if( curTrack->Type() != PCB_TRACE_T || prevTrack->Type() != PCB_TRACE_T )
    {
        return false;
    }

    int segm_step_45 = wxRound( GetScreen()->GetGridSize().x / 2 );

    if( segm_step_45 < ( curTrack->m_Width * 2 ) )
        segm_step_45 = curTrack->m_Width * 2;

    // Test if the segments are horizontal or vertical.
    dx0 = prevTrack->m_End.x - prevTrack->m_Start.x;
    dy0 = prevTrack->m_End.y - prevTrack->m_Start.y;

    dx1 = curTrack->m_End.x - curTrack->m_Start.x;
    dy1 = curTrack->m_End.y - curTrack->m_Start.y;

    // Segments must be of sufficient length.
    if( MAX( abs( dx0 ), abs( dy0 ) ) < ( segm_step_45 * 2 ) )
        return false;

    if( MAX( abs( dx1 ), abs( dy1 ) ) < ( segm_step_45 * 2 ) )
        return false;

    /* Create a new segment and connect it with the previous 2 segments. */
    TRACK* newTrack = curTrack->Copy();

    newTrack->m_Start = prevTrack->m_End;
    newTrack->m_End   = curTrack->m_Start;

    if( dx0 == 0 )          // Previous segment is Vertical
    {
        if( dy1 != 0 )      // 2 segments are not 90 degrees.
        {
            delete newTrack;
            return false;
        }

        /* Calculate coordinates the connection point.
         * The new segment connects the 1st vertical segment and the 2nd
         * horizontal segment.
         */
        if( dy0 > 0 )
            newTrack->m_Start.y -= segm_step_45;
        else
            newTrack->m_Start.y += segm_step_45;

        if( dx1 > 0 )
            newTrack->m_End.x += segm_step_45;
        else
            newTrack->m_End.x -= segm_step_45;

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

    if( dy0 == 0 )      // Previous segment is horizontal
    {
        if( dx1 != 0 )  // 2 segments are not 90 degrees
        {
            delete newTrack;
            return false;
        }

        /*  Calculate the coordinates of the point of connection:
         * A new segment has been created, connecting segment 1
         * (horizontal) and segment 2 (vertical)
         */
        if( dx0 > 0 )
            newTrack->m_Start.x -= segm_step_45;
        else
            newTrack->m_Start.x += segm_step_45;

        if( dy1 > 0 )
            newTrack->m_End.y += segm_step_45;
        else
            newTrack->m_End.y -= segm_step_45;

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


bool PCB_EDIT_FRAME::End_Route( TRACK* aTrack, wxDC* aDC )
{
    int layerMask = GetLayerMask( GetScreen()->m_Active_Layer );

    if( aTrack == NULL )
        return false;

    if( Drc_On && BAD_DRC == m_drc->Drc( g_CurrentTrackSegment, GetBoard()->m_Track ) )
        return false;

    /* Saving the coordinate of end point of the trace */
    wxPoint pos = g_CurrentTrackSegment->m_End;

    D( g_CurrentTrackList.VerifyListIntegrity(); );

    if( Begin_Route( aTrack, aDC ) == NULL )
        return false;

    ShowNewTrackWhenMovingCursor( m_canvas, aDC, wxDefaultPosition, true );
    ShowNewTrackWhenMovingCursor( m_canvas, aDC, wxDefaultPosition, false );
    TraceAirWiresToTargets( aDC );

    /* cleanup
     *  if( g_CurrentTrackSegment->Next() != NULL )
     *  {
     *   delete g_CurrentTrackSegment->Next();
     *   g_CurrentTrackSegment->SetNext( NULL );
     *  }
     */

    D( g_CurrentTrackList.VerifyListIntegrity(); );


    /* The track here is now chained to the list of track segments.
     * It must be seen in the area of net
     * As close as possible to the segment base (or end), because
     * This helps to reduce the computing time */

    // Attaching the end point of the new track to a pad or a track
    BOARD_CONNECTED_ITEM* LockPoint = GetBoard()->GetLockPoint( pos, layerMask );

    if( LockPoint )
    {
        if( LockPoint->Type() ==  PCB_PAD_T )     // End of track is on a pad.
        {
            EnsureEndTrackOnPad( (D_PAD*) LockPoint );
        }
        else        // If end point of is on a different track,
                    // creates a lock point if not exists
        {
            TRACK* adr_buf = (TRACK*) LockPoint;
            GetBoard()->SetHighLightNet( adr_buf->GetNet() );

            // Creates a lock point (if not already exists):
            LockPoint = GetBoard()->CreateLockPoint( g_CurrentTrackSegment->m_End,
                                                     adr_buf,
                                                     &s_ItemsListPicker );
        }
    }

    // Delete null length segments:
    DeleteNullTrackSegments( GetBoard(), g_CurrentTrackList );

    // Insert new segments if they exist.
    // g_FirstTrackSegment can be NULL on a double click on the starting point
    if( g_FirstTrackSegment != NULL )
    {
        int    netcode    = g_FirstTrackSegment->GetNet();
        TRACK* firstTrack = g_FirstTrackSegment;
        int    newCount   = g_CurrentTrackList.GetCount();

        // Put entire new current segment list in BOARD, and prepare undo command
        TRACK* track;
        TRACK* insertBeforeMe = g_CurrentTrackSegment->GetBestInsertPoint( GetBoard() );

        while( ( track = g_CurrentTrackList.PopFront() ) != NULL )
        {
            ITEM_PICKER picker( track, UR_NEW );
            s_ItemsListPicker.PushItem( picker );
            GetBoard()->m_Track.Insert( track, insertBeforeMe );
        }

        TraceAirWiresToTargets( aDC );

        int i = 0;

        for( track = firstTrack; track && i < newCount; ++i, track = track->Next() )
        {
            track->ClearFlags();
            track->SetState( BUSY, OFF );
        }

        // delete the old track, if it exists and is redundant
        if( g_AutoDeleteOldTrack )
        {
            EraseRedundantTrack( aDC, firstTrack, newCount, &s_ItemsListPicker );
        }

        SaveCopyInUndoList( s_ItemsListPicker, UR_UNSPECIFIED );
        s_ItemsListPicker.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items

        // compute the new ratsnest
        TestNetConnection( aDC, netcode );
        OnModify();
        GetBoard()->DisplayInfo( this );

        // Redraw the entire new track.
        DrawTraces( m_canvas, aDC, firstTrack, newCount, GR_OR );
    }

    wxASSERT( g_FirstTrackSegment == NULL );
    wxASSERT( g_CurrentTrackSegment == NULL );
    wxASSERT( g_CurrentTrackList.GetCount() == 0 );

    if( GetBoard()->IsHighLightNetON() )
        HighLight( aDC );

    GetBoard()->PopHighLight();

    if( GetBoard()->IsHighLightNetON() )
        GetBoard()->DrawHighLight( m_canvas, aDC, GetBoard()->GetHighLightNetCode() );

    m_canvas->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );

    return true;
}


TRACK* LocateIntrusion( TRACK* listStart, TRACK* aTrack, int aLayer, const wxPoint& aRef )
{
    int     net   = aTrack->GetNet();
    int     width = aTrack->m_Width;

    TRACK*  found = NULL;

    for( TRACK* track = listStart; track; track = track->Next() )
    {
        if( track->Type() == PCB_TRACE_T )    // skip vias
        {
            if( track->GetState( BUSY | IS_DELETED ) )
                continue;

            if( aLayer != track->GetLayer() )
                continue;

            if( track->GetNet() == net )
                continue;

            /* TRACK::HitTest */
            int     dist = (width + track->m_Width) / 2 + aTrack->GetClearance( track );

            wxPoint pos = aRef - track->m_Start;
            wxPoint vec = track->m_End - track->m_Start;

            if( !DistanceTest( dist, vec.x, vec.y, pos.x, pos.y ) )
                continue;

            found = track;

            /* prefer intrusions from the side, not the end */
            double tmp = (double) pos.x * vec.x + (double) pos.y * vec.y;

            if( tmp >= 0 && tmp <= (double) vec.x * vec.x + (double) vec.y * vec.y )
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
static void PushTrack( EDA_DRAW_PANEL* panel )
{
    PCB_SCREEN* screen = ( (PCB_BASE_FRAME*) (panel->GetParent()) )->GetScreen();
    BOARD*  pcb    = ( (PCB_BASE_FRAME*) (panel->GetParent()) )->GetBoard();
    wxPoint cursor = screen->GetCrossHairPosition();
    wxPoint cv, vec, n;
    TRACK*  track = g_CurrentTrackSegment;
    TRACK*  other;
    double  det;
    int     dist;
    double  f;

    other = LocateIntrusion( pcb->m_Track, track, screen->m_Active_Layer, screen->RefPos( true ) );

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

    dist = (track->m_Width + 1) / 2 + (other->m_Width + 1) / 2 + track->GetClearance( other ) + 2;

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


/* Redraw the current track being created when the mouse cursor is moved
 */
void ShowNewTrackWhenMovingCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                   bool aErase )
{
    D( g_CurrentTrackList.VerifyListIntegrity(); );

    PCB_SCREEN*     screen = (PCB_SCREEN*) aPanel->GetScreen();
    PCB_BASE_FRAME* frame  = (PCB_BASE_FRAME*) aPanel->GetParent();

    bool      Track_fill_copy = DisplayOpt.DisplayPcbTrackFill;
    DisplayOpt.DisplayPcbTrackFill = true;
    TRACE_CLEARANCE_DISPLAY_MODE_T showTrackClearanceMode = DisplayOpt.ShowTrackClearanceMode;

    if ( g_FirstTrackSegment == NULL )
        return;

    NETCLASS* netclass = g_FirstTrackSegment->GetNetClass();

    if( showTrackClearanceMode != DO_NOT_SHOW_CLEARANCE )
        DisplayOpt.ShowTrackClearanceMode = SHOW_CLEARANCE_ALWAYS;

#ifndef USE_WX_OVERLAY
    /* Erase old track */
    if( aErase )
    {
        DrawTraces( aPanel, aDC, g_FirstTrackSegment, g_CurrentTrackList.GetCount(), GR_XOR );

        frame->TraceAirWiresToTargets( aDC );

        if( showTrackClearanceMode >= SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS )
        {
            int color = g_ColorsSettings.GetLayerColor( g_CurrentTrackSegment->GetLayer() );

            GRCircle( &aPanel->m_ClipBox, aDC, g_CurrentTrackSegment->m_End.x,
                      g_CurrentTrackSegment->m_End.y,
                      ( netclass->GetViaDiameter() / 2 ) + netclass->GetClearance(),
                      color );
        }
    }
#endif
    // MacOSX seems to need this.
    if( g_CurrentTrackList.GetCount() == 0 )
        return;

    // Set track parameters, that can be modified while creating the track
    g_CurrentTrackSegment->SetLayer( screen->m_Active_Layer );

    if( !frame->GetBoard()->GetDesignSettings().m_UseConnectedTrackWidth )
        g_CurrentTrackSegment->m_Width = frame->GetBoard()->GetCurrentTrackWidth();

    if( g_TwoSegmentTrackBuild )
    {
        TRACK* previous_track = g_CurrentTrackSegment->Back();

        if( previous_track  &&  previous_track->Type()==PCB_TRACE_T )
        {
            previous_track->SetLayer( screen->m_Active_Layer );

            if( !frame->GetBoard()->GetDesignSettings().m_UseConnectedTrackWidth )
                previous_track->m_Width = frame->GetBoard()->GetCurrentTrackWidth();
        }
    }

    if( g_Track_45_Only_Allowed )
    {
        if( g_TwoSegmentTrackBuild )
        {
            g_CurrentTrackSegment->m_End = screen->GetCrossHairPosition();

            if( Drc_On )
                PushTrack( aPanel );

            ComputeBreakPoint( g_CurrentTrackSegment,
                               g_CurrentTrackList.GetCount(),
                               g_CurrentTrackSegment->m_End );
        }
        else
        {
            /* Calculate of the end of the path for the permitted directions:
             * horizontal, vertical or 45 degrees.
             */
            CalculateSegmentEndPoint( screen->GetCrossHairPosition(),
                                      g_CurrentTrackSegment->m_Start.x,
                                      g_CurrentTrackSegment->m_Start.y,
                                      &g_CurrentTrackSegment->m_End.x,
                                      &g_CurrentTrackSegment->m_End.y );
        }
    }
    else    /* Here the angle is arbitrary */
    {
        g_CurrentTrackSegment->m_End = screen->GetCrossHairPosition();
    }

    /* Redraw the new track */
    D( g_CurrentTrackList.VerifyListIntegrity(); );
    DrawTraces( aPanel, aDC, g_FirstTrackSegment, g_CurrentTrackList.GetCount(), GR_XOR );

    if( showTrackClearanceMode >= SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS )
    {
        int color = g_ColorsSettings.GetLayerColor(g_CurrentTrackSegment->GetLayer());

        GRCircle( &aPanel->m_ClipBox, aDC, g_CurrentTrackSegment->m_End.x,
                  g_CurrentTrackSegment->m_End.y,
                  ( netclass->GetViaDiameter() / 2 ) + netclass->GetClearance(),
                  color );
    }

    /* Display info about current segment and the full new track:
     *  Choose the interesting segment: because we are using a 2 segments step,
     *  the last segment can be null, and the previous segment can be the
     * interesting segment.
     */
    TRACK* isegm = g_CurrentTrackSegment;

    if( isegm->GetLength() == 0 && g_CurrentTrackSegment->Back() )
        isegm = g_CurrentTrackSegment->Back();

    // display interesting segment info only:
    isegm->DisplayInfoBase( frame );

    // Display current track length (on board) and the the actual track len
    // if there is an extra len due to the len die on the starting pad (if any)
    double   trackLen = 0.0;
    double   lenDie = 0.0;
    wxString msg;

    // If the starting point is on a pad, add current track length+ length die
    if( g_FirstTrackSegment->GetState( BEGIN_ONPAD ) )
    {
        D_PAD * pad = (D_PAD *) g_FirstTrackSegment->start;
        lenDie = (double) pad->m_LengthDie;
    }

    // calculate track len on board:
    for( TRACK* track = g_FirstTrackSegment; track; track = track->Next() )
        trackLen += track->GetLength();

    valeur_param( wxRound( trackLen ), msg );
    frame->AppendMsgPanel( _( "Track Len" ), msg, DARKCYAN );

    if( lenDie != 0 )      // display the track len on board and the actual track len
    {
        frame->AppendMsgPanel( _( "Full Len" ), msg, DARKCYAN );
        valeur_param( wxRound( trackLen+lenDie ), msg );
        frame->AppendMsgPanel( _( "On Die" ), msg, DARKCYAN );
    }

    // Add current segments count (number of segments in this new track):
    msg.Printf( wxT( "%d" ), g_CurrentTrackList.GetCount() );
    frame->AppendMsgPanel( _( "Segs Count" ), msg, DARKCYAN );

    DisplayOpt.ShowTrackClearanceMode = showTrackClearanceMode;
    DisplayOpt.DisplayPcbTrackFill    = Track_fill_copy;

    frame->BuildAirWiresTargetsList( NULL, g_CurrentTrackSegment->m_End, false );
    frame->TraceAirWiresToTargets( aDC );
}


/* Determine the coordinate to advanced the the current segment
 * in 0, 90, or 45 degrees, depending on position of origin and \a aPosition.
 */
void CalculateSegmentEndPoint( const wxPoint& aPosition, int ox, int oy, int* fx, int* fy )
{
    int deltax, deltay, angle;

    deltax = aPosition.x - ox;
    deltay = aPosition.y - oy;

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
        *fx = aPosition.x;
        *fy = oy;
        break;

    case 45:
        deltax = MIN( deltax, deltay );
        deltay = deltax;

        /* Recalculate the signs for deltax and deltaY. */
        if( ( aPosition.x - ox ) < 0 )
            deltax = -deltax;

        if( ( aPosition.y - oy ) < 0 )
            deltay = -deltay;

        *fx = ox + deltax;
        *fy = oy + deltay;
        break;

    case 90:
        *fx = ox;
        *fy = aPosition.y;
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
        if(( (lastTrack->m_End.x == lastTrack->m_Start.x)
           || (lastTrack->m_End.y == lastTrack->m_Start.y) )
        && !g_Alternate_Track_Posture)
        {
            iAngle = 45;
        }
    }
    else
    {
        if( g_Alternate_Track_Posture )
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

        /* Recalculate the signs for deltax and deltaY. */
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


/* Delete track segments which have len = 0 after creating a new track
 *  return a pointer on the first segment (start of track list)
 */
void DeleteNullTrackSegments( BOARD* pcb, DLIST<TRACK>& aTrackList )
{
    if( aTrackList.GetCount() == 0 )
        return;

    TRACK*      track = aTrackList.GetFirst();
    TRACK*      firsttrack = track;
    TRACK*      oldtrack;

    BOARD_CONNECTED_ITEM* LockPoint = track->start;

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

    if( LockPoint && LockPoint->Type()==PCB_PAD_T )
        firsttrack->SetState( BEGIN_ONPAD, ON );

    track = firsttrack;

    while( track != NULL )
    {
        TRACK* next_track = track->Next();
        LockPoint = pcb->GetPad( track, END );

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


/* Ensure the end point of g_CurrentTrackSegment is on the pad "Pad"
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
