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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <memory>
#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/fs_zip.h>
#include <wx/uri.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <core/arraydim.h>
#include <macros.h>
#include <project/project_archiver.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <wxstream_helper.h>
#include <wx/log.h>
#include <kiplatform/io.h>

#include <regex>
#include <set>


#define ZipFileExtension wxT( "zip" )

class PROJECT_ARCHIVER_DIR_TRAVERSER : public wxDirTraverser
{
public:
    PROJECT_ARCHIVER_DIR_TRAVERSER( const std::string& aExtRegex, wxArrayString& aFiles ) :
        m_files( aFiles ),
        m_fileExtRegex( aExtRegex, std::regex_constants::ECMAScript | std::regex_constants::icase )
    {}

    virtual wxDirTraverseResult OnFile( const wxString& aFilename ) override
    {
        if( std::regex_search( aFilename.ToStdString(), m_fileExtRegex ) )
            m_files.Add( aFilename );

        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnDir( const wxString& aDirname ) override
    {
        return wxDIR_CONTINUE;
    }

private:
    wxArrayString& m_files;
    std::regex     m_fileExtRegex;
};

PROJECT_ARCHIVER::PROJECT_ARCHIVER()
{
}

bool PROJECT_ARCHIVER::AreZipArchivesIdentical( const wxString& aZipFileA,
                                                const wxString& aZipFileB, REPORTER& aReporter )
{
    wxFFileInputStream streamA( aZipFileA );
    wxFFileInputStream streamB( aZipFileB );

    if( !streamA.IsOk() || !streamB.IsOk() )
    {
        aReporter.Report( _( "Could not open archive file." ), RPT_SEVERITY_ERROR );
        return false;
    }

    wxZipInputStream zipStreamA = wxZipInputStream( streamA );
    wxZipInputStream zipStreamB = wxZipInputStream( streamB );

    std::set<wxUint32> crcsA;
    std::set<wxUint32> crcsB;


    for( wxZipEntry* entry = zipStreamA.GetNextEntry(); entry; entry = zipStreamA.GetNextEntry() )
    {
        crcsA.insert( entry->GetCrc() );
    }

    for( wxZipEntry* entry = zipStreamB.GetNextEntry(); entry; entry = zipStreamB.GetNextEntry() )
    {
        crcsB.insert( entry->GetCrc() );
    }

    return crcsA == crcsB;
}


// Unarchive Files code comes from wxWidgets sample/archive/archive.cpp
bool PROJECT_ARCHIVER::Unarchive( const wxString& aSrcFile, const wxString& aDestDir,
                                  REPORTER& aReporter )
{
    wxFFileInputStream stream( aSrcFile );

    if( !stream.IsOk() )
    {
        aReporter.Report( _( "Could not open archive file." ), RPT_SEVERITY_ERROR );
        return false;
    }

    const wxArchiveClassFactory* archiveClassFactory =
            wxArchiveClassFactory::Find( aSrcFile, wxSTREAM_FILEEXT );

    if( !archiveClassFactory )
    {
        aReporter.Report( _( "Invalid archive file format." ), RPT_SEVERITY_ERROR );
        return false;
    }

    std::unique_ptr<wxArchiveInputStream> archiveStream( archiveClassFactory->NewStream( stream ) );

    wxString fileStatus;

    for( wxArchiveEntry* entry = archiveStream->GetNextEntry(); entry;
         entry = archiveStream->GetNextEntry() )
    {
        fileStatus.Printf( _( "Extracting file '%s'." ), entry->GetName() );
        aReporter.Report( fileStatus, RPT_SEVERITY_INFO );

        wxString fullname = aDestDir + entry->GetName();

        // Ensure the target directory exists and create it if not
        wxString t_path = wxPathOnly( fullname );

        if( !wxDirExists( t_path ) )
        {
            wxFileName::Mkdir( t_path, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
        }

        // Directory entries need only be created, not extracted (0 size)
        if( entry->IsDir() )
            continue;


        wxTempFileOutputStream outputFileStream( fullname );

        if( CopyStreamData( *archiveStream, outputFileStream, entry->GetSize() ) )
            outputFileStream.Commit();
        else
            aReporter.Report( _( "Error extracting file!" ), RPT_SEVERITY_ERROR );

        // Now let's set the filetimes based on what's in the zip
        wxFileName outputFileName( fullname );
        wxDateTime fileTime = entry->GetDateTime();
        // For now we set access, mod, create to the same datetime
        // create (third arg) is only used on Windows
        outputFileName.SetTimes( &fileTime, &fileTime, &fileTime );
    }

    aReporter.Report( wxT( "Extracted project." ), RPT_SEVERITY_INFO );
    return true;
}


bool PROJECT_ARCHIVER::Archive( const wxString& aSrcDir, const wxString& aDestFile,
                                REPORTER& aReporter, bool aVerbose, bool aIncludeExtraFiles )
{

#define EXT( ext )              "\\." + ext + "|"
#define NAME( name )            name + "|"
#define EXT_NO_PIPE( ext )      "\\." + ext
#define NAME_NO_PIPE( name )    name

    // List of file extensions that are always archived
    std::string fileExtensionRegex = "("
            EXT( FILEEXT::ProjectFileExtension )
            EXT( FILEEXT::ProjectLocalSettingsFileExtension )
            EXT( FILEEXT::KiCadSchematicFileExtension )
            EXT( FILEEXT::KiCadSymbolLibFileExtension )
            EXT( FILEEXT::KiCadPcbFileExtension )
            EXT( FILEEXT::KiCadFootprintFileExtension )
            EXT( FILEEXT::DesignRulesFileExtension )
            EXT( FILEEXT::DrawingSheetFileExtension )
            EXT( FILEEXT::KiCadJobSetFileExtension )
            EXT( FILEEXT::JsonFileExtension )                  // for design blocks
            EXT( FILEEXT::WorkbookFileExtension ) +
            NAME( FILEEXT::FootprintLibraryTableFileName ) +
            NAME( FILEEXT::SymbolLibraryTableFileName ) +
            NAME_NO_PIPE( FILEEXT::DesignBlockLibraryTableFileName );

    // List of additional file extensions that are only archived when aIncludeExtraFiles is true
    if( aIncludeExtraFiles )
    {
        fileExtensionRegex += "|"
            EXT( FILEEXT::LegacyProjectFileExtension )
            EXT( FILEEXT::LegacySchematicFileExtension )
            EXT( FILEEXT::LegacySymbolLibFileExtension )
            EXT( FILEEXT::LegacySymbolDocumentFileExtension )
            EXT( FILEEXT::FootprintAssignmentFileExtension )
            EXT( FILEEXT::LegacyPcbFileExtension )
            EXT( FILEEXT::LegacyFootprintLibPathExtension )
            EXT( FILEEXT::StepFileAbrvExtension )                   // 3d files
            EXT( FILEEXT::StepFileExtension )                       // 3d files
            EXT( FILEEXT::VrmlFileExtension )                       // 3d files
            EXT( FILEEXT::GerberFileExtensionsRegex )               // Gerber files (g?, g??, .gm12 (from protel export))
            EXT( FILEEXT::GerberJobFileExtension )                  // Gerber job files
            EXT( FILEEXT::FootprintPlaceFileExtension )             // Our position files
            EXT( FILEEXT::DrillFileExtension )                      // Fab drill files
            EXT( "nc" )                                             // Fab drill files
            EXT( "xnc" )                                            // Fab drill files
            EXT( FILEEXT::IpcD356FileExtension )
            EXT( FILEEXT::ReportFileExtension )
            EXT( FILEEXT::NetlistFileExtension )
            EXT( FILEEXT::PythonFileExtension )
            EXT( FILEEXT::PdfFileExtension )
            EXT( FILEEXT::TextFileExtension )
            EXT( FILEEXT::SpiceFileExtension )                      // SPICE files
            EXT( FILEEXT::SpiceSubcircuitFileExtension )            // SPICE files
            EXT( FILEEXT::SpiceModelFileExtension )                 // SPICE files
            EXT_NO_PIPE( FILEEXT::IbisFileExtension );
    }

    fileExtensionRegex += ")";

#undef EXT
#undef NAME
#undef EXT_NO_PIPE
#undef NAME_NO_PIPE

    bool     success = true;
    wxString msg;
    wxString oldCwd = wxGetCwd();

    wxFileName sourceDir( aSrcDir );
    KIPLATFORM::IO::LongPathAdjustment( sourceDir );

    wxSetWorkingDirectory( sourceDir.GetFullPath() );

    wxFFileOutputStream ostream( aDestFile );

    if( !ostream.IsOk() )   // issue to create the file. Perhaps not writable dir
    {
        msg.Printf( _( "Failed to create file '%s'." ), aDestFile );
        aReporter.Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    wxZipOutputStream zipstream( ostream, -1, wxConvUTF8 );

    // Build list of filenames to put in zip archive
    wxString currFilename;
    wxArrayString files;
    wxDir projectDir( aSrcDir );

    if ( projectDir.IsOpened() )
    {
        try
        {
            PROJECT_ARCHIVER_DIR_TRAVERSER traverser( fileExtensionRegex, files );
            projectDir.Traverse(traverser);
        }
        catch( const std::regex_error& e )
        {
            // Something bad happened here with the regex
            wxASSERT_MSG( false, e.what() );
            return false;
        }
    }

    for( unsigned ii = 0; ii < files.GetCount(); ++ii )
    {
        if( files[ii].EndsWith( FILEEXT::IbisFileExtension ) )
        {
            wxFileName package( files[ ii ] );
            package.MakeRelativeTo( aSrcDir );
            package.SetExt( wxS( "pkg" ) );
            KIPLATFORM::IO::LongPathAdjustment( package );

            if( package.Exists() )
                files.push_back( package.GetFullName() );
        }
    }

    files.Sort();

    unsigned long uncompressedBytes = 0;

    // Our filename collector can store duplicate filenames. for instance *.gm2
    // matches both *.g?? and *.gm??.
    // So skip duplicate filenames (they are sorted, so it is easy.
    wxString lastStoredFile;

    for( unsigned ii = 0; ii < files.GetCount(); ii++ )
    {
        if( lastStoredFile == files[ii] )   // duplicate name: already stored
            continue;

        lastStoredFile = files[ii];

        wxFileSystem fsfile;

        wxFileName curr_fn( files[ii] );
        KIPLATFORM::IO::LongPathAdjustment( curr_fn );
        curr_fn.MakeRelativeTo( sourceDir.GetFullPath() );

        currFilename = curr_fn.GetFullPath();

        // Read input file and add it to the zip file:
        wxFSFile* infile = fsfile.OpenFile( currFilename );

        if( infile )
        {
            zipstream.PutNextEntry( currFilename, infile->GetModificationTime() );
            infile->GetStream()->Read( zipstream );
            zipstream.CloseEntry();

            uncompressedBytes += infile->GetStream()->GetSize();

            if( aVerbose )
            {
                msg.Printf( _( "Archived file '%s'." ), currFilename );
                aReporter.Report( msg, RPT_SEVERITY_INFO );
            }

            delete infile;
        }
        else
        {
            if( aVerbose )
            {
                msg.Printf( _( "Failed to archive file '%s'." ), currFilename );
                aReporter.Report( msg, RPT_SEVERITY_ERROR );
            }

            success = false;
        }
    }

    auto reportSize =
            []( unsigned long aSize ) -> wxString
            {
                constexpr float KB = 1024.0;
                constexpr float MB = KB * 1024.0;

                if( aSize >= MB )
                    return wxString::Format( wxT( "%0.2f MB" ), aSize / MB );
                else if( aSize >= KB )
                    return wxString::Format( wxT( "%0.2f KB" ), aSize / KB );
                else
                    return wxString::Format( wxT( "%lu bytes" ), aSize );
            };

    size_t zipBytesCnt = ostream.GetSize();

    if( zipstream.Close() )
    {
        msg.Printf( _( "Zip archive '%s' created (%s uncompressed, %s compressed)." ),
                    aDestFile,
                    reportSize( uncompressedBytes ),
                    reportSize( zipBytesCnt ) );
        aReporter.Report( msg, RPT_SEVERITY_INFO );
    }
    else
    {
        msg.Printf( wxT( "Failed to create file '%s'." ), aDestFile );
        aReporter.Report( msg, RPT_SEVERITY_ERROR );
        success = false;
    }

    wxSetWorkingDirectory( oldCwd );
    return success;
}
