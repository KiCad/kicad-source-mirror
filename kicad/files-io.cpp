/****************/
/* files-io.cpp */
/****************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include <wx/fs_zip.h>
#include <wx/zipstrm.h>
#include <wx/docview.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <wx/dir.h>

#include "confirm.h"
#include "gestfich.h"
#include "macros.h"

#include "kicad.h"
#include "prjconfig.h"

static const wxString ZipFileExtension( wxT( "zip" ) );
static const wxString ZipFileWildcard( wxT( "Zip file (*.zip) | *.zip" ) );


void KICAD_MANAGER_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Kicad project file" ) );

    if( fn != wxEmptyString )
    {
        wxCommandEvent cmd( 0, wxID_ANY );
        m_ProjectFileName = fn;
        OnLoadProject( cmd );
    }
}

void KICAD_MANAGER_FRAME::OnUnarchiveFiles( wxCommandEvent& event )
{
    wxFileName fn = m_ProjectFileName;
    fn.SetExt( ZipFileExtension );

    wxFileDialog dlg( this, _( "Unzip Project" ), fn.GetPath(),
                      fn.GetFullName(), ZipFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    PrintMsg( _( "\nOpen " ) + dlg.GetPath() + wxT( "\n" ) );

    wxDirDialog dirDlg( this, _( "Target Directory" ), fn.GetPath(),
                        wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

    if( dirDlg.ShowModal() == wxID_CANCEL )
        return;

    wxSetWorkingDirectory( dirDlg.GetPath() );
    PrintMsg( _( "Unzipping project in " ) + dirDlg.GetPath() + wxT( "\n" ) );

    wxFileSystem zipfilesys;
    zipfilesys.AddHandler( new wxZipFSHandler );
    zipfilesys.ChangePathTo( dlg.GetPath() + wxT( "#zip:" ) );

    wxFSFile* zipfile = NULL;
    wxString  localfilename = zipfilesys.FindFirst( wxT( "*.*" ) );

    while( !localfilename.IsEmpty() )
    {
        zipfile = zipfilesys.OpenFile( localfilename );
        if( zipfile == NULL )
        {
            DisplayError( this, wxT( "Zip file read error" ) );
            break;
        }

        wxString unzipfilename = localfilename.AfterLast( ':' );

        PrintMsg( _( "Extract file " ) + unzipfilename );

        wxInputStream*       stream = zipfile->GetStream();

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

    wxSetWorkingDirectory( fn.GetPath() );
}


void KICAD_MANAGER_FRAME::OnArchiveFiles( wxCommandEvent& event )
{
    /* List of file extensions to save. */
    static const wxChar* extentionList[] = {
        wxT( "*.sch" ), wxT( "*.lib" ), wxT( "*.cmp" ), wxT( "*.brd" ),
        wxT( "*.net" ), wxT( "*.pro" ), wxT( "*.pho" ), wxT( "*.py" ),
        wxT( "*.pdf" ), wxT( "*.txt" ), wxT( "*.dcm" ),
        NULL
    };

    wxString msg;
    size_t i;
    wxFileName fileName = m_ProjectFileName;
    wxString oldPath = wxGetCwd();

    fileName.SetExt( wxT( "zip" ) );

    wxFileDialog dlg( this, _( "Archive Project Files" ),
                      fileName.GetPath(), fileName.GetFullName(),
                      ZipFileWildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;


    wxFileName zip = dlg.GetPath();

    wxString currdirname = wxT( "." );
    currdirname += zip.GetPathSeparator();
    wxDir dir( currdirname );

    if( !dir.IsOpened() )
        return;

    // Prepare the zip file
    wxString zipfilename = zip.GetFullPath();

    wxFFileOutputStream ostream(zipfilename);
    wxZipOutputStream zipstream( ostream );

    // Build list of filenames to put in zip archive
    wxString currFilename;
    int zipBytesCnt = 0;    // Size of the zip file
    for( i = 0; extentionList[i] != 0; i++ )
    {
        bool cont = dir.GetFirst( &currFilename, extentionList[i] );

        while( cont )
        {
            wxFileSystem fsfile;
            PrintMsg( _( "Archive file " ) + currFilename );
            // Read input file and put it in zip file:
            wxFSFile * infile = fsfile.OpenFile(currFilename);
            if( infile )
            {
                zipstream.PutNextEntry( currFilename, infile->GetModificationTime() );
                infile->GetStream()->Read( zipstream );
                zipstream.CloseEntry();
                int zippedsize = zipstream.GetSize() - zipBytesCnt;
                zipBytesCnt = zipstream.GetSize();
                PrintMsg( wxT("  ") );
                msg.Printf( _( "(%d bytes, compressed %d bytes)\n"),
                            infile->GetStream()->GetSize(), zippedsize );
                PrintMsg( msg );
                delete infile;
             }
            else
            {
                PrintMsg( _(" >>Error\n") );
            }

            cont = dir.GetNext( &currFilename );
        }
    }

    zipBytesCnt = ostream.GetSize();
    if( zipstream.Close() )
    {
        msg.Printf( _("\nZip archive <%s> created (%d bytes)" ),
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

    wxSetWorkingDirectory( oldPath );
}
