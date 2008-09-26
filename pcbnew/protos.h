/***************************************/
/* prototypage des fonctions de PCBNEW */
/***************************************/

#ifndef PROTO_H
#define PROTO_H


#include <vector>


/* install function for DialogNonCopperZonesEditor dialog frame :*/
bool InstallDialogNonCopperZonesEditor(WinEDA_PcbFrame* aParent, ZONE_CONTAINER* aZone);

/***************/
/* PAD_CONNECT.CPP */
/***************/

class D_PAD;

/**
 * Function CreateSortedPadListByXCoord
 * first empties then fills the vector with all pads and sorts them by
 * increasing x coordinate.  The vector only holds pointers to the pads and
 * those pointers are only references to pads which are owned by the BOARD
 * through other links.
 * @param aBoard Which board to gather pads from.
 * @param aVector Where to put the pad pointers.
 */
void CreateSortedPadListByXCoord( BOARD* aBoard, std::vector<D_PAD*>* aVector );


/* Create a sorted list of pointers to pads.
 * This list is sorted by X ccordinate value.
 * The list must be freed bu user
 */

/**************/
/* PCBCFG.CPP */
/**************/
bool    Read_Config( const wxString& project_name );
bool    Read_Hotkey_Config( WinEDA_DrawFrame* frame, bool verbose );

/***************/
/* TRACEPCB.CPP */
/***************/
void    Trace_MirePcb( WinEDA_DrawPanel* panel, wxDC* DC, MIREPCB* MirePcb, int mode_color );

/***************/
/* TRPISTE.CPP */
/***************/

void    Trace_Pistes( WinEDA_DrawPanel* panel, BOARD* Pcb, wxDC* DC, int drawmode );
void    Trace_Une_Piste( WinEDA_DrawPanel* panel,
                         wxDC* DC,
                         TRACK* pt_start_piste,
                         int nbsegment,
                         int mode_color );

/* routine de trace de n segments consecutifs en memoire.
 * Utile pour monter une piste en cours de trace car les segments de cette
 *  piste sont alors contigus en memoire
 * Parametres :
 * pt_start_piste = adresse de depart de la liste des segments
 *  nbsegment = nombre de segments a tracer
 * mode_color = mode ( GrXOR, GrOR..)
 * ATTENTION:
 * le point de depart d'une piste suivante DOIT exister: peut etre
 *  donc mis a 0 avant appel a la routine si la piste a tracer est la derniere
 */


/****************/
/* TRACEMOD.C : */
/****************/
void Trace_Pads_Only( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* Module, int ox, int oy,
                      int MasqueLayer, int mode_color );

/* Trace les pads d'un module en mode SKETCH.
  * Utilisee pour afficher les pastilles d'un module lorsque celui ci n'est
  * pas affiche par les options d'affichage des Modules
  * Les pads affiches doivent apparaitre sur les couches donnees par
  * MasqueLayer */

/****************/
/* LOCATE.CPP : */
/****************/

/* Recherche d'une empreinte par son nom */

/* Recherche d'un pad par son nom, pour le module Module */

TRACK*          Locate_Via( BOARD* Pcb, const wxPoint& pos, int layer = -1 );

/**
 * Function Locate_Via_Area
 * finds the first SEGVIA which covers the given aPos with a matching layer.
 * @param aStart The starting TRACK or SEGVIA in the BOARD's list.
 * @param aPos The wxPoint to HitTest() against.
 * @param aLayer The layer to match, pass -1 for a don't care.
 * @return TRACK* - actually a SEGVIA* if found, else NULL.
 */
TRACK*          Locate_Via_Area( TRACK* aStart, const wxPoint& aPos, int aLayer = -1 );

TRACK*          Fast_Locate_Via( TRACK* start_adr, TRACK* end_adr,
                                 const wxPoint& pos, int masquelayer );

/* Localise la via de centre le point x,y , sur les couches donnees
  *  par masquelayer.
  *  la recherche se fait de l'adresse start_adr a end_adr(non comprise)
 */

