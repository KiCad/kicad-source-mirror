/*************************/
/* PCBNEW: main program  */
/*************************/

#define MAIN
#define eda_global

#include "fctsys.h"

#include <wx/file.h>

#include "common.h"
#include "pcbnew.h"
#include "plot_common.h"
#include "pcbplot.h"
#include "autorout.h"
#include "trigo.h"
#include "cell.h"
#include "worksheet.h"
#include "zones.h"

#include "protos.h"

#include "drag.h"

#include "eda_dde.h"

wxString g_Main_Title( wxT( "PCBnew" ) );

IMPLEMENT_APP( WinEDA_App )

/****************************/
bool WinEDA_App::OnInit()
/****************************/
{
    wxString         FFileName;
    WinEDA_PcbFrame* frame = NULL;

    InitEDA_Appl( wxT( "pcbnew" ) );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Pcbnew is already running, Continue?" ) ) )
            return false;
    }

    ScreenPcb = new PCB_SCREEN();
    GetSettings();

    if( argc > 1 )
    {
        FFileName = MakeFileName( wxEmptyString, argv[1], PcbExtBuffer );
        wxSetWorkingDirectory( wxPathOnly( FFileName ) );
    }

    Read_Config( FFileName );
    g_DrawBgColor = BLACK;
    Read_Hotkey_Config( frame, false );  /* Must be called before creating the
                                          * main frame in order to display the
                                          * real hotkeys in menus or tool tips */


    /* allocation de la memoire pour le fichier et autres buffers: */
    /* On reserve BUFMEMSIZE octets de ram pour calcul */
    buf_work  = adr_lowmem = (char*) MyZMalloc( BUFMEMSIZE );   /* adresse de la zone de calcul */
    adr_himem = adr_lowmem + BUFMEMSIZE;                        /* adr limite haute */
    adr_max   = adr_lowmem;

    if( adr_lowmem == NULL )
    {
        printf( "No Memory, Fatal err Memory alloc\n" );
        return FALSE;
    }
    frame = new WinEDA_PcbFrame( NULL, wxT( "PcbNew" ),
                                 wxPoint( 0, 0 ), wxSize( 600, 400 ) );
    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    frame->SetTitle( Title );
    ActiveScreen = ScreenPcb;

    SetTopWindow( frame );
    frame->Show( TRUE );

    if( CreateServer( frame, KICAD_PCB_PORT_SERVICE_NUMBER ) )
    {
        SetupServerFunction( RemoteCommand );
    }

    frame->Zoom_Automatique( TRUE );

    /* Load file specified in the command line. */
    if( !FFileName.IsEmpty() )
    {
        frame->LoadOnePcbFile( FFileName, FALSE );

        // update the layer names in the listbox
        frame->ReCreateLayerBox( NULL );
    }

    return TRUE;
}
