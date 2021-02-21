/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2010 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <set>
#include <vector>
#include <bitset>
#include <wx/string.h>


/**
 * This can be replaced with int and removed.  Until then, it is something you can increment,
 * and its meaning is only advisory but can extend beyond PCB layers into view layers
 * and gerber layers.
 */
typedef int     LAYER_NUM;

/**
 * A quick note on layer IDs:
 *
 * The layers are stored in separate enums so that certain functions can
 * take in the enums as data types and don't have to know about layers from
 * other applications.
 *
 * Layers that are shared between applications should be in the GAL_LAYER_ID enum.
 *
 * The PCB_LAYER_ID struct must start at zero for compatibility with legacy board files.
 *
 * Some functions accept any layer ID, so they start at zero (i.e. F_Cu) and go up to
 * the LAYER_ID_COUNT, which needs to be kept up-to-date if new enums are added.
 */


/**
 * This is the definition of all layers used in Pcbnew.
 *
 * The PCB layer types are fixed at value 0 through LAYER_ID_COUNT to ensure compatibility
 * with legacy board files.
 */
enum PCB_LAYER_ID: int
{
    UNDEFINED_LAYER = -1,
    UNSELECTED_LAYER = -2,

    PCBNEW_LAYER_ID_START = 0,
    F_Cu = PCBNEW_LAYER_ID_START,
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
    F_Mask,         // 39

    Dwgs_User,
    Cmts_User,
    Eco1_User,
    Eco2_User,
    Edge_Cuts,
    Margin,         // 45

    B_CrtYd,
    F_CrtYd,

    B_Fab,
    F_Fab,          // 49

    // User definable layers.
    User_1,
    User_2,
    User_3,
    User_4,
    User_5,
    User_6,
    User_7,
    User_8,
    User_9,

    Rescue,         // 59

    // Four reserved layers (60 - 63) for future expansion within the 64 bit integer limit.

    PCB_LAYER_ID_COUNT
};

#define MAX_CU_LAYERS       (B_Cu - F_Cu + 1)

/// Dedicated layers for net names used in Pcbnew
enum NETNAMES_LAYER_ID: int
{

    NETNAMES_LAYER_ID_START = PCB_LAYER_ID_COUNT,

    /// Reserved space for board layer netnames

    NETNAMES_LAYER_ID_RESERVED = NETNAMES_LAYER_ID_START + PCB_LAYER_ID_COUNT,

    /// Additional netnames layers (not associated with a PCB layer)

    LAYER_PAD_FR_NETNAMES,
    LAYER_PAD_BK_NETNAMES,
    LAYER_PAD_NETNAMES,
    LAYER_VIA_NETNAMES,

    NETNAMES_LAYER_ID_END
};

/// Macro for obtaining netname layer for a given PCB layer
#define NETNAMES_LAYER_INDEX( layer )   ( NETNAMES_LAYER_ID_START + layer )

/**
 *  GAL layers are "virtual" layers, i.e. not tied into design data.
 *  Some layers here are shared between applications.
 *
 *  NOTE: Be very careful where you add new layers here.  Layers below GAL_LAYER_ID_BITMASK_END
 *  must never be re-ordered and new layers must always be added after this value, because the
 *  layers before this value are mapped to bit locations in legacy board files.
 *
 *  The values in this enum that are used to store visibility state are explicitly encoded with an
 *  offset from GAL_LAYER_ID_START, which is explicitly encoded itself. The exact value of
 *  GAL_LAYER_ID_START is not that sensitive, but the offsets should never be changed or else any
 *  existing visibility settings will be disrupted.
 */
enum GAL_LAYER_ID: int
{
    GAL_LAYER_ID_START = NETNAMES_LAYER_ID_END,

