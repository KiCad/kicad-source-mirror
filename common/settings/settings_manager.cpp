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

#include "settings/json_settings.h"
#include <regex>
#include <wx/debug.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/snglinst.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <build_version.h>
#include <confirm.h>
#include <dialogs/dialog_migrate_settings.h>
#include <gestfich.h>
#include <kiplatform/environment.h>
#include <kiplatform/io.h>
#include <kiway.h>
#include <lockfile.h>
#include <macros.h>
#include <pgm_base.h>
#include <paths.h>

#include <algorithm>
#include <project.h>
#include <project/project_archiver.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/json_settings_internals.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>
#include <env_vars.h>
#include <libraries/library_manager.h>


SETTINGS_MANAGER::SETTINGS_MANAGER( bool aHeadless ) :
        m_headless( aHeadless ),
        m_kiway( nullptr ),
        m_common_settings( nullptr ),
        m_migration_source(),
        m_migrateLibraryTables( true )
{
    // Check if the settings directory already exists, and if not, perform a migration if possible
    if( !MigrateIfNeeded() )
    {
        m_ok = false;
        return;
    }

    m_ok = true;

    // create the common settings shared by all applications.  Not loaded immediately
    m_common_settings = RegisterSettings( new COMMON_SETTINGS, false );

    // Create the built-in color settings
    // Here to allow the Python API to access the built-in colors
    registerBuiltinColorSettings();

    wxFileName commonSettings( GetPathForSettingsFile( m_common_settings ),
                               m_common_settings->GetFullFilename() );

    if( !wxFileExists( commonSettings.GetFullPath() ) )
    {
        m_common_settings->Load();
        Save( m_common_settings );
    }
}


SETTINGS_MANAGER::~SETTINGS_MANAGER()
{
    for( std::unique_ptr<PROJECT>& project : m_projects_list )
        project.reset();

    m_projects.clear();

    for( std::unique_ptr<JSON_SETTINGS>& settings : m_settings )
        settings.reset();

    m_settings.clear();

    m_color_settings.clear();
}


void SETTINGS_MANAGER::ResetToDefaults()
{
    for( std::unique_ptr<JSON_SETTINGS>& settings : m_settings )
    {
        if( settings->GetLocation() == SETTINGS_LOC::USER || settings->GetLocation() == SETTINGS_LOC::COLORS )
        {
            std::map<std::string, nlohmann::json> fileHistories = settings->GetFileHistories();

            settings->Internals()->clear();
            settings->Load();   // load from nothing (ie: load defaults)

            for( const auto& [path, history] : fileHistories )
                settings->Set( path, history );

            settings->SaveToFile( GetPathForSettingsFile( settings.get() ) );
        }
    }
}


void SETTINGS_MANAGER::ClearFileHistory()
{
    for( std::unique_ptr<JSON_SETTINGS>& settings : m_settings )
    {
        if( settings->GetLocation() == SETTINGS_LOC::USER )
        {
            for( const auto& [path, history] : settings->GetFileHistories() )
                settings->Set( path, nlohmann::json::array() );

            settings->SaveToFile( GetPathForSettingsFile( settings.get() ) );
        }
    }
}


JSON_SETTINGS* SETTINGS_MANAGER::registerSettings( JSON_SETTINGS* aSettings, bool aLoadNow )
{
    std::unique_ptr<JSON_SETTINGS> ptr( aSettings );

    ptr->SetManager( this );

    wxLogTrace( traceSettings, wxT( "Registered new settings object <%s>" ),
                ptr->GetFullFilename() );

    if( aLoadNow )
        ptr->LoadFromFile( GetPathForSettingsFile( ptr.get() ) );

    m_settings.push_back( std::move( ptr ) );
    return m_settings.back().get();
}


void SETTINGS_MANAGER::Load()
{
    // TODO(JE) We should check for dirty settings here and write them if so, because
    // Load() could be called late in the application lifecycle
    std::vector<JSON_SETTINGS*> toLoad;

    // Cache a copy of raw pointers; m_settings may be modified during the load loop
    std::transform( m_settings.begin(), m_settings.end(), std::back_inserter( toLoad ),
                    []( std::unique_ptr<JSON_SETTINGS>& aSettings )
                    {
                        return aSettings.get();
                    } );

    for( JSON_SETTINGS* settings : toLoad )
        settings->LoadFromFile( GetPathForSettingsFile( settings ) );
}


void SETTINGS_MANAGER::Load( JSON_SETTINGS* aSettings )
{
    auto it = std::find_if( m_settings.begin(), m_settings.end(),
                            [&aSettings]( const std::unique_ptr<JSON_SETTINGS>& aPtr )
                            {
                                return aPtr.get() == aSettings;
                            } );

    if( it != m_settings.end() )
        ( *it )->LoadFromFile( GetPathForSettingsFile( it->get() ) );
}


void SETTINGS_MANAGER::Save()
{
    for( auto&& settings : m_settings )
    {
        // Never automatically save color settings, caller should use SaveColorSettings
        if( dynamic_cast<COLOR_SETTINGS*>( settings.get() ) )
            continue;

        // Never automatically save project file, caller should use SaveProject or UnloadProject
        // We do want to save the project local settings, though because they are generally view
        // settings that should persist even if the project is not saved
        if( dynamic_cast<PROJECT_FILE*>( settings.get() ) )
        {
            continue;
        }

        settings->SaveToFile( GetPathForSettingsFile( settings.get() ) );
    }
}


