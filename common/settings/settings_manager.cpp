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

#include <regex>
#include <wx/debug.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

#include <build_version.h>
#include <confirm.h>
#include <dialogs/dialog_migrate_settings.h>
#include <gestfich.h>
#include <macros.h>
#include <project.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/app_settings.h>
#include <settings/color_settings.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <wildcards_and_files_ext.h>


/**
 * Flag to enable settings tracing
 * @ingroup trace_env_vars
 */
const char* traceSettings = "SETTINGS";


/// Project settings path will be <projectname> + this
#define PROJECT_BACKUPS_DIR_SUFFIX wxT( "-backups" )


SETTINGS_MANAGER::SETTINGS_MANAGER( bool aHeadless ) :
        m_headless( aHeadless ),
        m_common_settings( nullptr ),
        m_migration_source()
{
    // Check if the settings directory already exists, and if not, perform a migration if possible
    if( !MigrateIfNeeded() )
    {
        m_ok = false;
        return;
    }

    m_ok = true;

    // create the common settings shared by all applications.  Not loaded immediately
    m_common_settings =
            static_cast<COMMON_SETTINGS*>( RegisterSettings( new COMMON_SETTINGS, false ) );

    loadAllColorSettings();
}

SETTINGS_MANAGER::~SETTINGS_MANAGER()
{
    m_settings.clear();
    m_color_settings.clear();
    m_projects.clear();
}


JSON_SETTINGS* SETTINGS_MANAGER::RegisterSettings( JSON_SETTINGS* aSettings, bool aLoadNow )
{
    std::unique_ptr<JSON_SETTINGS> ptr( aSettings );

    ptr->SetManager( this );

    wxLogTrace( traceSettings, "Registered new settings object %s", ptr->GetFullFilename() );

    if( aLoadNow )
        ptr->LoadFromFile( GetPathForSettingsFile( ptr.get() ) );

    m_settings.push_back( std::move( ptr ) );
    return m_settings.back().get();
}


void SETTINGS_MANAGER::Load()
{
    // TODO(JE) We should check for dirty settings here and write them if so, because
    // Load() could be called late in the application lifecycle

    for( auto&& settings : m_settings )
        settings->LoadFromFile( GetPathForSettingsFile( settings.get() ) );
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
        wxLogTrace( traceSettings, "Saving %s", ( *it )->GetFullFilename() );
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
        wxLogTrace( traceSettings, "Flush and release %s", ( *it )->GetFullFilename() );

        if( aSave )
            ( *it )->SaveToFile( GetPathForSettingsFile( it->get() ) );

        m_settings.erase( it );
    }
}


COLOR_SETTINGS* SETTINGS_MANAGER::GetColorSettings( const wxString& aName )
{
    if( m_color_settings.count( aName ) )
        return m_color_settings.at( aName );

    COLOR_SETTINGS* ret = nullptr;

    if( !aName.empty() )
        ret = loadColorSettingsByName( aName );

    // This had better work
    if( !ret )
        ret = m_color_settings.at( "user" );

    return ret;
}


COLOR_SETTINGS* SETTINGS_MANAGER::loadColorSettingsByName( const wxString& aName )
{
    wxLogTrace( traceSettings, "Attempting to load color theme %s", aName );

    wxFileName fn( GetColorSettingsPath(), aName, "json" );

    if( !fn.IsOk() || !fn.Exists() )
    {
        wxLogTrace( traceSettings, "Theme file %s.json not found, falling back to user", aName );
        return nullptr;
    }

    auto cs = static_cast<COLOR_SETTINGS*>(
            RegisterSettings( new COLOR_SETTINGS( aName.ToStdString() ) ) );

    if( cs->GetFilename() != aName.ToStdString() )
    {
        wxLogTrace( traceSettings, "Warning: stored filename is actually %s, ", cs->GetFilename() );
    }

    m_color_settings[aName] = cs;

    return cs;
}


class COLOR_SETTINGS_LOADER : public wxDirTraverser
{
private:
    std::function<void( const wxString& )> m_action;

public:
    explicit COLOR_SETTINGS_LOADER( std::function<void( const wxString& )> aAction )
            : m_action( std::move( aAction ) )
    {
    }

