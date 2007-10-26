/*************************/
/* PCBNEW: main program  */
/*************************/

#define MAIN
#define eda_global

#include "fctsys.h"

#include <wx/image.h>
#include <wx/file.h>

#include "common.h"
#include "pcbnew.h"
#include "pcbplot.h"
#include "autorout.h"
#include "trigo.h"
#include "cell.h"
#include "worksheet.h"

#include "protos.h"

#include "drag.h"

#include "eda_dde.h"

wxString g_Main_Title( wxT( "PCBNEW" ) );

IMPLEMENT_APP( WinEDA_App )

/****************************/
bool WinEDA_App::OnInit()
/****************************/
{
    wxString FFileName;

    g_EDA_Appl = this;
    InitEDA_Appl( wxT( "pcbnew" ) );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Pcbnew is already running, Continue?" ) ) )
            return false;
    }

    /* Add image handlers for screen hardcopy */
    wxImage::AddHandler( new wxPNGHandler );
    wxImage::AddHandler( new wxJPEGHandler );

    ScreenPcb = new PCB_SCREEN( PCB_FRAME );
    GetSettings();

    if( argc > 1 )
    {
        FFileName = MakeFileName( wxEmptyString, argv[1], PcbExtBuffer );
        wxSetWorkingDirectory( wxPathOnly( FFileName ) );
    }

    Read_Config( FFileName );
    g_DrawBgColor = BLACK;
    Read_Hotkey_Config( m_PcbFrame, false );  /* Must be called before creating the main frame
                                               *  in order to display the real hotkeys
                                               *  in menus or tool tips */


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
    m_PcbFrame = new WinEDA_PcbFrame( NULL, this, wxT( "PcbNew" ),
                                     wxPoint( 0, 0 ), wxSize( 600, 400 ) );
    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    m_PcbFrame->SetTitle( Title );
    ActiveScreen      = ScreenPcb;
    m_PcbFrame->m_Pcb = new BOARD( NULL, m_PcbFrame );

    SetTopWindow( m_PcbFrame );
    m_PcbFrame->Show( TRUE );

    if( CreateServer( m_PcbFrame, KICAD_PCB_PORT_SERVICE_NUMBER ) )
    {
        SetupServerFunction( RemoteCommand );
    }

    m_PcbFrame->Zoom_Automatique( TRUE );

    /* Load file specified in the command line. */
    if( !FFileName.IsEmpty() )
    {
        wxClientDC dc( m_PcbFrame->DrawPanel );

        m_PcbFrame->DrawPanel->PrepareGraphicContext( &dc );
        m_PcbFrame->LoadOnePcbFile( FFileName, &dc, FALSE );
    }

    return TRUE;
}
