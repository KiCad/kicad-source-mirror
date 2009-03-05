/*********************************************/
/* Edition des pistes: Routines d'effacement */
/* Effacement de segment, piste, net et zone */
/*********************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"


typedef std::vector<TRACK*> TRACK_PTRS;


/* Routines externes : */
void        Montre_Position_New_Piste( int flag );/* defini dans editrack.cc */


/* Routines Locales */
static void Marque_Chaine_segments( BOARD* Pcb, wxPoint ref_pos, int masklayer, TRACK_PTRS* aList );

/* Variables locales */


/* Routine de Marquage de 1 piste, a partir du segment pointe par pt_segm.
 *  le segment pointe est marque puis les segments connectes
 *  jusqu'a un pad ou un point de jonction de plus de 2 segments
 *  le marquage est la mise a 1 du bit BUSY
 *  Les segments sont ensuite reclasses pour etre contigus en liste chainee
 *  Retourne:
 *      adresse du 1er segment de la chaine creee
 *      nombre de segments
 */

/**
 * Function Marque_Une_Piste
 * marks a chain of track segments, starting at aTrackList.
 * Each segment is marked by setting the BUSY bit into m_Flags.  Electrical continuity
 * is detected by walking each segment, and finally the segments are rearranged
 * into a contiguous chain within the given list.
 * @param aTrackList The first interesting segment within a list of many
 *  interesting and uninteresting segments.
 * @return TRACK* the first in the chain of interesting segments.
 */
TRACK* Marque_Une_Piste( WinEDA_BasePcbFrame* frame, wxDC* DC,
                         TRACK* aTrackList, int* nb_segm, int flagcolor )
{
    int         NbSegmBusy;

    TRACK_PTRS  trackList;

    *nb_segm = 0;
    if( aTrackList == NULL )
        return NULL;

    if( flagcolor )
        aTrackList->Draw( frame->DrawPanel, DC, flagcolor );

    // Ensure the flag BUSY is cleared because we use it to mark segments of the track
    for( TRACK* track = frame->GetBoard()->m_Track; track; track = track->Next() )
        track->SetState( BUSY , OFF );

    /* Set flags of the initial track segment */
    aTrackList->SetState( BUSY, ON );
    int masque_layer = aTrackList->ReturnMaskLayer();

    trackList.push_back( aTrackList );

    /* Examine the initial track segment : if it is really a segment, this is easy.
     *  If it is a via, one must search for connected segments.
     *  If <=2, this via connect 2 segments (or is connected to only one segment)
     *      and this via and these 2 segments are a part of a track.
     *  If > 2 only this via is flagged (the track has only this via)
     */
    if( aTrackList->Type() == TYPE_VIA )
    {
        TRACK* Segm1, * Segm2 = NULL, * Segm3 = NULL;
        Segm1 = Fast_Locate_Piste( frame->GetBoard()->m_Track, NULL,
                                   aTrackList->m_Start, masque_layer );
        if( Segm1 )
        {
            Segm2 = Fast_Locate_Piste( Segm1->Next(), NULL,
                                      aTrackList->m_Start, masque_layer );
        }
        if( Segm2 )
        {
            Segm3 = Fast_Locate_Piste( Segm2->Next(), NULL,
                                      aTrackList->m_Start, masque_layer );
        }
        if( Segm3 )     // More than 2 segments are connected to this via. the "track" is only this via
        {
            *nb_segm = 1;
            return aTrackList;
        }
        if( Segm1 )     // search for others segments connected to the initial segment start point
        {
            masque_layer = Segm1->ReturnMaskLayer();
            Marque_Chaine_segments( frame->GetBoard(), aTrackList->m_Start, masque_layer, &trackList );
        }
        if( Segm2 )     // search for others segments connected to the initial segment end point
        {
            masque_layer = Segm2->ReturnMaskLayer();
            Marque_Chaine_segments( frame->GetBoard(), aTrackList->m_Start, masque_layer, &trackList );
        }
    }

    else    // mark the chain using both ends of the initial segment
    {
        Marque_Chaine_segments( frame->GetBoard(), aTrackList->m_Start, masque_layer, &trackList );
        Marque_Chaine_segments( frame->GetBoard(), aTrackList->m_End, masque_layer, &trackList );
    }

    //  marquage des vias (vias non connectees ou inutiles
    // go through the list backwards.
    for( int i = trackList.size()-1;  i>=0;  --i )
    {
        TRACK*  via = trackList[i];

        if( via->Type() != TYPE_VIA )
            continue;

        if( via == aTrackList )
            continue;

        via->SetState( BUSY, ON );

        masque_layer = via->ReturnMaskLayer();

        TRACK* track = Fast_Locate_Piste( frame->GetBoard()->m_Track,
                                         NULL, via->m_Start, masque_layer );
        if( track == NULL )
            continue;

        /* Test des connexions: si via utile: suppression marquage */
        int layer = track->GetLayer();

        while( ( track = Fast_Locate_Piste( track->Next(), NULL,
                         via->m_Start, masque_layer ) ) != NULL )
        {
            if( layer != track->GetLayer() )
            {
                via->SetState( BUSY, OFF );
                break;
            }
        }
    }

    /* Reclassement des segments marques en une chaine */
    NbSegmBusy = 0;
    TRACK* firstTrack;
    for( firstTrack = frame->GetBoard()->m_Track;  firstTrack;  firstTrack = firstTrack->Next() )
    {
        // recherche du debut de la liste des segments marques a BUSY
        if( firstTrack->GetState( BUSY ) )
        {
            NbSegmBusy = 1;
            break;
        }
    }

    wxASSERT( firstTrack );

    if( firstTrack )
    {
        DLIST<TRACK>* list = (DLIST<TRACK>*)firstTrack->GetList();
        wxASSERT(list);

        /* Reclassement de la chaine debutant a FirstTrack et finissant
         * au dernier segment marque. FirstTrack n'est pas modifie
         */
        TRACK* next;
        for( TRACK* track = firstTrack->Next(); track; track = next )
        {
            next = track->Next();
            if( track->GetState( BUSY ) )
            {
                NbSegmBusy++;
                track->UnLink();
                list->Insert( track, firstTrack->Next() );
            }
        }
    }

    *nb_segm = NbSegmBusy;

    if( flagcolor )
        Trace_Une_Piste( frame->DrawPanel, DC, firstTrack, NbSegmBusy, flagcolor );

    return firstTrack;
}


