/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#ifndef _SETTINGS_MANAGER_H
#define _SETTINGS_MANAGER_H

#include <algorithm>
#include <mutex>
#include <typeinfo>
#include <core/wx_stl_compat.h> // for wxString hash
#include <settings/color_settings.h>
#include <pgm_base.h>

class COLOR_SETTINGS;
class COMMON_SETTINGS;
class KIWAY;
class PROJECT;
class PROJECT_FILE;
class REPORTER;
class wxSingleInstanceChecker;
class wxFileName;
class LOCKFILE;


/// Project settings path will be <projectname> + this
#define PROJECT_BACKUPS_DIR_SUFFIX wxT( "-backups" )

#define DEFAULT_THEME wxString( wxT( "user" ) )


class KICOMMON_API SETTINGS_MANAGER
{
public:
    SETTINGS_MANAGER();

    ~SETTINGS_MANAGER();

    /**
     * @return true if the settings directory for this version of KiCad exists and has at least a
     * common settings file (kicad_common.json).  Used to know whether or not the first-run wizard
     * needs to be shown.
     */
    bool SettingsDirectoryValid() const;

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
     * Take ownership of the pointer passed in.
     *
     * @param aSettings is a settings object to register.
     * @return a handle to the owned pointer.
     */
    template<typename T>
    T* RegisterSettings( T* aSettings, bool aLoadNow = true )
    {
        return static_cast<T*>( registerSettings( aSettings, aLoadNow ) );
    }

    void Load();

    void Load( JSON_SETTINGS* aSettings );

    void Save();

    void Save( JSON_SETTINGS* aSettings );

    /**
     * If the given settings object is registered, save it to disk and unregister it.
     *
     * @param aSettings is the object to release
     */
    void FlushAndRelease( JSON_SETTINGS* aSettings, bool aSave = true );

    /**
     * Reset all program settings to defaults.
     */
    void ResetToDefaults();

    /**
     * Clear saved file history from all settings files.
     */
    void ClearFileHistory();

    /**
     * Return a handle to the a given settings by type.
     *
     * If the settings have already been loaded, returns the existing pointer.
     * If the settings have not been loaded, creates a new object owned by the
     * settings manager and returns a pointer to it.
     *
     * @tparam T is a type derived from APP_SETTINGS_BASE.
     * @param aFilename is used to find the correct settings under clang (where
     *                  RTTI doesn't work across compile boundaries).
     * @return a pointer to a loaded settings object.
     */
    template<typename T>
    T* GetAppSettings( const char* aFilename )
    {
        std::lock_guard lock( m_app_settings_mutex );

        T*     ret      = nullptr;
        size_t typeHash = typeid( T ).hash_code();

         if( m_app_settings_cache.count( typeHash ) )
            ret = static_cast<T*>( m_app_settings_cache.at( typeHash ) );

        if( ret )
            return ret;

#if defined(__clang__)
        auto it = std::find_if( m_settings.begin(), m_settings.end(),
                                [&]( const std::unique_ptr<JSON_SETTINGS>& aSettings )
                                {
                                    return aSettings->GetFilename() == aFilename;
                                } );
#else
        auto it = std::find_if( m_settings.begin(), m_settings.end(),
                                []( const std::unique_ptr<JSON_SETTINGS>& aSettings )
                                {
                                    return dynamic_cast<T*>( aSettings.get() );
                                } );
#endif

        if( it != m_settings.end() )
        {
            // Do NOT use dynamic_cast here.  CLang will think it's the wrong class across
            // compile boundaries and return nullptr.
            ret = static_cast<T*>( it->get() );
        }
        else
        {
            wxFAIL_MSG( "Tried to GetAppSettings before registering" );
        }

        m_app_settings_cache[typeHash] = ret;

        return ret;
    }

