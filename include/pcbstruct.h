/**************************************************************/
/*  pcbstruct.h :  some classes and definitions used in pcbnew */
/**************************************************************/

#ifndef PCBSTRUCT_H
#define PCBSTRUCT_H

#include "base_struct.h"
#include "class_base_screen.h"
#include "class_board_item.h"
#include "layers_id_colors_and_visibility.h"

// Definitions relatives aux libraries
#define ENTETE_LIBRAIRIE        "PCBNEW-LibModule-V1"
#define ENTETE_LIBDOC           "PCBNEW-LibDoc----V1"
#define L_ENTETE_LIB            18
#define EXT_DOC                 wxT( "mdc" )


#define FLAG1                   (1 << 13)   // flag for free local computations
#define FLAG0                   (1 << 12)   // flag for free local computations
#define BEGIN_ONPAD             (1 << 11)   // flag indicating a start of segment pad
#define END_ONPAD               (1 << 10)   // flag indicating an end of segment pad
#define BUSY                    (1 << 9)    // flag indicating that the structure has
                                            // already been edited, in some routines
#define DELETED                 (1 << 8)    // structures erased and set string "DELETED"
#define NO_TRACE                (1 << 7)    // The element must not be displayed

#define SURBRILL                (1 << 5)    // element highlighted
#define DRAG                    (1 << 4)    // segment in drag mode
#define EDIT                    (1 << 3)    // element being edited
#define SEGM_FIXE               (1 << 2)    // segment fixed (not erase global)
#define SEGM_AR                 (1 << 1)    // segment marked for auto routing
#define CHAIN                   (1 << 0)    // mark segment


class NETINFO_ITEM;
class MARKER_PCB;
class RATSNEST_ITEM;


/* main window classes : */
#include "wxBasePcbFrame.h"

/* Class to handle a board */
#include "class_board.h"


// Values for m_DisplayViaMode member:
enum DisplayViaMode {
    VIA_HOLE_NOT_SHOW = 0,
    VIA_SPECIAL_HOLE_SHOW,
    ALL_VIA_HOLE_SHOW,
    OPT_VIA_HOLE_END
};

/* Handle info to display a board */
#include "class_pcb_screen.h"

/**********************************/
/* Module (Footprint) description */
/**********************************/

#include "class_pad.h"          // class for pads
#include "class_edge_mod.h"     // Class for  footprint graphic elements
#include "class_text_mod.h"     // Class for  footprint fields
#include "class_module.h"       // Class for the footprint
#include "class_netinfo.h"      // Class for nets

#include "class_drawsegment.h"
#include "class_pcb_text.h"
#include "class_dimension.h"
#include "class_mire.h"
#include "class_track.h"
#include "class_marker_pcb.h"
#include "class_zone.h"

/* Values for DISPLAY_OPTIONS.ShowTrackClearanceMode parameter option
 *  This parameter controls how to show tracks and vias clearance area
 */
enum ShowTrackClearanceModeList {
    DO_NOT_SHOW_CLEARANCE = 0,                // Do not show clearance areas
    SHOW_CLEARANCE_NEW_TRACKS,                /* Show clearance areas only
                                               * for new track during track
                                               * creation */
    SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS,  /* Show clearance areas only
                                               * for new track during track
                                               * creation, and shows a via
                                               * clearance area at end of
                                               * current new segment (guide
                                               * to place a new via
                                               */
    SHOW_CLEARANCE_ALWAYS                      /* Show Always clearance areas
                                                * for track and vias
                                                */
};

class DISPLAY_OPTIONS
{
public:
    bool DisplayPadFill;
    bool DisplayViaFill;
    bool DisplayPadNum;
    bool DisplayPadNoConn;
    bool DisplayPadIsol;

    int  DisplayModEdge;
    int  DisplayModText;
    bool DisplayPcbTrackFill;     /* FALSE = sketch , TRUE = filled */
    int  ShowTrackClearanceMode;  /* = 0 , 1 or 2
                                   *  0 = do not show clearance
                                   *  1 = show track clearance
                                   *  2 = show clearance + via area
                                   *  (useful to know what clearance area is
                                   * needed if we want to put a via on
                                   * terminal track point)
                                   */

    int m_DisplayViaMode;       /* 0 do not show via hole,
                                 * 1 show via hole for non default value
                                 * 2 show all via hole */

    bool DisplayPolarCood;
    int  DisplayZonesMode;
    int  DisplayNetNamesMode;   /* 0 do not show netnames,
                                 * 1 show netnames on pads
                                 * 2 show netnames on tracks
                                 * 3 show netnames on tracks and pads
                                 */

    int  DisplayDrawItems;
    bool ContrastModeDisplay;

public:
    DISPLAY_OPTIONS();
};

#endif // PCBSTRUCT_H
