/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef LAYERS_ID_AND_VISIBILITY_H_
#define LAYERS_ID_AND_VISIBILITY_H_

#include <stdint.h>
#include <vector>
#include <bitset>
#include <wx/string.h>
#include <macros.h>

class BOARD;


/**
 * Type LAYER_NUM
 * can be replaced with int and removed.  Until then, it is something you can increment,
 * and its meaning is only advisory but can extend beyond PCB layers into view layers
 * and gerber layers.
 */
typedef int     LAYER_NUM;


/**
 * Enum LAYER_ID
 * is the set of PCB layers.  It has nothing to do with gerbers or view layers.
 * One of these cannot be "incremented".
 */
enum LAYER_ID
#if __cplusplus >= 201103L
    : unsigned char
#endif
{
    F_Cu,           // 0
    In1_Cu,
    In2_Cu,
    In3_Cu,
    In4_Cu,
    In5_Cu,
    In6_Cu,
    In7_Cu,
    In8_Cu,
    In9_Cu,
    In10_Cu,
    In11_Cu,
    In12_Cu,
    In13_Cu,
    In14_Cu,
    In15_Cu,
    In16_Cu,
    In17_Cu,
    In18_Cu,
    In19_Cu,
    In20_Cu,
    In21_Cu,
    In22_Cu,
    In23_Cu,
    In24_Cu,
    In25_Cu,
    In26_Cu,
    In27_Cu,
    In28_Cu,
    In29_Cu,
    In30_Cu,
    B_Cu,           // 31

    B_Adhes,
    F_Adhes,

    B_Paste,
    F_Paste,

    B_SilkS,
    F_SilkS,

    B_Mask,
    F_Mask,

    Dwgs_User,
    Cmts_User,
    Eco1_User,
    Eco2_User,
    Edge_Cuts,
    Margin,

    B_CrtYd,
    F_CrtYd,

    B_Fab,
    F_Fab,

    LAYER_ID_COUNT
};


#define UNDEFINED_LAYER     LAYER_ID(-1)
#define UNSELECTED_LAYER    LAYER_ID(-2)

#define MAX_CU_LAYERS       (B_Cu - F_Cu + 1)

/* These were moved to legacy_plugin.cpp, please don't ever use them
   outside there.  Now with the advent of class LSEQ, we don't iterate over
   LAYER_ID any more, so therefore FIRST_COPPER_LAYER and LAST_COPPER_LAYER are
   dead concepts.  They in fact failed to do what they were intended to do because
   they implied a particular sequence which in and of itself was subject to change
   and actually did when we flipped the pretty and *.kicad_pcb copper layer stack.
   LSEQ is the way to go, use it.  It gives a level of manipulation between
   LAYER_ID and iteration.
#define FIRST_COPPER_LAYER      brain dead
#define LAST_COPPER_LAYER       brain dead
#define FIRST_LAYER             brain dead
#define NB_LAYERS               use LAYER_ID_COUNT instead
#define NB_COPPER_LAYERS        was always a max, not a number, use MAX_CU_LAYERS now.
*/


/// A sequence of layers, a sequence provides a certain order.
typedef std::vector<LAYER_ID>   BASE_SEQ;


/**
 * Class LSEQ
 * is a sequence (and therefore also a set) of LAYER_IDs.  A sequence provides
 * a certain order.
 * <p>
 * It can also be used as an iterator:
 * <code>
 *
 *      for( LSEQ cu_stack = aSet.CuStack();  cu_stack;  ++cu_stack )
 *      {
 *          layer_id = *cu_stack;
 *          :
 *          things to do with layer_id;
 *      }
 *
 * </code>
 */
class LSEQ : public BASE_SEQ
{
    unsigned   m_index;

public:

    LSEQ() :
        m_index( 0 )
    {}

    template <class InputIterator>
    LSEQ( InputIterator start, InputIterator end ) :
        BASE_SEQ( start, end ),
        m_index( 0 )
    {}

