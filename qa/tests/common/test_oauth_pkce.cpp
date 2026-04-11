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

#include <oauth/oauth_pkce.h>


BOOST_AUTO_TEST_SUITE( OAuthPkceTests )

BOOST_AUTO_TEST_CASE( Rfc7636ChallengeExampleMatches )
{
    const wxString verifier = wxS( "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk" );
    const wxString challenge = OAUTH_PKCE::CreateCodeChallenge( verifier );

    BOOST_CHECK_EQUAL( challenge, wxString( "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM" ) );
}

BOOST_AUTO_TEST_CASE( GeneratedVerifierAndStateLookValid )
{
    const wxString verifier = OAUTH_PKCE::GenerateCodeVerifier();
    const wxString state = OAUTH_PKCE::GenerateState();

    BOOST_CHECK_GE( verifier.Length(), 43 );
    BOOST_CHECK_LE( verifier.Length(), 128 );
    BOOST_CHECK( verifier.find_first_not_of(
            wxS( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~" ) )
                 == wxString::npos );
    BOOST_CHECK_GE( state.Length(), 32 );
    BOOST_CHECK( !OAUTH_PKCE::CreateCodeChallenge( verifier ).IsEmpty() );
}

BOOST_AUTO_TEST_SUITE_END()
