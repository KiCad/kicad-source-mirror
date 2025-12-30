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

#import <Security/Security.h>

bool KIPLATFORM::SECRETS::StoreSecret(const wxString& aService, const wxString& aKey, const wxString& aValue)
{
    // Store UTF-8 strings in variables to keep them alive during API calls
    wxScopedCharBuffer utf8Service = aService.utf8_str();
    wxScopedCharBuffer utf8Key = aKey.utf8_str();
    wxScopedCharBuffer utf8Value = aValue.utf8_str();

    // Create a query for the secret
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, CFStringCreateWithCString(NULL, utf8Service.data(), kCFStringEncodingUTF8));
    CFDictionarySetValue(query, kSecAttrAccount, CFStringCreateWithCString(NULL, utf8Key.data(), kCFStringEncodingUTF8));

    // Try to find the existing item
    OSStatus status = SecItemCopyMatching(query, NULL);

    if (status == errSecItemNotFound)
    {
        // Add the new secret to the keychain
        CFDictionarySetValue(query, kSecValueData, CFDataCreate(NULL, (const UInt8*)utf8Value.data(), utf8Value.length()));
        status = SecItemAdd(query, NULL);
    }
    else if (status == errSecSuccess)
    {
        // Update the existing secret in the keychain
        CFMutableDictionaryRef updateQuery = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(updateQuery, kSecValueData, CFDataCreate(NULL, (const UInt8*)utf8Value.data(), utf8Value.length()));
        status = SecItemUpdate(query, updateQuery);
        CFRelease(updateQuery);
    }

    CFRelease(query);
    return status == errSecSuccess;
}


bool KIPLATFORM::SECRETS::GetSecret(const wxString& aService, const wxString& aKey, wxString& aValue)
{
    // Store UTF-8 strings in variables to keep them alive during API calls
    wxScopedCharBuffer utf8Service = aService.utf8_str();
    wxScopedCharBuffer utf8Key = aKey.utf8_str();

    // Create a query for the secret
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, CFStringCreateWithCString(NULL, utf8Service.data(), kCFStringEncodingUTF8));
    CFDictionarySetValue(query, kSecAttrAccount, CFStringCreateWithCString(NULL, utf8Key.data(), kCFStringEncodingUTF8));
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue); // Return the secret data

    // Retrieve the secret from the keychain
    CFDataRef secretData = NULL;
    OSStatus status = SecItemCopyMatching(query, (CFTypeRef*)&secretData);

    if (status == errSecSuccess)
    {
        aValue = wxString::FromUTF8((const char*)CFDataGetBytePtr(secretData), CFDataGetLength(secretData));
        CFRelease(secretData);
    }

    CFRelease(query);
    return status == errSecSuccess;
}