	/***************************************/
	/*	GENERAL.H :  declarations communes */
	/***************************************/

#ifndef _GENERAL_H_
#define _GENERAL_H_

#ifndef eda_global
#define eda_global extern
#endif

#define us unsigned short
#define uc unsigned char
#define ul unsigned long

/* Entete des fichiers schematique */
#define EESCHEMA_VERSION 2

#define SCHEMATIC_HEAD_STRING "Schematic File Version"
#define EESCHEMA_FILE_STAMP  "EESchema"
#define NULL_STRING "_NONAME_"

#define MAX_PIN_INFO	10

#define TXTMARGE 10					/* Decalage (en 1/1000") des textes places
									sur fils ( labels, num pins ) */

#define HIGHLIGHT_COLOR WHITE


/* Used for EDA_BaseStruct, .m_Select member */
#define IS_SELECTED 1

#define TEXT_NO_VISIBLE 1


//#define GR_DEFAULT_DRAWMODE GR_COPY
#define GR_DEFAULT_DRAWMODE GR_COPY

#define DANGLING_SYMBOL_SIZE 12

/* Message de presentation */
extern wxString g_Main_Title;
eda_global wxString g_DefaultSchematicFileName
#ifdef MAIN
 (wxT("noname.sch"))
#endif
;

/* Masque de recherche pour localisation d'objets a editer */
#define LIBITEM 1
#define WIREITEM 2
#define BUSITEM 4
#define RACCORDITEM 4
#define JUNCTIONITEM 0x10
#define DRAWITEM 0x20
#define TEXTITEM 0x40
#define LABELITEM 0x80
#define SHEETITEM 0x100
#define MARKERITEM 0x200
#define NOCONNECTITEM 0x400
#define SEARCH_PINITEM	0x800
#define SHEETLABELITEM 0x1000
#define FIELDCMPITEM 0x2000
#define EXCLUDE_WIRE_BUS_ENDPOINTS 0x4000
#define WIRE_BUS_ENDPOINTS_ONLY 0x8000

#define SEARCHALL LIBITEM|WIREITEM|BUSITEM|RACCORDITEM|JUNCTIONITEM\
		|DRAWITEM|TEXTITEM|LABELITEM|SHEETITEM|MARKERITEM\
		|NOCONNECTITEM|SEARCH_PINITEM|SHEETLABELITEM

/* Numero des couches de travail */
typedef enum {
	LAYER_WIRE,
	LAYER_BUS,
	LAYER_JUNCTION,
	LAYER_LOCLABEL,
	LAYER_GLOBLABEL,
 	LAYER_HIERLABEL,
	LAYER_PINFUN,
	LAYER_PINNUM,
	LAYER_PINNAM,
	LAYER_REFERENCEPART,
	LAYER_VALUEPART,
	LAYER_FIELDS,
	LAYER_DEVICE,
	LAYER_NOTES,
	LAYER_NETNAM,
	LAYER_PIN,
	LAYER_SHEET,
	LAYER_SHEETNAME,
	LAYER_SHEETFILENAME,
	LAYER_SHEETLABEL,
	LAYER_NOCONNECT,
	LAYER_ERC_WARN,
	LAYER_ERC_ERR,
	LAYER_DEVICE_BACKGROUND,

	MAX_LAYER					/* Nombre de couches */
	} LayerNumber;

typedef enum {
	FILE_SAVE_AS,
	FILE_SAVE_NEW
} FileSaveType;

eda_global wxSize g_GridList[]
#ifdef MAIN
 = {
	wxSize(50,50), wxSize(20,20), wxSize(10,10),
	wxSize(-1,-1), wxSize(0,0)
}
#endif
;


/* variables generales */
eda_global wxArrayString g_LibName_List;	// library list (short filenames) to load
eda_global LibraryStruct *g_LibraryList;	// All part libs are saved here.

eda_global int g_NetFormat;			/* Numero de reference du type de netliste */
eda_global int g_OptNetListUseNames;	/* TRUE pour utiliser les noms de net plutot que
								les numeros (netlist PSPICE seulement) */
eda_global int g_BGColor;			/* couleur fond d'ecran (normalement blanc) */
eda_global SCH_ITEM * g_ItemToRepeat; /* pointeur sur la derniere structure
								dessinee pouvant etre dupliquee par la commande
								Repeat ( NULL si aucune struct existe ) */
eda_global wxSize g_RepeatStep;
eda_global int g_RepeatDeltaLabel;

eda_global SCH_ITEM * g_ItemToUndoCopy; /* copy of last modified schematic item
		before it is modified (used for undo managing to restore old values ) */

eda_global bool g_LastSearchIsMarker;	// True if last seach is a marker serach
										// False for a schematic item search
										// Used for hotkey next search

/* Block operation (copy, paste) */
eda_global SCH_ITEM * g_BlockSaveDataList; // List of items to paste (Created by Block Save)

// Gestion d'options
eda_global int g_ShowAllPins;
eda_global int g_ShowGrid;			// Bool: display grid
#ifdef MAIN
wxSize g_User_Grid(50,50);
int g_HVLines = 1;				// Bool: force H or V directions (Wires, Bus ..)
#else
extern wxSize g_User_Grid;
extern int g_HVLines;
#endif

