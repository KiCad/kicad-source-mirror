/***************************************************/
/* Localisation des elements pointes par la souris */
/***************************************************/

#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "trigo.h"

#include "protos.h"


/* variables locales */
int ux0, uy0, dx, dy, spot_cX, spot_cY;     /* Variables utilisees pour
                                             *  la localisation des segments */
/* fonctions locales */
static TRACK*       Locate_Zone( TRACK* start_adresse, int layer, int typeloc );
static TRACK*       Locate_Zone( TRACK* start_adresse, wxPoint ref, int layer );
static TRACK*       Locate_Pistes( TRACK* start_adresse, int Layer, int typeloc );
static TRACK*       Locate_Pistes( TRACK* start_adresse, wxPoint ref, int Layer );
static DRAWSEGMENT* Locate_Segment_Pcb( BOARD* Pcb, int typeloc );
static TEXTE_PCB*   Locate_Texte_Pcb( TEXTE_PCB* pt_txt_pcb, int typeloc );
static int          distance( int seuil );

/**/

/* Macro de calcul de la coord de pointage selon le curseur
 *  (ON/OFF grille) choisi
 */
#define SET_REF_POS( ref )  if( typeloc == CURSEUR_ON_GRILLE ) \
    { ref = ActiveScreen->m_Curseur; } \
    else { ref = ActiveScreen->m_MousePosition; }


/*************************************************************/
EDA_BaseStruct* WinEDA_GerberFrame::Locate( int typeloc )
/*************************************************************/

/* Fonction de localisation generale
 *  Affiche les caract de la stucture localisée et retourne un pointeur
 *  sur celle-ci
 */
{
    TEXTE_PCB*   pt_texte_pcb;
    TRACK*       Track, * TrackLocate;
    DRAWSEGMENT* DrawSegm;
    int          layer;

    /* Localistion des pistes et vias, avec priorite aux vias */
    layer = GetScreen()->m_Active_Layer;
    Track = Locate_Pistes( m_Pcb->m_Track, -1, typeloc );
    if( Track != NULL )
    {
        TrackLocate = Track;   /* Reperage d'une piste ou via */
        /* recherche de 1 via eventuelle */
        while( ( TrackLocate = Locate_Pistes( TrackLocate, layer, typeloc ) ) != NULL )
        {
            Track = TrackLocate;
            if( TrackLocate->m_StructType == TYPEVIA )
                break;
            TrackLocate = (TRACK*) TrackLocate->Pnext;
        }

        Track->Display_Infos( this );
        return Track;
    }


    pt_texte_pcb = Locate_Texte_Pcb( (TEXTE_PCB*) m_Pcb->m_Drawings, typeloc );
    if( pt_texte_pcb ) // texte type PCB localise
    {
        pt_texte_pcb->Display_Infos( this );
        return pt_texte_pcb;
    }

    if( ( DrawSegm = Locate_Segment_Pcb( m_Pcb, typeloc ) ) != NULL )
    {
        return DrawSegm;
    }

    if( ( TrackLocate = Locate_Zone( (TRACK*) m_Pcb->m_Zone,
                                    GetScreen()->m_Active_Layer, typeloc ) ) != NULL )
    {
        TrackLocate->Display_Infos( this ); 
        return TrackLocate;
    }

    MsgPanel->EraseMsgBox();
    return NULL;
}


/********************************************************/
DRAWSEGMENT* Locate_Segment_Pcb( BOARD* Pcb, int typeloc )
/********************************************************/

