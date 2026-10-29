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

#include "kigit_failure.h"
#include "git_repo_mixin.h"
#include "kicad_git_compat.h"
#include "kicad_git_memory.h"

#include <trace_helpers.h>

#include <git2.h>
#include <wx/ffile.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/regex.h>
#include <wx/tokenzr.h>
#include <wx/translation.h>

#include <cstdlib>
#include <memory>


namespace
{

struct TEXT_PATTERN
{
    const char*   text; ///< Lower case
    KIGIT_FAILURE kind;
};


// Resolver and connection texts libgit2 and libssh2 produce themselves, as captured from real failures. They are
// English in every locale and never translated. Nothing a server sends is matched, only the codes it returns.
const TEXT_PATTERN c_textPatterns[] = {
    { "failed to resolve address for", KIGIT_FAILURE::SERVER_NOT_FOUND },
    { "failed to connect to", KIGIT_FAILURE::HOST_UNREACHABLE },
    { "failed getting banner", KIGIT_FAILURE::HOST_UNREACHABLE },
    // WinHTTP's failures to reach a server. The system text after the prefix is localized and is not matched.
    { "failed to send request", KIGIT_FAILURE::HOST_UNREACHABLE },
    { "failed to receive response", KIGIT_FAILURE::HOST_UNREACHABLE },
    { "error loading known_hosts", KIGIT_FAILURE::KNOWN_HOSTS_UNREADABLE },
    { "unable to open public key file", KIGIT_FAILURE::KEY_LOAD_FAILED },
    { "proxy returned unexpected status", KIGIT_FAILURE::PROXY },
};


// wxString::Lower follows the C library locale, which turns I into a dotless i in Turkish. The texts matched here are
// ASCII, so only ASCII is folded.
wxString asciiLower( const wxString& aText )
{
    wxString lower;

    for( wxUniChar c : aText )
    {
        const wxUniChar::value_type value = c.GetValue();

        lower += ( value >= 'A' && value <= 'Z' ) ? wxUniChar( value + ( 'a' - 'A' ) ) : c;
    }

    return lower;
}


// libgit2 and its WinHTTP transport word a refused request differently, but both end in the status
int httpStatusOf( const KIGIT_ERROR_INFO& aError, const wxString& aLower )
{
    static const wxString marker = wxS( "status code: " );
    const int             at = aLower.Find( marker );

    return ( aError.klass == GIT_ERROR_HTTP && at != wxNOT_FOUND ) ? wxAtoi( aLower.Mid( at + marker.length() ) ) : 0;
}


KIGIT_COMMON::GIT_CONN_TYPE connTypeOf( const KIGIT_AUTH_ATTEMPT& aAuth, const KIGIT_FAILURE_CONTEXT& aContext )
{
    KIGIT_COMMON::GIT_CONN_TYPE connType = KIGIT_COMMON::ParseRemoteURL( aContext.url ).connType;

    // The prompt can choose a transport for an address that names none
    if( connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL )
        connType = aAuth.connType;

    return connType;
}


bool lastKeyEncrypted( const KIGIT_AUTH_ATTEMPT& aAuth )
{
    // wxFFile reports an unopenable file to the user, and a key that is gone is simply not encrypted
    if( aAuth.keysTried.empty() || !wxFileName::IsFileReadable( aAuth.keysTried.back() ) )
        return false;

    wxFFile  file( aAuth.keysTried.back(), wxS( "rb" ) );
    wxString text;

    return file.IsOpened() && file.ReadAll( &text ) && KIGIT::IsPrivateKeyEncrypted( text );
}


// Lines kept for the details, without git's own transfer progress, bounded to the end where a refusal explains itself.
// Server text arrives framed by blank "error:" lines and progress carriage returns.
wxString serverText( const wxString& aText )
{
    // The titles git's progress.c and pack-objects print, so a server's own "Error code: 403" survives
    static thread_local const wxRegEx progress( wxS( "^(((Enumerating|Counting|Compressing|Writing|Receiving) objects"
                                                     "|Resolving deltas): .*|Total [0-9]+ \\(delta [0-9]+\\).*)$" ),
                                                wxRE_ADVANCED );

    wxArrayString     lines;
    wxStringTokenizer tokens( KIGIT::PrintableServerText( aText ), wxS( "\r\n" ), wxTOKEN_STRTOK );

    while( tokens.HasMoreTokens() )
    {
        const wxString line = tokens.GetNextToken().Trim().Trim( false );

        if( !line.IsEmpty() && line != wxS( "error:" ) && line != wxS( "remote:" ) && !progress.Matches( line ) )
            lines.Add( line );
    }

    // Already scrubbed, so the cut cannot leave half a URL that still names its password
    return wxJoin( lines, '\n', 0 ).Right( KIGIT::MAX_SERVER_TEXT );
}

} // namespace