    void Rewind()           { m_index = 0; }

    void operator ++ ()     { ++m_index; }  // returns nothing, used in simple statements only.

    void operator ++ (int)  { ++m_index; }

    operator bool ()        { return m_index < size(); }

    LAYER_ID operator * () const
    {
        return at( m_index );       // throws std::out_of_range
    }
};


typedef std::bitset<LAYER_ID_COUNT>     BASE_SET;


/**
 * Class LSET
 * is a set of LAYER_IDs.  It can be converted to numerous purpose LSEQs using
 * the various member functions, most of which are based on Seq(). The advantage
 * of converting to LSEQ using purposeful code, is it removes any dependency
 * on order/sequence inherent in this set.
 */
class LSET : public BASE_SET
{
public:

    // The constructor flavors are carefully chosen to prevent LSET( int ) from compiling.
    // That excludes  "LSET s = 0;" and excludes "LSET s = -1;", etc.
    // LSET s = 0;  needs to be removed from the code, this accomplishes that.
    // Remember LSET( LAYER_ID(0) ) sets bit 0, so "LSET s = 0;" is illegal
    // to prevent that surprize.  Therefore LSET's constructor suite is significantly
    // different than the base class from which it is derived.

    // Other member functions (non-constructor functions) are identical to the base
    // class's and therefore are re-used from the base class.

    /**
     * Constructor LSET()
     * creates an empty (cleared) set.
     */
    LSET() :
        BASE_SET()  // all bits are set to zero in BASE_SET()
    {
    }

    LSET( const BASE_SET& aOther ) :
        BASE_SET( aOther )
    {
    }

    /**
     * Constructor LSET( LAYER_ID )
     * takes a LAYER_ID and sets that bit.  This makes the following code into
     * a bug:
     *
     * <code>   LSET s = 0;  </code>
     *
     * Instead use:
     *
     * <code>
     *    LSET s;
     * </code>
     *
     * for an empty set.
     */
    LSET( LAYER_ID aLayer ) :    // LAYER_ID deliberately exludes int and relatives
        BASE_SET()
    {
        set( aLayer );
    }

    /**
     * Constructor LSET( const LAYER_ID* aArray, unsigned aCount )
     * works well with an arry or LSEQ.
     */
    LSET( const LAYER_ID* aArray, unsigned aCount );

    /**
     * Constructor LSET( unsigned, LAYER_ID, ...)
     * takes one or more LAYER_IDs in the argument list to construct
     * the set.  Typically only used in static construction.
     *
     * @param aIdCount is the number of LAYER_IDs which follow.
     * @param aFirst is the first included in @a aIdCount and must always be present, and can
     *  be followed by any number of additional LAYER_IDs so long as @a aIdCount accurately
     *  reflects the count.
     */
    LSET( unsigned aIdCount, LAYER_ID aFirst, ... );  // args chosen to prevent LSET( int ) from compiling

    /**
     * Function Name
     * returns the fixed name association with aLayerId.
     */
    static const wxChar* Name( LAYER_ID aLayerId );

    /**
     * Function InternalCuMask()
     * returns a complete set of internal copper layers, which is all Cu layers
     * except F_Cu and B_Cu.
     */
    static LSET InternalCuMask();

    /**
     * Function AllCuMask
     * returns a mask holding the requested number of Cu LAYER_IDs.
     */
    static LSET AllCuMask( int aCuLayerCount = MAX_CU_LAYERS );

    /**
     * Function AllNonCuMask
     * returns a mask holding all layer minus CU layers.
     */
    static LSET AllNonCuMask();

    static LSET AllLayersMask();

    /**
     * Function FrontTechMask
     * returns a mask holding all technical layers (no CU layer) on front side.
     */
    static LSET FrontTechMask();

    /**
     * Function BackTechMask
     * returns a mask holding all technical layers (no CU layer) on back side.
     */
    static LSET BackTechMask();
    static LSET AllTechMask();