    LAYER_VIAS               = GAL_LAYER_ID_START +  0, ///< Meta control for all vias opacity/visibility
    LAYER_VIA_MICROVIA       = GAL_LAYER_ID_START +  1, ///< to draw micro vias
    LAYER_VIA_BBLIND         = GAL_LAYER_ID_START +  2, ///< to draw blind/buried vias
    LAYER_VIA_THROUGH        = GAL_LAYER_ID_START +  3, ///< to draw usual through hole vias
    LAYER_NON_PLATEDHOLES    = GAL_LAYER_ID_START +  4, ///< handle color for not plated holes (holes, not pads)
    LAYER_MOD_TEXT_FR        = GAL_LAYER_ID_START +  5,
    LAYER_MOD_TEXT_BK        = GAL_LAYER_ID_START +  6,
    LAYER_MOD_TEXT_INVISIBLE = GAL_LAYER_ID_START +  7, ///< text marked as invisible
    LAYER_ANCHOR             = GAL_LAYER_ID_START +  8, ///< anchor of items having an anchor point (texts, footprints)
    LAYER_PAD_FR             = GAL_LAYER_ID_START +  9, ///< smd pads, front layer
    LAYER_PAD_BK             = GAL_LAYER_ID_START + 10, ///< smd pads, back layer
    LAYER_RATSNEST           = GAL_LAYER_ID_START + 11,
    LAYER_GRID               = GAL_LAYER_ID_START + 12,
    LAYER_GRID_AXES          = GAL_LAYER_ID_START + 13,
    LAYER_NO_CONNECTS        = GAL_LAYER_ID_START + 14, ///< show a marker on pads with no nets
    LAYER_MOD_FR             = GAL_LAYER_ID_START + 15, ///< show footprints on front
    LAYER_MOD_BK             = GAL_LAYER_ID_START + 16, ///< show footprints on back
    LAYER_MOD_VALUES         = GAL_LAYER_ID_START + 17, ///< show footprints values (when texts are visibles)
    LAYER_MOD_REFERENCES     = GAL_LAYER_ID_START + 18, ///< show footprints references (when texts are visibles)
    LAYER_TRACKS             = GAL_LAYER_ID_START + 19,
    LAYER_PADS_TH            = GAL_LAYER_ID_START + 20, ///< multilayer pads, usually with holes
    LAYER_PAD_PLATEDHOLES    = GAL_LAYER_ID_START + 21, ///< to draw pad holes (plated)
    LAYER_VIA_HOLES          = GAL_LAYER_ID_START + 22, ///< to draw via holes (pad holes do not use this layer)
    LAYER_DRC_ERROR          = GAL_LAYER_ID_START + 23, ///< layer for drc markers with SEVERITY_ERROR
    LAYER_WORKSHEET          = GAL_LAYER_ID_START + 24, ///< worksheet frame
    LAYER_GP_OVERLAY         = GAL_LAYER_ID_START + 25, ///< general purpose overlay
    LAYER_SELECT_OVERLAY     = GAL_LAYER_ID_START + 26, ///< currently selected items overlay
    LAYER_PCB_BACKGROUND     = GAL_LAYER_ID_START + 27, ///< PCB background color
    LAYER_CURSOR             = GAL_LAYER_ID_START + 28, ///< PCB cursor
    LAYER_AUX_ITEMS          = GAL_LAYER_ID_START + 29, ///< Auxiliary items (guides, rule, etc)
    LAYER_DRAW_BITMAPS       = GAL_LAYER_ID_START + 30, ///< to handle and draw images bitmaps

    /// This is the end of the layers used for visibility bit masks in Pcbnew
    GAL_LAYER_ID_BITMASK_END = GAL_LAYER_ID_START + 31,

    // Layers in this section have visibility controls but were not present in legacy board files.

    LAYER_PADS               = GAL_LAYER_ID_START + 32, ///< Meta control for all pads opacity/visibility (color ignored)
    LAYER_ZONES              = GAL_LAYER_ID_START + 33, ///< Control for copper zone opacity/visibility (color ignored)

    LAYER_PAD_HOLEWALLS      = GAL_LAYER_ID_START + 34,
    LAYER_VIA_HOLEWALLS      = GAL_LAYER_ID_START + 35,
    LAYER_DRC_WARNING        = GAL_LAYER_ID_START + 36, ///< layer for drc markers with SEVERITY_WARNING
    LAYER_DRC_EXCLUSION      = GAL_LAYER_ID_START + 37, ///< layer for drc markers which have been individually excluded
    LAYER_MARKER_SHADOWS     = GAL_LAYER_ID_START + 38, ///< shadows for drc markers

    // Add layers below this point that do not have visibility controls, so don't need explicit
    // enum values

    LAYER_WORKSHEET_PAGE1,      ///< for pageLayout editor previewing
    LAYER_WORKSHEET_PAGEn,      ///< for pageLayout editor previewing

    /// Virtual layers for stacking zones and tracks on a given copper layer
    LAYER_ZONE_START,
    LAYER_ZONE_END = LAYER_ZONE_START + PCB_LAYER_ID_COUNT,

