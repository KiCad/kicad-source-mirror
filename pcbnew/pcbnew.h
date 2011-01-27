/**************************************/
/*	PCBNEW.H :  headers */
/**************************************/
#ifndef PCBNEW_H
#define PCBNEW_H

#include "pcbstruct.h"
#include "pcbcommon.h"
#include "class_board_design_settings.h"

#define U_PCB (PCB_INTERNAL_UNIT / EESCHEMA_INTERNAL_UNIT)

// Arcs are appromed by segments: define the number of segments per 360 deg (kicad use 0.1 deg approx:
#define ARC_APPROX_SEGMENTS_COUNT_LOW_DEF 16        // be aware 3600/ARC_APPROX_SEGMENTS_COUNT_LOW_DEF is an integer
#define ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF 32

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
extern bool g_Drag_Pistes_On;
extern bool g_Show_Module_Ratsnest;
extern bool g_Show_Pads_Module_in_Move;
extern bool g_Raccord_45_Auto;

extern const wxString g_FootprintLibFileWildcard;   // Wildcard for footprint libraries filesnames


extern bool        g_Track_45_Only_Allowed;
extern bool        g_Alternate_Track_Posture;
extern bool        Segments_45_Only;
extern wxString    g_Shapes3DExtBuffer;
extern wxString    g_DocModulesFileName;

/* Variables used in footprint handling */
extern wxSize      g_ModuleTextSize; /* Default footprint texts size */
extern int         g_ModuleTextWidth;
extern int         g_ModuleSegmentWidth;

/* Layer pair for auto routing and switch layers by hotkey */
extern int         Route_Layer_TOP;
extern int         Route_Layer_BOTTOM;

extern int         g_MaxLinksShowed; // Max count links showed in routing
extern bool        g_TwoSegmentTrackBuild;

extern int         g_MagneticPadOption;
extern int         g_MagneticTrackOption;

/* Variables to handle highlight nets */
extern bool        g_HighLight_Status;
extern int         g_HighLight_NetCode;

extern wxPoint     g_Offset_Module;     /* Offset de trace du modul en depl */

extern wxString    g_Current_PadName;   // Last used pad name (pad num)


enum MagneticPadOptionValues {
    no_effect,
    capture_cursor_in_track_tool,
    capture_always
};


#endif /* PCBNEW_H */
