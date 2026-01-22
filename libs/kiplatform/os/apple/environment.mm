/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <Ian.S.McInerney at ieee.org>
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

#include <kiplatform/environment.h>

#import <Cocoa/Cocoa.h>
#import <CFNetwork/CFNetwork.h>
#include <wx/osx/core/cfstring.h>
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/uri.h>


void KIPLATFORM::ENV::Init()
{
    // Disable the automatic window tabbing OSX does
    [NSWindow setAllowsAutomaticWindowTabbing:NO];

    // No tasks for this platform
}


bool KIPLATFORM::ENV::MoveToTrash( const wxString& aPath, wxString& aError )
{
    bool     isDirectory = wxDirExists( aPath );
    NSURL*   url = [NSURL fileURLWithPath:wxCFStringRef( aPath ).AsNSString() isDirectory:isDirectory];
    NSError* err = nullptr;

    BOOL result = [[NSFileManager defaultManager] trashItemAtURL:url resultingItemURL:nil error:&err];

    // Extract the error string if the operation failed
    if( result == NO )
    {
        NSString* errmsg;

        if( err.localizedRecoverySuggestion == nil )
        {
            errmsg = err.localizedFailureReason;
        }
        else
        {
            errmsg = [err.localizedFailureReason stringByAppendingFormat:@"\n\n%@",
                      err.localizedRecoverySuggestion];
        }

        aError = wxCFStringRef::AsString( (CFStringRef) errmsg );
        return false;
    }

    return true;
}


bool KIPLATFORM::ENV::IsNetworkPath( const wxString& aPath )
{
    // placeholder, we "nerf" behavior if its a network path so return false by default
    return false;
}


wxString KIPLATFORM::ENV::GetDocumentsPath()
{
    return wxStandardPaths::Get().GetDocumentsDir();
}


wxString KIPLATFORM::ENV::GetUserConfigPath()
{
    return wxStandardPaths::Get().GetUserConfigDir();
}


wxString KIPLATFORM::ENV::GetUserDataPath()
{
    return wxStandardPaths::Get().GetUserDataDir();
}


wxString KIPLATFORM::ENV::GetUserLocalDataPath()
{
    return wxStandardPaths::Get().GetUserLocalDataDir();
}


wxString KIPLATFORM::ENV::GetUserCachePath()
{
    NSURL* url = [[NSFileManager defaultManager] URLForDirectory:NSCachesDirectory
                                                 inDomain:NSUserDomainMask
                                                 appropriateForURL:nil
                                                 create:NO error:nil];

    return wxCFStringRef::AsString( ( CFStringRef) url.path );
}


