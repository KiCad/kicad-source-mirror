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

#ifndef PATHS_H
#define PATHS_H

#include <wx/filename.h>
#include <wx/string.h>

/**
 * Helper class to centralize the paths used throughout kicad
 */
class PATHS
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
     * Gets the user path for 3d viewer plugin
     */
    static wxString GetUserPlugins3DPath();

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
     * Gets the stock (install) scripting path
     */
    static wxString GetStockScriptingPath();

    /**
     * Gets the stock (install) plugins path
     */
    static wxString GetStockPluginsPath();

    /**
     * Gets the stock (install) 3d viewer pluginspath
     */
    static wxString GetStockPlugins3DPath();

    /**
     * Gets the stock (install) 3d viewer pluginspath
     */
    static wxString GetUserCachePath();

    /**
     * Gets the documentation path, which is the base path for help files
     */
    static wxString GetDocumentationPath();

    /**
     * Attempts to create a given path if it does not exist
     */
    static bool EnsurePathExists( const wxString& aPath );

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

private:
    // we are a static helper
    PATHS() {}

    /**
     * Gets the user path for the current kicad version which acts as the root for other user paths
     *
     * @param aPath Variable to receive the path
     */
    static void getUserDocumentPath( wxFileName& aPath );

#ifdef __WXWINDOWS__
    /**
     * Gets the root of the kicad install on Windows specifically.
     * KiCad on Windows has a pseudo posix folder structure contained in its installed folder
     * This retrieves that root for usage in other methods
     */
    static wxString getWindowsKiCadRoot();
#endif
};

#endif
