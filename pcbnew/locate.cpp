/*****************************/
/* Localisation des elements */
/*****************************/


#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"
#include "trigo.h"
#include "autorout.h"

#include "protos.h"


/* variables locales */
int ux0, uy0, dx, dy, spot_cX, spot_cY;     /* Variables utilisees pour
                                             *  la localisation des segments */
/* fonctions locales */
EDA_BaseStruct* Locate_MirePcb( EDA_BaseStruct* PtStruct, int LayerSearch, int typeloc );


/**
 * Function RefPos
 * returns the reference position, coming from either the mouse position or the
 * the cursor position, based on whether the typeloc has the CURSEUR_OFF_GRILLE 
 * flag ORed in or not.
 * @param typeloc int with possible CURSEUR_OFF_GRILLE bit on.
 * @return wxPoint - The reference point, either the mouse position or 
 *   the cursor position.
 */
wxPoint inline RefPos( int typeloc )
{
    if( typeloc & CURSEUR_OFF_GRILLE )
        return ActiveScreen->m_MousePosition;
    else
        return ActiveScreen->m_Curseur;
}
 

/**
 * Function IsModuleLayerVisible
 * expects either of the two layers on which a module can reside, and returns
 * whether that layer is visible.
 * @param layer One of the two allowed layers for modules: CMP_N or CUIVRE_N
 * @return bool - true if the layer is visible, else false.
 */
bool inline IsModuleLayerVisible( int layer ) 
{
    if( layer==CMP_N )
        return DisplayOpt.Show_Modules_Cmp;

    else if( layer==CUIVRE_N )
        return DisplayOpt.Show_Modules_Cu;

    else
        return true;
}


/*************************************************************/
MODULE* ReturnModule( BOARD* pcb, const wxString& reference )
/*************************************************************/

/*
 *  Recherche d'un module par sa reference
 *  Retourne:
 *      un pointeur sur le module
 *      Null si pas localisé
 */
{
    MODULE* Module = pcb->m_Modules;

    for(  ; Module != NULL; Module = (MODULE*) Module->Pnext )
    {
        if( reference.CmpNoCase( Module->m_Reference->m_Text ) == 0 )
            return Module;
    }

    return NULL;
}


/********************************************************/
D_PAD* ReturnPad( MODULE* module, const wxString& name )
/********************************************************/

/* Recherche d'un pad par son nom, pour le module Module
 */
{
    D_PAD*   pt_pad;
    wxString buf;

    if( module == NULL )
        return NULL;

    pt_pad = module->m_Pads;

    for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
    {
        pt_pad->ReturnStringPadName( buf );
        if( buf.CmpNoCase( name ) == 0 )
            return pt_pad;
    }

    return NULL;
}


/*******************************************************************************/
EDA_BaseStruct* WinEDA_BasePcbFrame::Locate( int typeloc, int LayerSearch )
/*******************************************************************************/

/* General locate function
 * Display infos relatives to the item found
 * return a pointer to this item ( or NULL )
 */
{
    TEXTE_PCB*      pt_texte_pcb;
    TRACK*          Track, * TrackLocate;
    DRAWSEGMENT*    DrawSegm;
    MODULE*         module;
    D_PAD*          pt_pad;
    int             masque_layer;
    EDA_BaseStruct* item;

    pt_texte_pcb = Locate_Texte_Pcb( m_Pcb->m_Drawings, LayerSearch, typeloc );
    if( pt_texte_pcb ) // a PCB text is found
    {
        Affiche_Infos_PCB_Texte( this, pt_texte_pcb );
        return pt_texte_pcb;
    }

    DrawSegm = Locate_Segment_Pcb( m_Pcb, LayerSearch, typeloc );
    if( DrawSegm != NULL )
    {
        Affiche_Infos_DrawSegment( this, DrawSegm );
        return DrawSegm;
    }

    item = Locate_Cotation( m_Pcb, LayerSearch, typeloc );
    if( item != NULL )
        return item;

    item = Locate_MirePcb( m_Pcb->m_Drawings, LayerSearch, typeloc );
    if( item != NULL )
        return item;

    /* Search for tracks and vias, with via priority */
    if( LayerSearch == -1 )
        masque_layer = ALL_LAYERS;
    else
        masque_layer = g_TabOneLayerMask[LayerSearch];

    Track = Locate_Pistes( m_Pcb->m_Track, masque_layer, typeloc );
    if( Track != NULL )
    {
        TrackLocate = Track;   /* a track or a via is found */
        
        /* Search for a via */
        while( ( TrackLocate = Locate_Pistes( TrackLocate,
                                              masque_layer, typeloc ) ) != NULL )
        {
            Track = TrackLocate;
            if( TrackLocate->m_StructType == TYPEVIA )
                break;

            TrackLocate = (TRACK*) TrackLocate->Pnext;
        }

        Affiche_Infos_Piste( this, Track );
        return Track;
    }

    /* Search for Pads */
    if( ( pt_pad = Locate_Any_Pad( m_Pcb, typeloc ) ) != NULL )
    {
        pt_pad->Display_Infos( this ); return pt_pad;
    }

    /* Search for a footprint text */

    // First search: locate texts for footprints on copper or component layer
    // Priority to the active layer (component or copper).
    // This is useful for small smd components when 2 texts overlap but are not 
    // on the same layer
    if( LayerSearch == LAYER_CUIVRE_N   ||   LayerSearch == CMP_N )
    {
        for( module = m_Pcb->m_Modules; module != NULL; module = (MODULE*) module->Pnext )
        {
            TEXTE_MODULE* pt_texte;

            if( module->m_Layer != LayerSearch )
                continue;

            pt_texte = LocateTexteModule( m_Pcb, &module, typeloc | VISIBLE_ONLY );
            if( pt_texte != NULL )
            {
                Affiche_Infos_E_Texte( this, module, pt_texte );
                return pt_texte;
            }
        }
    }

    // Now Search footprint texts on all layers
    module = NULL;
    {
        TEXTE_MODULE* pt_texte;
        pt_texte = LocateTexteModule( m_Pcb, &module, typeloc | VISIBLE_ONLY );
        if( pt_texte != NULL )
        {
            Affiche_Infos_E_Texte( this, module, pt_texte );
            return pt_texte;
        }
    }

    /* Search for a footprint */
    module = Locate_Prefered_Module( m_Pcb, typeloc | VISIBLE_ONLY );
    if( module != NULL )
    {
        module->Display_Infos( this );
        return module;
    }

    /* Search for zones */
    if( ( TrackLocate = Locate_Zone( (TRACK*) m_Pcb->m_Zone,
                                    GetScreen()->m_Active_Layer, typeloc ) ) != NULL )
    {
        Affiche_Infos_Piste( this, TrackLocate );
        return TrackLocate;
    }

    MsgPanel->EraseMsgBox();
    return NULL;
}


