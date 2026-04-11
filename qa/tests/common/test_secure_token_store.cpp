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

#include <map>

#include <json_common.h>

#include <oauth/secure_token_store.h>
#include <remote_provider_settings.h>


namespace
{
class MEMORY_SECRET_BACKEND : public OAUTH_SECRET_BACKEND
{
public:
    bool StoreSecret( const wxString& aService, const wxString& aKey, const wxString& aSecret ) override
    {
        m_entries[aService + wxS( "::" ) + aKey] = aSecret;
        return true;
    }

    bool GetSecret( const wxString& aService, const wxString& aKey, wxString& aSecret ) const override
    {
        auto it = m_entries.find( aService + wxS( "::" ) + aKey );

        if( it == m_entries.end() )
            return false;

        aSecret = it->second;
        return !aSecret.IsEmpty();
    }

    bool DeleteSecret( const wxString& aService, const wxString& aKey ) override
    {
        m_entries.erase( aService + wxS( "::" ) + aKey );
        return true;
    }

private:
    std::map<wxString, wxString> m_entries;
};
} // namespace


BOOST_AUTO_TEST_SUITE( SecureTokenStoreTests )

BOOST_AUTO_TEST_CASE( StoresLoadsAndDeletesTokens )
{
    auto backend = std::make_unique<MEMORY_SECRET_BACKEND>();
    SECURE_TOKEN_STORE store( std::move( backend ) );

    OAUTH_TOKEN_SET tokens;
    tokens.access_token = wxS( "access-token" );
    tokens.refresh_token = wxS( "refresh-token" );
    tokens.id_token = wxS( "id-token" );
    tokens.token_type = wxS( "Bearer" );
    tokens.scope = wxS( "openid profile parts.read" );
    tokens.expires_at = 1777777777;

    BOOST_REQUIRE( store.StoreTokens( wxS( "provider-acme" ), wxS( "user@example.test" ), tokens ) );

    std::optional<OAUTH_TOKEN_SET> loaded =
            store.LoadTokens( wxS( "provider-acme" ), wxS( "user@example.test" ) );

    BOOST_REQUIRE( loaded.has_value() );
    BOOST_CHECK_EQUAL( loaded->access_token, wxString( "access-token" ) );
    BOOST_CHECK_EQUAL( loaded->refresh_token, wxString( "refresh-token" ) );
    BOOST_CHECK_EQUAL( loaded->scope, wxString( "openid profile parts.read" ) );

    BOOST_REQUIRE( store.DeleteTokens( wxS( "provider-acme" ), wxS( "user@example.test" ) ) );
    BOOST_CHECK( !store.LoadTokens( wxS( "provider-acme" ), wxS( "user@example.test" ) ).has_value() );
}

BOOST_AUTO_TEST_CASE( TokenStoreDoesNotSerializeIntoProviderSettings )
{
    REMOTE_PROVIDER_SETTINGS settings;
    REMOTE_PROVIDER_ENTRY provider;
    provider.provider_id = wxS( "provider-acme" );
    provider.metadata_url = wxS( "https://provider.example.test/.well-known/kicad-remote-provider" );
    provider.last_account_label = wxS( "Acme User" );
    provider.last_auth_status = wxS( "signed_in" );
    settings.providers.push_back( provider );

    const std::string dumped = nlohmann::json( settings ).dump();

    BOOST_CHECK( dumped.find( "access-token" ) == std::string::npos );
    BOOST_CHECK( dumped.find( "refresh-token" ) == std::string::npos );
    BOOST_CHECK( dumped.find( "id-token" ) == std::string::npos );
}

BOOST_AUTO_TEST_SUITE_END()
