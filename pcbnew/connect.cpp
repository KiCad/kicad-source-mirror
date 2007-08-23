/*************************************************************/
/******************* editeur de PCB **************************/
/*  traitement du Chevelu: routines de calcul des connexions */
/*************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "autorout.h"

#include "protos.h"


/* variables locales */

/* routines exportees */
void        Recalcule_all_net_connexion( wxDC* DC );

/* Routines locales */
static void propage_equipot( TRACK* pt_start_conn, TRACK* pt_end_conn );
static void calcule_connexite_1_net( TRACK* pt_start_conn, TRACK* pt_end_conn );
static void RebuildTrackChain( BOARD* pcb );
static int  tri_par_netcode( TRACK** pt_ref, TRACK** pt_compare );

/*..*/


/*****************************************************************/
static int change_equipot( TRACK* pt_start_conn, TRACK* pt_end_conn,
                           int old_val, int new_val )
/*****************************************************************/

/*
 *  Change les num locaux d'equipot old valeur en new valeur
 *  retourne le nombre de changements
 *  si pt_end_conn = NULL: recherche jusqu'a fin de chaine
 */
{
    TRACK* pt_conn;
    int    nb_change = 0;
    D_PAD* pt_pad;

    if( old_val == new_val )
        return 0;

    if( (old_val > 0) && (old_val < new_val) )
        EXCHG( old_val, new_val );

    pt_conn = pt_start_conn;
    for( ; pt_conn != NULL; pt_conn = (TRACK*) pt_conn->Pnext )
    {
        if( pt_conn->m_Sous_Netcode != old_val )
        {
            if( pt_conn == pt_end_conn )
                break;
            continue;
        }

        nb_change++;
        pt_conn->m_Sous_Netcode = new_val;

        if( pt_conn->start && ( pt_conn->start->m_StructType == TYPEPAD) )
        {
            pt_pad = (D_PAD*) (pt_conn->start);
            if( pt_pad->m_physical_connexion == old_val )
                pt_pad->m_physical_connexion = pt_conn->m_Sous_Netcode;
        }

        if( pt_conn->end && (pt_conn->end->m_StructType == TYPEPAD) )
        {
            pt_pad = (D_PAD*) (pt_conn->end);
            if( pt_pad->m_physical_connexion == old_val )
                pt_pad->m_physical_connexion = pt_conn->m_Sous_Netcode;
        }
        if( pt_conn == pt_end_conn )
            break;
    }

    return nb_change;
}


/******************************************************************/
static void propage_equipot( TRACK* pt_start_conn, TRACK* pt_end_conn )
/******************************************************************/

