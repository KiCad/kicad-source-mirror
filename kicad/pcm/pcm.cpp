/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kicad_curl/kicad_curl.h>
#include "kicad_curl/kicad_curl_easy.h"

#include "pcm.h"
#include "core/wx_stl_compat.h"
#include "kicad_build_version.h"
#include "paths.h"
#include "pgm_base.h"
#include "picosha2.h"
#include "settings/settings_manager.h"
#include "widgets/wx_progress_reporters.h"

#include <fstream>
#include <iomanip>
#include <memory>
#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/fs_zip.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/sstream.h>
#include <wx/tokenzr.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>


const std::tuple<int, int> PLUGIN_CONTENT_MANAGER::m_kicad_version =
        KICAD_MAJOR_MINOR_VERSION_TUPLE;


PLUGIN_CONTENT_MANAGER::PLUGIN_CONTENT_MANAGER( wxWindow* aParent ) : m_dialog( aParent )
{
    // Get 3rd party path
    const ENV_VAR_MAP& env = Pgm().GetLocalEnvVariables();
    auto               it = env.find( "KICAD6_3RD_PARTY" );

    if( it != env.end() && !it->second.GetValue().IsEmpty() )
        m_3rdparty_path = it->second.GetValue();
    else
        m_3rdparty_path = PATHS::GetDefault3rdPartyPath();

    // Read and store pcm schema
    wxFileName schema_file( PATHS::GetStockDataPath( true ), "pcm.v1.schema.json" );
    schema_file.Normalize();
    schema_file.AppendDir( "schemas" );

    std::ifstream  schema_stream( schema_file.GetFullPath().ToUTF8() );
    nlohmann::json schema;

    try
    {
        schema_stream >> schema;
        m_schema_validator.set_root_schema( schema );
    }
    catch( std::exception& e )
    {
        if( !schema_file.FileExists() )
            wxLogError( wxString::Format( _( "schema file '%s' not found" ),
                                          schema_file.GetFullPath() ) );
        else
            wxLogError( wxString::Format( _( "Error loading schema: %s" ), e.what() ) );
    }

    // Load currently installed packages
    wxFileName f( SETTINGS_MANAGER::GetUserSettingsPath(), "installed_packages.json" );

    if( f.FileExists() )
    {
        std::ifstream  installed_stream( f.GetFullPath().ToUTF8() );
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
        wxFileName d( m_3rdparty_path, "" );
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

                    m_installed.emplace( actual_package_id, entry );
                }

                more = package_dir.GetNext( &subdir );
            }
        }
    }

    // Calculate package compatibility
    std::for_each( m_installed.begin(), m_installed.end(),
                   [&]( auto& entry )
                   {
                       preparePackage( entry.second.package );
                   } );
}


bool PLUGIN_CONTENT_MANAGER::DownloadToStream( const wxString& aUrl, std::ostream* aOutput,
                                               const wxString& aDialogTitle,
                                               const size_t    aSizeLimit )
{
    bool size_exceeded = false;

    std::unique_ptr<WX_PROGRESS_REPORTER> reporter(
            new WX_PROGRESS_REPORTER( m_dialog, aDialogTitle, 1 ) );

    TRANSFER_CALLBACK callback = [&]( size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow )
    {
        if( aSizeLimit > 0 && ( dltotal > aSizeLimit || dlnow > aSizeLimit ) )
        {
            size_exceeded = true;

            // Non zero return means abort.
            return true;
        }

        if( dltotal > 1024 )
        {
            reporter->SetCurrentProgress( dlnow / (double) dltotal );
            reporter->Report( wxString::Format( _( "Downloading %lld/%lld Kb" ), dlnow / 1024,
                                                dltotal / 1024 ) );
        }
        else
        {
            reporter->SetCurrentProgress( 0.0 );
        }

        return !reporter->KeepRefreshing();
    };

    KICAD_CURL_EASY curl;
    curl.SetOutputStream( aOutput );
    curl.SetURL( aUrl.ToUTF8().data() );
    curl.SetFollowRedirects( true );
    curl.SetTransferCallback( callback, 250000L );

    int code = curl.Perform();

    if( !reporter->IsCancelled() )
        reporter->SetCurrentProgress( 1.0 );

    if( code != CURLE_OK )
    {
        if( code == CURLE_ABORTED_BY_CALLBACK && size_exceeded )
            wxMessageBox( _( "Download is too large." ) );
        else if( code != CURLE_ABORTED_BY_CALLBACK )
            wxLogError( wxString( curl.GetErrorText( code ) ) );

        return false;
    }

    return true;
}