    GAL_LAYER_ID_END
};

/// Use this macro to convert a GAL layer to a 0-indexed offset from LAYER_VIAS
#define GAL_LAYER_INDEX( x ) ( x - GAL_LAYER_ID_START )

/// Macro for getting the zone layer for a given copper layer
#define ZONE_LAYER_FOR( copperLayer ) ( LAYER_ZONE_START + copperLayer )

constexpr int GAL_LAYER_ID_COUNT = GAL_LAYER_ID_END - GAL_LAYER_ID_START;

inline GAL_LAYER_ID operator++( GAL_LAYER_ID& a )
{
    a = GAL_LAYER_ID( int( a ) + 1 );
    return a;
}

inline GAL_LAYER_ID ToGalLayer( int aInteger )
{
    wxASSERT( aInteger >= GAL_LAYER_ID_START && aInteger <= GAL_LAYER_ID_END );
    return static_cast<GAL_LAYER_ID>( aInteger );
}

/// Used for via types
inline GAL_LAYER_ID operator+( const GAL_LAYER_ID& a, int b )
{
    GAL_LAYER_ID t = GAL_LAYER_ID( int( a ) + b );
    wxASSERT( t <= GAL_LAYER_ID_END );
    return t;
}

typedef std::bitset<GAL_LAYER_ID_COUNT> GAL_BASE_SET;

/// Helper for storing and iterating over GAL_LAYER_IDs
class GAL_SET : public GAL_BASE_SET
{
private:
    static constexpr int start = static_cast<int>( GAL_LAYER_ID_START );

public:
    GAL_SET() : std::bitset<GAL_LAYER_ID_COUNT>()
    {
    }

    GAL_SET( const GAL_SET& aOther ) : std::bitset<GAL_LAYER_ID_COUNT>( aOther )
    {
    }

    GAL_SET( const GAL_LAYER_ID* aArray, unsigned aCount );

    GAL_SET& set()
    {
        GAL_BASE_SET::set();
        return *this;
    }

    GAL_SET& set( int aPos, bool aVal = true )
    {
        GAL_BASE_SET::set( aPos, aVal );
        return *this;
    }

    GAL_SET& set( GAL_LAYER_ID aPos, bool aVal = true )
    {
        GAL_BASE_SET::set( static_cast<std::size_t>( aPos ) - start, aVal );
        return *this;
    }

    bool Contains( GAL_LAYER_ID aPos )
    {
        return test( static_cast<std::size_t>( aPos ) - start );
    }

    std::vector<GAL_LAYER_ID> Seq() const;

    static GAL_SET DefaultVisible();
};

/// Eeschema drawing layers
enum SCH_LAYER_ID: int
{
    SCH_LAYER_ID_START = GAL_LAYER_ID_END,

    LAYER_WIRE = SCH_LAYER_ID_START,
    LAYER_BUS,
    LAYER_JUNCTION,
    LAYER_LOCLABEL,
    LAYER_GLOBLABEL,
    LAYER_HIERLABEL,
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
    LAYER_SHEETFIELDS,
    LAYER_SHEETLABEL,
    LAYER_NOCONNECT,
    LAYER_ERC_WARN,
    LAYER_ERC_ERR,
    LAYER_DEVICE_BACKGROUND,
    LAYER_SHEET_BACKGROUND,
    LAYER_SCHEMATIC_GRID,
    LAYER_SCHEMATIC_GRID_AXES,
    LAYER_SCHEMATIC_BACKGROUND,
    LAYER_SCHEMATIC_CURSOR,
    LAYER_BRIGHTENED,
    LAYER_HIDDEN,
    LAYER_SELECTION_SHADOWS,
    LAYER_SCHEMATIC_WORKSHEET,
    LAYER_BUS_JUNCTION,
    LAYER_SCHEMATIC_AUX_ITEMS,

    SCH_LAYER_ID_END
};

#define SCH_LAYER_ID_COUNT ( SCH_LAYER_ID_END - SCH_LAYER_ID_START )

#define SCH_LAYER_INDEX( x ) ( x - SCH_LAYER_ID_START )

inline SCH_LAYER_ID operator++( SCH_LAYER_ID& a )
{
    a = SCH_LAYER_ID( int( a ) + 1 );
    return a;
}

