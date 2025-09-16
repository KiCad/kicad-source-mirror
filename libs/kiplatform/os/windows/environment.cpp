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
#include <wx/intl.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/app.h>
#include <wx/uri.h>
#include <wx/window.h>

#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <propkey.h>
#include <propvarutil.h>
#if defined( __MINGW32__ )
    #include <shobjidl.h>
#else
    #include <shobjidl_core.h>
#endif
#include <winhttp.h>

#include <softpub.h>

#if defined( __MINGW32__ )
    #include <shlobj.h>
#else
    #include <shlobj_core.h>
#endif

#include <wincrypt.h>
#include <wintrust.h>

#define INCLUDE_KICAD_VERSION // fight me
#include <kicad_build_version.h>


void KIPLATFORM::ENV::Init()
{
    ::SetCurrentProcessExplicitAppUserModelID( GetAppUserModelId().wc_str() );
}


bool KIPLATFORM::ENV::MoveToTrash( const wxString& aPath, wxString& aError )
{
    // The filename field must be a double-null terminated string
    wxString temp = aPath + '\0';

    SHFILEOPSTRUCT fileOp;
    ::ZeroMemory( &fileOp, sizeof( fileOp ) );

    fileOp.hwnd   = nullptr; // Set to null since there is no progress dialog
    fileOp.wFunc  = FO_DELETE;
    fileOp.pFrom  = temp.c_str();
    fileOp.pTo    = nullptr; // Set to to NULL since we aren't moving the file
    fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOERRORUI | FOF_NOCONFIRMATION | FOF_SILENT;

    int eVal = SHFileOperation( &fileOp );

    if( eVal != 0 )
    {
        aError = wxString::Format( _( "Error code: %d" ), eVal );
        return false;
    }

    return true;
}


bool KIPLATFORM::ENV::IsNetworkPath( const wxString& aPath )
{
    return ::PathIsNetworkPathW( aPath.wc_str() );
}


wxString KIPLATFORM::ENV::GetDocumentsPath()
{
    // If called by a python script in stand-alone (outside KiCad), wxStandardPaths::Get()
    // complains about not existing app. so use a dummy app
    if( wxTheApp ==  nullptr )
    {
        wxApp dummy;
        return wxStandardPaths::Get().GetDocumentsDir();
    }

    return wxStandardPaths::Get().GetDocumentsDir();
}


wxString KIPLATFORM::ENV::GetUserConfigPath()
{
    // If called by a python script in stand-alone (outside KiCad), wxStandardPaths::Get()
    // complains about not existing app. so use a dummy app
    if( wxTheApp ==  nullptr )
    {
        wxApp dummy;
        return wxStandardPaths::Get().GetUserConfigDir();
    }

    return wxStandardPaths::Get().GetUserConfigDir();
}


wxString KIPLATFORM::ENV::GetUserDataPath()
{
    // If called by a python script in stand-alone (outside KiCad), wxStandardPaths::Get()
    // complains about not existing app. so use a dummy app
    if( wxTheApp ==  nullptr )
    {
        wxApp dummy;
        return wxStandardPaths::Get().GetUserDataDir();
    }

    return wxStandardPaths::Get().GetUserDataDir();
}


wxString KIPLATFORM::ENV::GetUserLocalDataPath()
{
    // If called by a python script in stand-alone (outside KiCad), wxStandardPaths::Get()
    // complains about not existing app. so use a dummy app
    if( wxTheApp == nullptr )
    {
        wxApp dummy;
        return wxStandardPaths::Get().GetUserLocalDataDir();
    }

    return wxStandardPaths::Get().GetUserLocalDataDir();
}


wxString KIPLATFORM::ENV::GetUserCachePath()
{
    // Unfortunately AppData/Local is the closest analog to "Cache" directories of other platforms

    // Make sure we don't include the "appinfo" (appended app name)

    // If called by a python script in stand-alone (outside KiCad), wxStandardPaths::Get()
    // complains about not existing app. so use a dummy app
    if( wxTheApp ==  nullptr )
    {
        wxApp dummy;
        wxStandardPaths::Get().UseAppInfo( wxStandardPaths::AppInfo_None );

        return wxStandardPaths::Get().GetUserLocalDataDir();
   }

    wxStandardPaths::Get().UseAppInfo( wxStandardPaths::AppInfo_None );

    return wxStandardPaths::Get().GetUserLocalDataDir();
}


