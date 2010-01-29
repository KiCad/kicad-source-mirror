/**************************************************************/
/*  pcbstruct.h :  some classes and definitions used in pcbnew */
/**************************************************************/

#ifndef _LAYERS_ID_AND_VISIBILITY_H_
#define _LAYERS_ID_AND_VISIBILITY_H_

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
#define ADHESIVE_N_BACK         16
#define ADHESIVE_N_FRONT        17
#define SOLDERPASTE_N_BACK      18
#define SOLDERPASTE_N_FRONT     19
#define SILKSCREEN_N_BACK       20
#define SILKSCREEN_N_FRONT      21
#define SOLDERMASK_N_BACK       22
#define SOLDERMASK_N_FRONT      23
#define DRAW_N                  24
#define COMMENT_N               25
#define ECO1_N                  26
#define ECO2_N                  27
#define EDGE_N                  28
#define LAST_NO_COPPER_LAYER    28
#define UNUSED_LAYER_29         29
#define UNUSED_LAYER_30         30
#define UNUSED_LAYER_31         31
#define NB_LAYERS               (LAST_NO_COPPER_LAYER + 1)

#define LAYER_COUNT             32

// Masks to identify a layer by a bit map
#define LAYER_BACK              (1 << LAYER_N_BACK)     ///< bit mask for copper layer
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
#define LAYER_FRONT             (1 << LAYER_N_FRONT)    ///< bit mask for component layer
#define ADHESIVE_LAYER_BACK     (1 << ADHESIVE_N_BACK)
#define ADHESIVE_LAYER_FRONT    (1 << ADHESIVE_N_FRONT)
#define SOLDERPASTE_LAYER_BACK  (1 << SOLDERPASTE_N_BACK)
#define SOLDERPASTE_LAYER_FRONT (1 << SOLDERPASTE_N_FRONT)
#define SILKSCREEN_LAYER_BACK   (1 << SILKSCREEN_N_BACK)
#define SILKSCREEN_LAYER_FRONT  (1 << SILKSCREEN_N_FRONT)
#define SOLDERMASK_LAYER_BACK   (1 << SOLDERMASK_N_BACK)
#define SOLDERMASK_LAYER_FRONT  (1 << SOLDERMASK_N_FRONT)
#define DRAW_LAYER              (1 << DRAW_N)
#define COMMENT_LAYER           (1 << COMMENT_N)
#define ECO1_LAYER              (1 << ECO1_N)
#define ECO2_LAYER              (1 << ECO2_N)
#define EDGE_LAYER              (1 << EDGE_N)

#define FIRST_NON_COPPER_LAYER  ADHESIVE_N_BACK
#define LAST_NON_COPPER_LAYER   EDGE_N

//      extra bits              0xE0000000
/* Helpful global layers mask : */
#define ALL_LAYERS              0x1FFFFFFF              // Pcbnew used 29 layers
#define FULL_LAYERS             0xFFFFFFFF              // Gerbview used 32 layers
#define ALL_NO_CU_LAYERS        0x1FFF0000
#define ALL_CU_LAYERS           0x0000FFFF
#define INTERNAL_LAYERS         0x00007FFE
#define EXTERNAL_LAYERS         0x00008001


// layers order in dialogs (plot, print and toolbars)
// in same order than in setup layers dialog
// (Front or Top to Back or Bottom)
#define DECLARE_LAYERS_ORDER_LIST(list) int list[LAYER_COUNT] =\
{   LAYER_N_FRONT,\
    LAYER_N_15, LAYER_N_14, LAYER_N_13, LAYER_N_12,\
    LAYER_N_11, LAYER_N_10, LAYER_N_9, LAYER_N_8,\
    LAYER_N_7, LAYER_N_6, LAYER_N_5, LAYER_N_4,\
    LAYER_N_3, LAYER_N_2,\
    LAYER_N_BACK,\
    ADHESIVE_N_FRONT , ADHESIVE_N_BACK,\
    SOLDERPASTE_N_FRONT, SOLDERPASTE_N_BACK,\
    SILKSCREEN_N_FRONT, SILKSCREEN_N_BACK,\
    SOLDERMASK_N_FRONT, SOLDERMASK_N_BACK,\
    DRAW_N,\
    COMMENT_N,\
    ECO1_N, ECO2_N,\
    EDGE_N,\
    UNUSED_LAYER_29, UNUSED_LAYER_30, UNUSED_LAYER_31\
};

/**
 * Enum PCB_VISIBLE
 * is a set of visible PCB elements.
 * @see BOARD::SetVisibleElementColor()
 * @see BOARD::SetVisibleElement()
 */
enum PCB_VISIBLE
{
    VIAS_VISIBLE,
    VIA_MICROVIA_VISIBLE,
    VIA_BBLIND_VISIBLE,
    VIA_THROUGH_VISIBLE,
    MOD_TEXT_FR_VISIBLE,
    MOD_TEXT_BK_VISIBLE,
    MOD_TEXT_INVISIBLE,         ///< text marked as invisible
    ANCHOR_VISIBLE,
    PAD_FR_VISIBLE,
    PAD_BK_VISIBLE,
    RATSNEST_VISIBLE,
    GRID_VISIBLE,

    // the rest of these do not currently support color changes:
    NO_CONNECTS_VISIBLE,        ///< show a marker on pads with no nets
    MOD_FR_VISIBLE,             ///< show modules on front
    MOD_BK_VISIBLE,             ///< show modules on back
    MOD_VALUES_VISIBLE,         ///< show modules values (when texts are visibles)
    MOD_REFERENCES_VISIBLE,     ///< show modules references (when texts are visibles)

    END_PCB_VISIBLE_LIST  // sentinel
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

#endif // _LAYERS_ID_AND_VISIBILITY_H_
