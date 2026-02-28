/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include "string_any_map.h"


#include <array>
#include <cstdint>
#include <optional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>


namespace ALLEGRO
{

/**
 * The base class for all blocks in the main body of an Allegro file.
 *
 * This records metadata about the block, such as its type and offset in the file.
 *
 * In the file format, blocks do not seem to know their own length,
 * so it's very important that each block type gets this right, or the next block
 * will not be read properly and everything will fall apart.
 */
class BLOCK_BASE
{
public:
    BLOCK_BASE( uint8_t aBlockType, size_t aOffset ) :
        m_blockType( aBlockType ),
        m_offset( aOffset )
    {}

    virtual ~BLOCK_BASE() = default;

    uint8_t GetBlockType() const { return m_blockType; }
    size_t  GetOffset() const { return m_offset; }

    // If this block data has a key, return it, else 0
    uint32_t GetKey() const;

private:
    uint8_t m_blockType;
    size_t  m_offset;
};


template <typename T>
class BLOCK : public BLOCK_BASE
{
public:
    BLOCK( uint8_t aBlockType, size_t aOffset ) :
        BLOCK_BASE( aBlockType, aOffset )
    {}

    const T& GetData() const { return m_data; }
    T&       GetData() { return m_data; }

private:
    T m_data;
};


enum BLOCK_TYPE
{
    x1B_NET = 0x1B,
};


/**
 * The format of an Allego file.
 *
 * Allgro formats seem to be versioned per the magic, with the lowest
 * byte masked out (or at least there no known case of the lower
 * magic byte changing the format.
 *
 * But the version does not seem to be directly related to the
 * Allegro version (e.g. 17.2) itself.
 */
enum class FMT_VER
{
    V_UNKNOWN,
    V_PRE_V16, // Allegro versions before 16.0 (unsupported binary format)
    V_160, // Allegro 16.0, 0x00130000
    V_162, // Allegro 16.2  0x00130400
    V_164, // Allegro 16.4, 0x00130C00
    V_165, // Allegro 16.5, 0x00131000
    V_166, // Allegro 16.6, 0x00131500
    V_172, // Allegro 17.2, 0x00140400
    V_174, // Allegro 17.4, 0x00140900
    V_175, // Allegro 17.5, 0x00141500
    V_180, // Allegro 18.0, 0x00150000
};

constexpr bool operator>=( FMT_VER lhs, FMT_VER rhs )
{
    return static_cast<uint32_t>( lhs ) >= static_cast<uint32_t>( rhs );
}


template <typename T>
struct COND_FIELD_BASE
{
    using value_type = T;

    /**
     * Define this function in the derived class to determine if the field
     * exists in the given version of the file.
     */
    virtual bool exists( FMT_VER ver ) const = 0;

    // Access to the value (use std::optional-like semantics)
    T&       value() { return *m_Value; }
    const T& value() const { return *m_Value; }
    bool     has_value() const { return m_Value.has_value(); }

    const T& value_or( const T& aDefault ) const { return has_value() ? value() : aDefault; }

    T&       operator*() { return *m_Value; }
    const T& operator*() const { return *m_Value; }

    T*       operator->() { return m_Value.operator->(); }
    const T* operator->() const { return m_Value.operator->(); }

    // Assigmnent operator
    COND_FIELD_BASE& operator=( const T& value )
    {
        m_Value = value;
        return *this;
    }
    COND_FIELD_BASE& operator=( T&& value )
    {
        m_Value = std::move( value );
        return *this;
    }

private:
    std::optional<T> m_Value;
};


/**
 * This is a conditional field that only exists in versions of a file
 * of or above a certain version.
 */
template <FMT_VER MinVersion, typename T>
struct COND_GE : public COND_FIELD_BASE<T>
{
    constexpr bool exists( FMT_VER ver ) const override { return ver >= MinVersion; }

    using COND_FIELD_BASE<T>::operator=;
};


/**
 * This is a conditional field that only exists in versions of a file
 * less than a certain version.
 */
template <FMT_VER MaxVersion, typename T>
struct COND_LT : public COND_FIELD_BASE<T>
{
    constexpr bool exists( FMT_VER ver ) const override { return ver < MaxVersion; }

    using COND_FIELD_BASE<T>::operator=;
};


/**
 * This is a conditional field that only exists in versions of a file
 * less than a certain version and greater than or equal to a certain version.
 */
template <FMT_VER GEVersion, FMT_VER LTVersion, typename T>
struct COND_GE_LT : public COND_FIELD_BASE<T>
{
    constexpr bool exists( FMT_VER ver ) const override { return ver >= GEVersion && ver < LTVersion; }

    using COND_FIELD_BASE<T>::operator=;
};


enum BOARD_UNITS
{
    IMPERIAL = 0x01,
    METRIC = 0x03,
};


/**
 * Allegro files start with this header. It is mostly full of a lot
 * of linked lists, but also a few key parameters of the file such
 * as units.
 *
 * It does not seem to have things that vary between file versions.
 */
struct FILE_HEADER
{
    struct LAYER_MAP_ENTRY
    {
        uint32_t m_A;
        uint32_t m_LayerList0x2A;
    };

    /**
     * This is apparently some kind of linked list that chains though subsets
     * objects in the file. It's not clear if this has any utility for KiCad, as we
     * can just keep a map of object types as we go.
     */
    struct LINKED_LIST
    {
        uint32_t m_Tail;
        uint32_t m_Head;
    };

    uint32_t                m_Magic;

    uint32_t                m_Unknown1a;        // 0x03
    uint32_t                m_FileRole;         // 0x01 (.brd) or 0x02 (.dra) (?)
    uint32_t                m_Unknown1b;        // 0x03
    uint32_t                m_WriterProgram;    // 0x09 (Allegro) or 0x130000 (DB Doctor) (?)

    uint32_t                m_ObjectCount;

    uint32_t                m_UnknownMagic;     // This is always 0x000a0d0a?
    uint32_t                m_UnknownFlags;     // This looks like flags: 0x01000000, 0x0400000, 0x06000000

    // In this block of 7 uint32s, it seems they have very different meanings pre and post v18

    // Pre v18, these are all unknown
    // - 3 and 4 are the same.
    // - 5 and 6 arer of a similar size
    COND_LT<FMT_VER::V_180, std::array<uint32_t, 7>> m_Unknown2_preV18;

    // V18
    COND_GE<FMT_VER::V_180, uint32_t> m_Unknown2a_V18;          // Looks like an 'end pointer' like 0x27_end
    COND_GE<FMT_VER::V_180, uint32_t> m_Unknown2b_V18;          // 0x00
    COND_GE<FMT_VER::V_180, uint32_t> m_0x27_End_V18;
    COND_GE<FMT_VER::V_180, uint32_t> m_Unknown2d_V18;          // 0x00
    COND_GE<FMT_VER::V_180, uint32_t> m_Unknown2e_V18;          // 25? (perhaps layer count?)
    COND_GE<FMT_VER::V_180, uint32_t> m_StringCount_V18;
    COND_GE<FMT_VER::V_180, uint32_t> m_Unknown2g_V18;          // 0x00

    // V180 has 6 additional linked lists in the header
    // 5 of them are at the start
    COND_GE<FMT_VER::V_180, LINKED_LIST> m_LL_V18_1;
    COND_GE<FMT_VER::V_180, LINKED_LIST> m_LL_V18_2;
    COND_GE<FMT_VER::V_180, LINKED_LIST> m_LL_V18_3;
    COND_GE<FMT_VER::V_180, LINKED_LIST> m_LL_V18_4;
    COND_GE<FMT_VER::V_180, LINKED_LIST> m_LL_V18_5;