bool KIPLATFORM::ENV::GetSystemProxyConfig( const wxString& aURL, PROXY_CONFIG& aCfg )
{
    // Original source from Microsoft sample (public domain)
    // https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/WinhttpProxy/cpp/GetProxy.cpp#L844
    bool                                 autoProxyDetect = false;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ieProxyConfig = { 0 };
    WINHTTP_AUTOPROXY_OPTIONS            autoProxyOptions = { 0 };
    WINHTTP_PROXY_INFO                   autoProxyInfo = { 0 };
    HINTERNET                            proxyResolveSession = NULL;
    bool                                 success = false;

    wxURI uri( aURL );

    LPWSTR proxyStr = NULL;
    LPWSTR bypassProxyStr = NULL;

    if( WinHttpGetIEProxyConfigForCurrentUser( &ieProxyConfig ) )
    {
        // welcome to the wonderful world of IE
        // we use the ie config simply to handle it off to the other win32 api
        if( ieProxyConfig.fAutoDetect )
        {
            autoProxyDetect = true;
        }

        if( ieProxyConfig.lpszAutoConfigUrl != NULL )
        {
            autoProxyDetect = true;
            autoProxyOptions.lpszAutoConfigUrl = ieProxyConfig.lpszAutoConfigUrl;
        }
    }
    else if( GetLastError() == ERROR_FILE_NOT_FOUND )
    {
        // this is the only error code where we want to continue attempting to find a proxy
        autoProxyDetect = true;
    }

    if( autoProxyDetect )
    {
        proxyResolveSession =
                WinHttpOpen( NULL, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                             WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC );

        if( proxyResolveSession )
        {
            // either we use the ie url or we set the auto detect mode
            if( autoProxyOptions.lpszAutoConfigUrl != NULL )
            {
                autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
            }
            else
            {
                autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
                autoProxyOptions.dwAutoDetectFlags =
                        WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
            }

            // dont do auto logon at first, this allows windows to use an cache
            // per https://docs.microsoft.com/en-us/windows/win32/winhttp/autoproxy-cache
            autoProxyOptions.fAutoLogonIfChallenged = FALSE;

            autoProxyDetect = WinHttpGetProxyForUrl( proxyResolveSession, aURL.c_str(),
                                                     &autoProxyOptions, &autoProxyInfo );

            if( !autoProxyDetect && GetLastError() == ERROR_WINHTTP_LOGIN_FAILURE )
            {
                autoProxyOptions.fAutoLogonIfChallenged = TRUE;

                // try again with auto login now
                autoProxyDetect = WinHttpGetProxyForUrl( proxyResolveSession, aURL.c_str(),
                                                         &autoProxyOptions, &autoProxyInfo );
            }

            if( autoProxyDetect )
            {
                if( autoProxyInfo.dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY )
                {
                    proxyStr = autoProxyInfo.lpszProxy;
                    bypassProxyStr = autoProxyInfo.lpszProxyBypass;
                }
            }

            WinHttpCloseHandle( proxyResolveSession );
        }
    }

    if( !autoProxyDetect && ieProxyConfig.lpszProxy != NULL )
    {
        proxyStr = ieProxyConfig.lpszProxy;
        bypassProxyStr = ieProxyConfig.lpszProxyBypass;
    }

    bool bypassed = false;

    if( bypassProxyStr != NULL )
    {
        wxStringTokenizer tokenizer( bypassProxyStr, ";" );

        while( tokenizer.HasMoreTokens() )
        {
            wxString host = tokenizer.GetNextToken();

            if( host == uri.GetServer() )
            {
                // the given url has a host in the proxy bypass list
                return false;
            }

            // <local> is a special case that says all local sites bypass
            // the windows way for considering local is any host without periods in the name that would imply
            // some non-internal dns resolution
            if( host == "<local>" )
            {
                if( !uri.GetServer().Contains( "." ) )
                {
                    // great its a local uri that is bypassed
                    bypassed = true;
                    break;
                }
            }
        }
    }

    if( !bypassed && proxyStr != NULL )
    {
        // proxyStr can be in the following format per MSDN
        //([<scheme>=][<scheme>"://"]<server>[":"<port>])
        //and separated by semicolons or whitespace
        wxStringTokenizer tokenizer( proxyStr, "; \t" );

        while( tokenizer.HasMoreTokens() )
        {
            wxString entry = tokenizer.GetNextToken();

            // deal with the [<scheme>=] part, which may or may not exist
            if( entry.Contains( "=" ) )
            {
                wxString scheme = entry.BeforeFirst( '=' ).Lower();
                entry = entry.AfterFirst( '=' );

                // skip processing if the scheme doesnt match
                if( scheme != uri.GetScheme().Lower() )
                {
                    continue;
                }

                // we continue with the [<scheme>=] stripped off if we matched
            }

            // is the entry left not empty? we just take the first result
            // : and :: are also special cases we want to ignore
            if( entry != "" && entry != ":" && entry != "::" )
            {
                aCfg.host = entry;
                success = true;
                break;
            }
        }
    }


    // We have to clean up the strings the win32 api returned
    if( autoProxyInfo.lpszProxy )
    {
        GlobalFree( autoProxyInfo.lpszProxy );
        autoProxyInfo.lpszProxy = NULL;
    }

    if( autoProxyInfo.lpszProxyBypass )
    {
        GlobalFree( autoProxyInfo.lpszProxyBypass );
        autoProxyInfo.lpszProxyBypass = NULL;
    }

    if( ieProxyConfig.lpszAutoConfigUrl != NULL )
    {
        GlobalFree( ieProxyConfig.lpszAutoConfigUrl );
        ieProxyConfig.lpszAutoConfigUrl = NULL;
    }

    if( ieProxyConfig.lpszProxy != NULL )
    {
        GlobalFree( ieProxyConfig.lpszProxy );
        ieProxyConfig.lpszProxy = NULL;
    }

    if( ieProxyConfig.lpszProxyBypass != NULL )
    {
        GlobalFree( ieProxyConfig.lpszProxyBypass );
        ieProxyConfig.lpszProxyBypass = NULL;
    }

    return success;
}


