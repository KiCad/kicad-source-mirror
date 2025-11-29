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

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>

#include <kiplatform/environment.h>
#include <paths.h>
#include <config.h>
#include <build_version.h>
#include <macros.h>
#include <wx_filename.h>


void PATHS::getUserDocumentPath( wxFileName& aPath )
{
    wxString envPath;

    if( wxGetEnv( wxT( "KICAD_DOCUMENTS_HOME" ), &envPath ) )
        aPath.AssignDir( envPath );
    else
        aPath.AssignDir( KIPLATFORM::ENV::GetDocumentsPath() );

    aPath.AppendDir( KICAD_PATH_STR );
    aPath.AppendDir( GetMajorMinorVersion().ToStdString() );
}


wxString PATHS::GetUserPluginsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "plugins" ) );

    return tmp.GetPath();
}


wxString PATHS::GetUserScriptingPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "scripting" ) );

    return tmp.GetPath();
}


wxString PATHS::GetUserTemplatesPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "template" ) );

    return tmp.GetPathWithSep();
}


wxString PATHS::GetDefaultUserSymbolsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "symbols" ) );

    return tmp.GetPath();
}


wxString PATHS::GetDefaultUserFootprintsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "footprints" ) );

    return tmp.GetPath();
}


wxString PATHS::GetDefaultUserDesignBlocksPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "blocks" ) );

    return tmp.GetPath();
}


wxString PATHS::GetDefaultUser3DModelsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "3dmodels" ) );

    return tmp.GetPath();
}


wxString PATHS::GetDefault3rdPartyPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "3rdparty" ) );

    return tmp.GetAbsolutePath();
}


wxString PATHS::GetDefaultUserProjectsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "projects" ) );

    return tmp.GetPath();
}


#if !defined( __WXMAC__ ) && !defined( __WXMSW__ )
/**
 * Get the CMake build root directory for the current executable
 * (which assumes the executable is in a build directory).
 *
 * This is done because not all executable are located at the same
 * depth in the build directory.
 */
static wxString getBuildDirectoryRoot()
{
    // We don't have a perfect way to spot a build directory (e.g. when archived as artifacts in
    // CI) but we can assume that the build directory will have a schemas directory that contains
    // JSON files, as that's one of the things that we use this path for.
    const auto looksLikeBuildDir = []( const wxFileName& aPath ) -> bool
    {
        const wxDir schema_dir( aPath.GetPathWithSep() + wxT( "schemas" ) );

        if( !schema_dir.IsOpened() )
            return false;

        wxString   filename;
        const bool found = schema_dir.GetFirst( &filename, wxT( "*.json" ), wxDIR_FILES );
        return found;
    };

    const wxString execPath = PATHS::GetExecutablePath();
    wxFileName     fn = execPath;

    // Climb the directory tree until we find a directory that looks like a build directory
    // Normally we expect to climb one or two levels only.
    while( fn.GetDirCount() > 0 && !looksLikeBuildDir( fn ) )
    {
        fn.RemoveLastDir();
    }

    wxASSERT_MSG(
            fn.GetDirCount() > 0,
            wxString::Format( wxT( "Could not find build root directory above %s" ), execPath ) );

    return fn.GetPath();
}
#endif


wxString PATHS::GetStockDataPath( bool aRespectRunFromBuildDir )
{
    wxString path;

    if( aRespectRunFromBuildDir && wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        // Allow debugging from build dir by placing relevant files/folders in the build root
#if defined( __WXMAC__ )
        wxFileName fn = wxStandardPaths::Get().GetExecutablePath();

        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        path = fn.GetPath();
#elif defined( __WXMSW__ )
        path = getWindowsKiCadRoot();
#else
        path = getBuildDirectoryRoot();
#endif
    }
    else if( wxGetEnv( wxT( "KICAD_STOCK_DATA_HOME" ), &path ) && !path.IsEmpty() )
    {
        return path;
    }
    else
    {
#if defined( __WXMAC__ )
        path = GetOSXKicadDataDir();
#elif defined( __WXMSW__ )
        path = getWindowsKiCadRoot() + wxT( "share/kicad" );
#else
        path = wxString::FromUTF8Unchecked( KICAD_DATA );
#endif
    }

    return path;
}


#ifdef _WIN32

