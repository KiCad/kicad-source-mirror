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

#include <boost/test/unit_test.hpp>

#include <remote_provider_metadata.h>


namespace
{
wxFileName schemaPath()
{
    wxFileName schemaFile = wxFileName::DirName( wxString::FromUTF8( QA_SRC_ROOT ) );
    schemaFile.AppendDir( wxS( "resources" ) );
    schemaFile.AppendDir( wxS( "schemas" ) );
    schemaFile.SetFullName( wxS( "kicad-remote-provider-metadata-v1.schema.json" ) );
    return schemaFile;
}


nlohmann::json validMetadata()
{
    return nlohmann::json{
        { "provider_name", "Acme Parts" },
        { "provider_version", "1.0.0" },
        { "api_base_url", "https://provider.example.test/api" },
        { "panel_url", "https://provider.example.test/app" },
        { "session_bootstrap_url", "https://provider.example.test/session/bootstrap" },
        { "auth", {
            { "type", "oauth2" },
            { "metadata_url", "https://provider.example.test/.well-known/oauth-authorization-server" },
            { "client_id", "kicad-desktop" },
            { "scopes", nlohmann::json::array( { "openid", "profile", "parts.read" } ) }
        } },
        { "capabilities", {
            { "web_ui_v1", true },
            { "direct_downloads_v1", true },
            { "inline_payloads_v1", true }
        } },
        { "max_download_bytes", 10485760 },
        { "supported_asset_types", nlohmann::json::array( { "symbol", "footprint", "3dmodel" } ) },
        { "parts", { { "endpoint_template", "/v1/parts/{part_id}" } } },
        { "documentation_url", "https://provider.example.test/docs" },
        { "terms_url", "https://provider.example.test/terms" },
        { "privacy_url", "https://provider.example.test/privacy" }
    };
}
} // namespace


BOOST_AUTO_TEST_SUITE( RemoteProviderMetadataTests )

BOOST_AUTO_TEST_CASE( ValidMetadataParses )
{
    wxString error;
    std::optional<REMOTE_PROVIDER_METADATA> metadata =
            REMOTE_PROVIDER_METADATA::FromJson( validMetadata(), schemaPath(), error );

    BOOST_REQUIRE_MESSAGE( metadata.has_value(), error );
    BOOST_CHECK_EQUAL( metadata->provider_name, wxString( "Acme Parts" ) );
    BOOST_CHECK_EQUAL( metadata->panel_url, wxString( "https://provider.example.test/app" ) );
    BOOST_CHECK_EQUAL( metadata->session_bootstrap_url,
                       wxString( "https://provider.example.test/session/bootstrap" ) );
    BOOST_CHECK( metadata->web_ui_v1 );
    BOOST_CHECK( metadata->direct_downloads_v1 );
    BOOST_CHECK( metadata->inline_payloads_v1 );
    BOOST_CHECK_EQUAL( metadata->auth.metadata_url,
                       wxString( "https://provider.example.test/.well-known/oauth-authorization-server" ) );
}

BOOST_AUTO_TEST_CASE( DefaultSchemaPathParsesWhenSchemaIsStaged )
{
    wxFileName defaultPath = REMOTE_PROVIDER_METADATA::DefaultSchemaPath();

    if( !defaultPath.IsFileReadable() )
    {
        BOOST_TEST_MESSAGE( "Skipping: default schema path not readable (" + defaultPath.GetFullPath() + ")" );
        return;
    }

    wxString error;
    std::optional<REMOTE_PROVIDER_METADATA> metadata =
            REMOTE_PROVIDER_METADATA::FromJson( validMetadata(), error );

    BOOST_REQUIRE_MESSAGE( metadata.has_value(), error );
}

BOOST_AUTO_TEST_CASE( MissingAuthMetadataRejected )
{
    nlohmann::json metadata = validMetadata();
    metadata["auth"].erase( "metadata_url" );

    wxString error;
    std::optional<REMOTE_PROVIDER_METADATA> parsed =
            REMOTE_PROVIDER_METADATA::FromJson( metadata, schemaPath(), error );

    BOOST_CHECK( !parsed.has_value() );
    BOOST_CHECK( error.Contains( wxString( "schema validation" ) ) );
    BOOST_CHECK( error.Contains( wxString( "metadata_url" ) ) );
}

BOOST_AUTO_TEST_CASE( InsecureProviderUrlRejectedUnlessLoopbackExplicitlyAllowed )
{
    nlohmann::json metadata = validMetadata();
    metadata["api_base_url"] = "http://provider.example.test/api";

    wxString error;
    std::optional<REMOTE_PROVIDER_METADATA> parsed =
            REMOTE_PROVIDER_METADATA::FromJson( metadata, schemaPath(), error );

    BOOST_CHECK( !parsed.has_value() );
    BOOST_CHECK( error.Contains( wxString( "api_base_url" ) ) );

    metadata["allow_insecure_localhost"] = true;
    metadata["api_base_url"] = "http://127.0.0.1:8080/api";
    metadata["auth"]["metadata_url"] = "http://127.0.0.1:8080/.well-known/oauth-authorization-server";

    parsed = REMOTE_PROVIDER_METADATA::FromJson( metadata, schemaPath(), error );
    BOOST_REQUIRE_MESSAGE( parsed.has_value(), error );
}

BOOST_AUTO_TEST_CASE( UnsupportedCapabilityRejectedCleanly )
{
    nlohmann::json metadata = validMetadata();
    metadata["capabilities"]["future_magic_v9"] = true;

    wxString error;
    std::optional<REMOTE_PROVIDER_METADATA> parsed =
            REMOTE_PROVIDER_METADATA::FromJson( metadata, schemaPath(), error );

    BOOST_CHECK( !parsed.has_value() );
    BOOST_CHECK( error.Contains( wxString( "Unsupported provider capability" ) )
                 || error.Contains( wxString( "schema validation" ) ) );
}

BOOST_AUTO_TEST_CASE( MissingPanelUrlRejected )
{
    nlohmann::json metadata = validMetadata();
    metadata.erase( "panel_url" );

    wxString error;
    std::optional<REMOTE_PROVIDER_METADATA> parsed =
            REMOTE_PROVIDER_METADATA::FromJson( metadata, schemaPath(), error );

    BOOST_CHECK( !parsed.has_value() );
    BOOST_CHECK( error.Contains( wxString( "panel_url" ) ) );
}

BOOST_AUTO_TEST_CASE( OAuthProviderRequiresSessionBootstrapUrl )
{
    nlohmann::json metadata = validMetadata();
    metadata.erase( "session_bootstrap_url" );

    wxString error;
    std::optional<REMOTE_PROVIDER_METADATA> parsed =
            REMOTE_PROVIDER_METADATA::FromJson( metadata, schemaPath(), error );

    BOOST_CHECK( !parsed.has_value() );
    BOOST_CHECK( error.Contains( wxString( "session_bootstrap_url" ) ) );
}

BOOST_AUTO_TEST_CASE( SessionBootstrapUrlMustShareOriginWithPanelUrl )
{
    nlohmann::json metadata = validMetadata();
    metadata["session_bootstrap_url"] = "https://tokens.example.test/session/bootstrap";

    wxString error;
    std::optional<REMOTE_PROVIDER_METADATA> parsed =
            REMOTE_PROVIDER_METADATA::FromJson( metadata, schemaPath(), error );

    BOOST_CHECK( !parsed.has_value() );
    BOOST_CHECK( error.Contains( wxString( "session_bootstrap_url" ) ) );
}

BOOST_AUTO_TEST_SUITE_END()
