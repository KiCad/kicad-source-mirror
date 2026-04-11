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

#include <stdexcept>

#include <oauth/oauth_session.h>
#include <oauth/secure_token_store.h>
#include <remote_provider_client.h>
#include <wx/utils.h>


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


nlohmann::json providerMetadataJson( const wxString& aAuthType = wxS( "oauth2" ) )
{
    nlohmann::json auth = { { "type", aAuthType.ToStdString() } };

    if( aAuthType == wxS( "oauth2" ) )
    {
        auth["metadata_url"] = "https://provider.example.test/.well-known/oauth-authorization-server";
        auth["client_id"] = "kicad-desktop";
        auth["scopes"] = nlohmann::json::array( { "openid", "parts.read" } );
    }

    return nlohmann::json{
        { "provider_name", "Acme Parts" },
        { "provider_version", "1.0.0" },
        { "api_base_url", "https://provider.example.test/api" },
        { "panel_url", "https://provider.example.test/app" },
        { "session_bootstrap_url", "https://provider.example.test/session/bootstrap" },
        { "auth", auth },
        { "capabilities",
          { { "web_ui_v1", true },
            { "parts_v1", true },
            { "direct_downloads_v1", true },
            { "inline_payloads_v1", true } } },
        { "max_download_bytes", 10485760 },
        { "supported_asset_types", nlohmann::json::array( { "symbol", "footprint", "3dmodel" } ) },
        { "parts", { { "endpoint_template", "/v1/parts/{part_id}" } } }
    };
}


REMOTE_PROVIDER_METADATA parseProviderMetadata( const wxString& aAuthType = wxS( "oauth2" ) )
{
    wxString                                error;
    std::optional<REMOTE_PROVIDER_METADATA> metadata =
            REMOTE_PROVIDER_METADATA::FromJson( providerMetadataJson( aAuthType ), schemaPath(), error );

    if( !metadata.has_value() )
        throw std::runtime_error( error.ToStdString() );

    return *metadata;
}


nlohmann::json authServerMetadataJson()
{
    return nlohmann::json{ { "issuer", "https://provider.example.test" },
                           { "authorization_endpoint", "https://provider.example.test/oauth/authorize" },
                           { "token_endpoint", "https://provider.example.test/oauth/token" },
                           { "revocation_endpoint", "https://provider.example.test/oauth/revoke" } };
}

nlohmann::json manifestJson()
{
    return nlohmann::json{
        { "part_id", "acme-res-10k" },
        { "display_name", "10k Resistor" },
        { "summary", "10k 0603 thick film resistor" },
        { "license", "CC-BY-4.0" },
        { "assets", nlohmann::json::array(
                            { { { "asset_type", "symbol" },
                                { "name", "acme-res-10k.kicad_sym" },
                                { "content_type", "application/x-kicad-symbol" },
                                { "size_bytes", 2048 },
                                { "sha256", "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" },
                                { "download_url", "https://provider.example.test/downloads/acme-res-10k.kicad_sym" },
                                { "required", true } },
                              { { "asset_type", "footprint" },
                                { "name", "R_0603.pretty" },
                                { "content_type", "application/x-kicad-footprint" },
                                { "size_bytes", 4096 },
                                { "sha256", "abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789" },
                                { "download_url", "https://provider.example.test/downloads/R_0603.pretty" },
                                { "required", false } } } ) }
    };
}


wxString dumpJson( const nlohmann::json& aJson )
{
    return wxString::FromUTF8( aJson.dump().c_str() );
}
struct BUILD_DIR_FIXTURE
{
    BUILD_DIR_FIXTURE()
    {
        m_wasSet = wxGetEnv( wxS( "KICAD_RUN_FROM_BUILD_DIR" ), &m_oldValue );

        if( !m_wasSet )
            wxSetEnv( wxS( "KICAD_RUN_FROM_BUILD_DIR" ), wxS( "1" ) );
    }

    ~BUILD_DIR_FIXTURE()
    {
        if( !m_wasSet )
            wxUnsetEnv( wxS( "KICAD_RUN_FROM_BUILD_DIR" ) );
    }