void SETTINGS_MANAGER::Save( JSON_SETTINGS* aSettings )
{
    auto it = std::find_if( m_settings.begin(), m_settings.end(),
                            [&aSettings]( const std::unique_ptr<JSON_SETTINGS>& aPtr )
                            {
                                return aPtr.get() == aSettings;
                            } );

    if( it != m_settings.end() )
    {
        wxLogTrace( traceSettings, wxT( "Saving %s" ), ( *it )->GetFullFilename() );
        ( *it )->SaveToFile( GetPathForSettingsFile( it->get() ) );
    }
}


void SETTINGS_MANAGER::FlushAndRelease( JSON_SETTINGS* aSettings, bool aSave )
{
    auto it = std::find_if( m_settings.begin(), m_settings.end(),
                            [&aSettings]( const std::unique_ptr<JSON_SETTINGS>& aPtr )
                            {
                                return aPtr.get() == aSettings;
                            } );

    if( it != m_settings.end() )
    {
        wxLogTrace( traceSettings, wxT( "Flush and release %s" ), ( *it )->GetFullFilename() );

        if( aSave )
            ( *it )->SaveToFile( GetPathForSettingsFile( it->get() ) );

        JSON_SETTINGS* tmp = it->get(); // We use a temporary to suppress a Clang warning
        size_t         typeHash = typeid( *tmp ).hash_code();

        if( m_app_settings_cache.count( typeHash ) )
            m_app_settings_cache.erase( typeHash );

        m_settings.erase( it );
    }
}


COLOR_SETTINGS* SETTINGS_MANAGER::GetColorSettings( const wxString& aName )
{
    // Find settings the fast way
    if( m_color_settings.count( aName ) )
        return m_color_settings.at( aName );

    // Maybe it's the display name (cli is one method of invoke)
    auto it = std::find_if( m_color_settings.begin(), m_color_settings.end(),
                            [&aName]( const std::pair<wxString, COLOR_SETTINGS*>& p )
                            {
                                return p.second->GetName().Lower() == aName.Lower();
                            } );

    if( it != m_color_settings.end() )
    {
        return it->second;
    }

    // No match? See if we can load it
    if( !aName.empty() )
    {
        COLOR_SETTINGS* ret = loadColorSettingsByName( aName );

        if( !ret )
        {
            ret = registerColorSettings( aName );
            *ret = *m_color_settings.at( COLOR_SETTINGS::COLOR_BUILTIN_DEFAULT );
            ret->SetFilename( DEFAULT_THEME );
            ret->SetReadOnly( false );
        }

        return ret;
    }

    // This had better work
    return m_color_settings.at( COLOR_SETTINGS::COLOR_BUILTIN_DEFAULT );
}


COLOR_SETTINGS* SETTINGS_MANAGER::loadColorSettingsByName( const wxString& aName )
{
    wxLogTrace( traceSettings, wxT( "Attempting to load color theme %s" ), aName );

    wxFileName fn( GetColorSettingsPath(), aName, wxS( "json" ) );

    if( !fn.IsOk() || !fn.Exists() )
    {
        wxLogTrace( traceSettings, wxT( "Theme file %s.json not found, falling back to user" ),
                    aName );
        return nullptr;
    }

    COLOR_SETTINGS* settings = RegisterSettings( new COLOR_SETTINGS( aName ) );

    if( settings->GetFilename() != aName.ToStdString() )
    {
        wxLogTrace( traceSettings, wxT( "Warning: stored filename is actually %s, " ),
                    settings->GetFilename() );
    }

    m_color_settings[aName] = settings;

    return settings;
}


class JSON_DIR_TRAVERSER : public wxDirTraverser
{
private:
    std::function<void( const wxFileName& )> m_action;

public:
    explicit JSON_DIR_TRAVERSER( std::function<void( const wxFileName& )> aAction )
            : m_action( std::move( aAction ) )
    {
    }

