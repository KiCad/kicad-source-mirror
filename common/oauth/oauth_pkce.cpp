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

#include <oauth/oauth_pkce.h>

#include <random>
#include <vector>

#include <picosha2.h>
#include <wx/base64.h>


namespace
{
const char TOKEN_CHARS[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";


wxString randomToken( size_t aLength )
{
    static std::random_device rd;
    static std::mt19937_64 gen( rd() );
    std::uniform_int_distribution<size_t> dist( 0, sizeof( TOKEN_CHARS ) - 2 );

    wxString token;
    token.reserve( aLength );

    for( size_t ii = 0; ii < aLength; ++ii )
        token.Append( TOKEN_CHARS[dist( gen )] );

    return token;
}


wxString base64UrlEncode( const std::vector<unsigned char>& aBytes )
{
    wxString encoded = wxBase64Encode( aBytes.data(), aBytes.size() );
    encoded.Replace( wxS( "+" ), wxS( "-" ) );
    encoded.Replace( wxS( "/" ), wxS( "_" ) );
    encoded.Replace( wxS( "=" ), wxEmptyString );
    encoded.Replace( wxS( "\n" ), wxEmptyString );
    return encoded;
}
} // namespace


wxString OAUTH_PKCE::GenerateCodeVerifier()
{
    return randomToken( 64 );
}


wxString OAUTH_PKCE::GenerateState()
{
    return randomToken( 32 );
}


wxString OAUTH_PKCE::CreateCodeChallenge( const wxString& aVerifier )
{
    const std::string verifier = aVerifier.ToStdString();
    std::vector<unsigned char> digest( picosha2::k_digest_size );

    picosha2::hash256( verifier.begin(), verifier.end(), digest.begin(), digest.end() );

    return base64UrlEncode( digest );
}