// number of draw layers in Gerbview
#define GERBER_DRAWLAYERS_COUNT PCB_LAYER_ID_COUNT

/// GerbView draw layers
enum GERBVIEW_LAYER_ID: int
{
    GERBVIEW_LAYER_ID_START = SCH_LAYER_ID_END,

    /// GerbView draw layers and d-code layers
    GERBVIEW_LAYER_ID_RESERVED = GERBVIEW_LAYER_ID_START + ( 2 * GERBER_DRAWLAYERS_COUNT ),

    LAYER_DCODES,
    LAYER_NEGATIVE_OBJECTS,
    LAYER_GERBVIEW_GRID,
    LAYER_GERBVIEW_AXES,
    LAYER_GERBVIEW_BACKGROUND,
    LAYER_GERBVIEW_WORKSHEET,

    GERBVIEW_LAYER_ID_END
};

#define GERBER_DRAW_LAYER( x ) ( GERBVIEW_LAYER_ID_START + x )

#define GERBER_DCODE_LAYER( x ) ( GERBER_DRAWLAYERS_COUNT + x )

#define GERBER_DRAW_LAYER_INDEX( x ) ( x - GERBVIEW_LAYER_ID_START )


/// 3D Viewer virtual layers for color settings
enum LAYER_3D_ID : int
{
        LAYER_3D_START = GERBVIEW_LAYER_ID_END,

        LAYER_3D_BACKGROUND_BOTTOM,
        LAYER_3D_BACKGROUND_TOP,
        LAYER_3D_BOARD,
        LAYER_3D_COPPER,
        LAYER_3D_SILKSCREEN_BOTTOM,
        LAYER_3D_SILKSCREEN_TOP,
        LAYER_3D_SOLDERMASK,
        LAYER_3D_SOLDERPASTE,

        LAYER_3D_END = LAYER_3D_SOLDERPASTE
};

/// Must update this if you add any enums after GerbView!
#define LAYER_ID_COUNT LAYER_3D_END


/**
 * Returns the string equivalent of a given layer
 * @param aLayer is a valid layer ID
 */
wxString LayerName( int aLayer );


// Some elements do not have yet a visibility control
// from a dialog, but have a visibility control flag.
// Here is a mask to set them visible, to be sure they are displayed
// after loading a board for instance
#define MIN_VISIBILITY_MASK int( ( 1 << GAL_LAYER_INDEX( LAYER_PAD_PLATEDHOLES ) ) +\
                                 ( 1 << GAL_LAYER_INDEX( LAYER_VIA_HOLES ) ) +\
                                 ( 1 << GAL_LAYER_INDEX( LAYER_SELECT_OVERLAY ) ) +\
                                 ( 1 << GAL_LAYER_INDEX( LAYER_GP_OVERLAY ) ) +\
                                 ( 1 << GAL_LAYER_INDEX( LAYER_RATSNEST ) ) )


/// A sequence of layers, a sequence provides a certain order.
typedef std::vector<PCB_LAYER_ID>   BASE_SEQ;


