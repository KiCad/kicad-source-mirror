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

#include <remote_provider_client.h>

#include <curl/curl.h>
#include <kicad_curl/kicad_curl_easy.h>
#include <oauth/oauth_session.h>
#include <remote_provider_utils.h>
#include <wx/datetime.h>
#include <wx/intl.h>


namespace
{
wxString joinUrl( const wxString& aBaseUrl, const wxString& aPath )
{
    if( aPath.StartsWith( wxS( "https://" ) ) || aPath.StartsWith( wxS( "http://" ) ) )
        return aPath;

    wxString base = aBaseUrl;

    while( base.EndsWith( wxS( "/" ) ) )
        base.RemoveLast();

    if( aPath.StartsWith( wxS( "/" ) ) )
        return base + aPath;

    return base + wxS( "/" ) + aPath;
}


wxString buildPartsUrl( const REMOTE_PROVIDER_METADATA& aProvider, const wxString& aPartId )
{
    wxString endpoint = aProvider.parts_endpoint_template;
    endpoint.Replace( wxS( "{part_id}" ), UrlEncode( aPartId ) );
    return joinUrl( aProvider.api_base_url, endpoint );
}


bool defaultHttpHandler( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                         wxString& aError )
{
    KICAD_CURL_EASY curl;
    curl.SetUserAgent( "KiCad-RemoteProvider/1.0" );
    curl.SetFollowRedirects( true );
    curl.SetConnectTimeout( 10 );

    if( !curl.SetURL( aRequest.url.ToStdString() ) )
    {
        aError = wxString::Format( wxS( "Unable to set URL '%s'." ), aRequest.url );
        return false;
    }

    for( const REMOTE_PROVIDER_HTTP_HEADER& header : aRequest.headers )
        curl.SetHeader( header.name.ToStdString(), header.value.ToStdString() );

    if( aRequest.method == REMOTE_PROVIDER_HTTP_METHOD::POST )
        curl.SetPostFields( aRequest.body.ToStdString() );

    int result = curl.Perform();

    if( result != CURLE_OK )
    {
        aError = wxString::FromUTF8( curl.GetErrorText( result ).c_str() );
        return false;
    }

    aResponse.status_code = curl.GetResponseStatusCode();
    aResponse.body = wxString::FromUTF8( curl.GetBuffer().c_str() );
    return true;
}
} // namespace


REMOTE_PROVIDER_CLIENT::REMOTE_PROVIDER_CLIENT() :
        m_handler( defaultHttpHandler )
{
}


REMOTE_PROVIDER_CLIENT::REMOTE_PROVIDER_CLIENT( REMOTE_PROVIDER_HTTP_HANDLER aHandler ) :
        m_handler( std::move( aHandler ) )
{
}


wxString REMOTE_PROVIDER_CLIENT::MetadataDiscoveryUrl( const wxString& aProviderUrl )
{
    static const wxString suffix = wxS( "/.well-known/kicad-remote-provider" );

    if( aProviderUrl.EndsWith( suffix ) )
        return aProviderUrl;

    wxString base = aProviderUrl;

    while( base.EndsWith( wxS( "/" ) ) )
        base.RemoveLast();

    return base + suffix;
}


bool REMOTE_PROVIDER_CLIENT::DiscoverProvider( const wxString& aProviderUrl, REMOTE_PROVIDER_METADATA& aMetadata,
                                               REMOTE_PROVIDER_ERROR& aError ) const
{
    aError.Clear();

    REMOTE_PROVIDER_HTTP_REQUEST request;
    request.method = REMOTE_PROVIDER_HTTP_METHOD::GET;
    request.url = MetadataDiscoveryUrl( aProviderUrl );
    request.headers.push_back( { wxS( "Accept" ), wxS( "application/json" ) } );

    nlohmann::json responseJson;

    if( !FetchJson( request, responseJson, aError ) )
        return false;

    wxString                                error;
    std::optional<REMOTE_PROVIDER_METADATA> metadata = REMOTE_PROVIDER_METADATA::FromJson( responseJson, error );

    if( !metadata )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message = error;
        return false;
    }

    aMetadata = *metadata;
    return true;
}


