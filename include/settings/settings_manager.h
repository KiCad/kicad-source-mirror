/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _SETTINGS_MANAGER_H
#define _SETTINGS_MANAGER_H

#include <algorithm>
#include <typeinfo>
#include <core/wx_stl_compat.h> // for wxString hash
#include <settings/color_settings.h>

class COLOR_SETTINGS;
class COMMON_SETTINGS;
class KIWAY;
class PROJECT;
class PROJECT_FILE;
class REPORTER;
class wxSingleInstanceChecker;


class SETTINGS_MANAGER
{
public:
    SETTINGS_MANAGER( bool aHeadless = false );

    ~SETTINGS_MANAGER();

    /**
     * @return true if settings load was successful
     */
    bool IsOK() { return m_ok; }

    /**
     * Associate this setting manager with the given Kiway.
     *
     * @param aKiway is the kiway this settings manager should use
     */
    void SetKiway( KIWAY* aKiway ) { m_kiway = aKiway; }

    /**
     * Takes ownership of the pointer passed in
     * @param aSettings is a settings object to register
     * @return a handle to the owned pointer
     */
    JSON_SETTINGS* RegisterSettings( JSON_SETTINGS* aSettings, bool aLoadNow = true );

    void Load();

    void Load( JSON_SETTINGS* aSettings );

    void Save();

    void Save( JSON_SETTINGS* aSettings );

    /**
     * If the given settings object is registered, save it to disk and unregister it
     * @param aSettings is the object to release
     */
    void FlushAndRelease( JSON_SETTINGS* aSettings, bool aSave = true );

    /**
     * Returns a handle to the a given settings by type
     * If the settings have already been loaded, returns the existing pointer.
     * If the settings have not been loaded, creates a new object owned by the
     * settings manager and returns a pointer to it.
     *
     * @tparam AppSettings is a type derived from APP_SETTINGS_BASE
     * @param aLoadNow is true to load the registered file from disk immediately
     * @return a pointer to a loaded settings object
     */
    template<typename AppSettings>
    AppSettings* GetAppSettings( bool aLoadNow = true )
    {
        AppSettings* ret      = nullptr;
        size_t       typeHash = typeid( AppSettings ).hash_code();

         if( m_app_settings_cache.count( typeHash ) )
            ret = dynamic_cast<AppSettings*>( m_app_settings_cache.at( typeHash ) );

        if( ret )
            return ret;

        auto it = std::find_if( m_settings.begin(), m_settings.end(),
                                []( const std::unique_ptr<JSON_SETTINGS>& aSettings )
                                {
                                    return dynamic_cast<AppSettings*>( aSettings.get() );
                                } );

        if( it != m_settings.end() )
        {
            ret = dynamic_cast<AppSettings*>( it->get() );
        }
        else
        {
            try
            {
                ret = static_cast<AppSettings*>( RegisterSettings( new AppSettings, aLoadNow ) );
            }
            catch( ... )
            {
            }

        }

        m_app_settings_cache[typeHash] = ret;

        return ret;
    }

    /**
     * Retrieves a color settings object that applications can read colors from.
     * If the given settings file cannot be found, returns the default settings.
     *
     * @param aName is the name of the color scheme to load
     * @return a loaded COLOR_SETTINGS object
     */
    COLOR_SETTINGS* GetColorSettings( const wxString& aName = "user" );

    std::vector<COLOR_SETTINGS*> GetColorSettingsList()
    {
        std::vector<COLOR_SETTINGS*> ret;

        for( const std::pair<const wxString, COLOR_SETTINGS*>& entry : m_color_settings )
            ret.push_back( entry.second );

        std::sort( ret.begin(), ret.end(), []( COLOR_SETTINGS* a, COLOR_SETTINGS* b )
                                           {
                                               return a->GetName() < b->GetName();
                                           } );

        return ret;
    }

    /**
     * Safely saves a COLOR_SETTINGS to disk, preserving any changes outside the given namespace.
     *
     * A color settings namespace is one of the top-level JSON objects like "board", etc.
     * This will perform a read-modify-write
     *
     * @param aSettings is a pointer to a valid COLOR_SETTINGS object managed by SETTINGS_MANAGER
     * @param aNamespace is the namespace of settings to save
     */
    void SaveColorSettings( COLOR_SETTINGS* aSettings, const std::string& aNamespace = "" );

