/************************************************/
/*			GERBVIEW		 main file			*/
/************************************************/

#define MAIN
#define eda_global

#include "fctsys.h"
#include "common.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"

#include "gerbview.h"
#include "pcbplot.h"
#include "bitmaps.h"
#include "protos.h"
#include "zones.h"

#include <wx/file.h>
#include <wx/snglinst.h>


wxString g_Main_Title = wxT( "GerbView" );

IMPLEMENT_APP( WinEDA_App )

bool WinEDA_App::OnInit()
{
    WinEDA_GerberFrame* frame = NULL;

    InitEDA_Appl( wxT( "gerbview" ) );

    ScreenPcb = new PCB_SCREEN();
    ScreenPcb->m_CurrentSheetDesc = &g_Sheet_GERBER;

    ActiveScreen = ScreenPcb;
    GetSettings();
    extern PARAM_CFG_BASE* ParamCfgList[];
    wxGetApp().ReadCurrentSetupValues( ParamCfgList );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "GerbView is already running. Continue?" ) ) )
            return false;
    }

    g_DrawBgColor = BLACK;

    Read_Hotkey_Config( frame, false );  /* Must be called before creating the main frame
                                          *  in order to display the real hotkeys
                                          *  in menus or tool tips */

    frame = new  WinEDA_GerberFrame( NULL, wxT( "GerbView" ),
                                     wxPoint( 0, 0 ),
                                     wxSize( 600, 400 ) );

    /* Gerbview mainframe title */
    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    frame->SetTitle( Title );
    frame->SetBoard( new BOARD( NULL, frame ) );

    SetTopWindow( frame );              // Set GerbView mainframe on top
    frame->Show( TRUE );                // Show GerbView mainframe
    frame->Zoom_Automatique( TRUE );    // Zoomfit drawing in frame

    Read_Config();

    if( argc > 1 )
    {
        wxString fileName = MakeFileName( wxEmptyString,
                                          argv[1],
                                          g_PhotoFilenameExt );

        if( !fileName.IsEmpty() )
        {
            wxClientDC dc( frame->DrawPanel );
            frame->DrawPanel->PrepareGraphicContext( &dc );

            wxString   path = wxPathOnly( fileName );

            if( path != wxEmptyString )
                wxSetWorkingDirectory( path );

            // Load all files specified on the command line.
            for( int i = 1;  i<argc;  ++i )
            {
                fileName = MakeFileName( wxEmptyString,
                                         argv[i],
                                         g_PhotoFilenameExt );

                if( wxFileExists( fileName ) )
                {
                    ( (PCB_SCREEN*) frame->GetScreen() )->
                        m_Active_Layer = i - 1;
                    frame->LoadOneGerberFile( fileName, &dc, FALSE );
                }
            }
        }
    }

    return TRUE;
}
