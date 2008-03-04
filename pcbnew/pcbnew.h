/**************************************/
/*	PCBNEW.H :  headers */
/**************************************/
#ifndef PCBNEW_H
#define PCBNEW_H

#ifndef eda_global
#define eda_global extern
#endif

#include "pcbstruct.h"
#include "macros.h"

#define U_PCB (PCB_INTERNAL_UNIT / EESCHEMA_INTERNAL_UNIT)

/* Flag used in locate functions
 * the locate ref point is the on grid cursor or the off grid mouse cursor */
#define CURSEUR_ON_GRILLE   (0 << 0)
#define CURSEUR_OFF_GRILLE  (1 << 0)

#define IGNORE_LOCKED       (1 << 1)    ///< if module is locked, do not select for single module operation
#define MATCH_LAYER         (1 << 2)    ///< if module not on current layer, do not select
#define VISIBLE_ONLY        (1 << 3)    ///< if module not on a visible layer, do not select


#define START 0     /* Flag used in locale routines */
#define END   1

#define DIM_ANCRE_MODULE 3  /* Anchor size (footprint centre) */
#define DIM_ANCRE_TEXTE  2  /* nchor size (Text centre) */

/* Used in Zoom menu */
#define ZOOM_PLUS   -1
#define ZOOM_MOINS  -2
#define ZOOM_AUTO   -3
#define ZOOM_CENTER -4
#define ZOOM_REDRAW -5

/* Flag to force the SKETCH mode to display items (.flags member) */
#define FORCE_SKETCH (DRAG | EDIT )

/* Flags used in read board file */
#define APPEND_PCB 1    /* used to append the new board to the existing board */
#define NEWPCB     0    /* used for normal load file */


eda_global wxArrayString g_LibName_List;    // library list to load

eda_global wxSize        g_GridList[]
#ifdef MAIN
= {
    wxSize( 1000, 1000 ), wxSize( 500, 500 ), wxSize( 250, 250 ), wxSize( 200, 200 ),
    wxSize( 100,  100 ),  wxSize( 50,  50 ),  wxSize( 25,  25 ),  wxSize( 20,  20 ),
    wxSize( 10,   10 ),   wxSize( 5,   5 ),   wxSize( 2,   2 ),   wxSize( 1,   1 ),
    wxSize( -1,   -1 ),   wxSize( 0,   0 )
}


#endif
;

#define UNDELETE_STACK_SIZE 10
eda_global BOARD_ITEM* g_UnDeleteStack[UNDELETE_STACK_SIZE]; // Linked list of deleted items
eda_global int         g_UnDeleteStackPtr;

eda_global bool        g_ShowGrid
#ifdef MAIN
= TRUE
#endif
;

