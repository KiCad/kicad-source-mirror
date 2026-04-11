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

#include <oauth/secure_token_store.h>

#include <json_conversions.h>
#include <kiplatform/secrets.h>

#include <nlohmann/json.hpp>


void to_json( nlohmann::json& aJson, const OAUTH_TOKEN_SET& aTokens )
{
    aJson = nlohmann::json{
        { "access_token", aTokens.access_token },
        { "refresh_token", aTokens.refresh_token },
        { "id_token", aTokens.id_token },
        { "token_type", aTokens.token_type },
        { "scope", aTokens.scope },
        { "expires_at", aTokens.expires_at }
    };
}


void from_json( const nlohmann::json& aJson, OAUTH_TOKEN_SET& aTokens )
{
    aTokens = OAUTH_TOKEN_SET();
    aJson.at( "access_token" ).get_to( aTokens.access_token );
    aJson.at( "refresh_token" ).get_to( aTokens.refresh_token );
    aJson.at( "id_token" ).get_to( aTokens.id_token );
    aJson.at( "token_type" ).get_to( aTokens.token_type );
    aJson.at( "scope" ).get_to( aTokens.scope );
    aJson.at( "expires_at" ).get_to( aTokens.expires_at );
}


bool PLATFORM_SECRET_BACKEND::StoreSecret( const wxString& aService, const wxString& aKey,
                                           const wxString& aSecret )
{
    return KIPLATFORM::SECRETS::StoreSecret( aService, aKey, aSecret );
}


bool PLATFORM_SECRET_BACKEND::GetSecret( const wxString& aService, const wxString& aKey,
                                         wxString& aSecret ) const
{
    return KIPLATFORM::SECRETS::GetSecret( aService, aKey, aSecret );
}


bool PLATFORM_SECRET_BACKEND::DeleteSecret( const wxString& aService, const wxString& aKey )
{
    return KIPLATFORM::SECRETS::DeleteSecret( aService, aKey );
}


SECURE_TOKEN_STORE::SECURE_TOKEN_STORE( std::unique_ptr<OAUTH_SECRET_BACKEND> aBackend ) :
        m_backend( std::move( aBackend ) )
{
}


bool SECURE_TOKEN_STORE::StoreTokens( const wxString& aProviderId, const wxString& aAccountId,
                                      const OAUTH_TOKEN_SET& aTokens )
{
    const wxString secret = wxString::FromUTF8( nlohmann::json( aTokens ).dump().c_str() );
    return m_backend->StoreSecret( MakeServiceName( aProviderId ), aAccountId, secret );
}


std::optional<OAUTH_TOKEN_SET> SECURE_TOKEN_STORE::LoadTokens( const wxString& aProviderId,
                                                               const wxString& aAccountId ) const
{
    wxString secret;

    if( !m_backend->GetSecret( MakeServiceName( aProviderId ), aAccountId, secret )
        || secret.IsEmpty() )
    {
        return std::nullopt;
    }

    try
    {
        return nlohmann::json::parse( secret.ToStdString() ).get<OAUTH_TOKEN_SET>();
    }
    catch( ... )
    {
        return std::nullopt;
    }
}


bool SECURE_TOKEN_STORE::DeleteTokens( const wxString& aProviderId, const wxString& aAccountId )
{
    return m_backend->DeleteSecret( MakeServiceName( aProviderId ), aAccountId );
}


wxString SECURE_TOKEN_STORE::MakeServiceName( const wxString& aProviderId )
{
    return wxS( "org.kicad.remote_provider." ) + aProviderId;
}