wxString PATHS::GetWindowsBaseSharePath()
{
    return getWindowsKiCadRoot() + wxT( "share\\" );
}

#endif


wxString PATHS::GetStockEDALibraryPath()
{
    wxString path;

#if defined( __WXMAC__ )
    path = GetOSXKicadMachineDataDir();
#elif defined( __WXMSW__ )
    path = GetStockDataPath( false );
#else
    path = wxString::FromUTF8Unchecked( KICAD_LIBRARY_DATA );
#endif

    return path;
}


wxString PATHS::GetStockSymbolsPath()
{
    wxString path;

    path = GetStockEDALibraryPath() + wxT( "/symbols" );

    return path;
}


wxString PATHS::GetStockFootprintsPath()
{
    wxString path;

    path = GetStockEDALibraryPath() + wxT( "/footprints" );

    return path;
}


wxString PATHS::GetStockDesignBlocksPath()
{
    wxString path;

    path = GetStockEDALibraryPath() + wxT( "/blocks" );

    return path;
}


wxString PATHS::GetStock3dmodelsPath()
{
    wxString path;

    path = GetStockEDALibraryPath() + wxT( "/3dmodels" );

    return path;
}


wxString PATHS::GetStockScriptingPath()
{
    wxString path;

    path = GetStockDataPath() + wxT( "/scripting" );

    return path;
}


wxString PATHS::GetStockTemplatesPath()
{
    wxString path;

    path = GetStockEDALibraryPath() + wxT( "/template" );

    return path;
}


wxString PATHS::GetLocaleDataPath()
{
    wxString path;

    path = GetStockDataPath() + wxT( "/internat" );

    return path;
}


wxString PATHS::GetStockPluginsPath()
{
    wxFileName fn;

#if defined( __WXMSW__ )
    fn.AssignDir( GetExecutablePath() );
    fn.AppendDir( wxT( "scripting" ) );
#else
    fn.AssignDir( PATHS::GetStockDataPath( false ) );
#endif
    fn.AppendDir( wxT( "plugins" ) );

    return fn.GetPathWithSep();
}


wxString PATHS::GetStockPlugins3DPath()
{
    wxFileName fn;

#if defined( __WXMSW__ )
    if( wxGetEnv( wxT( "KICAD_RUN_FROM_BUILD_DIR" ), nullptr ) )
    {
        fn.AssignDir( getWindowsKiCadRoot() );
    }
    else
    {
        fn.AssignDir( GetExecutablePath() );
    }

    fn.AppendDir( wxT( "plugins" ) );
#elif defined( __WXMAC__ )
    fn.Assign( wxStandardPaths::Get().GetPluginsDir(), wxEmptyString );

    // This must be mapped to main bundle for everything but kicad.app
    const wxArrayString dirs = fn.GetDirs();

    // Check if we are the main kicad binary.  in this case, the path will be
    //     /path/to/bundlename.app/Contents/PlugIns
    // If we are an aux binary, the path will be something like
    //     /path/to/bundlename.app/Contents/Applications/<standalone>.app/Contents/PlugIns
    if( dirs.GetCount() >= 6 &&
        dirs[dirs.GetCount() - 4] == wxT( "Applications" ) &&
        dirs[dirs.GetCount() - 6].Lower().EndsWith( wxT( "app" ) ) )
    {
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.RemoveLastDir();
        fn.AppendDir( wxT( "PlugIns" ) );
    }
#else
    wxString envPath;

    // AppImages have a different path to the plugins, otherwise we end up with host sytem
    // plugins being loaded.
    if( wxGetEnv( wxT( "APPDIR" ), &envPath ) )
    {
        fn.Assign( envPath, wxEmptyString );
        fn.AppendDir( wxT( "usr" ) );
        fn.AppendDir( wxT( "lib" ) );
        fn.AppendDir( wxT( "x86_64-linux-gnu" ) );
    }
    else
    {
        // KICAD_PLUGINDIR = CMAKE_INSTALL_FULL_LIBDIR path is the absolute path
        // corresponding to the install path used for constructing KICAD_USER_PLUGIN
        wxString tfname = wxString::FromUTF8Unchecked( KICAD_PLUGINDIR );
        fn.Assign( tfname, "" );
    }

    fn.AppendDir( wxT( "kicad" ) );
    fn.AppendDir( wxT( "plugins" ) );
#endif

    fn.AppendDir( wxT( "3d" ) );

    return fn.GetPathWithSep();
}