TRACK*          Fast_Locate_Piste( TRACK* start_adr, TRACK* end_adr,
                                   const wxPoint& ref_pos, int masquelayer );

/* Localiste le segment dont une extremite coincide avec le point x,y
  *  sur les couches donnees par masquelayer
  *  la recherche se fait de l'adresse start_adr a end_adr(non comprise)
 */

TRACK*          Locate_Piste_Connectee( TRACK* ptr_piste, TRACK* pt_base,
                                        TRACK* pt_lim, int extr );

/* recherche le segment connecte au segment pointe par
 * ptr_piste:
 * si int = START, le point de debut du segment est utilise
 *   si int = END, le point de fin du segment est utilise
 *   La recherche ne se fait que sur les EXTREMITES des segments
 * La recherche est limitee a la zone [pt_base...]pt_lim.
 */

TRACK*          Locate_Pistes( TRACK* start_adresse, int layer, int typeloc );
TRACK*          Locate_Pistes( TRACK* start_adresse,
                               const wxPoint& ref_pos, int layer );

/*
  * 1 -  routine de localisation du segment de piste pointe par la souris.
  * 2 -  routine de localisation du segment de piste pointe par le point
  *     ref_pX , ref_pY.
 *
  * Si layer < 0 la couche n'est pas testee.
 *
  * La recherche commence a l'adresse start_adresse
 */

D_PAD*          Locate_Pad_Connecte( BOARD* Pcb, TRACK* ptr_segment, int extr );

/* localisation de la pastille connectee au debut ou fin du segment
  * entree : pointeur sur le segment, et flag = START ou END
  * retourne:
  *      un pointeur sur la description de la pastille si localisation
  *       pointeur NULL si pastille non trouvee */

D_PAD*          Locate_Any_Pad( BOARD* Pcb, int typeloc, bool OnlyCurrentLayer = FALSE );
D_PAD*          Locate_Any_Pad( BOARD* Pcb, const wxPoint& ref_pos, bool OnlyCurrentLayer = FALSE );

/*
  * localisation de la pastille pointee par la coordonnee ref_pX,,ref_pY, ou
  * par la souris,  recherche faite sur toutes les empreintes.
  *  entree : (OVERHAEAD)
  *      - coord souris (Curseur_X et Curseur_Y)
  *       ou ref_pX, ref_pY
  *  retourne:
  *       pointeur sur la description de la pastille si localisation
  *       pointeur NULL si pastille non trouvee
 */

D_PAD*          Locate_Pads( MODULE* Module, int layer, int typeloc );
D_PAD*          Locate_Pads( MODULE* Module, const wxPoint& ref_pos, int layer );

/* localisation de la pastille pointee par la coordonnee ref_pX,,ref_pY, ou
  * par la souris,  concernant l'empreinte  en cours.
  * entree :
  *      - parametres generaux de l'empreinte mise a jous par caract()
  *      - layer = couche ou doit se trouver la pastille
  *         (si layer < 0 ) la couche est ignoree)
  *  retourne:
  *       un pointeur sur la description de la pastille si localisation
  *       pointeur NULL si pastille non trouvee
 */

MODULE*         Locate_Prefered_Module( BOARD* Pcb, int typeloc );

/* localisation d'une empreinte par son rectangle d'encadrement */

D_PAD*          Locate_Pads( MODULE* Module, int typeloc );

/* localisation de la pastille pointee par la souris, concernant l'empreinte
  * Module.
  *  entree :
  *      - parametres generaux de l'empreinte mise a jous par caract()
  *  retourne:
  *       un pointeur sur la description de la pastille si localisation
  *       pointeur NULL si pastille non trouvee
 */

TRACK*          Locate_Pistes( TRACK* start_adresse, int typeloc );

/* routine de localisation du segment de piste pointe par la souris
  * La recherche commence a l'adresse start_adresse */

DRAWSEGMENT*    Locate_Segment_Pcb( BOARD* Pcb, int LayerSearch, int typeloc );


D_PAD*          Fast_Locate_Pad_Connecte( BOARD* Pcb, const wxPoint& ref_pos, int layer );

