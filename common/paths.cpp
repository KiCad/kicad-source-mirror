/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/string.h>

#include <kiplatform/environment.h>
#include <paths.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <config.h>

// lowercase or pretty case depending on platform
#if defined( __WXMAC__ ) || defined( __WXMSW__ )
#define KICAD_PATH_STR "KiCad"
#else
#define KICAD_PATH_STR "kicad"
#endif

wxString PATHS::GetUserPluginsPath()
{
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetDocumentsPath() );
    tmp.AppendDir( KICAD_PATH_STR );
    tmp.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );
    tmp.AppendDir( "plugins" );

    return tmp.GetFullPath();
}


wxString PATHS::GetUserPlugins3DPath()
{
    wxFileName tmp;

    tmp.AssignDir( PATHS::GetUserPluginsPath() );
    tmp.AppendDir( "3d" );

    return tmp.GetFullPath();
}


wxString PATHS::GetUserScriptingPath()
{
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetDocumentsPath() );
    tmp.AppendDir( KICAD_PATH_STR );
    tmp.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );
    tmp.AppendDir( "scripting" );

    return tmp.GetFullPath();
}


wxString PATHS::GetUserTemplatesPath()
{
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetDocumentsPath() );
    tmp.AppendDir( KICAD_PATH_STR );
    tmp.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );
    tmp.AppendDir( "template" );

    return tmp.GetFullPath();
}


wxString PATHS::GetDefaultUserProjectsPath()
{
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetDocumentsPath() );
    tmp.AppendDir( KICAD_PATH_STR );
    tmp.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );
    tmp.AppendDir( "projects" );

    return tmp.GetFullPath();
}


wxString PATHS::GetStockScriptingPath()
{
    wxString path;

    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        // Allow debugging from build dir by placing a "scripting" folder in the build root
        path = Pgm().GetExecutablePath() + wxT( "../scripting" );
    }
    else
    {
        //TODO(snh) break out the directory functions into KIPLATFORM
#if defined( __WXMAC__ )
        path = GetOSXKicadDataDir() + wxT( "/scripting" );
#elif defined( __WXMSW__ )
        path = Pgm().GetExecutablePath() + wxT( "../share/kicad/scripting" );
#else
        path = wxString( KICAD_DATA ) + wxS( "/scripting" );
#endif
    }

    return path;
}


wxString PATHS::GetStockPluginsPath()
{
    wxFileName fn;

#if defined( __WXMAC__ )
    fn.Assign( Pgm().GetExecutablePath() );
    fn.AppendDir( wxT( "Contents" ) );
    fn.AppendDir( wxT( "PlugIns" ) );
#elif defined( __WXMSW__ )
    fn.Assign( Pgm().GetExecutablePath() + wxT( "../plugins/" ) );
#else
    // PLUGINDIR = CMAKE_INSTALL_FULL_LIBDIR path is the absolute path
    // corresponding to the install path used for constructing KICAD_USER_PLUGIN
    wxString tfname = wxString::FromUTF8Unchecked( KICAD_PLUGINDIR );
    fn.Assign( tfname, "" );
    fn.AppendDir( "kicad" );    // linux use lowercase
    fn.AppendDir( wxT( "plugins" ) );
#endif

    return fn.GetPathWithSep();
}


wxString PATHS::GetStockPlugins3DPath()
{
    wxFileName fn;

    fn.Assign( PATHS::GetStockPluginsPath() );
    fn.AppendDir( "3d" );

    return fn.GetPathWithSep();
}


wxString PATHS::GetUserCachePath()
{
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetUserCachePath() );
    tmp.AppendDir( KICAD_PATH_STR );
    tmp.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );

    return tmp.GetPathWithSep();
}