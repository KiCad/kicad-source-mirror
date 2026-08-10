/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "kigit_remote_url.h"

#include <wx/regex.h>
#include <wx/utils.h>

#include <cctype>
#include <string>

namespace
{

// Index of the '@' ending the userinfo of proto://user:password@host/path, or npos. The last
// one, since the first would split inside a password that contains an '@'
size_t findUserinfoAt( const wxString& aUrl )
{
    size_t protoSep = aUrl.find( "://" );

    if( protoSep == wxString::npos )
        return wxString::npos;

    size_t hostStart = protoSep + 3;
    size_t pathStart = aUrl.find_first_of( "/?#", hostStart );
    size_t searchEnd = ( pathStart == wxString::npos ) ? aUrl.length() : pathStart;

    if( searchEnd <= hostStart )
        return wxString::npos;

    size_t at = aUrl.rfind( '@', searchEnd - 1 );

    return ( at != wxString::npos && at >= hostStart ) ? at : wxString::npos;
}


// libgit2's git_str_decode_percent when the whole result is UTF-8; otherwise the component stays as written
wxString decodePercent( const wxString& aText )
{
    const std::string text = aText.utf8_string();
    std::string       decoded;

    for( size_t ii = 0; ii < text.size(); ++ii )
    {
        if( text[ii] == '%' && ii + 2 < text.size() && isxdigit( (unsigned char) text[ii + 1] )
            && isxdigit( (unsigned char) text[ii + 2] ) )
        {
            decoded += static_cast<char>( std::stoi( text.substr( ii + 1, 2 ), nullptr, 16 ) );
            ii += 2;
        }
        else
        {
            decoded += text[ii];
        }
    }

    wxString result = wxString::FromUTF8( decoded );

    // wxString cannot hold bytes that are not UTF-8, so such text stays as written
    return result.IsEmpty() && !decoded.empty() ? aText : result;
}


// 0 when the address names no port, -1 when it names one that cannot be used
int parsePort( const wxString& aPort )
{
    long port = 0;

    if( aPort.IsEmpty() )
        return 0;

    if( !aPort.ToLong( &port ) || port <= 0 || port > 65535 )
        return -1;

    return static_cast<int>( port );
}


bool isScpIpv6( const wxString& aText, size_t aBracket )
{
    size_t colons = 0;

    for( size_t ii = aBracket + 1; ii < aText.length(); ++ii )
    {
        wxUniChar c = aText[ii];

        if( c == ':' )
            colons++;

        if( c == ']' )
            return colons > 1;

        if( c != ':' && !wxIsxdigit( c ) )
            return false;
    }

    return false;
}


bool scpHasUser( const wxString& aText, size_t aStart )
{
    for( size_t ii = aStart; ii < aText.length(); ++ii )
    {
        if( aText[ii] == '@' )
            return true;

        if( aText[ii] == ':' )
            return false;
    }

    return false;
}


// libgit2's git_net_url_parse_scp, state for state, so the host key check binds the host and port it dials
bool parseScpAddress( const wxString& aURL, wxString& aUser, wxString& aHost, wxString& aPort )
{
    enum
    {
        NONE,
        USER,
        HOST_START,
        HOST,
        HOST_END,
        IPV6,
        IPV6_END,
        PORT_START,
        PORT,
        PORT_END,
        PATH_START
    } state = NONE;

    size_t user = 0, userLen = 0, host = 0, hostLen = 0, port = 0, portLen = 0;
    int    bracket = 0;
    bool   havePath = false;

    for( size_t ii = 0; ii < aURL.length() && !havePath; ++ii )
    {
        wxUniChar c = aURL[ii];

        switch( state )
        {
        case NONE:
            if( c == '@' || c == ':' )
                return false;

            if( c == '[' )
            {
                if( isScpIpv6( aURL, ii ) )
                {
                    state = IPV6;
                    host = ii;
                }
                else if( bracket++ > 1 )
                {
                    return false;
                }
            }
            else if( scpHasUser( aURL, ii ) )
            {
                state = USER;
                user = ii;
            }
            else
            {
                state = HOST;
                host = ii;
            }

            break;

        case USER:
            if( c == '@' )
            {
                userLen = ii - user;
                state = HOST_START;
            }

            break;

        case HOST_START:
            state = c == '[' ? IPV6 : HOST;
            host = ii;
            break;

        case HOST:
            if( c == ':' )
            {
                hostLen = ii - host;
                state = bracket ? PORT_START : PATH_START;
            }
            else if( c == ']' )
            {
                if( bracket-- == 0 )
                    return false;

                hostLen = ii - host;
                state = HOST_END;
            }

            break;

        case HOST_END:
            if( c != ':' )
                return false;

            state = PATH_START;
            break;

        case IPV6:
            if( c == ']' )
                state = IPV6_END;

            break;

        case IPV6_END:
            if( c != ':' )
                return false;

            hostLen = ii - host;
            state = bracket ? PORT_START : PATH_START;
            break;

        case PORT_START:
            port = ii;
            state = PORT;
            break;

        case PORT:
            if( c == ']' )
            {
                if( bracket-- == 0 )
                    return false;

                portLen = ii - port;
                state = PORT_END;
            }

            break;

        case PORT_END:
            if( c != ':' )
                return false;

            state = PATH_START;
            break;

        case PATH_START: havePath = true; break;
        }
    }

    if( !havePath || hostLen == 0 )
        return false;

    aUser = aURL.Mid( user, userLen );
    aHost = aURL.Mid( host, hostLen );
    aPort = aURL.Mid( port, portLen );

    // libgit2 keeps an IPv6 host in its brackets here, but hands it out without them for URLs
    if( aHost.StartsWith( "[" ) && aHost.EndsWith( "]" ) )
        aHost = aHost.Mid( 1, aHost.length() - 2 );

    return true;
}


} // namespace


