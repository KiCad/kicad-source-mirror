/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <array>
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
    V_160, // Allegro 16.0, 0x00130000
    V_162, // Allegro 16.2  0x00130400
    V_164, // Allegro 16.4, 0x00130C00
    V_165, // Allegro 16.5, 0x00131000
    V_166, // Allegro 16.6, 0x00131500
    V_172, // Allegro 17.2, 0x00140400
    V_174, // Allegro 17.4, 0x00140900
    V_175, // Allegro 17.5, 0x00141500
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
    constexpr bool exists( FMT_VER ver ) const { return ver >= MinVersion; }

    using COND_FIELD_BASE<T>::operator=;
};


/**
 * This is a conditional field that only exists in versions of a file
 * less than a certain version.
 */
template <FMT_VER MaxVersion, typename T>
struct COND_LT : public COND_FIELD_BASE<T>
{
    constexpr bool exists( FMT_VER ver ) const { return ver < MaxVersion; }

    using COND_FIELD_BASE<T>::operator=;
};


/**
 * This is a conditional field that only exists in versions of a file
 * less than a certain version.
 */
template <FMT_VER GEVersion, FMT_VER LTVersion, typename T>
struct COND_GE_LT : public COND_FIELD_BASE<T>
{
    constexpr bool exists( FMT_VER ver ) const { return ver >= ver && ver < LTVersion; }

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
        uint32_t m_B;
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
    std::array<uint32_t, 4> m_Unknown1;
    uint32_t                m_ObjectCount;
    std::array<uint32_t, 9> m_Unknown2;

    // A group of linked lists that refer to certain groups of elements
    // TODO revisit when we know what all these codes are
    LINKED_LIST m_LL_0x04;
    LINKED_LIST m_LL_0x06;
    LINKED_LIST m_LL_0x0C;
    LINKED_LIST m_LL_Shapes; // 0xE and 0x28
    LINKED_LIST m_LL_0x14;
    LINKED_LIST m_LL_Nets; // 0x1B Nets
    LINKED_LIST m_LL_0x1C;
    LINKED_LIST m_LL_0x24_0x28;
    LINKED_LIST m_LL_Unknown1;
    LINKED_LIST m_LL_0x2B;
    LINKED_LIST m_LL_0x03_0x30; // string items? 0x30 = string wrapper
    LINKED_LIST m_LL_0x0A;
    LINKED_LIST m_LL_0x1D_0x1E_0x1F;
    LINKED_LIST m_LL_Unknown2;
    LINKED_LIST m_LL_0x38;
    LINKED_LIST m_LL_0x2C;
    LINKED_LIST m_LL_0x0C_2; // Another 0x0C?
    LINKED_LIST m_LL_Unknown3;

    // For some reason the 0x35 extents are recorded here
    uint32_t m_0x35_Start;
    uint32_t m_0x35_End;

    // And more linked lists
    LINKED_LIST m_LL_Unknown4;
    LINKED_LIST m_LL_Unknown5;
    LINKED_LIST m_LL_Unknown6;
    LINKED_LIST m_LL_Unknown7;

    uint32_t m_Unknown3;

    // Fixed length string field
    std::array<char, 60> m_AllegroVersion;

    uint32_t m_Unknown4;

    uint32_t m_MaxKey;

    std::array<uint32_t, 17> m_Unknown5;

    BOARD_UNITS m_BoardUnits;
    // 3 empty bytes here?

    uint32_t m_Unknown6;
    uint32_t m_Unknown7;

    // The end of the 0x27 object(?)
    uint32_t m_0x27_End;

    uint32_t m_Unknown8;

    uint32_t m_StringsCount;

    std::array<uint32_t, 53> m_Unknown9;

    // E.g. 1000 for mils
    uint32_t m_UnitsDivisor;

    /**
     * The layer maps is a list of groups of two numbers.
     *
     * All files so far seem to have 25 entries, but unsure if that
     * is a universal value. But there's no obvious nearby value of '25'.
     */
    std::array<LAYER_MAP_ENTRY, 25> m_LayerMap;
};


/**
 * 0x01 objects are arcs
 */
struct BLK_0x01_ARC
{
    uint8_t  m_UnknownByte;
    uint8_t  m_SubType;
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_Parent;
    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown6;

    uint32_t               m_Width;
    std::array<int32_t, 4> m_Coords;

