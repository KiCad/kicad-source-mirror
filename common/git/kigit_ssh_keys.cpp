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

#include "kigit_ssh_keys.h"

#include <trace_helpers.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include <wx/arrstr.h>
#include <wx/base64.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/utils.h>

#ifndef _WIN32
#include <cerrno>
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#endif


namespace
{

// Quotes only group, so a Windows path keeps its backslashes
std::vector<wxString> splitArgs( const wxString& aText )
{
    std::vector<wxString> args;
    wxString              current;
    bool                  quoted = false;
    bool                  inArg = false;

    for( wxUniChar c : aText )
    {
        // As ssh, a comment starts only where an argument could
        if( !quoted && !inArg && c == '#' )
            break;

        if( c == '"' )
        {
            quoted = !quoted;
            inArg = true;
        }
        else if( !quoted && ( c == ' ' || c == '\t' ) )
        {
            if( inArg )
                args.push_back( current );

            current.clear();
            inArg = false;
        }
        else
        {
            current += c;
            inArg = true;
        }
    }

    if( inArg )
        args.push_back( current );

    return args;
}


// git runs core.sshCommand through sh, Git for Windows included, so its words split as sh splits them
std::vector<wxString> splitShellArgs( const wxString& aText )
{
    std::vector<wxString> args;
    wxString              current;
    bool                  inArg = false;

    for( auto it = aText.begin(); it != aText.end(); ++it )
    {
        const wxUniChar c = *it;

        if( c == '\'' )
        {
            inArg = true;

            while( ++it != aText.end() && *it != '\'' )
                current += *it;

            if( it == aText.end() )
                break;
        }
        else if( c == '"' )
        {
            inArg = true;

            while( ++it != aText.end() && *it != '"' )
            {
                auto next = std::next( it );

                if( *it == '\\' && next != aText.end()
                    && ( *next == '"' || *next == '\\' || *next == '$' || *next == '`' ) )
                {
                    it = next;
                }

                current += *it;
            }

            if( it == aText.end() )
                break;
        }
        else if( c == '\\' )
        {
            inArg = true;

            if( ++it == aText.end() )
                break;

            current += *it;
        }
        else if( c == ' ' || c == '\t' || c == '\n' )
        {
            if( inArg )
                args.push_back( current );

            current.clear();
            inArg = false;
        }
        else
        {
            current += c;
            inArg = true;
        }
    }

    if( inArg )
        args.push_back( current );

    return args;
}


// A keyword ends at whitespace or '=', and either or both may separate it from its arguments
void splitKeyword( const wxString& aLine, wxString& aKeyword, wxString& aArgs )
{
    size_t end = aLine.find_first_of( wxS( " \t=" ) );

    if( end == wxString::npos )
    {
        aKeyword = aLine;
        aArgs.clear();
        return;
    }

    aKeyword = aLine.substr( 0, end );

    size_t start = aLine.find_first_not_of( wxS( " \t" ), end );

    if( start != wxString::npos && aLine[start] == '=' )
        start = aLine.find_first_not_of( wxS( " \t" ), start + 1 );

    aArgs = start == wxString::npos ? wxString() : aLine.substr( start );
}


// A matching negated pattern rejects the host even when another pattern accepts it. ssh compares
// patterns case-sensitively with the host as typed in the address
bool hostMatches( const std::vector<wxString>& aPatterns, const wxString& aHost )
{
    bool matched = false;

    for( const wxString& pattern : aPatterns )
    {
        const bool negated = pattern.StartsWith( wxS( "!" ) );
        wxString   glob = negated ? pattern.Mid( 1 ) : pattern;

        if( !aHost.Matches( glob ) )
            continue;

        if( negated )
            return false;

        matched = true;
    }

    return matched;
}


wxString expandPath( const wxString& aValue, const wxString& aHomeDir )
{
    // The other tokens need the connection's user, port and the like, which we do not know
    for( size_t pos = aValue.find( '%' ); pos != wxString::npos; pos = aValue.find( '%', pos + 2 ) )
    {
        if( pos + 1 >= aValue.size() || aValue[pos + 1] != 'd' )
            return aValue;
    }

    wxString head;
    wxString tail = aValue;

    // "~user" names another account's home, which is not ours to guess
    if( aValue == wxS( "~" ) || aValue.StartsWith( wxS( "~/" ) ) || aValue.StartsWith( wxS( "~\\" ) ) )
    {
        head = aHomeDir;
        tail = aValue.Mid( 1 );
    }

    tail.Replace( wxS( "%d" ), aHomeDir );
    return head + tail;
}


// sshd allows 6 attempts by default. The agent and each key refused before this one use some, so a long tail
// of keys found by name alone would never be reached
constexpr size_t MAX_DISCOVERED_KEYS = 4;


struct DERIVED_PUBLIC_KEYS
{
    std::mutex mutex;
    wxString   folder;
};


DERIVED_PUBLIC_KEYS& derivedPublicKeys()
{
    static DERIVED_PUBLIC_KEYS store;
    return store;
}


// FNV-1a, so a key finds the same file in every session and build
wxString derivedPublicKeyPath( const wxString& aKey )
{
    wxString folder;

    {
        DERIVED_PUBLIC_KEYS&        store = derivedPublicKeys();
        std::lock_guard<std::mutex> lock( store.mutex );
        folder = store.folder;
    }

    if( folder.IsEmpty() || aKey.IsEmpty() )
        return wxEmptyString;

    uint64_t hash = 14695981039346656037ULL;

    for( unsigned char c : wxFileName( aKey ).GetFullPath().utf8_string() )
    {
        hash ^= c;
        hash *= 1099511628211ULL;
    }

    return wxFileName( folder, wxString::Format( wxS( "%016llx.pub" ), static_cast<unsigned long long>( hash ) ) )
            .GetFullPath();
}


uint32_t readUint32( const unsigned char* aData )
{
    return ( static_cast<uint32_t>( aData[0] ) << 24 ) | ( static_cast<uint32_t>( aData[1] ) << 16 )
           | ( static_cast<uint32_t>( aData[2] ) << 8 ) | static_cast<uint32_t>( aData[3] );
}


// The "<type> <base64 blob>" line for an OpenSSH private key, read from its unencrypted header. Empty for any
// other format, whose public half is only reachable with the passphrase
std::string openSSHPublicKeyLine( const std::string& aKeyText )
{
    const std::string begin = "-----BEGIN OPENSSH PRIVATE KEY-----";
    size_t            start = aKeyText.find( begin );

    if( start == std::string::npos )
        return {};

    start += begin.size();
    size_t end = aKeyText.find( "-----END", start );

    if( end == std::string::npos )
        return {};

    wxMemoryBuffer raw = wxBase64Decode( aKeyText.data() + start, end - start, wxBase64DecodeMode_SkipWS );

    const unsigned char* data = static_cast<const unsigned char*>( raw.GetData() );
    const size_t         size = raw.GetDataLen();
    const char           magic[] = "openssh-key-v1";
    size_t               pos = sizeof( magic );

    if( size < pos || std::memcmp( data, magic, pos ) != 0 )
        return {};

    // Cipher name, KDF name and KDF options come before the key count and the first public key
    for( int field = 0; field < 3; ++field )
    {
        if( size < pos + 4 || size - pos - 4 < readUint32( data + pos ) )
            return {};

        pos += 4 + readUint32( data + pos );
    }

    if( size < pos + 8 || readUint32( data + pos ) == 0 )
        return {};

    const uint32_t blobLen = readUint32( data + pos + 4 );
    pos += 8;

    if( size - pos < blobLen || blobLen < 4 || blobLen - 4 < readUint32( data + pos ) )
        return {};

    const std::string type( reinterpret_cast<const char*>( data + pos + 4 ), readUint32( data + pos ) );

    if( type.empty() )
        return {};

    return type + " " + wxBase64Encode( data + pos, blobLen ).utf8_string() + "\n";
}


// A private key named only by being in ~/.ssh with a .pub beside it, sorted so the order is stable
std::vector<wxString> scanSSHFolder( const wxString& aSSHDir )
{
    std::vector<wxString> keys;
    wxDir                 dir;

    if( !wxDir::Exists( aSSHDir ) || !dir.Open( aSSHDir ) )
        return keys;

    wxString name;

    for( bool more = dir.GetFirst( &name, wxEmptyString, wxDIR_FILES ); more; more = dir.GetNext( &name ) )
    {
        const wxString path = wxFileName( aSSHDir, name ).GetFullPath();

        if( name.EndsWith( wxS( ".pub" ) ) || !wxFileName::FileExists( path + wxS( ".pub" ) ) )
            continue;

        wxFFile file( path, wxS( "rb" ) );
        char    head[64] = {};
        size_t  read = file.IsOpened() ? file.Read( head, sizeof( head ) - 1 ) : 0;

        if( std::string( head, read ).find( "PRIVATE KEY" ) != std::string::npos )
            keys.push_back( path );
    }

    std::sort( keys.begin(), keys.end() );
    return keys;
}


struct REMEMBERED_KEYS
{
    std::mutex                   mutex;
    wxString                     path;
    bool                         loaded = false;
    std::map<wxString, wxString> keys;    ///< Remote address to key file
    std::map<wxString, wxString> pending; ///< Remembered but not yet saved
};


REMEMBERED_KEYS& rememberedKeys()
{
    static REMEMBERED_KEYS store;
    return store;
}


std::map<wxString, wxString> readRememberedKeys( const wxString& aPath )
{
    std::map<wxString, wxString> keys;

    if( aPath.IsEmpty() )
        return keys;

    std::ifstream file( aPath.fn_str(), std::ios::binary );

    if( !file )
        return keys;

    // A damaged file only costs the order keys are offered in, so it is dropped rather than reported
    nlohmann::json json = nlohmann::json::parse( file, nullptr, false );

    if( !json.is_object() )
        return keys;

    for( const auto& [remote, key] : json.items() )
    {
        if( key.is_string() )
            keys[wxString::FromUTF8( remote )] = wxString::FromUTF8( key.get<std::string>() );
    }

    return keys;
}


bool writeRememberedKeys( const wxString& aPath, const std::map<wxString, wxString>& aKeys )
{
    nlohmann::json json = nlohmann::json::object();

    for( const auto& [remote, key] : aKeys )
        json[remote.utf8_string()] = key.utf8_string();

    wxFileName fn( aPath );

    if( !fn.DirExists() && !fn.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        return false;

    // Commit renames a uniquely named file over the old one, so a reader never sees half a file
    wxTempFile out( aPath );

    return out.IsOpened() && out.Write( wxString::FromUTF8( json.dump( 2 ) ), wxConvUTF8 ) && out.Commit();
}


// The caller holds the store's mutex
void loadRememberedKeys( REMEMBERED_KEYS& aStore )
{
    if( aStore.loaded )
        return;

    aStore.loaded = true;
    aStore.keys = readRememberedKeys( aStore.path );
}

} // namespace


std::vector<wxString> KIGIT::ParseSSHConfigIdentityFiles( const wxString& aConfigText, const wxString& aHost,
                                                          const wxString& aHomeDir )
{
    std::vector<wxString> files;

    // Lines before the first Host apply to every host
    bool active = true;

    for( wxString line : wxSplit( aConfigText, '\n', '\0' ) )
    {
        line.Trim( true ).Trim( false );

        if( line.IsEmpty() || line.StartsWith( wxS( "#" ) ) )
            continue;

        wxString keyword;
        wxString args;
        splitKeyword( line, keyword, args );
        keyword.MakeLower();

        if( keyword == wxS( "host" ) )
        {
            active = hostMatches( splitArgs( args ), aHost );
        }
        else if( keyword == wxS( "match" ) )
        {
            // Match criteria need the resolved connection, so its block is never applied
            active = false;
        }
        else if( active && keyword == wxS( "identityfile" ) )
        {
            std::vector<wxString> values = splitArgs( args );

            if( !values.empty() )
                files.push_back( expandPath( values.front(), aHomeDir ) );
        }
    }

    return files;
}


std::vector<wxString> KIGIT::ParseSSHCommandIdentityFiles( const wxString& aCommand, const wxString& aHomeDir )
{
    std::vector<wxString> files;
    std::vector<wxString> args = splitShellArgs( aCommand );

    for( size_t ii = 0; ii < args.size(); ++ii )
    {
        if( args[ii] == wxS( "-i" ) && ii + 1 < args.size() )
            files.push_back( expandPath( args[++ii], aHomeDir ) );
        else if( args[ii].StartsWith( wxS( "-i" ) ) && args[ii].size() > 2 )
            files.push_back( expandPath( args[ii].Mid( 2 ), aHomeDir ) );
    }

    return files;
}


std::vector<wxString> KIGIT::DefaultSSHKeyCandidates( const wxString& aHomeDir, const wxString& aHost,
                                                      const wxString& aSSHCommand )
{
    std::vector<wxString> keys;

    // An empty home would make every key path relative to the working directory
    if( aHomeDir.IsEmpty() )
        return keys;

    wxFileName sshDir( aHomeDir, wxEmptyString );
    sshDir.AppendDir( wxS( ".ssh" ) );

    const wxString        configPath = wxFileName( sshDir.GetPath(), wxS( "config" ) ).GetFullPath();
    std::vector<wxString> candidates = ParseSSHCommandIdentityFiles( aSSHCommand, aHomeDir );
    std::ifstream         config( configPath.fn_str(), std::ios::binary );

    if( config )
    {
        std::string text( ( std::istreambuf_iterator<char>( config ) ), std::istreambuf_iterator<char>() );

        // ssh rejects a BOM, but Windows editors write one and it would hide the first Host line
        if( text.rfind( "\xEF\xBB\xBF", 0 ) == 0 )
            text.erase( 0, 3 );

        wxString decoded = wxString::FromUTF8( text );

        // An ANSI-saved config would otherwise decode to nothing
        if( decoded.IsEmpty() && !text.empty() )
            decoded = wxString( text.data(), wxConvISO8859_1, text.size() );

        std::vector<wxString> configured = ParseSSHConfigIdentityFiles( decoded, aHost, aHomeDir );
        candidates.insert( candidates.end(), configured.begin(), configured.end() );
    }

    for( const wxString& name : { wxS( "id_ed25519" ), wxS( "id_ecdsa" ), wxS( "id_rsa" ), wxS( "id_dsa" ) } )
        candidates.push_back( wxFileName( sshDir.GetPath(), name ).GetFullPath() );

    auto isKnown = [&]( const wxString& aPath )
    {
        return std::any_of( keys.begin(), keys.end(),
                            [&]( const wxString& aKey )
                            {
                                return IsSameSSHKey( aKey, aPath );
                            } );
    };

    for( const wxString& candidate : candidates )
    {
        wxFileName fn( candidate );

        // ssh would resolve a relative path against its working directory, which ours is not
        if( !fn.IsAbsolute() || !wxFileName::FileExists( fn.GetFullPath() ) )
            continue;

        const wxString path = fn.GetFullPath();

        if( !isKnown( path ) && !EnsurePublicKeyFile( path ).IsEmpty() )
            keys.push_back( path );
    }

    for( const wxString& path : scanSSHFolder( sshDir.GetPath() ) )
    {
        if( keys.size() >= MAX_DISCOVERED_KEYS )
            break;

        if( !isKnown( path ) )
            keys.push_back( path );
    }

    return keys;
}


bool KIGIT::IsSameSSHKey( const wxString& aKey, const wxString& aOther )
{
    if( aKey.IsEmpty() || aOther.IsEmpty() )
        return aKey == aOther;

    return wxFileName( aKey ).GetFullPath().IsSameAs( wxFileName( aOther ).GetFullPath(),
                                                      wxFileName::IsCaseSensitive() );
}


// wxGetenv converts through an unlocked static buffer except on MSVC, and key discovery runs on any thread
std::vector<wxString> KIGIT::HomeDirCandidates()
{
    std::vector<wxString> homes;

#ifdef _WIN32
    wxString drive;
    wxString path;
    wxString profile;

    homes.push_back( wxGetHomeDir() );

    // wxGetHomeDir drops HOMEPATH "", which is also how a mapped network home drive such as H: looks
    if( wxGetEnv( wxS( "HOMEDRIVE" ), &drive ) && wxGetEnv( wxS( "HOMEPATH" ), &path ) && !drive.IsEmpty() )
        homes.push_back( drive + path );

    if( wxGetEnv( wxS( "USERPROFILE" ), &profile ) && !profile.IsEmpty() )
        homes.push_back( profile );
#else
    if( const char* home = std::getenv( "HOME" ); home && *home )
    {
        homes.emplace_back( home, wxConvLibc );
        return homes;
    }

    passwd            entry;
    passwd*           found = nullptr;
    std::vector<char> buffer( 1024 );
    int               rc;

    while( ( rc = getpwuid_r( getuid(), &entry, buffer.data(), buffer.size(), &found ) ) == ERANGE )
        buffer.resize( buffer.size() * 2 );

    if( rc == 0 && found && found->pw_dir )
        homes.emplace_back( found->pw_dir, wxConvLibc );
#endif

    return homes;
}


wxString KIGIT::HomeDir()
{
    const std::vector<wxString> homes = HomeDirCandidates();

    // Git for Windows keeps keys under HOME and Windows OpenSSH under USERPROFILE
    for( const wxString& home : homes )
    {
        wxFileName sshDir( home, wxEmptyString );
        sshDir.AppendDir( wxS( ".ssh" ) );

        if( sshDir.DirExists() )
            return home;
    }

    return homes.empty() ? wxString() : homes.front();
}


wxString KIGIT::SSHKeyFolder()
{
    const wxString home = HomeDir();

    if( home.IsEmpty() )
        return wxEmptyString;

    wxFileName sshDir( home, wxEmptyString );
    sshDir.AppendDir( wxS( ".ssh" ) );
    return sshDir.GetPath();
}


wxString KIGIT::PrivateKeyPath( const wxString& aPicked )
{
    return aPicked.ends_with( wxS( ".pub" ) ) ? aPicked.Left( aPicked.size() - 4 ) : aPicked;
}


std::string KIGIT::PublicKeyBlob( const wxString& aKey )
{
    wxFFile  file( PublicKeyPath( aKey ), wxS( "rb" ) );
    wxString text;

    if( !file.IsOpened() || !file.ReadAll( &text ) )
        return {};

    // "<type> <base64 blob> [comment]" on the first line
    wxArrayString fields = wxSplit( text.BeforeFirst( '\n' ).Trim().Trim( false ), ' ', '\0' );

    if( fields.size() < 2 )
        return {};

    wxMemoryBuffer blob = wxBase64Decode( fields[1] );

    return std::string( static_cast<const char*>( blob.GetData() ), blob.GetDataLen() );
}


bool KIGIT::IsOfferableSSHKey( const wxString& aKey )
{
    return !aKey.IsEmpty() && wxFileName::FileExists( aKey ) && !EnsurePublicKeyFile( aKey ).IsEmpty();
}


wxString KIGIT::PublicKeyPath( const wxString& aKey )
{
    if( aKey.IsEmpty() )
        return wxEmptyString;

    if( wxFileName::FileExists( aKey + wxS( ".pub" ) ) )
        return aKey + wxS( ".pub" );

    const wxString derived = derivedPublicKeyPath( aKey );

    return !derived.IsEmpty() && wxFileName::FileExists( derived ) ? derived : wxString();
}


wxString KIGIT::EnsurePublicKeyFile( const wxString& aKey )
{
    if( aKey.IsEmpty() || wxFileName::FileExists( aKey + wxS( ".pub" ) ) )
        return PublicKeyPath( aKey );

    const wxString derived = derivedPublicKeyPath( aKey );
    std::ifstream  file( aKey.fn_str(), std::ios::binary );

    if( derived.IsEmpty() || !file )
        return wxEmptyString;

    const std::string line = openSSHPublicKeyLine(
            std::string( ( std::istreambuf_iterator<char>( file ) ), std::istreambuf_iterator<char>() ) );

    if( line.empty() )
        return wxEmptyString;

    // Rewritten when it differs, so a key regenerated at the same path does not keep the old public half
    std::ifstream     old( derived.fn_str(), std::ios::binary );
    const std::string current( ( std::istreambuf_iterator<char>( old ) ), std::istreambuf_iterator<char>() );

    if( current == line )
        return derived;

    wxFileName fn( derived );

    if( !fn.DirExists() && !fn.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        return wxEmptyString;

    wxTempFile out( derived );

    if( out.IsOpened() && out.Write( wxString::FromUTF8( line ), wxConvUTF8 ) && out.Commit() )
        return derived;

    wxLogTrace( traceGit, "Could not write the public key for %s to %s", aKey, derived );
    return wxEmptyString;
}


void KIGIT::SetDerivedPublicKeyFolder( const wxString& aDir )
{
    DERIVED_PUBLIC_KEYS&        store = derivedPublicKeys();
    std::lock_guard<std::mutex> lock( store.mutex );
    store.folder = aDir;
}


wxString KIGIT::RememberedSSHKey( const wxString& aRemote )
{
    REMEMBERED_KEYS&            store = rememberedKeys();
    std::lock_guard<std::mutex> lock( store.mutex );
    loadRememberedKeys( store );

    auto it = store.keys.find( aRemote );
    return it == store.keys.end() ? wxString() : it->second;
}


void KIGIT::RememberSSHKey( const wxString& aRemote, const wxString& aKey )
{
    if( aRemote.IsEmpty() || aKey.IsEmpty() )
        return;

    REMEMBERED_KEYS&            store = rememberedKeys();
    std::lock_guard<std::mutex> lock( store.mutex );
    loadRememberedKeys( store );

    auto it = store.keys.find( aRemote );

    if( it != store.keys.end() && IsSameSSHKey( it->second, aKey ) && store.pending.empty() )
        return;

    store.keys[aRemote] = aKey;
    store.pending[aRemote] = aKey;

    if( store.path.IsEmpty() )
        return;

    // Another KiCad may have saved since this one loaded, so merge into what is on disk now
    std::map<wxString, wxString> merged = readRememberedKeys( store.path );

    for( const auto& [remote, key] : store.pending )
        merged[remote] = key;

    if( writeRememberedKeys( store.path, merged ) )
    {
        store.keys = std::move( merged );
        store.pending.clear();
    }
    else
    {
        wxLogTrace( traceGit, "Could not save remembered SSH keys to %s", store.path );
    }
}


void KIGIT::SetRememberedSSHKeyStore( const wxString& aPath )
{
    REMEMBERED_KEYS&            store = rememberedKeys();
    std::lock_guard<std::mutex> lock( store.mutex );
    store.path = aPath;
    store.loaded = false;
    store.keys.clear();
    store.pending.clear();
}
