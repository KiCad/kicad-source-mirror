/*******************/
/* File: cvpcb.cpp */
/*******************/
#define MAIN
#define eda_global

#include "fctsys.h"
#include "appl_wxstruct.h"
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

wxString g_Main_Title = wxT( "CVpcb" );

// Create a new application object
IMPLEMENT_APP( WinEDA_App )

/* fonctions locales */

/************************************/
/* Called to initialize the program */
/************************************/

bool WinEDA_App::OnInit()
{
    wxString           msg;
    wxString           currCWD = wxGetCwd();
    WinEDA_CvpcbFrame* frame   = NULL;

    InitEDA_Appl( wxT( "cvpcb" ) );

    if( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if( !IsOK( NULL, _( "Cvpcb is already running, Continue?" ) ) )
            return false;
    }

    GetSettings();                      // read current setup

    wxSetWorkingDirectory( currCWD );   // mofifie par GetSetting
    SetRealLibraryPath( wxT( "modules" ) );

    if( argc > 1 )
    {
        NetInNameBuffer = argv[1];
        NetNameBuffer   = argv[1];
    }

    if( !NetInNameBuffer.IsEmpty() )
        wxSetWorkingDirectory( wxPathOnly( NetInNameBuffer ) );
    g_DrawBgColor = BLACK;

    Read_Config( NetInNameBuffer );

    wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
    frame = new WinEDA_CvpcbFrame( Title );

    msg.Printf( wxT( "Modules: %d" ), nblib );
    frame->SetStatusText( msg, 2 );

    // Show the frame
    SetTopWindow( frame );

    frame->Show( TRUE );

    listlib();
    frame->BuildFootprintListBox();

    if( !NetInNameBuffer.IsEmpty() )  /* nom de fichier passe a la commande */
    {
        FFileName = MakeFileName( NetDirBuffer,
                                  NetInNameBuffer, NetInExtBuffer );

        frame->ReadNetListe();
    }
    else        /* Mise a jour du titre de la fenetre principale */
    {
        wxString Title = g_Main_Title + wxT( " " ) + GetBuildVersion();
        msg.Printf( wxT( "%s {%s%c} [no file]" ),
                    Title.GetData(), wxGetCwd().GetData(), DIR_SEP );
        frame->SetTitle( msg );
    }

    return TRUE;
}
