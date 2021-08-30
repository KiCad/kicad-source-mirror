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
// kicad_curl_easy.h **must be** included before any wxWidgets header to avoid conflicts
// at least on Windows/msys2
#include <kicad_curl/kicad_curl.h>
#include "kicad_curl/kicad_curl_easy.h"

#include "pcm_task_manager.h"
#include "paths.h"
#include "picosha2.h"
#include "reporter.h"
#include "wxstream_helper.h"

#include <fstream>
#include <thread>
#include <unordered_set>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/fs_zip.h>
#include <wx/msgdlg.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>


void PCM_TASK_MANAGER::DownloadAndInstall( const PCM_PACKAGE& aPackage, const wxString& aVersion,
                                           const wxString& aRepositoryId )
{
    PCM_TASK download_task = [aPackage, aVersion, aRepositoryId, this]()
    {
        wxFileName file_path( m_pcm->Get3rdPartyPath(), "" );
        file_path.AppendDir( "cache" );
        file_path.SetFullName( wxString::Format( "%s_v%s.zip", aPackage.identifier, aVersion ) );

        auto find_pkgver = std::find_if( aPackage.versions.begin(), aPackage.versions.end(),
                                    [&aVersion]( const PACKAGE_VERSION& pv )
                                    {
                                        return pv.version == aVersion;
                                    } );

        wxASSERT_MSG( find_pkgver != aPackage.versions.end(), "Package version not found" );

        if( !wxDirExists( file_path.GetPath() )
            && !wxFileName::Mkdir( file_path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        {
            m_reporter->Report( _( "Unable to create download directory!" ), RPT_SEVERITY_ERROR );
            return;
        }

        int code = downloadFile( file_path.GetFullPath(), find_pkgver->download_url.get() );

        if( code == CURLE_OK )
        {
            PCM_TASK install_task = [aPackage, aVersion, aRepositoryId, file_path, this]()
            {
                auto get_pkgver = std::find_if( aPackage.versions.begin(), aPackage.versions.end(),
                                            [&aVersion]( const PACKAGE_VERSION& pv )
                                            {
                                                return pv.version == aVersion;
                                            } );

                const boost::optional<wxString>& hash = get_pkgver->download_sha256;
                bool                             hash_match = true;

                if( hash )
                {
                    std::ifstream stream( file_path.GetFullPath().ToUTF8(), std::ios::binary );
                    hash_match = m_pcm->VerifyHash( stream, hash.get() );
                }

                if( !hash_match )
                {
                    m_reporter->Report(
                            wxString::Format(
                                    _( "Downloaded archive hash for package %s does not match "
                                       "repository entry. This may indicate a problem with the "
                                       "package, if the issue persists report this to repository "
                                       "maintainers." ),
                                    aPackage.identifier ),
                            RPT_SEVERITY_ERROR );
                }
                else
                {
                    m_reporter->Report( wxString::Format( _( "Extracting package '%s'." ),
                                                          aPackage.identifier ) );

                    if( extract( file_path.GetFullPath(), aPackage.identifier ) )
                    {
                        m_pcm->MarkInstalled( aPackage, get_pkgver->version, aRepositoryId );
                        // TODO register libraries.
                    }
                    else
                    {
                        // Cleanup possibly partially extracted package
                        deletePackageDirectories( aPackage.identifier );
                    }
                }

                m_reporter->Report( wxString::Format( _( "Removing downloaded archive '%s'." ),
                                                      file_path.GetFullName() ) );
                wxRemoveFile( file_path.GetFullPath() );
            };

            m_install_queue.push( install_task );
        }
        else
        {
            // Cleanup after ourselves.
            wxRemoveFile( file_path.GetFullPath() );
        }
    };

    m_download_queue.push( download_task );
}


int PCM_TASK_MANAGER::downloadFile( const wxString& aFilePath, const wxString& url )
{
    TRANSFER_CALLBACK callback = [&]( size_t dltotal, size_t dlnow, size_t ultotal, size_t ulnow )
    {
        if( dltotal > 1024 )
        {
            m_reporter->SetDownloadProgress( dlnow, dltotal );
        }
        else
        {
            m_reporter->SetDownloadProgress( 0.0, 0.0 );
        }
        return m_reporter->IsCancelled();
    };

    std::ofstream out( aFilePath.ToUTF8(), std::ofstream::binary );

    KICAD_CURL_EASY curl;
    curl.SetOutputStream( &out );
    curl.SetURL( url.ToUTF8().data() );
    curl.SetFollowRedirects( true );
    curl.SetTransferCallback( callback, 250000L );

    m_reporter->Report( wxString::Format( _( "Downloading package url: '%s'" ), url ) );

    int code = curl.Perform();

    out.close();

    uint64_t download_total;

    if( CURLE_OK == curl.GetTransferTotal( download_total ) )
        m_reporter->SetDownloadProgress( download_total, download_total );

    if( code != CURLE_OK && code != CURLE_ABORTED_BY_CALLBACK )
    {
        m_reporter->Report( wxString::Format( _( "Failed to download url %s\n%s" ), url,
                                              curl.GetErrorText( code ) ),
                            RPT_SEVERITY_ERROR );
    }

    return code;
}


bool PCM_TASK_MANAGER::extract( const wxString& aFilePath, const wxString& aPackageId )
{
    wxFFileInputStream stream( aFilePath );
    wxZipInputStream   zip( stream );

    wxLogNull no_wx_logging;

    int entries = zip.GetTotalEntries();
    int extracted = 0;

    wxArchiveEntry* entry = zip.GetNextEntry();

    if( !zip.IsOk() )
    {
        m_reporter->Report( _( "Error extracting file!" ), RPT_SEVERITY_ERROR );
        return false;
    }

    for( ; entry; entry = zip.GetNextEntry() )
    {
        wxArrayString path_parts =
                wxSplit( entry->GetName(), wxFileName::GetPathSeparator(), (wxChar) NULL );

        if( path_parts.size() < 2
            || PCM_PACKAGE_DIRECTORIES.find( path_parts[0] ) == PCM_PACKAGE_DIRECTORIES.end()
            || path_parts[path_parts.size() - 1].IsEmpty() )
        {
            // Ignore files in the root of the archive and files outside of package dirs.
            continue;
        }

        // m_reporter->Report( wxString::Format( _( "Extracting file \"%s\"\n" ), entry->GetName() ),
        //                     RPT_SEVERITY_INFO );

        // Transform paths from
        // <PackageRoot>/$folder/$contents
        // To
        // $KICAD6_3RD_PARTY/$folder/$package_id/$contents
        path_parts.Insert( aPackageId, 1 );
        path_parts.Insert( m_pcm->Get3rdPartyPath(), 0 );

        wxString fullname = wxJoin( path_parts, wxFileName::GetPathSeparator(), (wxChar) NULL );

        // Ensure the target directory exists and create it if not.
        wxString t_path = wxPathOnly( fullname );

        if( !wxDirExists( t_path ) )
        {
            wxFileName::Mkdir( t_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
        }

        wxTempFileOutputStream out( fullname );

        if( !( CopyStreamData( zip, out, entry->GetSize() ) && out.Commit() ) )
        {
            m_reporter->Report( _( "Error extracting file!" ), RPT_SEVERITY_ERROR );
            return false;
        }

        extracted++;
        m_reporter->SetOverallProgress( extracted, entries );

        if( m_reporter->IsCancelled() )
            break;
    }

    zip.CloseEntry();

    if( m_reporter->IsCancelled() )
    {
        m_reporter->Report( _( "Aborting package installation." ) );
        return false;
    }

    m_reporter->Report( wxT( "Extracted package\n" ), RPT_SEVERITY_INFO );
    m_reporter->SetOverallProgress( entries, entries );

    return true;
}


void PCM_TASK_MANAGER::InstallFromFile( wxWindow* aParent, const wxString& aFilePath )
{
    wxFFileInputStream stream( aFilePath );

    if( !stream.IsOk() )
    {
        wxLogError( _( "Could not open archive file." ) );
        return;
    }

    wxZipInputStream zip( stream );

    if( !zip.IsOk() )
    {
        wxLogError( _( "Invalid archive file format." ) );
        return;
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
        return;
    }

    PCM_PACKAGE package = metadata.get<PCM_PACKAGE>();

    if( package.versions.size() != 1 )
    {
        wxLogError( _( "Archive metadata must have a single version defined" ) );
        return;
    }

    const auto installed_packages = m_pcm->GetInstalledPackages();
    if( std::find_if( installed_packages.begin(), installed_packages.end(),
                      [&]( const PCM_INSTALLATION_ENTRY& entry )
                      {
                          return entry.package.identifier == package.identifier;
                      } )
        != installed_packages.end() )
    {
        wxLogError( wxString::Format( _( "Package with identifier %s is already installed, you "
                                         "must first uninstall this package." ),
                                      package.identifier ) );
        return;
    }

    m_reporter = new DIALOG_PCM_PROGRESS( aParent, false );
    m_reporter->Show();

    if( extract( aFilePath, package.identifier ) )
        m_pcm->MarkInstalled( package, package.versions[0].version, "" );

    m_reporter->SetFinished();
    m_reporter->ShowModal();
    m_reporter->Destroy();
}


void PCM_TASK_MANAGER::deletePackageDirectories( const wxString& aPackageId )
{
    for( const wxString& dir : PCM_PACKAGE_DIRECTORIES )
    {
        wxFileName d( m_pcm->Get3rdPartyPath(), "" );
        d.AppendDir( dir );
        d.AppendDir( aPackageId );

        if( d.DirExists() )
        {
            m_reporter->Report( wxString::Format( _( "Removing directory %s" ), d.GetPath() ) );

            if( !d.Rmdir( wxPATH_RMDIR_RECURSIVE ) )
            {
                m_reporter->Report(
                        wxString::Format( _( "Failed to remove directory %s" ), d.GetPath() ),
                        RPT_SEVERITY_ERROR );
            }
        }
    }
}


void PCM_TASK_MANAGER::Uninstall( const PCM_PACKAGE& aPackage )
{
    PCM_TASK task = [aPackage, this]
    {
        deletePackageDirectories( aPackage.identifier );

        m_pcm->MarkUninstalled( aPackage );

        m_reporter->Report(
                wxString::Format( _( "Package %s uninstalled" ), aPackage.identifier ) );
    };

    m_install_queue.push( task );
}


void PCM_TASK_MANAGER::RunQueue( wxWindow* aParent )
{
    m_reporter = new DIALOG_PCM_PROGRESS( aParent );

    m_reporter->SetOverallProgressPhases( m_download_queue.size() + m_install_queue.size() );
    m_reporter->Show();

    std::mutex              mutex;
    std::condition_variable condvar;
    bool                    download_complete = false;

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

                m_reporter->SetDownloadsFinished();

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
                        task();
                        m_reporter->AdvanceOverallProgressPhase();
                    }

                    lock.lock();

                } while( ( !m_install_queue.empty() || !download_complete )
                         && !m_reporter->IsCancelled() );

                if( m_reporter->IsCancelled() )
                {
                    m_reporter->SetOverallProgressPhases( 1 );
                    m_reporter->SetOverallProgress( 1, 1 );
                    m_reporter->Report( _( "Aborting remaining tasks." ) );
                }

                m_reporter->Report( _( "Done." ) );

                m_reporter->SetFinished();
            } );

    m_reporter->ShowModal();
    m_reporter->Destroy();

    download_thread.join();
    install_thread.join();
}
