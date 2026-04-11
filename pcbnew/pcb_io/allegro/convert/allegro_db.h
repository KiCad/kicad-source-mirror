/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <convert/allegro_pcb_structs.h>


namespace ALLEGRO
{

class BLOCK_BASE;

/**
 * An Allegro board database representing the contents of a .brd (and presumably .dra) file.
 *
 * Owns all parsed blocks, the string table, sentinel keys, file header, and
 * the key-to-block lookup map.
 */
class BRD_DB
{
public:
    BRD_DB();

    void AddString( uint32_t aKey, wxString&& aStr )
    {
        m_StringTable.emplace( aKey, std::move( aStr ) );
    }

    size_t GetObjectCount() const { return m_Blocks.size(); }

    bool IsSentinel( uint32_t aKey ) const
    {
        return m_SentinelKeys.count( aKey ) > 0;
    }

    void AddSentinelKey( uint32_t aKey )
    {
        if( aKey != 0 )
            m_SentinelKeys.insert( aKey );
    }

    void InsertBlock( std::unique_ptr<BLOCK_BASE> aBlock );

    /**
     * Pre-allocate storage for the expected number of objects and strings.
     * Avoids incremental rehashing as elements are inserted.
     */
    void ReserveCapacity( size_t aObjectCount, size_t aStringCount );

    /**
     * Populate sentinel keys from the file header linked lists.
     */
    bool ResolveAndValidate();

    /**
     * Get a block by its key.
     */
    const BLOCK_BASE* GetObjectByKey( uint32_t aKey ) const
    {
        auto it = m_ObjectKeyMap.find( aKey );
        return it != m_ObjectKeyMap.end() ? it->second : nullptr;
    }

    /**
     * Get a string from the string table by key.
     */
    const wxString& GetString( uint32_t aKey ) const
    {
        static const wxString empty;
        auto it = m_StringTable.find( aKey );
        return it != m_StringTable.end() ? it->second : empty;
    }

    FMT_VER                      m_FmtVer;
    std::unique_ptr<FILE_HEADER> m_Header;

    std::vector<std::unique_ptr<BLOCK_BASE>>  m_Blocks;
    std::unordered_map<uint32_t, BLOCK_BASE*> m_ObjectKeyMap;
    std::unordered_map<uint32_t, wxString>    m_StringTable;

private:
    // V18+ linked list sentinel keys. References to these are null (end-of-chain).
    std::unordered_set<uint32_t> m_SentinelKeys;
};

} // namespace ALLEGRO