    /**
     * Registers a new color settings object with the given filename
     * @param aFilename is the location to store the new settings object
     * @return a pointer to the new object
     */
    COLOR_SETTINGS* AddNewColorSettings( const wxString& aFilename );

    /**
     * Returns a color theme for storing colors migrated from legacy (5.x and earlier) settings,
     * creating the theme if necessary.  This theme will be called "user.json" / "User".
     * @return the color settings to be used for migrating legacy settings
     */
    COLOR_SETTINGS* GetMigratedColorSettings();

    /**
     * Retrieves the common settings shared by all applications
     * @return a pointer to a loaded COMMON_SETTINGS
     */
    COMMON_SETTINGS* GetCommonSettings() const { return m_common_settings; }

    /**
     * Returns the path a given settings file should be loaded from / stored to.
     * @param aSettings is the settings object
     * @return a path based on aSettings->m_location
     */
    wxString GetPathForSettingsFile( JSON_SETTINGS* aSettings );

    /**
     * Handles the initialization of the user settings directory and migration from previous
     * KiCad versions as needed.
     *
     * This method will check for the existence of the user settings path for this KiCad version.
     * If it exists, settings load will proceed normally using that path.
     *
     * If that directory is empty or does not exist, the migration wizard will be launched, which
     * will give users the option to migrate settings from a previous KiCad version (if one is
     * found), manually specify a directory to migrate fromm, or start with default settings.
     *
     * @return true if migration was successful or not necessary, false otherwise
     */
    bool MigrateIfNeeded();

    /**
     * Helper for DIALOG_MIGRATE_SETTINGS to specify a source for migration
     * @param aSource is a directory containing settings files to migrate from (can be empty)
     */
    void SetMigrationSource( const wxString& aSource ) { m_migration_source = aSource; }

    void SetMigrateLibraryTables( bool aMigrate = true ) { m_migrateLibraryTables = aMigrate; }

    /**
     * Retrieves the name of the most recent previous KiCad version that can be found in the
     * user settings directory.  For legacy versions (5.x, and 5.99 builds before this code was
     * written), this will return "5.x"
     *
     * @param aName is filled with the name of the previous version, if one exists
     * @return true if a previous version to migrate from exists
     */
    bool GetPreviousVersionPaths( std::vector<wxString>* aName = nullptr );

    /**
     * Re-scans the color themes directory, reloading any changes it finds.
     */
    void ReloadColorSettings();

    /**
     * Loads a project or sets up a new project with a specified path
     * @param aFullPath is the full path to the project
     * @param aSetActive if true will set the loaded project as the active project
     * @return true if the PROJECT_FILE was successfully loaded from disk
     */
    bool LoadProject( const wxString& aFullPath, bool aSetActive = true );

    /**
     * Saves, unloads and unregisters the given PROJECT
     * @param aProject is the project object to unload
     * @param aSave if true will save the project before unloading
     * @return true if the PROJECT file was successfully saved
     */
    bool UnloadProject( PROJECT* aProject, bool aSave = true );

    /**
     * Helper for checking if we have a project open
     * TODO: This should be deprecated along with Prj() once we support multiple projects fully
     * @return true if a call to Prj() will succeed
     */
    bool IsProjectOpen() const;

    /**
     * A helper while we are not MDI-capable -- return the one and only project
     * @return the loaded project
     */
    PROJECT& Prj() const;

    /**
     * Retrieves a loaded project by name
     * @param aFullPath is the full path including name and extension to the project file
     * @return a pointer to the project if loaded, or nullptr
     */
    PROJECT* GetProject( const wxString& aFullPath ) const;

    /**
     * @return a list of open projects
     */
    std::vector<wxString> GetOpenProjects() const;

    /**
     * Saves a loaded project.
     * @param aFullPath is the project name to save.  If empty, will save the first loaded project.
     * @return true if save was successful
     */
    bool SaveProject( const wxString& aFullPath = wxEmptyString );

    /**
     * Sets the currently loaded project path and saves it (pointers remain valid)
     * Note that this will not modify the read-only state of the project, so it will have no effect
     * if the project is marked as read-only!
     * @param aFullPath is the full filename to set for the project
     */
    void SaveProjectAs( const wxString& aFullPath );

