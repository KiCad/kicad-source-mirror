/***********************************/
/* eeschema.cpp - module principal */
/***********************************/

#ifdef __GNUG__
#pragma implementation
#endif

#define eda_global
#define MAIN

#include "fctsys.h"

#include <wx/image.h>

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "netlist.h"
#include "worksheet.h"
#include "trigo.h"
#include "protos.h"
#include "bitmaps.h"
#include "eda_dde.h"


/* Routines locales */
static void CreateScreens();

// Global variables
wxString    g_Main_Title( wxT( "EESchema" ) );

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

    EDA_Appl = this;

    g_DebugLevel = 0;   // Debug level */

    InitEDA_Appl( wxT( "eeschema" ) );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Eeschema is already running, Continue?" ) ) )
            return false;
    }

    if( argc > 1 )
        FFileName = argv[1];

    CreateScreens();

    /* init EESCHEMA */
    GetSettings();                                  // read current setup
    SeedLayers();
    Read_Hotkey_Config( SchematicFrame, false );    /* Must be called before creating the main frame
                                                     *  in order to display the real hotkeys in menus
                                                     *  or tool tips */

    // Create main frame (schematic frame) :
    SchematicFrame = new WinEDA_SchematicFrame( NULL, this,
                                               wxT( "EESchema" ),
                                               wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    SetTopWindow( SchematicFrame );
    SchematicFrame->Show( TRUE );

    if( CreateServer( SchematicFrame, KICAD_SCH_PORT_SERVICE_NUMBER ) )
    {
        // RemoteCommand is in controle.cpp and is called when PCBNEW
        // sends EESCHEMA a command
        SetupServerFunction( RemoteCommand );
    }

    SchematicFrame->Zoom_Automatique( TRUE );

    /* Load file specified in the command line. */
    if( !FFileName.IsEmpty() )
    {
        ChangeFileNameExt( FFileName, g_SchExtBuffer );
        wxSetWorkingDirectory( wxPathOnly( FFileName ) );
        if( SchematicFrame->DrawPanel )
            if( SchematicFrame->LoadOneEEProject( FFileName, FALSE ) <= 0 )
                SchematicFrame->DrawPanel->Refresh( TRUE ); // File not found or error
    }
    else
    {
        Read_Config( wxEmptyString, TRUE ); // Read config file ici si pas de fichier a charger
        if( SchematicFrame->DrawPanel )
            SchematicFrame->DrawPanel->Refresh( TRUE );
    }

    return TRUE;
}


/******************************/
static void CreateScreens()
/******************************/

/*
 *  Fonction d'init des écrans utilisés dans EESchema:
 */
{
    /* creation des ecrans Sch , Lib  */

    if( ScreenSch == NULL )
        ScreenSch = new SCH_SCREEN( SCHEMATIC_FRAME );
    ScreenSch->m_FileName = g_DefaultSchematicFileName;
    ScreenSch->m_Date = GenDate();
    ActiveScreen = ScreenSch;

    if( ScreenLib == NULL )
        ScreenLib = new SCH_SCREEN( LIBEDITOR_FRAME );
    ScreenLib->SetZoom( 4 );
    ScreenLib->m_UndoRedoCountMax = 10;
}
