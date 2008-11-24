/*********************************************/
/* Edition des pistes: Routines d'effacement */
/* Effacement de segment, piste, net et zone */
/*********************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

class TSTSEGM   /* memorisation des segments marques */
{
public:
    TSTSEGM* Pnext, * Pback;
    TRACK*   RefTrack;

public:
    TSTSEGM( TRACK * Father ) {
        Pback    = Pnext = NULL;
        RefTrack = Father;
    }
};

/* Routines externes : */
void        Montre_Position_New_Piste( int flag );/* defini dans editrack.cc */


/* Routines Locales */
static void Marque_Chaine_segments( BOARD* Pcb, wxPoint ref_pos, int masklayer );

/* Variables locales */
TSTSEGM* ListSegm = NULL;

/****************************************************************************/
TRACK* Marque_Une_Piste( WinEDA_BasePcbFrame* frame, wxDC* DC,
                         TRACK* pt_segm, int* nb_segm, int flagcolor )
/****************************************************************************/

/* Routine de Marquage de 1 piste, a partir du segment pointe par pt_segm.
 *  le segment pointe est marque puis les segments connectes
 *  jusqu'a un pad ou un point de jonction de plus de 2 segments
 *  le marquage est la mise a 1 du bit BUSY
 *  Les segments sont ensuite reclasses pour etre contigus en liste chainee
 *  Retourne:
 *      adresse du 1er segment de la chaine creee
 *      nombre de segments
 */
{
    int      NbSegmBusy, masque_layer;
    TRACK*   Track, * FirstTrack, * NextTrack;
    TSTSEGM* Segm, * NextSegm;

    *nb_segm = 0;
    if( pt_segm == NULL )
        return NULL;

    /* Marquage du segment pointe */
    if( flagcolor )
        pt_segm->Draw( frame->DrawPanel, DC, flagcolor );

    pt_segm->SetState( BUSY, ON );
    masque_layer = pt_segm->ReturnMaskLayer();
    ListSegm = new TSTSEGM( pt_segm );

    /* Traitement du segment pointe : si c'est un segment, le cas est simple.
     *  Si c'est une via, on doit examiner le nombre de segments connectes.
     *  Si <=2, on doit detecter une piste, si > 2 seule la via est marquee
     */
    if( pt_segm->Type() == TYPEVIA )
    {
        TRACK* Segm1, * Segm2 = NULL, * Segm3 = NULL;
        Segm1 = Fast_Locate_Piste( frame->m_Pcb->m_Track, NULL,
                                   pt_segm->m_Start, masque_layer );
        if( Segm1 )
        {
            Segm2 = Fast_Locate_Piste( Segm1->Next(), NULL,
                                      pt_segm->m_Start, masque_layer );
        }
        if( Segm2 )
        {
            Segm3 = Fast_Locate_Piste( Segm2->Next(), NULL,
                                      pt_segm->m_Start, masque_layer );
        }
        if( Segm3 )
        {
            *nb_segm = 1; return pt_segm;
        }
        if( Segm1 )
        {
            masque_layer = Segm1->ReturnMaskLayer();
            Marque_Chaine_segments( frame->m_Pcb, pt_segm->m_Start, masque_layer );
        }
        if( Segm2 )
        {
            masque_layer = Segm2->ReturnMaskLayer();
            Marque_Chaine_segments( frame->m_Pcb, pt_segm->m_Start, masque_layer );
        }
    }
    else /* Marquage de la chaine connectee aux extremites du segment */
    {
        Marque_Chaine_segments( frame->m_Pcb, pt_segm->m_Start, masque_layer );
        Marque_Chaine_segments( frame->m_Pcb, pt_segm->m_End, masque_layer );
    }

    /* marquage des vias (vias non connectees ou inutiles */
    for( Segm = ListSegm; Segm != NULL; Segm = Segm->Pnext )
    {
        int layer;
        if( Segm->RefTrack->Type() != TYPEVIA )
            continue;

        if( Segm->RefTrack == pt_segm )
            continue;

        Segm->RefTrack->SetState( BUSY, ON );

        masque_layer = Segm->RefTrack->ReturnMaskLayer();

        Track = Fast_Locate_Piste( frame->m_Pcb->m_Track, NULL,
                                   Segm->RefTrack->m_Start,
                                   masque_layer );
        if( Track == NULL )
            continue;

        /* Test des connexions: si via utile: suppression marquage */
        layer = Track->GetLayer();

        while( ( Track = Fast_Locate_Piste( (TRACK*) Track->Next(), NULL,
                                           Segm->RefTrack->m_Start,
                                           masque_layer ) ) != NULL )
        {
            if( layer != Track->GetLayer() )
            {
                Segm->RefTrack->SetState( BUSY, OFF );
                break;
            }
        }
    }

    /* liberation memoire */
    for( Segm = ListSegm; Segm != NULL; Segm = NextSegm )
    {
        NextSegm = Segm->Pnext;
        delete Segm;
    }

    ListSegm = NULL;

    /* Reclassement des segments marques en une chaine */
    FirstTrack = frame->m_Pcb->m_Track; NbSegmBusy = 0;
    for( ; FirstTrack != NULL; FirstTrack = (TRACK*) FirstTrack->Next() )
    {
        /* recherche du debut de la liste des segments marques a BUSY */
        if( FirstTrack->GetState( BUSY ) )
        {
            NbSegmBusy = 1;
            break;
        }
    }

    /* Reclassement de la chaine debutant a FirstTrack et finissant
     *  au dernier segment marque. FirstTrack n'est pas modifie */
    Track = (TRACK*) FirstTrack->Next();
    for( ; Track != NULL; Track = NextTrack )
    {
        NextTrack = (TRACK*) Track->Next();
        if( Track->GetState( BUSY ) )
        {
            NbSegmBusy++;
            Track->UnLink();
            Track->Insert( frame->m_Pcb, FirstTrack );
        }
    }

    *nb_segm = NbSegmBusy;

    if( flagcolor )
        Trace_Une_Piste( frame->DrawPanel, DC, FirstTrack, NbSegmBusy, flagcolor );

    return FirstTrack;
}


/********************************************************************************/
static void Marque_Chaine_segments( BOARD* Pcb, wxPoint ref_pos, int masque_layer )
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
    TSTSEGM* Segm;

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
            Segm = new TSTSEGM( pt_via );

            Segm->Pnext     = ListSegm;
            ListSegm->Pback = Segm;
            ListSegm = Segm;
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
                pt_segm = (TRACK*) pt_segm->Next();
                continue;
            }

            if( pt_segm == pt_via )  /* deja traite */
            {
                pt_segm = (TRACK*) pt_segm->Next();
                continue;
            }

            NbSegm++;
            if( NbSegm == 1 ) /* 1ere detection de segment de piste */
            {
                MarqSegm = pt_segm;
                pt_segm  = (TRACK*) pt_segm->Next();
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
            Segm = new TSTSEGM( MarqSegm );

            Segm->Pnext     = ListSegm;
            ListSegm->Pback = Segm;
            ListSegm = Segm;
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
    for( ; (Track != NULL) && (ii < NbSegm); ii++, Track = (TRACK*) Track->Next() )
    {
        TrackListEnd   = Track;
        Track->m_Param = 0;
    }

    /* Calcul des extremites */
    NbEnds = 0; Track = RefTrack; ii = 0;
    for( ; (Track != NULL) && (ii < NbSegm); ii++, Track = (TRACK*) Track->Next() )
    {
        if( Track->Type() == TYPEVIA )
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
                ok = 1; return ok;
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
                *StartTrack = Track; NbEnds++;
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
                ok = 1; return ok;
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
