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


void PATHS::getUserDocumentPath( wxFileName& aPath )
{
    wxString envPath;

    if( wxGetEnv( wxT( "KICAD_DOCUMENTS_HOME" ), &envPath ) )
        aPath.AssignDir( envPath );
    else
        aPath.AssignDir( KIPLATFORM::ENV::GetDocumentsPath() );

    aPath.AppendDir( KICAD_PATH_STR );
    aPath.AppendDir( SETTINGS_MANAGER::GetSettingsVersion() );
}


wxString PATHS::GetUserPluginsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( "plugins" );

    return tmp.GetPath();
}


wxString PATHS::GetUserPlugins3DPath()
{
    wxFileName tmp;

    tmp.AssignDir( PATHS::GetUserPluginsPath() );
    tmp.AppendDir( "3d" );

    return tmp.GetPath();
}


wxString PATHS::GetUserScriptingPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( "scripting" );

    return tmp.GetPath();
}


wxString PATHS::GetUserTemplatesPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( "template" );

    return tmp.GetPathWithSep();
}


wxString PATHS::GetDefaultUserSymbolsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( "symbols" );

    return tmp.GetPath();
}


wxString PATHS::GetDefaultUserFootprintsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( "footprints" );

    return tmp.GetPath();
}


wxString PATHS::GetDefaultUser3DModelsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( "3dmodels" );

    return tmp.GetPath();
}


wxString PATHS::GetDefaultUserProjectsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( "projects" );

    return tmp.GetPath();
}


wxString PATHS::GetStockDataPath( bool aRespectRunFromBuildDir )
{
    wxString path;

    if( aRespectRunFromBuildDir && wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        // Allow debugging from build dir by placing relevant files/folders in the build root
        path = Pgm().GetExecutablePath() + wxT( ".." );
    }
    else
    {
#if defined( __WXMAC__ )
        path = GetOSXKicadDataDir();
#elif defined( __WXMSW__ )
        path = Pgm().GetExecutablePath() + wxT( "../share/kicad" );
#else
        path = wxString::FromUTF8Unchecked( KICAD_DATA );
#endif
    }

    return path;
}


wxString PATHS::GetStockScriptingPath()
{
    wxString path;

    path = GetStockDataPath() + wxT( "/scripting" );

    return path;
}


wxString PATHS::GetStockPluginsPath()
{
    wxFileName fn;

#if defined( __WXMSW__ )
    fn.AssignDir( Pgm().GetExecutablePath() );
#else
    fn.AssignDir( PATHS::GetStockDataPath( false ) );
#endif
    fn.AppendDir( wxT( "plugins" ) );

    return fn.GetPathWithSep();
}


wxString PATHS::GetStockPlugins3DPath()
{
    wxFileName fn;

#ifdef __WXGTK__
    // KICAD_PLUGINDIR = CMAKE_INSTALL_FULL_LIBDIR path is the absolute path
    // corresponding to the install path used for constructing KICAD_USER_PLUGIN
    wxString tfname = wxString::FromUTF8Unchecked( KICAD_PLUGINDIR );
    fn.Assign( tfname, "" );
    fn.AppendDir( wxT( "kicad" ) );
    fn.AppendDir( wxT( "plugins" ) );
#elif defined( __WXMAC__ )
    fn.Assign( wxStandardPaths::Get().GetPluginsDir(), wxEmptyString );
#else
    fn.Assign( PATHS::GetStockPluginsPath() );
#endif

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


bool PATHS::EnsurePathExists( const wxString& aPath )
{
    wxFileName path( aPath );
    if( !path.Normalize() )
    {
        return false;
    }

    if( !wxFileName::DirExists( aPath ) )
    {
        if( !wxFileName::Mkdir( aPath, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            return false;
        }
    }

    return true;
}


void PATHS::EnsureUserPathsExist()
{
    EnsurePathExists( GetUserPluginsPath() );
    EnsurePathExists( GetUserPlugins3DPath() );
    EnsurePathExists( GetUserScriptingPath() );
    EnsurePathExists( GetUserTemplatesPath() );
    EnsurePathExists( GetDefaultUserProjectsPath() );
    EnsurePathExists( GetDefaultUserSymbolsPath() );
    EnsurePathExists( GetDefaultUserFootprintsPath() );
    EnsurePathExists( GetDefaultUser3DModelsPath() );
}


#ifdef __WXMAC__
wxString PATHS::GetOSXKicadUserDataDir()
{
    // According to wxWidgets documentation for GetUserDataDir:
    // Mac: ~/Library/Application Support/appname
    wxFileName udir( wxStandardPaths::Get().GetUserDataDir(), wxEmptyString );

    // Since appname is different if started via launcher or standalone binary
    // map all to "kicad" here
    udir.RemoveLastDir();
    udir.AppendDir( "kicad" );

    return udir.GetPath();
}


wxString PATHS::GetOSXKicadMachineDataDir()
{
    return wxT( "/Library/Application Support/kicad" );
}


wxString PATHS::GetOSXKicadDataDir()
{
    // According to wxWidgets documentation for GetDataDir:
    // Mac: appname.app/Contents/SharedSupport bundle subdirectory
    wxFileName ddir( wxStandardPaths::Get().GetDataDir(), wxEmptyString );

    // This must be mapped to main bundle for everything but kicad.app
    const wxArrayString dirs = ddir.GetDirs();
    if( dirs[dirs.GetCount() - 3].Lower() != wxT( "kicad.app" ) )
    {
        // Bundle structure resp. current path is
        //   kicad.app/Contents/Applications/<standalone>.app/Contents/SharedSupport
        // and will be mapped to
        //   kicad.app/Contents/SharedSupprt
        ddir.RemoveLastDir();
        ddir.RemoveLastDir();
        ddir.RemoveLastDir();
        ddir.RemoveLastDir();
        ddir.AppendDir( wxT( "SharedSupport" ) );
    }

    return ddir.GetPath();
}
#endif
