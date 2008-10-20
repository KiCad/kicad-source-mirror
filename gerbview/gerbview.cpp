/************************************************/
/*			GERBVIEW		 main file			*/
/************************************************/

#define MAIN
#define eda_global

#include "fctsys.h"

#include <wx/image.h>
#include <wx/file.h>

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"
#include "trigo.h"
#include "zones.h"
#include "bitmaps.h"
#include "protos.h"


wxString g_Main_Title = wxT( "GerbView" );

IMPLEMENT_APP( WinEDA_App )

bool WinEDA_App::OnInit()
{
    g_EDA_Appl = this;

    InitEDA_Appl( wxT( "gerbview" ) );

    ScreenPcb = new PCB_SCREEN( PCB_FRAME );

    ActiveScreen = ScreenPcb;
    GetSettings();
    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "GerbView is already running. Continue?" ) ) )
            return false;
    }

    g_DrawBgColor = BLACK;

    Read_Hotkey_Config( m_PcbFrame, false );  /* Must be called before creating the main frame
                                               *  in order to display the real hotkeys
                                               *  in menus or tool tips */

    m_GerberFrame = new  WinEDA_GerberFrame( NULL, this, wxT( "GerbView" ),
                                             wxPoint( 0, 0 ), wxSize( 600, 400 ) );

    /* Gerbview mainframe title */
    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    m_GerberFrame->SetTitle( Title );
    m_GerberFrame->m_Pcb = new BOARD( NULL, m_GerberFrame );

    SetTopWindow( m_GerberFrame ); // Set GerbView mainframe on top

    m_GerberFrame->Show( TRUE );              // Show GerbView mainframe
    m_GerberFrame->Zoom_Automatique( TRUE );  // Zoomfit drawing in frame

    if( argc > 1 )
    {
        wxString fileName = MakeFileName( wxEmptyString, argv[1], g_PhotoFilenameExt );

        if( !fileName.IsEmpty() )
        {
            wxClientDC dc( m_GerberFrame->DrawPanel );
            m_GerberFrame->DrawPanel->PrepareGraphicContext( &dc );

            wxString path = wxPathOnly( fileName );

            if( path != wxEmptyString )
                wxSetWorkingDirectory( path );

            Read_Config();

            // Load all files specified on the command line.
            for( int i=1;  i<argc;  ++i )
            {
                fileName = MakeFileName( wxEmptyString, argv[i], g_PhotoFilenameExt );

                if( wxFileExists( fileName ) )
                {
                    ((PCB_SCREEN*)m_GerberFrame->GetScreen())->m_Active_Layer = i-1;
                    m_GerberFrame->LoadOneGerberFile( fileName, &dc, FALSE );
                }
            }
        }
    }
    else
        Read_Config();

    return TRUE;
}