/*******************************************************************/
TRACK* Locate_Via( BOARD* Pcb, const wxPoint& pos, int layer )
/*******************************************************************/

/* Localise une via au point pX,pY
 *  Si layer < 0 la via sera localisee quelle que soit la couche
 *  Si layer = 0 .. 15 la via sera localisee selon son type:
 *      - traversante : toutes couches
 *      - aveugle = entre couches utiles
 *      - borgnes idem
 *  Entree : coord du point de reference, couche
 *  Sortie: NULL si pas de via
 *          (TRACK*) adresse de la via
 */
{
    TRACK* Track;

    for( Track = Pcb->m_Track; Track != NULL; Track = Track->Next() )
    {
        if( Track->m_StructType != TYPEVIA )
            continue;
        if( Track->m_Start != pos )
            continue;
        if( Track->GetState( BUSY | DELETED ) )
            continue;
        if( layer < 0 )
            return Track;
        if( ( (SEGVIA*) Track )->IsViaOnLayer( layer ) )
            return Track;
    }

    return NULL;
}


/********************************************************************/
D_PAD* Locate_Pad_Connecte( BOARD* Pcb, TRACK* ptr_piste, int extr )
/********************************************************************/

/* localisation de la pastille connectee au point de piste a tester
 *  entree : ptr_piste: pointeur sur le segment de piste
 *          extr = flag = START -> debut du segment a tester
 *                      = END -> fin du segment a tester
 *  retourne:
 *       un pointeur sur la description de la pastille si localisation
 *       pointeur NULL si pastille non trouvee
 */
{
    D_PAD*  ptr_pad = NULL;
    int     masque_layer;
    MODULE* module;
    wxPoint ref_pos;

    masque_layer = g_TabOneLayerMask[ptr_piste->m_Layer];
    if( extr == START )
    {
        ref_pos = ptr_piste->m_Start;
    }
    else
    {
        ref_pos = ptr_piste->m_End;
    }
    module = Pcb->m_Modules;
    for( ; module != NULL; module = (MODULE*) module->Pnext )
    {
        ptr_pad = Locate_Pads( module, ref_pos, masque_layer );
        if( ptr_pad != NULL )
            break;
    }

    return ptr_pad;
}


/****************************************************************/
EDGE_MODULE* Locate_Edge_Module( MODULE* module, int typeloc )
/****************************************************************/

/* Localisation de segments de contour du type edge MODULE
 *      Les contours sont de differents type:
 *      simple : succession de droites
 *      Arcs de cercles : on a alors debut arc, fin arc , centre
 *                      si debut arc = fin arc : cercle complet
 * 
 *  Retourne:
 *      Pointeur sur le segment localise
 *      NULL si rien trouve
 */
{
    EDGE_MODULE*    edge_mod;
    EDA_BaseStruct* PtStruct;
    int             uxf, uyf, type_trace;
    int             rayon, dist;
    wxPoint         ref_pos;        /* coord du point de localisation */

    /* pour localisation d'arcs, angle du point de debut, de fin et du point de reference */
    int             StAngle, EndAngle, MouseAngle;

    if( !module )
        return NULL;

    ref_pos = RefPos( typeloc );

    PtStruct = module->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->m_StructType != TYPEEDGEMODULE )
            continue;
        edge_mod   = (EDGE_MODULE*) PtStruct;
        type_trace = edge_mod->m_Shape;
        ux0 = edge_mod->m_Start.x; uy0 = edge_mod->m_Start.y;
        uxf = edge_mod->m_End.x; uyf = edge_mod->m_End.y;

        switch( type_trace )
        {
        case S_SEGMENT:
            /* recalcul des coordonnees avec ux0,uy0 = origine des coord. */
            spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;
            dx = uxf - ux0; dy = uyf - uy0;
            /* detection : */
            if( distance( edge_mod->m_Width / 2 ) )
                return edge_mod;
            break;

        case S_CIRCLE:
            rayon = (int) hypot( (double) (uxf - ux0), (double) (uyf - uy0) );
            dist  = (int) hypot( (double) (ref_pos.x - ux0), (double) (ref_pos.y - uy0) );

            if( abs( rayon - dist ) <= edge_mod->m_Width )
                return edge_mod;
            break;

        case S_ARC:
            rayon = (int) hypot( (double) (uxf - ux0), (double) (uyf - uy0) );
            dist  = (int) hypot( (double) (ref_pos.x - ux0), (double) (ref_pos.y - uy0) );

            if( abs( rayon - dist ) > edge_mod->m_Width )
                break;

            /* pour un arc, controle complementaire */
            MouseAngle = (int) ArcTangente( ref_pos.y - uy0, ref_pos.x - ux0 );
            StAngle    = (int) ArcTangente( uyf - uy0, uxf - ux0 );
            EndAngle   = StAngle + edge_mod->m_Angle;

            if( EndAngle > 3600 )
            {
                StAngle -= 3600; EndAngle -= 3600;
            }

            if( (MouseAngle >= StAngle) && (MouseAngle <= EndAngle) )
                return edge_mod;

            break;
        }
    }

    return NULL;
}


