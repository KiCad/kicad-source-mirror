/*****************************************************************/
/* Routines de tracage des pistes ( Toutes, 1 piste, 1 segment ) */
/*****************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "trigo.h"

#include "protos.h"

/* variables locales : */

/*********************************************************************************/
void Trace_Pistes( WinEDA_DrawPanel* panel, BOARD* Pcb, wxDC* DC, int drawmode )
/********************************************************************************/

/* Draw all tracks and zones.  As long as dark colors are used for the tracks,
 * Then the OR draw mode should show tracks underneath other tracks.  But a white
 * track will cover any other color since it has more bits to OR in.
 */
{
    for( TRACK* track = Pcb->m_Track;  track;   track = track->Next() )
    {
        track->Draw( panel, DC, drawmode );
    }

    for( SEGZONE* zone = Pcb->m_Zone;  zone;   zone = zone->Next() )
    {
        zone->Draw( panel, DC, drawmode );
    }
}


/************************************************************************/
void Trace_Une_Piste( WinEDA_DrawPanel* panel, wxDC* DC, TRACK* Track,
                      int nbsegment, int draw_mode )
/************************************************************************/

/* routine de trace de n segments consecutifs en memoire.
 *  Utile pour monter une piste en cours de trace car les segments de cette
 *  piste sont alors contigus en memoire
 *  Parametres :
 *  pt_start_piste = adresse de depart de la liste des segments
 *  nbsegment = nombre de segments a tracer
 *  draw_mode = mode ( GR_XOR, GR_OR..)
 *  ATTENTION:
 *  le point de depart d'une piste suivante DOIT exister: peut etre
 *  donc mis a 0 avant appel a la routine si la piste a tracer est la derniere
 */
{
    for(  ;   nbsegment > 0  && Track;   nbsegment--, Track = Track->Next() )
    {
        Track->Draw( panel, DC, draw_mode );
    }
}


