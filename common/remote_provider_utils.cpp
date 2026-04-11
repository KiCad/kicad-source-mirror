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

#include <remote_provider_utils.h>

#include <wx/intl.h>
#include <wx/uri.h>


wxString UrlEncode( const wxString& aValue )
{
    wxString           encoded;
    const wxScopedCharBuffer utf8 = aValue.ToUTF8();
    const char*        p = utf8.data();
    size_t             len = utf8.length();

    for( size_t i = 0; i < len; ++i )
    {
        unsigned char byte = static_cast<unsigned char>( p[i] );

        if( ( byte >= 'A' && byte <= 'Z' ) || ( byte >= 'a' && byte <= 'z' )
            || ( byte >= '0' && byte <= '9' ) || byte == '-' || byte == '_'
            || byte == '.' || byte == '~' )
        {
            encoded.Append( static_cast<char>( byte ) );
        }
        else
        {
            encoded.Append( wxString::Format( wxS( "%%%02X" ), byte ) );
        }
    }

    return encoded;
}


wxString RemoteProviderJsonString( const nlohmann::json& aObject, const char* aKey )
{
    auto it = aObject.find( aKey );

    if( it != aObject.end() && it->is_string() )
        return wxString::FromUTF8( it->get_ref<const std::string&>().c_str() );

    return wxString();
}


bool IsLoopbackHost( const wxString& aHost )
{
    wxString host = aHost.Lower();

    if( host.StartsWith( wxS( "[" ) ) && host.EndsWith( wxS( "]" ) ) )
    {
        host.RemoveLast();
        host.Remove( 0, 1 );
    }

    return host == wxS( "localhost" ) || host == wxS( "127.0.0.1" ) || host == wxS( "::1" );
}


bool ValidateRemoteUrlSecurity( const wxString& aUrl, bool aAllowInsecureLocalhost,
                                wxString& aError, const wxString& aLabel )
{
    if( aUrl.IsEmpty() )
        return true;

    wxURI    uri( aUrl );
    wxString scheme = uri.GetScheme().Lower();

    if( scheme == wxS( "https" ) )
        return true;

    if( scheme == wxS( "http" ) && aAllowInsecureLocalhost && IsLoopbackHost( uri.GetServer() ) )
        return true;

    aError = wxString::Format(
            _( "%s must use HTTPS unless allow_insecure_localhost is enabled for a loopback URL." ),
            aLabel );
    return false;
}


wxString NormalizedUrlOrigin( const wxString& aUrl )
{
    if( aUrl.IsEmpty() )
        return wxString();

    wxURI uri( aUrl );

    if( !uri.HasScheme() || !uri.HasServer() )
        return wxString();

    wxString scheme = uri.GetScheme().Lower();
    wxString host = uri.GetServer().Lower();
    wxString port = uri.GetPort();

    if( port.IsEmpty() )
    {
        if( scheme == wxS( "https" ) )
            port = wxS( "443" );
        else if( scheme == wxS( "http" ) )
            port = wxS( "80" );
    }

    return scheme + wxS( "://" ) + host + wxS( ":" ) + port;
}


void COLLECTING_JSON_ERROR_HANDLER::error( const nlohmann::json::json_pointer& aPointer,
                                           const nlohmann::json&               aInstance,
                                           const std::string&                  aMessage )
{
    wxUnusedVar( aInstance );
    m_errors.emplace_back( wxString::Format( wxS( "%s: %s" ),
                                             wxString::FromUTF8( aPointer.to_string().c_str() ),
                                             wxString::FromUTF8( aMessage.c_str() ) ) );
}


wxString COLLECTING_JSON_ERROR_HANDLER::FirstError() const
{
    if( m_errors.empty() )
        return wxString();

    return m_errors.front();
}