/* balaye la liste des SEGMENTS de PISTE
 *  - debut = pt_start_conn
 *  - fin	 = pt_end_conn (pointe le dernier segment a analyser)
 *  pour attribuer ou propager un numero d'equipotentielle par
 *  blocs de connexions existantes
 *  la zone balayee est supposee appartenir au meme net, c'est a dire que
 *  les segments de pistes sont tries par net_code
 */
{
    TRACK*          pt_conn;
    int             sous_net_code;
    D_PAD*          pt_pad;
    TRACK*          pt_autre_piste;
    BOARD_ITEM*     PtStruct;

    /* Initialisations prealables */
    pt_conn = pt_start_conn;
    for( ; pt_conn != NULL; pt_conn = (TRACK*) pt_conn->Pnext )
    {
        pt_conn->m_Sous_Netcode = 0;
        PtStruct = pt_conn->start;
        if( PtStruct && (PtStruct->m_StructType == TYPEPAD) )
            ( (D_PAD*) PtStruct )->m_physical_connexion = 0;

        PtStruct = pt_conn->end;
        if( PtStruct && (PtStruct->m_StructType == TYPEPAD) )
            ( (D_PAD*) PtStruct )->m_physical_connexion = 0;

        if( pt_conn == pt_end_conn )
            break;
    }

    sous_net_code = 1; 
    pt_start_conn->m_Sous_Netcode = sous_net_code;

    /* debut du calcul de propagation */
    pt_conn = pt_start_conn;
    for( ; pt_conn != NULL; pt_conn = (TRACK*) pt_conn->Pnext )
    {
        /* Traitement des connexions a pads */
        PtStruct = pt_conn->start;
        if( PtStruct && (PtStruct->m_StructType == TYPEPAD) )
        /* la connexion debute sur 1 pad */
        {
            pt_pad = (D_PAD*) PtStruct;
            if( pt_conn->m_Sous_Netcode )               /* la connexion fait deja partie d'une chaine */
            {
                if( pt_pad->m_physical_connexion > 0 )  /* le pad fait aussi partie d'une chaine */
                {
                    change_equipot( pt_start_conn, pt_end_conn,
                                    pt_pad->m_physical_connexion, pt_conn->m_Sous_Netcode );
                }
                else
                    pt_pad->m_physical_connexion = pt_conn->m_Sous_Netcode;
            }
            else    /* la connexion ne fait pas partie encore d'une chaine */
            {
                if( pt_pad->m_physical_connexion > 0 )
                {
                    pt_conn->m_Sous_Netcode = pt_pad->m_physical_connexion;
                }
                else
                {
                    sous_net_code++; 
                    pt_conn->m_Sous_Netcode = sous_net_code;
                    pt_pad->m_physical_connexion = pt_conn->m_Sous_Netcode;
                }
            }
        }

        PtStruct = pt_conn->end;
        if( PtStruct && (PtStruct->m_StructType == TYPEPAD) )
        /* la connexion finit sur 1 pad */
        {
            pt_pad = (D_PAD*) PtStruct;
            if( pt_conn->m_Sous_Netcode )
            {
                if( pt_pad->m_physical_connexion > 0 )
                {
                    change_equipot( pt_start_conn, pt_end_conn,
                                    pt_pad->m_physical_connexion, pt_conn->m_Sous_Netcode );
                }
                else
                    pt_pad->m_physical_connexion = pt_conn->m_Sous_Netcode;
            }
            else
            {
                if( pt_pad->m_physical_connexion > 0 )
                {
                    pt_conn->m_Sous_Netcode = pt_pad->m_physical_connexion;
                }
                else
                {
                    sous_net_code++; pt_conn->m_Sous_Netcode = sous_net_code;
                    pt_pad->m_physical_connexion = pt_conn->m_Sous_Netcode;
                }
            }
        }


        /* traitement des connexions entre segments */
        PtStruct = pt_conn->start;
        if( PtStruct && (PtStruct->m_StructType != TYPEPAD) )
        {      
            /* debut sur une autre piste */
            pt_autre_piste = (TRACK*) PtStruct;

            if( pt_conn->m_Sous_Netcode )  /* La connexion fait deja partie d'un block */
            {
                if( pt_autre_piste->m_Sous_Netcode )
                {
                    change_equipot( pt_start_conn, pt_end_conn,
                                    pt_autre_piste->m_Sous_Netcode, pt_conn->m_Sous_Netcode );
                }
                else
                {
                    pt_autre_piste->m_Sous_Netcode = pt_conn->m_Sous_Netcode;
                }
            }
            else       /* La connexion ne fait partie d'aucun block */
            {
                if( pt_autre_piste->m_Sous_Netcode )
                {
                    pt_conn->m_Sous_Netcode = pt_autre_piste->m_Sous_Netcode;
                }
                else
                {
                    sous_net_code++; pt_conn->m_Sous_Netcode = sous_net_code;
                    pt_autre_piste->m_Sous_Netcode = pt_conn->m_Sous_Netcode;
                }
            }
        }

        PtStruct = pt_conn->end;
        if( PtStruct && (PtStruct->m_StructType != TYPEPAD) )
        {                                   
            /* fin connectee a une autre piste */
            pt_autre_piste = (TRACK*) PtStruct;
            
            if( pt_conn->m_Sous_Netcode )   /* La connexion fait deja partie d'un block */
            {
                if( pt_autre_piste->m_Sous_Netcode )
                {
                    change_equipot( pt_start_conn, pt_end_conn,
                                    pt_autre_piste->m_Sous_Netcode, pt_conn->m_Sous_Netcode );
                }
                else
                    pt_autre_piste->m_Sous_Netcode = pt_conn->m_Sous_Netcode;
            }
            else    /* La connexion ne fait partie d'aucun block */
            {
                if( pt_autre_piste->m_Sous_Netcode )
                {
                    pt_conn->m_Sous_Netcode = pt_autre_piste->m_Sous_Netcode;
                }
                else
                {
                    sous_net_code++; pt_conn->m_Sous_Netcode = sous_net_code;
                    pt_autre_piste->m_Sous_Netcode = pt_conn->m_Sous_Netcode;
                }
            }
        }
        if( pt_conn == pt_end_conn )
            break;
    }
}


