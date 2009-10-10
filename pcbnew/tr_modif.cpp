/**************************************************/
/*					 Edition des pistes			  */
/* Routines de modification automatique de pistes */
/**************************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "protos.h"

/** function EraseRedundantTrack
 * Called after creating a track
 * Remove (if exists) the old track that have the same starting and the same ending point as the new created track
 * (this is the redunding track)
 * @param aDC = the current device context (can be NULL)
 * @param aNewTrack = the new created track (a pointer to a segment of the track list)
 * @param aNewTrackSegmentsCount = number of segments in this new track
 * @param aItemsListPicker = the list picker to use for an undo command (can be NULL)
 */
int WinEDA_PcbFrame::EraseRedundantTrack( wxDC* aDC, TRACK* aNewTrack, int aNewTrackSegmentsCount, PICKED_ITEMS_LIST*  aItemsListPicker )
{
    TRACK*  StartTrack, * EndTrack;/* Pointeurs des segments de debut et fin
                                 *  (extremites) de la nouvelle piste */
    TRACK*  pt_segm;
    TRACK*  pt_del;
    int     ii, jj, nb_segm, nbconnect;
    wxPoint start;                          /* coord du point de depart de la piste */
    wxPoint end;                            /* coord du point de fin de la piste */
    int     startmasklayer, endmasklayer;   /* masque couche de depart et de fin */
    TRACK*  BufDeb, * BufEnd;   /* Pointeurs de debut et de fin de la zone
                                 *  des pistes equipotentielles */

    int     netcode = aNewTrack->GetNet();


    /* Reconstitution de la piste complete ( la nouvelle piste
     * a pu demarrer sur un segment de piste en l'air
     */

    ListSetState( aNewTrack, aNewTrackSegmentsCount, BUSY, OFF );

    /* si la nouvelle piste commence par une via, il est plus sur de rechercher
     *  la piste complete en utilisant le segment suivant comme reference, car
     *  une via est souvent sur un carrefour de segments, et ne caracterise pas
     *  une piste */
    if( aNewTrack->Type() == TYPE_VIA && (aNewTrackSegmentsCount > 1 ) )
        aNewTrack = aNewTrack->Next();

    aNewTrack = Marque_Une_Piste( GetBoard(), aNewTrack, &aNewTrackSegmentsCount, NULL, true );
    wxASSERT( aNewTrack );

#if 0 && defined(DEBUG)
    TRACK*  EndNewTrack;    /* Pointeur sur le dernier segment de la liste
                             *  chainee de la mouvelle piste */

    EndNewTrack = aNewTrack;
    for( ii = 1;  ii < aNewTrackSegmentsCount; ii++ )
    {
        wxASSERT( EndNewTrack->GetState(-1) != 0 );
        D(printf("track %p is newly part of net %d\n", EndNewTrack, netcode );)
        EndNewTrack = EndNewTrack->Next();
    }

    wxASSERT( EndNewTrack->GetState(-1) != 0 );
    D(printf("track %p is newly part of net %d\n", EndNewTrack, netcode );)

    for( TRACK* track = m_Pcb->m_Track;  track;  track = track->Next() )
        track->Show( 0, std::cout );
#endif

    /* Calcul des limites de recherche des segments de piste */
    /* BufDeb pointe le 1er segment utile */
    BufDeb = m_Pcb->m_Track->GetStartNetCode( netcode );

    /* BufEnd Pointe le dernier segment */
    BufEnd = BufDeb->GetEndNetCode( netcode );

    /* nettoyage des flags pour tout le net */
    for( pt_del = BufDeb;  pt_del;  pt_del = pt_del->Next() )
    {
        D(printf("track %p turning off BUSY | EDIT | CHAIN\n", pt_del );)
        pt_del->SetState( BUSY | EDIT | CHAIN, OFF );
        if( pt_del == BufEnd )  // Last segment reached
            break;
    }

    /* Calcul des points limites de la nouvelle piste */
    if( ReturnEndsTrack( aNewTrack, aNewTrackSegmentsCount,
                         &StartTrack, &EndTrack ) == 0 )
        return 0;

    if( (StartTrack == NULL) || (EndTrack == NULL) )
        return 0;

    /* Calcul des caracteristiques des points de debut et de fin */
    start = StartTrack->m_Start;
    end   = EndTrack->m_End;

    /* Les points de depart et de fin doivent etre distincts */
    if( start == end )
        return 0;

    /* Determinations des couches interconnectees a ces points */
    startmasklayer = StartTrack->ReturnMaskLayer();
    endmasklayer   = EndTrack->ReturnMaskLayer();

    /* Il peut y avoir une via ou un pad sur les extremites: */
    pt_segm = Fast_Locate_Via( m_Pcb->m_Track, NULL, start, startmasklayer );
    if( pt_segm )
        startmasklayer |= pt_segm->ReturnMaskLayer();

    if( StartTrack->start && (StartTrack->start->Type() == TYPE_PAD) )
    {
        /* start sur pad */
        D_PAD* pt_pad = (D_PAD*) (StartTrack->start);
        startmasklayer |= pt_pad->m_Masque_Layer;
    }

    pt_segm = Fast_Locate_Via( m_Pcb->m_Track, NULL, end, endmasklayer );
    if( pt_segm )
        endmasklayer |= pt_segm->ReturnMaskLayer();

    if( EndTrack->end && ( EndTrack->end->Type() == TYPE_PAD) )
    {
        D_PAD* pt_pad = (D_PAD*) (EndTrack->end);
        endmasklayer |= pt_pad->m_Masque_Layer;
    }

    /* Marquage a DELETED de la piste nouvelle (qui ne doit pas intervenir
     * dans la recherche d'autres connexions)
     */
    ListSetState( aNewTrack, aNewTrackSegmentsCount, DELETED, ON );

    /* test : un segment doit etre connecte au point de depart car sinon
     * il est inutile d'analyser l'autre point
     */

    pt_segm = Fast_Locate_Piste( BufDeb, BufEnd, start, startmasklayer );

    if( pt_segm == NULL )     /* Pas de piste reliee au point de depart */
    {
        /* Suppression du flag DELETED */
        ListSetState( aNewTrack, aNewTrackSegmentsCount, DELETED, OFF );
        return 0;
    }

    /* Marquage a CHAIN des segments candidats connectes au point de fin
     *  Remarque: les vias ne sont pas prises en compte car elles ne permettent
     *  pas de definir une piste, puisque elles sont sur un carrefour */
    for( pt_del = BufDeb, nbconnect = 0; ; )
    {
        pt_segm = Fast_Locate_Piste( pt_del, BufEnd, end, endmasklayer );
        if( pt_segm == NULL )
            break;

        if( pt_segm->Type() != TYPE_VIA )
        {
            /* Segment trouve */
            if( pt_segm->GetState( CHAIN ) == 0 )
            {
                pt_segm->SetState( CHAIN, ON );
                nbconnect++;
            }
        }
        if( pt_del == BufEnd )
            break;

        pt_del = pt_segm->Next();
    }

    if( nbconnect == 0 )
    {
        /* Clear used flagss */
        for( pt_del = BufDeb; pt_del; pt_del = pt_del->Next() )
        {
            pt_del->SetState( BUSY | DELETED | EDIT | CHAIN, OFF );
            if( pt_del == BufEnd )  // Last segment reached
                break;
        }

        return 0;
    }

    /* Marquage a EDIT de la piste nouvelle (qui ne doit pas intervenir
     *  dans la recherche d'autres pistes) */
    ListSetState( aNewTrack, aNewTrackSegmentsCount, DELETED, OFF );

    ListSetState( aNewTrack, aNewTrackSegmentsCount, EDIT, ON );

    /* Examen de tous les segments marques */
    while( nbconnect )
    {
        for( pt_del = BufDeb; pt_del; pt_del = pt_del->Next() )
        {
            if( pt_del->GetState( CHAIN ) )
                break;
            if( pt_del == BufEnd )  // Last segment reached
                break;
        }

        nbconnect--;
        pt_del->SetState( CHAIN, OFF );

        pt_del = Marque_Une_Piste( GetBoard(), pt_del, &nb_segm, NULL, true );

        /* Test si La piste marquee est redondante, c'est a dire si l'un des
         * segments marques est connecte au point de depart de la piste nouvelle
         */
        ii = 0;
        pt_segm = pt_del;
        for( ; pt_segm && (ii < nb_segm); pt_segm = pt_segm->Next(), ii++ )
        {
            if( pt_segm->GetState( BUSY ) == 0 )
                break;

            if( pt_segm->m_Start == start || pt_segm->m_End == start )
            {
                /* la piste marquee peut etre effacee */
                TRACK* NextS;
                Trace_Une_Piste( DrawPanel, aDC, pt_del, nb_segm, GR_XOR | GR_SURBRILL );

                for( jj = 0; jj < nb_segm; jj++, pt_del = NextS )
                {
                    NextS = pt_del->Next();
                    if( aItemsListPicker )
                    {
                        pt_del->UnLink();
                        pt_del->SetStatus( 0 );
                        pt_del->m_Flags = 0;
                        ITEM_PICKER picker( pt_del, UR_DELETED );
                        aItemsListPicker->PushItem( picker );
                   }
                    else
                        pt_del->DeleteStructure();
                }

                /* nettoyage des flags */
                for( pt_del = m_Pcb->m_Track; pt_del != NULL; pt_del = pt_del->Next() )
                {
                    if( pt_del->GetState( EDIT ) )
                    {
                        pt_del->SetState( EDIT, OFF );
                        if( aDC )
                            pt_del->Draw( DrawPanel, aDC, GR_OR );
                    }
                    pt_del->SetState( EDIT | CHAIN, OFF );
                }

                return 1;
            }
        }

        /* nettoyage du flag BUSY puisque ici la piste marquee n'a pas
         *  ete retenuee */
        ListSetState( pt_del, nb_segm, BUSY, OFF );
    }

    /* Clear used flags */
    for( pt_del = m_Pcb->m_Track; pt_del; pt_del = pt_del->Next() )
    {
        pt_del->SetState( BUSY | DELETED | EDIT | CHAIN, OFF );
        if( pt_del == BufEnd )  // Last segment reached
            break;
    }

    return 0;
}