    // Linked lists grouping top-level elements by type
    LINKED_LIST m_LL_0x04;              // Net assignments
    LINKED_LIST m_LL_0x06;              // Component definitions
    LINKED_LIST m_LL_0x0C;              // Pin definitions
    LINKED_LIST m_LL_Shapes;            // Shape segments (0x0E) and shapes (0x28)
    LINKED_LIST m_LL_0x14;              // Graphics
    LINKED_LIST m_LL_0x1B_Nets;         // Nets
    LINKED_LIST m_LL_0x1C;              // Padstacks
    LINKED_LIST m_LL_0x24_0x28;         // Rects and shapes
    LINKED_LIST m_LL_Unknown1;
    LINKED_LIST m_LL_0x2B;              // Footprint definitions
    LINKED_LIST m_LL_0x03_0x30;         // Fields (0x03) and string wrappers (0x30)
    LINKED_LIST m_LL_0x0A;              // DRC elements
    LINKED_LIST m_LL_0x1D_0x1E_0x1F;    // Constraint sets, SI models, padstack dims
    LINKED_LIST m_LL_Unknown2;
    LINKED_LIST m_LL_0x38;              // Films
    LINKED_LIST m_LL_0x2C;              // Tables
    LINKED_LIST m_LL_0x0C_2;            // Secondary pin definitions
    LINKED_LIST m_LL_Unknown3;

    // For some reason the 0x35 extents are recorded in the header
    COND_LT<FMT_VER::V_180, uint32_t> m_0x35_Start_preV18;
    COND_LT<FMT_VER::V_180, uint32_t> m_0x35_End_preV18;

    COND_GE<FMT_VER::V_180, LINKED_LIST> m_LL_Unknown5_V18;
    LINKED_LIST m_LL_0x36;
    COND_LT<FMT_VER::V_180, LINKED_LIST> m_LL_Unknown5_preV18;

    LINKED_LIST m_LL_Unknown6;
    LINKED_LIST m_LL_0x0A_2;

    COND_LT<FMT_VER::V_180, uint32_t> m_Unknown3;

    COND_GE<FMT_VER::V_180, LINKED_LIST> m_LL_V18_6;

    COND_GE<FMT_VER::V_180, uint32_t> m_0x35_Start_V18;
    COND_GE<FMT_VER::V_180, uint32_t> m_0x35_End_V18;

    // Fixed length string field
    std::array<char, 60> m_AllegroVersion;

    uint32_t m_Unknown4;

    uint32_t m_MaxKey;

    COND_LT<FMT_VER::V_180, std::array<uint32_t, 17>> m_Unknown5_preV18;
    COND_GE<FMT_VER::V_180, std::array<uint32_t, 9>>  m_Unknown5_V18;

    BOARD_UNITS m_BoardUnits;
    // 3 empty bytes here?

    uint32_t m_Unknown6;
    COND_LT<FMT_VER::V_180, uint32_t> m_Unknown7;

    // The end of the 0x27 object(?)
    // In V18, this is relocated to m_0x27_End_V18
    COND_LT<FMT_VER::V_180, uint32_t> m_0x27_End_preV18;

    uint32_t m_Unknown8;

    COND_LT<FMT_VER::V_180, uint32_t> m_StringCount_preV18;

    std::array<uint32_t, 50> m_Unknown9;

    uint32_t m_Unknown10a;  // Often 0x000500FF
    uint32_t m_Unknown10b;  // Similar to 0xFFA60000
    uint32_t m_Unknown10c;  // Usually 0x01

    // E.g. 1000 for mils
    uint32_t m_UnitsDivisor;

    /**
     * The layer maps is a list of groups of two numbers.
     *
     * All files so far seem to have 25 entries, but unsure if that
     * is a universal value. But there's no obvious nearby value of '25'.
     */
    std::array<LAYER_MAP_ENTRY, 25> m_LayerMap;

    uint32_t GetStringsCount() const
    {
        if( m_StringCount_V18.has_value() )
            return m_StringCount_V18.value();

        return m_StringCount_preV18.value();
    }

    uint32_t Get_0x27_End() const
    {
        if( m_0x27_End_V18.has_value() )
            return m_0x27_End_V18.value();

        return m_0x27_End_preV18.value();
    }

    const LINKED_LIST& GetUnknown5() const
    {
        if( m_LL_Unknown5_V18.has_value() )
            return m_LL_Unknown5_V18.value();

        return m_LL_Unknown5_preV18.value();
    }
};


struct LAYER_INFO
{
    enum CLASS
    {
        BOARD_GEOMETRY       = 0x01,
        COMPONENT_VALUE      = 0x02,
        DEVICE_TYPE          = 0x03,
        DRAWING_FORMAT       = 0x04,
        DRC_ERROR            = 0x05,
        ETCH                 = 0x06,
        MANUFACTURING        = 0x07,
        ANALYSIS             = 0x08,
        PACKAGE_GEOMETRY     = 0x09,
        PACKAGE_KEEPIN       = 0x0A,
        PACKAGE_KEEPOUT      = 0x0B,
        PIN                  = 0x0C,
        REF_DES              = 0x0d,
        ROUTE_KEEPIN         = 0x0e,
        ROUTE_KEEPOUT        = 0x0f,
        TOLERANCE            = 0x10,
        USER_PART_NUMBER     = 0x11,
        VIA_CLASS            = 0x12,
        VIA_KEEPOUT          = 0x13,
        ANTI_ETCH            = 0x14,
        BOUNDARY             = 0x15,
    };

    /**
     * The second byte in a CLASS:SUBCLASS pair.
     *
     * THe same meanings can have different subclass codes in different classes
     */
    enum SUBCLASS
    {
        // BOARD_GEOMETRY
        BGEOM_OUTLINE                 = 0xEA,
        BGEOM_CONSTRAINT_AREA         = 0xEB,
        BGEOM_OFF_GRID_AREA           = 0xEC,
        BGEOM_SOLDERMASK_BOTTOM       = 0xED,
        BGEOM_SOLDERMASK_TOP          = 0xEE,
        BGEOM_ASSEMBLY_DETAIL         = 0xEF,
        BGEOM_SILKSCREEN_BOTTOM       = 0xF0,
        BGEOM_SILKSCREEN_TOP          = 0xF1,
        BGEOM_SWITCH_AREA_BOTTOM      = 0xF2,
        BGEOM_SWITCH_AREA_TOP         = 0xF3,
        BGEOM_BOTH_ROOMS              = 0xF4,
        BGEOM_BOTTOM_ROOM             = 0xF5,
        BGEOM_TOP_ROOM                = 0xF6,
        BGEOM_PLACE_GRID_BOTTOM       = 0xF7,
        BGEOM_PLACE_GRID_TOP          = 0xF8,
        BGEOM_DIMENSION               = 0xF9,
        BGEOM_TOOLING_CORNERS         = 0xFA,
        BGEOM_ASSEMBLY_NOTES          = 0xFB,
        BGEOM_PLATING_BAR             = 0xFC,

        // COMPONENT_VALUE / DEVICE_TYPE / USER_PART_NUMBER
        // REF_DES / TOLERANCE
        DISPLAY_BOTTOM               = 0xF8,
        DISPLAY_TOP                  = 0xF9,
        SILKSCREEN_BOTTOM            = 0xFA,
        SILKSCREEN_TOP               = 0xFB,
        ASSEMBLY_BOTTOM              = 0xFC,
        ASSEMBLY_TOP                 = 0xFD,

        // ANALYSIS
        ANALYSIS_PCB_TEMPERATURE     = 0xF8,
        ANALYSIS_HIGH_ISOCONTOUR     = 0xF9,
        ANALYSIS_MEDIUM3_ISOCONTOUR  = 0xFA,
        ANALYSIS_MEDIUM2_ISOCONTOUR  = 0xFB,
        ANALYSIS_MEDIUM1_ISOCONTOUR  = 0xFC,
        ANALYSIS_LOW_ISOCONTOUR      = 0xFD,

        // DRAWING_FORMAT
        DFMT_REVISION_DATA           = 0xF9,
        DFMT_REVISION_BLOCK          = 0xFA,
        DFMT_TITLE_DATA              = 0xFB,
        DFMT_TITLE_BLOCK             = 0xFC,
        DFMT_OUTLINE                 = 0xFD,

