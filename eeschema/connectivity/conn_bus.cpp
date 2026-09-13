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

#include "conn_bus.h"

#include <project/net_settings.h>
#include <string_utils.h>

#include <algorithm>
#include <functional>

namespace
{
using namespace SCH_CONNECTIVITY;

class SCHEMA_PARSER
{
public:
    explicit SCHEMA_PARSER( const BUS_ALIASES& aAliases )
    {
        for( const auto& [name, members] : aAliases )
            m_aliases.insert_or_assign( name.Strip( wxString::both ), std::cref( members ) );
    }

    std::optional<BUS_SCHEMA> Parse( const wxString& aText, std::set<wxString>* aAliasesUsed = nullptr )
    {
        const bool parsed = expand( aText, {}, {}, m_schema.root, true );

        if( aAliasesUsed )
            *aAliasesUsed = m_schema.aliasesUsed;

        if( !parsed )
            return std::nullopt;

        return std::move( m_schema );
    }

private:
    bool expandMember( const wxString& aText, const wxString& aPrefix, const std::vector<wxString>& aPath,
                       std::vector<BUS_SCHEMA::NODE>& aMembers )
    {
        wxString key = UnescapeString( aText );
        key.Replace( "\\ ", " " );
        key = key.Strip( wxString::both );
        m_schema.aliasesUsed.insert( key );
        const auto alias = m_aliases.find( key );

        if( alias == m_aliases.end() )
        {
            aMembers.emplace_back();
            return expand( aText, aPrefix, aPath, aMembers.back(), false );
        }

        if( !m_activeAliases.insert( key ).second )
            return false;

        for( const wxString& text : alias->second.get() )
        {
            const wxString member = text.Strip( wxString::both );

            if( !member.IsEmpty()
                && !expandMember( EscapeString( member, CTX_NETNAME ), aPrefix, aPath, aMembers ) )
                return false;
        }

        m_activeAliases.erase( key );
        return true;
    }

    bool expand( const wxString& aText, wxString aPrefix, std::vector<wxString> aPath,
                 BUS_SCHEMA::NODE& aNode, bool aRoot )
    {
        const wxString        text = UnescapeString( aText );
        wxString              prefix;
        std::vector<wxString> members;

        if( NET_SETTINGS::ParseBusVector( text, &prefix, &members ) )
        {
            aNode.kind = BUS_SCHEMA::NODE::KIND::VECTOR;
            aNode.text = aText;
            aNode.prefix = EscapeString( prefix, CTX_NETNAME );

            if( aRoot )
            {
                m_schema.shape = BUS_SCHEMA::SHAPE::VECTOR;
                m_schema.prefix = EscapeString( prefix, CTX_NETNAME );
            }

            for( const wxString& member : members )
            {
                aNode.members.emplace_back();
                addLeaf( member, aPrefix, aPath, aNode.members.back() );
            }

            return true;
        }

        size_t prefixEnd = 0;

        if( NET_SETTINGS::ParseBusGroup( text, &prefix, &members, &prefixEnd ) )
        {
            aNode.kind = BUS_SCHEMA::NODE::KIND::GROUP;
            aNode.text = aText;
            aNode.prefix = EscapeString( prefix, CTX_NETNAME );

            if( aRoot )
            {
                m_schema.shape = BUS_SCHEMA::SHAPE::GROUP;
                m_schema.prefix = EscapeString( prefix, CTX_NETNAME );
                const wxString sourcePrefix = text.Left( prefixEnd );
                const auto matches = [&]( size_t offset )
                {
                    return offset < aText.length() && aText[offset] == '{'
                           && UnescapeString( aText.Left( offset ) ) == sourcePrefix;
                };
                m_schema.prefixEnd = EscapeString( sourcePrefix, CTX_NETNAME ).length();

                // Other escape contexts have tokens that CTX_NETNAME does not re-emit
                if( !matches( m_schema.prefixEnd ) )
                {
                    m_schema.prefixEnd = aText.find( '{' );

                    while( m_schema.prefixEnd != wxString::npos && !matches( m_schema.prefixEnd ) )
                        m_schema.prefixEnd = aText.find( '{', m_schema.prefixEnd + 1 );

                    if( m_schema.prefixEnd == wxString::npos )
                        return false;
                }
            }
            else if( !prefix.IsEmpty() )
            {
                aPath.push_back( EscapeString( prefix, CTX_NETNAME ) );
            }

            if( !prefix.IsEmpty() )
                aPrefix += prefix + ".";

            for( const wxString& member : members )
            {
                if( !expandMember( member, aPrefix, aPath, aNode.members ) )
                    return false;
            }

            return true;
        }

        if( aRoot )
            return false;

        wxString local = text;
        local.Replace( "\\ ", " " );
        addLeaf( local, aPrefix, aPath, aNode );
        return true;
    }

