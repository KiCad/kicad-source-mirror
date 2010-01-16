/*******************/
/* File: cvpcb.cpp */
/*******************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "cvpcb.h"
#include "zones.h"
#include "bitmaps.h"
#include "protos.h"
#include "cvstruct.h"

#include "build_version.h"

#include <wx/snglinst.h>


/* Constant string definitions for CvPcb */
const wxString ComponentFileExtension( wxT( "cmp" ) );
const wxString RetroFileExtension( wxT( "stf" ) );
const wxString FootprintAliasFileExtension( wxT( "equ" ) );

// Wildcard for schematic retroannotation (import footprint names in schematic):
const wxString RetroFileWildcard( _( "Kicad retroannotation files (*.stf)|*.stf" ) );
const wxString FootprintAliasFileWildcard( _( "Kicad footprint alias files (*.equ)|*.equ" ) );

const wxString titleLibLoadError( _( "Library Load Error" ) );


/* MacOSX: Needed for file association 
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void WinEDA_App::MacOpenFile(const wxString &fileName) {
    wxFileName    fn = fileName;
    wxString      oldPath;
    WinEDA_CvpcbFrame * frame = ((WinEDA_CvpcbFrame*)GetTopWindow());

    if( frame->m_NetlistFileName.DirExists() )
        oldPath = frame->m_NetlistFileName.GetPath();

    /* Update the library search path list. */
    if( wxGetApp().GetLibraryPathList().Index( oldPath ) != wxNOT_FOUND )
        wxGetApp().GetLibraryPathList().Remove( oldPath );
    wxGetApp().GetLibraryPathList().Insert( fn.GetPath(), 0 );

    frame->m_NetlistFileName = fn;

    if( frame->ReadNetList() )
    {
        frame->SetLastProject( fn.GetFullPath() );

        frame->SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
                  wxT( " " ) + fn.GetFullPath() );
    }
    else
    {
        frame->SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() );
    }

    frame->ReCreateMenuBar();

}

// Create a new application object
IMPLEMENT_APP( WinEDA_App )

/************************************/
/* Called to initialize the program */
/************************************/

bool WinEDA_App::OnInit()
{
    wxFileName         fn;
    wxString           msg;
    WinEDA_CvpcbFrame* frame   = NULL;

    InitEDA_Appl( wxT( "Cvpcb" ), APP_TYPE_CVPCB );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Cvpcb is already running, Continue?" ) ) )
            return false;
    }

    if( argc > 1 )
    {
        fn = argv[1];
        wxSetWorkingDirectory( fn.GetPath() );
    }

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings(reopenLastUsedDirectory);

    g_DrawBgColor = BLACK;

    wxString Title = GetTitle() + wxT( " " ) + GetBuildVersion();
    frame = new WinEDA_CvpcbFrame( Title );

    // Show the frame
    SetTopWindow( frame );

    frame->LoadProjectFile( fn.GetFullPath() );
    frame->Show( TRUE );
    frame->BuildFOOTPRINTS_LISTBOX();

    if( fn.IsOk() && fn.FileExists() )
    {
        frame->m_NetlistFileName = fn;

        if( frame->ReadNetList() )
        {
            frame->m_NetlistFileExtension = fn.GetExt();
            return true;
        }
    }

    LoadFootprintFiles( frame->m_ModuleLibNames, frame->m_footprints );
    frame->m_NetlistFileExtension = wxT( "net" );
    frame->m_NetlistFileName.Clear();
    frame->SetTitle( GetTitle() + wxT( " " ) + GetBuildVersion() +
                     wxGetCwd() + wxFileName::GetPathSeparator() +
                     _( " [no file]" ) );

    return true;
}
