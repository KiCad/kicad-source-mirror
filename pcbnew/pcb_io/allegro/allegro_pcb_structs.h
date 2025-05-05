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
    V_160, // Allegro 16.0
    V_164, // Allegro 16.4
    V_165,
    V_166,
    V_172,
    V_174,
    V_175,
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
