/**************************************/
/*	PCBNEW.H :  headers */
/**************************************/
#ifndef PCBNEW_H
#define PCBNEW_H

#include "pcbstruct.h"
#include "macros.h"
#include "pcbcommon.h"

#define U_PCB (PCB_INTERNAL_UNIT / EESCHEMA_INTERNAL_UNIT)

/* Flag used in locate functions
 * the locate ref point is the on grid cursor or the off grid mouse cursor */
#define CURSEUR_ON_GRILLE  (0 << 0)
#define CURSEUR_OFF_GRILLE (1 << 0)

#define IGNORE_LOCKED (1 << 1)          ///< if module is locked, do not select for single module operation
#define MATCH_LAYER   (1 << 2)          ///< if module not on current layer, do not select
#define VISIBLE_ONLY  (1 << 3)          ///< if module not on a visible layer, do not select


#define START 0     /* Flag used in locale routines */
#define END   1

#define DIM_ANCRE_MODULE 3      /* Anchor size (footprint centre) */
#define DIM_ANCRE_TEXTE  2      /* Anchor size (Text centre) */

#define TEXTS_MIN_SIZE  50      // Min size in pcbnew units value (50 * 0.0001 mils)
#define TEXTS_MAX_SIZE  100000  // Min size in pcbnew units value (1 inch) )
#define TEXTS_MAX_WIDTH 5000    // Max width in pcbnew units value (0.5 inches)

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


/* variables */
extern bool Drc_On;
extern bool g_AutoDeleteOldTrack;
extern bool g_No_Via_Route;
extern bool g_Drag_Pistes_On;
extern bool g_Show_Ratsnest;
extern bool g_Show_Module_Ratsnest;
extern bool g_Show_Pads_Module_in_Move;
extern bool g_Raccord_45_Auto;

extern const wxString g_FootprintLibFileWildcard;   // Wildcard for footprint libraries filesnames

/**
 * Function IsModuleLayerVisible
 * expects either of the two layers on which a module can reside, and returns
 * whether that layer is visible.
 * @param layer One of the two allowed layers for modules: CMP_N or COPPER_LAYER_N
 * @return bool - true if the layer is visible, else false.
 */
bool inline IsModuleLayerVisible( int layer )
{
    if( layer==CMP_N )
        return DisplayOpt.Show_Modules_Cmp;

    else if( layer==COPPER_LAYER_N )
        return DisplayOpt.Show_Modules_Cu;

    else
        return true;
}


extern bool        Track_45_Only;
extern bool        Segments_45_Only;
extern wxString    g_Shapes3DExtBuffer;
extern wxString    g_DocModulesFileName;

/* Variables used in footprint handling */
extern int         Angle_Rot_Module;
extern wxSize      ModuleTextSize; /* Default footprint texts size */
extern int         ModuleTextWidth;
extern int         ModuleSegmentWidth;

/* Layer pair for auto routing and switch layers by hotkey */
extern int         Route_Layer_TOP;
extern int         Route_Layer_BOTTOM;

extern int         g_MaxLinksShowed; // Max count links showed in routing
extern bool        g_TwoSegmentTrackBuild;

extern int         g_MagneticPadOption;
extern int         g_MagneticTrackOption;

/* Variables to handle hightlight nets */
extern bool        g_HightLigt_Status;
extern int         g_HightLigth_NetCode;

extern wxPoint     g_Offset_Module;     /* Offset de trace du modul en depl */

extern wxString    g_Current_PadName;   // Last used pad name (pad num)


enum MagneticPadOptionValues {
    no_effect,
    capture_cursor_in_track_tool,
    capture_always
};


#endif /* PCBNEW_H */