    float m_X;
    float m_Y;
    float m_R;

    std::array<int32_t, 4> m_BoundingBoxCoords;
};


/**
 * 0x03 objects has some kind of variable substruct
 */
struct BLK_0x03
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

    uint32_t m_Key;
    uint32_t m_Next;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint8_t  m_SubType;
    uint8_t  m_UnknownByte;
    uint16_t m_Size;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    std::variant<uint32_t, std::array<uint32_t, 2>, std::string, SUB_0x6C, SUB_0x70_0x74, SUB_0xF6> m_Substruct;
};


/**
 * 0x05 objects represent tracks
 *
 * They then refer out to, say, 0x17 segments
 */
struct BLK_0x05_TRACK
{
    uint32_t m_Key;
    uint32_t m_Unknown1;
    uint32_t m_UnknownPtr1;
    uint32_t m_UnknownPtr2;
    uint32_t m_Unknown2a;
    uint32_t m_Unknown2b;
    uint32_t m_UnknownPtr3;
    uint32_t m_UnknownPtr4;
    uint32_t m_Unknown3;

    uint32_t m_Ptr0x3A;

    COND_LT<FMT_VER::V_172, uint32_t> m_Ptr0x3B;

    COND_GE<FMT_VER::V_172, uint32_t> m_PtrA;
    COND_GE<FMT_VER::V_172, uint32_t> m_PtrB;
    COND_GE<FMT_VER::V_172, uint32_t> m_PtrC;

    uint32_t m_FirstSegPtr;
    uint32_t m_UnknownPtr5;
    uint32_t m_Unknown4;
};


/**
 * 0x06 objects are usually the first objects in the file.
 *
 * Exact purpose not clear yet.
 */
struct BLK_0x06
{
    uint32_t m_Key;

    // Pointer to the next BLK_0x06
    uint32_t m_Next;

    uint32_t m_PtrStr;

    uint32_t m_UnknownPtr;

    uint32_t m_PtrInstance;

    uint32_t m_PtrFootprint;

    uint32_t m_Ptr0x08;

    // Points to 0x03, schematic symbol
    uint32_t m_PtrSymbol;

    COND_GE<FMT_VER::V_172, uint32_t> m_UnknownPtr2;
};


/**
 * 0x07 objects.
 */
struct BLK_0x07
{
    uint32_t m_Key;

    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_UnknownPtr1;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;

    uint32_t m_Ptr0x2D;

    COND_LT<FMT_VER::V_172, uint32_t> m_Unknown4;

    uint32_t m_RefDesRef;
    uint32_t m_UnknownPtr2;
    uint32_t m_UnknownPtr3; // 0x03 or null
    uint32_t m_Unknown5;
    uint32_t m_UnknownPtr4; // 0x32 or null
};


/**
 * 0x08 objects.
 */
struct BLK_0x08
{
    uint8_t  m_Type;
    uint16_t m_R;
    uint32_t m_Key;

    COND_GE<FMT_VER::V_172, uint32_t> m_Ptr1;
    COND_LT<FMT_VER::V_172, uint32_t> m_StrPtr16x;

    uint32_t m_Ptr2;

    COND_GE<FMT_VER::V_172, uint32_t> m_StrPtr;

    uint32_t m_Ptr3;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint32_t m_Ptr4;
};


/**
 * 0x09 objects.
 */
struct BLK_0x09
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
 * 0x0C objects.
 */
struct BLK_0x0C
{
    uint32_t m_Key;
    uint32_t m_Next;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    uint32_t m_Unknown3;
    uint32_t m_Unknown4;
    uint32_t m_KeyInd;

    uint32_t               m_Unknown5;
    std::array<int32_t, 4> m_Coords;

    std::array<uint32_t, 3> m_UnknownArray;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown6;
};


/**
 * 0x0D objects represent pads.
 */
struct BLK_0x0D_PAD
{
    uint32_t m_Key;
    uint32_t m_StrPtr;
    uint32_t m_UnknownPtr;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown1;

    std::array<int32_t, 2> m_Coords;

    uint32_t m_PadPtr;
    uint32_t m_Unknown2;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;

    uint32_t m_Flags;
    uint32_t m_Rotation;
};


/**
 * 0x0F objects.
 *
 * Exact purpose not clear yet.
 *
 * Number of 0x0F objects in a file seems to match 0x06 objects.
 */
