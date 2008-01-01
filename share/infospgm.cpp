		/****************************************************/
		/* Display a generic info about kikac (copyright..) */
		/* Common tp CVPCB, EESCHEMA, PCBNEW and GERBVIEW	*/
		/****************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

#ifdef KICAD_PYTHON
#include  <pyhandler.h>
#endif

// Import:
extern wxString g_Main_Title;

// Local
#ifdef GERBVIEW
static wxString MsgInfos(wxT("** GERBVIEW  (jul 2001 .. 2008) **"));
#else
#ifdef PCBNEW
static wxString MsgInfos(wxT("** PCBNEW  (sept 1992 .. 2008) **"));
#endif
#endif

#ifdef CVPCB
static wxString MsgInfos(wxT("** CVPCB  (sept 1992 .. 2008) **"));
#endif

#ifdef KICAD
static wxString MsgInfos(wxT("** KICAD  (jul 2000 .. 2008) **"));
#endif

#ifdef EESCHEMA
static wxString MsgInfos(wxT("** EESCHEMA  (sept 1994 .. 2008) **"));
#endif

// Routines Locales

/*******************************************/
void Print_Kicad_Infos(wxWindow * frame)
/*******************************************/
{
wxString AboutCaption = wxT("About ");

wxString Msg = MsgInfos;
	Msg << wxT("\n\n") << _("Build Version:") << wxT("\n") ;

	Msg << g_Main_Title << wxT(" ") << GetBuildVersion();
#if wxUSE_UNICODE
	Msg << wxT(" - Unicode version");
#else
	Msg << wxT(" - Ansi version");
#endif

#ifdef KICAD_PYTHON
	Msg << wxT("\n");
	Msg << wxT( "python : " );
	Msg << wxString::FromAscii( PyHandler::GetInstance()->GetVersion() );
#endif

	Msg << wxT("\n\n") << _("Author:");
	Msg << wxT(" JP CHARRAS\n\n") << _("Based on wxWidgets ");
	Msg << wxMAJOR_VERSION << wxT(".") <<
		wxMINOR_VERSION << wxT(".") << wxRELEASE_NUMBER;
	if( wxSUBRELEASE_NUMBER )
		Msg << wxT(".") << wxSUBRELEASE_NUMBER;
	Msg << _("\n\nGPL License");
	Msg << _("\n\nAuthor's sites:\n");
	Msg << wxT("http://iut-tice.ujf-grenoble.fr/kicad/\n");
	Msg << wxT("http://www.gipsa-lab.inpg.fr/realise_au_lis/kicad/");
	Msg << _("\n\nInternational wiki:\n");
	Msg << wxT("http://kicad.sourceforge.net/\n");

	AboutCaption << g_Main_Title << wxT(" ") << GetBuildVersion();

	wxMessageBox(Msg, AboutCaption, wxICON_INFORMATION, frame);
}