        // PACKAGE_GEOMETRY
        DFA_BOUND_BOTTOM             = 0xEE,
        DFA_BOUND_TOP                = 0xEF,
        PGEOM_DISPLAY_BOTTOM         = 0xF1,
        PGEOM_DISPLAY_TOP            = 0xF2,
        PGEOM_SOLDERMASK_BOTTOM      = 0xF3,
        PGEOM_SOLDERMASK_TOP         = 0xF4,
        PGEOM_BODY_CENTER            = 0xF5,
        PGEOM_SILKSCREEN_BOTTOM      = 0xF6,
        PGEOM_SILKSCREEN_TOP         = 0xF7,
        PGEOM_PAD_STACK_NAME         = 0xF8,
        PGEOM_PIN_NUMBER             = 0xF9,
        PGEOM_PLACE_BOUND_BOTTOM     = 0xFA,
        PGEOM_PLACE_BOUND_TOP        = 0xFB,
        PGEOM_ASSEMBLY_BOTTOM        = 0xFC,
        PGEOM_ASSEMBLY_TOP           = 0xFD,

        // MANUFACTURING
        MFR_XSECTION_CHART           = 0xF0,
        MFR_NO_PROBE_BOTTOM          = 0xF1,
        MFR_NO_PROBE_TOP             = 0xF2,
        MFR_AUTOSILK_BOTTOM          = 0xF3,
        MFR_AUTOSILK_TOP             = 0xF4,
        MFR_PROBE_BOTTOM             = 0xF5,
        MFR_PROBE_TOP                = 0xF6,
        MFR_NCDRILL_FIGURE           = 0xF7,
        MFR_NCDRILL_LEGEND           = 0xF8,
        MFR_NO_GLOSS_INTERNAL        = 0xF9,
        MFR_NO_GLOSS_BOTTOM          = 0xFA,
        MFR_NO_GLOSS_TOP             = 0xFB,
        MFR_NO_GLOSS_ALL             = 0xFC,
        MFR_PHOTOPLOT_OUTLINE        = 0xFD,
    };

    uint8_t m_Class;
    uint8_t m_Subclass;

    bool operator==( const LAYER_INFO& ) const = default;
};


/**
 * Arc segment used in tracks, zone outlines, and shape boundaries. The sweep direction
 * is encoded in m_SubType bit 6 (0x40 = clockwise). Center and radius are stored as
 * word-swapped IEEE 754 doubles (ReadAllegroFloat).
 */
struct BLK_0x01_ARC
{
    uint8_t  m_UnknownByte;
    uint8_t  m_SubType;     ///< Bit 6 (0x40) = clockwise direction
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_Parent;
    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown6;

    uint32_t m_Width;

    int32_t m_StartX;
    int32_t m_StartY;
    int32_t m_EndX;
    int32_t m_EndY;

    double m_CenterX; // Center
    double m_CenterY;
    double m_Radius;

    std::array<int32_t, 4> m_BoundingBoxCoords;
};


/**
 * Field/property references with variable-typed substructs. Used for constraint set names,
 * net properties, trace widths, and schematic cross-references.
 */
struct BLK_0x03_FIELD
{
    struct SUB_0x6C
    {
        uint32_t              m_NumEntries;
        std::vector<uint32_t> m_Entries;
    };

    struct SUB_0x70_0x74
    {
        uint16_t              m_X0;
        uint16_t              m_X1;
        std::vector<uint32_t> m_Entries;
    };

    struct SUB_0xF6
    {
        std::array<uint32_t, 20> m_Entries;
    };

    uint16_t m_Hdr1;

    uint32_t m_Key;
    uint32_t m_Next;


    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint8_t  m_SubType;
    uint8_t  m_Hdr2;
    uint16_t m_Size;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    std::variant<uint32_t, std::array<uint32_t, 2>, std::string, SUB_0x6C, SUB_0x70_0x74, SUB_0xF6> m_Substruct;
};

/**
 * Hdr1 keys for field roles
 */
enum FIELD_KEYS
{
    LOGICAL_PATH = 0x37,
    MIN_LINE_WIDTH = 0x55,
    NET_SCHEDULE = 0x77,          ///< Net class/schedule assignment
    MAX_LINE_WIDTH = 0x173,
    MIN_NECK_WIDTH = 0x5c,
    MAX_NECK_LENGTH = 0x1fb,
    PHYS_CONSTRAINT_SET = 0x1a0,  ///< Physical Constraint Set assignment
};


/**
 * Net assignment linking a net (0x1B) to its member objects. Each NET has a chain of
 * 0x04 blocks, each pointing to a connected item (0x05 track, 0x32 placed pad, 0x33 via,
 * or 0x28 shape for copper fills).
 */
struct BLK_0x04_NET_ASSIGNMENT
{
    uint8_t  m_Type;
    uint16_t m_R;
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_Net;
    uint32_t m_ConnItem;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown;
};


/**
 * Track segment container. Each track has a layer, a net assignment (via 0x04), and a
 * linked list of line/arc segments starting at m_FirstSegPtr (0x15/0x16/0x17 lines or
 * 0x01 arcs). Also used for zone fill shapes (0x28) on the net assignment chain.
 */
struct BLK_0x05_TRACK
{
    LAYER_INFO m_Layer;

    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_NetAssignment;
    uint32_t m_UnknownPtr1;
    uint32_t m_Unknown2;
    uint32_t m_Unknown3;
    uint32_t m_UnknownPtr2a;
    uint32_t m_UnknownPtr2b;
    uint32_t m_Unknown4;
    uint32_t m_UnknownPtr3a;
    uint32_t m_UnknownPtr3b;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown5a;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown5b;

    uint32_t m_FirstSegPtr;
    uint32_t m_UnknownPtr5;
    uint32_t m_Unknown6;
};


/**
 * Component/symbol definitions. Links to instances (0x07), function slots (0x0F),
 * and pin numbers (0x08).
 */
struct BLK_0x06_COMPONENT
{
    uint32_t m_Key;

    // Pointer to the next BLK_0x06_COMPONENT
    uint32_t m_Next;
    // Pointer to COMP_DEVICE_TYPE string
    uint32_t m_CompDeviceType;
    // Pointer to SYM_NAME string
    uint32_t m_SymbolName;
    // Points to 0x07, first instance
    uint32_t m_FirstInstPtr;
    // Points to 0x0F, function slot
    uint32_t m_PtrFunctionSlot;
    // Points to 0x08, pin number
    uint32_t m_PtrPinNumber;

    // Points to 0x03, first 'field'
    uint32_t m_Fields;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;
};


/**
 * Component instance reference data. Links a placed footprint (0x2D) to its reference
 * designator string, function instance (0x10), and placed pad list (0x32). One 0x07
 * exists per placed component.
 */
struct BLK_0x07_COMPONENT_INST
{
    uint32_t m_Key;

    uint32_t m_Next;

    COND_GE<FMT_VER::V_172, uint32_t> m_UnknownPtr1;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;

    uint32_t m_FpInstPtr;

    COND_LT<FMT_VER::V_172, uint32_t> m_Unknown4;

    uint32_t m_RefDesStrPtr;
    uint32_t m_FunctionInstPtr;
    uint32_t m_X03Ptr; // 0x03 or null
    uint32_t m_Unknown5;
    uint32_t m_FirstPadPtr; // 0x32 or null
};


/**
 * Pin number within a component. Links to pin name (0x11) and is chained from the
 * component definition (0x06). The pin number string is version-dependent (m_StrPtr
 * for V172+, m_StrPtr16x for pre-V172).
 */
struct BLK_0x08_PIN_NUMBER
{
    uint8_t  m_Type;
    uint16_t m_R;
    uint32_t m_Key;

    COND_GE<FMT_VER::V_172, uint32_t> m_Previous;
    COND_LT<FMT_VER::V_172, uint32_t> m_StrPtr16x;

    uint32_t m_Next;

    COND_GE<FMT_VER::V_172, uint32_t> m_StrPtr;

    ///< Pointer to 0x11 PIN_NAME object
    uint32_t m_PinNamePtr;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint32_t m_Ptr4;

