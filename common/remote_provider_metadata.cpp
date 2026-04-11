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

#include <remote_provider_metadata.h>

#include <set>

#include <json_schema_validator.h>
#include <paths.h>
#include <remote_provider_utils.h>
#include <wx_filename.h>
#include <wx/intl.h>


wxFileName REMOTE_PROVIDER_METADATA::DefaultSchemaPath()
{
    wxFileName schemaFile( PATHS::GetStockDataPath( true ),
                           wxS( "kicad-remote-provider-metadata-v1.schema.json" ) );
    schemaFile.AppendDir( wxS( "schemas" ) );
    schemaFile.Normalize( FN_NORMALIZE_FLAGS );
    return schemaFile;
}


std::optional<REMOTE_PROVIDER_METADATA> REMOTE_PROVIDER_METADATA::FromJson(
        const nlohmann::json& aJson, wxString& aError )
{
    return FromJson( aJson, DefaultSchemaPath(), aError );
}


std::optional<REMOTE_PROVIDER_METADATA> REMOTE_PROVIDER_METADATA::FromJson(
        const nlohmann::json& aJson, const wxFileName& aSchemaFile, wxString& aError )
{
    if( !aSchemaFile.IsFileReadable() )
    {
        aError = wxString::Format( _( "Remote provider metadata schema '%s' is not readable." ),
                                   aSchemaFile.GetFullPath() );
        return std::nullopt;
    }

    COLLECTING_JSON_ERROR_HANDLER handler;
    JSON_SCHEMA_VALIDATOR         validator( aSchemaFile );
    validator.Validate( aJson, handler );

    if( handler.HasErrors() )
    {
        aError = wxString::Format( _( "Remote provider metadata failed schema validation: %s" ),
                                   handler.FirstError() );
        return std::nullopt;
    }

    REMOTE_PROVIDER_METADATA metadata;

    try
    {
        metadata.provider_name =
                wxString::FromUTF8( aJson.at( "provider_name" ).get_ref<const std::string&>().c_str() );
        metadata.provider_version =
                wxString::FromUTF8( aJson.at( "provider_version" ).get_ref<const std::string&>().c_str() );
        metadata.api_base_url =
                wxString::FromUTF8( aJson.at( "api_base_url" ).get_ref<const std::string&>().c_str() );
        metadata.panel_url = RemoteProviderJsonString( aJson, "panel_url" );
        metadata.session_bootstrap_url = RemoteProviderJsonString( aJson, "session_bootstrap_url" );
        metadata.max_download_bytes = aJson.at( "max_download_bytes" ).get<long long>();
        metadata.allow_insecure_localhost = aJson.value( "allow_insecure_localhost", false );
        metadata.documentation_url = RemoteProviderJsonString( aJson, "documentation_url" );
        metadata.terms_url = RemoteProviderJsonString( aJson, "terms_url" );
        metadata.privacy_url = RemoteProviderJsonString( aJson, "privacy_url" );

        for( const nlohmann::json& assetType : aJson.at( "supported_asset_types" ) )
            metadata.supported_asset_types.emplace_back( wxString::FromUTF8(
                    assetType.get_ref<const std::string&>().c_str() ) );

        const nlohmann::json& auth = aJson.at( "auth" );
        wxString authType = RemoteProviderJsonString( auth, "type" );

        if( authType.IsSameAs( wxS( "oauth2" ), false ) )
        {
            metadata.auth.type = REMOTE_PROVIDER_AUTH_TYPE::OAUTH2;
            metadata.auth.metadata_url = RemoteProviderJsonString( auth, "metadata_url" );
            metadata.auth.client_id = RemoteProviderJsonString( auth, "client_id" );

            if( auth.contains( "scopes" ) )
            {
                for( const nlohmann::json& scope : auth.at( "scopes" ) )
                    metadata.auth.scopes.emplace_back(
                            wxString::FromUTF8( scope.get_ref<const std::string&>().c_str() ) );
            }
        }
        else
        {
            metadata.auth.type = REMOTE_PROVIDER_AUTH_TYPE::NONE;
        }

        const nlohmann::json& capabilities = aJson.at( "capabilities" );
        static const std::set<std::string> supportedCapabilities = {
            "web_ui_v1",
            "parts_v1",
            "direct_downloads_v1",
            "inline_payloads_v1"
        };

        for( const auto& [name, value] : capabilities.items() )
        {
            if( !supportedCapabilities.count( name ) )
            {
                aError = wxString::Format( _( "Unsupported provider capability '%s'." ),
                                           wxString::FromUTF8( name.c_str() ) );
                return std::nullopt;
            }

            if( name == "web_ui_v1" )
                metadata.web_ui_v1 = value.get<bool>();
            else if( name == "parts_v1" )
                metadata.parts_v1 = value.get<bool>();
            else if( name == "direct_downloads_v1" )
                metadata.direct_downloads_v1 = value.get<bool>();
            else if( name == "inline_payloads_v1" )
                metadata.inline_payloads_v1 = value.get<bool>();
        }

        if( aJson.contains( "parts" ) )
        {
            const nlohmann::json& parts = aJson.at( "parts" );
            metadata.parts_endpoint_template = RemoteProviderJsonString( parts, "endpoint_template" );
        }
    }
    catch( const std::exception& e )
    {
        aError = wxString::Format( _( "Unable to parse remote provider metadata: %s" ),
                                   wxString::FromUTF8( e.what() ) );
        return std::nullopt;
    }

    if( !metadata.web_ui_v1 )
    {
        aError = _( "Remote provider metadata must enable web_ui_v1." );
        return std::nullopt;
    }

    if( !metadata.direct_downloads_v1 && !metadata.inline_payloads_v1 )
    {
        aError = _(
                "Remote provider metadata must enable direct_downloads_v1, inline_payloads_v1, or both." );
        return std::nullopt;
    }

    if( metadata.auth.type == REMOTE_PROVIDER_AUTH_TYPE::OAUTH2
        && metadata.session_bootstrap_url.IsEmpty() )
    {
        aError = _( "Remote provider metadata must define session_bootstrap_url for oauth2." );
        return std::nullopt;
    }

    if( !ValidateRemoteUrlSecurity( metadata.api_base_url, metadata.allow_insecure_localhost, aError, "api_base_url" ) )
    {
        return std::nullopt;
    }

    if( !ValidateRemoteUrlSecurity( metadata.panel_url, metadata.allow_insecure_localhost, aError, "panel_url" ) )
    {
        return std::nullopt;
    }

    if( !ValidateRemoteUrlSecurity( metadata.session_bootstrap_url, metadata.allow_insecure_localhost, aError,
                                    "session_bootstrap_url" ) )
    {
        return std::nullopt;
    }

    if( metadata.auth.type == REMOTE_PROVIDER_AUTH_TYPE::OAUTH2
        && NormalizedUrlOrigin( metadata.panel_url ) != NormalizedUrlOrigin( metadata.session_bootstrap_url ) )
    {
        aError = _( "session_bootstrap_url must share the same origin as panel_url." );
        return std::nullopt;
    }

    if( metadata.auth.type == REMOTE_PROVIDER_AUTH_TYPE::OAUTH2
        && !ValidateRemoteUrlSecurity( metadata.auth.metadata_url, metadata.allow_insecure_localhost, aError,
                                       "auth.metadata_url" ) )
    {
        return std::nullopt;
    }

    return metadata;
}
