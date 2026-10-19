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

#include <git/git_config_handler.h>
#include <settings/common_settings.h>

#include <memory>
#include <wx/string.h>

class EDA_BASE_FRAME;
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

/// Repository values and whether Git can use them to record a commit.
struct GIT_IDENTITY
{
    GitUserConfig config;
    bool          configured = false;
};

/// Read on a worker while holding the session's action lock.
GIT_IDENTITY ReadGitIdentity( KIGIT_COMMON& aCommon );

/// Fill missing values from custom preferences, then use the OS user name if needed.
GitUserConfig ResolveGitIdentity( const GIT_IDENTITY& aIdentity, const COMMON_SETTINGS::GIT& aPreferences,
                                  const wxString& aUserName );

/// Edit the repository's author and refresh Git views after saving. UI thread only.
/// Refuse to save if the frame changes project while the dialog is open.
bool EditGitIdentity( wxWindow* aParent, EDA_BASE_FRAME* aFrame, const std::shared_ptr<KIGIT_COMMON>& aCommon,
                      const GIT_IDENTITY& aIdentity );

/// Open the same editor when the worker snapshot has no usable Git identity.
bool EnsureGitIdentity( wxWindow* aParent, EDA_BASE_FRAME* aFrame, const std::shared_ptr<KIGIT_COMMON>& aCommon,
                        const GIT_IDENTITY& aIdentity );

} // namespace KIGIT

#endif // GIT_CREDENTIALS_PROMPT_H