bool REMOTE_PROVIDER_CLIENT::FetchOAuthServerMetadata( const REMOTE_PROVIDER_METADATA&        aProvider,
                                                       REMOTE_PROVIDER_OAUTH_SERVER_METADATA& aMetadata,
                                                       REMOTE_PROVIDER_ERROR&                 aError ) const
{
    aError.Clear();

    if( aProvider.auth.type != REMOTE_PROVIDER_AUTH_TYPE::OAUTH2 )
    {
        aMetadata = REMOTE_PROVIDER_OAUTH_SERVER_METADATA();
        return true;
    }

    REMOTE_PROVIDER_HTTP_REQUEST request;
    request.method = REMOTE_PROVIDER_HTTP_METHOD::GET;
    request.url = aProvider.auth.metadata_url;
    request.headers.push_back( { wxS( "Accept" ), wxS( "application/json" ) } );

    nlohmann::json responseJson;

    if( !FetchJson( request, responseJson, aError ) )
        return false;

    wxString                                             error;
    std::optional<REMOTE_PROVIDER_OAUTH_SERVER_METADATA> metadata =
            REMOTE_PROVIDER_OAUTH_SERVER_METADATA::FromJson( responseJson, aProvider.allow_insecure_localhost, error );

    if( !metadata )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message = error;
        return false;
    }

    aMetadata = *metadata;
    return true;
}


bool REMOTE_PROVIDER_CLIENT::ExchangeAuthorizationCode(
        const REMOTE_PROVIDER_OAUTH_SERVER_METADATA& aMetadata, const OAUTH_SESSION& aSession,
        const wxString& aCode, OAUTH_TOKEN_SET& aTokens, REMOTE_PROVIDER_ERROR& aError ) const
{
    aError.Clear();

    REMOTE_PROVIDER_HTTP_REQUEST request;
    request.method = REMOTE_PROVIDER_HTTP_METHOD::POST;
    request.url = aMetadata.token_endpoint;
    request.body = wxString( wxS( "grant_type=authorization_code" ) )
                   + wxS( "&code=" ) + UrlEncode( aCode )
                   + wxS( "&client_id=" ) + UrlEncode( aSession.client_id )
                   + wxS( "&redirect_uri=" ) + UrlEncode( aSession.redirect_uri )
                   + wxS( "&code_verifier=" ) + UrlEncode( aSession.code_verifier );
    request.headers.push_back( { wxS( "Accept" ), wxS( "application/json" ) } );
    request.headers.push_back(
            { wxS( "Content-Type" ), wxS( "application/x-www-form-urlencoded" ) } );
    request.headers.push_back( { wxS( "Cache-Control" ), wxS( "no-store" ) } );

    REMOTE_PROVIDER_HTTP_RESPONSE response;

    if( !SendRequest( request, response, aError ) )
        return false;

    return ParseTokenResponse( response, aTokens, aError );
}


bool REMOTE_PROVIDER_CLIENT::RefreshAccessToken(
        const REMOTE_PROVIDER_OAUTH_SERVER_METADATA& aMetadata, const wxString& aClientId,
        const wxString& aRefreshToken, OAUTH_TOKEN_SET& aTokens, REMOTE_PROVIDER_ERROR& aError ) const
{
    aError.Clear();

    REMOTE_PROVIDER_HTTP_REQUEST request;
    request.method = REMOTE_PROVIDER_HTTP_METHOD::POST;
    request.url = aMetadata.token_endpoint;
    request.body = wxString( wxS( "grant_type=refresh_token" ) )
                   + wxS( "&refresh_token=" ) + UrlEncode( aRefreshToken )
                   + wxS( "&client_id=" ) + UrlEncode( aClientId );
    request.headers.push_back( { wxS( "Accept" ), wxS( "application/json" ) } );
    request.headers.push_back(
            { wxS( "Content-Type" ), wxS( "application/x-www-form-urlencoded" ) } );
    request.headers.push_back( { wxS( "Cache-Control" ), wxS( "no-store" ) } );

    REMOTE_PROVIDER_HTTP_RESPONSE response;

    if( !SendRequest( request, response, aError ) )
        return false;

    return ParseTokenResponse( response, aTokens, aError );
}


