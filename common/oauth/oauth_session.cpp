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

#include <oauth/oauth_session.h>

#include <oauth/oauth_pkce.h>
#include <remote_provider_utils.h>

#include <wx/uri.h>


wxString OAUTH_SESSION::BuildAuthorizationUrl() const
{
    wxString url = authorization_endpoint;

    url << ( authorization_endpoint.Find( '?' ) == wxNOT_FOUND ? wxS( "?" ) : wxS( "&" ) )
        << wxS( "response_type=code" )
        << wxS( "&client_id=" ) << UrlEncode( client_id )
        << wxS( "&redirect_uri=" ) << UrlEncode( redirect_uri )
        << wxS( "&scope=" ) << UrlEncode( scope )
        << wxS( "&state=" ) << UrlEncode( state )
        << wxS( "&code_challenge_method=S256" )
        << wxS( "&code_challenge=" ) << UrlEncode( OAUTH_PKCE::CreateCodeChallenge( code_verifier ) );

    return url;
}
