/**************************************************************/
/*  pcbstruct.h :  some classes and definitions used in pcbnew */
/**************************************************************/

#ifndef PCBSTRUCT_H
#define PCBSTRUCT_H

#include "base_struct.h"
#include "class_base_screen.h"
#include "class_board_item.h"

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


/* Layer identification (layer number) */
#define FIRST_COPPER_LAYER      0
#define LAYER_N_BACK            0
#define LAYER_N_2               1
#define LAYER_N_3               2
#define LAYER_N_4               3
#define LAYER_N_5               4
#define LAYER_N_6               5
#define LAYER_N_7               6
#define LAYER_N_8               7
#define LAYER_N_9               8
#define LAYER_N_10              9
#define LAYER_N_11              10
#define LAYER_N_12              11
#define LAYER_N_13              12
#define LAYER_N_14              13
#define LAYER_N_15              14
#define LAYER_N_FRONT           15
#define LAST_COPPER_LAYER       LAYER_N_FRONT
#define NB_COPPER_LAYERS        (LAST_COPPER_LAYER + 1)

#define FIRST_NO_COPPER_LAYER   16
#define ADHESIVE_N_CU           16
#define ADHESIVE_N_CMP          17
#define SOLDERPASTE_N_CU        18
#define SOLDERPASTE_N_CMP       19
#define SILKSCREEN_N_CU         20
#define SILKSCREEN_N_CMP        21
#define SOLDERMASK_N_CU         22
#define SOLDERMASK_N_CMP        23
#define DRAW_N                  24
#define COMMENT_N               25
#define ECO1_N                  26
#define ECO2_N                  27
#define EDGE_N                  28
#define LAST_NO_COPPER_LAYER    28
#define NB_LAYERS               (LAST_NO_COPPER_LAYER + 1)

#define LAYER_COUNT             32


#define LAYER_BACK              (1 << LAYER_N_BACK)     ///< bit mask for copper layer
#define CUIVRE_LAYER            (1 << LAYER_N_BACK)     ///< bit mask for copper layer
#define LAYER_2                 (1 << LAYER_N_2)        ///< bit mask for layer 2
#define LAYER_3                 (1 << LAYER_N_3)        ///< bit mask for layer 3
#define LAYER_4                 (1 << LAYER_N_4)        ///< bit mask for layer 4
#define LAYER_5                 (1 << LAYER_N_5)        ///< bit mask for layer 5
#define LAYER_6                 (1 << LAYER_N_6)        ///< bit mask for layer 6
#define LAYER_7                 (1 << LAYER_N_7)        ///< bit mask for layer 7
#define LAYER_8                 (1 << LAYER_N_8)        ///< bit mask for layer 8
#define LAYER_9                 (1 << LAYER_N_9)        ///< bit mask for layer 9
#define LAYER_10                (1 << LAYER_N_10)       ///< bit mask for layer 10
#define LAYER_11                (1 << LAYER_N_11)       ///< bit mask for layer 11
#define LAYER_12                (1 << LAYER_N_12)       ///< bit mask for layer 12
#define LAYER_13                (1 << LAYER_N_13)       ///< bit mask for layer 13
#define LAYER_14                (1 << LAYER_N_14)       ///< bit mask for layer 14
#define LAYER_15                (1 << LAYER_N_15)       ///< bit mask for layer 15
#define CMP_LAYER               (1 << LAYER_N_FRONT)    ///< bit mask for component layer
#define LAYER_FRONT             (1 << LAYER_N_FRONT)    ///< bit mask for component layer
#define ADHESIVE_LAYER_CU       (1 << ADHESIVE_N_CU)
#define ADHESIVE_LAYER_CMP      (1 << ADHESIVE_N_CMP)
#define SOLDERPASTE_LAYER_CU    (1 << SOLDERPASTE_N_CU)
#define SOLDERPASTE_LAYER_CMP   (1 << SOLDERPASTE_N_CMP)
#define SILKSCREEN_LAYER_CU     (1 << SILKSCREEN_N_CU)
#define SILKSCREEN_LAYER_CMP    (1 << SILKSCREEN_N_CMP)
#define SOLDERMASK_LAYER_CU     (1 << SOLDERMASK_N_CU)
#define SOLDERMASK_LAYER_CMP    (1 << SOLDERMASK_N_CMP)
#define DRAW_LAYER              (1 << DRAW_N)
#define COMMENT_LAYER           (1 << COMMENT_N)
#define ECO1_LAYER              (1 << ECO1_N)
#define ECO2_LAYER              (1 << ECO2_N)
#define EDGE_LAYER              (1 << EDGE_N)

#define FIRST_NON_COPPER_LAYER  ADHESIVE_N_CU
#define LAST_NON_COPPER_LAYER   EDGE_N

//      extra bits              0xE0000000
/* Helpful global layers mask : */
#define ALL_LAYERS              0x1FFFFFFF              // Pcbnew used 29 layers
#define FULL_LAYERS             0xFFFFFFFF              // Gerbview used 32 layers
#define ALL_NO_CU_LAYERS        0x1FFF0000
#define ALL_CU_LAYERS           0x0000FFFF
#define INTERNAL_LAYERS         0x00007FFE
#define EXTERNAL_LAYERS         0x00008001

class NETINFO_ITEM;
class MARKER_PCB;
class RATSNEST_ITEM;


/* main window classes : */
#include "wxBasePcbFrame.h"

/* Class to handle a board */
#include "class_board.h"

enum ELEMENTS_NUMBERS
{
    VIAS_VISIBLE                = 0,
    VIA_NOT_DEFINED_VISIBLE     =  VIAS_VISIBLE,
    VIA_MICROVIA_VISIBLE,
    VIA_BLIND_BURIED_VISIBLE,
    VIA_THROUGH_VISIBLE,
    MODULE_TEXT_CMP_VISIBLE,
    MODULE_TEXT_CU_VISIBLE,
    MODULE_TEXT_NOV_VISIBLE,
    ANCHOR_VISIBLE,
    PAD_CU_VISIBLE,
    PAD_CMP_VISIBLE
};

/**
 * Function IsValidLayerIndex
 * tests whether a given integer is a valid layer index
 * @param aLayerIndex = Layer index to test
 * @return true if aLayerIndex is a valid layer index
 */
inline bool IsValidLayerIndex( int aLayerIndex )
{
    return aLayerIndex >= 0 && aLayerIndex < NB_LAYERS;
}

/**
 * Function IsValidCopperLayerIndex
 * tests whether an integer is a valid copper layer index
 * @param aLayerIndex = Layer index to test
 * @return true if aLayerIndex is a valid copper layer index
 */
inline bool IsValidCopperLayerIndex( int aLayerIndex )
{
    return aLayerIndex >= FIRST_COPPER_LAYER && aLayerIndex <= LAST_COPPER_LAYER;
}

/**
 * Function IsValidNonCopperLayerIndex
 * tests whether an integer is a valid non copper layer index
 * @param aLayerIndex = Layer index to test
 * @return true if aLayerIndex is a valid non copper layer index
 */
inline bool IsValidNonCopperLayerIndex( int aLayerIndex )
{
    return aLayerIndex >= FIRST_NO_COPPER_LAYER
        && aLayerIndex <= LAST_NO_COPPER_LAYER;
}


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
#include "class_cotation.h"
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

    bool Show_Modules_Cmp;
    bool Show_Modules_Cu;

    int  DisplayDrawItems;
    bool ContrastModeDisplay;

public:
    DISPLAY_OPTIONS();
};

#endif // PCBSTRUCT_H