bool PLUGIN_CONTENT_MANAGER::FetchRepository( const wxString& aUrl, PCM_REPOSITORY& aRepository )
{
    std::stringstream repository_stream;
    if( !DownloadToStream( aUrl, &repository_stream, _( "Fetching repository" ), 20480 ) )
    {
        wxLogError( _( "Unable to load repository url" ) );
        return false;
    }

    nlohmann::json repository_json;

    try
    {
        repository_stream >> repository_json;

        ValidateJson( repository_json, nlohmann::json_uri( "#/definitions/Repository" ) );

        aRepository = repository_json.get<PCM_REPOSITORY>();
    }
    catch( const std::exception& e )
    {
        wxLogError( wxString::Format( _( "Unable to parse repository:\n\n%s" ), e.what() ) );
        return false;
    }

    return true;
}


void PLUGIN_CONTENT_MANAGER::ValidateJson( const nlohmann::json&     aJson,
                                           const nlohmann::json_uri& aUri ) const
{
    nlohmann::json_schema::basic_error_handler error_handler;
    m_schema_validator.validate( aJson, error_handler, aUri );
}


bool PLUGIN_CONTENT_MANAGER::fetchPackages( const wxString&                  aUrl,
                                            const boost::optional<wxString>& aHash,
                                            std::vector<PCM_PACKAGE>&        aPackages )
{
    std::stringstream packages_stream;

    if( !DownloadToStream( aUrl, &packages_stream, _( "Fetching repository packages" ) ) )
    {
        wxLogError( _( "Unable to load repository packages url." ) );
        return false;
    }

    std::istringstream isstream( packages_stream.str() );

    if( aHash && !VerifyHash( isstream, aHash.get() ) )
    {
        wxLogError( _( "Packages hash doesn't match. Repository may be corrupted." ) );
        return false;
    }

    try
    {
        nlohmann::json packages_json = nlohmann::json::parse( packages_stream.str() );
        ValidateJson( packages_json, nlohmann::json_uri( "#/definitions/PackageArray" ) );

        for( nlohmann::json& package : packages_json["packages"] )
        {
            aPackages.push_back( package.get<PCM_PACKAGE>() );
        }
    }
    catch( std::exception& e )
    {
        wxLogError( wxString::Format( _( "Unable to parse packages metadata:\n\n%s" ), e.what() ) );
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
                  "Repository is not cached." );

    return m_repository_cache.at( aRepositoryId );
}