/**
 * LSEQ is a sequence (and therefore also a set) of PCB_LAYER_IDs.  A sequence provides
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
    LSEQ( InputIterator aStart, InputIterator aEnd ) :
        BASE_SEQ( aStart, aEnd ), m_index( 0 )
    {}

    void Rewind()           { m_index = 0; }

    void operator ++ ()     { ++m_index; }  // returns nothing, used in simple statements only.

    void operator ++ (int)  { ++m_index; }

    operator bool ()        { return m_index < size(); }

    PCB_LAYER_ID operator * () const
    {
        return at( m_index );       // throws std::out_of_range
    }
};


typedef std::bitset<PCB_LAYER_ID_COUNT>     BASE_SET;


/**
 * LSET is a set of PCB_LAYER_IDs.  It can be converted to numerous purpose LSEQs using
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
    // Remember LSET( PCB_LAYER_ID(0) ) sets bit 0, so "LSET s = 0;" is illegal
    // to prevent that surprise.  Therefore LSET's constructor suite is significantly
    // different than the base class from which it is derived.

    // Other member functions (non-constructor functions) are identical to the base
    // class's and therefore are re-used from the base class.

    /**
     * Create an empty (cleared) set.
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
     * Take a PCB_LAYER_ID and sets that bit.  This makes the following code into
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
    LSET( PCB_LAYER_ID aLayer ) :    // PCB_LAYER_ID deliberately excludes int and relatives
        BASE_SET()
    {
        set( aLayer );
    }

    /**
     * Create an array or LSEQ.
     */
    LSET( const PCB_LAYER_ID* aArray, unsigned aCount );

    /**
     * Take one or more PCB_LAYER_IDs in the argument list to construct the set.  Typically
     * only used in static construction.
     *
     * @param aIdCount is the number of PCB_LAYER_IDs which follow.
     * @param aFirst is the first included in @a aIdCount and must always be present, and can
     *  be followed by any number of additional PCB_LAYER_IDs so long as @a aIdCount accurately
     *  reflects the count.
     *
     *  Parameter is 'int' to avoid va_start undefined behavior.
     */
    LSET( unsigned aIdCount, int aFirst, ... ); // args chosen to prevent LSET( int ) from compiling

    /**
     * See if the layer set contains a PCB layer.
     *
     * @param aLayer is the layer to check
     * @return true if the layer is included
     */
    bool Contains( PCB_LAYER_ID aLayer )
    {
        return test( aLayer );
    }

    /**
     * Return the fixed name association with aLayerId.
     */
    static const wxChar* Name( PCB_LAYER_ID aLayerId );

    /**
     * Return a complete set of internal copper layers which is all Cu layers
     * except F_Cu and B_Cu.
     */
    static LSET InternalCuMask();

    /**
     * Return a complete set of all top assembly layers which is all F_SilkS and F_Mask
     */
    static LSET FrontAssembly();

    /**
     * Return a complete set of all bottom assembly layers which is all B_SilkS and B_Mask
     */
    static LSET BackAssembly();

    /**
     * Return a mask holding the requested number of Cu PCB_LAYER_IDs.
     */
    static LSET AllCuMask( int aCuLayerCount = MAX_CU_LAYERS );

    /**
     * Return a mask holding the Front and Bottom layers.
     */
    static LSET ExternalCuMask();

    /**
     * Return a mask holding all layer minus CU layers.
     */
    static LSET AllNonCuMask();

    static LSET AllLayersMask();

    /**
     * Return a mask holding all technical layers (no CU layer) on front side.
     */
    static LSET FrontTechMask();

    /**
     * Return a mask holding technical layers used in a board fabrication
     * (no CU layer) on front side.
     */
    static LSET FrontBoardTechMask();

    /**
     * Return a mask holding all technical layers (no CU layer) on back side.
     */
    static LSET BackTechMask();

    /**
     * Return a mask holding technical layers used in a board fabrication
     * (no CU layer) on Back side.
     */
    static LSET BackBoardTechMask();

    /**
     * Return a mask holding all technical layers (no CU layer) on both side.
     */
    static LSET AllTechMask();

    /**
     * Return a mask holding board technical layers (no CU layer) on both side.
     */
    static LSET AllBoardTechMask();

    /**
     * Return a mask holding all technical layers and the external CU layer on front side.
     */
    static LSET FrontMask();

    /**
     * Return a mask holding all technical layers and the external CU layer on back side.
     */
    static LSET BackMask();

    static LSET UserMask();

    /**
     * Return a mask holding all layers which are physically realized.  Equivalent to the copper
     * layers + the board tech mask.
     */
    static LSET PhysicalLayersMask();

    /**
     * Return a mask with all of the allowable user defined layers.
     */
    static LSET UserDefinedLayers();

    /**
     * Layers which are not allowed within footprint definitions.  Currently internal
     * copper layers and Margin.
     */

    static LSET ForbiddenFootprintLayers();

    /**
     * Return a sequence of copper layers in starting from the front/top
     * and extending to the back/bottom.  This specific sequence is depended upon
     * in numerous places.
     */
    LSEQ CuStack() const;

    /**
     * Return a sequence of technical layers.  A sequence provides a certain order.
     *
     * @param aSubToOmit is the subset of the technical layers to omit, defaults to none.
     */
    LSEQ Technicals( LSET aSubToOmit = LSET() ) const;

    /// *_User layers.
    LSEQ Users() const;

    /// Returns the technical and user layers in the order shown in layer widget
    LSEQ TechAndUserUIOrder() const;

    LSEQ UIOrder() const;

    /**
     * Return an LSEQ from the union of this LSET and a desired sequence.  The LSEQ
     * element will be in the same sequence as aWishListSequence if they are present.
     * @param aWishListSequence establishes the order of the returned LSEQ, and the LSEQ will only
     * contain PCB_LAYER_IDs which are present in this set.
     * @param aCount is the length of aWishListSequence array.
     */
    LSEQ Seq( const PCB_LAYER_ID* aWishListSequence, unsigned aCount ) const;

    /**
     * Return a LSEQ from this LSET in ascending PCB_LAYER_ID order.  Each LSEQ
     * element will be in the same sequence as in PCB_LAYER_ID and only present
     * in the resultant LSEQ if present in this set.  Therefore the sequence is
     * subject to change, use it only when enumeration and not order is important.
     */
    LSEQ Seq() const;

    /**
     * Return the sequence that is typical for a bottom-to-top stack-up.
     * For instance, to plot multiple layers in a single image, the top layers output last.
     */
    LSEQ SeqStackupBottom2Top() const;

    /**
     * Return a hex string showing contents of this LSEQ.
     */
    std::string FmtHex() const;

    /**
     * Convert the output of FmtHex() and replaces this set's values
     * with those given in the input string.  Parsing stops at the first
     * non hex ASCII byte, except that marker bytes output from FmtHex are
     * not terminators.
     * @return int - number of bytes consumed
     */
    int ParseHex( const char* aStart, int aCount );

    /**
     * Return a binary string showing contents of this LSEQ.
     */
    std::string FmtBin() const;

    /**
     * Find the first set PCB_LAYER_ID. Returns UNDEFINED_LAYER if more
     * than one is set or UNSELECTED_LAYER if none is set.
     */
    PCB_LAYER_ID ExtractLayer() const;

