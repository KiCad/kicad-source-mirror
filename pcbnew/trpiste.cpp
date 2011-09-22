/********************************/
/* Routines for plotting traces */
/********************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "trigo.h"

#include "protos.h"


/*
 * Draws n consecutive track segments in list.
 * Useful to show a track when it is a chain of segments
 * (fir instance when creating a new track)
 * param aTrackList = First segment
 * param nbsegment = number of segments in list
 * param Mode_color = mode (GRXOR, GROR ..)
 */
void DrawTraces( EDA_DRAW_PANEL* panel, wxDC* DC, TRACK* aTrackList, int nbsegment, int draw_mode )
{
    // preserve the start of the list for debugging.
    for( TRACK* track = aTrackList; nbsegment > 0  && track; nbsegment--, track = track->Next() )
    {
        track->Draw( panel, DC, draw_mode );
    }
}