/*************************************************************************/
EDA_BaseStruct* Locate_Cotation( BOARD* Pcb, int LayerSearch, int typeloc )
/*************************************************************************/

/* Serach for a cotation item , on LayerSearch,
 *  (if LayerSearch == -1 , no yaere restriction )
 *  return a pointer to the located item, or NULL
 */
{
    EDA_BaseStruct* PtStruct;
    COTATION*       Cotation;
    TEXTE_PCB*      pt_txt;
    wxPoint         ref_pos;
    int             ux0, uy0;

    ref_pos = RefPos( typeloc );

    PtStruct = Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->m_StructType != TYPECOTATION )
            continue;
        Cotation = (COTATION*) PtStruct;
        if( (Cotation->m_Layer != LayerSearch) && (LayerSearch != -1) )
            continue;

        /* Localisation du texte ? */
        pt_txt = Cotation->m_Text;
        if( pt_txt )
        {
            if( pt_txt->Locate( ref_pos ) )
                return PtStruct;
        }

        /* Localisation des SEGMENTS ?) */
        ux0 = Cotation->Barre_ox; uy0 = Cotation->Barre_oy;
        
        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx      = Cotation->Barre_fx - ux0; dy = Cotation->Barre_fy - uy0;
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        /* detection : */
        if( distance( Cotation->m_Width / 2 ) )
            return PtStruct;

        ux0 = Cotation->TraitG_ox; uy0 = Cotation->TraitG_oy;
        
        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx      = Cotation->TraitG_fx - ux0; dy = Cotation->TraitG_fy - uy0;
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        /* detection : */
        if( distance( Cotation->m_Width / 2 ) )
            return PtStruct;

        ux0 = Cotation->TraitD_ox; uy0 = Cotation->TraitD_oy;
        
        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx      = Cotation->TraitD_fx - ux0; dy = Cotation->TraitD_fy - uy0;
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        /* detection : */
        if( distance( Cotation->m_Width / 2 ) )
            return PtStruct;

        ux0 = Cotation->FlecheD1_ox; uy0 = Cotation->FlecheD1_oy;
        
        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx      = Cotation->FlecheD1_fx - ux0; dy = Cotation->FlecheD1_fy - uy0;
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        /* detection : */
        if( distance( Cotation->m_Width / 2 ) )
            return PtStruct;

        ux0 = Cotation->FlecheD2_ox; uy0 = Cotation->FlecheD2_oy;
        
        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx      = Cotation->FlecheD2_fx - ux0; dy = Cotation->FlecheD2_fy - uy0;
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        /* detection : */
        if( distance( Cotation->m_Width / 2 ) )
            return PtStruct;

        ux0 = Cotation->FlecheG1_ox; uy0 = Cotation->FlecheG1_oy;
        
        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx      = Cotation->FlecheG1_fx - ux0; dy = Cotation->FlecheG1_fy - uy0;
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        /* detection : */
        if( distance( Cotation->m_Width / 2 ) )
            return PtStruct;

        ux0 = Cotation->FlecheG2_ox; uy0 = Cotation->FlecheG2_oy;
        
        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx      = Cotation->FlecheG2_fx - ux0; dy = Cotation->FlecheG2_fy - uy0;
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        /* detection : */
        if( distance( Cotation->m_Width / 2 ) )
            return PtStruct;
    }

    return NULL;
}


/*************************************************************************/
DRAWSEGMENT* Locate_Segment_Pcb( BOARD* Pcb, int LayerSearch, int typeloc )
/*************************************************************************/