static KIGIT_FAILURE classifyGitFailure( const KIGIT_ERROR_INFO& aError, const KIGIT_AUTH_ATTEMPT& aAuth,
                                         const KIGIT_FAILURE_CONTEXT& aContext )
{
    const KIGIT_COMMON::GIT_CONN_TYPE connType = connTypeOf( aAuth, aContext );
    const wxString                    lower = asciiLower( aError.message );

    if( aError.code == GIT_EUSER )
        return KIGIT_FAILURE::CANCELLED;

    if( aError.code == GIT_ENONFASTFORWARD )
        return KIGIT_FAILURE::PUSH_NOT_FAST_FORWARD;

    if( aError.code == GIT_ECERTIFICATE )
    {
        if( aContext.hostKey )
        {
            KIGIT::KNOWN_HOST_STATE state = aContext.hostKey->state;

            if( state == KIGIT::KNOWN_HOST_STATE::MISMATCH || state == KIGIT::KNOWN_HOST_STATE::REVOKED )
                return KIGIT_FAILURE::HOST_KEY_CHANGED;

            return KIGIT_FAILURE::HOST_KEY_UNKNOWN;
        }

        if( !aContext.hostKeyRefusal.IsEmpty() )
            return KIGIT_FAILURE::HOST_KEY_UNVERIFIED;

        return connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH ? KIGIT_FAILURE::OTHER
                                                                     : KIGIT_FAILURE::TLS_UNTRUSTED;
    }

    // git_remote_push reports a refused branch only through the callback, which fails the push with its status
    if( !aContext.pushRefusal.IsEmpty() )
        return KIGIT_FAILURE::SERVER_REFUSED;

    if( aError.code == GIT_EAUTH )
    {
        if( aError.klass == GIT_ERROR_HTTP && lower.Contains( wxS( "proxy" ) ) )
            return KIGIT_FAILURE::PROXY;

        if( connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS )
            return KIGIT_FAILURE::AUTH_HTTPS_REJECTED;

        // An empty or absent agent fails exactly as a rejected agent key does, so neither is claimed
        if( connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
            return aAuth.keysTried.empty() ? KIGIT_FAILURE::AUTH_NO_SSH_KEY : KIGIT_FAILURE::AUTH_SSH_KEY_REJECTED;

        return KIGIT_FAILURE::OTHER;
    }

    // libgit2 returns any SSH stderr as GIT_EEOF when stdout is empty. The whole message is then the server's text,
    // which says that the server refused and is never matched for its wording.
    if( aError.code == GIT_EEOF && aError.klass == GIT_ERROR_SSH )
    {
        return ( connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH && !aContext.authFailed
                 && !serverText( aError.message ).IsEmpty() )
                       ? KIGIT_FAILURE::SERVER_REFUSED
                       : KIGIT_FAILURE::OTHER;
    }

    // mbedTLS fails an untrusted chain with -1 rather than GIT_ECERTIFICATE, and WinHTTP reports it as an HTTP error
    if( ( aError.klass == GIT_ERROR_SSL || aError.klass == GIT_ERROR_HTTP ) && lower.Contains( wxS( "certificate" ) ) )
        return KIGIT_FAILURE::TLS_UNTRUSTED;

    const int httpStatus = httpStatusOf( aError, lower );

    if( httpStatus == 403 )
    {
        return aContext.operation == KIGIT_OPERATION::PUSH ? KIGIT_FAILURE::PUSH_FORBIDDEN
                                                           : KIGIT_FAILURE::REPO_NOT_FOUND;
    }

    if( httpStatus == 404 )
        return KIGIT_FAILURE::REPO_NOT_FOUND;

    for( const TEXT_PATTERN& pattern : c_textPatterns )
    {
        if( lower.Contains( wxString::FromUTF8( pattern.text ) ) )
        {
            // A key that failed to load was the last one the callback handed out
            if( pattern.kind == KIGIT_FAILURE::KEY_LOAD_FAILED && aAuth.keysTried.empty() )
                return KIGIT_FAILURE::OTHER;

            return pattern.kind;
        }
    }

    return KIGIT_FAILURE::OTHER;
}


namespace
{

wxString summaryFor( KIGIT_FAILURE aKind, const KIGIT_AUTH_ATTEMPT& aAuth, const KIGIT_FAILURE_CONTEXT& aContext )
{
    const KIGIT_COMMON::REMOTE_URL parsed = KIGIT_COMMON::ParseRemoteURL( aContext.url );
    const wxString                 key = aAuth.keysTried.empty() ? wxString() : aAuth.keysTried.back();
    wxString                       host = parsed.hostname;

    // A lab or company server often listens on another port, and the bare host would name the wrong service
    const int defaultPort = parsed.connType == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH
                                    ? 22
                                    : ( aContext.url.Lower().StartsWith( wxS( "http://" ) ) ? 80 : 443 );

    if( !host.IsEmpty() && parsed.port > 0 && parsed.port != defaultPort )
        host = wxString::Format( host.Contains( wxS( ":" ) ) ? wxS( "[%s]:%d" ) : wxS( "%s:%d" ), host, parsed.port );

    if( host.IsEmpty() )
        host = aContext.remoteName.IsEmpty() ? KIGIT::StripEmbeddedUserinfo( aContext.url ) : aContext.remoteName;

    switch( aKind )
    {
    case KIGIT_FAILURE::AUTH_SSH_KEY_REJECTED:
        // An encrypted key with a wrong passphrase fails exactly as an unregistered one does
        if( lastKeyEncrypted( aAuth ) && !aAuth.passphraseGiven )
        {
            return wxString::Format( _( "The SSH key %s requires a passphrase to connect to %s." ), key, host );
        }

        if( lastKeyEncrypted( aAuth ) )
        {
            return wxString::Format( _( "%s did not accept the SSH key %s. Check the key's passphrase, and that "
                                        "%s.pub is added to your account on %s." ),
                                     host, key, key, host );
        }

        return wxString::Format( _( "%s did not accept the SSH key %s. Check that %s.pub is added to your account "
                                    "on %s, or choose another key." ),
                                 host, key, key, host );

    case KIGIT_FAILURE::AUTH_NO_SSH_KEY:
        return wxString::Format( _( "%s did not accept any SSH key. KiCad found no key files, and your SSH agent "
                                    "offered none that %s accepts. Create or choose a key, and add its .pub to your "
                                    "account on %s." ),
                                 host, host, host );

    case KIGIT_FAILURE::AUTH_HTTPS_REJECTED:
        return wxString::Format( _( "%s did not accept the username and password. Many servers require a personal "
                                    "access token in place of a password." ),
                                 host );

    case KIGIT_FAILURE::KEY_LOAD_FAILED:
        return wxString::Format( _( "The SSH key %s could not be opened. Check the passphrase, and that you chose "
                                    "the private key file rather than the .pub file." ),
                                 key );

    case KIGIT_FAILURE::HOST_KEY_UNKNOWN:
        return wxString::Format( _( "%s is not a known server yet. Use Get Changes to check its key and remember "
                                    "it." ),
                                 host );

    case KIGIT_FAILURE::HOST_KEY_CHANGED:
        return KIGIT::HostKeyRefusalMessage( *aContext.hostKey );

    case KIGIT_FAILURE::KNOWN_HOSTS_UNREADABLE:
        return wxString::Format( _( "The known hosts file %s has a line that could not be read. Fix or remove that "
                                    "line, then try again." ),
                                 KIGIT::KnownHostsPath() );

    case KIGIT_FAILURE::SERVER_NOT_FOUND:
        return wxString::Format( _( "Could not find the server %s. Check the address, and that you are connected to "
                                    "the network or VPN that reaches it." ),
                                 host );

    case KIGIT_FAILURE::HOST_UNREACHABLE:
        return wxString::Format( _( "%s did not answer. Check your network or VPN connection, then try again." ),
                                 host );

    case KIGIT_FAILURE::TLS_UNTRUSTED:
        return wxString::Format( _( "This computer does not trust the security certificate of %s. Ask your "
                                    "administrator to install your organization's certificate." ),
                                 host );

    case KIGIT_FAILURE::PROXY:
        return wxString::Format( _( "The proxy refused the connection to %s. Check the proxy configured for Git "
                                    "(http.proxy) or in your system settings." ),
                                 host );

    case KIGIT_FAILURE::REPO_NOT_FOUND:
        // Clone and Test Connection show the address in their own Location field
        if( aContext.operation == KIGIT_OPERATION::CLONE || aContext.operation == KIGIT_OPERATION::CONNECT )
        {
            return wxString::Format( _( "%s has no repository at this address, or your account cannot see it. "
                                        "Check the repository address." ),
                                     host );
        }

        return wxString::Format( _( "%s has no repository at this address, or your account cannot see it. Check the "
                                    "remote's address in the Advanced Git window, Remotes page." ),
                                 host );

    case KIGIT_FAILURE::PUSH_FORBIDDEN:
        return wxString::Format( _( "%s refused the push. Your account may not have write access, or the branch is "
                                    "protected." ),
                                 host );

    case KIGIT_FAILURE::SERVER_REFUSED:
        if( aContext.operation == KIGIT_OPERATION::PUSH )
            return wxString::Format( _( "%s refused the push. The server's reason is shown in the details." ), host );

        return wxString::Format( _( "%s refused the request. The server's reason is shown in the details." ), host );

    case KIGIT_FAILURE::PUSH_NOT_FAST_FORWARD:
        return _( "The remote has changes you do not have yet. Get changes first, then push again." );

    case KIGIT_FAILURE::CANCELLED:
        return _( "Cancelled." );

    case KIGIT_FAILURE::HOST_KEY_UNVERIFIED:
        return aContext.hostKeyRefusal;

    case KIGIT_FAILURE::OTHER:
        break;
    }

    const wxString remote = aContext.remoteName.IsEmpty() ? host : aContext.remoteName;

    // Whole sentences, so translators never assemble a verb and its object
    switch( aContext.operation )
    {
    case KIGIT_OPERATION::CLONE:
        return wxString::Format( _( "Could not clone from %s. The details below may help your administrator." ),
                                 remote );

    case KIGIT_OPERATION::FETCH:
        return wxString::Format( _( "Could not get changes from %s. The details below may help your administrator." ),
                                 remote );

    case KIGIT_OPERATION::PUSH:
        return wxString::Format( _( "Could not push to %s. The details below may help your administrator." ), remote );

    case KIGIT_OPERATION::SET_UP_REMOTE:
        return wxString::Format( _( "Could not set up the remote %s. The details below may help your administrator." ),
                                 remote );

    case KIGIT_OPERATION::CONNECT:
        return wxString::Format( _( "Could not connect to %s. The details below may help your administrator." ),
                                 remote );
    }

    return wxString();
}


wxString detailsFor( const KIGIT_ERROR_INFO& aError, const KIGIT_AUTH_ATTEMPT& aAuth,
                     const KIGIT_FAILURE_CONTEXT& aContext )
{
    // An SSH server's stderr arrives as libgit2's message, so it is treated as server text too
    wxString details = serverText( aError.message );

    auto addParagraph = [&]( const wxString& aText )
    {
        if( aText.IsEmpty() )
            return;

        if( !details.IsEmpty() )
            details << wxS( "\n\n" );

        details << aText;
    };

    const wxString pushRefusal = serverText( aContext.pushRefusal );

    if( !pushRefusal.IsEmpty() && !details.Contains( pushRefusal ) )
        addParagraph( pushRefusal );

    addParagraph( serverText( aContext.serverMessage ) );

    wxString offered = aAuth.agentTried ? _( "SSH agent" ) : wxString();

    for( const wxString& key : aAuth.keysTried )
        offered << ( offered.IsEmpty() ? wxString() : wxString( wxS( ", " ) ) ) << key;

    if( !offered.IsEmpty() )
        addParagraph( wxString::Format( _( "Keys offered: %s" ), offered ) );

    if( !aContext.url.IsEmpty() )
        addParagraph( wxString::Format( _( "Address: %s" ), KIGIT::StripEmbeddedUserinfo( aContext.url ) ) );

    return KIGIT::StripEmbeddedUserinfo( details );
}

} // namespace


KIGIT_FAILURE_REPORT KIGIT::DescribeGitFailure( const KIGIT_ERROR_INFO& aError, const KIGIT_AUTH_ATTEMPT& aAuth,
                                                const KIGIT_FAILURE_CONTEXT& aContext )
{
    KIGIT_FAILURE_REPORT report;
    report.kind = classifyGitFailure( aError, aAuth, aContext );
    report.summary = summaryFor( report.kind, aAuth, aContext );
    report.details = detailsFor( aError, aAuth, aContext );
    return report;
}


wxString KIGIT::UnexplainedFailure( KIGIT_OPERATION aOperation, const wxString& aUrl )
{
    KIGIT_FAILURE_CONTEXT context;
    context.operation = aOperation;
    context.url = aUrl;
    return summaryFor( KIGIT_FAILURE::OTHER, {}, context );
}


bool KIGIT::IsCredentialFailure( KIGIT_FAILURE aKind )
{
    return aKind == KIGIT_FAILURE::AUTH_SSH_KEY_REJECTED || aKind == KIGIT_FAILURE::AUTH_NO_SSH_KEY
           || aKind == KIGIT_FAILURE::AUTH_HTTPS_REJECTED || aKind == KIGIT_FAILURE::KEY_LOAD_FAILED;
}


wxString KIGIT::DecodeServerText( const char* aData, size_t aLength )
{
    wxString text = wxString::FromUTF8( aData, aLength );

    // An empty result would drop the line, and with it the scheme the scrub looks for
    if( text.IsEmpty() && aLength > 0 )
        text = wxString::From8BitData( aData, aLength );

    return text;
}


wxString KIGIT::StripEmbeddedUserinfo( const wxString& aText )
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


wxString KIGIT::PrintableServerText( const wxString& aText )
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


/// aDir spelled as libgit2 1.9's p_realpath spells the working folder it checks safe.directory against
static wxFileName resolveLikeLibgit2( const wxString& aDir )
{
#ifdef __WINDOWS__
    // win32 p_realpath is GetFullPathNameW then GetLongPathNameW, so links and junctions stay as written
    wxFileName dir = wxFileName::DirName( aDir );
    dir.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE | wxPATH_NORM_LONG );
    return dir;
#else
    std::unique_ptr<char, decltype( &std::free )> resolved( realpath( aDir.utf8_str(), nullptr ), &std::free );
    return wxFileName::DirName( resolved ? wxString::FromUTF8( resolved.get() ) : aDir );
#endif
}


