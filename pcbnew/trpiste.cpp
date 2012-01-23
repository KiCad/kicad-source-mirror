/**
 * @file trpiste.cpp
 * @brief Routine for plotting traces.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <trigo.h>

#include <class_track.h>

#include <pcbnew.h>
#include <protos.h>


void DrawTraces( EDA_DRAW_PANEL* panel, wxDC* DC, TRACK* aTrackList, int nbsegment, int draw_mode )
{
    // preserve the start of the list for debugging.
    for( TRACK* track = aTrackList; nbsegment > 0  && track; nbsegment--, track = track->Next() )
    {
        track->Draw( panel, DC, draw_mode );
    }
}
