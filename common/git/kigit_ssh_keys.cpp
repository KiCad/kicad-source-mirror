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

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>

#include <wx/arrstr.h>
#include <wx/filename.h>
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


std::vector<wxString> KIGIT::DefaultSSHKeyCandidates( const wxString& aHomeDir, const wxString& aHost )
{
    std::vector<wxString> keys;

    // An empty home would make every key path relative to the working directory
    if( aHomeDir.IsEmpty() )
        return keys;

    wxFileName sshDir( aHomeDir, wxEmptyString );
    sshDir.AppendDir( wxS( ".ssh" ) );

    const wxString        configPath = wxFileName( sshDir.GetPath(), wxS( "config" ) ).GetFullPath();
    std::vector<wxString> candidates;
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

        candidates = ParseSSHConfigIdentityFiles( decoded, aHost, aHomeDir );
    }

    for( const wxString& name : { wxS( "id_ed25519" ), wxS( "id_ecdsa" ), wxS( "id_rsa" ), wxS( "id_dsa" ) } )
        candidates.push_back( wxFileName( sshDir.GetPath(), name ).GetFullPath() );

    for( const wxString& candidate : candidates )
    {
        wxFileName fn( candidate );

        // ssh would resolve a relative path against its working directory, which ours is not
        if( !fn.IsAbsolute() || !IsOfferableSSHKey( fn.GetFullPath() ) )
            continue;

        const wxString path = fn.GetFullPath();

        bool seen = std::any_of( keys.begin(), keys.end(),
                                 [&]( const wxString& aKey )
                                 {
                                     return IsSameSSHKey( aKey, path );
                                 } );

        if( !seen )
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
wxString KIGIT::HomeDir()
{
#ifdef _WIN32
    return wxGetHomeDir();
#else
    if( const char* home = std::getenv( "HOME" ); home && *home )
        return wxString( home, wxConvLibc );

    passwd            entry;
    passwd*           found = nullptr;
    std::vector<char> buffer( 1024 );
    int               rc;

    while( ( rc = getpwuid_r( getuid(), &entry, buffer.data(), buffer.size(), &found ) ) == ERANGE )
        buffer.resize( buffer.size() * 2 );

    if( rc == 0 && found && found->pw_dir )
        return wxString( found->pw_dir, wxConvLibc );

    return wxEmptyString;
#endif
}


bool KIGIT::IsOfferableSSHKey( const wxString& aKey )
{
    return !aKey.IsEmpty() && wxFileName::FileExists( aKey ) && wxFileName::FileExists( aKey + wxS( ".pub" ) );
}
