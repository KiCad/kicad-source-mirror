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

#include <remote_provider_models.h>

#include <json_schema_validator.h>
#include <paths.h>
#include <remote_provider_utils.h>
#include <wx_filename.h>
#include <wx/intl.h>


std::optional<REMOTE_PROVIDER_OAUTH_SERVER_METADATA>
REMOTE_PROVIDER_OAUTH_SERVER_METADATA::FromJson( const nlohmann::json& aJson, bool aAllowInsecureLocalhost,
                                                 wxString& aError )
{
    REMOTE_PROVIDER_OAUTH_SERVER_METADATA metadata;

    try
    {
        metadata.issuer = RemoteProviderJsonString( aJson, "issuer" );
        metadata.authorization_endpoint = RemoteProviderJsonString( aJson, "authorization_endpoint" );
        metadata.token_endpoint = RemoteProviderJsonString( aJson, "token_endpoint" );
        metadata.revocation_endpoint = RemoteProviderJsonString( aJson, "revocation_endpoint" );
    }
    catch( const std::exception& e )
    {
        aError = wxString::Format( _( "Unable to parse OAuth metadata: %s" ), wxString::FromUTF8( e.what() ) );
        return std::nullopt;
    }

    if( metadata.authorization_endpoint.IsEmpty() || metadata.token_endpoint.IsEmpty() )
    {
        aError = _( "OAuth metadata must include authorization_endpoint and token_endpoint." );
        return std::nullopt;
    }

    if( !ValidateRemoteUrlSecurity( metadata.authorization_endpoint, aAllowInsecureLocalhost, aError,
                                    "authorization_endpoint" ) )
    {
        return std::nullopt;
    }

    if( !ValidateRemoteUrlSecurity( metadata.token_endpoint, aAllowInsecureLocalhost, aError, "token_endpoint" ) )
    {
        return std::nullopt;
    }

    if( !ValidateRemoteUrlSecurity( metadata.revocation_endpoint, aAllowInsecureLocalhost, aError,
                                    "revocation_endpoint" ) )
    {
        return std::nullopt;
    }

    return metadata;
}


wxFileName REMOTE_PROVIDER_PART_MANIFEST::DefaultSchemaPath()
{
    wxFileName schemaFile( PATHS::GetStockDataPath( true ), wxS( "kicad-remote-symbol-manifest-v1.schema.json" ) );
    schemaFile.AppendDir( wxS( "schemas" ) );
    schemaFile.Normalize( FN_NORMALIZE_FLAGS );
    return schemaFile;
}


std::optional<REMOTE_PROVIDER_PART_MANIFEST>
REMOTE_PROVIDER_PART_MANIFEST::FromJson( const nlohmann::json& aJson, bool aAllowInsecureLocalhost, wxString& aError )
{
    return FromJson( aJson, DefaultSchemaPath(), aAllowInsecureLocalhost, aError );
}


std::optional<REMOTE_PROVIDER_PART_MANIFEST> REMOTE_PROVIDER_PART_MANIFEST::FromJson( const nlohmann::json& aJson,
                                                                                      const wxFileName&     aSchemaFile,
                                                                                      bool      aAllowInsecureLocalhost,
                                                                                      wxString& aError )
{
    if( !aSchemaFile.IsFileReadable() )
    {
        aError = wxString::Format( _( "Remote provider manifest schema '%s' is not readable." ),
                                   aSchemaFile.GetFullPath() );
        return std::nullopt;
    }

    COLLECTING_JSON_ERROR_HANDLER handler;
    JSON_SCHEMA_VALIDATOR    validator( aSchemaFile );
    validator.Validate( aJson, handler );

    if( handler.HasErrors() )
    {
        aError = wxString::Format( _( "Remote provider manifest failed schema validation: %s" ), handler.FirstError() );
        return std::nullopt;
    }

    REMOTE_PROVIDER_PART_MANIFEST manifest;

    try
    {
        manifest.part_id = RemoteProviderJsonString( aJson, "part_id" );
        manifest.display_name = RemoteProviderJsonString( aJson, "display_name" );
        manifest.summary = RemoteProviderJsonString( aJson, "summary" );
        manifest.license = RemoteProviderJsonString( aJson, "license" );

        for( const nlohmann::json& assetJson : aJson.at( "assets" ) )
        {
            REMOTE_PROVIDER_PART_ASSET asset;
            asset.asset_type = RemoteProviderJsonString( assetJson, "asset_type" );
            asset.name = RemoteProviderJsonString( assetJson, "name" );
            asset.target_library = RemoteProviderJsonString( assetJson, "target_library" );
            asset.target_name = RemoteProviderJsonString( assetJson, "target_name" );
            asset.content_type = RemoteProviderJsonString( assetJson, "content_type" );
            asset.size_bytes = assetJson.at( "size_bytes" ).get<long long>();
            asset.sha256 = RemoteProviderJsonString( assetJson, "sha256" );
            asset.download_url = RemoteProviderJsonString( assetJson, "download_url" );
            asset.required = assetJson.at( "required" ).get<bool>();

            if( !ValidateRemoteUrlSecurity( asset.download_url, aAllowInsecureLocalhost, aError,
                                            "assets[].download_url" ) )
            {
                return std::nullopt;
            }

            manifest.assets.push_back( asset );
        }
    }
    catch( const std::exception& e )
    {
        aError =
                wxString::Format( _( "Unable to parse remote provider manifest: %s" ), wxString::FromUTF8( e.what() ) );
        return std::nullopt;
    }

    return manifest;
}