    wxDirTraverseResult OnFile( const wxString& aFilePath ) override
    {
        wxFileName file( aFilePath );

        if( file.GetExt() == wxS( "json" ) )
            m_action( file );

        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnDir( const wxString& dirPath ) override
    {
        return wxDIR_CONTINUE;
    }
};


COLOR_SETTINGS* SETTINGS_MANAGER::registerColorSettings( const wxString& aName, bool aAbsolutePath )
{
    if( !m_color_settings.count( aName ) )
    {
        COLOR_SETTINGS* colorSettings = RegisterSettings( new COLOR_SETTINGS( aName,
                                                                              aAbsolutePath ) );
        m_color_settings[aName] = colorSettings;
    }

    return m_color_settings.at( aName );
}


COLOR_SETTINGS* SETTINGS_MANAGER::AddNewColorSettings( const wxString& aName )
{
    if( aName.EndsWith( wxT( ".json" ) ) )
        return registerColorSettings( aName.BeforeLast( '.' ) );
    else
        return registerColorSettings( aName );
}


COLOR_SETTINGS* SETTINGS_MANAGER::GetMigratedColorSettings()
{
    if( !m_color_settings.count( "user" ) )
    {
        COLOR_SETTINGS* settings = registerColorSettings( wxT( "user" ) );
        settings->SetName( wxT( "User" ) );
        Save( settings );
    }

    return m_color_settings.at( "user" );
}


void SETTINGS_MANAGER::registerBuiltinColorSettings()
{
    for( COLOR_SETTINGS* settings : COLOR_SETTINGS::CreateBuiltinColorSettings() )
        m_color_settings[settings->GetFilename()] = RegisterSettings( settings, false );
}


void SETTINGS_MANAGER::loadAllColorSettings()
{
    // Create the built-in color settings
    registerBuiltinColorSettings();

    wxFileName third_party_path;
    const ENV_VAR_MAP& env = Pgm().GetLocalEnvVariables();
    auto               it = env.find( ENV_VAR::GetVersionedEnvVarName( wxS( "3RD_PARTY" ) ) );

    if( it != env.end() && !it->second.GetValue().IsEmpty() )
        third_party_path.SetPath( it->second.GetValue() );
    else
        third_party_path.SetPath( PATHS::GetDefault3rdPartyPath() );

    third_party_path.AppendDir( wxS( "colors" ) );

    // PCM-managed themes
    wxDir third_party_colors_dir( third_party_path.GetFullPath() );

    // System-installed themes
    wxDir system_colors_dir( PATHS::GetStockDataPath( false ) + "/colors" );

    // User-created themes
    wxDir colors_dir( GetColorSettingsPath() );

    // Search for and load any other settings
    JSON_DIR_TRAVERSER loader( [&]( const wxFileName& aFilename )
                               {
                                   registerColorSettings( aFilename.GetName() );
                               } );

    JSON_DIR_TRAVERSER readOnlyLoader(
            [&]( const wxFileName& aFilename )
            {
                COLOR_SETTINGS* settings = registerColorSettings( aFilename.GetFullPath(), true );
                settings->SetReadOnly( true );
            } );

    if( system_colors_dir.IsOpened() )
        system_colors_dir.Traverse( readOnlyLoader );

    if( third_party_colors_dir.IsOpened() )
       third_party_colors_dir.Traverse( readOnlyLoader );

    if( colors_dir.IsOpened() )
        colors_dir.Traverse( loader );
}


void SETTINGS_MANAGER::ReloadColorSettings()
{
    m_color_settings.clear();
    loadAllColorSettings();
}


void SETTINGS_MANAGER::SaveColorSettings( COLOR_SETTINGS* aSettings, const std::string& aNamespace )
{
    // The passed settings should already be managed
    wxASSERT( std::find_if( m_color_settings.begin(), m_color_settings.end(),
                            [aSettings] ( const std::pair<wxString, COLOR_SETTINGS*>& el )
                            {
                                return el.second->GetFilename() == aSettings->GetFilename();
                            }
                            ) != m_color_settings.end() );

    if( aSettings->IsReadOnly() )
        return;

    if( !aSettings->Store() )
    {
        wxLogTrace( traceSettings, wxT( "Color scheme %s not modified; skipping save" ),
                    aNamespace );
        return;
    }

    wxASSERT( aSettings->Contains( aNamespace ) );

    wxLogTrace( traceSettings, wxT( "Saving color scheme %s, preserving %s" ),
                aSettings->GetFilename(),
                aNamespace );

    std::optional<nlohmann::json> backup = aSettings->GetJson( aNamespace );
    wxString path = GetColorSettingsPath();

    aSettings->LoadFromFile( path );

    if( backup )
        ( *aSettings->Internals() )[aNamespace].update( *backup );

    aSettings->Load();

    aSettings->SaveToFile( path, true );
}


wxString SETTINGS_MANAGER::GetPathForSettingsFile( JSON_SETTINGS* aSettings )
{
    wxASSERT( aSettings );

    switch( aSettings->GetLocation() )
    {
    case SETTINGS_LOC::USER:
        return PATHS::GetUserSettingsPath();

    case SETTINGS_LOC::PROJECT:
        // TODO: MDI support
        return Prj().GetProjectPath();

    case SETTINGS_LOC::COLORS:
        return GetColorSettingsPath();

    case SETTINGS_LOC::TOOLBARS:
        return GetToolbarSettingsPath();

    case SETTINGS_LOC::NONE:
        return "";

    default:
        wxASSERT_MSG( false, wxT( "Unknown settings location!" ) );
    }

    return "";
}


class MIGRATION_TRAVERSER : public wxDirTraverser
{
private:
    wxString m_src;
    wxString m_dest;
    wxString m_errors;
    bool     m_migrateTables;

public:
    MIGRATION_TRAVERSER( const wxString& aSrcDir, const wxString& aDestDir, bool aMigrateTables ) :
            m_src( aSrcDir ),
            m_dest( aDestDir ),
            m_migrateTables( aMigrateTables )
    {
    }

    wxString GetErrors() { return m_errors; }

