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

            secret_password_store_sync( &schema,
                                        SECRET_COLLECTION_DEFAULT,
                                        display.mb_str(),   // Display name
                                        aSecret.mb_str(),   // Secret value
                                        nullptr,
                                        &error,
                                        "service", aService.ToStdString().c_str(),
                                        "key", aKey.ToStdString().c_str(),
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
            gchar* secret = secret_password_lookup_sync( &schema,
                                                         nullptr,
                                                         &error,
                                                         "service", aService.ToStdString().c_str(),
                                                         "key", aKey.ToStdString().c_str(),
                                                         nullptr );

            if( error )
            {
                g_error_free( error );
                return false;
            }

            aSecret = secret;
            g_free( secret );

            return true;
        }
    }
}