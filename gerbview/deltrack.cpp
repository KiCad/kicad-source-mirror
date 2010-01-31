/*********************************************/
/* Edit Track: Erase Routines                */
/* Drop the segment, track, and net area     */
/*********************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "gerbview.h"
#include "protos.h"


void WinEDA_GerberFrame::Delete_DCode_Items( wxDC* DC,
                                             int   dcode_value,
                                             int   layer_number )
{
    if( dcode_value < FIRST_DCODE )  // No tool selected
        return;

    TRACK* next;
    for( TRACK* track = GetBoard()->m_Track; track; track = next  )
    {
        next = track->Next();

        if( dcode_value != track->GetNet() )
            continue;

        if( layer_number >= 0 && layer_number != track->GetLayer() )
            continue;

        Delete_Segment( DC, track );
    }

    GetScreen()->SetCurItem( NULL );
}


/* Removes 1 segment of track.
 *
 * If There is evidence of new track: erase segment
 * Otherwise: Delete segment under the cursor.
 */
TRACK* WinEDA_GerberFrame::Delete_Segment( wxDC* DC, TRACK* Track )
{
    if( Track == NULL )
        return NULL;

    if( Track->m_Flags & IS_NEW )  // Trace in progress, delete the last
                                   // segment
    {
        if( g_CurrentTrackList.GetCount() > 0 )
        {
            // Change track.
            delete g_CurrentTrackList.PopBack();

            if( g_CurrentTrackList.GetCount()
                && g_CurrentTrackSegment->Type() == TYPE_VIA )
            {
                delete g_CurrentTrackList.PopBack();
            }

            UpdateStatusBar();

            if( g_CurrentTrackList.GetCount() == 0 )
            {
                DrawPanel->ManageCurseur = NULL;
                DrawPanel->ForceCloseManageCurseur = NULL;
                return NULL;
            }
            else
            {
                if( DrawPanel->ManageCurseur )
                    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
                return g_CurrentTrackSegment;
            }
        }

        return NULL;
    }

    Trace_Segment( GetBoard(), DrawPanel, DC, Track, GR_XOR );

    DLIST<TRACK>* container = (DLIST<TRACK>*)Track->GetList();
    wxASSERT( container );
    container->Remove( Track );

    GetScreen()->SetModify();
    return NULL;
}