bool REMOTE_PROVIDER_CLIENT::RevokeToken( const REMOTE_PROVIDER_OAUTH_SERVER_METADATA& aMetadata,
                                          const wxString& aClientId, const wxString& aToken,
                                          REMOTE_PROVIDER_ERROR& aError ) const
{
    aError.Clear();

    if( aMetadata.revocation_endpoint.IsEmpty() )
        return true;

    REMOTE_PROVIDER_HTTP_REQUEST request;
    request.method = REMOTE_PROVIDER_HTTP_METHOD::POST;
    request.url = aMetadata.revocation_endpoint;
    request.body = wxString( wxS( "token=" ) ) + UrlEncode( aToken )
                   + wxS( "&client_id=" ) + UrlEncode( aClientId );
    request.headers.push_back( { wxS( "Accept" ), wxS( "application/json" ) } );
    request.headers.push_back(
            { wxS( "Content-Type" ), wxS( "application/x-www-form-urlencoded" ) } );

    REMOTE_PROVIDER_HTTP_RESPONSE response;
    return SendRequest( request, response, aError );
}


REMOTE_PROVIDER_SIGNIN_STATE REMOTE_PROVIDER_CLIENT::GetSignInState( const REMOTE_PROVIDER_METADATA& aProvider,
                                                                     const wxString& aAccessToken ) const
{
    if( aProvider.auth.type != REMOTE_PROVIDER_AUTH_TYPE::OAUTH2 )
        return REMOTE_PROVIDER_SIGNIN_STATE::NOT_REQUIRED;

    if( aAccessToken.IsEmpty() )
        return REMOTE_PROVIDER_SIGNIN_STATE::REQUIRED;

    return REMOTE_PROVIDER_SIGNIN_STATE::AVAILABLE;
}


bool REMOTE_PROVIDER_CLIENT::FetchManifest( const REMOTE_PROVIDER_METADATA& aProvider, const wxString& aPartId,
                                            const wxString& aAccessToken, REMOTE_PROVIDER_PART_MANIFEST& aManifest,
                                            REMOTE_PROVIDER_ERROR& aError ) const
{
    aError.Clear();

    if( GetSignInState( aProvider, aAccessToken ) == REMOTE_PROVIDER_SIGNIN_STATE::REQUIRED )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::AUTH_REQUIRED;
        aError.message = _( "Sign in required for this provider." );
        return false;
    }

    REMOTE_PROVIDER_HTTP_REQUEST request;
    request.method = REMOTE_PROVIDER_HTTP_METHOD::GET;
    request.url = buildPartsUrl( aProvider, aPartId );
    request.headers.push_back( { wxS( "Accept" ), wxS( "application/json" ) } );

    if( !aAccessToken.IsEmpty() )
        request.headers.push_back( { wxS( "Authorization" ), wxS( "Bearer " ) + aAccessToken } );

    nlohmann::json responseJson;

    if( !FetchJson( request, responseJson, aError ) )
        return false;

    wxString                                     error;
    std::optional<REMOTE_PROVIDER_PART_MANIFEST> manifest =
            REMOTE_PROVIDER_PART_MANIFEST::FromJson( responseJson, aProvider.allow_insecure_localhost, error );

    if( !manifest )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message = error;
        return false;
    }

    aManifest = *manifest;
    return true;
}


bool REMOTE_PROVIDER_CLIENT::ExchangeBootstrapNonce( const REMOTE_PROVIDER_METADATA& aMetadata,
                                                      const wxString& aAccessToken, wxString& aNonceUrl,
                                                      REMOTE_PROVIDER_ERROR& aError ) const
{
    aError.Clear();

    if( aMetadata.session_bootstrap_url.IsEmpty() )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message = _( "Provider metadata does not include a session bootstrap URL." );
        return false;
    }

    nlohmann::json body;
    body["access_token"] = aAccessToken.ToStdString();
    body["next_url"] = aMetadata.panel_url.ToStdString();

    REMOTE_PROVIDER_HTTP_REQUEST request;
    request.method = REMOTE_PROVIDER_HTTP_METHOD::POST;
    request.url = aMetadata.session_bootstrap_url;
    request.body = wxString::FromUTF8( body.dump().c_str() );
    request.headers.push_back( { wxS( "Accept" ), wxS( "application/json" ) } );
    request.headers.push_back( { wxS( "Content-Type" ), wxS( "application/json" ) } );

    nlohmann::json responseJson;

    if( !FetchJson( request, responseJson, aError ) )
        return false;

    if( !responseJson.contains( "nonce_url" ) || !responseJson["nonce_url"].is_string() )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message = _( "Bootstrap response did not include a nonce URL." );
        return false;
    }

    aNonceUrl = wxString::FromUTF8( responseJson["nonce_url"].get_ref<const std::string&>().c_str() );

    wxString securityError;

    if( !ValidateRemoteUrlSecurity( aNonceUrl, aMetadata.allow_insecure_localhost, securityError,
                                    wxS( "Bootstrap nonce URL" ) ) )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message = securityError;
        return false;
    }

    if( NormalizedUrlOrigin( aNonceUrl ) != NormalizedUrlOrigin( aMetadata.session_bootstrap_url ) )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message = _( "Bootstrap nonce URL origin does not match the bootstrap endpoint." );
        return false;
    }

    return true;
}