private:

    /// Take this off the market, it may not be used because of LSET( PCB_LAYER_ID ).
    LSET( unsigned long __val )
    {
        // not usable, it's private.
    }
};


/**
 * Test whether a given integer is a valid layer index, i.e. can
 * be safely put in a PCB_LAYER_ID
 *
 * @param aLayerId = Layer index to test. It can be an int, so its useful during I/O
 * @return true if aLayerIndex is a valid layer index
 */
inline bool IsValidLayer( LAYER_NUM aLayerId )
{
    return unsigned( aLayerId ) < PCB_LAYER_ID_COUNT;
}

/**
 * Test whether a layer is a valid layer for Pcbnew
 *
 * @param aLayer = Layer to test
 * @return true if aLayer is a layer valid in Pcbnew
 */
inline bool IsPcbLayer( LAYER_NUM aLayer )
{
    return aLayer >= F_Cu && aLayer < PCB_LAYER_ID_COUNT;
}

/**
 * Tests whether a layer is a copper layer.
 *
 * @param aLayerId = Layer  to test
 * @return true if aLayer is a valid copper layer
 */
inline bool IsCopperLayer( LAYER_NUM aLayerId )
{
    return aLayerId >= F_Cu && aLayerId <= B_Cu;
}

/**
 * Test whether a layer is a non copper layer.
 *
 * @param aLayerId = Layer to test
 * @return true if aLayer is a non copper layer
 */
inline bool IsNonCopperLayer( LAYER_NUM aLayerId )
{
    return aLayerId > B_Cu && aLayerId <= PCB_LAYER_ID_COUNT;
}

/**
 * Tests whether a layer is a copper layer, optionally including synthetic copper layers such
 * as LAYER_VIA_THROUGH, LAYER_PAD_FR, etc.
 *
 * @param aLayerId
 * @param aIncludeSyntheticCopperLayers
 * @return
 */
inline bool IsCopperLayer( LAYER_NUM aLayerId, bool aIncludeSyntheticCopperLayers )
{
    if( aIncludeSyntheticCopperLayers )
        return !IsNonCopperLayer( aLayerId );
    else
        return IsCopperLayer( aLayerId );
}

inline bool IsViaPadLayer( LAYER_NUM aLayer )
{
    return aLayer == LAYER_VIA_THROUGH
            || aLayer == LAYER_VIA_MICROVIA
            || aLayer == LAYER_VIA_BBLIND;
}

inline bool IsHoleLayer( LAYER_NUM aLayer )
{
    return aLayer == LAYER_VIA_HOLES
            || aLayer == LAYER_VIA_HOLEWALLS
            || aLayer == LAYER_PAD_PLATEDHOLES
            || aLayer == LAYER_PAD_HOLEWALLS
            || aLayer == LAYER_NON_PLATEDHOLES;
}

