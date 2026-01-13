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
#include <wx/utils.h>

#include <json_common.h>

#include <windows.h>

// WebView2 does not exist when compiling on msys2/mingw. So disable cookies management
// not buildable on msys2 (there are also compil issues with client.h and event.h)
#if !defined(__MINGW32__)
    #include <wrl/client.h>
    #include <wrl/event.h>
    #include <WebView2.h>

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Callback;
#endif

namespace KIPLATFORM::WEBVIEW
{

bool SaveCookies( wxWebView* aWebView, const wxString& aTargetFile )
{
    if( !aWebView )
        return false;

    void* nativeBackend = aWebView->GetNativeBackend();

    if( !nativeBackend )
        return false;
#if defined(__MINGW32__)
    return false;
#else

    ICoreWebView2* coreWebView = static_cast<ICoreWebView2*>( nativeBackend );

    // We need ICoreWebView2_2 to access the CookieManager
    ComPtr<ICoreWebView2_2> webView2;

    if( FAILED( coreWebView->QueryInterface( IID_PPV_ARGS( &webView2 ) ) ) )
    {
        wxLogTrace( "webview", "Failed to get ICoreWebView2_2 interface. WebView2 runtime might be too old." );
        return false;
    }

    ComPtr<ICoreWebView2CookieManager> cookieManager;

    if( FAILED( webView2->get_CookieManager( &cookieManager ) ) )
    {
        wxLogTrace( "webview", "Failed to get cookie manager" );
        return false;
    }

    nlohmann::json cookieArray = nlohmann::json::array();
    bool           callbackFinished = false;
    bool           success = false;

    HRESULT hr = cookieManager->GetCookies(
            nullptr, // Get all cookies
            Callback<ICoreWebView2GetCookiesCompletedHandler>(
                    [&]( HRESULT result, ICoreWebView2CookieList* list ) -> HRESULT
                    {
                        if( FAILED( result ) || !list )
                        {
                            wxLogTrace( "webview", "GetCookies failed or returned null list" );
                            callbackFinished = true;
                            return S_OK;
                        }

                        UINT count = 0;
                        list->get_Count( &count );

                        for( UINT i = 0; i < count; ++i )
                        {
                            ComPtr<ICoreWebView2Cookie> cookie;
                            if( FAILED( list->GetValueAtIndex( i, &cookie ) ) )
                                continue;

                            nlohmann::json cookieObj;

                            LPWSTR name = nullptr;
                            cookie->get_Name( &name );

                            if( name )
                            {
                                cookieObj["name"] = std::string( wxString( name ).ToUTF8() );
                                CoTaskMemFree( name );
                            }

                            LPWSTR value = nullptr;
                            cookie->get_Value( &value );

                            if( value )
                            {
                                cookieObj["value"] = std::string( wxString( value ).ToUTF8() );
                                CoTaskMemFree( value );
                            }

                            LPWSTR domain = nullptr;
                            cookie->get_Domain( &domain );

                            if( domain )
                            {
                                cookieObj["domain"] = std::string( wxString( domain ).ToUTF8() );
                                CoTaskMemFree( domain );
                            }

                            LPWSTR path = nullptr;
                            cookie->get_Path( &path );

                            if( path )
                            {
                                cookieObj["path"] = std::string( wxString( path ).ToUTF8() );
                                CoTaskMemFree( path );
                            }

                            double expires = 0;
                            cookie->get_Expires( &expires );
                            cookieObj["expires"] = static_cast<int64_t>( expires );

                            BOOL secure = FALSE;
                            cookie->get_IsSecure( &secure );
                            cookieObj["secure"] = ( secure == TRUE );

                            BOOL httpOnly = FALSE;
                            cookie->get_IsHttpOnly( &httpOnly );
                            cookieObj["httpOnly"] = ( httpOnly == TRUE );

                            cookieArray.push_back( cookieObj );
                        }

                        success = true;
                        callbackFinished = true;
                        return S_OK;
                    } ).Get() );

    if( FAILED( hr ) )
    {
        wxLogTrace( "webview", "Failed to initiate GetCookies" );
        return false;
    }

    // Wait for callback with timeout (e.g., 5 seconds)
    // We must pump messages because WebView2 callbacks run on the UI thread
    long long startTime = wxGetLocalTimeMillis().GetValue();

    while( !callbackFinished )
    {
        wxSafeYield();

        if( ( wxGetLocalTimeMillis().GetValue() - startTime ) > 5000 )
        {
            wxLogTrace( "webview", "Timeout waiting for GetCookies callback" );
            return false;
        }

        // Sleep a tiny bit to avoid 100% CPU if Yield returns immediately
        wxMilliSleep( 10 );
    }

    if( !success )
        return false;

    // Write to file (binary mode to preserve UTF-8)
    wxFFile file( aTargetFile, "wb" );

    if( !file.IsOpened() )
    {
        wxLogTrace( "webview", "Failed to open cookie file for writing: %s", aTargetFile );
        return false;
    }

    std::string jsonStr = cookieArray.dump( 2 );
    file.Write( jsonStr.c_str(), jsonStr.size() );

    return true;
#endif
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

#if defined(__MINGW32__)
    return false;
#else

    ICoreWebView2* coreWebView = static_cast<ICoreWebView2*>( nativeBackend );

    ComPtr<ICoreWebView2_2> webView2;

    if( FAILED( coreWebView->QueryInterface( IID_PPV_ARGS( &webView2 ) ) ) )
    {
        wxLogTrace( "webview", "Failed to get ICoreWebView2_2 interface" );
        return false;
    }

    ComPtr<ICoreWebView2CookieManager> cookieManager;

    if( FAILED( webView2->get_CookieManager( &cookieManager ) ) )
    {
        wxLogDebug( "Failed to get cookie manager" );
        return false;
    }

    // Read JSON file (binary mode)
    wxFFile file( aSourceFile, "rb" );

    if( !file.IsOpened() )
    {
        wxLogTrace( "webview", "Failed to open cookie file for reading: %s", aSourceFile );
        return false;
    }

    wxFileOffset len = file.Length();

    if( len == 0 )
        return false;

    std::string jsonStr;
    jsonStr.resize( len );

    if( file.Read( &jsonStr[0], len ) != len )
    {
        wxLogTrace( "webview", "Failed to read cookie file" );
        return false;
    }

    nlohmann::json cookieArray;

    try
    {
        cookieArray = nlohmann::json::parse( jsonStr );
    }
    catch( const nlohmann::json::exception& e )
    {
        wxLogTrace( "webview", "Failed to parse cookie JSON: %s", e.what() );
        return false;
    }

    for( const auto& cookieObj : cookieArray )
    {
        std::string name = cookieObj.value( "name", "" );
        std::string value = cookieObj.value( "value", "" );
        std::string domain = cookieObj.value( "domain", "" );
        std::string path = cookieObj.value( "path", "/" );
        bool        secure = cookieObj.value( "secure", false );
        bool        httpOnly = cookieObj.value( "httpOnly", false );
        int64_t     expires = cookieObj.value( "expires", 0 );

        ComPtr<ICoreWebView2Cookie> cookie;
        if( FAILED( cookieManager->CreateCookie(
                    wxString::FromUTF8( name.c_str() ).wc_str(),
                    wxString::FromUTF8( value.c_str() ).wc_str(),
                    wxString::FromUTF8( domain.c_str() ).wc_str(),
                    wxString::FromUTF8( path.c_str() ).wc_str(),
                    &cookie ) ) )
        {
            continue;
        }

        cookie->put_Expires( static_cast<double>( expires ) );
        cookie->put_IsSecure( secure ? TRUE : FALSE );
        cookie->put_IsHttpOnly( httpOnly ? TRUE : FALSE );

        cookieManager->AddOrUpdateCookie( cookie.Get() );
    }

    return true;
#endif
}

bool DeleteCookies( wxWebView* aWebView )
{
    if( !aWebView )
        return false;

    void* nativeBackend = aWebView->GetNativeBackend();

    if( !nativeBackend )
        return false;
#if defined(__MINGW32__)
    return false;
#else

    ICoreWebView2* coreWebView = static_cast<ICoreWebView2*>( nativeBackend );

    ComPtr<ICoreWebView2_2> webView2;

    if( FAILED( coreWebView->QueryInterface( IID_PPV_ARGS( &webView2 ) ) ) )
    {
        wxLogTrace( "webview", "Failed to get ICoreWebView2_2 interface. WebView2 runtime might be too old." );
        return false;
    }

    ComPtr<ICoreWebView2CookieManager> cookieManager;

    if( FAILED( webView2->get_CookieManager( &cookieManager ) ) )
    {
        wxLogTrace( "webview", "Failed to get cookie manager" );
        return false;
    }

    if( FAILED( cookieManager->DeleteAllCookies() ) )
    {
        wxLogTrace( "webview", "Failed to delete all cookies" );
        return false;
    }

    return true;
#endif
}

} // namespace KIPLATFORM::WEBVIEW
