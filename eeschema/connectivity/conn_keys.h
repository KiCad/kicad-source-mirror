/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <kiid.h>
#include <wx/string.h>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace SCH_CONNECTIVITY
{
using INST_ID = uint32_t;
using NAME_ID = uint32_t;
using NODE_ID = uint32_t;
using SCREEN_ID = uint64_t;
constexpr uint32_t INVALID_ID = std::numeric_limits<uint32_t>::max();

enum class KIND : uint8_t
{
    SIGNAL,
    BUNDLE
};
enum class SCOPE : uint8_t
{
    SHEET,
    GLOBAL,
    PORT
};

struct ITEM_KEY
{
    KIID    item = niluuid;
    INST_ID inst = INVALID_ID;
    bool    operator==( const ITEM_KEY& ) const = default;
};

struct RECORD_KEY
{
    INST_ID inst = INVALID_ID;
    KIID    anchor = niluuid;
    bool    operator==( const RECORD_KEY& ) const = default;
};

struct RECORD_NODE
{
    RECORD_KEY record;
    KIND       kind = KIND::SIGNAL;
    bool       operator==( const RECORD_NODE& ) const = default;
};

struct NAME_KEY
{
    SCOPE   scope = SCOPE::SHEET;
    INST_ID inst = INVALID_ID;
    NAME_ID text = INVALID_ID;
    KIND    kind = KIND::SIGNAL;

    bool operator==( const NAME_KEY& aOther ) const
    {
        return scope == aOther.scope && text == aOther.text && ( scope == SCOPE::GLOBAL || inst == aOther.inst )
               && ( scope != SCOPE::PORT || kind == aOther.kind );
    }
};

struct SLOT_KEY
{
    ITEM_KEY bundleDriver;
    uint32_t leaf = 0;
    bool     operator==( const SLOT_KEY& ) const = default;
};

using NODE_KEY = std::variant<RECORD_NODE, NAME_KEY, SLOT_KEY>;

/**
 * Session IDs are dense handles, never a canonical ordering. Keys contain no model pointers.
 * Intern on the main thread before worker stages; values and their addresses remain stable.
 */
class SESSION_KEYS
{
public:
    SESSION_KEYS() = default;
    SESSION_KEYS( const SESSION_KEYS& ) = delete;
    SESSION_KEYS& operator=( const SESSION_KEYS& ) = delete;

    INST_ID InternInstance( const KIID_PATH& aPath ) { return m_instances.Intern( aPath ); }
    NAME_ID InternName( const wxString& aText ) { return m_names.Intern( { aText, aText.utf8_string() } ); }
    NODE_ID InternNode( NODE_KEY aKey );
    std::optional<NAME_ID> FindName( const wxString& aText ) const
    {
        return m_names.Find( { {}, aText.utf8_string() } );
    }

    std::optional<INST_ID> FindInstance( const KIID_PATH& aPath ) const { return m_instances.Find( aPath ); }

    const KIID_PATH& Instance( INST_ID aId ) const { return m_instances.Value( aId ); }
    const wxString&  Name( NAME_ID aId ) const { return m_names.Value( aId ).text; }
    const NODE_KEY&  Node( NODE_ID aId ) const { return m_nodes.Value( aId ); }
    size_t           NodeCount() const { return m_nodes.Size(); }
    bool             NameLess( NAME_ID aLeft, NAME_ID aRight ) const
    {
        return m_names.Value( aLeft ).utf8 < m_names.Value( aRight ).utf8;
    }

    bool Less( const ITEM_KEY& aLeft, const ITEM_KEY& aRight ) const;
    bool Less( const RECORD_KEY& aLeft, const RECORD_KEY& aRight ) const;
    bool Less( const RECORD_NODE& aLeft, const RECORD_NODE& aRight ) const;
    bool Less( const NAME_KEY& aLeft, const NAME_KEY& aRight ) const;
    bool Less( const SLOT_KEY& aLeft, const SLOT_KEY& aRight ) const;
    bool Less( const NODE_KEY& aLeft, const NODE_KEY& aRight ) const;

private:
    template <typename VALUE, typename MAP = std::map<VALUE, uint32_t>>
    class INTERN_TABLE
    {
    public:
        INTERN_TABLE() = default;
        INTERN_TABLE( const INTERN_TABLE& ) = delete;
        INTERN_TABLE& operator=( const INTERN_TABLE& ) = delete;

        uint32_t Intern( const VALUE& aValue )
        {
            // Size never exceeds INVALID_ID, so a full table proposes exactly INVALID_ID
            auto [it, inserted] = m_ids.try_emplace( aValue, static_cast<uint32_t>( m_values.size() ) );

            if( !inserted )
                return it->second;

            try
            {
                if( it->second == INVALID_ID )
                    throw std::overflow_error( "Connectivity intern IDs exhausted" );

                m_values.push_back( &it->first );
            }
            catch( ... )
            {
                m_ids.erase( it );
                throw;
            }

            return it->second;
        }

        std::optional<uint32_t> Find( const VALUE& aValue ) const
        {
            const auto found = m_ids.find( aValue );
            return found == m_ids.end() ? std::nullopt : std::optional<uint32_t>( found->second );
        }

        const VALUE& Value( uint32_t aId ) const { return *m_values.at( aId ); }
        size_t       Size() const { return m_values.size(); }

    private:
        MAP m_ids;
        // Both ordered and unordered maps keep key addresses stable during growth.
        std::vector<const VALUE*> m_values;
    };

    struct INTERNED_NAME
    {
        wxString    text;
        std::string utf8;
        bool        operator<( const INTERNED_NAME& aOther ) const { return utf8 < aOther.utf8; }
    };

    struct NODE_HASH
    {
        size_t operator()( const NODE_KEY& aKey ) const;
    };

    INTERN_TABLE<KIID_PATH>     m_instances;
    INTERN_TABLE<INTERNED_NAME> m_names;

    INTERN_TABLE<NODE_KEY, std::unordered_map<NODE_KEY, uint32_t, NODE_HASH>> m_nodes;
};

struct NAME_LESS
{
    const SESSION_KEYS* keys;
    bool                operator()( NAME_ID aLeft, NAME_ID aRight ) const { return keys->NameLess( aLeft, aRight ); }
};

struct KEY_LESS
{
    const SESSION_KEYS& keys;

    template <typename KEY>
    bool operator()( const KEY& aLeft, const KEY& aRight ) const
    {
        return keys.Less( aLeft, aRight );
    }
};
} // namespace SCH_CONNECTIVITY
