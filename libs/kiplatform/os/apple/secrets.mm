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

#import <Security/Security.h>
#include <dispatch/dispatch.h>

#include <memory>
#include <mutex>
#include <utility>


// The interaction switch is process-wide, so every keychain call holds this; otherwise a no-prompt read could
// silence another call's unlock prompt, or that call could re-enable prompting mid-read
static std::mutex& keychainMutex()
{
    static std::mutex mutex;
    return mutex;
}


// Keychain calls wait on unlock prompts, so aWork runs off the main thread and aDone on the main queue
// Only a pointer to aDone crosses threads, since it may own window state
template <typename RESULT>
static void runOffMainThread( std::function<RESULT()> aWork, std::function<void( RESULT )> aDone )
{
    auto* done = new std::function<void( RESULT )>( std::move( aDone ) );

    dispatch_async( dispatch_get_global_queue( QOS_CLASS_USER_INITIATED, 0 ),
                    ^{
                        RESULT result = aWork();

                        dispatch_async( dispatch_get_main_queue(),
                                        ^{
                                            std::unique_ptr<std::function<void( RESULT )>> completion( done );

                                            if( *completion )
                                                ( *completion )( result );
                                        } );
                    } );
}


bool KIPLATFORM::SECRETS::StoreSecret( const wxString& aService, const wxString& aKey, const wxString& aValue )
{
    std::lock_guard<std::mutex> lock( keychainMutex() );

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


void KIPLATFORM::SECRETS::StoreSecretAsync( const wxString& aService, const wxString& aKey, const wxString& aSecret,
                                           std::function<void( bool )> aDone )
{
    runOffMainThread<bool>(
            [service = aService, key = aKey, secret = aSecret]()
            {
                return StoreSecret( service, key, secret );
            },
            std::move( aDone ) );
}


static bool getSecret(const wxString& aService, const wxString& aKey, wxString& aValue)
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


bool KIPLATFORM::SECRETS::GetSecret( const wxString& aService, const wxString& aKey, wxString& aValue )
{
    std::lock_guard<std::mutex> lock( keychainMutex() );

    return getSecret( aService, aKey, aValue );
}


void KIPLATFORM::SECRETS::GetSecretAsync( const wxString& aService, const wxString& aKey,
                                         std::function<void( bool, const wxString& )> aDone )
{
    using FOUND = std::pair<bool, wxString>;

    runOffMainThread<FOUND>(
            [service = aService, key = aKey]()
            {
                FOUND found;
                found.first = GetSecret( service, key, found.second );
                return found;
            },
            [done = std::move( aDone )]( FOUND aFound )
            {
                done( aFound.first, aFound.second );
            } );
}


bool KIPLATFORM::SECRETS::GetSecretNoPrompt( const wxString& aService, const wxString& aKey, wxString& aValue )
{
    std::unique_lock<std::mutex> lock( keychainMutex(), std::try_to_lock );

    // Another call may be waiting on the user, and reading as absent beats waiting behind it
    if( !lock.owns_lock() )
        return false;

    // A locked keychain then fails with errSecInteractionNotAllowed instead of asking to be unlocked
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    Boolean previous = true;
    SecKeychainGetUserInteractionAllowed( &previous );
    SecKeychainSetUserInteractionAllowed( false );

    bool found = getSecret( aService, aKey, aValue );

    SecKeychainSetUserInteractionAllowed( previous );
#pragma clang diagnostic pop

    return found;
}


bool KIPLATFORM::SECRETS::DeleteSecret(const wxString& aService, const wxString& aKey)
{
    std::lock_guard<std::mutex> lock( keychainMutex() );

    wxScopedCharBuffer utf8Service = aService.utf8_str();
    wxScopedCharBuffer utf8Key = aKey.utf8_str();

    CFMutableDictionaryRef query = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, CFStringCreateWithCString(NULL, utf8Service.data(), kCFStringEncodingUTF8));
    CFDictionarySetValue(query, kSecAttrAccount, CFStringCreateWithCString(NULL, utf8Key.data(), kCFStringEncodingUTF8));

    OSStatus status = SecItemDelete(query);
    CFRelease(query);
    return status == errSecSuccess || status == errSecItemNotFound;
}