    uint32_t GetStrPtr() const
    {
        return m_StrPtr.value_or( m_StrPtr16x.value_or( 0 ) );
    }
};


/**
 * Intermediate link between copper fills and their parent shapes. Appears in fill-to-shape
 * resolution chains. Not directly imported.
 */
struct BLK_0x09_FILL_LINK
{
    uint32_t m_Key;

    std::array<uint32_t, 4> m_UnknownArray;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint32_t m_UnknownPtr1;
    uint32_t m_UnknownPtr2;
    uint32_t m_Unknown2;
    uint32_t m_UnknownPtr3;
    uint32_t m_UnknownPtr4;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown3;
};


/**
 * 0x0A objects represent DRC (Design Rule Check) elements.
 */
struct BLK_0x0A_DRC
{
    uint8_t    m_T;
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;
    uint32_t   m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    std::array<int32_t, 4>  m_Coords;
    std::array<uint32_t, 4> m_Unknown4;
    std::array<uint32_t, 5> m_Unknown5;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown6;
};


/**
 * Pin definition with shape type, drill character, coordinates, and size. Contains
 * version-dependent fields for the pad shape and drill character encoding.
 */
struct BLK_0x0C_PIN_DEF
{
    enum MARKER_SHAPE
    {
        // These are in the same order as the pad shapes, at least for the 'simple' shapes
        CIRCLE = 0x02,
        OCTAGON = 0x03,
        CROSS = 0x04,
        SQUARE = 0x05,
        RECTANGLE = 0x06,
        DIAMOND = 0x07,
        PENTAGON = 0x0a,
        OBLONG_X = 0x0b,
        OBLONG_Y = 0x0c,
        HEXAGON_X = 0x0f,
        HEXAGON_Y = 0x10,
        TRIANGLE = 0x12,
    };

    uint8_t    m_T;
    LAYER_INFO m_Layer;
    uint32_t m_Key;
    uint32_t m_Next;

    uint32_t m_Unknown1;
    uint32_t m_Unknown2;

    COND_LT<FMT_VER::V_172, uint8_t>  m_Shape;
    COND_LT<FMT_VER::V_172, uint8_t>  m_DrillChar;
    COND_LT<FMT_VER::V_172, uint16_t> m_UnknownPadding; // or drill char?

    COND_GE<FMT_VER::V_172, uint32_t> m_Shape16x;
    COND_GE<FMT_VER::V_172, uint32_t> m_DrillChars;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown_16x;

    uint32_t m_Unknown4;

    std::array<int32_t, 2> m_Coords;
    std::array<int32_t, 2> m_Size;

    std::array<uint32_t, 3> m_UnknownArray;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown6;

    uint32_t GetShape() const
    {
        return m_Shape16x.value_or( m_Shape.value_or( 0 ) );
    }
};


/**
 * Pad geometry and placement in board-absolute coordinates. References a padstack (0x1C)
 * for shape/drill definitions. Coordinates and rotation are board-absolute; for KiCad
 * footprint-local space, subtract the parent footprint's position and rotation.
 */
struct BLK_0x0D_PAD
{
    uint32_t m_Key;
    uint32_t m_NameStrId;
    uint32_t m_Next;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown1;

    int32_t m_CoordsX;  ///< Board coordinates. Use SetFPRelativePosition() for KiCad FP-local space.
    int32_t m_CoordsY;  ///< Board coordinates. Use SetFPRelativePosition() for KiCad FP-local space.

    uint32_t m_PadStack;
    uint32_t m_Unknown2;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;

    uint32_t m_Flags;
    uint32_t m_Rotation;  ///< Board-absolute millidegrees. Subtract footprint rotation for FP-local orientation.
};


/**
 * Rectangular shape.
 */
struct BLK_0x0E_RECT
{
    uint8_t  m_T;
    LAYER_INFO m_Layer;
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_FpPtr;

    uint32_t m_Unknown1;
    uint32_t m_Unknown2;
    uint32_t m_Unknown3;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown4;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown5;

    std::array<int32_t, 4> m_Coords;

    std::array<uint32_t, 3> m_UnknownArr;
    /// Rotation in millidegrees
    uint32_t m_Rotation;
};


/**
 * Function slot in a multi-slot component (e.g. a quad op-amp has 4 slots). Contains
 * the slot name string, device type, and links to the parent component (0x06) and
 * pin name list (0x11).
 */
struct BLK_0x0F_FUNCTION_SLOT
{
    uint32_t m_Key;

    uint32_t m_SlotName;

    std::array<char, 32> m_CompDeviceType;

    uint32_t m_Ptr0x06;
    uint32_t m_Ptr0x11;

    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown3;
};


/**
 * Function instance linking a component instance (0x07) to its schematic function,
 * pin cross-references (0x12), and function slots (0x0F). Count matches 0x07 objects
 * one-to-one.
 */
struct BLK_0x10_FUNCTION_INST
{
    uint32_t m_Key;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint32_t m_ComponentInstPtr;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown2;

    uint32_t m_PtrX12;
    uint32_t m_Unknown3;
    uint32_t m_FunctionName;
    uint32_t m_Slots; // 0x0F?
    uint32_t m_Fields;     // Presumably a pointer to a string
};


/**
 * Pin name within a component, linked from function slots (0x0F). Maps pin names to pin
 * numbers (0x08) for schematic-to-layout cross-referencing.
 */
struct BLK_0x11_PIN_NAME
{
    uint8_t  m_Type;
    uint16_t m_R;
    uint32_t m_Key;
    ///< Pointer to pin name string
    uint32_t m_PinNameStrPtr;
    ///< Pointer to next 0x11 PIN_NAME object or 0x0F SLOT
    uint32_t m_Next;
    ///< Pointer to 0x08 PIN_NUMBER object
    uint32_t m_PinNumberPtr;
    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown2;
};


/**
 * Cross-reference between objects. Exact semantics not fully understood; appears in
 * component and function instance chains.
 */
struct BLK_0x12_XREF
{
    uint8_t  m_Type;
    uint16_t m_R;
    uint32_t m_Key;
    uint32_t m_Ptr1;
    uint32_t m_Ptr2;
    uint32_t m_Ptr3;
    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_165, uint32_t> m_Unknown2;
    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown3;
};


/**
 * Graphics container holding a chain of line segments and arcs. Each 0x14 has a layer,
 * a parent pointer (usually a footprint 0x2D or board-level object), and a head pointer
 * to the first segment (0x15/0x16/0x17 or 0x01 arc).
 */
struct BLK_0x14_GRAPHIC
{
    uint8_t    m_Type;
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;
    uint32_t   m_Parent;
    uint32_t   m_Flags;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    uint32_t m_SegmentPtr;
    uint32_t m_Ptr0x03;
    uint32_t m_Ptr0x26;
};


/**
 * 0x15 , 0x16, 0x17 are segments:
 *
 *  - 0x15: horizontal
 *  - 0x16: not horizontal or vertical
 *  - 0x17: vertical
 */
struct BLK_0x15_16_17_SEGMENT
{
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_Parent;
    uint32_t m_Flags;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    uint32_t m_Width;

    int32_t m_StartX;
    int32_t m_StartY;
    int32_t m_EndX;
    int32_t m_EndY;
};

/**
 * 0x1B objects are nets.
 *
 * They have names and pointers to various other objects.
 */
struct BLK_0x1B_NET
{
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_NetName;

    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    uint32_t m_Type;

