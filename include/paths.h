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

#ifndef PATHS_H
#define PATHS_H

#include <kicommon.h>
#include <wx/filename.h>
#include <wx/string.h>

/**
 * @note Do we really need these defined?
 */
#define UNIX_STRING_DIR_SEP wxT( "/" )
#define WIN_STRING_DIR_SEP wxT( "\\" )


// lowercase or pretty case depending on platform
#if defined( __WXMAC__ ) || defined( __WXMSW__ )
#define KICAD_PATH_STR wxT( "KiCad" )
#else
#define KICAD_PATH_STR  wxT( "kicad" )
#endif

/**
 * Helper class to centralize the paths used throughout kicad
 */
class KICOMMON_API PATHS
{
public:

    /**
     * Gets the user path for python scripts
     */
    static wxString GetUserScriptingPath();

    /**
     * Gets the user path for custom templates
     */
    static wxString GetUserTemplatesPath();

    /**
     * Gets the user path for plugins
     */
    static wxString GetUserPluginsPath();

    /**
     * Gets the default path we point users to create projects
     */
    static wxString GetDefaultUserProjectsPath();

    /**
     * Gets the default path we point users to create projects
     */
    static wxString GetDefaultUserSymbolsPath();

    /**
     * Gets the default path we point users to create projects
     */
    static wxString GetDefaultUserFootprintsPath();

    /**
     * Gets the default path we point users to create projects
     */
    static wxString GetDefaultUserDesignBlocksPath();

    /**
     * Gets the default path we point users to create projects
     */
    static wxString GetDefaultUser3DModelsPath();

    /**
     * Gets the stock (install) data path, which is the base path for things like scripting, etc
     */
    static wxString GetStockDataPath( bool aRespectRunFromBuildDir = true );

    /**
     * Gets the stock (install) EDA library data path, which is the base path for
     * templates, schematic symbols, footprints, and 3D models.
     */
    static wxString GetStockEDALibraryPath();

    /**
     * Gets the default path for PCM packages
     */
    static wxString GetDefault3rdPartyPath();

    /**
     * Gets the stock (install) symbols  path
     */
    static wxString GetStockSymbolsPath();

    /**
     * Gets the stock (install) footprints path
     */
    static wxString GetStockFootprintsPath();

    /**
     * Gets the stock (install) footprints path
     */
    static wxString GetStockDesignBlocksPath();

    /**
     * Gets the stock (install) 3dmodels path
     */
    static wxString GetStock3dmodelsPath();

    /**
     * Gets the stock (install) scripting path
     */
    static wxString GetStockScriptingPath();

    /**
     * Gets the stock (install) plugins path
     */
    static wxString GetStockPluginsPath();

    /**
     * Gets the stock (install) 3d viewer plugins path
     */
    static wxString GetStockPlugins3DPath();

    /**
     * Gets the stock (install) demos path
     */
    static wxString GetStockDemosPath();

    /**
     * Gets the stock (install) templates path
     */
    static wxString GetStockTemplatesPath();

    /**
     * Gets the locales translation data path
     */
    static wxString GetLocaleDataPath();

    /**
     * Gets the stock (install) 3d viewer plugins path
     */
    static wxString GetUserCachePath();

    /**
     * Gets the documentation path, which is the base path for help files
     */
    static wxString GetDocumentationPath();

    /**
     * Gets the path used for wxSingleInstanceChecker lock files
     */
    static wxString GetInstanceCheckerPath();

    /**
     * Gets a path to use for user-visible log files
     */
    static wxString GetLogsPath();

    /**
     * Attempts to create a given path if it does not exist
     */
    static bool EnsurePathExists( const wxString& aPath, bool aPathToFile = false );

    /**
     * Ensures/creates user default paths
     */
    static void EnsureUserPathsExist();

#ifdef __WXMAC__
    /**
     * OSX specific function GetOSXKicadUserDataDir
     *
     * @return The macOS specific user data directory for KiCad.
     */
    static wxString GetOSXKicadUserDataDir();

    /**
     * @return The macOS specific machine data directory for KiCad
     */
    static wxString GetOSXKicadMachineDataDir();

    /**
     * @return The macOS specific bundle data directory for KiCad
     */
    static wxString GetOSXKicadDataDir();
#endif

#ifdef _WIN32
    /**
     * @return The directory the font config support files can be found
     */
    static wxString GetWindowsFontConfigDir();


    /**
     * Gets the stock (install) data path, which is the base path for things like scripting, etc
     */
    static wxString GetWindowsBaseSharePath();
#endif

    /**
     * Return the user configuration path used to store KiCad's configuration files.
     *
     * @see calculateUserSettingsPath
     *
     * NOTE: The path is cached at startup, it will never change during program lifetime!
     *
     * @return A string containing the config path for Kicad
     */
    static wxString GetUserSettingsPath();

    /**
     * Determines the base path for user settings files.
     *
     * The configuration path order of precedence is determined by the following criteria:
     *
     * - The value of the KICAD_CONFIG_HOME environment variable
     * - The value of the XDG_CONFIG_HOME environment variable.
     * - The result of the call to wxStandardPaths::GetUserConfigDir() with ".config" appended
     *   as required on Linux builds.
     *
     * @param aIncludeVer will append the current KiCad version if true (default)
     * @param aUseEnv will prefer the base path found in the KICAD_CONFIG_DIR if found (default)
     * @return A string containing the config path for Kicad
     */
    static wxString CalculateUserSettingsPath( bool aIncludeVer = true, bool aUseEnv = true );

    static const wxString& GetExecutablePath();
private:
    // we are a static helper
    PATHS() {}

    /**
     * Gets the user path for the current kicad version which acts as the root for other user paths
     *
     * @param aPath Variable to receive the path
     */
    static void getUserDocumentPath( wxFileName& aPath );

#ifdef _WIN32
    /**
     * Gets the root of the kicad install on Windows specifically.
     * KiCad on Windows has a pseudo posix folder structure contained in its installed folder
     * This retrieves that root for usage in other methods
     */
    static wxString getWindowsKiCadRoot();
#endif
};

#endif
