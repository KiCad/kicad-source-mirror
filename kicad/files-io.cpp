/****************/
/* files-io.cpp */
/****************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include <wx/fs_zip.h>
#include <wx/docview.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <wx/dir.h>

#include "common.h"
#include "bitmaps.h"
#include "confirm.h"
#include "gestfich.h"
#include "macros.h"

#include "kicad.h"
#include "prjconfig.h"

static const wxString ZipFileExtension( wxT( "zip" ) );
static const wxString ZipFileWildcard( wxT( "Zip file (*.zip) | *.zip" ) );


void WinEDA_MainFrame::OnFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( fn != wxEmptyString )
    {
        wxCommandEvent cmd( 0, wxID_ANY );
        m_ProjectFileName = fn;
        OnLoadProject( cmd );
    }

    ReCreateMenuBar();
}

void WinEDA_MainFrame::OnUnarchiveFiles( wxCommandEvent& event )
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


void WinEDA_MainFrame::OnArchiveFiles( wxCommandEvent& event )
{
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

    /* List of file extensions to save. */
    static const wxChar* extList[] = {
        wxT( "*.sch" ), wxT( "*.lib" ), wxT( "*.cmp" ), wxT( "*.brd" ),
        wxT( "*.net" ), wxT( "*.pro" ), wxT( "*.pho" ), wxT( "*.py" ),
        wxT( "*.pdf" ), wxT( "*.txt" ), wxT( "*.dcm" ),
        NULL
    };

    wxString cmd = wxT( "-o " );    // run minizip with option -o (overwrite)
    cmd += QuoteFullPath(zip);

    wxString currdirname = wxT( "." );
    currdirname += zip.GetPathSeparator();
    wxDir dir( currdirname );

    if( !dir.IsOpened() )
        return;

    wxString f;

    for( i = 0; extList[i] != 0; i++ )
    {
        bool cont = dir.GetFirst( &f, extList[i] );

        while( cont )
        {
            wxFileName fn( f );
            wxString filename = QuoteFullPath(fn);
            cmd += wxT( " " ) + filename;
            PrintMsg( _( "Archive file " ) + filename + wxT( "\n" ) );
            cont = dir.GetNext( &f );
        }
    }

#ifdef __WINDOWS__
#define ZIPPER wxT( "minizip.exe" )
#else
#define ZIPPER wxT( "minizip" )
#endif
    if( ExecuteFile( this, ZIPPER, cmd ) >= 0 )
    {
        wxString msg;
        wxString filename = QuoteFullPath(zip);
        msg.Printf( _("\nZip archive <%s> created" ), GetChars( filename ) );
        PrintMsg( msg );
        PrintMsg( wxT( "\n** end **\n" ) );
    }
    else
        PrintMsg( wxT( "Minizip command error, abort\n" ) );

    wxSetWorkingDirectory( oldPath );
}
