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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "odb_export_job.h"
#include "odb_util.h"

#include <thread_pool.h>

#include <wx/dir.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/tarstrm.h>
#include <wx/zstream.h>

#include <board.h>
#include <reporter.h>
#include <paths.h>
#include <progress_reporter.h>
#include <project.h>
#include <io/io_mgr.h>
#include <jobs/job_export_pcb_odb.h>
#include <pcb_io/pcb_io_mgr.h>
#include <kiplatform/io.h>
#include <locale_io.h>


namespace
{
class TEMP_ODB_DIRECTORY
{
public:
    bool Create()
    {
        m_path = wxFileName::CreateTempFileName( wxS( "kicad-odb" ) );

        if( m_path.IsEmpty() || !wxRemoveFile( m_path ) )
            return false;

        return wxFileName::Mkdir( m_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    }

    ~TEMP_ODB_DIRECTORY()
    {
        if( wxDirExists( m_path ) )
            wxFileName::Rmdir( m_path, wxPATH_RMDIR_RECURSIVE );
        else if( wxFileExists( m_path ) )
            wxRemoveFile( m_path );
    }

    const wxString& Path() const { return m_path; }

private:
    wxString m_path;
};


bool AddArchiveContents( wxArchiveOutputStream& aArchive, const wxString& aSourceDir, const wxString& aParent )
{
    wxDir dir( aSourceDir );

    if( !dir.IsOpened() )
        return false;

    wxString name;
    bool     cont = dir.GetFirst( &name, wxEmptyString, wxDIR_DEFAULT );

    while( cont )
    {
        wxFileName source( aSourceDir, name );
        wxString   relative = aParent + wxS( "/" ) + name;

        if( wxDirExists( source.GetFullPath() ) )
        {
            if( !aArchive.PutNextDirEntry( relative )
                || !AddArchiveContents( aArchive, source.GetFullPath(), relative ) )
                return false;
        }
        else
        {
            wxFFileInputStream input( source.GetFullPath() );
            wxFileOffset       expectedLength = input.IsOk() ? input.GetLength() : wxInvalidOffset;

            if( !input.IsOk() || !aArchive.PutNextEntry( relative, wxDateTime::Now(), expectedLength ) )
                return false;

            input.Read( aArchive );

            if( input.TellI() != expectedLength || !aArchive.IsOk() || aArchive.GetLastError() != wxSTREAM_NO_ERROR
                || !aArchive.CloseEntry() )
                return false;
        }

        cont = dir.GetNext( &name );
    }

    return true;
}
} // namespace


bool WriteOdbArchive( const wxString& aSourceDir, wxOutputStream& aOut, bool aTgz, const wxString& aTopDir )
{
    if( !aOut.IsOk() || !wxDirExists( aSourceDir ) || aTopDir.IsEmpty() )
        return false;

    auto fill = [&]( wxArchiveOutputStream& aArchive )
    {
        bool ok = aArchive.PutNextDirEntry( aTopDir ) && AddArchiveContents( aArchive, aSourceDir, aTopDir );
        bool archiveOk = aArchive.IsOk() && aArchive.GetLastError() == wxSTREAM_NO_ERROR;
        bool archiveClosed = aArchive.Close();
        return ok && archiveOk && archiveClosed;
    };

    if( aTgz )
    {
        wxZlibOutputStream gzip( aOut, -1, wxZLIB_GZIP );
        wxTarOutputStream  archive( gzip );
        bool               filled = fill( archive );
        bool               gzipClosed = gzip.Close();
        return filled && gzipClosed && aOut.IsOk();
    }

    wxZipOutputStream archive( aOut );
    return fill( archive ) && aOut.IsOk();
}


bool WriteOdbArchiveFile( const wxString& aSourceDir, const wxString& aTarget, bool aTgz, const wxString& aTopDir,
                          const ODB_STREAM_FACTORY& aOpen, wxString* aError )
{
    wxString tempPath;
    wxString error;
    FILE*    fp = KIPLATFORM::IO::OpenUniqueSiblingTempFile( aTarget, wxS( "wb" ), &tempPath, &error );

    if( !fp )
    {
        if( aError )
            *aError = error.IsEmpty() ? _( "Cannot create temporary archive file" ) : error;

        return false;
    }

    // Use a large I/O buffer for cloud-synced folders
    // See KIPLATFORM::IO::CLOUD_SYNC_BUFFER_SIZE
    setvbuf( fp, nullptr, _IOFBF, KIPLATFORM::IO::CLOUD_SYNC_BUFFER_SIZE );

    bool ok = false;

    {
        wxFFile file( fp );

        try
        {
            std::unique_ptr<wxOutputStream> output = aOpen( file );

            if( output )
                ok = WriteOdbArchive( aSourceDir, *output, aTgz, aTopDir );
        }
        catch( const std::exception& exception )
        {
            ok = false;
            error = wxString::FromUTF8( exception.what() );
        }

        bool  writeOk = ok;
        bool  streamError = file.Error();
        FILE* raw = file.Detach();
        bool  flushed = KIPLATFORM::IO::FlushToDisk( raw );
        bool  closed = fclose( raw ) == 0;
        ok = writeOk && !streamError && flushed && closed;

        if( !writeOk || streamError )
        {
            if( error.IsEmpty() )
                error = _( "Write error" );
        }
        else if( !flushed || !closed )
        {
            error = _( "Cannot flush or close temporary file" );
        }
    }

    if( ok )
    {
        ok = KIPLATFORM::IO::CommitTempFile( tempPath, aTarget, &error );

        if( !ok && error.IsEmpty() )
            error = _( "Cannot replace archive" );
    }

    if( !ok && wxFileExists( tempPath ) )
    {
        if( !wxRemoveFile( tempPath ) )
            error += _( "\nCannot remove temporary archive file" );
    }

    if( aError )
        *aError = error;

    return ok;
}


wxFileName ResolveOdbOutputPath( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard )
{
    wxString outputPath = aJob.GetFullOutputPath( aBoard->GetProject() );

    if( outputPath.IsEmpty() )
        outputPath = wxFileName( aJob.m_filename ).GetPath();

    wxFileName outputFn( outputPath );
    // Write through symlinks, don't replace them
    WX_FILENAME::ResolvePossibleSymlinks( outputFn );

    if( outputFn.GetPath().IsEmpty() && outputFn.HasName() )
        outputFn.MakeAbsolute();

    return outputFn;
}


bool GenerateODBPPFiles( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard, PROGRESS_REPORTER* aProgressReporter,
                         REPORTER* aReporter )
{
    LOCALE_IO toggle;

    if( !aBoard )
    {
        if( aReporter )
            aReporter->Report( _( "No board for ODB++ export." ), RPT_SEVERITY_ERROR );

        return false;
    }

    wxFileName outputFn = ResolveOdbOutputPath( aJob, aBoard );
    bool       compressed = aJob.m_compressionMode != JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE;
    wxString   msg;

    if( !PATHS::EnsurePathExists( outputFn.GetFullPath(), compressed ) )
    {
        msg.Printf( _( "Cannot create output directory '%s'." ), outputFn.GetFullPath() );

        if( aReporter )
            aReporter->Report( msg, RPT_SEVERITY_ERROR );

        return false;
    }

    if( outputFn.IsDir() && !outputFn.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to folder '%s'." ), outputFn.GetPath() );

        if( aReporter )
            aReporter->Report( msg, RPT_SEVERITY_ERROR );

        return false;
    }

    if( compressed )
    {
        bool writable = outputFn.FileExists() ? outputFn.IsFileWritable() : outputFn.IsDirWritable();

        if( !writable )
        {
            msg.Printf( _( "Insufficient permissions to save file '%s'." ), outputFn.GetFullPath() );

            if( aReporter )
                aReporter->Report( msg, RPT_SEVERITY_ERROR );

            return false;
        }
    }

    TEMP_ODB_DIRECTORY temporary;
    wxString           treePath = outputFn.GetFullPath();

    if( compressed )
    {
        if( !temporary.Create() )
        {
            if( aReporter )
                aReporter->Report( _( "Cannot create temporary output directory." ), RPT_SEVERITY_ERROR );

            return false;
        }

        treePath = temporary.Path();
    }

    std::map<std::string, UTF8> props;
    props["units"] = aJob.m_units == JOB_EXPORT_PCB_ODB::ODB_UNITS::MM ? "mm" : "inch";
    props["sigfig"] = wxString::Format( "%d", aJob.m_precision );

    auto saveFile = [&]() -> bool
    {
        try
        {
            IO_RELEASER<PCB_IO> plugin( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::ODBPP ) );
            plugin->SetReporter( aReporter );
            plugin->SetProgressReporter( aProgressReporter );
            plugin->SaveBoard( treePath, *aBoard, &props );
            return true;
        }
        catch( const IO_ERROR& error )
        {
            if( aReporter )
            {
                msg = wxString::Format( _( "Error generating ODB++ files '%s'.\n%s" ), treePath, error.What() );
                aReporter->Report( msg, RPT_SEVERITY_ERROR );
            }

            return false;
        }
    };

