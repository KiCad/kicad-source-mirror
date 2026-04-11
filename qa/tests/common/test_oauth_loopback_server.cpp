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

#include <oauth/oauth_loopback_server.h>


BOOST_AUTO_TEST_SUITE( OAuthLoopbackServerTests )

BOOST_AUTO_TEST_CASE( RedirectUriUsesLoopback )
{
    OAUTH_LOOPBACK_SERVER server( nullptr, wxS( "/oauth/callback" ), wxS( "expected-state" ) );

    BOOST_REQUIRE( server.Start() );
    BOOST_CHECK( server.GetRedirectUri().StartsWith( wxS( "http://127.0.0.1:" ) ) );
    BOOST_CHECK( server.GetRedirectUri().EndsWith( wxS( "/oauth/callback" ) ) );
}

BOOST_AUTO_TEST_CASE( WrongStateRejected )
{
    OAUTH_AUTHORIZATION_RESPONSE response;
    wxString error;

    const bool ok = OAUTH_LOOPBACK_SERVER::ParseAuthorizationResponse(
            wxS( "GET /oauth/callback?code=abc&state=wrong HTTP/1.1" ),
            wxS( "/oauth/callback" ), wxS( "expected-state" ), response, error );

    BOOST_CHECK( !ok );
    BOOST_CHECK( error.Contains( wxString( "state" ) ) );
}

BOOST_AUTO_TEST_CASE( WrongCallbackPathRejected )
{
    OAUTH_AUTHORIZATION_RESPONSE response;
    wxString error;

    const bool ok = OAUTH_LOOPBACK_SERVER::ParseAuthorizationResponse(
            wxS( "GET /wrong/path?code=abc&state=expected-state HTTP/1.1" ),
            wxS( "/oauth/callback" ), wxS( "expected-state" ), response, error );

    BOOST_CHECK( !ok );
    BOOST_CHECK( error.Contains( wxString( "callback path" ) ) );
}

BOOST_AUTO_TEST_CASE( MissingCodeRejected )
{
    OAUTH_AUTHORIZATION_RESPONSE response;
    wxString error;

    const bool ok = OAUTH_LOOPBACK_SERVER::ParseAuthorizationResponse(
            wxS( "GET /oauth/callback?state=expected-state HTTP/1.1" ),
            wxS( "/oauth/callback" ), wxS( "expected-state" ), response, error );

    BOOST_CHECK( !ok );
    BOOST_CHECK( error.Contains( wxString( "authorization code" ) ) );
}

BOOST_AUTO_TEST_SUITE_END()