wxString PATHS::GetStockDemosPath()
{
    wxFileName fn;

    fn.AssignDir( PATHS::GetStockDataPath( false ) );
    fn.AppendDir( wxT( "demos" ) );

    return fn.GetPathWithSep();
}


wxString PATHS::GetUserCachePath()
{
    wxString   envPath;
    wxFileName tmp;

    tmp.AssignDir( KIPLATFORM::ENV::GetUserCachePath() );

    // Use KICAD_CACHE_HOME to allow the user to force a specific cache path.
    if( wxGetEnv( wxT( "KICAD_CACHE_HOME" ), &envPath ) && !envPath.IsEmpty() )
    {
        // Override the assignment above with KICAD_CACHE_HOME
        tmp.AssignDir( envPath );
    }

    tmp.AppendDir( KICAD_PATH_STR );
    tmp.AppendDir( GetMajorMinorVersion().ToStdString() );

    return tmp.GetPathWithSep();
}


wxString PATHS::GetDocumentationPath()
{
    wxString path;

#if defined( __WXMAC__ )
    path = GetOSXKicadDataDir();
#elif defined( __WXMSW__ )
    path = getWindowsKiCadRoot() + wxT( "share/doc/kicad" );
#else
    path = wxString::FromUTF8Unchecked( KICAD_DOCS );
#endif

    return path;
}


wxString PATHS::GetInstanceCheckerPath()
{
    wxFileName path;
    path.AssignDir( wxStandardPaths::Get().GetTempDir() );
    path.AppendDir( "org.kicad.kicad" );
    path.AppendDir( "instances" );
    return path.GetPathWithSep();
}


wxString PATHS::GetLogsPath()
{
    wxFileName tmp;
    getUserDocumentPath( tmp );

    tmp.AppendDir( wxT( "logs" ) );

    return tmp.GetPath();
}


