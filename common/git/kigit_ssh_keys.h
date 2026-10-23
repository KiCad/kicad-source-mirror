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
 * Private keys to offer @p aHost, best first.
 *
 * The ssh config's IdentityFile entries for the host come first, then id_ed25519, id_ecdsa,
 * id_rsa and id_dsa from @p aHomeDir/.ssh. Only keys whose private and public files both
 * exist are listed. An empty @p aHomeDir gives no keys.
 */
APIEXPORT std::vector<wxString> DefaultSSHKeyCandidates( const wxString& aHomeDir, const wxString& aHost );

/**
 * Whether two key paths name the same file. Separators are normalised, and case is ignored where
 * the file system ignores it.
 */
APIEXPORT bool IsSameSSHKey( const wxString& aKey, const wxString& aOther );

/**
 * The user's home folder, where ssh looks for its config and default keys. Empty when unknown.
 * Safe to call from any thread.
 */
APIEXPORT wxString HomeDir();

/**
 * Whether @p aKey can be offered to a server. libgit2 opens both files only once the
 * connection is under way, and a missing one ends the attempt instead of trying the next key.
 */
APIEXPORT bool IsOfferableSSHKey( const wxString& aKey );

} // namespace KIGIT

#endif // KIGIT_SSH_KEYS_H_