    void addLeaf( const wxString& aLocal, const wxString& aPrefix, const std::vector<wxString>& aPath,
                  BUS_SCHEMA::NODE& aNode )
    {
        aNode.leaf = m_schema.leaves.size();
        m_schema.leaves.push_back(
                { EscapeString( aPrefix + aLocal, CTX_NETNAME ), EscapeString( aLocal, CTX_NETNAME ), aPath } );
    }

    std::map<wxString, std::reference_wrapper<const std::vector<wxString>>> m_aliases;
    std::set<wxString>                                                      m_activeAliases;
    BUS_SCHEMA                                                              m_schema;
};
} // namespace

std::optional<SCH_CONNECTIVITY::BUS_SCHEMA> SCH_CONNECTIVITY::BUS_SCHEMA::Parse( const wxString&    aText,
                                                                                 const BUS_ALIASES& aAliases )
{
    return SCHEMA_PARSER( aAliases ).Parse( aText );
}

bool SCH_CONNECTIVITY::BUS_PARSE_CACHE::SetAliases( const BUS_ALIASES& aAliases )
{
    BUS_ALIASES normalized;

    for( const auto& [name, members] : aAliases )
    {
        std::vector<wxString> values;
        values.reserve( members.size() );

        for( const wxString& member : members )
            values.push_back( member.Strip( wxString::both ) );

        normalized.insert_or_assign( name.Strip( wxString::both ), std::move( values ) );
    }

    if( normalized == m_aliases )
        return false;

    // Stage invalidations so allocation failure leaves the previous alias environment usable
    auto dirty = m_dirty;

    for( const auto& [text, entry] : m_cache.Entries() )
    {
        for( const wxString& name : entry->value.aliasesUsed )
        {
            const auto before = m_aliases.find( name );
            const auto after = normalized.find( name );

            if( ( before == m_aliases.end() ) != ( after == normalized.end() )
                || ( before != m_aliases.end() && after != normalized.end() && before->second != after->second ) )
            {
                dirty.insert( text );
                break;
            }
        }
    }

    m_dirty.swap( dirty );
    m_aliases.swap( normalized );
    return true;
}

const SCH_CONNECTIVITY::BUS_PARSE_CACHE::ENTRY& SCH_CONNECTIVITY::BUS_PARSE_CACHE::Parse( const wxString& aText )
{
    const ENTRY* cached = m_cache.Find( aText );

    if( cached && !m_dirty.contains( aText ) )
        return *cached;

    RESULT result;

    if( auto schema = SCHEMA_PARSER( m_aliases ).Parse( aText, &result.aliasesUsed ) )
    {
        result.tree = std::make_shared<const BUS_SCHEMA::NODE>( std::exchange( schema->root, {} ) );

        if( cached && cached->value.schema && *cached->value.schema == *schema )
            result.schema = cached->value.schema;
        else
            result.schema = std::make_shared<const BUS_SCHEMA>( std::move( *schema ) );
    }

    const ENTRY& entry = m_cache.Set( aText, std::move( result ) );
    m_dirty.erase( aText );
    return entry;
}

void SCH_CONNECTIVITY::BUS_PARSE_CACHE::Retain( const std::set<wxString>& aLive )
{
    for( auto it = m_cache.Entries().begin(); it != m_cache.Entries().end(); )
    {
        const auto current = it++;

        if( !aLive.contains( current->first ) )
        {
            m_dirty.erase( current->first );
            m_cache.Erase( current->first );
        }
    }
}

std::shared_ptr<const SCH_CONNECTIVITY::BUS_SCHEMA::NODE>
SCH_CONNECTIVITY::BUS_PARSE_CACHE::FindTree( const wxString& aText ) const
{
    if( m_dirty.contains( aText ) )
        return {};

    const ENTRY* entry = m_cache.Find( aText );
    return entry ? entry->value.tree : nullptr;
}

SCH_CONNECTIVITY::BUS_ALIGNMENT SCH_CONNECTIVITY::Align( const BUS_SCHEMA& aLeft, const BUS_SCHEMA& aRight )
{
    BUS_ALIGNMENT result;
    const bool    byName = aLeft.shape == BUS_SCHEMA::SHAPE::GROUP && aRight.shape == BUS_SCHEMA::SHAPE::GROUP;
    using KEY = std::pair<std::vector<wxString>, wxString>;
    std::map<KEY, std::vector<size_t>> candidates;

    if( byName )
    {
        for( size_t right = aRight.leaves.size(); right-- > 0; )
        {
            const auto& leaf = aRight.leaves[right];
            candidates[{ leaf.groupPath, leaf.localName }].push_back( right );
        }
    }

    for( size_t left = 0; left < aLeft.leaves.size(); ++left )
    {
        size_t right = left;

        if( byName )
        {
            const auto& leaf = aLeft.leaves[left];
            const auto  found = candidates.find( { leaf.groupPath, leaf.localName } );
            right = aRight.leaves.size();

            if( found != candidates.end() && !found->second.empty() )
            {
                right = found->second.back();
                found->second.pop_back();
            }
        }

        if( right < aRight.leaves.size() )
            result.matched.emplace_back( left, right );
        else
            result.unmappedLeft.push_back( left );
    }

    return result;
}