/* Localisation de segments de contour du type edge pcb ou draw
 *  (selon couche active)
 *  Retourne:
 *      Pointeur sur DEBUT du segment localise
 *      NULL si rien trouve
 */
{
    EDA_BaseStruct* PtStruct;
    DRAWSEGMENT*    pts;
    wxPoint         ref;
    PCB_SCREEN*     screen = (PCB_SCREEN*) ActiveScreen;

    SET_REF_POS( ref );

    PtStruct = Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->m_StructType != TYPEDRAWSEGMENT )
            continue;
        pts = (DRAWSEGMENT*) PtStruct;
        ux0 = pts->m_Start.x; uy0 = pts->m_Start.y;
        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx      = pts->m_End.x - ux0; dy = pts->m_End.y - uy0;
        spot_cX = ref.x - ux0; spot_cY = ref.y - uy0;

        /* detection : */
        if( pts->m_Layer != screen->m_Active_Layer )
            continue;

        if( (pts->m_Shape == S_CIRCLE) || (pts->m_Shape == S_ARC) )
        {
            int rayon, dist, StAngle, EndAngle, MouseAngle;
            rayon = (int) hypot( (double) (dx), (double) (dy) );
            dist  = (int) hypot( (double) (spot_cX), (double) (spot_cY) );
            if( abs( rayon - dist ) <= (pts->m_Width / 2) )
            {
                if( pts->m_Shape == S_CIRCLE )
                    return pts;
                /* pour un arc, controle complementaire */
                MouseAngle = (int) ArcTangente( spot_cY, spot_cX );
                StAngle    = (int) ArcTangente( dy, dx );
                EndAngle   = StAngle + pts->m_Angle;

                if( EndAngle > 3600 )
                {
                    StAngle -= 3600; EndAngle -= 3600;
                }
                if( (MouseAngle >= StAngle) && (MouseAngle <= EndAngle) )
                    return pts;
            }
        }
        else
        {
            if( distance( pts->m_Width / 2 ) )
                return pts;
        }
    }

    return NULL;
}


/****************************************************************************/
/* TRACK *Locate_Pistes(TRACK * start_adresse, int MasqueLayer,int typeloc)	*/
/* TRACK *Locate_Pistes(TRACK * start_adresse, wxPoint ref, int MasqueLayer)*/
/****************************************************************************/

/*
 *  1 -  routine de localisation du segment de piste pointe par la souris.
 *  2 -  routine de localisation du segment de piste pointe par le point
 *          ref_pX , ref_pY.r
 * 
 *  La recherche commence a l'adresse start_adresse
 */

TRACK* Locate_Pistes( TRACK* start_adresse, int Layer, int typeloc )
{
    wxPoint ref;

    SET_REF_POS( ref );

    return Locate_Pistes( start_adresse, ref, Layer );
}


TRACK* Locate_Pistes( TRACK* start_adresse, wxPoint ref, int Layer )
{
    TRACK* Track;               /* pointeur sur les pistes */
    int    l_piste;             /* demi-largeur de la piste */

    for( Track = start_adresse; Track != NULL; Track = (TRACK*) Track->Pnext )
    {
        if( Track->GetState( BUSY | DELETED ) )
            continue;
        /* calcul des coordonnees du segment teste */
        l_piste = Track->m_Width >> 1;                  /* l_piste = demi largeur piste */
        ux0 = Track->m_Start.x; uy0 = Track->m_Start.y; /* coord de depart */
        dx  = Track->m_End.x; dy = Track->m_End.y;      /* coord d'arrivee */

        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx     -= ux0; dy -= uy0;
        spot_cX = ref.x - ux0; spot_cY = ref.y - uy0;

        if( Track->m_StructType == TYPEVIA ) /* VIA rencontree */
        {
            if( (abs( spot_cX ) <= l_piste ) && (abs( spot_cY ) <=l_piste) )
            {
                return Track;
            }
            continue;
        }

        if( Layer >= 0 )
            if( Track->m_Layer != Layer )
                continue;/* Segments sur couches differentes */
        if( distance( l_piste ) )
            return Track;
    }

    return NULL;
}


/****************************************************************/
/* TRACK * Locate_Zone(TRACK * start_adresse, int layer,	 */
/*											int typeloc)		*/
/* TRACK * Locate_Zone(TRACK * start_adresse,wxPoint ref, int layer) */
/****************************************************************/

/*
 *  1 -  routine de localisation du segment de zone pointe par la souris.
 *  2 -  routine de localisation du segment de zone pointe par le point
 *          ref_pX , ref_pY.r
 * 
 *  Si layer == -1 , le tst de la couche n'est pas fait
 * 
 *  La recherche commence a l'adresse start_adresse
 */

TRACK* Locate_Zone( TRACK* start_adresse, int layer, int typeloc )
{
    wxPoint ref;

    SET_REF_POS( ref );

    return Locate_Zone( start_adresse, ref, layer );
}