const bool PLUGIN_CONTENT_MANAGER::CacheRepository( const wxString& aRepositoryId )
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

    nlohmann::json js;
    PCM_REPOSITORY current_repo;

    if( !FetchRepository( url, current_repo ) )
        return false;

    bool packages_cache_exists = false;

    // First load repository data from local filesystem if available.
    wxFileName repo_cache = wxFileName( m_3rdparty_path, "repository.json" );
    repo_cache.AppendDir( "cache" );
    repo_cache.AppendDir( aRepositoryId );
    wxFileName packages_cache( repo_cache.GetPath(), "packages.json" );

    if( repo_cache.FileExists() && packages_cache.FileExists() )
    {
        std::ifstream repo_stream( repo_cache.GetFullPath().ToUTF8() );
        repo_stream >> js;
        PCM_REPOSITORY saved_repo = js.get<PCM_REPOSITORY>();

        if( saved_repo.packages.update_timestamp == current_repo.packages.update_timestamp )
        {
            // Cached repo is up to date, use data on disk
            js.clear();
            std::ifstream packages_cache_stream( packages_cache.GetFullPath().ToUTF8() );

            try
            {
                packages_cache_stream >> js;
                saved_repo.package_list = js["packages"].get<std::vector<PCM_PACKAGE>>();

                std::for_each( saved_repo.package_list.begin(), saved_repo.package_list.end(),
                               &preparePackage );

                m_repository_cache[aRepositoryId] = std::move( saved_repo );

                packages_cache_exists = true;
            }
            catch( ... )
            {
                wxLogError( _( "Packages cache for current repository is "
                               "corrupted, it will be redownloaded." ) );
            }
        }
    }

    if( !packages_cache_exists )
    {
        // Cache doesn't exist or is out of date
        if( !fetchPackages( current_repo.packages.url, current_repo.packages.sha256,
                            current_repo.package_list ) )
        {
            return false;
        }

        std::for_each( current_repo.package_list.begin(), current_repo.package_list.end(),
                       &preparePackage );

        repo_cache.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        std::ofstream repo_cache_stream( repo_cache.GetFullPath().ToUTF8() );
        repo_cache_stream << std::setw( 4 ) << nlohmann::json( current_repo ) << std::endl;

        std::ofstream packages_cache_stream( packages_cache.GetFullPath().ToUTF8() );
        js.clear();
        js["packages"] = nlohmann::json( current_repo.package_list );
        packages_cache_stream << std::setw( 4 ) << js << std::endl;

        m_repository_cache[aRepositoryId] = std::move( current_repo );
    }

    if( current_repo.resources )
    {
        // Check resources file date, redownload if needed
        PCM_RESOURCE_REFERENCE& resources = current_repo.resources.get();

        wxFileName resource_file( repo_cache.GetPath(), "resources.zip" );

        time_t mtime = 0;

        if( resource_file.FileExists() )
            mtime = wxFileModificationTime( resource_file.GetFullPath() );

        if( mtime + 600 < getCurrentTimestamp() && mtime < (time_t) resources.update_timestamp )
        {
            std::ofstream resources_stream( resource_file.GetFullPath().ToUTF8(),
                                            std::ios_base::binary );

            // 100 Mb resource file limit
            bool success = DownloadToStream( resources.url, &resources_stream,
                                             _( "Downloading resources" ), 100 * 1024 * 1024 );

            resources_stream.close();

            if( success )
            {
                std::ifstream read_stream( resource_file.GetFullPath().ToUTF8(),
                                           std::ios_base::binary );


                if( resources.sha256 && !VerifyHash( read_stream, resources.sha256.get() ) )
                {
                    read_stream.close();
                    wxLogError(
                            _( "Resources file hash doesn't match and will not be used. Repository "
                               "may be corrupted." ) );
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

    return true;
}


void PLUGIN_CONTENT_MANAGER::preparePackage( PCM_PACKAGE& aPackage )
{
    // Parse package version strings
    for( PACKAGE_VERSION& ver : aPackage.versions )
    {
        int epoch = 0, major = 0, minor = 0, patch = 0;

        if( ver.version_epoch )
            epoch = ver.version_epoch.get();

        wxStringTokenizer version_tokenizer( ver.version, "." );

        major = wxAtoi( version_tokenizer.GetNextToken() );

        if( version_tokenizer.HasMoreTokens() )
            minor = wxAtoi( version_tokenizer.GetNextToken() );

        if( version_tokenizer.HasMoreTokens() )
            patch = wxAtoi( version_tokenizer.GetNextToken() );

        ver.parsed_version = std::make_tuple( epoch, major, minor, patch );

        // Determine compatibility
        ver.compatible = true;

        auto parse_major_minor = []( const wxString& version )
        {
            wxStringTokenizer tokenizer( version, "." );
            int               ver_major = wxAtoi( tokenizer.GetNextToken() );
            int               ver_minor = wxAtoi( tokenizer.GetNextToken() );
            return std::tuple<int, int>( ver_major, ver_minor );
        };

        if( parse_major_minor( ver.kicad_version ) > m_kicad_version )
            ver.compatible = false;

        if( ver.kicad_version_max
            && parse_major_minor( ver.kicad_version_max.get() ) < m_kicad_version )
            ver.compatible = false;

#ifdef __WXMSW__
        wxString platform = wxT( "windows" );
#endif
#ifdef __WXOSX__
        wxString platform = wxT( "macos" );
#endif
#ifdef __WXGTK__
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
    return getCachedRepository( aRepositoryId ).package_list;
}


void PLUGIN_CONTENT_MANAGER::SetRepositoryList( const STRING_PAIR_LIST& aRepositories )
{
    // Clean up cache folder if repository is not in new list
    for( const auto& entry : m_repository_list )
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

    for( const auto& repo : aRepositories )
    {
        std::string url_sha = picosha2::hash256_hex_string( repo.second );
        m_repository_list.push_back(
                std::make_tuple( url_sha.substr( 0, 16 ), repo.first, repo.second ) );
    }
}


void PLUGIN_CONTENT_MANAGER::DiscardRepositoryCache( const wxString& aRepositoryId )
{
    if( m_repository_cache.count( aRepositoryId ) > 0 )
        m_repository_cache.erase( aRepositoryId );

    wxFileName repo_cache( m_3rdparty_path, "" );
    repo_cache.AppendDir( "cache" );
    repo_cache.AppendDir( aRepositoryId );

    if( repo_cache.DirExists() )
        repo_cache.Rmdir( wxPATH_RMDIR_RECURSIVE );
}


void PLUGIN_CONTENT_MANAGER::MarkInstalled( const PCM_PACKAGE& aPackage, const wxString& aVersion,
                                            const wxString& aRepositoryId )
{
    PCM_INSTALLATION_ENTRY entry;
    entry.package = aPackage;
    entry.current_version = aVersion;
    entry.repository_id = aRepositoryId;

    if( !aRepositoryId.IsEmpty() )
        entry.repository_name = getCachedRepository( aRepositoryId ).name;
    else
        entry.repository_name = _( "Local file" );

    entry.install_timestamp = getCurrentTimestamp();

    m_installed.emplace( aPackage.identifier, entry );
}


void PLUGIN_CONTENT_MANAGER::MarkUninstalled( const PCM_PACKAGE& aPackage )
{
    m_installed.erase( aPackage.identifier );
}


PCM_PACKAGE_STATE PLUGIN_CONTENT_MANAGER::GetPackageState( const wxString& aRepositoryId,
                                                           const wxString& aPackageId )
{
    if( m_installed.find( aPackageId ) != m_installed.end() )
        return PPS_INSTALLED;

    if( aRepositoryId.IsEmpty() || !CacheRepository( aRepositoryId ) )
        return PPS_UNAVAILABLE;

    const PCM_REPOSITORY& repo = getCachedRepository( aRepositoryId );

    auto pkg_it = std::find_if( repo.package_list.begin(), repo.package_list.end(),
                                [&aPackageId]( const PCM_PACKAGE& pkg )
                                {
                                    return pkg.identifier == aPackageId;
                                } );

    if( pkg_it == repo.package_list.end() )
        return PPS_UNAVAILABLE;

    const PCM_PACKAGE& pkg = *pkg_it;

    auto ver_it = std::find_if( pkg.versions.begin(), pkg.versions.end(),
                                []( const PACKAGE_VERSION& ver )
                                {
                                    return ver.compatible;
                                } );

    if( ver_it == pkg.versions.end() )
        return PPS_UNAVAILABLE;
    else
        return PPS_AVAILABLE;
}


time_t PLUGIN_CONTENT_MANAGER::getCurrentTimestamp() const
{
    return std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::system_clock::now().time_since_epoch() )
            .count();
}


PLUGIN_CONTENT_MANAGER::~PLUGIN_CONTENT_MANAGER()
{
    // Save current installed packages list.

    try
    {
        nlohmann::json js;
        js["packages"] = nlohmann::json::array();

        for( const auto& entry : m_installed )
        {
            js["packages"].emplace_back( entry.second );
        }

        wxFileName    f( SETTINGS_MANAGER::GetUserSettingsPath(), "installed_packages.json" );
        std::ofstream stream( f.GetFullPath().ToUTF8() );

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
                   [&v]( const auto& entry )
                   {
                       v.push_back( entry.second );
                   } );

    return v;
}


const wxString&
PLUGIN_CONTENT_MANAGER::GetInstalledPackageVersion( const wxString& aPackageId ) const
{
    wxASSERT_MSG( m_installed.find( aPackageId ) != m_installed.end(),
                  "Installed package not found." );

    return m_installed.at( aPackageId ).current_version;
}


int PLUGIN_CONTENT_MANAGER::GetPackageSearchRank( const PCM_PACKAGE& aPackage,
                                                  const wxString&    aSearchTerm )
{
    wxArrayString terms = wxStringTokenize( aSearchTerm.Lower(), " ", wxTOKEN_STRTOK );
    int           rank = 0;

    const auto find_term_matches = [&]( const wxString& str )
    {
        int      result = 0;
        wxString lower = str.Lower();

        for( const wxString& term : terms )
            if( lower.Find( term ) != wxNOT_FOUND )
                result += 1;

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
        rank += 3 * find_term_matches( aPackage.maintainer.get().name );

    // Match on resources
    for( const auto& entry : aPackage.resources )
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

    wxFileName resources_file = wxFileName( m_3rdparty_path, "resources.zip" );
    resources_file.AppendDir( "cache" );
    resources_file.AppendDir( aRepositoryId );

    if( !resources_file.FileExists() )
        return bitmaps;

    wxFFileInputStream stream( resources_file.GetFullPath() );
    wxZipInputStream   zip( stream );

    if( !zip.IsOk() || zip.GetTotalEntries() == 0 )
        return bitmaps;

    for( wxArchiveEntry* entry = zip.GetNextEntry(); entry; entry = zip.GetNextEntry() )
    {
        wxArrayString path_parts =
                wxSplit( entry->GetName(), wxFileName::GetPathSeparator(), (wxChar) NULL );

        if( path_parts.size() != 2 || path_parts[1] != "icon.png" )
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
            wxLogTrace( "Error loading png bitmap for entry %s from %s", entry->GetName(),
                        resources_file.GetFullPath() );
        }
    }

    return bitmaps;
}


std::unordered_map<wxString, wxBitmap> PLUGIN_CONTENT_MANAGER::GetInstalledPackageBitmaps()
{
    std::unordered_map<wxString, wxBitmap> bitmaps;

    wxFileName resources_dir_fn( m_3rdparty_path, "" );
    resources_dir_fn.AppendDir( "resources" );
    wxDir resources_dir( resources_dir_fn.GetPath() );

    if( !resources_dir.IsOpened() )
        return bitmaps;

    wxString subdir;
    bool     more = resources_dir.GetFirst( &subdir, "", wxDIR_DIRS | wxDIR_HIDDEN );

    while( more )
    {
        wxFileName icon( resources_dir_fn.GetPath(), "icon.png" );
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
                wxLogTrace( "Error loading png bitmap from %s", icon.GetFullPath() );
            }
        }

        more = resources_dir.GetNext( &subdir );
    }

    return bitmaps;
}
