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

#ifndef KIGIT_PLATFORM_H_
#define KIGIT_PLATFORM_H_

#include <functional>

#include <import_export.h>
#include <settings/common_settings.h>
#include <wx/string.h>

namespace KIGIT
{

/**
 * What the git core asks of the application around it. kicommon fills these in when it loads. A process
 * without kicommon leaves them empty, so it reads no stored secret, keeps no SSH key memory and removes no folder.
 */
struct PLATFORM_SERVICES
{
    /// Read a stored secret without prompting to unlock the store
    std::function<bool( const wxString& aService, const wxString& aKey, wxString& aSecret )> getSecretNoPrompt;

    /// Read a stored secret, prompting to unlock the store if it must, and report it through aDone
    std::function<void( const wxString& aService, const wxString& aKey,
                        std::function<void( bool aFound, const wxString& aSecret )> aDone )>
            getSecretAsync;

    /// Store a secret, false when the store refused it. It may wait on an unlock prompt, so never on the UI thread
    std::function<bool( const wxString& aService, const wxString& aKey, const wxString& aSecret )> storeSecret;

    /// As storeSecret, but called on the UI thread, which an unlock prompt never holds. aDone runs there
    std::function<void( const wxString& aService, const wxString& aKey, const wxString& aSecret,
                        std::function<void( bool aStored )> aDone )>
            storeSecretAsync;

    /// Delete a stored secret, false when none was removed
    std::function<bool( const wxString& aService, const wxString& aKey )> deleteSecret;

    /// Drop what this run remembers for a realm whose password was just saved elsewhere
    std::function<void( const wxString& aRealm )> forgetRealm;

    /// The username saved for a credential realm, or empty
    std::function<wxString( const wxString& aRealm )> getAccount;

    /// Save the username for a credential realm, from any thread
    std::function<void( const wxString& aRealm, const wxString& aUsername )> setAccount;

    /// The saved author preference
    std::function<COMMON_SETTINGS::GIT()> getAuthorPreference;

    /// The folder that holds the user's cached files
    std::function<wxString()> userCachePath;

    /// Remove a folder and everything in it. aErrors receives why it could not
    std::function<bool( const wxString& aPath, wxString* aErrors )> removeDirectory;
};

APIEXPORT PLATFORM_SERVICES& PlatformServices();

/**
 * Save a password for aUsername under aUrl's credential realm, where every remote and workspace on that server
 * finds it, retiring aUrl's own item, and remember aUsername for the realm. An SSH address keeps its own item.
 * Callable from any thread. A locked keyring prompts without holding the event loop, and aDone runs on the main
 * thread with whether the keyring took it.
 */
APIEXPORT void SaveCredentialAsync( const wxString& aUrl, const wxString& aUsername, const wxString& aPassword,
                                    std::function<void( bool aStored )> aDone = nullptr );

} // namespace KIGIT

#endif // KIGIT_PLATFORM_H_