/* Localisation de segments de contour du type drawing
 *  Retourne:
 *      Pointeur sur DEBUT du segment localise
 *      NULL si rien trouve
 *  Le segment sur la couche active est détecté en priorite
 */
{
    EDA_BaseStruct* PtStruct;
    DRAWSEGMENT*    pts, * locate_segm = NULL;
    wxPoint         ref_pos;
    PCB_SCREEN*     screen = (PCB_SCREEN*) ActiveScreen;

    ref_pos = RefPos( typeloc );

    PtStruct = Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->m_StructType != TYPEDRAWSEGMENT )
            continue;
        pts = (DRAWSEGMENT*) PtStruct;
        if( (pts->m_Layer != LayerSearch) && (LayerSearch != -1) )
            continue;
        
        ux0 = pts->m_Start.x; uy0 = pts->m_Start.y;
        
        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx      = pts->m_End.x - ux0; dy = pts->m_End.y - uy0;
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        /* detection : */
        if( (pts->m_Shape == S_CIRCLE) || (pts->m_Shape == S_ARC) )
        {
            int rayon, dist, StAngle, EndAngle, MouseAngle;

            rayon = (int) hypot( (double) (dx), (double) (dy) );
            dist  = (int) hypot( (double) (spot_cX), (double) (spot_cY) );
            
            if( abs( rayon - dist ) <= (pts->m_Width / 2) )
            {
                if( pts->m_Shape == S_CIRCLE )
                {
                    if( pts->m_Layer == screen->m_Active_Layer )
                        return pts;
                    else if( !locate_segm )
                        locate_segm = pts;
                }

                /* pour un arc, controle complementaire */
                MouseAngle = (int) ArcTangente( spot_cY, spot_cX );
                StAngle    = (int) ArcTangente( dy, dx );
                EndAngle   = StAngle + pts->m_Angle;

                if( EndAngle > 3600 )
                {
                    StAngle -= 3600; EndAngle -= 3600;
                }
                if( (MouseAngle >= StAngle) && (MouseAngle <= EndAngle) )
                {
                    if( pts->m_Layer == screen->m_Active_Layer )
                        return pts;
                    else if( !locate_segm )
                        locate_segm = pts;
                }
            }
        }
        else
        {
            if( distance( pts->m_Width / 2 ) )
            {
                if( pts->m_Layer == screen->m_Active_Layer )
                    return pts;
                else if( !locate_segm )
                    locate_segm = pts;
            }
        }
    }

    return locate_segm;
}


/*************************************************/
/*      D_PAD * Locate_Any_Pad(int typeloc, bool OnlyCurrentLayer)  */
/* D_PAD* Locate_Any_Pad(int ref_pos, bool OnlyCurrentLayer) */
/*************************************************/

/*
 *  localisation de la pastille pointee par la coordonnee ref_pos.x,,ref_pos.y, ou
 *  par la souris,  recherche faite sur toutes les empreintes.
 *  entree :
 *      - coord souris
 *       ou ref_pos
 *  retourne:
 *       pointeur sur la description de la pastille si localisation
 *       pointeur NULL si pastille non trouvee
 *       num_empr = numero d'empreinte du pad
 * 
 *  la priorité est donnée a la couche active
 */

D_PAD* Locate_Any_Pad( BOARD* Pcb, int typeloc, bool OnlyCurrentLayer )
{
    wxPoint ref_pos = RefPos( typeloc );
    return Locate_Any_Pad( Pcb, ref_pos, OnlyCurrentLayer );
}


D_PAD* Locate_Any_Pad( BOARD* Pcb, const wxPoint& ref_pos, bool OnlyCurrentLayer )
{
    D_PAD*  pt_pad;
    MODULE* module;
    int     layer_mask = g_TabOneLayerMask[ ( (PCB_SCREEN*) ActiveScreen )->m_Active_Layer];

    module = Pcb->m_Modules;
    for( ; module != NULL; module = (MODULE*) module->Pnext )
    {
        /* First: Search a pad on the active layer: */
        if( ( pt_pad = Locate_Pads( module, ref_pos, layer_mask ) ) != NULL )
            return pt_pad;

        /* If not found, search on other layers: */
        if( !OnlyCurrentLayer )
        {
            if( ( pt_pad = Locate_Pads( module, ref_pos, ALL_LAYERS ) ) != NULL )
                return pt_pad;
        }
    }

    return NULL;
}


/******************************************************************************/
/* D_PAD* Locate_Pads(MODULE * module, int masque_layer,int typeloc)          */
/* D_PAD* Locate_Pads(MODULE * module, const wxPoint & ref_pos,int masque_layer) */
/******************************************************************************/

/* localisation de la pastille pointee par la coordonnee ref_pos.x,,ref_pos.y, ou
 *  par la souris,  concernant l'empreinte  en cours.
 *  entree :
 *      - parametres generaux de l'empreinte mise a jour par caract()
 *      - masque_layer = couche(s) (bit_masque)sur laquelle doit etre la pastille
 *  retourne:
 *       un pointeur sur la description de la pastille si localisation
 *       pointeur NULL si pastille non trouvee
 */

D_PAD* Locate_Pads( MODULE* module, int masque_layer, int typeloc )
{
    wxPoint ref_pos = RefPos( typeloc );
    return Locate_Pads( module, ref_pos, masque_layer );
}


