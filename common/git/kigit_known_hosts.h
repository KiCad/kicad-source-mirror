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

#ifndef KIGIT_KNOWN_HOSTS_H_
#define KIGIT_KNOWN_HOSTS_H_

#include <import_export.h>

#include <array>
#include <cstdint>
#include <string>

#include <wx/string.h>

namespace KIGIT
{

/// How a server's host key stands against known_hosts.
enum class KNOWN_HOST_STATE
{
    UNKNOWN,        ///< No entry names the host with a key of this type.
    MATCH,          ///< An entry names the host with this key.
    MISMATCH,       ///< Entries name the host with another key of this type, and none with this one.
    OTHER_KEY_TYPE, ///< Entries name the host only with keys of other types.
    REVOKED         ///< A @revoked entry carries this key.
};


/// A server key the connection refused, for the UI to offer to trust or to report.
struct HOST_KEY_FAILURE
{
    KNOWN_HOST_STATE state = KNOWN_HOST_STATE::UNKNOWN;
    wxString         host;
    int              port = 22;
    wxString         keyType;     ///< As known_hosts names it, such as ssh-ed25519.
    std::string      rawKey;      ///< The key as the server sent it.
    wxString         fingerprint; ///< SHA256 in unpadded base64, as ssh shows it after "SHA256:".
};


/// HMAC-SHA1 (RFC 2104), as OpenSSH hashes host names in known_hosts.
APIEXPORT std::array<uint8_t, 20> HmacSha1( const std::string& aKey, const std::string& aData );

/**
 * Classify a server's key against the text of a known_hosts file, as OpenSSH reads it.
 *
 * Entries name the host as `[host]:port`, or as `host` on port 22, either plainly, with `*`, `?` and
 * `!` patterns, or hashed. A @revoked `host` entry applies on every port, because libssh2 also accepts
 * a plain `host` entry for every port. @cert-authority entries are ignored.
 *
 * @param aKeyType The key's type as known_hosts names it.
 * @param aRawKey The key as the server sent it.
 */
APIEXPORT KNOWN_HOST_STATE LookupKnownHost( const wxString& aKnownHostsText, const wxString& aHost, int aPort,
                                            const wxString& aKeyType, const std::string& aRawKey );

/// The known_hosts line ssh would write for a key, without a line ending.
APIEXPORT wxString FormatKnownHostsLine( const wxString& aHost, int aPort, const wxString& aKeyType,
                                         const wxString& aBase64Key );

/// The type a raw SSH key names for itself, such as ssh-ed25519. Empty when the key is malformed.
APIEXPORT wxString HostKeyType( const std::string& aRawKey );

/// SHA256 of a raw key in unpadded base64, as ssh shows it after "SHA256:".
APIEXPORT wxString HostKeyFingerprint( const std::string& aRawKey );

/// The known_hosts file libgit2 checks host keys against. Empty when it has no home folder.
APIEXPORT wxString KnownHostsPath();

/**
 * Whether the user may be asked to trust a refused key. A changed or revoked key never may, and an
 * operation already retried after a trust reports a second refusal instead of asking again.
 */
APIEXPORT bool MayOfferTrust( const HOST_KEY_FAILURE& aFailure, bool aRetriedAfterTrust );

/// Why a key the user may not trust was refused, and what to do next.
APIEXPORT wxString HostKeyRefusalMessage( const HOST_KEY_FAILURE& aFailure );

/**
 * The home folder libgit2 1.5 reads `.ssh/known_hosts` under, which it has no option to report.
 * 1.5.1 reads HOME at each connection and 1.5.2 takes the folder 1.6 would. 1.5.0 reads no trust store,
 * so KiCad keeps its own where 1.5.2 would look.
 *
 * @param aRevision The patch level of the libgit2 1.5 in use.
 */
APIEXPORT wxString Libgit2LegacyHome( int aRevision );

/// The text of KnownHostsPath(). Empty when it is missing or cannot be read.
APIEXPORT wxString ReadKnownHosts();

/**
 * Append the refused key to KnownHostsPath(), creating the file and its folder when missing.
 * Existing entries are never rewritten. Returns false when the line could not be written.
 */
APIEXPORT bool AddKnownHost( const HOST_KEY_FAILURE& aKey );

} // namespace KIGIT

#endif // KIGIT_KNOWN_HOSTS_H_
