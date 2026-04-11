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

#include <json_common.h>

#include <remote_provider_settings.h>


namespace
{
REMOTE_PROVIDER_ENTRY makeProvider( const wxString& aId, const wxString& aUrl,
                                    const wxString& aLabel = wxEmptyString,
                                    const wxString& aStatus = wxEmptyString )
{
    REMOTE_PROVIDER_ENTRY provider;
    provider.provider_id = aId;
    provider.metadata_url = aUrl;
    provider.display_name_override = aId + wxS( " display" );
    provider.last_account_label = aLabel;
    provider.last_auth_status = aStatus;
    return provider;
}
} // namespace


BOOST_AUTO_TEST_SUITE( RemoteProviderSettingsTests )

BOOST_AUTO_TEST_CASE( MultipleProvidersRoundTrip )
{
    REMOTE_PROVIDER_SETTINGS settings;
    settings.providers = {
        makeProvider( wxS( "acme" ), wxS( "https://provider.example.test/.well-known/kicad-remote-provider" ),
                      wxS( "Acme User" ), wxS( "signed_in" ) ),
        makeProvider( wxS( "local-dev" ), wxS( "http://127.0.0.1:8080/.well-known/kicad-remote-provider" ),
                      wxS( "Local Dev" ), wxS( "signed_out" ) )
    };
    settings.last_used_provider_id = wxS( "local-dev" );
    settings.destination_dir = wxS( "${KIPRJMOD}/VendorParts" );
    settings.library_prefix = wxS( "vendor" );
    settings.add_to_global_table = true;

    nlohmann::json serialized = settings;
    REMOTE_PROVIDER_SETTINGS roundTrip = serialized.get<REMOTE_PROVIDER_SETTINGS>();

    BOOST_CHECK_EQUAL( roundTrip.providers.size(), 2 );
    BOOST_CHECK_EQUAL( roundTrip.providers[0].provider_id, wxString( "acme" ) );
    BOOST_CHECK_EQUAL( roundTrip.providers[1].metadata_url,
                       wxString( "http://127.0.0.1:8080/.well-known/kicad-remote-provider" ) );
    BOOST_CHECK_EQUAL( roundTrip.last_used_provider_id, wxString( "local-dev" ) );
    BOOST_CHECK_EQUAL( roundTrip.destination_dir, wxString( "${KIPRJMOD}/VendorParts" ) );
    BOOST_CHECK_EQUAL( roundTrip.library_prefix, wxString( "vendor" ) );
    BOOST_CHECK( roundTrip.add_to_global_table );
}

BOOST_AUTO_TEST_CASE( SecretsAreNotPersisted )
{
    REMOTE_PROVIDER_SETTINGS settings;
    settings.providers = {
        makeProvider( wxS( "acme" ), wxS( "https://provider.example.test/.well-known/kicad-remote-provider" ),
                      wxS( "Acme User" ), wxS( "signed_in" ) )
    };

    nlohmann::json serialized = settings;
    const std::string dumped = serialized.dump();

    BOOST_CHECK_EQUAL( serialized["providers"][0]["last_account_label"].get<wxString>(),
                       wxString( "Acme User" ) );
    BOOST_CHECK( dumped.find( "token" ) == std::string::npos );
    BOOST_CHECK( dumped.find( "refresh" ) == std::string::npos );
    BOOST_CHECK( dumped.find( "secret" ) == std::string::npos );
    BOOST_CHECK( dumped.find( "user_id" ) == std::string::npos );
}

BOOST_AUTO_TEST_CASE( LastUsedProviderDefaultsAndRoundTrips )
{
    REMOTE_PROVIDER_SETTINGS defaults;

    BOOST_CHECK( defaults.last_used_provider_id.IsEmpty() );
    BOOST_CHECK_EQUAL( defaults.destination_dir, REMOTE_PROVIDER_SETTINGS::DefaultDestinationDir() );
    BOOST_CHECK_EQUAL( defaults.library_prefix, REMOTE_PROVIDER_SETTINGS::DefaultLibraryPrefix() );
    BOOST_CHECK( !defaults.add_to_global_table );

    defaults.last_used_provider_id = wxS( "acme" );

    REMOTE_PROVIDER_SETTINGS roundTrip = nlohmann::json( defaults ).get<REMOTE_PROVIDER_SETTINGS>();
    BOOST_CHECK_EQUAL( roundTrip.last_used_provider_id, wxString( "acme" ) );
}

BOOST_AUTO_TEST_SUITE_END()