/********************************************************************************/
static void Marque_Chaine_segments( BOARD* Pcb, wxPoint ref_pos, int masque_layer, TRACK_PTRS* aList )
/********************************************************************************/

/*
 *  routine utilisee par Supprime_1_Piste()
 *  Positionne le bit BUSY dans la chaine de segments commencant
 *  au point ox, oy sur la couche layer
 *
 *  Les vias sont mises en liste des segments traites mais ne sont pas
 *  marquees.
 */
{
    TRACK*   pt_segm,   // Pointe le segment courant analyse
    * pt_via,           // pointe la via reperee, eventuellement a detruire
    * MarqSegm;         // pointe le segment a detruire (= NULL ou pt_segm
    int      NbSegm;

    if( Pcb->m_Track == NULL )
        return;

    /* Marquage de la chaine */
    for( ; ; )
    {
        if( Fast_Locate_Pad_Connecte( Pcb, ref_pos, masque_layer ) != NULL )
            return;

        /* Localisation d'une via (car elle connecte plusieurs segments) */
        pt_via = Fast_Locate_Via( Pcb->m_Track, NULL, ref_pos, masque_layer );
        if( pt_via )
        {
            if( pt_via->GetState( EDIT ) )
                return;

            masque_layer = pt_via->ReturnMaskLayer();

            aList->push_back( pt_via );
        }

        /* Recherche des segments connectes au point ref_pos
         *  si 1 segment: peut etre marque
         *  si > 1 segment:
         *      le segment ne peut etre marque
         */
        pt_segm = Pcb->m_Track; MarqSegm = NULL;
        NbSegm  = 0;
        while( ( pt_segm = Fast_Locate_Piste( pt_segm, NULL,
                                              ref_pos, masque_layer ) ) != NULL )
        {
            if( pt_segm->GetState( EDIT ) ) /* Fin de piste */
                return;

            if( pt_segm->GetState( BUSY ) )
            {
                pt_segm = pt_segm->Next();
                continue;
            }

            if( pt_segm == pt_via )  /* deja traite */
            {
                pt_segm = pt_segm->Next();
                continue;
            }

            NbSegm++;
            if( NbSegm == 1 ) /* 1ere detection de segment de piste */
            {
                MarqSegm = pt_segm;
                pt_segm  = pt_segm->Next();
            }
            else /* 2eme detection de segment -> fin de piste */
            {
                return;
            }
        }

        if( MarqSegm )
        {
            /* preparation de la nouvelle recherche */
            masque_layer = MarqSegm->ReturnMaskLayer();

            if( ref_pos == MarqSegm->m_Start )
            {
                ref_pos = MarqSegm->m_End;
            }
            else
            {
                ref_pos = MarqSegm->m_Start;
            }

            pt_segm = Pcb->m_Track; /* reinit recherche des segments */

            /* Marquage et mise en liste du segment */
            aList->push_back( MarqSegm );
            MarqSegm->SetState( BUSY, ON );
        }
        else
            return;
    }
}


