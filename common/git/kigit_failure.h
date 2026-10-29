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

#ifndef KIGIT_FAILURE_H_
#define KIGIT_FAILURE_H_

#include <git/kicad_git_common.h>
#include <git/kigit_known_hosts.h>
#include <import_export.h>

#include <optional>
#include <vector>

#include <wx/string.h>


/// What the credential callback handed out during one operation, kept to explain a rejection.
struct KIGIT_AUTH_ATTEMPT
{
    KIGIT_COMMON::GIT_CONN_TYPE connType = KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_LOCAL;
    std::vector<wxString>       keysTried;          ///< Key files offered, in order.
    bool                        agentTried = false; ///< The SSH agent was offered before any key file.
    bool                        passphraseGiven = false; ///< A passphrase went with the last key file offered.
};


/// Why a transfer failed, in terms of what the user can do about it.
enum class KIGIT_FAILURE
{
    AUTH_SSH_KEY_REJECTED,
    AUTH_NO_SSH_KEY,
    AUTH_HTTPS_REJECTED,
    KEY_LOAD_FAILED,
    HOST_KEY_UNKNOWN,
    HOST_KEY_CHANGED,
    HOST_KEY_UNVERIFIED, ///< A key KiCad could not check, so there is nothing to offer to trust.
    KNOWN_HOSTS_UNREADABLE,
    SERVER_NOT_FOUND,
    HOST_UNREACHABLE,
    TLS_UNTRUSTED,
    PROXY,
    REPO_NOT_FOUND,
    SERVER_REFUSED, ///< The server answered with text alone. Its wording is shown but never interpreted.
    PUSH_FORBIDDEN,
    PUSH_NOT_FAST_FORWARD,
    CANCELLED,
    OTHER
};


/// The transfer that failed, so a message nothing more specific explains can say what did not happen.
enum class KIGIT_OPERATION
{
    CONNECT,
    CLONE,
    FETCH,
    PUSH,
    SET_UP_REMOTE
};


/// What the failed operation was doing, as far as the message needs it.
struct KIGIT_FAILURE_CONTEXT
{
    KIGIT_OPERATION operation = KIGIT_OPERATION::CONNECT;
    wxString        remoteName; ///< Empty when the operation has no named remote, as for a clone.
    wxString        url;        ///< The address connected to. Messages show only its host and its stripped form.
    bool            authFailed = false; ///< The credential callback had nothing left to offer.

    std::optional<KIGIT::HOST_KEY_FAILURE> hostKey;        ///< The host key the operation refused, if any.
    wxString                               hostKeyRefusal; ///< Why a key that cannot be trusted was refused.
    wxString                               pushRefusal;    ///< The status the server gave a refused branch.
    wxString                               serverMessage;  ///< Text the server sent for the user.
};


struct KIGIT_FAILURE_REPORT
{
    KIGIT_FAILURE kind = KIGIT_FAILURE::OTHER;
    wxString      summary; ///< What happened and what to do next, in the user's terms. Never raw text.
    wxString      details; ///< The raw error, the server's text and the address without its password.
};


/// Why git will not open a repository another user owns, and the command that trusts it.
struct KIGIT_OWNERSHIP_REFUSAL
{
    wxString summary;
    wxString trustCommand; ///< Kept apart from the summary so it can be shown in full and copied. Empty when
                           ///< no safe command exists for the folder, such as a name the shell could act on or
                           ///< one git reads as a wildcard.
};


namespace KIGIT
{

/**
 * Turn a failed transfer into a message the user can act on.
 *
 * The libgit2 code decides first, then its error class, then what the credential callback offered,
 * and last the resolver and connection texts libgit2 and libssh2 write themselves. Those are never translated.
 * Text a server sends is shown to the user but never decides the kind, so only its codes are relied on.
 */
APIEXPORT KIGIT_FAILURE_REPORT DescribeGitFailure( const KIGIT_ERROR_INFO& aError, const KIGIT_AUTH_ATTEMPT& aAuth,
                                                   const KIGIT_FAILURE_CONTEXT& aContext );

/// The summary for an operation that failed without saying why, naming the server it was talking to.
APIEXPORT wxString UnexplainedFailure( KIGIT_OPERATION aOperation, const wxString& aUrl );

/// True for the failures another login, key or passphrase can fix, which the credential prompt answers.
APIEXPORT bool IsCredentialFailure( KIGIT_FAILURE aKind );

/// The longest server text a failure keeps. A refusal's explanation comes last, after any transfer progress.
constexpr size_t MAX_SERVER_TEXT = 4096;

/// Bytes a server sent, as UTF-8 or, when they are not valid UTF-8, one character for each byte.
APIEXPORT wxString DecodeServerText( const char* aData, size_t aLength );

/// Remove the user and password from every URL quoted in a message, since the remote's can carry a password.
APIEXPORT wxString StripEmbeddedUserinfo( const wxString& aText );

/**
 * Text a server sent, made safe to show: no terminal escapes, control or bidi characters, and no URL userinfo.
 * Line breaks are kept.
 */
APIEXPORT wxString PrintableServerText( const wxString& aText );

/**
 * The message for a repository git refuses to open because another user owns it.
 *
 * git checks the working folder, so that is the one named: the nearest folder at or above aStartDir that holds
 * a .git file or folder, spelled as libgit2 resolves it. A bare repository has none and keeps aGitDir, the git
 * directory discovery found.
 */
APIEXPORT KIGIT_OWNERSHIP_REFUSAL DescribeOwnershipRefusal( const wxString& aStartDir, const wxString& aGitDir );

} // namespace KIGIT

#endif // KIGIT_FAILURE_H_
