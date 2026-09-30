/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GIT_CREDENTIALS_PROMPT_H
#define GIT_CREDENTIALS_PROMPT_H

class wxWindow;
class KIGIT_COMMON;
class PROJECT;

namespace KIGIT
{

/**
 * Ask the user for the credentials of the session's remote and apply them to the session.
 * Optionally stores them in the OS keychain. Clears the auth-failure state so the caller
 * can retry the operation.
 *
 * @param aParent Parent window for the dialog.
 * @param aCommon Session to update with the entered credentials.
 * @param aProject When given, the corrected username and key are also written to the project's
 *                 stored settings, so a later repository attach does not revert them.
 * @return True when the user confirmed and credentials were applied.
 */
bool PromptForCredentials( wxWindow* aParent, KIGIT_COMMON* aCommon, PROJECT* aProject = nullptr );

/**
 * Make sure the repository has a name and email to record commits with. When missing, asks
 * once and stores the answer in the repository's configuration.
 *
 * @return False when an identity is still missing, because the user declined or it could not
 *         be saved.
 */
bool EnsureGitIdentity( wxWindow* aParent, KIGIT_COMMON* aCommon );

} // namespace KIGIT

#endif // GIT_CREDENTIALS_PROMPT_H