    /**
     * Return a handle to the given toolbar settings
     *
     * If the settings have already been loaded, returns the existing pointer.
     * If the settings have not been loaded, creates a new object owned by the
     * settings manager and returns a pointer to it.
     *
     * @tparam T is a type derived from TOOLBAR_SETTINGS.
     * @param aFilename is used to find the correct settings under clang (where
     *                  RTTI doesn't work across compile boundaries).
     * @return a pointer to a loaded settings object.
     */
    template<typename T>
    T* GetToolbarSettings( const wxString& aFilename )
    {
        T* ret = nullptr;

#if defined(__clang__)
        auto it = std::find_if( m_settings.begin(), m_settings.end(),
                                [&]( const std::unique_ptr<JSON_SETTINGS>& aSettings )
                                {
                                    return aSettings->GetFilename() == aFilename;
                                } );
#else
        auto it = std::find_if( m_settings.begin(), m_settings.end(),
                                []( const std::unique_ptr<JSON_SETTINGS>& aSettings )
                                {
                                    return dynamic_cast<T*>( aSettings.get() );
                                } );
#endif

        if( it != m_settings.end() )
        {
            // Do NOT use dynamic_cast here.  CLang will think it's the wrong class across
            // compile boundaries and return nullptr.
            ret = static_cast<T*>( it->get() );
        }
        else
        {
            ret = RegisterSettings( new T );
        }

        return ret;
    }

    /**
     * Retrieve a color settings object that applications can read colors from.
     *
     * If the given settings file cannot be found, returns the default settings.
     *
     * @param aName is the name of the color scheme to load.
     * @return a loaded COLOR_SETTINGS object.
     */
    COLOR_SETTINGS* GetColorSettings( const wxString& aName );

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
     * Safely save a #COLOR_SETTINGS to disk, preserving any changes outside the given namespace.
     *
     * A color settings namespace is one of the top-level JSON objects like "board", etc.
     * This will perform a read-modify-write
     *
     * @param aSettings is a pointer to a valid COLOR_SETTINGS object managed by SETTINGS_MANAGER.
     * @param aNamespace is the namespace of settings to save.
     */
    void SaveColorSettings( COLOR_SETTINGS* aSettings, const std::string& aNamespace = "" );

    /**
     * Register a new color settings object with the given filename.
     *
     * @param aFilename is the location to store the new settings object.
     * @return a pointer to the new object.
     */
    COLOR_SETTINGS* AddNewColorSettings( const wxString& aFilename );

    /**
     * Return a color theme for storing colors migrated from legacy (5.x and earlier) settings,
     * creating the theme if necessary.
     *
     * This theme will be called "user.json" / "User".
     *
     * @return the color settings to be used for migrating legacy settings.
     */
    COLOR_SETTINGS* GetMigratedColorSettings();

    /**
     * Retrieve the common settings shared by all applications.
     *
     * @return a pointer to a loaded #COMMON_SETTINGS.
     */
    COMMON_SETTINGS* GetCommonSettings() const { return m_common_settings; }

    /**
     * Return the path a given settings file should be loaded from / stored to.
     *
     * @param aSettings is the settings object.
     * @return a path based on aSettings->m_location.
     */
    wxString GetPathForSettingsFile( JSON_SETTINGS* aSettings );

    /**
     * Handle migration of the settings from previous KiCad versions.
     *
     * @return true if migration was successful, false otherwise.
     */
    bool MigrateFromPreviousVersion( const wxString& aSourcePath );

    /**
     * Retrieve the name of the most recent previous KiCad version that can be found in the
     * user settings directory.
     *
     * For legacy versions (5.x, and 5.99 builds before this code was written), this will return
     * "5.x".
     *
     * @param aName is filled with the name of the previous version, if one exists.
     * @return true if a previous version to migrate from exists.
     */
    bool GetPreviousVersionPaths( std::vector<wxString>* aName = nullptr );

    /**
     * Re-scan the color themes directory, reloading any changes it finds.
     */
    void ReloadColorSettings();