struct BLK_0x0F
{
    uint32_t m_Key;

    uint32_t m_Ptr;

    std::array<char, 32> m_Name;

    uint32_t m_Ptr0x06;
    uint32_t m_Ptr0x11;

    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;
    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown3;
};


/**
 * 0x10 objects.
 *
 * Use unclear for now.
 *
 * Number of 0x10 objects in a file seems to match 0x07 objects.
 */
struct BLK_0x10
{
    uint32_t m_Key;

    COND_GE<FMT_VER::V_172, uint32_t> m_UnknownPtr1;

    uint32_t m_UnknownPtr2;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    uint32_t m_UnknownPtr3;
    uint32_t m_Unknown2;
    uint32_t m_UnknownPtr4;
    uint32_t m_UnknownPtr5; // 0x0F?
    uint32_t m_PathStr;     // Presumably a pointer to a string
};


/**
 * 0x15 is a segment object.
 *
 * As for 0x16, unsure of the distinction between 0x15 and 0x16/0x17
 */
struct BLK_0x15_SEGMENT
{
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_Parent;
    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    uint32_t               m_Width;
    std::array<int32_t, 4> m_Coords;
};


/**
 * 0x16 is a segment object.
 *
 * It has flags, that 0x17 objects call "unknown". The two may be the same?
 */
struct BLK_0x16_SEGMENT
{
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_Parent;
    uint32_t m_Flags;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    uint32_t               m_Width;
    std::array<int32_t, 4> m_Coords;
};


/**
 * 0x17 is a segment object.
 *
 * Tracks (0x05) refer to segments.
 */
struct BLK_0x17_SEGMENT
{
    uint32_t m_Key;
    uint32_t m_Next;
    uint32_t m_Parent;
    uint32_t m_Unknown1;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown2;

    uint32_t               m_Width;
    std::array<int32_t, 4> m_Coords;
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

    uint32_t m_UnknownPtr1;
    uint32_t m_UnknownPtr2;
    uint32_t m_PathStrPtr;
    uint32_t m_UnknownPtr3;
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
 * Substruct in a padstack object.
 *
 * It's not quite clear what the fields actually mean, but presumably
 * relate to per layer(?) structures.
 */
struct PADSTACK_COMPONENT
{
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
    uint32_t m_Unknown1;
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

    std::array<uint32_t, 8> m_UnknownArr8;

    COND_GE<FMT_VER::V_172, std::array<uint32_t, 28>> m_UnknownArr28;

    COND_GE_LT<FMT_VER::V_165, FMT_VER::V_172, std::array<uint32_t, 8>> m_UnknownArr8_2;

    /**
     * Collection of components that make up the padstack.
     *
     * The number of components appears to be fixed by version:
     *
     * *  < 17.2: 10 + layer_count * 3
     * * >= 17.2: 21 + layer_count * 4
     */
    std::vector<PADSTACK_COMPONENT> m_Components;

    /**
     * Some structure of m_N * 8 or 10:
     *
     * *  < 17.2: 8
     * * >= 17.2: 10
     */
    std::vector<uint32_t> m_UnknownArrN;
};


/**
 * 0x1D objects.
 */
struct BLK_0x1D
{
    uint32_t m_Key;
    uint32_t m_Unknown1;
    uint32_t m_Unknown2;
    uint32_t m_Unknown3;
    uint16_t m_SizeA;
    uint16_t m_SizeB;

    /**
     * Size of this is m_SizeB * 56
     *
     * Presumably blocks of 56?
     */
    std::vector<std::array<uint8_t, 56>> m_DataB;
    /**
     * Size of this is m_SizeA * 256
     */
    std::vector<std::array<uint8_t, 256>> m_DataA;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown4;
};


/**
 * 0x1F objects
 */
struct BLK_0x1F
{
    uint32_t m_Key;
    uint32_t m_Unknown1;
    uint32_t m_Unknown2;
    uint32_t m_Unknown3;
    uint32_t m_Unknown4;
    uint16_t m_Unknown5;
    uint16_t m_Size;

    /**
     * Some kind of quite substantial substruct that varies in version
     */
    std::vector<uint8_t> m_Substruct;
};


/**
 * 0x21 objects.
 *
 * Some kind of headered data structure.
 *
 * The contained data appears to be quite substantial - hundreds or
 * thousands of bytes.
 */
struct BLK_0x21
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


struct BLK_0x2A
{
    struct NONREF_ENTRY
    {
        std::array<uint8_t, 36> m_Unknown;
    };