/***************************************************/
void WinEDA_BasePcbFrame::test_connexions( wxDC* DC )
/***************************************************/

/*
 *  Routine recherchant les connexions deja faites et mettant a jour
 *  le status du chevelu ( Bit CH_ACTIF mis a 0 si connexion trouvee
 *  Les pistes sont supposees etre triees par ordre de net_code croissant
 */
{
    TRACK*     pt_start_conn, * pt_end_conn;
    int        ii;
    LISTE_PAD* pt_pad;
    int        current_net_code;

    /* Etablissement des equipotentielles vraies */
    pt_pad = m_Pcb->m_Pads;
    for( ii = 0; ii < m_Pcb->m_NbPads; ii++, pt_pad++ )
    {
        (*pt_pad)->m_physical_connexion = 0;
    }

    ////////////////////////////
    // Calcul de la connexite //
    ////////////////////////////

    /*  Les pointeurs .start et .end sont mis a jour, si la
     *   connexion est du type segment a segment
     */

    pt_start_conn = m_Pcb->m_Track;
    while( pt_start_conn != NULL )
    {
        current_net_code = pt_start_conn->m_NetCode;
        pt_end_conn = pt_start_conn->GetEndNetCode( current_net_code );

        /* Calcul des connexions type segment du net courant */
        calcule_connexite_1_net( pt_start_conn, pt_end_conn );

        pt_start_conn = (TRACK*) pt_end_conn->Pnext;
    }

    return;
}


/**************************************************************/
void WinEDA_BasePcbFrame::Recalcule_all_net_connexion( wxDC* DC )
/**************************************************************/

/*
 *  Routine Recalculant les pointeurs sur connexions types piste a piste
 *  relatives a tous les nets.
 *  Cette routine est utilisee apres reclassement des segments de piste par
 *  ordre de net_code, puisque des pointeurs sur connexions deviennent faux
 *  ( les pointeurs type pad restent bons )
 */
{
    TRACK* EndConn;
    int    net_code, net_code_max;

    if( m_Pcb->m_Track == NULL )
        return;

    /* calcul du net_code max */
    EndConn = m_Pcb->m_Track;
    while( EndConn->Pnext )
        EndConn = (TRACK*) EndConn->Pnext;

    net_code_max = EndConn->m_NetCode;

    for( net_code = 0; net_code <= net_code_max; net_code++ )
    {
        test_1_net_connexion( DC, net_code );
    }
}


/*************************************************************************/
void WinEDA_BasePcbFrame::test_1_net_connexion( wxDC* DC, int net_code )
/*************************************************************************/

/*
 *  Routine recherchant les connexions deja faites relatives a 1 net
 */
{
    TRACK*     pt_start_conn, * pt_end_conn;
    int        ii, nb_net_noconnect = 0;
    LISTE_PAD* pt_pad;
    wxString   msg;

    if( net_code == 0 )
        return;

    if( (m_Pcb->m_Status_Pcb & LISTE_CHEVELU_OK) == 0 )
        Compile_Ratsnest( DC, TRUE );

    pt_pad = (LISTE_PAD*) m_Pcb->m_Pads;
    for( ii = 0; ii < m_Pcb->m_NbPads; ii++, pt_pad++ )
    {
        int pad_net_code = (*pt_pad)->m_NetCode;
        if( pad_net_code < net_code )
            continue;
        if( pad_net_code > net_code )
            break;
        (*pt_pad)->m_physical_connexion = 0;
    }

    /* Determination des limites du net */
    if( m_Pcb->m_Track )
    {
        pt_end_conn   = NULL;
        pt_start_conn = m_Pcb->m_Track->GetStartNetCode( net_code );

        if( pt_start_conn )
            pt_end_conn = pt_start_conn->GetEndNetCode( net_code );

        if( pt_start_conn && pt_end_conn ) // c.a.d. s'il y a des segments
        {
            calcule_connexite_1_net( pt_start_conn, pt_end_conn );
        }
    }

    /* Test des chevelus */
    nb_net_noconnect = Test_1_Net_Ratsnest( DC, net_code );

    /* Affichage des resultats */
    msg.Printf( wxT( "links %d nc %d  net:nc %d" ),
                m_Pcb->m_NbLinks, m_Pcb->GetNumNoconnect(),
                nb_net_noconnect );

    Affiche_Message( msg );
    return;
}