bool PATHS::EnsurePathExists( const wxString& aPath, bool aPathToFile )
{
    wxString   pathString = aPath;
    if( !aPathToFile )
    {
        // ensures the path is treated fully as directory
        pathString += wxFileName::GetPathSeparator();
    }

    wxFileName path( pathString );
    if( !path.MakeAbsolute() )
    {
        return false;
    }

    if( !wxFileName::DirExists( path.GetPath() ) )
    {
        if( !wxFileName::Mkdir( path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            return false;
        }
    }

    return true;
}


void PATHS::EnsureUserPathsExist()
{
    EnsurePathExists( GetUserCachePath() );
    EnsurePathExists( GetUserPluginsPath() );
    EnsurePathExists( GetUserScriptingPath() );
    EnsurePathExists( GetUserTemplatesPath() );
    EnsurePathExists( GetDefaultUserProjectsPath() );
    EnsurePathExists( GetDefaultUserSymbolsPath() );
    EnsurePathExists( GetDefaultUserFootprintsPath() );
    EnsurePathExists( GetDefaultUser3DModelsPath() );
    EnsurePathExists( GetDefault3rdPartyPath() );

#ifdef _WIN32
    // Warn about Controlled folder access
    wxFileName tmp;
    getUserDocumentPath( tmp );

    if( !tmp.DirExists() )
    {
        wxString msg = wxString::Format(
                _( "KiCad was unable to use '%s'.\n"
                   "\n"
                   "1. Disable 'Controlled folder access' in Windows settings or Group Policy\n"
                   "2. Make sure no other antivirus software interferes with KiCad\n"
                   "3. Make sure you have correct permissions set up" ),
                tmp.GetPath() );

        wxMessageBox( msg, _( "Warning" ), wxICON_WARNING );
    }
#endif
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
    udir.AppendDir(  wxT( "kicad" ) );

    return udir.GetPath();
}


wxString PATHS::GetOSXKicadMachineDataDir()
{
    // 6.0 forward:  Same as the main data dir
    return GetOSXKicadDataDir();
}


wxString PATHS::GetOSXKicadDataDir()
{
    // According to wxWidgets documentation for GetDataDir:
    // Mac: appname.app/Contents/SharedSupport bundle subdirectory
    wxFileName ddir( wxStandardPaths::Get().GetDataDir(), wxEmptyString );

    // This must be mapped to main bundle for everything but kicad.app
    const wxArrayString dirs = ddir.GetDirs();

    // Check if we are the main kicad binary.  in this case, the path will be
    //     /path/to/bundlename.app/Contents/SharedSupport
    // If we are an aux binary, the path will be something like
    //     /path/to/bundlename.app/Contents/Applications/<standalone>.app/Contents/SharedSupport
    if( dirs.GetCount() >= 6 &&
        dirs[dirs.GetCount() - 4] == wxT( "Applications" ) &&
        dirs[dirs.GetCount() - 6].Lower().EndsWith( wxT( "app" ) ) )
    {
        ddir.RemoveLastDir();
        ddir.RemoveLastDir();
        ddir.RemoveLastDir();
        ddir.RemoveLastDir();
        ddir.AppendDir( wxT( "SharedSupport" ) );
    }

    return ddir.GetPath();
}
#endif


#ifdef _WIN32
wxString PATHS::GetWindowsFontConfigDir()
{
    wxFileName fn;
    fn.AssignDir( getWindowsKiCadRoot() );
    fn.AppendDir( wxS( "etc" ) );
    fn.AppendDir( wxS( "fonts" ) );

    return fn.GetPathWithSep();
}


wxString PATHS::getWindowsKiCadRoot()
{
    wxFileName root( GetExecutablePath() +  wxT( "/../" ) );
    root.MakeAbsolute();

    return root.GetPathWithSep();
}
#endif


wxString PATHS::GetUserSettingsPath()
{
    static wxString user_settings_path;

    if( user_settings_path.empty() )
        user_settings_path = CalculateUserSettingsPath();

    return user_settings_path;
}


wxString PATHS::CalculateUserSettingsPath( bool aIncludeVer, bool aUseEnv )
{
    wxFileName cfgpath;

    // http://docs.wxwidgets.org/3.0/classwx_standard_paths.html#a7c7cf595d94d29147360d031647476b0

    wxString envstr;
    if( aUseEnv && wxGetEnv( wxT( "KICAD_CONFIG_HOME" ), &envstr ) && !envstr.IsEmpty() )
    {
        // Override the assignment above with KICAD_CONFIG_HOME
        cfgpath.AssignDir( envstr );
    }
    else
    {
        cfgpath.AssignDir( KIPLATFORM::ENV::GetUserConfigPath() );

        cfgpath.AppendDir( TO_STR( KICAD_CONFIG_DIR ) );
    }

    if( aIncludeVer )
        cfgpath.AppendDir( GetMajorMinorVersion().ToStdString() );

    return cfgpath.GetPath();
}


const wxString& PATHS::GetExecutablePath()
{
    static wxString exe_path;

    if( exe_path.empty() )
    {
        wxString bin_dir = wxStandardPaths::Get().GetExecutablePath();

#ifdef __WXMAC__
        // On OSX GetExecutablePath() will always point to main
        // bundle directory, e.g., /Applications/kicad.app/

        wxFileName fn( bin_dir );
        WX_FILENAME::ResolvePossibleSymlinks( fn );

        if( fn.GetName() == wxT( "kicad" ) || fn.GetName() == wxT( "kicad-cli" ) )
        {
            // kicad launcher, so just remove the Contents/MacOS part
            fn.RemoveLastDir();
            fn.RemoveLastDir();
        }
        else
        {
            // standalone binaries live in Contents/Applications/<standalone>.app/Contents/MacOS
            fn.RemoveLastDir();
            fn.RemoveLastDir();
            fn.RemoveLastDir();
            fn.RemoveLastDir();
            fn.RemoveLastDir();
        }

        bin_dir = fn.GetPath() + wxT( "/" );
#else
        // Use unix notation for paths. I am not sure this is a good idea,
        // but it simplifies compatibility between Windows and Unices.
        // However it is a potential problem in path handling under Windows.
        bin_dir.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

        // Remove file name form command line:
        while( bin_dir.Last() != '/' && !bin_dir.IsEmpty() )
            bin_dir.RemoveLast();
#endif
        exe_path = bin_dir;
    }

    return exe_path;
}