/* Routine cherchant le pad contenant le point px,py, sur la couche layer
  * ( extremite de piste )
  *  La liste des pads doit deja exister.
 *
  * retourne :
  *     NULL si pas de pad localise.
  *      pointeur sur le pad correspondante si pad trouve
  *      (bonne position ET bonne couche). */


TRACK*          Locate_Zone( TRACK* start_adresse, int layer, int typeloc );
TRACK*          Locate_Zone( TRACK* start_adresse, const wxPoint& ref_pos, int layer );

/*
  * 1 -  routine de localisation du segment de zone pointe par la souris.
  * 2 -  routine de localisation du segment de zone pointe par le point
  *     ref_pX , ref_pY.r
 *
  * Si layer == -1 , le tst de la couche n'est pas fait
 *
  * La recherche commence a l'adresse start_adresse
 */


/*************/
/* MODULES.C */
/*************/
int     ChangeSideNumLayer( int oldlayer );
void    DrawModuleOutlines( WinEDA_DrawPanel* panel, wxDC* DC, MODULE* module );
void    Montre_Position_Empreinte( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


/* LOADCMP.C : */
MODULE* Load_Module_From_Library( WinEDA_DrawFrame* frame, wxDC* DC );

/****************/
/* EDITRACK.C : */
/****************/

TRACK*  LocateIntrusion( TRACK* start, int net, int width );

void    ShowNewTrackWhenMovingCursor( WinEDA_DrawPanel* panel,
                                      wxDC* DC, bool erase );

void    Calcule_Coord_Extremite_45( int ox, int oy, int* fx, int* fy );

/* determine les coord fx et fy d'un segment
  * pour avoir un segment oriente a 0, 90 ou 45 degres, selon position
  * du point d'origine (ox,oy) et de la souris */

/*****************/
/* TRACK.CPP : */
/*****************/
TRACK*  Marque_Une_Piste( WinEDA_BasePcbFrame* frame, wxDC* DC,
                          TRACK* pt_segm, int* nb_segm, int flagcolor );

/* Routine de Marquage de 1 piste, a partir du segment pointe par pt_segm.
  * le segment pointe est marque puis les segments adjacents
  * jusqu'a un pad ou un point de jonction de plus de 2 segments
  * le marquage est la mise a 1 du bit BUSY du parametre .status
  * Les segments sont ensuite reclasses pour etre contigus en memoire
  * Retourne:
  *     adresse du 1er segment de la chaine creee
  *     nombre de segments */

int     ReturnEndsTrack( TRACK* RefTrack, int NbSegm,
                         TRACK** StartTrack, TRACK** EndTrack );

/* Calcule les coordonnes des extremites d'une piste
  *  retourne 1 si OK, 0 si piste bouclee
  *  Les coord sont retournees en StartTrack->ox, oy
  *                             et EndTrack->fx, fy si OK
  *  Les segments sont supposes chaines de facon consecutive */

void    ListSetState( EDA_BaseStruct* Start, int Nbitem, int State, int onoff );

/* Met a jour le membre .state d'une chaine de structures */

/*****************/
/* EDITEDGE.CPP : */
/*****************/
void    Trace_1_Edge( WinEDA_DrawPanel* panel,
                      wxDC* DC,
                      TRACK* start_edge,
                      int nbpoint,
                      int mode_color );

/************/
/* DRC.CPP : */
/************/
int     Drc( WinEDA_BasePcbFrame* frame, wxDC* DC,
             TRACK* pt_segment, TRACK* pt_start_buffer, int show_err );

/* Teste le segment pointe par pt_segment:
  *     debsegment = adresse du segment a tester
  *     pt_start_buffer = adresse de la zone piste
  *      show_err (flag) si 0 pas d'affichage d'erreur sur ecran
  *     retourne :
  *          BAD_DRC (1) si Violation DRC
  *          OK_DRC  (0) si OK  */


/*****************/
/* TR_MODIF.CPP : */
/*****************/

int     EraseOldTrack( WinEDA_BasePcbFrame* frame, BOARD* Pcb, wxDC* DC,
                       TRACK* pt_new_track, int nbptnewpiste );
void    Modif_Auto_Route( TRACK* pt_debut_new_piste );


/**************/
/* CLEAN.CPP : */
/**************/

int Netliste_Controle_piste( WinEDA_PcbFrame* frame, wxDC* DC, int affiche );

/* Supprime les segments mal connectes, cad interconnectant des segments
  * de net_code differents */


/************/
/* BLOCK.CPP */
/************/

void    Block_Affiche( int on_off ); /*
                                  * routine de trace du cadre d'un Block en cours de delimitation
                                  * Si on_off = 0 : effacement du cadre
                                  * Si on_off = 1 : affichage  du cadre */

void    Trace_Block( WinEDA_DrawPanel* panel, wxDC* DC, int ox, int oy, int fx, int fy, int color );

/* Routine de trace d'un rectangle symbolisant un block
  *   (toujours en mode XOR) */

/***************/
/* PLOT_RTN.CPP */
/***************/

void Affiche_erreur( int nb_err );


/*****************************************************************/
/* AFFICHE.CPP: (Fonctions d'affichage de messages, parametres... */
/*****************************************************************/
void    Affiche_Infos_Equipot( int netcode, WinEDA_BasePcbFrame* frame );

/************/
/* ZONES.CPP */
/************/
int     Propagation( WinEDA_PcbFrame* frame );

/***************/
/* ATTRIBUT.CPP */
/***************/

void MasqueAttributs( int* masque_set, int* masque_clr );

/* Calcule les attributs a remettre a 0 (masque_clr) et a mettre a 1
  *   (masque_set), en fonction des options d'attributs
 *
  *   ces attributs sont normalement le membre .flags de la structure TRACK
  *   les pointeurs NULLs sont acceptes
 */


/***************/
/* DUPLTRAC.CPP */
/***************/

EDA_BaseStruct* LocateLockPoint( BOARD* Pcb, wxPoint pos, int LayerMask );

/* Routine trouvant le point " d'accrochage " d'une extremite de piste.
 *  Ce point peut etre un PAD ou un autre segment de piste
 * Retourne:
 *      - pointeur sur ce PAD ou:
 *      - pointeur sur le segment ou:
 *      - NULL
 *  Parametres d'appel:
 *   coord pX, pY du point tst
 *   masque des couches a tester
 */


TRACK*          CreateLockPoint( int* pX, int* pY, TRACK* ptsegm, TRACK* refsegm );

/* Routine de creation d'un point intermediaire sur un segment
  * le segment ptsegm est casse en 2 segments se raccordant au point pX, pY
  *  retourne:
  *      NULL si pas de nouveau point ( c.a.d si pX, pY correspondait deja
  *     a une extremite ou:
  *      pointeur sur le segment cree
  * si refsegm != NULL refsegm est pointeur sur le segment incident,
  * et le point cree est l'ntersection des 2 axes des segments ptsegm et
  * refsegm
  * retourne la valeur exacte de pX et pY
 */

/****************/
/* CONTROLE.CPP */
/****************/
void RemoteCommand( const char* cmdline );
bool Project( wxPoint* res, wxPoint on_grid, const TRACK* track );


/***************/
/* AUTOPLACE.CPP */
/***************/
int     Calcule_Encadrement_EdgeBoard();

/***************/
/* AUTOROUT.CPP */
/***************/
void    DisplayBoard( WinEDA_DrawPanel* panel, wxDC* DC ); /* routine de Debug */


/**************/
/* NETLIST.CPP */
/**************/
MODULE* ListAndSelectModuleName( COMMAND* Cmd );

/* liste les noms des modules du PCB
  *   Retourne un pointeur sur le module selectionne
  *   ( ou NULL si pas de selection ) */

/***************/
/* LAY2PLOT.CPP */
/***************/

/*****************/
/* SET_COLOR.CPP */
/*****************/
void DisplayColorSetupFrame( WinEDA_PcbFrame* parent,
                             const wxPoint&    framepos );


#endif  /* #define PROTO_H */
