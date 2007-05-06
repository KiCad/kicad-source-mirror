		/********************************/
		/* kicad.cpp - module principal */
		/********************************/

#ifdef __GNUG__
#pragma implementation
#endif

#define MAIN
#define eda_global

#include "fctsys.h"

#include <wx/image.h>

//#define SPLASH_OK

#ifdef SPLASH_OK
#include <wx/splash.h>
#endif

#include "wxstruct.h"
#include "common.h"
#include "bitmaps.h"
#include "kicad.h"
#include "macros.h"

/* Routines exportees */

/* fonctions importees */
char *GetFileName(char *FullPathName);
void ShowLogo(char * FonteFileName);

/* Routines locales */

	/************************************/
	/* Called to initialize the program */
	/************************************/

// Create a new application object
IMPLEMENT_APP(WinEDA_App)

bool WinEDA_App::OnInit(void)
{
	EDA_Appl = this;
	InitEDA_Appl( wxT("kicad"));
	
	/* init kicad */
	GetSettings();					// read current setup

	m_MainFrame = new WinEDA_MainFrame(this, NULL, wxT("KiCad"),
				 wxPoint(0,0), wxSize(600,400) );
	if(argc > 1 ) m_MainFrame->m_PrjFileName = argv[1];
	else if ( m_EDA_Config )
	{
		m_MainFrame->m_PrjFileName = m_EDA_Config->Read(wxT("LastProject"),
				wxT("noname.pro") );
	}
	else m_MainFrame->m_PrjFileName = wxT("noname.pro");

	wxString Title = g_Main_Title + wxT(" ") + GetBuildVersion();
	Title += wxT(" ") + m_MainFrame->m_PrjFileName;
	m_MainFrame->SetTitle(Title);
	m_MainFrame->ReCreateMenuBar();
	m_MainFrame->RecreateBaseHToolbar();

	m_MainFrame->m_LeftWin->ReCreateTreePrj();
	SetTopWindow(m_MainFrame);
	m_MainFrame->Show(TRUE);

	/* Preparation Affichage du logo */
#ifdef SPLASH_OK
wxString logoname( wxString(m_BinDir) + wxT("logokicad.png") );
wxBitmap image;
	if ( image.LoadFile( logoname, wxBITMAP_TYPE_PNG ) )
		{
		wxSplashScreen * logoscreen = new wxSplashScreen( image,
				wxSPLASH_CENTRE_ON_PARENT | wxSPLASH_TIMEOUT,
				500, m_MainFrame, -1,
				wxDefaultPosition, wxDefaultSize,
				wxSIMPLE_BORDER | wxSTAY_ON_TOP);
		}
#endif

	if ( wxFileExists(m_MainFrame->m_PrjFileName) )
	{
		m_MainFrame->Load_Prj_Config();
	}

	return TRUE;
}