bool KIPLATFORM::ENV::VerifyFileSignature( const wxString& aPath )
{
    WINTRUST_FILE_INFO fileData;
    memset( &fileData, 0, sizeof( fileData ) );
    fileData.cbStruct = sizeof( WINTRUST_FILE_INFO );
    fileData.pcwszFilePath = aPath.wc_str();

    // verifies entire certificate chain
    GUID policy = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    WINTRUST_DATA trustData;
    memset( &trustData, 0, sizeof( trustData ) );

    trustData.cbStruct = sizeof( trustData );
    trustData.dwUIChoice = WTD_UI_NONE;
    // revocation checking incurs latency penalities due to need for online queries
    trustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    trustData.dwUnionChoice = WTD_CHOICE_FILE;
    trustData.dwStateAction = WTD_STATEACTION_VERIFY;
    trustData.pFile = &fileData;


    bool verified = false;
    LONG status = WinVerifyTrust( NULL, &policy, &trustData );

    verified = ( status == ERROR_SUCCESS );

    // Cleanup/release (yes its weird looking)
    trustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust( NULL, &policy, &trustData );

    return verified;
}


wxString KIPLATFORM::ENV::GetAppUserModelId()
{
    // The application model id allows for taskbar grouping
    // However, be warned, this cannot be too unique like per-process
    // Because longer scope Windows features, such as "Pin to Taskbar"
    // on a running application, depend on this being consistent.
    std::vector<wxString> modelIdComponents;
    modelIdComponents.push_back( wxS( "Kicad" ) );
    modelIdComponents.push_back( wxS( "Kicad" ) );
    modelIdComponents.push_back( wxTheApp->GetAppName() );
    modelIdComponents.push_back( KICAD_MAJOR_MINOR_VERSION );

    wxString modelId;
    for( const auto& str : modelIdComponents )
    {
        modelId += str;
        modelId += wxS( "." );
    }

    modelId.RemoveLast();                      // remove trailing dot
    modelId.Replace( wxS( " " ), wxS( "_" ) ); // remove spaces sanity

    // the other limitation is 127 characters but we arent trying to hit that limit yet

    return modelId;
}


void KIPLATFORM::ENV::SetAppDetailsForWindow( wxWindow* aWindow, const wxString& aRelaunchCommand,
                                              const wxString& aRelaunchDisplayName )
{
    IPropertyStore* pps;
    HRESULT         hr = ::SHGetPropertyStoreForWindow( aWindow->GetHWND(), IID_PPV_ARGS( &pps ) );
    if( SUCCEEDED( hr ) )
    {
        PROPVARIANT pv;

        // This is required for any the other properties to actually work
        hr = ::InitPropVariantFromString( GetAppUserModelId().wc_str(), &pv );

        if( SUCCEEDED( hr ) )
        {
            hr = pps->SetValue( PKEY_AppUserModel_ID, pv );
            PropVariantClear( &pv );
        }


        if( !aRelaunchCommand.empty() )
        {
            hr = ::InitPropVariantFromString( aRelaunchCommand.wc_str(), &pv );
        }
        else
        {
            // empty var
            ::PropVariantInit( &pv );
        }

        if( SUCCEEDED( hr ) )
        {
            hr = pps->SetValue( PKEY_AppUserModel_RelaunchCommand, pv );
            PropVariantClear( &pv );
        }

        if( !aRelaunchDisplayName.empty() )
        {
            hr = ::InitPropVariantFromString( aRelaunchDisplayName.wc_str(), &pv );
        }
        else
        {
            // empty var
            ::PropVariantInit( &pv );
        }

        if( SUCCEEDED( hr ) )
        {
            hr = pps->SetValue( PKEY_AppUserModel_RelaunchDisplayNameResource, pv );
            PropVariantClear( &pv );
        }

        pps->Release();
    }
}


wxString KIPLATFORM::ENV::GetCommandLineStr()
{
    return ::GetCommandLine();
}


void KIPLATFORM::ENV::AddToRecentDocs( const wxString& aPath )
{
    IShellItem* psi = nullptr;
    HRESULT     hr = SHCreateItemFromParsingName( aPath.wc_str(), NULL, IID_PPV_ARGS( &psi ) );

    if( SUCCEEDED( hr ) )
    {
        wxString       appID = GetAppUserModelId();
        SHARDAPPIDINFO info;
        info.psi = psi;
        info.pszAppID = appID.wc_str();
        ::SHAddToRecentDocs( SHARD_APPIDINFO, &info );

        psi->Release();
    }

    ::SHAddToRecentDocs( SHARD_PATHW, aPath.wc_str() );
}