D_PAD* Locate_Pads( MODULE* module, const wxPoint& ref_pos, int masque_layer )
{
    D_PAD*  pt_pad;
    int     deltaX, deltaY;
    wxPoint shape_pos;
    double  dist;

    pt_pad = module->m_Pads;
    for( ; pt_pad != NULL; pt_pad = (D_PAD*) pt_pad->Pnext )
    {
        shape_pos = pt_pad->ReturnShapePos();
        ux0 = shape_pos.x; uy0 = shape_pos.y; /* pos x,y du centre du pad */

        deltaX = ref_pos.x - ux0; deltaY = ref_pos.y - uy0;

        /* Test rapide: le point a tester doit etre a l'interieur du cercle
         *  exinscrit ... */
        if( (abs( deltaX ) > pt_pad->m_Rayon )
           || (abs( deltaY ) > pt_pad->m_Rayon) )
            continue;

        /* ... et sur la bonne couche */
        if( (pt_pad->m_Masque_Layer & masque_layer) == 0 )
            continue;

        /* calcul des demi dim  dx et dy */
        dx = pt_pad->m_Size.x >> 1; // dx also is the radius for rounded pads
        dy = pt_pad->m_Size.y >> 1;

        /* localisation ? */
        switch( pt_pad->m_PadShape & 0x7F )
        {
        case CIRCLE:
            dist = hypot( deltaX, deltaY );
            if( (int) ( round( dist ) ) <= dx )
                return pt_pad;
            break;

        default:
            /* calcul des coord du point test  dans le repere du Pad */
            RotatePoint( &deltaX, &deltaY, -pt_pad->m_Orient );
            if( (abs( deltaX ) <= dx ) && (abs( deltaY ) <= dy) )
                return pt_pad;
            break;
        }
    }

    return NULL;
}


/********************************************************/
MODULE* Locate_Prefered_Module( BOARD* Pcb, int typeloc )
/********************************************************/

/*
 *  localisation d'une empreinte par son rectangle d'encadrement
 *  Si plusieurs empreintes sont possibles, la priorite est:
 *      - sur la couche active
 *      - la plus petite
 */
{
    MODULE* pt_module;
    int     lx, ly;                         /* dimensions du rectangle d'encadrement du module */
    MODULE* module      = NULL;             /* module localise sur la couche active */
    MODULE* Altmodule   = NULL;             /* module localise sur les couches non actives */
    int     min_dim     = 0x7FFFFFFF;       /* dim mini du module localise sur la couche active */
    int     alt_min_dim = 0x7FFFFFFF;       /* dim mini du module localise sur les couches non actives */
    int     layer;                          /* pour calcul de couches prioritaires */
    wxPoint ref_pos;                        /* coord du point de reference pour la localisation */

    ref_pos = RefPos( typeloc );

    pt_module = Pcb->m_Modules;
    for(  ;  pt_module;  pt_module = (MODULE*) pt_module->Pnext )
    {
        /* calcul des dimensions du cadre :*/
        lx = pt_module->m_BoundaryBox.GetWidth();
        ly = pt_module->m_BoundaryBox.GetHeight();

        /* Calcul des coord souris dans le repere module */
        spot_cX = ref_pos.x - pt_module->m_Pos.x;
        spot_cY = ref_pos.y - pt_module->m_Pos.y;
        RotatePoint( &spot_cX, &spot_cY, -pt_module->m_Orient );

        /* la souris est-elle dans ce rectangle : */
        if( !pt_module->m_BoundaryBox.Inside( spot_cX, spot_cY ) )
            continue;

        // if caller wants to ignore locked modules, and this one is locked, skip it.
        if( (typeloc & IGNORE_LOCKED) && pt_module->IsLocked() )
            continue;

        /* Localisation: test des dimensions minimales, choix du meilleur candidat */

        /* calcul de priorite: la priorite est donnee a la couche
         *  d'appartenance du module et a la couche cuivre si le module
         *  est sur couche serigr,adhesive cuivre, a la couche cmp si le module
         *  est sur couche serigr,adhesive composant */
        layer = pt_module->m_Layer;

        if( layer==ADHESIVE_N_CU || layer==SILKSCREEN_N_CU )
            layer = CUIVRE_N;

        else if( layer==ADHESIVE_N_CMP || layer==SILKSCREEN_N_CMP )
            layer = CMP_N;

        if( ( (PCB_SCREEN*) ActiveScreen )->m_Active_Layer == layer )
        {
            if( min( lx, ly ) <= min_dim )
            {
                /* meilleure empreinte localisee sur couche active */
                module = pt_module; min_dim = min( lx, ly );
            }
        }
        else if( !(typeloc & MATCH_LAYER)
            && ( !(typeloc & VISIBLE_ONLY) || IsModuleLayerVisible( layer ) ) )
        {
            if( min( lx, ly ) <= alt_min_dim )
            {
                /* meilleure empreinte localisee sur autres couches */
                Altmodule   = pt_module;
                alt_min_dim = min( lx, ly );
            }
        }
    }

    if( module )
    {
        return module;
    }

    if( Altmodule )
    {
        return Altmodule;
    }

    return NULL;
}


/*****************************************************************************/
TEXTE_MODULE* LocateTexteModule( BOARD* Pcb, MODULE** PtModule, int typeloc )
/*****************************************************************************/

