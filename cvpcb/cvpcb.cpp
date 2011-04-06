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
#include "cvpcb_mainframe.h"
#include "colors_selection.h"
#include "cvpcb_id.h"

#include "build_version.h"

#include <wx/snglinst.h>

// Colors for layers and items
COLORS_DESIGN_SETTINGS g_ColorsSettings;

/* Constant string definitions for CvPcb */
const wxString ComponentFileExtension( wxT( "cmp" ) );
const wxString RetroFileExtension( wxT( "stf" ) );
const wxString FootprintAliasFileExtension( wxT( "equ" ) );

// Wildcard for schematic retroannotation (import footprint names in schematic):
const wxString RetroFileWildcard( _( "Kicad retroannotation files (*.stf)|*.stf" ) );
const wxString FootprintAliasFileWildcard( _( "Kicad footprint alias files (*.equ)|*.equ" ) );

const wxString titleLibLoadError( _( "Library Load Error" ) );


/*
 * MacOSX: Needed for file association
 * http://wiki.wxwidgets.org/WxMac-specific_topics
 */
void WinEDA_App::MacOpenFile(const wxString &fileName)
{
    wxFileName    filename = fileName;
    wxString      oldPath;
    CVPCB_MAINFRAME * frame = ((CVPCB_MAINFRAME*)GetTopWindow());

    if(!filename.FileExists())
	return;

    if( frame->m_NetlistFileName.DirExists() )
        oldPath = frame->m_NetlistFileName.GetPath();

    /* Update the library search path list. */
    if( wxGetApp().GetLibraryPathList().Index( oldPath ) != wxNOT_FOUND )
        wxGetApp().GetLibraryPathList().Remove( oldPath );
    wxGetApp().GetLibraryPathList().Insert( filename.GetPath(), 0 );

    frame->m_NetlistFileName = filename;

    if( frame->ReadNetList() )
    {
        frame->UpdateFileHistory( filename.GetFullPath() );

        frame->SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
                  wxT( " " ) + filename.GetFullPath() );
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
    wxFileName         filename;
    wxString           message;
    CVPCB_MAINFRAME* frame   = NULL;

    InitEDA_Appl( wxT( "CvPCB" ), APP_TYPE_CVPCB );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Cvpcb is already running, Continue?" ) ) )
            return false;
    }

    if( argc > 1 )
    {
        filename = argv[1];
        wxSetWorkingDirectory( filename.GetPath() );
    }

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings(reopenLastUsedDirectory);

    g_DrawBgColor = BLACK;

    wxString Title = GetTitle() + wxT( " " ) + GetBuildVersion();
    frame = new CVPCB_MAINFRAME( Title );

    // Show the frame
    SetTopWindow( frame );

    frame->LoadProjectFile( filename.GetFullPath() );
    frame->Show( TRUE );
    frame->BuildFOOTPRINTS_LISTBOX();

    if( filename.IsOk() && filename.FileExists() )
    {
        frame->m_NetlistFileName = filename;

        if( frame->ReadNetList() )
        {
            frame->m_NetlistFileExtension = filename.GetExt();
            return true;
        }
    }

    frame->LoadFootprintFiles();
    frame->m_NetlistFileExtension = wxT( "net" );
    frame->m_NetlistFileName.Clear();
    frame->SetTitle( GetTitle() + wxT( " " ) + GetBuildVersion() +
                     wxGetCwd() + wxFileName::GetPathSeparator() +
                     _( " [no file]" ) );

    return true;
}
