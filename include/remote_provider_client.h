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

#ifndef REMOTE_PROVIDER_CLIENT_H
#define REMOTE_PROVIDER_CLIENT_H

#include <functional>

#include <kicommon.h>
#include <oauth/oauth_session.h>
#include <oauth/secure_token_store.h>
#include <remote_provider_metadata.h>
#include <remote_provider_models.h>
#include <wx/string.h>


enum class REMOTE_PROVIDER_HTTP_METHOD
{
    GET,
    POST
};


struct KICOMMON_API REMOTE_PROVIDER_HTTP_HEADER
{
    wxString name;
    wxString value;
};


struct KICOMMON_API REMOTE_PROVIDER_HTTP_REQUEST
{
    REMOTE_PROVIDER_HTTP_METHOD              method = REMOTE_PROVIDER_HTTP_METHOD::GET;
    wxString                                 url;
    wxString                                 body;
    std::vector<REMOTE_PROVIDER_HTTP_HEADER> headers;
};


struct KICOMMON_API REMOTE_PROVIDER_HTTP_RESPONSE
{
    int      status_code = 0;
    wxString body;
};


using REMOTE_PROVIDER_HTTP_HANDLER =
        std::function<bool( const REMOTE_PROVIDER_HTTP_REQUEST&, REMOTE_PROVIDER_HTTP_RESPONSE&, wxString& )>;


class KICOMMON_API REMOTE_PROVIDER_CLIENT
{
public:
    REMOTE_PROVIDER_CLIENT();
    explicit REMOTE_PROVIDER_CLIENT( REMOTE_PROVIDER_HTTP_HANDLER aHandler );

    bool DiscoverProvider( const wxString& aProviderUrl, REMOTE_PROVIDER_METADATA& aMetadata,
                           REMOTE_PROVIDER_ERROR& aError ) const;

    bool FetchOAuthServerMetadata( const REMOTE_PROVIDER_METADATA&        aProvider,
                                   REMOTE_PROVIDER_OAUTH_SERVER_METADATA& aMetadata,
                                   REMOTE_PROVIDER_ERROR&                 aError ) const;

    bool ExchangeAuthorizationCode( const REMOTE_PROVIDER_OAUTH_SERVER_METADATA& aMetadata,
                                    const OAUTH_SESSION&                         aSession,
                                    const wxString&                             aCode,
                                    OAUTH_TOKEN_SET&                            aTokens,
                                    REMOTE_PROVIDER_ERROR&                      aError ) const;

    bool RefreshAccessToken( const REMOTE_PROVIDER_OAUTH_SERVER_METADATA& aMetadata,
                             const wxString&                              aClientId,
                             const wxString&                              aRefreshToken,
                             OAUTH_TOKEN_SET&                             aTokens,
                             REMOTE_PROVIDER_ERROR&                       aError ) const;

    bool RevokeToken( const REMOTE_PROVIDER_OAUTH_SERVER_METADATA& aMetadata,
                      const wxString&                              aClientId,
                      const wxString&                              aToken,
                      REMOTE_PROVIDER_ERROR&                       aError ) const;

    REMOTE_PROVIDER_SIGNIN_STATE GetSignInState( const REMOTE_PROVIDER_METADATA& aProvider,
                                                 const wxString&                 aAccessToken ) const;

    bool FetchManifest( const REMOTE_PROVIDER_METADATA& aProvider, const wxString& aPartId,
                        const wxString& aAccessToken, REMOTE_PROVIDER_PART_MANIFEST& aManifest,
                        REMOTE_PROVIDER_ERROR& aError ) const;

    bool ExchangeBootstrapNonce( const REMOTE_PROVIDER_METADATA& aMetadata,
                                 const wxString& aAccessToken, wxString& aNonceUrl,
                                 REMOTE_PROVIDER_ERROR& aError ) const;

    static wxString MetadataDiscoveryUrl( const wxString& aProviderUrl );

private:
    bool SendRequest( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, REMOTE_PROVIDER_HTTP_RESPONSE& aResponse,
                      REMOTE_PROVIDER_ERROR& aError ) const;
    bool FetchJson( const REMOTE_PROVIDER_HTTP_REQUEST& aRequest, nlohmann::json& aJson,
                    REMOTE_PROVIDER_ERROR& aError ) const;
    bool ParseTokenResponse( const REMOTE_PROVIDER_HTTP_RESPONSE& aResponse, OAUTH_TOKEN_SET& aTokens,
                             REMOTE_PROVIDER_ERROR& aError ) const;

private:
    REMOTE_PROVIDER_HTTP_HANDLER m_handler;
};

#endif // REMOTE_PROVIDER_CLIENT_H