/* localisation du texte pointe par la souris (texte sur empreinte)
 * 
 *  si * PtModule == NULL; recherche sur tous les modules
 *  sinon sur le module pointe par module
 * 
 *  retourne
 *  - pointeur sur le texte localise ( ou NULL )
 *  - si Ptmodule != NULL: pointeur sur module module ( non modifie sinon )
 * 
 *  if typeloc has the flag VISIBLE_ONLY set, only footprints which are 
 * "visible" are considered
 */
{
    EDA_BaseStruct* PtStruct;
    TEXTE_MODULE*   pt_txt_mod;
    MODULE*         module;
    wxPoint         ref_pos;

    ref_pos = RefPos( typeloc );

    module = *PtModule;
    if( module == NULL )
    {
        module = Pcb->m_Modules;
    }

    for( ; module != NULL; module = (MODULE*) module->Pnext )
    {
        int layer = module->m_Layer;
        if( layer==ADHESIVE_N_CU || layer==SILKSCREEN_N_CU )
            layer = CUIVRE_N;
        else if( layer==ADHESIVE_N_CMP || layer==SILKSCREEN_N_CMP )
            layer = CMP_N;

        if( typeloc & VISIBLE_ONLY )
        {
            if( !IsModuleLayerVisible( layer ) )
                continue;
        }

        if( typeloc & MATCH_LAYER )
        {
            if( ( (PCB_SCREEN*) ActiveScreen )->m_Active_Layer != layer )
                continue;
        }

        // hit-test the reference text
        pt_txt_mod = module->m_Reference;
        if( pt_txt_mod->Locate( ref_pos ) )
        {
            if( PtModule )
                *PtModule = module;
            return pt_txt_mod;
        }

        // hit-test the value text
        pt_txt_mod = module->m_Value;
        if( pt_txt_mod->Locate( ref_pos ) )
        {
            if( PtModule )
                *PtModule = module;
            return pt_txt_mod;
        }

        // hit-test any other texts
        PtStruct = module->m_Drawings;
        for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
        {
            if( PtStruct->m_StructType != TYPETEXTEMODULE )
                continue;
            
            pt_txt_mod = (TEXTE_MODULE*) PtStruct;
            if( pt_txt_mod->Locate( ref_pos ) )
            {
                if( PtModule )
                    *PtModule = module;
                return pt_txt_mod;
            }
        }

        if( *PtModule != NULL )
            break;                     /* Recherche limitee a 1 seul module */
    }

    return NULL;
}


/**************************************************************/
TRACK* Locate_Piste_Connectee( TRACK* PtRefSegm, TRACK* pt_base,
                               TRACK* pt_lim, int extr )
/**************************************************************/

/* recherche le segment connecte au segment pointe par
 *  PtRefSegm:
 *  si int extr = START, le point de debut du segment est utilise
 *  si int extr = END, le point de fin du segment est utilise
 *  La recherche ne se fait que sur les EXTREMITES des segments
 * 
 *  La recherche se fait de l'adresse :
 *      pt_base a pt_lim (borne finale comprise)
 *      si pt_lim = NULL, la recherche se fait jusqu'a la fin de la liste
 *  Afin d'accelerer la recherche, une 1ere passe est faite, avec une recherche
 *  realisee sur un ensemble de +/- 100 points autour du point courant.
 *  Si echec: recherche generale
 */
{
    TRACK*  PtSegmB, * PtSegmN;
    int     Reflayer;
    wxPoint pos_ref;
    int     ii;

    if( extr == START )
        pos_ref = PtRefSegm->m_Start;
    else
        pos_ref = PtRefSegm->m_End;

    Reflayer = PtRefSegm->ReturnMaskLayer();

    /* 1ere passe */
    PtSegmB = PtSegmN = PtRefSegm;

    for( ii = 0; ii < 50; ii++ )
    {
        if( (PtSegmN == NULL) && (PtSegmB == NULL) )
            break;
        if( PtSegmN )
        {
            if( PtSegmN->GetState( BUSY | DELETED ) )
                goto suite;
            if( PtSegmN == PtRefSegm )
                goto suite;
            if( pos_ref == PtSegmN->m_Start )
            {       /* Test des couches */
                if( Reflayer & PtSegmN->ReturnMaskLayer() )
                    return PtSegmN;
            }

            if( pos_ref == PtSegmN->m_End )
            {       /* Test des couches */
                if( Reflayer & PtSegmN->ReturnMaskLayer() )
                    return PtSegmN;
            }
suite:
            if( PtSegmN == pt_lim )
                PtSegmN = NULL;
            else
                PtSegmN = (TRACK*) PtSegmN->Pnext;
        }

        if( PtSegmB )
        {
            if( PtSegmB->GetState( BUSY | DELETED ) )
                goto suite1;
            if( PtSegmB == PtRefSegm )
                goto suite1;
            if( pos_ref == PtSegmB->m_Start )
            {       /* Test des couches */
                if( Reflayer & PtSegmB->ReturnMaskLayer() )
                    return PtSegmB;
            }

            if( pos_ref == PtSegmB->m_End )
            {       /* Test des couches */
                if( Reflayer & PtSegmB->ReturnMaskLayer() )
                    return PtSegmB;
            }
suite1:
            if( PtSegmB == pt_base )
                PtSegmB = NULL;
            else if( PtSegmB->m_StructType != TYPEPCB )
                PtSegmB = (TRACK*) PtSegmB->Pback;
            else
                PtSegmB = NULL;
        }
    }

    /* Recherche generale */
    for( PtSegmN = pt_base; PtSegmN != NULL; PtSegmN = (TRACK*) PtSegmN->Pnext )
    {
        if( PtSegmN->GetState( DELETED | BUSY ) )
        {
            if( PtSegmN == pt_lim )
                break;
            continue;
        }
        if( PtSegmN == PtRefSegm )
        {
            if( PtSegmN == pt_lim )
                break;
            continue;
        }

        if( pos_ref == PtSegmN->m_Start )
        {       /* Test des couches */
            if( Reflayer & PtSegmN->ReturnMaskLayer() )
                return PtSegmN;
        }

        if( pos_ref == PtSegmN->m_End )
        {       /* Test des couches */
            if( Reflayer & PtSegmN->ReturnMaskLayer() )
                return PtSegmN;
        }
        if( PtSegmN == pt_lim )
            break;
    }

    return NULL;
}


