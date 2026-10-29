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

#ifndef KIGIT_SSH_KEYS_H_
#define KIGIT_SSH_KEYS_H_

#include <import_export.h>

#include <string>
#include <vector>

#include <wx/string.h>

namespace KIGIT
{

/**
 * The IdentityFile entries an ssh config gives @p aHost, in file order.
 *
 * Host patterns match @p aHost case-sensitively, as typed in the remote address, without
 * resolving HostName. Match blocks are skipped. A leading `~` and every `%d` expand to
 * @p aHomeDir; a value holding any other `%` token is returned unexpanded.
 */
APIEXPORT std::vector<wxString> ParseSSHConfigIdentityFiles( const wxString& aConfigText, const wxString& aHost,
                                                             const wxString& aHomeDir );

/**
 * The `-i` key files in an ssh command line such as git's core.sshCommand, in order. Words split
 * and unquote as sh does it, and a leading `~` expands to @p aHomeDir.
 */
APIEXPORT std::vector<wxString> ParseSSHCommandIdentityFiles( const wxString& aCommand, const wxString& aHomeDir );

/**
 * Private keys to offer @p aHost, best first.
 *
 * The `-i` keys of @p aSSHCommand come first, then the ssh config's IdentityFile entries for the
 * host, then id_ed25519, id_ecdsa, id_rsa and id_dsa from @p aHomeDir/.ssh, then a few other keys
 * found in that folder with a .pub beside them. Only keys that can be offered are listed. An empty
 * @p aHomeDir gives no keys.
 */
APIEXPORT std::vector<wxString> DefaultSSHKeyCandidates( const wxString& aHomeDir, const wxString& aHost,
                                                         const wxString& aSSHCommand = wxEmptyString );

/**
 * Whether two key paths name the same file. Separators are normalised, and case is ignored where
 * the file system ignores it.
 */
APIEXPORT bool IsSameSSHKey( const wxString& aKey, const wxString& aOther );

/**
 * The folders that may be the user's home, most likely first. On Windows wxGetHomeDir, HOMEDRIVE with
 * HOMEPATH, and USERPROFILE can each name a different folder. Safe to call from any thread.
 */
APIEXPORT std::vector<wxString> HomeDirCandidates();

/**
 * The user's home folder, where ssh looks for its config and default keys: the first candidate with a
 * .ssh folder, else the first candidate. Empty when unknown. Safe to call from any thread.
 */
APIEXPORT wxString HomeDir();

/// The folder ssh keeps keys in, for a file picker to open at. Empty when the home folder is unknown.
APIEXPORT wxString SSHKeyFolder();

/**
 * Whether @p aKey can be offered to a server. libgit2 opens both files only once the
 * connection is under way, and a missing one ends the attempt instead of trying the next key.
 * Writes the public key file EnsurePublicKeyFile would.
 */
APIEXPORT bool IsOfferableSSHKey( const wxString& aKey );

/**
 * The public key file to offer with @p aKey. That is the .pub beside it, else one written by
 * EnsurePublicKeyFile, else empty.
 */
APIEXPORT wxString PublicKeyPath( const wxString& aKey );

/**
 * PublicKeyPath for @p aKey, first writing a public key file to KiCad's cache when the key has no
 * .pub beside it. An OpenSSH key keeps its public half unencrypted, so this needs no passphrase.
 */
APIEXPORT wxString EnsurePublicKeyFile( const wxString& aKey );

/**
 * Keep public key files written by EnsurePublicKeyFile in @p aDir. Until this is called none are
 * written. Finding the user's cache folder is not safe off the main thread, so the caller resolves it.
 */
APIEXPORT void SetDerivedPublicKeyFolder( const wxString& aDir );

/**
 * The private key for a path picked in a file dialog, which may be the key's .pub file.
 */
APIEXPORT wxString PrivateKeyPath( const wxString& aPicked );

/// The public key blob from @p aKey's public key file, as the server sees it, or empty when it cannot be read.
APIEXPORT std::string PublicKeyBlob( const wxString& aKey );

/**
 * The key that last connected to @p aRemote, or empty. Kept across sessions, so the next
 * connection offers it before keys the server refuses. Safe to call from any thread.
 */
APIEXPORT wxString RememberedSSHKey( const wxString& aRemote );

/// Remember @p aKey as the key that connected to @p aRemote. Safe to call from any thread.
APIEXPORT void RememberSSHKey( const wxString& aRemote, const wxString& aKey );

/**
 * Keep remembered keys in @p aPath. Until this is called they last for the session only. Finding the
 * user's cache folder is not safe off the main thread, so the caller resolves it.
 */
APIEXPORT void SetRememberedSSHKeyStore( const wxString& aPath );

} // namespace KIGIT

#endif // KIGIT_SSH_KEYS_H_
