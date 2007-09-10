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
#include "bitmaps.h"
#include "protos.h"


wxString g_Main_Title = wxT("GerbView");

IMPLEMENT_APP(WinEDA_App)

bool WinEDA_App::OnInit(void)
{
wxString FFileName;

	EDA_Appl = this;
	InitEDA_Appl( wxT("gerbview") );

	if(argc > 1) FFileName = MakeFileName(wxEmptyString, argv[1], g_PhotoFilenameExt);

	ScreenPcb = new PCB_SCREEN(PCB_FRAME);
	ActiveScreen = ScreenPcb;
	GetSettings();
    if ( m_Checker && m_Checker->IsAnotherRunning() )
    {
        if ( ! IsOK(NULL, _("GerbView is already running. Continue?") ) )
			return false;
    }


	g_DrawBgColor = BLACK;

    Read_Hotkey_Config( m_PcbFrame, false );  /* Must be called before creating the main frame
                                               *  in order to display the real hotkeys
                                               *  in menus or tool tips */

	m_GerberFrame = new WinEDA_GerberFrame(NULL, this, wxT("GerbView"),
				 wxPoint(0,0), wxSize(600,400) );
	wxString Title = g_Main_Title + wxT(" ") + GetBuildVersion();
	m_GerberFrame->SetTitle(Title);
	m_GerberFrame->m_Pcb = new BOARD(NULL, m_GerberFrame);

	SetTopWindow(m_GerberFrame);
	m_GerberFrame->Show(TRUE);

	m_GerberFrame->m_Pcb = new BOARD(NULL, m_GerberFrame);
	m_GerberFrame->Zoom_Automatique(TRUE);

	/* Load file specified in the command line. */
	if( ! FFileName.IsEmpty() )
	{
		wxString path = wxPathOnly(FFileName);
		wxSetWorkingDirectory(path);
		Read_Config();
		if ( wxFileExists(FFileName) )
		{
			wxClientDC dc(m_GerberFrame->DrawPanel);
			m_GerberFrame->DrawPanel->PrepareGraphicContext(&dc);
			m_GerberFrame->LoadOneGerberFile(FFileName, &dc, FALSE);
		}
	}
	else Read_Config();

	return TRUE;
}


