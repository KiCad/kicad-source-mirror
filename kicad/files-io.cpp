/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file kicad/files-io.cpp
 */


#include <wx/dir.h>
#include <wx/fs_zip.h>
#include <wx/uri.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <confirm.h>
#include <kiway.h>
#include "pgm_kicad.h"
#include "wildcards_and_files_ext.h"

#include "kicad_manager_frame.h"


#define ZipFileExtension wxT( "zip" )


void KICAD_MANAGER_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxFileName projFileName = GetFileFromHistory( event.GetId(), _( "KiCad project file" ) );
    if( !projFileName.FileExists() )
        return;

    LoadProject( projFileName );
}


void KICAD_MANAGER_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


// Unarchive Files code comes from wxWidgets sample/archive/archive.cpp
static bool CopyStreamData( wxInputStream& inputStream, wxOutputStream& outputStream, wxFileOffset size )
{
    wxChar buf[128 * 1024];
    int readSize = 128 * 1024;
    wxFileOffset copiedData = 0;

    for (;;)
    {
        if (size != -1 && copiedData + readSize > size)
            readSize = size - copiedData;

        inputStream.Read( buf, readSize );

        size_t actuallyRead = inputStream.LastRead();
        outputStream.Write( buf, actuallyRead );

        if( outputStream.LastWrite() != actuallyRead )
        {
            wxLogError( "Failed to output data" );
            //return false;
        }

        if (size == -1)
        {
            if (inputStream.Eof())
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

void KICAD_MANAGER_FRAME::OnUnarchiveFiles( wxCommandEvent& event )
{
    wxFileName fn = GetProjectFileName();

    fn.SetExt( ZipFileExtension );

    wxFileDialog zipfiledlg( this, _( "Unzip Project" ), fn.GetPath(),
                             fn.GetFullName(), ZipFileWildcard(),
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( zipfiledlg.ShowModal() == wxID_CANCEL )
        return;

    wxString msg = wxString::Format( _( "\nOpen \"%s\"\n" ), zipfiledlg.GetPath() );
    PrintMsg( msg );

    wxDirDialog dirDlg( this, _( "Target Directory" ), fn.GetPath(), wxDD_DEFAULT_STYLE );

    if( dirDlg.ShowModal() == wxID_CANCEL )
        return;

    wxString unzipDir = dirDlg.GetPath() + wxT( "/" );
    msg.Printf( _( "Unzipping project in \"%s\"\n" ), unzipDir );
    PrintMsg( msg );

    wxString archiveFileName = zipfiledlg.GetPath();
    wxFileInputStream fileInputStream( archiveFileName );

    if (!fileInputStream.IsOk())
        return;

    const wxArchiveClassFactory* archiveClassFactory =
        wxArchiveClassFactory::Find( archiveFileName, wxSTREAM_FILEEXT );

    if( archiveClassFactory )
    {
        wxScopedPtr<wxArchiveInputStream> archiveStream( archiveClassFactory->NewStream(fileInputStream) );

        for (wxArchiveEntry* entry = archiveStream->GetNextEntry(); entry;
             entry = archiveStream->GetNextEntry())
        {
            PrintMsg( wxString::Format( _( "Extract file \"%s\"" ), entry->GetName() ) );

            wxString fullname = unzipDir + entry->GetName();

            // Ensure the target directory exists and created it if not
            wxString t_path = wxPathOnly( fullname );

            if( !wxDirExists( t_path ) )
            {
                // To create t_path, we need to create all subdirs from unzipDir
                // to t_path.
                wxFileName pathToCreate;
                pathToCreate.AssignDir( t_path );
                pathToCreate.MakeRelativeTo( unzipDir );

                // Create the list of subdirs candidates
                wxArrayString subDirs;
                subDirs = pathToCreate.GetDirs();
                pathToCreate.AssignDir( unzipDir );

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
            {
                outputFileStream.Commit();
                PrintMsg( _( " OK\n" ) );
            }
            else
                PrintMsg( _( " *ERROR*\n" ) );
        }
    }

    PrintMsg( wxT( "** end **\n" ) );

    if( unzipDir == Prj().GetProjectPath() )
        RefreshProjectTree();
}


void KICAD_MANAGER_FRAME::OnArchiveFiles( wxCommandEvent& event )
{
    // List of file extensions to save.
    static const wxChar* extensionList[] = {
        wxT( "*.pro" ),
        wxT( "*.kicad_pro" ),
        wxT( "*.kicad_prl" ),
        wxT( "*.sch" ),                         // Legacy schematic files
        wxT( "*.kicad_sch" ),                   // Schematic files
        wxT( "*.lib" ), wxT( "*.dcm" ),         // Legacy schematic library files
        wxT( "*.kicad_sym" ),                   // schematic library files
        wxT( "*.cmp" ),
        wxT( "*.brd" ), wxT( "*.kicad_pcb" ),   // Brd files
        wxT( "*.mod" ), wxT( "*.kicad_mod" ),   // fp files
        wxT( "*.gb?" ), wxT( "*.gbrjob" ),      // Gerber files
        wxT( "*.gko" ), wxT( "*.gm1" ),
        wxT( "*.gm2" ), wxT( "*.g?" ),
        wxT( "*.gp1" ), wxT( "*.gp2" ),
        wxT( "*.gpb" ), wxT( "*.gpt" ),
        wxT( "*.gt?" ),
        wxT( "*.pos" ), wxT( "*.drl" ), wxT( "*.nc" ), wxT( "*.xnc" ),  // Fab files
        wxT( "*.d356" ), wxT( "*.rpt" ),
        wxT( "*.stp" ), wxT( "*.step" ),        // 3d files
        wxT( "*.wrl" ),
        wxT( "*.net" ), wxT( "*.py" ),
        wxT( "*.pdf" ), wxT( "*.txt" ), wxT( "*.kicad_wks" ),
        wxT( "fp-lib-table" ), wxT( "sym-lib-table" )
    };

    wxString    msg;
    wxFileName  fileName = GetProjectFileName();
    wxString    oldCwd = wxGetCwd();

    fileName.SetExt( wxT( "zip" ) );

    wxFileDialog dlg( this, _( "Archive Project Files" ),
                      fileName.GetPath(), fileName.GetFullName(),
                      ZipFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName zip = dlg.GetPath();

    wxString currdirname = fileName.GetPathWithSep();
    wxDir dir( currdirname );

    if( !dir.IsOpened() )   // wxWidgets display a error message on issue.
        return;

    wxSetWorkingDirectory( currdirname );

    // Prepare the zip file
    wxString zipfilename = zip.GetFullPath();

    wxFFileOutputStream ostream( zipfilename );

    if( !ostream.IsOk() )   // issue to create the file. Perhaps not writable dir
    {
        wxMessageBox( wxString::Format( _( "Unable to create zip archive file \"%s\"" ),
                                        zipfilename ) );
        return;
    }

    wxZipOutputStream zipstream( ostream );

    // Build list of filenames to put in zip archive
    wxString currFilename;

	wxArrayString files;

    for( unsigned ii = 0; ii < arrayDim( extensionList ); ii++ )
        wxDir::GetAllFiles( currdirname, &files, extensionList[ii] );

    files.Sort();

    int zipBytesCnt = 0;

    for( unsigned ii = 0; ii < files.GetCount(); ii++ )
    {
        wxFileSystem fsfile;

        wxFileName curr_fn( files[ii] );
        curr_fn.MakeRelativeTo( currdirname );
        currFilename = curr_fn.GetFullPath();

        msg.Printf( _( "Archive file \"%s\"" ), currFilename );
        PrintMsg( msg );

        // Read input file and add it to the zip file:
        wxFSFile* infile = fsfile.OpenFile( currFilename );

        if( infile )
        {
            zipstream.PutNextEntry( currFilename, infile->GetModificationTime() );
            infile->GetStream()->Read( zipstream );
            zipstream.CloseEntry();
            int zippedsize = zipstream.GetSize() - zipBytesCnt;
            zipBytesCnt = zipstream.GetSize();
            PrintMsg( wxT("  ") );
            msg.Printf( _( "(%lu bytes, compressed %d bytes)\n" ),
                        (unsigned long)infile->GetStream()->GetSize(), zippedsize );
            PrintMsg( msg );
            delete infile;
        }
        else
            PrintMsg( _( " >>Error\n" ) );
    }

    zipBytesCnt = ostream.GetSize();

    if( zipstream.Close() )
    {
        msg.Printf( _( "\nZip archive \"%s\" created (%d bytes)" ),
                    zipfilename, zipBytesCnt );
        PrintMsg( msg );
        PrintMsg( wxT( "\n** end **\n" ) );
    }
    else
    {
        msg.Printf( wxT( "Unable to create archive \"%s\", abort\n" ),
                    zipfilename );
        PrintMsg( msg );
    }

    wxSetWorkingDirectory( oldCwd );
}
