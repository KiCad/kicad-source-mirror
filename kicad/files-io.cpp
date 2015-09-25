/**
 * @file kicad/files-io.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras
 * Copyright (C) 2004-2015 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <pgm_kicad.h>
#include <kiway.h>
#include <wx/fs_zip.h>
#include <wx/zipstrm.h>
#include <wx/docview.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <wx/dir.h>

#include <confirm.h>
#include <gestfich.h>
#include <macros.h>

#include <kicad.h>

#define     ZipFileExtension    wxT( "zip" )
#define     ZipFileWildcard     _( "Zip file (*.zip)|*.zip" )


void KICAD_MANAGER_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString fn = GetFileFromHistory( event.GetId(),
                    _( "KiCad project file" ), &Pgm().GetFileHistory() );

    if( fn.size() )
    {
        // Any open KIFACE's must be closed before changing the project.
        // We never want a KIWAY_PLAYER open on a KIWAY that isn't in the same project,
        // and then break one project.
        // Remember when saving files, the full path is build from the current project path
        // User is prompted here to close those KIWAY_PLAYERs:
        if( !Kiway.PlayersClose( false ) )
            return;

        // We can now set the new project filename and load this project
        SetProjectFileName( fn );
        wxCommandEvent cmd( 0, wxID_ANY );
        OnLoadProject( cmd );
    }
}


void KICAD_MANAGER_FRAME::OnUnarchiveFiles( wxCommandEvent& event )
{
    wxFileName fn = GetProjectFileName();

    fn.SetExt( ZipFileExtension );

    wxFileDialog zipfiledlg( this, _( "Unzip Project" ), fn.GetPath(),
                      fn.GetFullName(), ZipFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( zipfiledlg.ShowModal() == wxID_CANCEL )
        return;

    wxString msg = wxString::Format( _("\nOpen '%s'\n" ), GetChars( zipfiledlg.GetPath() ) );
    PrintMsg( msg );

    wxDirDialog dirDlg( this, _( "Target Directory" ), fn.GetPath(),
                        wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

    if( dirDlg.ShowModal() == wxID_CANCEL )
        return;

    wxString unzipDir = dirDlg.GetPath() + wxT( "/" );
    msg.Printf( _( "Unzipping project in '%s'\n" ), GetChars( unzipDir ) );
    PrintMsg( msg );

    wxFileSystem zipfilesys;

    zipfilesys.AddHandler( new wxZipFSHandler );
    zipfilesys.ChangePathTo( zipfiledlg.GetPath() + wxT( "#zip:" ), true );

    wxFSFile* zipfile = NULL;
    wxString  localfilename = zipfilesys.FindFirst( wxT( "*.*" ) );

    while( !localfilename.IsEmpty() )
    {
        zipfile = zipfilesys.OpenFile( localfilename );
        if( !zipfile )
        {
            DisplayError( this, wxT( "Zip file read error" ) );
            break;
        }

        wxFileName uzfn = localfilename.AfterLast( ':' );
        uzfn.MakeAbsolute( unzipDir );
        wxString unzipfilename = uzfn.GetFullPath();

        msg.Printf( _( "Extract file '%s'" ), GetChars( unzipfilename ) );
        PrintMsg( msg );

        wxInputStream* stream = zipfile->GetStream();
        wxFFileOutputStream* ofile = new wxFFileOutputStream( unzipfilename );

        if( ofile->Ok() )
        {
            ofile->Write( *stream );
            PrintMsg( _( " OK\n" ) );
        }
        else
            PrintMsg( _( " *ERROR*\n" ) );

        delete ofile;
        delete zipfile;

        localfilename = zipfilesys.FindNext();
    }

    PrintMsg( wxT( "** end **\n" ) );
}


void KICAD_MANAGER_FRAME::OnArchiveFiles( wxCommandEvent& event )
{
    // List of file extensions to save.
    static const wxChar* extentionList[] = {
        wxT( "*.sch" ), wxT( "*.lib" ), wxT( "*.mod" ), wxT( "*.cmp" ),
        wxT( "*.brd" ), wxT( "*.kicad_pcb" ), wxT( "*.gbr" ), wxT( "*.pos" ),
        wxT( "*.net" ), wxT( "*.pro" ), wxT( "*.drl" ), wxT( "*.py" ),
        wxT( "*.pdf" ), wxT( "*.txt" ), wxT( "*.dcm" ), wxT( "*.kicad_wks" ),
    };

    wxString    msg;
    wxFileName  fileName = GetProjectFileName();
    wxString    oldCwd = wxGetCwd();

    fileName.SetExt( wxT( "zip" ) );

    wxFileDialog dlg( this, _( "Archive Project Files" ),
                      fileName.GetPath(), fileName.GetFullName(),
                      ZipFileWildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName zip = dlg.GetPath();

    wxString currdirname = fileName.GetPathWithSep();
    wxDir dir( currdirname );

    if( !dir.IsOpened() )
        return;

    wxSetWorkingDirectory( currdirname );

    // Prepare the zip file
    wxString zipfilename = zip.GetFullPath();

    wxFFileOutputStream ostream( zipfilename );
    wxZipOutputStream zipstream( ostream );

    // Build list of filenames to put in zip archive
    wxString currFilename;

	wxArrayString files;

    for( unsigned ii = 0; ii < DIM( extentionList ); ii++ )
        wxDir::GetAllFiles( currdirname, &files, extentionList[ii] );

    files.Sort();

    int zipBytesCnt = 0;

    for( unsigned ii = 0; ii < files.GetCount(); ii++ )
    {
        wxFileSystem fsfile;

        wxFileName curr_fn( files[ii] );
        curr_fn.MakeRelativeTo( currdirname );
        currFilename = curr_fn.GetFullPath();

        msg.Printf( _( "Archive file <%s>" ), GetChars( currFilename ) );
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
        msg.Printf( _( "\nZip archive <%s> created (%d bytes)" ),
                    GetChars( zipfilename ), zipBytesCnt );
        PrintMsg( msg );
        PrintMsg( wxT( "\n** end **\n" ) );
    }
    else
    {
        msg.Printf( wxT( "Unable to create archive <%s>, abort\n" ),
                    GetChars( zipfilename ) );
        PrintMsg( msg );
    }

    wxSetWorkingDirectory( oldCwd );
}