bool REMOTE_PROVIDER_CLIENT::SendRequest( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest,
                                          REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                                          REMOTE_PROVIDER_ERROR& aError ) const
{
    wxString transportError;

    if( !m_handler( aRequest, aResponse, transportError ) )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::NETWORK;
        aError.message = transportError;
        return false;
    }

    aError.http_status = aResponse.status_code;

    if( aResponse.status_code < 200 || aResponse.status_code >= 300 )
    {
        if( aResponse.status_code == 401 )
            aError.type = REMOTE_PROVIDER_ERROR_TYPE::AUTH_REQUIRED;
        else if( aResponse.status_code == 403 )
            aError.type = REMOTE_PROVIDER_ERROR_TYPE::ACCESS_DENIED;
        else if( aResponse.status_code == 404 )
            aError.type = REMOTE_PROVIDER_ERROR_TYPE::NOT_FOUND;
        else if( aResponse.status_code >= 500 )
            aError.type = REMOTE_PROVIDER_ERROR_TYPE::SERVER;
        else
            aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;

        aError.message = wxString::Format( _( "Remote provider request failed with HTTP %d." ),
                                           aResponse.status_code );
        return false;
    }

    return true;
}


bool REMOTE_PROVIDER_CLIENT::FetchJson( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, nlohmann::json& aJson,
                                        REMOTE_PROVIDER_ERROR& aError ) const
{
    REMOTE_PROVIDER_HTTP_RESPONSE response;

    if( !SendRequest( aRequest, response, aError ) )
        return false;

    try
    {
        aJson = nlohmann::json::parse( response.body.ToStdString() );
    }
    catch( const std::exception& e )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message =
                wxString::Format( _( "Remote provider returned invalid JSON: %s" ), wxString::FromUTF8( e.what() ) );
        return false;
    }

    return true;
}


bool REMOTE_PROVIDER_CLIENT::ParseTokenResponse( const REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                                                 OAUTH_TOKEN_SET& aTokens,
                                                 REMOTE_PROVIDER_ERROR& aError ) const
{
    nlohmann::json json;

    try
    {
        json = nlohmann::json::parse( aResponse.body.ToStdString() );
    }
    catch( const std::exception& e )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message = wxString::Format( _( "Remote provider returned invalid token JSON: %s" ),
                                           wxString::FromUTF8( e.what() ) );
        return false;
    }

    try
    {
        aTokens = OAUTH_TOKEN_SET();
        aTokens.access_token =
                wxString::FromUTF8( json.at( "access_token" ).get_ref<const std::string&>().c_str() );
        aTokens.refresh_token = json.contains( "refresh_token" )
                                        ? wxString::FromUTF8(
                                                  json.at( "refresh_token" ).get_ref<const std::string&>().c_str() )
                                        : wxString();
        aTokens.id_token = json.contains( "id_token" )
                                   ? wxString::FromUTF8(
                                             json.at( "id_token" ).get_ref<const std::string&>().c_str() )
                                   : wxString();
        aTokens.token_type = json.contains( "token_type" )
                                     ? wxString::FromUTF8(
                                               json.at( "token_type" ).get_ref<const std::string&>().c_str() )
                                     : wxString( "Bearer" );
        aTokens.scope = json.contains( "scope" )
                                ? wxString::FromUTF8( json.at( "scope" ).get_ref<const std::string&>().c_str() )
                                : wxString();

        const long long expiresIn = json.contains( "expires_in" ) ? json.at( "expires_in" ).get<long long>() : 0;
        aTokens.expires_at = static_cast<long long>( wxDateTime::Now().GetTicks() ) + expiresIn;
    }
    catch( const std::exception& e )
    {
        aError.type = REMOTE_PROVIDER_ERROR_TYPE::INVALID_RESPONSE;
        aError.message = wxString::Format( _( "Remote provider token response was incomplete: %s" ),
                                           wxString::FromUTF8( e.what() ) );
        return false;
    }

    return true;
}