    wxDirTraverseResult OnFile( const wxString& aFilePath ) override
    {
        wxFileName file( aFilePath );

        if( file.GetExt() != "json" )
            return wxDIR_CONTINUE;

        if( file.GetName() == "user" )
            return wxDIR_CONTINUE;

        m_action( file.GetName() );

        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnDir( const wxString& dirPath ) override
    {
        return wxDIR_IGNORE;
    }
};


void SETTINGS_MANAGER::registerColorSettings( const wxString& aFilename )
{
    if( m_color_settings.count( aFilename ) )
        return;

    m_color_settings[aFilename] = static_cast<COLOR_SETTINGS*>(
            RegisterSettings( new COLOR_SETTINGS( aFilename.ToStdString() ) ) );
}


COLOR_SETTINGS* SETTINGS_MANAGER::AddNewColorSettings( const wxString& aFilename )
{
    wxString filename = aFilename;

    if( filename.EndsWith( wxT( ".json" ) ) )
        filename = filename.BeforeLast( '.' );

    registerColorSettings( filename );
    return m_color_settings[filename];
}


void SETTINGS_MANAGER::loadAllColorSettings()
{
    // Create the default color settings
    registerColorSettings( "user" );

    // Search for and load any other settings
    COLOR_SETTINGS_LOADER loader( [&]( const wxString& aFilename )
                                  {
                                      registerColorSettings( aFilename );
                                  } );

    wxDir colors_dir( GetColorSettingsPath() );

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

    nlohmann::json::json_pointer ptr = JSON_SETTINGS::PointerFromString( aNamespace );

    if( !aSettings->Store() )
    {
        wxLogTrace( traceSettings, "Color scheme %s not modified; skipping save",
                aSettings->GetFilename(), aNamespace );
        return;
    }

    wxASSERT( aSettings->contains( ptr ) );

    wxLogTrace( traceSettings, "Saving color scheme %s, preserving %s", aSettings->GetFilename(),
            aNamespace );

    nlohmann::json backup = aSettings->at( ptr );
    std::string path = GetColorSettingsPath();

    aSettings->LoadFromFile( path );

    ( *aSettings )[ptr].update( backup );
    aSettings->Load();

    aSettings->SaveToFile( path, true );
}


std::string SETTINGS_MANAGER::GetPathForSettingsFile( JSON_SETTINGS* aSettings )
{
    wxASSERT( aSettings );

    switch( aSettings->GetLocation() )
    {
    case SETTINGS_LOC::USER:
        return GetUserSettingsPath();

    case SETTINGS_LOC::PROJECT:
        return std::string( Prj().GetProjectPath().ToUTF8() );

    case SETTINGS_LOC::COLORS:
        return GetColorSettingsPath();

    case SETTINGS_LOC::NONE:
        return "";

    default:
        wxASSERT_MSG( false, "Unknown settings location!" );
    }

    return "";
}


class MIGRATION_TRAVERSER : public wxDirTraverser
{
private:
    wxString m_src;
    wxString m_dest;
    wxString m_errors;

public:
    MIGRATION_TRAVERSER( const wxString& aSrcDir, const wxString& aDestDir ) :
            m_src( aSrcDir ), m_dest( aDestDir )
    {
    }

    wxString GetErrors() { return m_errors; }

