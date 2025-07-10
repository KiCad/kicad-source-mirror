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
#include <wx/utils.h>


void KIPLATFORM::ENV::Init()
{
    // Disable proxy menu in Unity window manager. Only usual menubar works with
    // wxWidgets (at least <= 3.1).  When the proxy menu menubar is enable, some
    // important things for us do not work: menuitems UI events and shortcuts.
    wxString wm;

    if( wxGetEnv( wxT( "XDG_CURRENT_DESKTOP" ), &wm ) && wm.CmpNoCase( wxT( "Unity" ) ) == 0 )
        wxSetEnv( wxT( "UBUNTU_MENUPROXY" ), wxT( "0" ) );

#if !wxUSE_GLCANVAS_EGL
    // Force the use of X11 backend (or wayland-x11 compatibility layer).  This is
    // required until wxWidgets supports the Wayland compositors
    wxSetEnv( wxT( "GDK_BACKEND" ), wxT( "x11" ) );
#endif

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
    return false;
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