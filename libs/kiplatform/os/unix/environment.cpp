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

#include <glib.h>
#include <gio/gio.h>
#include <kiplatform/environment.h>
#include <wx/filename.h>
#include <wx/uri.h>
#include <wx/utils.h>
#include <wx/sysopt.h>


void KIPLATFORM::ENV::Init()
{
    // Disable proxy menu in Unity window manager. Only usual menubar works with
    // wxWidgets (at least <= 3.1).  When the proxy menu menubar is enable, some
    // important things for us do not work: menuitems UI events and shortcuts.
    wxString wm;

    if( wxGetEnv( wxT( "XDG_CURRENT_DESKTOP" ), &wm ) && wm.CmpNoCase( wxT( "Unity" ) ) == 0 )
        wxSetEnv( wxT( "UBUNTU_MENUPROXY" ), wxT( "0" ) );

    bool forceX11 = false;

#if wxCHECK_VERSION( 3, 3, 2 )
    #if wxHAS_EGL
        // Prefer GLX backend on X11 and EGL backend on Wayland.
        if( !wxGetEnv( wxT( "WAYLAND_DISPLAY" ), nullptr ) )
            wxSystemOptions::SetOption( "opengl.egl", 0 );
    #else
        // Forces GLX on X11 and XWayland
        forceX11 = true;
    #endif
#else
    #if !wxUSE_GLCANVAS_EGL
        // Forces GLX on X11 and XWayland
        forceX11 = true;
    #endif
#endif

    if( forceX11 )
        wxSetEnv( wxT( "GDK_BACKEND" ), wxT( "x11" ) );

    // Set GTK2-style input instead of xinput2.  This disables touchscreen and smooth
    // scrolling.  It's needed to ensure that we are not getting multiple mouse scroll
    // events.
    wxSetEnv( wxT( "GDK_CORE_DEVICE_EVENTS" ), wxT( "1" ) );
}


bool KIPLATFORM::ENV::MoveToTrash( const wxString& aPath, wxString& aError )
{
    GError* err   = nullptr;
    GFile*  file  = g_file_new_for_path( aPath.fn_str() );

    bool retVal = g_file_trash( file, nullptr, &err );

    // Extract the error string if the operation failed
    if( !retVal && err )
        aError = err->message;

    g_clear_error( &err );
    g_object_unref( file );

    return retVal;
}


bool KIPLATFORM::ENV::IsNetworkPath( const wxString& aPath )
{
    // placeholder, we "nerf" behavior if its a network path so return false by default
    return false;
}


wxString KIPLATFORM::ENV::GetDocumentsPath()
{
    wxString docsPath = g_get_user_data_dir();

    if( docsPath.IsEmpty() )
    {
        wxFileName fallback;

        fallback.AssignDir( g_get_home_dir() );
        fallback.AppendDir( wxT( ".local" ) );
        fallback.AppendDir( wxT( "share" ) );
        fallback.MakeAbsolute();

        docsPath = fallback.GetFullPath();
    }

    return docsPath;
}


wxString KIPLATFORM::ENV::GetUserConfigPath()
{
    return g_get_user_config_dir();
}


wxString KIPLATFORM::ENV::GetUserDataPath()
{
    return g_get_user_data_dir();
}


wxString KIPLATFORM::ENV::GetUserLocalDataPath()
{
    return g_get_user_data_dir();
}


wxString KIPLATFORM::ENV::GetUserCachePath()
{
    return g_get_user_cache_dir();
}


bool KIPLATFORM::ENV::GetSystemProxyConfig( const wxString& aURL, PROXY_CONFIG& aCfg )
{
    bool success = false;

    GProxyResolver* resolver = g_proxy_resolver_get_default();

    if( !resolver )
        return false;

    GError* error = nullptr;
    char**  proxyList = g_proxy_resolver_lookup( resolver, aURL.utf8_str(), nullptr, &error );

    if( error )
    {
        g_error_free( error );
        return false;
    }

    if( !proxyList )
        return false;

    // GProxyResolver returns a NULL-terminated list of proxy URIs.
    // We use the first non-direct proxy.
    for( int i = 0; proxyList[i] != nullptr; i++ )
    {
        wxString proxyUriStr( proxyList[i], wxConvUTF8 );

        // "direct://" means no proxy needed
        if( proxyUriStr == wxT( "direct://" ) )
            continue;

        wxURI proxyUri( proxyUriStr );

        // Clear any stale data from previous lookups
        aCfg.host.clear();
        aCfg.username.clear();
        aCfg.password.clear();

        // Build scheme://host:port string for curl.
        // The scheme is required for non-HTTP proxies (socks5, https, etc.)
        wxString scheme = proxyUri.GetScheme();

        if( !scheme.IsEmpty() )
            aCfg.host = scheme + wxT( "://" );

        wxString server = proxyUri.GetServer();

        // Bracket IPv6 addresses when port will be appended
        bool isIPv6 = server.Contains( wxT( ":" ) );

        if( isIPv6 && proxyUri.HasPort() )
            aCfg.host += wxT( "[" ) + server + wxT( "]" );
        else
            aCfg.host += server;

        if( proxyUri.HasPort() )
            aCfg.host += wxT( ":" ) + proxyUri.GetPort();

        // Extract credentials from userinfo (format: user[:password])
        wxString userInfo = proxyUri.GetUserInfo();

        if( !userInfo.IsEmpty() )
        {
            int colonPos = userInfo.Find( wxT( ":" ) );

            if( colonPos != wxNOT_FOUND )
            {
                aCfg.username = userInfo.Left( colonPos );
                aCfg.password = userInfo.Mid( colonPos + 1 );
            }
            else
            {
                aCfg.username = userInfo;
            }
        }

        success = true;
        break;
    }

    g_strfreev( proxyList );

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