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

#include "mdw_git_push_outcomes.h"

#include <mdw_runtime/mdw_file_io.h>

#include <nlohmann/json.hpp>

#include <ctime>
#include <optional>

namespace ws = kiapi::workspace;


namespace
{

std::string keyOf( const std::string& aRemote, const std::string& aUser )
{
    return aRemote + '\n' + aUser;
}


nlohmann::json load( const std::filesystem::path& aFile )
{
    std::optional<std::string> text = MDW_ReadFile( aFile );
    nlohmann::json             json = text ? nlohmann::json::parse( *text, nullptr, false ) : nlohmann::json();

    return json.is_object() ? json : nlohmann::json::object();
}

} // namespace


MDW_GIT_PUSH_OUTCOMES::MDW_GIT_PUSH_OUTCOMES( std::filesystem::path aFile ) :
        m_file( std::move( aFile ) )
{
}


MDW_GIT_PUSH_OUTCOMES::OUTCOME MDW_GIT_PUSH_OUTCOMES::Get( const std::string& aRemote, const std::string& aUser,
                                                           std::uint64_t aGeneration ) const
{
    const std::string key = keyOf( aRemote, aUser );

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        auto                        it = m_seen.find( key );

        if( it != m_seen.end() && it->second.first == aGeneration )
            return { it->second.second, false };
    }

    if( m_file.empty() )
        return {};

    const nlohmann::json json = load( m_file );
    auto                 it = json.find( key );

    return { ws::PS_UNKNOWN, it != json.end() && it->is_object() && it->value( "state", "" ) == "denied" };
}


void MDW_GIT_PUSH_OUTCOMES::Set( const std::string& aRemote, const std::string& aUser, std::uint64_t aGeneration,
                                 ws::PermissionState aState )
{
    const std::string key = keyOf( aRemote, aUser );

    {
        std::lock_guard<std::mutex> lock( m_mutex );
        m_seen[key] = { aGeneration, aState };
    }

    if( m_file.empty() )
        return;

    // The record only warns, so a failure to keep it must never fail the push it describes
    try
    {
        std::error_code ec;
        std::filesystem::create_directories( m_file.parent_path(), ec );

        std::filesystem::path lockPath = m_file;
        lockPath += ".lock";

        // Backend processes share the file, so a read and its rewrite must not interleave with another's
        MDW_FILE_LOCK lock( lockPath );

        if( !lock.Locked() )
            return;

        nlohmann::json json = load( m_file );

        json[key] = { { "state", aState == ws::PS_DENIED ? "denied" : "allowed" },
                      { "at", static_cast<std::int64_t>( std::time( nullptr ) ) } };

        MDW_ReplaceFile( m_file, json.dump(), 0600 );
    }
    catch( const std::exception& )
    {
    }
}
