/*********************************************/
/* Edition des pistes: Routines d'effacement */
/* Effacement de segment, piste, net et zone */
/*********************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"

#include "protos.h"

/* Routines externes : */

/* Routines Locales */

/* Variables locales */


/****************************************************************************************/
void WinEDA_GerberFrame::Delete_DCode_Items( wxDC* DC, int dcode_value, int layer_number )
/****************************************************************************************/
{
    if( dcode_value < FIRST_DCODE )  // No tool selected
        return;
    BOARD* Pcb   = m_Pcb;
    TRACK* track = Pcb->m_Track, * next_track;
    for( ; track != NULL; track = next_track )
    {
        next_track = track->Next();
        if( dcode_value != track->GetNet() )
            continue;
        if( layer_number >= 0 && layer_number != track->GetLayer() )
            continue;
        Delete_Segment( DC, track );
    }

    GetScreen()->SetCurItem( NULL );
}


/*****************************************************************/
TRACK* WinEDA_GerberFrame::Delete_Segment( wxDC* DC, TRACK* Track )
/*****************************************************************/

/* Supprime 1 segment de piste.
 *  2 Cas possibles:
 *  Si On est en trace de nouvelle piste: Effacement du segment en
 *      cours de trace
 *  Sinon : Effacment du segment sous le curseur.
 */
{
    if( Track == NULL )
        return NULL;

    if( Track->m_Flags & IS_NEW )  // Trace en cours, on peut effacer le dernier segment
    {
        if( g_TrackSegmentCount > 0 )
        {
            // modification du trace
            Track = g_CurrentTrackSegment;
            g_CurrentTrackSegment = (TRACK*) g_CurrentTrackSegment->Pback;
            delete Track; g_TrackSegmentCount--;

            if( g_TrackSegmentCount && (g_CurrentTrackSegment->Type() == TYPEVIA) )
            {
                Track = g_CurrentTrackSegment;
                g_CurrentTrackSegment = (TRACK*) g_CurrentTrackSegment->Pback;
                delete Track;
                g_TrackSegmentCount--;
            }
            if( g_CurrentTrackSegment )
                g_CurrentTrackSegment->Pnext = NULL;

            Affiche_Status_Box();

            if( g_TrackSegmentCount == 0 )
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
    } // Fin traitement si trace en cours


    Trace_Segment( DrawPanel, DC, Track, GR_XOR );

    SaveItemEfface( Track, 1 );
    GetScreen()->SetModify();
    return NULL;
}
