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
#include <gestfich.h>

#include <algorithm>
#include <array>
#include <filesystem>

#include <wx/dir.h>

#include <board.h>
#include <reporter.h>
#include <paths.h>
#include <pcb_edit_frame.h>
#include <progress_reporter.h>
#include <project.h>
#include <io/io_mgr.h>
#include <jobs/job_export_pcb_odb.h>
#include <pcb_io/pcb_io_mgr.h>
#include <locale_io.h>


namespace
{
class TEMP_ODB_DIRECTORY
{
public:
    bool Create( const wxString& aPrefix = wxS( "kicad-odb" ) )
    {
        m_path = wxFileName::CreateTempFileName( aPrefix );

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


std::filesystem::path toFsPath( const wxString& aPath )
{
#ifdef __WXMSW__
    return std::filesystem::path( std::wstring( aPath.wc_str() ) );
#else
    return std::filesystem::path( aPath.utf8_string() );
#endif
}


wxString fromFsPath( const std::filesystem::path& aPath )
{
#ifdef __WXMSW__
    return wxString( aPath.wstring() );
#else
    std::u8string utf8 = aPath.u8string();
    return wxString::FromUTF8( reinterpret_cast<const char*>( utf8.data() ), utf8.size() );
#endif
}


bool containsBoardFile( const std::filesystem::path& aTarget, const wxString& aBoardFile )
{
    namespace fs = std::filesystem;

    if( aBoardFile.IsEmpty() )
        return false;

    fs::path        file = toFsPath( aBoardFile );
    std::error_code ec;

    if( !fs::exists( file, ec ) )
        return bool( ec );

    for( fs::path dir = file.parent_path(); !dir.empty(); dir = dir.parent_path() )
    {
        if( fs::equivalent( aTarget, dir, ec ) )
            return true;

        if( ec )
            return true;

        if( dir == dir.root_path() )
            break;
    }

    return false;
}


bool canReplaceOdbDirectory( const wxString& aTarget, const wxString& aBoardFile,
                             const wxString& aJobFile, wxString& aError )
{
    namespace fs = std::filesystem;
    fs::path        target = toFsPath( aTarget );
    std::error_code ec;

    if( !fs::exists( target, ec ) )
    {
        if( ec )
            aError = wxString::FromUTF8( ec.message() );

        return !ec;
    }

    if( !fs::is_directory( target, ec ) )
    {
        aError = ec ? wxString::FromUTF8( ec.message() ) : _( "Output path is not a directory" );
        return false;
    }

    bool empty = fs::is_empty( target, ec );

    if( ec )
    {
        aError = wxString::FromUTF8( ec.message() );
        return false;
    }

    if( empty )
        return true;

    if( !fs::is_regular_file( target / "matrix" / "matrix", ec ) )
    {
        aError = _( "Output directory is not an ODB++ product" );

        if( ec )
            aError += wxS( "\n" ) + wxString::FromUTF8( ec.message() );

        return false;
    }

    static const std::array<fs::path, 9> allowed = {
        fs::path( "ext" ), fs::path( "fonts" ), fs::path( "input" ), fs::path( "matrix" ), fs::path( "misc" ),
        fs::path( "steps" ), fs::path( "symbols" ), fs::path( "user" ), fs::path( "wheels" )
    };

    fs::directory_iterator child( target, ec );
    fs::directory_iterator end;

    while( !ec && child != end )
    {
        fs::path name = child->path().filename();

        if( std::find( allowed.begin(), allowed.end(), name ) == allowed.end() )
        {
            aError = wxString::Format( _( "Output directory contains unrelated entry '%s'" ), fromFsPath( name ) );
            return false;
        }

        child.increment( ec );
    }

    if( ec )
    {
        aError = wxString::FromUTF8( ec.message() );
        return false;
    }

    if( containsBoardFile( target, aBoardFile ) || containsBoardFile( target, aJobFile ) )
    {
        aError = _( "Output directory contains the source board" );
        return false;
    }

    fs::recursive_directory_iterator nested( target, ec );
    fs::recursive_directory_iterator nestedEnd;

    while( !ec && nested != nestedEnd )
    {
        bool regular = nested->is_regular_file( ec );

        if( ec )
        {
            aError = wxString::FromUTF8( ec.message() );
            return false;
        }

        if( regular )
        {
            wxString extension = fromFsPath( nested->path().extension() ).Lower();

            if( extension.StartsWith( wxS( ".kicad_" ) ) || extension == wxS( ".pro" )
                || extension == wxS( ".sch" ) || extension == wxS( ".pcb" ) )
            {
                aError = wxString::Format( _( "Output directory contains project file '%s'" ),
                                            fromFsPath( nested->path().filename() ) );
                return false;
            }
        }

        nested.increment( ec );
    }

    if( ec )
        aError = wxString::FromUTF8( ec.message() );

    return !ec;
}


bool CommitOdbDirectory( const wxString& aSource, const wxString& aTarget,
                         const wxString& aBoardFile, const wxString& aJobFile,
                         REPORTER* aReporter, wxString& aError )
{
    namespace fs = std::filesystem;
    fs::path        source = toFsPath( aSource );
    fs::path        target = toFsPath( aTarget );
    std::error_code ec;

    if( !canReplaceOdbDirectory( aTarget, aBoardFile, aJobFile, aError ) )
        return false;

    if( !fs::exists( target, ec ) )
    {
        if( ec )
        {
            aError = wxString::FromUTF8( ec.message() );
            return false;
        }

        fs::rename( source, target, ec );

        if( ec )
            aError = wxString::FromUTF8( ec.message() );

        return !ec;
    }

    wxString backup = wxFileName::CreateTempFileName( aTarget + wxS( ".kicad-backup-" ) );

    if( backup.IsEmpty() || !wxRemoveFile( backup ) )
    {
        aError = _( "Cannot create temporary backup path" );
        return false;
    }

    fs::path backupPath = toFsPath( backup );
    fs::rename( target, backupPath, ec );

    if( ec )
    {
        aError = wxString::FromUTF8( ec.message() );
        return false;
    }

    fs::rename( source, target, ec );

    if( ec )
    {
        aError = wxString::FromUTF8( ec.message() );
        std::error_code restoreError;
        fs::rename( backupPath, target, restoreError );

        if( restoreError )
            aError += wxS( "\n" ) + wxString::FromUTF8( restoreError.message() );

        return false;
    }

    fs::remove_all( backupPath, ec );

    if( ec && aReporter )
    {
        aReporter->Report( wxString::Format( _( "Cannot remove previous ODB++ directory '%s': %s" ),
                                             backup, wxString::FromUTF8( ec.message() ) ), RPT_SEVERITY_WARNING );
    }

    return true;
}


} // namespace


wxFileName ResolveOdbOutputPath( const JOB_EXPORT_PCB_ODB& aJob, BOARD* aBoard )
{
    wxString outputPath = aJob.GetFullOutputPath( aBoard->GetProject() );

    if( outputPath.IsEmpty() )
        outputPath = wxFileName( aJob.m_filename ).GetPath();

    bool compressed = aJob.m_compressionMode != JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::NONE;

    if( !compressed && !outputPath.IsEmpty() )
    {
        std::filesystem::path directory = toFsPath( outputPath );

        if( directory.filename().empty() && directory != directory.root_path() )
            outputPath = fromFsPath( directory.parent_path() );
    }

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

    if( !PATHS::EnsurePathExists( outputFn.GetFullPath(), true ) )
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
    else
    {
        wxString replaceError;

        if( !canReplaceOdbDirectory( outputFn.GetFullPath(), aBoard->GetFileName(), aJob.m_filename, replaceError ) )
        {
            if( aReporter )
            {
                aReporter->Report( wxString::Format( _( "Cannot use ODB++ directory '%s'.\n%s" ),
                                                     outputFn.GetFullPath(), replaceError ), RPT_SEVERITY_ERROR );
            }

            return false;
        }
    }

    TEMP_ODB_DIRECTORY temporary;
    wxString           prefix = compressed ? wxString( wxS( "kicad-odb" ) )
                                           : outputFn.GetFullPath() + wxS( ".kicad-temp-" );

    if( !temporary.Create( prefix ) )
    {
        if( aReporter )
            aReporter->Report( _( "Cannot create temporary output directory." ), RPT_SEVERITY_ERROR );

        return false;
    }

    wxString treePath = temporary.Path();

    std::map<std::string, UTF8> props;
    props["units"] = aJob.m_units == JOB_EXPORT_PCB_FAB::UNITS::MM ? "mm" : "inch";
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

        ARCHIVE_FORMAT format = aJob.m_compressionMode == JOB_EXPORT_PCB_ODB::ODB_COMPRESSION::TGZ
                                ? ARCHIVE_FORMAT::TGZ : ARCHIVE_FORMAT::ZIP;
        wxString archiveError;

        if( !WriteDirectoryArchive( treePath, outputFn.GetFullPath(), format, product, &archiveError ) )
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
    else
    {
        wxString commitError;

        if( !CommitOdbDirectory( treePath, outputFn.GetFullPath(), aBoard->GetFileName(),
                                 aJob.m_filename, aReporter, commitError ) )
        {
            if( aReporter )
            {
                aReporter->Report( wxString::Format( _( "Cannot replace ODB++ directory '%s'.\n%s" ),
                                                     outputFn.GetFullPath(), commitError ), RPT_SEVERITY_ERROR );
            }

            return false;
        }
    }

    if( aProgressReporter )
        aProgressReporter->SetCurrentProgress( 1 );

    return true;
}