    uint32_t m_Assignment;
    uint32_t m_Ratline;
    ///< Pointer to first 0x03 FIELD object or null
    uint32_t m_FieldsPtr;
    uint32_t m_MatchGroupPtr; ///< Diff pair / match group pointer (0x26 or 0x2C)
    uint32_t m_ModelPtr;
    uint32_t m_UnknownPtr4;
    uint32_t m_UnknownPtr5;
    uint32_t m_UnknownPtr6;
};


/**
 * The type of the padstack.
 *
 * This seems a little uncertain in places (there seems to be 2 codes for SMD for example)
 */
enum class PAD_TYPE
{
    THROUGH_VIA,
    VIA,
    SMD_PIN,
    SLOT,
    NPTH,
};


/**
 * Substruct in a padstack object. Each component is one slot in the padstack, representing
 * either a fixed technical layer or a per-copper-layer pad/antipad/thermal relief.
 */
struct PADSTACK_COMPONENT
{
    enum TYPE
    {
        TYPE_NULL = 0x00,
        TYPE_CIRCLE = 0x02,
        TYPE_OCTAGON = 0x03,
        TYPE_CROSS = 0x04,
        TYPE_SQUARE = 0x05,
        TYPE_RECTANGLE = 0x06,
        TYPE_DIAMOND = 0x07,
        TYPE_PENTAGON = 0x0a,
        TYPE_OBLONG_X = 0x0b,
        TYPE_OBLONG_Y = 0x0c,
        TYPE_HEXAGON_X = 0x0f,
        TYPE_HEXAGON_Y = 0x10,
        TYPE_TRIANGLE = 0x12,
        TYPE_SHAPE_SYMBOL = 0x16,
        TYPE_FLASH = 0x17,
        TYPE_DONUT = 0x19,
        TYPE_ROUNDED_RECTANGLE = 0x1b,
        TYPE_CHAMFERED_RECTANGLE = 0x1c,
        TYPE_NSIDED_POLYGON = 0x1e,
        TYPE_APERTURE_EXT = 0xee,
    };

    uint8_t m_Type;
    uint8_t m_UnknownByte1;
    uint8_t m_UnknownByte2;
    uint8_t m_UnknownByte3;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    int32_t m_W;
    int32_t m_H;

    COND_GE<FMT_VER::V_172, int16_t> m_Z1;

    int32_t m_X3;
    int32_t m_X4;

    COND_GE<FMT_VER::V_172, int16_t> m_Z;

    /**
     * Seems to point to various things:
     *
     * * 0x0F objects when the type is 0x06
     * * 0x28 objects when the type is 0x16
     */
    uint32_t m_StrPtr;

    // In versions < 17.2, seems to be not present in the last entry.
    std::optional<uint32_t> m_Z2;
};


/**
 * Padstack definition containing drill dimensions and a table of per-layer pad/antipad/thermal
 * components. The component table has fixed technical layer slots (solder mask, paste mask, etc.)
 * followed by per-copper-layer groups. Drill location is version-dependent (m_Drill for pre-V172,
 * m_DrillArr for V172+). Slot dimensions for oblong drills are stored as (primary, secondary)
 * not (X, Y), requiring orientation correction based on the copper pad aspect ratio.
 */
struct BLK_0x1C_PADSTACK
{
    uint8_t m_UnknownByte1;

    /**
     * The number of something. Drives the size of an array (with a multiplier).
     */
    uint8_t  m_N;
    uint8_t  m_UnknownByte2;
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_PadStr;

    /**
     * In < V172, this is the drill diameter in internal coordinates.
     * In >= V172, the drill diameter moved to m_DrillArr[DRILL_DIAMETER].
     */
    uint32_t m_Drill;
    uint32_t m_Unknown2;
    uint32_t m_PadPath;

    COND_LT<FMT_VER::V_172, uint32_t> m_Unknown3;
    COND_LT<FMT_VER::V_172, uint32_t> m_Unknown4;
    COND_LT<FMT_VER::V_172, uint32_t> m_Unknown5;
    COND_LT<FMT_VER::V_172, uint32_t> m_Unknown6;

    PAD_TYPE m_Type;

    // Not sure if this is really a substruct
    // Only lower 4 bits (top 4 are type)
    uint8_t m_A;
    uint8_t m_B;
    uint8_t m_C;
    uint8_t m_D;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown7;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown8;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown9;

    COND_LT<FMT_VER::V_172, uint16_t> m_Unknown10;

    uint16_t m_LayerCount;

    // Presumably the counterpart to m_Unknown10
    // Or just padding (?)
    COND_GE<FMT_VER::V_172, uint16_t> m_Unknown11;

    /**
     * In >= V172, elements [4] and [7] hold drill dimensions:
     *   [4] = drill diameter (or width for oblong drills)
     *   [7] = drill height for oblong drills (0 for round)
     * All values are in internal coordinate units (mils * divisor).
     */
    std::array<uint32_t, 8> m_DrillArr;

    /**
     * In V172+, elements [0] and [3] hold the true slot outline dimensions (X, Y)
     * in internal coordinate units. For routed slots (round drill bit routed along a path),
     * m_DrillArr holds only the bit diameter while this array holds the full slot envelope.
     * For punched oblong drills, these values match m_DrillArr[4] and [7].
     */
    COND_GE<FMT_VER::V_172, std::array<uint32_t, 28>> m_SlotAndUnknownArr;

    COND_GE_LT<FMT_VER::V_165, FMT_VER::V_172, std::array<uint32_t, 8>> m_UnknownArr8_2;

    /**
     * V180 inserts 8 extra uint32s between the fixed arrays and the component table.
     * Despite the name, this is read before the components, not after.
     */
    COND_GE<FMT_VER::V_180, std::array<uint32_t, 8>> m_V180Trailer;

    /**
     * Fixed slot indices in the component table.
     *
     * The fixed slots come before the per-layer copper entries.
     * V<172 has 10 fixed slots, V>=172 has 21.
     *
     * All fixed slots are technical layers (solder mask, paste mask, film mask,
     * assembly variant, etc). The exact slot-to-layer mapping is version-dependent
     * and not fully contiguous. Verified mappings from WORKLOG reverse engineering:
     *
     * V<172 (10 fixed):
     *   Slot 0  = ~TSM (top solder mask)
     *   Slot 5  = ~TPM (top paste mask)
     *   Slot 7  = ~TFM (top film mask)
     *
     * V>=172 (21 fixed):
     *   Slot 14 = ~TSM (top solder mask)
     */
    enum SLOTS
    {
        // V<172 verified slots
        SOLDERMASK_TOP_V16X = 0,
        PASTEMASK_TOP_V16X  = 5,
        FILMMASK_TOP_V16X   = 7,

        // V>=172 verified slots
        SOLDERMASK_TOP_V17X = 14,
    };

    /**
     * Component table layer offsets
     * In the component table's layer section, each layer has 3 or 4 slots, depending on version.
     */
    enum LAYER_COMP_SLOT
    {
        // First slot: Antipad
        ANTIPAD = 0,
        // Thermal relief shape
        THERMAL_RELIEF = 1,
        // Pad shape
        PAD = 2,
        // Unsure what this layer component slot is
        // But I suspect it's keepout, as that was added in V172.
        UNKNOWN_GE_V172 = 3,
    };

    /**
     * Collection of components that make up the padstack.
     *
     * The number of components appears to be fixed by version:
     *
     * *  < 17.2: 10 + layer_count * 3
     * * >= 17.2: 21 + layer_count * 4
     *
     * The first 10/21 components seem to be a fixed set of technical layers.
     *
     * Then, a set of groups of 3/4 components for each layer.
     */
    std::vector<PADSTACK_COMPONENT> m_Components;

    /**
     * How many of the entries are fixed roles (after this is n*layers)
     */
    size_t m_NumFixedCompEntries;
    size_t m_NumCompsPerLayer;

    /**
     * Some structure of m_N * 8 or 10:
     *
     * *  < 17.2: 8
     * * >= 17.2: 10
     */
    std::vector<uint32_t> m_UnknownArrN;
};


/**
 * Physical constraint sets containing trace width, clearance, and routing rules.
 */
struct BLK_0x1D_CONSTRAINT_SET
{
    uint32_t m_Key;
    uint32_t m_Next;         ///< Linked list next pointer (used by LL_WALKER)
    uint32_t m_NameStrKey;   ///< String table key for constraint set name
    uint32_t m_FieldPtr;     ///< Pointer to 0x03 FIELD with CS name (fallback when m_NameStrKey fails)
    uint16_t m_SizeA;
    uint16_t m_SizeB;