    struct REF_ENTRY
    {
        uint32_t mPtr;
        uint32_t m_Properties;
        uint32_t m_Unknown;
    };

    uint16_t m_NumEntries;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown;

    COND_LT<FMT_VER::V_164, std::vector<NONREF_ENTRY>> m_NonRefEntries;
    COND_GE<FMT_VER::V_164, std::vector<REF_ENTRY>>    m_RefEntries;

    uint32_t m_Key;
};


/**
 * 0x2B objects.
 */
struct BLK_0x2B
{
    uint32_t m_Key;

    uint32_t m_FpStrRef;
    uint32_t m_Unknown1;

    // Could these be signed?
    std::array<uint32_t, 4> m_Coords;

    uint32_t m_Next;
    uint32_t m_UnknownPtr2;
    uint32_t m_UnknownPtr3;
    uint32_t m_UnknownPtr4;
    uint32_t m_UnknownPtr5;
    uint32_t m_StrPtr;
    uint32_t m_UnknownPtr6;
    uint32_t m_UnknownPtr7;
    uint32_t m_UnknownPtr8;

    COND_GE<FMT_VER::V_164, uint32_t> m_Unknown2;
    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;
};


/**
 * 0x2D objects.
 */
struct BLK_0x2D
{
    uint32_t m_Key;

    uint32_t m_Next;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown1;

    // Unsure what 16x means
    COND_LT<FMT_VER::V_172, uint32_t> m_InstRef16x;

    uint16_t m_Unknown2;
    uint16_t m_Unknown3;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown;

    uint32_t m_Flags;

    uint32_t m_Rotation;
    int32_t  m_CoordX;
    int32_t  m_CoordY;

    // Presumably equivalent to m_InstRef16x
    COND_GE<FMT_VER::V_172, uint32_t> m_InstRef;

    uint32_t m_UnknownPtr1;
    uint32_t m_FirstPadPtr;
    uint32_t m_UnknownPtr2;

    std::array<uint32_t, 4> m_UnknownPtrs1;
};


struct LAYER_INFO
{
    enum class FAMILY
    {
        BOARD_GEOM,
        COPPER,
        SILK,
        UNKNOWN_0x12,
    };

    FAMILY  m_Family;
    uint8_t m_Ordinal;
};


/**
 * 0x33 objects are vias.
 */
struct BLK_0x33_VIA
{
    LAYER_INFO m_LayerInfo;
    uint32_t   m_Key;
    uint32_t   m_Unknown1;
    uint32_t   m_NetPtr;
    uint32_t   m_Unknown2;

    COND_GE<FMT_VER::V_172, uint32_t> m_Unknown3;

    uint32_t m_UnknownPtr1;

    COND_GE<FMT_VER::V_172, uint32_t> m_UnknownPtr2;

    std::array<int32_t, 2> m_Coords;

    uint32_t m_UnknownPtr3;
    uint32_t m_UnknownPtr4;
    uint32_t m_UnknownPtr5;
    uint32_t m_UnknownPtr6;

    uint32_t m_Unknown4;
    uint32_t m_Unknown5;

    std::array<int32_t, 4> m_BoundingBoxCoords;
};


/**
 * 0x35 objects.
 */
struct BLK_0x35
{
    uint8_t  m_T2;
    uint16_t m_T3;

    std::array<uint8_t, 120> m_Content;
};


/**
 * 0x36 objects.
 */
struct BLK_0x36
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

    struct X08
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
    };

    using SubstructVariant = std::variant<X02, X03, X05, X06, X08, X0B, X0C, X0D, X0F, X10>;

    std::vector<SubstructVariant> m_Items;
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
struct TYPE_3A_FILM_LIST_NODE
{
    LAYER_INFO m_Layer;
    uint32_t   m_Key;
    uint32_t   m_Next;
    uint32_t   m_Unknown;

    COND_GE<FMT_VER::V_174, uint32_t> m_Unknown1;
};


/**
 * Raw board structure that we will build as we parse the file.
 */
struct RAW_BOARD
{
public:
    std::unique_ptr<FILE_HEADER> m_Header;

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
};

} // namespace ALLEGRO
