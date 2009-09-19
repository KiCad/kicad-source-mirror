/************************************************/
/*			GERBVIEW		 main file			*/
/************************************************/

#include "fctsys.h"
#include "common.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"

#include "gerbview.h"
#include "wxGerberFrame.h"
#include "pcbplot.h"
#include "bitmaps.h"
#include "protos.h"
#include "zones.h"

#include <wx/file.h>
#include <wx/snglinst.h>


wxString g_PhotoFilenameExt;
wxString g_DrillFilenameExt;
wxString g_PenFilenameExt;

int      g_DCodesColor;
int      g_Default_GERBER_Format;
int      g_Plot_Spot_Mini;        /* Diametre mini de l'ouverture pour */
int      g_DisplayPolygonsModeSketch;

GERBER*  g_GERBER_List[32];


IMPLEMENT_APP( WinEDA_App )


bool WinEDA_App::OnInit()
{
    wxFileName fn;
    WinEDA_GerberFrame* frame = NULL;

    InitEDA_Appl( wxT( "GerbView" ), APP_TYPE_GERBVIEW );

    ScreenPcb = new PCB_SCREEN();
    ScreenPcb->m_CurrentSheetDesc = &g_Sheet_GERBER;

    ActiveScreen = ScreenPcb;

    // read current setup and reopen last directory if no filename to open in command line
    bool reopenLastUsedDirectory = argc == 1;
    GetSettings(reopenLastUsedDirectory);

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
    frame->SetTitle( GetTitle() + wxT( " " ) + GetBuildVersion() );
    frame->SetBoard( new BOARD( NULL, frame ) );

    // Initialize some display options
    DisplayOpt.DisplayPadIsol = false;      // Pad clearance has no meaning here
    DisplayOpt.ShowTrackClearanceMode = 0;  // tracks and vias clearance has no meaning here

    SetTopWindow( frame );              // Set GerbView mainframe on top
    frame->Show( TRUE );                // Show GerbView mainframe
    frame->Zoom_Automatique( TRUE );    // Zoomfit drawing in frame

    Read_Config();

    if( argc <= 1 )
        return true;

    fn = argv[1];
    fn.SetExt( g_PhotoFilenameExt );

    if( fn.IsOk() )
    {
        wxClientDC dc( frame->DrawPanel );
        frame->DrawPanel->PrepareGraphicContext( &dc );

        if( fn.DirExists() )
            wxSetWorkingDirectory( fn.GetPath() );

        // Load all files specified on the command line.
        for( int i = 1; i < argc; ++i )
        {
            fn = wxFileName( argv[i] );
            fn.SetExt( g_PhotoFilenameExt );

            wxLogDebug( wxT( "Opening file <%s> in GerbView." ),
                        fn.GetFullPath().c_str() );

            if( fn.FileExists() )
            {
                ( (PCB_SCREEN*) frame->GetScreen() )->m_Active_Layer = i - 1;
                frame->LoadOneGerberFile( fn.GetFullPath(), &dc, FALSE );
            }
        }
    }

    return true;
}
