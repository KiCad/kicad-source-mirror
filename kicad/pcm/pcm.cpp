/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
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

// kicad_curl.h *must be* included before any wxWidgets header to avoid conflicts
// at least on Windows/msys2
#include "kicad_curl/kicad_curl_easy.h"
#include <kicad_curl/kicad_curl.h>

#include "core/wx_stl_compat.h"
#include <env_vars.h>
#include <background_jobs_monitor.h>
#include <json_schema_validator.h>
#include "build_version.h"
#include "paths.h"
#include "pcm.h"
#include <eda_base_frame.h>
#include "dialogs/dialog_pcm.h"
#include "pgm_base.h"
#include "picosha2.h"
#include "settings/settings_manager.h"
#include <wx_filename.h>
#include <settings/common_settings.h>
#include <kiway.h>

#include <fstream>
#include <iomanip>
#include <memory>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>


/**
 * Flag to enable PCM debugging output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar tracePcm[] = wxT( "KICAD_PCM" );


const std::tuple<int, int, int> PLUGIN_CONTENT_MANAGER::m_kicad_version =
        GetMajorMinorPatchTuple();


class THROWING_ERROR_HANDLER : public nlohmann::json_schema::error_handler
{
    void error( const json::json_pointer& ptr, const json& instance,
                const std::string& message ) override
    {
        throw std::invalid_argument( std::string( "At " ) + ptr.to_string() + ", value:\n"
                                     + instance.dump() + "\n" + message + "\n" );
    }
};

#include <locale_io.h>
PLUGIN_CONTENT_MANAGER::PLUGIN_CONTENT_MANAGER(
                                        std::function<void( int )> aAvailableUpdateCallback ) :
        m_dialog( nullptr ),
        m_availableUpdateCallback( aAvailableUpdateCallback )
{
    ReadEnvVar();

    // Read and store pcm schema
    wxFileName schema_file( PATHS::GetStockDataPath( true ), wxS( "pcm.v1.schema.json" ) );
    schema_file.Normalize( FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS );
    schema_file.AppendDir( wxS( "schemas" ) );

    m_schema_validator = std::make_unique<JSON_SCHEMA_VALIDATOR>( schema_file );

    // Load currently installed packages
    wxFileName f( PATHS::GetUserSettingsPath(), wxT( "installed_packages.json" ) );

    if( f.FileExists() )
    {
        std::ifstream  installed_stream( f.GetFullPath().fn_str() );
        nlohmann::json installed;

        try
        {
            installed_stream >> installed;

            if( installed.contains( "packages" ) && installed["packages"].is_array() )
            {
                for( const auto& js_entry : installed["packages"] )
                {
                    PCM_INSTALLATION_ENTRY entry = js_entry.get<PCM_INSTALLATION_ENTRY>();
                    m_installed.emplace( entry.package.identifier, entry );
                }
            }
        }
        catch( std::exception& e )
        {
            wxLogError( wxString::Format( _( "Error loading installed packages list: %s" ),
                                          e.what() ) );
        }
    }

    // As a fall back populate installed from names of directories

    for( const wxString& dir : PCM_PACKAGE_DIRECTORIES )
    {
        wxFileName d( m_3rdparty_path, wxEmptyString );
        d.AppendDir( dir );

        if( d.DirExists() )
        {
            wxDir package_dir( d.GetPath() );

            if( !package_dir.IsOpened() )
                continue;

            wxString subdir;
            bool     more = package_dir.GetFirst( &subdir, "", wxDIR_DIRS | wxDIR_HIDDEN );

            while( more )
            {
                wxString actual_package_id = subdir;
                actual_package_id.Replace( '_', '.' );

                if( m_installed.find( actual_package_id ) == m_installed.end() )
                {
                    PCM_INSTALLATION_ENTRY entry;
                    wxFileName             subdir_file( d.GetPath(), subdir );

                    // wxFileModificationTime bugs out on windows for directories
                    wxStructStat stat;
                    int          stat_code = wxStat( subdir_file.GetFullPath(), &stat );

                    entry.package.name = subdir;
                    entry.package.identifier = actual_package_id;
                    entry.current_version = "0.0";
                    entry.repository_name = wxT( "<unknown>" );

                    if( stat_code == 0 )
                        entry.install_timestamp = stat.st_mtime;

                    PACKAGE_VERSION version;
                    version.version = "0.0";
                    version.status = PVS_STABLE;
                    version.kicad_version = GetMajorMinorVersion();

                    entry.package.versions.emplace_back( version );

                    m_installed.emplace( actual_package_id, entry );
                }

                more = package_dir.GetNext( &subdir );
            }
        }
    }

    // Calculate package compatibility
    std::for_each( m_installed.begin(), m_installed.end(),
                   [&]( std::pair<const wxString, PCM_INSTALLATION_ENTRY>& entry )
                   {
                       PreparePackage( entry.second.package );
                   } );
}


void PLUGIN_CONTENT_MANAGER::ReadEnvVar()
{
    // Get 3rd party path
    const ENV_VAR_MAP& env = Pgm().GetLocalEnvVariables();

    if( std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( env, wxT( "3RD_PARTY" ) ) )
        m_3rdparty_path = *v;
    else
        m_3rdparty_path = PATHS::GetDefault3rdPartyPath();
}


bool PLUGIN_CONTENT_MANAGER::DownloadToStream( const wxString& aUrl, std::ostream* aOutput,
                                               PROGRESS_REPORTER* aReporter,
                                               const size_t       aSizeLimit )
{
    bool size_exceeded = false;

    TRANSFER_CALLBACK callback = [&]( size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow )
    {
        if( aSizeLimit > 0 && ( dltotal > aSizeLimit || dlnow > aSizeLimit ) )
        {
            size_exceeded = true;

            // Non zero return means abort.
            return true;
        }

        if( dltotal > 1000 )
        {
            aReporter->SetCurrentProgress( dlnow / (double) dltotal );
            aReporter->Report( wxString::Format( _( "Downloading %lld/%lld kB" ), dlnow / 1000,
                                                 dltotal / 1000 ) );
        }
        else
        {
            aReporter->SetCurrentProgress( 0.0 );
        }

        return !aReporter->KeepRefreshing();
    };

    KICAD_CURL_EASY curl;
    curl.SetOutputStream( aOutput );
    curl.SetURL( aUrl.ToUTF8().data() );
    curl.SetFollowRedirects( true );
    curl.SetTransferCallback( callback, 250000L );

    int code = curl.Perform();

    if( !aReporter->IsCancelled() )
        aReporter->SetCurrentProgress( 1.0 );

    if( code != CURLE_OK )
    {
        if( m_dialog )
        {
            if( code == CURLE_ABORTED_BY_CALLBACK && size_exceeded )
                wxMessageBox( _( "Download is too large." ) );
            else if( code != CURLE_ABORTED_BY_CALLBACK )
                wxLogError( wxString( curl.GetErrorText( code ) ) );
        }

        return false;
    }

    return true;
}


bool PLUGIN_CONTENT_MANAGER::FetchRepository( const wxString& aUrl, PCM_REPOSITORY& aRepository,
                                              PROGRESS_REPORTER* aReporter )
{
    std::stringstream repository_stream;

    aReporter->SetTitle( _( "Fetching repository" ) );

    if( !DownloadToStream( aUrl, &repository_stream, aReporter, 20480 ) )
        return false;

    nlohmann::json repository_json;

    try
    {
        repository_stream >> repository_json;

        ValidateJson( repository_json, nlohmann::json_uri( "#/definitions/Repository" ) );

        aRepository = repository_json.get<PCM_REPOSITORY>();
    }
    catch( const std::exception& e )
    {
        if( m_dialog )
        {
            wxLogError( _( "Unable to parse repository: %s" ), e.what() );
            wxLogError( _( "The given repository URL does not look like a valid KiCad package "
                           "repository. Please double check the URL." ) );
        }

        return false;
    }

    return true;
}


void PLUGIN_CONTENT_MANAGER::ValidateJson( const nlohmann::json&     aJson,
                                           const nlohmann::json_uri& aUri ) const
{
    THROWING_ERROR_HANDLER error_handler;
    m_schema_validator->Validate( aJson, error_handler, aUri );
}


bool PLUGIN_CONTENT_MANAGER::fetchPackages( const wxString&                aUrl,
                                            const std::optional<wxString>& aHash,
                                            std::vector<PCM_PACKAGE>&      aPackages,
                                            PROGRESS_REPORTER*             aReporter )
{
    std::stringstream packages_stream;

    aReporter->SetTitle( _( "Fetching repository packages" ) );

    if( !DownloadToStream( aUrl, &packages_stream, aReporter ) )
    {
        if( m_dialog )
            wxLogError( _( "Unable to load repository packages url." ) );

        return false;
    }

    std::istringstream isstream( packages_stream.str() );

    if( aHash && !VerifyHash( isstream, *aHash ) )
    {
        if( m_dialog )
            wxLogError( _( "Packages hash doesn't match. Repository may be corrupted." ) );

        return false;
    }

    try
    {
        nlohmann::json packages_json = nlohmann::json::parse( packages_stream.str() );
        ValidateJson( packages_json, nlohmann::json_uri( "#/definitions/PackageArray" ) );

        aPackages = packages_json["packages"].get<std::vector<PCM_PACKAGE>>();
    }
    catch( std::exception& e )
    {
        if( m_dialog )
        {
            wxLogError( wxString::Format( _( "Unable to parse packages metadata:\n\n%s" ),
                                          e.what() ) );
        }

        return false;
    }

    return true;
}


bool PLUGIN_CONTENT_MANAGER::VerifyHash( std::istream& aStream, const wxString& aHash ) const
{
    std::vector<unsigned char> bytes( picosha2::k_digest_size );

    picosha2::hash256( std::istreambuf_iterator<char>( aStream ), std::istreambuf_iterator<char>(),
                       bytes.begin(), bytes.end() );
    std::string hex_str = picosha2::bytes_to_hex_string( bytes.begin(), bytes.end() );

    return aHash.compare( hex_str ) == 0;
}


const PCM_REPOSITORY&
PLUGIN_CONTENT_MANAGER::getCachedRepository( const wxString& aRepositoryId ) const
{
    wxASSERT_MSG( m_repository_cache.find( aRepositoryId ) != m_repository_cache.end(),
                  wxT( "Repository is not cached." ) );

    return m_repository_cache.at( aRepositoryId );
}


bool PLUGIN_CONTENT_MANAGER::CacheRepository( const wxString& aRepositoryId )
{
    if( m_repository_cache.find( aRepositoryId ) != m_repository_cache.end() )
        return true;

    const auto repository_tuple =
            std::find_if( m_repository_list.begin(), m_repository_list.end(),
                          [&aRepositoryId]( const std::tuple<wxString, wxString, wxString>& t )
                          {
                              return std::get<0>( t ) == aRepositoryId;
                          } );

    if( repository_tuple == m_repository_list.end() )
        return false;

    wxString url = std::get<2>( *repository_tuple );

    nlohmann::json  js;
    PCM_REPOSITORY  current_repo;
    PCM_REPOSITORY& current_repo_ref = current_repo;

    std::shared_ptr<PROGRESS_REPORTER> reporter;

    if( m_dialog )
        reporter = std::make_shared<WX_PROGRESS_REPORTER>( m_dialog, wxT( "" ), 1, PR_CAN_ABORT );
    else
        reporter = m_updateBackgroundJob->m_reporter;

    if( !FetchRepository( url, current_repo, reporter.get() ) )
        return false;

    bool packages_cache_exists = false;

    // First load repository data from local filesystem if available.
    wxFileName repo_cache = wxFileName( PATHS::GetUserCachePath(), wxT( "repository.json" ) );
    repo_cache.AppendDir( wxT( "pcm" ) );
    repo_cache.AppendDir( aRepositoryId );
    wxFileName packages_cache( repo_cache.GetPath(), wxT( "packages.json" ) );

    if( repo_cache.FileExists() && packages_cache.FileExists() )
    {
        std::ifstream  repo_stream( repo_cache.GetFullPath().fn_str() );
        PCM_REPOSITORY saved_repo;
        try
        {
            repo_stream >> js;
            saved_repo = js.get<PCM_REPOSITORY>();
        }
        catch( ... )
        {
            if( m_dialog )
                wxLogError( _( "Failed to parse locally stored repository.json." ) );
        }

        if( saved_repo.packages.update_timestamp == current_repo.packages.update_timestamp )
        {
            // Cached repo is up to date, use data on disk
            js.clear();
            std::ifstream packages_cache_stream( packages_cache.GetFullPath().fn_str() );

            try
            {
                packages_cache_stream >> js;
                saved_repo.package_list = js["packages"].get<std::vector<PCM_PACKAGE>>();

                for( size_t i = 0; i < saved_repo.package_list.size(); i++ )
                {
                    PreparePackage( saved_repo.package_list[i] );
                    saved_repo.package_map[saved_repo.package_list[i].identifier] = i;
                }

                m_repository_cache[aRepositoryId] = std::move( saved_repo );

                packages_cache_exists = true;
            }
            catch( ... )
            {
                if( m_dialog )
                {
                    wxLogError( _( "Packages cache for current repository is corrupted, it will "
                                   "be redownloaded." ) );
                }
            }
        }
    }

    if( !packages_cache_exists )
    {
        // Cache doesn't exist or is out of date
        if( !fetchPackages( current_repo.packages.url, current_repo.packages.sha256,
                            current_repo.package_list, reporter.get() ) )
        {
            return false;
        }

        for( size_t i = 0; i < current_repo.package_list.size(); i++ )
        {
            PreparePackage( current_repo.package_list[i] );
            current_repo.package_map[current_repo.package_list[i].identifier] = i;
        }

        repo_cache.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        std::ofstream repo_cache_stream( repo_cache.GetFullPath().fn_str() );
        repo_cache_stream << std::setw( 4 ) << nlohmann::json( current_repo ) << std::endl;

        std::ofstream packages_cache_stream( packages_cache.GetFullPath().fn_str() );
        js.clear();
        js["packages"] = nlohmann::json( current_repo.package_list );
        packages_cache_stream << std::setw( 4 ) << js << std::endl;

        m_repository_cache[aRepositoryId] = std::move( current_repo );
        current_repo_ref = m_repository_cache[aRepositoryId];
    }

    if( current_repo_ref.resources )
    {
        // Check resources file date, redownload if needed
        PCM_RESOURCE_REFERENCE& resources = *current_repo_ref.resources;

        wxFileName resource_file( repo_cache.GetPath(), wxT( "resources.zip" ) );

        time_t mtime = 0;

        if( resource_file.FileExists() )
            mtime = wxFileModificationTime( resource_file.GetFullPath() );

        if( mtime + 600 < getCurrentTimestamp() && mtime < (time_t) resources.update_timestamp )
        {
            std::ofstream resources_stream( resource_file.GetFullPath().fn_str(),
                                            std::ios_base::binary );

            reporter->SetTitle( _( "Downloading resources" ) );

            // 100 Mb resource file limit
            bool success = DownloadToStream( resources.url, &resources_stream, reporter.get(),
                                             100 * 1024 * 1024 );

            resources_stream.close();

            if( success )
            {
                std::ifstream read_stream( resource_file.GetFullPath().fn_str(),
                                           std::ios_base::binary );


                if( resources.sha256 && !VerifyHash( read_stream, *resources.sha256 ) )
                {
                    read_stream.close();

                    if( m_dialog )
                    {
                        wxLogError( _( "Resources file hash doesn't match and will not be used. "
                                       "Repository may be corrupted." ) );
                    }

                    wxRemoveFile( resource_file.GetFullPath() );
                }
            }
            else
            {
                // Not critical, just clean up the file
                wxRemoveFile( resource_file.GetFullPath() );
            }
        }
    }

    updateInstalledPackagesMetadata( aRepositoryId );

    return true;
}


void PLUGIN_CONTENT_MANAGER::updateInstalledPackagesMetadata( const wxString& aRepositoryId )
{
    const PCM_REPOSITORY* repository;

    try
    {
        repository = &getCachedRepository( aRepositoryId );
    }
    catch( ... )
    {
        wxLogTrace( tracePcm, wxS( "Invalid/Missing repository " ) + aRepositoryId );
        return;
    }

    for( std::pair<const wxString, PCM_INSTALLATION_ENTRY>& pair : m_installed )
    {
        PCM_INSTALLATION_ENTRY& entry = pair.second;

        // If current package is not from this repository, skip it
        if( entry.repository_id != aRepositoryId )
            continue;

        // If current package is no longer in this repository, keep it as is
        if( repository->package_map.count( entry.package.identifier ) == 0 )
            continue;

        std::optional<PACKAGE_VERSION> current_version;

        auto current_version_it =
                std::find_if( entry.package.versions.begin(), entry.package.versions.end(),
                              [&]( const PACKAGE_VERSION& version )
                              {
                                  return version.version == entry.current_version;
                              } );

        if( current_version_it != entry.package.versions.end() )
            current_version = *current_version_it; // copy

        // Copy repository metadata into installation entry
        entry.package = repository->package_list[repository->package_map.at( entry.package.identifier )];

        // Insert current version if it's missing from repository metadata
        current_version_it =
                std::find_if( entry.package.versions.begin(), entry.package.versions.end(),
                              [&]( const PACKAGE_VERSION& version )
                              {
                                  return version.version == entry.current_version;
                              } );

        if( current_version_it == entry.package.versions.end() )
        {
            entry.package.versions.emplace_back( *current_version );

            // Re-sort the versions by descending version
            std::sort( entry.package.versions.begin(), entry.package.versions.end(),
                       []( const PACKAGE_VERSION& a, const PACKAGE_VERSION& b )
                       {
                           return a.parsed_version > b.parsed_version;
                       } );
        }
    }
}


void PLUGIN_CONTENT_MANAGER::PreparePackage( PCM_PACKAGE& aPackage )
{
    // Parse package version strings
    for( PACKAGE_VERSION& ver : aPackage.versions )
    {
        int epoch = 0, major = 0, minor = 0, patch = 0;

        if( ver.version_epoch )
            epoch = *ver.version_epoch;

        wxStringTokenizer version_tokenizer( ver.version, "." );

        major = wxAtoi( version_tokenizer.GetNextToken() );

        if( version_tokenizer.HasMoreTokens() )
            minor = wxAtoi( version_tokenizer.GetNextToken() );

        if( version_tokenizer.HasMoreTokens() )
            patch = wxAtoi( version_tokenizer.GetNextToken() );

        ver.parsed_version = std::make_tuple( epoch, major, minor, patch );

        // Determine compatibility
        ver.compatible = true;

        auto parse_version_tuple =
                []( const wxString& version, int deflt )
                {
                    int ver_major = deflt;
                    int ver_minor = deflt;
                    int ver_patch = deflt;

                    wxStringTokenizer tokenizer( version, "." );

                    ver_major = wxAtoi( tokenizer.GetNextToken() );

                    if( tokenizer.HasMoreTokens() )
                        ver_minor = wxAtoi( tokenizer.GetNextToken() );

                    if( tokenizer.HasMoreTokens() )
                        ver_patch = wxAtoi( tokenizer.GetNextToken() );

                    return std::tuple<int, int, int>( ver_major, ver_minor, ver_patch );
                };

        if( parse_version_tuple( ver.kicad_version, 0 ) > m_kicad_version )
            ver.compatible = false;

        if( ver.kicad_version_max
            && parse_version_tuple( *ver.kicad_version_max, 999 ) < m_kicad_version )
            ver.compatible = false;

#if defined( _WIN32 )
        wxString platform = wxT( "windows" );
#elif defined( __APPLE__ )
        wxString platform = wxT( "macos" );
#else
        wxString platform = wxT( "linux" );
#endif

        if( ver.platforms.size() > 0
            && std::find( ver.platforms.begin(), ver.platforms.end(), platform )
                       == ver.platforms.end() )
        {
            ver.compatible = false;
        }
    }

    // Sort by descending version
    std::sort( aPackage.versions.begin(), aPackage.versions.end(),
               []( const PACKAGE_VERSION& a, const PACKAGE_VERSION& b )
               {
                   return a.parsed_version > b.parsed_version;
               } );
}


const std::vector<PCM_PACKAGE>&
PLUGIN_CONTENT_MANAGER::GetRepositoryPackages( const wxString& aRepositoryId ) const
{
    static std::vector<PCM_PACKAGE> empty{};

    try
    {
        return getCachedRepository( aRepositoryId ).package_list;
    }
    catch( ... )
    {
        return empty;
    }
}


void PLUGIN_CONTENT_MANAGER::SetRepositoryList( const STRING_PAIR_LIST& aRepositories )
{
    // Clean up cache folder if repository is not in new list
    for( const std::tuple<wxString, wxString, wxString>& entry : m_repository_list )
    {
        auto it = std::find_if( aRepositories.begin(), aRepositories.end(),
                                [&]( const auto& new_entry )
                                {
                                    return new_entry.first == std::get<1>( entry );
                                } );

        if( it == aRepositories.end() )
        {
            DiscardRepositoryCache( std::get<0>( entry ) );
        }
    }

    m_repository_list.clear();
    m_repository_cache.clear();

    for( const std::pair<wxString, wxString>& repo : aRepositories )
    {
        std::string url_sha = picosha2::hash256_hex_string( repo.second );
        m_repository_list.push_back( std::make_tuple( url_sha.substr( 0, 16 ), repo.first,
                                                      repo.second ) );
    }
}


void PLUGIN_CONTENT_MANAGER::DiscardRepositoryCache( const wxString& aRepositoryId )
{
    if( m_repository_cache.count( aRepositoryId ) > 0 )
        m_repository_cache.erase( aRepositoryId );

    wxFileName repo_cache = wxFileName( PATHS::GetUserCachePath(), "" );
    repo_cache.AppendDir( wxT( "pcm" ) );
    repo_cache.AppendDir( aRepositoryId );

    if( repo_cache.DirExists() )
        repo_cache.Rmdir( wxPATH_RMDIR_RECURSIVE );
}


void PLUGIN_CONTENT_MANAGER::MarkInstalled( const PCM_PACKAGE& aPackage, const wxString& aVersion,
                                            const wxString& aRepositoryId )
{
    // In case of package update remove old data but keep pinned state
    bool pinned = false;

    if( m_installed.count( aPackage.identifier ) )
    {
        pinned = m_installed.at( aPackage.identifier ).pinned;
        MarkUninstalled( aPackage );
    }

    PCM_INSTALLATION_ENTRY entry;
    entry.package = aPackage;
    entry.current_version = aVersion;
    entry.repository_id = aRepositoryId;

    try
    {
        if( !aRepositoryId.IsEmpty() )
            entry.repository_name = getCachedRepository( aRepositoryId ).name;
        else
            entry.repository_name = _( "Local file" );
    }
    catch( ... )
    {
        entry.repository_name = _( "Unknown" );
    }

    entry.install_timestamp = getCurrentTimestamp();
    entry.pinned = pinned;

    m_installed.emplace( aPackage.identifier, entry );

    if( m_dialog
        && ( aPackage.versions[0].runtime.value_or( PCM_PACKAGE_RUNTIME::PPR_SWIG )
             == PCM_PACKAGE_RUNTIME::PPR_IPC )
        && !Pgm().GetCommonSettings()->m_Api.enable_server )
    {
        if( wxMessageBox( _( "This plugin requires the KiCad API, which is currently "
                             "disabled in preferences. Would you like to enable it?" ),
                         _( "Enable KiCad API" ), wxICON_QUESTION | wxYES_NO, m_dialog )
                   == wxYES )
        {
            Pgm().GetCommonSettings()->m_Api.enable_server = true;
            m_dialog->ParentFrame()->Kiway().CommonSettingsChanged();
        }
    }
}


void PLUGIN_CONTENT_MANAGER::MarkUninstalled( const PCM_PACKAGE& aPackage )
{
    m_installed.erase( aPackage.identifier );
}


PCM_PACKAGE_STATE PLUGIN_CONTENT_MANAGER::GetPackageState( const wxString& aRepositoryId,
                                                           const wxString& aPackageId )
{
    bool installed = m_installed.find( aPackageId ) != m_installed.end();

    if( aRepositoryId.IsEmpty() || !CacheRepository( aRepositoryId ) )
        return installed ? PPS_INSTALLED : PPS_UNAVAILABLE;

    const PCM_REPOSITORY* repo;

    try
    {
        repo = &getCachedRepository( aRepositoryId );
    }
    catch( ... )
    {
        return installed ? PPS_INSTALLED : PPS_UNAVAILABLE;
    }

    if( repo->package_map.count( aPackageId ) == 0 )
        return installed ? PPS_INSTALLED : PPS_UNAVAILABLE;

    const PCM_PACKAGE& pkg = repo->package_list[repo->package_map.at( aPackageId )];

    if( installed )
    {
        // Package is installed, check for available updates at the same or
        // higher (numerically lower) version stability level
        wxString update_version = GetPackageUpdateVersion( pkg );

        return update_version.IsEmpty() ? PPS_INSTALLED : PPS_UPDATE_AVAILABLE;
    }
    else
    {
        // Find any compatible version
        auto ver_it = std::find_if( pkg.versions.begin(), pkg.versions.end(),
                                    []( const PACKAGE_VERSION& ver )
                                    {
                                        return ver.compatible;
                                    } );

        return ver_it == pkg.versions.end() ? PPS_UNAVAILABLE : PPS_AVAILABLE;
    }
}


const wxString PLUGIN_CONTENT_MANAGER::GetPackageUpdateVersion( const PCM_PACKAGE& aPackage )
{
    wxASSERT_MSG( m_installed.find( aPackage.identifier ) != m_installed.end(),
                  wxT( "GetPackageUpdateVersion called on a not installed package" ) );

    const PCM_INSTALLATION_ENTRY& entry = m_installed.at( aPackage.identifier );

    auto installed_ver_it = std::find_if(
            entry.package.versions.begin(), entry.package.versions.end(),
            [&]( const PACKAGE_VERSION& ver )
            {
                return ver.version == entry.current_version;
            } );

    wxASSERT_MSG( installed_ver_it != entry.package.versions.end(),
                  wxT( "Installed package version not found" ) );

    auto ver_it = std::find_if( aPackage.versions.begin(), aPackage.versions.end(),
                                [&]( const PACKAGE_VERSION& ver )
                                {
                                    return ver.compatible
                                           && installed_ver_it->status >= ver.status
                                           && installed_ver_it->parsed_version < ver.parsed_version;
                                } );

    return ver_it == aPackage.versions.end() ? wxString( wxT( "" ) ) : ver_it->version;
}

time_t PLUGIN_CONTENT_MANAGER::getCurrentTimestamp() const
{
    return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch() ).count();
}


void PLUGIN_CONTENT_MANAGER::SaveInstalledPackages()
{
    try
    {
        nlohmann::json js;
        js["packages"] = nlohmann::json::array();

        for( std::pair<const wxString, PCM_INSTALLATION_ENTRY>& pair : m_installed )
        {
            js["packages"].emplace_back( pair.second );
        }

        wxFileName    f( PATHS::GetUserSettingsPath(), wxT( "installed_packages.json" ) );
        std::ofstream stream( f.GetFullPath().fn_str() );

        stream << std::setw( 4 ) << js << std::endl;
    }
    catch( nlohmann::detail::exception& )
    {
        // Ignore
    }
}


const std::vector<PCM_INSTALLATION_ENTRY> PLUGIN_CONTENT_MANAGER::GetInstalledPackages() const
{
    std::vector<PCM_INSTALLATION_ENTRY> v;

    std::for_each( m_installed.begin(), m_installed.end(),
                   [&v]( const std::pair<const wxString, PCM_INSTALLATION_ENTRY>& entry )
                   {
                       v.push_back( entry.second );
                   } );

    std::sort( v.begin(), v.end(),
               []( const PCM_INSTALLATION_ENTRY& a, const PCM_INSTALLATION_ENTRY& b )
               {
                   return ( a.install_timestamp < b.install_timestamp )
                          || ( a.install_timestamp == b.install_timestamp
                               && a.package.identifier < b.package.identifier );
               } );

    return v;
}


const wxString&
PLUGIN_CONTENT_MANAGER::GetInstalledPackageVersion( const wxString& aPackageId ) const
{
    wxASSERT_MSG( m_installed.find( aPackageId ) != m_installed.end(),
                  wxT( "Installed package not found." ) );

    return m_installed.at( aPackageId ).current_version;
}


bool PLUGIN_CONTENT_MANAGER::IsPackagePinned( const wxString& aPackageId ) const
{
    if( m_installed.find( aPackageId ) == m_installed.end() )
        return false;

    return m_installed.at( aPackageId ).pinned;
}


void PLUGIN_CONTENT_MANAGER::SetPinned( const wxString& aPackageId, const bool aPinned )
{
    if( m_installed.find( aPackageId ) == m_installed.end() )
        return;

    m_installed.at( aPackageId ).pinned = aPinned;
}


int PLUGIN_CONTENT_MANAGER::GetPackageSearchRank( const PCM_PACKAGE& aPackage,
                                                  const wxString&    aSearchTerm )
{
    wxArrayString terms = wxStringTokenize( aSearchTerm.Lower(), wxS( " " ), wxTOKEN_STRTOK );
    int           rank = 0;

    const auto find_term_matches =
            [&]( const wxString& str )
            {
                int      result = 0;
                wxString lower = str.Lower();

                for( const wxString& term : terms )
                {
                    if( lower.Find( term ) != wxNOT_FOUND )
                        result += 1;
                }

                return result;
            };

    // Match on package id
    if( terms.size() == 1 && terms[0] == aPackage.identifier )
        rank += 10000;

    if( terms.size() == 1 && find_term_matches( aPackage.identifier ) )
        rank += 1000;

    // Match on package name
    rank += 500 * find_term_matches( aPackage.name );

    // Match on tags
    for( const std::string& tag : aPackage.tags )
        rank += 100 * find_term_matches( wxString( tag ) );

    // Match on package description
    rank += 10 * find_term_matches( aPackage.description );
    rank += 10 * find_term_matches( aPackage.description_full );

    // Match on author/maintainer
    rank += find_term_matches( aPackage.author.name );

    if( aPackage.maintainer )
        rank += 3 * find_term_matches( aPackage.maintainer->name );

    // Match on resources
    for( const std::pair<const std::string, wxString>& entry : aPackage.resources )
    {
        rank += find_term_matches( entry.first );
        rank += find_term_matches( entry.second );
    }

    // Match on license
    if( terms.size() == 1 && terms[0] == aPackage.license )
        rank += 1;

    return rank;
}


std::unordered_map<wxString, wxBitmap>
PLUGIN_CONTENT_MANAGER::GetRepositoryPackageBitmaps( const wxString& aRepositoryId )
{
    std::unordered_map<wxString, wxBitmap> bitmaps;

    wxFileName resources_file = wxFileName( PATHS::GetUserCachePath(), wxT( "resources.zip" ) );
    resources_file.AppendDir( wxT( "pcm" ) );
    resources_file.AppendDir( aRepositoryId );

    if( !resources_file.FileExists() )
        return bitmaps;

    wxFFileInputStream stream( resources_file.GetFullPath() );
    wxZipInputStream   zip( stream );

    if( !zip.IsOk() || zip.GetTotalEntries() == 0 )
        return bitmaps;

    for( wxArchiveEntry* entry = zip.GetNextEntry(); entry; entry = zip.GetNextEntry() )
    {
        wxArrayString path_parts = wxSplit( entry->GetName(), wxFileName::GetPathSeparator(),
                                            (wxChar) 0 );

        if( path_parts.size() != 2 || path_parts[1] != wxT( "icon.png" ) )
            continue;

        try
        {
            wxMemoryInputStream image_stream( zip, entry->GetSize() );
            wxImage             image( image_stream, wxBITMAP_TYPE_PNG );
            bitmaps.emplace( path_parts[0], wxBitmap( image ) );
        }
        catch( ... )
        {
            // Log and ignore
            wxLogTrace( wxT( "Error loading png bitmap for entry %s from %s" ), entry->GetName(),
                        resources_file.GetFullPath() );
        }
    }

    return bitmaps;
}


std::unordered_map<wxString, wxBitmap> PLUGIN_CONTENT_MANAGER::GetInstalledPackageBitmaps()
{
    std::unordered_map<wxString, wxBitmap> bitmaps;

    wxFileName resources_dir_fn( m_3rdparty_path, wxEmptyString );
    resources_dir_fn.AppendDir( wxT( "resources" ) );
    wxDir resources_dir( resources_dir_fn.GetPath() );

    if( !resources_dir.IsOpened() )
        return bitmaps;

    wxString subdir;
    bool     more = resources_dir.GetFirst( &subdir, wxEmptyString, wxDIR_DIRS | wxDIR_HIDDEN );

    while( more )
    {
        wxFileName icon( resources_dir_fn.GetPath(), wxT( "icon.png" ) );
        icon.AppendDir( subdir );

        if( icon.FileExists() )
        {
            wxString actual_package_id = subdir;
            actual_package_id.Replace( '_', '.' );

            try
            {
                wxBitmap bitmap( icon.GetFullPath(), wxBITMAP_TYPE_PNG );
                bitmaps.emplace( actual_package_id, bitmap );
            }
            catch( ... )
            {
                // Log and ignore
                wxLogTrace( wxT( "Error loading png bitmap from %s" ), icon.GetFullPath() );
            }
        }

        more = resources_dir.GetNext( &subdir );
    }

    return bitmaps;
}


struct UPDATE_CANCELLER
{
    UPDATE_CANCELLER( std::shared_ptr<BACKGROUND_JOB>& aJob ) : m_jobToCancel( aJob ) {};
    ~UPDATE_CANCELLER()
    {
        if( m_jobToCancel )
        {
            Pgm().GetBackgroundJobMonitor().Remove( m_jobToCancel );
            m_jobToCancel.reset();
        }
    }

    std::shared_ptr<BACKGROUND_JOB>& m_jobToCancel;
};


void PLUGIN_CONTENT_MANAGER::RunBackgroundUpdate()
{
    // If the thread is already running don't create it again
    if( m_updateThread.joinable() )
        return;

    m_updateBackgroundJob = Pgm().GetBackgroundJobMonitor().Create( _( "PCM Update" ) );

    m_updateThread = std::thread(
            [this]()
            {
                UPDATE_CANCELLER canceller( m_updateBackgroundJob );

                if( m_installed.size() == 0 )
                    return;

                int maxProgress = m_repository_list.size() + m_installed.size();
                m_updateBackgroundJob->m_reporter->SetNumPhases( maxProgress );
                m_updateBackgroundJob->m_reporter->Report( _( "Preparing to fetch repositories" ) );

                // Only fetch repositories that have installed not pinned packages
                std::unordered_set<wxString> repo_ids;

                for( std::pair<const wxString, PCM_INSTALLATION_ENTRY>& pair : m_installed )
                {
                    if( !pair.second.pinned )
                        repo_ids.insert( pair.second.repository_id );
                }

                for( const auto& [ repository_id, name, url ] : m_repository_list )
                {
                    m_updateBackgroundJob->m_reporter->AdvancePhase();
                    if( repo_ids.count( repository_id ) == 0 )
                        continue;

                    m_updateBackgroundJob->m_reporter->Report(
                            _( "Fetching repository..." ) );
                    CacheRepository( repository_id );

                    if( m_updateBackgroundJob->m_reporter->IsCancelled() )
                        break;
                }

                if( m_updateBackgroundJob->m_reporter->IsCancelled() )
                    return;

                // Count packages with updates
                int availableUpdateCount = 0;

                m_updateBackgroundJob->m_reporter->Report( _( "Reviewing packages..." ) );
                for( std::pair<const wxString, PCM_INSTALLATION_ENTRY>& pair : m_installed )
                {
                    PCM_INSTALLATION_ENTRY& entry = pair.second;

                    m_updateBackgroundJob->m_reporter->AdvancePhase();

                    if( m_repository_cache.find( entry.repository_id ) != m_repository_cache.end() )
                    {
                        PCM_PACKAGE_STATE state = GetPackageState( entry.repository_id,
                                                                   entry.package.identifier );

                        if( state == PPS_UPDATE_AVAILABLE && !entry.pinned )
                            availableUpdateCount++;
                    }

                    if( m_updateBackgroundJob->m_reporter->IsCancelled() )
                        return;
                }

                // Update the badge on PCM button
                m_availableUpdateCallback( availableUpdateCount );
            } );
}


void PLUGIN_CONTENT_MANAGER::StopBackgroundUpdate()
{
    if( m_updateThread.joinable() )
    {
        if( m_updateBackgroundJob )
            m_updateBackgroundJob->m_reporter->Cancel();

        m_updateThread.join();
    }
}


PLUGIN_CONTENT_MANAGER::~PLUGIN_CONTENT_MANAGER()
{
    // By the time object is being destroyed the thread should be
    // stopped already but just in case do it here too.
    StopBackgroundUpdate();
}