/***************************************************************************/
static void calcule_connexite_1_net( TRACK* pt_start_conn, TRACK* pt_end_conn )
/***************************************************************************/

/* calcule la connexite d'un net constitue de segments de piste consecutifs.
 *  Entree:
 *      pt_start_conn = adresse du 1er segment ( debut du net )
 *      pt_end_conn = adr de fin (dernier segment)
 *  Les connexions relatives aux pads doivent etre deja calculees, car elles
 *  ne sont pas ici recalculees ( pour des raisons de temps de calcul, et
 *  du fait que lors des modif de pistes, les pads ne sont pas touches
 */
{
    TRACK* Track;

    /*  Les pointeurs .start et .end sont mis a jour, si la
     *   connexion est du type segment a segment.
     *  la connexion sur pads est supposee etre deja calculee */

    /* Raz des pointeurs sur pistes */
    for( Track = pt_start_conn; Track != NULL; Track = (TRACK*) Track->Pnext )
    {
        Track->m_Sous_Netcode = 0;

        if( Track->GetState( BEGIN_ONPAD ) == 0 )
            Track->start = NULL;
        if( Track->GetState( END_ONPAD ) == 0 )
            Track->end = NULL;

        if( Track == pt_end_conn )
            break;
    }

    /* calcul des connexions */
    for( Track = pt_start_conn; Track != NULL; Track = (TRACK*) Track->Pnext )
    {
        if( Track->m_StructType == TYPEVIA )
        {
            TRACK* pt_segm;
            int    layermask = Track->ReturnMaskLayer();
            for( pt_segm = pt_start_conn; pt_segm != NULL; pt_segm = (TRACK*) pt_segm->Pnext )
            {
                int curlayermask = pt_segm->ReturnMaskLayer();
                if( !pt_segm->start && (pt_segm->m_Start == Track->m_Start)
                   && ( layermask & curlayermask ) )
                {
                    pt_segm->start = Track;
                }
                if( !pt_segm->end && (pt_segm->m_End == Track->m_Start)
                   && (layermask & curlayermask) )
                {
                    pt_segm->end = Track;
                }
                if( pt_segm == pt_end_conn )
                    break;
            }
        }

        if( Track->start == NULL )
        {
            Track->start = Locate_Piste_Connectee( Track, Track, pt_end_conn, START );
        }

        if( Track->end == NULL )
        {
            Track->end = Locate_Piste_Connectee( Track, Track, pt_end_conn, END );
        }
        if( Track == pt_end_conn )
            break;
    }

    /* Generation des sous equipots du net */
    propage_equipot( pt_start_conn, pt_end_conn );
}


/********************************************/
/* Reattribution des net_codes de reference */
/********************************************/

/*
 *  reattribution des net_codes aux segments de pistes.
 *  Routine utilisee apres modification generale des noms de nets (apres
 *  lecrure netliste ou edition de nets sur pads, effacement /ajout de
 *  modules...)
 * 
 *  Elle se fait en 2 passes:
 *      1 - reattribution des segments commencant ( et/ou finissant ) sur 1 pad
 *          les autres segments sont mis a reference NULLE
 *      2 - reattribution des segments restantes a ref NULLE
 */

#define POS_AFF_CHREF 62

