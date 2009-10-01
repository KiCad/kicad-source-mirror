/***************************************/
/*	GENERAL.H :  declarations communes */
/***************************************/

#ifndef _GENERAL_H_
#define _GENERAL_H_


/* Entete des fichiers schematique */
#define EESCHEMA_VERSION 2

#define SCHEMATIC_HEAD_STRING "Schematic File Version"
#define EESCHEMA_FILE_STAMP   "EESchema"
#define NULL_STRING           "_NONAME_"

#define MAX_PIN_INFO 10

#define TXTMARGE 10                 /* Decalage (en 1/1000") des textes places
                                     * sur fils ( labels, num pins ) */

#define HIGHLIGHT_COLOR WHITE


/* Used for EDA_BaseStruct, .m_Select member */
#define IS_SELECTED 1

#define TEXT_NO_VISIBLE 1


//#define GR_DEFAULT_DRAWMODE GR_COPY
#define GR_DEFAULT_DRAWMODE GR_COPY

#define DANGLING_SYMBOL_SIZE 12

/* Message de presentation */
extern wxString g_DefaultSchematicFileName;

/* Masque de recherche pour localisation d'objets a editer */
#define LIBITEM                    1
#define WIREITEM                   2
#define BUSITEM                    4
#define RACCORDITEM                4
#define JUNCTIONITEM               0x10
#define DRAWITEM                   0x20
#define TEXTITEM                   0x40
#define LABELITEM                  0x80
#define SHEETITEM                  0x100
#define MARKERITEM                 0x200
#define NOCONNECTITEM              0x400
#define SEARCH_PINITEM             0x800
#define SHEETLABELITEM             0x1000
#define FIELDCMPITEM               0x2000
#define EXCLUDE_WIRE_BUS_ENDPOINTS 0x4000
#define WIRE_BUS_ENDPOINTS_ONLY    0x8000

#define SEARCHALL LIBITEM | WIREITEM | BUSITEM | RACCORDITEM | JUNCTIONITEM \
    | DRAWITEM | TEXTITEM | LABELITEM | SHEETITEM | MARKERITEM \
    | NOCONNECTITEM | SEARCH_PINITEM | SHEETLABELITEM

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

    MAX_LAYER                   /* Nombre de couches */
} LayerNumber;

typedef enum {
    FILE_SAVE_AS,
    FILE_SAVE_NEW
} FileSaveType;


/* variables generales */
extern int            g_OptNetListUseNames; /* TRUE pour utiliser les noms de net plutot que
                                             * les numeros (netlist PSPICE seulement) */
extern SCH_ITEM*      g_ItemToRepeat; /* pointeur sur la derniere structure
                                       * dessinee pouvant etre dupliquee par la commande
                                       * Repeat ( NULL si aucune struct existe ) */
extern wxSize         g_RepeatStep;
extern int            g_RepeatDeltaLabel;

extern SCH_ITEM*      g_ItemToUndoCopy; /* copy of last modified schematic item
                                         * before it is modified (used for undo managing to restore old values ) */

extern bool           g_LastSearchIsMarker; // True if last seach is a marker serach
// False for a schematic item search
// Used for hotkey next search

/* Block operation (copy, paste) */
extern BLOCK_SELECTOR g_BlockSaveDataList; // List of items to paste (Created by Block Save)

// Gestion d'options
extern bool      g_HVLines;

// Gestion de diverses variables, options... devant etre memorisees mais
// Remises a 0 lors d'un rechargement de projetc
struct EESchemaVariables
{
    int NbErrorErc;
    int NbWarningErc;
};

extern struct EESchemaVariables g_EESchemaVar;


/* Variables globales pour Schematic Edit */
extern int      g_DefaultTextLabelSize;

/* Variables globales pour LibEdit */
extern int      g_LastTextSize;
extern int      g_LastTextOrient;

extern bool     g_FlDrawSpecificUnit;
extern bool     g_FlDrawSpecificConvert;

/********************************************************/
/* Description des structures des parametres principaux */
/********************************************************/

/* Gestion des trace sur table tracante */

/* For HPGL plotting: Pen caract : */
struct HPGL_Pen_Descr_Struct
{
    int m_Pen_Num;      /* num de plume a charger */
    int m_Pen_Speed;    /* vitesse en cm/s */
    int m_Pen_Diam;     /* Pen diameter in mils */
};
extern HPGL_Pen_Descr_Struct g_HPGL_Pen_Descr;

/* First and main (root) screen */
extern DrawSheetStruct* g_RootSheet;


/*************************************/
/* Gestion de recherche des elements */
/*************************************/

/* valeur de flag indicant si le pointeur de reference pour une localisation
 * est le curseur sur grille ou le curseur a deplacement fin hors grille */
#define CURSEUR_ON_GRILLE  0
#define CURSEUR_OFF_GRILLE 1

/* Gestion des librairies schematiques */
extern wxString       g_NetCmpExtBuffer;

extern const wxString SymbolFileExtension;
extern const wxString SymbolFileWildcard;

extern const wxString CompLibFileExtension;
extern const wxString CompLibFileWildcard;

extern wxString       g_SimulatorCommandLine;   // ligne de commande pour l'appel au simulateur (gnucap, spice..)
extern wxString       g_NetListerCommandLine;   // ligne de commande pour l'appel au simulateur (gnucap, spice..)

extern LayerStruct    g_LayerDescr;             /* couleurs des couches  */

extern bool           g_EditPinByPinIsOn;   /* true to do not synchronize pins edition
                                             *  when they are at the same location */

extern int            g_LibSymbolDefaultLineWidth; /* default line width  (in EESCHEMA units)
                                                    *  used when creating a new graphic item in libedit.
                                                    *  0 = use default line thicknes
                                                    */
extern int            g_DrawDefaultLineThickness; /* Default line (in EESCHEMA units) thickness
                                                   *  used to draw/plot items having a default thickness line value (i.e. = 0 ).
                                                   *  0 = single pixel line width
                                                   */

// Color to draw selected items
extern int g_ItemSelectetColor;

// Color to draw items flagged invisible, in libedit (they are insisible in eeschema
extern int g_InvisibleItemColor;

#endif   // _GENERAL_H_
