/********************************/
/* Routines for plotting traces */
/********************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "trigo.h"

#include "protos.h"


/* Trace consecutive segments in memory.
 *
 * Parameters:
 * Pt_start_piste = first segment in the list
 * Nbsegment = number of segments traced
 * Draw_mode = mode (GR_XOR, GR_OR ..)
 * CAUTION:
 * The starting point of a track following MUST exist: may be
 * then put a 0 before calling a routine if the track is the last drawn.
 */
void Trace_Une_Piste( WinEDA_DrawPanel* panel, wxDC* DC, TRACK* aTrackList,
                      int nbsegment, int draw_mode )
{
    // preserve the start of the list for debugging.
    for( TRACK* track = aTrackList; nbsegment > 0  && track;
         nbsegment--, track = track->Next() )
    {
        track->Draw( panel, DC, draw_mode );
    }
}
