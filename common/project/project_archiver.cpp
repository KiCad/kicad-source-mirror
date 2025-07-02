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
#include <wx/regex.h>
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

class PROJECT_ARCHIVER_DIR_ZIP_TRAVERSER : public wxDirTraverser
{
public:
    PROJECT_ARCHIVER_DIR_ZIP_TRAVERSER( const wxString& aPrjDir ) :
        m_prjDir( aPrjDir )
    {}

    virtual wxDirTraverseResult OnFile( const wxString& aFilename ) override
    {
        m_files.emplace_back( aFilename );

        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnDir( const wxString& aDirname ) override
    {
        return wxDIR_CONTINUE;
    }

    const std::vector<wxString>& GetFilesToArchive() const
    {
        return m_files;
    }

private:
    wxString              m_prjDir;
    std::vector<wxString> m_files;
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

    std::set<wxString> extensions;
    std::set<wxString> files;                // File names without extensions such as fp-lib-table.

    extensions.emplace( FILEEXT::ProjectFileExtension );
    extensions.emplace( FILEEXT::ProjectLocalSettingsFileExtension );
    extensions.emplace( FILEEXT::KiCadSchematicFileExtension );
    extensions.emplace( FILEEXT::KiCadSymbolLibFileExtension );
    extensions.emplace( FILEEXT::KiCadPcbFileExtension );
    extensions.emplace( FILEEXT::KiCadFootprintFileExtension );
    extensions.emplace( FILEEXT::DesignRulesFileExtension );
    extensions.emplace( FILEEXT::DrawingSheetFileExtension );
    extensions.emplace( FILEEXT::KiCadJobSetFileExtension );
    extensions.emplace( FILEEXT::JsonFileExtension );                  // for design blocks
    extensions.emplace( FILEEXT::WorkbookFileExtension );

    files.emplace( FILEEXT::FootprintLibraryTableFileName );
    files.emplace( FILEEXT::SymbolLibraryTableFileName );
    files.emplace( FILEEXT::DesignBlockLibraryTableFileName );

    // List of additional file extensions that are only archived when aIncludeExtraFiles is true
    if( aIncludeExtraFiles )
    {
        extensions.emplace( FILEEXT::LegacyProjectFileExtension );
        extensions.emplace( FILEEXT::LegacySchematicFileExtension );
        extensions.emplace( FILEEXT::LegacySymbolLibFileExtension );
        extensions.emplace( FILEEXT::LegacySymbolDocumentFileExtension );
        extensions.emplace( FILEEXT::FootprintAssignmentFileExtension );
        extensions.emplace( FILEEXT::LegacyPcbFileExtension );
        extensions.emplace( FILEEXT::LegacyFootprintLibPathExtension );
        extensions.emplace( FILEEXT::StepFileAbrvExtension );
        extensions.emplace( FILEEXT::StepFileExtension );                       // 3d files
        extensions.emplace( FILEEXT::VrmlFileExtension );                       // 3d files
        extensions.emplace( FILEEXT::GerberJobFileExtension );                  // Gerber job files
        extensions.emplace( FILEEXT::FootprintPlaceFileExtension );             // Our position files
        extensions.emplace( FILEEXT::DrillFileExtension );                      // Fab drill files
        extensions.emplace( "nc" );                                             // Fab drill files
        extensions.emplace( "xnc" );                                            // Fab drill files
        extensions.emplace( FILEEXT::IpcD356FileExtension );
        extensions.emplace( FILEEXT::ReportFileExtension );
        extensions.emplace( FILEEXT::NetlistFileExtension );
        extensions.emplace( FILEEXT::PythonFileExtension );
        extensions.emplace( FILEEXT::PdfFileExtension );
        extensions.emplace( FILEEXT::TextFileExtension );
        extensions.emplace( FILEEXT::SpiceFileExtension );                      // SPICE files
        extensions.emplace( FILEEXT::SpiceSubcircuitFileExtension );            // SPICE files
        extensions.emplace( FILEEXT::SpiceModelFileExtension );                 // SPICE files
        extensions.emplace( FILEEXT::IbisFileExtension );
        extensions.emplace( "pkg" );
        extensions.emplace( FILEEXT::GencadFileExtension );
    }

    // Gerber files (g?, g??, .gm12 (from protel export)).
    wxRegEx gerberFiles( FILEEXT::GerberFileExtensionsRegex );
    wxASSERT( gerberFiles.IsValid() );

    bool     success = true;
    wxString msg;
    wxString oldCwd = wxGetCwd();

    wxFileName sourceDir( aSrcDir, wxEmptyString, wxEmptyString );

    wxSetWorkingDirectory( aSrcDir );

    wxFFileOutputStream ostream( aDestFile );

    if( !ostream.IsOk() )   // issue to create the file. Perhaps not writable dir
    {
        msg.Printf( _( "Failed to create file '%s'." ), aDestFile );
        aReporter.Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    wxZipOutputStream zipstream( ostream, -1, wxConvUTF8 );

    wxDir projectDir( aSrcDir );

    if( !projectDir.IsOpened() )
    {
        if( aVerbose )
        {
            msg.Printf( _( "Error opening directory: '%s'." ), aSrcDir );
            aReporter.Report( msg, RPT_SEVERITY_ERROR );
        }

        wxSetWorkingDirectory( oldCwd );
        return false;
    }

    size_t uncompressedBytes = 0;
    PROJECT_ARCHIVER_DIR_ZIP_TRAVERSER traverser( aSrcDir );

    projectDir.Traverse( traverser );

    for( const wxString& fileName : traverser.GetFilesToArchive() )
    {
        wxFileName fn( fileName );
        wxString extLower = fn.GetExt().Lower();
        wxString fileNameLower = fn.GetName().Lower();
        bool archive = false;

        if( !extLower.IsEmpty() )
        {
            if( ( extensions.find( extLower ) != extensions.end() )
              || ( aIncludeExtraFiles && gerberFiles.Matches( extLower ) ) )
                archive = true;
        }
        else if( !fileNameLower.IsEmpty() && ( files.find( fileNameLower ) != files.end() ) )
        {
                archive = true;
        }

        if( !archive )
            continue;

        wxFileSystem fsFile;
        fn.MakeRelativeTo( aSrcDir );

        wxString relativeFn = fn.GetFullPath();

        // Read input file and add it to the zip file:
        wxFSFile* infile = fsFile.OpenFile( relativeFn );

        if( infile )
        {
            zipstream.PutNextEntry( relativeFn, infile->GetModificationTime() );
            infile->GetStream()->Read( zipstream );
            zipstream.CloseEntry();

            uncompressedBytes += infile->GetStream()->GetSize();

            if( aVerbose )
            {
                msg.Printf( _( "Archived file '%s'." ), relativeFn );
                aReporter.Report( msg, RPT_SEVERITY_INFO );
            }

            delete infile;
        }
        else
        {
            if( aVerbose )
            {
                msg.Printf( _( "Failed to archive file '%s'." ), relativeFn );
                aReporter.Report( msg, RPT_SEVERITY_ERROR );
            }
        }
    }

    auto reportSize =
            []( size_t aSize ) -> wxString
            {
                constexpr float KB = 1024.0;
                constexpr float MB = KB * 1024.0;

                if( aSize >= MB )
                    return wxString::Format( wxT( "%0.2f MB" ), aSize / MB );
                else if( aSize >= KB )
                    return wxString::Format( wxT( "%0.2f KB" ), aSize / KB );
                else
                    return wxString::Format( wxT( "%zu bytes" ), aSize );
            };

    size_t        zipBytesCnt       = ostream.GetSize();

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