/* recherche le pad connecte a l'extremite de la piste de coord px, py
 *  parametres d'appel:
 *      px, py = coord du point tst
 *      masque_layer = couche(s) de connexion
 *      pt_liste = adresse de la liste des pointeurs de pads, tels que
 *      apparaissant apres build_liste_pad, mais classee par position X
 *      de pads croissantes.
 *  retourne : pointeur sur le pad connecte
 *  la routine travaille par dichotomie sur la liste des pads tries par pos X
 *  croissante, elle est donc beaucoup plus rapide que Fast_Locate_Pad_connecte,
 *  mais implique le calcul de cette liste.
 * 
 *  (la liste placee en m_Pcb->m_Pads et elle triee par netcodes croissants)
 */

static D_PAD* SuperFast_Locate_Pad_Connecte( BOARD* pcb, LISTE_PAD* pt_liste,
                                             int px, int py, int masque_layer )
{
    D_PAD*     pad;
    LISTE_PAD* ptr_pad, * lim;
    int        nb_pad = pcb->m_NbPads;
    int        ii;

    lim     = pt_liste + (pcb->m_NbPads - 1 );
    ptr_pad = pt_liste;
    while( nb_pad )
    {
        pad      = *ptr_pad;
        ii       = nb_pad;
        nb_pad >>= 1; if( (ii & 1) && ( ii > 1 ) )
            nb_pad++;
        if( pad->m_Pos.x < px ) /* on doit chercher plus loin */
        {
            ptr_pad += nb_pad; if( ptr_pad > lim )
                ptr_pad = lim;
            continue;
        }
        if( pad->m_Pos.x > px ) /* on doit chercher moins loin */
        {
            ptr_pad -= nb_pad;
            if( ptr_pad < pt_liste )
                ptr_pad = pt_liste;
            continue;
        }

        if( pad->m_Pos.x == px )  /* zone de classement trouvee */
        {
            /* recherche du debut de la zone */
            while( ptr_pad >= pt_liste )
            {
                pad = *ptr_pad;
                if( pad->m_Pos.x == px )
                    ptr_pad--;
                else
                    break;
            }

            ptr_pad++;  /* pointe depart de zone a pad->m_Pos.x = px */

            for( ; ; ptr_pad++ )
            {
                if( ptr_pad > lim )
                    return NULL;                        /* hors zone */
                pad = *ptr_pad;
                if( pad->m_Pos.x != px )
                    return NULL;                        /* hors zone */
                if( pad->m_Pos.y != py )
                    continue;
                /* Pad peut-etre trouve ici: il doit etre sur la bonne couche */
                if( pad->m_Masque_Layer & masque_layer )
                    return pad;
            }
        }
    }

    return NULL;
}


static int SortPadsByXCoord( void* pt_ref, void* pt_comp )

/* used to Sort a pad list by x coordinate value
 */
{
    D_PAD* ref  = *(LISTE_PAD*) pt_ref;
    D_PAD* comp = *(LISTE_PAD*) pt_comp;

    return ref->m_Pos.x - comp->m_Pos.x;
}


/****************************************************/
LISTE_PAD* CreateSortedPadListByXCoord( BOARD* pcb )
/****************************************************/

/* Create a sorted list of pointers to pads.
 *  This list is sorted by X ccordinate value.
 *  The list must be freed bu user
 */
{
    LISTE_PAD* pad_list = (LISTE_PAD*) MyMalloc( pcb->m_NbPads * sizeof( D_PAD *) );

    memcpy( pad_list, pcb->m_Pads, pcb->m_NbPads * sizeof( D_PAD *) );
    qsort( pad_list, pcb->m_NbPads, sizeof( D_PAD *),
           ( int( * ) ( const void*, const void* ) )SortPadsByXCoord );
    return pad_list;
}


