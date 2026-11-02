/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
*
* This program is free software: you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation, either version 3 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef KIPLATFORM_SECRETS_H_
#define KIPLATFORM_SECRETS_H_

#include <functional>

#include <wx/string.h>

namespace KIPLATFORM
{
    namespace SECRETS
    {

        bool StoreSecret( const wxString& aService, const wxString& aKey, const wxString& aSecret );

        /**
         * Like StoreSecret(), but never blocks the event loop while the platform asks the user to unlock
         * the store. Call it from the main thread; aDone, when given, runs there with the outcome, possibly
         * before this returns on platforms that store without prompting.
         */
        void StoreSecretAsync( const wxString& aService, const wxString& aKey, const wxString& aSecret,
                               std::function<void( bool )> aDone = nullptr );

        bool GetSecret( const wxString& aService, const wxString& aKey, wxString& aSecret );

        /**
         * Like GetSecret(), but never blocks the event loop while the platform asks the user to unlock
         * the store. Call it from the main thread; aDone runs there with whether the secret was found and
         * its value, possibly before this returns on platforms that read without prompting.
         */
        void GetSecretAsync( const wxString& aService, const wxString& aKey,
                             std::function<void( bool, const wxString& )> aDone );

        /**
         * Like GetSecret(), but never waits on the user. A locked keyring is reported as not found
         * instead of showing an unlock prompt, so it is safe on paths that must not block.
         */
        bool GetSecretNoPrompt( const wxString& aService, const wxString& aKey, wxString& aSecret );

        bool DeleteSecret( const wxString& aService, const wxString& aKey );

    }
}

#endif // KIPLATFORM_SECRETS_H_
