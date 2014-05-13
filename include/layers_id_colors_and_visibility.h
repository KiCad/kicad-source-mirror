/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file layers_id_colors_and_visibility.h
 * @brief Board layer functions and definitions.
 */

#ifndef _LAYERS_ID_AND_VISIBILITY_H_
#define _LAYERS_ID_AND_VISIBILITY_H_

class BOARD;

/* NOTE: the idea here is to have LAYER_NUM and LAYER_MSK as abstract
 * type as possible (even if they're currently implemented as int and
 * unsigned int, respectively). In this way it would be reasonably easy
 * to overcome the current 32 layer limit. For example switching to a 64
 * bit mask or even some kind of bit array */

/* Layer identification (layer number) */
typedef int LAYER_NUM;
#define UNDEFINED_LAYER         -1
#define FIRST_LAYER             0
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
#define NB_COPPER_LAYERS        (LAST_COPPER_LAYER - FIRST_COPPER_LAYER + 1)

#define FIRST_NON_COPPER_LAYER  16
#define FIRST_TECHNICAL_LAYER   16
#define FIRST_USER_LAYER        24
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
#define LAST_NON_COPPER_LAYER   28
#define LAST_TECHNICAL_LAYER    23
#define LAST_USER_LAYER   27
#define NB_PCB_LAYERS           (LAST_NON_COPPER_LAYER + 1)
#define UNUSED_LAYER_29         29
#define UNUSED_LAYER_30         30
#define UNUSED_LAYER_31         31
#define NB_GERBER_LAYERS        32
#define NB_LAYERS               32
#define UNSELECTED_LAYER        32

// Masks to identify a layer by a bit map
typedef unsigned LAYER_MSK;
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

//      extra bits              0xE0000000

// Helpful global layer masks:
// ALL_AUX_LAYERS layers are technical layers, ALL_NO_CU_LAYERS has user
// and edge layers too!
#define ALL_LAYERS              0x1FFFFFFF              // Pcbnew used 29 layers
#define FULL_LAYERS             0xFFFFFFFF              // Gerbview used 32 layers
#define ALL_NO_CU_LAYERS        0x1FFF0000
#define ALL_CU_LAYERS           0x0000FFFF
#define INTERNAL_CU_LAYERS      0x00007FFE
#define EXTERNAL_CU_LAYERS      0x00008001
#define FRONT_TECH_LAYERS       (SILKSCREEN_LAYER_FRONT | SOLDERMASK_LAYER_FRONT \
                               | ADHESIVE_LAYER_FRONT | SOLDERPASTE_LAYER_FRONT)
#define BACK_TECH_LAYERS        (SILKSCREEN_LAYER_BACK | SOLDERMASK_LAYER_BACK \
                               | ADHESIVE_LAYER_BACK | SOLDERPASTE_LAYER_BACK)
#define ALL_TECH_LAYERS         (FRONT_TECH_LAYERS | BACK_TECH_LAYERS)
#define BACK_LAYERS            (LAYER_BACK | BACK_TECH_LAYERS)
#define FRONT_LAYERS           (LAYER_FRONT | FRONT_TECH_LAYERS)

#define ALL_USER_LAYERS         (DRAW_LAYER | COMMENT_LAYER |\
                                 ECO1_LAYER | ECO2_LAYER )

#define NO_LAYERS               0x00000000

/**
 * @return a one bit layer mask from a layer number
 * @param aLayerNumber = the layer number to convert (0 .. LAYERS-1)
 */
inline LAYER_MSK GetLayerMask( LAYER_NUM aLayerNumber )
{
    return 1 << aLayerNumber;
}

/**
 * @return bool if aLayerNumber is a layer contained in aMask
 * @param aMask = a layer mask
 * @param aLayerNumber is the layer id to test
 */
inline bool IsLayerInList( LAYER_MSK aMask, LAYER_NUM aLayerNumber )
{
    return (aMask & GetLayerMask( aLayerNumber )) != 0;
}

/**
 * @return bool if 2 layer masks have a comman layer
 * @param aMask1 = a layer mask
 * @param aMask2 = an other layer mask
 */
inline bool IsLayerMasksIntersect( LAYER_MSK aMask1, LAYER_MSK aMask2 )
{
    return (aMask1 & aMask2) != 0;
}

