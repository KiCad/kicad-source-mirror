/***********************************/
/* eeschema.cpp - module principal */
/***********************************/

#ifdef __GNUG__
#pragma implementation
#endif

#define eda_global
#define MAIN

#include "fctsys.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"
#include "worksheet.h"
#include "trigo.h"
#include "bitmaps.h"
#include "eda_dde.h"

#include "protos.h"

// Global variables
wxString g_Main_Title( wxT( "EESchema" ) );

/************************************/
/* Called to initialize the program */
/************************************/

// Create a new application object: this macro will allow wxWindows to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also declares the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP( WinEDA_App )

bool WinEDA_App::OnInit()
{
    wxString FFileName;
    WinEDA_SchematicFrame* frame = NULL;

    g_DebugLevel = 0;   // Debug level */

    InitEDA_Appl( wxT( "eeschema" ) );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Eeschema is already running, Continue?" ) ) )
            return false;
    }

    if( argc > 1 )
        FFileName = argv[1];

    /* init EESCHEMA */
    GetSettings();                                  // read current setup
    SeedLayers();
    Read_Hotkey_Config( frame, false );   /* Must be called before creating
                                           * the main frame  in order to
                                           * display the real hotkeys in menus
                                           * or tool tips */

    // Create main frame (schematic frame) :
    frame = new WinEDA_SchematicFrame( NULL, wxT( "EESchema" ),
                                       wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    SetTopWindow( frame );
    frame->Show( TRUE );

    if( CreateServer( frame, KICAD_SCH_PORT_SERVICE_NUMBER ) )
    {
        // RemoteCommand is in controle.cpp and is called when PCBNEW
        // sends EESCHEMA a command
        SetupServerFunction( RemoteCommand );
    }

    ActiveScreen = frame->GetScreen();
    frame->Zoom_Automatique( TRUE );

    /* Load file specified in the command line. */
    if( !FFileName.IsEmpty() )
    {
        ChangeFileNameExt( FFileName, g_SchExtBuffer );
        wxSetWorkingDirectory( wxPathOnly( FFileName ) );
        if( frame->DrawPanel )
            if( frame->LoadOneEEProject( FFileName, FALSE ) <= 0 )
                frame->DrawPanel->Refresh( TRUE ); // File not found or error
    }
    else
    {
        Read_Config( wxEmptyString, TRUE ); // Read a default config file if no file to load
        if( frame->DrawPanel )
            frame->DrawPanel->Refresh( TRUE );
    }

    return TRUE;
}