/******************************************************************/
void WinEDA_BasePcbFrame::reattribution_reference_piste( int affiche )
/******************************************************************/
{
    TRACK*          pt_piste,
    * pt_next;
    int             a_color;
    char            new_passe_request = 1, flag;
    LISTE_PAD*      pt_mem;
    BOARD_ITEM* PtStruct;
    int             masque_layer;
    wxString        msg;

    if( m_Pcb->m_NbPads == 0 )
        return;
    a_color = CYAN;
    if( affiche )
        Affiche_1_Parametre( this, POS_AFF_CHREF, wxT( "DataBase" ), wxT( "Netcodes" ), a_color );

    recalcule_pad_net_code();

    if( affiche )
        Affiche_1_Parametre( this, -1, wxEmptyString, wxT( "Gen Pads " ), a_color );

    //////////////////////////////////////////////////////
    // Connexion des pistes accrochees a 1 pad au moins //
    //////////////////////////////////////////////////////
    pt_mem = CreateSortedPadListByXCoord( m_Pcb );

    if( affiche )
        Affiche_1_Parametre( this, -1, wxEmptyString, wxT( "Conn Pads" ), a_color );

    /* Raz des flags particuliers des segments de piste */
    pt_piste = m_Pcb->m_Track;
    for( ; pt_piste != NULL; pt_piste = (TRACK*) pt_piste->Pnext )
    {
        pt_piste->SetState( BUSY | EDIT | BEGIN_ONPAD | END_ONPAD, OFF );
        pt_piste->m_NetCode = 0;
    }

    pt_piste = m_Pcb->m_Track;
    for( ; pt_piste != NULL; pt_piste = (TRACK*) pt_piste->Pnext )
    {
        flag = 0;
        masque_layer = g_TabOneLayerMask[pt_piste->GetLayer()];

        /* y a t-il une pastille sur une extremite */
        pt_piste->start = SuperFast_Locate_Pad_Connecte( m_Pcb,
                                                         pt_mem,
                                                         pt_piste->m_Start.x,
                                                         pt_piste->m_Start.y,
                                                         masque_layer );
        if( pt_piste->start != NULL )
        {
            pt_piste->SetState( BEGIN_ONPAD, ON );
            pt_piste->m_NetCode = ( (D_PAD*) (pt_piste->start) )->m_NetCode;
        }

        pt_piste->end = SuperFast_Locate_Pad_Connecte( m_Pcb,
                                                       pt_mem,
                                                       pt_piste->m_End.x,
                                                       pt_piste->m_End.y,
                                                       masque_layer );

        if( pt_piste->end != NULL )
        {
            pt_piste->SetState( END_ONPAD, ON );
            pt_piste->m_NetCode = ( (D_PAD*) (pt_piste->end) )->m_NetCode;
        }
    }

    MyFree( pt_mem );

    ////////////////////////////////////////////////////
    // Calcul de la connexite entre segments de piste //
    ////////////////////////////////////////////////////

    /*  Les pointeurs .start et .end sont mis a jour, s'ils etaient NULLs.
     *  La connexion est alors du type segment a segment
     */
    if( affiche )
        Affiche_1_Parametre( this, POS_AFF_CHREF, wxEmptyString, wxT( "Conn Segm" ), a_color );

    for( pt_piste = m_Pcb->m_Track; pt_piste != NULL; pt_piste = pt_piste->Next() )
    {
        if( pt_piste->start == NULL )
        {
            pt_piste->start = Locate_Piste_Connectee( pt_piste, m_Pcb->m_Track, NULL, START );
        }

        if( pt_piste->end == NULL )
        {
            pt_piste->end = Locate_Piste_Connectee( pt_piste, m_Pcb->m_Track, NULL, END );
        }
    }

    ////////////////////////////////
    // Reattribution des net_code //
    ////////////////////////////////

    a_color = YELLOW;

    while( new_passe_request )
    {
        bool reset_flag = FALSE;
        new_passe_request = 0;
        if( affiche )
        {
            msg.Printf( wxT( "Net->Segm pass %d  " ), new_passe_request + 1 );
            Affiche_1_Parametre( this, POS_AFF_CHREF, wxEmptyString, msg, a_color );
        }

        /* look for vias which could be connect many tracks */
        for( TRACK* via = m_Pcb->m_Track; via != NULL; via = via->Next() )
        {
            if( via->m_StructType != TYPEVIA )
                continue;
            if( via->m_NetCode > 0 )
                continue;                       // Netcode already known
            // Lock for a connection to a track with a known netcode
            pt_next = m_Pcb->m_Track;
            while( ( pt_next = Locate_Piste_Connectee( via, pt_next, NULL, START ) ) != NULL )
            {
                if( pt_next->m_NetCode )
                {
                    via->m_NetCode = pt_next->m_NetCode;
                    break;
                }
                pt_next->SetState( BUSY, ON );
                reset_flag = TRUE;
            }
        }

        if( reset_flag )
            for( pt_piste = m_Pcb->m_Track; pt_piste != NULL; pt_piste = pt_piste->Next() )
            {
                pt_piste->SetState( BUSY, OFF );
            }

        for( pt_piste = m_Pcb->m_Track; pt_piste != NULL; pt_piste = pt_piste->Next() )
        {
            /* Traitement du point de debut */
            PtStruct = (BOARD_ITEM*) pt_piste->start;
            if( PtStruct && (PtStruct->m_StructType != TYPEPAD) )
            {    // Begin on an other track segment
                pt_next = (TRACK*) PtStruct;
                if( pt_piste->m_NetCode )
                {
                    if( pt_next->m_NetCode == 0 )
                    {
                        new_passe_request  = 1;
                        pt_next->m_NetCode = pt_piste->m_NetCode;
                    }
                }
                else
                {
                    if( pt_next->m_NetCode != 0 )
                    {
                        pt_piste->m_NetCode = pt_next->m_NetCode;
                        new_passe_request   = 1;
                    }
                }
            }

            /* Localisation du point de fin */
            PtStruct = pt_piste->end;
            if( PtStruct &&(PtStruct->m_StructType != TYPEPAD) )
            {        // End sur piste
                pt_next = (TRACK*) PtStruct;
                if( pt_piste->m_NetCode )
                {
                    if( pt_next->m_NetCode == 0 )
                    {
                        new_passe_request  = 1;
                        pt_next->m_NetCode = pt_piste->m_NetCode;
                    }
                }
                else
                {
                    if( pt_next->m_NetCode != 0 )
                    {
                        pt_piste->m_NetCode = pt_next->m_NetCode;
                        new_passe_request   = 1;
                    }
                }
            }
        }
    }

    /* Reclassemment des pistes par numero de net: */
    if( affiche )
        Affiche_1_Parametre( this, -1, wxEmptyString, wxT( "Reorder " ), a_color );
    RebuildTrackChain( m_Pcb );

    if( affiche )
        Affiche_1_Parametre( this, -1, wxEmptyString, wxT( "         " ), a_color );
}