/********************************************************/
int ReturnEndsTrack( TRACK* RefTrack, int NbSegm,
                     TRACK** StartTrack, TRACK** EndTrack )
/**********************************************************/

/* Calcule les coordonnes des extremites d'une piste
 *  retourne 1 si OK, 0 si piste bouclee
 *  Retourne dans *StartTrack en *EndTrack les segments de debut et fin
 *  Les coord StartTrack->m_Start.x, m_Start.y contiennent le debut de la piste
 *  Les coord EndTrack->m_End.x, m_End.y contiennent le debut de la piste
 *  Les segments sont supposes chaines de facon consecutive
 */
{
    TRACK* Track, * via, * segm, * TrackListEnd;
    int    NbEnds, masque_layer, ii, ok = 0;

    if( NbSegm <= 1 )
    {
        *StartTrack = *EndTrack = RefTrack;
        return 1;   /* cas trivial */
    }

    /* calcul de la limite d'analyse */
    *StartTrack  = *EndTrack = NULL;
    TrackListEnd = Track = RefTrack; ii = 0;
    for( ; (Track != NULL) && (ii < NbSegm); ii++, Track = Track->Next() )
    {
        TrackListEnd   = Track;
        Track->m_Param = 0;
    }

    /* Calcul des extremites */
    NbEnds = 0; Track = RefTrack; ii = 0;
    for( ; (Track != NULL) && (ii < NbSegm); ii++, Track = Track->Next() )
    {
        if( Track->Type() == TYPE_VIA )
            continue;

        masque_layer = Track->ReturnMaskLayer();
        via = Fast_Locate_Via( RefTrack, TrackListEnd,
                               Track->m_Start, masque_layer );
        if( via )
        {
            masque_layer |= via->ReturnMaskLayer();
            via->SetState( BUSY, ON );
        }

        Track->SetState( BUSY, ON );
        segm = Fast_Locate_Piste( RefTrack, TrackListEnd,
                                  Track->m_Start, masque_layer );
        Track->SetState( BUSY, OFF );
        if( via )
            via->SetState( BUSY, OFF );

        if( segm == NULL )
        {
            switch( NbEnds )
            {
            case 0:
                *StartTrack = Track; NbEnds++;
                break;

            case 1:
                int BeginPad, EndPad;
                *EndTrack = Track;

                /* permutation de ox,oy avec fx,fy */
                BeginPad = Track->GetState( BEGIN_ONPAD );
                EndPad   = Track->GetState( END_ONPAD );

                Track->SetState( BEGIN_ONPAD | END_ONPAD, OFF );

                if( BeginPad )
                    Track->SetState( END_ONPAD, ON );
                if( EndPad )
                    Track->SetState( BEGIN_ONPAD, ON );

                EXCHG( Track->m_Start, Track->m_End );
                EXCHG( Track->start, Track->end );
                ok = 1;
                return ok;
            }
        }

        masque_layer = Track->ReturnMaskLayer();
        via = Fast_Locate_Via( RefTrack, TrackListEnd,
                               Track->m_End, masque_layer );
        if( via )
        {
            masque_layer |= via->ReturnMaskLayer();
            via->SetState( BUSY, ON );
        }

        Track->SetState( BUSY, ON );
        segm = Fast_Locate_Piste( RefTrack, TrackListEnd,
                                  Track->m_End, masque_layer );
        Track->SetState( BUSY, OFF );
        if( via )
            via->SetState( BUSY, OFF );
        if( segm == NULL )
        {
            switch( NbEnds )
            {
            case 0:
                int BeginPad, EndPad;
                *StartTrack = Track;
                NbEnds++;

                /* permutation de ox,oy avec fx,fy */
                BeginPad = Track->GetState( BEGIN_ONPAD );
                EndPad   = Track->GetState( END_ONPAD );

                Track->SetState( BEGIN_ONPAD | END_ONPAD, OFF );

                if( BeginPad )
                    Track->SetState( END_ONPAD, ON );
                if( EndPad )
                    Track->SetState( BEGIN_ONPAD, ON );

                EXCHG( Track->m_Start, Track->m_End );
                EXCHG( Track->start, Track->end );
                break;

            case 1:
                *EndTrack = Track;
                ok = 1;
                return ok;
            }
        }
    }

    return ok;
}


/***************************************************************************/
void ListSetState( EDA_BaseStruct* Start, int NbItem, int State, int onoff )
/***************************************************************************/

/* Met a jour le membre .state d'une chaine de structures
 */
{
    if( Start == NULL )
        return;

    for( ; (Start != NULL) && (NbItem > 0); NbItem--, Start = Start->Next() )
    {
        Start->SetState( State, onoff );
    }
}
