/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file magnetic_tracks_functions.cpp
 */

/* functions used to control the cursor position, when creating a track
 * and when the "magnetic tracks" option is on
 * (the current created track is kept near existing tracks
 *  the distance is the clearance between tracks)
 */

#include <fctsys.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <macros.h>

#include <class_board.h>
#include <class_track.h>

#include <protos.h>
#include <pcbnew_id.h>


/**
 * Function Join
 * finds the point where line segment (b1,b0) intersects with segment (a1,a0).
 * If that point would be outside of (a0,a1), the respective endpoint is used.
 * Join returns the point in "res" and "true" if a suitable point was found,
 * "false" if both lines are parallel or if the length of either segment is zero.
 */
static bool Join( wxPoint* aIntersectPoint, wxPoint a0, wxPoint a1, wxPoint b0, wxPoint b1 )
{
    /* References:
        http://local.wasp.uwa.edu.au/~pbourke/geometry/lineline2d/
        http://www.gekkou.co.uk/blogs/monologues/2007/12/13/1197586800000.html
    */

    double  denom;
    double  t;

    // if either segment is zero length
    if( a1.x==a0.x && a1.y==a0.y )
        return false;

    if( b1.x==b0.x && b1.y==b0.y )
        return false;

    a1 -= a0;
    b1 -= b0;

    b0 -= a0;

    denom = (double) b1.y * a1.x - (double) b1.x * a1.y;

    if( !denom )
    {
        return false;       // parallel
    }

    t = ((double) b1.y * b0.x - (double) b1.x * b0.y ) / denom;

    t = std::min( std::max( t, 0.0 ), 1.0 );

    aIntersectPoint->x = KiROUND( a0.x + t * a1.x );
    aIntersectPoint->y = KiROUND( a0.y + t * a1.y );

    return true;
}


/*
 * "Project" finds the projection of a grid point on a track. This is the point
 * from where we want to draw new orthogonal tracks when starting on a track.
 */
bool Project( wxPoint* aNearPos, wxPoint on_grid, const TRACK* track )
{
    if( track->GetStart ()== track->GetEnd() )
        return false;

    wxPoint vec = track->GetEnd() - track->GetStart();

    double t = double( on_grid.x - track->GetStart().x ) * vec.x +
               double( on_grid.y - track->GetStart().y ) * vec.y;

    t /= (double) vec.x * vec.x + (double) vec.y * vec.y;
    t = std::min( std::max( t, 0.0 ), 1.0 );

    aNearPos->x = KiROUND( track->GetStart().x + t * vec.x );
    aNearPos->y = KiROUND( track->GetStart().y + t * vec.y );

    return true;
}


/**
 * Function Magnetize
 * tests to see if there are any magnetic items within near reach of the given
 * "curpos".  If yes, then curpos is adjusted appropriately according to that
 * near magnetic item and true is returned.
 * @param frame = the current frame
 * @param aCurrentTool = the current tool id (from vertical right toolbar)
 * @param aGridSize = the current grid size
 * @param on_grid = the on grid position near initial position ( often on_grid = curpos)
 * @param curpos The initial position, and what to adjust if a change is needed.
 * @return bool - true if the position was adjusted magnetically, else false.
 */
