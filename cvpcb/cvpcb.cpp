/*******************/
/* File: cvpcb.cpp */
/*******************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "id.h"

#include "cvpcb.h"
#include "zones.h"
#include "bitmaps.h"
#include "protos.h"
#include "cvstruct.h"

#include <wx/snglinst.h>


/* Constant string definitions for CvPcb */
const wxString ComponentFileExtension( wxT( "cmp" ) );
const wxString RetroFileExtension( wxT( "stf" ) );
const wxString EquivFileExtension( wxT( "equ" ) );

/* TODO: What is a stuff file???  Please fix this wild card in English if
 *       you know. */
const wxString RetroFileWildcard( _( "Kicad component list files (*.stf)|*.stf" ) );
const wxString EquivFileWildcard( _( "Kicad footprint alias files (*.equ)|*.equ" ) );

const wxString titleLibLoadError( _( "Library Load Error" ) );


/* Global variables used in CVPcb. */
int g_FlagEESchema;
int Rjustify;
int modified;
int nbcomp;
int nblib;
int composants_non_affectes;

STOREMOD* g_BaseListePkg = NULL;
STORECMP* g_BaseListeCmp = NULL;

wxString g_UserNetDirBuffer;  // Netlist path (void = current working directory)
wxString g_NetlistFileExtension;

wxArrayString g_ListName_Equ; // list of .equ files to load


// Create a new application object
IMPLEMENT_APP( WinEDA_App )

/* fonctions locales */

/************************************/
/* Called to initialize the program */
/************************************/

bool WinEDA_App::OnInit()
{
    wxFileName         fn;
    wxString           msg;
    wxString           currCWD = wxGetCwd();
    WinEDA_CvpcbFrame* frame   = NULL;

    InitEDA_Appl( wxT( "CVpcb" ), APP_TYPE_CVPCB );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Cvpcb is already running, Continue?" ) ) )
            return false;
    }

    GetSettings();                      // read current setup

    wxSetWorkingDirectory( currCWD );   // mofifie par GetSetting

    if( argc > 1 )
    {
        wxLogDebug( wxT( "CvPcb opening file <%s>" ), argv[1] );
        fn = argv[1];
        wxSetWorkingDirectory( fn.GetPath() );
    }

    g_DrawBgColor = BLACK;

    wxString Title = GetTitle() + wxT( " " ) + GetBuildVersion();
    frame = new WinEDA_CvpcbFrame( Title );

    msg.Printf( wxT( "Modules: %d" ), nblib );
    frame->SetStatusText( msg, 2 );

    // Show the frame
    SetTopWindow( frame );

    Read_Config( fn.GetFullPath() );

    frame->Show( TRUE );
    frame->BuildFootprintListBox();

    if( fn.IsOk() && fn.FileExists() )
    {
        frame->m_NetlistFileName = fn;

        if( frame->ReadNetList() )
        {
            g_NetlistFileExtension = fn.GetExt();
            return true;
        }
    }

    listlib();
    g_NetlistFileExtension = wxT( "net" );
    frame->m_NetlistFileName.Clear();
    frame->SetTitle( GetTitle() + wxT( " " ) + GetBuildVersion() +
                     wxGetCwd() + wxFileName::GetPathSeparator() +
                     _( " [no file]" ) );

    return true;
}