    virtual wxDirTraverseResult OnFile( const wxString& aSrcFilePath ) override
    {
        wxFileName file( aSrcFilePath );
        wxString path = file.GetPath();

        path.Replace( m_src, m_dest, false );
        file.SetPath( path );

        wxLogTrace( traceSettings, "Copying %s to %s", aSrcFilePath, file.GetFullPath() );

        // For now, just copy everything
        CopyFile( aSrcFilePath, file.GetFullPath(), m_errors );

        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnDir( const wxString& dirPath ) override
    {
        wxFileName dir( dirPath );

        // Whitelist of directories to migrate
        if( dir.GetName() == "colors" ||
            dir.GetName() == "3d" )
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
    wxFileName path( GetUserSettingsPath(), "" );
    wxLogTrace( traceSettings, "Using settings path %s", path.GetFullPath() );

    if( path.DirExists() )
    {
        wxFileName common = path;
        common.SetName( "kicad_common" );
        common.SetExt( "json" );

        if( common.Exists() )
        {
            wxLogTrace( traceSettings, "Path exists and has a kicad_common, continuing!" );
            return true;
        }
    }

    if( m_headless )
    {
        wxLogTrace( traceSettings, "Manual settings migration required but running headless!" );
        return false;
    }

    // Now we have an empty path, let's figure out what to put in it
    DIALOG_MIGRATE_SETTINGS dlg( this );

    if( dlg.ShowModal() != wxID_OK )
    {
        wxLogTrace( traceSettings, "Migration dialog canceled; exiting" );
        return false;
    }

    if( !path.DirExists() )
    {
        wxLogTrace( traceSettings, "Path didn't exist; creating it" );
        path.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    }

    if( m_migration_source.IsEmpty() )
    {
        wxLogTrace( traceSettings, "No migration source given; starting with defaults" );
        return true;
    }

    MIGRATION_TRAVERSER traverser( m_migration_source, path.GetFullPath() );
    wxDir source_dir( m_migration_source );

    source_dir.Traverse( traverser );

    if( !traverser.GetErrors().empty() )
        DisplayErrorMessage( nullptr, traverser.GetErrors() );

    return true;
}


bool SETTINGS_MANAGER::GetPreviousVersionPaths( std::vector<wxString>* aPaths )
{
    wxASSERT( aPaths );

    aPaths->clear();

    wxDir dir;
    std::vector<wxFileName> base_paths;

    base_paths.emplace_back( wxFileName( calculateUserSettingsPath( false ), "" ) );

    // If the env override is set, also check the default paths
    if( wxGetEnv( wxT( "KICAD_CONFIG_HOME" ), nullptr ) )
        base_paths.emplace_back( wxFileName( calculateUserSettingsPath( false, false ), "" ) );

    wxString subdir;
    std::string mine = GetSettingsVersion();

    auto check_dir = [&] ( const wxString& aSubDir )
    {
        // Only older versions are valid for migration
        if( compareVersions( aSubDir.ToStdString(), mine ) <= 0 )
        {
            wxString sub_path = dir.GetNameWithSep() + aSubDir;

            if( IsSettingsPathValid( sub_path ) )
            {
                aPaths->push_back( sub_path );
                wxLogTrace( traceSettings, "GetPreviousVersionName: %s is valid", sub_path );
            }
        }
    };

    for( auto base_path : base_paths )
    {
        if( !dir.Open( base_path.GetFullPath() ) )
        {
            wxLogTrace( traceSettings, "GetPreviousVersionName: could not open base path %s",
                    base_path.GetFullPath() );
            continue;
        }

        wxLogTrace( traceSettings, "GetPreviousVersionName: checking base path %s",
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
                    "GetPreviousVersionName: root path %s is valid", dir.GetName() );
            aPaths->push_back( dir.GetName() );
        }
    }

    return aPaths->size() > 0;
}


bool SETTINGS_MANAGER::IsSettingsPathValid( const wxString& aPath )
{
    wxFileName test( aPath, "kicad_common" );
    return test.Exists();
}


std::string SETTINGS_MANAGER::GetColorSettingsPath()
{
    wxFileName path;

    path.AssignDir( GetUserSettingsPath() );
    path.AppendDir( "colors" );

    return path.GetPath().ToStdString();
}


std::string SETTINGS_MANAGER::GetUserSettingsPath()
{
    static std::string user_settings_path;

    if( user_settings_path.empty() )
        user_settings_path = calculateUserSettingsPath();

    return user_settings_path;
}


std::string SETTINGS_MANAGER::calculateUserSettingsPath( bool aIncludeVer, bool aUseEnv )
{
    wxFileName cfgpath;

    // http://docs.wxwidgets.org/3.0/classwx_standard_paths.html#a7c7cf595d94d29147360d031647476b0
    cfgpath.AssignDir( wxStandardPaths::Get().GetUserConfigDir() );

    // GetUserConfigDir() does not default to ~/.config which is the current standard
    // configuration file location on Linux.  This has been fixed in later versions of wxWidgets.
#if !defined( __WXMSW__ ) && !defined( __WXMAC__ )
    wxArrayString dirs = cfgpath.GetDirs();

    if( dirs.Last() != ".config" )
        cfgpath.AppendDir( ".config" );
#endif

    wxString envstr;

    // This shouldn't cause any issues on Windows or MacOS.
    if( wxGetEnv( wxT( "XDG_CONFIG_HOME" ), &envstr ) && !envstr.IsEmpty() )
    {
        // Override the assignment above with XDG_CONFIG_HOME
        cfgpath.AssignDir( envstr );
    }

    cfgpath.AppendDir( TO_STR( KICAD_CONFIG_DIR ) );

    // Use KICAD_CONFIG_HOME to allow the user to force a specific configuration path.
    if( aUseEnv && wxGetEnv( wxT( "KICAD_CONFIG_HOME" ), &envstr ) && !envstr.IsEmpty() )
    {
        // Override the assignment above with KICAD_CONFIG_HOME
        cfgpath.AssignDir( envstr );
    }

    if( aIncludeVer )
        cfgpath.AppendDir( GetSettingsVersion() );

    return cfgpath.GetPath().ToStdString();
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
        wxLogTrace( traceSettings, "compareSettingsVersions: bad input (%s, %s)", aFirst, aSecond );
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
            *aMajor = std::stoi( match[1].str() );
            *aMinor = std::stoi( match[2].str() );
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

    if( path.GetExt() == LegacyProjectFileExtension )
        path.SetExt( ProjectFileExtension );

    wxString fullPath = path.GetFullPath();

    // If already loaded, we are all set.  This might be called more than once over a project's
    // lifetime in case the project is first loaded by the KiCad manager and then eeschema or
    // pcbnew try to load it again when they are launched.
    if( m_projects.count( fullPath ) )
        return true;

    // No MDI yet
    if( aSetActive && !m_projects.empty() && !UnloadProject( m_projects.begin()->second.get() ) )
        return false;

    wxLogTrace( traceSettings, "Load project %s", fullPath );

    std::unique_ptr<PROJECT> project = std::make_unique<PROJECT>();
    project->setProjectFullName( fullPath );

    bool success = loadProjectFile( *project );

    m_projects[fullPath].reset( project.release() );

    std::string fn( path.GetName() );

    PROJECT_LOCAL_SETTINGS* settings = static_cast<PROJECT_LOCAL_SETTINGS*>(
            RegisterSettings( new PROJECT_LOCAL_SETTINGS( fn ) ) );

    m_projects[fullPath]->setLocalSettings( settings );
    settings->SetProject( m_projects[fullPath].get() );

    return success;
}


bool SETTINGS_MANAGER::UnloadProject( PROJECT* aProject, bool aSave )
{
    if( !aProject || !m_projects.count( aProject->GetProjectFullName() ) )
        return false;

    if( !unloadProjectFile( aProject, aSave ) )
        return false;

    wxLogTrace( traceSettings, "Unload project %s", aProject->GetProjectFullName() );

    m_projects.erase( aProject->GetProjectFullName() );

    return true;
}


PROJECT& SETTINGS_MANAGER::Prj() const
{
    // No MDI yet:  First project in the list is the active project
    return *m_projects.begin()->second;
}


PROJECT* SETTINGS_MANAGER::GetProject( const wxString& aFullPath ) const
{
    if( m_projects.count( aFullPath ) )
        return m_projects.at( aFullPath ).get();

    return nullptr;
}


bool SETTINGS_MANAGER::SaveProject( const wxString& aFullPath )
{
    wxString path = aFullPath;

    if( path.empty() )
        path = Prj().GetProjectFullName();

    if( !m_project_files.count( path ) )
        return false;

    PROJECT_FILE* project     = m_project_files.at( path );
    std::string   projectPath = GetPathForSettingsFile( project );

    project->SaveToFile( projectPath );
    Prj().GetLocalSettings().SaveToFile( projectPath );

    return true;
}


bool SETTINGS_MANAGER::loadProjectFile( PROJECT& aProject )
{
    wxFileName fullFn( aProject.GetProjectFullName() );
    std::string fn( fullFn.GetName().ToUTF8() );

    PROJECT_FILE* file =
            static_cast<PROJECT_FILE*>( RegisterSettings( new PROJECT_FILE( fn ), false ) );

    m_project_files[aProject.GetProjectFullName()] = file;

    aProject.setProjectFile( file );
    file->SetProject( &aProject );

    return file->LoadFromFile( std::string( fullFn.GetPath().ToUTF8() ) );
}


bool SETTINGS_MANAGER::unloadProjectFile( PROJECT* aProject, bool aSave )
{
    if( !aProject )
        return false;

    wxString name = aProject->GetProjectFullName();

    if( !m_project_files.count( name ) )
        return false;

    PROJECT_FILE* file = m_project_files[name];

    auto it = std::find_if( m_settings.begin(), m_settings.end(),
                            [&file]( const std::unique_ptr<JSON_SETTINGS>& aPtr )
                            {
                              return aPtr.get() == file;
                            } );

    if( it != m_settings.end() )
    {
        std::string projectPath = GetPathForSettingsFile( it->get() );

        FlushAndRelease( &aProject->GetLocalSettings(), aSave );

        if( aSave )
            ( *it )->SaveToFile( projectPath );

        m_settings.erase( it );
    }

    m_project_files.erase( name );

    return true;
}
