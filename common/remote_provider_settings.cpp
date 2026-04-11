/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
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

#include <remote_provider_settings.h>

#include <algorithm>

#include <json_conversions.h>
#include <picosha2.h>

#include <nlohmann/json.hpp>


namespace
{
wxString normalizeProviderUrl( const wxString& aUrl )
{
    wxString normalized = aUrl;

    normalized.Trim( true ).Trim( false );

    if( normalized.EndsWith( wxS( "/" ) ) )
        normalized.RemoveLast();

    return normalized.Lower();
}
} // namespace


wxString REMOTE_PROVIDER_SETTINGS::DefaultDestinationDir()
{
    return wxS( "${KIPRJMOD}/RemoteLibrary" );
}


wxString REMOTE_PROVIDER_SETTINGS::DefaultLibraryPrefix()
{
    return wxS( "remote" );
}


wxString REMOTE_PROVIDER_SETTINGS::CreateProviderId( const wxString& aMetadataUrl )
{
    const std::string normalized = normalizeProviderUrl( aMetadataUrl ).ToStdString();
    std::string       hashHex;

    picosha2::hash256_hex_string( normalized, hashHex );

    return wxString::Format( wxS( "provider-%s" ),
                             wxString::FromUTF8( hashHex.substr( 0, 12 ).c_str() ) );
}


void REMOTE_PROVIDER_SETTINGS::ResetToDefaults()
{
    providers.clear();
    last_used_provider_id.clear();
    destination_dir = DefaultDestinationDir();
    library_prefix = DefaultLibraryPrefix();
    add_to_global_table = false;
}


REMOTE_PROVIDER_ENTRY* REMOTE_PROVIDER_SETTINGS::FindProviderById( const wxString& aProviderId )
{
    auto it = std::find_if( providers.begin(), providers.end(),
                            [&]( const REMOTE_PROVIDER_ENTRY& aEntry )
                            {
                                return aEntry.provider_id == aProviderId;
                            } );

    return it == providers.end() ? nullptr : &(*it);
}


const REMOTE_PROVIDER_ENTRY* REMOTE_PROVIDER_SETTINGS::FindProviderById(
        const wxString& aProviderId ) const
{
    return const_cast<REMOTE_PROVIDER_SETTINGS*>( this )->FindProviderById( aProviderId );
}


REMOTE_PROVIDER_ENTRY* REMOTE_PROVIDER_SETTINGS::FindProviderByMetadataUrl(
        const wxString& aMetadataUrl )
{
    const wxString normalized = normalizeProviderUrl( aMetadataUrl );

    auto it = std::find_if( providers.begin(), providers.end(),
                            [&]( const REMOTE_PROVIDER_ENTRY& aEntry )
                            {
                                return normalizeProviderUrl( aEntry.metadata_url ) == normalized;
                            } );

    return it == providers.end() ? nullptr : &(*it);
}


const REMOTE_PROVIDER_ENTRY* REMOTE_PROVIDER_SETTINGS::FindProviderByMetadataUrl(
        const wxString& aMetadataUrl ) const
{
    return const_cast<REMOTE_PROVIDER_SETTINGS*>( this )->FindProviderByMetadataUrl( aMetadataUrl );
}


REMOTE_PROVIDER_ENTRY& REMOTE_PROVIDER_SETTINGS::UpsertProvider( const wxString& aMetadataUrl )
{
    if( REMOTE_PROVIDER_ENTRY* existing = FindProviderByMetadataUrl( aMetadataUrl ) )
        return *existing;

    REMOTE_PROVIDER_ENTRY provider;
    provider.metadata_url = normalizeProviderUrl( aMetadataUrl );
    provider.provider_id = CreateProviderId( provider.metadata_url );
    provider.last_auth_status = wxS( "signed_out" );
    providers.push_back( provider );
    return providers.back();
}


void to_json( nlohmann::json& aJson, const REMOTE_PROVIDER_ENTRY& aEntry )
{
    aJson = nlohmann::json{
        { "provider_id", aEntry.provider_id },
        { "metadata_url", aEntry.metadata_url },
        { "display_name_override", aEntry.display_name_override },
        { "last_account_label", aEntry.last_account_label },
        { "last_auth_status", aEntry.last_auth_status }
    };
}


void from_json( const nlohmann::json& aJson, REMOTE_PROVIDER_ENTRY& aEntry )
{
    aEntry = REMOTE_PROVIDER_ENTRY();

    if( aJson.contains( "provider_id" ) )
        aEntry.provider_id = aJson.at( "provider_id" ).get<wxString>();

    if( aJson.contains( "metadata_url" ) )
        aEntry.metadata_url = aJson.at( "metadata_url" ).get<wxString>();

    if( aJson.contains( "display_name_override" ) )
        aEntry.display_name_override = aJson.at( "display_name_override" ).get<wxString>();

    if( aJson.contains( "last_account_label" ) )
        aEntry.last_account_label = aJson.at( "last_account_label" ).get<wxString>();

    if( aJson.contains( "last_auth_status" ) )
        aEntry.last_auth_status = aJson.at( "last_auth_status" ).get<wxString>();

    aEntry.metadata_url = normalizeProviderUrl( aEntry.metadata_url );

    if( aEntry.provider_id.IsEmpty() && !aEntry.metadata_url.IsEmpty() )
        aEntry.provider_id = REMOTE_PROVIDER_SETTINGS::CreateProviderId( aEntry.metadata_url );
}


void to_json( nlohmann::json& aJson, const REMOTE_PROVIDER_SETTINGS& aSettings )
{
    aJson = nlohmann::json{
        { "providers", aSettings.providers },
        { "last_used_provider_id", aSettings.last_used_provider_id },
        { "destination_dir", aSettings.destination_dir },
        { "library_prefix", aSettings.library_prefix },
        { "add_to_global_table", aSettings.add_to_global_table }
    };
}


void from_json( const nlohmann::json& aJson, REMOTE_PROVIDER_SETTINGS& aSettings )
{
    aSettings.ResetToDefaults();

    if( aJson.contains( "providers" ) )
        aSettings.providers = aJson.at( "providers" ).get<std::vector<REMOTE_PROVIDER_ENTRY>>();

    if( aJson.contains( "last_used_provider_id" ) )
        aSettings.last_used_provider_id = aJson.at( "last_used_provider_id" ).get<wxString>();

    if( aJson.contains( "destination_dir" ) )
        aSettings.destination_dir = aJson.at( "destination_dir" ).get<wxString>();

    if( aJson.contains( "library_prefix" ) )
        aSettings.library_prefix = aJson.at( "library_prefix" ).get<wxString>();

    if( aJson.contains( "add_to_global_table" ) )
        aSettings.add_to_global_table = aJson.at( "add_to_global_table" ).get<bool>();
}