namespace KIGIT
{

REMOTE_URL ParseRemoteURL( const wxString& aURL )
{
    REMOTE_URL result;
    result.url = aURL;

    size_t schemeEnd = aURL.find( "://" );
    size_t hostStart = 0;
    size_t hostEnd;

    if( schemeEnd != wxString::npos )
    {
        wxString scheme = aURL.Left( schemeEnd ).Lower();

        if( scheme == "https" || scheme == "http" )
            result.connType = GIT_CONN_TYPE::GIT_CONN_HTTPS;
        else if( scheme == "ssh" || scheme == "ssh+git" || scheme == "git+ssh" )
            result.connType = GIT_CONN_TYPE::GIT_CONN_SSH;
        else
            return result;

        hostStart = schemeEnd + 3;
        hostEnd = aURL.find_first_of( "/?#", hostStart );
        size_t at = findUserinfoAt( aURL );

        if( at != wxString::npos )
        {
            wxString credentials = aURL.Mid( hostStart, at - hostStart );
            size_t   colon = credentials.find( ':' );
            wxString user = credentials.Left( colon );
            result.username = decodePercent( user );

            if( colon != wxString::npos )
                result.password = decodePercent( credentials.Mid( colon + 1 ) );

            wxString userPrefix;

            if( result.connType == GIT_CONN_TYPE::GIT_CONN_SSH && !user.IsEmpty() )
                userPrefix = user + "@";

            result.url = aURL.Left( hostStart ) + userPrefix + aURL.Mid( at + 1 );
            hostStart = at + 1;
        }
    }
    else
    {
        // A colon before the path marks an SCP address, except for Windows drive paths
        if( aURL.length() >= 2 && wxIsalpha( aURL[0] ) && aURL[1] == ':' )
            return result;

        hostEnd = aURL.find( ':' );
        size_t bracket = aURL.find( '[' );

        if( bracket != wxString::npos && bracket < hostEnd )
        {
            size_t endBracket = aURL.find( ']', bracket );

            if( endBracket != wxString::npos )
                hostEnd = aURL.find( ':', endBracket + 1 );
        }

        size_t slash = aURL.find_first_of( "/\\" );

        if( hostEnd == wxString::npos || hostEnd == 0 || ( slash != wxString::npos && slash < hostEnd ) )
        {
            return result;
        }

        result.connType = GIT_CONN_TYPE::GIT_CONN_SSH;
        wxString port;

        // An address libgit2 cannot read leaves no host, which the host key check refuses
        if( parseScpAddress( aURL, result.username, result.hostname, port ) )
            result.port = parsePort( port );

        return result;
    }

    wxString host = aURL.Mid( hostStart, hostEnd == wxString::npos ? wxString::npos : hostEnd - hostStart );
    wxString port;

    if( host.StartsWith( "[" ) )
    {
        result.hostname = host.AfterFirst( '[' ).BeforeFirst( ']' );
        port = host.AfterFirst( ']' ).AfterFirst( ':' );
    }
    else
    {
        result.hostname = host.BeforeFirst( ':' );
        port = host.AfterFirst( ':' );
    }

    // libgit2 connects to the decoded name, so %67he.example is ghe.example
    result.hostname = decodePercent( result.hostname );
    result.port = parsePort( port );

    return result;
}


wxString CredentialServer( const wxString& aUrl )
{
    REMOTE_URL parsed = ParseRemoteURL( aUrl );

    if( parsed.connType == GIT_CONN_TYPE::GIT_CONN_LOCAL || parsed.hostname.IsEmpty() || parsed.port < 0 )
        return wxEmptyString;

    const wxString scheme = aUrl.Contains( wxS( "://" ) ) ? aUrl.BeforeFirst( ':' ).Lower() : wxString( wxS( "ssh" ) );
    const int      defaultPort = scheme == wxS( "https" ) ? 443 : scheme == wxS( "http" ) ? 80 : 22;
    const wxString host = parsed.hostname.Lower();
    wxString       server = host.Contains( wxS( ":" ) ) ? wxS( "[" ) + host + wxS( "]" ) : host;

    if( parsed.port > 0 && parsed.port != defaultPort )
        server << wxS( ":" ) << parsed.port;

    return server;
}


wxString CredentialRealm( const wxString& aUrl )
{
    const wxString server = CredentialServer( aUrl );

    if( server.IsEmpty() || ParseRemoteURL( aUrl ).connType != GIT_CONN_TYPE::GIT_CONN_HTTPS )
        return wxEmptyString;

    return aUrl.BeforeFirst( ':' ).Lower() + wxS( "://" ) + server;
}


wxString StripEmbeddedUserinfo( const wxString& aText )
{
    wxString result;
    size_t   lineStart = 0;

    result.reserve( aText.length() );

    while( true )
    {
        size_t lineEnd = aText.find_first_of( wxS( "\r\n" ), lineStart );

        if( lineEnd == wxString::npos )
            lineEnd = aText.length();

        // A password can hold anything but a line break, even what looks like another URL, so the whole line from
        // its first "://" to its last "@" goes
        wxString     line = aText.Mid( lineStart, lineEnd - lineStart );
        const size_t sep = line.find( wxS( "://" ) );
        const size_t at = line.rfind( '@' );

        if( sep != wxString::npos && at != wxString::npos && at >= sep + 3 )
            line.erase( sep + 3, at - sep - 2 );

        result << line;

        if( lineEnd == aText.length() )
            return result;

        result << aText[lineEnd];
        lineStart = lineEnd + 1;
    }
}


wxString DecodeServerText( const char* aData, size_t aLength )
{
    wxString text = wxString::FromUTF8( aData, aLength );

    // An empty result would drop the line, and with it the scheme the scrub looks for
    if( text.IsEmpty() && aLength > 0 )
        text = wxString::From8BitData( aData, aLength );

    return text;
}


wxString PrintableServerText( const wxString& aText )
{
    static thread_local const wxRegEx escapes( wxS( "\x1B\\[[0-?]*[ -/]*[@-~]" ), wxRE_ADVANCED );

    wxString text = aText;

    // wxRegEx stops replacing at a NUL, which would leave the rest of a password in place
    text.Replace( wxString( wxUniChar( 0 ) ), wxEmptyString );

    // An escape can end in the "@" that closes a userinfo, so the raw text is scrubbed first
    text = StripEmbeddedUserinfo( text );
    escapes.ReplaceAll( &text, wxEmptyString );

    wxString printable;

    for( wxUniChar c : text )
    {
        const wxUint32 code = c.GetValue();
        const bool     control = code < 0x20 || code == 0x7F || ( code >= 0x80 && code <= 0x9F );
        const bool     bidi = ( code >= 0x202A && code <= 0x202E ) || ( code >= 0x2066 && code <= 0x2069 );

        if( code == '\t' )
            printable << wxS( " " );
        else if( ( !control && !bidi ) || code == '\n' || code == '\r' )
            printable << c;
    }

    // Removing escapes and controls can join a "://" the raw text split
    return StripEmbeddedUserinfo( printable );
}

} // namespace KIGIT
