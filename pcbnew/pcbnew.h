	/**************************************/
	/*	PCBNEW.H :  déclarations communes */
	/**************************************/
#ifndef PCBNEW_H
#define PCBNEW_H

#ifndef eda_global
#define eda_global extern
#endif

#include "pcbstruct.h"
#include "macros.h"

#define U_PCB (PCB_INTERNAL_UNIT/EESCHEMA_INTERNAL_UNIT)

/* valeur de flag indicant si le pointeur de reference pour une localisation
est le curseur sur grille ou le curseur a deplacement fin hors grille */
#define CURSEUR_ON_GRILLE   (0<<0)  
#define CURSEUR_OFF_GRILLE  (1<<1)
#define IGNORE_LOCKED       (1<<2)  ///< if module is locked, do not select for single module operation  
#define MATCH_LAYER         (1<<3)  ///< if module not on current layer, do not select 
#define VISIBLE_ONLY        (1<<4)  ///< if module not on a visible layer, do not select


#define START 0		/* ctes parametre dans les routines de localisation */
#define END 1

#define DIM_ANCRE_MODULE 3  /* dim du symbole de l'ancre (centre) des modules */
#define DIM_ANCRE_TEXTE 2	/* dim du symbole de l'ancre (centre) des textes */

/* Gestion du Menu Zoom */
#define ZOOM_PLUS	-1
#define ZOOM_MOINS  -2
#define ZOOM_AUTO	-3
#define ZOOM_CENTER -4
#define ZOOM_REDRAW -5

/* Bits Flags utilisés en édition (membre .m_Flags de EDA_BaseStruct)*/
#define IS_LINKED 1
#define IN_EDIT 2
#define IS_MOVED 4
#define IS_NEW 8
#define IS_RESIZED 0x10
#define IS_DRAGGED 0x20
#define STARTPOINT 0x100
#define ENDPOINT 0x200
#define SELECTED 0x400

/* Definition des cas ou l'on force l'affichage en SKETCH (membre .flags) */
#define FORCE_SKETCH (DRAG | EDIT )

/* Constantes pour options lecture fichier PCB */
#define APPEND_PCB 1	/* pour ajout d'un nouveau circuit */
#define NEWPCB 0		/* pour chargement normal */


eda_global wxArrayString g_LibName_List;	// library list to load

eda_global wxSize g_GridList[]
#ifdef MAIN
 = {
	wxSize(1000,1000), wxSize(500,500), wxSize(250,250), wxSize(200,200),
	wxSize(100,100), wxSize(50,50), wxSize(25,25), wxSize(20,20),
	wxSize(10,10), wxSize(5,5), wxSize(2,2), wxSize(1,1),
	wxSize(-1,-1), wxSize(0,0)
}
#endif
;

#define UNDELETE_STACK_SIZE 10
eda_global EDA_BaseStruct * g_UnDeleteStack[UNDELETE_STACK_SIZE]; //Liste des elements supprimes
eda_global int  g_UnDeleteStackPtr;

eda_global bool g_ShowGrid
#ifdef MAIN
= TRUE
#endif
;

/* Look up Table for conversion one layer number -> one bit layer mask: */
eda_global int g_TabOneLayerMask[LAYER_COUNT]
#if defined MAIN
	= { 0x00000001,0x00000002,0x00000004,0x00000008,
		0x00000010,0x00000020,0x00000040,0x00000080,
		0x00000100,0x00000200,0x00000400,0x00000800,
		0x00001000,0x00002000,0x00004000,0x00008000,
		0x00010000,0x00020000,0x00040000,0x00080000,
		0x00100000,0x00200000,0x00400000,0x00800000,
		0x01000000,0x02000000,0x04000000,0x08000000,
		0x10000000,0x20000000,0x40000000,0x80000000
	}
#endif
	;

/* Look up Table for conversion copper layer count -> general copper layer mask: */
eda_global int g_TabAllCopperLayerMask[NB_COPPER_LAYERS]
#if defined MAIN
	= {
	0x0001, 0x8001, 0x8003, 0x8007,
	0x800F, 0x801F, 0x803F, 0x807F,
	0x80FF, 0x81FF, 0x83FF, 0x87FF,
	0x8FFF, 0x9FFF, 0xCFFF, 0xFFFF
};
#endif
	;

/* variables */

extern wxString g_Main_Title;

eda_global bool Drc_On
#ifdef MAIN
= TRUE
#endif
;
eda_global bool g_AutoDeleteOldTrack	/* autorise effacement automatique
							de l'ancienne piste lors des redessins de pistes */
#ifdef MAIN
= TRUE
#endif
;
eda_global bool g_No_Via_Route;
eda_global bool g_Drag_Pistes_On;
eda_global bool g_Show_Ratsnest;
eda_global bool g_Show_Module_Ratsnest;
eda_global bool g_Show_Pads_Module_in_Move
#ifdef MAIN
= TRUE
#endif
;
eda_global bool g_Raccord_45_Auto
#ifdef MAIN
= TRUE
#endif
;
eda_global bool g_ShowIsolDuringCreateTrack; /* .State controle l'affichage
									de l'isolation en trace de piste */

	/**************************************************************/
	/* Options d'affichages (remplissages des segments, textes..) */
	/**************************************************************/

eda_global DISPLAY_OPTIONS DisplayOpt;

eda_global bool Track_45_Only;			/* Flag pour limiter l'inclinaison
										pistes a 45 degres seulement */
eda_global bool Segments_45_Only ;		 /* Flag pour limiter l'inclinaison
										edge pcb a 45 degres seulement */