/// aPath quoted for the shell the trust command is pasted into, or empty when it cannot be done safely
static wxString quoteForShell( const wxString& aPath )
{
#ifdef __WINDOWS__
    // cmd and PowerShell each expand or end a quote on different characters, so only known-inert ones pass
    static const wxString inert = wxS( " -_.,:/\\'()[]{}+=#@~" );

    for( wxUniChar c : aPath )
    {
        if( !wxIsalnum( c ) && inert.Find( c ) == wxNOT_FOUND )
            return wxEmptyString;
    }

    return wxS( "\"" ) + aPath + wxS( "\"" );
#else
    // Single quotes stop sh expanding $, `, " and \ in a folder name
    wxString quoted = aPath;
    quoted.Replace( wxS( "'" ), wxS( "'\\''" ) );
    return wxS( "'" ) + quoted + wxS( "'" );
#endif
}


/// True when libgit2's git_fs_path_root takes aPath as rooted rather than joining it onto a base folder
static bool isRootedLikeLibgit2( const wxString& aPath )
{
    // libgit2 skips a drive prefix on every platform, not only Windows
    const size_t offset = aPath.length() > 1 && aPath[1] == ':' ? 2 : 0;

    if( offset >= aPath.length() )
        return false;

#ifdef __WINDOWS__
    return aPath[offset] == '/' || aPath[offset] == '\\';
#else
    return aPath[offset] == '/';
#endif
}