    bool     m_wasSet;
    wxString m_oldValue;
};

} // namespace


BOOST_FIXTURE_TEST_SUITE( RemoteProviderClientTests, BUILD_DIR_FIXTURE )

BOOST_AUTO_TEST_CASE( DiscoveryFetchesWellKnownMetadata )
{
    std::vector<wxString> requestedUrls;

    REMOTE_PROVIDER_CLIENT client(
            [&]( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                 wxString& aError )
            {
                wxUnusedVar( aError );
                requestedUrls.push_back( aRequest.url );
                aResponse.status_code = 200;
                aResponse.body = dumpJson( providerMetadataJson() );
                return true;
            } );

    REMOTE_PROVIDER_METADATA metadata;
    REMOTE_PROVIDER_ERROR    error;

    BOOST_REQUIRE( client.DiscoverProvider( wxString( "https://provider.example.test" ), metadata, error ) );
    BOOST_CHECK_EQUAL( requestedUrls.size(), 1U );
    BOOST_CHECK_EQUAL( requestedUrls.front(),
                       wxString( "https://provider.example.test/.well-known/kicad-remote-provider" ) );
    BOOST_CHECK_EQUAL( metadata.provider_name, wxString( "Acme Parts" ) );
}

BOOST_AUTO_TEST_CASE( OAuthServerMetadataFetchParsesRfc8414Fields )
{
    REMOTE_PROVIDER_METADATA metadata = parseProviderMetadata();

    REMOTE_PROVIDER_CLIENT client(
            [&]( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                 wxString& aError )
            {
                wxUnusedVar( aError );
                BOOST_CHECK_EQUAL( aRequest.url,
                                   wxString( "https://provider.example.test/.well-known/oauth-authorization-server" ) );
                aResponse.status_code = 200;
                aResponse.body = dumpJson( authServerMetadataJson() );
                return true;
            } );

    REMOTE_PROVIDER_OAUTH_SERVER_METADATA authMetadata;
    REMOTE_PROVIDER_ERROR                 error;

    BOOST_REQUIRE( client.FetchOAuthServerMetadata( metadata, authMetadata, error ) );
    BOOST_CHECK_EQUAL( authMetadata.authorization_endpoint,
                       wxString( "https://provider.example.test/oauth/authorize" ) );
    BOOST_CHECK_EQUAL( authMetadata.token_endpoint, wxString( "https://provider.example.test/oauth/token" ) );
}

BOOST_AUTO_TEST_CASE( ManifestParsesDigestSizeAndDownloadUrls )
{
    REMOTE_PROVIDER_METADATA metadata = parseProviderMetadata( wxS( "none" ) );

    REMOTE_PROVIDER_CLIENT client(
            [&]( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                 wxString& aError )
            {
                wxUnusedVar( aError );
                BOOST_CHECK( aRequest.method == REMOTE_PROVIDER_HTTP_METHOD::GET );
                BOOST_CHECK_EQUAL( aRequest.url,
                                   wxString( "https://provider.example.test/api/v1/parts/acme-res-10k" ) );
                aResponse.status_code = 200;
                aResponse.body = dumpJson( manifestJson() );
                return true;
            } );

    REMOTE_PROVIDER_PART_MANIFEST manifest;
    REMOTE_PROVIDER_ERROR         error;

    BOOST_REQUIRE( client.FetchManifest( metadata, wxString( "acme-res-10k" ), wxString(), manifest, error ) );
    BOOST_CHECK_EQUAL( manifest.part_id, wxString( "acme-res-10k" ) );
    BOOST_REQUIRE_EQUAL( manifest.assets.size(), 2U );
    BOOST_CHECK_EQUAL( manifest.assets.front().size_bytes, 2048LL );
    BOOST_CHECK_EQUAL( manifest.assets.front().sha256,
                       wxString( "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef" ) );
    BOOST_CHECK( manifest.assets.front().required );
}