    /**
     * Per-copper-layer dimension values, 14 x int32 per record. Count equals board copper
     * layer count. V172+: f[1]=line_width, f[4]=clearance. Pre-V172: f[0]=line_width,
     * f[1]=spacing.
     */
    std::vector<std::array<uint8_t, 56>> m_DataB;
    /**
     * Records contain ASCII padstack/via name strings (null-terminated at offset 4)
     * and file path references. Size is m_SizeA * 256.
     */
    std::vector<std::array<uint8_t, 256>> m_DataA;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown4;
};


/**
 * Signal integrity and simulation model data (IBIS netlists). m_String contains ASCII
 * netlist text. Not imported.
 */
struct BLK_0x1E_SI_MODEL
{
    uint8_t  m_Type;
    uint16_t m_T2;
    uint32_t m_Key;
    uint32_t m_Next;         ///< Linked list next pointer (used by LL_WALKER)

    // Versioning seems unsure here
    // At least it is in Kinoma (V_164)
    COND_GE<FMT_VER::V_164, uint16_t> m_Unknown2;
    COND_GE<FMT_VER::V_164, uint16_t> m_Unknown3;

    uint32_t m_StrPtr;
    uint32_t m_Size;

    std::string m_String;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown4;
};


/**
 * Per-padstack dimension records with name and value. Not imported.
 */
struct BLK_0x1F_PADSTACK_DIM
{
    uint32_t m_Key;
    uint32_t m_Next;         ///< Linked list next pointer (used by LL_WALKER)
    uint32_t m_Unknown2;
    uint32_t m_Unknown3;
    uint32_t m_Unknown4;
    uint16_t m_Unknown5;
    uint16_t m_Size;

    /**
     * Version-dependent substruct holding padstack dimension name and value
     */
    std::vector<uint8_t> m_Substruct;
};


/**
 * Unknown purpose.
 *
 * Seems to contain one or two fixed-size arrays of uint32 values
 * (but they could be some set of fields).
 *
 * This block is sometimes used at the parent of, e.g. line segments.
 *
 * Only seen so far in Project Olympus 15061-1b.brd
 */
struct BLK_0x20_UNKNOWN
{
    uint8_t                 m_Type;
    uint16_t                m_R;
    uint32_t                m_Key;
    uint32_t                m_Next;
    std::array<uint32_t, 7> m_UnknownArray1;

    COND_GE<FMT_VER::V_174, std::array<uint32_t, 10>> m_UnknownArray2;
};

/**
 * Headered data blob containing structured board data such as layer stackup definitions,
 * material properties, and design rule tables. The payload (m_Data) is a variable-length
 * byte array whose interpretation depends on context.
 */
struct BLK_0x21_BLOB
{
    uint8_t  m_Type;
    uint16_t m_R;
    uint32_t m_Size;
    uint32_t m_Key;

    /**
     * An array of bytes that seems to be a variable length
     *
     * Size = m_Size - 12 (i.e. size is the whole header size)
     */
    std::vector<uint8_t> m_Data;
};


/**
 * Purpose not determined. Contains an 8-element array of uint32 values.
 */
struct BLK_0x22_UNKNOWN
{
    uint8_t  m_Type;
    uint16_t m_T2;
    uint32_t m_Key;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown1;

    std::array<uint32_t, 8> m_UnknownArray;
};


/**
 * 0x23 objects represent ratlines.
 */
struct BLK_0x23_RATLINE
{
    uint8_t    m_Type;
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;

    std::array<uint32_t, 2> m_Flags;

    uint32_t m_Ptr1;
    uint32_t m_Ptr2;
    uint32_t m_Ptr3;

    std::array<int32_t, 5> m_Coords;

    std::array<uint32_t, 4> m_Unknown1;

    COND_GE<FMT_VER::V_164, std::array<uint32_t, 4>> m_Unknown2;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown3;
};


/**
 * Rectangle defined by four coordinates. Appears on the m_LL_0x24_0x28 header linked
 * list for keepout areas and other rectangular regions. Has a layer and parent pointer.
 *
 * Not entirely clear how this differs from 0x0E yet.
 */
struct BLK_0x24_RECT
{
    uint8_t    m_Type;
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;
    uint32_t   m_Parent;
    uint32_t   m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    std::array<int32_t, 4> m_Coords;

    uint32_t m_Ptr2;

    uint32_t m_Unknown3;
    uint32_t m_Unknown4;
    /// Rotation in millidegrees
    uint32_t m_Rotation;
};


/**
 * Match group indirection for differential pairs and match groups.
 * NET.m_MatchGroupPtr -> 0x26 -> m_GroupPtr -> 0x2C TABLE -> group name string (V172+).
 */
struct BLK_0x26_MATCH_GROUP
{
    uint8_t  m_Type;
    uint16_t m_R;
    uint32_t m_Key;
    uint32_t m_MemberPtr;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint32_t m_GroupPtr;
    uint32_t m_ConstPtr;  ///< Points to timing/delay constraints (field type 0x63), not physical constraints

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown2;
};


/**
 * Serialized Constraint Manager database containing secondary name table (V172+),
 * material stackup, color palette, and constraint manager state. NOT a heap pointer table.
 *
 * Pre-V172 uses runtime heap pointer format. V172+ uses compact indices that match
 * geometric block keys. The block extends to an offset defined in the header (m_0x27_End).
 * Not imported directly since it is a display/navigation structure, not design data.
 */
struct BLK_0x27_CSTRMGR_XREF
{
    std::vector<uint32_t> m_Refs;
};


/**
 * Polygon shape defined by a linked list of segments starting at m_FirstSegmentPtr
 * (0x15/0x16/0x17 lines and 0x01 arcs). Used for zone outlines (BOUNDARY class on
 * m_LL_Shapes), computed copper fills (on net assignment chains), board outline, keepout
 * areas, place bounds, and custom pad shapes (shape symbol type 0x16).
 *
 * Zone net resolution follows m_Ptr7 (V172+) or m_Ptr7_16x (pre-V172) through a
 * 0x2C TABLE and 0x37 pointer array to reach the owning 0x1B NET.
 */
struct BLK_0x28_SHAPE
{
    uint8_t    m_Type;
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;
    uint32_t   m_Ptr1;
    uint32_t   m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;

    uint32_t m_Ptr2;
    uint32_t m_Ptr3;
    uint32_t m_Ptr4;
    uint32_t m_FirstSegmentPtr;
    uint32_t m_Unknown4;
    uint32_t m_Unknown5;

    COND_GE<FMT_VER::V_172, uint32_t> m_Ptr7;

    uint32_t m_Ptr6;

    COND_LT<FMT_VER::V_172, uint32_t> m_Ptr7_16x;

    std::array<int32_t, 4> m_Coords;
};


/**
 * 0x29 objects may represent pins in .dra files.
 *
 * Full version-specific structures not clear yet.
 */
struct BLK_0x29_PIN
{
    uint8_t  m_Type;
    uint16_t m_T;
    uint32_t m_Key;

    // Points to something in the header
    uint32_t m_Ptr1;
    uint32_t m_Ptr2;

    uint32_t m_Null; // Null value

    uint32_t m_Ptr3;

    int32_t m_Coord1;
    int32_t m_Coord2;

    uint32_t m_PtrPadstack;

    uint32_t m_Unknown1;

    // Pointer to a string, e.g., "2" in R0603
    uint32_t m_PtrX30;

    uint32_t m_Unknown2;
    uint32_t m_Unknown3;
    uint32_t m_Unknown4;
};


/**
 * Represents a list of layers.
 */
struct BLK_0x2A_LAYER_LIST
{
    struct NONREF_ENTRY
    {
        std::string m_Name;
    };

    struct REF_ENTRY
    {
        uint32_t mLayerNameId; // string ID
        uint32_t m_Properties;
        uint32_t m_Unknown;
    };

    uint16_t m_NumEntries;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown;

    COND_LT<FMT_VER::V_165, std::vector<NONREF_ENTRY>> m_NonRefEntries;
    COND_GE<FMT_VER::V_165, std::vector<REF_ENTRY>>    m_RefEntries;