/****************************************************************************/
/* TRACK *Locate_Pistes(TRACK * start_adresse, int MasqueLayer,int typeloc) */
/* TRACK *Locate_Pistes(TRACK * start_adresse, int ref_pos.x, int ref_pos.y,*/
/*                                      int MasqueLayer)                    */
/****************************************************************************/

/*
 *  1 -  routine de localisation du segment de piste pointe par la souris.
 *  2 -  routine de localisation du segment de piste pointe par le point
 *          ref_pos.x , ref_pos.y.r
 * 
 *  La recherche commence a l'adresse start_adresse
 */

TRACK* Locate_Pistes( TRACK* start_adresse, int MasqueLayer, int typeloc )
{
    wxPoint ref_pos = RefPos( typeloc );

    return Locate_Pistes( start_adresse, ref_pos, MasqueLayer );
}


TRACK* Locate_Pistes( TRACK* start_adresse, const wxPoint& ref_pos, int MasqueLayer )
{
    TRACK* Track;               /* pointeur sur les pistes */
    int    l_piste;             /* demi-largeur de la piste */

    for( Track = start_adresse; Track != NULL; Track = (TRACK*) Track->Pnext )
    {
        if( Track->GetState( BUSY | DELETED ) )
            continue;
        if( (g_DesignSettings.m_LayerColor[Track->m_Layer] & ITEM_NOT_SHOW) )
            continue;
        /* calcul des coordonnees du segment teste */
        l_piste = Track->m_Width >> 1;                  /* l_piste = demi largeur piste */
        ux0 = Track->m_Start.x; uy0 = Track->m_Start.y; /* coord de depart */
        dx  = Track->m_End.x; dy = Track->m_End.y;      /* coord d'arrivee */

        /* recalcul des coordonnees avec ux0, uy0 = origine des coordonnees */
        dx     -= ux0; dy -= uy0;
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        if( Track->m_StructType == TYPEVIA ) /* VIA rencontree */
        {
            if( (abs( spot_cX ) <= l_piste ) && (abs( spot_cY ) <=l_piste) )
            {
                return Track;
            }
            continue;
        }

        if( MasqueLayer != -1 )
            if( (g_TabOneLayerMask[Track->m_Layer] & MasqueLayer) == 0 )
                continue;/* Segments sur couches differentes */
        if( distance( l_piste ) )
            return Track;
    }

    return NULL;
}


/****************************************************************/
/* TRACK *  Locate_Zone(TRACK * start_adresse, int layer,    */
/*                                          int typeloc)        */
/* TRACK *  Locate_Zone(TRACK * start_adresse,              */
/*                                      const wxPoint & ref_pos, */
/*                                      int layer)              */
/****************************************************************/

/*
 *  1 -  routine de localisation du segment de zone pointe par la souris.
 *  2 -  routine de localisation du segment de zone pointe par le point
 *          ref_pos.x , ref_pos.y.r
 * 
 *  Si layer == -1 , le tst de la couche n'est pas fait
 * 
 *  La recherche commence a l'adresse start_adresse
 */

TRACK* Locate_Zone( TRACK* start_adresse, int layer, int typeloc )
{
    wxPoint ref_pos = RefPos( typeloc );

    return Locate_Zone( start_adresse, ref_pos, layer );
}


TRACK* Locate_Zone( TRACK* start_adresse, const wxPoint& ref_pos, int layer )
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
        spot_cX = ref_pos.x - ux0; spot_cY = ref_pos.y - uy0;

        if( (layer != -1) && (Zone->m_Layer != layer) )
            continue;
        if( distance( l_segm ) )
            return Zone;
    }

    return NULL;
}


/************************************************************************************/
TEXTE_PCB* Locate_Texte_Pcb( EDA_BaseStruct* PtStruct, int LayerSearch, int typeloc )
/************************************************************************************/

/* localisation des inscriptions sur le Pcb:
 *  entree : EDA_BaseStruct pointeur sur le debut de la zone de recherche
 *  retour : pointeur sur la description du texte localise
 */
{
    wxPoint ref = RefPos( typeloc );

    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->m_StructType != TYPETEXTE )
            continue;
        TEXTE_PCB* pt_txt_pcb = (TEXTE_PCB*) PtStruct;
        if( pt_txt_pcb->Locate( ref ) )
        {
            if( pt_txt_pcb->m_Layer == LayerSearch )
                return pt_txt_pcb;
        }
    }

    return NULL;
}