/**
 * Test whether a layer is a non copper and a non tech layer.
 *
 * @param aLayerId = Layer to test
 * @return true if aLayer is a user layer
 */
inline bool IsUserLayer( PCB_LAYER_ID aLayerId )
{
    return aLayerId >= Dwgs_User && aLayerId <= Eco2_User;
}

/*
   @todo Where does this comment actually belong?

   IMPORTANT: If a layer is not a front layer not necessarily is true
   the converse. The same hold for a back layer.
   So a layer can be:
   - Front
   - Back
   - Neither (internal or auxiliary)

   The check most frequent is for back layers, since it involves flips
*/


/**
 * Layer classification: check if it's a front layer
 */
inline bool IsFrontLayer( PCB_LAYER_ID aLayerId )
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
inline bool IsBackLayer( PCB_LAYER_ID aLayerId )
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
 * @return the layer number after flipping an item
 * some (not all) layers: external copper, and paired layers( Mask, Paste, solder ... )
 * are swapped between front and back sides
 * internal layers are flipped only if the copper layers count is known
 * @param aLayerId = the PCB_LAYER_ID to flip
 * @param aCopperLayersCount = the number of copper layers. if 0 (in fact if < 4 )
 *  internal layers will be not flipped because the layer count is not known
 */
PCB_LAYER_ID FlipLayer( PCB_LAYER_ID aLayerId, int aCopperLayersCount = 0 );

/**
 * Calculate the mask layer when flipping a footprint.
 *
 * BACK and FRONT copper layers, mask, paste, solder layers are swapped
 * internal layers are flipped only if the copper layers count is known
 * @param aMask = the LSET to flip
 * @param aCopperLayersCount = the number of copper layers. if 0 (in fact if < 4 )
 *  internal layers will be not flipped because the layer count is not known
 */
LSET FlipLayerMask( LSET aMask, int aCopperLayersCount = 0 );


/**
 * Returns a netname layer corresponding to the given layer.
 */
inline int GetNetnameLayer( int aLayer )
{
    if( IsCopperLayer( aLayer ) )
        return NETNAMES_LAYER_INDEX( aLayer );
    else if( aLayer == LAYER_PADS_TH )
        return LAYER_PAD_NETNAMES;
    else if( aLayer == LAYER_PAD_FR )
        return LAYER_PAD_FR_NETNAMES;
    else if( aLayer == LAYER_PAD_BK )
        return LAYER_PAD_BK_NETNAMES;
    else if( IsViaPadLayer( aLayer ) )
        return LAYER_VIA_NETNAMES;

    // Fallback
    return Cmts_User;
}

/**
 * Test whether a layer is a netname layer.
 *
 * @param aLayer = Layer to test
 * @return true if aLayer is a valid netname layer
 */
inline bool IsNetnameLayer( LAYER_NUM aLayer )
{
    return aLayer >= NETNAMES_LAYER_INDEX( F_Cu ) &&
           aLayer < NETNAMES_LAYER_ID_END;
}


inline bool IsZoneLayer( LAYER_NUM aLayer )
{
    return aLayer >= LAYER_ZONE_START && aLayer <= LAYER_ZONE_END;
}


inline bool IsDCodeLayer( int aLayer )
{
    return aLayer >= (GERBVIEW_LAYER_ID_START + GERBER_DRAWLAYERS_COUNT) &&
           aLayer < (GERBVIEW_LAYER_ID_START + (2 * GERBER_DRAWLAYERS_COUNT));
}


/**
 * Checks if the given layer is "net copper", meaning it is eligible for net coloring.
 *
 * @param aLayer is the layer to test
 * @return true if the layer is one that participates in net coloring
 */
inline bool IsNetCopperLayer( LAYER_NUM aLayer )
{
    static std::set<LAYER_NUM> netCopperLayers =
            {
                    LAYER_PAD_FR,
                    LAYER_PAD_BK,
                    LAYER_PADS_TH,
                    LAYER_PAD_HOLEWALLS,
                    LAYER_VIA_THROUGH,
                    LAYER_VIA_BBLIND,
                    LAYER_VIA_MICROVIA,
                    LAYER_VIA_HOLEWALLS
            };

    return IsCopperLayer( aLayer ) || netCopperLayers.count( aLayer );
}


PCB_LAYER_ID ToLAYER_ID( int aLayer );

#endif // LAYERS_ID_AND_VISIBILITY_H_