eda_global int g_PlotPSColorOpt;	// True = plot postcript color (see plotps.cpp)


// Gestion de diverses variables, options... devant etre m�moris�es mais
// Remises a 0 lors d'un rechargement de projetc
struct EESchemaVariables
{
	int NbErrorErc;
	int NbWarningErc;
};

eda_global struct EESchemaVariables g_EESchemaVar;
/* Variable fonction print */
eda_global int g_PrintFillMask;	/* pour les options "FILL",
							l'option reelle est m_Fill & ~PrintFillMask */

/* Variables globales pour Libview */
eda_global wxString g_CurrentViewLibraryName;			/* nom de la librairie en cours d'examen */
eda_global wxString g_CurrentViewComponentName;		/* nom du le composant en cours d'examen */
eda_global int g_ViewConvert;						/* Vue normal / convert */
eda_global int g_ViewUnit;						/* unit� a afficher (A, B ..) */

/* Variables globales pour Schematic Edit */
eda_global int g_DefaultTextLabelSize
#ifdef MAIN
= DEFAULT_SIZE_TEXT
#endif
;

/* Variables globales pour LibEdit */
eda_global int g_LastTextSize
#ifdef MAIN
= DEFAULT_SIZE_TEXT
#endif
;
eda_global int g_LastTextOrient
#ifdef MAIN
= TEXT_ORIENT_HORIZ
#endif
;

eda_global bool g_FlDrawSpecificUnit
#ifdef MAIN
= FALSE
#endif
;
eda_global bool g_FlDrawSpecificConvert
#ifdef MAIN
= TRUE
#endif
;

	/********************************************************/
	/* Description des structures des parametres principaux */
	/********************************************************/

	/* Gestion des trace sur table tracante */

eda_global int g_PlotFormat;		/* flag = TYPE_HPGL, TYPE_PS... */
eda_global int g_PlotMargin;		/* Marge pour traces du cartouche */
eda_global float g_PlotScaleX, g_PlotScaleY;	/* coeff d'echelle de trace en unites table tracante */



/* For HPGL plotting: Pen caract : */
struct HPGL_Pen_Descr_Struct
{
	int m_Pen_Num;		/* num de plume a charger */
	int m_Pen_Speed;	/* vitesse en cm/s */
	int m_Pen_Diam;		/* Pen diameter in mils */
};
eda_global HPGL_Pen_Descr_Struct g_HPGL_Pen_Descr;

/* Ecrans usuels */
//eda_global SCH_SCREEN * ScreenSch;
eda_global DrawSheetStruct* g_RootSheet;
eda_global SCH_SCREEN * ScreenLib;

	/*************************************/
	/* Gestion de recherche des elements */
	/*************************************/

/* valeur de flag indicant si le pointeur de reference pour une localisation
est le curseur sur grille ou le curseur a deplacement fin hors grille */
#define CURSEUR_ON_GRILLE 0
#define CURSEUR_OFF_GRILLE 1

/* Gestion des librairies schematiques */
eda_global wxString g_NetNameBuffer;

#ifdef MAIN
wxString g_NetCmpExtBuffer( wxT(".cmp") );
wxString g_SymbolExtBuffer( wxT(".sym") );
wxString g_NetExtBuffer( wxT(".net") );
wxString g_SchExtBuffer( wxT(".sch") );
wxString g_LibExtBuffer( wxT(".lib") );
#else
eda_global wxString g_NetCmpExtBuffer;
eda_global wxString g_SymbolExtBuffer;
eda_global wxString g_NetExtBuffer;
eda_global wxString g_SchExtBuffer;
eda_global wxString g_LibExtBuffer;
#endif

eda_global wxString g_SimulatorCommandLine;	// ligne de commande pour l'appel au simulateur (gnucap, spice..)
eda_global wxString g_NetListerCommandLine;	// ligne de commande pour l'appel au simulateur (gnucap, spice..)

eda_global LayerStruct g_LayerDescr;		/* couleurs des couches  */

eda_global bool g_EditPinByPinIsOn	/* bool: TRUE si edition des pins pin a pin au lieu */
#ifdef MAIN						/* de l'edition simultan�e des pins de meme coordonn�es */
 = FALSE
#endif
;

eda_global int g_LibSymbolDefaultLineWidth;	/* default line width  (in EESCHEMA units) used when creating a new graphic item in libedit : 0 = default */
eda_global int g_DrawMinimunLineWidth;		/* Minimum line (in EESCHEMA units) width used to draw items on screen; 0 = single pixel line width */
eda_global int g_PlotPSMinimunLineWidth;	/* Minimum line (in EESCHEMA units) width used to Plot items , postscript format */

/* Config keys */
#define MINI_DRAW_LINE_WIDTH_KEY wxT("MinimunDrawLineWidth")
#define MINI_PLOTPS_LINE_WIDTH_KEY wxT("MinimunPlotPSLineWidth")

#endif   // _GENERAL_H_