bool Magnetize( PCB_EDIT_FRAME* frame, int aCurrentTool, wxSize aGridSize,
                wxPoint on_grid, wxPoint* curpos )
{
    bool    doCheckNet = g_MagneticPadOption != capture_always && g_Drc_On;
    bool    doTrack = false;
    bool    doPad = false;
    bool    amMovingVia = false;

    BOARD* m_Pcb = frame->GetBoard();
    TRACK*      currTrack = g_CurrentTrackSegment;
    BOARD_ITEM* currItem  = frame->GetCurItem();
    PCB_SCREEN* screen = frame->GetScreen();
    wxPoint     pos = frame->RefPos( true );

    // D( printf( "currTrack=%p currItem=%p currTrack->Type()=%d currItem->Type()=%d\n",  currTrack, currItem, currTrack ? currTrack->Type() : 0, currItem ? currItem->Type() : 0 ); )

    if( !currTrack && currItem && currItem->Type()==PCB_VIA_T && currItem->GetFlags() )
    {
        // moving a VIA
        currTrack = (TRACK*) currItem;
        amMovingVia = true;

        return false;   // comment this return out and play with it.
    }
    else if( currItem != currTrack )
    {
        currTrack = NULL;
    }

    if( g_MagneticPadOption == capture_always )
        doPad = true;

    if( g_MagneticTrackOption == capture_always )
        doTrack = true;

    if( aCurrentTool == ID_TRACK_BUTT || amMovingVia )
    {
        int q = capture_cursor_in_track_tool;

        if( g_MagneticPadOption == q )
            doPad = true;

        if( g_MagneticTrackOption == q )
            doTrack = true;
    }

    // D(printf("doPad=%d doTrack=%d aCurrentTool=%d amMovingVia=%d\n", doPad, doTrack, aCurrentTool, amMovingVia );)

    //  The search precedence order is pads, then tracks/vias

    if( doPad )
    {
        LSET    layer_mask( screen->m_Active_Layer );
        D_PAD*  pad = m_Pcb->GetPad( pos, layer_mask );

        if( pad )
        {
            if( doCheckNet && currTrack && currTrack->GetNetCode() != pad->GetNetCode() )
                return false;

            *curpos = pad->GetPosition();
            return true;
        }
    }

    // after pads, only track & via tests remain, skip them if not desired
    if( doTrack )
    {
        LAYER_ID layer = screen->m_Active_Layer;

        for( TRACK* via = m_Pcb->m_Track;
                via && (via = via->GetVia( *curpos, layer )) != NULL;
                via = via->Next() )
        {
            if( via != currTrack )   // a via cannot influence itself
            {
                if( !doCheckNet || !currTrack || currTrack->GetNetCode() == via->GetNetCode() )
                {
                    *curpos = via->GetStart();
                    // D(printf("via hit\n");)
                    return true;
                }
            }
        }

        if( !currTrack )
        {
            LSET layer_mask( layer );

            TRACK* track = m_Pcb->GetTrack( m_Pcb->m_Track, pos, layer_mask );

            if( !track || track->Type() != PCB_TRACE_T )
            {
                // D(printf("!currTrack and track=%p not found, layer_mask=0x%X\n", track, layer_mask );)
                return false;
            }

            // D( printf( "Project\n" ); )
            return Project( curpos, on_grid, track );
        }

        /*
         * In two segment mode, ignore the final segment if it's inside a grid square.
         */
        if( !amMovingVia && currTrack && g_TwoSegmentTrackBuild && currTrack->Back()
            && currTrack->GetStart().x - aGridSize.x < currTrack->GetEnd().x
            && currTrack->GetStart().x + aGridSize.x > currTrack->GetEnd().x
            && currTrack->GetStart().y - aGridSize.y < currTrack->GetEnd().y
            && currTrack->GetStart().y + aGridSize.y > currTrack->GetEnd().y )
        {
            currTrack = currTrack->Back();
        }


        for( TRACK* track = m_Pcb->m_Track; track; track = track->Next() )
        {
            if( track->Type() != PCB_TRACE_T )
                continue;

            if( doCheckNet && currTrack && currTrack->GetNetCode() != track->GetNetCode() )
                continue;

            if( m_Pcb->IsLayerVisible( track->GetLayer() ) == false )
                continue;

            // omit the layer check if moving a via
            if( !amMovingVia && !track->IsOnLayer( layer ) )
                continue;

            if( !track->HitTest( *curpos ) )
                continue;

            // D(printf( "have track prospect\n");)

            if( Join( curpos, track->GetStart(), track->GetEnd(), currTrack->GetStart(), currTrack->GetEnd() ) )
            {
                // D(printf( "join currTrack->Type()=%d\n", currTrack->Type() );)
                return true;
            }

            if( aCurrentTool == ID_TRACK_BUTT || amMovingVia )
            {
                // At this point we have a drawing mouse on a track, we are drawing
                // a new track and that new track is parallel to the track the
                // mouse is on. Find the nearest end point of the track under mouse
                // to the mouse and return that.
                double distStart = GetLineLength( *curpos, track->GetStart() );
                double distEnd   = GetLineLength( *curpos, track->GetEnd() );

                // if track not via, or if its a via dragging but not with its adjacent track
                if( currTrack->Type() != PCB_VIA_T
                  || ( currTrack->GetStart() != track->GetStart() && currTrack->GetStart() != track->GetEnd() ))
                {
                    double max_dist = currTrack->GetWidth() / 2.0f;

                    if( distStart <= max_dist )
                    {
                        // D(printf("nearest end is start\n");)
                        *curpos = track->GetStart();
                        return true;
                    }

                    if( distEnd <= max_dist )
                    {
                        // D(printf("nearest end is end\n");)
                        *curpos = track->GetEnd();
                        return true;
                    }

                    // @todo otherwise confine curpos such that it stays centered within "track"
                }
            }
        }
    }

    return false;
}