/* Look up Table for conversion one layer number -> one bit layer mask: */
eda_global int g_TabOneLayerMask[LAYER_COUNT]
#if defined MAIN
= {
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
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
eda_global bool g_AutoDeleteOldTrack    /* Allows automatic deletion of the old track after
                                          * creation of a new track */
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
eda_global bool g_ShowIsolDuringCreateTrack; /* Show clearance while track creation */

/*********************************/
/* Display options for board items */
/*********************************/

eda_global DISPLAY_OPTIONS DisplayOpt;

/**
 * Function IsModuleLayerVisible
 * expects either of the two layers on which a module can reside, and returns
 * whether that layer is visible.
 * @param layer One of the two allowed layers for modules: CMP_N or COPPER_LAYER_N
 * @return bool - true if the layer is visible, else false.
 */
bool inline IsModuleLayerVisible( int layer ) {
    if( layer==CMP_N )
        return DisplayOpt.Show_Modules_Cmp;

    else if( layer==COPPER_LAYER_N )
        return DisplayOpt.Show_Modules_Cu;

    else
        return true;
}


eda_global bool            Track_45_Only;   /* Flag pour limiter l'inclinaison
                                            *  pistes a 45 degres seulement */
eda_global bool            Segments_45_Only; /* Flag pour limiter l'inclinaison
                                             *  edge pcb a 45 degres seulement */
eda_global wxString        PcbExtBuffer     // Board file extension
#ifdef MAIN
( wxT( ".brd" ) )
#endif
;
eda_global wxString g_SaveFileName      // File Name for periodic saving
#ifdef MAIN
( wxT( "$savepcb" ) )
#endif
;
eda_global wxString NetNameBuffer;      // Netlist file extension
eda_global wxString NetExtBuffer
#ifdef MAIN
( wxT( ".net" ) )
#endif
;
eda_global wxString NetCmpExtBuffer     // cmp/footprint association file extension
#ifdef MAIN
( wxT( ".cmp" ) )
#endif
;

// Gestion de la liste des librairies
eda_global wxString LibExtBuffer
#ifdef MAIN
( wxT( ".mod" ) )
#endif
;
eda_global wxString g_Shapes3DExtBuffer     //3D shape file extension
#ifdef MAIN
( wxT( ".wrl" ) )
#endif
;

eda_global int g_NetType;       // for cvpcb: Net type identifier

eda_global int g_CurrentVersionPCB
#ifdef MAIN
= 1
#endif
;

/* A buffer used in some computations (will be removed in next cleanup code, do not use) */
#define BUFMEMSIZE 256000       /* buffer size (in bytes) */
eda_global char* buf_work;      /* pointeur sur le buffer de travail */
eda_global char* adr_lowmem;    /* adresse de base memoire de calcul disponible*/
eda_global char* adr_himem;     /* adresse haute limite de la memoire disponible*/
eda_global char* adr_max;       /* adresse haute maxi utilisee pour la memoire */


/* variables g�erales */

eda_global char   cbuf[1024];           /* buffer for some text printing */
eda_global BOARD* g_ModuleEditor_Pcb;   /* board used to edit footprints (used by modedit)*/
eda_global int    g_TimeOut;            // Timer for automatic saving
eda_global int    g_SaveTime;           // Time for next saving


/* Variables used in footprint handling */
extern int        Angle_Rot_Module;
eda_global wxSize ModuleTextSize;  /* Default footprint texts size */
eda_global int    ModuleTextWidth;
eda_global int    ModuleSegmentWidth;
eda_global int    Texte_Module_Type;


/***********************/
/* pistes , vias , pads*/
/***********************/

#define L_MIN_DESSIN 1  /* Min width segments to allow draws with tickness */

// Current designe settings:
eda_global class EDA_BoardDesignSettings g_DesignSettings;

// Default values for pad editions
#ifndef GERBVIEW
#ifdef MAIN
D_PAD        g_Pad_Master( (MODULE*) NULL );

#else
extern D_PAD g_Pad_Master;
#endif
#endif


/* Layer pair for auto routing and switch layers by hotkey */
eda_global int  Route_Layer_TOP;
eda_global int  Route_Layer_BOTTOM;

eda_global int  g_MaxLinksShowed;       // Mxa count links showed in routing
eda_global bool g_TwoSegmentTrackBuild  // FALSE = 1 segment build, TRUE = 2 45 deg segm build
#ifdef MAIN
= TRUE
#endif
;

/* How to handle magnetic pads & tracks: feature to move the pcb cursor on a pad center / track length */
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
eda_global int g_MagneticTrackOption
#ifdef MAIN
= capture_cursor_in_track_tool
#endif
;
/* Variables to handle hightlight nets */
eda_global bool g_HightLigt_Status;
eda_global int  g_HightLigth_NetCode
#ifdef MAIN
= -1
#endif
;

/* used in track creation : */
eda_global TRACK*   g_CurrentTrackSegment;      // current created segment
eda_global TRACK*   g_FirstTrackSegment;        // first segment created
eda_global int      g_TrackSegmentCount;        // New created segment count


eda_global wxString g_ViaType_Name[4]
#if defined MAIN
= {
    _( "??? Via" ),                 // Not used yet
    _( "Micro Via" ),               // from external layer (TOP or BOTTOM) from the near neightbour inner layer only
    _( "Blind/Buried Via" ),        // from inner or external to inner or external layer (no restriction)
    _( "Through Via" )              // Usual via (from TOP to BOTTOM layer only )
}


#endif
;
/* couleurs des autres items des empreintes */
#if defined MAIN
int                    g_PadCMPColor = RED;
int                    g_PadCUColor  = GREEN;
int                    g_AnchorColor = BLUE;
int                    g_ModuleTextCMPColor = LIGHTGRAY;
int                    g_ModuleTextCUColor  = MAGENTA;
int                    g_ModuleTextNOVColor = DARKGRAY;
#else
eda_global int         g_ModuleTextCMPColor;
eda_global int         g_ModuleTextCUColor;
eda_global int         g_ModuleTextNOVColor;
eda_global int         g_AnchorColor;
eda_global int         g_PadCUColor;
eda_global int         g_PadCMPColor;
#endif

eda_global PCB_SCREEN* ScreenPcb;       /* Ecran principal */
eda_global PCB_SCREEN* ScreenModule;    /* Ecran de l'editeur de modules */


/****************************************************/
/* Gestion du deplacement des modules et des pistes */
/****************************************************/

eda_global wxPoint  g_Offset_Module;    /* Offset de trace du modul en depl */

/* Pad editing */
eda_global wxString g_Current_PadName;  // Last used pad name (pad num)


#endif /* PCBNEW_H */
