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

#include <wx/string.h>

namespace KIPLATFORM
{
    namespace SECRETS
    {

        bool StoreSecret( const wxString& aService, const wxString& aKey, const wxString& aSecret );

        bool GetSecret( const wxString& aService, const wxString& aKey, wxString& aSecret );

        /**
         * Like GetSecret(), but never waits on the user. A locked keyring is reported as not found
         * instead of showing an unlock prompt, so it is safe on paths that must not block.
         */
        bool GetSecretNoPrompt( const wxString& aService, const wxString& aKey, wxString& aSecret );

        bool DeleteSecret( const wxString& aService, const wxString& aKey );

    }
}

#endif // KIPLATFORM_SECRETS_H_
