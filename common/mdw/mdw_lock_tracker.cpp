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

#include <mdw/mdw_lock_tracker.h>
#include <mdw/client/mdw_events.h>

#include <nlohmann/json.hpp>

#include <algorithm>

#include <wx/datetime.h>


namespace
{

/// The lock an ITEM_LOCKED event names, with each field its payload carries overriding the envelope's guess
MDW_LOCK_INFO lockOf( const MDW_EVENT& aEvent )
{
    MDW_LOCK_INFO info;
    info.is_locked = true;
    info.owner_id = aEvent.client_identity;
    info.owner_display_name = aEvent.client_identity;
    info.locked_by_me = aEvent.from_this_client;

    nlohmann::json payload = nlohmann::json::parse( aEvent.payload.utf8_string(), nullptr, false );

    if( !payload.is_object() )
        return info;

    auto text = [&]( const char* aKey, wxString& aField )
    {
        if( auto it = payload.find( aKey ); it != payload.end() && it->is_string() )
            aField = wxString::FromUTF8( it->get<std::string>() );
    };

    auto flag = [&]( const char* aKey, bool& aField )
    {
        if( auto it = payload.find( aKey ); it != payload.end() && it->is_boolean() )
            aField = it->get<bool>();
    };

    text( "owner_id", info.owner_id );
    text( "owner_display_name", info.owner_display_name );

    // The backend judges ownership by user, so a lock taken from another of our clients is still ours
    flag( "locked_by_me", info.locked_by_me );
    flag( "advisory", info.advisory );

    if( auto it = payload.find( "lock_timestamp_unix" ); it != payload.end() && it->is_number_integer() )
    {
        // Zero is the protocol's absent timestamp, as for GetLockInfo
        if( std::int64_t since = it->get<std::int64_t>(); since != 0 )
            info.lock_timestamp = wxDateTime( static_cast<time_t>( since ) );
    }

    return info;
}

} // namespace


void MDW_LOCK_TRACKER::Set( const wxString& aItemId, const MDW_LOCK_INFO& aInfo )
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_changed[aItemId] = ++m_clock;

    if( aInfo.is_locked )
        m_locks[aItemId] = aInfo;
    else
        m_locks.erase( aItemId );
}


std::uint64_t MDW_LOCK_TRACKER::generationOf( const wxString& aItemId ) const
{
    auto it = m_changed.find( aItemId );

    return std::max( m_snapshot, it == m_changed.end() ? 0 : it->second );
}


std::uint64_t MDW_LOCK_TRACKER::Generation( const wxString& aItemId ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );

    return generationOf( aItemId );
}


bool MDW_LOCK_TRACKER::SetIfUnchanged( const wxString& aItemId, const MDW_LOCK_INFO& aInfo,
                                       std::uint64_t aGeneration )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    if( generationOf( aItemId ) != aGeneration )
        return false;

    m_changed[aItemId] = ++m_clock;

    if( aInfo.is_locked )
        m_locks[aItemId] = aInfo;
    else
        m_locks.erase( aItemId );

    return true;
}


MDW_LOCK_INFO MDW_LOCK_TRACKER::Get( const wxString& aItemId ) const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    auto                        it = m_locks.find( aItemId );

    if( it != m_locks.end() )
        return it->second;

    MDW_LOCK_INFO free;
    free.known = m_seeded;
    return free;
}


void MDW_LOCK_TRACKER::ApplyEvent( const MDW_EVENT& aEvent )
{
    if( aEvent.item_id.IsEmpty() )
        return;

    if( aEvent.type == MDW_EVENT_TYPE::ITEM_LOCKED )
        Set( aEvent.item_id, lockOf( aEvent ) );
    else if( aEvent.type == MDW_EVENT_TYPE::ITEM_UNLOCKED || aEvent.type == MDW_EVENT_TYPE::REVISION_RELEASED )
    {
        // A release gives up the claim that revised the item
        Set( aEvent.item_id, MDW_LOCK_INFO() );
    }
}


std::uint64_t MDW_LOCK_TRACKER::Clock() const
{
    std::lock_guard<std::mutex> lock( m_mutex );
    return m_clock;
}


void MDW_LOCK_TRACKER::ReplaceAll( const std::map<wxString, MDW_LOCK_INFO>& aLocks, std::uint64_t aSince )
{
    std::lock_guard<std::mutex> lock( m_mutex );

    auto unchanged = [&]( const wxString& aItemId )
    {
        return generationOf( aItemId ) <= aSince;
    };

    std::erase_if( m_locks,
                   [&]( const auto& aEntry )
                   {
                       return unchanged( aEntry.first );
                   } );

    for( const auto& [item, info] : aLocks )
    {
        if( info.is_locked && unchanged( item ) )
            m_locks[item] = info;
    }

    // Every item now dates from the snapshot, so an item query begun before it cannot store a stale lock
    m_snapshot = ++m_clock;
    m_changed.clear();
    m_seeded = true;
}


void MDW_LOCK_TRACKER::Invalidate()
{
    std::lock_guard<std::mutex> lock( m_mutex );

    // A lock held when events stopped may have gone since, so it reads as unknown too
    m_locks.clear();
    m_seeded = false;
}


void MDW_LOCK_TRACKER::TrustEvents()
{
    std::lock_guard<std::mutex> lock( m_mutex );
    m_seeded = true;
}