/**
 * Count the number of set layers in the mask
 */
inline int LayerMaskCountSet( LAYER_MSK aMask )
{
    int count = 0;

    for( LAYER_NUM i = FIRST_LAYER; i < NB_LAYERS; ++i )
    {
        if( aMask & GetLayerMask( i ) )
            ++count;
    }
    return count;
}


// layers order in dialogs (plot, print and toolbars)
// in same order than in setup layers dialog
// (Front or Top to Back or Bottom)
#define DECLARE_LAYERS_ORDER_LIST(list) const LAYER_NUM list[NB_LAYERS] =\
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
    NON_PLATED_VISIBLE,
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

    TRACKS_VISIBLE,
    PADS_VISIBLE,               ///< multilayer pads, usually with holes
    PADS_HOLES_VISIBLE,
    VIAS_HOLES_VISIBLE,

    DRC_VISIBLE,                ///< drc markers
    WORKSHEET,                  ///< worksheet frame
    GP_OVERLAY,                 ///< general purpose overlay

    END_PCB_VISIBLE_LIST        // sentinel
};

/**
 * Enum NETNAMES_VISIBLE
 * is a set of layers specific for displaying net names.
 * Their visiblity is not supposed to be saved in a board file,
 * they are only to be used by the GAL.
 */
enum NETNAMES_VISIBLE
{
    LAYER_1_NETNAMES_VISIBLE,   // bottom layer
    LAYER_2_NETNAMES_VISIBLE,
    LAYER_3_NETNAMES_VISIBLE,
    LAYER_4_NETNAMES_VISIBLE,
    LAYER_5_NETNAMES_VISIBLE,
    LAYER_6_NETNAMES_VISIBLE,
    LAYER_7_NETNAMES_VISIBLE,
    LAYER_8_NETNAMES_VISIBLE,
    LAYER_9_NETNAMES_VISIBLE,
    LAYER_10_NETNAMES_VISIBLE,
    LAYER_11_NETNAMES_VISIBLE,
    LAYER_12_NETNAMES_VISIBLE,
    LAYER_13_NETNAMES_VISIBLE,
    LAYER_14_NETNAMES_VISIBLE,
    LAYER_15_NETNAMES_VISIBLE,
    LAYER_16_NETNAMES_VISIBLE,  // top layer

    PAD_FR_NETNAMES_VISIBLE,
    PAD_BK_NETNAMES_VISIBLE,
    PADS_NETNAMES_VISIBLE,

    END_NETNAMES_VISIBLE_LIST   // sentinel
};

/// macro for obtaining layer number for specific item (eg. pad or text)
#define ITEM_GAL_LAYER(layer)       (NB_LAYERS + layer)

#define NETNAMES_GAL_LAYER(layer)   (NB_LAYERS + END_PCB_VISIBLE_LIST + layer )

/// number of *all* layers including PCB and item layers
#define TOTAL_LAYER_COUNT	        (NB_LAYERS + END_PCB_VISIBLE_LIST + END_NETNAMES_VISIBLE_LIST)

/**
 * Function IsValidLayer
 * tests whether a given integer is a valid layer index, i.e. can
 * be safely put in a LAYER_NUM
 * @param aLayerIndex = Layer index to test. It can be an int, so its
 * useful during I/O
 * @return true if aLayerIndex is a valid layer index
 */
inline bool IsValidLayer( int aLayerIndex )
{
    return aLayerIndex >= FIRST_LAYER && aLayerIndex < NB_LAYERS;
}

/**
 * Function IsPcbLayer
 * tests whether a layer is a valid layer for pcbnew
 * @param aLayer = Layer to test
 * @return true if aLayer is a layer valid in pcbnew
 */
inline bool IsPcbLayer( LAYER_NUM aLayer )
{
    return aLayer >= FIRST_LAYER && aLayer < NB_PCB_LAYERS;
}

/**
 * Function IsCopperLayer
 * tests whether a layer is a copper layer
 * @param aLayer = Layer  to test
 * @return true if aLayer is a valid copper layer
 */
inline bool IsCopperLayer( LAYER_NUM aLayer )
{
    return aLayer >= FIRST_COPPER_LAYER
        && aLayer <= LAST_COPPER_LAYER;
}