    /**
     * Load a project or sets up a new project with a specified path.
     *
     * @param aFullPath is the full path to the project.
     * @param aSetActive if true will set the loaded project as the active project.
     * @return true if the #PROJECT_FILE was successfully loaded from disk.
     */
    bool LoadProject( const wxString& aFullPath, bool aSetActive = true );

    /**
     * Save, unload and unregister the given #PROJECT.
     *
     * @param aProject is the project object to unload.
     * @param aSave if true will save the project before unloading.
     * @return true if the #PROJECT file was successfully saved.
     */
    bool UnloadProject( PROJECT* aProject, bool aSave = true );

    /**
     * Helper for checking if we have a project open.
     *
     * @todo This should be deprecated along with Prj() once we support multiple projects fully.
     *
     * @return true if a call to Prj() will succeed.
     */
    bool IsProjectOpen() const;

    /**
     * Helper for checking if we have a project open that is not a dummy project.
     *
     * @return true if a call to Prj() will succeed and the project is not a dummy project.
     */
    bool IsProjectOpenNotDummy() const;

    /**
     * A helper while we are not MDI-capable -- return the one and only project.
     *
     * @return the loaded project.
     */
    PROJECT& Prj() const;

    /**
     * Retrieve a loaded project by name.
     *
     * @param aFullPath is the full path including name and extension to the project file.
     * @return a pointer to the project if loaded, or nullptr.
     */
    PROJECT* GetProject( const wxString& aFullPath ) const;

    /**
     * @return a list of open projects.
     */
    std::vector<wxString> GetOpenProjects() const;

    /**
     * Save a loaded project.
     *
     * @param aFullPath is the project name to save.  If empty, will save the first loaded project.
     * @param aProject is the project to save, or nullptr to save the active project (Prj() return).
     * @return true if save was successful.
     */
    bool SaveProject( const wxString& aFullPath = wxEmptyString, PROJECT* aProject = nullptr );

    /**
     * Set the currently loaded project path and saves it (pointers remain valid).
     *
     * @note that this will not modify the read-only state of the project, so it will have no effect
     * if the project is marked as read-only!
     *
     * @param aFullPath is the full filename to set for the project.
     * @param aProject is the project to save, or nullptr to save the active project (Prj() return).
     */
    void SaveProjectAs( const wxString& aFullPath, PROJECT* aProject = nullptr );

    /**
     * Save a copy of the current project under the given path.
     *
     * @warning This will save the copy even if the current project is marked as read-only.
     *
     * @param aFullPath is the full filename to set for the project.
     * @param aProject is the project to save, or nullptr to save the active project (Prj() return).
     */
    void SaveProjectCopy( const wxString& aFullPath, PROJECT* aProject = nullptr );

    /**
     * @return the full path to where project backups should be stored.
     */
    wxString GetProjectBackupsPath() const;

    /**
     * Create a backup archive of the current project.
     *
     * @param aReporter is used for progress reporting.
     * @param aTarget is the full path to the backup file.  If empty, will generate and return the
     * full path to the backup file.
     * @return true if everything succeeded.
     */
    bool BackupProject( REPORTER& aReporter, wxFileName& aTarget ) const;

    /**
     * Call BackupProject() if a new backup is needed according to the current backup policy.
     *
     * @param aReporter is used for progress reporting.
     * @return if everything succeeded.
     */
    bool TriggerBackupIfNeeded( REPORTER& aReporter ) const;

    /**
     * Check if a given path is probably a valid KiCad configuration directory.
     *
     * Actually it just checks if a file called "kicad_common" exists, because that's probably
     * good enough for now.
     *
     * @param aPath is the path to check.
     * @return true if the path contains KiCad settings.
     */
    static bool IsSettingsPathValid( const wxString& aPath );

    /**
     * Return the path where color scheme files are stored; creating it if missing
     * (normally ./colors/ under the user settings path).
     */
    static wxString GetColorSettingsPath();

    /**
     * Return the path where toolbar configuration files are stored; creating it if missing
     * (normally ./toolbars/ under the user settings path).
     */
    static wxString GetToolbarSettingsPath();