    uint32_t m_Key;
};


/**
 * Footprint definition (template) shared by multiple placed instances. Contains the
 * library symbol path (m_SymLibPathPtr), bounding box, and a linked list of placed
 * instances starting at m_FirstInstPtr (0x2D blocks).
 */
struct BLK_0x2B_FOOTPRINT_DEF
{
    uint32_t m_Key;

    uint32_t m_FpStrRef;
    uint32_t m_Unknown1;

    // Could these be signed?
    std::array<uint32_t, 4> m_Coords;

    uint32_t m_Next;
    uint32_t m_FirstInstPtr;
    uint32_t m_UnknownPtr3;
    uint32_t m_UnknownPtr4;
    uint32_t m_UnknownPtr5;
    uint32_t m_SymLibPathPtr;
    uint32_t m_UnknownPtr6;
    uint32_t m_UnknownPtr7;
    uint32_t m_UnknownPtr8;

    COND_GE<FMT_VER::V_164, uint32_t> m_Unknown2;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;
};


/**
 * Lookup table used for named associations. Contains a string pointer (m_StringPtr) and
 * child pointers (m_Ptr1/m_Ptr2/m_Ptr3). Used in match group name resolution (diff pair
 * and match group names), zone net resolution (intermediate between 0x28 shape and 0x37
 * pointer array), and other key-to-name mappings.
 */
struct BLK_0x2C_TABLE
{
    uint8_t  m_Type;
    uint16_t m_T2;
    uint32_t m_Key;
    uint32_t m_Next;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;

    uint32_t m_StringPtr;

    COND_LT<FMT_VER::V_172, uint32_t> m_Unknown4;

    uint32_t m_Ptr1;
    uint32_t m_Ptr2;
    uint32_t m_Ptr3;

    uint32_t m_Flags;
};


/**
 * Placed footprint instance on the board. Contains position (m_CoordX/Y), rotation
 * (m_Rotation in millidegrees), and layer (0=top, 1=bottom). Links to graphics (0x14
 * via m_GraphicPtr), placed pads (0x32 via m_FirstPadPtr), text (0x30 via m_TextPtr),
 * and instance reference data (0x07 via m_InstRef). Bottom-layer footprints must be
 * flipped AFTER adding all children.
 */
struct BLK_0x2D_FOOTPRINT_INST
{
    uint8_t  m_UnknownByte1;
    uint8_t  m_Layer;         // 0 = top (F_Cu), 1 = bottom (B_Cu)
    uint8_t  m_UnknownByte2;

    uint32_t m_Key;

    uint32_t m_Next;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    // Unsure what 16x means
    COND_LT<FMT_VER::V_172, uint32_t> m_InstRef16x;

    uint16_t m_Unknown2;
    uint16_t m_Unknown3;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown4;

    uint32_t m_Flags;

    uint32_t m_Rotation;  ///< Millidegrees (divide by 1000 for degrees)
    int32_t  m_CoordX;
    int32_t  m_CoordY;

    // Presumably equivalent to m_InstRef16x
    COND_GE<FMT_VER::V_172, uint32_t> m_InstRef;

    uint32_t m_GraphicPtr;
    uint32_t m_FirstPadPtr;
    uint32_t m_TextPtr;  // Points to 0x30

    uint32_t m_AssemblyPtr;
    uint32_t m_AreasPtr;
    uint32_t m_UnknownPtr1;
    uint32_t m_UnknownPtr2;

    uint32_t GetInstRef() const
    {
        return m_InstRef.value_or( m_InstRef16x.value_or( 0 ) );
    }
};


/**
 * Connection point at a track junction or pad-to-track transition. Contains coordinates
 * and links to the net assignment and connected track.
 */
struct BLK_0x2E_CONNECTION
{
    uint8_t  m_Type;
    uint16_t m_T2;
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_NetAssignment;
    uint32_t m_Unknown1;
    int32_t m_CoordX;
    int32_t m_CoordY;
    uint32_t m_Connection;
    uint32_t m_Unknown2;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;
};


/**
 * Purpose not determined. Contains a 6-element array of uint32 values.
 */
struct BLK_0x2F_UNKNOWN
{
    uint8_t  m_Type;
    uint16_t m_T2;
    uint32_t m_Key;

    std::array<uint32_t, 6> m_UnknownArray;
};


/**
 * Text object with position, rotation, layer, font properties, and alignment. References
 * a 0x31 string graphic (m_StrGraphicPtr) for the actual text content. Used for reference
 * designators, component values, and other board text. Font index in TEXT_PROPERTIES.m_Key
 * is 1-based into the 0x36 FontDef_X08 list.
 */
struct BLK_0x30_STR_WRAPPER
{
    enum class TEXT_REVERSAL
    {
        STRAIGHT,
        REVERSED,
        UNKNOWN,
    };

    enum class TEXT_ALIGNMENT
    {
        LEFT,
        RIGHT,
        CENTER,
        UNKNOWN,
    };

    struct TEXT_PROPERTIES
    {
        uint8_t        m_Key;
        uint8_t        m_Flags;
        TEXT_ALIGNMENT m_Alignment;
        TEXT_REVERSAL  m_Reversal;
    };

    uint8_t    m_Type;
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;

    COND_GE<FMT_VER::V_172, uint32_t>        m_Unknown1;
    COND_GE<FMT_VER::V_172, uint32_t>        m_Unknown2;
    COND_GE<FMT_VER::V_172, TEXT_PROPERTIES> m_Font;
    COND_GE<FMT_VER::V_172, uint32_t>        m_Ptr1;
    COND_GE<FMT_VER::V_174, uint32_t>        m_Unknown3;

    uint32_t m_StrGraphicPtr;

    COND_GE<FMT_VER::V_172, uint32_t> m_PtrGroup_17x;
    COND_LT<FMT_VER::V_172, uint32_t> m_Unknown4;

    COND_LT<FMT_VER::V_172, TEXT_PROPERTIES> m_Font16x;

    COND_GE<FMT_VER::V_172, uint32_t> m_Ptr2;

    int32_t m_CoordsX;
    int32_t m_CoordsY;

    uint32_t m_Unknown5;
    uint32_t m_Rotation;  ///< Millidegrees

    COND_LT<FMT_VER::V_172, uint32_t> m_PtrGroup_16x;

    uint32_t GetGroupPtr() const
    {
        return m_PtrGroup_17x.value_or( m_PtrGroup_16x.value_or( 0 ) );
    }
};


/**
 * String graphic content holding the actual text value and its display layer category.
 * Referenced by 0x30 text wrapper objects via m_StrGraphicPtr.
 */
struct BLK_0x31_SGRAPHIC
{
    enum class STRING_LAYER : uint16_t
    {
        BOT_TEXT,
        TOP_TEXT,
        BOT_PIN,
        TOP_PIN,
        TOP_PIN_LABEL,
        BOT_REFDES,
        TOP_REFDES,
        UNKNOWN,
    };

    uint8_t      m_T;
    STRING_LAYER m_Layer;
    uint32_t     m_Key;
    uint32_t     m_StrGraphicWrapperPtr;

    int32_t m_CoordsX;
    int32_t m_CoordsY;

    uint16_t m_Unknown;
    uint16_t m_Len;

    COND_GE<FMT_VER::V_174, uint32_t> m_Un2;

    std::string m_Value;
};


/**
 * Placed pad instance linking a pad definition (0x0D via m_PadPtr) to its parent
 * footprint (m_ParentFp) and net (m_NetPtr). Chained within a footprint via m_NextInFp
 * and within a component instance via m_NextInCompInst.
 */
struct BLK_0x32_PLACED_PAD
{
    uint8_t    m_Type;
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;
    uint32_t   m_NetPtr;
    uint32_t   m_Flags;

    COND_GE<FMT_VER::V_172, uint32_t> m_Prev;

    uint32_t m_NextInFp;
    uint32_t m_ParentFp;
    uint32_t m_Track;
    uint32_t m_PadPtr;
    uint32_t m_Ptr6;
    uint32_t m_Ratline;
    uint32_t m_PtrPinNumber;
    uint32_t m_NextInCompInst;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    uint32_t m_NameText;
    uint32_t m_Ptr11;