    wxDirTraverseResult OnFile( const wxString& aSrcFilePath ) override
    {
        wxFileName file( aSrcFilePath );

        if( !m_migrateTables && ( file.GetName() == FILEEXT::SymbolLibraryTableFileName ||
                                  file.GetName() == FILEEXT::FootprintLibraryTableFileName ) )
        {
            return wxDIR_CONTINUE;
        }

        // Skip migrating PCM installed packages as packages themselves are not moved
        if( file.GetFullName() == wxT( "installed_packages.json" ) )
            return wxDIR_CONTINUE;

        // Don't migrate hotkeys config files; we don't have a reasonable migration handler for them
        // and so there is no way to resolve conflicts at the moment
        if( file.GetExt() == wxT( "hotkeys" ) )
            return wxDIR_CONTINUE;

        wxString path = file.GetPath();

        path.Replace( m_src, m_dest, false );
        file.SetPath( path );

        wxLogTrace( traceSettings, wxT( "Copying %s to %s" ), aSrcFilePath, file.GetFullPath() );

        // For now, just copy everything
        KiCopyFile( aSrcFilePath, file.GetFullPath(), m_errors );

        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnDir( const wxString& dirPath ) override
    {
        wxFileName dir( dirPath );

        // Whitelist of directories to migrate
        if( dir.GetName() == wxS( "colors" ) ||
            dir.GetName() == wxS( "3d" ) )
        {

            wxString path = dir.GetPath();

            path.Replace( m_src, m_dest, false );
            dir.SetPath( path );

            wxMkdir( dir.GetFullPath() );

            return wxDIR_CONTINUE;
        }
        else
        {
            return wxDIR_IGNORE;
        }
    }
};


bool SETTINGS_MANAGER::MigrateIfNeeded()
{
    wxFileName path( PATHS::GetUserSettingsPath(), wxS( "" ) );
    wxLogTrace( traceSettings, wxT( "Using settings path %s" ), path.GetFullPath() );

    if( m_headless )
    {
        // Special case namely for cli
        // Ensure the settings directory at least exists to prevent additional loading errors
        // from subdirectories.
        // TODO review headless (unit tests) vs cli needs, this should be fine for unit tests though
        if( !path.DirExists() )
        {
            wxLogTrace( traceSettings, wxT( "Path didn't exist; creating it" ) );
            path.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
        }

        wxLogTrace( traceSettings, wxT( "Settings migration not checked; running headless" ) );
        return true;
    }

    if( path.DirExists() )
    {
        wxFileName common = path;
        common.SetName( wxS( "kicad_common" ) );
        common.SetExt( wxS( "json" ) );

        if( common.Exists() )
        {
            wxLogTrace( traceSettings, wxT( "Path exists and has a kicad_common, continuing!" ) );
            return true;
        }
    }

    // Now we have an empty path, let's figure out what to put in it
    DIALOG_MIGRATE_SETTINGS dlg( this );

    if( dlg.ShowModal() != wxID_OK )
    {
        wxLogTrace( traceSettings, wxT( "Migration dialog canceled; exiting" ) );
        return false;
    }

    if( !path.DirExists() )
    {
        wxLogTrace( traceSettings, wxT( "Path didn't exist; creating it" ) );
        path.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    }

    if( m_migration_source.IsEmpty() )
    {
        wxLogTrace( traceSettings, wxT( "No migration source given; starting with defaults" ) );
        return true;
    }

    wxLogTrace( traceSettings, wxT( "Migrating from path %s" ), m_migration_source );

    MIGRATION_TRAVERSER traverser( m_migration_source, path.GetFullPath(), m_migrateLibraryTables );
    wxDir source_dir( m_migration_source );

    source_dir.Traverse( traverser );

    if( !traverser.GetErrors().empty() )
        DisplayErrorMessage( nullptr, traverser.GetErrors() );

    // Remove any library configuration if we didn't choose to import
    if( !m_migrateLibraryTables )
    {
        COMMON_SETTINGS common;
        wxString        commonPath = GetPathForSettingsFile( &common );
        common.LoadFromFile( commonPath );

        const std::vector<wxString> libKeys = {
            wxT( "KICAD6_SYMBOL_DIR" ),
            wxT( "KICAD6_3DMODEL_DIR" ),
            wxT( "KICAD6_FOOTPRINT_DIR" ),
            wxT( "KICAD6_TEMPLATE_DIR" ), // Stores the default library table to be copied
            wxT( "KICAD7_SYMBOL_DIR" ),
            wxT( "KICAD7_3DMODEL_DIR" ),
            wxT( "KICAD7_FOOTPRINT_DIR" ),
            wxT( "KICAD7_TEMPLATE_DIR" ),
            wxT( "KICAD8_SYMBOL_DIR" ),
            wxT( "KICAD8_3DMODEL_DIR" ),
            wxT( "KICAD8_FOOTPRINT_DIR" ),
            wxT( "KICAD8_TEMPLATE_DIR" ),

            // Deprecated keys
            wxT( "KICAD_PTEMPLATES" ),
            wxT( "KISYS3DMOD" ),
            wxT( "KISYSMOD" ),
            wxT( "KICAD_SYMBOL_DIR" ),
        };

        for( const wxString& key : libKeys )
            common.m_Env.vars.erase( key );

        common.SaveToFile( commonPath  );
    }

    return true;
}


bool SETTINGS_MANAGER::GetPreviousVersionPaths( std::vector<wxString>* aPaths )
{
    wxASSERT( aPaths );

    aPaths->clear();

    wxDir dir;
    std::vector<wxFileName> base_paths;

    base_paths.emplace_back( wxFileName( PATHS::CalculateUserSettingsPath( false ), wxS( "" ) ) );

    // If the env override is set, also check the default paths
    if( wxGetEnv( wxT( "KICAD_CONFIG_HOME" ), nullptr ) )
        base_paths.emplace_back( wxFileName( PATHS::CalculateUserSettingsPath( false, false ),
                                             wxS( "" ) ) );

#ifdef __WXGTK__
    // When running inside FlatPak, KIPLATFORM::ENV::GetUserConfigPath() will return a sandboxed
    // path.  In case the user wants to move from non-FlatPak KiCad to FlatPak KiCad, let's add our
    // best guess as to the non-FlatPak config path.  Unfortunately FlatPak also hides the host
    // XDG_CONFIG_HOME, so if the user customizes their config path, they will have to browse
    // for it.
    {
        wxFileName wxGtkPath;
        wxGtkPath.AssignDir( wxS( "~/.config/kicad" ) );
        wxGtkPath.MakeAbsolute();
        base_paths.emplace_back( wxGtkPath );

        // We also want to pick up regular flatpak if we are nightly
        wxGtkPath.AssignDir( wxS( "~/.var/app/org.kicad.KiCad/config/kicad" ) );
        wxGtkPath.MakeAbsolute();
        base_paths.emplace_back( wxGtkPath );
    }
#endif

    wxString subdir;
    std::string mine = GetSettingsVersion();

    auto check_dir =
            [&] ( const wxString& aSubDir )
            {
                // Only older versions are valid for migration
                if( compareVersions( aSubDir.ToStdString(), mine ) <= 0 )
                {
                    wxString sub_path = dir.GetNameWithSep() + aSubDir;

                    if( IsSettingsPathValid( sub_path ) )
                    {
                        aPaths->push_back( sub_path );
                        wxLogTrace( traceSettings, wxT( "GetPreviousVersionName: %s is valid" ), sub_path );
                    }
                }
            };

    std::set<wxString> checkedPaths;

    for( const wxFileName& base_path : base_paths )
    {
        if( checkedPaths.count( base_path.GetFullPath() ) )
            continue;

        checkedPaths.insert( base_path.GetFullPath() );

        if( !dir.Open( base_path.GetFullPath() ) )
        {
            wxLogTrace( traceSettings, wxT( "GetPreviousVersionName: could not open base path %s" ),
                        base_path.GetFullPath() );
            continue;
        }

        wxLogTrace( traceSettings, wxT( "GetPreviousVersionName: checking base path %s" ),
                    base_path.GetFullPath() );

        if( dir.GetFirst( &subdir, wxEmptyString, wxDIR_DIRS ) )
        {
            if( subdir != mine )
                check_dir( subdir );

            while( dir.GetNext( &subdir ) )
            {
                if( subdir != mine )
                    check_dir( subdir );
            }
        }

        // If we didn't find one yet, check for legacy settings without a version directory
        if( IsSettingsPathValid( dir.GetNameWithSep() ) )
        {
            wxLogTrace( traceSettings,
                        wxT( "GetPreviousVersionName: root path %s is valid" ), dir.GetName() );
            aPaths->push_back( dir.GetName() );
        }
    }

    std::erase_if( *aPaths,
                   []( const wxString& aPath ) -> bool
                   {
                       wxFileName fulldir = wxFileName::DirName( aPath );
                       const wxArrayString& dirs = fulldir.GetDirs();

                       if( dirs.empty() || !fulldir.IsDirReadable() )
                           return true;

                       std::string ver = dirs.back().ToStdString();

                       if( !extractVersion( ver ) )
                           return true;

                       return false;
                    } );

    std::sort( aPaths->begin(), aPaths->end(),
               [&]( const wxString& a, const wxString& b ) -> bool
               {
                   wxFileName aPath = wxFileName::DirName( a );
                   wxFileName bPath = wxFileName::DirName( b );

                   const wxArrayString& aDirs = aPath.GetDirs();
                   const wxArrayString& bDirs = bPath.GetDirs();

                   if( aDirs.empty() )
                       return false;

                   if( bDirs.empty() )
                       return true;

                   std::string verA = aDirs.back().ToStdString();
                   std::string verB = bDirs.back().ToStdString();

                   if( !extractVersion( verA ) )
                       return false;

                   if( !extractVersion( verB ) )
                       return true;

                   return compareVersions( verA, verB ) >= 0;
               } );

    return aPaths->size() > 0;
}


bool SETTINGS_MANAGER::IsSettingsPathValid( const wxString& aPath )
{
    wxFileName test( aPath, wxS( "kicad_common" ) );

    if( test.Exists() )
        return true;

    test.SetExt( "json" );

    return test.Exists();
}


wxString SETTINGS_MANAGER::GetColorSettingsPath()
{
    wxFileName path;

    path.AssignDir( PATHS::GetUserSettingsPath() );
    path.AppendDir( wxS( "colors" ) );

    if( !path.DirExists() )
    {
        if( !wxMkdir( path.GetPath() ) )
        {
            wxLogTrace( traceSettings,
                        wxT( "GetColorSettingsPath(): Path %s missing and could not be created!" ),
                        path.GetPath() );
        }
    }

    return path.GetPath();
}


wxString SETTINGS_MANAGER::GetToolbarSettingsPath()
{
    wxFileName path;

    path.AssignDir( PATHS::GetUserSettingsPath() );
    path.AppendDir( wxS( "toolbars" ) );

    if( !path.DirExists() )
    {
        if( !wxMkdir( path.GetPath() ) )
        {
            wxLogTrace( traceSettings,
                        wxT( "GetToolbarSettingsPath(): Path %s missing and could not be created!" ),
                        path.GetPath() );
        }
    }

    return path.GetPath();
}


std::string SETTINGS_MANAGER::GetSettingsVersion()
{
    // CMake computes the major.minor string for us.
    return GetMajorMinorVersion().ToStdString();
}


int SETTINGS_MANAGER::compareVersions( const std::string& aFirst, const std::string& aSecond )
{
    int a_maj = 0;
    int a_min = 0;
    int b_maj = 0;
    int b_min = 0;

    if( !extractVersion( aFirst, &a_maj, &a_min ) || !extractVersion( aSecond, &b_maj, &b_min ) )
    {
        wxLogTrace( traceSettings, wxT( "compareSettingsVersions: bad input (%s, %s)" ),
                    aFirst, aSecond );
        return -1;
    }

    if( a_maj < b_maj )
    {
        return -1;
    }
    else if( a_maj > b_maj )
    {
        return 1;
    }
    else
    {
        if( a_min < b_min )
        {
            return -1;
        }
        else if( a_min > b_min )
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}


bool SETTINGS_MANAGER::extractVersion( const std::string& aVersionString, int* aMajor, int* aMinor )
{
    std::regex  re_version( "(\\d+)\\.(\\d+)" );
    std::smatch match;

    if( std::regex_match( aVersionString, match, re_version ) )
    {
        try
        {
            int major = std::stoi( match[1].str() );
            int minor = std::stoi( match[2].str() );

            if( aMajor )
                *aMajor = major;

            if( aMinor )
                *aMinor = minor;
        }
        catch( ... )
        {
            return false;
        }

        return true;
    }

    return false;
}


bool SETTINGS_MANAGER::LoadProject( const wxString& aFullPath, bool aSetActive )
{
    // Normalize path to new format even if migrating from a legacy file
    wxFileName path( aFullPath );

    if( path.GetExt() == FILEEXT::LegacyProjectFileExtension )
        path.SetExt( FILEEXT::ProjectFileExtension );

    wxString fullPath = path.GetFullPath();

    // If already loaded, we are all set.  This might be called more than once over a project's
    // lifetime in case the project is first loaded by the KiCad manager and then Eeschema or
    // Pcbnew try to load it again when they are launched.
    if( m_projects.count( fullPath ) )
        return true;

    LOCKFILE lockFile( fullPath );

    if( !lockFile.Valid() )
    {
        wxLogTrace( traceSettings, wxT( "Project %s is locked; opening read-only" ), fullPath );
    }

    // No MDI yet
    if( aSetActive && !m_projects.empty() )
    {
        PROJECT* oldProject = m_projects.begin()->second;
        unloadProjectFile( oldProject, false );
        m_projects.erase( m_projects.begin() );

        auto it = std::find_if( m_projects_list.begin(), m_projects_list.end(),
                                [&]( const std::unique_ptr<PROJECT>& ptr )
                                {
                                    return ptr.get() == oldProject;
                                } );

        wxASSERT( it != m_projects_list.end() );
        m_projects_list.erase( it );
    }

    wxLogTrace( traceSettings, wxT( "Load project %s" ), fullPath );

    std::unique_ptr<PROJECT> project = std::make_unique<PROJECT>();
    project->setProjectFullName( fullPath );

    if( aSetActive )
    {
        // until multiple projects are in play, set an environment variable for the
        // the project pointer.
        wxFileName projectPath( fullPath );
        wxSetEnv( PROJECT_VAR_NAME, projectPath.GetPath() );

        // set the cwd but don't impact kicad-cli
        if( !projectPath.GetPath().IsEmpty() && wxTheApp && wxTheApp->IsGUI() )
            wxSetWorkingDirectory( projectPath.GetPath() );
    }

    bool success = loadProjectFile( *project );

    if( success )
    {
        project->SetReadOnly( !lockFile.Valid() || project->GetProjectFile().IsReadOnly() );

        if( lockFile && aSetActive )
            project->SetProjectLock( new LOCKFILE( std::move( lockFile ) ) );
    }

    m_projects_list.push_back( std::move( project ) );
    m_projects[fullPath] = m_projects_list.back().get();

    wxString fn( path.GetName() );

    PROJECT_LOCAL_SETTINGS* settings = new PROJECT_LOCAL_SETTINGS( m_projects[fullPath], fn );

    if( aSetActive )
        settings = RegisterSettings( settings );
    else
        settings->LoadFromFile( path.GetPath() );

    m_projects[fullPath]->setLocalSettings( settings );

    // If not running from SWIG; notify the library manager of the new project
    // TODO(JE) this maybe could be handled through kiway (below) in the future
    if( aSetActive && PgmOrNull() )
        Pgm().GetLibraryManager().ProjectChanged();

    if( aSetActive && m_kiway )
        m_kiway->ProjectChanged();

    return success;
}


bool SETTINGS_MANAGER::UnloadProject( PROJECT* aProject, bool aSave )
{
    if( !aProject || !m_projects.count( aProject->GetProjectFullName() ) )
        return false;

    if( !unloadProjectFile( aProject, aSave ) )
        return false;

    wxString projectPath = aProject->GetProjectFullName();
    wxLogTrace( traceSettings, wxT( "Unload project %s" ), projectPath );

    PROJECT* toRemove = m_projects.at( projectPath );
    bool wasActiveProject = m_projects_list.begin()->get() == toRemove;

    auto it = std::find_if( m_projects_list.begin(), m_projects_list.end(),
                            [&]( const std::unique_ptr<PROJECT>& ptr )
                            {
                                return ptr.get() == toRemove;
                            } );

    wxASSERT( it != m_projects_list.end() );
    m_projects_list.erase( it );

    m_projects.erase( projectPath );

    if( wasActiveProject )
    {
        // Immediately reload a null project; this is required until the rest of the application
        // is refactored to not assume that Prj() always works
        if( m_projects_list.empty() )
            LoadProject( "" );

        // Remove the reference in the environment to the previous project
        wxSetEnv( PROJECT_VAR_NAME, wxS( "" ) );

        if( m_kiway )
            m_kiway->ProjectChanged();
    }

    return true;
}


PROJECT& SETTINGS_MANAGER::Prj() const
{
    // No MDI yet:  First project in the list is the active project
    wxASSERT_MSG( m_projects_list.size(), wxT( "no project in list" ) );
    return *m_projects_list.begin()->get();
}


bool SETTINGS_MANAGER::IsProjectOpen() const
{
    return !m_projects.empty();
}


bool SETTINGS_MANAGER::IsProjectOpenNotDummy() const
{
    return m_projects.size() > 1 || ( m_projects.size() == 1
                                          && !m_projects.begin()->second->GetProjectFullName().IsEmpty() );
}


PROJECT* SETTINGS_MANAGER::GetProject( const wxString& aFullPath ) const
{
    if( m_projects.count( aFullPath ) )
        return m_projects.at( aFullPath );

    return nullptr;
}


std::vector<wxString> SETTINGS_MANAGER::GetOpenProjects() const
{
    std::vector<wxString> ret;

    for( const std::pair<const wxString, PROJECT*>& pair : m_projects )
    {
        // Don't save empty projects (these are the default project settings)
        if( !pair.first.IsEmpty() )
            ret.emplace_back( pair.first );
    }

    return ret;
}


bool SETTINGS_MANAGER::SaveProject( const wxString& aFullPath, PROJECT* aProject )
{
    if( !aProject )
        aProject = &Prj();

    wxString path = aFullPath;

    if( path.empty() )
        path = aProject->GetProjectFullName();

    // TODO: refactor for MDI
    if( aProject->IsReadOnly() )
        return false;

    if( !m_project_files.count( path ) )
        return false;

    PROJECT_FILE* project     = m_project_files.at( path );
    wxString      projectPath = aProject->GetProjectPath();

    project->SaveToFile( projectPath );
    aProject->GetLocalSettings().SaveToFile( projectPath );

    return true;
}


void SETTINGS_MANAGER::SaveProjectAs( const wxString& aFullPath, PROJECT* aProject )
{
    if( !aProject )
        aProject = &Prj();

    wxString oldName = aProject->GetProjectFullName();

    if( aFullPath.IsSameAs( oldName ) )
    {
        SaveProject( aFullPath, aProject );
        return;
    }

    // Changing this will cause UnloadProject to not save over the "old" project when loading below
    aProject->setProjectFullName( aFullPath );

    wxFileName fn( aFullPath );

    PROJECT_FILE* project = m_project_files.at( oldName );

    // Ensure read-only flags are copied; this allows doing a "Save As" on a standalone board/sch
    // without creating project files if the checkbox is turned off
    project->SetReadOnly( aProject->IsReadOnly() );
    aProject->GetLocalSettings().SetReadOnly( aProject->IsReadOnly() );

    project->SetFilename( fn.GetName() );
    project->SaveToFile( fn.GetPath() );

    aProject->GetLocalSettings().SetFilename( fn.GetName() );
    aProject->GetLocalSettings().SaveToFile( fn.GetPath() );

    m_project_files[fn.GetFullPath()] = project;
    m_project_files.erase( oldName );

    m_projects[fn.GetFullPath()] = m_projects[oldName];
    m_projects.erase( oldName );
}


void SETTINGS_MANAGER::SaveProjectCopy( const wxString& aFullPath, PROJECT* aProject )
{
    if( !aProject )
        aProject = &Prj();

    PROJECT_FILE* project = m_project_files.at( aProject->GetProjectFullName() );
    wxString      oldName = project->GetFilename();
    wxFileName    fn( aFullPath );

    bool readOnly = project->IsReadOnly();
    project->SetReadOnly( false );

    project->SetFilename( fn.GetName() );
    project->SaveToFile( fn.GetPath() );
    project->SetFilename( oldName );

    PROJECT_LOCAL_SETTINGS& localSettings = aProject->GetLocalSettings();

    localSettings.SetFilename( fn.GetName() );
    localSettings.SaveToFile( fn.GetPath() );
    localSettings.SetFilename( oldName );

    project->SetReadOnly( readOnly );
}


bool SETTINGS_MANAGER::loadProjectFile( PROJECT& aProject )
{
    wxFileName fullFn( aProject.GetProjectFullName() );
    wxString fn( fullFn.GetName() );

    PROJECT_FILE* file = RegisterSettings( new PROJECT_FILE( fn ), false );

    m_project_files[aProject.GetProjectFullName()] = file;

    aProject.setProjectFile( file );
    file->SetProject( &aProject );

    wxString path( fullFn.GetPath() );

    return file->LoadFromFile( path );
}


bool SETTINGS_MANAGER::unloadProjectFile( PROJECT* aProject, bool aSave )
{
    if( !aProject )
        return false;

    wxString name = aProject->GetProjectFullName();

    if( !m_project_files.count( name ) )
        return false;

    PROJECT_FILE* file = m_project_files[name];

    if( !file->ShouldAutoSave() )
        aSave = false;

    auto it = std::find_if( m_settings.begin(), m_settings.end(),
                            [&file]( const std::unique_ptr<JSON_SETTINGS>& aPtr )
                            {
                              return aPtr.get() == file;
                            } );

    if( it != m_settings.end() )
    {
        wxString projectPath = GetPathForSettingsFile( it->get() );

        bool saveLocalSettings = aSave && aProject->GetLocalSettings().ShouldAutoSave();

        FlushAndRelease( &aProject->GetLocalSettings(), saveLocalSettings );

        if( aSave )
            ( *it )->SaveToFile( projectPath );

        m_settings.erase( it );
    }

    m_project_files.erase( name );

    return true;
}


wxString SETTINGS_MANAGER::GetProjectBackupsPath() const
{
    return Prj().GetProjectPath() + Prj().GetProjectName() + PROJECT_BACKUPS_DIR_SUFFIX;
}


wxString SETTINGS_MANAGER::backupDateTimeFormat = wxT( "%Y-%m-%d_%H%M%S" );


bool SETTINGS_MANAGER::BackupProject( REPORTER& aReporter, wxFileName& aTarget ) const
{
    wxDateTime timestamp = wxDateTime::Now();

    wxString fileName = wxString::Format( wxT( "%s-%s" ), Prj().GetProjectName(),
                                          timestamp.Format( backupDateTimeFormat ) );

    if( !aTarget.IsOk() )
    {
        aTarget.SetPath( GetProjectBackupsPath() );
        aTarget.SetName( fileName );
        aTarget.SetExt( FILEEXT::ArchiveFileExtension );
    }

    if( !aTarget.DirExists() && !wxMkdir( aTarget.GetPath() ) )
    {
        wxLogTrace( traceSettings, wxT( "Could not create project backup path %s" ),
                    aTarget.GetPath() );
        return false;
    }

    if( !aTarget.IsDirWritable() )
    {
        wxLogTrace( traceSettings, wxT( "Backup directory %s is not writable" ),
                    aTarget.GetPath() );
        return false;
    }

    wxLogTrace( traceSettings, wxT( "Backing up project to %s" ), aTarget.GetPath() );

    return PROJECT_ARCHIVER::Archive( Prj().GetProjectPath(), aTarget.GetFullPath(), aReporter );
}


class VECTOR_INSERT_TRAVERSER : public wxDirTraverser
{
public:
    VECTOR_INSERT_TRAVERSER( std::vector<wxString>& aVec,
                             std::function<bool( const wxString& )> aCond ) :
            m_files( aVec ),
            m_condition( std::move( aCond ) )
    {
    }

    wxDirTraverseResult OnFile( const wxString& aFile ) override
    {
        if( m_condition( aFile ) )
            m_files.emplace_back( aFile );

        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnDir( const wxString& aDirName ) override
    {
        return wxDIR_CONTINUE;
    }

private:
    std::vector<wxString>& m_files;

    std::function<bool( const wxString& )> m_condition;
};


bool SETTINGS_MANAGER::TriggerBackupIfNeeded( REPORTER& aReporter ) const
{
    COMMON_SETTINGS::AUTO_BACKUP settings = GetCommonSettings()->m_Backup;

    if( !settings.enabled )
        return true;

    wxString prefix = Prj().GetProjectName() + '-';

    auto modTime =
            [&prefix]( const wxString& aFile )
            {
                wxDateTime dt;
                wxString fn( wxFileName( aFile ).GetName() );
                fn.Replace( prefix, wxS( "" ) );
                dt.ParseFormat( fn, backupDateTimeFormat );
                return dt;
            };

    wxFileName projectPath( Prj().GetProjectPath(), wxEmptyString, wxEmptyString );

    // Skip backup if project path isn't valid or writable
    if( !projectPath.IsOk() || !projectPath.Exists() || !projectPath.IsDirWritable() )
        return true;

    wxString backupPath = GetProjectBackupsPath();

    if( !wxDirExists( backupPath ) )
    {
        wxLogTrace( traceSettings, wxT( "Backup path %s doesn't exist, creating it" ), backupPath );

        if( !wxMkdir( backupPath ) )
        {
            wxLogTrace( traceSettings, wxT( "Could not create backups path!  Skipping backup" ) );
            return false;
        }
    }

    wxDir dir( backupPath );

    if( !dir.IsOpened() )
    {
        wxLogTrace( traceSettings, wxT( "Could not open project backups path %s" ), dir.GetName() );
        return false;
    }

    std::vector<wxString> files;

    VECTOR_INSERT_TRAVERSER traverser( files,
            [&modTime]( const wxString& aFile )
            {
                return modTime( aFile ).IsValid();
            } );

    dir.Traverse( traverser, wxT( "*.zip" ) );

    // Sort newest-first
    std::sort( files.begin(), files.end(),
               [&]( const wxString& aFirst, const wxString& aSecond ) -> bool
               {
                   wxDateTime first  = modTime( aFirst );
                   wxDateTime second = modTime( aSecond );

                   return first.GetTicks() > second.GetTicks();
               } );

    // Do we even need to back up?
    if( !files.empty() )
    {
        wxDateTime lastTime = modTime( files[0] );

        if( lastTime.IsValid() )
        {
            wxTimeSpan delta = wxDateTime::Now() - modTime( files[0] );

            if( delta.IsShorterThan( wxTimeSpan::Seconds( settings.min_interval ) ) )
                return true;
        }
    }

    // Backup
    wxFileName target;
    bool backupSuccessful = BackupProject( aReporter, target );

    if( !backupSuccessful )
        return false;

    // Update the file list
    files.insert( files.begin(), target.GetFullPath() );

    // Are there any changes since the last backup?
    if( files.size() >= 2
        && PROJECT_ARCHIVER::AreZipArchivesIdentical( files[0], files[1], aReporter ) )
    {
        wxRemoveFile( files[0] );
        return true;
    }

    // Now that we know a backup is needed, apply the retention policy

    // Step 1: if we're over the total file limit, remove the oldest
    if( !files.empty() && settings.limit_total_files > 0 )
    {
        while( files.size() > static_cast<size_t>( settings.limit_total_files ) )
        {
            wxRemoveFile( files.back() );
            files.pop_back();
        }
    }

    // Step 2: Stay under the total size limit
    if( settings.limit_total_size > 0 )
    {
        wxULongLong totalSize = 0;

        for( const wxString& file : files )
            totalSize += wxFileName::GetSize( file );

        while( !files.empty() && totalSize > static_cast<wxULongLong>( settings.limit_total_size ) )
        {
            totalSize -= wxFileName::GetSize( files.back() );
            wxRemoveFile( files.back() );
            files.pop_back();
        }
    }

    // Step 3: Stay under the daily limit
    if( settings.limit_daily_files > 0 && files.size() > 1 )
    {
        wxDateTime day = modTime( files[0] );
        int        num = 1;

        wxASSERT( day.IsValid() );

        std::vector<wxString> filesToDelete;

        for( size_t i = 1; i < files.size(); i++ )
        {
            wxDateTime dt = modTime( files[i] );

            if( dt.IsSameDate( day ) )
            {
                num++;

                if( num > settings.limit_daily_files )
                    filesToDelete.emplace_back( files[i] );
            }
            else
            {
                day = dt;
                num = 1;
            }
        }

        for( const wxString& file : filesToDelete )
            wxRemoveFile( file );
    }

    return true;
}


wxString SETTINGS_MANAGER::GetUserSettingsPath()
{
    return PATHS::GetUserSettingsPath();
}