BOOST_AUTO_TEST_CASE( AuthorizationCodeExchangeParsesTokens )
{
    REMOTE_PROVIDER_METADATA metadata = parseProviderMetadata();
    REMOTE_PROVIDER_OAUTH_SERVER_METADATA oauth;
    oauth.authorization_endpoint = wxString( "https://provider.example.test/oauth/authorize" );
    oauth.token_endpoint = wxString( "https://provider.example.test/oauth/token" );

    REMOTE_PROVIDER_CLIENT client(
            [&]( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                 wxString& aError )
            {
                wxUnusedVar( aError );
                BOOST_CHECK( aRequest.method == REMOTE_PROVIDER_HTTP_METHOD::POST );
                BOOST_CHECK_EQUAL( aRequest.url, oauth.token_endpoint );
                BOOST_CHECK( aRequest.body.Contains( wxString( "grant_type=authorization_code" ) ) );
                BOOST_CHECK( aRequest.body.Contains( wxString( "code=test-code" ) ) );
                aResponse.status_code = 200;
                aResponse.body = dumpJson( {
                    { "access_token", "access-123" },
                    { "refresh_token", "refresh-123" },
                    { "token_type", "Bearer" },
                    { "scope", "openid parts.read" },
                    { "expires_in", 3600 }
                } );
                return true;
            } );

    OAUTH_SESSION session;
    session.client_id = metadata.auth.client_id;
    session.redirect_uri = wxString( "http://127.0.0.1:9000/oauth/callback" );
    session.code_verifier = wxString( "verifier" );
    OAUTH_TOKEN_SET tokens;
    REMOTE_PROVIDER_ERROR error;

    BOOST_REQUIRE( client.ExchangeAuthorizationCode( oauth, session, wxString( "test-code" ), tokens, error ) );
    BOOST_CHECK_EQUAL( tokens.access_token, wxString( "access-123" ) );
    BOOST_CHECK_EQUAL( tokens.refresh_token, wxString( "refresh-123" ) );
    BOOST_CHECK_EQUAL( tokens.token_type, wxString( "Bearer" ) );
}

BOOST_AUTO_TEST_CASE( RefreshTokenExchangeParsesTokens )
{
    REMOTE_PROVIDER_OAUTH_SERVER_METADATA oauth;
    oauth.token_endpoint = wxString( "https://provider.example.test/oauth/token" );

    REMOTE_PROVIDER_CLIENT client(
            [&]( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                 wxString& aError )
            {
                wxUnusedVar( aError );
                BOOST_CHECK( aRequest.body.Contains( wxString( "grant_type=refresh_token" ) ) );
                BOOST_CHECK( aRequest.body.Contains( wxString( "refresh_token=refresh-123" ) ) );
                aResponse.status_code = 200;
                aResponse.body = dumpJson( {
                    { "access_token", "access-456" },
                    { "refresh_token", "refresh-456" },
                    { "token_type", "Bearer" },
                    { "scope", "openid parts.read" },
                    { "expires_in", 3600 }
                } );
                return true;
            } );

    OAUTH_TOKEN_SET tokens;
    REMOTE_PROVIDER_ERROR error;
    BOOST_REQUIRE( client.RefreshAccessToken( oauth, wxString( "kicad-desktop" ),
                                              wxString( "refresh-123" ), tokens, error ) );
    BOOST_CHECK_EQUAL( tokens.access_token, wxString( "access-456" ) );
    BOOST_CHECK_EQUAL( tokens.refresh_token, wxString( "refresh-456" ) );
}

BOOST_AUTO_TEST_CASE( RevokeTokenPostsToRevocationEndpoint )
{
    REMOTE_PROVIDER_OAUTH_SERVER_METADATA oauth;
    oauth.revocation_endpoint = wxString( "https://provider.example.test/oauth/revoke" );

    REMOTE_PROVIDER_CLIENT client(
            [&]( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                 wxString& aError )
            {
                wxUnusedVar( aError );
                BOOST_CHECK( aRequest.body.Contains( wxString( "token=access-123" ) ) );
                aResponse.status_code = 200;
                aResponse.body = wxString( "{}" );
                return true;
            } );

    REMOTE_PROVIDER_ERROR error;
    BOOST_CHECK( client.RevokeToken( oauth, wxString( "kicad-desktop" ), wxString( "access-123" ), error ) );
}

BOOST_AUTO_TEST_SUITE_END()