    std::array<int32_t, 4> m_Coords;
};


/**
 * Via instance with board position, padstack reference (m_Padstack for drill/annular ring
 * definitions), and net assignment (m_NetPtr). Drill diameter comes from the referenced
 * padstack (version-dependent location).
 */
struct BLK_0x33_VIA
{
    LAYER_INFO m_LayerInfo;
    uint32_t   m_Key;
    uint32_t   m_Next;
    uint32_t   m_NetPtr;
    uint32_t   m_Unknown2;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;

    uint32_t m_UnknownPtr1;

    COND_GE<FMT_VER::V_172, uint32_t> m_UnknownPtr2;

    int32_t m_CoordsX;
    int32_t m_CoordsY;

    uint32_t m_Connection;
    uint32_t m_Padstack;
    uint32_t m_UnknownPtr5;
    uint32_t m_UnknownPtr6;

    uint32_t m_Unknown4;
    uint32_t m_Unknown5;

    std::array<int32_t, 4> m_BoundingBoxCoords;
};


/**
 * 0x34 objects represent keepouts.
 */
struct BLK_0x34_KEEPOUT
{
    uint8_t    m_T;
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;
    uint32_t   m_Ptr1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint32_t m_Flags;
    uint32_t m_Ptr2;
    uint32_t m_Ptr3;
    uint32_t m_Unknown2;
};


/**
 * File path references to Allegro log and report files (terminator.log, eclrpt.txt).
 * Content is a 120-byte fixed buffer with null-terminated path strings. Not imported.
 */
struct BLK_0x35_FILE_REF
{
    uint8_t  m_T2;
    uint16_t m_T3;

    std::array<uint8_t, 120> m_Content;
};


/**
 * Heterogeneous definition table containing font metrics (FontDef_X08), layer name
 * definitions (X03), film definitions (X02), and other board-level configuration data.
 * Items are stored as a variant vector indexed by substruct code.
 */
struct BLK_0x36_DEF_TABLE
{
    uint16_t m_Code;
    uint32_t m_Key;
    uint32_t m_Next;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint32_t m_NumItems;
    uint32_t m_Count;
    uint32_t m_LastIdx;
    uint32_t m_Unknown2;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown3;

    struct X02
    {
        std::string              m_String;
        std::array<uint32_t, 14> m_Xs;

        COND_GE<FMT_VER::V_164, std::array<uint32_t, 3>> m_Ys;
        COND_GE<FMT_VER::V_172, std::array<uint32_t, 2>> m_Zs;
    };

    struct X03
    {
        COND_GE<FMT_VER::V_172, std::string> m_Str;
        COND_LT<FMT_VER::V_172, std::string> m_Str16x;
        COND_GE<FMT_VER::V_174, uint32_t>    m_Unknown1;
    };

    struct X05
    {
        std::array<uint8_t, 28> m_Unknown;
    };

    struct X06
    {
        uint16_t m_N;
        uint8_t  m_R;
        uint8_t  m_S;
        uint32_t m_Unknown1;

        COND_LT<FMT_VER::V_172, std::array<uint32_t, 50>> m_Unknown2;
    };

    struct FontDef_X08
    {
        uint32_t m_A;
        uint32_t m_B;
        uint32_t m_CharHeight;
        uint32_t m_CharWidth;

        COND_GE<FMT_VER::V_174, uint32_t> m_Unknown2;

        std::array<uint32_t, 4> m_Xs;

        COND_GE<FMT_VER::V_172, std::array<uint32_t, 8>> m_Ys;
    };

    struct X0B
    {
        std::array<uint8_t, 1016> m_Unknown;
    };

    struct X0C
    {
        std::array<uint8_t, 232> m_Unknown;
    };

    struct X0D
    {
        std::array<uint8_t, 200> m_Unknown;
    };

    struct X0F
    {
        uint32_t                m_Key;
        std::array<uint32_t, 3> m_Ptrs;
        uint32_t                m_Ptr2;
    };

    struct X10
    {
        std::array<uint8_t, 108> m_Unknown;

        // V18 has an extra uint32 "somewhere" in this block
        COND_GE<FMT_VER::V_180, uint32_t> m_Unknown2;
    };

    using SubstructVariant = std::variant<X02, X03, X05, X06, FontDef_X08, X0B, X0C, X0D, X0F, X10>;

    std::vector<SubstructVariant> m_Items;
};


/**
 * Fixed-capacity pointer array (100 entries). Used in zone net resolution where
 * m_Ptrs[0] points to the owning 0x1B NET block. m_Count indicates how many entries
 * are valid.
 */
struct BLK_0x37_PTR_ARRAY
{
    uint8_t  m_T;
    uint16_t m_T2;
    uint32_t m_Key;
    uint32_t m_GroupPtr;
    uint32_t m_Next;
    uint32_t m_Capacity;
    uint32_t m_Count;
    uint32_t m_Unknown2;

    std::array<uint32_t, 100> m_Ptrs;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown3;
};


/**
 * 0x38 objects represent films.
 */
struct BLK_0x38_FILM
{
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_LayerList;

    COND_LT<FMT_VER::V_166, std::string> m_FilmName;

    COND_GE<FMT_VER::V_166, uint32_t> m_LayerNameStr;
    COND_GE<FMT_VER::V_166, uint32_t> m_Unknown2;

    std::array<uint32_t, 7> m_UnknownArray1;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown3;
};


/**
 * 0x39 objects represent a film layer list.
 */
struct BLK_0x39_FILM_LAYER_LIST
{
    uint32_t m_Key;
    uint32_t m_Parent;
    uint32_t m_Head;

    // Array of 22 uint16_t values
    std::array<uint16_t, 22> m_X;
};


/**
 * 0x3A objects represent a list of films
 */
struct BLK_0x3A_FILM_LIST_NODE
{
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;
    uint32_t   m_Unknown;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown1;
};


/**
 * Named property with type and value strings. Carries board-level metadata such as
 * component attributes and design parameters.
 */
struct BLK_0x3B_PROPERTY
{
    uint8_t  m_T;
    uint16_t m_SubType;
    uint32_t m_Len;

    std::string m_Name;
    std::string m_Type;

    uint32_t m_Unknown1;
    uint32_t m_Unknown2;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;

    std::string m_Value;
};


/**
 * Ordered list of block keys. Context-dependent usage; appears alongside other
 * block types for grouping related objects.
 */
struct BLK_0x3C_KEY_LIST
{
    uint8_t  m_T;
    uint16_t m_T2;
    uint32_t m_Key;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown;

    uint32_t              m_NumEntries;
    std::vector<uint32_t> m_Entries;
};


/**
 * Raw board structure that we will build as we parse the file.
 */
struct RAW_BOARD
{
public:
    RAW_BOARD();

    std::unique_ptr<FILE_HEADER> m_Header;

    /**
     * What version is this file? We will need this to correctly interpret some structures.
     */
    FMT_VER m_FmtVer;

    /**
     * The string map is a map of U32 ID to strings.
     * It seems to always be located at byte 0x1200 in the file.
     */
    std::unordered_map<uint32_t, std::string> m_StringTable;

    // All the objects in the file
    std::vector<std::unique_ptr<BLOCK_BASE>> m_Objects;

    // Map of keys to objects (for the objects we can get keys for)
    std::unordered_map<uint32_t, BLOCK_BASE*> m_ObjectKeyMap;

    // Lists of the objects by type
    std::unordered_map<uint8_t, std::vector<BLOCK_BASE*>> m_ObjectLists;

    static const size_t STRING_TABLE_OFFSET = 0x1200;

    const BLOCK_BASE* GetObjectByKey( uint32_t aKey ) const
    {
        auto it = m_ObjectKeyMap.find( aKey );
        if( it != m_ObjectKeyMap.end() )
            return it->second;
        return nullptr;
    }

    const std::string& GetString( uint32_t aId ) const
    {
        if( m_StringTable.count( aId ) )
            return m_StringTable.at( aId );

        static const std::string empty;
        return empty;
    }
};

} // namespace ALLEGRO
