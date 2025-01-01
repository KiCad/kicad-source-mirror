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
// kicad_curl_easy.h **must be** included before any wxWidgets header to avoid conflicts
// at least on Windows/msys2
#include "kicad_curl/kicad_curl.h"
#include "kicad_curl/kicad_curl_easy.h"

#include <paths.h>
#include "pcm_task_manager.h"
#include <reporter.h>
#include <wxstream_helper.h>

#include <fstream>
#include <thread>
#include <unordered_set>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>


void compile_keep_on_update_regex( const PCM_PACKAGE& pkg, const PACKAGE_VERSION& ver,
                                   std::forward_list<wxRegEx>& aKeepOnUpdate )
{
    auto compile_regex = [&]( const wxString& regex )
    {
        aKeepOnUpdate.emplace_front( regex, wxRE_DEFAULT );

        if( !aKeepOnUpdate.front().IsValid() )
            aKeepOnUpdate.pop_front();
    };

    std::for_each( pkg.keep_on_update.begin(), pkg.keep_on_update.end(), compile_regex );
    std::for_each( ver.keep_on_update.begin(), ver.keep_on_update.end(), compile_regex );
}


PCM_TASK_MANAGER::STATUS PCM_TASK_MANAGER::DownloadAndInstall( const PCM_PACKAGE& aPackage, const wxString& aVersion,
                                           const wxString& aRepositoryId, const bool isUpdate )
{
    PCM_TASK download_task = [aPackage, aVersion, aRepositoryId, isUpdate, this]() -> PCM_TASK_MANAGER::STATUS
    {
        wxFileName file_path( PATHS::GetUserCachePath(), "" );
        file_path.AppendDir( "pcm" );
        file_path.SetFullName( wxString::Format( "%s_v%s.zip", aPackage.identifier, aVersion ) );

        auto find_pkgver = std::find_if( aPackage.versions.begin(), aPackage.versions.end(),
                                         [&aVersion]( const PACKAGE_VERSION& pv )
                                         {
                                             return pv.version == aVersion;
                                         } );

        if( find_pkgver == aPackage.versions.end() )
        {
            m_reporter->PCMReport( wxString::Format( _( "Version %s of package %s not found!" ),
                                                     aVersion, aPackage.identifier ),
                                   RPT_SEVERITY_ERROR );
            return PCM_TASK_MANAGER::STATUS::FAILED;
        }

        if( !wxDirExists( file_path.GetPath() )
            && !wxFileName::Mkdir( file_path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            m_reporter->PCMReport( _( "Unable to create download directory!" ),
                                   RPT_SEVERITY_ERROR );
            return PCM_TASK_MANAGER::STATUS::FAILED;
        }

        int code = downloadFile( file_path.GetFullPath(), *find_pkgver->download_url );

        if( code != CURLE_OK )
        {
            // Cleanup after ourselves and exit
            wxRemoveFile( file_path.GetFullPath() );
            return PCM_TASK_MANAGER::STATUS::FAILED;
        }

        PCM_TASK install_task = [aPackage, aVersion, aRepositoryId, file_path, isUpdate, this]()
        {
            return installDownloadedPackage( aPackage, aVersion, aRepositoryId, file_path, isUpdate );
        };

        m_install_queue.push( install_task );

        return PCM_TASK_MANAGER::STATUS::SUCCESS;
    };

    m_download_queue.push( download_task );
    return PCM_TASK_MANAGER::STATUS::SUCCESS;
}


int PCM_TASK_MANAGER::downloadFile( const wxString& aFilePath, const wxString& url )
{
    TRANSFER_CALLBACK callback = [&]( size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow )
    {
        if( dltotal > 1024 )
            m_reporter->SetDownloadProgress( dlnow, dltotal );
        else
            m_reporter->SetDownloadProgress( 0.0, 0.0 );

        return m_reporter->IsCancelled();
    };

    std::ofstream out( aFilePath.ToUTF8(), std::ofstream::binary );

    KICAD_CURL_EASY curl;
    curl.SetOutputStream( &out );
    curl.SetURL( url.ToUTF8().data() );
    curl.SetFollowRedirects( true );
    curl.SetTransferCallback( callback, 250000L );

    m_reporter->PCMReport( wxString::Format( _( "Downloading package url: '%s'" ), url ),
                           RPT_SEVERITY_INFO );

    int code = curl.Perform();

    out.close();

    uint64_t download_total;

    if( CURLE_OK == curl.GetTransferTotal( download_total ) )
        m_reporter->SetDownloadProgress( download_total, download_total );

    if( code != CURLE_OK && code != CURLE_ABORTED_BY_CALLBACK )
    {
        m_reporter->PCMReport( wxString::Format( _( "Failed to download url %s\n%s" ), url,
                                                 curl.GetErrorText( code ) ),
                               RPT_SEVERITY_ERROR );
    }

    return code;
}


PCM_TASK_MANAGER::STATUS PCM_TASK_MANAGER::installDownloadedPackage( const PCM_PACKAGE& aPackage,
                                                 const wxString&    aVersion,
                                                 const wxString&    aRepositoryId,
                                                 const wxFileName& aFilePath, const bool isUpdate )
{
    auto pkgver = std::find_if( aPackage.versions.begin(), aPackage.versions.end(),
                                [&aVersion]( const PACKAGE_VERSION& pv )
                                {
                                    return pv.version == aVersion;
                                } );

    if( pkgver == aPackage.versions.end() )
    {
        m_reporter->PCMReport( wxString::Format( _( "Version %s of package %s not found!" ),
                                                 aVersion, aPackage.identifier ),
                               RPT_SEVERITY_ERROR );
        return PCM_TASK_MANAGER::STATUS::FAILED;
    }

    // wxRegEx is not CopyConstructible hence the weird choice of forward_list
    std::forward_list<wxRegEx> keep_on_update;

    if( isUpdate )
        compile_keep_on_update_regex( aPackage, *pkgver, keep_on_update );

    const std::optional<wxString>& hash = pkgver->download_sha256;
    bool                             hash_match = true;

    if( hash )
    {
        std::ifstream stream( aFilePath.GetFullPath().fn_str(), std::ios::binary );
        hash_match = m_pcm->VerifyHash( stream, *hash );
    }

    if( !hash_match )
    {
        m_reporter->PCMReport( wxString::Format( _( "Downloaded archive hash for package "
                                                    "%s does not match repository entry. "
                                                    "This may indicate a problem with the "
                                                    "package, if the issue persists "
                                                    "report this to repository maintainers." ),
                                                 aPackage.name ),
                               RPT_SEVERITY_ERROR );
        wxRemoveFile( aFilePath.GetFullPath() );
        return PCM_TASK_MANAGER::STATUS::FAILED;
    }
    else
    {
        if( isUpdate )
        {
            m_reporter->PCMReport(
                    wxString::Format( _( "Removing previous version of package '%s'." ),
                                      aPackage.name ),
                    RPT_SEVERITY_INFO );

            deletePackageDirectories( aPackage.identifier, keep_on_update );
        }

        m_reporter->PCMReport(
                wxString::Format( _( "Installing package '%s'." ), aPackage.name ),
                RPT_SEVERITY_INFO );

        if( extract( aFilePath.GetFullPath(), aPackage.identifier, true ) )
        {
            m_pcm->MarkInstalled( aPackage, pkgver->version, aRepositoryId );
        }
        else
        {
            // Cleanup possibly partially extracted package
            deletePackageDirectories( aPackage.identifier );
        }

        std::unique_lock lock( m_changed_package_types_guard );
        m_changed_package_types.insert( aPackage.type );
    }

    wxRemoveFile( aFilePath.GetFullPath() );
    return PCM_TASK_MANAGER::STATUS::SUCCESS;
}


bool PCM_TASK_MANAGER::extract( const wxString& aFilePath, const wxString& aPackageId,
                                bool isMultiThreaded )
{
    wxFFileInputStream stream( aFilePath );
    wxZipInputStream   zip( stream );

    wxLogNull no_wx_logging;

    int entries = zip.GetTotalEntries();
    int extracted = 0;

    wxArchiveEntry* entry = zip.GetNextEntry();

    if( !zip.IsOk() )
    {
        m_reporter->PCMReport( _( "Error extracting file!" ), RPT_SEVERITY_ERROR );
        return false;
    }

    // Namespace delimiter changed on disk to allow flat loading of Python modules
    wxString clean_package_id = aPackageId;
    clean_package_id.Replace( '.', '_' );

    for( ; entry; entry = zip.GetNextEntry() )
    {
        wxArrayString path_parts =
                wxSplit( entry->GetName(), wxFileName::GetPathSeparator(), (wxChar) 0 );

        if( path_parts.size() < 2
            || PCM_PACKAGE_DIRECTORIES.find( path_parts[0] ) == PCM_PACKAGE_DIRECTORIES.end()
            || path_parts[path_parts.size() - 1].IsEmpty() )
        {
            // Ignore files in the root of the archive and files outside of package dirs.
            continue;
        }


        // Transform paths from
        // <PackageRoot>/$folder/$contents
        // To
        // $KICAD7_3RD_PARTY/$folder/$package_id/$contents
        path_parts.Insert( clean_package_id, 1 );
        path_parts.Insert( m_pcm->Get3rdPartyPath(), 0 );

        wxString fullname = wxJoin( path_parts, wxFileName::GetPathSeparator(), (wxChar) 0 );

        // Ensure the target directory exists and create it if not.
        wxString t_path = wxPathOnly( fullname );

        if( !wxDirExists( t_path ) )
        {
            wxFileName::Mkdir( t_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
        }

        wxTempFileOutputStream out( fullname );

        if( !( CopyStreamData( zip, out, entry->GetSize() ) && out.Commit() ) )
        {
            m_reporter->PCMReport( _( "Error extracting file!" ), RPT_SEVERITY_ERROR );
            return false;
        }

        extracted++;
        m_reporter->SetPackageProgress( extracted, entries );

        if( !isMultiThreaded )
            m_reporter->KeepRefreshing( false );

        if( m_reporter->IsCancelled() )
            break;
    }

    zip.CloseEntry();

    if( m_reporter->IsCancelled() )
    {
        m_reporter->PCMReport( _( "Aborting package installation." ), RPT_SEVERITY_INFO );
        return false;
    }

    m_reporter->SetPackageProgress( entries, entries );

    return true;
}


PCM_TASK_MANAGER::STATUS PCM_TASK_MANAGER::InstallFromFile( wxWindow*       aParent,
                                                            const wxString& aFilePath )
{
    wxFFileInputStream stream( aFilePath );

    if( !stream.IsOk() )
    {
        wxLogError( _( "Could not open archive file." ) );
        return PCM_TASK_MANAGER::STATUS::FAILED;
    }

    wxZipInputStream zip( stream );

    if( !zip.IsOk() )
    {
        wxLogError( _( "Invalid archive file format." ) );
        return PCM_TASK_MANAGER::STATUS::FAILED;
    }

    nlohmann::json metadata;

    for( wxArchiveEntry* entry = zip.GetNextEntry(); entry != nullptr; entry = zip.GetNextEntry() )
    {
        // Find and load metadata.json
        if( entry->GetName() != "metadata.json" )
            continue;

        wxStringOutputStream strStream;

        if( CopyStreamData( zip, strStream, entry->GetSize() ) )
        {
            try
            {
                metadata = nlohmann::json::parse( strStream.GetString().ToUTF8().data() );
                m_pcm->ValidateJson( metadata );
            }
            catch( const std::exception& e )
            {
                wxLogError( wxString::Format( _( "Unable to parse package metadata:\n\n%s" ),
                                              e.what() ) );
                break;
            }
        }
    }

    if( metadata.empty() )
    {
        wxLogError( _( "Archive does not contain a valid metadata.json file" ) );
        return PCM_TASK_MANAGER::STATUS::FAILED;
    }

    PCM_PACKAGE package = metadata.get<PCM_PACKAGE>();
    PLUGIN_CONTENT_MANAGER::PreparePackage( package );

    if( package.versions.size() != 1 )
    {
        wxLogError( _( "Archive metadata must have a single version defined" ) );
        return PCM_TASK_MANAGER::STATUS::FAILED;
    }

    if( !package.versions[0].compatible
        && wxMessageBox( _( "This package version is incompatible with your KiCad version or "
                            "platform. Are you sure you want to install it anyway?" ),
                         _( "Install package" ), wxICON_EXCLAMATION | wxYES_NO, aParent )
                   == wxNO )
    {
        return PCM_TASK_MANAGER::STATUS::FAILED;
    }

    bool isUpdate = false;
    // wxRegEx is not CopyConstructible hence the weird choice of forward_list
    std::forward_list<wxRegEx>                keep_on_update;
    const std::vector<PCM_INSTALLATION_ENTRY> installed_packages = m_pcm->GetInstalledPackages();

    if( std::find_if( installed_packages.begin(), installed_packages.end(),
                      [&]( const PCM_INSTALLATION_ENTRY& entry )
                      {
                          return entry.package.identifier == package.identifier;
                      } )
        != installed_packages.end() )
    {
        if( wxMessageBox(
                    wxString::Format(
                            _( "Package with identifier %s is already installed. "
                               "Would you like to update it to the version from selected file?" ),
                            package.identifier ),
                    _( "Update package" ), wxICON_EXCLAMATION | wxYES_NO, aParent )
            == wxNO )
            return PCM_TASK_MANAGER::STATUS::INITIALIZED;

        isUpdate = true;

        compile_keep_on_update_regex( package, package.versions[0], keep_on_update );
    }

    m_reporter = std::make_unique<DIALOG_PCM_PROGRESS>( aParent, false );
#ifdef __WXMAC__
    m_reporter->ShowWindowModal();
#else
    m_reporter->Show();
#endif

    if( isUpdate )
    {
        m_reporter->PCMReport( wxString::Format( _( "Removing previous version of package '%s'." ),
                                                 package.name ),
                               RPT_SEVERITY_INFO );

        deletePackageDirectories( package.identifier, keep_on_update );
    }

    if( extract( aFilePath, package.identifier, false ) )
        m_pcm->MarkInstalled( package, package.versions[0].version, "" );

    m_reporter->SetFinished();
    m_reporter->KeepRefreshing( false );
    m_reporter->Destroy();
    m_reporter.reset();

    aParent->Raise();

    std::unique_lock lock( m_changed_package_types_guard );
    m_changed_package_types.insert( package.type );
    return PCM_TASK_MANAGER::STATUS::SUCCESS;
}


class PATH_COLLECTOR : public wxDirTraverser
{
private:
    std::vector<wxString>& m_files;
    std::vector<wxString>& m_dirs;

public:
    explicit PATH_COLLECTOR( std::vector<wxString>& aFiles, std::vector<wxString>& aDirs ) :
            m_files( aFiles ), m_dirs( aDirs )
    {
    }

    wxDirTraverseResult OnFile( const wxString& aFilePath ) override
    {
        m_files.push_back( aFilePath );
        return wxDIR_CONTINUE;
    }

    wxDirTraverseResult OnDir( const wxString& dirPath ) override
    {
        m_dirs.push_back( dirPath );
        return wxDIR_CONTINUE;
    }
};


void PCM_TASK_MANAGER::deletePackageDirectories( const wxString&                   aPackageId,
                                                 const std::forward_list<wxRegEx>& aKeep )
{
    // Namespace delimiter changed on disk to allow flat loading of Python modules
    wxString clean_package_id = aPackageId;
    clean_package_id.Replace( '.', '_' );

    int path_prefix_len = m_pcm->Get3rdPartyPath().Length();

    auto sort_func = []( const wxString& a, const wxString& b )
    {
        if( a.length() > b.length() )
            return true;
        if( a.length() < b.length() )
            return false;

        if( a != b )
            return a < b;

        return false;
    };

    for( const wxString& dir : PCM_PACKAGE_DIRECTORIES )
    {
        wxFileName d( m_pcm->Get3rdPartyPath(), "" );
        d.AppendDir( dir );
        d.AppendDir( clean_package_id );

        if( !d.DirExists() )
            continue;

        m_reporter->PCMReport( wxString::Format( _( "Removing directory %s" ), d.GetPath() ),
                               RPT_SEVERITY_INFO );

        if( aKeep.empty() )
        {
            if( !d.Rmdir( wxPATH_RMDIR_RECURSIVE ) )
            {
                m_reporter->PCMReport(
                        wxString::Format( _( "Failed to remove directory %s" ), d.GetPath() ),
                        RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            std::vector<wxString> files;
            std::vector<wxString> dirs;
            PATH_COLLECTOR        collector( files, dirs );

            wxDir( d.GetFullPath() )
                    .Traverse( collector, wxEmptyString, wxDIR_DEFAULT | wxDIR_NO_FOLLOW );

            // Do a poor mans post order traversal by sorting paths in reverse length order
            std::sort( files.begin(), files.end(), sort_func );
            std::sort( dirs.begin(), dirs.end(), sort_func );

            // Delete files that don't match any of the aKeep regexs
            for( const wxString& file : files )
            {
                bool del = true;

                for( const wxRegEx& re : aKeep )
                {
                    wxString tmp = file.Mid( path_prefix_len );
                    tmp.Replace( "\\", "/" );

                    if( re.Matches( tmp ) )
                    {
                        del = false;
                        break;
                    }
                }

                if( del )
                    wxRemoveFile( file );
            }

            // Delete any empty dirs
            for( const wxString& empty_dir : dirs )
            {
                wxFileName dname( empty_dir, "" );
                dname.Rmdir(); // not passing any flags here will only remove empty directories
            }
        }
    }
}


PCM_TASK_MANAGER::STATUS PCM_TASK_MANAGER::Uninstall( const PCM_PACKAGE& aPackage )
{
    PCM_TASK task = [aPackage, this]
    {
        deletePackageDirectories( aPackage.identifier );

        m_pcm->MarkUninstalled( aPackage );

        std::unique_lock lock( m_changed_package_types_guard );
        m_changed_package_types.insert( aPackage.type );

        m_reporter->PCMReport(
                wxString::Format( _( "Package %s uninstalled" ), aPackage.name ),
                RPT_SEVERITY_INFO );
        return PCM_TASK_MANAGER::STATUS::SUCCESS;
    };

    m_install_queue.push( task );
    return PCM_TASK_MANAGER::STATUS::SUCCESS;
}


void PCM_TASK_MANAGER::RunQueue( wxWindow* aParent )
{
    m_reporter = std::make_unique<DIALOG_PCM_PROGRESS>( aParent );

    m_reporter->SetNumPhases( m_download_queue.size() + m_install_queue.size() );
#ifdef __WXMAC__
    m_reporter->ShowWindowModal();
#else
    m_reporter->Show();
#endif

    wxSafeYield();

    std::mutex              mutex;
    std::condition_variable condvar;
    bool                    download_complete = false;
    int                     count_tasks = 0;
    int                     count_failed_tasks = 0;
    int                     count_success_tasks = 0;

    std::thread download_thread(
            [&]()
            {
                while( !m_download_queue.empty() && !m_reporter->IsCancelled() )
                {
                    PCM_TASK task;
                    m_download_queue.pop( task );
                    task();
                    condvar.notify_all();
                }

                std::unique_lock<std::mutex> lock( mutex );
                download_complete = true;
                condvar.notify_all();
            } );

    std::thread install_thread(
            [&]()
            {
                std::unique_lock<std::mutex> lock( mutex );

                do
                {
                    condvar.wait( lock,
                                  [&]()
                                  {
                                      return download_complete || !m_install_queue.empty()
                                             || m_reporter->IsCancelled();
                                  } );

                    lock.unlock();

                    while( !m_install_queue.empty() && !m_reporter->IsCancelled() )
                    {
                        PCM_TASK task;
                        m_install_queue.pop( task );
                        PCM_TASK_MANAGER::STATUS task_status = task();

                        count_tasks++;

                        if( task_status == PCM_TASK_MANAGER::STATUS::SUCCESS )
                            count_success_tasks++;
                        else if( task_status != PCM_TASK_MANAGER::STATUS::INITIALIZED )
                            count_failed_tasks++;

                        m_reporter->AdvancePhase();
                    }

                    lock.lock();

                } while( ( !m_install_queue.empty() || !download_complete )
                         && !m_reporter->IsCancelled() );

                if( count_failed_tasks != 0 )
                {
                    m_reporter->PCMReport(
                            wxString::Format( _( "%d out of %d operations failed." ), count_failed_tasks, count_tasks ),
                            RPT_SEVERITY_INFO );
                }
                else
                {
                    if( count_success_tasks == count_tasks )
                    {
                        m_reporter->PCMReport( _( "All operations completed successfully." ), RPT_SEVERITY_INFO );
                    }
                    else
                    {
                        m_reporter->PCMReport(
                                wxString::Format( _( "%d out of %d operations were initialized but not successful." ),
                                                    count_tasks - count_success_tasks, count_tasks ),
                                RPT_SEVERITY_INFO );
                    }
                }

                m_reporter->SetFinished();
            } );

    m_reporter->KeepRefreshing( true );

    download_thread.join();
    install_thread.join();

    // Destroy the reporter only after the threads joined
    // Incase the reporter terminated due to cancellation
    m_reporter->Destroy();
    m_reporter.reset();

    aParent->Raise();
}
