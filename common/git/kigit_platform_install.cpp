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

#include <git/kigit_platform.h>

#include <gestfich.h>
#include <kiplatform/secrets.h>
#include <mdw/mdw_framework.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/common_settings.h>

#include <mutex>

#include <wx/app.h>
#include <wx/thread.h>


namespace
{

COMMON_SETTINGS* commonSettings()
{
    return PgmOrNull() ? Pgm().GetCommonSettings() : nullptr;
}


// kicommon is a shared library, so this runs before any program that links it touches git
[[maybe_unused]] const bool installed = []()
{
    KIGIT::PLATFORM_SERVICES& services = KIGIT::PlatformServices();

    services.getSecretNoPrompt = &KIPLATFORM::SECRETS::GetSecretNoPrompt;
    services.getSecretAsync = &KIPLATFORM::SECRETS::GetSecretAsync;
    services.storeSecret = &KIPLATFORM::SECRETS::StoreSecret;
    services.storeSecretAsync = &KIPLATFORM::SECRETS::StoreSecretAsync;
    services.deleteSecret = &KIPLATFORM::SECRETS::DeleteSecret;
    services.forgetRealm = []( const wxString& aRealm )
    {
        PGM_BASE* pgm = PgmOrNull();

        if( MDW_FRAMEWORK_LEASE lease = pgm ? pgm->LeaseMdwFramework( false ) : MDW_FRAMEWORK_LEASE() )
            lease->Credentials().Forget( aRealm );
    };
    services.getAccount = []( const wxString& aRealm )
    {
        COMMON_SETTINGS*            settings = commonSettings();
        std::lock_guard<std::mutex> lock( COMMON_SETTINGS::AccountsMutex() );

        if( !settings || !settings->m_Workspace.accounts.contains( aRealm ) )
            return wxString();

        return settings->m_Workspace.accounts.at( aRealm );
    };
    services.setAccount = []( const wxString& aRealm, const wxString& aUsername )
    {
        auto set = [aRealm, aUsername]()
        {
            std::lock_guard<std::mutex> lock( COMMON_SETTINGS::AccountsMutex() );

            if( COMMON_SETTINGS* settings = commonSettings() )
                settings->m_Workspace.accounts[aRealm] = aUsername;
        };

        // Saving the settings reads the map on the UI thread without the lock
        if( wxTheApp && !wxIsMainThread() )
            wxTheApp->CallAfter( set );
        else
            set();
    };
    services.getAuthorPreference = []()
    {
        COMMON_SETTINGS::GIT prefs{};
        prefs.useDefaultAuthor = true;

        COMMON_SETTINGS*            settings = commonSettings();
        std::lock_guard<std::mutex> lock( COMMON_SETTINGS::AccountsMutex() );

        // The repository list is rewritten by a settings load without this lock, so only the author is copied
        if( settings )
        {
            prefs.useDefaultAuthor = settings->m_Git.useDefaultAuthor;
            prefs.authorName = settings->m_Git.authorName;
            prefs.authorEmail = settings->m_Git.authorEmail;
        }

        return prefs;
    };
    services.userCachePath = &PATHS::GetUserCachePath;
    services.removeDirectory = &RmDirRecursive;

    return true;
}();

} // namespace