namespace
{

// Helper to extract proxy config from a resolved proxy dictionary
bool extractProxyFromDict( CFDictionaryRef aProxy, KIPLATFORM::ENV::PROXY_CONFIG& aCfg )
{
    CFStringRef proxyType = (CFStringRef) CFDictionaryGetValue( aProxy, kCFProxyTypeKey );

    if( !proxyType )
        return false;

    // Skip direct connections
    if( CFEqual( proxyType, kCFProxyTypeNone ) )
        return false;

    // Get proxy host
    CFStringRef host = (CFStringRef) CFDictionaryGetValue( aProxy, kCFProxyHostNameKey );

    if( !host )
        return false;

    // Clear any stale data from previous lookups
    aCfg.host.clear();
    aCfg.username.clear();
    aCfg.password.clear();

    // Determine scheme based on proxy type
    wxString scheme;

    if( CFEqual( proxyType, kCFProxyTypeHTTP ) )
        scheme = wxT( "http" );
    else if( CFEqual( proxyType, kCFProxyTypeHTTPS ) )
        scheme = wxT( "https" );
    else if( CFEqual( proxyType, kCFProxyTypeSOCKS ) )
        scheme = wxT( "socks5" );

    // Build scheme://host:port string
    if( !scheme.IsEmpty() )
        aCfg.host = scheme + wxT( "://" );

    wxString hostStr = wxCFStringRef::AsString( host );

    // Get port if present
    CFNumberRef port = (CFNumberRef) CFDictionaryGetValue( aProxy, kCFProxyPortNumberKey );
    int         portNum = 0;

    if( port )
        CFNumberGetValue( port, kCFNumberIntType, &portNum );

    // Bracket IPv6 addresses when port will be appended
    bool isIPv6 = hostStr.Contains( wxT( ":" ) );

    if( isIPv6 && portNum > 0 )
        aCfg.host += wxT( "[" ) + hostStr + wxT( "]" );
    else
        aCfg.host += hostStr;

    if( portNum > 0 )
        aCfg.host += wxString::Format( wxT( ":%d" ), portNum );

    // Get credentials if present
    CFStringRef username = (CFStringRef) CFDictionaryGetValue( aProxy, kCFProxyUsernameKey );
    CFStringRef password = (CFStringRef) CFDictionaryGetValue( aProxy, kCFProxyPasswordKey );

    if( username )
        aCfg.username = wxCFStringRef::AsString( username );

    if( password )
        aCfg.password = wxCFStringRef::AsString( password );

    return true;
}


// Evaluate a PAC script and extract proxy configuration
bool evaluatePACScript( CFURLRef aPacUrl, CFURLRef aTargetUrl, KIPLATFORM::ENV::PROXY_CONFIG& aCfg )
{
    // Synchronously fetch the PAC script
    NSURLRequest* request = [NSURLRequest requestWithURL:(__bridge NSURL*) aPacUrl
                                             cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
                                         timeoutInterval:10.0];

    __block NSData*  scriptData = nil;
    __block NSError* fetchError = nil;

    dispatch_semaphore_t semaphore = dispatch_semaphore_create( 0 );

    NSURLSessionDataTask* task =
            [[NSURLSession sharedSession] dataTaskWithRequest:request
                                            completionHandler:^( NSData* data, NSURLResponse* response,
                                                                 NSError* error ) {
                                                scriptData = data;
                                                fetchError = error;
                                                dispatch_semaphore_signal( semaphore );
                                            }];

    [task resume];
    dispatch_semaphore_wait( semaphore, dispatch_time( DISPATCH_TIME_NOW, 10 * NSEC_PER_SEC ) );

    if( fetchError || !scriptData )
        return false;

    NSString* scriptString = [[NSString alloc] initWithData:scriptData encoding:NSUTF8StringEncoding];

    if( !scriptString )
        return false;

    CFErrorRef  error = nullptr;
    CFArrayRef  pacProxies = CFNetworkCopyProxiesForAutoConfigurationScript(
            (__bridge CFStringRef) scriptString, aTargetUrl, &error );

    if( error )
    {
        CFRelease( error );
        return false;
    }

    if( !pacProxies )
        return false;

    bool success = false;

    for( CFIndex i = 0; i < CFArrayGetCount( pacProxies ) && !success; i++ )
    {
        CFDictionaryRef proxy = (CFDictionaryRef) CFArrayGetValueAtIndex( pacProxies, i );
        success = extractProxyFromDict( proxy, aCfg );
    }

    CFRelease( pacProxies );

    return success;
}

} // anonymous namespace


bool KIPLATFORM::ENV::GetSystemProxyConfig( const wxString& aURL, PROXY_CONFIG& aCfg )
{
    CFURLRef url = CFURLCreateWithString( kCFAllocatorDefault,
                                          wxCFStringRef( aURL ).AsNSString(),
                                          nullptr );
    if( !url )
        return false;

    CFDictionaryRef proxySettings = CFNetworkCopySystemProxySettings();

    if( !proxySettings )
    {
        CFRelease( url );
        return false;
    }

    CFArrayRef proxies = CFNetworkCopyProxiesForURL( url, proxySettings );
    CFRelease( proxySettings );

    if( !proxies )
    {
        CFRelease( url );
        return false;
    }

    bool success = false;

    for( CFIndex i = 0; i < CFArrayGetCount( proxies ) && !success; i++ )
    {
        CFDictionaryRef proxy = (CFDictionaryRef) CFArrayGetValueAtIndex( proxies, i );
        CFStringRef     proxyType = (CFStringRef) CFDictionaryGetValue( proxy, kCFProxyTypeKey );

        if( !proxyType )
            continue;

        // Handle PAC (Proxy Auto-Configuration) URLs
        if( CFEqual( proxyType, kCFProxyTypeAutoConfigurationURL ) )
        {
            CFURLRef pacUrl = (CFURLRef) CFDictionaryGetValue( proxy,
                                                               kCFProxyAutoConfigurationURLKey );

            if( pacUrl )
                success = evaluatePACScript( pacUrl, url, aCfg );

            continue;
        }

        success = extractProxyFromDict( proxy, aCfg );
    }

    CFRelease( proxies );
    CFRelease( url );

    return success;
}


bool KIPLATFORM::ENV::VerifyFileSignature( const wxString& aPath )
{
    return true;
}


wxString KIPLATFORM::ENV::GetAppUserModelId()
{
    return wxEmptyString;
}


void KIPLATFORM::ENV::SetAppDetailsForWindow( wxWindow* aWindow, const wxString& aRelaunchCommand,
                                              const wxString& aRelaunchDisplayName )
{
}


wxString KIPLATFORM::ENV::GetCommandLineStr()
{
    return wxEmptyString;
}


void KIPLATFORM::ENV::AddToRecentDocs( const wxString& aPath )
{
}