    auto future = GetKiCadThreadPool().submit_task( saveFile );

    while( future.wait_for( std::chrono::milliseconds( 250 ) ) != std::future_status::ready )
    {
        if( aProgressReporter )
            aProgressReporter->KeepRefreshing();
    }

    try
    {
        if( !future.get() )
            return false;
    }
    catch( const std::exception& error )
    {
        if( aReporter )
        {
            aReporter->Report( wxString::Format( _( "Exception in ODB++ generation: %s" ), error.what() ),
                               RPT_SEVERITY_ERROR );
        }

        return false;
    }

    if( compressed )
    {
        if( aProgressReporter )
            aProgressReporter->AdvancePhase( _( "Compressing output" ) );

        wxString boardName = wxFileName( aBoard->GetFileName() ).GetName();

        if( boardName.IsEmpty() )
            boardName = wxFileName( aJob.m_filename ).GetName();

        wxString product = ODB::GenLegalEntityName( boardName );

        if( product.IsEmpty() )
            product = wxS( "odb" );

        ODB_STREAM_FACTORY open = []( wxFFile& aFile )
        {
            return std::make_unique<wxFFileOutputStream>( aFile );
        };

        bool tgz = aJob.m_compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ;

        wxString archiveError;

        if( !WriteOdbArchiveFile( treePath, outputFn.GetFullPath(), tgz, product, open, &archiveError ) )
        {
            if( aReporter )
            {
                aReporter->Report( wxString::Format( _( "Cannot write ODB++ archive '%s'.\n%s" ),
                                                     outputFn.GetFullPath(), archiveError ),
                                   RPT_SEVERITY_ERROR );
            }

            return false;
        }
    }

    if( aProgressReporter )
        aProgressReporter->SetCurrentProgress( 1 );

    return true;
}