/**
 * Function IsNonCopperLayer
 * tests whether a layer is a non copper layer
 * @param aLayer = Layer to test
 * @return true if aLayer is a non copper layer
 */
inline bool IsNonCopperLayer( LAYER_NUM aLayer )
{
    return aLayer >= FIRST_NON_COPPER_LAYER && aLayer <= LAST_NON_COPPER_LAYER;
}

/**
 * Function IsUserLayer
 * tests whether a layer is a non copper and a non tech layer
 * @param aLayer = Layer to test
 * @return true if aLayer is a user layer
 */
inline bool IsUserLayer( LAYER_NUM aLayer )
{
    return aLayer >= FIRST_USER_LAYER && aLayer <= LAST_USER_LAYER;
}

/* IMPORTANT: If a layer is not a front layer not necessarily is true
   the converse. The same hold for a back layer.
   So a layer can be:
   - Front
   - Back
   - Neither (internal or auxiliary)

   The check most frequent is for back layers, since it involves flips */


/**
 * Layer classification: check if it's a front layer
 */
inline bool IsFrontLayer( LAYER_NUM aLayer )
{
    return ( aLayer == LAYER_N_FRONT ||
             aLayer == ADHESIVE_N_FRONT ||
             aLayer == SOLDERPASTE_N_FRONT ||
             aLayer == SILKSCREEN_N_FRONT ||
             aLayer == SOLDERPASTE_N_FRONT );
}

/**
 * Layer classification: check if it's a back layer
 */
inline bool IsBackLayer( LAYER_NUM aLayer )
{
    return ( aLayer == LAYER_N_BACK ||
             aLayer == ADHESIVE_N_BACK ||
             aLayer == SOLDERPASTE_N_BACK ||
             aLayer == SILKSCREEN_N_BACK ||
             aLayer == SOLDERPASTE_N_BACK );
}

/**
 * Function FlippedLayerNumber
 * @return the layer number after flipping an item
 * some (not all) layers: external copper, Mask, Paste, and solder
 * are swapped between front and back sides
 */
LAYER_NUM FlipLayer( LAYER_NUM oldlayer );

/**
 * Calculate the mask layer when flipping a footprint
 * BACK and FRONT copper layers, mask, paste, solder layers are swapped
 */
LAYER_MSK FlipLayerMask( LAYER_MSK aMask );

/**
 * Extract the set layer from a mask. Returns UNDEFINED_LAYER if more
 * than one is set or UNSELECTED_LAYER if none is
 */
LAYER_NUM ExtractLayer( LAYER_MSK aMask );

/**
 * Return a string (to be shown to the user) describing a layer mask.
 * Useful for showing where is a pad, track, entity, etc.
 * The BOARD is needed because layer names are (somewhat) customizable
 */
wxString LayerMaskDescribe( const BOARD *aBoard, LAYER_MSK aMask );

/**
 * Returns a netname layer corresponding to the given layer.
 */
inline LAYER_NUM GetNetnameLayer( LAYER_NUM aLayer )
{
    if( IsCopperLayer( aLayer ) )
        return NETNAMES_GAL_LAYER( aLayer );
    else if( aLayer == ITEM_GAL_LAYER( PADS_VISIBLE ) )
        return NETNAMES_GAL_LAYER( PADS_NETNAMES_VISIBLE );
    else if( aLayer == ITEM_GAL_LAYER( PAD_FR_VISIBLE ) )
        return NETNAMES_GAL_LAYER( PAD_FR_NETNAMES_VISIBLE );
    else if( aLayer == ITEM_GAL_LAYER( PAD_BK_VISIBLE ) )
        return NETNAMES_GAL_LAYER( PAD_BK_NETNAMES_VISIBLE );

    // Fallback
    return COMMENT_N;
}

/**
 * Function IsNetnameLayer
 * tests whether a layer is a netname layer
 * @param aLayer = Layer to test
 * @return true if aLayer is a valid netname layer
 */
inline bool IsNetnameLayer( LAYER_NUM aLayer )
{
    return aLayer >= NETNAMES_GAL_LAYER( LAYER_1_NETNAMES_VISIBLE ) &&
           aLayer < NETNAMES_GAL_LAYER( END_NETNAMES_VISIBLE_LIST );
}

#endif // _LAYERS_ID_AND_VISIBILITY_H_