/*
 *  routine de tri de connexion utilisee par la fonction QSORT
 *  le tri est fait par numero de net
 */
int tri_par_netcode( TRACK** pt_ref, TRACK** pt_compare )
{
    int ii;

    ii = (*pt_ref)->m_NetCode - (*pt_compare)->m_NetCode;
    return ii;
}


/*****************************************/
static void RebuildTrackChain( BOARD* pcb )
/*****************************************/

/* Recalcule le chainage des pistes pour que le chainage soit fait par
 *  netcodes croissants
 */
{
    TRACK* Track, ** Liste;
    int    ii, nbsegm;

    /* Count segments */
    nbsegm = pcb->GetNumSegmTrack();
    if( pcb->m_Track == NULL )
        return;

    Liste = (TRACK**) MyZMalloc( (nbsegm + 1) * sizeof(TRACK *) );

    ii = 0; Track = pcb->m_Track;
    for( ; Track != NULL; ii++, Track = (TRACK*) Track->Pnext )
    {
        Liste[ii] = Track;
    }

    qsort( Liste, nbsegm, sizeof(TRACK *),
           ( int( * ) ( const void*, const void* ) )tri_par_netcode );

    /* Update the linked list pointers */

    Track = Liste[0];
    Track->Pback = pcb; Track->Pnext = Liste[1];
    pcb->m_Track = Track;
    for( ii = 1; ii < nbsegm; ii++ )
    {
        Track = Liste[ii];
        Track->Pback = Liste[ii - 1];
        Track->Pnext = Liste[ii + 1];
    }

    MyFree( Liste );
}