    /**
     * Function FrontMask
     * returns a mask holding all technical layers and the external CU layer on front side.
     */
    static LSET FrontMask();

    /**
     * Function BackMask
     * returns a mask holding all technical layers and the external CU layer on back side.
     */
    static LSET BackMask();

    static LSET UserMask();


    /**
     * Function CuStack
     * returns a sequence of copper layers in starting from the front/top
     * and extending to the back/bottom.  This specific sequence is depended upon
     * in numerous places.
     */
    LSEQ CuStack() const;

    /**
     * Function Technicals
     * returns a sequence of technical layers.  A sequence provides a certain
     * order.
     * @param aSubToOmit is the subset of the techical layers to omit, defaults to none.
     */
    LSEQ Technicals( LSET aSubToOmit = LSET() ) const;

    /// *_User layers.
    LSEQ Users() const;

    LSEQ UIOrder() const;

    /**
     * Function Seq
     * returns an LSEQ from the union of this LSET and a desired sequence.  The LSEQ
     * element will be in the same sequence as aWishListSequence if they are present.
     * @param aWishListSequence establishes the order of the returned LSEQ, and the LSEQ will only
     * contiain LAYER_IDs which are present in this set.
     * @param aCount is the length of aWishListSequence array.
     */
    LSEQ Seq( const LAYER_ID* aWishListSequence, unsigned aCount ) const;

    /**
     * Function Seq
     * returns a LSEQ from this LSET in ascending LAYER_ID order.  Each LSEQ
     * element will be in the same sequence as in LAYER_ID and only present
     * in the resultant LSEQ if present in this set.  Therefore the sequence is
     * subject to change, use it only when enumeration and not order is important.
     */
    LSEQ Seq() const;

    /**
     * Function SVG
     * returns the sequence used to output an SVG plot.
    LSEQ SVG() const;
     put this in the needed source file using Seq() there.
    */

    /**
     * Function FmtHex
     * returns a hex string showing contents of this LSEQ.
     */
    std::string FmtHex() const;

    /**
     * Function ParseHex
     * understands the output of FmtHex() and replaces this set's values
     * with those given in the input string.  Parsing stops at the first
     * non hex ASCII byte, except that marker bytes output from FmtHex are
     * not terminators.
     * @return int - number of bytes consumed
     */
    int ParseHex( const char* aStart, int aCount );

    /**
     * Function FmtBin
     * returns a binary string showing contents of this LSEQ.
     */
    std::string FmtBin() const;

    /**
     * Find the first set LAYER_ID. Returns UNDEFINED_LAYER if more
     * than one is set or UNSELECTED_LAYER if none is set.
     */
    LAYER_ID ExtractLayer() const;

private:

    /// Take this off the market, it may not be used because of LSET( LAYER_ID ).
    LSET( unsigned long __val )
    {
        // not usable, it's private.
    }
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
#if 0
// was:
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
#else
enum NETNAMES_VISIBLE
{
    PAD_FR_NETNAMES_VISIBLE = B_Cu+1,
    PAD_BK_NETNAMES_VISIBLE,
    PADS_NETNAMES_VISIBLE,

