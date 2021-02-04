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


#define ZipFileExtension wxT( "zip" )


PROJECT_ARCHIVER::PROJECT_ARCHIVER()
{
}


// Unarchive Files code comes from wxWidgets sample/archive/archive.cpp
static bool CopyStreamData( wxInputStream& inputStream, wxOutputStream& outputStream,
                            wxFileOffset size )
{
    wxChar buf[128 * 1024];
    int readSize = 128 * 1024;
    wxFileOffset copiedData = 0;

    for( ; ; )
    {
        if(size != -1 && copiedData + readSize > size )
            readSize = size - copiedData;

        inputStream.Read( buf, readSize );

        size_t actuallyRead = inputStream.LastRead();
        outputStream.Write( buf, actuallyRead );

        if( outputStream.LastWrite() != actuallyRead )
        {
            wxLogError( "Failed to output data" );
            //return false;
        }

        if( size == -1 )
        {
            if( inputStream.Eof() )
                break;
        }
        else
        {
            copiedData += actuallyRead;
            if( copiedData >= size )
                break;
        }
    }

    return true;
}


bool PROJECT_ARCHIVER::Unarchive( const wxString& aSrcFile, const wxString& aDestDir,
                                  REPORTER& aReporter )
{
    wxFFileInputStream stream( aSrcFile );

    if( !stream.IsOk() )
    {
        aReporter.Report( _( "Could not open archive file\n" ), RPT_SEVERITY_ERROR );
        return false;
    }

    const wxArchiveClassFactory* archiveClassFactory =
            wxArchiveClassFactory::Find( aSrcFile, wxSTREAM_FILEEXT );

    if( !archiveClassFactory )
    {
        aReporter.Report( _( "Invalid archive file format\n" ), RPT_SEVERITY_ERROR );
        return false;
    }

    wxScopedPtr<wxArchiveInputStream> archiveStream( archiveClassFactory->NewStream( stream ) );

    wxString fileStatus;

    for( wxArchiveEntry* entry = archiveStream->GetNextEntry(); entry;
         entry = archiveStream->GetNextEntry() )
    {
        fileStatus.Printf( _( "Extracting file \"%s\"\n" ), entry->GetName() );
        aReporter.Report( fileStatus, RPT_SEVERITY_INFO );

        wxString fullname = aDestDir + entry->GetName();

        // Ensure the target directory exists and created it if not
        wxString t_path = wxPathOnly( fullname );

        if( !wxDirExists( t_path ) )
        {
            // To create t_path, we need to create all subdirs from unzipDir
            // to t_path.
            wxFileName pathToCreate;
            pathToCreate.AssignDir( t_path );
            pathToCreate.MakeRelativeTo( aDestDir );

            // Create the list of subdirs candidates
            wxArrayString subDirs;
            subDirs = pathToCreate.GetDirs();
            pathToCreate.AssignDir( aDestDir );

            for( size_t ii = 0; ii < subDirs.Count(); ii++ )
            {
                pathToCreate.AppendDir( subDirs[ii] );
                wxString currPath = pathToCreate.GetPath();

                if( !wxDirExists( currPath ) )
                    wxMkdir( currPath );
            }
        }

        wxTempFileOutputStream outputFileStream( fullname );

        if( CopyStreamData( *archiveStream, outputFileStream, entry->GetSize() ) )
            outputFileStream.Commit();
        else
            aReporter.Report( _( "Error extracting file!\n" ), RPT_SEVERITY_ERROR );
    }

    aReporter.Report( wxT( "Extracted project\n" ), RPT_SEVERITY_INFO );
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
            wxT( "fp-lib-table" ),
            wxT( "sym-lib-table" )
        };

    // List of additional file extensions that are only archived when aIncludeExtraFiles is true
    static const wxChar* extraExtensionList[] = {
            wxT( "*.pro" ),
            wxT( "*.sch" ),                         // Legacy schematic files
            wxT( "*.lib" ), wxT( "*.dcm" ),         // Legacy schematic library files
            wxT( "*.cmp" ),
            wxT( "*.brd" ),
            wxT( "*.mod" ),
            wxT( "*.stp" ), wxT( "*.step" ),        // 3d files
            wxT( "*.wrl" ),
            wxT( "*.gb?" ), wxT( "*.gbrjob" ),      // Gerber files
            wxT( "*.gko" ), wxT( "*.gm1" ),
            wxT( "*.gm2" ), wxT( "*.g?" ),
            wxT( "*.gp1" ), wxT( "*.gp2" ),
            wxT( "*.gpb" ), wxT( "*.gpt" ),
            wxT( "*.gt?" ),
            wxT( "*.pos" ), wxT( "*.drl" ), wxT( "*.nc" ), wxT( "*.xnc" ),  // Fab files
            wxT( "*.d356" ), wxT( "*.rpt" ),
            wxT( "*.net" ), wxT( "*.py" ),
            wxT( "*.pdf" ), wxT( "*.txt" )
        };

    bool     success = true;
    wxString msg;
    wxString oldCwd = wxGetCwd();

    wxSetWorkingDirectory( aSrcDir );

    wxFFileOutputStream ostream( aDestFile );

    if( !ostream.IsOk() )   // issue to create the file. Perhaps not writable dir
    {
        msg.Printf( _( "Unable to create archive file \"%s\"\n" ), aDestFile );
        aReporter.Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    wxZipOutputStream zipstream( ostream, -1, wxConvUTF8 );

    // Build list of filenames to put in zip archive
    wxString currFilename;

    wxArrayString files;

    for( unsigned ii = 0; ii < arrayDim( extensionList ); ii++ )
        wxDir::GetAllFiles( aSrcDir, &files, extensionList[ii] );

    if( aIncludeExtraFiles )
    {
        for( unsigned ii = 0; ii < arrayDim( extraExtensionList ); ii++ )
            wxDir::GetAllFiles( aSrcDir, &files, extraExtensionList[ii] );
    }

    files.Sort();

    unsigned long uncompressedBytes = 0;

    for( unsigned ii = 0; ii < files.GetCount(); ii++ )
    {
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
                msg.Printf( _( "Archive file \"%s\"\n" ), currFilename );
                aReporter.Report( msg, RPT_SEVERITY_INFO );
            }

            delete infile;
        }
        else
        {
            if( aVerbose )
            {
                msg.Printf( _( "Archive file \"%s\": Failed!\n" ), currFilename );
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
        msg.Printf( _( "Zip archive \"%s\" created (%s uncompressed, %s compressed)\n" ), aDestFile,
                    reportSize( uncompressedBytes ), reportSize( zipBytesCnt ) );
        aReporter.Report( msg, RPT_SEVERITY_INFO );
    }
    else
    {
        msg.Printf( wxT( "Unable to create archive \"%s\"\n" ), aDestFile );
        aReporter.Report( msg, RPT_SEVERITY_ERROR );
        success = false;
    }

    wxSetWorkingDirectory( oldCwd );
    return success;
}