    /**
     * Saves a copy of the current project under the given path.  Will save the copy even if the
     * current project is marked as read-only.
     */
    void SaveProjectCopy( const wxString& aFullPath );

    /**
     * @return the full path to where project backups should be stored
     */
    wxString GetProjectBackupsPath() const;

    /**
     * Creates a backup archive of the current project
     * @param aReporter is used for progress reporting
     * @return true if everything succeeded
     */
    bool BackupProject( REPORTER& aReporter ) const;

    /**
     * Calls BackupProject if a new backup is needed according to the current backup policy.
     * @param aReporter is used for progress reporting
     * @return if everything succeeded
     */
    bool TriggerBackupIfNeeded( REPORTER& aReporter ) const;

    /**
     * Checks if a given path is probably a valid KiCad configuration directory.
     * Actually it just checks if a file called "kicad_common" exists, because that's probably
     * good enough for now.
     *
     * @param aPath is the path to check
     * @return true if the path contains KiCad settings
     */
    static bool IsSettingsPathValid( const wxString& aPath );

    /**
     * Returns the path where color scheme files are stored; creating it if missing
     * (normally ./colors/ under the user settings path)
     */
    static wxString GetColorSettingsPath();

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
     * Parses the current KiCad build version and extracts the major and minor revision to use
     * as the name of the settings directory for this KiCad version.
     *
     * @return a string such as "5.1"
     */
    static std::string GetSettingsVersion();

private:

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
    static wxString calculateUserSettingsPath( bool aIncludeVer = true, bool aUseEnv = true );

    /**
     * Compares two settings versions, like "5.99" and "6.0"
     * @return -1 if aFirst is older than aSecond, 1 if aFirst is newer than aSecond, 0 otherwise
     */
    static int compareVersions( const std::string& aFirst, const std::string& aSecond );

    /**
     * Extracts the numeric version from a given settings string
     * @param aVersionString is the string to split at the "."
     * @param aMajor will store the first part
     * @param aMinor will store the second part
     * @return true if extraction succeeded
     */
    static bool extractVersion( const std::string& aVersionString, int* aMajor, int* aMinor );

    /**
     * Attempts to load a color theme by name (the color theme directory and .json ext are assumed)
     * @param aName is the filename of the color theme (without the extension or path)
     * @return the loaded settings, or nullptr if load failed
     */
    COLOR_SETTINGS* loadColorSettingsByName( const wxString& aName );

    COLOR_SETTINGS* registerColorSettings( const wxString& aFilename );

    void loadAllColorSettings();

    /**
     * Registers a PROJECT_FILE and attempts to load it from disk
     * @param aProject is the project object to load the file for
     * @return true if the PROJECT_FILE was successfully loaded
     */
    bool loadProjectFile( PROJECT& aProject );

    /**
     * Optionally saves, and then unloads and unregisters the given PROJECT_FILE
     * @param aProject is the project object to unload the file for
     * @param aSave if true will save the project file before unloading
     * @return true if the PROJECT file was successfully saved
     */
    bool unloadProjectFile( PROJECT* aProject, bool aSave );

private:

    /// True if running outside a UI context
    bool m_headless;

    /// The kiway this settings manager interacts with
    KIWAY* m_kiway;

    std::vector<std::unique_ptr<JSON_SETTINGS>> m_settings;

    std::unordered_map<wxString, COLOR_SETTINGS*> m_color_settings;

    /// Cache for app settings
    std::unordered_map<size_t, JSON_SETTINGS*> m_app_settings_cache;

    // Convenience shortcut
    COMMON_SETTINGS* m_common_settings;

    wxString m_migration_source;

    /// If true, the symbol and footprint library tables will be migrated from the previous version
    bool m_migrateLibraryTables;

    /// True if settings loaded successfully at construction
    bool m_ok;

    /// Loaded projects (ownership here)
    std::vector<std::unique_ptr<PROJECT>> m_projects_list;

    /// Loaded projects, mapped according to project full name
    std::map<wxString, PROJECT*> m_projects;

    /// Loaded project files, mapped according to project full name
    std::map<wxString, PROJECT_FILE*> m_project_files;

    /// Lock for loaded project (expand to multiple once we support MDI)
    std::unique_ptr<wxSingleInstanceChecker> m_project_lock;

    static wxString backupDateTimeFormat;
};

#endif