    END_NETNAMES_VISIBLE_LIST   // sentinel
};
#endif


/// macro for obtaining layer number for specific item (eg. pad or text)
#define ITEM_GAL_LAYER(layer)       (LAYER_ID_COUNT + layer)

#define NETNAMES_GAL_LAYER(layer)   (LAYER_ID_COUNT + END_PCB_VISIBLE_LIST + layer )

/// number of *all* GAL layers including PCB and item layers
#define TOTAL_LAYER_COUNT	        (LAYER_ID_COUNT + END_PCB_VISIBLE_LIST + END_NETNAMES_VISIBLE_LIST)

/**
 * Function IsValidLayer
 * tests whether a given integer is a valid layer index, i.e. can
 * be safely put in a LAYER_ID
 * @param aLayerId = Layer index to test. It can be an int, so its
 * useful during I/O
 * @return true if aLayerIndex is a valid layer index
 */
inline bool IsValidLayer( LAYER_NUM aLayerId )
{
    return unsigned( aLayerId ) < LAYER_ID_COUNT;
}

/**
 * Function IsPcbLayer
 * tests whether a layer is a valid layer for pcbnew
 * @param aLayer = Layer to test
 * @return true if aLayer is a layer valid in pcbnew
 */
inline bool IsPcbLayer( LAYER_NUM aLayer )
{
    return aLayer >= F_Cu && aLayer < LAYER_ID_COUNT;
}

/**
 * Function IsCopperLayer
 * tests whether a layer is a copper layer
 * @param aLayerId = Layer  to test
 * @return true if aLayer is a valid copper layer
 */
inline bool IsCopperLayer( LAYER_NUM aLayerId )
{
    return aLayerId >= F_Cu && aLayerId <= B_Cu;
}

/**
 * Function IsNonCopperLayer
 * tests whether a layer is a non copper layer
 * @param aLayerId = Layer to test
 * @return true if aLayer is a non copper layer
 */
inline bool IsNonCopperLayer( LAYER_NUM aLayerId )
{
    return aLayerId > B_Cu && aLayerId <= LAYER_ID_COUNT;
}

/**
 * Function IsUserLayer
 * tests whether a layer is a non copper and a non tech layer
 * @param aLayerId = Layer to test
 * @return true if aLayer is a user layer
 */
inline bool IsUserLayer( LAYER_ID aLayerId )
{
    return aLayerId >= Dwgs_User && aLayerId <= Eco2_User;
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
inline bool IsFrontLayer( LAYER_ID aLayerId )
{
    switch( aLayerId )
    {
    case F_Cu:
    case F_Adhes:
    case F_Paste:
    case F_SilkS:
    case F_Mask:
    case F_CrtYd:
    case F_Fab:
        return true;
    default:
        ;
    }

    return false;
}


/**
 * Layer classification: check if it's a back layer
 */
inline bool IsBackLayer( LAYER_ID aLayerId )
{
    switch( aLayerId )
    {
    case B_Cu:
    case B_Adhes:
    case B_Paste:
    case B_SilkS:
    case B_Mask:
    case B_CrtYd:
    case B_Fab:
        return true;
    default:
        ;
    }

    return false;
}


/**
 * Function FlippedLayerNumber
 * @return the layer number after flipping an item
 * some (not all) layers: external copper, Mask, Paste, and solder
 * are swapped between front and back sides
 */
LAYER_ID FlipLayer( LAYER_ID oldlayer );

/**
 * Calculate the mask layer when flipping a footprint
 * BACK and FRONT copper layers, mask, paste, solder layers are swapped
 */
LSET FlipLayerMask( LSET aMask );

/**
 * Return a string (to be shown to the user) describing a layer mask.
 * Useful for showing where is a pad, track, entity, etc.
 * The BOARD is needed because layer names are (somewhat) customizable
 */
wxString LayerMaskDescribe( const BOARD* aBoard, LSET aMask );

/**
 * Returns a netname layer corresponding to the given layer.
 */
inline int GetNetnameLayer( int aLayer )
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
    return Cmts_User;
}

/**
 * Function IsNetnameLayer
 * tests whether a layer is a netname layer
 * @param aLayer = Layer to test
 * @return true if aLayer is a valid netname layer
 */
inline bool IsNetnameLayer( LAYER_NUM aLayer )
{
    return aLayer >= NETNAMES_GAL_LAYER( F_Cu ) &&
           aLayer < NETNAMES_GAL_LAYER( END_NETNAMES_VISIBLE_LIST );
}


LAYER_ID ToLAYER_ID( int aLayer );

#endif // LAYERS_ID_AND_VISIBILITY_H_
