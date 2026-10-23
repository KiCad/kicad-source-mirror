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

#include "kigit_known_hosts.h"
#include "kigit_ssh_keys.h"

#include <git/kicad_git_compat.h>

#include <boost/uuid/detail/sha1.hpp>
#include <boost/version.hpp>
#include <picosha2.h>

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#endif

#include <wx/arrstr.h>
#include <wx/base64.h>
#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/tokenzr.h>


namespace
{

std::array<uint8_t, 20> sha1( const std::string& aData )
{
    boost::uuids::detail::sha1 hasher;
    hasher.process_bytes( aData.data(), aData.size() );

    std::array<uint8_t, 20> result;

    // Boost 1.86 changed the digest from five words to twenty bytes
#if BOOST_VERSION >= 108600
    unsigned char digest[20];
    hasher.get_digest( digest );
    std::copy( digest, digest + 20, result.begin() );
#else
    unsigned int digest[5];
    hasher.get_digest( digest );

    for( size_t ii = 0; ii < 5; ++ii )
    {
        for( size_t jj = 0; jj < 4; ++jj )
            result[ii * 4 + jj] = static_cast<uint8_t>( digest[ii] >> ( 24 - 8 * jj ) );
    }
#endif

    return result;
}


std::string decodeBase64( const wxString& aText )
{
    size_t         errorPos = 0;
    wxMemoryBuffer buffer = wxBase64Decode( aText, wxBase64DecodeMode_Strict, &errorPos );

    return std::string( static_cast<const char*>( buffer.GetData() ), buffer.GetDataLen() );
}


wxString hostToken( const wxString& aHost, int aPort )
{
    if( aPort <= 0 || aPort == 22 )
        return aHost.Lower();

    return wxString::Format( wxS( "[%s]:%d" ), aHost.Lower(), aPort );
}


// ssh hashes the lower-cased name, and compares plain patterns ignoring case
bool hostsFieldMatches( const wxString& aField, const wxString& aToken )
{
    if( aField.StartsWith( wxS( "|1|" ) ) )
    {
        wxArrayString parts = wxSplit( aField.Mid( 3 ), '|', '\0' );

        if( parts.size() != 2 )
            return false;

        std::array<uint8_t, 20> mac = KIGIT::HmacSha1( decodeBase64( parts[0] ), aToken.utf8_string() );

        return decodeBase64( parts[1] ) == std::string( mac.begin(), mac.end() );
    }

    bool matched = false;

    for( const wxString& pattern : wxSplit( aField.Lower(), ',', '\0' ) )
    {
        const bool negated = pattern.StartsWith( wxS( "!" ) );

        if( !aToken.Matches( negated ? pattern.Mid( 1 ) : pattern ) )
            continue;

        if( negated )
            return false;

        matched = true;
    }

    return matched;
}


// A path list as libgit2 keeps one, where a backslash escapes the separator
wxString firstExistingDir( const wxString& aList )
{
    wxString current;

    for( size_t ii = 0; ii <= aList.size(); ++ii )
    {
        if( ii < aList.size() && ( aList[ii] != GIT_PATH_LIST_SEPARATOR || ( ii > 0 && aList[ii - 1] == '\\' ) ) )
        {
            current += aList[ii];
            continue;
        }

        if( !current.IsEmpty() && wxFileName::DirExists( current ) )
            return current;

        current.clear();
    }

    return wxEmptyString;
}


// ssh would accept any key a writable trust store gained, so a file we create is the owner's alone
bool appendToFile( const wxString& aPath, const std::string& aText )
{
#ifdef _WIN32
    std::ofstream out( aPath.fn_str(), std::ios::binary | std::ios::app );
    out << aText;
    out.flush();

    return out.good();
#else
    struct FILE_DESCRIPTOR
    {
        int fd;

        ~FILE_DESCRIPTOR()
        {
            if( fd >= 0 )
                close( fd );
        }
    } file{ open( aPath.fn_str(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, 0600 ) };

    if( file.fd < 0 )
        return false;

    for( size_t written = 0; written < aText.size(); )
    {
        ssize_t rc = write( file.fd, aText.data() + written, aText.size() - written );

        if( rc < 0 && errno == EINTR )
            continue;

        if( rc <= 0 )
            return false;

        written += static_cast<size_t>( rc );
    }

    int fd = file.fd;
    file.fd = -1;
    return close( fd ) == 0;
#endif
}


// Unknown counts as not ending in one, as a blank line costs nothing and a joined line loses an entry
bool endsWithNewline( const wxString& aPath )
{
    if( !wxFileName::FileExists( aPath ) )
        return true;

    std::ifstream in( aPath.fn_str(), std::ios::binary | std::ios::ate );

    if( !in )
        return false;

    std::streamoff size = in.tellg();

    if( size == 0 )
        return true;

    char last = 0;
    in.seekg( size - 1 );
    in.get( last );

    return in && last == '\n';
}

} // namespace


std::array<uint8_t, 20> KIGIT::HmacSha1( const std::string& aKey, const std::string& aData )
{
    constexpr size_t blockSize = 64;

    std::string key = aKey;

    if( key.size() > blockSize )
    {
        std::array<uint8_t, 20> hashed = sha1( key );
        key.assign( hashed.begin(), hashed.end() );
    }

    key.resize( blockSize, '\0' );

    std::string inner( blockSize, '\0' );
    std::string outer( blockSize, '\0' );

    for( size_t ii = 0; ii < blockSize; ++ii )
    {
        inner[ii] = static_cast<char>( key[ii] ^ 0x36 );
        outer[ii] = static_cast<char>( key[ii] ^ 0x5c );
    }

    std::array<uint8_t, 20> innerHash = sha1( inner + aData );

    return sha1( outer + std::string( innerHash.begin(), innerHash.end() ) );
}


KIGIT::KNOWN_HOST_STATE KIGIT::LookupKnownHost( const wxString& aKnownHostsText, const wxString& aHost, int aPort,
                                                const wxString& aKeyType, const std::string& aRawKey )
{
    const int      port = aPort > 0 ? aPort : 22;
    const wxString plain = hostToken( aHost, 22 );

    // libssh2 tries [host]:port before host, even for port 22
    const wxString bracketed = wxString::Format( wxS( "[%s]:%d" ), aHost.Lower(), port );

    bool revoked = false;
    bool matched = false;
    bool mismatched = false;
    bool otherType = false;

    for( wxString line : wxSplit( aKnownHostsText, '\n', '\0' ) )
    {
        line.Trim( true ).Trim( false );

        if( line.IsEmpty() || line.StartsWith( wxS( "#" ) ) )
            continue;

        wxArrayString fields = wxStringTokenize( line, wxS( " \t" ), wxTOKEN_STRTOK );
        wxString      marker;

        if( !fields.IsEmpty() && fields[0].StartsWith( wxS( "@" ) ) )
        {
            marker = fields[0];
            fields.RemoveAt( 0 );
        }

        // Neither KiCad nor libgit2 checks host certificates, so a CA line vouches for nothing here
        if( fields.size() < 3 || ( !marker.IsEmpty() && marker != wxS( "@revoked" ) ) )
            continue;

        const bool sameKey = decodeBase64( fields[2] ) == aRawKey;

        if( !marker.IsEmpty() )
        {
            if( sameKey && ( hostsFieldMatches( fields[0], bracketed ) || hostsFieldMatches( fields[0], plain ) ) )
                revoked = true;

            continue;
        }

        if( !hostsFieldMatches( fields[0], bracketed ) && ( port != 22 || !hostsFieldMatches( fields[0], plain ) ) )
            continue;

        if( sameKey )
            matched = true;
        else if( fields[1] == aKeyType )
            mismatched = true;
        else
            otherType = true;
    }

    if( revoked )
        return KNOWN_HOST_STATE::REVOKED;

    if( matched )
        return KNOWN_HOST_STATE::MATCH;

    if( mismatched )
        return KNOWN_HOST_STATE::MISMATCH;

    return otherType ? KNOWN_HOST_STATE::OTHER_KEY_TYPE : KNOWN_HOST_STATE::UNKNOWN;
}


wxString KIGIT::FormatKnownHostsLine( const wxString& aHost, int aPort, const wxString& aKeyType,
                                      const wxString& aBase64Key )
{
    return hostToken( aHost, aPort ) + wxS( " " ) + aKeyType + wxS( " " ) + aBase64Key;
}


wxString KIGIT::HostKeyType( const std::string& aRawKey )
{
    if( aRawKey.size() < 4 )
        return wxEmptyString;

    const unsigned char* data = reinterpret_cast<const unsigned char*>( aRawKey.data() );
    uint32_t             nameLen = 0;

    for( size_t ii = 0; ii < 4; ++ii )
        nameLen = ( nameLen << 8 ) | data[ii];

    if( nameLen == 0 || nameLen > aRawKey.size() - 4 )
        return wxEmptyString;

    return wxString::FromUTF8( aRawKey.substr( 4, nameLen ) );
}


wxString KIGIT::HostKeyFingerprint( const std::string& aRawKey )
{
    std::vector<unsigned char> digest( picosha2::k_digest_size );
    picosha2::hash256( aRawKey.begin(), aRawKey.end(), digest.begin(), digest.end() );

    wxString encoded = wxBase64Encode( digest.data(), digest.size() );
    encoded.Replace( wxS( "=" ), wxEmptyString );
    return encoded;
}


wxString KIGIT::KnownHostsPath()
{
    wxString home;

    // libgit2 picks the first existing folder of this list, so asking it is the only way to agree
#if KIGIT_HAS_HOMEDIR_OPT
    git_buf homeList = GIT_BUF_INIT;

    if( git_libgit2_opts( GIT_OPT_GET_HOMEDIR, &homeList ) == GIT_OK && homeList.ptr )
        home = firstExistingDir( wxString::FromUTF8( homeList.ptr ) );

    git_buf_dispose( &homeList );
#else
    // The headers can be older than the library, which the distribution may update on its own
    int major = 0;
    int minor = 0;
    int revision = 0;
    git_libgit2_version( &major, &minor, &revision );
    home = Libgit2LegacyHome( revision );
#endif

    if( home.IsEmpty() )
        return wxEmptyString;

    wxFileName path( home, wxS( "known_hosts" ) );
    path.AppendDir( wxS( ".ssh" ) );
    return path.GetFullPath();
}


wxString KIGIT::ReadKnownHosts()
{
    const wxString path = KnownHostsPath();

    if( path.IsEmpty() )
        return wxEmptyString;

    std::ifstream in( path.fn_str(), std::ios::binary );

    if( !in )
        return wxEmptyString;

    std::string text( ( std::istreambuf_iterator<char>( in ) ), std::istreambuf_iterator<char>() );
    wxString    decoded = wxString::FromUTF8( text );

    // A comment in a legacy encoding must not hide every entry
    if( decoded.IsEmpty() && !text.empty() )
        decoded = wxString( text.data(), wxConvISO8859_1, text.size() );

    return decoded;
}


bool KIGIT::AddKnownHost( const HOST_KEY_FAILURE& aKey )
{
    const wxString path = KnownHostsPath();

    if( path.IsEmpty() || aKey.keyType.IsEmpty() || aKey.rawKey.empty() )
        return false;

    // The type comes from the server, and whitespace or a line break in it would write entries of its own
    for( wxUniChar c : aKey.keyType )
    {
        if( c <= ' ' || wxIscntrl( c ) )
            return false;
    }

    wxFileName fn( path );

    // The prompt explains a failure itself
    wxLogNull quiet;

    // ssh refuses a .ssh folder others can write to
    if( !fn.DirExists() && !fn.Mkdir( 0700, wxPATH_MKDIR_FULL ) )
        return false;

    const wxString encoded = wxBase64Encode( aKey.rawKey.data(), aKey.rawKey.size() );
    std::string    line = FormatKnownHostsLine( aKey.host, aKey.port, aKey.keyType, encoded ).utf8_string() + "\n";

    if( !endsWithNewline( path ) )
        line.insert( 0, "\n" );

    return appendToFile( path, line );
}


bool KIGIT::MayOfferTrust( const HOST_KEY_FAILURE& aFailure, bool aRetriedAfterTrust )
{
    if( aRetriedAfterTrust )
        return false;

    return aFailure.state == KNOWN_HOST_STATE::UNKNOWN || aFailure.state == KNOWN_HOST_STATE::OTHER_KEY_TYPE;
}


wxString KIGIT::HostKeyRefusalMessage( const HOST_KEY_FAILURE& aFailure )
{
    wxString host = aFailure.host;

    if( aFailure.port != 22 )
        host << wxS( ":" ) << aFailure.port;

    if( aFailure.state == KNOWN_HOST_STATE::REVOKED )
    {
        return wxString::Format( _( "The key for %s has been revoked. Ask your administrator before connecting "
                                    "again." ),
                                 host );
    }

    if( aFailure.state == KNOWN_HOST_STATE::MISMATCH )
    {
        return wxString::Format( _( "The key for %s has changed since the last connection. This can mean the server "
                                    "was replaced or that someone is intercepting the connection. Ask your "
                                    "administrator before connecting again." ),
                                 host );
    }

    return wxString::Format( _( "%s still could not be verified after its key was saved. Ask your administrator "
                                "before connecting again." ),
                             host );
}


wxString KIGIT::Libgit2LegacyHome( int aRevision )
{
#ifdef _WIN32
    // 1.5.1 joins HOME as it stands at each connection
    if( aRevision == 1 )
    {
        DWORD                len = GetEnvironmentVariableW( L"HOME", nullptr, 0 );
        std::vector<wchar_t> value( len + 1 );

        if( len == 0 || GetEnvironmentVariableW( L"HOME", value.data(), len ) == 0 )
            return wxEmptyString;

        return wxString( value.data() );
    }

    // libgit2's GIT_WIN_PATH_UTF16
    constexpr DWORD bufferSize = 4096 + 6;

    // The others keep the first of these that exists, expanded as libgit2's find_win32_dirs does
    for( const wchar_t* tmpl : { L"%HOME%\\", L"%HOMEDRIVE%%HOMEPATH%\\", L"%USERPROFILE%\\" } )
    {
        std::vector<wchar_t> path( bufferSize );
        DWORD                len = ExpandEnvironmentStringsW( tmpl, path.data(), bufferSize );

        if( len == 0 || len > bufferSize || path[0] == L'%' || _waccess( path.data(), 0 ) != 0 )
            continue;

        return wxString( path.data() );
    }

    return wxEmptyString;
#else
    wxString home;

    // 1.5.2 trusts the passwd entry over HOME when setuid or in a macOS sandbox
    if( aRevision != 1 && ( std::getenv( "APP_SANDBOX_CONTAINER_ID" ) || getuid() != geteuid() ) )
    {
        passwd            entry;
        passwd*           found = nullptr;
        std::vector<char> buffer( 1024 );
        int               rc;

        while( ( rc = getpwuid_r( geteuid(), &entry, buffer.data(), buffer.size(), &found ) ) == ERANGE
               && buffer.size() < 65536 )
        {
            buffer.resize( buffer.size() * 2 );
        }

        if( rc == 0 && found && found->pw_dir )
            home = wxString( found->pw_dir, wxConvLibc );
    }
    else if( const char* value = std::getenv( "HOME" ) )
    {
        home = wxString( value, wxConvLibc );
    }

    // 1.5.1 reads the file under HOME whether or not the folder exists
    return aRevision == 1 ? home : firstExistingDir( home );
#endif
}
