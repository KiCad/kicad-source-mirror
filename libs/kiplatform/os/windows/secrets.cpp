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
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <kiplatform/secrets.h>

#include <windows.h>
#include <wincred.h>
#include <wx/string.h>

namespace KIPLATFORM
{
    namespace SECRETS
    {
        bool StoreSecret( const wxString& aService, const wxString& aKey, const wxString& aSecret )
        {
            wxString display = aService + wxS( ":" ) + aKey;

            // Store the UTF-8 string in a variable to keep it alive during the API call
            wxScopedCharBuffer utf8Secret = aSecret.utf8_str();

            CREDENTIALW cred = { 0 };
            cred.Type = CRED_TYPE_GENERIC;
            cred.TargetName = (LPWSTR) display.wc_str();
            cred.CredentialBlobSize = (DWORD) utf8Secret.length();
            cred.CredentialBlob = (LPBYTE) utf8Secret.data();
            cred.Persist = CRED_PERSIST_ENTERPRISE;

            return CredWriteW( &cred, 0 );
        }

        bool GetSecret( const wxString& aService, const wxString& aKey, wxString& aSecret )
        {
            wxString display = aService + wxS( ":" ) + aKey;

            CREDENTIALW* cred = nullptr;
            bool result = CredReadW( display.wc_str(), CRED_TYPE_GENERIC, 0, &cred );

            if( result )
            {
                aSecret = wxString::FromUTF8( (const char*) cred->CredentialBlob,
                                              cred->CredentialBlobSize );
                CredFree( cred );
            }

            return result;
        }
    }
}