/*****************************/
int distance( int seuil )
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
{
    int cXrot, cYrot,   /* coord du point (souris) dans le repere tourne */
        segX, segY;     /* coord extremite segment tj >= 0 */
    int pointX, pointY; /* coord point a tester dans repere modifie dans lequel
                         * segX et segY sont >=0 */

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
         * de piste soit horizontal dans le nouveau repere */
        int angle;

        angle = (int) ( atan2( (float) segY, (float) segX ) * 1800 / M_PI);
        cXrot = pointX; cYrot = pointY;
        RotatePoint( &cXrot, &cYrot, angle );   /* Rotation du point a tester */
        RotatePoint( &segX, &segY, angle );     /* Rotation du segment */

        /* la piste est Horizontale , par suite des modifs de coordonnes
         * et d'axe, donc segX = longueur du segment */

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


/*******************************************************************************/
D_PAD* Fast_Locate_Pad_Connecte( BOARD* Pcb, const wxPoint& ref_pos, int masque_layer )
/*******************************************************************************/

/* Routine cherchant le pad de centre px,py,
 *  sur la couche indiquee par masque_layer (bit a bit)
 *  ( extremite de piste )
 *  La liste des pads doit deja exister.
 * 
 *  retourne :
 *      NULL si pas de pad localise.
 *      pointeur sur la structure descr_pad correspondante si pad trouve
 *      (bonne position ET bonne couche).
 */
{
    D_PAD*     pad;
    LISTE_PAD* ptr_pad, * lim;

    lim = (LISTE_PAD*) Pcb->m_Pads + Pcb->m_NbPads;
    for( ptr_pad = (LISTE_PAD*) Pcb->m_Pads; ptr_pad < lim; ptr_pad++ )
    {
        pad = *ptr_pad;
        if( pad->m_Pos != ref_pos )
            continue;

        /* Pad peut-etre trouve ici : il doit etre sur la bonne couche */
        if( pad->m_Masque_Layer & masque_layer )
            return pad;
    }

    return NULL;
}


/***********************************************************************************/
TRACK* Fast_Locate_Piste( TRACK* start_adr, TRACK* end_adr,
                          const wxPoint& ref_pos, int MaskLayer )
/***********************************************************************************/

/* Localiste le segment dont une extremite coincide avec le point x,y
 *  sur les couches donnees par masklayer
 *  la recherche se fait de l'adresse start_adr a end_adr
 *  si end_adr = NULL, recherche jusqu'a la fin de la liste
 *  Les segments de piste marques avec le flag DELETED ne sont pas
 *  pris en compte
 */
{
    TRACK* PtSegm;

    if( start_adr == NULL )
        return NULL;

    for( PtSegm = start_adr; PtSegm != NULL; PtSegm = (TRACK*) PtSegm->Pnext )
    {
        if( PtSegm->GetState( DELETED | BUSY ) == 0 )
        {
            if( ref_pos == PtSegm->m_Start )
            {       
                /* Test des couches */
                if( MaskLayer & PtSegm->ReturnMaskLayer() )
                    return PtSegm;
            }

            if( ref_pos == PtSegm->m_End )
            {       
                /* Test des couches */
                if( MaskLayer & PtSegm->ReturnMaskLayer() )
                    return PtSegm;
            }
        }
        if( PtSegm == end_adr )
            break;
    }

    return NULL;
}


/*******************************************************************/
TRACK* Fast_Locate_Via( TRACK* start_adr, TRACK* end_adr,
                        const wxPoint& pos, int MaskLayer )
/*******************************************************************/

/* Localise la via de centre le point x,y , sur les couches donnees
 *  par masklayer
 *  la recherche se fait de l'adresse start_adr a end_adr.
 *  si end_adr = NULL, recherche jusqu'a la fin de la liste
 *  les vias dont le parametre State a le bit DELETED ou BUSY = 1 sont ignorees
 */
{
    TRACK* PtSegm;

    for( PtSegm = start_adr; PtSegm != NULL; PtSegm = (TRACK*) PtSegm->Pnext )
    {
        if( PtSegm->m_StructType == TYPEVIA )
        {
            if( pos == PtSegm->m_Start )
            {
                if( PtSegm->GetState( BUSY | DELETED ) == 0 )
                {    
                    /* Test des couches */
                    if( MaskLayer & PtSegm->ReturnMaskLayer() )
                        return PtSegm;
                }
            }
        }
        if( PtSegm == end_adr )
            break;
    }

    return NULL;
}


/***********************************************************************/
EDA_BaseStruct* Locate_MirePcb( EDA_BaseStruct* PtStruct, int LayerSearch,
                                int typeloc )
/***********************************************************************/

/* Serach for a photo target
 */
{
    wxPoint ref_pos;/* coord du point de localisation */
    int     dX, dY, rayon;

    if( PtStruct == NULL )
        return NULL;

    ref_pos = RefPos( typeloc );

    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        MIREPCB* item;
        if( PtStruct->m_StructType != TYPEMIRE )
            continue;

        item = (MIREPCB*) PtStruct;
        if( LayerSearch != -1 && item->m_Layer != LayerSearch )
            continue;

        dX    = ref_pos.x - item->m_Pos.x;
        dY    = ref_pos.y - item->m_Pos.y;
        rayon = item->m_Size / 2;

        if( (abs( dX ) <= rayon ) && ( abs( dY ) <= rayon ) )
            break;  /* Mire Localisee */
    }

    return PtStruct;
}
