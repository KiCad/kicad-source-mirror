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

#ifndef KIGIT_REMOTE_URL_H
#define KIGIT_REMOTE_URL_H

#include <import_export.h>

#include <wx/string.h>

namespace KIGIT
{

enum class GIT_CONN_TYPE
{
    GIT_CONN_HTTPS = 0,
    GIT_CONN_SSH,
    GIT_CONN_LOCAL,
    GIT_CONN_LAST
};

/// A transport address with any embedded credentials extracted.
struct REMOTE_URL
{
    wxString      url; ///< Keeps the SSH username, scheme and port, but never a password.
    wxString      username;
    wxString      password;
    wxString      hostname;
    int           port = 0; ///< 0 when the address names none.
    GIT_CONN_TYPE connType = GIT_CONN_TYPE::GIT_CONN_LOCAL;
};

/// Split a remote address the way libgit2 reads it, so a host key is checked against the host it dials
APIEXPORT REMOTE_URL ParseRemoteURL( const wxString& aURL );

/**
 * The folder aUrl names when it is a drive, UNC or absolute path or a file:// URL, else empty. A relative path is
 * left to libgit2, since only it knows the folder that path is relative to.
 */
APIEXPORT wxString LocalRemotePath( const wxString& aUrl );

/// The host and port aUrl reaches, as `host[:port]` without the scheme's default port. Empty for a local path
/// or an address naming no usable host.
APIEXPORT wxString CredentialServer( const wxString& aUrl );

/**
 * The server an HTTP(S) credential for aUrl serves, as `scheme://host[:port]` with no user, path or default port,
 * the key git's own credential helpers use by default. Empty for any other address, since an SSH secret is the
 * passphrase of a key rather than a password for the server.
 */
APIEXPORT wxString CredentialRealm( const wxString& aUrl );

/// Remove the user and password from every URL quoted in a message, since the remote's can carry a password.
APIEXPORT wxString StripEmbeddedUserinfo( const wxString& aText );

/// Bytes a server sent, as UTF-8 or, when they are not valid UTF-8, one character for each byte.
APIEXPORT wxString DecodeServerText( const char* aData, size_t aLength );

/**
 * Text a server sent, made safe to show by removing terminal escapes, control and bidi characters, and URL
 * userinfo. Line breaks are kept.
 */
APIEXPORT wxString PrintableServerText( const wxString& aText );

} // namespace KIGIT

#endif // KIGIT_REMOTE_URL_H
