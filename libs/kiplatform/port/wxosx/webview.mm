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

#include <kiplatform/webview.h>

#include <wx/webview.h>
#include <wx/log.h>
#include <wx/ffile.h>
#include <wx/filename.h>

#include <nlohmann/json.hpp>

#import <WebKit/WebKit.h>

namespace KIPLATFORM::WEBVIEW
{

bool SaveCookies( wxWebView* aWebView, const wxString& aTargetFile )
{
    if( !aWebView )
        return false;

    void* nativeBackend = aWebView->GetNativeBackend();

    if( !nativeBackend )
        return false;

    WKWebView* wkWebView = (__bridge WKWebView*) nativeBackend;
    WKHTTPCookieStore* cookieStore = wkWebView.configuration.websiteDataStore.httpCookieStore;

    __block nlohmann::json cookieArray = nlohmann::json::array();
    __block bool completed = false;

    [cookieStore getAllCookies:^( NSArray<NSHTTPCookie*>* cookies )
    {
        for( NSHTTPCookie* cookie in cookies )
        {
            nlohmann::json cookieObj;
            cookieObj["name"] = std::string( [cookie.name UTF8String] );
            cookieObj["value"] = std::string( [cookie.value UTF8String] );
            cookieObj["domain"] = std::string( [cookie.domain UTF8String] );
            cookieObj["path"] = std::string( [cookie.path UTF8String] );
            cookieObj["secure"] = cookie.isSecure;
            cookieObj["httpOnly"] = cookie.isHTTPOnly;

            if( cookie.expiresDate )
            {
                cookieObj["expires"] = static_cast<int64_t>(
                        [cookie.expiresDate timeIntervalSince1970] );
            }
            else
            {
                cookieObj["expires"] = 0; // Session cookie
            }

            cookieArray.push_back( cookieObj );
        }

        completed = true;
    }];

    // Wait for async completion - run the run loop
    NSDate* timeout = [NSDate dateWithTimeIntervalSinceNow:5.0];

    while( !completed && [[NSDate date] compare:timeout] == NSOrderedAscending )
    {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                 beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    }

    if( !completed )
    {
        wxLogDebug( "Timeout waiting for cookies" );
        return false;
    }

    // Write to file
    wxFFile file( aTargetFile, "w" );

    if( !file.IsOpened() )
    {
        wxLogDebug( "Failed to open cookie file for writing: %s", aTargetFile );
        return false;
    }

    std::string jsonStr = cookieArray.dump( 2 );
    file.Write( jsonStr.c_str(), jsonStr.size() );

    return true;
}


bool LoadCookies( wxWebView* aWebView, const wxString& aSourceFile )
{
    if( !aWebView )
        return false;

    if( !wxFileName::FileExists( aSourceFile ) )
        return false;

    void* nativeBackend = aWebView->GetNativeBackend();

    if( !nativeBackend )
        return false;

    WKWebView* wkWebView = (__bridge WKWebView*) nativeBackend;
    WKHTTPCookieStore* cookieStore = wkWebView.configuration.websiteDataStore.httpCookieStore;

    // Read JSON file
    wxFFile file( aSourceFile, "r" );

    if( !file.IsOpened() )
    {
        wxLogDebug( "Failed to open cookie file for reading: %s", aSourceFile );
        return false;
    }

    wxString contents;
    file.ReadAll( &contents );

    nlohmann::json cookieArray;

    try
    {
        cookieArray = nlohmann::json::parse( contents.ToStdString() );
    }
    catch( const nlohmann::json::exception& e )
    {
        wxLogDebug( "Failed to parse cookie JSON: %s", e.what() );
        return false;
    }

    __block int pendingCount = static_cast<int>( cookieArray.size() );

    for( const auto& cookieObj : cookieArray )
    {
        std::string name = cookieObj.value( "name", "" );
        std::string value = cookieObj.value( "value", "" );
        std::string domain = cookieObj.value( "domain", "" );
        std::string path = cookieObj.value( "path", "/" );
        bool        secure = cookieObj.value( "secure", false );
        bool        httpOnly = cookieObj.value( "httpOnly", false );
        int64_t     expires = cookieObj.value( "expires", 0 );

        NSMutableDictionary* properties = [NSMutableDictionary dictionary];
        properties[NSHTTPCookieName] = [NSString stringWithUTF8String:name.c_str()];
        properties[NSHTTPCookieValue] = [NSString stringWithUTF8String:value.c_str()];
        properties[NSHTTPCookieDomain] = [NSString stringWithUTF8String:domain.c_str()];
        properties[NSHTTPCookiePath] = [NSString stringWithUTF8String:path.c_str()];

        if( expires > 0 )
        {
            properties[NSHTTPCookieExpires] =
                    [NSDate dateWithTimeIntervalSince1970:static_cast<double>( expires )];
        }

        if( secure )
            properties[NSHTTPCookieSecure] = @"TRUE";

        NSHTTPCookie* cookie = [NSHTTPCookie cookieWithProperties:properties];

        if( cookie )
        {
            [cookieStore setCookie:cookie
                 completionHandler:^{
                     --pendingCount;
                 }];
        }
        else
        {
            --pendingCount;
        }
    }

    // Wait for all cookies to be set
    NSDate* timeout = [NSDate dateWithTimeIntervalSinceNow:5.0];

    while( pendingCount > 0 && [[NSDate date] compare:timeout] == NSOrderedAscending )
    {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                                 beforeDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];
    }

    return true;
}

bool DeleteCookies( wxWebView* aWebView )
{
    if( !aWebView )
        return false;

    void* nativeBackend = aWebView->GetNativeBackend();

    if( !nativeBackend )
        return false;

    WKWebView* wkWebView = (__bridge WKWebView*) nativeBackend;
    WKWebsiteDataStore* dataStore = wkWebView.configuration.websiteDataStore;

    NSSet* websiteDataTypes = [NSSet setWithObject:WKWebsiteDataTypeCookies];
    NSDate* dateFrom = [NSDate dateWithTimeIntervalSince1970:0];

    [dataStore removeDataOfTypes:websiteDataTypes modifiedSince:dateFrom completionHandler:^{
        // Done
    }];

    return true;
}

} // namespace KIPLATFORM::WEBVIEW