KIGIT_OWNERSHIP_REFUSAL KIGIT::DescribeOwnershipRefusal( const wxString& aStartDir, const wxString& aGitDir )
{
    const wxFileName gitDir = wxFileName::DirName( aGitDir );
    wxFileName       trusted = gitDir;
    wxFileName       dir = resolveLikeLibgit2( aStartDir );

    // Reaching the git directory first means a bare repository, which is trusted by its own path
    while( !dir.SameAs( gitDir ) )
    {
        if( wxFileName::Exists( dir.GetPathWithSep() + wxS( ".git" ) ) )
        {
            trusted = dir;
            break;
        }

        if( dir.GetDirCount() == 0 )
            break;

        dir.RemoveLastDir();
    }

    // core.worktree moves the working folder git checks, except in a bare repository or linked worktree
    git_config* rawConfig = nullptr;

    if( !wxFileName::FileExists( gitDir.GetPathWithSep() + wxS( "commondir" ) )
        && git_config_open_ondisk( &rawConfig, ( gitDir.GetPathWithSep() + wxS( "config" ) ).utf8_str() ) == GIT_OK )
    {
        KIGIT::GitConfigPtr config( rawConfig );
        int                 error = GIT_OK;
        int                 bare = 0;
        git_buf             worktree = GIT_BUF_INIT;

#if KIGIT_HAS_WORKTREE_CONFIG
        int worktreeConfig = 0;

        // With the extension on, libgit2 lets config.worktree override config
        if( git_config_get_bool( &worktreeConfig, config.get(), "extensions.worktreeConfig" ) == GIT_OK
            && worktreeConfig )
        {
            error = git_config_add_file_ondisk( config.get(),
                                                ( gitDir.GetPathWithSep() + wxS( "config.worktree" ) ).utf8_str(),
                                                GIT_CONFIG_LEVEL_WORKTREE, nullptr, 0 );
        }
#endif

        if( ( error == GIT_OK || error == GIT_ENOTFOUND )
            && ( git_config_get_bool( &bare, config.get(), "core.bare" ) != GIT_OK || !bare )
            && git_config_get_string_buf( &worktree, config.get(), "core.worktree" ) == GIT_OK )
        {
            KIGIT::GitBufPtr worktreePtr( &worktree );
            wxString         configured = wxString::FromUTF8( worktree.ptr );

            // libgit2 joins a relative value onto the git directory and leaves .. and ~ to realpath
            if( !isRootedLikeLibgit2( configured ) )
                configured = gitDir.GetPathWithSep() + configured;

            trusted = resolveLikeLibgit2( configured );
        }
    }

    wxString path = trusted.GetPath();

#ifdef __WINDOWS__
    // git compares safe.directory against a forward-slash path, on Windows too
    path.Replace( wxS( "\\" ), wxS( "/" ) );
#endif

    // git reads a trailing /* as every repository below the folder, so trusting this one would trust them all
    if( path.EndsWith( wxS( "/*" ) ) )
    {
        return { wxString::Format( _( "The repository at '%s' belongs to another user, so git will not open it. "
                                      "git reads a folder named '*' as a wildcard, so rename the folder before "
                                      "trusting it." ),
                                   trusted.GetPath() ),
                 wxEmptyString };
    }

    const wxString quoted = quoteForShell( path );

    if( quoted.IsEmpty() )
    {
        return { wxString::Format( _( "The repository at '%s' belongs to another user, so git will not open it. To "
                                      "trust it, add its folder to git's safe.directory setting and reopen the "
                                      "project." ),
                                   trusted.GetPath() ),
                 wxEmptyString };
    }

    return { wxString::Format( _( "The repository at '%s' belongs to another user, so git will not open it. To "
                                  "trust it, run the command below and reopen the project." ),
                               trusted.GetPath() ),
             wxS( "git config --global --add safe.directory " ) + quoted };
}
