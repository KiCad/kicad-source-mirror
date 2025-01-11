/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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


#define ZipFileExtension wxT( "zip" )


PROJECT_ARCHIVER::PROJECT_ARCHIVER()
{
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
    // List of file extensions that are always archived
    static const wxChar* extensionList[] = {
            wxT( "*.kicad_pro" ),
            wxT( "*.kicad_prl" ),
            wxT( "*.kicad_sch" ),
            wxT( "*.kicad_sym" ),
            wxT( "*.kicad_pcb" ),
            wxT( "*.kicad_mod" ),
            wxT( "*.kicad_dru" ),
            wxT( "*.kicad_wks" ),
            wxT( "*.wbk" ),
            wxT( "fp-lib-table" ),
            wxT( "sym-lib-table" )
        };

    // List of additional file extensions that are only archived when aIncludeExtraFiles is true
    static const wxChar* extraExtensionList[] = {
            wxT( "*.pro" ),                         // Legacy project files
            wxT( "*.sch" ),                         // Legacy schematic files
            wxT( "*.lib" ), wxT( "*.dcm" ),         // Legacy schematic library files
            wxT( "*.cmp" ),
            wxT( "*.brd" ),                         // Legacy PCB files
            wxT( "*.mod" ),                         // Legacy footprint library files
            wxT( "*.stp" ), wxT( "*.step" ),        // 3d files
            wxT( "*.wrl" ),
            wxT( "*.g?" ), wxT( "*.g??" ),          // Gerber files
            wxT( "*.gm??"),                         // Some gerbers like .gm12 (from protel export)
            wxT( "*.gbrjob" ),                      // Gerber job files
            wxT( "*.pos" ),                         // our position files
            wxT( "*.drl" ), wxT( "*.nc" ), wxT( "*.xnc" ),  // Fab drill files
            wxT( "*.d356" ),
            wxT( "*.rpt" ),
            wxT( "*.net" ),
            wxT( "*.py" ),
            wxT( "*.pdf" ),
            wxT( "*.txt" ),
            wxT( "*.cir" ), wxT( "*.sub" ), wxT( "*.model" ),    // SPICE files
            wxT( "*.ibs" )
        };

    bool     success = true;
    wxString msg;
    wxString oldCwd = wxGetCwd();

    wxSetWorkingDirectory( aSrcDir );

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

    for( unsigned ii = 0; ii < arrayDim( extensionList ); ii++ )
        wxDir::GetAllFiles( aSrcDir, &files, extensionList[ii], wxDIR_FILES | wxDIR_DIRS );

    if( aIncludeExtraFiles )
    {
        for( unsigned ii = 0; ii < arrayDim( extraExtensionList ); ii++ )
            wxDir::GetAllFiles( aSrcDir, &files, extraExtensionList[ii], wxDIR_FILES | wxDIR_DIRS );
    }

    for( unsigned ii = 0; ii < files.GetCount(); ++ii )
    {
        if( files[ii].EndsWith( wxS( ".ibs" ) ) )
        {
            wxFileName package( files[ ii ] );
            package.MakeRelativeTo( aSrcDir );
            package.SetExt( wxS( "pkg" ) );

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
        curr_fn.MakeRelativeTo( aSrcDir );
        currFilename = curr_fn.GetFullPath();

        // Read input file and add it to the zip file:
        wxFSFile* infile = fsfile.OpenFile( wxFileSystem::FileNameToURL( curr_fn ) );

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
