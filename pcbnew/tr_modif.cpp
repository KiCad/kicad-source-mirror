/**************************************************/
/*					 Edition des pistes			  */
/* Routines de modification automatique de pistes */
/**************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

/*********************************************************************/
int EraseOldTrack( WinEDA_BasePcbFrame* frame, BOARD* Pcb, wxDC* DC,
                   TRACK* pt_new_track, int nbptnewpiste )
/*********************************************************************/

/* Routine d'effacement de la piste redondante a la piste nouvellement cree
 *  pointee par pt_new_track (nbptnewpiste segments)
 *
 *  la piste cree est supposee constituee de segments contigus en memoire avec:
 *      point de depart pt_newtrack->m_Start.x,y
 *      point d'arrivee pt_newtrack->m_End.x,y
 */
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

    int     netcode = pt_new_track->GetNet();


    /* Reconstitution de la piste complete ( la nouvelle piste
     * a pu demarrer sur un segment de piste en l'air
     */

    ListSetState( pt_new_track, nbptnewpiste, BUSY, OFF );

    /* si la novelle piste commence par une via, il est plus sur de rechercher
     *  la piste complete en utilisant le segment suivant comme reference, car
     *  une via est souvent sur un carrefour de segments, et ne caracterise pas
     *  une piste */
    if( pt_new_track->Type() == TYPE_VIA && (nbptnewpiste > 1 ) )
        pt_new_track = pt_new_track->Next();

    pt_new_track = Marque_Une_Piste( frame, DC, pt_new_track, &nbptnewpiste, 0 );
    wxASSERT( pt_new_track );

#if 0 && defined(DEBUG)
    TRACK*  EndNewTrack;    /* Pointeur sur le dernier segment de la liste
                             *  chainee de la mouvelle piste */

    EndNewTrack = pt_new_track;
    for( ii = 1;  ii < nbptnewpiste; ii++ )
    {
        wxASSERT( EndNewTrack->GetState(-1) != 0 );
        D(printf("track %p is newly part of net %d\n", EndNewTrack, netcode );)
        EndNewTrack = EndNewTrack->Next();
    }

    wxASSERT( EndNewTrack->GetState(-1) != 0 );
    D(printf("track %p is newly part of net %d\n", EndNewTrack, netcode );)

    for( TRACK* track = Pcb->m_Track;  track;  track = track->Next() )
        track->Show( 0, std::cout );
#endif

    /* Calcul des limites de recherche des segments de piste */
    /* BufDeb pointe le 1er segment utile */
    BufDeb = Pcb->m_Track->GetStartNetCode( netcode );

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
    if( ReturnEndsTrack( pt_new_track, nbptnewpiste,
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
    pt_segm = Fast_Locate_Via( Pcb->m_Track, NULL, start, startmasklayer );
    if( pt_segm )
        startmasklayer |= pt_segm->ReturnMaskLayer();

    if( StartTrack->start && (StartTrack->start->Type() == TYPE_PAD) )
    {
        /* start sur pad */
        D_PAD* pt_pad = (D_PAD*) (StartTrack->start);
        startmasklayer |= pt_pad->m_Masque_Layer;
    }

    pt_segm = Fast_Locate_Via( Pcb->m_Track, NULL, end, endmasklayer );
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
    ListSetState( pt_new_track, nbptnewpiste, DELETED, ON );

    /* test : un segment doit etre connecte au point de depart car sinon
     * il est inutile d'analyser l'autre point
     */

    pt_segm = Fast_Locate_Piste( BufDeb, BufEnd, start, startmasklayer );

    if( pt_segm == NULL )     /* Pas de piste reliee au point de depart */
    {
        /* Suppression du flag DELETED */
        ListSetState( pt_new_track, nbptnewpiste, DELETED, OFF );
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
    ListSetState( pt_new_track, nbptnewpiste, DELETED, OFF );

    ListSetState( pt_new_track, nbptnewpiste, EDIT, ON );

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

        pt_del = Marque_Une_Piste( frame, DC, pt_del, &nb_segm, 0 );

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
                Trace_Une_Piste( frame->DrawPanel, DC, pt_del, nb_segm, GR_XOR | GR_SURBRILL );

                for( jj = 0; jj < nb_segm; jj++, pt_del = NextS )
                {
                    NextS = pt_del->Next();
                    pt_del->DeleteStructure();
                }

                /* nettoyage des flags */
                for( pt_del = Pcb->m_Track; pt_del != NULL; pt_del = pt_del->Next() )
                {
                    if( pt_del->GetState( EDIT ) )
                    {
                        pt_del->SetState( EDIT, OFF );
                        pt_del->Draw( frame->DrawPanel, DC, GR_OR );
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
    for( pt_del = Pcb->m_Track; pt_del; pt_del = pt_del->Next() )
    {
        pt_del->SetState( BUSY | DELETED | EDIT | CHAIN, OFF );
        if( pt_del == BufEnd )  // Last segment reached
            break;
    }

    return 0;
}
