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

#include <kiplatform/secrets.h>

#include <libsecret/secret.h>

namespace KIPLATFORM
{
    namespace SECRETS
    {
        static const SecretSchema schema =
        {
            "org.kicad.kicad", SECRET_SCHEMA_NONE,
            {
                { "service", SECRET_SCHEMA_ATTRIBUTE_STRING },
                { "key", SECRET_SCHEMA_ATTRIBUTE_STRING },
                { nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING }
            }
        };

        bool StoreSecret( const wxString& aService, const wxString& aKey, const wxString& aSecret )
        {
            GError* error = nullptr;
            wxString display = aService + ":" + aKey;

            // libsecret takes UTF-8, not the locale encoding
            wxScopedCharBuffer utf8Display = display.utf8_str();
            wxScopedCharBuffer utf8Secret = aSecret.utf8_str();
            wxScopedCharBuffer utf8Service = aService.utf8_str();
            wxScopedCharBuffer utf8Key = aKey.utf8_str();

            secret_password_store_sync( &schema,
                                        SECRET_COLLECTION_DEFAULT,
                                        utf8Display.data(), // Display name
                                        utf8Secret.data(),  // Secret value
                                        nullptr,
                                        &error,
                                        "service", utf8Service.data(),
                                        "key", utf8Key.data(),
                                        nullptr );

            if( error )
            {
                g_error_free( error );
                return false;
            }

            return true;
        }

        bool GetSecret( const wxString& aService, const wxString& aKey, wxString& aSecret )
        {
            GError* error = nullptr;
            wxScopedCharBuffer utf8Service = aService.utf8_str();
            wxScopedCharBuffer utf8Key = aKey.utf8_str();
            gchar* secret = secret_password_lookup_sync( &schema,
                                                         nullptr,
                                                         &error,
                                                         "service", utf8Service.data(),
                                                         "key", utf8Key.data(),
                                                         nullptr );

            if( error )
            {
                g_error_free( error );
                return false;
            }

            // libsecret reports a missing entry as a null secret with no error
            if( !secret )
                return false;

            aSecret = wxString::FromUTF8( secret );
            g_free( secret );

            return true;
        }

        bool DeleteSecret( const wxString& aService, const wxString& aKey )
        {
            GError* error = nullptr;
            wxScopedCharBuffer utf8Service = aService.utf8_str();
            wxScopedCharBuffer utf8Key = aKey.utf8_str();

            secret_password_clear_sync( &schema,
                                        nullptr,
                                        &error,
                                        "service", utf8Service.data(),
                                        "key", utf8Key.data(),
                                        nullptr );

            if( error )
            {
                g_error_free( error );
                return false;
            }

            return true;
        }
    }
}
