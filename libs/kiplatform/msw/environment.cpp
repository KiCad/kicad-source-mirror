/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <Ian.S.McInerney at ieee.org>
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <Windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <winhttp.h>


void KIPLATFORM::ENV::Init()
{
    // No tasks for this platform
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
    HINTERNET                            session = NULL;
    bool                                 success = false;

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
    else
    {
        autoProxyDetect = true;
    }

    if( autoProxyDetect )
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

        autoProxyOptions.fAutoLogonIfChallenged = TRUE;

        autoProxyDetect =
                WinHttpGetProxyForUrl( session, aURL.c_str(), &autoProxyOptions, &autoProxyInfo );

        if( session )
        {
            WinHttpCloseHandle( session );
        }
    }

    if( autoProxyDetect )
    {
        if( autoProxyInfo.dwAccessType == WINHTTP_ACCESS_TYPE_NAMED_PROXY )
        {
            // autoProxyInfo will return a list of proxies that are semicolon delimited
            // todo...maybe figure out better selection logic
            wxString          proxyList = autoProxyInfo.lpszProxy;
            wxStringTokenizer tokenizer( proxyList, ";" );

            if( tokenizer.HasMoreTokens() )
            {
                aCfg.host = tokenizer.GetNextToken();
            }

            success = true;
        }
    }
    else
    {
        if( ieProxyConfig.lpszProxy != NULL )
        {
            // ie proxy configs may return : or :: for an empty proxy

            aCfg.host = ieProxyConfig.lpszProxy;

            if(aCfg.host != ":" && aCfg.host != "::")
            {
                success = true;
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