eda_global wxString PcbExtBuffer
#ifdef MAIN
( wxT(".brd"))
#endif
;
eda_global wxString g_SaveFileName		// File Name for periodic saving
#ifdef MAIN
( wxT("$savepcb") )
#endif
;
eda_global wxString NetNameBuffer;
eda_global wxString NetExtBuffer
#ifdef MAIN
( wxT(".net") )
#endif
;
eda_global wxString NetCmpExtBuffer
#ifdef MAIN
( wxT(".cmp") )
#endif
;

// Gestion de la liste des librairies
eda_global wxString LibExtBuffer
#ifdef MAIN
( wxT(".mod") )
#endif
;
eda_global wxString g_Shapes3DExtBuffer		// extension des fichiers de formes 3D
#ifdef MAIN
( wxT(".wrl") )
#endif
;

eda_global int g_NetType;		// for cvpcb: Net type identifier

eda_global int g_CurrentVersionPCB
#ifdef MAIN
= 1
#endif
;

#define BUFMEMSIZE 256000	/* taille du buffer de travail (en octets) */
eda_global char *buf_work ;		/* pointeur sur le buffer de travail */
eda_global char  * adr_lowmem ;	/* adresse de base memoire de calcul disponible*/
eda_global char  * adr_himem ;	/* adresse haute limite de la memoire disponible*/
eda_global char  * adr_max ;	/* adresse haute maxi utilisee pour la memoire */


/* variables génerales */
eda_global char cbuf[1024] ;		/* buffer de formatage texte */
eda_global BOARD * g_ModuleEditor_Pcb;	/* Pointeur de l'entete pcb de l'editeur de module*/
eda_global int g_TimeOut;			// Duree entre 2 sauvegardes automatiques
eda_global int g_SaveTime;			// heure de la prochaine sauvegarde


/* Variables generales d'empreintes */
extern int Angle_Rot_Module;
eda_global wxSize ModuleTextSize;  /* dim des textes sur Modules par defaut */
eda_global int ModuleTextWidth;
eda_global int ModuleSegmentWidth;
eda_global int Texte_Module_Type;


	/***********************/
	/* pistes , vias , pads*/
	/***********************/

#define L_MIN_DESSIN 1	/* seuil de largeur des pistes pour trace autre que filaire */

// Current designe settings:
eda_global class EDA_BoardDesignSettings g_DesignSettings;

// valeurs par defaut des caract. des pads
#ifndef GERBVIEW
#ifdef MAIN
D_PAD g_Pad_Master( (MODULE *) NULL);
#else
extern D_PAD g_Pad_Master;
#endif
#endif


eda_global int Route_Layer_TOP ;
eda_global int Route_Layer_BOTTOM;		/* couches de routage actif */

eda_global int g_MaxLinksShowed;		// determine le nombre max de links affichés
									//		en routage manuel
eda_global bool g_TwoSegmentTrackBuild	// FALSE = 1 segment build, TRUE = 2 45 deg segm build
#ifdef MAIN
= TRUE
#endif
;

/* How to handle magentic pad: feature to move the pcb cursor on a pad center */
enum MagneticPadOptionValues {
	no_effect,
	capture_cursor_in_track_tool,
	capture_always
};
eda_global int g_MagneticPadOption
#ifdef MAIN
= capture_cursor_in_track_tool
#endif
;
eda_global bool g_HightLigt_Status;
eda_global int g_HightLigth_NetCode		/* pour mise en surbrillance des pistes */
#ifdef MAIN
= -1
#endif
;	/* net_code du net a mettre en surbrillance */

eda_global TRACK * g_CurrentTrackSegment ;		// pointeur sur le segment en cours de trace
eda_global TRACK * g_FirstTrackSegment ;	// pointeur sur le debut de la piste en cours
eda_global int g_TrackSegmentCount ;		// nombre de points deja traces


eda_global wxString g_ViaType_Name[4]
#if defined MAIN
	= {
	wxT("???"),	// Unused
	_("Blind Via"),		// from inner layer to external layer (TOP or BOTTOM)
	_("Buried Via"),	// from inner to inner layer
	_("Standard Via")	// Usual via (from TOP to BOTTOM layer)
	}
#endif
;
eda_global int g_ViaHoleLastValue;	// Last value for non default value via hole

/* Couleur de fond affichage de bas d'ecran */
eda_global int g_PcbGridColor;

/* couleurs des autres items des empreintes */
#if defined MAIN
int g_PadCMPColor = RED;
int g_PadCUColor = GREEN;
int g_AnchorColor = BLUE;
int g_ModuleTextCMPColor = LIGHTGRAY;
int g_ModuleTextCUColor = MAGENTA;
int g_ModuleTextNOVColor = DARKGRAY;
#else
eda_global int g_ModuleTextCMPColor;
eda_global int g_ModuleTextCUColor;
eda_global int g_ModuleTextNOVColor;
eda_global int g_AnchorColor;
eda_global int g_PadCUColor;
eda_global int g_PadCMPColor;
#endif

eda_global PCB_SCREEN* ScreenPcb;		/* Ecran principal */
eda_global PCB_SCREEN* ScreenModule;	/* Ecran de l'editeur de modules */


	/****************************************************/
	/* Gestion du deplacement des modules et des pistes */
	/****************************************************/

eda_global wxPoint g_Offset_Module;	/* Offset de trace du modul en depl */

/* Pad editing */
eda_global wxString g_Current_PadName;	// Last used pad name (pad num)


#endif /* PCBNEW_H */