TRACK* Locate_Zone( TRACK* start_adresse, wxPoint ref, int layer )
{
    TRACK* Zone;                /* pointeur sur les pistes */
    int    l_segm;              /* demi-largeur de la piste */

    for( Zone = start_adresse; Zone != NULL; Zone = (TRACK*) Zone->Pnext )
    {
        /* calcul des coordonnees du segment teste */
        l_segm = Zone->m_Width >> 1;                        /* l_piste = demi largeur piste */
        ux0    = Zone->m_Start.x; uy0 = Zone->m_Start.y;    /* coord de depart */
        dx = Zone->m_End.x; dy = Zone->m_End.y;             /* coord d'arrivee */

        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx     -= ux0; dy -= uy0;
        spot_cX = ref.x - ux0; spot_cY = ref.y - uy0;

        if( (layer != -1) && (Zone->m_Layer != layer) )
            continue;
        if( distance( l_segm ) )
            return Zone;
    }

    return NULL;
}


/***************************************************************/
/* TEXTE_PCB * Locate_Texte_Pcb(char * pt_txt_pcb,int typeloc) */
/***************************************************************/

/* localisation des inscriptions sur le Pcb:
 *  entree : char pointeur sur le debut de la zone de recherche
 *  retour : pointeur sur la description du texte localise
 */

TEXTE_PCB* Locate_Texte_Pcb( TEXTE_PCB* pt_txt_pcb, int typeloc )
{
    int             angle;
    EDA_BaseStruct* PtStruct;
    wxPoint         ref;

    SET_REF_POS( ref );
    PtStruct = (EDA_BaseStruct*) pt_txt_pcb;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->m_StructType != TYPETEXTE )
            continue;
        pt_txt_pcb = (TEXTE_PCB*) PtStruct;

        angle = pt_txt_pcb->m_Orient;
        ux0   = pt_txt_pcb->m_Pos.x; uy0 = pt_txt_pcb->m_Pos.y;
        dx    = ( pt_txt_pcb->m_Size.x * pt_txt_pcb->GetLength() ) / 2;
        dy    = pt_txt_pcb->m_Size.y / 2;

        dx *= 13; dx /= 9;  /* Facteur de forme des lettres : 13/9 */

        /* la souris est-elle dans ce rectangle  autour du centre  */
        spot_cX = ref.x - ux0; spot_cY = ref.y - uy0;
        RotatePoint( &spot_cX, &spot_cY, -angle );
        if( ( abs( spot_cX ) <= abs( dx ) ) && ( abs( spot_cY ) <= abs( dy ) ) )
            return pt_txt_pcb;
    }

    return NULL;
}


/*****************************/
/* int distance(int seuil) */
/*****************************/

/*
 *  Calcul de la distance du curseur souris a un segment de droite :
 *  ( piste, edge, contour module ..
 *  retourne:
 *      0 si distance > seuil
 *      1 si distance <= seuil
 *  Variables utilisees ( doivent etre initialisees avant appel , et
 *  sont ramenees au repere centre sur l'origine du segment)
 *      dx, dy = coord de l'extremite segment.
 *      spot_cX,spot_cY = coord du curseur souris
 *  la recherche se fait selon 4 cas:
 *      segment horizontal
 *      segment vertical
 *      segment 45
 *      segment quelconque
 */

