/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#import <Security/Security.h>

bool KIPLATFORM::SECRETS::StoreSecret( const wxString& aService, const wxString& aKey, const wxString& aValue )
{
    SecKeychainItemRef itemRef = NULL;

    OSStatus status = SecKeychainFindGenericPassword( NULL, aService.length(), aService.utf8_str(),
                                                      aKey.length(), aKey.utf8_str(),
                                                      NULL, NULL, &itemRef );

    if( status == errSecItemNotFound )
    {
        status = SecKeychainAddGenericPassword( NULL, aService.length(), aService.utf8_str(),
                                                aKey.length(), aKey.utf8_str(),
                                                aValue.length(), aValue.utf8_str(),
                                                NULL );

        CFRelease( itemRef );
    }
    else if( status == errSecSuccess )
    {
        status = SecKeychainItemModifyAttributesAndData( itemRef, NULL, aValue.length(), aValue.utf8_str() );
    }


    return status == errSecSuccess;
}

bool KIPLATFORM::SECRETS::GetSecret( const wxString& aService, const wxString& aKey, wxString& aValue )
{
    SecKeychainItemRef itemRef = NULL;

    OSStatus status = SecKeychainFindGenericPassword( NULL, aService.length(), aService.utf8_str(),
                                                      aKey.length(), aKey.utf8_str(),
                                                      NULL, NULL, &itemRef );

    if( status == errSecSuccess )
    {
        UInt32 length;
        char* data;

        status = SecKeychainItemCopyAttributesAndData( itemRef, NULL, NULL, NULL, &length, (void**)&data );

        if( status == errSecSuccess )
        {
            aValue = wxString::FromUTF8( data, length );
            SecKeychainItemFreeAttributesAndData( NULL, data );
        }

        CFRelease( itemRef );
    }

    return status == errSecSuccess;
}