    /**
     * Parse the current KiCad build version and extracts the major and minor revision to use
     * as the name of the settings directory for this KiCad version.
     *
     * @return a string such as "5.1".
     */
    static std::string GetSettingsVersion();

    /**
     * A proxy for PATHS::GetUserSettingsPath() rather than fighting swig.
     */
    static wxString GetUserSettingsPath();

private:
    JSON_SETTINGS* registerSettings( JSON_SETTINGS* aSettings, bool aLoadNow = true );

    /**
     * Compare two settings versions, like "5.99" and "6.0".
     *
     * @return -1 if aFirst is older than aSecond, 1 if aFirst is newer than aSecond, 0 otherwise.
     */
    static int compareVersions( const std::string& aFirst, const std::string& aSecond );

    /**
     * Extract the numeric version from a given settings string.
     *
     * @param aVersionString is the string to split at the ".".
     * @param aMajor will store the first part.
     * @param aMinor will store the second part.
     * @return true if extraction succeeded.
     */
    static bool extractVersion( const std::string& aVersionString, int* aMajor = nullptr,
                                int* aMinor = nullptr );

    /**
     * Attempt to load a color theme by name (the color theme directory and .json ext are assumed).
     *
     * @param aName is the filename of the color theme (without the extension or path).
     * @return the loaded settings, or nullptr if load failed.
     */
    COLOR_SETTINGS* loadColorSettingsByName( const wxString& aName );

    COLOR_SETTINGS* registerColorSettings( const wxString& aFilename, bool aAbsolutePath = false );

    void loadAllColorSettings();

    /**
     * Register a #PROJECT_FILE and attempt to load it from disk.
     *
     * @param aProject is the project object to load the file for.
     * @return true if the #PROJECT_FILE was successfully loaded.
     */
    bool loadProjectFile( PROJECT& aProject );

    /**
     * Optionally save, unload and unregister the given #PROJECT_FILE.
     *
     * @param aProject is the project object to unload the file for.
     * @param aSave if true will save the project file before unloading.
     * @return true if the #PROJECT file was successfully saved.
     */
    bool unloadProjectFile( PROJECT* aProject, bool aSave );

    ///< Helper to create built-in colors and register them.
    void registerBuiltinColorSettings();

private:

    /// The kiway this settings manager interacts with.
    KIWAY* m_kiway;

    std::vector<std::unique_ptr<JSON_SETTINGS>> m_settings;

    std::unordered_map<wxString, COLOR_SETTINGS*> m_color_settings;

    /// Cache for app settings
    std::unordered_map<size_t, JSON_SETTINGS*> m_app_settings_cache;
    std::mutex m_app_settings_mutex;

    // Convenience shortcut
    COMMON_SETTINGS* m_common_settings;

    /// If true, the symbol and footprint library tables will be migrated from the previous version.
    bool m_migrateLibraryTables;

    /// True if settings loaded successfully at construction.
    bool m_ok;

    /// Loaded projects (ownership here).
    std::vector<std::unique_ptr<PROJECT>> m_projects_list;

    /// Loaded projects, mapped according to project full name.
    std::map<wxString, PROJECT*> m_projects;

    /// Loaded project files, mapped according to project full name.
    std::map<wxString, PROJECT_FILE*> m_project_files;

    static wxString backupDateTimeFormat;
};


template<typename T>
T* GetAppSettings( const char* aFilename )
{
    if( PGM_BASE* pgm = PgmOrNull() )
        return pgm->GetSettingsManager().GetAppSettings<T>( aFilename );

    return nullptr;
}


template<typename T>
T* GetToolbarSettings( const wxString& aFilename )
{
    if( PGM_BASE* pgm = PgmOrNull() )
        return pgm->GetSettingsManager().GetToolbarSettings<T>( aFilename );

    return nullptr;
}


inline COLOR_SETTINGS* GetColorSettings( const wxString& aName )
{
    return Pgm().GetSettingsManager().GetColorSettings( aName );
}

#endif
