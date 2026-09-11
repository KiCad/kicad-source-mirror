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

#include "conn_keys.h"

#include <hashtables.h>
#include <optional>
#include <type_traits>

namespace SCH_CONNECTIVITY
{
NODE_ID SESSION_KEYS::InternNode( NODE_KEY aKey )
{
    if( auto* name = std::get_if<NAME_KEY>( &aKey ) )
    {
        Name( name->text );

        if( name->scope == SCOPE::GLOBAL )
            name->inst = 0;
        else
            Instance( name->inst );

        if( name->scope != SCOPE::PORT )
            name->kind = KIND::SIGNAL;
    }
    else if( const auto* record = std::get_if<RECORD_NODE>( &aKey ) )
    {
        Instance( record->record.inst );
    }
    else
    {
        Instance( std::get<SLOT_KEY>( aKey ).bundleDriver.inst );
    }

    return m_nodes.Intern( aKey );
}

size_t SESSION_KEYS::NODE_HASH::operator()( const NODE_KEY& aKey ) const
{
    size_t hash = aKey.index();

    if( const auto* record = std::get_if<RECORD_NODE>( &aKey ) )
    {
        hash = KiHashCombine( hash, record->record.inst );
        hash = KiHashCombine( hash, std::hash<KIID>{}( record->record.anchor ) );
        hash = KiHashCombine( hash, static_cast<size_t>( record->kind ) );
    }
    else if( const auto* name = std::get_if<NAME_KEY>( &aKey ) )
    {
        hash = KiHashCombine( hash, static_cast<size_t>( name->scope ) );
        hash = KiHashCombine( hash, name->text );

        if( name->scope != SCOPE::GLOBAL )
            hash = KiHashCombine( hash, name->inst );

        if( name->scope == SCOPE::PORT )
            hash = KiHashCombine( hash, static_cast<size_t>( name->kind ) );
    }
    else
    {
        const auto& slot = std::get<SLOT_KEY>( aKey );
        hash = KiHashCombine( hash, std::hash<KIID>{}( slot.bundleDriver.item ) );
        hash = KiHashCombine( hash, slot.bundleDriver.inst );
        hash = KiHashCombine( hash, slot.leaf );
    }

    return hash;
}

namespace
{
    // Default keys hold INVALID_ID, which no table resolves, so unset ids order first without a lookup.
    std::optional<bool> unsetIdLess( uint32_t aLeft, uint32_t aRight )
    {
        if( aLeft != INVALID_ID && aRight != INVALID_ID )
            return std::nullopt;

        return aLeft == INVALID_ID && aRight != INVALID_ID;
    }
} // namespace

bool SESSION_KEYS::Less( const ITEM_KEY& aLeft, const ITEM_KEY& aRight ) const
{
    if( aLeft.item != aRight.item )
        return aLeft.item < aRight.item;

    if( aLeft.inst == aRight.inst )
        return false;

    if( std::optional<bool> order = unsetIdLess( aLeft.inst, aRight.inst ) )
        return *order;

    return Instance( aLeft.inst ) < Instance( aRight.inst );
}

bool SESSION_KEYS::Less( const RECORD_KEY& aLeft, const RECORD_KEY& aRight ) const
{
    if( aLeft.inst == aRight.inst )
        return aLeft.anchor < aRight.anchor;

    if( std::optional<bool> order = unsetIdLess( aLeft.inst, aRight.inst ) )
        return *order;

    return Instance( aLeft.inst ) < Instance( aRight.inst );
}

bool SESSION_KEYS::Less( const RECORD_NODE& aLeft, const RECORD_NODE& aRight ) const
{
    return aLeft.record == aRight.record ? aLeft.kind < aRight.kind : Less( aLeft.record, aRight.record );
}

bool SESSION_KEYS::Less( const NAME_KEY& aLeft, const NAME_KEY& aRight ) const
{
    if( aLeft.scope != aRight.scope )
        return aLeft.scope < aRight.scope;

    if( aLeft.scope != SCOPE::GLOBAL && aLeft.inst != aRight.inst )
    {
        if( std::optional<bool> order = unsetIdLess( aLeft.inst, aRight.inst ) )
            return *order;

        return Instance( aLeft.inst ) < Instance( aRight.inst );
    }

    if( aLeft.text != aRight.text )
    {
        if( std::optional<bool> order = unsetIdLess( aLeft.text, aRight.text ) )
            return *order;

        return NameLess( aLeft.text, aRight.text );
    }

    return aLeft.scope == SCOPE::PORT && aLeft.kind < aRight.kind;
}

bool SESSION_KEYS::Less( const SLOT_KEY& aLeft, const SLOT_KEY& aRight ) const
{
    return aLeft.bundleDriver == aRight.bundleDriver ? aLeft.leaf < aRight.leaf
                                                     : Less( aLeft.bundleDriver, aRight.bundleDriver );
}

bool SESSION_KEYS::Less( const NODE_KEY& aLeft, const NODE_KEY& aRight ) const
{
    if( aLeft.index() != aRight.index() )
        return aLeft.index() < aRight.index();

    return std::visit(
            [&]( const auto& left )
            {
                return Less( left, std::get<std::decay_t<decltype( left )>>( aRight ) );
            },
            aLeft );
}
} // namespace SCH_CONNECTIVITY