int distance( int seuil )
{
    int cXrot, cYrot,   /* coord du point (souris) dans le repere tourne */
        segX, segY;     /* coord extremite segment tj >= 0 */
    int pointX, pointY;/* coord point a tester dans repere modifie dans lequel
                     *  segX et segY sont >=0 */

    segX = dx; segY = dy; pointX = spot_cX; pointY = spot_cY;

    /*Recalcul coord pour que le segment soit dans 1er quadrant (coord >= 0)*/
    if( segX < 0 )   /* mise en >0 par symetrie par rapport a l'axe Y */
    {
        segX = -segX; pointX = -pointX;
    }
    if( segY < 0 )   /* mise en > 0 par symetrie par rapport a l'axe X */
    {
        segY = -segY; pointY = -pointY;
    }


    if( segY == 0 ) /* piste Horizontale */
    {
        if( abs( pointY ) <= seuil )
        {
            if( (pointX >= 0) && (pointX <= segX) )
                return 1;
            /* Etude des extremites : cercle de rayon seuil */
            if( (pointX < 0) && (pointX >= -seuil) )
            {
                if( ( (pointX * pointX) + (pointY * pointY) ) <= (seuil * seuil) )
                    return 1;
            }
            if( (pointX > segX) && ( pointX <= (segX + seuil) ) )
            {
                if( ( ( (pointX - segX) * (pointX - segX) ) + (pointY * pointY) ) <=
                   (seuil * seuil) )
                    return 1;
            }
        }
    }
    else if( segX == 0 ) /* piste verticale */
    {
        if( abs( pointX ) <= seuil )
        {
            if( (pointY >= 0 ) && (pointY <= segY) )
                return 1;
            if( (pointY < 0) && (pointY >= -seuil) )
            {
                if( ( (pointY * pointY) + (pointX * pointX) ) <= (seuil * seuil) )
                    return 1;
            }
            if( (pointY > segY) && ( pointY <= (segY + seuil) ) )
            {
                if( ( ( (pointY - segY) * (pointY - segY) ) + (pointX * pointX) ) <=
                   (seuil * seuil) )
                    return 1;
            }
        }
    }
    else if( segX == segY )    /* piste a 45 degre */
    {
        /* on fait tourner les axes de 45 degre. la souris a alors les
         *  coord : x1 = x*cos45 + y*sin45
         *      y1 = y*cos45 - x*sin45
         *  et le segment de piste est alors horizontal.
         *  recalcul des coord de la souris ( sin45 = cos45 = .707 = 7/10
         *  remarque : sin ou cos45 = .707, et lors du recalcul des coord
         *  dx45 et dy45, lec coeff .707 est neglige, dx et dy sont en fait .707 fois
         *  trop grands. (c.a.d trop petits)
         *  spot_cX,Y doit etre * par .707 * .707 = 0.5 */

        cXrot = (pointX + pointY) >> 1;
        cYrot = (pointY - pointX) >> 1;

        /* recalcul des coord de l'extremite du segment , qui sera vertical
         *  suite a l'orientation des axes sur l'ecran : dx45 = pointX (ou pointY)
         *  et est en fait 1,414 plus grand , et dy45 = 0 */

        // seuil doit etre * .707 pour tenir compte du coeff de reduction sur dx,dy
        seuil *= 7; seuil /= 10;
        if( abs( cYrot ) <= seuil ) /* ok sur axe vertical) */
        {
            if( (cXrot >= 0) && (cXrot <= segX) )
                return 1;

            /* Etude des extremites : cercle de rayon seuil */
            if( (cXrot < 0) && (cXrot >= -seuil) )
            {
                if( ( (cXrot * cXrot) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return 1;
            }
            if( (cXrot > segX) && ( cXrot <= (segX + seuil) ) )
            {
                if( ( ( (cXrot - segX) * (cXrot - segX) ) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return 1;
            }
        }
    }
    else    /* orientation quelconque */
    {
        /* On fait un changement d'axe (rotation) de facon a ce que le segment
         *  de piste soit horizontal dans le nouveau repere */
        int angle;

        angle = (int) ( atan2( (float) segY, (float) segX ) * 1800 / M_PI);
        cXrot = pointX; cYrot = pointY;
        RotatePoint( &cXrot, &cYrot, angle );   /* Rotation du point a tester */
        RotatePoint( &segX, &segY, angle );     /* Rotation du segment */

        /*la piste est Horizontale , par suite des modifs de coordonnes
         *  et d'axe, donc segX = longueur du segment */

        if( abs( cYrot ) <= seuil ) /* ok sur axe vertical) */
        {
            if( (cXrot >= 0) && (cXrot <= segX) )
                return 1;
            /* Etude des extremites : cercle de rayon seuil */
            if( (cXrot < 0) && (cXrot >= -seuil) )
            {
                if( ( (cXrot * cXrot) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return 1;
            }
            if( (cXrot > segX) && ( cXrot <= (segX + seuil) ) )
            {
                if( ( ( (cXrot - segX) * (cXrot - segX) ) + (cYrot * cYrot) ) <= (seuil * seuil) )
                    return 1;
            }
        }
    }
    return 0;
}
