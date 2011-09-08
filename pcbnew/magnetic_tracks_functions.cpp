/********************************************************/
/* magnetic_tracks_functions.cpp */
/********************************************************/

/* functions used to controle the cursor position, when creating a track
 * and when the "magnetic tracks" option is on
 * (the current created track is kept near existing tracks
 *  the distance is the clearance between tracks)
 */

#include "fctsys.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "protos.h"
#include "pcbnew_id.h"


/**
 * Function Join
 * finds the point where line segment (b1,b0) intersects with segment (a1,a0).
 * If that point would be outside of (a0,a1), the respective endpoint is used.
 * Join returns the point in "res" and "true" if a suitable point was found,
 * "false" if both lines are parallel or if the length of either segment is zero.
 */
static bool Join( wxPoint* res, wxPoint a0, wxPoint a1, wxPoint b0, wxPoint b1 )
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

    t = min( max( t, 0.0 ), 1.0 );

    res->x = wxRound( a0.x + t * a1.x );
    res->y = wxRound( a0.y + t * a1.y );

    return true;
}


/*
 * "Project" finds the projection of a grid point on a track. This is the point
 * from where we want to draw new orthogonal tracks when starting on a track.
 */
bool Project( wxPoint* res, wxPoint on_grid, const TRACK* track )
{
    if( track->m_Start == track->m_End )
        return false;

    wxPoint vec = track->m_End - track->m_Start;

    double t = double( on_grid.x - track->m_Start.x ) * vec.x +
               double( on_grid.y - track->m_Start.y ) * vec.y;

    t /= (double) vec.x * vec.x + (double) vec.y * vec.y;
    t = min( max( t, 0.0 ), 1.0 );

    res->x = wxRound( track->m_Start.x + t * vec.x );
    res->y = wxRound( track->m_Start.y + t * vec.y );

    return true;
}


/**
 * Function Magnetize
 * tests to see if there are any magnetic items within near reach of the given
 * "curpos".  If yes, then curpos is adjusted appropriately according to that
 * near magnetic item and true is returned.
 * @param m_Pcb = the current board
 * @param frame = the current frame
 * @param aCurrentTool = the current tool id (from vertical right toolbar)
 * @param grid = the grid size
 * @param on_grid = TODO
 * @param curpos The initial position, and what to adjust if a change is needed.
 * @return bool - true if the position was adjusted magnetically, else false.
 */
bool Magnetize( BOARD* m_Pcb, PCB_EDIT_FRAME* frame, int aCurrentTool, wxSize grid,
                wxPoint on_grid, wxPoint* curpos )
{
    bool    doCheckNet = g_MagneticPadOption != capture_always && Drc_On;
    bool    doTrack = false;
    bool    doPad = false;
    bool    amMovingVia = false;

    TRACK*      currTrack = g_CurrentTrackSegment;
    BOARD_ITEM* currItem  = frame->GetCurItem();
    PCB_SCREEN* screen = frame->GetScreen();
    wxPoint     pos = screen->RefPos( true );

    // D( printf( "currTrack=%p currItem=%p currTrack->Type()=%d currItem->Type()=%d\n",  currTrack, currItem, currTrack ? currTrack->Type() : 0, currItem ? currItem->Type() : 0 ); )

    if( !currTrack && currItem && currItem->Type()==TYPE_VIA && currItem->m_Flags )
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
        int layer_mask = g_TabOneLayerMask[screen->m_Active_Layer];
        D_PAD* pad = Locate_Any_Pad( m_Pcb, pos, layer_mask );

        if( pad )
        {
            if( doCheckNet && currTrack && currTrack->GetNet() != pad->GetNet() )
                return false;

            *curpos = pad->m_Pos;
            return true;
        }
    }

    // after pads, only track & via tests remain, skip them if not desired
    if( doTrack )
    {
        int layer = screen->m_Active_Layer;

        for( TRACK* via = m_Pcb->m_Track;
             via && (via = Locate_Via_Area( via, *curpos, layer )) != NULL;
             via = via->Next() )
        {
            if( via != currTrack )   // a via cannot influence itself
            {
                if( !doCheckNet || !currTrack || currTrack->GetNet() == via->GetNet() )
                {
                    *curpos = via->m_Start;
                    // D(printf("via hit\n");)
                    return true;
                }
            }
            else
            {
                //D( printf( "skipping self\n" ); )
            }
        }

        if( !currTrack )
        {
            int layer_mask = g_TabOneLayerMask[layer];

            TRACK* track = GetTrace( m_Pcb, m_Pcb->m_Track, pos, layer_mask );

            if( !track || track->Type() != TYPE_TRACK )
            {
                // D(printf("!currTrack and track=%p not found, layer_mask=0x%X\n", track, layer_mask );)
                return false;
            }

            // D( printf( "Project\n" ); )
            return Project( curpos, on_grid, track );
        }

        /*
         * In two segment mode, ignore the final segment if it's inside a grid
         * square.
         */
        if( !amMovingVia && currTrack && g_TwoSegmentTrackBuild && currTrack->Back()
            && currTrack->m_Start.x - grid.x < currTrack->m_End.x
            && currTrack->m_Start.x + grid.x > currTrack->m_End.x
            && currTrack->m_Start.y - grid.y < currTrack->m_End.y
            && currTrack->m_Start.y + grid.y > currTrack->m_End.y )
        {
            currTrack = currTrack->Back();
        }


        for( TRACK* track = m_Pcb->m_Track;  track;  track = track->Next() )
        {
            if( track->Type() != TYPE_TRACK )
                continue;

            if( doCheckNet && currTrack && currTrack->GetNet() != track->GetNet() )
                continue;

            if( m_Pcb->IsLayerVisible( track->GetLayer() ) == false )
                continue;

            // omit the layer check if moving a via
            if( !amMovingVia && !track->IsOnLayer( layer ) )
                continue;

            if( !track->HitTest( *curpos ) )
                continue;

            D(printf( "have track prospect\n");)

            if( Join( curpos, track->m_Start, track->m_End, currTrack->m_Start, currTrack->m_End ) )
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
                double distStart = hypot( double( curpos->x - track->m_Start.x ),
                                          double( curpos->y - track->m_Start.y ));

                double distEnd   = hypot( double( curpos->x - track->m_End.x ),
                                          double( curpos->y - track->m_End.y ));

                // if track not via, or if its a via dragging but not with its adjacent track
                if( currTrack->Type() != TYPE_VIA
                    || ( currTrack->m_Start != track->m_Start && currTrack->m_Start != track->m_End ))
                {
                    if( distStart <= currTrack->m_Width/2 )
                    {
                        // D(printf("nearest end is start\n");)
                        *curpos = track->m_Start;
                        return true;
                    }

                    if( distEnd <= currTrack->m_Width/2 )
                    {
                        // D(printf("nearest end is end\n");)
                        *curpos = track->m_End;
                        return true;
                    }

                    // @todo otherwise confine curpos such that it stays centered
                    // within "track"
                }
            }
        }
    }

    return false;
}
